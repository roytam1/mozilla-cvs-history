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

#ifdef SingleSignon
#include "nsIDocument.h"
#include "prmem.h"
#include "nsIURL.h"
#include "nsINetService.h"
#include "nsIServiceManager.h"
static NS_DEFINE_IID(kINetServiceIID, NS_INETSERVICE_IID);
static NS_DEFINE_IID(kNetServiceCID, NS_NETSERVICE_CID);
#endif

static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kTextCID, NS_TEXTFIELD_CID);
static NS_DEFINE_IID(kTextAreaCID, NS_TEXTAREA_CID);
static NS_DEFINE_IID(kITextWidgetIID, NS_ITEXTWIDGET_IID);
static NS_DEFINE_IID(kITextAreaWidgetIID, NS_ITEXTAREAWIDGET_IID);
static NS_DEFINE_IID(kIDOMHTMLTextAreaElementIID, NS_IDOMHTMLTEXTAREAELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLInputElementIID, NS_IDOMHTMLINPUTELEMENT_IID);

nsresult
NS_NewTextControlFrame(nsIContent* aContent,
                       nsIFrame*   aParent,
                       nsIFrame*&  aResult)
{
  aResult = new nsTextControlFrame(aContent, aParent);
  if (nsnull == aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsTextControlFrame::nsTextControlFrame(nsIContent* aContent,
                                       nsIFrame* aParentFrame)
  : nsFormControlFrame(aContent, aParentFrame)
{
}

nsTextControlFrame::~nsTextControlFrame()
{
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
nsTextControlFrame::GetVerticalInsidePadding(float aPixToTwip, 
                                             nscoord aInnerHeight) const
{
#ifdef XP_PC
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_TEXTAREA == type) {
    return (nscoord)NSToIntRound(float(aInnerHeight) * 0.40f);
  } else {
    return (nscoord)NSToIntRound(float(aInnerHeight) * 0.25f);
  }
#endif
#ifdef XP_UNIX
  return NSIntPixelsToTwips(10, aPixToTwip); // XXX this is probably wrong
#endif
}

nscoord 
nsTextControlFrame::GetHorizontalInsidePadding(nsIPresContext& aPresContext,
                                               float aPixToTwip, 
                                               nscoord aInnerWidth,
                                               nscoord aCharWidth) const
{
#ifdef XP_PC
  nscoord padding;
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_TEXTAREA == type) {
    padding = (nscoord)(40 * aCharWidth / 100);
  } else {
    padding = (nscoord)(95 * aCharWidth / 100);
  }
  nscoord min = NSIntPixelsToTwips(3, aPixToTwip);
  if (padding > min) {
    return padding;
  } else {
    return min;
  }
#endif
#ifdef XP_UNIX
  return NSIntPixelsToTwips(6, aPixToTwip);  // XXX this is probably wrong
#endif
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
nsTextControlFrame::EnterPressed(nsIPresContext& aPresContext) 
{
  if (mFormFrame && mFormFrame->CanSubmit(*this)) {
    nsEventStatus mStatus = nsEventStatus_eIgnore;
    nsEvent mEvent;
    mEvent.eventStructType = NS_EVENT;
    mEvent.message = NS_FORM_SUBMIT;
    mContent->HandleDOMEvent(aPresContext, &mEvent, nsnull, DOM_EVENT_INIT, mStatus); 

    mFormFrame->OnSubmit(&aPresContext, this);
  }
}

void 
nsTextControlFrame::GetDesiredSize(nsIPresContext* aPresContext,
                                   const nsHTMLReflowState& aReflowState,
                                   nsHTMLReflowMetrics& aDesiredLayoutSize,
                                   nsSize& aDesiredWidgetSize)
{
  nsCompatibility mode;
  aPresContext->GetCompatibilityMode(mode);

  // get the css size and let the frame use or override it
  nsSize styleSize;
  GetStyleSize(*aPresContext, aReflowState, styleSize);

  nsSize size;
  
  PRBool widthExplicit, heightExplicit;
  PRInt32 ignore;
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_TEXT == type) || (NS_FORM_INPUT_PASSWORD == type)) {
    PRInt32 width;
    if (NS_CONTENT_ATTR_HAS_VALUE != GetSize(&width)) {
      width = 20;
    }
    //if (eCompatibility_NavQuirks == mode) {
    //  width += 1;
    //}
    nsInputDimensionSpec textSpec(nsnull, PR_FALSE, nsnull,
                                  nsnull, width, PR_FALSE, nsnull, 1);
    CalculateSize(aPresContext, this, styleSize, textSpec, size, 
                  widthExplicit, heightExplicit, ignore,
                  aReflowState.rendContext);
  } else {
    nsInputDimensionSpec areaSpec(nsHTMLAtoms::cols, PR_FALSE, nsnull, nsnull, 20, 
                                  PR_FALSE, nsHTMLAtoms::rows, 1);
    CalculateSize(aPresContext, this, styleSize, areaSpec, size, 
                  widthExplicit, heightExplicit, ignore,
                  aReflowState.rendContext);
  }

  if (NS_FORM_TEXTAREA == type) {
    float p2t = aPresContext->GetPixelsToTwips();
    nscoord scrollbarWidth = GetScrollbarWidth(p2t);

    if (!heightExplicit) {
      size.height += scrollbarWidth;
    } 
    if (!widthExplicit) {
      size.width += scrollbarWidth;
    }
  }


  aDesiredLayoutSize.width  = size.width;
  aDesiredLayoutSize.height = size.height;
  aDesiredLayoutSize.ascent = aDesiredLayoutSize.height;
  aDesiredLayoutSize.descent = 0;
  aDesiredWidgetSize.width  = aDesiredLayoutSize.width;
  aDesiredWidgetSize.height = aDesiredLayoutSize.height;
}

