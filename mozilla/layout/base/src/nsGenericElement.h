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
#ifndef nsGenericElement_h___
#define nsGenericElement_h___

#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsVoidArray.h"
#include "nsIScriptObjectOwner.h"
#include "nsIJSScriptObject.h"
#include "nsILinkHandler.h"
#include "nsGenericDOMNodeList.h"
#include "nsIEventListenerManager.h"

extern const nsIID kIDOMNodeIID;
extern const nsIID kIDOMElementIID;
extern const nsIID kIDOMEventReceiverIID;
extern const nsIID kIDOMEventTargetIID;
extern const nsIID kIScriptObjectOwnerIID;
extern const nsIID kIJSScriptObjectIID;
extern const nsIID kISupportsIID;
extern const nsIID kIContentIID;

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIFrame;
class nsISupportsArray;
class nsIDOMScriptObjectFactory;
class nsDOMCSSDeclaration;
class nsIDOMCSSStyleDeclaration;
class nsDOMAttributeMap;
class nsIURI;


// Class that holds the child list of a content element and also
// implements the nsIDOMNodeList interface.
class nsChildContentList : public nsGenericDOMNodeList 
{
public:
  nsChildContentList(nsIContent *aContent);
  virtual ~nsChildContentList();

  // nsIDOMNodeList interface
  NS_DECL_IDOMNODELIST
  
  void DropReference();

private:
  nsIContent *mContent;
};

class nsCheapVoidArray {
public:
  nsCheapVoidArray();
  ~nsCheapVoidArray();

  PRInt32 Count() const;
  void* ElementAt(PRInt32 aIndex) const;
  PRInt32 IndexOf(void* aPossibleElement) const;
  PRBool InsertElementAt(void* aElement, PRInt32 aIndex);
  PRBool ReplaceElementAt(void* aElement, PRInt32 aIndex);
  PRBool AppendElement(void* aElement);
  PRBool RemoveElement(void* aElement);
  PRBool RemoveElementAt(PRInt32 aIndex);
  void Compact();

private:
  typedef unsigned long PtrBits;

  PRBool HasSingleChild() const { return (mChildren && (PtrBits(mChildren) & 0x1)); }
  void* GetSingleChild() const { return (mChildren ? ((void*)(PtrBits(mChildren) & ~0x1)) : nsnull); }
  void SetSingleChild(void *aChild);
  nsVoidArray* GetChildVector() const { return (nsVoidArray*)mChildren; }
  nsVoidArray* SwitchToVector();

  // A tagged pointer that's either a pointer to a single child
  // or a pointer to a vector of multiple children. This is a space
  // optimization since a large number of containers have only a 
  // single child.
  void *mChildren;  
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
  nsDOMAttributeMap* mAttributeMap;
  nsVoidArray *mRangeList;
  nsIContent* mCapturer;
  nsIEventListenerManager* mListenerManager;
} nsDOMSlots;

class nsGenericElement {
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
  nsresult    GetAttribute(const nsString& aName, nsString& aReturn);
  nsresult    SetAttribute(const nsString& aName, const nsString& aValue);
  nsresult    RemoveAttribute(const nsString& aName);
  nsresult    GetAttributeNode(const nsString& aName,
                               nsIDOMAttr** aReturn);
  nsresult    SetAttributeNode(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn);
  nsresult    RemoveAttributeNode(nsIDOMAttr* aOldAttr, nsIDOMAttr** aReturn);
  nsresult    GetElementsByTagName(const nsString& aTagname,
                                   nsIDOMNodeList** aReturn);
  nsresult    Normalize();

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
  nsresult GetNameSpaceID(PRInt32& aNameSpaceID) const;
  nsresult GetTag(nsIAtom*& aResult) const;
  nsresult HandleDOMEvent(nsIPresContext* aPresContext,
                          nsEvent* aEvent,
                          nsIDOMEvent** aDOMEvent,
                          PRUint32 aFlags,
                          nsEventStatus* aEventStatus);

  nsresult GetContentID(PRUint32* aID) {
    *aID = mContentID;
    return NS_OK;
  }
  nsresult SetContentID(PRUint32 aID) {
    mContentID = aID;
    return NS_OK;
  }

