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
 */
#include "nsCOMPtr.h"
#include "nsIDOMHTMLLabelElement.h"
#include "nsIDOMHTMLFormElement.h"
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
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsISizeOfHandler.h"
#include "nsIFormControlFrame.h"
#include "nsIPresShell.h"

static NS_DEFINE_IID(kIDOMHTMLLabelElementIID, NS_IDOMHTMLLABELELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLFormElementIID, NS_IDOMHTMLFORMELEMENT_IID);
static NS_DEFINE_IID(kIFormIID, NS_IFORM_IID);
static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);

class nsHTMLLabelElement : public nsIDOMHTMLLabelElement,
                  public nsIJSScriptObject,
                  public nsIHTMLContent,
                  public nsIFormControl

{
public:
  nsHTMLLabelElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLLabelElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement, because of the "htmlFor" attribute handling we can't
  // use the NS_IMPL_IDOMELEMENT_USING_GENERIC macro here...
  NS_IMETHOD GetTagName(nsString& aTagName) {
    return mInner.GetTagName(aTagName);
  }
  NS_IMETHOD GetAttribute(const nsString& aName, nsString& aReturn) {
    if (aName.EqualsIgnoreCase("htmlfor")) {
      return mInner.GetAttribute(NS_ConvertASCIItoUCS2("for"), aReturn);
    }
    return mInner.GetAttribute(aName, aReturn);
  }
  NS_IMETHOD SetAttribute(const nsString& aName, const nsString& aValue) {
    if (aName.EqualsIgnoreCase("htmlfor")) {
      return mInner.SetAttribute(NS_ConvertASCIItoUCS2("for"), aValue);
    }
    return mInner.SetAttribute(aName, aValue);
  }
  NS_IMETHOD RemoveAttribute(const nsString& aName) {
    if (aName.EqualsIgnoreCase("htmlfor")) {
      return mInner.RemoveAttribute(NS_ConvertASCIItoUCS2("for"));
    }
    return mInner.RemoveAttribute(aName);
  }
  NS_IMETHOD GetAttributeNode(const nsString& aName,
                              nsIDOMAttr** aReturn) {
    if (aName.EqualsIgnoreCase("htmlfor")) {
      return mInner.GetAttributeNode(NS_ConvertASCIItoUCS2("for"), aReturn);
    }
    return mInner.GetAttributeNode(aName, aReturn);
  }
  NS_IMETHOD SetAttributeNode(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn) {
    return mInner.SetAttributeNode(aNewAttr, aReturn);
  }
  NS_IMETHOD RemoveAttributeNode(nsIDOMAttr* aOldAttr, nsIDOMAttr** aReturn) {
    return mInner.RemoveAttributeNode(aOldAttr, aReturn);
  }
  NS_IMETHOD GetElementsByTagName(const nsString& aTagname,
                                  nsIDOMNodeList** aReturn) {
    return mInner.GetElementsByTagName(aTagname, aReturn);
  }
  NS_IMETHOD GetAttributeNS(const nsString& aNamespaceURI,
                            const nsString& aLocalName, nsString& aReturn) {
    return mInner.GetAttributeNS(aNamespaceURI, aLocalName, aReturn);
  }
  NS_IMETHOD SetAttributeNS(const nsString& aNamespaceURI,
                            const nsString& aQualifiedName,
                            const nsString& aValue) {
    return mInner.SetAttributeNS(aNamespaceURI, aQualifiedName, aValue);
  }
  NS_IMETHOD RemoveAttributeNS(const nsString& aNamespaceURI,
                               const nsString& aLocalName) {
    return mInner.RemoveAttributeNS(aNamespaceURI, aLocalName);
  }
  NS_IMETHOD GetAttributeNodeNS(const nsString& aNamespaceURI,
                                const nsString& aLocalName,
                                nsIDOMAttr** aReturn) {
    return mInner.GetAttributeNodeNS(aNamespaceURI, aLocalName, aReturn);
  }
  NS_IMETHOD SetAttributeNodeNS(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn) {
    return mInner.SetAttributeNodeNS(aNewAttr, aReturn);
  }
  NS_IMETHOD GetElementsByTagNameNS(const nsString& aNamespaceURI,
                                    const nsString& aLocalName,
                                    nsIDOMNodeList** aReturn) {
    return mInner.GetElementsByTagNameNS(aNamespaceURI, aLocalName, aReturn);
  }
  NS_IMETHOD HasAttribute(const nsString& aName, PRBool* aReturn) {
    return HasAttribute(aName, aReturn);
  }
  NS_IMETHOD HasAttributeNS(const nsString& aNamespaceURI,
                            const nsString& aLocalName, PRBool* aReturn) {
    return mInner.HasAttributeNS(aNamespaceURI, aLocalName, aReturn);
  }

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLLabelElement
  NS_IMETHOD GetForm(nsIDOMHTMLFormElement** aForm);
  NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm);
  NS_IMETHOD GetAccessKey(nsString& aAccessKey);
  NS_IMETHOD SetAccessKey(const nsString& aAccessKey);
  NS_IMETHOD GetHtmlFor(nsString& aHtmlFor);
  NS_IMETHOD SetHtmlFor(const nsString& aHtmlFor);

  // nsIJSScriptObject
  NS_IMPL_IJSSCRIPTOBJECT_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_NO_SETPARENT_NO_SETDOCUMENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

  // nsIFormControl
  NS_IMETHOD GetType(PRInt32* aType) { return NS_FORM_LABEL; }
  NS_IMETHOD Init() { return NS_OK; }

