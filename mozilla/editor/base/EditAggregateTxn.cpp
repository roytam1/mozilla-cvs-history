/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL") you may not use this file except in
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

#include "EditAggregateTxn.h"
#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsVoidArray.h"

EditAggregateTxn::EditAggregateTxn()
  : EditTxn()
{
  NS_INIT_REFCNT();
  mChildren = new nsVoidArray();
}

EditAggregateTxn::~EditAggregateTxn()
{
  if (nsnull!=mChildren)
  {
    PRInt32 i;
    PRInt32 count = mChildren->Count();
    for (i=0; i<count; i++)
    {
      EditTxn *txn = (EditTxn*)(mChildren->ElementAt(i));
      NS_IF_RELEASE(txn);
    }
    delete mChildren;
  }
}

nsresult EditAggregateTxn::Do(void)
{
  nsresult result=NS_OK;  // it's legal (but not very useful) to have an empty child list
  if (nsnull!=mChildren)
  {
    PRInt32 i;
    PRInt32 count = mChildren->Count();
    for (i=0; i<count; i++)
    {
      EditTxn *txn = (EditTxn*)(mChildren->ElementAt(i));
      result = txn->Do();
      if (NS_FAILED(result))
        break;
    }  
  }
  return result;
}

nsresult EditAggregateTxn::Undo(void)
{
  nsresult result=NS_OK;  // it's legal (but not very useful) to have an empty child list
  if (nsnull!=mChildren)
  {
    PRInt32 i;
    PRInt32 count = mChildren->Count();
    // undo goes through children backwards
    for (i=count-1; i>=0; i--)
    {
      EditTxn *txn = (EditTxn*)(mChildren->ElementAt(i));
      result = txn->Undo();
      if (NS_FAILED(result))
        break;
    }  
  }
  return result;
}

nsresult EditAggregateTxn::Redo(void)
{
  nsresult result=NS_OK;  // it's legal (but not very useful) to have an empty child list
  if (nsnull!=mChildren)
  {
    PRInt32 i;
    PRInt32 count = mChildren->Count();
    for (i=0; i<count; i++)
    {
      EditTxn *txn = (EditTxn*)(mChildren->ElementAt(i));
      result = txn->Redo();
      if (NS_FAILED(result))
        break;
    }  
  }
  return result;
}

nsresult EditAggregateTxn::GetIsTransient(PRBool *aIsTransient)
{
  if (nsnull!=aIsTransient)
    *aIsTransient = PR_FALSE;
  return NS_OK;
}

nsresult EditAggregateTxn::Merge(PRBool *aDidMerge, nsITransaction *aTransaction)
{
  nsresult result=NS_OK;  // it's legal (but not very useful) to have an empty child list
  if (nsnull!=aDidMerge)
    *aDidMerge=PR_FALSE;
  if (nsnull!=mChildren)
  {
    PRInt32 i;
    PRInt32 count = mChildren->Count();
    EditTxn *txn = (EditTxn*)(mChildren->ElementAt(count-1));
    result = txn->Merge(aDidMerge, aTransaction);
  }
  return result;

}

nsresult EditAggregateTxn::Write(nsIOutputStream *aOutputStream)
{
  return NS_OK;
}

nsresult EditAggregateTxn::GetUndoString(nsString **aString)
{
  if (nsnull!=aString)
    *aString=nsnull;
  return NS_OK;
}

nsresult EditAggregateTxn::GetRedoString(nsString **aString)
{
  if (nsnull!=aString)
    *aString=nsnull;
  return NS_OK;
}

nsresult EditAggregateTxn::AppendChild(EditTxn *aTxn)
{
  if ((nsnull!=mChildren) && (nsnull!=aTxn))
  {
    mChildren->AppendElement(aTxn);
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

nsresult EditAggregateTxn::SetName(nsIAtom *aName)
{
  mName = do_QueryInterface(aName);
  return NS_OK;
}

nsresult EditAggregateTxn::GetName(nsIAtom **aName)
{
  if (aName)
  {
    if (mName)
    {
      *aName = mName;
      NS_ADDREF(*aName);
      return NS_OK;
    }
  }
  return NS_ERROR_NULL_POINTER;
}

nsresult EditAggregateTxn::GetCount(PRInt32 *aCount)
{
  if (!aCount) {
    return NS_ERROR_NULL_POINTER;
  }
  *aCount=0;
  if (mChildren) {
    *aCount = mChildren->Count();
  }
  return NS_OK;
}

nsresult EditAggregateTxn::GetTxnAt(PRInt32 aIndex, EditTxn **aTxn)
{
  if (!aTxn) {
    return NS_ERROR_NULL_POINTER;
  }
  if (!mChildren) {
    return NS_ERROR_UNEXPECTED;
  }
  const PRInt32 txnCount = mChildren->Count();
  if (0>aIndex || txnCount<=aIndex) {
    return NS_ERROR_UNEXPECTED;
  }
  *aTxn = (EditTxn *)(mChildren->ElementAt(aIndex));
  if (!*aTxn)
    return NS_ERROR_UNEXPECTED;
  NS_ADDREF(*aTxn);
  return NS_OK;
}





