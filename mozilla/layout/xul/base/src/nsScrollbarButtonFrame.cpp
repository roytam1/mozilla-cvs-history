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

//
// NS_NewToolbarFrame
//
// Creates a new Toolbar frame and returns it in |aNewFrame|
//
nsresult
NS_NewScrollbarButtonFrame ( nsIFrame** aNewFrame )
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsScrollbarButtonFrame* it = new nsScrollbarButtonFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewScrollBarButtonFrame

/*
nsScrollbarButtonFrame::nsScrollbarButtonFrame()
{
}
*/

NS_IMETHODIMP
nsScrollbarButtonFrame::HandleEvent(nsIPresContext& aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus& aEventStatus)
{  
   switch (aEvent->message) {
    case NS_MOUSE_LEFT_CLICK:
       MouseClicked(aPresContext);
    break;
   }

  return nsTitledButtonFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

void
nsScrollbarButtonFrame::MouseClicked(nsIPresContext& aPresContext) 
{
   // when we are clicked either increment or decrement the slider position.

   // get the scrollbar control
   nsIFrame* scrollbar;
   GetParentWithTag(nsXULAtoms::scrollbar, this, scrollbar);

   if (scrollbar == nsnull)
       return;

   // get the scrollbars content node
   nsCOMPtr<nsIContent> content;
   scrollbar->GetContent(getter_AddRefs(content));

   // get the current pos
   PRInt32 curpos = nsSliderFrame::GetCurrentPosition(content);

   // get the max pos
   PRInt32 maxpos = nsSliderFrame::GetMaxPosition(content);

   // get the increment amount
   PRInt32 increment = nsSliderFrame::GetIncrement(content);

   nsString value;
   if (NS_CONTENT_ATTR_HAS_VALUE == mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::kClass, value))
   {
     // if our class is DecrementButton subtract the current pos by increment amount
     // if our class is IncrementButton increment the current pos by the decrement amount
     if (value.Equals("decrement"))
         curpos -= increment;
     else if (value.Equals("increment"))
         curpos += increment;

      // make sure the current positon is between the current and max positions
    if (curpos < 0)
       curpos = 0;
    else if (curpos > maxpos)
       curpos = maxpos;

    // set the current position of the slider.
    char v[100];
    sprintf(v, "%d", curpos);

    content->SetAttribute(kNameSpaceID_None, nsXULAtoms::curpos, v, PR_TRUE);
   
   }


}

nsresult
nsScrollbarButtonFrame::GetChildWithTag(nsIAtom* atom, nsIFrame* start, nsIFrame*& result)
{
  // recursively search our children
  nsIFrame* childFrame;
  start->FirstChild(nsnull, &childFrame); 
  while (nsnull != childFrame) 
  {    
    // get the content node
    nsCOMPtr<nsIContent> child;  
    childFrame->GetContent(getter_AddRefs(child));

    if (child) {
      // see if it is the child
       nsIAtom* tag = nsnull;
       child->GetTag(tag);
       if (tag == atom)
       {
         result = childFrame;
         return NS_OK;
       }
    }

     // recursive search the child
     GetChildWithTag(atom, childFrame, result);
     if (result != nsnull) 
       return NS_OK;

    nsresult rv = childFrame->GetNextSibling(&childFrame);
    NS_ASSERTION(rv == NS_OK,"failed to get next child");
  }

  result = nsnull;
  return NS_OK;
}

nsresult
nsScrollbarButtonFrame::GetParentWithTag(nsIAtom* toFind, nsIFrame* start, nsIFrame*& result)
{
   while(nsnull != start)
   {
      start->GetParent(&start);

      if (start) {
        // get the content node
        nsCOMPtr<nsIContent> child;  
        start->GetContent(getter_AddRefs(child));

        nsIAtom* atom;
        if (child && child->GetTag(atom) == NS_OK && atom == toFind) {
           result = start;
           return NS_OK;
        }
      }
   }

   result = nsnull;
   return NS_OK;
}