nsWidgetInitData*
nsTextControlFrame::GetWidgetInitData(nsIPresContext& aPresContext)
{
  PRInt32 type;
  GetType(&type);

  nsTextWidgetInitData* data = nsnull;

  PRBool readOnly = nsFormFrame::GetReadonly(this);

  if ((NS_FORM_INPUT_PASSWORD == type) || readOnly) {
    data = new nsTextWidgetInitData();
    data->mIsPassword = PR_FALSE;
    data->mIsReadOnly = PR_FALSE;
    if (NS_FORM_INPUT_PASSWORD == type) {
      data->clipChildren = PR_TRUE;
      data->mIsPassword = PR_TRUE;
    } 
    if (readOnly) {
      data->mIsReadOnly = PR_TRUE;
    }
  }

  return data;
}

NS_IMETHODIMP
nsTextControlFrame::GetText(nsString* aText)
{
  nsresult result = NS_CONTENT_ATTR_NOT_THERE;
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_TEXT == type) || (NS_FORM_INPUT_PASSWORD == type)) {
    nsIDOMHTMLInputElement* textElem = nsnull;
    result = mContent->QueryInterface(kIDOMHTMLInputElementIID, (void**)&textElem);
    if ((NS_OK == result) && textElem) {
      result = textElem->GetDefaultValue(*aText);
      NS_RELEASE(textElem);
    }
  } else {
    nsIDOMHTMLTextAreaElement* textArea = nsnull;
    result = mContent->QueryInterface(kIDOMHTMLTextAreaElementIID, (void**)&textArea);
    if ((NS_OK == result) && textArea) {
      result = textArea->GetDefaultValue(*aText);
      NS_RELEASE(textArea);
    }
  }
  return result;
}

NS_IMETHODIMP
nsTextControlFrame::AttributeChanged(nsIPresContext* aPresContext,
                                       nsIContent*     aChild,
                                       nsIAtom*        aAttribute,
                                       PRInt32         aHint)
{
  nsresult result = NS_OK;
  PRInt32 type;
  GetType(&type);
  if (mWidget) {
    nsITextWidget* text = nsnull;
    result = mWidget->QueryInterface(kITextWidgetIID, (void**)&text);
    if ((NS_SUCCEEDED(result)) && (nsnull != text)) {
      if (nsHTMLAtoms::value == aAttribute) {
          nsString value;
          nsresult result = GetText(&value);
          PRUint32 ignore;
          text->SetText(value, ignore);
          nsFormFrame::StyleChangeReflow(aPresContext, this);
        }
      } else if (nsHTMLAtoms::size == aAttribute) {
        nsFormFrame::StyleChangeReflow(aPresContext, this);
      } else if (nsHTMLAtoms::maxlength == aAttribute) {
        PRInt32 maxLength;
        nsresult result = GetMaxLength(&maxLength);
        if (NS_CONTENT_ATTR_NOT_THERE != result) {
          text->SetMaxTextLength(maxLength);
        }
      }
      NS_RELEASE(text);
  }

  return result;
}

