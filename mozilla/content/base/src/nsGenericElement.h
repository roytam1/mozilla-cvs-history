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
#include "nsIHTMLContent.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMLinkStyle.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsVoidArray.h"
#include "nsILinkHandler.h"
#include "nsGenericDOMNodeList.h"
#include "nsIEventListenerManager.h"
#include "nsINodeInfo.h"
#include "nsIParser.h"
#include "nsContentUtils.h"

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIFrame;
class nsISupportsArray;
class nsDOMCSSDeclaration;
class nsIDOMCSSStyleDeclaration;
class nsDOMAttributeMap;
class nsIURI;
class nsINodeInfo;

// Class that holds the child list of a content element and also
// implements the nsIDOMNodeList interface.
class nsChildContentList : public nsGenericDOMNodeList 
{
public:
  nsChildContentList(nsIContent *aContent);
  virtual ~nsChildContentList();

  // nsIDOMNodeList interface
  NS_DECL_NSIDOMNODELIST
  
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

  PRBool HasSingleChild() const
  {
    return (mChildren && (PtrBits(mChildren) & 0x1));
  }
  void* GetSingleChild() const
  {
    return (mChildren ? ((void*)(PtrBits(mChildren) & ~0x1)) : nsnull);
  }
  void SetSingleChild(void *aChild);
  nsVoidArray* GetChildVector() const
  {
    return (nsVoidArray*)mChildren;
  }
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
  nsChildContentList *mChildNodes;
  nsDOMCSSDeclaration *mStyle;
  nsDOMAttributeMap* mAttributeMap;
  nsVoidArray *mRangeList;
  nsIEventListenerManager* mListenerManager;
  nsIContent* mBindingParent; // The nearest enclosing content node with a binding
                              // that created us. [Weak]
} nsDOMSlots;

class nsGenericElement : public nsIHTMLContent
{
public:
  nsGenericElement();
  virtual ~nsGenericElement();

  NS_DECL_ISUPPORTS

  nsresult Init(nsINodeInfo *aNodeInfo);

  // Free globals, to be called from module destructor
  static void Shutdown();

