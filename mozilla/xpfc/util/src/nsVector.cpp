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

#include "nsVector.h"
#include "nsVectorIterator.h"
#include "nsxpfcCIID.h"
#include "nsxpfcutil.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kCVectorCID, NS_VECTOR_CID);

nsVector :: nsVector()
{
  NS_INIT_REFCNT();

  mVoidArray = nsnull ;
}

nsVector :: ~nsVector()
{
  DeleteIfObject(mVoidArray) ;
}

NS_IMPL_QUERY_INTERFACE(nsVector, kCVectorCID)
NS_IMPL_ADDREF(nsVector)
NS_IMPL_RELEASE(nsVector)

nsresult nsVector :: Init()
{
  mVoidArray = NewObject(nsVoidArray) ;

  return NS_OK ;
}

PRUint32 nsVector :: Count()
{
  return mVoidArray->Count() ;
}

PRBool nsVector :: Empty()
{
  return (Count() ? PR_TRUE : PR_FALSE) ;
}

PRBool nsVector :: Contains(nsComponent aComponent)
{
  if (IndexOf(aComponent))
    return PR_TRUE ;
  else
    return PR_FALSE ;
}

PRUint32 nsVector :: IndexOf(nsComponent aComponent)
{
  return (mVoidArray->IndexOf(aComponent)) ;
}

nsComponent nsVector :: ElementAt(PRUint32 aIndex)
{
  return (mVoidArray->ElementAt(aIndex)) ; ;
}

/*
 *  Inserts an element into the list in a sorted fashion.
 *  @param newElement   the element to insert
 *  @param compare      the sorting function
 *  @param bAllowDups   if TRUE-> insert duplicates into the list, if FALSE-> don't insert duplicates in the list
 *  @returns  -1 means that the element existed and that dupliates were not allowed, so it was not inserted.
 *             0 means the element was inserted, no problems
 *             1 means the element was not inserted because of an internal error
 */
PRInt32 nsVector::InsertBinary(nsComponent newElement, nsVectorCompareProc aCompareFn, PRBool bAllowDups)
{
	PRInt32 iCurrent = 0;
	PRInt32 iLeft = 0;
	PRInt32 iRight = Count() - 1;
	PRInt32 iCompare = 0;
	
	while (iLeft <= iRight)
	{
		iCurrent = (iLeft + iRight) / 2;
		void* pCurrent = ElementAt(iCurrent);
        iCompare = aCompareFn(&pCurrent, &newElement);
		if (iCompare == 0)
        {
            if (0 == bAllowDups)
                return -1;
            else
			    break;
        }
		else if (iCompare > 0)
			iRight = iCurrent - 1;
		else
			iLeft = iCurrent + 1;
	}
	
	if (iCompare < 0)
		iCurrent += 1;
	if (PR_TRUE == mVoidArray->InsertElementAt(newElement, iCurrent))
        return 0;
    return 1;
}

nsresult nsVector :: Insert(PRUint32 aIndex, nsComponent aComponent)
{
  nsresult res = NS_OK;

  if (PR_TRUE == mVoidArray->InsertElementAt(aComponent, aIndex))
    return NS_OK ;

  // XXX What to return here
  return res ;
}

nsresult nsVector :: Append(nsComponent aComponent)
{
  nsresult res = NS_OK;

  if (PR_TRUE == mVoidArray->AppendElement(aComponent))
    return NS_OK ;

  // XXX What to return here
  return res ;
}

nsresult nsVector :: Remove(nsComponent aComponent)
{
  nsresult res = NS_OK;

  if (PR_TRUE == mVoidArray->RemoveElement(aComponent))
    return NS_OK ;

  // XXX What to return here
  return res ;
}

nsresult nsVector :: RemoveAll()
{
  mVoidArray->Clear();

  return NS_OK ;
}

nsresult nsVector :: RemoveAt(PRUint32 aIndex)
{
  nsresult res = NS_OK;

  if (PR_TRUE == mVoidArray->RemoveElementAt(aIndex))
    return NS_OK ;

  // XXX What to return here
  return res ;
}

nsresult nsVector :: CreateIterator(nsIIterator ** aIterator)
{
  static NS_DEFINE_IID(kCVectorIteratorCID, NS_VECTOR_ITERATOR_CID);

  nsresult res ;

#ifdef NS_WIN32
  #define XPFC_DLL "xpfc10.dll"
#else
  #define XPFC_DLL "libxpfc10.so"
#endif

  nsRepository::RegisterFactory(kCVectorIteratorCID, XPFC_DLL, PR_FALSE, PR_FALSE);

  nsVectorIterator * aVectorIterator ;

  *aIterator = nsnull;

  res = nsRepository::CreateInstance(kCVectorIteratorCID, 
                                     nsnull, 
                                     kCVectorIteratorCID, 
                                     (void **)&aVectorIterator);

  if (NS_OK != res)
    return res ;

  aVectorIterator->Init(this);

  *aIterator = (nsIIterator *) aVectorIterator;

  return res ;
}

