/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#include "nsFrameList.h"

void
nsFrameList::DeleteFrames(nsIPresContext& aPresContext)
{
  nsIFrame* frame = mFirstChild;
  while (nsnull != frame) {
    nsIFrame* next;
    frame->GetNextSibling(next);
    frame->DeleteFrame(aPresContext);
    delete frame;
    frame = next;
  }
}

void
nsFrameList::AppendFrames(nsIFrame* aParent, nsIFrame* aFrameList)
{
  NS_PRECONDITION(nsnull != aFrameList, "null ptr");
  if (nsnull != aFrameList) {
    nsIFrame* lastChild = LastChild();
    if (nsnull == lastChild) {
      mFirstChild = aFrameList;
    }
    else {
      lastChild->SetNextSibling(aFrameList);
    }
    if (nsnull != aParent) {
      nsIFrame* frame = aFrameList;
      while (nsnull != frame) {
        frame->SetParent(aParent);
        frame->GetNextSibling(frame);
      }
    }
  }
}

void
nsFrameList::AppendFrame(nsIFrame* aParent, nsIFrame* aFrame)
{
  NS_PRECONDITION(nsnull != aFrame, "null ptr");
  if (nsnull != aFrame) {
    nsIFrame* lastChild = LastChild();
    if (nsnull == lastChild) {
      mFirstChild = aFrame;
    }
    else {
      lastChild->SetNextSibling(aFrame);
    }
    if (nsnull != aParent) {
      aFrame->SetParent(aParent);
    }
  }
}

