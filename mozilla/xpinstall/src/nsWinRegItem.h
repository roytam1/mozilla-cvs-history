/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#ifndef nsWinRegItem_h__
#define nsWinRegItem_h__

#include "prtypes.h"

#include "nsSoftwareUpdate.h"
#include "nsInstallObject.h"
#include "nsWinReg.h"

PR_BEGIN_EXTERN_C

class nsWinRegItem : public nsInstallObject {

public:

  /* Public Fields */

  /* Public Methods */
  nsWinRegItem(nsWinReg* regObj,
               PRInt32   root,
               PRInt32   action,
               nsString  sub,
               nsString  valname,
               nsString  val);
  
  ~nsWinRegItem();

  PRInt32 Prepare(void);

  PRInt32 Complete();
  
  char* toString();
  
  void Abort();
  
  float GetInstallOrder();
  
  
/* should these be protected? */
  PRBool CanUninstall();
  PRBool RegisterPackageNode();
	  
private:
  
  /* Private Fields */
  
  nsWinReg* reg;         // initiating WinReg object
  PRInt32   rootkey;
  PRInt32   command;
  nsString* subkey;     // Name of section
  nsString* name;         // Name of key
  void* value;       // data to write
  
  /* Private Methods */

  nsString* keystr(PRInt32 root, nsString* subkey, nsString* name);

  char* itoa(PRInt32 n);
  void reverseString(char* s);
};

PR_END_EXTERN_C

#endif /* nsWinRegItem_h__ */
