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
#include "nsTextControlFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIFrame.h"
#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsIHTMLContent.h"
#include "nsHTMLIIDs.h"
#include "nsITextWidget.h"
#include "nsITextAreaWidget.h"
#include "nsWidgetsCID.h"
#include "nsSize.h"
#include "nsString.h"
#include "nsHTMLAtoms.h"
#include "nsIStyleContext.h"
#include "nsFont.h"
#include "nsDOMEvent.h"
#include "nsIFormControl.h"
#include "nsFormFrame.h"
#include "nsIContent.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLTextAreaElement.h"

#include "nsCSSRendering.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsIStatefulFrame.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"

static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kTextCID, NS_TEXTFIELD_CID);
static NS_DEFINE_IID(kTextAreaCID, NS_TEXTAREA_CID);
static NS_DEFINE_IID(kITextWidgetIID, NS_ITEXTWIDGET_IID);
static NS_DEFINE_IID(kITextAreaWidgetIID, NS_ITEXTAREAWIDGET_IID);
static NS_DEFINE_IID(kIDOMHTMLTextAreaElementIID, NS_IDOMHTMLTEXTAREAELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLInputElementIID, NS_IDOMHTMLINPUTELEMENT_IID);

const nscoord kSuggestedNotSet = -1;
nsTextControlFrame::nsTextControlFrame() 
{
  mSuggestedWidth  = kSuggestedNotSet;
  mSuggestedHeight = kSuggestedNotSet;
}

nsTextControlFrame::~nsTextControlFrame() 
{
}

// Frames are not refcounted, no need to AddRef
NS_IMETHODIMP
nsTextControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  } else  if (aIID.Equals(NS_GET_IID(nsIStatefulFrame))) {
    *aInstancePtr = (void*)(nsIStatefulFrame*) this;
    return NS_OK;                                                        
  }
  
  return nsFormControlFrame::QueryInterface(aIID, aInstancePtr);
}

nscoord 
nsTextControlFrame::GetVerticalBorderWidth(float aPixToTwip) const
{
   return NSIntPixelsToTwips(4, aPixToTwip);
}

nscoord 
nsTextControlFrame::GetHorizontalBorderWidth(float aPixToTwip) const
{
  return GetVerticalBorderWidth(aPixToTwip);
}

// for a text area aInnerHeight is the height of one line
nscoord 
nsTextControlFrame::GetVerticalInsidePadding(nsIPresContext& aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerHeight) const
{

  // XXX NOTE: the enums eMetric_TextShouldUseVerticalInsidePadding and eMetric_TextVerticalInsidePadding
  // are ONLY needed because GTK is not using the "float" padding values and wants to only use an 
  // integer value for the padding instead of calculating like the other platforms.
  //
  // If GTK decides to start calculating the value, PLEASE remove these two enum from nsILookAndFeel and
  // all the platforms nsLookAndFeel impementations so we don't have these extra values remaining in the code.
  // The two enums are:
  //    eMetric_TextVerticalInsidePadding
  //    eMetric_TextShouldUseVerticalInsidePadding
  //
  float   padTextArea;
  float   padTextField;
  PRInt32 vertPad;
  PRInt32 shouldUseVertPad;
  nsCOMPtr<nsILookAndFeel> lookAndFeel;
  if (NS_SUCCEEDED(aPresContext.GetLookAndFeel(getter_AddRefs(lookAndFeel)))) {
   lookAndFeel->GetMetric(nsILookAndFeel::eMetricFloat_TextAreaVerticalInsidePadding,  padTextArea);
   lookAndFeel->GetMetric(nsILookAndFeel::eMetricFloat_TextFieldVerticalInsidePadding,  padTextField);
   // These two (below) are really only needed for GTK
   lookAndFeel->GetMetric(nsILookAndFeel::eMetric_TextVerticalInsidePadding,  vertPad);
   lookAndFeel->GetMetric(nsILookAndFeel::eMetric_TextShouldUseVerticalInsidePadding,  shouldUseVertPad);
  }

  if (1 == shouldUseVertPad) {
    return NSIntPixelsToTwips(vertPad, aPixToTwip); // XXX this is probably wrong (for GTK)
  } else {
    PRInt32 type;
    GetType(&type);
    if (NS_FORM_TEXTAREA == type) {
      return (nscoord)NSToIntRound(float(aInnerHeight) * padTextArea);
    } else {
      return (nscoord)NSToIntRound(float(aInnerHeight) * padTextField);
    }
  }
}
//static float pad = 0.95F;
nscoord 
nsTextControlFrame::GetHorizontalInsidePadding(nsIPresContext& aPresContext,
                                               float aPixToTwip, 
                                               nscoord aInnerWidth,
                                               nscoord aCharWidth) const
{
  // XXX NOTE: the enum eMetric_TextShouldUseHorizontalInsideMinimumPadding
  // is ONLY needed because GTK is not using the "float" padding values and wants to only use the 
  // "minimum" integer value for the padding instead of calculating and comparing like the other platforms.
  //
  // If GTK decides to start calculating and comparing the values, 
  // PLEASE remove these the enum from nsILookAndFeel and
  // all the platforms nsLookAndFeel impementations so we don't have these extra values remaining in the code.
  // The enum is:
  //    eMetric_TextShouldUseHorizontalInsideMinimumPadding
  //
  float   padTextField;
  float   padTextArea;
  PRInt32 padMinText;
  PRInt32 shouldUsePadMinText;

  nsCOMPtr<nsILookAndFeel> lookAndFeel;
  if (NS_SUCCEEDED(aPresContext.GetLookAndFeel(getter_AddRefs(lookAndFeel)))) {
   lookAndFeel->GetMetric(nsILookAndFeel::eMetricFloat_TextFieldHorizontalInsidePadding,  padTextField);
   lookAndFeel->GetMetric(nsILookAndFeel::eMetricFloat_TextAreaHorizontalInsidePadding,  padTextArea);
   lookAndFeel->GetMetric(nsILookAndFeel::eMetric_TextHorizontalInsideMinimumPadding,  padMinText);
   // This one (below) is really only needed for GTK
   lookAndFeel->GetMetric(nsILookAndFeel::eMetric_TextShouldUseHorizontalInsideMinimumPadding,  shouldUsePadMinText);
  }
//padTextField = pad;
  nscoord padding;
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_TEXTAREA == type) {
    padding = (nscoord)(aCharWidth * padTextArea);
  } else {
    padding = (nscoord)(aCharWidth * padTextField);
  }

  
  nscoord min = NSIntPixelsToTwips(padMinText, aPixToTwip);
  if (padding > min && (1 == shouldUsePadMinText)) {
    return padding;
  } else {
    return min;
  }

}


