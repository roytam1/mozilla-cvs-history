/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

// ==============================
// ! Scriptability related code !
// ==============================
//

/////////////////////////////////////////////////////
//
// This file implements the nsScriptablePeer object
// The native methods of this class are supposed to
// be callable from JavaScript
//
#include "plugin.h"

static NS_DEFINE_IID(kI4xScrPluginIID, NS_I4XSCRPLUGIN_IID);
static NS_DEFINE_IID(kISecurityCheckedComponentIID, NS_ISECURITYCHECKEDCOMPONENT_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

nsScriptablePeer::nsScriptablePeer(CPlugin* aPlugin)
{
  mPlugin = aPlugin;
  mRefCnt = 0;
}

nsScriptablePeer::~nsScriptablePeer()
{
}

// AddRef, Release and QueryInterface are common methods and must 
// be implemented for any interface
NS_IMETHODIMP_(nsrefcnt) nsScriptablePeer::AddRef() 
{ 
  ++mRefCnt; 
  return mRefCnt; 
} 

NS_IMETHODIMP_(nsrefcnt) nsScriptablePeer::Release() 
{ 
  --mRefCnt; 
  if (mRefCnt == 0) { 
    delete this;
    return 0; 
  } 
  return mRefCnt; 
} 

// here nsScriptablePeer should return three interfaces it can be asked for by their iid's
// static casts are necessary to ensure that correct pointer is returned
NS_IMETHODIMP nsScriptablePeer::QueryInterface(const nsIID& aIID, void** aInstancePtr) 
{ 
  if(!aInstancePtr) 
    return NS_ERROR_NULL_POINTER; 

  if(aIID.Equals(kI4xScrPluginIID)) {
    *aInstancePtr = static_cast<nsI4xScrPlugin*>(this); 
    AddRef();
    return NS_OK;
  }

  if(aIID.Equals(kISecurityCheckedComponentIID)) {
    *aInstancePtr = static_cast<nsISecurityCheckedComponent*>(this); 
    AddRef();
    return NS_OK;
  }

  if(aIID.Equals(kISupportsIID)) {
    *aInstancePtr = static_cast<nsISupports*>(static_cast<nsI4xScrPlugin*>(this)); 
    AddRef();
    return NS_OK;
  }

  return NS_NOINTERFACE; 
}

//
// the following two methods will be callable from JavaScript
//
NS_IMETHODIMP nsScriptablePeer::ShowVersion()
{
  if (mPlugin)
    mPlugin->showVersion();

  return NS_OK;
}

NS_IMETHODIMP nsScriptablePeer::Clear()
{
  if (mPlugin)
    mPlugin->clear();

  return NS_OK;
}

//
// the purpose of the rest of the code is to get succesfully 
// through the Mozilla Security Manager
//
static const char gAllAccess[] = "AllAccess";

NS_IMETHODIMP nsScriptablePeer::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  *_retval = (char*)NPN_MemAlloc(sizeof(gAllAccess)+1);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  strcpy(*_retval, gAllAccess);

  return NS_OK;
}

NS_IMETHODIMP nsScriptablePeer::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  *_retval = (char*)NPN_MemAlloc(sizeof(gAllAccess)+1);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  strcpy(*_retval, gAllAccess);

  return NS_OK;
}

NS_IMETHODIMP nsScriptablePeer::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  *_retval = (char*)NPN_MemAlloc(sizeof(gAllAccess)+1);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  strcpy(*_retval, gAllAccess);

  return NS_OK;
}

NS_IMETHODIMP nsScriptablePeer::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  *_retval = (char*)NPN_MemAlloc(sizeof(gAllAccess)+1);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  strcpy(*_retval, gAllAccess);

  return NS_OK;
}
