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

//
// Eric Vaughan
// Netscape Communications
//
// See documentation in associated header file
//

#include "nsGrippyFrame.h"
#include "nsScrollbarButtonFrame.h"
#include "nsIStyleContext.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsHTMLIIDs.h"
#include "nsUnitConversion.h"
#include "nsINameSpaceManager.h"
#include "nsHTMLAtoms.h"
#include "nsXULAtoms.h"
#include "nsIReflowCommand.h"
#include "nsSliderFrame.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsDocument.h"

//
// NS_NewToolbarFrame
//
// Creates a new Toolbar frame and returns it in |aNewFrame|
//
nsresult
NS_NewGrippyFrame ( nsIFrame** aNewFrame )
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsGrippyFrame* it = new nsGrippyFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewGrippyFrame

nsGrippyFrame::nsGrippyFrame():mCollapsed(PR_FALSE)
{
}

void
nsGrippyFrame::MouseClicked(nsIPresContext& aPresContext) 
{

  nsString style;

  if (mCollapsed) {
    style = mCollapsedChildStyle;
  } else {
    // when clicked see if we are in a splitter. 
    nsIFrame* splitter;
    nsScrollbarButtonFrame::GetParentWithTag(nsXULAtoms::splitter, this, splitter);

    if (splitter == nsnull)
       return;

    // get the splitters content node
    nsCOMPtr<nsIContent> content;
    splitter->GetContent(getter_AddRefs(content));

    // get the collapse attribute. If the attribute is not set collapse
    // the child before otherwise collapse the child after
    PRBool before = PR_TRUE;
    nsString value;
    if (NS_CONTENT_ATTR_HAS_VALUE == content->GetAttribute(kNameSpaceID_None, nsXULAtoms::collapse, value))
    {
     if (value=="after")
       before = PR_FALSE;
    }

    // find the child just in the box just before the splitter. If we are not currently collapsed then
    // then get the childs style attribute and store it. Then set the child style attribute to be display none.
    // if we are already collapsed then set the child's style back to our stored value.
    nsIFrame* child = GetChildBeforeAfter(splitter,before);
    if (child == nsnull)
      return;

    child->GetContent(getter_AddRefs(mCollapsedChild));

    style = "visibility: collapse";
    mCollapsedChildStyle = "";
    mCollapsedChild->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::style, mCollapsedChildStyle);
  }

  mCollapsedChild->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::style, style, PR_TRUE);

  mCollapsed = !mCollapsed;
}

nsIFrame*
nsGrippyFrame::GetChildBeforeAfter(nsIFrame* start, PRBool before)
{
   nsIFrame* parent = nsnull;
   start->GetParent(&parent);
   PRInt32 index = IndexOf(parent,start);
   PRInt32 count = CountFrames(parent);

   if (index == -1) 
     return nsnull;

   if (before) {
     if (index == 0) {
         return nsnull;
     }

     return GetChildAt(parent, index-1);
   }


   if (index == count-1)
       return nsnull;

   return GetChildAt(parent, index+1);

}

PRInt32
nsGrippyFrame::IndexOf(nsIFrame* parent, nsIFrame* child)
{
  PRInt32 count = 0;

  nsIFrame* childFrame;
  parent->FirstChild(nsnull, &childFrame); 
  while (nsnull != childFrame) 
  {    
    if (childFrame == child)
       return count;

    nsresult rv = childFrame->GetNextSibling(&childFrame);
    NS_ASSERTION(rv == NS_OK,"failed to get next child");
    count++;
  }

  return -1;
}

PRInt32
nsGrippyFrame::CountFrames(nsIFrame* aFrame)
{
  PRInt32 count = 0;

  nsIFrame* childFrame;
  aFrame->FirstChild(nsnull, &childFrame); 
  while (nsnull != childFrame) 
  {    
    nsresult rv = childFrame->GetNextSibling(&childFrame);
    NS_ASSERTION(rv == NS_OK,"failed to get next child");
    count++;
  }

  return count;
}

nsIFrame*
nsGrippyFrame::GetChildAt(nsIFrame* parent, PRInt32 index)
{
  PRInt32 count = 0;

  nsIFrame* childFrame;
  parent->FirstChild(nsnull, &childFrame); 
  while (nsnull != childFrame) 
  {    
    if (count == index)
       return childFrame;

    nsresult rv = childFrame->GetNextSibling(&childFrame);
    NS_ASSERTION(rv == NS_OK,"failed to get next child");
    count++;
  }

  return nsnull;
}

