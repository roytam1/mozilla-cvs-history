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
#ifndef nsGenericElement_h___
#define nsGenericElement_h___

#include "nsIContent.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsVoidArray.h"
#include "nsIScriptObjectOwner.h"
#include "nsIJSScriptObject.h"

extern const nsIID kIDOMNodeIID;
extern const nsIID kIDOMElementIID;
extern const nsIID kIDOMEventReceiverIID;
extern const nsIID kIScriptObjectOwnerIID;
extern const nsIID kIJSScriptObjectIID;
extern const nsIID kISupportsIID;
extern const nsIID kIContentIID;

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIEventListenerManager;
class nsIFrame;
class nsISupportsArray;
class nsIDOMScriptObjectFactory;
class nsDOMCSSDeclaration;
class nsIDOMCSSStyleDeclaration;
class nsIHTMLAttributes;

// Attribute helper class used to wrap up an attribute with a dom
// object that implements nsIDOMAttr and nsIDOMNode and
// nsIScriptObjectOwner
class nsDOMAttribute : public nsIDOMAttr, public nsIScriptObjectOwner {
public:
  nsDOMAttribute(const nsString &aName, const nsString &aValue);
  ~nsDOMAttribute();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetScriptObject(nsIScriptContext* aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void *aScriptObject);

  // nsIDOMAttr interface
  NS_IMETHOD GetSpecified(PRBool* aSpecified);
  NS_IMETHOD GetName(nsString& aReturn);
  NS_IMETHOD GetValue(nsString& aReturn);
  NS_IMETHOD SetValue(const nsString& aValue);
  
  // nsIDOMNode interface
  NS_IMETHOD GetNodeName(nsString& aNodeName);
  NS_IMETHOD GetNodeValue(nsString& aNodeValue);
  NS_IMETHOD SetNodeValue(const nsString& aNodeValue);
  NS_IMETHOD GetNodeType(PRUint16* aNodeType);
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode);
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes);
  NS_IMETHOD HasChildNodes(PRBool* aHasChildNodes);
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
  NS_IMETHOD CloneNode(PRBool aDeep, nsIDOMNode** aReturn);
  NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument);

private:
  nsString mName;
  nsString mValue;
  void* mScriptObject;
};

// Another helper class that implements the nsIDOMNamedNodeMap interface.
class nsDOMAttributeMap : public nsIDOMNamedNodeMap,
			  public nsIScriptObjectOwner
{
public:
  nsDOMAttributeMap(nsIContent* aContent);
  ~nsDOMAttributeMap();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetScriptObject(nsIScriptContext* aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void *aScriptObject);

  // nsIDOMNamedNodeMap interface
  NS_IMETHOD GetLength(PRUint32* aSize);
  NS_IMETHOD GetNamedItem(const nsString& aName, nsIDOMNode** aReturn);
  NS_IMETHOD SetNamedItem(nsIDOMNode* aNode, nsIDOMNode** aReturn);
  NS_IMETHOD RemoveNamedItem(const nsString& aName, nsIDOMNode** aReturn);
  NS_IMETHOD Item(PRUint32 aIndex, nsIDOMNode** aReturn);

private:
  nsIContent* mContent;
  void* mScriptObject;
};


// Class that holds the child list of a content element and also
// implements the nsIDOMNodeList interface.
class nsChildContentList : public nsIDOMNodeList, 
                           public nsIScriptObjectOwner 
{
public:
  nsChildContentList(nsIContent *aContent);
  ~nsChildContentList() {}
  
  NS_DECL_ISUPPORTS
  
  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void *aScriptObject);
  
  // nsIDOMNodeList interface
  NS_DECL_IDOMNODELIST
  
  void DropReference();

private:
  nsIContent *mContent;
  void *mScriptObject;
};

