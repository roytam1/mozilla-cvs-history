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

#include "nsFileControlFrame.h"
#include "nsFormControlHelper.h"
#include "nsButtonControlFrame.h"
#include "nsIRenderingContext.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIStyleContext.h"
#include "nsLeafFrame.h"
#include "nsCSSRendering.h"
#include "nsHTMLIIDs.h"
#include "nsIButton.h"
#include "nsIViewManager.h"
#include "nsISupports.h"
#include "nsHTMLAtoms.h"
#include "nsIButton.h"
#include "nsIView.h"
#include "nsViewsCID.h"
#include "nsWidgetsCID.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsIFormControl.h"
#include "nsStyleUtil.h"
#include "nsDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsIHTMLAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsFormFrame.h"
#include "nsILookAndFeel.h"
#include "nsCOMPtr.h"
#include "nsDOMEvent.h"
#include "nsIViewManager.h"

static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kIButtonIID,      NS_IBUTTON_IID);

void
nsButtonControlFrame::GetDefaultLabel(nsString& aString) 
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_INPUT_RESET == type) {
    aString = "Reset";
  } 
  else if (NS_FORM_INPUT_SUBMIT == type) {
    aString = "Submit Query";
  } 
  else if (NS_FORM_BROWSE == type) {
    aString = "Browse...";
  } 
  else {
    aString = " ";
  }
}

PRBool
nsButtonControlFrame::IsSuccessful(nsIFormControlFrame* aSubmitter)
{
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_HIDDEN == type) || (this == aSubmitter)) {
    return Inherited::IsSuccessful(aSubmitter);
  } else {
    return PR_FALSE;
  }
}

PRInt32
nsButtonControlFrame::GetMaxNumValues() 
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
nsButtonControlFrame::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
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
    /*XXX nsresult valResult = */GetValue(&value);
    aValues[0] = value;
    aNames[0]  = name;
    aNumValues = 1;
    return PR_TRUE;
  }
}

nscoord 
nsButtonControlFrame::GetVerticalBorderWidth(float aPixToTwip) const
{
   return NSIntPixelsToTwips(4, aPixToTwip);
}

nscoord 
nsButtonControlFrame::GetHorizontalBorderWidth(float aPixToTwip) const
{
  return GetVerticalBorderWidth(aPixToTwip);
}

nscoord 
nsButtonControlFrame::GetVerticalInsidePadding(nsIPresContext& aPresContext,
                                               float aPixToTwip, 
                                               nscoord aInnerHeight) const
{
  float pad;
  nsCOMPtr<nsILookAndFeel> lookAndFeel;
  if (NS_SUCCEEDED(aPresContext.GetLookAndFeel(getter_AddRefs(lookAndFeel)))) {
   lookAndFeel->GetMetric(nsILookAndFeel::eMetricFloat_ButtonVerticalInsidePadding,  pad);
  }
  return (nscoord)NSToIntRound((float)aInnerHeight * pad);
}

nscoord 
nsButtonControlFrame::GetHorizontalInsidePadding(nsIPresContext& aPresContext,
                                                 float aPixToTwip, 
                                                 nscoord aInnerWidth,
                                                 nscoord aCharWidth) const
{
  nsCompatibility mode;
  aPresContext.GetCompatibilityMode(&mode);

  float   pad;
  PRInt32 padQuirks;
  PRInt32 padQuirksOffset;
  nsCOMPtr<nsILookAndFeel> lookAndFeel;
  if (NS_SUCCEEDED(aPresContext.GetLookAndFeel(getter_AddRefs(lookAndFeel)))) {
    lookAndFeel->GetMetric(nsILookAndFeel::eMetricFloat_ButtonHorizontalInsidePadding,  pad);
    lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ButtonHorizontalInsidePaddingNavQuirks,  padQuirks);
    lookAndFeel->GetMetric(nsILookAndFeel::eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks,  padQuirksOffset);
  }
  if (eCompatibility_NavQuirks == mode) {
    return (nscoord)NSToIntRound(float(aInnerWidth) * pad);
  } else {
    return NSIntPixelsToTwips(padQuirks, aPixToTwip) + padQuirksOffset;
  }
}


void
nsButtonControlFrame::MouseClicked(nsIPresContext* aPresContext) 
{
  //XXX: This method should probably go away. The nsHTMLButtonControl frame's
  //mouse click should be sufficient.
  PRInt32 type;
  GetType(&type);

  if ((nsnull != mFormFrame) && !nsFormFrame::GetDisabled(this)) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    nsIContent *formContent = nsnull;
    mFormFrame->GetContent(&formContent);

    switch(type) {
    case NS_FORM_INPUT_RESET:
      event.message = NS_FORM_RESET;
      if (nsnull != formContent) {
        formContent->HandleDOMEvent(*aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);
      }
      if (nsEventStatus_eConsumeNoDefault != status) {
        mFormFrame->OnReset(aPresContext);
      }
      break;
    case NS_FORM_INPUT_SUBMIT:
      event.message = NS_FORM_SUBMIT;
      if (nsnull != formContent) {
        formContent->HandleDOMEvent(*aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status); 
      }
      if (nsEventStatus_eConsumeNoDefault != status) {
        mFormFrame->OnSubmit(aPresContext, this);
      }
    }
    NS_IF_RELEASE(formContent);
  } 
  //else if ((NS_FORM_BROWSE == type) && mMouseListener) {
  else if (nsnull != mMouseListener) {
    mMouseListener->MouseClicked(aPresContext);
  }
}