  nsresult RangeAdd(nsIDOMRange& aRange);
  nsresult RangeRemove(nsIDOMRange& aRange);
  nsresult GetRangeList(nsVoidArray*& aResult) const;
  nsresult SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult,
                  size_t aInstanceSize) const;
  
  // Implementation for nsIJSScriptObject
  PRBool    AddProperty(JSContext *aContext, JSObject *aObj, 
                        jsval aID, jsval *aVp);
  PRBool    DeleteProperty(JSContext *aContext, JSObject *aObj, 
                        jsval aID, jsval *aVp);
  PRBool    GetProperty(JSContext *aContext, JSObject *aObj, 
                        jsval aID, jsval *aVp);
  PRBool    SetProperty(JSContext *aContext, JSObject *aObj, 
                        jsval aID, jsval *aVp);
  PRBool    EnumerateProperty(JSContext *aContext, JSObject *aObj);
  PRBool    Resolve(JSContext *aContext, JSObject *aObj, jsval aID);
  PRBool    Convert(JSContext *aContext, JSObject *aObj, jsval aID);
  void      Finalize(JSContext *aContext, JSObject *aObj);

  // Implementation for nsISupports
  nsresult QueryInterface(REFNSIID aIID,
                          void** aInstancePtr);
  nsrefcnt AddRef(void);
  nsrefcnt Release(void);

  //----------------------------------------

  nsresult GetListenerManager(nsIEventListenerManager** aInstancePtrResult);

  nsresult RenderFrame(nsIPresContext*);

  nsresult AddScriptEventListener(nsIAtom* aAttribute,
                                  const nsString& aValue,
                                  REFNSIID aIID);

  nsresult TriggerLink(nsIPresContext* aPresContext,
                       nsLinkVerb aVerb,
                       nsIURI* aBaseURL,
                       const nsString& aURLSpec,
                       const nsString& aTargetSpec,
                       PRBool aClick);

  nsresult JoinTextNodes(nsIContent* aFirst,
                         nsIContent* aSecond);

  static void SetDocumentInChildrenOf(nsIContent* aContent, 
				      nsIDocument* aDocument);

  static nsresult GetScriptObjectFactory(nsIDOMScriptObjectFactory **aFactory);

  static nsIDOMScriptObjectFactory *gScriptObjectFactory;

  static nsIAtom* CutNameSpacePrefix(nsString& aString);

  nsDOMSlots *GetDOMSlots();
  void MaybeClearDOMSlots();

  // Up pointer to the real content object that we are
  // supporting. Sometimes there is work that we just can't do
  // ourselves, so this is needed to ask the real object to do the
  // work.
  nsIContent* mContent;

  nsIDocument* mDocument;                   // WEAK
  nsIContent* mParent;                      // WEAK
  nsIAtom* mTag;
  nsDOMSlots *mDOMSlots;
  PRUint32 mContentID;
};

class nsGenericContainerElement : public nsGenericElement {
public:
  nsGenericContainerElement();
  ~nsGenericContainerElement();

  nsresult CopyInnerTo(nsIContent* aSrcContent,
                       nsGenericContainerElement* aDest,
                       PRBool aDeep);

