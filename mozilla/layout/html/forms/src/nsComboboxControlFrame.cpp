/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nsCOMPtr.h"
#include "nsComboboxControlFrame.h"
#include "nsFormFrame.h"
#include "nsFormControlFrame.h"
#include "nsIHTMLContent.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLParts.h"
#include "nsIFormControl.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMEventReceiver.h"
#include "nsLayoutAtoms.h"
#include "nsIDOMElement.h"
#include "nsListControlFrame.h"
#include "nsIListControlFrame.h"
#include "nsIDOMHTMLCollection.h" 
#include "nsIDOMHTMLSelectElement.h" 
#include "nsIDOMHTMLOptionElement.h" 
#include "nsIPresShell.h"
#include "nsISupportsArray.h"
#include "nsIDeviceContext.h"
#include "nsIView.h"

static NS_DEFINE_IID(kIFormControlFrameIID,      NS_IFORMCONTROLFRAME_IID);
static NS_DEFINE_IID(kIComboboxControlFrameIID,  NS_ICOMBOBOXCONTROLFRAME_IID);
static NS_DEFINE_IID(kIListControlFrameIID,      NS_ILISTCONTROLFRAME_IID);
static NS_DEFINE_IID(kIDOMMouseListenerIID,      NS_IDOMMOUSELISTENER_IID);
static NS_DEFINE_IID(kIFrameIID,                 NS_IFRAME_IID);
static NS_DEFINE_IID(kIAnonymousContentCreatorIID, NS_IANONYMOUS_CONTENT_CREATOR_IID);

// Drop down list event management.
// The combo box uses the following strategy for managing the
// drop-down list.
// If the combo box or it's arrow button is clicked on the drop-down list is displayed
// If mouse exit's the combo box with the drop-down list displayed the drop-down list
// is asked to capture events
// The drop-down list will capture all events including mouse down and up and will always
// return with ListWasSelected method call regardless of whether an item in the list was
// actually selected.
// The ListWasSelected code will turn off mouse-capture for the drop-down list.
// The drop-down list does not explicitly set capture when it is in the drop-down mode.


//XXX: This is temporary. It simulates psuedo states by using a attribute selector on 

const char * kMozDropdownActive = "-moz-dropdown-active";

nsresult
NS_NewComboboxControlFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsComboboxControlFrame* it = new nsComboboxControlFrame;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsComboboxControlFrame::nsComboboxControlFrame()
  : nsAreaFrame() 
{
  mPresContext                 = nsnull;
  mFormFrame                   = nsnull;       
  mListControlFrame            = nsnull;
  mTextStr                     = "";
  mDisplayContent              = nsnull;
  mButtonContent               = nsnull;
  mDroppedDown                 = PR_FALSE;
  mDisplayFrame                = nsnull;
  mButtonFrame                 = nsnull;
  mDropdownFrame               = nsnull;

   //Shrink the area around it's contents
  SetFlags(NS_BLOCK_SHRINK_WRAP);
}

//--------------------------------------------------------------
nsComboboxControlFrame::~nsComboboxControlFrame()
{
  mFormFrame = nsnull;
  NS_IF_RELEASE(mPresContext);
  NS_IF_RELEASE(mDisplayContent);
  NS_IF_RELEASE(mButtonContent);
}

//--------------------------------------------------------------
nsresult
nsComboboxControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIComboboxControlFrameIID)) {
    *aInstancePtr = (void*) ((nsIComboboxControlFrame*) this);
    return NS_OK;
  }
  if (aIID.Equals(kIFormControlFrameIID)) {
    *aInstancePtr = (void*) ((nsIFormControlFrame*) this);
    return NS_OK;
  } else  if (aIID.Equals(kIDOMMouseListenerIID)) {                                         
    *aInstancePtr = (void*)(nsIDOMMouseListener*) this;                                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  } else if (aIID.Equals(kIAnonymousContentCreatorIID)) {                                         
    *aInstancePtr = (void*)(nsIAnonymousContentCreator*) this;                                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;   
  }

  return nsAreaFrame::QueryInterface(aIID, aInstancePtr);
}


NS_IMETHODIMP
nsComboboxControlFrame::Init(nsIPresContext&  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIStyleContext* aContext,
              nsIFrame*        aPrevInFlow)
{
   // Need to hold on the pres context because it is used later in methods
   // which don't have it passed in.
  mPresContext = &aPresContext;
  NS_ADDREF(mPresContext);
  return nsAreaFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
}

