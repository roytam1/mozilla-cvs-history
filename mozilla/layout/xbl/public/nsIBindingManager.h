/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsIBinding_Manager_h__
#define nsIBinding_Manager_h__

#include "nsString.h"
#include "nsISupports.h"
#include "nsISupportsArray.h"

class nsIContent;
class nsIXBLBinding;
class nsIXBLBindingAttachedHandler;
class nsIXBLDocumentInfo;
class nsIAtom;
class nsIStreamListener;
class nsIXPConnectWrappedJS;

// {55D70FE0-C8E5-11d3-97FB-00400553EEF0}
#define NS_IBINDING_MANAGER_IID \
{ 0x55d70fe0, 0xc8e5, 0x11d3, { 0x97, 0xfb, 0x0, 0x40, 0x5, 0x53, 0xee, 0xf0 } }

class nsIBindingManager : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IBINDING_MANAGER_IID; return iid; }

  NS_IMETHOD GetBinding(nsIContent* aContent, nsIXBLBinding** aResult) = 0;
  NS_IMETHOD SetBinding(nsIContent* aContent, nsIXBLBinding* aBinding) = 0;

  NS_IMETHOD GetInsertionParent(nsIContent* aContent, nsIContent** aResult)=0;
  NS_IMETHOD SetInsertionParent(nsIContent* aContent, nsIContent* aResult)=0;

  NS_IMETHOD GetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS** aResult) = 0;
  NS_IMETHOD SetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS* aResult) = 0;

  /**
   * Notify the binding manager that an element
   * has been moved from one document to another,
   * so that it can update any bindings or
   * nsIAnonymousContentCreator-created anonymous
   * content that may depend on the document.
   * @param aContent the element that's being moved
   * @param aOldDocument the old document in which the
   *   content resided. May be null if the the content
   *   was not in any document.
   * @param aNewDocument the document in which the
   *   content will reside. May be null if the content
   *   will not reside in any document, or if the
   *   content is being destroyed.
   */
  NS_IMETHOD ChangeDocumentFor(nsIContent* aContent, nsIDocument* aOldDocument,
                               nsIDocument* aNewDocument) = 0;


  NS_IMETHOD ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID, nsIAtom** aResult) = 0;

  // For a given element with an insertion point child, returns a flat list of all the real children.
  NS_IMETHOD GetContentListFor(nsIContent* aContent, nsIDOMNodeList** aResult) = 0;
  NS_IMETHOD SetContentListFor(nsIContent* aContent, nsISupportsArray* aList)=0;

  NS_IMETHOD GetAnonymousNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult) = 0;
  NS_IMETHOD SetAnonymousNodesFor(nsIContent* aContent, nsISupportsArray* aList) = 0;

  // Encapsulates logic for handling both of above
  NS_IMETHOD GetXBLChildNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult) = 0;

  NS_IMETHOD GetInsertionPoint(nsIContent* aParent, nsIContent* aChild, nsIContent** aResult, PRUint32* aIndex) = 0;
  NS_IMETHOD GetSingleInsertionPoint(nsIContent* aParent, nsIContent** aResult, PRUint32* aIndex,  
                                     PRBool* aMultipleInsertionPoints) = 0;

  NS_IMETHOD AddLayeredBinding(nsIContent* aContent, const nsAReadableString& aURL) = 0;
  NS_IMETHOD RemoveLayeredBinding(nsIContent* aContent, const nsAReadableString& aURL) = 0;
  NS_IMETHOD LoadBindingDocument(nsIDocument* aDocument, const nsAReadableString& aURL,
                                 nsIDocument** aResult) = 0;

  NS_IMETHOD AddToAttachedQueue(nsIXBLBinding* aBinding)=0;
  NS_IMETHOD AddHandlerToAttachedQueue(nsIXBLBindingAttachedHandler* aHandler)=0;
  NS_IMETHOD ClearAttachedQueue()=0;
  NS_IMETHOD ProcessAttachedQueue()=0;

  NS_IMETHOD ExecuteDetachedHandlers()=0;

  NS_IMETHOD PutXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo)=0;
  NS_IMETHOD GetXBLDocumentInfo(const nsCString& aURL, nsIXBLDocumentInfo** aResult)=0;

  NS_IMETHOD PutLoadingDocListener(const nsCString& aURL, nsIStreamListener* aListener) = 0;
  NS_IMETHOD GetLoadingDocListener(const nsCString& aURL, nsIStreamListener** aResult) = 0;
  NS_IMETHOD RemoveLoadingDocListener(const nsCString& aURL)=0;

  NS_IMETHOD InheritsStyle(nsIContent* aContent, PRBool* aResult) = 0;
  NS_IMETHOD FlushChromeBindings() = 0;

  NS_IMETHOD GetBindingImplementation(nsIContent* aContent, void* aScriptObject, REFNSIID aIID, void** aResult)=0;

  NS_IMETHOD ShouldBuildChildFrames(nsIContent* aContent, PRBool* aResult) = 0;
};

#endif // nsIBinding_Manager_h__