// There are a set of DOM- and scripting-specific instance variables
// that may only be instantiated when a content object is accessed
// through the DOM. Rather than burn actual slots in the content
// objects for each of these instance variables, we put them off
// in a side structure that's only allocated when the content is
// accessed through the DOM.
typedef struct {
  void *mScriptObject;
  nsChildContentList *mChildNodes;
  nsDOMCSSDeclaration *mStyle;
} nsDOMSlots;

class nsGenericElement : public nsIJSScriptObject {
public:
  nsGenericElement();
  ~nsGenericElement();

  void Init(nsIContent* aOuterContentObject, nsIAtom* aTag);

  // Implementation for nsIDOMNode
  nsresult    GetNodeName(nsString& aNodeName);
  nsresult    GetNodeValue(nsString& aNodeValue);
  nsresult    SetNodeValue(const nsString& aNodeValue);
  nsresult    GetNodeType(PRUint16* aNodeType);
  nsresult    GetParentNode(nsIDOMNode** aParentNode);
  nsresult    GetAttributes(nsIDOMNamedNodeMap** aAttributes);
  nsresult    GetPreviousSibling(nsIDOMNode** aPreviousSibling);
  nsresult    GetNextSibling(nsIDOMNode** aNextSibling);
  nsresult    GetOwnerDocument(nsIDOMDocument** aOwnerDocument);

  // Implementation for nsIDOMElement
  nsresult    GetTagName(nsString& aTagName);
  nsresult    GetDOMAttribute(const nsString& aName, nsString& aReturn);
  nsresult    SetDOMAttribute(const nsString& aName, const nsString& aValue);
  nsresult    RemoveAttribute(const nsString& aName);
  nsresult    GetAttributeNode(const nsString& aName,
                               nsIDOMAttr** aReturn);
  nsresult    SetAttributeNode(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn);
  nsresult    RemoveAttributeNode(nsIDOMAttr* aOldAttr, nsIDOMAttr** aReturn);
  nsresult    GetElementsByTagName(const nsString& aTagname,
                                   nsIDOMNodeList** aReturn);
  nsresult    Normalize();

  // nsIDOMEventReceiver interface
  nsresult AddEventListener(nsIDOMEventListener *aListener, const nsIID& aIID);
  nsresult RemoveEventListener(nsIDOMEventListener* aListener,
                               const nsIID& aIID);
  nsresult GetListenerManager(nsIEventListenerManager** aInstancePtrResult);
  nsresult GetNewListenerManager(nsIEventListenerManager** aInstancePtrResult);

  // nsIScriptObjectOwner interface
  nsresult GetScriptObject(nsIScriptContext* aContext, void** aScriptObject);
  nsresult SetScriptObject(void *aScriptObject);

  // Implementation for nsIContent
  nsresult GetDocument(nsIDocument*& aResult) const;
  nsresult SetDocument(nsIDocument* aDocument, PRBool aDeep);
  nsresult GetParent(nsIContent*& aResult) const;
  nsresult SetParent(nsIContent* aParent);
  nsresult IsSynthetic(PRBool& aResult) {
    aResult = PR_FALSE;
    return NS_OK;
  }
  nsresult GetTag(nsIAtom*& aResult) const;
  nsresult HandleDOMEvent(nsIPresContext& aPresContext,
                          nsEvent* aEvent,
                          nsIDOMEvent** aDOMEvent,
                          PRUint32 aFlags,
                          nsEventStatus& aEventStatus);