protected:
  nsGenericHTMLContainerFormElement mInner;
  nsIForm*                          mForm;
};

// construction, destruction
nsresult
NS_NewHTMLLabelElement(nsIHTMLContent** aInstancePtrResult,
                       nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);
  NS_ENSURE_ARG_POINTER(aNodeInfo);

  nsIHTMLContent* it = new nsHTMLLabelElement(aNodeInfo);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}


nsHTMLLabelElement::nsHTMLLabelElement(nsINodeInfo *aNodeInfo)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aNodeInfo);
  mForm = nsnull;
}

nsHTMLLabelElement::~nsHTMLLabelElement()
{
  if (nsnull != mForm) {
    // prevent mForm from decrementing its ref count on us
    mForm->RemoveElement(this, PR_FALSE); 
    NS_RELEASE(mForm);
  }
}

// nsISupports 

NS_IMPL_ADDREF(nsHTMLLabelElement)

NS_IMETHODIMP_(nsrefcnt)
nsHTMLLabelElement::Release()
{
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "nsHTMLLabelElement");
	if (mRefCnt <= 0) {
    delete this;                                       
    return 0;                                          
  } else if ((1 == mRefCnt) && mForm) {     
    NS_LOG_RELEASE(this, mRefCnt, "nsHTMLLabelElement");
    delete this;
    return 0;
  } else {
    return mRefCnt;
  }
}

nsresult
nsHTMLLabelElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLLabelElementIID)) {
    *aInstancePtr = (void*)(nsIDOMHTMLLabelElement*)this;
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

// nsIDOMHTMLLabelElement

nsresult
nsHTMLLabelElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsHTMLLabelElement* it = new nsHTMLLabelElement(mInner.mNodeInfo);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner, aDeep);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

// nsIContent

NS_IMETHODIMP
nsHTMLLabelElement::SetParent(nsIContent* aParent)
{
  return mInner.SetParentForFormControls(aParent, this, mForm);
}

NS_IMETHODIMP
nsHTMLLabelElement::SetDocument(nsIDocument* aDocument, PRBool aDeep, PRBool aCompileEventHandlers)
{
  return mInner.SetDocumentForFormControls(aDocument, aDeep, aCompileEventHandlers, this, mForm);
}

NS_IMETHODIMP
nsHTMLLabelElement::GetForm(nsIDOMHTMLFormElement** aForm)
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

// An important assumption is that if aForm is null, the previous mForm will not be released
// This allows nsHTMLFormElement to deal with circular references.
NS_IMETHODIMP
nsHTMLLabelElement::SetForm(nsIDOMHTMLFormElement* aForm)
{
  nsresult result;
  nsIFormControl *formControl;

  result = QueryInterface(kIFormControlIID, (void**)&formControl);
  if (NS_FAILED(result))
    formControl = nsnull;

  if (mForm && formControl)
    mForm->RemoveElement(formControl, PR_TRUE);

  if (nsnull == aForm)
    mForm = nsnull;
  else {
    NS_IF_RELEASE(mForm);
    if (formControl) {
      result = aForm->QueryInterface(kIFormIID, (void**)&mForm); // keep the ref
      if ((NS_OK == result) && mForm) {
        mForm->AddElement(formControl);
      }
    }
  }
  NS_IF_RELEASE(formControl);

  mInner.SetForm(mForm);
  
  return result;
}

NS_IMPL_STRING_ATTR(nsHTMLLabelElement, AccessKey, accesskey)
//NS_IMPL_STRING_ATTR(nsHTMLLabelElement, HtmlFor, _for)

NS_IMETHODIMP
nsHTMLLabelElement::GetHtmlFor(nsString& aValue)
{
  mInner.GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::_for, aValue);                 
  return NS_OK;                                                    
}  

