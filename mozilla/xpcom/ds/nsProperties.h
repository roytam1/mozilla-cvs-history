/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL. You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All Rights
 * Reserved.
 */

#ifndef nsProperties_h___
#define nsProperties_h___

#include "nsIProperties.h"
#include "nsHashtable.h"

class nsIUnicharInputStream;

class nsProperties : public nsIProperties, public nsHashtable {
public:

  NS_DECL_ISUPPORTS

  // nsIProperties methods:
  NS_IMETHOD DefineProperty(const char* prop, nsISupports* initialValue);
  NS_IMETHOD UndefineProperty(const char* prop);
  NS_IMETHOD GetProperty(const char* prop, nsISupports* *result);
  NS_IMETHOD SetProperty(const char* prop, nsISupports* value);
  NS_IMETHOD HasProperty(const char* prop, nsISupports* value); 

  // nsProperties methods:
  nsProperties();
  virtual ~nsProperties();

  static PRBool ReleaseValues(nsHashKey* key, void* data, void* closure);

};

class nsPersistentProperties : public nsIPersistentProperties
{
public:
  nsPersistentProperties();
  virtual ~nsPersistentProperties();

  NS_DECL_ISUPPORTS

  // nsIProperties methods:
  NS_IMETHOD DefineProperty(const char* prop, nsISupports* initialValue);
  NS_IMETHOD UndefineProperty(const char* prop);
  NS_IMETHOD GetProperty(const char* prop, nsISupports* *result);
  NS_IMETHOD SetProperty(const char* prop, nsISupports* value);
  NS_IMETHOD HasProperty(const char* prop, nsISupports* value); 

  // nsIPersistentProperties methods:
  NS_IMETHOD Load(nsIInputStream* aIn);
  NS_IMETHOD Save(nsIOutputStream* aOut, const nsString& aHeader);
  NS_IMETHOD Subclass(nsIPersistentProperties* aSubclass);

  // XXX these 2 methods will be subsumed by the ones from 
  // nsIProperties once we figure this all out
  NS_IMETHOD GetProperty(const nsString& aKey, nsString& aValue);
  NS_IMETHOD SetProperty(const nsString& aKey, nsString& aNewValue,
                         nsString& aOldValue);

  // nsPersistentProperties methods:
  PRInt32 Read();
  PRInt32 SkipLine(PRInt32 c);
  PRInt32 SkipWhiteSpace(PRInt32 c);

  nsIUnicharInputStream* mIn;
  nsIPersistentProperties* mSubclass;
  struct PLHashTable*    mTable;
};

#endif /* nsProperties_h___ */
