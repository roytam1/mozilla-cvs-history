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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsITransactionManager_h__
#define nsITransactionManager_h__

#include "nsISupports.h"
#include "nsIOutputStream.h"
#include "nsITransaction.h"
#include "nsITransactionListener.h"

/*
Transaction Manager interface to outside world
*/

#define NS_ITRANSACTIONMANAGER_IID \
{ /* 58E330C2-7B48-11d2-98B9-00805F297D89 */ \
0x58e330c2, 0x7b48, 0x11d2, \
{ 0x98, 0xb9, 0x0, 0x80, 0x5f, 0x29, 0x7d, 0x89 } }


/**
 * A transaction manager specific interface. 
 * <P>
 * It's implemented by an object that tracks transactions.
 */
class nsITransactionManager  : public nsISupports{
public:

  static const nsIID& GetIID() { static nsIID iid = NS_ITRANSACTIONMANAGER_IID; return iid; }

  /**
   * Calls a transaction's Do() method, then pushes it on the undo stack.
   * <P>
   * This method calls the transaction's AddRef() method.
   * The transaction's Release() method will be called when the undo or redo
   * stack is pruned or when the transaction manager is destroyed.
   * @param aTransaction the transaction to do.
   */
  NS_IMETHOD Do(nsITransaction *aTransaction) = 0;

  /**
   * Pops the topmost transaction on the undo stack, calls it's Undo() method,
   * then pushes it on the redo stack.
   */
  NS_IMETHOD Undo(void) = 0;

  /**
   * Pops the topmost transaction on the redo stack, calls it's Redo() method,
   * then pushes it on the undo stack.
   */
  NS_IMETHOD Redo(void) = 0;

  /**
   * Clears the undo and redo stacks.
   */
  NS_IMETHOD Clear(void) = 0;

  /**
   * Turns on the transaction manager's batch mode, forcing all transactions
   * executed by the transaction manager's Do() method to be aggregated
   * together until EndBatch() is called.  This mode allows an application to
   * execute and group together several independent transactions so they
   * can be undone with a single call to Undo().
   */
  NS_IMETHOD BeginBatch() = 0;

  /**
   * Turns off the transaction manager's batch mode.
   */
  NS_IMETHOD EndBatch() = 0;

  /**
   * Returns the number of items on the undo stack.
   * @param aNumItems will contain number of items.
   */
  NS_IMETHOD GetNumberOfUndoItems(PRInt32 *aNumItems) = 0;

  /**
   * Returns the number of items on the redo stack.
   * @param aNumItems will contain number of items.
   */
  NS_IMETHOD GetNumberOfRedoItems(PRInt32 *aNumItems) = 0;

  /**
   * Sets the maximum number of transaction items the transaction manager will
   * maintain at any time. This is commonly referred to as the number of levels
   * of undo.
   * @param aMaxCount A value of -1 means no limit. A value of zero means the
   * transaction manager will execute each transaction, then immediately release
   * all references it has to the transaction without pushing it on the undo
   * stack. A value greater than zero indicates the max number of transactions
   * that can exist at any time on both the undo and redo stacks. This method
   * will prune the neccessary number of transactions on the undo and redo
   * stacks if the value specified is less than the number of items that exist
   * on both the undo and redo stacks.
   */
  NS_IMETHOD SetMaxTransactionCount(PRInt32 aMaxCount) = 0;

  /**
   * Returns a pointer to the transaction at the top of the undo stack.
   * @param aTransaction will contain pointer to the transaction.
   */
  NS_IMETHOD PeekUndoStack(nsITransaction **aTransaction) = 0;

  /**
   * Returns a pointer to the transaction at the top of the redo stack.
   * @param aTransaction will contain pointer to the transaction.
   */
  NS_IMETHOD PeekRedoStack(nsITransaction **aTransaction) = 0;

  /**
   * Writes a stream representation of the transaction manager and it's
   * execution stacks. Calls the Write() method of each transaction on the
   * execution stacks.
   * @param aOutputStream the stream to write to.
   */
  NS_IMETHOD Write(nsIOutputStream *aOutputStream) = 0;

  /**
   * Adds a listener to the transaction manager's notification list. Listeners
   * are notified whenever a transaction is done, undone, or redone.
   * <P>
   * The listener's AddRef() method is called.
   * @param aListener the lister to add.
   */
  NS_IMETHOD AddListener(nsITransactionListener *aListener) = 0;

  /**
   * Removes a listener from the transaction manager's notification list.
   * <P>
   * The listener's Release() method is called.
   * @param aListener the lister to remove.
   */
  NS_IMETHOD RemoveListener(nsITransactionListener *aListener) = 0;
};

#endif // nsITransactionManager_h__

