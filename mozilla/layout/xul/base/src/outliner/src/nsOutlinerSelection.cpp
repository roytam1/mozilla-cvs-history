/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 */

#include "nsCOMPtr.h"
#include "nsOutlinerSelection.h"
#include "nsIOutlinerBoxObject.h"
#include "nsIOutlinerView.h"
#include "nsString.h"
#include "nsIDOMElement.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"

// A helper class for managing our ranges of selection.
struct nsOutlinerRange
{
  PRInt32 mMin;
  PRInt32 mMax;

  nsOutlinerRange* mNext;
  nsOutlinerRange* mPrev;

  nsOutlinerSelection* mSelection;

  nsOutlinerRange(nsOutlinerSelection* aSel, PRInt32 aSingleVal)
    :mSelection(aSel), mPrev(nsnull), mNext(nsnull), mMin(aSingleVal), mMax(aSingleVal) {};
  nsOutlinerRange(nsOutlinerSelection* aSel, PRInt32 aMin, PRInt32 aMax) 
    :mSelection(aSel), mPrev(nsnull), mNext(nsnull), mMin(aMin), mMax(aMax) {};

  ~nsOutlinerRange() { delete mNext; };

  void Connect(nsOutlinerRange* aPrev = nsnull, nsOutlinerRange* aNext = nsnull) {
    if (aPrev)
      aPrev->mNext = this;
    else
      mSelection->mFirstRange = this;

    if (aNext)
      aNext->mPrev = this;

    mPrev = aPrev;
    mNext = aNext;
  };

  void RemoveRange(PRInt32 aStart, PRInt32 aEnd) {
    // See if this range overlaps.
    if (aStart >= mMin && aStart <= mMax) {
      // We start within this range.
      // Do we also end within this range?
      if (aEnd >= mMin && aEnd <= mMax) {
        // We do.  This range should be split into
        // two new ranges.
        PRInt32 aNewStart = aEnd+1;
        PRInt32 aNewEnd = mMax;
        mMax = aStart-1;
        nsOutlinerRange* range = new nsOutlinerRange(mSelection, aNewStart, aNewEnd);
        range->Connect(this, mNext);
        return; // We're done, since we were entirely contained within this range.
      }
      else {
        // The end goes outside our range.  We should
        // move our max down to before the start.
        mMax = aStart-1;
        if (mNext)
          mNext->RemoveRange(aStart, aEnd);
      }
    }
    else if (aEnd >= mMin && aEnd <= mMax) {
      // The start precedes our range, but the end is contained
      // within us.  Pull the range up past the end.
      mMin = aEnd+1;
      return; // We're done, since we contained the end.
    }
    else {
      // Neither the start nor the end was contained inside us.
      // Do the start and end encompass us instead?
      if (mMin >= aStart && mMax <= aEnd) {
        // They do.  We should simply be excised from the list.
        if (mPrev)
          mPrev->mNext = mNext;
        else
          mSelection->mFirstRange = mNext;
        
        nsOutlinerRange* next = mNext;
        if (next)
          next->mPrev = mPrev;
        mPrev = mNext = nsnull;
        delete this;
        if (next)
          next->RemoveRange(aStart, aEnd);
        return;
      }

      if (mNext)
        mNext->RemoveRange(aStart, aEnd);
    }
  };

  void Remove(PRInt32 aIndex) {
    if (aIndex >= mMin && aIndex <= mMax) {
      // We have found the range that contains us.
      if (mMin == mMax) {
        // Delete the whole range.
        if (mPrev)
          mPrev->mNext = mNext;
        if (mNext)
          mNext->mPrev = mPrev;
        nsOutlinerRange* first = mSelection->mFirstRange;
        if (first == this)
          mSelection->mFirstRange = mNext;
        mNext = mPrev = nsnull;
        delete this;
      }
      else if (aIndex == mMin)
        mMin++;
      else if (aIndex == mMax)
        mMax--;
    }
    else if (mNext)
      mNext->Remove(aIndex);
  };