//--------------------------------------------------------------
PRBool
nsComboboxControlFrame::IsSuccessful(nsIFormControlFrame* aSubmitter)
{
  nsAutoString name;
  return (NS_CONTENT_ATTR_HAS_VALUE == GetName(&name));
}


// Initialize the text string in the combobox using either the current
// selection in the list box or the first item item in the list box.

void 
nsComboboxControlFrame::InitTextStr(PRBool aUpdate)
{
  nsIFormControlFrame* fcFrame = nsnull;
  nsIFrame* dropdownFrame = GetDropdownFrame();
  nsresult result = dropdownFrame->QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
 
   // Reset the list, so we can ask it for it's selected item.
  fcFrame->Reset();

   // Update the selected text string
  mListControlFrame->GetSelectedItem(mTextStr);

  if (mTextStr == "") {
     // No selection so use the first item in the list box
 
    if ((NS_OK == result) && (nsnull != fcFrame)) {
       // Set listbox selection to first item in the list box
      fcFrame->SetProperty(nsHTMLAtoms::selectedindex, "0");
       // Get the listbox selection as a string
      mListControlFrame->GetSelectedItem(mTextStr);
    }
  }

   // Update the display by setting the value attribute
  mDisplayContent->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::value, mTextStr, aUpdate);
}

//--------------------------------------------------------------

// Reset the combo box back to it original state.

void 
nsComboboxControlFrame::Reset()
{
   // Reset the dropdown list to its original state
  nsIFormControlFrame* fcFrame = nsnull;
  nsIFrame* dropdownFrame = GetDropdownFrame();
  nsresult result = dropdownFrame->QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
  if ((NS_OK == result) && (nsnull != fcFrame)) {
    fcFrame->Reset(); 
  }
 
    // Update the combobox using the text string returned from the dropdown list
  InitTextStr(PR_TRUE);
}

void 
nsComboboxControlFrame::PostCreateWidget(nsIPresContext* aPresContext,
                                       nscoord& aWidth,
                                       nscoord& aHeight)
{
  Reset();
}

//--------------------------------------------------------------
NS_IMETHODIMP 
nsComboboxControlFrame::GetType(PRInt32* aType) const
{
  *aType = NS_FORM_SELECT;
  return NS_OK;
}

//--------------------------------------------------------------
NS_IMETHODIMP
nsComboboxControlFrame::GetFormContent(nsIContent*& aContent) const
{
  nsIContent* content;
  nsresult rv;
  rv = GetContent(&content);
  aContent = content;
  return rv;
}

//--------------------------------------------------------------
NS_IMETHODIMP
nsComboboxControlFrame::GetFont(nsIPresContext*        aPresContext, 
                             nsFont&                aFont)
{
  nsFormControlHelper::GetFont(this, aPresContext, mStyleContext, aFont);
  return NS_OK;
}


//--------------------------------------------------------------
nscoord 
nsComboboxControlFrame::GetVerticalBorderWidth(float aPixToTwip) const
{
   return 0;
}


//--------------------------------------------------------------
nscoord 
nsComboboxControlFrame::GetHorizontalBorderWidth(float aPixToTwip) const
{
  return 0;
}


//--------------------------------------------------------------
nscoord 
nsComboboxControlFrame::GetVerticalInsidePadding(float aPixToTwip, 
                                               nscoord aInnerHeight) const
{
   return 0;
}

//--------------------------------------------------------------
nscoord 
nsComboboxControlFrame::GetHorizontalInsidePadding(nsIPresContext& aPresContext,
                                               float aPixToTwip, 
                                               nscoord aInnerWidth,
                                               nscoord aCharWidth) const
{
  return 0;
}



void 
nsComboboxControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
  //XXX: TODO Implement focus for combobox. 
}

// Toggle dropdown list.

void 
nsComboboxControlFrame::ToggleList(nsIPresContext* aPresContext)
{
  if (PR_TRUE == mDroppedDown) {
    ShowList(aPresContext, PR_FALSE);
  } else {
    ShowList(aPresContext, PR_TRUE);
  }
}


