/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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

#define NS_WEAK_OBSERVERS

#define NS_IMPL_IDS
#include "pratom.h"
#include "nsIObserverList.h"
#include "nsObserverList.h"
#include "nsString.h"
#include "nsAutoLock.h"
#include "nsCOMPtr.h"
#include "nsIWeakReference.h"


#define NS_AUTOLOCK(__monitor) nsAutoLock __lock(__monitor)

static NS_DEFINE_CID(kObserverListCID, NS_OBSERVERLIST_CID);


class nsObserverListEnumerator : public nsIBidirectionalEnumerator
	{
		public:
			nsObserverListEnumerator( nsISupportsArray* );
			virtual ~nsObserverListEnumerator() {};
			NS_DECL_ISUPPORTS
			NS_DECL_NSIENUMERATOR
			NS_DECL_NSIBIDIRECTIONALENUMERATOR

		private:
			PRUint32 GetTargetArraySize() const;
			nsresult MoveToIndex( PRUint32 );

		private:
			nsCOMPtr<nsISupportsArray>	mTargetArray;
			PRUint32										mCurrentItemIndex;
	};

NS_IMPL_ISUPPORTS2(nsObserverListEnumerator,
                   nsIBidirectionalEnumerator,
                   nsIEnumerator)

nsObserverListEnumerator::nsObserverListEnumerator( nsISupportsArray* anArray )
		: mRefCnt(0),
			mTargetArray(anArray),
			mCurrentItemIndex(0)
	{
		NS_INIT_REFCNT();
	}

PRUint32
nsObserverListEnumerator::GetTargetArraySize() const
	{
		PRUint32 array_size = 0;
		mTargetArray->Count(&array_size);
		return array_size;
	}

nsresult
nsObserverListEnumerator::MoveToIndex( PRUint32 aNewIndex )
	{
		mCurrentItemIndex = aNewIndex;
		return (aNewIndex < GetTargetArraySize()) ? NS_OK : NS_ERROR_FAILURE;
	}


NS_IMETHODIMP
nsObserverListEnumerator::First()
	{
		NS_ASSERTION(mTargetArray, "must enumerate over an non-null list");

		return MoveToIndex(0);
	}

NS_IMETHODIMP
nsObserverListEnumerator::Last()
	{
		NS_ASSERTION(mTargetArray, "must enumerate over an non-null list");

		return MoveToIndex(GetTargetArraySize()-1);
	}

NS_IMETHODIMP
nsObserverListEnumerator::Prev()
	{
		NS_ASSERTION(mTargetArray, "must enumerate over an non-null list");

		return MoveToIndex(mCurrentItemIndex-1);
	}

NS_IMETHODIMP
nsObserverListEnumerator::Next()
	{
		NS_ASSERTION(mTargetArray, "must enumerate over an non-null list");

		return MoveToIndex(mCurrentItemIndex+1);
	}

NS_IMETHODIMP
nsObserverListEnumerator::CurrentItem( nsISupports** aItemPtr )
	{
		NS_ASSERTION(mTargetArray, "must enumerate over an non-null list");
		NS_ASSERTION(aItemPtr, "must supply a place to put the result");

		if ( mCurrentItemIndex >= GetTargetArraySize() )
			return NS_ERROR_FAILURE;

		nsCOMPtr<nsISupports> elementPtr = getter_AddRefs(mTargetArray->ElementAt(mCurrentItemIndex));

		if ( !aItemPtr || !elementPtr )
			return NS_ERROR_NULL_POINTER;

		nsCOMPtr<nsIWeakReference> weakRef = do_QueryInterface(elementPtr);

    nsresult status;
    if ( weakRef )
      status = weakRef->QueryReferent(NS_GET_IID(nsIObserver), NS_REINTERPRET_CAST(void**, aItemPtr));
    else
      status = elementPtr->QueryInterface(NS_GET_IID(nsIObserver), NS_REINTERPRET_CAST(void**, aItemPtr));

    return status;
	}

