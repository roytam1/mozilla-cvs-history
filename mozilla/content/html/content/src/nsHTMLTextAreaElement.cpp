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
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIWidget.h"
#include "nsITextWidget.h"
#include "nsIHTMLAttributes.h"

static NS_DEFINE_IID(kIDOMHTMLTextAreaElementIID, NS_IDOMHTMLTEXTAREAELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLFormElementIID, NS_IDOMHTMLFORMELEMENT_IID);
static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kIFormIID, NS_IFORM_IID);
static NS_DEFINE_IID(kITextWidgetIID, NS_ITEXTWIDGET_IID);

class nsHTMLTextAreaElement : public nsIDOMHTMLTextAreaElement,
                              public nsIScriptObjectOwner,
                              public nsIDOMEventReceiver,
                              public nsIHTMLContent,
                              public nsIFormControl
{
public:
  nsHTMLTextAreaElement(nsIAtom* aTag);
  ~nsHTMLTextAreaElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLTextAreaElement
  NS_IMETHOD GetDefaultValue(nsString& aDefaultValue);
  NS_IMETHOD SetDefaultValue(const nsString& aDefaultValue);
  NS_IMETHOD GetForm(nsIDOMHTMLFormElement** aForm);
  NS_IMETHOD GetAccessKey(nsString& aAccessKey);
  NS_IMETHOD SetAccessKey(const nsString& aAccessKey);
  NS_IMETHOD GetCols(PRInt32* aCols);
  NS_IMETHOD SetCols(PRInt32 aCols);
  NS_IMETHOD GetDisabled(PRBool* aDisabled);
  NS_IMETHOD SetDisabled(PRBool aDisabled);
  NS_IMETHOD GetName(nsString& aName);
  NS_IMETHOD SetName(const nsString& aName);
  NS_IMETHOD GetReadOnly(PRBool* aReadOnly);
  NS_IMETHOD SetReadOnly(PRBool aReadOnly);
  NS_IMETHOD GetRows(PRInt32* aRows);
  NS_IMETHOD SetRows(PRInt32 aRows);
  NS_IMETHOD GetTabIndex(PRInt32* aTabIndex);
  NS_IMETHOD SetTabIndex(PRInt32 aTabIndex);
  NS_IMETHOD GetType(nsString& aType);
  NS_IMETHOD GetValue(nsString& aValue);
  NS_IMETHOD SetValue(const nsString& aValue);
  NS_IMETHOD Blur();
  NS_IMETHOD Focus();
  NS_IMETHOD Select();

  // nsIScriptObjectOwner
  NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(mInner)

  // nsIDOMEventReceiver
  NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

  // nsIFormControl
  NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm);
  NS_IMETHOD GetType(PRInt32* aType);
  NS_IMETHOD SetWidget(nsIWidget* aWidget);
  NS_IMETHOD Init() { return NS_OK; }

protected:
  nsGenericHTMLContainerElement mInner;
  nsIWidget* mWidget; // XXX this needs to go away when FindFrameWithContent is efficient
  nsIForm*   mForm;
};

nsresult
NS_NewHTMLTextAreaElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLTextAreaElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLTextAreaElement::nsHTMLTextAreaElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
  mForm = nsnull;
  mWidget = nsnull;
}

nsHTMLTextAreaElement::~nsHTMLTextAreaElement()
{
  NS_IF_RELEASE(mWidget);
  if (nsnull != mForm) {
    // prevent mForm from decrementing its ref count on us
    mForm->RemoveElement(this, PR_FALSE); 
    NS_RELEASE(mForm);
  }
}

NS_IMETHODIMP
nsHTMLTextAreaElement::AddRef(void)
{
  PRInt32 refCnt = mRefCnt;  // debugging 
  return ++mRefCnt; 
}

nsresult
nsHTMLTextAreaElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLTextAreaElementIID)) {
    *aInstancePtr = (void*)(nsIDOMHTMLTextAreaElement*) this;
    mRefCnt++;
    return NS_OK;
  }
  else if (aIID.Equals(kIFormControlIID)) {
    *aInstancePtr = (void*)(nsIFormControl*) this;
    mRefCnt++;
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMETHODIMP_(nsrefcnt)
nsHTMLTextAreaElement::Release()
{
  --mRefCnt;
	if (mRefCnt <= 0) {
    delete this;                                       
    return 0;                                          
  } else if ((1 == mRefCnt) && mForm) { 
    mRefCnt = 0;
    delete this;
    return 0;
  } else {
    return mRefCnt;
  }
}

// nsIDOMHTMLTextAreaElement

nsresult
nsHTMLTextAreaElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsHTMLTextAreaElement* it = new nsHTMLTextAreaElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  nsresult result = NS_OK;
  *aForm = nsnull;
  if (nsnull != mForm) {
    nsIDOMHTMLFormElement* formElem = nsnull;
    result = mForm->QueryInterface(kIDOMHTMLFormElementIID, (void**)&formElem);
    if (NS_OK == result) {
      *aForm = formElem;
    }
  }
  return result;
}