  // Remainder of nsIDOMHTMLElement (and nsIDOMNode)
  nsresult    GetAttribute(const nsString& aName, nsString& aReturn) 
  {
    return nsGenericElement::GetAttribute(aName, aReturn);
  }
  nsresult    SetAttribute(const nsString& aName, const nsString& aValue)
  {
    return nsGenericElement::SetAttribute(aName, aValue);
  }
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
  nsresult SetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName, 
                        const nsString& aValue,
                        PRBool aNotify);
  nsresult GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName, 
                        nsString& aResult) const;
  nsresult UnsetAttribute(PRInt32 aNameSpaceID, nsIAtom* aAttribute, 
                          PRBool aNotify);
  nsresult GetAttributeNameAt(PRInt32 aIndex,
                              PRInt32& aNameSpaceID, 
                              nsIAtom*& aName) const;
  nsresult GetAttributeCount(PRInt32& aResult) const;
  nsresult List(FILE* out, PRInt32 aIndent) const;
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
  nsresult SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult,
                  size_t aInstanceSize) const;

  void ListAttributes(FILE* out) const;

  nsVoidArray* mAttributes;
  nsCheapVoidArray mChildren;
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
  NS_IMETHOD GetAttribute(const nsString& aName, nsString& aReturn) {         \
    return _g.GetAttribute(aName, aReturn);                                   \
  }                                                                           \
  NS_IMETHOD SetAttribute(const nsString& aName, const nsString& aValue) {    \
    return _g.SetAttribute(aName, aValue);                                    \
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
#define NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(_g)                             \
  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener,              \
                                   const nsIID& aIID) {                         \
    return _g.AddEventListenerByIID(aListener, aIID);                           \
  }                                                                             \
  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener,           \
                                      const nsIID& aIID) {                      \
    return _g.RemoveEventListenerByIID(aListener, aIID);                        \
  }                                                                             \
  NS_IMETHOD GetListenerManager(nsIEventListenerManager** aResult) {            \
    return _g.GetListenerManager(aResult);                                      \
  }                                                                             \
  NS_IMETHOD GetNewListenerManager(nsIEventListenerManager** aResult) {         \
    return _g.GetNewListenerManager(aResult);                                   \
  }                                                                             \
  NS_IMETHOD HandleEvent(nsIDOMEvent *aEvent) {                                 \
    return _g.HandleEvent(aEvent);                                              \
  }                                                                             \
  NS_IMETHOD AddEventListener(const nsString& aType,                            \
                              nsIDOMEventListener* aListener,                   \
                              PRBool aUseCapture) {                             \
    return _g.AddEventListener(aType, aListener, aUseCapture);    \
  }                                                                             \
  NS_IMETHOD RemoveEventListener(const nsString& aType,                         \
                                 nsIDOMEventListener* aListener,                \
                                 PRBool aUseCapture) {                          \
    return _g.RemoveEventListener(aType, aListener, aUseCapture); \
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
  NS_IMETHOD GetNameSpaceID(PRInt32& aResult) const {                      \
    return _g.GetNameSpaceID(aResult);                                     \
  }                                                                        \
  NS_IMETHOD GetTag(nsIAtom*& aResult) const {                             \
    return _g.GetTag(aResult);                                             \
  }                                                                        \
  NS_IMETHOD ParseAttributeString(const nsString& aStr,                    \
                                  nsIAtom*& aName,                         \
                                  PRInt32& aNameSpaceID) {                 \
    return _g.ParseAttributeString(aStr, aName, aNameSpaceID);             \
  }                                                                        \
  NS_IMETHOD GetNameSpacePrefixFromId(PRInt32 aNameSpaceID,                \
                                nsIAtom*& aPrefix) {                       \
    return _g.GetNameSpacePrefixFromId(aNameSpaceID, aPrefix);             \
  }                                                                        \
  NS_IMETHOD SetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,            \
                          const nsString& aValue, PRBool aNotify) {        \
    return _g.SetAttribute(aNameSpaceID, aName, aValue, aNotify);          \
  }                                                                        \
  NS_IMETHOD GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,            \
                          nsString& aResult) const {                       \
    return _g.GetAttribute(aNameSpaceID, aName, aResult);                  \
  }                                                                        \
  NS_IMETHOD UnsetAttribute(PRInt32 aNameSpaceID, nsIAtom* aAttribute,     \
                            PRBool aNotify) {                              \
    return _g.UnsetAttribute(aNameSpaceID, aAttribute, aNotify);           \
  }                                                                        \
  NS_IMETHOD GetAttributeNameAt(PRInt32 aIndex,                            \
                                PRInt32& aNameSpaceID,                     \
                                nsIAtom*& aName) const {                   \
    return _g.GetAttributeNameAt(aIndex, aNameSpaceID, aName);             \
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
  NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext,                  \
                            nsEvent* aEvent,                               \
                            nsIDOMEvent** aDOMEvent,                       \
                            PRUint32 aFlags,                               \
                            nsEventStatus* aEventStatus);                  \
  NS_IMETHOD GetContentID(PRUint32* aID) {                                 \
    return _g.GetContentID(aID);                                           \
  }                                                                        \
  NS_IMETHOD SetContentID(PRUint32 aID) {                                  \
    return _g.SetContentID(aID);                                           \
  }                                                                        \
  NS_IMETHOD RangeAdd(nsIDOMRange& aRange) {                               \
    return _g.RangeAdd(aRange);                                            \
  }                                                                        \
  NS_IMETHOD RangeRemove(nsIDOMRange& aRange) {                            \
    return _g.RangeRemove(aRange);                                         \
  }                                                                        \
  NS_IMETHOD GetRangeList(nsVoidArray*& aResult) const {                   \
    return _g.GetRangeList(aResult);                                       \
  }                                                                        \
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;

