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
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsIControllers.h"
#include "nsIEditorController.h"
#include "nsRDFCID.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
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
#include "nsIMutableStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIHTMLAttributes.h"
#include "nsIFormControlFrame.h"
#include "nsIEventStateManager.h"
#include "nsISizeOfHandler.h"
#include "nsLinebreakConverter.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"

static NS_DEFINE_IID(kIDOMHTMLTextAreaElementIID, NS_IDOMHTMLTEXTAREAELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLFormElementIID, NS_IDOMHTMLFORMELEMENT_IID);
static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kIFormIID, NS_IFORM_IID);
static NS_DEFINE_CID(kXULControllersCID,  NS_XULCONTROLLERS_CID);

class nsHTMLTextAreaElement : public nsIDOMHTMLTextAreaElement,
                              public nsIDOMNSHTMLTextAreaElement,
                              public nsIJSScriptObject,
                              public nsIHTMLContent,
                              public nsIFormControl
{
public:
  nsHTMLTextAreaElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLTextAreaElement();

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

  // nsIDOMNSHTMLTextAreaElement
  NS_DECL_IDOMNSHTMLTEXTAREAELEMENT

  // nsIJSScriptObject
  NS_IMPL_IJSSCRIPTOBJECT_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_NO_SETPARENT_NO_SETDOCUMENT_NO_FOCUS_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

  // nsIFormControl
  NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm);
  NS_IMETHOD GetType(PRInt32* aType);
  NS_IMETHOD Init() { return NS_OK; }

protected:
  nsGenericHTMLContainerFormElement mInner;
  nsIForm*   mForm;
  nsCOMPtr<nsIControllers> mControllers;

  NS_IMETHOD SelectAll(nsIPresContext* aPresContext);
};

nsresult
NS_NewHTMLTextAreaElement(nsIHTMLContent** aInstancePtrResult,
                          nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);
  NS_ENSURE_ARG_POINTER(aNodeInfo);

  nsIHTMLContent* it = new nsHTMLTextAreaElement(aNodeInfo);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}


nsHTMLTextAreaElement::nsHTMLTextAreaElement(nsINodeInfo *aNodeInfo)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aNodeInfo);
  mForm = nsnull;
}

nsHTMLTextAreaElement::~nsHTMLTextAreaElement()
{
  // Null out form's pointer to us - no ref counting here!
  SetForm(nsnull);
}

NS_IMPL_ADDREF(nsHTMLTextAreaElement)
NS_IMPL_RELEASE(nsHTMLTextAreaElement)

