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
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsIFormManager.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNSHTMLFormElement.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMNSHTMLFormControlList.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsIMutableStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"   
#include "nsIFrame.h"
#include "nsIFormControlFrame.h"
#include "nsISizeOfHandler.h"
#include "nsIScriptGlobalObject.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"
#include "nsHashtable.h"
#include "nsContentList.h"

static const int NS_FORM_CONTROL_LIST_HASHTABLE_SIZE = 16;

class nsFormControlList;

// nsHTMLFormElement

class nsHTMLFormElement : public nsGenericHTMLContainerElement,
                          public nsIDOMHTMLFormElement,
                          public nsIDOMNSHTMLFormElement,
                          public nsIForm
{
public:
  nsHTMLFormElement();
  virtual ~nsHTMLFormElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLContainerElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLFormElement
  NS_DECL_NSIDOMHTMLFORMELEMENT

  // nsIDOMNSHTMLFormElement
  NS_DECL_NSIDOMNSHTMLFORMELEMENT  

  // nsIForm
  NS_IMETHOD AddElement(nsIFormControl* aElement);
  NS_IMETHOD AddElementToTable(nsIFormControl* aChild,
                               const nsAReadableString& aName);
  NS_IMETHOD GetElementAt(PRInt32 aIndex, nsIFormControl** aElement) const;
  NS_IMETHOD GetElementCount(PRUint32* aCount) const;
  NS_IMETHOD RemoveElement(nsIFormControl* aElement);
  NS_IMETHOD RemoveElementFromTable(nsIFormControl* aElement,
                                    const nsAReadableString& aName);
  NS_IMETHOD ResolveName(const nsAReadableString& aName,
                         nsISupports **aReturn);
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;

  // nsIContent
  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAWritableString& aResult) const;
  NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext, nsEvent* aEvent,
                            nsIDOMEvent** aDOMEvent, PRUint32 aFlags,
                            nsEventStatus* aEventStatus);

protected:
  nsFormControlList*       mControls;
};

// nsFormControlList
class nsFormControlList : public nsIDOMNSHTMLFormControlList,
                          public nsIDOMHTMLCollection
{
public:
  nsFormControlList(nsIDOMHTMLFormElement* aForm);
  virtual ~nsFormControlList();

  void Clear();
  void SetForm(nsIDOMHTMLFormElement* aForm);

  NS_DECL_ISUPPORTS

  // nsIDOMHTMLCollection interface
  NS_DECL_NSIDOMHTMLCOLLECTION

  // nsIDOMNSHTMLFormControlList interface
  NS_DECL_NSIDOMNSHTMLFORMCONTROLLIST

  nsresult GetNamedObject(const nsAReadableString& aName,
                          nsISupports **aResult);

  nsresult AddElementToTable(nsIFormControl* aChild,
                             const nsAReadableString& aName);
  nsresult RemoveElementFromTable(nsIFormControl* aChild,
                                  const nsAReadableString& aName);

#ifdef DEBUG
  nsresult SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
#endif

  nsIDOMHTMLFormElement* mForm;  // WEAK - the form owns me

  nsVoidArray mElements;  // Holds WEAK references - bug 36639

  // This hash holds on to all form controls that are not named form
  // control (see IsNamedFormControl()), this is needed to properly
  // clean up the bi-directional references (both weak and strong)
  // between the form and its form controls.

  nsHashtable* mNoNameLookupTable; // Holds WEAK references

protected:
  // A map from an ID or NAME attribute to the form control(s), this
  // hash holds strong references either to the named form control, or
  // to a list of named form controls, in the case where this hash
  // holds on to a list of named form controls the list has weak
  // references to the form control.

  nsSupportsHashtable mNameLookupTable;
};