#define NS_IMPL_ICONTENT_NO_SETPARENT_USING_GENERIC(_g)                    \
  NS_IMETHOD GetDocument(nsIDocument*& aResult) const {                    \
    return _g.GetDocument(aResult);                                        \
  }                                                                        \
  NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep) {           \
    return _g.SetDocument(aDocument, aDeep);                               \
  }                                                                        \
  NS_IMETHOD GetParent(nsIContent*& aResult) const {                       \
    return _g.GetParent(aResult);                                          \
  }                                                                        \
  NS_IMETHOD SetParent(nsIContent* aParent);                               \
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
  NS_IMETHOD GetNameSpaceID(PRInt32& aResult) const {                      \
    return _g.GetNameSpaceID(aResult);                                     \
  }                                                                        \
  NS_IMETHOD GetTag(nsIAtom*& aResult) const {                             \
    return _g.GetTag(aResult);                                             \
  }                                                                        \
  NS_IMETHOD ParseAttributeString(const nsString& aStr,                    \
                                  nsIAtom*& aName,                         \
                                  PRInt32& aNameSpaceID) {                 \
    return _g.ParseAttributeString(aStr, aName, aNameSpaceID);             \
  }                                                                        \
  NS_IMETHOD GetNameSpacePrefixFromId(PRInt32 aNameSpaceID,                \
                                nsIAtom*& aPrefix) {                       \
    return _g.GetNameSpacePrefixFromId(aNameSpaceID, aPrefix);             \
  }                                                                        \
  NS_IMETHOD SetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,            \
                          const nsString& aValue, PRBool aNotify) {        \
    return _g.SetAttribute(aNameSpaceID, aName, aValue, aNotify);          \
  }                                                                        \
  NS_IMETHOD GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,            \
                          nsString& aResult) const {                       \
    return _g.GetAttribute(aNameSpaceID, aName, aResult);                  \
  }                                                                        \
  NS_IMETHOD UnsetAttribute(PRInt32 aNameSpaceID, nsIAtom* aAttribute,     \
                            PRBool aNotify) {                              \
    return _g.UnsetAttribute(aNameSpaceID, aAttribute, aNotify);           \
  }                                                                        \
  NS_IMETHOD GetAttributeNameAt(PRInt32 aIndex,                            \
                                PRInt32& aNameSpaceID,                     \
                                nsIAtom*& aName) const {                   \
    return _g.GetAttributeNameAt(aIndex, aNameSpaceID, aName);             \
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
  NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext,                  \
                            nsEvent* aEvent,                               \
                            nsIDOMEvent** aDOMEvent,                       \
                            PRUint32 aFlags,                               \
                            nsEventStatus* aEventStatus);                  \
  NS_IMETHOD GetContentID(PRUint32* aID) {                                 \
    return _g.GetContentID(aID);                                           \
  }                                                                        \
  NS_IMETHOD SetContentID(PRUint32 aID) {                                  \
    return _g.SetContentID(aID);                                           \
  }                                                                        \
  NS_IMETHOD RangeAdd(nsIDOMRange& aRange) {                               \
    return _g.RangeAdd(aRange);                                            \
  }                                                                        \
  NS_IMETHOD RangeRemove(nsIDOMRange& aRange) {                            \
    return _g.RangeRemove(aRange);                                         \
  }                                                                        \
  NS_IMETHOD GetRangeList(nsVoidArray*& aResult) const {                   \
    return _g.GetRangeList(aResult);                                       \
  }                                                                        \
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;