const nsIID&
nsTextControlFrame::GetIID()
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_TEXTAREA == type) {
    return kITextAreaWidgetIID;
  } else {
    return kITextWidgetIID;
  }
}
  
const nsIID&
nsTextControlFrame::GetCID()
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_TEXTAREA == type) {
    return kTextAreaCID;
  } else {
    return kTextCID;
  }
}

void 
nsTextControlFrame::GetDesiredSize(nsIPresContext* aPresContext,
                                   const nsHTMLReflowState& aReflowState,
                                   nsHTMLReflowMetrics& aDesiredLayoutSize)
{
  nsSize widgetSize;
  GetDesiredSize(aPresContext, aReflowState, aDesiredLayoutSize, widgetSize);
}

void 
nsTextControlFrame::GetDesiredSize(nsIPresContext* aPresContext,
                                   const nsHTMLReflowState& aReflowState,
                                   nsHTMLReflowMetrics& aDesiredLayoutSize,
                                   nsSize& aDesiredWidgetSize)
{
  nsCompatibility mode;
  aPresContext->GetCompatibilityMode(&mode);

  // get the css size and let the frame use or override it
  nsSize styleSize;
  GetStyleSize(*aPresContext, aReflowState, styleSize);

  nsSize desiredSize;
  nsSize minSize;
  
  PRBool widthExplicit, heightExplicit;
  PRInt32 ignore;
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_TEXT == type) || (NS_FORM_INPUT_PASSWORD == type)) {
    PRInt32 width;
    if (NS_CONTENT_ATTR_HAS_VALUE != GetSizeFromContent(&width)) {
      width = GetDefaultColumnWidth();
    }
    //if (eCompatibility_NavQuirks == mode) {
    //  width += 1;
    //}
    nsInputDimensionSpec textSpec(nsnull, PR_FALSE, nsnull,
                                  nsnull, width, PR_FALSE, nsnull, 1);
    nsFormControlHelper::CalculateSize(aPresContext, aReflowState.rendContext, this, styleSize, 
                                       textSpec, desiredSize, minSize, widthExplicit, heightExplicit, ignore);
  } else {
    nsInputDimensionSpec areaSpec(nsHTMLAtoms::cols, PR_FALSE, nsnull, nsnull, GetDefaultColumnWidth(), 
                                  PR_FALSE, nsHTMLAtoms::rows, 1);
    nsFormControlHelper::CalculateSize(aPresContext, aReflowState.rendContext, this, styleSize, 
                                       areaSpec, desiredSize, minSize, widthExplicit, heightExplicit, ignore);
  }

  // CalculateSize makes calls in the nsFormControlHelper that figures
  // out the entire size of the control when in NavQuirks mode. For the
  // textarea, this means the scrollbar sizes hav already been added to
  // its overall size and do not need to be added here.
  float   p2t;
  aPresContext->GetPixelsToTwips(&p2t);
  if (NS_FORM_TEXTAREA == type && mode == eCompatibility_Standard) {
    nscoord scrollbarWidth  = 0;
    nscoord scrollbarHeight = 0;
    float   scale;
    nsCOMPtr<nsIDeviceContext> dx;
    aPresContext->GetDeviceContext(getter_AddRefs(dx));
    if (dx) { 
      float sbWidth;
      float sbHeight;
      dx->GetCanonicalPixelScale(scale);
      dx->GetScrollBarDimensions(sbWidth, sbHeight);
      scrollbarWidth  = PRInt32(sbWidth * scale);
      scrollbarHeight = PRInt32(sbHeight * scale);
    } else {
      scrollbarWidth  = GetScrollbarWidth(p2t);
      scrollbarHeight = scrollbarWidth;
    }

    if (!heightExplicit) {
      desiredSize.height += scrollbarHeight;
      minSize.height     += scrollbarHeight;
    } 
    if (!widthExplicit) {
      desiredSize.width += scrollbarWidth;
      minSize.width     += scrollbarWidth;
    }
  }

  aDesiredLayoutSize.width   = desiredSize.width;
  aDesiredLayoutSize.height  = desiredSize.height;
  aDesiredLayoutSize.ascent  = aDesiredLayoutSize.height;
  aDesiredLayoutSize.descent = 0;

  if (aDesiredLayoutSize.maxElementSize) {
    aDesiredLayoutSize.maxElementSize->width  = minSize.width;
    aDesiredLayoutSize.maxElementSize->height = minSize.height;
  }

  aDesiredWidgetSize.width  = aDesiredLayoutSize.width;
  aDesiredWidgetSize.height = aDesiredLayoutSize.height;

}