  // nsIContent interface methods
  NS_IMETHOD GetDocument(nsIDocument*& aResult) const;
  NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep,
                         PRBool aCompileEventHandlers);
  NS_IMETHOD GetParent(nsIContent*& aResult) const;
  NS_IMETHOD SetParent(nsIContent* aParent);
  NS_IMETHOD GetNameSpaceID(PRInt32& aNameSpaceID) const;
  NS_IMETHOD GetTag(nsIAtom*& aResult) const;
  NS_IMETHOD GetNodeInfo(nsINodeInfo*& aResult) const;
  // NS_IMETHOD CanContainChildren(PRBool& aResult) const;
  // NS_IMETHOD ChildCount(PRInt32& aResult) const;
  // NS_IMETHOD ChildAt(PRInt32 aIndex, nsIContent*& aResult) const;
  // NS_IMETHOD IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const;
  // NS_IMETHOD InsertChildAt(nsIContent* aKid, PRInt32 aIndex,
  //                          PRBool aNotify);
  // NS_IMETHOD ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex,
  //                           PRBool aNotify);
  // NS_IMETHOD AppendChildTo(nsIContent* aKid, PRBool aNotify);
  // NS_IMETHOD RemoveChildAt(PRInt32 aIndex, PRBool aNotify);
  // NS_IMETHOD NormalizeAttributeString(const nsAReadableString& aStr,
  //                                     nsINodeInfo*& aNodeInfo);
  // NS_IMETHOD SetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName, 
  //                         const nsAReadableString& aValue,
  //                         PRBool aNotify);
  // NS_IMETHOD SetAttribute(nsINodeInfo* aNodeInfo,
  //                         const nsAReadableString& aValue,
  //                         PRBool aNotify);
  // NS_IMETHOD GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName, 
  //                         nsAWritableString& aResult) const;
  // NS_IMETHOD GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName, 
  //                         nsIAtom*& aPrefix,
  //                         nsAWritableString& aResult) const;
  // NS_IMETHOD UnsetAttribute(PRInt32 aNameSpaceID, nsIAtom* aAttribute, 
  //                           PRBool aNotify);
  // NS_IMETHOD GetAttributeNameAt(PRInt32 aIndex,
  //                               PRInt32& aNameSpaceID, 
  //                               nsIAtom*& aName,
  //                               nsIAtom*& aPrefix) const;
  // NS_IMETHOD GetAttributeCount(PRInt32& aResult) const;
  // NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  // NS_IMETHOD DumpContent(FILE* out, PRInt32 aIndent,PRBool aDumpAll) const;
  NS_IMETHOD RangeAdd(nsIDOMRange& aRange);
  NS_IMETHOD RangeRemove(nsIDOMRange& aRange);
  NS_IMETHOD GetRangeList(nsVoidArray*& aResult) const;
  NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext,
                            nsEvent* aEvent,
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus* aEventStatus);
  NS_IMETHOD GetContentID(PRUint32* aID);
  NS_IMETHOD SetContentID(PRUint32 aID);
  NS_IMETHOD SetFocus(nsIPresContext* aContext);
  NS_IMETHOD RemoveFocus(nsIPresContext* aContext);
  NS_IMETHOD GetBindingParent(nsIContent** aContent);
  NS_IMETHOD SetBindingParent(nsIContent* aParent);

  // nsIStyledContent interface methods
  NS_IMETHOD GetID(nsIAtom*& aResult) const;
  NS_IMETHOD GetClasses(nsVoidArray& aArray) const;
  NS_IMETHOD HasClass(nsIAtom* aClass) const;
  NS_IMETHOD GetContentStyleRules(nsISupportsArray* aRules);
  NS_IMETHOD GetInlineStyleRules(nsISupportsArray* aRules);
  NS_IMETHOD GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                      PRInt32& aHint) const;

  // nsIXMLContent interface methods
  NS_IMETHOD SetContainingNameSpace(nsINameSpace* aNameSpace);
  NS_IMETHOD GetContainingNameSpace(nsINameSpace*& aNameSpace) const;
  NS_IMETHOD MaybeTriggerAutoLink(nsIWebShell *aShell);
  NS_IMETHOD GetXMLBaseURI(nsIURI **aURI);

  // nsIHTMLContent interface methods
  NS_IMETHOD Compact();
  NS_IMETHOD SetHTMLAttribute(nsIAtom* aAttribute,
                              const nsHTMLValue& aValue,
                              PRBool aNotify);
  NS_IMETHOD GetHTMLAttribute(nsIAtom* aAttribute,
                              nsHTMLValue& aValue) const;
  NS_IMETHOD GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                          nsMapAttributesFunc& aMapFunc) const;
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAWritableString& aResult) const;
  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD GetBaseURL(nsIURI*& aBaseURL) const;
  NS_IMETHOD GetBaseTarget(nsAWritableString& aBaseTarget) const;


  // nsIDOMNode method implementation
  NS_IMETHOD GetNodeName(nsAWritableString& aNodeName);
  NS_IMETHOD GetLocalName(nsAWritableString& aLocalName);
  NS_IMETHOD GetNodeValue(nsAWritableString& aNodeValue);
  NS_IMETHOD SetNodeValue(const nsAReadableString& aNodeValue);
  NS_IMETHOD GetNodeType(PRUint16* aNodeType);
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode);
  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes);
  NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling);
  NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling);
  NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument);
  NS_IMETHOD GetNamespaceURI(nsAWritableString& aNamespaceURI);
  NS_IMETHOD GetPrefix(nsAWritableString& aPrefix);
  NS_IMETHOD SetPrefix(const nsAReadableString& aPrefix);
  NS_IMETHOD Normalize();
  NS_IMETHOD IsSupported(const nsAReadableString& aFeature,
                         const nsAReadableString& aVersion, PRBool* aReturn);
  NS_IMETHOD HasAttributes(PRBool* aHasAttributes);
  NS_IMETHOD GetBaseURI(nsAWritableString& aURI);

  // nsIDOMElement method implementation
  NS_IMETHOD GetTagName(nsAWritableString& aTagName);
  NS_IMETHOD GetAttribute(const nsAReadableString& aName,
                          nsAWritableString& aReturn);
  NS_IMETHOD SetAttribute(const nsAReadableString& aName,
                          const nsAReadableString& aValue);
  NS_IMETHOD RemoveAttribute(const nsAReadableString& aName);
  NS_IMETHOD GetAttributeNode(const nsAReadableString& aName,
                              nsIDOMAttr** aReturn);
  NS_IMETHOD SetAttributeNode(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn);
  NS_IMETHOD RemoveAttributeNode(nsIDOMAttr* aOldAttr, nsIDOMAttr** aReturn);
  NS_IMETHOD GetElementsByTagName(const nsAReadableString& aTagname,
                                  nsIDOMNodeList** aReturn);
  NS_IMETHOD GetAttributeNS(const nsAReadableString& aNamespaceURI,
                            const nsAReadableString& aLocalName,
                            nsAWritableString& aReturn);
  NS_IMETHOD SetAttributeNS(const nsAReadableString& aNamespaceURI,
                            const nsAReadableString& aQualifiedName,
                            const nsAReadableString& aValue);
  NS_IMETHOD RemoveAttributeNS(const nsAReadableString& aNamespaceURI,
                               const nsAReadableString& aLocalName);
  NS_IMETHOD GetAttributeNodeNS(const nsAReadableString& aNamespaceURI,
                                const nsAReadableString& aLocalName,
                                nsIDOMAttr** aReturn);
  NS_IMETHOD SetAttributeNodeNS(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn);
  NS_IMETHOD GetElementsByTagNameNS(const nsAReadableString& aNamespaceURI,
                                    const nsAReadableString& aLocalName,
                                    nsIDOMNodeList** aReturn);
  NS_IMETHOD HasAttribute(const nsAReadableString& aName, PRBool* aReturn);
  NS_IMETHOD HasAttributeNS(const nsAReadableString& aNamespaceURI,
                            const nsAReadableString& aLocalName,
                            PRBool* aReturn);

  // Generic DOMNode implementations
  nsresult  doInsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                           nsIDOMNode** aReturn);
  nsresult  doReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                           nsIDOMNode** aReturn);
  nsresult  doRemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);

  //----------------------------------------

  nsresult GetListenerManager(nsIEventListenerManager** aInstancePtrResult);

  nsresult RenderFrame(nsIPresContext*);

  nsresult AddScriptEventListener(nsIAtom* aAttribute,
                                  const nsAReadableString& aValue);

  nsresult TriggerLink(nsIPresContext* aPresContext,
                       nsLinkVerb aVerb,
                       nsIURI* aBaseURL,
                       const nsString& aURLSpec,
                       const nsString& aTargetSpec,
                       PRBool aClick);

  nsresult JoinTextNodes(nsIContent* aFirst,
                         nsIContent* aSecond);

  static void SetDocumentInChildrenOf(nsIContent* aContent, 
				      nsIDocument* aDocument, PRBool aCompileEventHandlers);

  static nsresult InternalIsSupported(const nsAReadableString& aFeature,
                                      const nsAReadableString& aVersion,
                                      PRBool* aReturn);

  static PRBool HasMutationListeners(nsIContent* aContent,
                                     PRUint32 aType);

