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

#ifndef nsITransaction_h__
#define nsITransaction_h__

#include "nsISupports.h"
#include "nsIOutputStream.h"
#include "nsString.h"

/*
Transaction interface to outside world
*/

#define NS_ITRANSACTION_IID \
{ /* 58E330C1-7B48-11d2-98B9-00805F297D89 */ \
0x58e330c1, 0x7b48, 0x11d2, \
{ 0x98, 0xb9, 0x0, 0x80, 0x5f, 0x29, 0x7d, 0x89 } }


/**
 * A transaction specific interface. 
 * <P>
 * It's implemented by an object that executes some behavior that must be
 * tracked by the transaction manager.
 */
class nsITransaction  : public nsISupports{
public:

  static const nsIID& GetIID() { static nsIID iid = NS_ITRANSACTION_IID; return iid; }

  /**
   * Executes the transaction.
   */
  NS_IMETHOD Do(void) = 0;

  /**
   * Restores the state to what it was before the transaction was executed.
   */
  NS_IMETHOD Undo(void) = 0;

  /**
   * Executes the transaction again. Can only be called on a transaction that
   * was previously undone.
   * <P>
   * In most cases, the Redo() method will actually call the Do() method to
   * execute the transaction again.
   */
  NS_IMETHOD Redo(void) = 0;

  /**
   * Retrieves the transaction's transient state. This method is called by
   * the transaction manager after the transaction's Do() method is executed.
   * If the transient state is false, a reference to the transaction is
   * held by the transaction manager so that the transactions' Undo() and
   * Redo() methods can be called. If the transient state is true, the
   * transaction manager returns immediately after the transaction's Do()
   * method is executed, no references to the transaction are maintained.
   * Transient transactions cannot be undone or redone by the transaction
   * manager.
   * @param aIsTransient will contain the transaction's transient state.
   */
  NS_IMETHOD GetIsTransient(PRBool *aIsTransient) = 0;

  /**
   * Attempts to merge a transaction into "this" transaction. Both transactions
   * must be in their undo state, Do() methods already executed. The transaction
   * manager calls this method to coalesce a new transaction with the
   * transaction on the top of the undo stack.
   * @param aDidMerge will contain merge result. True if transactions were
   * merged successfully. False if merge is not possible or failed. If true,
   * the transaction manager will Release() the new transacton instead of
   * pushing it on the undo stack.
   * @param aTransaction the previously executed transaction to merge.
   */
  NS_IMETHOD Merge(PRBool *aDidMerge, nsITransaction *aTransaction) = 0;

  /**
   * Write a stream representation of the current state of the transaction.
   * @param aOutputStream the stream to write to.
   */
  NS_IMETHOD Write(nsIOutputStream *aOutputStream) = 0;

  /**
   * Returns the string to display for the undo menu item.
   * @param aString will be set to the string to display.
   */
  NS_IMETHOD GetUndoString(nsString *aString) = 0;

  /**
   * Returns the string to display for the redo menu item.
   * @param aString will be set to the string to display.
   */
  NS_IMETHOD GetRedoString(nsString *aString) = 0;
};

#endif // nsITransaction_h__