static PRBool
IsNamedFormControl(nsIFormControl* aFormControl)
{
  PRInt32 type;

  aFormControl->GetType(&type);

  // For backwards compatibility (with 4.x and IE) we must not add
  // <input type=image> elements to the list of form controls in a
  // form.

  switch (type) {
  case NS_FORM_BUTTON_BUTTON :
  case NS_FORM_BUTTON_RESET :
  case NS_FORM_BUTTON_SUBMIT :
  case NS_FORM_INPUT_BUTTON :
  case NS_FORM_INPUT_CHECKBOX :
  case NS_FORM_INPUT_FILE :
  case NS_FORM_INPUT_HIDDEN :
  case NS_FORM_INPUT_RESET :
  case NS_FORM_INPUT_PASSWORD :
  case NS_FORM_INPUT_RADIO :
  case NS_FORM_INPUT_SUBMIT :
  case NS_FORM_INPUT_TEXT :
  case NS_FORM_SELECT :
  case NS_FORM_TEXTAREA :
  case NS_FORM_FIELDSET :
    return PR_TRUE;
  }

  // These form control types are not supposed to end up in the
  // form.elements array
  //
  // NS_FORM_INPUT_IMAGE
  // NS_FORM_LABEL
  // NS_FORM_OPTION
  // NS_FORM_OPTGROUP
  // NS_FORM_LEGEND

  return PR_FALSE;
}

// nsHTMLFormElement implementation

// construction, destruction
nsresult
NS_NewHTMLFormElement(nsIHTMLContent** aInstancePtrResult,
                      nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLFormElement* it = new nsHTMLFormElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = it->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    delete it;

    return rv;
  }

  *aInstancePtrResult = NS_STATIC_CAST(nsIHTMLContent *, it);
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}


nsHTMLFormElement::nsHTMLFormElement()
{
  mControls = new nsFormControlList(this);

  NS_IF_ADDREF(mControls);
}



nsHTMLFormElement::~nsHTMLFormElement()
{
  if (mControls) {
    mControls->Clear();
    mControls->SetForm(nsnull);

    NS_RELEASE(mControls);
  }
}


// nsISupports

NS_IMPL_ADDREF_INHERITED(nsHTMLFormElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLFormElement, nsGenericElement) 


// XPConnect interface list for nsHTMLFormElement
NS_CLASSINFO_MAP_BEGIN(HTMLFormElement)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMHTMLFormElement)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLFormElement)
  NS_CLASSINFO_MAP_ENTRY_FUNCTION(GetGenericHTMLElementIIDs)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for nsHTMLFormElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLFormElement,
                                    nsGenericHTMLContainerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLFormElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLFormElement)
  NS_INTERFACE_MAP_ENTRY(nsIForm)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLFormElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


// nsIDOMHTMLFormElement

