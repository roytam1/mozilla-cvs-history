/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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

#ifndef ilIURL_h___
#define ilIURL_h___

#include <stdio.h>
#include "nsISupports.h"


// IID for the ilIURL interface
#define IL_IURL_IID    \
{ 0x6d7a5600, 0xb412, 0x11d1,    \
{ 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } }

class ilIURL : public nsISupports {
public:
  virtual void SetReader(ilINetReader *aReader)=0;

  virtual ilINetReader *GetReader()=0;

  virtual int GetContentLength()=0;

  virtual char* GetAddress()=0;

  virtual time_t GetExpires()=0;

  virtual PRBool GetBackgroundLoad()=0;

  virtual void SetBackgroundLoad(PRBool aBgload)=0;

  virtual int GetOwnerId()=0;

  virtual void SetOwnerId(int)=0;
};

#endif