  // Implementation for nsIJSScriptObject
  PRBool    AddProperty(JSContext *aContext, jsval aID, jsval *aVp);
  PRBool    DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp);
  PRBool    GetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  PRBool    SetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  PRBool    EnumerateProperty(JSContext *aContext);
  PRBool    Resolve(JSContext *aContext, jsval aID);
  PRBool    Convert(JSContext *aContext, jsval aID);
  void      Finalize(JSContext *aContext);

  // Implementation for nsISupports
  NS_IMETHOD QueryInterface(REFNSIID aIID,
                            void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  //----------------------------------------

  nsresult RenderFrame();

  nsresult AddScriptEventListener(nsIAtom* aAttribute,
                                  const nsString& aValue,
                                  REFNSIID aIID);

  void TriggerLink(nsIPresContext& aPresContext,
                   const nsString& aBase,
                   const nsString& aURLSpec,
                   const nsString& aTargetSpec,
                   PRBool aClick);

  static void SetDocumentInChildrenOf(nsIContent* aContent, 
				      nsIDocument* aDocument);

  static nsresult GetScriptObjectFactory(nsIDOMScriptObjectFactory **aFactory);

  static nsIDOMScriptObjectFactory *gScriptObjectFactory;

  nsDOMSlots *GetDOMSlots();

  // Up pointer to the real content object that we are
  // supporting. Sometimes there is work that we just can't do
  // ourselves, so this is needed to ask the real object to do the
  // work.
  nsIContent* mContent;

  nsIDocument* mDocument;
  nsIContent* mParent;
  nsIAtom* mTag;
  nsIEventListenerManager* mListenerManager;
  nsDOMSlots *mDOMSlots;
};

//----------------------------------------------------------------------

class nsGenericContainerElement : public nsGenericElement {
public:
  nsGenericContainerElement();
  ~nsGenericContainerElement();

  nsresult CopyInnerTo(nsIContent* aSrcContent,
                       nsGenericContainerElement* aDest);

  // Remainder of nsIDOMHTMLElement (and nsIDOMNode)
  nsresult    GetChildNodes(nsIDOMNodeList** aChildNodes);
  nsresult    HasChildNodes(PRBool* aHasChildNodes);
  nsresult    GetFirstChild(nsIDOMNode** aFirstChild);
  nsresult    GetLastChild(nsIDOMNode** aLastChild);
  
  nsresult    InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                           nsIDOMNode** aReturn);
  nsresult    ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                           nsIDOMNode** aReturn);
  nsresult    RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);
  nsresult    AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn);

  // Remainder of nsIContent
  nsresult SetAttribute(const nsString& aName, const nsString& aValue,
                        PRBool aNotify);
  nsresult GetAttribute(const nsString& aName, nsString& aResult) const;
  nsresult UnsetAttribute(nsIAtom* aAttribute, PRBool aNotify);
  nsresult GetAllAttributeNames(nsISupportsArray* aArray,
                                PRInt32& aCount) const;
  nsresult GetAttributeCount(PRInt32& aResult) const;
  nsresult List(FILE* out, PRInt32 aIndent) const;
  nsresult SizeOf(nsISizeOfHandler* aHandler) const;
  nsresult CanContainChildren(PRBool& aResult) const;
  nsresult ChildCount(PRInt32& aResult) const;
  nsresult ChildAt(PRInt32 aIndex, nsIContent*& aResult) const;
  nsresult IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const;
  nsresult InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify);
  nsresult ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify);
  nsresult AppendChildTo(nsIContent* aKid, PRBool aNotify);
  nsresult RemoveChildAt(PRInt32 aIndex, PRBool aNotify);
  nsresult BeginConvertToXIF(nsXIFConverter& aConverter) const;
  nsresult ConvertContentToXIF(nsXIFConverter& aConverter) const;
  nsresult FinishConvertToXIF(nsXIFConverter& aConverter) const;

  void ListAttributes(FILE* out) const;

  nsIHTMLAttributes* mAttributes;
  nsVoidArray mChildren;
};

//----------------------------------------------------------------------

/**
 * Mostly implement the nsIDOMNode API by forwarding the methods to a
 * generic content object (either nsGenericHTMLLeafElement or
 * nsGenericHTMLContainerContent)
 *
 * Note that classes using this macro will need to implement:
 *       NS_IMETHOD CloneNode(PRBool aDeep, nsIDOMNode** aReturn);
 */
