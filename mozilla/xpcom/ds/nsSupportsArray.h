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
 */

#ifndef nsSupportsArray_h__
#define nsSupportsArray_h__

#include "nsISupportsArray.h"

static const PRUint32 kAutoArraySize = 4;

class NS_COM nsSupportsArray : public nsISupportsArray {
public:
  nsSupportsArray(void);
  virtual ~nsSupportsArray(void);

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  NS_DECL_ISUPPORTS

  // nsICollection methods:
  NS_IMETHOD Count(PRUint32 *result) { *result = mCount; return NS_OK; }
  NS_IMETHOD GetElementAt(PRUint32 aIndex, nsISupports* *result) {
    *result = ElementAt(aIndex);
    return NS_OK;
  }
  NS_IMETHOD QueryElementAt(PRUint32 aIndex, const nsIID & aIID, void * *aResult) {
    if (aIndex < mCount) {
      nsISupports* element = mArray[aIndex];
      if (nsnull != element)
        return element->QueryInterface(aIID, aResult);
    }
    return NS_ERROR_FAILURE;
  }
  NS_IMETHOD SetElementAt(PRUint32 aIndex, nsISupports* value) {
    PRBool ok = ReplaceElementAt(value, aIndex);
    return ok ? NS_OK : NS_ERROR_FAILURE;
  }
  NS_IMETHOD AppendElement(nsISupports *aElement) {
    // XXX This incorrectly returns a PRBool instead of an nsresult.
    return InsertElementAt(aElement, mCount);
  }
  NS_IMETHOD RemoveElement(nsISupports *aElement) {
    // XXX This incorrectly returns a PRBool instead of an nsresult.
    return RemoveElement(aElement, 0);
  }
  NS_IMETHOD Enumerate(nsIEnumerator* *result);
  NS_IMETHOD Clear(void);

  // nsISupportsArray methods:
  NS_IMETHOD_(PRBool) Equals(const nsISupportsArray* aOther);

  NS_IMETHOD_(nsISupports*) ElementAt(PRUint32 aIndex);

  NS_IMETHOD_(PRInt32) IndexOf(const nsISupports* aPossibleElement);
  NS_IMETHOD_(PRInt32) IndexOfStartingAt(const nsISupports* aPossibleElement,
                                         PRUint32 aStartIndex = 0);
  NS_IMETHOD_(PRInt32) LastIndexOf(const nsISupports* aPossibleElement);

  NS_IMETHOD GetIndexOf(nsISupports *aPossibleElement, PRInt32 *_retval) {
    *_retval = IndexOf(aPossibleElement);
    return NS_OK;
  }
  
  NS_IMETHOD GetIndexOfStartingAt(nsISupports *aPossibleElement,
                                  PRUint32 aStartIndex, PRInt32 *_retval) {
    *_retval = IndexOfStartingAt(aPossibleElement, aStartIndex);
    return NS_OK;
  }
  
  NS_IMETHOD GetLastIndexOf(nsISupports *aPossibleElement, PRInt32 *_retval) {
    *_retval = LastIndexOf(aPossibleElement);
    return NS_OK;
  }
  
  NS_IMETHOD_(PRBool) InsertElementAt(nsISupports* aElement, PRUint32 aIndex);

  NS_IMETHOD_(PRBool) ReplaceElementAt(nsISupports* aElement, PRUint32 aIndex);

  NS_IMETHOD_(PRBool) RemoveElementAt(PRUint32 aIndex);
  NS_IMETHOD_(PRBool) RemoveElement(const nsISupports* aElement, PRUint32 aStartIndex = 0);
  NS_IMETHOD_(PRBool) RemoveLastElement(const nsISupports* aElement);

  NS_IMETHOD DeleteLastElement(nsISupports *aElement) {
    return (RemoveLastElement(aElement) ? NS_OK : NS_ERROR_FAILURE);
  }
  
  NS_IMETHOD DeleteElementAt(PRUint32 aIndex) {
    return (RemoveElementAt(aIndex) ? NS_OK : NS_ERROR_FAILURE);
  }
  
  NS_IMETHOD_(PRBool) AppendElements(nsISupportsArray* aElements);
  
  NS_IMETHOD Compact(void);

  NS_IMETHOD_(PRBool) EnumerateForwards(nsISupportsArrayEnumFunc aFunc, void* aData);
  NS_IMETHOD_(PRBool) EnumerateBackwards(nsISupportsArrayEnumFunc aFunc, void* aData);

protected:
  NS_IMETHOD_(nsISupportsArray&) operator=(const nsISupportsArray& aOther);
  NS_IMETHOD_(PRBool) operator==(const nsISupportsArray& aOther) { return Equals(&aOther); }
  NS_IMETHOD_(nsISupports*) operator[](PRUint32 aIndex) { return ElementAt(aIndex); }
  
  void DeleteArray(void);

  nsISupports** mArray;
  PRUint32 mArraySize;
  PRUint32 mCount;
  nsISupports*  mAutoArray[kAutoArraySize];

private:
  // Copy constructors are not allowed
  nsSupportsArray(const nsISupportsArray& other);
};

#endif // nsSupportsArray_h__
