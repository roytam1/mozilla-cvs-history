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
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef IMETextTxn_h__
#define IMETextTxn_h__

#include "EditTxn.h"
#include "nsIDOMCharacterData.h"
#include "nsIPrivateTextRange.h"
#include "nsCOMPtr.h"
#include "nsWeakPtr.h"

// {D4D25721-2813-11d3-9EA3-0060089FE59B}
#define IME_TEXT_TXN_CID							\
{0xd4d25721, 0x2813, 0x11d3,						\
{0x9e, 0xa3, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b }}




class nsIPresShell;

/**
  * A transaction that inserts text into a content node. 
  */
class IMETextTxn : public EditTxn
{
public:
  static const nsIID& GetCID() { static nsIID iid = IME_TEXT_TXN_CID; return iid; }

  virtual ~IMETextTxn();

  /** used to name aggregate transactions that consist only of a single IMETextTxn,
    * or a DeleteSelection followed by an IMETextTxn.
    */
  static nsIAtom *gIMETextTxnName;
	
  /** initialize the transaction
    * @param aElement the text content node
    * @param aOffset  the location in aElement to do the insertion
	* @param aReplaceLength the length of text to replace (0= no replacement)
    * @param aString  the new text to insert
    * @param aPresShell used to get and set the selection
    */
  NS_IMETHOD Init(nsIDOMCharacterData *aElement,
                  PRUint32 aOffset,
				  PRUint32 aReplaceLength,
				  nsIPrivateTextRangeList* aTextRangeList,
                  const nsString& aString,
                  nsWeakPtr aPresShell);

private:
	
	IMETextTxn();

public:
	
  NS_IMETHOD Do(void);

  NS_IMETHOD Undo(void);

  NS_IMETHOD Merge(PRBool *aDidMerge, nsITransaction *aTransaction);

  NS_IMETHOD Write(nsIOutputStream *aOutputStream);

  NS_IMETHOD GetUndoString(nsString *aString);

  NS_IMETHOD GetRedoString(nsString *aString);

// nsISupports declarations

  // override QueryInterface to handle IMETextTxn request
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  /** return the string data associated with this transaction */
  NS_IMETHOD GetData(nsString& aResult, nsIPrivateTextRangeList** aTextRangeList);

  /** must be called before any IMETextTxn is instantiated */
  static nsresult ClassInit();

  /** must be called once we are guaranteed all IMETextTxn have completed */
  static nsresult ClassShutdown();

  enum { kTransactionID = 11220 };

protected:

  NS_IMETHOD CollapseTextSelection(void);

  /** the text element to operate upon */
  nsCOMPtr<nsIDOMCharacterData> mElement;
  
  /** the offsets into mElement where the insertion should be placed*/
  PRUint32 mOffset;

  PRUint32 mReplaceLength;

  /** the text to insert into mElement at mOffset */
  nsString mStringToInsert;

  /** the range list **/
  nsCOMPtr<nsIPrivateTextRangeList>	mRangeList;

  /** the presentation shell, which we'll need to get the selection */
  nsWeakPtr mPresShellWeak;  // use a weak reference

  PRBool	mFixed;

  friend class TransactionFactory;

  friend class nsDerivedSafe<IMETextTxn>; // work around for a compiler bug

};

#endif
