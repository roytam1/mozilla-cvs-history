/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCOMPtr.h"
#include "nsIXBLPrototypeBinding.h"
#include "nsIXBLPrototypeHandler.h"
#include "nsXBLPrototypeResources.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsWeakReference.h"

class nsIContent;
class nsIAtom;
class nsIDocument;
class nsIScriptContext;
class nsISupportsArray;
class nsSupportsHashtable;
class nsIXBLService;
class nsFixedSizeAllocator;

// *********************************************************************/
// The XBLPrototypeBinding class

class nsXBLPrototypeBinding: public nsIXBLPrototypeBinding, public nsSupportsWeakReference
{
  NS_DECL_ISUPPORTS

  // nsIXBLPrototypeBinding
  NS_IMETHOD GetBindingElement(nsIContent** aResult);
  NS_IMETHOD SetBindingElement(nsIContent* aElement);

  NS_IMETHOD GetBindingURI(nsCString& aResult);
  NS_IMETHOD GetDocURI(nsCString& aResult);
  NS_IMETHOD GetID(nsCString& aResult);

  NS_IMETHOD GetAllowScripts(PRBool* aResult);

  NS_IMETHOD BindingAttached(nsIDOMEventReceiver* aRec);
  NS_IMETHOD BindingDetached(nsIDOMEventReceiver* aRec);

  NS_IMETHOD LoadResources(PRBool* aResult);
  NS_IMETHOD AddResource(nsIAtom* aResourceType, const nsAString& aSrc);

  NS_IMETHOD InheritsStyle(PRBool* aResult);

  NS_IMETHOD GetPrototypeHandlers(nsIXBLPrototypeHandler** aHandler);
  NS_IMETHOD SetPrototypeHandlers(nsIXBLPrototypeHandler* aHandler);

  NS_IMETHOD GetConstructor(nsIXBLPrototypeHandler** aResult);
  NS_IMETHOD SetConstructor(nsIXBLPrototypeHandler* aConstructor);
  NS_IMETHOD GetDestructor(nsIXBLPrototypeHandler** aResult);
  NS_IMETHOD SetDestructor(nsIXBLPrototypeHandler* aDestructor);

  NS_IMETHOD InitClass(const nsCString& aClassName, nsIScriptContext * aContext, void * aScriptObject, void ** aClassObject);
  
  NS_IMETHOD ConstructInterfaceTable(const nsAString& aImpls);
  
  NS_IMETHOD SetImplementation(nsXBLProtoImpl* aImpl) { mImplementation = aImpl; return NS_OK; };
  NS_IMETHOD InstallImplementation(nsIContent* aBoundElement);

  NS_IMETHOD AttributeChanged(nsIAtom* aAttribute, PRInt32 aNameSpaceID,
                              PRBool aRemoveFlag, nsIContent* aChangedElement,
                              nsIContent* aAnonymousContent, PRBool aNotify);

  NS_IMETHOD SetBasePrototype(nsIXBLPrototypeBinding* aBinding);
  NS_IMETHOD GetBasePrototype(nsIXBLPrototypeBinding** aResult);

  NS_IMETHOD GetXBLDocumentInfo(nsIContent* aBoundElement, nsIXBLDocumentInfo** aResult);
  
  NS_IMETHOD HasBasePrototype(PRBool* aResult);
  NS_IMETHOD SetHasBasePrototype(PRBool aHasBase);

  NS_IMETHOD SetInitialAttributes(nsIContent* aBoundElement, nsIContent* aAnonymousContent);

  NS_IMETHOD GetRuleProcessors(nsISupportsArray** aResult);
  NS_IMETHOD GetStyleSheets(nsISupportsArray** aResult);

  NS_IMETHOD HasInsertionPoints(PRBool* aResult) { *aResult = (mInsertionPointTable != nsnull); return NS_OK; };
  
  NS_IMETHOD HasStyleSheets(PRBool* aResult) 
  { *aResult = (mResources && mResources->mStyleSheetList); return NS_OK; };

  NS_IMETHOD FlushSkinSheets();

  NS_IMETHOD InstantiateInsertionPoints(nsIXBLBinding* aBinding);