#define NS_IMPL_IDOMNODE_USING_GENERIC(_g)                              \
  NS_IMETHOD GetNodeName(nsString& aNodeName) {                         \
    return _g.GetNodeName(aNodeName);                                   \
  }                                                                     \
  NS_IMETHOD GetNodeValue(nsString& aNodeValue) {                       \
    return _g.GetNodeValue(aNodeValue);                                 \
  }                                                                     \
  NS_IMETHOD SetNodeValue(const nsString& aNodeValue) {                 \
    return _g.SetNodeValue(aNodeValue);                                 \
  }                                                                     \
  NS_IMETHOD GetNodeType(PRUint16* aNodeType) {                         \
    return _g.GetNodeType(aNodeType);                                   \
  }                                                                     \
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode) {                  \
    return _g.GetParentNode(aParentNode);                               \
  }                                                                     \
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes) {              \
    return _g.GetChildNodes(aChildNodes);                               \
  }                                                                     \
  NS_IMETHOD HasChildNodes(PRBool* aHasChildNodes) {                    \
    return _g.HasChildNodes(aHasChildNodes);                            \
  }                                                                     \
  NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild) {                  \
    return _g.GetFirstChild(aFirstChild);                               \
  }                                                                     \
  NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild) {                    \
    return _g.GetLastChild(aLastChild);                                 \
  }                                                                     \
  NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling) {        \
    return _g.GetPreviousSibling(aPreviousSibling);                     \
  }                                                                     \
  NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling) {                \
    return _g.GetNextSibling(aNextSibling);                             \
  }                                                                     \
  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes) {          \
    return _g.GetAttributes(aAttributes);                               \
  }                                                                     \
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, \
                             nsIDOMNode** aReturn) {                    \
    return _g.InsertBefore(aNewChild, aRefChild, aReturn);              \
  }                                                                     \
  NS_IMETHOD AppendChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) { \
    return _g.AppendChild(aOldChild, aReturn);                          \
  }                                                                     \
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, \
                             nsIDOMNode** aReturn) {                    \
    return _g.ReplaceChild(aNewChild, aOldChild, aReturn);              \
  }                                                                     \
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) { \
    return _g.RemoveChild(aOldChild, aReturn);                          \
  }                                                                     \
  NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument) {        \
    return _g.GetOwnerDocument(aOwnerDocument);                         \
  }                                                                     \
  NS_IMETHOD CloneNode(PRBool aDeep, nsIDOMNode** aReturn);

/**
 * Implement the nsIDOMElement API by forwarding the methods to a
 * generic content object (either nsGenericHTMLLeafElement or
 * nsGenericHTMLContainerContent)
 */
#define NS_IMPL_IDOMELEMENT_USING_GENERIC(_g)                                 \
  NS_IMETHOD GetTagName(nsString& aTagName) {                                 \
    return _g.GetTagName(aTagName);                                           \
  }                                                                           \
  NS_IMETHOD GetDOMAttribute(const nsString& aName, nsString& aReturn) {      \
    return _g.GetDOMAttribute(aName, aReturn);                                \
  }                                                                           \
  NS_IMETHOD SetDOMAttribute(const nsString& aName, const nsString& aValue) { \
    return _g.SetDOMAttribute(aName, aValue);                                 \
  }                                                                           \
  NS_IMETHOD RemoveAttribute(const nsString& aName) {                         \
    return _g.RemoveAttribute(aName);                                         \
  }                                                                           \
  NS_IMETHOD GetAttributeNode(const nsString& aName,                          \
                              nsIDOMAttr** aReturn) {                         \
    return _g.GetAttributeNode(aName, aReturn);                               \
  }                                                                           \
  NS_IMETHOD SetAttributeNode(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn) {   \
    return _g.SetAttributeNode(aNewAttr, aReturn);                            \
  }                                                                           \
  NS_IMETHOD RemoveAttributeNode(nsIDOMAttr* aOldAttr, nsIDOMAttr** aReturn) {\
    return _g.RemoveAttributeNode(aOldAttr, aReturn);                         \
  }                                                                           \
  NS_IMETHOD GetElementsByTagName(const nsString& aTagname,                   \
                                  nsIDOMNodeList** aReturn) {                 \
    return _g.GetElementsByTagName(aTagname, aReturn);                        \
  }                                                                           \
  NS_IMETHOD Normalize() {                                                    \
    return _g.Normalize();                                                    \
  }