protected:
  virtual PRUint32 BaseSizeOf(nsISizeOfHandler *aSizer) const;
  //  virtual PRBool InternalRegisterCompileEventHandler(JSContext* aContext, jsval aPropName,
  //                                                     jsval *aVp, PRBool aCompile);

  nsDOMSlots *GetDOMSlots();
  void MaybeClearDOMSlots();

  nsIDocument* mDocument;                   // WEAK
  nsIContent* mParent;                      // WEAK
  
  nsINodeInfo* mNodeInfo;                   // OWNER
  nsDOMSlots *mDOMSlots;                    // OWNER
  PRUint32 mContentID;
};

class nsGenericContainerElement : public nsGenericElement {
public:
  nsGenericContainerElement();
  virtual ~nsGenericContainerElement();

  NS_IMETHOD CopyInnerTo(nsIContent* aSrcContent,
                       nsGenericContainerElement* aDest,
                       PRBool aDeep);

  // nsIDOMElement methods
  NS_METHOD GetAttribute(const nsAReadableString& aName,
                        nsAWritableString& aReturn) 
  {
    return nsGenericElement::GetAttribute(aName, aReturn);
  }
  NS_METHOD SetAttribute(const nsAReadableString& aName,
                        const nsAReadableString& aValue)
  {
    return nsGenericElement::SetAttribute(aName, aValue);
  }

  // Remainder of nsIDOMHTMLElement (and nsIDOMNode)
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes);
  NS_IMETHOD HasChildNodes(PRBool* aHasChildNodes);
  NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild);
  NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild);
  
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                        nsIDOMNode** aReturn)
  {
    return nsGenericElement::doInsertBefore(aNewChild, aRefChild, aReturn);
  }
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                        nsIDOMNode** aReturn)
  {
    return nsGenericElement::doReplaceChild(aNewChild, aOldChild, aReturn);
  }
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
  {
    return nsGenericElement::doRemoveChild(aOldChild, aReturn);
  }
  NS_IMETHOD AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
  {
    return nsGenericElement::doInsertBefore(aNewChild, nsnull, aReturn);
  }

  // Remainder of nsIContent
  NS_IMETHOD NormalizeAttributeString(const nsAReadableString& aStr,
                                      nsINodeInfo*& aNodeInfo);
  NS_IMETHOD SetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,
                          const nsAReadableString& aValue,
                          PRBool aNotify);
  NS_IMETHOD SetAttribute(nsINodeInfo* aNodeInfo,
                          const nsAReadableString& aValue,
                          PRBool aNotify);
  NS_IMETHOD GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,
                          nsAWritableString& aResult) const;
  NS_IMETHOD GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,
                          nsIAtom*& aPrefix, nsAWritableString& aResult) const;
  NS_IMETHOD UnsetAttribute(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                            PRBool aNotify);
  NS_IMETHOD GetAttributeNameAt(PRInt32 aIndex,
                                PRInt32& aNameSpaceID,
                                nsIAtom*& aName,
                                nsIAtom*& aPrefix) const;
  NS_IMETHOD GetAttributeCount(PRInt32& aResult) const;
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD DumpContent(FILE* out, PRInt32 aIndent,PRBool aDumpAll) const;
  NS_IMETHOD CanContainChildren(PRBool& aResult) const;
  NS_IMETHOD ChildCount(PRInt32& aResult) const;
  NS_IMETHOD ChildAt(PRInt32 aIndex, nsIContent*& aResult) const;
  NS_IMETHOD IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const;
  NS_IMETHOD InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify);
  NS_IMETHOD ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify);
  NS_IMETHOD AppendChildTo(nsIContent* aKid, PRBool aNotify);
  NS_IMETHOD RemoveChildAt(PRInt32 aIndex, PRBool aNotify);

  void ListAttributes(FILE* out) const;

