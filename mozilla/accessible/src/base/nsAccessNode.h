/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Original Author: Aaron Leventhal (aaronl@netscape.com)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/* For documentation of the accessibility architecture, 
 * see http://lxr.mozilla.org/seamonkey/source/accessible/accessible-docs.html
 */

#ifndef _nsAccessNode_H_
#define _nsAccessNode_H_

#include "nsCOMPtr.h"
#include "nsIAccessNode.h"
#include "nsIDOMNode.h"
#include "nsIStringBundle.h"
#include "nsWeakReference.h"

#define OLD_HASH 1

#ifdef OLD_HASH
class nsSupportsHashtable;
class nsHashKey;
#include "nsHashtable.h"
#else
#include "nsInterfaceHashtable.h"
#include "pldhash.h"
#endif

class nsIPresShell;
class nsIPresContext;
class nsIAccessibleDocument;
class nsIFrame;
class nsIDOMNodeList;

enum { eChildCountUninitialized = 0xffff };
enum { eSiblingsUninitialized = -1, eSiblingsWalkNormalDOM = -2 };

#define ACCESSIBLE_BUNDLE_URL "chrome://global-platform/locale/accessible.properties"
#define PLATFORM_KEYS_BUNDLE_URL "chrome://global-platform/locale/platformKeys.properties"

#ifndef OLD_HASH
/* hashkey wrapper using void* KeyType
 *
 * @see nsTHashtable::EntryType for specification
 */
class NS_COM nsVoidHashKey : public PLDHashEntryHdr
{
public:  typedef void* voidPointer;
  typedef const voidPointer& KeyType;
  typedef const voidPointer* KeyTypePointer;
  
  const voidPointer mValue;

public:
  nsVoidHashKey(KeyTypePointer aKey) : mValue(*aKey) { }
  nsVoidHashKey(const nsVoidHashKey& toCopy) : mValue(toCopy.mValue) { }
  ~nsVoidHashKey() { }

  KeyType GetKey() const { return mValue; }
  KeyTypePointer GetKeyPointer() const { return &mValue; }
  PRBool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return NS_PTR_TO_INT32(*aKey) >> 2; }
  static PRBool AllowMemMove() { return PR_TRUE; }

};
#endif

class nsAccessNode: public nsIAccessNode
{
  public: // construction, destruction
    nsAccessNode(nsIDOMNode *, nsIWeakReference* aShell);
    virtual ~nsAccessNode();

    NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
    NS_IMETHOD_(nsrefcnt) AddRef(void);
    NS_IMETHOD_(nsrefcnt) Release(void);
    NS_DECL_NSIACCESSNODE

    static void InitXPAccessibility();
    static void ShutdownXPAccessibility();

    // Static methods for handling per-document cache
#ifdef OLD_HASH
    static void PutCacheEntry(nsSupportsHashtable *aCache, 
                              void* aUniqueID, nsIAccessNode *aAccessNode);
    static void GetCacheEntry(nsSupportsHashtable *aCache, void* aUniqueID, 
                              nsIAccessNode **aAccessNode);
    static void ClearCache(nsSupportsHashtable *aCache);
    static PRIntn PR_CALLBACK ClearCacheEntry(nsHashKey *aKey, void *aData, 
                                              void* aClosure);
#else
    static void PutCacheEntry(nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode> *aCache, 
                              void* aUniqueID, nsIAccessNode *aAccessNode);
    static void GetCacheEntry(nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode> *aCache, void* aUniqueID, 
                              nsIAccessNode **aAccessNode);
    static void ClearCache(nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode> *aCache);

    static PLDHashOperator PR_CALLBACK ClearCacheEntry(void *const& aKey, nsCOMPtr<nsIAccessNode> aAccessNode, void* aUserArg);
#endif

    // Static cache methods for global document cache
    static void GetDocAccessibleFor(nsIWeakReference *aPresShell,
                                    nsIAccessibleDocument **aDocAccessible);

  protected:
    already_AddRefed<nsIPresShell> GetPresShell();
    already_AddRefed<nsIPresContext> GetPresContext();
    already_AddRefed<nsIAccessibleDocument> GetDocAccessible();
    nsIFrame* GetFrame();

    nsCOMPtr<nsIDOMNode> mDOMNode;
    nsCOMPtr<nsIWeakReference> mWeakShell;

    PRInt16 mRefCnt;
    PRUint16 mAccChildCount;
    NS_DECL_OWNINGTHREAD

#ifdef DEBUG
    PRBool mIsInitialized;
#endif

    // Static data, we do our own refcounting for our static data
    static nsIStringBundle *gStringBundle;
    static nsIStringBundle *gKeyStringBundle;
    static nsIDOMNode *gLastFocusedNode;
    static PRBool gIsAccessibilityActive;
    static PRBool gIsCacheDisabled;

#ifdef OLD_HASH
    static nsSupportsHashtable *gGlobalDocAccessibleCache;
#else
    static nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode> *gGlobalDocAccessibleCache;
#endif
};

#endif