nsresult
nsHTMLFormElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLFormElement* it = new nsHTMLFormElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIDOMNode> kungFuDeathGrip(it);

  nsresult rv = it->Init(mNodeInfo);

  if (NS_FAILED(rv))
    return rv;

  CopyInnerTo(this, it, aDeep);

  *aReturn = NS_STATIC_CAST(nsIDOMNode *, it);

  NS_ADDREF(*aReturn);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::GetElements(nsIDOMHTMLCollection** aElements)
{
  *aElements = mControls;
  NS_ADDREF(mControls);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::GetName(nsAWritableString& aValue)
{
  return nsGenericHTMLContainerElement::GetAttribute(kNameSpaceID_HTML,
                                                     nsHTMLAtoms::name,
                                                     aValue);
}

NS_IMETHODIMP
nsHTMLFormElement::SetName(const nsAReadableString& aValue)
{
  return nsGenericHTMLContainerElement::SetAttribute(kNameSpaceID_HTML,
                                                     nsHTMLAtoms::name,
                                                     aValue, PR_TRUE);
}

NS_IMPL_STRING_ATTR(nsHTMLFormElement, AcceptCharset, acceptcharset)
NS_IMPL_STRING_ATTR(nsHTMLFormElement, Action, action)
NS_IMPL_STRING_ATTR(nsHTMLFormElement, Enctype, enctype)
NS_IMPL_STRING_ATTR(nsHTMLFormElement, Method, method)
NS_IMPL_STRING_ATTR(nsHTMLFormElement, Target, target)

NS_IMETHODIMP
nsHTMLFormElement::Submit()
{
  // Generate submit event
  nsresult rv = NS_OK;
  nsCOMPtr<nsIPresContext> presContext;
  GetPresContext(this, getter_AddRefs(presContext));
  if (presContext) {
    nsFormEvent event;
    event.eventStructType = NS_FORM_EVENT;
    event.message         = NS_FORM_SUBMIT;
    event.originator      = nsnull;
    nsEventStatus status  = nsEventStatus_eIgnore;
    rv = HandleDOMEvent(presContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
  }
  return rv;
}

NS_IMETHODIMP
nsHTMLFormElement::Reset()
{
  // Generate reset event
  nsresult rv = NS_OK;
  nsCOMPtr<nsIPresContext> presContext;
  GetPresContext(this, getter_AddRefs(presContext));
  if (presContext) {
    nsFormEvent event;
    event.eventStructType = NS_FORM_EVENT;
    event.message         = NS_FORM_RESET;
    event.originator      = nsnull;
    nsEventStatus status  = nsEventStatus_eIgnore;
    rv = HandleDOMEvent(presContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
  }
  return rv;
}

static nsGenericHTMLElement::EnumTable kFormMethodTable[] = {
  { "get", NS_FORM_METHOD_GET },
  { "post", NS_FORM_METHOD_POST },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kFormEnctypeTable[] = {
  { "multipart/form-data", NS_FORM_ENCTYPE_MULTIPART },
  { "application/x-www-form-urlencoded", NS_FORM_ENCTYPE_URLENCODED },
  { 0 }
};

NS_IMETHODIMP
nsHTMLFormElement::StringToAttribute(nsIAtom* aAttribute,
                                     const nsAReadableString& aValue,
                                     nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::method) {
    if (ParseEnumValue(aValue, kFormMethodTable, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::enctype) {
    if (ParseEnumValue(aValue, kFormEnctypeTable, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLFormElement::AttributeToString(nsIAtom* aAttribute,
                                     const nsHTMLValue& aValue,
                                     nsAWritableString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::method) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      EnumValueToString(aValue, kFormMethodTable, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::enctype) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      EnumValueToString(aValue, kFormEnctypeTable, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return nsGenericHTMLContainerElement::AttributeToString(aAttribute,
                                                          aValue, aResult);
}

NS_IMETHODIMP
nsHTMLFormElement::HandleDOMEvent(nsIPresContext* aPresContext,
                                  nsEvent* aEvent,
                                  nsIDOMEvent** aDOMEvent,
                                  PRUint32 aFlags,
                                  nsEventStatus* aEventStatus)
{
  nsresult ret = nsGenericHTMLContainerElement::HandleDOMEvent(aPresContext,
                                                               aEvent,
                                                               aDOMEvent,
                                                               aFlags,
                                                               aEventStatus);

  if ((NS_OK == ret) && (nsEventStatus_eIgnore == *aEventStatus) &&
      !(aFlags & NS_EVENT_FLAG_CAPTURE)) {

    switch (aEvent->message) {
      case NS_FORM_RESET:
      case NS_FORM_SUBMIT:
      {
        // Make sure the presentation is up-to-date
        nsCOMPtr<nsIDocument> doc;
        GetDocument(*getter_AddRefs(doc));
        if (doc) {
          doc->FlushPendingNotifications();
        }

        nsCOMPtr<nsIPresShell> shell;
        aPresContext->GetShell(getter_AddRefs(shell));
        if (shell) {
          nsIFrame* frame;
          shell->GetPrimaryFrameFor(this, &frame);
          if (frame) {
            nsIFormManager* formMan = nsnull;
            ret = frame->QueryInterface(NS_GET_IID(nsIFormManager),
                                        (void**)&formMan);
            if (formMan) {
              if (NS_FORM_RESET == aEvent->message) {
                ret = formMan->OnReset(aPresContext);
              }
              else {
                nsIFrame *originatingFrame = nsnull;

                // Get the originating frame (failure is non-fatal)
                if (aEvent) {
                  if (NS_FORM_EVENT == aEvent->eventStructType) {
                    nsIContent *originator = ((nsFormEvent *)aEvent)->originator;
                    if (originator) {
                      shell->GetPrimaryFrameFor(originator, &originatingFrame);
                    }
                  }
                }

                ret = formMan->OnSubmit(aPresContext, originatingFrame);
              }
            }
          }
        }
      }
      break;
    }
  }

  return ret;
}

// nsIForm

NS_IMETHODIMP
nsHTMLFormElement::GetElementCount(PRUint32* aCount) const 
{
  mControls->GetLength(aCount); 
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLFormElement::GetElementAt(PRInt32 aIndex,
                                nsIFormControl** aFormControl) const 
{ 
  *aFormControl = (nsIFormControl*) mControls->mElements.ElementAt(aIndex);
  NS_IF_ADDREF(*aFormControl);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::AddElement(nsIFormControl* aChild)
{
  NS_ENSURE_TRUE(mControls, NS_ERROR_UNEXPECTED);

  if (IsNamedFormControl(aChild)) {
    mControls->mElements.AppendElement(aChild);
    // WEAK - don't addref
  } else {
    if (!mControls->mNoNameLookupTable) {
      mControls->mNoNameLookupTable = new nsHashtable();
      NS_ENSURE_TRUE(mControls->mNoNameLookupTable, NS_ERROR_OUT_OF_MEMORY);
    }

    nsISupportsKey key(aChild);

    nsISupports *item =
      NS_STATIC_CAST(nsISupports *, mControls->mNoNameLookupTable->Get(&key));

    if (!item) {
      mControls->mNoNameLookupTable->Put(&key, aChild);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::AddElementToTable(nsIFormControl* aChild,
                                     const nsAReadableString& aName)
{
  NS_ENSURE_TRUE(mControls, NS_ERROR_UNEXPECTED);

  return mControls->AddElementToTable(aChild, aName);  
}


NS_IMETHODIMP 
nsHTMLFormElement::RemoveElement(nsIFormControl* aChild) 
{
  NS_ENSURE_TRUE(mControls, NS_ERROR_UNEXPECTED);

  mControls->mElements.RemoveElement(aChild);

  if (mControls->mNoNameLookupTable) {
    nsISupportsKey key(aChild);

    mControls->mNoNameLookupTable->Remove(&key);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::RemoveElementFromTable(nsIFormControl* aElement,
                                          const nsAReadableString& aName)
{
  NS_ENSURE_TRUE(mControls, NS_ERROR_UNEXPECTED);

  return mControls->RemoveElementFromTable(aElement, aName);
}

NS_IMETHODIMP
nsHTMLFormElement::ResolveName(const nsAReadableString& aName,
                               nsISupports **aResult)
{
  return mControls->GetNamedObject(aName, aResult);
}


NS_IMETHODIMP
nsHTMLFormElement::GetEncoding(nsAWritableString& aEncoding)
{
  return GetEnctype(aEncoding);
}
 
NS_IMETHODIMP
nsHTMLFormElement::SetEncoding(const nsAReadableString& aEncoding)
{
  return SetEnctype(aEncoding);
}
 
NS_IMETHODIMP    
nsHTMLFormElement::GetLength(PRInt32* aLength)
{
  *aLength = mControls->mElements.Count();
  
  return NS_OK;
}

//----------------------------------------------------------------------

// nsFormControlList implementation, this could go away if there were
// a lightweight collection implementation somewhere

nsFormControlList::nsFormControlList(nsIDOMHTMLFormElement* aForm)
  : mForm(aForm), mNameLookupTable(NS_FORM_CONTROL_LIST_HASHTABLE_SIZE),
    mNoNameLookupTable(nsnull)
{
  NS_INIT_REFCNT();
}

nsFormControlList::~nsFormControlList()
{
  delete mNoNameLookupTable;
  mNoNameLookupTable = nsnull;

  mForm = nsnull;
  Clear();
}

void
nsFormControlList::SetForm(nsIDOMHTMLFormElement* aForm)
{
  mForm = aForm; // WEAK - the form owns me
}

static PRBool PR_CALLBACK
FormControlResetEnumFunction(nsHashKey *aKey, void *aData, void* closure)
{
  nsIFormControl *f = NS_STATIC_CAST(nsIFormControl *, aData);

  f->SetForm(nsnull, PR_FALSE);

  return PR_TRUE;
}

void
nsFormControlList::Clear()
{
  // Null out childrens' pointer to me.  No refcounting here
  PRInt32 numControls = mElements.Count();

  while (numControls-- > 0) {
    nsIFormControl* f = NS_STATIC_CAST(nsIFormControl *,
                                       mElements.ElementAt(numControls)); 

    if (f) {
      f->SetForm(nsnull, PR_FALSE); 
    }
  }

  mElements.Clear();

  mNameLookupTable.Reset();

  if (mNoNameLookupTable) {
    mNoNameLookupTable->Reset(FormControlResetEnumFunction);
  }
}


// XPConnect interface list for nsFormControlList
NS_CLASSINFO_MAP_BEGIN(HTMLFormControlCollection)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLFormControlList)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMHTMLCollection)
NS_CLASSINFO_MAP_END


// XPConnect interface list for nsFormControlList
NS_INTERFACE_MAP_BEGIN(nsFormControlList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLFormControlList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLCollection)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMHTMLCollection)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_WITH_NAME(HTMLFormControlCollection,
                                                     HTMLCollection)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsFormControlList)
NS_IMPL_RELEASE(nsFormControlList)


// nsIDOMHTMLCollection interface

NS_IMETHODIMP    
nsFormControlList::GetLength(PRUint32* aLength)
{
  *aLength = mElements.Count();
  return NS_OK;
}

NS_IMETHODIMP
nsFormControlList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsIFormControl *control = (nsIFormControl*)mElements.ElementAt(aIndex);

  if (control) {
    return CallQueryInterface(control, aReturn);
  }

  *aReturn = nsnull;

  return NS_OK;
}

nsresult
nsFormControlList::GetNamedObject(const nsAReadableString& aName,
                                  nsISupports** aResult)
{
  *aResult = nsnull;

  if (!mForm) {
    // No form, no named objects
    return NS_OK;
  }
  
  nsresult rv = NS_OK;

  // Get the hash entry
  nsStringKey key(aName);

  *aResult = mNameLookupTable.Get(&key);

  return NS_OK;
}

NS_IMETHODIMP 
nsFormControlList::NamedItem(const nsAReadableString& aName,
                             nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsresult rv = NS_OK;

  nsStringKey key(aName);

  nsCOMPtr<nsISupports> supports(dont_AddRef(mNameLookupTable.Get(&key)));

  if (supports) {
    // We found something, check if it's a node
    CallQueryInterface(supports, aReturn);

    if (!*aReturn) {
      // If not, we check if it's a node list.
      nsCOMPtr<nsIDOMNodeList> nodeList(do_QueryInterface(supports));
      NS_WARN_IF_FALSE(nodeList, "Huh, what's going one here?");

      if (nodeList) {
        // And since we're only asking for one node here, we return the first
        // one from the list.
        rv = nodeList->Item(0, aReturn);
      }
    }
  }

  return rv;
}

NS_IMETHODIMP
nsFormControlList::NamedItem(const nsAReadableString& aName,
                             nsISupports** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  nsStringKey key(aName);

  *aReturn = mNameLookupTable.Get(&key);

  return NS_OK;
}

nsresult
nsFormControlList::AddElementToTable(nsIFormControl* aChild,
                                     const nsAReadableString& aName)
{
  if (!IsNamedFormControl(aChild)) {
    if (!mNoNameLookupTable) {
      mNoNameLookupTable = new nsHashtable();
      NS_ENSURE_TRUE(mNoNameLookupTable, NS_ERROR_OUT_OF_MEMORY);
    }

    nsISupportsKey key(aChild);

    nsISupports *item = NS_STATIC_CAST(nsISupports *,
                                       mNoNameLookupTable->Get(&key));

    if (!item) {
      mNoNameLookupTable->Put(&key, aChild);
    }

    return NS_OK;
  }

  nsStringKey key(aName);

  nsCOMPtr<nsISupports> supports;
  supports = dont_AddRef(mNameLookupTable.Get(&key));

  if (!supports) {
    // No entry found, add the form control
    nsCOMPtr<nsISupports> child(do_QueryInterface(aChild));

    mNameLookupTable.Put(&key, child);
  } else {
    // Found something in the hash, check its type
    nsCOMPtr<nsIContent> content(do_QueryInterface(supports));
    nsCOMPtr<nsIContent> newChild(do_QueryInterface(aChild));

    if (content) {
      // Check if the new content is the same as the one we found in the
      // hash, if it is then we leave it in the hash as it is, this will
      // happen if a form control has both a name and an id with the same
      // value
      if (content == newChild) {
        return NS_OK;
      }

      // Found an element, create a list, add the element to the list and put
      // the list in the hash
      nsContentList *list = new nsContentList(nsnull);
      NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

      list->AppendElement(content);

      // Add the new child too
      list->AppendElement(newChild);

      nsCOMPtr<nsISupports> listSupports;
      list->QueryInterface(NS_GET_IID(nsISupports),
                           getter_AddRefs(listSupports));

      // Replace the element with the list.
      mNameLookupTable.Put(&key, listSupports);
    } else {
      // There's already a list in the hash, add the child to the list
      nsCOMPtr<nsIDOMNodeList> nodeList(do_QueryInterface(supports));
      NS_ENSURE_TRUE(nodeList, NS_ERROR_FAILURE);

      // Upcast, uggly, but it works!
      nsContentList *list = NS_STATIC_CAST(nsContentList *,
                                           (nsIDOMNodeList *)nodeList.get());

      PRInt32 oldIndex = -1;
      list->IndexOf(newChild, oldIndex);

      // Add the new child only if it's not in our list already
      if (oldIndex < 0) {
        list->AppendElement(newChild);
      }
    }
  }

  return NS_OK;
}

nsresult
nsFormControlList::RemoveElementFromTable(nsIFormControl* aChild,
                                          const nsAReadableString& aName)
{
  if (!IsNamedFormControl(aChild)) {
    if (mNoNameLookupTable) {
      nsISupportsKey key(aChild);

      mNoNameLookupTable->Remove(&key);
    }

    return NS_OK;
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(aChild);  
  if (!content) {
    return NS_OK;
  }

  nsStringKey key(aName);

  nsCOMPtr<nsISupports> supports(dont_AddRef(mNameLookupTable.Get(&key)));

  if (!supports)
    return NS_OK;

  nsCOMPtr<nsIFormControl> fctrl(do_QueryInterface(supports));

  if (fctrl) {
    // Single element in the hash, just remove it...
    mNameLookupTable.Remove(&key);

    return NS_OK;
  }

  nsCOMPtr<nsIDOMNodeList> nodeList(do_QueryInterface(supports));
  NS_ENSURE_TRUE(nodeList, NS_ERROR_FAILURE);

  // Upcast, uggly, but it works!
  nsContentList *list = NS_STATIC_CAST(nsContentList *,
                                       (nsIDOMNodeList *)nodeList.get());

  list->RemoveElement(content);

  PRUint32 length = 0;
  list->GetLength(&length);

  if (!length) {
    // If the list is empty we remove if from our hash, this shouldn't
    // happen tho
    mNameLookupTable.Remove(&key);
  } else if (length == 1) {
    // Only one element left, replace the list in the hash with the
    // single element.
    nsCOMPtr<nsIDOMNode> node;
    list->Item(0, getter_AddRefs(node));

    if (node) {
      nsCOMPtr<nsISupports> tmp(do_QueryInterface(node));
      mNameLookupTable.Put(&key, tmp.get());
    }
  }

  return NS_OK;
}

#ifdef DEBUG
nsresult
nsFormControlList::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  if (!aResult) return NS_ERROR_NULL_POINTER;
  PRUint32 asize;
  mElements.SizeOf(aSizer, &asize);
  *aResult = sizeof(*this) - sizeof(mElements) + asize;
  return NS_OK;
}
#endif

NS_IMETHODIMP
nsHTMLFormElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}