protected:
  virtual PRUint32 BaseSizeOf(nsISizeOfHandler *aSizer) const;

  nsVoidArray* mAttributes;
  nsCheapVoidArray mChildren;
};


#define NS_FORWARD_NSIDOMNODE_NO_CLONENODE(_to)  \
  NS_IMETHOD    GetNodeName(nsAWritableString& aNodeName) { return _to GetNodeName(aNodeName); } \
  NS_IMETHOD    GetNodeValue(nsAWritableString& aNodeValue) { return _to GetNodeValue(aNodeValue); } \
  NS_IMETHOD    SetNodeValue(const nsAReadableString& aNodeValue) { return _to SetNodeValue(aNodeValue); } \
  NS_IMETHOD    GetNodeType(PRUint16* aNodeType) { return _to GetNodeType(aNodeType); } \
  NS_IMETHOD    GetParentNode(nsIDOMNode** aParentNode) { return _to GetParentNode(aParentNode); } \
  NS_IMETHOD    GetChildNodes(nsIDOMNodeList** aChildNodes) { return _to GetChildNodes(aChildNodes); } \
  NS_IMETHOD    GetFirstChild(nsIDOMNode** aFirstChild) { return _to GetFirstChild(aFirstChild); } \
  NS_IMETHOD    GetLastChild(nsIDOMNode** aLastChild) { return _to GetLastChild(aLastChild); } \
  NS_IMETHOD    GetPreviousSibling(nsIDOMNode** aPreviousSibling) { return _to GetPreviousSibling(aPreviousSibling); } \
  NS_IMETHOD    GetNextSibling(nsIDOMNode** aNextSibling) { return _to GetNextSibling(aNextSibling); } \
  NS_IMETHOD    GetAttributes(nsIDOMNamedNodeMap** aAttributes) { return _to GetAttributes(aAttributes); } \
  NS_IMETHOD    GetOwnerDocument(nsIDOMDocument** aOwnerDocument) { return _to GetOwnerDocument(aOwnerDocument); } \
  NS_IMETHOD    GetNamespaceURI(nsAWritableString& aNamespaceURI) { return _to GetNamespaceURI(aNamespaceURI); } \
  NS_IMETHOD    GetPrefix(nsAWritableString& aPrefix) { return _to GetPrefix(aPrefix); } \
  NS_IMETHOD    SetPrefix(const nsAReadableString& aPrefix) { return _to SetPrefix(aPrefix); } \
  NS_IMETHOD    GetLocalName(nsAWritableString& aLocalName) { return _to GetLocalName(aLocalName); } \
  NS_IMETHOD    InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn) { return _to InsertBefore(aNewChild, aRefChild, aReturn); }  \
  NS_IMETHOD    ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn) { return _to ReplaceChild(aNewChild, aOldChild, aReturn); }  \
  NS_IMETHOD    RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) { return _to RemoveChild(aOldChild, aReturn); }  \
  NS_IMETHOD    AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn) { return _to AppendChild(aNewChild, aReturn); }  \
  NS_IMETHOD    HasChildNodes(PRBool* aReturn) { return _to HasChildNodes(aReturn); }  \
  NS_IMETHOD    CloneNode(PRBool aDeep, nsIDOMNode** aReturn);  \
  NS_IMETHOD    Normalize() { return _to Normalize(); }  \
  NS_IMETHOD    IsSupported(const nsAReadableString& aFeature, const nsAReadableString& aVersion, PRBool* aReturn) { return _to IsSupported(aFeature, aVersion, aReturn); }  \
  NS_IMETHOD    HasAttributes(PRBool* aReturn) { return _to HasAttributes(aReturn); }  \
  NS_IMETHOD    GetBaseURI(nsAWritableString& aURI) { return _to GetBaseURI(aURI); }  \

#endif /* nsGenericElement_h___ */