  NS_IMETHOD GetInsertionPoint(nsIContent* aBoundElement, nsIContent* aCopyRoot,
                               nsIContent* aChild, nsIContent** aResult, PRUint32* aIndex,
                               nsIContent** aDefaultContent);

  NS_IMETHOD GetSingleInsertionPoint(nsIContent* aBoundElement, nsIContent* aCopyRoot,
                                     nsIContent** aResult, PRUint32* aIndex, PRBool* aMultiple,
                                     nsIContent** aDefaultContent);

  NS_IMETHOD GetBaseTag(PRInt32* aNamespaceID, nsIAtom** aTag);
  NS_IMETHOD SetBaseTag(PRInt32 aNamespaceID, nsIAtom* aTag);

  NS_IMETHOD ImplementsInterface(REFNSIID aIID, PRBool* aResult);

  NS_IMETHOD ShouldBuildChildFrames(PRBool* aResult);

  NS_IMETHOD AddResourceListener(nsIContent* aBoundElement);

  NS_IMETHOD Initialize();

public:
  nsXBLPrototypeBinding(const nsACString& aRef, nsIXBLDocumentInfo* aInfo, nsIContent* aElement);
  virtual ~nsXBLPrototypeBinding();

  
// Static members
  static PRUint32 gRefCnt;
 
  static nsFixedSizeAllocator* kAttrPool;
  static nsFixedSizeAllocator* kInsPool;

// Internal member functions
public:
  void GetImmediateChild(nsIAtom* aTag, nsIContent** aResult);
  void LocateInstance(nsIContent* aBoundElt, nsIContent* aTemplRoot, nsIContent* aCopyRoot,
                      nsIContent* aTemplChild, nsIContent** aCopyResult);

protected:  
  void ConstructAttributeTable(nsIContent* aElement);
  void ConstructInsertionTable(nsIContent* aElement);
  void GetNestedChildren(nsIAtom* aTag, nsIContent* aContent, nsISupportsArray** aList);
  
protected:
  // Internal helper class for managing our IID table.
  class nsIIDKey : public nsHashKey {
    protected:
      nsIID mKey;
  
    public:
      nsIIDKey(REFNSIID key) : mKey(key) {}
      ~nsIIDKey(void) {}

      PRUint32 HashCode(void) const {
        // Just use the 32-bit m0 field.
        return mKey.m0;
      }

      PRBool Equals(const nsHashKey *aKey) const {
        return mKey.Equals( ((nsIIDKey*) aKey)->mKey);
      }

      nsHashKey *Clone(void) const {
        return new nsIIDKey(mKey);
      }
  };

// MEMBER VARIABLES
protected:
  char* mID;

  nsCOMPtr<nsIContent> mBinding; // Strong. We own a ref to our content element in the binding doc.
  nsCOMPtr<nsIXBLPrototypeHandler> mPrototypeHandler; // Strong. DocInfo owns us, and we own the handlers.
  
  nsXBLProtoImpl* mImplementation; // Our prototype implementation (includes methods, properties, fields,
                                   // the constructor, and the destructor).

  nsCOMPtr<nsIXBLPrototypeBinding> mBaseBinding; // Strong. We own the base binding in our explicit inheritance chain.
  PRPackedBool mInheritStyle;
  PRPackedBool mHasBaseProto;
 
  nsXBLPrototypeResources* mResources; // If we have any resources, this will be non-null.
                                      
  nsWeakPtr mXBLDocInfoWeak; // A pointer back to our doc info.  Weak, since it owns us.

  nsSupportsHashtable* mAttributeTable; // A table for attribute entries.  Used to efficiently
                                        // handle attribute changes.

  nsSupportsHashtable* mInsertionPointTable; // A table of insertion points for placing explicit content
                                             // underneath anonymous content.

  nsSupportsHashtable* mInterfaceTable; // A table of cached interfaces that we support.

  PRInt32 mBaseNameSpaceID;    // If we extend a tagname/namespace, then that information will
  nsCOMPtr<nsIAtom> mBaseTag;  // be stored in here.
};