nsresult
nsHTMLTextAreaElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLTextAreaElementIID)) {
    *aInstancePtr = (void*)(nsIDOMHTMLTextAreaElement*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMNSHTMLTextAreaElement))) {
    *aInstancePtr = (void*)(nsIDOMNSHTMLTextAreaElement*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if (aIID.Equals(kIFormControlIID)) {
    *aInstancePtr = (void*)(nsIFormControl*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

// nsIDOMHTMLTextAreaElement

nsresult
nsHTMLTextAreaElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsHTMLTextAreaElement* it = new nsHTMLTextAreaElement(mInner.mNodeInfo);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner, aDeep);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

// nsIContent

NS_IMETHODIMP
nsHTMLTextAreaElement::SetParent(nsIContent* aParent)
{
  return mInner.SetParentForFormControls(aParent, this, mForm);
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetDocument(nsIDocument* aDocument, PRBool aDeep, PRBool aCompileEventHandlers)
{
  return mInner.SetDocumentForFormControls(aDocument, aDeep, aCompileEventHandlers, this, mForm);
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
  nsIFormControlFrame* formControlFrame = nsnull;
  nsresult rv = nsGenericHTMLElement::GetPrimaryFrame(this, formControlFrame);
  if (NS_SUCCEEDED(rv)) {
     // Ask the frame to Deselect focus (i.e Blur).
    formControlFrame->SetFocus(PR_FALSE, PR_TRUE);
    return NS_OK;
  }
  return rv;

}

NS_IMETHODIMP
nsHTMLTextAreaElement::Focus() 
{
  nsCOMPtr<nsIDocument> doc; // Strong
  nsresult rv = GetDocument(*getter_AddRefs(doc));
  if (NS_SUCCEEDED(rv) && doc) {

    PRInt32 numShells = doc->GetNumberOfShells();
    nsCOMPtr<nsIPresContext> context;
    for (PRInt32 i=0; i<numShells; i++) 
    {
      nsCOMPtr<nsIPresShell> shell = getter_AddRefs(doc->GetShellAt(i));
      if (!shell) { rv = NS_ERROR_NULL_POINTER; break; }

      rv = shell->GetPresContext(getter_AddRefs(context));
      if (NS_FAILED(rv)) { break; }
      if (!context) { rv = NS_ERROR_NULL_POINTER; break; }

      rv = SetFocus(context);
      if (NS_FAILED(rv)) { break; }
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetFocus(nsIPresContext* aPresContext)
{
  // first see if we are disabled or not. If disabled then do nothing.
  nsAutoString disabled;
  if (NS_CONTENT_ATTR_HAS_VALUE == mInner.GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::disabled, disabled)) {
    return NS_OK;
  }

  nsIEventStateManager* esm;
  if (NS_OK == aPresContext->GetEventStateManager(&esm)) {
    esm->SetContentState(this, NS_EVENT_STATE_FOCUS);
    NS_RELEASE(esm);
  }

  nsIFormControlFrame* formControlFrame = nsnull;
  nsresult rv = nsGenericHTMLElement::GetPrimaryFrame(this, formControlFrame);
  if (NS_SUCCEEDED(rv)) {
    formControlFrame->SetFocus(PR_TRUE, PR_TRUE);
    formControlFrame->ScrollIntoView(aPresContext);
    // Could call SelectAll(aPresContext) here to automatically
    // select text when we receive focus.
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::RemoveFocus(nsIPresContext* aPresContext)
{
  // XXX Should focus only this presContext
  Blur();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::Select()
{
  nsresult rv = NS_OK;

  // first see if we are disabled or not. If disabled then do nothing.
  nsAutoString disabled;
  if (NS_CONTENT_ATTR_HAS_VALUE == mInner.GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::disabled, disabled)) {
    return rv;
  }

  // Currently we have to give focus to the text field before we
  // can select text in it.  :S

  // Just like SetFocus() but without the ScrollIntoView()!
  nsCOMPtr<nsIPresContext> presContext;
  nsGenericHTMLElement::GetPresContext(this, getter_AddRefs(presContext));
  nsCOMPtr<nsIEventStateManager> esm;
  rv = presContext->GetEventStateManager(getter_AddRefs(esm));
  if (NS_SUCCEEDED(rv)) {
    rv = esm->SetContentState(this, NS_EVENT_STATE_FOCUS);

    if (NS_SUCCEEDED(rv)) {
      nsIFormControlFrame* formControlFrame = nsnull;
      rv = nsGenericHTMLElement::GetPrimaryFrame(this, formControlFrame);
      if (NS_SUCCEEDED(rv)) {
        formControlFrame->SetFocus(PR_TRUE, PR_TRUE);
      }
    }
  }

  // Now Select all the text!
  if (NS_SUCCEEDED(rv)) {
    rv = SelectAll(presContext);
  }
  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SelectAll(nsIPresContext* aPresContext)
{
  nsIFormControlFrame* formControlFrame = nsnull;
  nsresult rv = nsGenericHTMLElement::GetPrimaryFrame(this, formControlFrame);
  if (NS_SUCCEEDED(rv)) {
    if (formControlFrame )
    {
      formControlFrame->SetProperty(aPresContext, nsHTMLAtoms::select, nsAutoString());
      return NS_OK;
    }
  }
  return rv;
}

NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, AccessKey, accesskey)
NS_IMPL_INT_ATTR(nsHTMLTextAreaElement, Cols, cols)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, Disabled, disabled)
NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, ReadOnly, readonly)
NS_IMPL_INT_ATTR(nsHTMLTextAreaElement, Rows, rows)
NS_IMPL_INT_ATTR(nsHTMLTextAreaElement, TabIndex, tabindex)
  

NS_IMETHODIMP 
nsHTMLTextAreaElement::GetType(nsString& aType)
{
  aType.AssignWithConversion("textarea");
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLTextAreaElement::GetValue(nsString& aValue)
{
  nsIFormControlFrame* formControlFrame = nsnull;
  if (NS_OK == nsGenericHTMLElement::GetPrimaryFrame(this, formControlFrame)) {
      formControlFrame->GetProperty(nsHTMLAtoms::value, aValue);
      return NS_OK;
  }
   //XXX: Should this ASSERT instead of getting the default value here?
  return mInner.GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::value, aValue); 
}


NS_IMETHODIMP 
nsHTMLTextAreaElement::SetValue(const nsString& aValue)
{
  nsIFormControlFrame* formControlFrame = nsnull;
  if (NS_OK == nsGenericHTMLElement::GetPrimaryFrame(this, formControlFrame)) {
    nsIPresContext* presContext;
    nsGenericHTMLElement::GetPresContext(this, &presContext);
    formControlFrame->SetProperty(presContext, nsHTMLAtoms::value, aValue);
    NS_IF_RELEASE(presContext);
  }
  // Set the attribute in the DOM too, we call SetAttribute with aNotify
  // false so that we don't generate unnecessary reflows.
  mInner.SetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::value, aValue, PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetDefaultValue(nsString& aDefaultValue)
{
  mInner.GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::defaultvalue,
                      aDefaultValue);

  return NS_OK;                                                    
}  

NS_IMETHODIMP
nsHTMLTextAreaElement::SetDefaultValue(const nsString& aDefaultValue)
{
  // trim leading whitespace. -- why?
  static char whitespace[] = " \r\n\t";
  nsString defaultValue(aDefaultValue);
  defaultValue.Trim(whitespace, PR_TRUE, PR_FALSE);

  // normalize line breaks. Need this e.g. when the value is
  // coming from a URL, which used platform line breaks.
  nsLinebreakConverter::ConvertStringLineBreaks(defaultValue,
       nsLinebreakConverter::eLinebreakAny, nsLinebreakConverter::eLinebreakContent);
  
  mInner.SetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::defaultvalue, defaultValue, PR_TRUE);
  mInner.SetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::value, defaultValue, PR_TRUE);
  return NS_OK;

}

NS_IMETHODIMP
nsHTMLTextAreaElement::StringToAttribute(nsIAtom* aAttribute,
                                         const nsString& aValue,
                                         nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::disabled) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::cols) {
    if (nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::readonly) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::rows) {
    if (nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::tabindex) {
    if (nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::AttributeToString(nsIAtom* aAttribute,
                                         const nsHTMLValue& aValue,
                                         nsString& aResult) const
{
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

static void
MapAttributesInto(const nsIHTMLMappedAttributes* aAttributes,
                  nsIMutableStyleContext* aContext,
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
nsHTMLTextAreaElement::GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                                PRInt32& aHint) const
{
  if (aAttribute == nsHTMLAtoms::align) {
    aHint = NS_STYLE_HINT_REFLOW;
  }
  else if (! nsGenericHTMLElement::GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    aHint = NS_STYLE_HINT_CONTENT;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                                    nsMapAttributesFunc& aMapFunc) const
{
  aFontMapFunc = nsnull;
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTextAreaElement::HandleDOMEvent(nsIPresContext* aPresContext,
                                      nsEvent* aEvent,
                                      nsIDOMEvent** aDOMEvent,
                                      PRUint32 aFlags,
                                      nsEventStatus* aEventStatus)
{
  // Do not process any DOM events if the element is disabled
  PRBool disabled;
  nsresult rv = GetDisabled(&disabled);
  if (NS_FAILED(rv) || disabled) {
    return rv;
  }

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
nsHTMLTextAreaElement::SetForm(nsIDOMHTMLFormElement* aForm)
{
  nsCOMPtr<nsIFormControl> formControl;
  nsresult result = QueryInterface(kIFormControlIID, getter_AddRefs(formControl));
  if (NS_FAILED(result)) formControl = nsnull;

  nsAutoString nameVal, idVal;
  mInner.GetAttribute(kNameSpaceID_None, nsHTMLAtoms::name, nameVal);
  mInner.GetAttribute(kNameSpaceID_None, nsHTMLAtoms::id, idVal);

  if (mForm && formControl) {
    mForm->RemoveElement(formControl);

    if (nameVal.Length())
      mForm->RemoveElementFromTable(this, nameVal);

    if (idVal.Length())
      mForm->RemoveElementFromTable(this, idVal);
  }

  if (aForm) {
    nsCOMPtr<nsIForm> theForm = do_QueryInterface(aForm, &result);
    mForm = theForm;  // Even if we fail, update mForm (nsnull in failure)
    if ((NS_OK == result) && theForm) {
      if (formControl) {
        theForm->AddElement(formControl);

        if (nameVal.Length())
          theForm->AddElementToTable(this, nameVal);

        if (idVal.Length())
          theForm->AddElementToTable(this, idVal);
      }
    }
  } else {
    mForm = nsnull;
  }

  mInner.SetForm(mForm);

  return result;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  if (!aResult) return NS_ERROR_NULL_POINTER;
#ifdef DEBUG
  mInner.SizeOf(aSizer, aResult, sizeof(*this));
  if (mForm) {
    PRBool recorded;
    aSizer->RecordObject(mForm, &recorded);
    if (!recorded) {
      PRUint32 formSize;
      mForm->SizeOf(aSizer, &formSize);
      aSizer->AddSize(nsHTMLAtoms::iform, formSize);
    }
  }
#endif
  return NS_OK;
}


// Controllers Methods
NS_IMETHODIMP
nsHTMLTextAreaElement::GetControllers(nsIControllers** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  if (!mControllers)
  {
    NS_ENSURE_SUCCESS (
      nsComponentManager::CreateInstance(kXULControllersCID,
                                         nsnull,
                                         NS_GET_IID(nsIControllers),
                                         getter_AddRefs(mControllers)),
      NS_ERROR_FAILURE);
    if (!mControllers) { return NS_ERROR_NULL_POINTER; }

    nsresult rv;
    nsCOMPtr<nsIController> controller = do_CreateInstance("component://netscape/editor/editorcontroller", &rv);
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIEditorController> editorController = do_QueryInterface(controller, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = editorController->Init(nsnull);
    if (NS_FAILED(rv)) return rv;
    
    mControllers->AppendController(controller);
  }

  *aResult = mControllers;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}
