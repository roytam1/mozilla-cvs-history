/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

/*

  Private interface to the XBL Binding

*/

#ifndef nsIXBLBinding_h__
#define nsIXBLBinding_h__

#include "nsString.h"
#include "nsISupports.h"
#include "nsISupportsArray.h"

class nsIContent;
class nsIDocument;
class nsIDOMNodeList;
class nsIScriptContext;
class nsIXBLPrototypeBinding;

// {DDDBAD20-C8DF-11d3-97FB-00400553EEF0}
#define NS_IXBLBINDING_IID \
{ 0xdddbad20, 0xc8df, 0x11d3, { 0x97, 0xfb, 0x0, 0x40, 0x5, 0x53, 0xee, 0xf0 } }

class nsIXBLBinding : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IXBLBINDING_IID; return iid; }

  NS_IMETHOD GetPrototypeBinding(nsIXBLPrototypeBinding** aResult)=0;
  NS_IMETHOD SetPrototypeBinding(nsIXBLPrototypeBinding* aProtoBinding)=0;

  NS_IMETHOD GetBaseBinding(nsIXBLBinding** aResult) = 0;
  NS_IMETHOD SetBaseBinding(nsIXBLBinding* aBinding) = 0;

  NS_IMETHOD GetAnonymousContent(nsIContent** aParent) = 0;
  NS_IMETHOD SetAnonymousContent(nsIContent* aParent) = 0;

  NS_IMETHOD GetBindingElement(nsIContent** aResult) = 0;
  NS_IMETHOD SetBindingElement(nsIContent* aElement) = 0;

  NS_IMETHOD GetBoundElement(nsIContent** aResult) = 0;
  NS_IMETHOD SetBoundElement(nsIContent* aElement) = 0;

  NS_IMETHOD GenerateAnonymousContent() = 0;
  NS_IMETHOD InstallEventHandlers() = 0;
  NS_IMETHOD InstallProperties() = 0;
  
  NS_IMETHOD HasStyleSheets(PRBool* aResolveStyle) = 0;

  NS_IMETHOD GetFirstBindingWithConstructor(nsIXBLBinding** aResult)=0;

  NS_IMETHOD GetBaseTag(PRInt32* aNameSpaceID, nsIAtom** aResult) = 0;

  // Called when an attribute changes on a binding.
  NS_IMETHOD AttributeChanged(nsIAtom* aAttribute, PRInt32 aNameSpaceID, PRBool aRemoveFlag) = 0;

  NS_IMETHOD ExecuteAttachedHandler()=0;
  NS_IMETHOD ExecuteDetachedHandler()=0;
  NS_IMETHOD UnhookEventHandlers() = 0;
  NS_IMETHOD ChangeDocument(nsIDocument* aOldDocument, nsIDocument* aNewDocument) = 0;

  NS_IMETHOD GetBindingURI(nsCString& aResult) = 0;
  NS_IMETHOD GetDocURI(nsCString& aResult) = 0;
  NS_IMETHOD GetID(nsCString& aResult) = 0;

  NS_IMETHOD GetInsertionPointsFor(nsIContent* aParent, nsISupportsArray** aResult)=0;

  NS_IMETHOD GetInsertionPoint(nsIContent* aChild, nsIContent** aResult, PRUint32* aIndex,
                               nsIContent** aDefaultContent) = 0;
  NS_IMETHOD GetSingleInsertionPoint(nsIContent** aResult, PRUint32* aIndex, 
                                     PRBool* aMultipleInsertionPoints, nsIContent** aDefaultContent) = 0;

  NS_IMETHOD IsStyleBinding(PRBool* aResult) = 0;
  NS_IMETHOD SetIsStyleBinding(PRBool aIsStyle) = 0;

  NS_IMETHOD GetRootBinding(nsIXBLBinding** aResult) = 0;
  NS_IMETHOD GetFirstStyleBinding(nsIXBLBinding** aResult) = 0;

  NS_IMETHOD InheritsStyle(PRBool* aResult)=0;
  NS_IMETHOD WalkRules(nsISupportsArrayEnumFunc aFunc, void* aData)=0;
  NS_IMETHOD AttributeAffectsStyle(nsISupportsArrayEnumFunc aFunc, void* aData, PRBool* aAffects)=0;

  NS_IMETHOD MarkForDeath()=0;
  NS_IMETHOD MarkedForDeath(PRBool* aResult)=0;

  NS_IMETHOD ImplementsInterface(REFNSIID aIID, PRBool* aResult)=0;

  NS_IMETHOD GetAnonymousNodes(nsIDOMNodeList** aResult)=0;

  NS_IMETHOD ShouldBuildChildFrames(PRBool* aResult)=0;
};

extern nsresult
NS_NewXBLBinding(nsIXBLPrototypeBinding* aProtoBinding, nsIXBLBinding** aResult);

#endif // nsIXBLBinding_h__