NS_IMETHODIMP
nsHTMLLabelElement::SetHtmlFor(const nsString& aValue)
{
  // trim leading and trailing whitespace 
  static char whitespace[] = " \r\n\t";
  nsAutoString value(aValue);
  value.Trim(whitespace, PR_TRUE, PR_TRUE);
  return mInner.SetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::_for, value, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLLabelElement::StringToAttribute(nsIAtom* aAttribute,
                                      const nsString& aValue,
                                      nsHTMLValue& aResult)
{
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLLabelElement::AttributeToString(nsIAtom* aAttribute,
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
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext, aPresContext);
}

NS_IMETHODIMP
nsHTMLLabelElement::GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                             PRInt32& aHint) const
{
  if (! nsGenericHTMLElement::GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    aHint = NS_STYLE_HINT_CONTENT;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLLabelElement::GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                                 nsMapAttributesFunc& aMapFunc) const
{
  aFontMapFunc = nsnull;
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLabelElement::HandleDOMEvent(nsIPresContext* aPresContext,
                                   nsEvent* aEvent,
                                   nsIDOMEvent** aDOMEvent,
                                   PRUint32 aFlags,
                                   nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  nsresult rv = mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);

  // Now a little special trickery because we are a label:
  // We need to pass this event on to our child iff it is a focus,
  // keypress/up/dn, mouseclick/dblclick/up/down.
  if ((NS_OK == rv) && (NS_EVENT_FLAG_INIT & aFlags) &&
      ((nsEventStatus_eIgnore == *aEventStatus) ||
       (nsEventStatus_eConsumeNoDefault == *aEventStatus)) ) {
    PRBool isFormElement = PR_FALSE;
    nsCOMPtr<nsIHTMLContent> node; // Node we are a label for
    switch (aEvent->message) {
      case NS_FOCUS_CONTENT:
      case NS_KEY_PRESS:
      case NS_KEY_UP:
      case NS_KEY_DOWN:
      case NS_MOUSE_LEFT_CLICK:
      case NS_MOUSE_LEFT_DOUBLECLICK:
      case NS_MOUSE_LEFT_BUTTON_UP:
      case NS_MOUSE_LEFT_BUTTON_DOWN:
      case NS_MOUSE_MIDDLE_CLICK:
      case NS_MOUSE_MIDDLE_DOUBLECLICK:
      case NS_MOUSE_MIDDLE_BUTTON_UP:
      case NS_MOUSE_MIDDLE_BUTTON_DOWN:
      case NS_MOUSE_RIGHT_CLICK:
      case NS_MOUSE_RIGHT_DOUBLECLICK:
      case NS_MOUSE_RIGHT_BUTTON_UP:
      case NS_MOUSE_RIGHT_BUTTON_DOWN:
      {
        // Get the element that this label is for
        nsAutoString elementId;
        rv = GetHtmlFor(elementId);
        if (NS_SUCCEEDED(rv) && elementId.Length()) { // --- We have a FOR attr
          nsCOMPtr<nsIDocument> iDoc;
          rv = mInner.GetDocument(*getter_AddRefs(iDoc));
          if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIDOMElement> domElement;
// XXX This should be merged into nsIDocument
            nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(iDoc, &rv);
            if (NS_SUCCEEDED(rv) && htmlDoc) {
              rv = htmlDoc->GetElementById(elementId, getter_AddRefs(domElement));
            }
#ifdef INCLUDE_XUL
            else {
              nsCOMPtr<nsIDOMXULDocument> xulDoc = do_QueryInterface(iDoc, &rv);
              if (NS_SUCCEEDED(rv) && xulDoc) {
                rv = xulDoc->GetElementById(elementId, getter_AddRefs(domElement));
              }
            }
#endif // INCLUDE_XUL
            if (NS_SUCCEEDED(rv) && domElement) {
              // Get our grubby paws on the content interface
              node = do_QueryInterface(domElement, &rv);

              // Find out of this is a form element.
              if (NS_SUCCEEDED(rv)) {
                nsIFormControlFrame* control;
                nsresult gotFrame = nsGenericHTMLElement::GetPrimaryFrame(node, control);
                isFormElement = NS_SUCCEEDED(gotFrame) && control;
              }
            }
          }
        } else { // --- No FOR attribute, we are a label for our first child element
          PRInt32 numNodes;
          rv = mInner.ChildCount(numNodes);
          if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIContent> contNode;
	    PRInt32 i;
            for (i = 0; NS_SUCCEEDED(rv) && !isFormElement && (i < numNodes); i++) {
              rv = ChildAt(i, *getter_AddRefs(contNode));
              if (NS_SUCCEEDED(rv) && contNode) {
                // We need to make sure this child is a form element
                nsresult isHTMLContent = PR_FALSE;
                node = do_QueryInterface(contNode, &isHTMLContent);
                if (NS_SUCCEEDED(isHTMLContent) && node) {
                  // Find out of this is a form element.
                  nsIFormControlFrame* control;
                  nsresult gotFrame = nsGenericHTMLElement::GetPrimaryFrame(node, control);
                  isFormElement = NS_SUCCEEDED(gotFrame) && control;
                }
              }
            }
          }
        }
      } // Close should handle
    } // Close switch

    // If we found an element, pass along the event to it.
    if (NS_SUCCEEDED(rv) && node) {
      // Only pass along event if this is a form element
      if (isFormElement) {
        rv = node->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, aFlags, aEventStatus);
      }
    }
  } // Close trickery
  return rv;
}

NS_IMETHODIMP
nsHTMLLabelElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
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