void 
nsButtonControlFrame::GetDesiredSize(nsIPresContext* aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             nsHTMLReflowMetrics& aDesiredSize)
{
    // This "do-nothing" implementation exists to remove compiler warnings caused
    // by the fact that GetDesiredSize is both overloaded and overridden.
  Inherited::GetDesiredSize(aPresContext, aReflowState, aDesiredSize);
}


void 
nsButtonControlFrame::GetDesiredSize(nsIPresContext*          aPresContext,
                                     const nsHTMLReflowState& aReflowState,
                                     nsHTMLReflowMetrics&     aDesiredLayoutSize,
                                     nsSize&                  aDesiredWidgetSize)
{
  PRInt32 type;
  GetType(&type);


#if 0
  if ((NS_FORM_INPUT_HIDDEN != type) && ((NS_FORMSIZE_NOTSET != mSuggestedWidth) && (
      NS_FORMSIZE_NOTSET != mSuggestedHeight))) 
  { 
    aDesiredLayoutSize.width   = mSuggestedWidth;
    aDesiredLayoutSize.height  = mSuggestedHeight;
    aDesiredLayoutSize.ascent  = mSuggestedHeight;
    aDesiredLayoutSize.descent = 0;
    if (aDesiredLayoutSize.maxElementSize) {
      aDesiredLayoutSize.maxElementSize->width  = mSuggestedWidth;
      aDesiredLayoutSize.maxElementSize->height = mSuggestedWidth;
    }
    aDesiredWidgetSize.width = aDesiredLayoutSize.width;
    aDesiredWidgetSize.height= aDesiredLayoutSize.height;

    return;
  }
#endif


  

  if (NS_FORM_INPUT_HIDDEN == type) { // there is no physical rep
    aDesiredLayoutSize.width   = 0;
    aDesiredLayoutSize.height  = 0;
    aDesiredLayoutSize.ascent  = 0;
    aDesiredLayoutSize.descent = 0;
    if (aDesiredLayoutSize.maxElementSize) {
      aDesiredLayoutSize.maxElementSize->width  = 0;
      aDesiredLayoutSize.maxElementSize->height = 0;
    }
  } else {
    nsSize styleSize;
    GetStyleSize(*aPresContext, aReflowState, styleSize);
    // a browse button shares its style context with its parent nsInputFile
    // it uses everything from it except width
    if (NS_FORM_BROWSE == type) {
      styleSize.width = CSS_NOTSET;
    }
    nsSize desiredSize;
    nsSize minSize;
    PRBool widthExplicit, heightExplicit;
    PRInt32 ignore;
    nsAutoString defaultLabel;
    GetDefaultLabel(defaultLabel);
    nsInputDimensionSpec spec(nsHTMLAtoms::size, PR_TRUE, nsHTMLAtoms::value, 
                              &defaultLabel, 1, PR_FALSE, nsnull, 1);
    nsFormControlHelper::CalculateSize(aPresContext, aReflowState.rendContext, this, styleSize, 
                                       spec, desiredSize, minSize, widthExplicit, heightExplicit, ignore);   

    // set desired size, max element size
    aDesiredLayoutSize.width = desiredSize.width;
    aDesiredLayoutSize.height= desiredSize.height;
    if (aDesiredLayoutSize.maxElementSize) {
      aDesiredLayoutSize.maxElementSize->width  = minSize.width;
      aDesiredLayoutSize.maxElementSize->height = minSize.height;
    }
  }

  aDesiredWidgetSize.width = aDesiredLayoutSize.width;
  aDesiredWidgetSize.height= aDesiredLayoutSize.height;
}

const nsIID&
nsButtonControlFrame::GetIID()
{
  static NS_DEFINE_IID(kButtonIID, NS_IBUTTON_IID);
  return kButtonIID;
}
  
const nsIID&
nsButtonControlFrame::GetCID()
{
  static NS_DEFINE_IID(kButtonCID, NS_BUTTON_CID);
  return kButtonCID;
}

NS_IMETHODIMP
nsButtonControlFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("ButtonControl", aResult);
}

void 
nsButtonControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
}

void
nsButtonControlFrame::Redraw(nsIPresContext* aPresContext)
{	nsRect rect(0, 0, mRect.width, mRect.height);
    Invalidate(aPresContext, rect, PR_FALSE);

}


NS_IMETHODIMP nsButtonControlFrame::SetProperty(nsIAtom* aName, const nsString& aValue)
{
  return NS_OK;
}

NS_IMETHODIMP nsButtonControlFrame::GetProperty(nsIAtom* aName, nsString& aValue)
{
  return NS_OK;
}

nsresult nsButtonControlFrame::RequiresWidget(PRBool& aRequiresWidget)
{
  aRequiresWidget = PR_FALSE;
  return NS_OK;
}

