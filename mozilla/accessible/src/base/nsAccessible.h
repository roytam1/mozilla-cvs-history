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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *      John Gaunt (jgaunt@netscape.com)
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

#ifndef _nsAccessible_H_
#define _nsAccessible_H_

#include "nsCOMPtr.h"
#include "nsGenericAccessible.h"
#include "nsIAccessible.h"
#include "nsIAccessibilityService.h"
#include "nsIDOMNode.h"
#include "nsIFocusController.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsWeakReference.h"
#include "nsIDOMNodeList.h"
#include "nsIBindingManager.h"
#include "nsIStringBundle.h"

#define ACCESSIBLE_BUNDLE_URL "chrome://global-platform/locale/accessible.properties"
#define PLATFORM_KEYS_BUNDLE_URL "chrome://global-platform/locale/platformKeys.properties"

class nsIContent;
class nsIDocShell;
class nsIFrame;
class nsIWebShell;

enum { eSiblingsUninitialized = -1, eSiblingsWalkNormalDOM = -2};  // Used in sibling index field as flags

class nsAccessible : public nsGenericAccessible
{
public:
  // to eliminate the confusion of "magic numbers" -- if ( 0 ){ foo; }
  enum { eAction_Switch=0, eAction_Jump=0, eAction_Click=0, eAction_Select=0 };
  // how many actions
  enum { eNo_Action=0, eSingle_Action=1 };

  nsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  virtual ~nsAccessible();

  NS_IMETHOD GetAccName(nsAString& _retval);
  NS_IMETHOD GetAccId(PRInt32 *_retval);
  NS_IMETHOD GetAccKeyboardShortcut(nsAString& _retval);
  NS_IMETHOD GetAccDescription(nsAString& _retval);
  NS_IMETHOD GetAccParent(nsIAccessible **_retval); 
  NS_IMETHOD GetAccNextSibling(nsIAccessible **_retval); 
  NS_IMETHOD GetAccPreviousSibling(nsIAccessible **_retval); 
  NS_IMETHOD GetAccFirstChild(nsIAccessible **_retval); 
  NS_IMETHOD GetAccLastChild(nsIAccessible **_retval); 
  NS_IMETHOD GetAccChildCount(PRInt32 *_retval); 
  NS_IMETHOD GetAccState(PRUint32 *_retval); 
  NS_IMETHOD GetAccFocused(nsIAccessible **_retval); 
  NS_IMETHOD AccGetAt(PRInt32 x, PRInt32 y, nsIAccessible **_retval); 
  NS_IMETHOD AccGetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height); 
  NS_IMETHOD AccRemoveSelection(void); 
  NS_IMETHOD AccTakeSelection(void); 
  NS_IMETHOD AccTakeFocus(void); 
  NS_IMETHOD AccGetDOMNode(nsIDOMNode **_retval); 

protected:
  virtual nsIFrame* GetFrame();
  virtual nsIFrame* GetBoundsFrame();
  virtual void GetBounds(nsRect& aRect, nsIFrame** aRelativeFrame);
  virtual void GetPresContext(nsCOMPtr<nsIPresContext>& aContext);
  PRBool IsPartiallyVisible(PRBool *aIsOffscreen); 
  NS_IMETHOD AppendLabelText(nsIDOMNode *aLabelNode, nsAString& _retval);
  NS_IMETHOD AppendLabelFor(nsIContent *aLookNode, const nsAString *aId, nsAString *aLabel);
  NS_IMETHOD GetHTMLAccName(nsAString& _retval);
  NS_IMETHOD GetXULAccName(nsAString& _retval);
  NS_IMETHOD AppendFlatStringFromSubtree(nsIContent *aContent, nsAString *aFlatString);
  NS_IMETHOD AppendFlatStringFromContentNode(nsIContent *aContent, nsAString *aFlatString);
  NS_IMETHOD AppendStringWithSpaces(nsAString *aFlatString, const nsAString& textEquivalent);
  NS_IMETHOD GetFocusedNode(nsIDOMNode **aFocusedNode);
  NS_IMETHOD CacheOptimizations(nsIAccessible *aParent, PRInt32 aSiblingIndex, nsIDOMNodeList *aSiblingList);
  // helper method to verify frames
  static PRBool IsCorrectFrameType(nsIFrame* aFrame, nsIAtom* aAtom);
  static nsresult GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut);
  static nsresult GetTranslatedString(const nsAString& aKey, nsAString& aStringOut);
  void GetScrollOffset(nsRect *aRect);
  void GetScreenOrigin(nsIPresContext *aPresContext, nsIFrame *aFrame, nsRect *aRect);
  nsresult AppendFlatStringFromSubtreeRecurse(nsIContent *aContent, nsAString *aFlatString);

  // Data Members
  nsCOMPtr<nsIDOMNode> mDOMNode;
  nsCOMPtr<nsIWeakReference> mPresShell;
  nsCOMPtr<nsIAccessible> mParent;
  nsCOMPtr<nsIDOMNodeList> mSiblingList; // If some of our computed siblings are anonymous content nodes, cache node list
  PRInt32 mSiblingIndex; // Cache where we are in list of kids that we got from nsIBindingManager::GetContentList(parentContent)

  static PRUint32 gInstanceCount;
  static nsIStringBundle *gStringBundle;
  static nsIStringBundle *gKeyStringBundle;
};


/** This class is used to walk the DOM tree. It skips
  * everything but nodes that either implement nsIAccessible
  * or have primary frames that implement "GetAccessible"
  */

struct WalkState {
  nsCOMPtr<nsIAccessible> accessible;
  nsCOMPtr<nsIDOMNode> domNode;
  nsCOMPtr<nsIDOMNodeList> siblingList;
  PRInt32 siblingIndex;  // Holds a state flag or an index into the siblingList
  WalkState *prevState;
};

 
class nsAccessibleTreeWalker {
public:
  nsAccessibleTreeWalker(nsIWeakReference* aShell, nsIDOMNode* aContent, 
    PRInt32 aCachedSiblingIndex, nsIDOMNodeList *aCachedSiblingList, PRBool mWalkAnonymousContent);
  virtual ~nsAccessibleTreeWalker();

  NS_IMETHOD GetNextSibling();
  NS_IMETHOD GetPreviousSibling();
  NS_IMETHOD GetParent();
  NS_IMETHOD GetFirstChild();
  NS_IMETHOD GetLastChild();
  PRInt32 GetChildCount();
  WalkState mState;
  WalkState mInitialState;

protected:
  NS_IMETHOD GetChildBefore(nsIDOMNode* aParent, nsIDOMNode* aChild);
  PRBool IsHidden();
  PRBool GetAccessible();
  NS_IMETHOD GetFullTreeParentNode(nsIDOMNode *aChildNode, nsIDOMNode **aParentNodeOut);
  void GetSiblings(nsIDOMNode *aOneOfTheSiblings);
  void GetKids(nsIDOMNode *aParent);

  void ClearState();
  NS_IMETHOD PushState();
  NS_IMETHOD PopState();

  nsCOMPtr<nsIWeakReference> mPresShell;
  nsCOMPtr<nsIAccessibilityService> mAccService;
  nsCOMPtr<nsIBindingManager> mBindingManager;
};

#endif  