NS_IMETHODIMP
nsObserverListEnumerator::IsDone()
	{
		NS_ASSERTION(mTargetArray, "must enumerate over an non-null list");

		return  (mCurrentItemIndex >= GetTargetArraySize()) ? NS_OK : NS_ENUMERATOR_FALSE;
	}



////////////////////////////////////////////////////////////////////////////////
// nsObserverList Implementation


NS_IMPL_ISUPPORTS1(nsObserverList, nsIObserverList)

NS_COM nsresult NS_NewObserverList(nsIObserverList** anObserverList)
{

    if (anObserverList == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }  
    nsObserverList* it = new nsObserverList();

    if (it == 0) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    return it->QueryInterface(NS_GET_IID(nsIObserverList), (void **) anObserverList);
}

nsObserverList::nsObserverList()
    : mLock(nsnull),
        mObserverList(NULL)
{
    NS_INIT_REFCNT();
    mLock = PR_NewLock();
}

nsObserverList::~nsObserverList(void)
{
    PR_DestroyLock(mLock);
    NS_IF_RELEASE(mObserverList);
}


nsresult nsObserverList::AddObserver(nsIObserver** anObserver)
{
	nsresult rv;
	PRBool inserted;
    
	NS_AUTOLOCK(mLock);

    if (anObserver == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }  

	if(!mObserverList) {
        rv = NS_NewISupportsArray(&mObserverList);
		if (NS_FAILED(rv)) return rv;
    }

#ifdef NS_WEAK_OBSERVERS
  nsCOMPtr<nsISupportsWeakReference> weakRefFactory = do_QueryInterface(*anObserver);
  nsCOMPtr<nsISupports> observerRef;
  if ( weakRefFactory )
  	observerRef = getter_AddRefs(NS_STATIC_CAST(nsISupports*, NS_GetWeakReference(weakRefFactory)));
  else
  	observerRef = *anObserver;

	if(observerRef) {
		inserted = mObserverList->AppendElement(observerRef); 
#else
	if(*anObserver) {
		inserted = mObserverList->AppendElement(*anObserver);
#endif
		return inserted ? NS_OK : NS_ERROR_FAILURE;
    }

	return NS_ERROR_FAILURE;
}

nsresult nsObserverList::RemoveObserver(nsIObserver** anObserver)
{
	PRBool removed;

 	NS_AUTOLOCK(mLock);

    if (anObserver == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }  

    if(!mObserverList) {
        return NS_ERROR_FAILURE;
    }

#ifdef NS_WEAK_OBSERVERS
  nsCOMPtr<nsISupportsWeakReference> weakRefFactory = do_QueryInterface(*anObserver);
  nsCOMPtr<nsISupports> observerRef;
  if ( weakRefFactory )
  	observerRef = getter_AddRefs(NS_STATIC_CAST(nsISupports*, NS_GetWeakReference(weakRefFactory)));
  else
  	observerRef = *anObserver;

	if(observerRef) {
		removed = mObserverList->RemoveElement(observerRef);  
#else
	if(*anObserver) {
		removed = mObserverList->RemoveElement(*anObserver);
#endif
		return removed ? NS_OK : NS_ERROR_FAILURE;
    }

    return NS_ERROR_FAILURE;

}

NS_IMETHODIMP nsObserverList::EnumerateObserverList(nsIEnumerator** anEnumerator)
{
	NS_AUTOLOCK(mLock);

    if (anEnumerator == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }

    if(!mObserverList) {
        return NS_ERROR_FAILURE;
    }

#ifdef NS_WEAK_OBSERVERS
	nsCOMPtr<nsIBidirectionalEnumerator> enumerator = new nsObserverListEnumerator(mObserverList);
	if ( !enumerator )
		return NS_ERROR_OUT_OF_MEMORY;

	return CallQueryInterface(enumerator, anEnumerator);
#else
	return mObserverList->Enumerate(anEnumerator);
#endif
}