NS_IMETHODIMP
nsHTMLTextAreaElement::Blur() // XXX not tested
{
  if (nsnull != mWidget) {
    nsIWidget *mParentWidget = mWidget->GetParent();
    if (nsnull != mParentWidget) {
      mParentWidget->SetFocus();
      NS_RELEASE(mParentWidget);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::Focus() // XXX not tested
{
  if (nsnull != mWidget) {
    mWidget->SetFocus();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::Select() // XXX not tested
{
  if (nsnull != mWidget) {
    nsITextWidget *textWidget;
    if (NS_OK == mWidget->QueryInterface(kITextWidgetIID, (void**)&textWidget)) {
      textWidget->SelectAll();
      NS_RELEASE(textWidget);
    }
  }
  return NS_OK;
}

NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, AccessKey, accesskey, eSetAttrNotify_None)
NS_IMPL_INT_ATTR(nsHTMLTextAreaElement, Cols, cols, eSetAttrNotify_Reflow)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, Disabled, disabled, eSetAttrNotify_Render)
NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, Name, name, eSetAttrNotify_Restart)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, ReadOnly, readonly, eSetAttrNotify_Render)
NS_IMPL_INT_ATTR(nsHTMLTextAreaElement, Rows, rows, eSetAttrNotify_Reflow)
NS_IMPL_INT_ATTR(nsHTMLTextAreaElement, TabIndex, tabindex, eSetAttrNotify_None)
  

NS_IMETHODIMP 
nsHTMLTextAreaElement::GetType(nsString& aType)
{
  aType.SetString("textarea");
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLTextAreaElement::GetValue(nsString& aValue)
{
  // XXX TBI
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsHTMLTextAreaElement::SetValue(const nsString& aValue)
{
  // XXX TBI
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetDefaultValue(nsString& aDefaultValue)
{
  mInner.GetAttribute(nsHTMLAtoms::defaultvalue, aDefaultValue);                 
  return NS_OK;                                                    
}  

NS_IMETHODIMP
nsHTMLTextAreaElement::SetDefaultValue(const nsString& aDefaultValue)
{
  // trim leading whitespace 
  static char whitespace[] = " \r\n\t";
  nsString value(aDefaultValue);
  value.Trim(whitespace, PR_TRUE, PR_FALSE);
  return mInner.SetAttribute(nsHTMLAtoms::defaultvalue, value, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLTextAreaElement::StringToAttribute(nsIAtom* aAttribute,
                                         const nsString& aValue,
                                         nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (nsGenericHTMLElement::ParseFormAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::disabled) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::cols) {
    nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::readonly) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::rows) {
    nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::tabindex) {
    nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::AttributeToString(nsIAtom* aAttribute,
                                         nsHTMLValue& aValue,
                                         nsString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      nsGenericHTMLElement::FormAlignValueToString(aValue, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

static void
MapAttributesInto(nsIHTMLAttributes* aAttributes,
                  nsIStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  nsHTMLValue value;

  aAttributes->GetAttribute(nsHTMLAtoms::align, value);
  if (eHTMLUnit_Enumerated == value.GetUnit()) {
    nsStyleDisplay* display = (nsStyleDisplay*)
      aContext->GetMutableStyleData(eStyleStruct_Display);
    nsStyleText* text = (nsStyleText*)
      aContext->GetMutableStyleData(eStyleStruct_Text);
    switch (value.GetIntValue()) {
    case NS_STYLE_TEXT_ALIGN_LEFT:
      display->mFloats = NS_STYLE_FLOAT_LEFT;
      break;
    case NS_STYLE_TEXT_ALIGN_RIGHT:
      display->mFloats = NS_STYLE_FLOAT_RIGHT;
      break;
    default:
      text->mVerticalAlign.SetIntValue(value.GetIntValue(), eStyleUnit_Enumerated);
      break;
    }
  }
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext, aPresContext);
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetAttributeMappingFunction(nsMapAttributesFunc& aMapFunc) const
{
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTextAreaElement::HandleDOMEvent(nsIPresContext& aPresContext,
                                      nsEvent* aEvent,
                                      nsIDOMEvent** aDOMEvent,
                                      PRUint32 aFlags,
                                      nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}

// nsIFormControl

NS_IMETHODIMP
nsHTMLTextAreaElement::GetType(PRInt32* aType)
{
  if (aType) {
    *aType = NS_FORM_TEXTAREA;
    return NS_OK;
  } else {
    return NS_FORM_NOTOK;
  }
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetWidget(nsIWidget* aWidget)
{
  if (aWidget != mWidget) {
	  NS_IF_RELEASE(mWidget);
    NS_IF_ADDREF(aWidget);
    mWidget = aWidget;
  }
  return NS_OK;
}

// An important assumption is that if aForm is null, the previous mForm will not be released
// This allows nsHTMLFormElement to deal with circular references.
NS_IMETHODIMP
nsHTMLTextAreaElement::SetForm(nsIDOMHTMLFormElement* aForm)
{
  nsresult result = NS_OK;
  if (nsnull == aForm) {
    mForm = nsnull;
    return NS_OK;
  } else {
    NS_IF_RELEASE(mForm);
    nsIFormControl* formControl = nsnull;
    result = QueryInterface(kIFormControlIID, (void**)&formControl);
    if ((NS_OK == result) && formControl) {
      result = aForm->QueryInterface(kIFormIID, (void**)&mForm); // keep the ref
      if ((NS_OK == result) && mForm) {
        mForm->AddElement(formControl);
      }
      NS_RELEASE(formControl);
    }
  }
  return result;
}

