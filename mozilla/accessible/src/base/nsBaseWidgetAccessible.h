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
 *       John Gaunt (jgaunt@netscape.com)
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

#ifndef _nsBaseWidgetAccessible_H_
#define _nsBaseWidgetAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsIContent.h"

class nsIDOMNode;

/**
  * This file contains a number of classes that are used as base
  *  classes for the different accessibility implementations of
  *  the HTML and XUL widget sets.  --jgaunt
  */

/**
  * Special Accessible that knows how to handle hit detection for flowing text
  */
class nsBlockAccessible : public nsAccessibleWrap
{
public:
  nsBlockAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_IMETHOD AccGetAt(PRInt32 x, PRInt32 y, nsIAccessible **_retval);
};

/**
  * Special Accessible that just contains other accessible objects
  *   no actions, no name, no state, no value
  */
class nsContainerAccessible : public nsAccessibleWrap
{
public:
  nsContainerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_IMETHOD GetAccNumActions(PRUint8 *_retval);
  NS_IMETHOD GetAccActionName(PRUint8 index, nsAString& _retval);
  NS_IMETHOD AccDoAction(PRUint8 index);
  NS_IMETHOD GetAccState(PRUint32 *_retval);
  NS_IMETHOD GetAccValue(nsAString& _retval);
  NS_IMETHOD GetAccName(nsAString& _retval); 
};

/** 
  * Leaf version of DOM Accessible -- has no children
  */
class nsLeafAccessible : public nsAccessibleWrap
{
public:
  nsLeafAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_IMETHOD GetAccFirstChild(nsIAccessible **_retval);
  NS_IMETHOD GetAccLastChild(nsIAccessible **_retval);
  NS_IMETHOD GetAccChildCount(PRInt32 *_retval);
};

/**
  * A type of accessible for DOM nodes containing an href="" attribute.
  *  It knows how to report the state of the link ( traveled or not )
  *  and can activate ( click ) the link programmatically.
  */
class nsLinkableAccessible : public nsAccessibleWrap
{
public:
  nsLinkableAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_IMETHOD GetAccNumActions(PRUint8 *_retval);
  NS_IMETHOD GetAccActionName(PRUint8 index, nsAString& _retval);
  NS_IMETHOD AccDoAction(PRUint8 index);
  NS_IMETHOD GetAccState(PRUint32 *_retval);
  NS_IMETHOD GetAccValue(nsAString& _retval);
  NS_IMETHOD AccTakeFocus();
  NS_IMETHOD GetAccKeyboardShortcut(nsAString& _retval);
  NS_IMETHOD Shutdown();

protected:
  PRBool IsALink();
  nsCOMPtr<nsIContent> mLinkContent;
  PRPackedBool mIsALinkCached;  // -1 = unknown, 0 = not a link, 1 = is a link
  PRPackedBool mIsLinkVisited;
};

#endif  