  void Add(PRInt32 aIndex) {
    if (aIndex < mMin) {
      // We have found a spot to insert.
      if (aIndex + 1 == mMin)
        mMin = aIndex;
      else if (mPrev && mPrev->mMax+1 == aIndex)
        mPrev->mMax = aIndex;
      else {
        // We have to create a new range.
        nsOutlinerRange* newRange = new nsOutlinerRange(mSelection, aIndex);
        newRange->Connect(mPrev, this);
      }
    }
    else if (mNext)
      mNext->Add(aIndex);
    else {
      // Insert on to the end.
      if (mMax+1 == aIndex)
        mMax = aIndex;
      else {
        // We have to create a new range.
        nsOutlinerRange* newRange = new nsOutlinerRange(mSelection, aIndex);
        newRange->Connect(this, nsnull);
      }
    }
  };

  PRBool Contains(PRInt32 aIndex) {
    if (aIndex >= mMin && aIndex <= mMax)
      return PR_TRUE;

    if (mNext)
      return mNext->Contains(aIndex);

    return PR_FALSE;
  };

  PRInt32 Count() {
    PRInt32 total = mMax - mMin + 1;
    if (mNext)
      total += mNext->Count();
    return total;
  };

  void Invalidate() {
    mSelection->mOutliner->InvalidateRange(mMin, mMax);
    if (mNext)
      mNext->Invalidate();
  };

  void RemoveAllBut(PRInt32 aIndex) {
    if (aIndex >= mMin && aIndex <= mMax) {

      // Invalidate everything in this list.
      mSelection->mFirstRange->Invalidate();

      mMin = aIndex;
      mMax = aIndex;
      
      nsOutlinerRange* first = mSelection->mFirstRange;
      if (mPrev)
        mPrev->mNext = mNext;
      if (mNext)
        mNext->mPrev = mPrev;
      mNext = mPrev = nsnull;
      
      if (first != this) {
        delete mSelection->mFirstRange;
        mSelection->mFirstRange = this;
      }
    }
    else if (mNext)
      mNext->RemoveAllBut(aIndex);
  };

  void Insert(nsOutlinerRange* aRange) {
    if (mMin >= aRange->mMax)
      aRange->Connect(mPrev, this);
    else if (mNext)
      mNext->Insert(aRange);
    else 
      aRange->Connect(this, nsnull);
  };
};

nsOutlinerSelection::nsOutlinerSelection(nsIOutlinerBoxObject* aOutliner)
{
  NS_INIT_ISUPPORTS();
  mOutliner = aOutliner;
  mSuppressed = PR_FALSE;
  mFirstRange = nsnull;
  mShiftSelectPivot = -1;
  mCurrentIndex = -1;
}

nsOutlinerSelection::~nsOutlinerSelection()
{
  delete mFirstRange;
}

NS_IMPL_ISUPPORTS2(nsOutlinerSelection, nsIOutlinerSelection, nsISecurityCheckedComponent)

