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
#ifndef nsVoidArray_h___
#define nsVoidArray_h___

#include "nscore.h"

/// A basic zero-based array of void*'s that manages its own memory
class NS_BASE nsVoidArray {
public:
  nsVoidArray();
  ~nsVoidArray();

  nsVoidArray& operator=(const nsVoidArray& other);

  PRInt32 Count() const {
    return mCount;
  }

  void* ElementAt(PRInt32 aIndex) const;
  void* operator[](PRInt32 aIndex) const { return ElementAt(aIndex); }

  PRInt32 IndexOf(void* aPossibleElement) const;

  PRBool InsertElementAt(void* aElement, PRInt32 aIndex);

  PRBool ReplaceElementAt(void* aElement, PRInt32 aIndex);

  PRBool AppendElement(void* aElement) {
    return InsertElementAt(aElement, mCount);
  }

  PRBool RemoveElement(void* aElement);
  PRBool RemoveElementAt(PRInt32 aIndex);
  void   Clear();

  void Compact();

protected:
  void** mArray;
  PRInt32 mArraySize;
  PRInt32 mCount;

private:
  /// Copy constructors are not allowed
  nsVoidArray(const nsVoidArray& other);
};

#endif /* nsVoidArray_h___ */