void
nsComboboxControlFrame::ShowPopup(PRBool aShowPopup)
{
  //XXX: This is temporary. It simulates a psuedo dropdown states by using a attribute selector 
  // This will not be need if the event state supports active states for use with the dropdown list
  // Currently, the event state manager will reset the active state to the content which has focus
  // which causes the active state to be removed from the event state manager immediately after it
  // it the select is set to the active state.
  
  nsCOMPtr<nsIAtom> activeAtom ( dont_QueryInterface(NS_NewAtom(kMozDropdownActive)));
  if (PR_TRUE == aShowPopup) {
    mContent->SetAttribute(kNameSpaceID_None, activeAtom, "", PR_TRUE);
  } else {
    mContent->UnsetAttribute(kNameSpaceID_None, activeAtom, PR_TRUE);
  }
}

// Show the dropdown list

void 
nsComboboxControlFrame::ShowList(nsIPresContext* aPresContext, PRBool aShowList)
{
  if (PR_TRUE == aShowList) {
    mListControlFrame->AboutToDropDown();
    ShowPopup(PR_TRUE);
    mDroppedDown = PR_TRUE;
     // The listcontrol frame will call back to the nsComboboxControlFrame's ListWasSelected
     // which will stop the capture.
    mListControlFrame->CaptureMouseEvents(PR_TRUE);
  } else {
    ShowPopup(PR_FALSE);
    mDroppedDown = PR_FALSE;
  }
}


//-------------------------------------------------------------
// this is in response to the MouseClick from the containing browse button
// XXX: TODO still need to get filters from accept attribute
void 
nsComboboxControlFrame::MouseClicked(nsIPresContext* aPresContext)
{
   ToggleList(aPresContext);
}


nsresult
nsComboboxControlFrame::ReflowComboChildFrame(nsIFrame* aFrame, 
                                         nsIPresContext&  aPresContext, 
                                         nsHTMLReflowMetrics&     aDesiredSize,
                                         const nsHTMLReflowState& aReflowState, 
                                         nsReflowStatus&          aStatus,
                                         nscoord                  aAvailableWidth,
                                         nscoord                  aAvailableHeight)
{
   // Constrain the child's width and height to aAvailableWidth and aAvailableHeight
  nsSize availSize(aAvailableWidth, aAvailableHeight);
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, aFrame,
                                   availSize);
  kidReflowState.computedWidth = aAvailableWidth;
  kidReflowState.computedHeight = aAvailableHeight;
      
   // Reflow child
  nsresult rv = ReflowChild(aFrame, aPresContext, aDesiredSize, kidReflowState, aStatus);
 
   // Set the child's width and height to it's desired size
  nsRect rect;
  aFrame->GetRect(rect);
  rect.width = aDesiredSize.width;
  rect.height = aDesiredSize.height;
  aFrame->SetRect(rect);
  return rv;
}

// Suggest a size for the child frame. 
// Only frames which implement the nsIFormControlFrame interface and
// honor the SetSuggestedSize method will be placed and sized correctly.

void 
nsComboboxControlFrame::SetChildFrameSize(nsIFrame* aFrame, nscoord aWidth, nscoord aHeight) 
{
  nsIFormControlFrame* fcFrame = nsnull;
  nsresult result = aFrame->QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
  if (NS_SUCCEEDED(result) && (nsnull != fcFrame)) {
    fcFrame->SetSuggestedSize(aWidth, aHeight); 
  }
}

nsresult 
nsComboboxControlFrame::GetPrimaryComboFrame(nsIPresContext& aPresContext, nsIContent* aContent, nsIFrame** aFrame)
{
  nsresult rv = NS_OK;
   // Get the primary frame from the presentation shell.
  nsCOMPtr<nsIPresShell> presShell;
  rv = aPresContext.GetShell(getter_AddRefs(presShell));
  if (NS_SUCCEEDED(rv) && presShell) {
    presShell->GetPrimaryFrameFor(aContent, aFrame);
  }
  return rv;
}

nsIFrame*
nsComboboxControlFrame::GetButtonFrame(nsIPresContext& aPresContext)
{
  if (mButtonFrame == nsnull) {
    NS_ASSERTION(mButtonContent != nsnull, "nsComboboxControlFrame mButtonContent is null");
    GetPrimaryComboFrame(aPresContext, mButtonContent, &mButtonFrame);
  }

  return mButtonFrame;
}

nsIFrame* 
nsComboboxControlFrame::GetDisplayFrame(nsIPresContext& aPresContext)
{
  if (mDisplayFrame == nsnull) {
    NS_ASSERTION(mDisplayContent != nsnull, "nsComboboxControlFrame mDisplayContent is null");
    GetPrimaryComboFrame(aPresContext, mDisplayContent, &mDisplayFrame);
  }

  return mDisplayFrame;
}

