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

#include "InsertTextTxn.h"
#include "nsEditor.h"
#include "nsIDOMCharacterData.h"
#include "nsISelection.h"
#include "EditAggregateTxn.h"

static NS_DEFINE_IID(kIDOMSelectionIID, NS_ISELECTION_IID);

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#else
static const PRBool gNoisy = PR_FALSE;
#endif

nsIAtom *InsertTextTxn::gInsertTextTxnName;

nsresult InsertTextTxn::ClassInit()
{
  if (nsnull==gInsertTextTxnName)
    gInsertTextTxnName = NS_NewAtom("NS_InsertTextTxn");
  return NS_OK;
}

nsresult InsertTextTxn::ClassShutdown()
{
  NS_IF_RELEASE(gInsertTextTxnName);
  return NS_OK;
}

InsertTextTxn::InsertTextTxn()
  : EditTxn()
{
  SetTransactionDescriptionID( kTransactionID );
  /* log description initialized in parent constructor */
}

InsertTextTxn::~InsertTextTxn()
{
}

NS_IMETHODIMP InsertTextTxn::Init(nsIDOMCharacterData *aElement,
                                  PRUint32             aOffset,
                                  const nsString      &aStringToInsert,
                                  nsIEditor           *aEditor)
{
#if 0 //def DEBUG_cmanske
      nsAutoString text;
      aElement->GetData(text);
      printf("InsertTextTxn: Offset to insert at = %d. Text of the node to insert into:\n", aOffset);
      wprintf(text.GetUnicode());
      printf("\n");
#endif

  NS_ASSERTION(aElement && aEditor, "bad args");
  if (!aElement || !aEditor) return NS_ERROR_NULL_POINTER;

  mElement = do_QueryInterface(aElement);
  mOffset = aOffset;
  mStringToInsert = aStringToInsert;
  mEditor = aEditor;
  return NS_OK;
}

NS_IMETHODIMP InsertTextTxn::Do(void)
{
  if (gNoisy) { printf("Do Insert Text element = %p\n", mElement.get()); }
  NS_ASSERTION(mElement && mEditor, "bad state");
  if (!mElement || !mEditor) { return NS_ERROR_NOT_INITIALIZED; }

  nsresult result = mElement->InsertData(mOffset, mStringToInsert);
  if (NS_FAILED(result)) return result;

  // only set selection to insertion point if editor gives permission
  PRBool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection)
  {
    nsCOMPtr<nsISelection> selection;
    result = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) return result;
    if (!selection) return NS_ERROR_NULL_POINTER;
    result = selection->Collapse(mElement, mOffset+mStringToInsert.Length());
    NS_ASSERTION((NS_SUCCEEDED(result)), "selection could not be collapsed after insert.");
  }
  else
  {
    // do nothing - dom range gravity will adjust selection
  }

  return result;
}

NS_IMETHODIMP InsertTextTxn::Undo(void)
{
  if (gNoisy) { printf("Undo Insert Text element = %p\n", mElement.get()); }
  NS_ASSERTION(mElement && mEditor, "bad state");
  if (!mElement || !mEditor) { return NS_ERROR_NOT_INITIALIZED; }

  nsresult result;
  PRUint32 length = mStringToInsert.Length();
  result = mElement->DeleteData(mOffset, length);
  return result;
}

NS_IMETHODIMP InsertTextTxn::Merge(PRBool *aDidMerge, nsITransaction *aTransaction)
{
  // set out param default value
  if (nsnull!=aDidMerge)
    *aDidMerge=PR_FALSE;
  nsresult result = NS_OK;
  if ((nsnull!=aDidMerge) && (nsnull!=aTransaction))
  {
    // if aTransaction isa InsertTextTxn, and if the selection hasn't changed, 
    // then absorb it
    InsertTextTxn *otherInsTxn = nsnull;
    aTransaction->QueryInterface(InsertTextTxn::GetCID(), (void **)&otherInsTxn);
    if (otherInsTxn)
    {
      if (PR_TRUE==IsSequentialInsert(otherInsTxn))
      {
        nsAutoString otherData;
        otherInsTxn->GetData(otherData);
        mStringToInsert += otherData;
        *aDidMerge = PR_TRUE;
        if (gNoisy) { printf("InsertTextTxn assimilated %p\n", aTransaction); }
      }
      NS_RELEASE(otherInsTxn);
    }
    else
    { // the next InsertTextTxn might be inside an aggregate that we have special knowledge of
      EditAggregateTxn *otherTxn = nsnull;
      aTransaction->QueryInterface(EditAggregateTxn::GetCID(), (void **)&otherTxn);
      if (otherTxn)
      {
        nsCOMPtr<nsIAtom> txnName;
        otherTxn->GetName(getter_AddRefs(txnName));
        if (txnName && txnName.get()==gInsertTextTxnName)
        { // yep, it's one of ours.  By definition, it must contain only
          // another aggregate with a single child,
          // or a single InsertTextTxn
          EditTxn * childTxn;
          otherTxn->GetTxnAt(0, (&childTxn));
          if (childTxn)
          {
            InsertTextTxn * otherInsertTxn = nsnull;
            result = childTxn->QueryInterface(InsertTextTxn::GetCID(), (void**)&otherInsertTxn);
            if (NS_SUCCEEDED(result))
            {
              if (otherInsertTxn)
              {
                if (PR_TRUE==IsSequentialInsert(otherInsertTxn))
	              {
	                nsAutoString otherData;
	                otherInsertTxn->GetData(otherData);
	                mStringToInsert += otherData;
	                *aDidMerge = PR_TRUE;
	                if (gNoisy) { printf("InsertTextTxn assimilated %p\n", aTransaction); }
	              }
	              NS_RELEASE(otherInsertTxn);
	            }
            }
            
            NS_RELEASE(childTxn);
          }
        }
        NS_RELEASE(otherTxn);
      }
    }
  }
  return result;
}

NS_IMETHODIMP InsertTextTxn::Write(nsIOutputStream *aOutputStream)
{
  return NS_OK;
}

NS_IMETHODIMP InsertTextTxn::GetUndoString(nsString *aString)
{
  if (nsnull!=aString)
  {
    aString->AssignWithConversion("Remove Text: ");
    *aString += mStringToInsert;
  }
  return NS_OK;
}

NS_IMETHODIMP InsertTextTxn::GetRedoString(nsString *aString)
{
  if (nsnull!=aString)
  {
    aString->AssignWithConversion("Insert Text: ");
    *aString += mStringToInsert;
  }
  return NS_OK;
}

/* ============= nsISupports implementation ====================== */

NS_IMETHODIMP
InsertTextTxn::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(InsertTextTxn::GetCID())) {
    *aInstancePtr = (void*)(InsertTextTxn*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return (EditTxn::QueryInterface(aIID, aInstancePtr));
}

/* ============ protected methods ================== */

NS_IMETHODIMP InsertTextTxn::GetData(nsString& aResult)
{
  aResult = mStringToInsert;
  return NS_OK;
}

PRBool InsertTextTxn::IsSequentialInsert(InsertTextTxn *aOtherTxn)
{
  NS_ASSERTION(nsnull!=aOtherTxn, "null param");
  PRBool result=PR_FALSE;
  if (nsnull!=aOtherTxn)
  {
    if (aOtherTxn->mElement == mElement)
    {
      // here, we need to compare offsets.
      PRInt32 length = mStringToInsert.Length();
      if (aOtherTxn->mOffset==(mOffset+length))
      {
        result = PR_TRUE;
      }
    }
  }
  return result;
}
