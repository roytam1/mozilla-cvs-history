/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef _NS_IDBACCESSOR_H_
#define _NS_IDBACCESSOR_H_

#include "nsISupports.h"
#include "nsIFileSpec.h"

// nsIDBAccessorIID {6AADD4D0-7785-11d3-87FE-000629D01344}
#define NS_IDBACCESSOR_IID \
{ 0x6aadd4d0, 0x7785, 0x11d3, \
 {0x87, 0xfe, 0x0, 0x6, 0x29, 0xd0, 0x13, 0x44}}

// nsDBAccessorCID {6AADD4D1-7785-11d3-87FE-000629D01344}
#define NS_DBACCESSOR_CID \
{ 0x6aadd4d1, 0x7785, 0x11d3, \
  { 0x87, 0xfe, 0x0, 0x6, 0x29, 0xd0, 0x13, 0x44 }} 

class nsIDBAccessor : public nsISupports
{
  public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDBACCESSOR_IID)

  NS_IMETHOD Init(nsIFileSpec* DBFile) = 0 ;
  NS_IMETHOD Shutdown(void) = 0 ;

  NS_IMETHOD Put(PRInt32 aID, void* anEntry, PRUint32 aLength) = 0 ;

  NS_IMETHOD Get(PRInt32 aID, void** anEntry, PRUint32 *aLength) = 0 ;

  NS_IMETHOD Del(PRInt32 aID) = 0 ;

  NS_IMETHOD GenID(const char* key, PRUint32 length, PRInt32* aID) = 0 ;

  NS_IMETHOD EnumEntry(void* *anEntry, PRUint32* aLength, PRBool bReset) = 0 ;

} ;

#endif // _NS_IDBACCESSOR_H_