nsIFrame* 
nsComboboxControlFrame::GetDropdownFrame()
{
  return mDropdownFrame;
}


nsresult 
nsComboboxControlFrame::GetScreenHeight(nsIPresContext& aPresContext,
                                        nscoord& aHeight)
{
  aHeight = 0;
  nsIDeviceContext* context;
  aPresContext.GetDeviceContext( &context );
	if ( nsnull != context )
	{
		PRInt32 height;
    PRInt32 width;
		context->GetDeviceSurfaceDimensions(width, height);
		float devUnits;
 		context->GetDevUnitsToAppUnits(devUnits);
		aHeight = NSToIntRound(float( height) / devUnits );
		NS_RELEASE( context );
		return NS_OK;
	}

  return NS_ERROR_FAILURE;
}


nsresult 
nsComboboxControlFrame::PositionDropdown(nsIPresContext& aPresContext, 
                                         nscoord aHeight, 
                                         nsRect aAbsoluteTwipsRect, 
                                         nsRect aAbsolutePixelRect)
{
   // Position the dropdown list. It is positioned below the display frame if there is enough
   // room on the screen to display the entire list. Otherwise it is placed above the display
   // frame.

   // Note: As first glance, it appears that you could simply get the absolute bounding box for the
   // dropdown list by first getting it's view, then getting the view's nsIWidget, then asking the nsIWidget
   // for it's AbsoluteBounds. The problem with this approach, is that the dropdown lists y location can
   // change based on whether the dropdown is placed below or above the display frame.
   // The approach, taken here is to get use the absolute position of the display frame and use it's location
   // to determine if the dropdown will go offscreen.

   // Use the height calculated for the area frame so it includes both
   // the display and button heights.
  nsresult rv = NS_OK;
  nsIFrame* dropdownFrame = GetDropdownFrame();
  nscoord dropdownYOffset = aHeight;
/* XXX: Enable this code to debug popping up above the display frame, rather than below it
  nscoord screenHeightInPixels = 0;
  if (NS_SUCCEEDED(GetScreenHeight(aPresContext, screenHeightInPixels))) {
     nsRect absoluteRect;

       // Get the height of the dropdown list in pixels.
     nsRect dropdownRect;
     dropdownFrame->GetRect(dropdownRect);
     float t2p;
     aPresContext.GetTwipsToPixels(&t2p);
     nscoord absoluteDropDownHeight = NSTwipsToIntPixels(dropdownRect.height, t2p);
    
      // Check to see if the drop-down list will go offscreen
    if (NS_SUCCEEDED(rv) && ((aAbsolutePixelRect.y + aAbsolutePixelRect.height + absoluteDropDownHeight) > screenHeightInPixels)) {
      // move the dropdown list up
      dropdownYOffset = - (aAbsoluteTwipsRect.height);
    }
  } 
*/
 
  nsRect dropdownRect;
  dropdownFrame->GetRect(dropdownRect);
  dropdownRect.x = 0;
  dropdownRect.y = dropdownYOffset; 
  dropdownFrame->SetRect(dropdownRect);

  return rv;
}


