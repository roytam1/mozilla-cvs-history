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
#include "nsGenericHTMLElement.h"

#include "nsIAtom.h"
#include "nsIContentDelegate.h"
#include "nsICSSParser.h"
#include "nsICSSStyleRule.h"
#include "nsICSSDeclaration.h"
#include "nsIDocument.h"
#include "nsIDOMAttribute.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIEventListenerManager.h"
#include "nsIHTMLAttributes.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIHTMLDocument.h"
#include "nsIHTMLContent.h"
#include "nsILinkHandler.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsISizeOfHandler.h"
#include "nsIStyleContext.h"
#include "nsIStyleRule.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsStyleConsts.h"
#include "nsXIFConverter.h"
#include "nsFrame.h"
#include "nsIPresShell.h"
#include "nsIView.h"
#include "nsIViewManager.h"

#include "nsHTMLParts.h"
#include "nsString.h"
#include "nsHTMLAtoms.h"
#include "nsDOMEventsIIDs.h"
#include "nsBlockFrame.h"
#include "nsInlineFrame.h"
#include "nsIEventStateManager.h"
#include "nsDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsBodyFrame.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIDOMScriptObjectFactory.h"
#include "prprf.h"

// XXX todo: add in missing out-of-memory checks

NS_DEFINE_IID(kIDOMNodeIID, NS_IDOMNODE_IID);
NS_DEFINE_IID(kIDOMElementIID, NS_IDOMELEMENT_IID);
NS_DEFINE_IID(kIDOMHTMLElementIID, NS_IDOMHTMLELEMENT_IID);
NS_DEFINE_IID(kIDOMEventReceiverIID, NS_IDOMEVENTRECEIVER_IID);
NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
NS_DEFINE_IID(kIContentIID, NS_ICONTENT_IID);
//NS_DEFINE_IID(kIHTMLContentIID, NS_IHTMLCONTENT_IID);

static NS_DEFINE_IID(kIContentDelegateIID, NS_ICONTENTDELEGATE_IID);
static NS_DEFINE_IID(kIDOMAttributeIID, NS_IDOMATTRIBUTE_IID);
static NS_DEFINE_IID(kIDOMNamedNodeMapIID, NS_IDOMNAMEDNODEMAP_IID);
static NS_DEFINE_IID(kIPrivateDOMEventIID, NS_IPRIVATEDOMEVENT_IID);
static NS_DEFINE_IID(kIStyleRuleIID, NS_ISTYLE_RULE_IID);
static NS_DEFINE_IID(kIHTMLDocumentIID, NS_IHTMLDOCUMENT_IID);
static NS_DEFINE_IID(kICSSStyleRuleIID, NS_ICSS_STYLE_RULE_IID);
static NS_DEFINE_IID(kIDOMNodeListIID, NS_IDOMNODELIST_IID);

static nsIContentDelegate* gContentDelegate;

/**
 * THE html content delegate. There is exactly one instance of this
 * class and it's used for all html content. It just turns around
 * and asks the content object to create the frame.
 */
class ZContentDelegate : public nsIContentDelegate {
public:
  ZContentDelegate();
  NS_DECL_ISUPPORTS
  NS_IMETHOD CreateFrame(nsIPresContext* aPresContext,
                         nsIContent* aContent,
                         nsIFrame* aParentFrame,
                         nsIStyleContext* aStyleContext,
                         nsIFrame*& aResult);
protected:
  ~ZContentDelegate();
};

// Attribute helper class used to wrap up an attribute with a dom
// object that implements nsIDOMAttribute and nsIDOMNode and
// nsIScriptObjectOwner
class DOMAttribute : public nsIDOMAttribute, public nsIScriptObjectOwner {
public:
  DOMAttribute(const nsString &aName, const nsString &aValue);
  ~DOMAttribute();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetScriptObject(nsIScriptContext* aContext, void** aScriptObject);
  NS_IMETHOD ResetScriptObject();

  // nsIDOMAttribute interface
  NS_IMETHOD GetSpecified(PRBool* aSpecified);
  NS_IMETHOD SetSpecified(PRBool aSpecified);
  NS_IMETHOD GetName(nsString& aReturn);
  NS_IMETHOD GetValue(nsString& aReturn);

  // nsIDOMNode interface
  NS_IMETHOD GetNodeName(nsString& aNodeName);
  NS_IMETHOD GetNodeValue(nsString& aNodeValue);
  NS_IMETHOD SetNodeValue(const nsString& aNodeValue);
  NS_IMETHOD GetNodeType(PRInt32* aNodeType);
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode);
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes);
  NS_IMETHOD GetHasChildNodes(PRBool* aHasChildNodes);
  NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild);
  NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild);
  NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling);
  NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling);
  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes);
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                          nsIDOMNode** aReturn);
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                          nsIDOMNode** aReturn);
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);
  NS_IMETHOD AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn);
  NS_IMETHOD CloneNode(nsIDOMNode** aReturn);
  NS_IMETHOD Equals(nsIDOMNode* aNode, PRBool aDeep, PRBool* aReturn);

private:
  nsString mName;
  nsString mValue;
  void* mScriptObject;
};

// Another helper class that implements the nsIDOMNamedNodeMap interface.
class DOMAttributeMap : public nsIDOMNamedNodeMap,
                        public nsIScriptObjectOwner
{
public:
  DOMAttributeMap(nsIHTMLContent* aContent);
  virtual ~DOMAttributeMap();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetScriptObject(nsIScriptContext* aContext, void** aScriptObject);
  NS_IMETHOD ResetScriptObject();

  // nsIDOMNamedNodeMap interface
  NS_IMETHOD GetLength(PRUint32* aSize);
  NS_IMETHOD GetNamedItem(const nsString& aName, nsIDOMNode** aReturn);
  NS_IMETHOD SetNamedItem(nsIDOMNode* aNode);
  NS_IMETHOD RemoveNamedItem(const nsString& aName, nsIDOMNode** aReturn);
  NS_IMETHOD Item(PRUint32 aIndex, nsIDOMNode** aReturn);

private:
  nsIHTMLContent* mContent;
  void* mScriptObject;
};


// Class that holds the child list of a content element and also
// implements the nsIDOMNodeList interface.
class nsChildContentList : public nsIDOMNodeList, 
                           public nsIScriptObjectOwner 
{
public:
  nsChildContentList();
  virtual ~nsChildContentList() {}
  
  NS_DECL_ISUPPORTS
  
  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD ResetScriptObject();
  
  // nsIDOMNodeList interface
  NS_DECL_IDOMNODELIST
  
  // Methods stolen from nsVoidArray
  PRInt32 Count() { return mArray.Count(); }
  nsIContent* ElementAt(PRInt32 aIndex) const 
  { return (nsIContent*)mArray.ElementAt(aIndex); }
  PRBool InsertElementAt(nsIContent* aElement, PRInt32 aIndex)
  { return mArray.InsertElementAt(aElement, aIndex); }
  PRBool ReplaceElementAt(nsIContent* aElement, PRInt32 aIndex)
  { return mArray.ReplaceElementAt(aElement, aIndex); }
  PRBool AppendElement(nsIContent* aElement)
  { return mArray.AppendElement(aElement); }
  PRBool RemoveElementAt(PRInt32 aIndex)
  { return mArray.RemoveElementAt(aIndex); }
  PRInt32 IndexOf(nsIContent* aPossibleElement) const
  { return mArray.IndexOf(aPossibleElement); }
  void Compact() { mArray.Compact(); }

private:
  nsVoidArray mArray;
  void *mScriptObject;
};

//----------------------------------------------------------------------

ZContentDelegate::ZContentDelegate()
{
  NS_INIT_REFCNT();
}

NS_IMPL_ISUPPORTS(ZContentDelegate, kIContentDelegateIID);

ZContentDelegate::~ZContentDelegate()
{
}

NS_METHOD
ZContentDelegate::CreateFrame(nsIPresContext* aPresContext,
                             nsIContent* aContent,
                             nsIFrame* aParentFrame,
                             nsIStyleContext* aStyleContext,
                             nsIFrame*& aResult)
{
  NS_PRECONDITION(nsnull != aContent, "null ptr");

  // Make sure the content is html content
  nsIHTMLContent* hc;
  nsIFrame* frame = nsnull;
  nsresult rv = aContent->QueryInterface(kIHTMLContentIID, (void**) &hc);
  if (NS_OK != rv) {
    // This means that *somehow* somebody which is not an html
    // content object got ahold of this delegate and tried to
    // create a frame with it. Give them back an nsFrame.
    rv = nsFrame::NewFrame(&frame, aContent, aParentFrame);
    if (NS_OK == rv) {
      frame->SetStyleContext(aPresContext, aStyleContext);
    }
  }
  else {
    // Ask the content object to create the frame
    rv = hc->CreateFrame(aPresContext, aParentFrame, aStyleContext, frame);
    NS_RELEASE(hc);
  }
  aResult = frame;
  return rv;
}

//----------------------------------------------------------------------

DOMAttribute::DOMAttribute(const nsString& aName, const nsString& aValue)
  : mName(aName), mValue(aValue)
{
  mRefCnt = 1;
  mScriptObject = nsnull;
}

DOMAttribute::~DOMAttribute()
{
}

nsresult
DOMAttribute::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIDOMAttributeIID)) {
    nsIDOMAttribute* tmp = this;
    *aInstancePtr = (void*)tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptObjectOwnerIID)) {
    nsIScriptObjectOwner* tmp = this;
    *aInstancePtr = (void*)tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    nsIDOMAttribute* tmp1 = this;
    nsISupports* tmp2 = tmp1;
    *aInstancePtr = (void*)tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(DOMAttribute)

NS_IMPL_RELEASE(DOMAttribute)

nsresult
DOMAttribute::GetScriptObject(nsIScriptContext *aContext,
                                void** aScriptObject)
{
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    res = NS_NewScriptAttribute(aContext, 
                                (nsISupports *)(nsIDOMAttribute *)this, 
                                nsnull,
                                (void **)&mScriptObject);
  }
  *aScriptObject = mScriptObject;
  return res;
}

nsresult
DOMAttribute::ResetScriptObject()
{
  mScriptObject = nsnull;
  return NS_OK;
}

nsresult
DOMAttribute::GetName(nsString& aName)
{
  aName = mName;
  return NS_OK;
}

nsresult
DOMAttribute::GetValue(nsString& aValue)
{
  aValue = mValue;
  return NS_OK;
}

nsresult
DOMAttribute::GetSpecified(PRBool* aSpecified)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
DOMAttribute::SetSpecified(PRBool specified)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
DOMAttribute::GetNodeName(nsString& aNodeName)
{
  return GetName(aNodeName);
}

NS_IMETHODIMP
DOMAttribute::GetNodeValue(nsString& aNodeValue)
{
  return GetValue(aNodeValue);
}

