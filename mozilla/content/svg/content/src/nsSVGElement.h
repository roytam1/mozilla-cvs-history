/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is Crocodile Clips Ltd.
 * Portions created by Crocodile Clips are 
 * Copyright (C) 2001 Crocodile Clips Ltd. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 */

#ifndef __NS_SVGELEMENT_H__
#define __NS_SVGELEMENT_H__

/*
  nsSVGElement is the base class for all SVG content elements.
  It implements all the common DOM interfaces and handles attributes.
*/

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIDOMSVGElement.h"
#include "nsGenericElement.h"
#include "nsSVGAttributes.h"
#include "nsISVGValue.h"
#include "nsISVGValueObserver.h"
#include "nsINameSpace.h"
#include "nsWeakReference.h"
#include "nsISVGStyleValue.h"

extern void GetSVGElementIIDs(nsVoidArray& aArray);

class nsSVGElement : public nsGenericElement,    // :nsIHTMLContent:nsIStyledContent:nsIContent
                     public nsIDOMSVGElement,    // :nsIDOMElement:nsIDOMNode
                     public nsISVGValueObserver, 
                     public nsSupportsWeakReference // :nsISupportsWeakReference
{
protected:
  nsSVGElement();
  virtual ~nsSVGElement();

  virtual nsresult Init();

public:
  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIContent interface methods

  // NS_IMETHOD GetDocument(nsIDocument*& aResult) const;
  // NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep,
  //                       PRBool aCompileEventHandlers);
  // NS_IMETHOD GetParent(nsIContent*& aResult) const;
  // NS_IMETHOD SetParent(nsIContent* aParent);
  // NS_IMETHOD GetNameSpaceID(PRInt32& aNameSpaceID) const;
  // NS_IMETHOD GetTag(nsIAtom*& aResult) const;
  // NS_IMETHOD GetNodeInfo(nsINodeInfo*& aResult) const;
  
  NS_IMETHOD CanContainChildren(PRBool& aResult) const;
  NS_IMETHOD ChildCount(PRInt32& aResult) const;
  NS_IMETHOD ChildAt(PRInt32 aIndex, nsIContent*& aResult) const;
  NS_IMETHOD IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const;
  NS_IMETHOD InsertChildAt(nsIContent* aKid, PRInt32 aIndex,
                           PRBool aNotify,
                           PRBool aDeepSetDocument);
  NS_IMETHOD ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex,
                            PRBool aNotify,
                            PRBool aDeepSetDocument);
  NS_IMETHOD AppendChildTo(nsIContent* aKid, PRBool aNotify,
                           PRBool aDeepSetDocument);
  NS_IMETHOD RemoveChildAt(PRInt32 aIndex, PRBool aNotify);
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
                          nsIAtom*& aPrefix,
                          nsAWritableString& aResult) const;
  NS_IMETHOD UnsetAttribute(PRInt32 aNameSpaceID, nsIAtom* aAttribute, 
                            PRBool aNotify);
  NS_IMETHOD GetAttributeNameAt(PRInt32 aIndex,
                                PRInt32& aNameSpaceID, 
                                nsIAtom*& aName,
                                nsIAtom*& aPrefix) const;
  NS_IMETHOD GetAttributeCount(PRInt32& aResult) const;
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD DumpContent(FILE* out, PRInt32 aIndent,PRBool aDumpAll) const;
  
  // NS_IMETHOD RangeAdd(nsIDOMRange& aRange);
//   NS_IMETHOD RangeRemove(nsIDOMRange& aRange);
//   NS_IMETHOD GetRangeList(nsVoidArray*& aResult) const;
//   NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext,
//                             nsEvent* aEvent,
//                             nsIDOMEvent** aDOMEvent,
//                             PRUint32 aFlags,
//                             nsEventStatus* aEventStatus);
//   NS_IMETHOD GetContentID(PRUint32* aID);
//   NS_IMETHOD SetContentID(PRUint32 aID);
//   NS_IMETHOD SetFocus(nsIPresContext* aContext);
//   NS_IMETHOD RemoveFocus(nsIPresContext* aContext);
//   NS_IMETHOD GetBindingParent(nsIContent** aContent);
//   NS_IMETHOD SetBindingParent(nsIContent* aParent);

  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;  

  // nsIXMLContent
  NS_IMETHOD SetContainingNameSpace(nsINameSpace* aNameSpace);
  NS_IMETHOD GetContainingNameSpace(nsINameSpace*& aNameSpace) const;
//   NS_IMETHOD MaybeTriggerAutoLink(nsIWebShell *aShell);

  // nsIStyledContent
  NS_IMETHOD GetID(nsIAtom*& aResult) const;
//   NS_IMETHOD GetClasses(nsVoidArray& aArray) const;
//   NS_IMETHOD HasClass(nsIAtom* aClass) const;
  
  NS_IMETHOD WalkContentStyleRules(nsIRuleWalker* aRuleWalker);
  NS_IMETHOD WalkInlineStyleRules(nsIRuleWalker* aRuleWalker);
  
  NS_IMETHOD GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                      PRInt32& aHint) const;
  
  // nsIDOMNode
  NS_DECL_NSIDOMNODE
  
  // nsIDOMElement
  // NS_DECL_IDOMELEMENT
  NS_FORWARD_NSIDOMELEMENT(nsGenericElement::)
  
  // nsIDOMSVGElement
  NS_DECL_NSIDOMSVGELEMENT

  // nsISVGValueObserver
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable);

  // nsISupportsWeakReference
  // implementation inherited from nsSupportsWeakReference
  
protected:

  nsresult CopyNode(nsSVGElement* dest, PRBool deep);
  
  nsVoidArray                  mChildren;   
  nsSVGAttributes*             mAttributes;
  nsCOMPtr<nsINameSpace>       mNameSpace;
  nsCOMPtr<nsISVGStyleValue>   mStyle;
};

#endif // __NS_SVGELEMENT_H__