// Calculate a frame's position in screen coordinates
nsresult
nsComboboxControlFrame::GetAbsoluteFramePosition(nsIPresContext& aPresContext,
                                                 nsIFrame *aFrame, 
                                                 nsRect& aAbsoluteTwipsRect, 
                                                 nsRect& aAbsolutePixelRect)
{
  //XXX: This code needs to take the view's offset into account when calculating
  //the absolute coordinate of the frame.
  nsresult rv = NS_OK;
 
  aFrame->GetRect(aAbsoluteTwipsRect);
 
    // Get conversions between twips and pixels
  float t2p;
  float p2t;
  aPresContext.GetTwipsToPixels(&t2p);
  aPresContext.GetPixelsToTwips(&p2t);
  
   // Add in frame's offset from it it's containing view
  nsIView *containingView = nsnull;
  nsPoint offset;
  rv = aFrame->GetOffsetFromView(offset, &containingView);
  if (NS_SUCCEEDED(rv) && (nsnull != containingView)) {
    aAbsoluteTwipsRect.x += offset.x;
    aAbsoluteTwipsRect.y += offset.y;
     // Addin the containing view's offset form it's containing widget
    nsIWidget* widget = nsnull;
    nscoord widgetx = 0;
    nscoord widgety = 0;
    rv = containingView->GetOffsetFromWidget(&widgetx, &widgety, widget);
    if (NS_SUCCEEDED(rv) && (nsnull != widget)) {
      aAbsoluteTwipsRect.x += widgetx;
      aAbsoluteTwipsRect.y += widgety;
  
       // Add in the absolute offset of the widget.
      nsRect absBounds;
      widget->GetAbsoluteBounds(absBounds);

      // Convert widget coordinates to twips   
      aAbsoluteTwipsRect.x += NSIntPixelsToTwips(absBounds.x, p2t);
      aAbsoluteTwipsRect.y += NSIntPixelsToTwips(absBounds.y, p2t);   
      NS_RELEASE(widget);
    }
  }

   // convert to pixel coordinates
  if (NS_SUCCEEDED(rv)) {
   aAbsolutePixelRect.x = NSTwipsToIntPixels(aAbsoluteTwipsRect.x, t2p);
   aAbsolutePixelRect.y = NSTwipsToIntPixels(aAbsoluteTwipsRect.y, t2p);
   aAbsolutePixelRect.width = NSTwipsToIntPixels(aAbsoluteTwipsRect.width, t2p);
   aAbsolutePixelRect.height = NSTwipsToIntPixels(aAbsoluteTwipsRect.height, t2p);
  }

  return rv;
}