#define NS_IMPL_ICONTENT_NO_SETDOCUMENT_USING_GENERIC(_g)                  \
  NS_IMETHOD GetDocument(nsIDocument*& aResult) const {                    \
    return _g.GetDocument(aResult);                                        \
  }                                                                        \
  NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep);            \
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
  NS_IMETHOD GetNameSpaceID(PRInt32& aResult) const {                      \
    return _g.GetNameSpaceID(aResult);                                     \
  }                                                                        \
  NS_IMETHOD GetTag(nsIAtom*& aResult) const {                             \
    return _g.GetTag(aResult);                                             \
  }                                                                        \
  NS_IMETHOD ParseAttributeString(const nsString& aStr,                    \
                                  nsIAtom*& aName,                         \
                                  PRInt32& aNameSpaceID) {                 \
    return _g.ParseAttributeString(aStr, aName, aNameSpaceID);             \
  }                                                                        \
  NS_IMETHOD GetNameSpacePrefixFromId(PRInt32 aNameSpaceID,                \
                                nsIAtom*& aPrefix) {                       \
    return _g.GetNameSpacePrefixFromId(aNameSpaceID, aPrefix);             \
  }                                                                        \
  NS_IMETHOD SetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,            \
                          const nsString& aValue, PRBool aNotify) {        \
    return _g.SetAttribute(aNameSpaceID, aName, aValue, aNotify);          \
  }                                                                        \
  NS_IMETHOD GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,            \
                          nsString& aResult) const {                       \
    return _g.GetAttribute(aNameSpaceID, aName, aResult);                  \
  }                                                                        \
  NS_IMETHOD UnsetAttribute(PRInt32 aNameSpaceID, nsIAtom* aAttribute,     \
                            PRBool aNotify) {                              \
    return _g.UnsetAttribute(aNameSpaceID, aAttribute, aNotify);           \
  }                                                                        \
  NS_IMETHOD GetAttributeNameAt(PRInt32 aIndex,                            \
                                PRInt32& aNameSpaceID,                     \
                                nsIAtom*& aName) const {                   \
    return _g.GetAttributeNameAt(aIndex, aNameSpaceID, aName);             \
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
  NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext,                  \
                            nsEvent* aEvent,                               \
                            nsIDOMEvent** aDOMEvent,                       \
                            PRUint32 aFlags,                               \
                            nsEventStatus* aEventStatus);                  \
  NS_IMETHOD GetContentID(PRUint32* aID) {                                 \
    return _g.GetContentID(aID);                                           \
  }                                                                        \
  NS_IMETHOD SetContentID(PRUint32 aID) {                                  \
    return _g.SetContentID(aID);                                           \
  }                                                                        \
  NS_IMETHOD RangeAdd(nsIDOMRange& aRange) {                               \
    return _g.RangeAdd(aRange);                                            \
  }                                                                        \
  NS_IMETHOD RangeRemove(nsIDOMRange& aRange) {                            \
    return _g.RangeRemove(aRange);                                         \
  }                                                                        \
  NS_IMETHOD GetRangeList(nsVoidArray*& aResult) const {                   \
    return _g.GetRangeList(aResult);                                       \
  }                                                                        \
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;

#define NS_IMPL_ICONTENT_NO_SETPARENT_NO_SETDOCUMENT_USING_GENERIC(_g)     \
  NS_IMETHOD GetDocument(nsIDocument*& aResult) const {                    \
    return _g.GetDocument(aResult);                                        \
  }                                                                        \
  NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep);            \
  NS_IMETHOD GetParent(nsIContent*& aResult) const {                       \
    return _g.GetParent(aResult);                                          \
  }                                                                        \
  NS_IMETHOD SetParent(nsIContent* aParent);                               \
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
  NS_IMETHOD GetNameSpaceID(PRInt32& aResult) const {                      \
    return _g.GetNameSpaceID(aResult);                                     \
  }                                                                        \
  NS_IMETHOD GetTag(nsIAtom*& aResult) const {                             \
    return _g.GetTag(aResult);                                             \
  }                                                                        \
  NS_IMETHOD ParseAttributeString(const nsString& aStr,                    \
                                  nsIAtom*& aName,                         \
                                  PRInt32& aNameSpaceID) {                 \
    return _g.ParseAttributeString(aStr, aName, aNameSpaceID);             \
  }                                                                        \
  NS_IMETHOD GetNameSpacePrefixFromId(PRInt32 aNameSpaceID,                \
                                nsIAtom*& aPrefix) {                       \
    return _g.GetNameSpacePrefixFromId(aNameSpaceID, aPrefix);             \
  }                                                                        \
  NS_IMETHOD SetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,            \
                          const nsString& aValue, PRBool aNotify) {        \
    return _g.SetAttribute(aNameSpaceID, aName, aValue, aNotify);          \
  }                                                                        \
  NS_IMETHOD GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,            \
                          nsString& aResult) const {                       \
    return _g.GetAttribute(aNameSpaceID, aName, aResult);                  \
  }                                                                        \
  NS_IMETHOD UnsetAttribute(PRInt32 aNameSpaceID, nsIAtom* aAttribute,     \
                            PRBool aNotify) {                              \
    return _g.UnsetAttribute(aNameSpaceID, aAttribute, aNotify);           \
  }                                                                        \
  NS_IMETHOD GetAttributeNameAt(PRInt32 aIndex,                            \
                                PRInt32& aNameSpaceID,                     \
                                nsIAtom*& aName) const {                   \
    return _g.GetAttributeNameAt(aIndex, aNameSpaceID, aName);             \
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
  NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext,                  \
                            nsEvent* aEvent,                               \
                            nsIDOMEvent** aDOMEvent,                       \
                            PRUint32 aFlags,                               \
                            nsEventStatus* aEventStatus);                  \
  NS_IMETHOD GetContentID(PRUint32* aID) {                                 \
    return _g.GetContentID(aID);                                           \
  }                                                                        \
  NS_IMETHOD SetContentID(PRUint32 aID) {                                  \
    return _g.SetContentID(aID);                                           \
  }                                                                        \
  NS_IMETHOD RangeAdd(nsIDOMRange& aRange) {                               \
    return _g.RangeAdd(aRange);                                            \
  }                                                                        \
  NS_IMETHOD RangeRemove(nsIDOMRange& aRange) {                            \
    return _g.RangeRemove(aRange);                                         \
  }                                                                        \
  NS_IMETHOD GetRangeList(nsVoidArray*& aResult) const {                   \
    return _g.GetRangeList(aResult);                                       \
  }                                                                        \
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
  
