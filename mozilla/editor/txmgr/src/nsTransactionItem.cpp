/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsITransaction.h"
#include "nsITransactionManager.h"
#include "nsTransactionStack.h"
#include "nsTransactionManager.h"
#include "nsTransactionItem.h"
#include "nsCOMPtr.h"

nsTransactionItem::nsTransactionItem(nsITransaction *aTransaction)
    : mTransaction(aTransaction), mUndoStack(0), mRedoStack(0)
{
}

nsTransactionItem::~nsTransactionItem()
{
  if (mRedoStack)
    delete mRedoStack;

  if (mUndoStack)
    delete mUndoStack;

  NS_IF_RELEASE(mTransaction);
}

nsresult
nsTransactionItem::AddChild(nsTransactionItem *aTransactionItem)
{
  if (!aTransactionItem)
    return NS_ERROR_NULL_POINTER;

  if (!mUndoStack) {
    mUndoStack = new nsTransactionStack();
    if (!mUndoStack)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  mUndoStack->Push(aTransactionItem);

  return NS_OK;
}

nsresult
nsTransactionItem::GetTransaction(nsITransaction **aTransaction)
{
  if (!aTransaction)
    return NS_ERROR_NULL_POINTER;

  *aTransaction = mTransaction;

  return NS_OK;
}

nsresult
nsTransactionItem::GetNumberOfChildren(PRInt32 *aNumChildren)
{
  nsresult result;

  if (!aNumChildren)
    return NS_ERROR_NULL_POINTER;

  *aNumChildren = 0;

  PRInt32 ui = 0;
  PRInt32 ri = 0;

  result = GetNumberOfUndoItems(&ui);

  if (NS_FAILED(result))
    return result;

  result = GetNumberOfRedoItems(&ri);

  if (NS_FAILED(result))
    return result;

  *aNumChildren = ui + ri;

  return NS_OK;
}

nsresult
nsTransactionItem::Do()
{
  if (mTransaction)
    return mTransaction->Do();
  return NS_OK;
}

nsresult
nsTransactionItem::Undo(nsTransactionManager *aTxMgr)
{
  nsresult result = UndoChildren(aTxMgr);

  if (NS_FAILED(result)) {
    RecoverFromUndoError(aTxMgr);
    return result;
  }

  if (!mTransaction)
    return NS_OK;

  result = mTransaction->Undo();

  if (NS_FAILED(result)) {
    RecoverFromUndoError(aTxMgr);
    return result;
  }

  return NS_OK;
}

nsresult
nsTransactionItem::UndoChildren(nsTransactionManager *aTxMgr)
{
  nsTransactionItem *item;
  nsresult result = NS_OK;
  PRInt32 sz = 0;

  if (mUndoStack) {
    if (!mRedoStack && mUndoStack) {
      mRedoStack = new nsTransactionRedoStack();
      if (!mRedoStack)
        return NS_ERROR_OUT_OF_MEMORY;
    }

    /* Undo all of the transaction items children! */
    result = mUndoStack->GetSize(&sz);

    if (NS_FAILED(result))
      return result;

    while (sz-- > 0) {
      result = mUndoStack->Peek(&item);

      if (NS_FAILED(result)) {
        return result;
      }

      nsITransaction *t = 0;

      result = item->GetTransaction(&t);

      if (NS_FAILED(result)) {
        return result;
      }

      result = aTxMgr->WillUndoNotify(t);

      if (NS_FAILED(result)) {
        return result;
      }

      if (result == NS_COMFALSE) {
        return NS_OK;
      }

      result = item->Undo(aTxMgr);

      if (NS_SUCCEEDED(result)) {
        result = mUndoStack->Pop(&item);

        if (NS_SUCCEEDED(result)) {
          result = mRedoStack->Push(item);

          /* XXX: If we got an error here, I doubt we can recover!
           * XXX: Should we just push the item back on the undo stack?
           */
        }
      }

      nsresult result2 = aTxMgr->DidUndoNotify(t, result);

      if (NS_SUCCEEDED(result)) {
        result = result2;
      }
    }
  }

  return result;
}

nsresult
nsTransactionItem::Redo(nsTransactionManager *aTxMgr)
{
  nsresult result;

  if (mTransaction) {
    result = mTransaction->Redo();

    if (NS_FAILED(result))
      return result;
  }

  result = RedoChildren(aTxMgr);

  if (NS_FAILED(result)) {
    RecoverFromRedoError(aTxMgr);
    return result;
  }

  return NS_OK;
}

nsresult
nsTransactionItem::RedoChildren(nsTransactionManager *aTxMgr)
{
  nsTransactionItem *item;
  nsresult result = NS_OK;
  PRInt32 sz = 0;

  if (!mRedoStack)
    return NS_OK;

  /* Redo all of the transaction items children! */
  result = mRedoStack->GetSize(&sz);

  if (NS_FAILED(result))
    return result;


  while (sz-- > 0) {
    result = mRedoStack->Peek(&item);

    if (NS_FAILED(result)) {
      return result;
    }

    nsITransaction *t = 0;

    result = item->GetTransaction(&t);

    if (NS_FAILED(result)) {
      return result;
    }

    result = aTxMgr->WillRedoNotify(t);

    if (NS_FAILED(result)) {
      return result;
    }

    if (result == NS_COMFALSE) {
      return NS_OK;
    }

    result = item->Redo(aTxMgr);

    if (NS_SUCCEEDED(result)) {
      result = mRedoStack->Pop(&item);

      if (NS_SUCCEEDED(result)) {
        result = mUndoStack->Push(item);

        // XXX: If we got an error here, I doubt we can recover!
        // XXX: Should we just push the item back on the redo stack?
      }
    }

    nsresult result2 = aTxMgr->DidUndoNotify(t, result);

    if (NS_SUCCEEDED(result)) {
      result = result2;
    }
  }

  return result;
}

nsresult
nsTransactionItem::GetNumberOfUndoItems(PRInt32 *aNumItems)
{
  if (!aNumItems)
    return NS_ERROR_NULL_POINTER;

  if (!mUndoStack) {
    *aNumItems = 0;
    return NS_OK;
  }

  return mUndoStack->GetSize(aNumItems);
}

nsresult
nsTransactionItem::GetNumberOfRedoItems(PRInt32 *aNumItems)
{
  if (!aNumItems)
    return NS_ERROR_NULL_POINTER;

  if (!mRedoStack) {
    *aNumItems = 0;
    return NS_OK;
  }

  return mRedoStack->GetSize(aNumItems);
}

nsresult
nsTransactionItem::Write(nsIOutputStream *aOutputStream)
{
  PRUint32 len;

  if (mTransaction)
    mTransaction->Write(aOutputStream);

  aOutputStream->Write("    ItemUndoStack:\n", 19, &len);
  if (mUndoStack) {
    mUndoStack->Write(aOutputStream);
  }

  aOutputStream->Write("\n    ItemRedoStack:\n", 20, &len);
  if (mRedoStack) {
    mRedoStack->Write(aOutputStream);
  }
  aOutputStream->Write("\n", 1, &len);

  return NS_OK;
}

nsresult
nsTransactionItem::RecoverFromUndoError(nsTransactionManager *aTxMgr)
{
  //
  // If this method gets called, we never got to the point where we
  // successfully called Undo() for the transaction item itself.
  // Just redo any children that successfully called undo!
  //
  return RedoChildren(aTxMgr);
}

nsresult
nsTransactionItem::RecoverFromRedoError(nsTransactionManager *aTxMgr)
{
  //
  // If this method gets called, we already successfully called Redo()
  // for the transaction item itself. Undo all the children that successfully
  // called Redo(), then undo the transaction item itself.
  //

  nsresult result;

  result = UndoChildren(aTxMgr);

  if (NS_FAILED(result)) {
    return result;
  }

  if (!mTransaction)
    return NS_OK;

  return mTransaction->Undo();
}

