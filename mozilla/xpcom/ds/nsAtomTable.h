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

#ifndef nsAtomTable_h__
#define nsAtomTable_h__

#include "nsIAtom.h"

class AtomImpl : public nsIAtom {
public:
  AtomImpl();
  virtual ~AtomImpl();

  NS_DECL_ISUPPORTS

  void* operator new(size_t size, const PRUnichar* us, PRInt32 uslen);

  void operator delete(void* ptr) {
    ::operator delete(ptr);
  }

  virtual void ToString(nsString& aBuf) const;

  virtual const PRUnichar* GetUnicode() const;

  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler) const;

  // Actually more; 0 terminated. This slot is reserved for the
  // terminating zero.
  PRUnichar mString[1];
};

#endif // nsAtomTable_h__