NS_IMETHODIMP nsOutlinerSelection::GetOutliner(nsIOutlinerBoxObject * *aOutliner)
{
  NS_IF_ADDREF(mOutliner);
  *aOutliner = mOutliner;
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::SetOutliner(nsIOutlinerBoxObject * aOutliner)
{
  mOutliner = aOutliner; // WEAK
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::IsSelected(PRInt32 aIndex, PRBool* aResult)
{
  if (mFirstRange)
    *aResult = mFirstRange->Contains(aIndex);
  else
    *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::TimedSelect(PRInt32 aIndex, PRInt32 aMsec)
{
  PRBool suppressSelect = mSuppressed;

  if (aMsec != -1)
    mSuppressed = PR_TRUE;

  nsresult rv = Select(aIndex);
  if (NS_FAILED(rv))
    return rv;

  if (aMsec != -1) {
    mSuppressed = suppressSelect;
    if (!mSuppressed) {
      if (mSelectTimer)
        mSelectTimer->Cancel();

      mSelectTimer = do_CreateInstance("@mozilla.org/timer;1");
      mSelectTimer->Init(SelectCallback, this, aMsec, NS_PRIORITY_HIGH);
    }
  }

  return NS_OK;
}

void
nsOutlinerSelection::SelectCallback(nsITimer *aTimer, void *aClosure)
{
  nsOutlinerSelection* self = NS_STATIC_CAST(nsOutlinerSelection*, aClosure);
  if (self) {
    self->FireOnSelectHandler();
    aTimer->Cancel();
    self->mSelectTimer = nsnull;
  }
}

NS_IMETHODIMP nsOutlinerSelection::Select(PRInt32 aIndex)
{
  mShiftSelectPivot = -1;

  SetCurrentIndex(aIndex);

  if (mFirstRange) {
    PRBool alreadySelected = mFirstRange->Contains(aIndex);

    if (alreadySelected) {
      PRInt32 count = mFirstRange->Count();
      if (count > 1) {
        // We need to deselect everything but our item.
        mFirstRange->RemoveAllBut(aIndex);
        FireOnSelectHandler();
      }
      return NS_OK;
    }
    else {
       // Clear out our selection.
       mFirstRange->Invalidate();
       delete mFirstRange;
    }
  }

  // Create our new selection.
  mFirstRange = new nsOutlinerRange(this, aIndex);
  mFirstRange->Invalidate();

  // Fire the select event
  FireOnSelectHandler();
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::ToggleSelect(PRInt32 aIndex)
{
  // There are six cases that can occur on a ToggleSelect with our
  // range code.
  // (1) A new range should be made for a selection.
  // (2) A single range is removed from the selection.
  // (3) The item is added to an existing range.
  // (4) The item is removed from an existing range.
  // (5) The addition of the item causes two ranges to be merged.
  // (6) The removal of the item causes two ranges to be split.
  mShiftSelectPivot = -1;
  SetCurrentIndex(aIndex);

  if (!mFirstRange)
    Select(aIndex);
  else {
    if (!mFirstRange->Contains(aIndex))
      mFirstRange->Add(aIndex);
    else 
      mFirstRange->Remove(aIndex);

    mOutliner->InvalidateRow(aIndex);

    FireOnSelectHandler();
  }

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::RangedSelect(PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aAugment)
{
  if (!aAugment) {
    // Clear our selection.
    if (mFirstRange) {
        mFirstRange->Invalidate();
        delete mFirstRange;
    }
  }

  if (aStartIndex == -1) {
    if (mShiftSelectPivot != -1)
      aStartIndex = mShiftSelectPivot;
    else aStartIndex = mCurrentIndex;
  }

  mShiftSelectPivot = aStartIndex;
  SetCurrentIndex(aEndIndex);
  
  PRInt32 start = aStartIndex < aEndIndex ? aStartIndex : aEndIndex;
  PRInt32 end = aStartIndex < aEndIndex ? aEndIndex : aStartIndex;

  if (aAugment && mFirstRange) {
    // We need to remove all the items within our selected range from the selection,
    // and then we insert our new range into the list.
    mFirstRange->RemoveRange(start, end);
  }

  nsOutlinerRange* range =  new nsOutlinerRange(this, start, end);
  range->Invalidate();

  if (aAugment && mFirstRange)
    mFirstRange->Insert(range);
  else
    mFirstRange = range;

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::ClearSelection()
{
  if (mFirstRange) {
    mFirstRange->Invalidate();
    delete mFirstRange;
    mFirstRange = nsnull;
  }
  mShiftSelectPivot = -1;

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::InvertSelection()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsOutlinerSelection::SelectAll()
{
  mShiftSelectPivot = -1;

  // Invalidate not necessary when clearing selection, since 
  // we're going to invalidate the world on the SelectAll.
  delete mFirstRange;
  
  nsCOMPtr<nsIOutlinerView> view;
  mOutliner->GetView(getter_AddRefs(view));
  if (!view)
    return NS_OK;

  PRInt32 rowCount;
  view->GetRowCount(&rowCount);

  if (rowCount == 0)
    return NS_OK;

  mFirstRange = new nsOutlinerRange(this, 0, rowCount-1);
  mFirstRange->Invalidate();

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::GetRangeCount(PRInt32* aResult)
{
  PRInt32 count = 0;
  nsOutlinerRange* curr = mFirstRange;
  while (curr) {
    count++;
    curr = curr->mNext;
  }

  *aResult = count;
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::GetRangeAt(PRInt32 aIndex, PRInt32* aMin, PRInt32* aMax)
{
  *aMin = *aMax = -1;
  PRInt32 i = -1;
  nsOutlinerRange* curr = mFirstRange;
  while (curr) {
    i++;
    if (i == aIndex) {
      *aMin = curr->mMin;
      *aMax = curr->mMax;
      break;
    }
    curr = curr->mNext;
  }

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::GetSelectEventsSuppressed(PRBool *aSelectEventsSuppressed)
{
  *aSelectEventsSuppressed = mSuppressed;
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::SetSelectEventsSuppressed(PRBool aSelectEventsSuppressed)
{
  mSuppressed = aSelectEventsSuppressed;
  if (!mSuppressed)
    FireOnSelectHandler();
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::GetCurrentIndex(PRInt32 *aCurrentIndex)
{
  *aCurrentIndex = mCurrentIndex;
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerSelection::SetCurrentIndex(PRInt32 aIndex)
{
  if (mCurrentIndex != -1)
    mOutliner->InvalidateRow(mCurrentIndex);
  
  mCurrentIndex = aIndex;
  
  if (aIndex != -1)
    mOutliner->InvalidateRow(aIndex);

  return NS_OK;
}

NS_IMETHODIMP
nsOutlinerSelection::AdjustSelection(PRInt32 aIndex, PRInt32 aCount)
{
  return NS_OK;
}

nsresult
nsOutlinerSelection::FireOnSelectHandler()
{
  if (mSuppressed)
    return NS_OK;

  nsCOMPtr<nsIDOMElement> elt;
  mOutliner->GetOutlinerBody(getter_AddRefs(elt));

  nsCOMPtr<nsIContent> content(do_QueryInterface(elt));
  nsCOMPtr<nsIDocument> document;
  content->GetDocument(*getter_AddRefs(document));
 
  PRInt32 count = document->GetNumberOfShells();
	for (PRInt32 i = 0; i < count; i++) {
		nsCOMPtr<nsIPresShell> shell = getter_AddRefs(document->GetShellAt(i));
		if (nsnull == shell)
				continue;

		// Retrieve the context in which our DOM event will fire.
		nsCOMPtr<nsIPresContext> aPresContext;
		shell->GetPresContext(getter_AddRefs(aPresContext));
				
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    event.message = NS_FORM_SELECTED;

    content->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
  }

  return NS_OK;
}

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP nsOutlinerSelection::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  nsCAutoString str("AllAccess");
  *_retval = str.ToNewCString();
  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP nsOutlinerSelection::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  nsCAutoString str("AllAccess");
  *_retval = str.ToNewCString();
  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP nsOutlinerSelection::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  nsCAutoString str("AllAccess");
  *_retval = str.ToNewCString();
  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP nsOutlinerSelection::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  nsCAutoString str("AllAccess");
  *_retval = str.ToNewCString();
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////////////

nsresult
NS_NewOutlinerSelection(nsIOutlinerBoxObject* aOutliner, nsIOutlinerSelection** aResult)
{
  *aResult = new nsOutlinerSelection(aOutliner);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}
