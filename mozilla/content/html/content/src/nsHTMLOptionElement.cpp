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
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsIDOMNSHTMLOptionCollection.h"
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
#include "nsIDOMText.h"
#include "nsITextContent.h"
#include "nsIDOMNode.h"
#include "nsGenericElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIJSNativeInitializer.h"
#include "nsISelectElement.h"
#include "nsISelectControlFrame.h"
#include "nsIComboboxControlFrame.h"

// Notify/query select frame for selected state
#include "nsIFormControlFrame.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsCOMPtr.h"


static NS_DEFINE_IID(kIDOMHTMLSelectElementIID, NS_IDOMHTMLSELECTELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLOptionElementIID, NS_IDOMHTMLOPTIONELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLOptGroupElementIID, NS_IDOMHTMLOPTIONELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLFormElementIID, NS_IDOMHTMLFORMELEMENT_IID);
static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kISelectElementIID, NS_ISELECTELEMENT_IID);
static NS_DEFINE_IID(kIFormIID, NS_IFORM_IID);
static NS_DEFINE_IID(kIFormControlFrameIID, NS_IFORMCONTROLFRAME_IID); 
static NS_DEFINE_IID(kIJSNativeInitializerIID, NS_IJSNATIVEINITIALIZER_IID);

class nsHTMLOptionElement : public nsIDOMHTMLOptionElement,
                            public nsIJSScriptObject,
                            public nsIHTMLContent,
                            public nsIJSNativeInitializer
                            //public nsIFormControl
{
public:
  nsHTMLOptionElement(nsIAtom* aTag);
  virtual ~nsHTMLOptionElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLOptionElement
  NS_DECL_IDOMHTMLOPTIONELEMENT

  // nsIJSScriptObject
  NS_IMPL_IJSSCRIPTOBJECT_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_NO_SETPARENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

  // nsIJSNativeInitializer
  NS_IMETHOD Initialize(JSContext* aContext, JSObject *aObj, 
                        PRUint32 argc, jsval *argv);

protected:
  nsGenericHTMLContainerElement mInner;

  // Get the primary frame associated with this content
  nsresult GetPrimaryFrame(nsIFormControlFrame *&aFormControlFrame, PRBool aFlushNotifications = PR_TRUE);

  // Get the select content element that contains this option
  nsresult GetSelect(nsIDOMHTMLSelectElement *&aSelectElement);
};

nsresult
NS_NewHTMLOptionElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLOptionElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}


nsHTMLOptionElement::nsHTMLOptionElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
}

nsHTMLOptionElement::~nsHTMLOptionElement()
{
}

// ISupports

NS_IMPL_ADDREF(nsHTMLOptionElement)
NS_IMPL_RELEASE(nsHTMLOptionElement)

nsresult
nsHTMLOptionElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLOptionElementIID)) {
    *aInstancePtr = (void*)(nsIDOMHTMLOptionElement*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIJSNativeInitializerIID)) {
    nsIJSNativeInitializer* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }                                                             
  return NS_NOINTERFACE;
}

// the option has a ref to the form, but not vice versa. The form can get to the
// options via the select.

NS_IMETHODIMP 
nsHTMLOptionElement::SetParent(nsIContent* aParent)
{
  nsresult result = NS_OK;

  // Remove us from our old select element
  if (nsnull != mInner.mParent) {
    nsIDOMHTMLSelectElement* oldSelectElement = nsnull;
    GetSelect(oldSelectElement);
    if (nsnull != oldSelectElement) {
      nsISelectElement* sel;
      
      if (NS_SUCCEEDED(oldSelectElement->QueryInterface(kISelectElementIID, (void**)&sel))) {
        sel->RemoveOption(this);
        NS_RELEASE(sel);
      }

      NS_RELEASE(oldSelectElement);
    }
  }

  result = mInner.SetParent(aParent);

  if (nsnull != aParent) {
    nsIDOMHTMLSelectElement* newSelectElement = nsnull;
    GetSelect(newSelectElement);
    if (nsnull != newSelectElement) {
      nsISelectElement* sel;
      
      if (NS_SUCCEEDED(newSelectElement->QueryInterface(kISelectElementIID, (void**)&sel))) {
        sel->AddOption(this);
        NS_RELEASE(sel);
      }

      NS_RELEASE(newSelectElement);
    }
  }

  return result;
}