NS_IMETHODIMP 
nsComboboxControlFrame::Reflow(nsIPresContext&          aPresContext, 
                                             nsHTMLReflowMetrics&     aDesiredSize,
                                             const nsHTMLReflowState& aReflowState, 
                                             nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  nsIFrame* buttonFrame = GetButtonFrame(aPresContext);
  nsIFrame* displayFrame = GetDisplayFrame(aPresContext);
  nsIFrame* dropdownFrame = GetDropdownFrame();

   // Don't try to do any special sizing and positioning unless all of the frames
   // have been created.
  if ((nsnull == displayFrame) ||
     (nsnull == buttonFrame) ||
     (nsnull == dropdownFrame)) 
  {
     // Since combobox frames are missing just do a normal area frame reflow
    return nsAreaFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  // size of each part of the combo box
  nsRect displayRect;
  nsRect buttonRect;
  nsRect dropdownRect;

 
  if (!mFormFrame && (eReflowReason_Initial == aReflowState.reason)) {
    nsFormFrame::AddFormControlFrame(aPresContext, *this);
  }

   // Get the current sizes of the combo box child frames
  displayFrame->GetRect(displayRect);
  buttonFrame->GetRect(buttonRect);
  dropdownFrame->GetRect(dropdownRect);

  nsHTMLReflowState firstPassState(aReflowState);

    // Only reflow the display and button if they are the target of 
    // the incremental reflow, unless they change size. If they do
    // then everything needs to be reflowed.
  if (eReflowReason_Incremental == firstPassState.reason) {
    nsIFrame* targetFrame;
    firstPassState.reflowCommand->GetTarget(targetFrame);
    if ((targetFrame == buttonFrame) || (targetFrame == displayFrame)) {
      nsRect oldDisplayRect = displayRect;
      nsRect oldButtonRect = buttonRect;
      rv = nsAreaFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
      displayFrame->GetRect(displayRect);
      buttonFrame->GetRect(buttonRect);
      if ((oldDisplayRect == displayRect) && (oldButtonRect == buttonRect)) {
        // Reposition the popup.
        nsRect absoluteTwips;
        nsRect absolutePixels;
        GetAbsoluteFramePosition(aPresContext, displayFrame,  absoluteTwips, absolutePixels);
        PositionDropdown(aPresContext, displayRect.height, absoluteTwips, absolutePixels);
        return rv;
      }
    }
    nsIReflowCommand::ReflowType type;
    aReflowState.reflowCommand->GetType(type);
    firstPassState.reason = eReflowReason_StyleChange;
    firstPassState.reflowCommand = nsnull;
  }


  //Set the desired size for the button and display frame
  if (NS_UNCONSTRAINEDSIZE == firstPassState.computedWidth) {
    // A width has not been specified for the select so size the display area to
    // match the width of the longest item in the drop-down list. The dropdown
    // list has already been reflowed and sized to shrink around its contents above.

     // Reflow the dropdown shrink-wrapped.
    nsHTMLReflowMetrics  dropdownDesiredSize(aDesiredSize);
    ReflowComboChildFrame(dropdownFrame, aPresContext, dropdownDesiredSize, firstPassState, aStatus, NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);  
    dropdownFrame->GetRect(dropdownRect);
       // Get maximum size and height of a option in the dropdown

    nsSize size;
    mListControlFrame->GetMaximumSize(size);

     // Set width of display to match width of the drop down 
    SetChildFrameSize(displayFrame, dropdownRect.width, size.height);

    // Size the button to be the same height as the displayFrame
    SetChildFrameSize(buttonFrame, size.height, size.height);

     // Reflow display + button
    nsAreaFrame::Reflow(aPresContext, aDesiredSize, firstPassState, aStatus);
    displayFrame->GetRect(displayRect);

    // Reflow the dropdown list to match the width of the display + button
    ReflowComboChildFrame(dropdownFrame, aPresContext, dropdownDesiredSize, firstPassState, aStatus, aDesiredSize.width, NS_UNCONSTRAINEDSIZE);
    dropdownFrame->GetRect(dropdownRect);  

  } else {
    // A width has been specified for the select.
    // Make the display frame's width + button frame width = the width specified.
   
    nsHTMLReflowMetrics  dropdownDesiredSize(aDesiredSize);
    ReflowComboChildFrame(dropdownFrame, aPresContext, dropdownDesiredSize, firstPassState, aStatus, NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE); 
     // Get unconstrained size of the dropdown list.
    nsSize size;
    mListControlFrame->GetMaximumSize(size);

      // Size the button to be the same height as the displayFrame
    SetChildFrameSize(buttonFrame, size.height, size.height);

      // Compute display width
    buttonFrame->GetRect(buttonRect);
    nscoord displayWidth;
    displayWidth = firstPassState.computedWidth - buttonRect.width;

     // Set the displayFrame to match the displayWidth computed above
    SetChildFrameSize(displayFrame, displayWidth, size.height);

      // Reflow again with the width of the display frame set.
    nsAreaFrame::Reflow(aPresContext, aDesiredSize, firstPassState, aStatus);

     // Reflow the dropdown list to match the width of the display + button
    ReflowComboChildFrame(dropdownFrame, aPresContext, dropdownDesiredSize, firstPassState, aStatus, aDesiredSize.width, NS_UNCONSTRAINEDSIZE);
    
  }
 
  nsRect absoluteTwips;
  nsRect absolutePixels;
  GetAbsoluteFramePosition(aPresContext, displayFrame,  absoluteTwips, absolutePixels);
  PositionDropdown(aPresContext, aDesiredSize.height, absoluteTwips, absolutePixels);

  return rv;

}

//--------------------------------------------------------------
NS_IMETHODIMP
nsComboboxControlFrame::GetName(nsString* aResult)
{
  nsresult result = NS_FORM_NOTOK;
  if (mContent) {
    nsIHTMLContent* formControl = nsnull;
    result = mContent->QueryInterface(kIHTMLContentIID, (void**)&formControl);
    if ((NS_OK == result) && formControl) {
      nsHTMLValue value;
      result = formControl->GetHTMLAttribute(nsHTMLAtoms::name, value);
      if (NS_CONTENT_ATTR_HAS_VALUE == result) {
        if (eHTMLUnit_String == value.GetUnit()) {
          value.GetStringValue(*aResult);
        }
      }
      NS_RELEASE(formControl);
    }
  }
  return result;
}

PRInt32 
nsComboboxControlFrame::GetMaxNumValues()
{
  return 1;
}
  
PRBool
nsComboboxControlFrame::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                   nsString* aValues, nsString* aNames)
{
  nsAutoString name;
  nsresult result = GetName(&name);
  if ((aMaxNumValues <= 0) || (NS_CONTENT_ATTR_HAS_VALUE != result)) {
    return PR_FALSE;
  }

  // use our name and the text widgets value 
  aNames[0] = name;
  aValues[0] = mTextStr;
  aNumValues = 1;
  nsresult status = PR_TRUE;
  return status;
}

//--------------------------------------------------------------
NS_IMETHODIMP
nsComboboxControlFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("ComboboxControl", aResult);
}