PRInt32 
nsTextControlFrame::GetMaxNumValues()
{
  return 1;
}

NS_IMETHODIMP
nsTextControlFrame::GetCursor(nsIPresContext& aPresContext, nsPoint& aPoint, PRInt32& aCursor)
{
  /*const nsStyleColor* styleColor;
  GetStyleData(eStyleStruct_Color, (const nsStyleStruct*&)styleColor);
  aCursor = styleColor->mCursor;*/

  //XXX This is wrong, should be through style.
  aCursor = NS_STYLE_CURSOR_TEXT;
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTextControlFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("TextControl", aResult);
}
#endif

//---------------------------------------------------------
NS_IMETHODIMP 
nsTextControlFrame::SetSuggestedSize(nscoord aWidth, nscoord aHeight)
{
  mSuggestedWidth  = aWidth;
  mSuggestedHeight = aHeight;

  return NS_OK;
}

//----------------------------------------------------------------------
// nsIStatefulFrame
//----------------------------------------------------------------------
NS_IMETHODIMP
nsTextControlFrame::GetStateType(nsIPresContext* aPresContext, nsIStatefulFrame::StateType* aStateType)
{
  *aStateType = nsIStatefulFrame::eTextType;
  return NS_OK;
}

NS_IMETHODIMP
nsTextControlFrame::SaveState(nsIPresContext* aPresContext, nsISupports** aState)
{
  nsISupportsString* value = nsnull;
  nsAutoString string;
  nsresult res = GetProperty(nsHTMLAtoms::value, string);
  if (NS_SUCCEEDED(res)) {
    char* chars = string.ToNewCString();
    if (chars) {
      res = nsComponentManager::CreateInstance(NS_SUPPORTS_STRING_PROGID, nsnull, 
                                           NS_GET_IID(nsISupportsString), (void**)&value);
      if (NS_SUCCEEDED(res) && value) {
        value->SetData(chars);
      }
      nsCRT::free(chars);
    } else {
      res = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  *aState = (nsISupports*)value;
  return res;
}

NS_IMETHODIMP
nsTextControlFrame::RestoreState(nsIPresContext* aPresContext, nsISupports* aState)
{
  char* chars = nsnull;
  nsresult res = ((nsISupportsString*)aState)->GetData(&chars);
  if (NS_SUCCEEDED(res) && chars) {
    nsAutoString string(chars);
    res = SetProperty(aPresContext, nsHTMLAtoms::value, string);
    nsCRT::free(chars);
  }
  return res;
}