PRBool
nsFrameList::RemoveFrame(nsIFrame* aFrame)
{
  NS_PRECONDITION(nsnull != aFrame, "null ptr");
  if (nsnull != aFrame) {
    nsIFrame* nextFrame;
    aFrame->GetNextSibling(nextFrame);
    if (aFrame == mFirstChild) {
      mFirstChild = nextFrame;
      return PR_TRUE;
    }
    else {
      nsIFrame* prevSibling = GetPrevSiblingFor(aFrame);
      if (nsnull != prevSibling) {
        prevSibling->SetNextSibling(nextFrame);
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

PRBool
nsFrameList::DeleteFrame(nsIPresContext& aPresContext, nsIFrame* aFrame)
{
  NS_PRECONDITION(nsnull != aFrame, "null ptr");
  if (RemoveFrame(aFrame)) {
    aFrame->DeleteFrame(aPresContext);
    delete aFrame;
    return PR_TRUE;
  }
  return PR_FALSE;
}

void
nsFrameList::InsertFrame(nsIFrame* aParent,
                         nsIFrame* aPrevSibling,
                         nsIFrame* aNewFrame)
{
  NS_PRECONDITION(nsnull != aNewFrame, "null ptr");
  if (nsnull != aNewFrame) {
    if (nsnull == aPrevSibling) {
      aNewFrame->SetNextSibling(mFirstChild);
      mFirstChild = aNewFrame;
    }
    else {
      nsIFrame* nextFrame;
      aPrevSibling->GetNextSibling(nextFrame);
      aPrevSibling->SetNextSibling(aNewFrame);
      aNewFrame->SetNextSibling(nextFrame);
    }
    if (nsnull != aParent) {
      aNewFrame->SetParent(aParent);
    }
  }
}

void
nsFrameList::InsertFrames(nsIFrame* aParent,
                          nsIFrame* aPrevSibling,
                          nsIFrame* aFrameList)
{
  NS_PRECONDITION(nsnull != aFrameList, "null ptr");
  if (nsnull != aFrameList) {
    if (nsnull != aParent) {
      nsIFrame* frame = aFrameList;
      while (nsnull != frame) {
        frame->SetParent(aParent);
        frame->GetNextSibling(frame);
      }
    }

    nsFrameList tmp(aFrameList);
    nsIFrame* lastNewFrame = tmp.LastChild();
    if (nsnull == aPrevSibling) {
      lastNewFrame->SetNextSibling(mFirstChild);
      mFirstChild = aFrameList;
    }
    else {
      nsIFrame* nextFrame;
      aPrevSibling->GetNextSibling(nextFrame);
      aPrevSibling->SetNextSibling(aFrameList);
      lastNewFrame->SetNextSibling(nextFrame);
    }
  }
}

PRBool
nsFrameList::ReplaceFrame(nsIFrame* aParent,
                          nsIFrame* aOldFrame,
                          nsIFrame* aNewFrame)
{
  NS_PRECONDITION(nsnull != aOldFrame, "null ptr");
  NS_PRECONDITION(nsnull != aNewFrame, "null ptr");
  if ((nsnull != aOldFrame) && (nsnull != aNewFrame)) {
    nsIFrame* nextFrame;
    aOldFrame->GetNextSibling(nextFrame);
    if (aOldFrame == mFirstChild) {
      mFirstChild = aNewFrame;
      aNewFrame->SetNextSibling(nextFrame);
    }
    else {
      nsIFrame* prevSibling = GetPrevSiblingFor(aOldFrame);
      if (nsnull != prevSibling) {
        prevSibling->SetNextSibling(aNewFrame);
        aNewFrame->SetNextSibling(nextFrame);
      }
    }
    if (nsnull != aParent) {
      aNewFrame->SetParent(aParent);
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsFrameList::ReplaceAndDeleteFrame(nsIPresContext& aPresContext,
                                   nsIFrame* aParent,
                                   nsIFrame* aOldFrame,
                                   nsIFrame* aNewFrame)
{
  NS_PRECONDITION(nsnull != aOldFrame, "null ptr");
  NS_PRECONDITION(nsnull != aNewFrame, "null ptr");
  if (ReplaceFrame(aParent, aOldFrame, aNewFrame)) {
    aNewFrame->DeleteFrame(aPresContext);
    delete aNewFrame;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsFrameList::Split(nsIFrame* aAfterFrame, nsIFrame** aNextFrameResult)
{
  NS_PRECONDITION(nsnull != aAfterFrame, "null ptr");
  NS_PRECONDITION(nsnull != aNextFrameResult, "null ptr");
  NS_ASSERTION(aAfterFrame != mFirstChild, "bad split");
  NS_ASSERTION(ContainsFrame(aAfterFrame), "split after unknown frame");

  if ((nsnull != aNextFrameResult) && (nsnull != aAfterFrame) &&
      (aAfterFrame != mFirstChild)) {
    nsIFrame* nextFrame;
    aAfterFrame->GetNextSibling(nextFrame);
    aAfterFrame->SetNextSibling(nsnull);
    *aNextFrameResult = nextFrame;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsFrameList::PullFrame(nsIFrame* aParent,
                       nsFrameList& aFromList,
                       nsIFrame** aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull != aResult) {
    nsIFrame* pulledFrame = aFromList.FirstChild();
    if (nsnull != pulledFrame) {
      aFromList.RemoveFrame(pulledFrame);
      AppendFrame(aParent, pulledFrame);
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

nsIFrame*
nsFrameList::LastChild() const
{
  nsIFrame* frame = mFirstChild;
  while (nsnull != frame) {
    nsIFrame* next;
    frame->GetNextSibling(next);
    if (nsnull == next) {
      break;
    }
    frame = next;
  }
  return frame;
}

PRBool
nsFrameList::ContainsFrame(nsIFrame* aFrame) const
{
  NS_PRECONDITION(nsnull != aFrame, "null ptr");
  nsIFrame* frame = mFirstChild;
  while (nsnull != frame) {
    if (frame == aFrame) {
      return PR_TRUE;
    }
    frame->GetNextSibling(frame);
  }
  return PR_FALSE;
}

PRInt32
nsFrameList::GetLength() const
{
  PRInt32 count = 0;
  nsIFrame* frame = mFirstChild;
  while (nsnull != frame) {
    count++;
    frame->GetNextSibling(frame);
  }
  return count;
}

nsIFrame*
nsFrameList::GetPrevSiblingFor(nsIFrame* aFrame) const
{
  NS_PRECONDITION(nsnull != aFrame, "null ptr");
  if (aFrame == mFirstChild) {
    return nsnull;
  }
  nsIFrame* frame = mFirstChild;
  while (nsnull != frame) {
    nsIFrame* next;
    frame->GetNextSibling(next);
    if (next == aFrame) {
      break;
    }
    frame = next;
  }
  return frame;
}
