/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef __XPConnectFactory_h
#define __XPConnectFactory_h

#include "nsIDOMXPConnectFactory.h"
#include "nsIScriptObjectOwner.h"

class XPConnectFactoryImpl : public nsIDOMXPConnectFactory,
                             public nsIScriptObjectOwner
{
public:
  XPConnectFactoryImpl();

  NS_DECL_ISUPPORTS

  // nsIXPConnectFactory interface...
  NS_IMETHOD CreateInstance(const nsString &progID, nsISupports**_retval);

  // nsIScriptObjectOwner interface...
  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void *aScriptObject);

protected:
  virtual ~XPConnectFactoryImpl();

private:
  void *mScriptObject;
};

#endif