/**
 * Implement the nsIDOMEventReceiver API by forwarding the methods to a
 * generic content object (either nsGenericHTMLLeafElement or
 * nsGenericHTMLContainerContent)
 */
#define NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(_g)                     \
  NS_IMETHOD AddEventListener(nsIDOMEventListener *aListener,           \
                              const nsIID& aIID) {                      \
    return _g.AddEventListener(aListener, aIID);                        \
  }                                                                     \
  NS_IMETHOD RemoveEventListener(nsIDOMEventListener *aListener,        \
                                 const nsIID& aIID) {                   \
    return _g.RemoveEventListener(aListener, aIID);                     \
  }                                                                     \
  NS_IMETHOD GetListenerManager(nsIEventListenerManager** aResult) {    \
    return _g.GetListenerManager(aResult);                              \
  }                                                                     \
  NS_IMETHOD GetNewListenerManager(nsIEventListenerManager** aResult) { \
    return _g.GetNewListenerManager(aResult);                           \
  }

#define NS_IMPL_ICONTENT_USING_GENERIC(_g)                                 \
  NS_IMETHOD GetDocument(nsIDocument*& aResult) const {                    \
    return _g.GetDocument(aResult);                                        \
  }                                                                        \
  NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep) {           \
    return _g.SetDocument(aDocument, aDeep);                               \
  }                                                                        \
  NS_IMETHOD GetParent(nsIContent*& aResult) const {                       \
    return _g.GetParent(aResult);                                          \
  }                                                                        \
  NS_IMETHOD SetParent(nsIContent* aParent) {                              \
    return _g.SetParent(aParent);                                          \
  }                                                                        \
  NS_IMETHOD CanContainChildren(PRBool& aResult) const {                   \
    return _g.CanContainChildren(aResult);                                 \
  }                                                                        \
  NS_IMETHOD ChildCount(PRInt32& aResult) const {                          \
    return _g.ChildCount(aResult);                                         \
  }                                                                        \
  NS_IMETHOD ChildAt(PRInt32 aIndex, nsIContent*& aResult) const {         \
    return _g.ChildAt(aIndex, aResult);                                    \
  }                                                                        \
  NS_IMETHOD IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const { \
    return _g.IndexOf(aPossibleChild, aResult);                            \
  }                                                                        \
  NS_IMETHOD InsertChildAt(nsIContent* aKid, PRInt32 aIndex,               \
                           PRBool aNotify) {                               \
    return _g.InsertChildAt(aKid, aIndex, aNotify);                        \
  }                                                                        \
  NS_IMETHOD ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex,              \
                            PRBool aNotify) {                              \
    return _g.ReplaceChildAt(aKid, aIndex, aNotify);                       \
  }                                                                        \
  NS_IMETHOD AppendChildTo(nsIContent* aKid, PRBool aNotify) {             \
    return _g.AppendChildTo(aKid, aNotify);                                \
  }                                                                        \
  NS_IMETHOD RemoveChildAt(PRInt32 aIndex, PRBool aNotify) {               \
    return _g.RemoveChildAt(aIndex, aNotify);                              \
  }                                                                        \
  NS_IMETHOD IsSynthetic(PRBool& aResult) {                                \
    return _g.IsSynthetic(aResult);                                        \
  }                                                                        \
  NS_IMETHOD GetTag(nsIAtom*& aResult) const {                             \
    return _g.GetTag(aResult);                                             \
  }                                                                        \
  NS_IMETHOD SetAttribute(const nsString& aName, const nsString& aValue,   \
                          PRBool aNotify) {                                \
    return _g.SetAttribute(aName, aValue, aNotify);                        \
  }                                                                        \
  NS_IMETHOD GetAttribute(const nsString& aName,                           \
                          nsString& aResult) const {                       \
    return _g.GetAttribute(aName, aResult);                                \
  }                                                                        \
  NS_IMETHOD UnsetAttribute(nsIAtom* aAttribute, PRBool aNotify) {         \
    return _g.UnsetAttribute(aAttribute, aNotify);                         \
  }                                                                        \
  NS_IMETHOD GetAllAttributeNames(nsISupportsArray* aArray,                \
                                  PRInt32& aResult) const {                \
    return _g.GetAllAttributeNames(aArray, aResult);                       \
  }                                                                        \
  NS_IMETHOD GetAttributeCount(PRInt32& aResult) const {                   \
    return _g.GetAttributeCount(aResult);                                  \
  }                                                                        \
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const {                      \
    return _g.List(out, aIndent);                                          \
  }                                                                        \
  NS_IMETHOD BeginConvertToXIF(nsXIFConverter& aConverter) const {         \
    return _g.BeginConvertToXIF(aConverter);                               \
  }                                                                        \
  NS_IMETHOD ConvertContentToXIF(nsXIFConverter& aConverter) const {       \
    return _g.ConvertContentToXIF(aConverter);                             \
  }                                                                        \
  NS_IMETHOD FinishConvertToXIF(nsXIFConverter& aConverter) const {        \
    return _g.FinishConvertToXIF(aConverter);                              \
  }                                                                        \
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler) const {                    \
    return _g.SizeOf(aHandler);                                            \
  }                                                                        \
  NS_IMETHOD HandleDOMEvent(nsIPresContext& aPresContext,                  \
                            nsEvent* aEvent,                               \
                            nsIDOMEvent** aDOMEvent,                       \
                            PRUint32 aFlags,                               \
                            nsEventStatus& aEventStatus);

