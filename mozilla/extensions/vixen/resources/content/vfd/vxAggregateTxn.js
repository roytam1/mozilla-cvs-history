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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Ben Goodger <ben@netscape.com> (Original Author)
 */

/** 
 * vxAggregateTxn - Transaction that allows individual transactions to be
 *                  coalesced and done/undone in one operation
 * 
 * e.g., create a new element in the VFD document using the palette that
 *       has several attributes set on it.
 */

function vxAggregateTxn (aTransactionList)
{
  this.mTransactionList = { };
  for (var i = 0; i < aTransactionList.length; i++)
    this.mTransactionList[aTransactionList[i].mID] = aTransactionList[i];
  
  this.mID = "aggregate-txn::";
} 
 
vxAggregateTxn.prototype = {
  __proto__: vxBaseTxn.prototype,
  
  doTransaction: function ()
  {
    var irq = { };
    this.notifyListeners("willDo", this, irq);

    for (var i in this.mTransactionList)
      this.mTransactionList[i].doTransaction();
    
    this.notifyListeners("didDo", this, irq);
  },

  undoTransaction: function ()
  {
    var irq = { };
    this.notifyListeners("willUndo", this, irq);

    for (var i in this.mTransactionList)
      this.mTransactionList[i].undoTransaction();

    this.notifyListeners("didUndo", this, irq);
  },
  
  redoTransaction: function ()
  {
    var irq = { };
    this.notifyListeners("willRedo", this, irq);

    for (var i in this.mTransactionList)
      this.mTransactionList[i].redoTransaction();
    
    this.notifyListeners("didRedo", this, irq);
  },

  get commandString()
  {
    var commandString = "aggregate-txn::{ mTransactions: [";
    for (var txn in this.mTransactionList)
      commandString += this.mTransactionList[txn].commandString + ",";
    commandString += "] };";
    return commandString;    
  }
};

/** 
 * txnStack["create-element"]("button", someBox, 3);
 **/
 
 
 
 