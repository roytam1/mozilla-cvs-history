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
  nsWinRegItem(nsWinReg*        regObj,
               PRInt32          root,
               PRInt32          action,
               const nsString&  sub,
               const nsString&  valname,
               const nsString&  val,
               PRInt32*         aReturn);
  
  nsWinRegItem(nsWinReg*        regObj,
               PRInt32          root,
               PRInt32          action,
               const nsString&  sub,
               const nsString&  valname,
               PRInt32          val,
               PRInt32*         aReturn);
  
  virtual ~nsWinRegItem();

  PRInt32 Prepare(void);

  PRInt32 Complete();
  
  char* toString();
  
  void Abort();
  
/* should these be protected? */
  PRBool CanUninstall();
  PRBool RegisterPackageNode();
	  
private:
  
  /* Private Fields */
  
  nsWinReg* mReg;        // initiating WinReg object
  PRInt32   mRootkey;
  PRInt32   mCommand;
  nsString* mSubkey;     // Name of section
  nsString* mName;       // Name of key
  void*     mValue;      // data to write
  
  /* Private Methods */

  nsString* keystr(PRInt32 root, nsString* subkey, nsString* name);

  char* itoa(PRInt32 n);
  void reverseString(char* s);
};

PR_END_EXTERN_C

#endif /* nsWinRegItem_h__ */