#define NS_IMPL_CONTENT_QUERY_INTERFACE(_id, _iptr, _this, _base) \
  if (_id.Equals(kISupportsIID)) {                              \
    _base* tmp = _this;                                         \
    nsISupports* tmp2 = tmp;                                    \
    *_iptr = (void*) tmp2;                                      \
    NS_ADDREF_THIS();                                           \
    return NS_OK;                                               \
  }                                                             \
  if (_id.Equals(kIDOMNodeIID)) {                               \
    nsIDOMNode* tmp = _this;                                    \
    *_iptr = (void*) tmp;                                       \
    NS_ADDREF_THIS();                                           \
    return NS_OK;                                               \
  }                                                             \
  if (_id.Equals(kIDOMElementIID)) {                            \
    nsIDOMElement* tmp = _this;                                 \
    *_iptr = (void*) tmp;                                       \
    NS_ADDREF_THIS();                                           \
    return NS_OK;                                               \
  }                                                             \
  if (_id.Equals(kIDOMEventReceiverIID)) {                      \
    nsIDOMEventReceiver* tmp = _this;                           \
    *_iptr = (void*) tmp;                                       \
    NS_ADDREF_THIS();                                           \
    return NS_OK;                                               \
  }                                                             \
  if (_id.Equals(kIScriptObjectOwnerIID)) {                     \
    nsIScriptObjectOwner* tmp = _this;                          \
    *_iptr = (void*) tmp;                                       \
    NS_ADDREF_THIS();                                           \
    return NS_OK;                                               \
  }                                                             \
  if (_id.Equals(kIContentIID)) {                               \
    _base* tmp = _this;                                         \
    nsIContent* tmp2 = tmp;                                     \
    *_iptr = (void*) tmp2;                                      \
    NS_ADDREF_THIS();                                           \
    return NS_OK;                                               \
  }                                                             \
  if (_id.Equals(kIJSScriptObjectIID)) {                        \
    nsIJSScriptObject* tmp = (nsIJSScriptObject*)&mInner;       \
    *_iptr = (void*) tmp;                                       \
    NS_ADDREF_THIS();                                           \
    return NS_OK;                                               \
  }                                                             

#endif /* nsGenericElement_h___ */