nsresult
nsHTMLOptionElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsHTMLOptionElement* it = new nsHTMLOptionElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner, aDeep);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMETHODIMP
nsHTMLOptionElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  nsIDOMHTMLSelectElement* selectElement = nsnull;
  nsresult res = GetSelect(selectElement);
  if (NS_OK == res) {
    nsIFormControl* selectControl = nsnull;
    res = selectElement->QueryInterface(kIFormControlIID, (void**)&selectControl);
    NS_RELEASE(selectElement);

    if (NS_OK == res) {
      res = selectControl->GetForm(aForm);
      NS_RELEASE(selectControl);
    }
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLOptionElement::GetSelected(PRBool* aValue) 
{
  nsIFormControlFrame* formControlFrame = nsnull;
  nsresult rv = GetPrimaryFrame(formControlFrame);
  if (NS_SUCCEEDED(rv)) {
    PRInt32 indx;
    if (NS_OK == GetIndex(&indx)) {
      nsString value;
      value.AppendInt(indx, 10); // Save the index in base 10
      formControlFrame->GetProperty(nsHTMLAtoms::selected, value);
      *aValue = value.EqualsWithConversion("1");
    }
  }
  return rv;
}

NS_IMETHODIMP 
nsHTMLOptionElement::SetSelected(PRBool aValue)
{
  nsIFormControlFrame* fcFrame = nsnull;
  nsresult result = GetPrimaryFrame(fcFrame, PR_FALSE);
  if (NS_SUCCEEDED(result) && (nsnull != fcFrame)) {
    nsISelectControlFrame* selectFrame = nsnull;
    result = fcFrame->QueryInterface(NS_GET_IID(nsISelectControlFrame),(void **) &selectFrame);
    if (NS_SUCCEEDED(result) && (nsnull != selectFrame)) {
      PRInt32 indx;
      result = GetIndex(&indx);
      if (NS_SUCCEEDED(result)) {
        selectFrame->SetOptionSelected(indx, aValue);
      }
    }
  }
  return result;
}

//NS_IMPL_BOOL_ATTR(nsHTMLOptionElement, DefaultSelected, defaultselected)
//NS_IMPL_INT_ATTR(nsHTMLOptionElement, Index, index)
NS_IMPL_BOOL_ATTR(nsHTMLOptionElement, Disabled, disabled)
//NS_IMPL_STRING_ATTR(nsHTMLOptionElement, Label, label)
NS_IMPL_STRING_ATTR(nsHTMLOptionElement, Value, value)

NS_IMETHODIMP                                                      
nsHTMLOptionElement::GetLabel(nsString& aValue)                             
{                                                                  
  mInner.GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::label, aValue);                                                  
  return NS_OK;
}         
                                                         