NS_IMETHODIMP
DOMAttribute::SetNodeValue(const nsString& aNodeValue)
{
  // You can't actually do this, but we'll fail silently
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::GetNodeType(PRInt32* aNodeType)
{
  *aNodeType = (PRInt32)nsIDOMNode::ATTRIBUTE;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::GetParentNode(nsIDOMNode** aParentNode)
{
  *aParentNode = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  *aChildNodes = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::GetHasChildNodes(PRBool* aHasChildNodes)
{
  *aHasChildNodes = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::GetFirstChild(nsIDOMNode** aFirstChild)
{
  *aFirstChild = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::GetLastChild(nsIDOMNode** aLastChild)
{
  *aLastChild = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::GetPreviousSibling(nsIDOMNode** aPreviousSibling)
{
  *aPreviousSibling = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::GetNextSibling(nsIDOMNode** aNextSibling)
{
  *aNextSibling = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
  *aAttributes = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DOMAttribute::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DOMAttribute::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DOMAttribute::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DOMAttribute::CloneNode(nsIDOMNode** aReturn)
{
  DOMAttribute* newAttr = new DOMAttribute(mName, mValue);
  if (nsnull == newAttr) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *aReturn = newAttr;
  return NS_OK;
}

NS_IMETHODIMP
DOMAttribute::Equals(nsIDOMNode* aNode, PRBool aDeep, PRBool* aReturn)
{
  // XXX TBI
  return NS_OK;
}

//----------------------------------------------------------------------

DOMAttributeMap::DOMAttributeMap(nsIHTMLContent* aContent)
  : mContent(aContent)
{
  mRefCnt = 1;
  NS_ADDREF(mContent);
  mScriptObject = nsnull;
}

DOMAttributeMap::~DOMAttributeMap()
{
  NS_RELEASE(mContent);
}

nsresult
DOMAttributeMap::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIDOMNamedNodeMapIID)) {
    nsIDOMNamedNodeMap* tmp = this;
    *aInstancePtr = (void*)tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptObjectOwnerIID)) {
    nsIScriptObjectOwner* tmp = this;
    *aInstancePtr = (void*)tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    nsIDOMNamedNodeMap* tmp1 = this;
    nsISupports* tmp2 = tmp1;
    *aInstancePtr = (void*)tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(DOMAttributeMap)

NS_IMPL_RELEASE(DOMAttributeMap)

nsresult
DOMAttributeMap::GetScriptObject(nsIScriptContext *aContext,
                                   void** aScriptObject)
{
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    res = NS_NewScriptNamedNodeMap(aContext, 
                                   (nsISupports *)(nsIDOMNamedNodeMap *)this, 
                                   nsnull,
                                   (void**)&mScriptObject);
  }
  *aScriptObject = mScriptObject;
  return res;
}

nsresult
DOMAttributeMap::ResetScriptObject()
{
  mScriptObject = nsnull;
  return NS_OK;
}

nsresult
DOMAttributeMap::GetNamedItem(const nsString &aAttrName,
                                nsIDOMNode** aAttribute)
{
  nsAutoString value;
  mContent->GetAttribute(aAttrName, value);
  *aAttribute  = (nsIDOMNode *) new DOMAttribute(aAttrName, value);
  return NS_OK;
}

nsresult
DOMAttributeMap::SetNamedItem(nsIDOMNode *aNode)
{
  nsIDOMAttribute *attribute;
  nsAutoString name, value;
  nsresult err;

  if (NS_OK != (err = aNode->QueryInterface(kIDOMAttributeIID,
                                            (void **)&attribute))) {
    return err;
  }

  attribute->GetName(name);
  attribute->GetValue(value);
  NS_RELEASE(attribute);

  mContent->SetAttribute(name, value, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
DOMAttributeMap::RemoveNamedItem(const nsString& aName, nsIDOMNode** aReturn)
{
  nsresult res = GetNamedItem(aName, aReturn);
  if (NS_OK == res) {
    nsAutoString upper;
    aName.ToUpperCase(upper);
    nsIAtom* attr = NS_NewAtom(upper);
    mContent->UnsetAttribute(attr);
  }

  return res;
}

nsresult
DOMAttributeMap::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsresult res = NS_ERROR_FAILURE;
  nsAutoString name, value;
  nsISupportsArray *attributes = nsnull;
  if (NS_OK == NS_NewISupportsArray(&attributes)) {
    PRInt32 count;
    mContent->GetAllAttributeNames(attributes, count);
    if (count > 0) {
      if ((PRInt32)aIndex < count) {
        nsISupports *att = attributes->ElementAt(aIndex);
        static NS_DEFINE_IID(kIAtom, NS_IATOM_IID);
        nsIAtom *atName = nsnull;
        if (nsnull != att && NS_OK == att->QueryInterface(kIAtom, (void**)&atName)) {
          atName->ToString(name);
          if (NS_CONTENT_ATTR_NOT_THERE != mContent->GetAttribute(name, value)) {
            *aReturn = (nsIDOMNode *)new DOMAttribute(name, value);
            res = NS_OK;
          }
          NS_RELEASE(atName);
        }
      }
    }
    NS_RELEASE(attributes);
  }

  return res;
}

nsresult
DOMAttributeMap::GetLength(PRUint32 *aLength)
{
  PRInt32 n;
  nsresult rv = mContent->GetAttributeCount(n);
  *aLength = PRUint32(n);
  return rv;
}

//----------------------------------------------------------------------

static nsresult EnsureWritableAttributes(nsIHTMLContent* aContent,
                                         nsIHTMLAttributes*& aAttributes, PRBool aCreate)
{
  nsresult  result = NS_OK;

  if (nsnull == aAttributes) {
    if (PR_TRUE == aCreate) {
      nsMapAttributesFunc mapFunc;
      result = aContent->GetAttributeMappingFunction(mapFunc);
      if (NS_OK == result) {
        result = NS_NewHTMLAttributes(&aAttributes, mapFunc);
        if (NS_OK == result) {
          aAttributes->AddContentRef();
        }
      }
    }
  }
  else {
    PRInt32 contentRefCount;
    aAttributes->GetContentRefCount(contentRefCount);
    if (1 < contentRefCount) {
      nsIHTMLAttributes*  attrs;
      result = aAttributes->Clone(&attrs);
      if (NS_OK == result) {
        aAttributes->ReleaseContentRef();
        NS_RELEASE(aAttributes);
        aAttributes = attrs;
        aAttributes->AddContentRef();
      }
    }
  }
  return result;
}

static void ReleaseAttributes(nsIHTMLAttributes*& aAttributes)
{
  aAttributes->ReleaseContentRef();
  NS_RELEASE(aAttributes);
}

// XXX Currently, the script object factory is global. The way we
// obtain it should, at least, be made thread-safe later. Ideally,
// we'd find a better way.
nsIDOMScriptObjectFactory* nsGenericHTMLElement::gScriptObjectFactory = nsnull;

static NS_DEFINE_IID(kIDOMScriptObjectFactoryIID, NS_IDOM_SCRIPT_OBJECT_FACTORY_IID);
static NS_DEFINE_IID(kDOMScriptObjectFactoryCID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

nsresult 
nsGenericHTMLElement::GetScriptObjectFactory(nsIDOMScriptObjectFactory **aResult)
{
  nsresult result = NS_OK;

  if (nsnull == gScriptObjectFactory) {
    result = nsServiceManager::GetService(kDOMScriptObjectFactoryCID,
                                          kIDOMScriptObjectFactoryIID,
                                          (nsISupports **)&gScriptObjectFactory);
    if (result != NS_OK) {
      return result;
    }
  }

  *aResult = gScriptObjectFactory;
  NS_ADDREF(gScriptObjectFactory);
  return result;
}

nsGenericHTMLElement::nsGenericHTMLElement()
{
  mDocument = nsnull;
  mParent = nsnull;
  mAttributes = nsnull;
  mTag = nsnull;
  mContent = nsnull;
  mScriptObject = nsnull;
  mListenerManager = nsnull;

  // Create shared content delegate if this is the first html content
  // object being created.
  if (nsnull == gContentDelegate) {
    NS_NEWXPCOM(gContentDelegate, ZContentDelegate);
    NS_ADDREF(gContentDelegate);
  }
}

nsGenericHTMLElement::~nsGenericHTMLElement()
{
  if (nsnull != mAttributes) {
    ReleaseAttributes(mAttributes);
  }
  NS_IF_RELEASE(mTag);
  NS_IF_RELEASE(mListenerManager);
  // XXX what about mScriptObject? it's now safe to GC it...

#if 0
  // This code didn't work; and since content delegates are going
  // away, I'm not going to fix it. So for now, we will leak a content
  // delegate per address space. whoope doopie. -- kipp
  NS_PRECONDITION(nsnull != gContentDelegate, "null content delegate");
  if (nsnull != gContentDelegate) {
    // Remove our reference to the shared content delegate object.  If
    // the last reference just went away, null out gContentDelegate.
    nsrefcnt rc;
    // XXX:  This is now wrong.  gContentDelegate will be null after the
    //       first call to NS_RELEASE2(...)
    PR_ASSERT(0);
    NS_RELEASE2(gContentDelegate, rc);
    if (0 == rc) {
      gContentDelegate = nsnull;
    }
  }
#endif
}

void
nsGenericHTMLElement::Init(nsIHTMLContent* aOuterContentObject,
                           nsIAtom* aTag)
{
  NS_ASSERTION((nsnull == mContent) && (nsnull != aOuterContentObject),
               "null ptr");
  mContent = aOuterContentObject;
  mTag = aTag;
  NS_IF_ADDREF(aTag);
}

nsresult
nsGenericHTMLElement::GetNodeName(nsString& aNodeName)
{
  return GetTagName(aNodeName);
}

nsresult
nsGenericHTMLElement::GetNodeValue(nsString& aNodeValue)
{
  aNodeValue.Truncate();
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetNodeValue(const nsString& aNodeValue)
{
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetNodeType(PRInt32* aNodeType)
{
  *aNodeType = nsIDOMNode::ELEMENT;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetParentNode(nsIDOMNode** aParentNode)
{
  if (nsnull != mParent) {
    nsresult res = mParent->QueryInterface(kIDOMNodeIID, (void**)aParentNode);
    NS_ASSERTION(NS_OK == res, "Must be a DOM Node");
    return res;
  }
  else {
    *aParentNode = nsnull;
  }
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetPreviousSibling(nsIDOMNode** aNode)
{
  if (nsnull != mParent) {
    PRInt32 pos;
    mParent->IndexOf(mContent, pos);
    if (pos > -1) {
      nsIContent* prev;
      mParent->ChildAt(--pos, prev);
      if (nsnull != prev) {
        nsresult res = prev->QueryInterface(kIDOMNodeIID, (void**)aNode);
        NS_ASSERTION(NS_OK == res, "Must be a DOM Node");
        NS_RELEASE(prev); // balance the AddRef in ChildAt()
        return res;
      }
    }
  }
  *aNode = nsnull;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetNextSibling(nsIDOMNode** aNextSibling)
{
  if (nsnull != mParent) {
    PRInt32 pos;
    mParent->IndexOf(mContent, pos);
    if (pos > -1 ) {
      nsIContent* prev;
      mParent->ChildAt(++pos, prev);
      if (nsnull != prev) {
        nsresult res = prev->QueryInterface(kIDOMNodeIID,(void**)aNextSibling);
        NS_ASSERTION(NS_OK == res, "Must be a DOM Node");
        NS_RELEASE(prev); // balance the AddRef in ChildAt()
        return res;
      }
    }
  }
  *aNextSibling = nsnull;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
  NS_PRECONDITION(nsnull != aAttributes, "null pointer argument");
  if (nsnull != mAttributes) {
    // XXX Should we create a new one every time or should we
    // cache one after we create it? If we find that this is
    // something that's called often, we might need to do the
    // latter.
    *aAttributes = new DOMAttributeMap(mContent);
  }
  else {
    *aAttributes = nsnull;
  }
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetTagName(nsString& aTagName)
{
  aTagName.Truncate();
  if (nsnull != mTag) {
    mTag->ToString(aTagName);
  }
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetDOMAttribute(const nsString& aName, nsString& aReturn)
{
  GetAttribute(aName, aReturn);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetDOMAttribute(const nsString& aName,
                                      const nsString& aValue)
{
  SetAttribute(aName, aValue, PR_TRUE);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::RemoveAttribute(const nsString& aName)
{
  nsAutoString upper;
  aName.ToUpperCase(upper);
  nsIAtom* attr = NS_NewAtom(upper);
  UnsetAttribute(attr);
  NS_RELEASE(attr);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetAttributeNode(const nsString& aName,
                                       nsIDOMAttribute** aReturn)
{
  nsAutoString value;
  if (NS_CONTENT_ATTR_NOT_THERE != GetAttribute(aName, value)) {
    *aReturn = new DOMAttribute(aName, value);
  }
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetAttributeNode(nsIDOMAttribute* aAttribute)
{
  NS_PRECONDITION(nsnull != aAttribute, "null attribute");

  nsresult res = NS_ERROR_FAILURE;

  if (nsnull != aAttribute) {
    nsAutoString name, value;
    res = aAttribute->GetName(name);
    if (NS_OK == res) {
      res = aAttribute->GetValue(value);
      if (NS_OK == res) {
        SetAttribute(name, value, PR_TRUE);
      }
    }
  }
  return res;
}

nsresult
nsGenericHTMLElement::RemoveAttributeNode(nsIDOMAttribute* aAttribute)
{
  NS_PRECONDITION(nsnull != aAttribute, "null attribute");

  nsresult res = NS_ERROR_FAILURE;

  if (nsnull != aAttribute) {
    nsAutoString name;
    res = aAttribute->GetName(name);
    if (NS_OK == res) {
      nsAutoString upper;
      name.ToUpperCase(upper);
      nsIAtom* attr = NS_NewAtom(upper);
      UnsetAttribute(attr);
    }
  }

  return res;
}

nsresult
nsGenericHTMLElement::GetElementsByTagName(const nsString& aTagname,
                                           nsIDOMNodeList** aReturn)
{
  return NS_ERROR_NOT_IMPLEMENTED;/* XXX */
}

nsresult
nsGenericHTMLElement::Normalize()
{
  return NS_ERROR_NOT_IMPLEMENTED;/* XXX */
}

nsresult
nsGenericHTMLElement::GetId(nsString& aId)
{
  GetAttribute(nsHTMLAtoms::id, aId);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetId(const nsString& aId)
{
  SetAttr(nsHTMLAtoms::id, aId, eSetAttrNotify_Restart);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetTitle(nsString& aTitle)
{
  GetAttribute(nsHTMLAtoms::title, aTitle);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetTitle(const nsString& aTitle)
{
  SetAttr(nsHTMLAtoms::title, aTitle, eSetAttrNotify_None);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetLang(nsString& aLang)
{
  GetAttribute(nsHTMLAtoms::lang, aLang);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetLang(const nsString& aLang)
{
  SetAttr(nsHTMLAtoms::lang, aLang, eSetAttrNotify_Reflow);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetDir(nsString& aDir)
{
  GetAttribute(nsHTMLAtoms::dir, aDir);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetDir(const nsString& aDir)
{
  SetAttr(nsHTMLAtoms::dir, aDir, eSetAttrNotify_Reflow);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetClassName(nsString& aClassName)
{
  GetAttribute(nsHTMLAtoms::kClass, aClassName);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetClassName(const nsString& aClassName)
{
  SetAttr(nsHTMLAtoms::kClass, aClassName, eSetAttrNotify_Restart);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetDocument(nsIDocument*& aResult) const
{
  aResult = mDocument;
  NS_IF_ADDREF(mDocument);
  return NS_OK;
}

static nsIHTMLStyleSheet* GetAttrStyleSheet(nsIDocument* aDocument)
{
  nsIHTMLStyleSheet*  sheet = nsnull;
  nsIHTMLDocument*  htmlDoc;

  if (nsnull != aDocument) {
    if (NS_OK == aDocument->QueryInterface(kIHTMLDocumentIID, (void**)&htmlDoc)) {
      htmlDoc->GetAttributeStyleSheet(&sheet);
      NS_RELEASE(htmlDoc);
    }
  }
  NS_ASSERTION(nsnull != sheet, "can't get attribute style sheet");
  return sheet;
}

nsresult
nsGenericHTMLElement::SetDocument(nsIDocument* aDocument)
{
  mDocument = aDocument;

  // Once the element is added to the doc tree we need to check if
  // event handler were registered on it.  Unfortunately, this means
  // doing a GetAttribute for every type of handler.
  if ((nsnull != mDocument) && (nsnull != mAttributes)) {
    nsHTMLValue val;
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onclick, val))
      AddScriptEventListener(nsHTMLAtoms::onclick, val, kIDOMMouseListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::ondblclick, val))
      AddScriptEventListener(nsHTMLAtoms::onclick, val, kIDOMMouseListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onmousedown, val))
      AddScriptEventListener(nsHTMLAtoms::onmousedown, val, kIDOMMouseListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onmouseup, val))
      AddScriptEventListener(nsHTMLAtoms::onmouseup, val, kIDOMMouseListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onmouseover, val))
      AddScriptEventListener(nsHTMLAtoms::onmouseover, val, kIDOMMouseListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onmouseout, val))
      AddScriptEventListener(nsHTMLAtoms::onmouseout, val, kIDOMMouseListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onkeydown, val))
      AddScriptEventListener(nsHTMLAtoms::onkeydown, val, kIDOMKeyListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onkeyup, val))
      AddScriptEventListener(nsHTMLAtoms::onkeyup, val, kIDOMKeyListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onkeypress, val))
      AddScriptEventListener(nsHTMLAtoms::onkeypress, val, kIDOMKeyListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onmousemove, val))
      AddScriptEventListener(nsHTMLAtoms::onmousemove, val, kIDOMMouseMotionListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onload, val))
      AddScriptEventListener(nsHTMLAtoms::onload, val, kIDOMLoadListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onunload, val))
      AddScriptEventListener(nsHTMLAtoms::onunload, val, kIDOMLoadListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onabort, val))
      AddScriptEventListener(nsHTMLAtoms::onabort, val, kIDOMLoadListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onerror, val))
      AddScriptEventListener(nsHTMLAtoms::onerror, val, kIDOMLoadListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onfocus, val))
      AddScriptEventListener(nsHTMLAtoms::onfocus, val, kIDOMFocusListenerIID);
    if (NS_CONTENT_ATTR_HAS_VALUE == mAttributes->GetAttribute(nsHTMLAtoms::onblur, val))
      AddScriptEventListener(nsHTMLAtoms::onblur, val, kIDOMFocusListenerIID);

    nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
    if (nsnull != sheet) {
      sheet->SetAttributesFor(mContent, mAttributes); // sync attributes with sheet
      NS_RELEASE(sheet);
    }
  }

  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetParent(nsIContent*& aResult) const
{
  NS_IF_ADDREF(mParent);
  aResult = mParent;
  return NS_OK;;
}

nsresult
nsGenericHTMLElement::SetParent(nsIContent* aParent)
{
  mParent = aParent;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetTag(nsIAtom*& aResult) const
{
  NS_IF_ADDREF(mTag);
  aResult = mTag;
  return NS_OK;
}

//void
//nsHTMLTagContent::SizeOfWithoutThis(nsISizeOfHandler* aHandler) const
//{
//  if (!aHandler->HaveSeen(mTag)) {
//    mTag->SizeOf(aHandler);
//  }
//  if (!aHandler->HaveSeen(mAttributes)) {
//    mAttributes->SizeOf(aHandler);
//  }
//}

nsresult
nsGenericHTMLElement::HandleDOMEvent(nsIPresContext& aPresContext,
                                     nsEvent* aEvent,
                                     nsIDOMEvent** aDOMEvent,
                                     PRUint32 aFlags,
                                     nsEventStatus& aEventStatus)
{
  nsresult ret = NS_OK;
  
  nsIDOMEvent* domEvent = nsnull;
  if (DOM_EVENT_INIT == aFlags) {
    nsIEventStateManager *manager;
    if (NS_OK == aPresContext.GetEventStateManager(&manager)) {
      manager->SetEventTarget(mContent);
      NS_RELEASE(manager);
    }
    aDOMEvent = &domEvent;
  }
  
  //Capturing stage
  
  //Local handling stage
  if (nsnull != mListenerManager) {
    mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, aEventStatus);
  }

  //Bubbling stage
  if (DOM_EVENT_CAPTURE != aFlags && mParent != nsnull) {
    ret = mParent->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                  DOM_EVENT_BUBBLE, aEventStatus);
  }

  if (DOM_EVENT_INIT == aFlags) {
    // We're leaving the DOM event loop so if we created a DOM event,
    // release here.
    if (nsnull != *aDOMEvent) {
      nsrefcnt rc;
      nsIDOMEvent* DOMEvent = *aDOMEvent;
      // Release the copy since the macro will null the pointer 
      NS_RELEASE2(DOMEvent, rc);
      if (0 != rc) {
        // Okay, so someone in the DOM loop (a listener, JS object)
        // still has a ref to the DOM Event but the internal data
        // hasn't been malloc'd.  Force a copy of the data here so the
        // DOM Event is still valid.
        nsIPrivateDOMEvent *privateEvent;
        if (NS_OK == (*aDOMEvent)->QueryInterface(kIPrivateDOMEventIID, (void**)&privateEvent)) {
          privateEvent->DuplicatePrivateData();
          NS_RELEASE(privateEvent);
        }
      }
    }
    aDOMEvent = nsnull;
  }
  return ret;
}

nsresult
nsGenericHTMLElement::SetAttribute(const nsString& aName,
                                   const nsString& aValue,
                                   PRBool aNotify)
{
  nsAutoString upper;
  aName.ToUpperCase(upper);
  nsIAtom* attr = NS_NewAtom(upper);
  nsresult rv = SetAttribute(attr, aValue, aNotify);
  NS_RELEASE(attr);
  return rv;
}

#if 0
static nsGenericHTMLElement::EnumTable kDirTable[] = {
  { "ltr", NS_STYLE_DIRECTION_LTR },
  { "rtl", NS_STYLE_DIRECTION_RTL },
  { 0 }
};
#endif

nsresult
nsGenericHTMLElement::SetAttribute(nsIAtom* aAttribute,
                                   const nsString& aValue,
                                   PRBool aNotify)
{
  nsresult  result = NS_OK;
#if 0
  if (nsHTMLAtoms::dir == aAttribute) {
    nsHTMLValue val;
    if (ParseEnumValue(aValue, kDirTable, val)) {
      result = SetAttribute(aAttribute, val, aNotify);
    }
    else {
      result = SetStringAttribute(aAttribute, aValue, aNotify);
    }
  }
  else if (nsHTMLAtoms::lang == aAttribute) {
    result = SetStringAttribute(aAttribute, aValue, aNotify);
  }
  else if (nsHTMLAtoms::title == aAttribute) {
    result = SetStringAttribute(aAttribute, aValue, aNotify);
  }
  else
#endif
  if (nsHTMLAtoms::style == aAttribute) {
    // XXX the style sheet language is a document property that
    // should be used to lookup the style sheet parser to parse the
    // attribute.
    nsICSSParser* css;
    result = NS_NewCSSParser(&css);
    if (NS_OK != result) {
      return result;
    }
    nsIStyleRule* rule;
    result = css->ParseDeclarations(aValue, nsnull, rule);
    if ((NS_OK == result) && (nsnull != rule)) {
      result = SetAttribute(aAttribute, nsHTMLValue(rule), aNotify);
      NS_RELEASE(rule);
    }
    NS_RELEASE(css);
  }
  else {
    nsHTMLValue val;
    if (NS_CONTENT_ATTR_NOT_THERE !=
        mContent->StringToAttribute(aAttribute, aValue, val)) {
      // string value was mapped to nsHTMLValue, set it that way
      result = SetAttribute(aAttribute, val, aNotify);
    }
    else {
      // set as string value to avoid another string copy
      if (nsnull != mDocument) {  // set attr via style sheet
        nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
        if (nsnull != sheet) {
          result = sheet->SetAttributeFor(aAttribute, aValue, mContent, mAttributes);
          NS_RELEASE(sheet);
        }
      }
      else {  // manage this ourselves and re-sync when we connect to doc
        result = EnsureWritableAttributes(mContent, mAttributes, PR_TRUE);
        if (nsnull != mAttributes) {
          PRInt32   count;
          result = mAttributes->SetAttribute(aAttribute, aValue, count);
          if (0 == count) {
            ReleaseAttributes(mAttributes);
          }
        }
      }
    }
  }
  return result;
}

nsresult
nsGenericHTMLElement::SetAttribute(nsIAtom* aAttribute,
                                   const nsHTMLValue& aValue,
                                   PRBool aNotify)
{
  nsresult  result = NS_OK;
  if (nsnull != mDocument) {  // set attr via style sheet
    nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
    if (nsnull != sheet) {
      result = sheet->SetAttributeFor(aAttribute, aValue, mContent, mAttributes);
      NS_RELEASE(sheet);
    }
  }
  else {  // manage this ourselves and re-sync when we connect to doc
    result = EnsureWritableAttributes(mContent, mAttributes, PR_TRUE);
    if (nsnull != mAttributes) {
      PRInt32   count;
      result = mAttributes->SetAttribute(aAttribute, aValue, count);
      if (0 == count) {
        ReleaseAttributes(mAttributes);
      }
    }
  }
  return result;
}

/**
 * Handle attributes common to all html elements
 */
void
nsGenericHTMLElement::MapCommonAttributesInto(nsIHTMLAttributes* aAttributes, 
                                              nsIStyleContext* aStyleContext,
                                              nsIPresContext* aPresContext)
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;
    aAttributes->GetAttribute(nsHTMLAtoms::dir, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      nsStyleDisplay* display = (nsStyleDisplay*)
        aStyleContext->GetMutableStyleData(eStyleStruct_Display);
      display->mDirection = value.GetIntValue();
    }
  }
}

nsresult
nsGenericHTMLElement::UnsetAttribute(nsIAtom* aAttribute)
{
  nsresult result = NS_OK;
  if (nsnull != mDocument) {  // set attr via style sheet
    nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
    if (nsnull != sheet) {
      result = sheet->UnsetAttributeFor(aAttribute, mContent, mAttributes);
      NS_RELEASE(sheet);
    }
  }
  else {  // manage this ourselves and re-sync when we connect to doc
    result = EnsureWritableAttributes(mContent, mAttributes, PR_FALSE);
    if (nsnull != mAttributes) {
      PRInt32 count;
      result = mAttributes->UnsetAttribute(aAttribute, count);
      if (0 == count) {
        ReleaseAttributes(mAttributes);
      }
    }
  }
  return result;
}

nsresult
nsGenericHTMLElement::GetAttribute(const nsString& aName,
                                   nsString& aResult) const
{
  nsAutoString upper;
  aName.ToUpperCase(upper);
  nsIAtom* attr = NS_NewAtom(upper);
  nsresult result = GetAttribute(attr, aResult);
  NS_RELEASE(attr);
  return result;
}

nsresult
nsGenericHTMLElement::GetAttribute(nsIAtom *aAttribute,
                                   nsString &aResult) const
{
  nsHTMLValue value;
  nsresult result = GetAttribute(aAttribute, value);

  char cbuf[20];
  nscolor color;
  if (NS_CONTENT_ATTR_HAS_VALUE == result) {
    // Try subclass conversion routine first
    if (NS_CONTENT_ATTR_HAS_VALUE ==
        mContent->AttributeToString(aAttribute, value, aResult)) {
      return result;
    }

    // Provide default conversions for most everything
    switch (value.GetUnit()) {
    case eHTMLUnit_Empty:
      aResult.Truncate();
      break;

    case eHTMLUnit_String:
    case eHTMLUnit_Null:
      value.GetStringValue(aResult);
      break;

    case eHTMLUnit_Integer:
      aResult.Truncate();
      aResult.Append(value.GetIntValue(), 10);
      break;

    case eHTMLUnit_Pixel:
      aResult.Truncate();
      aResult.Append(value.GetPixelValue(), 10);
      break;

    case eHTMLUnit_Percent:
      aResult.Truncate(0);
      aResult.Append(PRInt32(value.GetPercentValue() * 100.0f), 10);
      aResult.Append('%');
      break;

    case eHTMLUnit_Color:
      color = nscolor(value.GetColorValue());
      PR_snprintf(cbuf, sizeof(cbuf), "#%02x%02x%02x",
                  NS_GET_R(color), NS_GET_G(color), NS_GET_B(color));
      aResult.Truncate(0);
      aResult.Append(cbuf);
      break;

    default:
    case eHTMLUnit_Enumerated:
      NS_NOTREACHED("no default enumerated value to string conversion");
      result = NS_CONTENT_ATTR_NOT_THERE;
      break;
    }
  }
  return result;
}

nsresult
nsGenericHTMLElement::GetAttribute(nsIAtom* aAttribute,
                                   nsHTMLValue& aValue) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetAttribute(aAttribute, aValue);
  }
  aValue.Reset();
  return NS_CONTENT_ATTR_NOT_THERE;
}

nsresult
nsGenericHTMLElement::GetAllAttributeNames(nsISupportsArray* aArray,
                                           PRInt32& aCount) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetAllAttributeNames(aArray, aCount);
  }
  aCount = 0;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetAttributeCount(PRInt32& aCount) const
{
  if (nsnull != mAttributes) {
    return mAttributes->Count(aCount);
  }
  aCount = 0;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetID(nsIAtom* aID)
{
  nsresult result = NS_OK;
  if (nsnull != mDocument) {  // set attr via style sheet
    nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
    if (nsnull != sheet) {
      result = sheet->SetIDFor(aID, mContent, mAttributes);
      NS_RELEASE(sheet);
    }
  }
  else {  // manage this ourselves and re-sync when we connect to doc
    EnsureWritableAttributes(mContent, mAttributes, PRBool(nsnull != aID));
    if (nsnull != mAttributes) {
      PRInt32 count;
      result = mAttributes->SetID(aID, count);
      if (0 == count) {
        ReleaseAttributes(mAttributes);
      }
    }
  }
  return result;
}

nsresult
nsGenericHTMLElement::GetID(nsIAtom*& aResult) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetID(aResult);
  }
  aResult = nsnull;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetClass(nsIAtom* aClass)
{
  nsresult result = NS_OK;
  if (nsnull != mDocument) {  // set attr via style sheet
    nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
    if (nsnull != sheet) {
      result = sheet->SetClassFor(aClass, mContent, mAttributes);
      NS_RELEASE(sheet);
    }
  }
  else {  // manage this ourselves and re-sync when we connect to doc
    EnsureWritableAttributes(mContent, mAttributes, PRBool(nsnull != aClass));
    if (nsnull != mAttributes) {
      PRInt32 count;
      result = mAttributes->SetClass(aClass, count);
      if (0 == count) {
        ReleaseAttributes(mAttributes);
      }
    }
  }
  return result;
}

nsresult
nsGenericHTMLElement::GetClass(nsIAtom*& aResult) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetClass(aResult);
  }
  aResult = nsnull;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetStyleRule(nsIStyleRule*& aResult)
{
  nsIStyleRule* result = nsnull;

  if (nsnull != mAttributes) {
    mAttributes->QueryInterface(kIStyleRuleIID, (void**)&result);
  }
  aResult = result;
  return NS_OK;
}

nsIContentDelegate*
nsGenericHTMLElement::GetDelegate(nsIPresContext* aCX)
{
  NS_ADDREF(gContentDelegate);
  return gContentDelegate;
}

void
nsGenericHTMLElement::ListAttributes(FILE* out) const
{
  nsISupportsArray* attrs;
  if (NS_OK == NS_NewISupportsArray(&attrs)) {
    PRInt32 index, count;
    GetAllAttributeNames(attrs, count);
    for (index = 0; index < count; index++) {
      // name
      nsIAtom* attr = (nsIAtom*)attrs->ElementAt(index);
      nsAutoString buffer;
      attr->ToString(buffer);

      // value
      nsAutoString value;
      GetAttribute(buffer, value);
      buffer.Append("=");
      buffer.Append(value);

      fputs(" ", out);
      fputs(buffer, out);
      NS_RELEASE(attr);
    }
    NS_RELEASE(attrs);
  }
}

nsresult
nsGenericHTMLElement::List(FILE* out, PRInt32 aIndent) const
{
  NS_PRECONDITION(nsnull != mDocument, "bad content");

  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  nsIAtom* tag;
  GetTag(tag);
  if (tag != nsnull) {
    nsAutoString buf;
    tag->ToString(buf);
    fputs(buf, out);
    NS_RELEASE(tag);
  }

  ListAttributes(out);

  nsIHTMLContent* hc = mContent;
  nsrefcnt r = NS_ADDREF(hc) - 1;
  NS_RELEASE(hc);
  fprintf(out, " refcount=%d<", r);

  PRBool canHaveKids;
  mContent->CanContainChildren(canHaveKids);
  if (canHaveKids) {
    fputs("\n", out);
    PRInt32 kids;
    mContent->ChildCount(kids);
    for (index = 0; index < kids; index++) {
      nsIContent* kid;
      mContent->ChildAt(index, kid);
      kid->List(out, aIndent + 1);
      NS_RELEASE(kid);
    }
    for (index = aIndent; --index >= 0; ) fputs("  ", out);
  }
  fputs(">\n", out);

  return NS_OK;
}

nsresult
nsGenericHTMLElement::ToHTML(FILE* out) const
{
  nsAutoString tmp;
  nsresult rv = ToHTMLString(tmp);
  fputs(tmp, out);
  return rv;
}

// XXX i18n: this is wrong (?) because we need to know the outgoing
// character set (I think)
void
NS_QuoteForHTML(const nsString& aValue, nsString& aResult)
{
  aResult.Truncate();
  const PRUnichar* cp = aValue.GetUnicode();
  const PRUnichar* end = aValue.GetUnicode() + aValue.Length();
  aResult.Append('"');
  while (cp < end) {
    PRUnichar ch = *cp++;
    if ((ch >= 0x20) && (ch <= 0x7f)) {
      if (ch == '\"') {
        aResult.Append("&quot;");
      }
      else {
        aResult.Append(ch);
      }
    }
    else {
      aResult.Append("&#");
      aResult.Append((PRInt32) ch, 10);
      aResult.Append(';');
    }
  }
  aResult.Append('"');
}

nsresult
nsGenericHTMLElement::ToHTMLString(nsString& aBuf) const
{
  aBuf.Truncate(0);
  aBuf.Append('<');

  if (nsnull != mTag) {
    nsAutoString tmp;
    mTag->ToString(tmp);
    aBuf.Append(tmp);
  } else {
    aBuf.Append("?NULL");
  }

  if (nsnull != mAttributes) {
    nsISupportsArray* attrs;
    nsresult rv = NS_NewISupportsArray(&attrs);
    if (NS_OK == rv) {
      PRInt32 i, n;
      mAttributes->GetAllAttributeNames(attrs, n);
      nsAutoString name, value, quotedValue;
      for (i = 0; i < n; i++) {
        nsIAtom* atom = (nsIAtom*) attrs->ElementAt(i);
        atom->ToString(name);
        aBuf.Append(' ');
        aBuf.Append(name);
        value.Truncate();
        GetAttribute(name, value);
        if (value.Length() > 0) {
          aBuf.Append('=');
          NS_QuoteForHTML(value, quotedValue);
          aBuf.Append(quotedValue);
        }
      }
      NS_RELEASE(attrs);
    }
  }

  aBuf.Append('>');
  return NS_OK;
}

//----------------------------------------------------------------------

nsresult
nsGenericHTMLElement::SetAttr(nsIAtom* aAttribute,
                              const nsString& aValue,
                              nsSetAttrNotify aNotify)
{
  nsresult rv = SetAttribute(aAttribute, aValue, PR_FALSE);
  if (NS_OK != rv) {
    return rv;
  }
  switch (aNotify) {
  case eSetAttrNotify_None:
    break;
  case eSetAttrNotify_Reflow:
    if (nsnull != mDocument) {
      mDocument->ContentChanged(mContent, nsnull);
    }
    break;
  case eSetAttrNotify_Render:
    RenderFrame();
    break;
  case eSetAttrNotify_Restart:
    break;
  }
  return rv;
}

nsresult
nsGenericHTMLElement::SetAttr(nsIAtom* aAttribute,
                              const nsHTMLValue& aValue,
                              nsSetAttrNotify aNotify)
{
  nsresult rv = SetAttribute(aAttribute, aValue, PR_FALSE);
  if (NS_OK != rv) {
    return rv;
  }
  switch (aNotify) {
  case eSetAttrNotify_None:
    break;
  case eSetAttrNotify_Reflow:
    break;
  case eSetAttrNotify_Render:
    RenderFrame();
    break;
  case eSetAttrNotify_Restart:
    break;
  }
  return rv;
}

nsresult
nsGenericHTMLElement::UnsetAttr(nsIAtom* aAttribute,
                                nsSetAttrNotify aNotify)
{
  nsresult rv = UnsetAttribute(aAttribute);
  if (NS_OK != rv) {
    return rv;
  }
  switch (aNotify) {
  case eSetAttrNotify_None:
    break;
  case eSetAttrNotify_Reflow:
    break;
  case eSetAttrNotify_Render:
    RenderFrame();
    break;
  case eSetAttrNotify_Restart:
    break;
  }
  return rv;
}

nsresult
nsGenericHTMLElement::RenderFrame()
{
  nsPoint offset;
  nsRect bounds;

  // Trigger damage repairs for each frame that maps the given content
  PRInt32 i, n;
  n = mDocument->GetNumberOfShells();
  for (i = 0; i < n; i++) {
    nsIPresShell* shell;
    shell = mDocument->GetShellAt(i);
    nsIFrame* frame;
    frame = shell->FindFrameWithContent(mContent);
    while (nsnull != frame) {
      nsIViewManager* vm;
      nsIView* view;

      // Determine damaged area and tell view manager to redraw it
      frame->GetRect(bounds);
      bounds.x = bounds.y = 0;

      // XXX We should tell the frame the damage area and let it invalidate
      // itself. Add some API calls to nsIFrame to allow a caller to invalidate
      // parts of the frame...
      frame->GetOffsetFromView(offset, view);
      view->GetViewManager(vm);
      bounds.x += offset.x;
      bounds.y += offset.y;

      vm->UpdateView(view, bounds, NS_VMREFRESH_IMMEDIATE);
      NS_RELEASE(vm);

      // If frame has a next-in-flow, repaint it too
      frame->GetNextInFlow(frame);
    }
    NS_RELEASE(shell);
  }

  return NS_OK;
}

//----------------------------------------------------------------------

// XXX this is REALLY temporary code

nsresult
nsGenericHTMLElement::CreateFrame(nsIPresContext*  aPresContext,
                                  nsIFrame*        aParentFrame,
                                  nsIStyleContext* aStyleContext,
                                  nsIFrame*&       aResult)
{
  nsIFrame* frame = nsnull;
  nsresult rv = NS_OK;

  // Handle specific frame types
  if (mTag == nsHTMLAtoms::applet) {
    rv = NS_NewObjectFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::body) {
    rv = NS_NewBodyFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::br) {
    rv = NS_NewBRFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::embed) {
    rv = NS_NewObjectFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::frame) {
    rv = NS_NewHTMLFrameOuterFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::frameset) {
    rv = NS_NewHTMLFramesetFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::hr) {
    rv = NS_NewHRFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::html) {
    rv = NS_NewHTMLFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::iframe) {
    rv = NS_NewHTMLFrameOuterFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::img) {
    rv = NS_NewImageFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::object) {
    rv = NS_NewObjectFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::spacer) {
    rv = NS_NewSpacerFrame(mContent, aParentFrame, frame);
  }
  else if (mTag == nsHTMLAtoms::wbr) {
    rv = NS_NewWBRFrame(mContent, aParentFrame, frame);
  }

  if (NS_OK != rv) {
    return rv;
  }

  // XXX add code in here to force the odd ones into the empty frame?
  // AREA, HEAD, META, MAP, etc...

  if (nsnull == frame) {
    // When there is no explicit frame to create, assume it's a
    // container and let style dictate the rest.
    const nsStyleDisplay* styleDisplay = (const nsStyleDisplay*)
      aStyleContext->GetStyleData(eStyleStruct_Display);

    // Use style to choose what kind of frame to create
    nsresult rv;
    switch (styleDisplay->mDisplay) {
    case NS_STYLE_DISPLAY_BLOCK:
    case NS_STYLE_DISPLAY_LIST_ITEM:
      rv = NS_NewBlockFrame(&frame, mContent, aParentFrame);
      break;

    case NS_STYLE_DISPLAY_INLINE:
      rv = NS_NewInlineFrame(&frame, mContent, aParentFrame);
      break;

    default:
      // Create an empty frame for holding content that is not being
      // reflowed.
      rv = nsFrame::NewFrame(&frame, mContent, aParentFrame);
      break;
    }
    if (NS_OK != rv) {
      return rv;
    }
  }

  frame->SetStyleContext(aPresContext, aStyleContext);
  aResult = frame;
  return NS_OK;
}

//----------------------------------------------------------------------

// nsIScriptObjectOwner implementation

nsresult
nsGenericHTMLElement::GetScriptObject(nsIScriptContext* aContext,
                                      void** aScriptObject)
{
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    nsIDOMScriptObjectFactory *factory;
    
    res = GetScriptObjectFactory(&factory);
    if (NS_OK != res) {
      return res;
    }
    
    nsAutoString tag;
    mTag->ToString(tag);
    res = factory->NewScriptElement(tag, aContext, mContent,
                                    mParent, (void**)&mScriptObject);
    NS_RELEASE(factory);
  }
  *aScriptObject = mScriptObject;
  return res;
}

nsresult
nsGenericHTMLElement::ResetScriptObject()
{
  mScriptObject = nsnull;
  return NS_OK;
}

//----------------------------------------------------------------------

// nsIDOMEventReceiver implementation

nsresult
nsGenericHTMLElement::GetListenerManager(nsIEventListenerManager** aResult)
{
  if (nsnull != mListenerManager) {
    NS_ADDREF(mListenerManager);
    *aResult = mListenerManager;
    return NS_OK;
  }
  nsresult rv = NS_NewEventListenerManager(aResult);
  if (NS_OK == rv) {
    mListenerManager = *aResult;
    NS_ADDREF(mListenerManager);
  }
  return rv;
}

nsresult
nsGenericHTMLElement::GetNewListenerManager(nsIEventListenerManager** aResult)
{
  return NS_NewEventListenerManager(aResult);
} 

nsresult
nsGenericHTMLElement::AddEventListener(nsIDOMEventListener* aListener,
                                       const nsIID& aIID)
{
  nsIEventListenerManager *manager;

  if (NS_OK == GetListenerManager(&manager)) {
    manager->AddEventListener(aListener, aIID);
    NS_RELEASE(manager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsGenericHTMLElement::RemoveEventListener(nsIDOMEventListener* aListener,
                                          const nsIID& aIID)
{
  if (nsnull != mListenerManager) {
    mListenerManager->RemoveEventListener(aListener, aIID);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

// nsIJSScriptObject implementation

PRBool    
nsGenericHTMLElement::AddProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
  return PR_TRUE;
}

PRBool    
nsGenericHTMLElement::DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
  return PR_TRUE;
}

PRBool    
nsGenericHTMLElement::GetProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
  return PR_TRUE;
}
 
PRBool    
nsGenericHTMLElement::SetProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
  nsIScriptObjectOwner *owner;

  if (NS_OK != mContent->QueryInterface(kIScriptObjectOwnerIID, (void **)&owner)) {
    return PR_FALSE;
  }

  if (JS_TypeOfValue(aContext, *aVp) == JSTYPE_FUNCTION && JSVAL_IS_STRING(aID)) {
    nsAutoString propName, prefix;
    propName.SetString(JS_GetStringChars(JS_ValueToString(aContext, aID)));
    prefix.SetString(propName, 2);
    if (prefix == "on") {
      nsIEventListenerManager *manager = nsnull;

      if (propName == "onmousedown" || propName == "onmouseup" || propName ==  "onclick" ||
         propName == "onmouseover" || propName == "onmouseout") {
        if (NS_OK == GetListenerManager(&manager)) {
          nsIScriptContext *mScriptCX = (nsIScriptContext *)JS_GetContextPrivate(aContext);
          if (NS_OK != manager->RegisterScriptEventListener(mScriptCX, owner, kIDOMMouseListenerIID)) {
            NS_RELEASE(manager);
            return PR_FALSE;
          }
        }
      }
      else if (propName == "onkeydown" || propName == "onkeyup" || propName == "onkeypress") {
        if (NS_OK == GetListenerManager(&manager)) {
          nsIScriptContext *mScriptCX = (nsIScriptContext *)JS_GetContextPrivate(aContext);
          if (NS_OK != manager->RegisterScriptEventListener(mScriptCX, owner, kIDOMKeyListenerIID)) {
            NS_RELEASE(manager);
            return PR_FALSE;
          }
        }
      }
      else if (propName == "onmousemove") {
        if (NS_OK == GetListenerManager(&manager)) {
          nsIScriptContext *mScriptCX = (nsIScriptContext *)JS_GetContextPrivate(aContext);
          if (NS_OK != manager->RegisterScriptEventListener(mScriptCX, owner, kIDOMMouseMotionListenerIID)) {
            NS_RELEASE(manager);
            return PR_FALSE;
          }
        }
      }
      else if (propName == "onfocus" || propName == "onblur") {
        if (NS_OK == GetListenerManager(&manager)) {
          nsIScriptContext *mScriptCX = (nsIScriptContext *)JS_GetContextPrivate(aContext);
          if (NS_OK != manager->RegisterScriptEventListener(mScriptCX, owner, kIDOMFocusListenerIID)) {
            NS_RELEASE(manager);
            return PR_FALSE;
          }
        }
      }
      else if (propName == "onsubmit" || propName == "onreset") {
        if (NS_OK == GetListenerManager(&manager)) {
          nsIScriptContext *mScriptCX = (nsIScriptContext *)JS_GetContextPrivate(aContext);
          if (NS_OK != manager->RegisterScriptEventListener(mScriptCX, owner, kIDOMFormListenerIID)) {
            NS_RELEASE(manager);
            return PR_FALSE;
          }
        }
      }
      else if (propName == "onload" || propName == "onunload" || propName == "onabort" ||
               propName == "onerror") {
        if (NS_OK == GetListenerManager(&manager)) {
          nsIScriptContext *mScriptCX = (nsIScriptContext *)JS_GetContextPrivate(aContext);
          if (NS_OK != manager->RegisterScriptEventListener(mScriptCX, owner, kIDOMLoadListenerIID)) {
            NS_RELEASE(manager);
            return PR_FALSE;
          }
        }
      }
      NS_IF_RELEASE(manager);
    }
  }

  NS_IF_RELEASE(owner);

  return PR_TRUE;
}
 
PRBool    
nsGenericHTMLElement::EnumerateProperty(JSContext *aContext)
{
  return PR_TRUE;
}

PRBool    
nsGenericHTMLElement::Resolve(JSContext *aContext, jsval aID)
{
  return PR_TRUE;
}

PRBool    
nsGenericHTMLElement::Convert(JSContext *aContext, jsval aID)
{
  return PR_TRUE;
}

void      
nsGenericHTMLElement::Finalize(JSContext *aContext)
{
}
 
//----------------------------------------------------------------------

// nsISupports implementation

NS_IMETHODIMP
nsGenericHTMLElement::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
  return mContent->QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP_(nsrefcnt) 
nsGenericHTMLElement::AddRef()
{
  return NS_ADDREF(mContent);
}

NS_IMETHODIMP_(nsrefcnt) 
nsGenericHTMLElement::Release()
{
  nsIHTMLContent* content = mContent;

  // Release the copy since the macro will null the pointer.
  return NS_RELEASE(content);
}


//----------------------------------------------------------------------

nsresult
nsGenericHTMLElement::AddScriptEventListener(nsIAtom* aAttribute,
                                             nsHTMLValue& aValue,
                                             REFNSIID aIID)
{
  nsresult ret = NS_OK;
  nsIScriptContext* context;
  nsIScriptContextOwner* owner;

  if (nsnull != mDocument) {
    owner = mDocument->GetScriptContextOwner();
    if (NS_OK == owner->GetScriptContext(&context)) {
      if (nsHTMLAtoms::body == mTag || nsHTMLAtoms::frameset == mTag) {
        nsIDOMEventReceiver *receiver;
        nsIScriptGlobalObject *global = context->GetGlobalObject();

        if (nsnull != global && NS_OK == global->QueryInterface(kIDOMEventReceiverIID, (void**)&receiver)) {
          nsIEventListenerManager *manager;
          if (NS_OK == receiver->GetListenerManager(&manager)) {
            nsIScriptObjectOwner *mObjectOwner;
            if (NS_OK == global->QueryInterface(kIScriptObjectOwnerIID, (void**)&mObjectOwner)) {
              nsString value;
              aValue.GetStringValue(value);
              ret = manager->AddScriptEventListener(context, mObjectOwner, aAttribute, value, aIID);
              NS_RELEASE(mObjectOwner);
            }
            NS_RELEASE(manager);
          }
          NS_RELEASE(receiver);
        }
        NS_IF_RELEASE(global);
      }
      else {
        nsIEventListenerManager *manager;
        if (NS_OK == GetListenerManager(&manager)) {
          nsString value;
          aValue.GetStringValue(value);
          nsIScriptObjectOwner* owner;
          if (NS_OK == mContent->QueryInterface(kIScriptObjectOwnerIID,
                                                (void**) &owner)) {
            ret = manager->AddScriptEventListener(context, owner,
                                                  aAttribute, value, aIID);
            NS_RELEASE(owner);
          }
          NS_RELEASE(manager);
        }
      }
      NS_RELEASE(context);
    }
    NS_RELEASE(owner);
  }
  return ret;
}

nsresult
nsGenericHTMLElement::AttributeToString(nsIAtom* aAttribute,
                                        nsHTMLValue& aValue,
                                        nsString& aResult) const
{
  if (nsHTMLAtoms::style == aAttribute) {
    if (eHTMLUnit_ISupports == aValue.GetUnit()) {
      nsIStyleRule* rule = (nsIStyleRule*) aValue.GetISupportsValue();
      nsICSSStyleRule*  cssRule;
      if (NS_OK == rule->QueryInterface(kICSSStyleRuleIID, (void**)&cssRule)) {
        nsICSSDeclaration* decl = cssRule->GetDeclaration();
        if (nsnull != decl) {
          decl->ToString(aResult);
        }
        NS_RELEASE(cssRule);
      }
      else {
        aResult = "Unknown rule type";
      }
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  aResult.Truncate();
  return NS_CONTENT_ATTR_NOT_THERE;
}

PRBool
nsGenericHTMLElement::ParseEnumValue(const nsString& aValue,
                                     EnumTable* aTable,
                                     nsHTMLValue& aResult)
{
  while (nsnull != aTable->tag) {
    if (aValue.EqualsIgnoreCase(aTable->tag)) {
      aResult.SetIntValue(aTable->value, eHTMLUnit_Enumerated);
      return PR_TRUE;
    }
    aTable++;
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::EnumValueToString(const nsHTMLValue& aValue,
                                        EnumTable* aTable,
                                        nsString& aResult)
{
  aResult.Truncate(0);
  if (aValue.GetUnit() == eHTMLUnit_Enumerated) {
    PRInt32 v = aValue.GetIntValue();
    while (nsnull != aTable->tag) {
      if (aTable->value == v) {
        aResult.Append(aTable->tag);
        return PR_TRUE;
      }
      aTable++;
    }
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseValueOrPercent(const nsString& aString,
                                          nsHTMLValue& aResult, 
                                          nsHTMLUnit aValueUnit)
{ // XXX should vave min/max values?
  nsAutoString tmp(aString);
  tmp.CompressWhitespace(PR_TRUE, PR_TRUE);
  PRInt32 ec, val = tmp.ToInteger(&ec);
  if (NS_OK == ec) {
    if (tmp.Last() == '%') {/* XXX not 100% compatible with ebina's code */
      if (val < 0) val = 0;
      if (val > 100) val = 100;
      aResult.SetPercentValue(float(val)/100.0f);
    } else {
      if (eHTMLUnit_Pixel == aValueUnit) {
        aResult.SetPixelValue(val);
      }
      else {
        aResult.SetIntValue(val, aValueUnit);
      }
    }
    return PR_TRUE;
  }

  // Illegal values are mapped to empty
  aResult.SetEmptyValue();
  return PR_FALSE;
}

/* used to parse attribute values that could be either:
 *   integer  (n), 
 *   percent  (n%),
 *   or proportional (n*)
 */
void
nsGenericHTMLElement::ParseValueOrPercentOrProportional(const nsString& aString,
                                                        nsHTMLValue& aResult, 
                                                        nsHTMLUnit aValueUnit)
{ // XXX should have min/max values?
  nsAutoString tmp(aString);
  tmp.CompressWhitespace(PR_TRUE, PR_TRUE);
  PRInt32 ec, val = tmp.ToInteger(&ec);
  if (tmp.Last() == '%') {/* XXX not 100% compatible with ebina's code */
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    aResult.SetPercentValue(float(val)/100.0f);
	} else if (tmp.Last() == '*') {
    if (val < 0) val = 0;
    aResult.SetIntValue(val, eHTMLUnit_Proportional); // proportional values are integers
  } else {
    if (eHTMLUnit_Pixel == aValueUnit) {
      aResult.SetPixelValue(val);
    }
    else {
      aResult.SetIntValue(val, aValueUnit);
    }
  }
}

PRBool
nsGenericHTMLElement::ValueOrPercentToString(const nsHTMLValue& aValue,
                                             nsString& aResult)
{
  aResult.Truncate(0);
  switch (aValue.GetUnit()) {
  case eHTMLUnit_Integer:
    aResult.Append(aValue.GetIntValue(), 10);
    return PR_TRUE;
  case eHTMLUnit_Pixel:
    aResult.Append(aValue.GetPixelValue(), 10);
    return PR_TRUE;
  case eHTMLUnit_Percent:
    aResult.Append(PRInt32(aValue.GetPercentValue() * 100.0f), 10);
    aResult.Append('%');
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseValue(const nsString& aString, PRInt32 aMin,
                                 nsHTMLValue& aResult, nsHTMLUnit aValueUnit)
{
  PRInt32 ec, val = aString.ToInteger(&ec);
  if (NS_OK == ec) {
    if (val < aMin) val = aMin;
    if (eHTMLUnit_Pixel == aValueUnit) {
      aResult.SetPixelValue(val);
    }
    else {
      aResult.SetIntValue(val, aValueUnit);
    }
    return PR_TRUE;
  }

  // Illegal values are mapped to empty
  aResult.SetEmptyValue();
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseValue(const nsString& aString, PRInt32 aMin,
                                 PRInt32 aMax,
                                 nsHTMLValue& aResult, nsHTMLUnit aValueUnit)
{
  PRInt32 ec, val = aString.ToInteger(&ec);
  if (NS_OK == ec) {
    if (val < aMin) val = aMin;
    if (val > aMax) val = aMax;
    if (eHTMLUnit_Pixel == aValueUnit) {
      aResult.SetPixelValue(val);
    }
    else {
      aResult.SetIntValue(val, aValueUnit);
    }
    return PR_TRUE;
  }

  // Illegal values are mapped to empty
  aResult.SetEmptyValue();
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseColor(const nsString& aString,
                                 nsHTMLValue& aResult)
{
  if (aString.Length() > 0) {
    nsAutoString  colorStr (aString);
    colorStr.CompressWhitespace();
    char cbuf[40];
    colorStr.ToCString(cbuf, sizeof(cbuf));
    nscolor color;
    if (NS_ColorNameToRGB(cbuf, &color)) {
      aResult.SetStringValue(colorStr);
      return PR_TRUE;
    }
    if (NS_HexToRGB(cbuf, &color)) {
      aResult.SetColorValue(color);
      return PR_TRUE;
    }
  }

  // Illegal values are mapped to empty
  aResult.SetEmptyValue();
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ColorToString(const nsHTMLValue& aValue,
                                    nsString& aResult)
{
  if (aValue.GetUnit() == eHTMLUnit_Color) {
    nscolor v = aValue.GetColorValue();
    char buf[10];
    PR_snprintf(buf, sizeof(buf), "#%02x%02x%02x",
                NS_GET_R(v), NS_GET_G(v), NS_GET_B(v));
    aResult.Truncate(0);
    aResult.Append(buf);
    return PR_TRUE;
  }
  if (aValue.GetUnit() == eHTMLUnit_String) {
    aValue.GetStringValue(aResult);
    return PR_TRUE;
  }
  if (aValue.GetUnit() == eHTMLUnit_Empty) {  // was illegal
    aResult.Truncate();
    return PR_TRUE;
  }
  return PR_FALSE;
}

// XXX check all mappings against ebina's usage
static nsGenericHTMLElement::EnumTable kAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },
  { "texttop", NS_STYLE_VERTICAL_ALIGN_TEXT_TOP },
  { "baseline", NS_STYLE_VERTICAL_ALIGN_BASELINE },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "bottom", NS_STYLE_VERTICAL_ALIGN_BOTTOM },
  { "top", NS_STYLE_VERTICAL_ALIGN_TOP },
  { "middle", NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { "absbottom", NS_STYLE_VERTICAL_ALIGN_BOTTOM },
  { "abscenter", NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { "absmiddle", NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kDivAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "middle", NS_STYLE_TEXT_ALIGN_CENTER },
  { "justify", NS_STYLE_TEXT_ALIGN_JUSTIFY },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kFrameborderQuirksTable[] = {
  { "yes", NS_STYLE_FRAME_YES },
  { "no", NS_STYLE_FRAME_NO },
  { "1", NS_STYLE_FRAME_1 },
  { "0", NS_STYLE_FRAME_0 },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kFrameborderStandardTable[] = {
  { "1", NS_STYLE_FRAME_1 },
  { "0", NS_STYLE_FRAME_0 },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kScrollingQuirksTable[] = {
  { "yes", NS_STYLE_FRAME_YES },
  { "no", NS_STYLE_FRAME_NO },
  { "on", NS_STYLE_FRAME_ON },
  { "off", NS_STYLE_FRAME_OFF },
  { "scroll", NS_STYLE_FRAME_SCROLL },
  { "noscroll", NS_STYLE_FRAME_NOSCROLL },
  { "auto", NS_STYLE_FRAME_AUTO },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kScrollingStandardTable[] = {
  { "yes", NS_STYLE_FRAME_YES },
  { "no", NS_STYLE_FRAME_NO },
  { "auto", NS_STYLE_FRAME_AUTO },
  { 0 }
};

PRBool
nsGenericHTMLElement::ParseAlignValue(const nsString& aString,
                                      nsHTMLValue& aResult)
{
  return ParseEnumValue(aString, kAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::AlignValueToString(const nsHTMLValue& aValue,
                                         nsString& aResult)
{
  return EnumValueToString(aValue, kAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseDivAlignValue(const nsString& aString,
                                         nsHTMLValue& aResult)
{
  return ParseEnumValue(aString, kDivAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::DivAlignValueToString(const nsHTMLValue& aValue,
                                            nsString& aResult)
{
  return EnumValueToString(aValue, kDivAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseImageAttribute(nsIAtom* aAttribute,
                                          const nsString& aString,
                                          nsHTMLValue& aResult)
{
  if ((aAttribute == nsHTMLAtoms::width) ||
      (aAttribute == nsHTMLAtoms::height)) {
    ParseValueOrPercent(aString, aResult, eHTMLUnit_Pixel);
    return PR_TRUE;
  }
  else if ((aAttribute == nsHTMLAtoms::hspace) ||
           (aAttribute == nsHTMLAtoms::vspace) ||
           (aAttribute == nsHTMLAtoms::border)) {
    ParseValue(aString, 0, aResult, eHTMLUnit_Pixel);
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ImageAttributeToString(nsIAtom* aAttribute,
                                             const nsHTMLValue& aValue,
                                             nsString& aResult)
{
  if ((aAttribute == nsHTMLAtoms::width) ||
      (aAttribute == nsHTMLAtoms::height) ||
      (aAttribute == nsHTMLAtoms::border) ||
      (aAttribute == nsHTMLAtoms::hspace) ||
      (aAttribute == nsHTMLAtoms::vspace)) {
    return ValueOrPercentToString(aValue, aResult);
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseFrameborderValue(PRBool aStandardMode,
                                            const nsString& aString,
                                            nsHTMLValue& aResult)
{
  if (aStandardMode) {
    return ParseEnumValue(aString, kFrameborderStandardTable, aResult);
  } else {
    return ParseEnumValue(aString, kFrameborderQuirksTable, aResult);
  }
}

PRBool
nsGenericHTMLElement::FrameborderValueToString(PRBool aStandardMode,
                                               const nsHTMLValue& aValue,
                                               nsString& aResult)
{
  if (aStandardMode) {
    return EnumValueToString(aValue, kFrameborderStandardTable, aResult);
  } else {
    return EnumValueToString(aValue, kFrameborderQuirksTable, aResult);
  }
}

PRBool
nsGenericHTMLElement::ParseScrollingValue(PRBool aStandardMode,
                                          const nsString& aString,
                                          nsHTMLValue& aResult)
{
  if (aStandardMode) {
    return ParseEnumValue(aString, kScrollingStandardTable, aResult);
  } else {
    return ParseEnumValue(aString, kScrollingQuirksTable, aResult);
  }
}

PRBool
nsGenericHTMLElement::ScrollingValueToString(PRBool aStandardMode,
                                             const nsHTMLValue& aValue,
                                             nsString& aResult)
{
  if (aStandardMode) {
    return EnumValueToString(aValue, kScrollingStandardTable, aResult);
  } else {
    return EnumValueToString(aValue, kScrollingQuirksTable, aResult);
  }
}

void
nsGenericHTMLElement::MapImageAttributesInto(nsIHTMLAttributes* aAttributes, 
                                             nsIStyleContext* aContext, 
                                             nsIPresContext* aPresContext)
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;

    float p2t = aPresContext->GetPixelsToTwips();
    nsStylePosition* pos = (nsStylePosition*)
      aContext->GetMutableStyleData(eStyleStruct_Position);
    nsStyleSpacing* spacing = (nsStyleSpacing*)
      aContext->GetMutableStyleData(eStyleStruct_Spacing);

    // width: value
    aAttributes->GetAttribute(nsHTMLAtoms::width, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      pos->mWidth.SetCoordValue(twips);
    }
    else if (value.GetUnit() == eHTMLUnit_Percent) {
      pos->mWidth.SetPercentValue(value.GetPercentValue());
    }

    // height: value
    aAttributes->GetAttribute(nsHTMLAtoms::height, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      pos->mHeight.SetCoordValue(twips);
    }
    else if (value.GetUnit() == eHTMLUnit_Percent) {
      pos->mHeight.SetPercentValue(value.GetPercentValue());
    }

    // hspace: value
    aAttributes->GetAttribute(nsHTMLAtoms::hspace, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      spacing->mMargin.SetRight(nsStyleCoord(twips));
    }
    else if (value.GetUnit() == eHTMLUnit_Percent) {
      spacing->mMargin.SetRight(nsStyleCoord(value.GetPercentValue(),
                                             eStyleUnit_Coord));
    }

    // vspace: value
    aAttributes->GetAttribute(nsHTMLAtoms::vspace, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      spacing->mMargin.SetBottom(nsStyleCoord(twips));
    }
    else if (value.GetUnit() == eHTMLUnit_Percent) {
      spacing->mMargin.SetBottom(nsStyleCoord(value.GetPercentValue(),
                                              eStyleUnit_Coord));
    }
  }
}

void
nsGenericHTMLElement::MapImageAlignAttributeInto(nsIHTMLAttributes* aAttributes,
                                                 nsIStyleContext* aContext,
                                                 nsIPresContext* aPresContext)
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;
    aAttributes->GetAttribute(nsHTMLAtoms::align, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      PRUint8 align = value.GetIntValue();
      nsStyleDisplay* display = (nsStyleDisplay*)
        aContext->GetMutableStyleData(eStyleStruct_Display);
      nsStyleText* text = (nsStyleText*)
        aContext->GetMutableStyleData(eStyleStruct_Text);
      nsStyleSpacing* spacing = (nsStyleSpacing*)
        aContext->GetMutableStyleData(eStyleStruct_Spacing);
      float p2t = aPresContext->GetPixelsToTwips();
      nsStyleCoord three(NSIntPixelsToTwips(3, p2t));
      switch (align) {
      case NS_STYLE_TEXT_ALIGN_LEFT:
        display->mFloats = NS_STYLE_FLOAT_LEFT;
        spacing->mMargin.SetLeft(three);
        spacing->mMargin.SetRight(three);
        break;
      case NS_STYLE_TEXT_ALIGN_RIGHT:
        display->mFloats = NS_STYLE_FLOAT_RIGHT;
        spacing->mMargin.SetLeft(three);
        spacing->mMargin.SetRight(three);
        break;
      default:
        text->mVerticalAlign.SetIntValue(align, eStyleUnit_Enumerated);
        break;
      }
    }
  }
}

void
nsGenericHTMLElement::MapImageBorderAttributesInto(nsIHTMLAttributes* aAttributes, 
                                                   nsIStyleContext* aContext, 
                                                   nsIPresContext* aPresContext,
                                                   nscolor aBorderColors[4])
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;

    // border: pixels
    aAttributes->GetAttribute(nsHTMLAtoms::border, value);
    if (value.GetUnit() != eHTMLUnit_Pixel) {
      if (nsnull == aBorderColors) {
        return;
      }
      // If no border is defined and we are forcing a border, force
      // the size to 2 pixels.
      value.SetPixelValue(2);
    }

    float p2t = aPresContext->GetPixelsToTwips();
    nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);

    // Fixup border-padding sums: subtract out the old size and then
    // add in the new size.
    nsStyleSpacing* spacing = (nsStyleSpacing*)
      aContext->GetMutableStyleData(eStyleStruct_Spacing);
    nsStyleCoord coord;
    coord.SetCoordValue(twips);
    spacing->mBorder.SetTop(coord);
    spacing->mBorder.SetRight(coord);
    spacing->mBorder.SetBottom(coord);
    spacing->mBorder.SetLeft(coord);
    spacing->mBorderStyle[0] = NS_STYLE_BORDER_STYLE_SOLID;
    spacing->mBorderStyle[1] = NS_STYLE_BORDER_STYLE_SOLID;
    spacing->mBorderStyle[2] = NS_STYLE_BORDER_STYLE_SOLID;
    spacing->mBorderStyle[3] = NS_STYLE_BORDER_STYLE_SOLID;

    // Use supplied colors if provided, otherwise use color for border
    // color
    if (nsnull != aBorderColors) {
      spacing->mBorderColor[0] = aBorderColors[0];
      spacing->mBorderColor[1] = aBorderColors[1];
      spacing->mBorderColor[2] = aBorderColors[2];
      spacing->mBorderColor[3] = aBorderColors[3];
    }
    else {
      // Color is inherited from "color"
      const nsStyleColor* styleColor = (const nsStyleColor*)
        aContext->GetStyleData(eStyleStruct_Color);
      nscolor color = styleColor->mColor;
      spacing->mBorderColor[0] = color;
      spacing->mBorderColor[1] = color;
      spacing->mBorderColor[2] = color;
      spacing->mBorderColor[3] = color;
    }
  }
}

void
nsGenericHTMLElement::MapBackgroundAttributesInto(nsIHTMLAttributes* aAttributes, 
                                                  nsIStyleContext* aContext,
                                                  nsIPresContext* aPresContext)
{
  nsHTMLValue value;

  // background
  if (NS_CONTENT_ATTR_HAS_VALUE ==
      aAttributes->GetAttribute(nsHTMLAtoms::background, value)) {
    if (eHTMLUnit_String == value.GetUnit()) {
      nsAutoString absURLSpec;
      nsAutoString spec;
      value.GetStringValue(spec);
      if (spec.Length() > 0) {
        // Resolve url to an absolute url
        nsIURL* docURL = nsnull;
        aPresContext->GetBaseURL(docURL);

        nsresult rv = NS_MakeAbsoluteURL(docURL, "", spec, absURLSpec);
        NS_IF_RELEASE(docURL);

        nsStyleColor* color = (nsStyleColor*)
          aContext->GetMutableStyleData(eStyleStruct_Color);
        color->mBackgroundImage = absURLSpec;
        color->mBackgroundFlags &= ~NS_STYLE_BG_IMAGE_NONE;
        color->mBackgroundRepeat = NS_STYLE_BG_REPEAT_XY;
      }
    }
  }

  // bgcolor
  if (NS_CONTENT_ATTR_HAS_VALUE == aAttributes->GetAttribute(nsHTMLAtoms::bgcolor, value)) {
    if (eHTMLUnit_Color == value.GetUnit()) {
      nsStyleColor* color = (nsStyleColor*)
        aContext->GetMutableStyleData(eStyleStruct_Color);
      color->mBackgroundColor = value.GetColorValue();
      color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
    }
    else if (eHTMLUnit_String == value.GetUnit()) {
      nsAutoString buffer;
      value.GetStringValue(buffer);
      char cbuf[40];
      buffer.ToCString(cbuf, sizeof(cbuf));

      nsStyleColor* color = (nsStyleColor*)
        aContext->GetMutableStyleData(eStyleStruct_Color);
      NS_ColorNameToRGB(cbuf, &(color->mBackgroundColor));
      color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
    }
  }
}

void
nsGenericHTMLElement::TriggerLink(nsIPresContext& aPresContext,
                                  const nsString& aBase,
                                  const nsString& aURLSpec,
                                  const nsString& aTargetSpec,
                                  PRBool aClick)
{
  nsILinkHandler* handler;
  if (NS_OK == aPresContext.GetLinkHandler(&handler) && (nsnull != handler)) {
    // Resolve url to an absolute url
    nsIURL* docURL = nsnull;
    nsIDocument* doc;
    if (NS_OK == GetDocument(doc)) {
      docURL = doc->GetDocumentURL();
      NS_RELEASE(doc);
    }

    nsAutoString absURLSpec;
    if (aURLSpec.Length() > 0) {
      nsresult rv = NS_MakeAbsoluteURL(docURL, aBase, aURLSpec, absURLSpec);
    }
    else {
      absURLSpec = aURLSpec;
    }

    if (nsnull != docURL) {
      NS_RELEASE(docURL);
    }

    // Now pass on absolute url to the click handler
    if (aClick) {
      handler->OnLinkClick(nsnull, absURLSpec, aTargetSpec);
    }
    else {
      handler->OnOverLink(nsnull, absURLSpec, aTargetSpec);
    }
    NS_RELEASE(handler);
  }
}

//----------------------------------------------------------------------

nsGenericHTMLLeafElement::nsGenericHTMLLeafElement()
{
}

nsGenericHTMLLeafElement::~nsGenericHTMLLeafElement()
{
}

nsresult
nsGenericHTMLLeafElement::CopyInnerTo(nsIHTMLContent* aSrcContent,
                                      nsGenericHTMLLeafElement* aDst)
{
  aDst->mContent = aSrcContent;
  // XXX should the node's document be set?
  // XXX copy attributes not yet impelemented
  return NS_OK;
}

nsresult
nsGenericHTMLLeafElement::Equals(nsIDOMNode* aNode, PRBool aDeep,
                                 PRBool* aReturn)
{
  // XXX not yet implemented
  *aReturn = PR_FALSE;
  return NS_OK;
}

nsresult
nsGenericHTMLLeafElement::BeginConvertToXIF(nsXIFConverter& aConverter) const
{
  nsresult rv = NS_OK;
  if (nsnull != mTag)
  {
    nsAutoString name;
    mTag->ToString(name);
    aConverter.BeginLeaf(name);    
  }

  // Add all attributes to the convert
  if (nsnull != mAttributes) 
  {
    nsISupportsArray* attrs;
    rv = NS_NewISupportsArray(&attrs);
    if (NS_OK == rv) 
    {
      PRInt32 i, n;
      mAttributes->GetAllAttributeNames(attrs, n);
      nsAutoString name, value;
      for (i = 0; i < n; i++) 
      {
        nsIAtom* atom = (nsIAtom*) attrs->ElementAt(i);
        atom->ToString(name);

        value.Truncate();
        GetAttribute(name, value);
        
        aConverter.AddHTMLAttribute(name,value);
      }
      NS_RELEASE(attrs);
    }
  }
  return rv;
}

nsresult
nsGenericHTMLLeafElement::ConvertContentToXIF(nsXIFConverter& aConverter) const
{
  return NS_OK;
}

nsresult
nsGenericHTMLLeafElement::FinishConvertToXIF(nsXIFConverter& aConverter) const
{
  if (nsnull != mTag)
  {
    nsAutoString name;
    mTag->ToString(name);
    aConverter.EndLeaf(name);    
  }
  return NS_OK;
}

// XXX not really implemented (yet)
nsresult
nsGenericHTMLLeafElement::SizeOf(nsISizeOfHandler* aHandler) const
{
  aHandler->Add(sizeof(*this));
  return NS_OK;
}

//----------------------------------------------------------------------

nsChildContentList::nsChildContentList()
{
  NS_INIT_REFCNT();
  mScriptObject = nsnull;
}

NS_IMPL_ADDREF(nsChildContentList);
NS_IMPL_RELEASE(nsChildContentList);

nsresult
nsChildContentList::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIDOMNodeListIID)) {
    nsIDOMNodeList* tmp = this;
    *aInstancePtr = (void*)tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptObjectOwnerIID)) {
    nsIScriptObjectOwner* tmp = this;
    *aInstancePtr = (void*)tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    nsIDOMNodeList* tmp1 = this;
    nsISupports* tmp2 = tmp1;
    *aInstancePtr = (void*)tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult 
nsChildContentList::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    res = NS_NewScriptNodeList(aContext, (nsISupports *)(nsIDOMNodeList *)this, nsnull, (void**)&mScriptObject);
  }
  *aScriptObject = mScriptObject;
  return res;
}

nsresult 
nsChildContentList::ResetScriptObject()
{
  mScriptObject = nsnull;
  return NS_OK;
}

NS_IMETHODIMP    
nsChildContentList::GetLength(PRUint32* aLength)
{
  *aLength = (PRUint32)mArray.Count();
  return NS_OK;
}

NS_IMETHODIMP
nsChildContentList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsIContent *content;
  nsresult res = NS_OK;
  
  content = (nsIContent *)mArray.ElementAt(aIndex);
  if (nsnull != content) {
    res = content->QueryInterface(kIDOMNodeIID, (void**)aReturn);
  }
  else {
    *aReturn = nsnull;
  }

  return res;
}


//----------------------------------------------------------------------

nsGenericHTMLContainerElement::nsGenericHTMLContainerElement()
{
  mChildren = new nsChildContentList();
  mChildren->AddRef();
}

nsGenericHTMLContainerElement::~nsGenericHTMLContainerElement()
{
  PRInt32 n = mChildren->Count();
  for (PRInt32 i = 0; i < n; i++) {
    nsIContent* kid = mChildren->ElementAt(i);
    NS_RELEASE(kid);
  }
  mChildren->Release();
}

nsresult
nsGenericHTMLContainerElement::CopyInnerTo(nsIHTMLContent* aSrcContent,
                                           nsGenericHTMLContainerElement* aDst)
{
  aDst->mContent = aSrcContent;
  // XXX should the node's document be set?
  // XXX copy attributes not yet impelemented
  // XXX deep copy?
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::Equals(nsIDOMNode* aNode, PRBool aDeep,
                                      PRBool* aReturn)
{
  // XXX not yet implemented
  *aReturn = PR_FALSE;
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  return mChildren->QueryInterface(kIDOMNodeListIID, (void **)aChildNodes);
}

nsresult
nsGenericHTMLContainerElement::GetHasChildNodes(PRBool* aReturn)
{
  if (0 != mChildren->Count()) {
    *aReturn = PR_TRUE;
  } 
  else {
    *aReturn = PR_FALSE;
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::GetFirstChild(nsIDOMNode** aNode)
{
  nsIContent *child = mChildren->ElementAt(0);
  if (nsnull != child) {
    nsresult res = child->QueryInterface(kIDOMNodeIID, (void**)aNode);
    NS_ASSERTION(NS_OK == res, "Must be a DOM Node"); // must be a DOM Node
    return res;
  }
  aNode = nsnull;
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::GetLastChild(nsIDOMNode** aNode)
{
  nsIContent *child = mChildren->ElementAt(mChildren->Count()-1);
  if (nsnull != child) {
    nsresult res = child->QueryInterface(kIDOMNodeIID, (void**)aNode);
    NS_ASSERTION(NS_OK == res, "Must be a DOM Node"); // must be a DOM Node
    return res;
  }
  aNode = nsnull;
  return NS_OK;
}

static void
SetDocumentInChildrenOf(nsIContent* aContent, nsIDocument* aDocument)
{
  PRInt32 i, n;
  aContent->ChildCount(n);
  for (i = 0; i < n; i++) {
    nsIContent* child;
    aContent->ChildAt(i, child);
    if (nsnull != child) {
      child->SetDocument(aDocument);
      SetDocumentInChildrenOf(child, aDocument);
    }
  }
}

// XXX It's possible that newChild has already been inserted in the
// tree; if this is the case then we need to remove it from where it
// was before placing it in it's new home

nsresult
nsGenericHTMLContainerElement::InsertBefore(nsIDOMNode* aNewChild,
                                            nsIDOMNode* aRefChild,
                                            nsIDOMNode** aReturn)
{
  if (nsnull == aNewChild) {
    *aReturn = nsnull;
    return NS_OK;/* XXX wrong error value */
  }

  // Get the nsIContent interface for the new content
  nsIContent* newContent = nsnull;
  nsresult res = aNewChild->QueryInterface(kIContentIID, (void**)&newContent);
  NS_ASSERTION(NS_OK == res, "New child must be an nsIContent");
  if (NS_OK == res) {
    if (nsnull == aRefChild) {
      // Append the new child to the end
      SetDocumentInChildrenOf(newContent, mDocument);
      res = AppendChildTo(newContent, PR_TRUE);
    }
    else {
      // Get the index of where to insert the new child
      nsIContent* refContent = nsnull;
      res = aRefChild->QueryInterface(kIContentIID, (void**)&refContent);
      NS_ASSERTION(NS_OK == res, "Ref child must be an nsIContent");
      if (NS_OK == res) {
        PRInt32 pos;
        IndexOf(refContent, pos);
        if (pos >= 0) {
          SetDocumentInChildrenOf(newContent, mDocument);
          res = InsertChildAt(newContent, pos, PR_TRUE);
        }
        NS_RELEASE(refContent);
      }
    }
    NS_RELEASE(newContent);

    *aReturn = aNewChild;
    NS_ADDREF(aNewChild);
  }
  else {
    *aReturn = nsnull;
  }

  return res;
}

nsresult
nsGenericHTMLContainerElement::ReplaceChild(nsIDOMNode* aNewChild,
                                            nsIDOMNode* aOldChild,
                                            nsIDOMNode** aReturn)
{
  nsIContent* content = nsnull;
  *aReturn = nsnull;
  nsresult res = aOldChild->QueryInterface(kIContentIID, (void**)&content);
  NS_ASSERTION(NS_OK == res, "Must be an nsIContent");
  if (NS_OK == res) {
    PRInt32 pos;
    IndexOf(content, pos);
    if (pos >= 0) {
      nsIContent* newContent = nsnull;
      nsresult res = aNewChild->QueryInterface(kIContentIID, (void**)&newContent);
      NS_ASSERTION(NS_OK == res, "Must be an nsIContent");
      if (NS_OK == res) {
        res = ReplaceChildAt(newContent, pos, PR_TRUE);
        NS_RELEASE(newContent);
      }
      *aReturn = aOldChild;
      NS_ADDREF(aOldChild);
    }
    NS_RELEASE(content);
  }
  
  return res;
}

nsresult
nsGenericHTMLContainerElement::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  nsIContent* content = nsnull;
  *aReturn = nsnull;
  nsresult res = aOldChild->QueryInterface(kIContentIID, (void**)&content);
  NS_ASSERTION(NS_OK == res, "Must be an nsIContent");
  if (NS_OK == res) {
    PRInt32 pos;
    IndexOf(content, pos);
    if (pos >= 0) {
      res = RemoveChildAt(pos, PR_TRUE);
      *aReturn = aOldChild;
      NS_ADDREF(aOldChild);
    }
    NS_RELEASE(content);
  }

  return res;
}

nsresult
nsGenericHTMLContainerElement::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
  return InsertBefore(aNewChild, nsnull, aReturn);
}

nsresult
nsGenericHTMLContainerElement::SizeOf(nsISizeOfHandler* aHandler) const
{
  aHandler->Add(sizeof(*this));
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::BeginConvertToXIF(nsXIFConverter& aConverter) const
{
  nsresult rv = NS_OK;
  if (nsnull != mTag)
  {
    nsAutoString name;
    mTag->ToString(name);
    aConverter.BeginContainer(name);
  }

  // Add all attributes to the convert
  if (nsnull != mAttributes) 
  {
    nsISupportsArray* attrs;
    rv = NS_NewISupportsArray(&attrs);
    if (NS_OK == rv) 
    {
      PRInt32 i, n;
      mAttributes->GetAllAttributeNames(attrs, n);
      nsAutoString name, value;
      for (i = 0; i < n; i++) 
      {
        nsIAtom* atom = (nsIAtom*) attrs->ElementAt(i);
        atom->ToString(name);

        value.Truncate();
        GetAttribute(name, value);
        
        aConverter.AddHTMLAttribute(name,value);
      }
      NS_RELEASE(attrs);
    }
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::ConvertContentToXIF(nsXIFConverter& aConverter) const
{
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::FinishConvertToXIF(nsXIFConverter& aConverter) const
{
  if (nsnull != mTag)
  {
    nsAutoString name;
    mTag->ToString(name);
    aConverter.EndContainer(name);
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::Compact()
{
  mChildren->Compact();
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::CanContainChildren(PRBool& aResult) const
{
  aResult = PR_TRUE;
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::ChildCount(PRInt32& aCount) const
{
  aCount = mChildren->Count();
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::ChildAt(PRInt32 aIndex,
                                       nsIContent*& aResult) const
{
  nsIContent *child = mChildren->ElementAt(aIndex);
  if (nsnull != child) {
    NS_ADDREF(child);
  }
  aResult = child;
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::IndexOf(nsIContent* aPossibleChild,
                                       PRInt32& aIndex) const
{
  NS_PRECONDITION(nsnull != aPossibleChild, "null ptr");
  aIndex = mChildren->IndexOf(aPossibleChild);
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::InsertChildAt(nsIContent* aKid,
                                             PRInt32 aIndex,
                                             PRBool aNotify)
{
  NS_PRECONDITION(nsnull != aKid, "null ptr");
  PRBool rv = mChildren->InsertElementAt(aKid, aIndex);/* XXX fix up void array api to use nsresult's*/
  if (rv) {
    NS_ADDREF(aKid);
    aKid->SetParent(mContent);
    nsIDocument* doc = mDocument;
    if (nsnull != doc) {
      aKid->SetDocument(doc);
      if (aNotify) {
        doc->ContentInserted(mContent, aKid, aIndex);
      }
    }
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::ReplaceChildAt(nsIContent* aKid,
                                              PRInt32 aIndex,
                                              PRBool aNotify)
{
  NS_PRECONDITION(nsnull != aKid, "null ptr");
  nsIContent* oldKid = mChildren->ElementAt(aIndex);
  PRBool rv = mChildren->ReplaceElementAt(aKid, aIndex);
  if (rv) {
    NS_ADDREF(aKid);
    aKid->SetParent(mContent);
    nsIDocument* doc = mDocument;
    if (nsnull != doc) {
      aKid->SetDocument(doc);
      if (aNotify) {
        doc->ContentReplaced(mContent, oldKid, aKid, aIndex);
      }
    }
    oldKid->SetDocument(nsnull);
    oldKid->SetParent(nsnull);
    NS_RELEASE(oldKid);
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::AppendChildTo(nsIContent* aKid, PRBool aNotify)
{
  NS_PRECONDITION((nsnull != aKid) && (aKid != mContent), "null ptr");
  PRBool rv = mChildren->AppendElement(aKid);
  if (rv) {
    NS_ADDREF(aKid);
    aKid->SetParent(mContent);
    nsIDocument* doc = mDocument;
    if (nsnull != doc) {
      aKid->SetDocument(doc);
      if (aNotify) {
        doc->ContentAppended(mContent, mChildren->Count() - 1);
      }
    }
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::RemoveChildAt(PRInt32 aIndex, PRBool aNotify)
{
  nsIContent* oldKid = mChildren->ElementAt(aIndex);
  if (nsnull != oldKid ) {
    nsIDocument* doc = mDocument;
    if (aNotify) {
      if (nsnull != doc) {
        doc->ContentWillBeRemoved(mContent, oldKid, aIndex);
      }
    }
    PRBool rv = mChildren->RemoveElementAt(aIndex);
    if (aNotify) {
      if (nsnull != doc) {
        doc->ContentHasBeenRemoved(mContent, oldKid, aIndex);
      }
    }
    oldKid->SetDocument(nsnull);
    oldKid->SetParent(nsnull);
    NS_RELEASE(oldKid);
  }

  return NS_OK;
}
