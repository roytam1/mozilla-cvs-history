/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include <string.h>
#include "nsSupportsArray.h"
#include "nsSupportsArrayEnumerator.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"

static const PRUint32 kGrowArrayBy = 8;

nsSupportsArray::nsSupportsArray()
{
  NS_INIT_REFCNT();
  mArray = mAutoArray;
  mArraySize = kAutoArraySize;
  mCount = 0;
}

nsSupportsArray::~nsSupportsArray()
{
  DeleteArray();
}

NS_METHOD
nsSupportsArray::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsSupportsArray *it = new nsSupportsArray();
  if (it == NULL)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(it);
  nsresult rv = it->QueryInterface(aIID, aResult);
  NS_RELEASE(it);
  return rv;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSupportsArray, nsISupportsArray)

NS_IMETHODIMP
nsSupportsArray::Read(nsIObjectInputStream *aStream)
{
  nsresult rv;

  rv = aStream->Read32(&mArraySize);
  if (mArraySize > kAutoArraySize) {
    nsISupports** array = new nsISupports*[mArraySize];
    if (!array)
      return NS_ERROR_OUT_OF_MEMORY;
    mArray = array;
  }
  else {
    NS_ASSERTION(mArraySize == kAutoArraySize, "bad default mArraySize!");
    NS_ASSERTION(mArray == mAutoArray, "bad default mArray!");
  }

  rv = aStream->Read32(&mCount);
  if (NS_FAILED(rv)) return rv;

  for (PRUint32 i = 0; i < mCount; i++) {
    rv = aStream->ReadObject(PR_TRUE, &mArray[i]);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArray::Write(nsIObjectOutputStream *aStream)
{
  nsresult rv;

  rv = aStream->Write32(mArraySize);
  if (NS_FAILED(rv)) return rv;

  rv = aStream->Write32(mCount);
  if (NS_FAILED(rv)) return rv;

  for (PRUint32 i = 0; i < mCount; i++) {
    rv = aStream->WriteObject(mArray[i], PR_TRUE);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArray::GetCID(nsCID *aResult)
{
  nsCID myCID = NS_SUPPORTSARRAY_CID;

  *aResult = myCID;
  return NS_OK;
}

void nsSupportsArray::DeleteArray(void)
{
  Clear();
  if (mArray != &(mAutoArray[0])) {
    delete[] mArray;
    mArray = mAutoArray;
    mArraySize = kAutoArraySize;
  }
}

#if 0
NS_IMETHODIMP_(nsISupportsArray&) 
nsISupportsArray::operator=(const nsISupportsArray& other)
{
  NS_ASSERTION(0, "should be an abstract method");
  return *this; // bogus
}
#endif

NS_IMETHODIMP_(nsISupportsArray&) 
nsSupportsArray::operator=(nsISupportsArray const& aOther)
{
  PRUint32 otherCount = 0;
  nsresult rv = ((nsISupportsArray&)aOther).Count(&otherCount);
  NS_ASSERTION(NS_SUCCEEDED(rv), "this method should return an error!");
  if (NS_FAILED(rv)) return *this;
  
  if (otherCount > mArraySize) {
    DeleteArray();
    nsISupports** array = new nsISupports*[otherCount];
    if (!array) {
      // Can't grow mArray, can't return an error -- copy mCount elements.
      otherCount = mCount;
    }
    else {
      mArraySize = otherCount;
      mArray = array;
    }
  }
  else {
    Clear();
  }
  mCount = otherCount;
  while (0 < otherCount--) {
    mArray[otherCount] = ((nsISupportsArray&)aOther).ElementAt(otherCount);
  }
  return *this;
}

NS_IMETHODIMP_(PRBool)
nsSupportsArray::Equals(const nsISupportsArray* aOther)
{
  if (0 != aOther) {
    const nsSupportsArray* other = (const nsSupportsArray*)aOther;
    if (mCount == other->mCount) {
      PRUint32 aIndex = mCount;
      while (0 < aIndex--) {
        if (mArray[aIndex] != other->mArray[aIndex]) {
          return PR_FALSE;
        }
      }
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

NS_IMETHODIMP_(nsISupports*)
nsSupportsArray::ElementAt(PRUint32 aIndex)
{
  if (aIndex < mCount) {
    nsISupports*  element = mArray[aIndex];
    NS_ADDREF(element);
    return element;
  }
  return 0;
}

NS_IMETHODIMP_(PRInt32)
nsSupportsArray::IndexOf(const nsISupports* aPossibleElement)
{
  return IndexOfStartingAt(aPossibleElement, 0);
}
  

NS_IMETHODIMP_(PRInt32)
nsSupportsArray::IndexOfStartingAt(const nsISupports* aPossibleElement,
                                   PRUint32 aStartIndex)
{
  if (aStartIndex < mCount) {
    const nsISupports** start = (const nsISupports**)mArray;  // work around goofy compiler behavior
    const nsISupports** ep = (start + aStartIndex);
    const nsISupports** end = (start + mCount);
    while (ep < end) {
      if (aPossibleElement == *ep) {
        return (ep - start);
      }
      ep++;
    }
  }
  return -1;
}

NS_IMETHODIMP_(PRInt32)
nsSupportsArray::LastIndexOf(const nsISupports* aPossibleElement)
{
  if (0 < mCount) {
    const nsISupports** start = (const nsISupports**)mArray;  // work around goofy compiler behavior
    const nsISupports** ep = (start + mCount);
    while (start <= --ep) {
      if (aPossibleElement == *ep) {
        return (ep - start);
      }
    }
  }
  return -1;
}

NS_IMETHODIMP_(PRBool)
nsSupportsArray::InsertElementAt(nsISupports* aElement, PRUint32 aIndex)
{
  if (!aElement) {
    return PR_FALSE;
  }
  if (aIndex <= mCount) {
    if (mArraySize < (mCount + 1)) {    // need to grow the array
      mArraySize += kGrowArrayBy;
      nsISupports** oldArray = mArray;
      mArray = new nsISupports*[mArraySize];
      if (!mArray) {                    // ran out of memory
        mArray = oldArray;
        mArraySize -= kGrowArrayBy;
        return PR_FALSE;
      }
      if (oldArray) {                   // need to move old data
        if (0 < aIndex) {
          ::memcpy(mArray, oldArray, aIndex * sizeof(nsISupports*));
        }
        PRUint32 slide = (mCount - aIndex);
        if (0 < slide) {
          ::memcpy(mArray + aIndex + 1, oldArray + aIndex, slide * sizeof(nsISupports*));
        }
        if (oldArray != &(mAutoArray[0])) {
          delete[] oldArray;
        }
      }
    }
    else {
      PRUint32 slide = (mCount - aIndex);
      if (0 < slide) {
        ::memmove(mArray + aIndex + 1, mArray + aIndex, slide * sizeof(nsISupports*));
      }
    }

    mArray[aIndex] = aElement;
    NS_ADDREF(aElement);
    mCount++;
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsSupportsArray::ReplaceElementAt(nsISupports* aElement, PRUint32 aIndex)
{
  if (aIndex < mCount) {
    NS_ADDREF(aElement);  // addref first in case it's the same object!
    NS_RELEASE(mArray[aIndex]);
    mArray[aIndex] = aElement;
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsSupportsArray::RemoveElementAt(PRUint32 aIndex)
{
  if (aIndex < mCount) {
    NS_RELEASE(mArray[aIndex]);
    mCount--;
    PRInt32 slide = (mCount - aIndex);
    if (0 < slide) {
      ::memmove(mArray + aIndex, mArray + aIndex + 1,
                slide * sizeof(nsISupports*));
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsSupportsArray::RemoveElement(const nsISupports* aElement, PRUint32 aStartIndex)
{
  if (aStartIndex < mCount) {
    nsISupports** ep = mArray;
    nsISupports** end = ep + mCount;
    while (ep < end) {
      if (*ep == aElement) {
        return RemoveElementAt(PRUint32(ep - mArray));
      }
      ep++;
    }
  }
  return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsSupportsArray::RemoveLastElement(const nsISupports* aElement)
{
  if (0 < mCount) {
    nsISupports** ep = (mArray + mCount);
    while (mArray <= --ep) {
      if (*ep == aElement) {
        return RemoveElementAt(PRUint32(ep - mArray));
      }
    }
  }
  return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsSupportsArray::AppendElements(nsISupportsArray* aElements)
{
  nsSupportsArray*  elements = (nsSupportsArray*)aElements;

  if (elements && (0 < elements->mCount)) {
    if (mArraySize < (mCount + elements->mCount)) { // need to grow the array
      PRUint32 count = mCount + elements->mCount;
      PRUint32 oldSize = mArraySize;
      while (mArraySize < count) {  // ick
        mArraySize += kGrowArrayBy;
      }
      nsISupports** oldArray = mArray;
      mArray = new nsISupports*[mArraySize];
      if (!mArray) {                                // ran out of memory
        mArray = oldArray;
        mArraySize = oldSize;
        return PR_FALSE;
      }
      if (oldArray) {                               // need to move old data
        if (0 < mCount) {
          ::memcpy(mArray, oldArray, mCount * sizeof(nsISupports*));
        }
        if (oldArray != &(mAutoArray[0])) {
          delete[] oldArray;
        }
      }
    }

    PRUint32 aIndex = 0;
    while (aIndex < elements->mCount) {
      NS_ADDREF(elements->mArray[aIndex]);
      mArray[mCount++] = elements->mArray[aIndex++];
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsSupportsArray::Clear(void)
{
  if (0 < mCount) {
    do {
      --mCount;
      NS_RELEASE(mArray[mCount]);
    } while (0 != mCount);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArray::Compact(void)
{
  if ((mArraySize != mCount) && (kAutoArraySize < mArraySize)) {
    nsISupports** oldArray = mArray;
    if (mCount <= kAutoArraySize) {
      mArray = mAutoArray;
      mArraySize = kAutoArraySize;
    }
    else {
      mArray = new nsISupports*[mCount];
      if (!mArray) {
        mArray = oldArray;
        return NS_OK;
      }
      mArraySize = mCount;
    }
    ::memcpy(mArray, oldArray, mCount * sizeof(nsISupports*));
    delete[] oldArray;
  }
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsSupportsArray::EnumerateForwards(nsISupportsArrayEnumFunc aFunc, void* aData)
{
  PRInt32 aIndex = -1;
  PRBool  running = PR_TRUE;

  while (running && (++aIndex < (PRInt32)mCount)) {
    running = (*aFunc)(mArray[aIndex], aData);
  }
  return running;
}

NS_IMETHODIMP_(PRBool)
nsSupportsArray::EnumerateBackwards(nsISupportsArrayEnumFunc aFunc, void* aData)
{
  PRUint32 aIndex = mCount;
  PRBool  running = PR_TRUE;

  while (running && (0 < aIndex--)) {
    running = (*aFunc)(mArray[aIndex], aData);
  }
  return running;
}

NS_IMETHODIMP
nsSupportsArray::Enumerate(nsIEnumerator* *result)
{
  nsSupportsArrayEnumerator* e = new nsSupportsArrayEnumerator(this);
  if (!e)
    return NS_ERROR_OUT_OF_MEMORY;
  *result = e;
  NS_ADDREF(e);
  return NS_OK;
}

static PRBool
CopyElement(nsISupports* aElement, void *aData)
{
  nsresult rv;
  nsISupportsArray* newArray = (nsISupportsArray*)aData;
  rv = newArray->AppendElement(aElement);
  return NS_SUCCEEDED(rv);
}

NS_IMETHODIMP
nsSupportsArray::Clone(nsISupportsArray* *result)
{
  nsresult rv;
  nsISupportsArray* newArray;
  rv = NS_NewISupportsArray(&newArray);
  PRBool ok = EnumerateForwards(CopyElement, newArray);
  if (!ok) return NS_ERROR_OUT_OF_MEMORY;
  *result = newArray;
  return NS_OK;
}

NS_COM nsresult
NS_NewISupportsArray(nsISupportsArray** aInstancePtrResult)
{
  nsresult rv;
  rv = nsSupportsArray::Create(NULL, NS_GET_IID(nsISupportsArray),
                               (void**)aInstancePtrResult);
  return rv;
}