NS_IMETHODIMP                                                      
nsHTMLOptionElement::SetLabel(const nsString& aValue)                       
{                                                                  
  nsresult result = mInner.SetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::label, aValue, PR_TRUE); 
  if (NS_SUCCEEDED(result)) {
    nsIFormControlFrame* fcFrame = nsnull;
    result = GetPrimaryFrame(fcFrame);
    if (NS_SUCCEEDED(result) && (nsnull != fcFrame)) {
      nsIComboboxControlFrame* selectFrame = nsnull;
      result = fcFrame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame),(void **) &selectFrame);
      if (NS_SUCCEEDED(result) && (nsnull != selectFrame)) {
        selectFrame->UpdateSelection(PR_FALSE, PR_TRUE, 0);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLOptionElement::GetDefaultSelected(PRBool* aDefaultSelected)
{
  nsHTMLValue val;                                                 
  nsresult rv = mInner.GetHTMLAttribute(nsHTMLAtoms::selected, val);
  *aDefaultSelected = (NS_CONTENT_ATTR_NOT_THERE != rv);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionElement::SetDefaultSelected(PRBool aDefaultSelected)
{
  nsresult rv = NS_OK;
  nsHTMLValue empty(eHTMLUnit_Empty);
  if (aDefaultSelected) {
    rv = mInner.SetHTMLAttribute(nsHTMLAtoms::selected, empty, PR_TRUE);
  } else {
    rv = mInner.UnsetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::selected, PR_TRUE);
  }
  if (NS_SUCCEEDED(rv)) {
    // When setting DefaultSelected, we must also reset Selected (DOM Errata)
    rv = SetSelected(aDefaultSelected);
  }
  return rv;
}

NS_IMETHODIMP 
nsHTMLOptionElement::GetIndex(PRInt32* aIndex)
{
  nsresult res = NS_ERROR_FAILURE;
  *aIndex = -1; // -1 indicates the index was not found

  // Get our nsIDOMNode interface to compare apples to apples.
  nsIDOMNode* thisNode = nsnull;
  if (NS_OK == this->QueryInterface(kIDOMNodeIID, (void**)&thisNode)) {

    // Get our containing select content object.
    nsIDOMHTMLSelectElement* selectElement = nsnull;
    if (NS_OK == GetSelect(selectElement)) {

      // Get the options from the select object.
      nsIDOMNSHTMLOptionCollection* options = nsnull;
      if (NS_OK == selectElement->GetOptions(&options)) {

        // Walk the options to find out where we are in the list (ick, O(n))
        PRUint32 length = 0;
        options->GetLength(&length);
        nsIDOMNode* thisOption = nsnull;
        for (PRUint32 i = 0; i < length; i++) {
          options->Item(i, &thisOption);
          if (thisNode == thisOption) {
            res = NS_OK;
            *aIndex = i;
            NS_IF_RELEASE(thisOption);
            break; // skips the release below, thus release above needed
          }
          NS_IF_RELEASE(thisOption);
        }
        NS_RELEASE(options);
      }
      NS_RELEASE(selectElement);
    }
    NS_RELEASE(thisNode);
  }
  return res;
}

NS_IMETHODIMP
nsHTMLOptionElement::StringToAttribute(nsIAtom* aAttribute,
                                const nsString& aValue,
                                nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::selected) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::disabled) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLOptionElement::AttributeToString(nsIAtom* aAttribute,
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
nsHTMLOptionElement::GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                              PRInt32& aHint) const
{
  if (aAttribute == nsHTMLAtoms::label) {
    aHint = NS_STYLE_HINT_REFLOW; 
  } else if (aAttribute == nsHTMLAtoms::text) {
    aHint = NS_STYLE_HINT_REFLOW; 
  } else if (! nsGenericHTMLElement::GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    aHint = NS_STYLE_HINT_CONTENT;
  }  

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLOptionElement::GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                                  nsMapAttributesFunc& aMapFunc) const
{
  aFontMapFunc = nsnull;
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLOptionElement::HandleDOMEvent(nsIPresContext* aPresContext,
                             nsEvent* aEvent,
                             nsIDOMEvent** aDOMEvent,
                             PRUint32 aFlags,
                             nsEventStatus* aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}

NS_IMETHODIMP
nsHTMLOptionElement::GetText(nsString& aText)
{
  aText.SetLength(0);
  PRInt32 numNodes;
  nsresult rv = mInner.ChildCount(numNodes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  for (PRInt32 i = 0; i < numNodes; i++) {
    nsIContent* node = nsnull;
    rv = ChildAt(i, node);
    if (NS_SUCCEEDED(rv) && node) {
      nsCOMPtr<nsIDOMText> domText;
      rv = node->QueryInterface(kIDOMTextIID, (void**)getter_AddRefs(domText));
      if (NS_SUCCEEDED(rv) && domText) {
        rv = domText->GetData(aText);
        // the option could be all spaces, so compress the white space
        // then make sure the length is greater than zero
        if (aText.Length() > 0) { 
          nsAutoString compressText = aText;
          compressText.CompressWhitespace(PR_TRUE, PR_TRUE);
          if (compressText.Length() != 0) {
            aText = compressText;
          }
        }
        NS_RELEASE(node);
        break;
      }
      NS_RELEASE(node);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionElement::SetText(const nsString& aText)
{
  PRInt32 numNodes;
  PRBool usedExistingTextNode = PR_FALSE;  // Do we need to create a text node?
  nsresult result = mInner.ChildCount(numNodes);
  if (NS_FAILED(result)) {
    return result;
  }

  for (PRInt32 i = 0; i < numNodes; i++) {
    nsIContent * node = nsnull;
    result = ChildAt(i, node);
    if (NS_SUCCEEDED(result) && node) {
      nsCOMPtr<nsIDOMText> domText;
      result = node->QueryInterface(kIDOMTextIID, (void**)getter_AddRefs(domText));
      if (NS_SUCCEEDED(result) && domText) {
        result = domText->SetData(aText);
        if (NS_SUCCEEDED(result)) {
          usedExistingTextNode = PR_TRUE;
        }
        NS_RELEASE(node);
        break;
      }
      NS_RELEASE(node);
    }
  }

  if (!usedExistingTextNode) {
    nsCOMPtr<nsIContent> text;
    result = NS_NewTextNode(getter_AddRefs(text));
    if (NS_OK == result) {
      nsIDOMText* domtext;
      result = text->QueryInterface(kIDOMTextIID, (void**)&domtext);
      if (NS_SUCCEEDED(result) && domtext) {
        result = domtext->SetData(aText);
	      if (NS_SUCCEEDED(result)) {
          result = AppendChildTo(text, PR_FALSE);
          if (NS_SUCCEEDED(result)) {
            nsIDocument * doc;
            result = GetDocument(doc);
            if (NS_SUCCEEDED(result)) {
              text->SetDocument(doc, PR_FALSE);
              NS_IF_RELEASE(doc);
            }
          }
        }
        NS_RELEASE(domtext);
      }
    }
  }

  if (NS_SUCCEEDED(result)) {
    nsIFormControlFrame* fcFrame = nsnull;
    result = GetPrimaryFrame(fcFrame);
    if (NS_SUCCEEDED(result) && (nsnull != fcFrame)) {
      nsIComboboxControlFrame* selectFrame = nsnull;
      result = fcFrame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame),(void **) &selectFrame);
      if (NS_SUCCEEDED(result) && (nsnull != selectFrame)) {
        selectFrame->UpdateSelection(PR_FALSE, PR_TRUE, 0);
      }
    }
  }
  return NS_OK;
}

// Options don't have frames - get the select content node
// then call nsGenericHTMLElement::GetPrimaryFrame()
nsresult nsHTMLOptionElement::GetPrimaryFrame(nsIFormControlFrame *&aIFormControlFrame, PRBool aFlushNotifications)
{
  nsIDOMHTMLSelectElement* selectElement = nsnull;
  nsresult res = GetSelect(selectElement);
  if (NS_OK == res) {
    nsIHTMLContent* selectContent = nsnull;
    nsresult gotContent = selectElement->QueryInterface(kIContentIID, (void**)&selectContent);
    NS_RELEASE(selectElement);

    if (NS_OK == gotContent) {
      res = nsGenericHTMLElement::GetPrimaryFrame(selectContent, aIFormControlFrame, aFlushNotifications);
      NS_RELEASE(selectContent);
    }
  }
  return res;
}

// Get the select content element that contains this option
nsresult nsHTMLOptionElement::GetSelect(nsIDOMHTMLSelectElement *&aSelectElement)
{
  // Get the containing element (Either a select or an optGroup)
  nsIDOMNode* parentNode = nsnull;
  nsresult res = NS_ERROR_FAILURE;
  if (NS_OK == this->GetParentNode(&parentNode)) {
      aSelectElement = nsnull;
      if (nsnull != parentNode) {
        
        res = parentNode->QueryInterface(kIDOMHTMLSelectElementIID, (void**)&aSelectElement);

        // If we are in an OptGroup we need to GetParentNode again (at least once)
        if (NS_OK != res) {
          nsIDOMHTMLOptGroupElement* optgroupElement = nsnull;
          while (1) { // Be ready for nested OptGroups
            if ((nsnull != parentNode) && (NS_OK == parentNode->QueryInterface(kIDOMHTMLOptGroupElementIID, (void**)&optgroupElement))) {
              NS_RELEASE(optgroupElement); // Don't need the optgroup, just seeing if it IS one.
              nsIDOMNode* grandParentNode = nsnull;
              if (NS_OK == parentNode->GetParentNode(&grandParentNode)) {
                NS_RELEASE(parentNode);
                parentNode = grandParentNode;
              } else {
                break; // Break out if we can't get our parent (we're screwed)
              }
            } else {
              break; // Break out if not a OptGroup (hopefully we have a select)
            }
          }
          res = parentNode->QueryInterface(kIDOMHTMLSelectElementIID, (void**)&aSelectElement);
        }

        // We have a select if we're gonna get one, so let go of the generic node
        NS_RELEASE(parentNode);
      }
  }

  return res;
}

NS_IMETHODIMP    
nsHTMLOptionElement::Initialize(JSContext* aContext, 
                                JSObject *aObj,
                                PRUint32 argc, 
                                jsval *argv)
{
  nsresult result = NS_OK;

  if (argc > 0) {
    // The first (optional) parameter is the text of the option
    JSString* jsstr = JS_ValueToString(aContext, argv[0]);
    if (nsnull != jsstr) {
      // Create a new text node and append it to the option
      nsCOMPtr<nsIContent> textNode;
      nsITextContent* content;

      result = NS_NewTextNode(getter_AddRefs(textNode));
      if (NS_FAILED(result)) {
        return result;
      }
      
      result = textNode->QueryInterface(kITextContentIID, (void**)&content);
      if (NS_FAILED(result)) {
        return result;
      }

      result = content->SetText(JS_GetStringChars(jsstr),
                                JS_GetStringLength(jsstr),
                                PR_FALSE);
      NS_RELEASE(content);
      if (NS_FAILED(result)) {
        return result;
      }
      
      // this addrefs textNode:
      result = mInner.AppendChildTo(textNode, PR_FALSE);
      if (NS_FAILED(result)) {
        return result;
      }
    }

    if (argc > 1) {
      // The second (optional) parameter is the value of the option
      jsstr = JS_ValueToString(aContext, argv[1]);
      if (nsnull != jsstr) {
        // Set the value attribute for this element
        nsAutoString value(JS_GetStringChars(jsstr));

        result = mInner.SetAttribute(kNameSpaceID_HTML,
                                     nsHTMLAtoms::value,
                                     value,
                                     PR_FALSE);
        if (NS_FAILED(result)) {
          return result;
        }
      }

      if (argc > 2) {
        // The third (optional) parameter is the defaultSelected value
        JSBool defaultSelected;
        if ((JS_TRUE == JS_ValueToBoolean(aContext,
                                         argv[2],
                                         &defaultSelected)) &&
            (JS_TRUE == defaultSelected)) {
          nsHTMLValue empty(eHTMLUnit_Empty);
          result = mInner.SetHTMLAttribute(nsHTMLAtoms::selected, 
                                           empty, 
                                           PR_FALSE);
          if (NS_FAILED(result)) {
            return result;
          }          
        }

        // XXX Since we don't store the selected state, we can't deal
        // with the fourth (optional) parameter that is meant to specify
        // whether the option element should be currently selected or
        // not. Does anyone depend on this behavior?
      }
    }
  }

  return result;
}

NS_IMETHODIMP
nsHTMLOptionElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  return mInner.SizeOf(aSizer, aResult, sizeof(*this));
}