NS_IMETHODIMP
nsComboboxControlFrame::ReResolveStyleContext(nsIPresContext* aPresContext,
                                              nsIStyleContext* aParentContext,
                                              PRInt32 aParentChange,
                                              nsStyleChangeList* aChangeList,
                                              PRInt32* aLocalChange)
{
   PRInt32 ourChange = aParentChange;
   nsresult rv = nsAreaFrame::ReResolveStyleContext(aPresContext, aParentContext, aParentChange, aChangeList, aLocalChange);
   
   if (NS_FAILED(rv)) {
     return rv;
   }

   if (aLocalChange) {
    *aLocalChange = ourChange;
   }

   if (NS_COMFALSE != rv) {
       PRInt32 childChange;
       // Update child list
      nsIFrame* dropdownFrame = GetDropdownFrame();
      dropdownFrame->ReResolveStyleContext(aPresContext, mStyleContext, ourChange, aChangeList, &childChange);
   }

   return rv;
}

nsresult
nsComboboxControlFrame::MouseDown(nsIDOMEvent* aMouseEvent)
{
  ToggleList(mPresContext);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsIComboboxControlFrame
//----------------------------------------------------------------------

NS_IMETHODIMP
nsComboboxControlFrame::SetDropDown(nsIFrame* aDropDownFrame)
{
  mDropdownFrame        = aDropDownFrame;
 
  if (NS_OK != mDropdownFrame->QueryInterface(kIListControlFrameIID, (void**)&mListControlFrame)) {
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}


void
nsComboboxControlFrame::SelectionChanged()
{
  if (nsnull != mDisplayContent) {
    mDisplayContent->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::value, mTextStr, PR_TRUE);
  }

   // Dispatch the NS_FORM_CHANGE event
  nsEventStatus status = nsEventStatus_eIgnore;
  nsGUIEvent event;
  event.eventStructType = NS_GUI_EVENT;
  event.widget = nsnull;
  event.message = NS_FORM_CHANGE;

   // Have the content handle the event.
  mContent->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
   // Now have the frame handle the event
  nsIFrame* frame = nsnull;
  nsIFrame* dropdownFrame = GetDropdownFrame();
  nsresult result = dropdownFrame->QueryInterface(kIFrameIID, (void**)&frame);
  if ((NS_SUCCEEDED(result)) && (nsnull != frame)) {
    frame->HandleEvent(*mPresContext, &event, status);
  }
  
}


NS_IMETHODIMP
nsComboboxControlFrame::ListWasSelected(nsIPresContext* aPresContext)
{
  ShowList(aPresContext, PR_FALSE);
  mListControlFrame->CaptureMouseEvents(PR_FALSE);

  nsString str;
  if (nsnull != mListControlFrame) {
    mListControlFrame->GetSelectedItem(str);
     // Check to see if the selection changed
    if (PR_FALSE == str.Equals(mTextStr)) {
      mTextStr = str;
      //XXX:TODO look at the ordinal position of the selected content in the listbox to tell
      // if the selection has changed, rather than looking at the text string.
      // There may be more than one item in the dropdown list with the same label. 
      SelectionChanged();
    } 
  }

  return NS_OK;
}


nsresult 
nsComboboxControlFrame::RequiresWidget(PRBool& aRequiresWidget)
{
  aRequiresWidget = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
nsComboboxControlFrame::SetProperty(nsIAtom* aName, const nsString& aValue)
{
  nsIFormControlFrame* fcFrame = nsnull;
  nsIFrame* dropdownFrame = GetDropdownFrame();
  nsresult result = dropdownFrame->QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
  if ((NS_SUCCEEDED(result)) && (nsnull != fcFrame)) {
    return fcFrame->SetProperty(aName, aValue);
  }
  return result;
}

NS_IMETHODIMP 
nsComboboxControlFrame::GetProperty(nsIAtom* aName, nsString& aValue)
{
  nsIFormControlFrame* fcFrame = nsnull;
  nsIFrame* dropdownFrame = GetDropdownFrame();
  nsresult result = dropdownFrame->QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
  if ((NS_SUCCEEDED(result)) && (nsnull != fcFrame)) {
    return fcFrame->GetProperty(aName, aValue);
  }
  return result;
}


NS_IMETHODIMP
nsComboboxControlFrame::CreateAnonymousContent(nsISupportsArray& aChildList)
{
  // The frames used to display the combo box and the button used to popup the dropdown list
  // are created through anonymous content. The dropdown list is not created through anonymous
  // content because it's frame is initialized specifically for the drop-down case and it is placed
  // a special list referenced through NS_COMBO_FRAME_POPUP_LIST_INDEX to keep separate from the
  // layout of the display and button. 
  //
  // Note: The value attribute of the display content is set when an item is selected in the dropdown list.
  // If the content specified below does not honor the value attribute than nothing will be displayed.
  // In addition, if the frame created by content below for does not implement the nsIFormControlFrame 
  // interface and honor the SetSuggestedSize method the placement and size of the display area will not
  // match what is normally desired for a combobox.


  // For now the content that is created corresponds to two input buttons. It would be better to create the
  // tag as something other than input, but then there isn't any way to create a button frame since it
  // isn't possible to set the display type in CSS2 to create a button frame.

    // create content used for display
  nsIAtom* tag = NS_NewAtom("input");
  NS_NewHTMLInputElement(&mDisplayContent, tag);
  NS_ADDREF(mDisplayContent);
  mDisplayContent->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::type, nsAutoString("button"), PR_FALSE);
    //XXX: Do not use nsHTMLAtoms::id use nsHTMLAtoms::kClass instead. There will end up being multiple
    //ids set to the same value which is illegal.
  mDisplayContent->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::id, nsAutoString("-moz-display"), PR_FALSE);
  aChildList.AppendElement(mDisplayContent);

  // create button which drops the list down
  tag = NS_NewAtom("input");
  NS_NewHTMLInputElement(&mButtonContent, tag);
  NS_ADDREF(mButtonContent);
  mButtonContent->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::type, nsAutoString("button"), PR_FALSE);
  aChildList.AppendElement(mButtonContent);

  // get the reciever interface from the browser button's content node
  nsCOMPtr<nsIDOMEventReceiver> reciever(do_QueryInterface(mButtonContent));

  // we shouldn't have to unregister this listener because when
  // our frame goes away all these content node go away as well
  // because our frame is the only one who references them.
  reciever->AddEventListenerByIID(this, kIDOMMouseListenerIID);

  // get the reciever interface from the browser button's content node
  nsCOMPtr<nsIDOMEventReceiver> displayReciever(do_QueryInterface(mDisplayContent));

  // we shouldn't have to unregister this listener because when
  // our frame goes away all these content node go away as well
  // because our frame is the only one who references them.
  displayReciever->AddEventListenerByIID(this, kIDOMMouseListenerIID);

  return NS_OK;
}