/**
 * Implement the nsIScriptObjectOwner API by forwarding the methods to a
 * generic content object
 */
#define NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(_g)     \
  NS_IMETHOD GetScriptObject(nsIScriptContext* aContext, \
                             void** aScriptObject) {     \
    return _g.GetScriptObject(aContext, aScriptObject);  \
  }                                                      \
  NS_IMETHOD SetScriptObject(void *aScriptObject) {      \
    return _g.SetScriptObject(aScriptObject);            \
  }

#define NS_IMPL_IJSSCRIPTOBJECT_USING_GENERIC(_g)                       \
  NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(_g)                          \
  virtual PRBool    AddProperty(JSContext *aContext, JSObject *aObj,    \
                        jsval aID, jsval *aVp) {                        \
    return _g.AddProperty(aContext, aObj, aID, aVp);                    \
  }                                                                     \
  virtual PRBool    DeleteProperty(JSContext *aContext, JSObject *aObj, \
                        jsval aID, jsval *aVp) {                        \
    return _g.DeleteProperty(aContext, aObj, aID, aVp);                 \
  }                                                                     \
  virtual PRBool    GetProperty(JSContext *aContext, JSObject *aObj,    \
                        jsval aID, jsval *aVp) {                        \
    return _g.GetProperty(aContext, aObj, aID, aVp);                    \
  }                                                                     \
  virtual PRBool    SetProperty(JSContext *aContext, JSObject *aObj,    \
                        jsval aID, jsval *aVp) {                        \
    return _g.SetProperty(aContext, aObj, aID, aVp);                    \
  }                                                                     \
  virtual PRBool    EnumerateProperty(JSContext *aContext, JSObject *aObj) { \
    return _g.EnumerateProperty(aContext, aObj);                             \
  }                                                                          \
  virtual PRBool    Resolve(JSContext *aContext, JSObject *aObj, jsval aID) { \
    return _g.EnumerateProperty(aContext, aObj);                              \
  }                                                                           \
  virtual PRBool    Convert(JSContext *aContext, JSObject *aObj, jsval aID) { \
    return _g.EnumerateProperty(aContext, aObj);                              \
  }                                                                           \
  virtual void      Finalize(JSContext *aContext, JSObject *aObj) {     \
    _g.Finalize(aContext, aObj);                                        \
  }

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
    nsCOMPtr<nsIEventListenerManager> man;                      \
    if (NS_SUCCEEDED(mInner.GetListenerManager(getter_AddRefs(man)))){ \
      return man->QueryInterface(kIDOMEventReceiverIID, (void**)_iptr); \
    }                                                           \
    return NS_NOINTERFACE;                                      \
  }                                                             \
  if (_id.Equals(kIDOMEventTargetIID)) {                        \
    nsCOMPtr<nsIEventListenerManager> man;                      \
    if (NS_SUCCEEDED(mInner.GetListenerManager(getter_AddRefs(man)))){ \
      return man->QueryInterface(kIDOMEventTargetIID, (void**)_iptr); \
    }                                                           \
    return NS_NOINTERFACE;                                      \
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
    nsIJSScriptObject* tmp = _this;                             \
    *_iptr = (void*) tmp;                                       \
    NS_ADDREF_THIS();                                           \
    return NS_OK;                                               \
  }                                                             

#endif /* nsGenericElement_h___ */
