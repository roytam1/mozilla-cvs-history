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
 *
 */

/**********************************************************************
*
* listener.cpp
*
* Implementation of the plugin stream listener class
*
***********************************************************************/

#include "xp.h"
#include "listener.h"
#include "dbg.h"

extern PRUint32 gPluginObjectCount;

static NS_DEFINE_IID(kIPluginStreamListenerIID, NS_IPLUGINSTREAMLISTENER_IID);

CPluginStreamListener::CPluginStreamListener(CPluginInstance* inst, const char* msgName) :
  fMessageName(msgName)
{
  gPluginObjectCount++;
  NS_INIT_REFCNT();
  dbgOut3("CPluginStreamListener::CPluginStreamListener, message '%s', gPluginObjectCount = %lu", 
          fMessageName, gPluginObjectCount);
}

CPluginStreamListener::~CPluginStreamListener()
{
  gPluginObjectCount--;
  dbgOut2("CPluginStreamListener::~CPluginStreamListener, gPluginObjectCount = %lu", gPluginObjectCount);
}

NS_IMPL_QUERY_INTERFACE(CPluginStreamListener, kIPluginStreamListenerIID)
NS_IMPL_ADDREF(CPluginStreamListener)
NS_IMPL_RELEASE(CPluginStreamListener)

NS_METHOD CPluginStreamListener::OnStartBinding(nsIPluginStreamInfo* streamInfo)
{
  dbgOut1("CPluginStreamListener::OnStartBinding");
  return NS_OK;
}

NS_METHOD CPluginStreamListener::OnDataAvailable(nsIPluginStreamInfo* streamInfo, 
                                                 nsIInputStream* inputStream, 
                                                 PRUint32 length)
{
  dbgOut1("CPluginStreamListener::OnDataAvailable");

  char* buffer = new char[length];

  if(buffer == nsnull) 
    return NS_ERROR_OUT_OF_MEMORY;

  PRUint32 amountRead = 0;

  nsresult rv = inputStream->Read(buffer, length, &amountRead);

  if(rv == NS_OK) 
  {
    dbgOut2("\t\tReceived %lu bytes", length);
  }

  delete buffer;

  return rv;
}

NS_METHOD CPluginStreamListener::OnFileAvailable(nsIPluginStreamInfo* streamInfo, const char* fileName)
{
  dbgOut1("CPluginStreamListener::OnFileAvailable");

  return NS_OK;
}

NS_METHOD CPluginStreamListener::OnStopBinding(nsIPluginStreamInfo* streamInfo, nsresult status)
{
  dbgOut1("CPluginStreamListener::OnStopBinding");

  return NS_OK;
}

NS_METHOD CPluginStreamListener::OnNotify(const char* url, nsresult status)
{
  dbgOut1("CPluginStreamListener::OnNotify");

  return NS_OK;
}

NS_METHOD CPluginStreamListener::GetStreamType(nsPluginStreamType *result)
{
  dbgOut1("CPluginStreamListener::GetStreamType");

  *result = nsPluginStreamType_Normal;
  return NS_OK;
}
