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

#include "nsGfxButtonControlFrame.h"
#include "nsIButton.h"
#include "nsWidgetsCID.h"

static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kIButtonIID,      NS_IBUTTON_IID);

const nscoord kSuggestedNotSet = -1;

nsGfxButtonControlFrame::nsGfxButtonControlFrame()
{
  mRenderer.SetNameSpace(kNameSpaceID_None);
  mSuggestedWidth  = kSuggestedNotSet;
  mSuggestedHeight = kSuggestedNotSet;
}

PRBool
nsGfxButtonControlFrame::IsSuccessful(nsIFormControlFrame* aSubmitter)
{
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_HIDDEN == type) || (this == aSubmitter)) {
     // Can not use the nsHTMLButtonControlFrame::IsSuccessful because
     // it will fail it's test of (this==aSubmitter)
    nsAutoString name;
    return (NS_CONTENT_ATTR_HAS_VALUE == GetName(&name));
  } else {
    return PR_FALSE;
  }
}

PRInt32
nsGfxButtonControlFrame::GetMaxNumValues() 
{
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_SUBMIT == type) || (NS_FORM_INPUT_HIDDEN == type)) {
    return 1;
  } else {
    return 0;
  }
}

PRBool
nsGfxButtonControlFrame::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                     nsString* aValues, nsString* aNames)
{
  nsAutoString name;
  nsresult result = GetName(&name);
  if ((aMaxNumValues <= 0) || (NS_CONTENT_ATTR_HAS_VALUE != result)) {
    return PR_FALSE;
  }

  PRInt32 type;
  GetType(&type);

  if (NS_FORM_INPUT_RESET == type) {
    aNumValues = 0;
    return PR_FALSE;
  } else {
    nsAutoString value;
    GetValue(&value);
    aValues[0] = value;
    aNames[0]  = name;
    aNumValues = 1;
    return PR_TRUE;
  }
}

nsresult
NS_NewGfxButtonControlFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsGfxButtonControlFrame* it = new nsGfxButtonControlFrame;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}
      

PRBool
nsGfxButtonControlFrame::IsReset(PRInt32 type)
{
  if (NS_FORM_INPUT_RESET == type) {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}

PRBool
nsGfxButtonControlFrame::IsSubmit(PRInt32 type)
{
  if (NS_FORM_INPUT_SUBMIT == type) {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}
                              
const nsIID&
nsGfxButtonControlFrame::GetIID()
{
  static NS_DEFINE_IID(kButtonIID, NS_IBUTTON_IID);
  return kButtonIID;
}
  
const nsIID&
nsGfxButtonControlFrame::GetCID()
{
  static NS_DEFINE_IID(kButtonCID, NS_BUTTON_CID);
  return kButtonCID;
}

#ifdef DEBUG
NS_IMETHODIMP
nsGfxButtonControlFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("ButtonControl", aResult);
}
#endif

NS_IMETHODIMP 
nsGfxButtonControlFrame::AddComputedBorderPaddingToDesiredSize(nsHTMLReflowMetrics& aDesiredSize,
                                                               const nsHTMLReflowState& aSuggestedReflowState)
{
  if (kSuggestedNotSet == mSuggestedWidth) {
    aDesiredSize.width  += aSuggestedReflowState.mComputedBorderPadding.left + aSuggestedReflowState.mComputedBorderPadding.right;
  }

  if (kSuggestedNotSet == mSuggestedHeight) {
    aDesiredSize.height += aSuggestedReflowState.mComputedBorderPadding.top + aSuggestedReflowState.mComputedBorderPadding.bottom;
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsGfxButtonControlFrame::Reflow(nsIPresContext&          aPresContext, 
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState, 
                                nsReflowStatus&          aStatus)
{
   // The mFormFrame is set in the initial reflow within nsHTMLButtonControlFrame
  nsresult rv = NS_OK;

  if ((kSuggestedNotSet != mSuggestedWidth) || 
      (kSuggestedNotSet != mSuggestedHeight)) {

    nsHTMLReflowState suggestedReflowState(aReflowState);

      // Honor the suggested width and/or height.
    if (kSuggestedNotSet != mSuggestedWidth) {
      suggestedReflowState.mComputedWidth = mSuggestedWidth;
    }

    if (kSuggestedNotSet != mSuggestedHeight) {
      suggestedReflowState.mComputedHeight = mSuggestedHeight;
    }

    rv = nsHTMLButtonControlFrame::Reflow(aPresContext, aDesiredSize, suggestedReflowState, aStatus);

  } else { // Normal reflow.
    rv = nsHTMLButtonControlFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  COMPARE_QUIRK_SIZE("nsGfxButtonControlFrame", 36, 24) //with the text "one" in it

  return rv;
}

NS_IMETHODIMP 
nsGfxButtonControlFrame::SetSuggestedSize(nscoord aWidth, nscoord aHeight)
{
  mSuggestedWidth = aWidth;
  mSuggestedHeight = aHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsGfxButtonControlFrame::HandleEvent(nsIPresContext& aPresContext, 
                                      nsGUIEvent*     aEvent,
                                      nsEventStatus&  aEventStatus)
{
  // Override the HandleEvent to prevent the nsFrame::HandleEvent
  // from being called. The nsFrame::HandleEvent causes the button label
  // to be selected (Drawn with an XOR rectangle over the label)
 
  // if disabled do nothing
  if (mRenderer.isDisabled()) {
    return NS_OK;
  }

   // lets see if the button was clicked
  switch (aEvent->message) {
     case NS_MOUSE_LEFT_CLICK:
        MouseClicked(&aPresContext);
     break;
  }

  return NS_OK;

}