NS_IMETHODIMP 
nsComboboxControlFrame::SetSuggestedSize(nscoord aWidth, nscoord aHeight)
{
  return NS_OK;
}



NS_IMETHODIMP
nsComboboxControlFrame::DeleteFrame(nsIPresContext& aPresContext)
{
   // Cleanup frames in popup child list
  mPopupFrames.DeleteFrames(aPresContext);
  return nsAreaFrame::DeleteFrame(aPresContext);
}


NS_IMETHODIMP
nsComboboxControlFrame::FirstChild(nsIAtom*   aListName,
                                      nsIFrame** aFirstChild) const
{
  if (nsLayoutAtoms::popupList == aListName) {
    *aFirstChild = mPopupFrames.FirstChild();
  } else {
    nsAreaFrame::FirstChild(aListName, aFirstChild);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsComboboxControlFrame::SetInitialChildList(nsIPresContext& aPresContext,
                                               nsIAtom*        aListName,
                                               nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;
  if (nsLayoutAtoms::popupList == aListName) {
    mPopupFrames.SetFrames(aChildList);
  } else {
    rv = nsAreaFrame::SetInitialChildList(aPresContext, aListName, aChildList);
    InitTextStr(PR_FALSE);
  }
  return rv;
}

NS_IMETHODIMP
nsComboboxControlFrame::GetAdditionalChildListName(PRInt32   aIndex,
                                         nsIAtom** aListName) const
{
   // Maintain a seperate child list for the dropdown list (i.e. popup listbox)
   // This is necessary because we don't want the listbox to be included in the layout
   // of the combox's children because it would take up space, when it is suppose to
   // be floating above the display.
  NS_PRECONDITION(nsnull != aListName, "null OUT parameter pointer");
  if (aIndex <= NS_AREA_FRAME_ABSOLUTE_LIST_INDEX) {
    return nsAreaFrame::GetAdditionalChildListName(aIndex, aListName);
  }
  
  *aListName = nsnull;
  if (NS_COMBO_FRAME_POPUP_LIST_INDEX == aIndex) {
    *aListName = nsLayoutAtoms::popupList;
    NS_ADDREF(*aListName);
  }
  return NS_OK;
}

PRIntn
nsComboboxControlFrame::GetSkipSides() const
{    
    // Don't skip any sides during border rendering
  return 0;
}