void 
nsTextControlFrame::PostCreateWidget(nsIPresContext* aPresContext,
                                     nscoord& aWidth, 
                                     nscoord& aHeight)
{
  if (!mWidget) {
    return;
  }

  PRInt32 type;
  GetType(&type);

  nsFont font(aPresContext->GetDefaultFixedFont()); 
  GetFont(aPresContext, font);
  mWidget->SetFont(font);
  SetColors(*aPresContext);

  PRUint32 ignore;
  nsAutoString value;

  nsITextAreaWidget* textArea = nsnull;
  nsITextWidget* text = nsnull;
  if (NS_OK == mWidget->QueryInterface(kITextWidgetIID,(void**)&text)) {

#ifdef SingleSignon
    /* get name of text */
    nsAutoString name;
    GetName(&name);

    /* get url name */
    char *URLName;
    nsIURL* docURL = nsnull;
    nsIDocument* doc = nsnull;
    mContent->GetDocument(doc);
    if (nsnull != doc) {
      docURL = doc->GetDocumentURL();
      NS_RELEASE(doc);
      URLName = (char*)PR_Malloc(PL_strlen(docURL->GetSpec())+1);
      PL_strcpy(URLName, docURL->GetSpec());
    }

    /* invoke single-signon to get previously-used value of text */
    nsINetService *service;
    nsresult res = nsServiceManager::GetService(kNetServiceCID,
                                          kINetServiceIID,
                                          (nsISupports **)&service);
    if ((NS_OK == res) && (nsnull != service)) {
      char* valueString = NULL;
      res = service->SI_RestoreSignonData(URLName, name.ToNewCString(), &valueString);
      NS_RELEASE(service);
      if (valueString && *valueString) {
        value = nsAutoString(valueString);
      } else {
        GetText(&value);
      }
    }

#else
  GetText(&value);
#endif

    text->SetText(value, ignore);
    PRInt32 maxLength;
    nsresult result = GetMaxLength(&maxLength);
    if (NS_CONTENT_ATTR_NOT_THERE != result) {
      text->SetMaxTextLength(maxLength);
    }
    NS_RELEASE(text);
  } else if (NS_OK == mWidget->QueryInterface(kITextAreaWidgetIID,(void**)&textArea)) {
    textArea->SetText(value, ignore);
    NS_RELEASE(textArea);
  }
  if (nsFormFrame::GetDisabled(this)) {
    mWidget->Enable(PR_FALSE);
  }
}

PRInt32 
nsTextControlFrame::GetMaxNumValues()
{
  return 1;
}
  
PRBool
nsTextControlFrame::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                   nsString* aValues, nsString* aNames)
{
  if (!mWidget) {
    return PR_FALSE;
  }

  nsAutoString name;
  nsresult result = GetName(&name);
  if ((aMaxNumValues <= 0) || (NS_CONTENT_ATTR_NOT_THERE == result)) {
    return PR_FALSE;
  }

  PRUint32 size;
  nsITextWidget* text = nsnull;
  nsITextAreaWidget* textArea = nsnull;

  aNames[0] = name;  
  aNumValues = 1;

  if (NS_OK == mWidget->QueryInterface(kITextWidgetIID,(void**)&text)) {
    text->GetText(aValues[0],0,size);  // the last parm is not used
    NS_RELEASE(text);
    return PR_TRUE;
  } else if (NS_OK == mWidget->QueryInterface(kITextAreaWidgetIID,(void**)&textArea)) {
    textArea->GetText(aValues[0],0,size);  // the last parm is not used
    NS_RELEASE(textArea);
    return PR_TRUE;
  }
  return PR_FALSE;
}


void 
nsTextControlFrame::Reset() 
{
  if (!mWidget) {
    return;
  }

  nsITextWidget* text = nsnull;
  nsITextAreaWidget* textArea = nsnull;

  nsAutoString value;
  nsresult valStatus = GetText(&value);

  PRUint32 size;
  if (NS_OK == mWidget->QueryInterface(kITextWidgetIID,(void**)&text)) {
    if (NS_CONTENT_ATTR_HAS_VALUE == valStatus) {
      text->SetText(value,size);
    } else {
      text->SetText("",size);
    }
    NS_RELEASE(text);
  } else if (NS_OK == mWidget->QueryInterface(kITextAreaWidgetIID,(void**)&textArea)) {
    if (NS_CONTENT_ATTR_HAS_VALUE == valStatus) {
      textArea->SetText(value,size);
    } else {
      textArea->SetText("",size);
    }
    NS_RELEASE(textArea);
  }

}  


