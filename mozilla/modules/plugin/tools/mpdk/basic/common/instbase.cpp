/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape Communications Corporation.
 * Portions created by Netscape Communications Corporation are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the LGPL or the GPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

/**********************************************************************
*
* instbase.cpp
*
* Implementation of the platform independent part of the plugin instance
* Eventual implementation is expected to be derived from this class
*
***********************************************************************/

#include "xplat.h"
#include "instbase.h"
#include "listener.h"

#include "dbg.h"

extern PRUint32 gPluginObjectCount;

CPluginInstance::CPluginInstance() :
  fPeer(nsnull), 
  fWindow(nsnull),
  fMode(nsPluginMode_Embedded)
{
  NS_INIT_REFCNT();
  gPluginObjectCount++;
  dbgOut2("CPluginInstance::CPluginInstance(), gPluginObjectCount = %lu", gPluginObjectCount);
}

CPluginInstance::~CPluginInstance()
{
  gPluginObjectCount--;
  dbgOut2("CPluginInstance::~CPluginInstance(), gPluginObjectCount = %lu", gPluginObjectCount);
}

NS_IMPL_ISUPPORTS1(CPluginInstance, nsIPluginInstance)

NS_METHOD CPluginInstance::Initialize(nsIPluginInstancePeer* peer)
{
  dbgOut1("CPluginInstance::Initialize");

  if(peer == nsnull)
    return NS_ERROR_NULL_POINTER;

  fPeer = peer;
  peer->AddRef();
  
  nsresult rv;
  rv = peer->GetMode(&fMode);

  if(NS_FAILED(rv)) 
    return rv;

  nsIPluginTagInfo* taginfo;
  const char* const* names = nsnull;
  const char* const* values = nsnull;
  PRUint16 count = 0;

  rv = peer->QueryInterface(NS_GET_IID(nsIPluginTagInfo), (void **)&taginfo);

  if(!NS_FAILED(rv))
  {
    taginfo->GetAttributes(count, names, values);
    NS_IF_RELEASE(taginfo);
  }

  PlatformNew();
  return NS_OK;
}

NS_METHOD CPluginInstance::GetPeer(nsIPluginInstancePeer* *result)
{
  dbgOut1("CPluginInstance::GetPeer");

  fPeer->AddRef();
  *result = fPeer;
  return NS_OK;
}

NS_METHOD CPluginInstance::Start()
{
  dbgOut1("CPluginInstance::Start");
  return NS_OK;
}

NS_METHOD CPluginInstance::Stop()
{
  dbgOut1("CPluginInstance::Stop");
  return NS_OK;
}

NS_METHOD CPluginInstance::Destroy()
{
  dbgOut1("CPluginInstance::Destroy");
  PlatformDestroy();
  return NS_OK;
}

NS_METHOD CPluginInstance::SetWindow(nsPluginWindow* window)
{
  dbgOut1("CPluginInstance::SetWindow");
  nsresult rv;
  rv = PlatformSetWindow(window);
  fWindow = window;
  return rv;
}

NS_METHOD CPluginInstance::NewStream(nsIPluginStreamListener** result)
{
  dbgOut1("CPluginInstance::NewStream");

	if(result == nsnull)
    return NS_ERROR_NULL_POINTER;

	CPluginStreamListener * listener = new CPluginStreamListener(this, "http://warp");

  if(listener == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  *result = listener;

  NS_ADDREF(listener);

	return NS_OK;
}

NS_METHOD CPluginInstance::Print(nsPluginPrint* printInfo)
{
  dbgOut1("CPluginInstance::Print");
  return NS_OK;
}

NS_METHOD CPluginInstance::HandleEvent(nsPluginEvent* event, PRBool* handled)
{
  dbgOut1("CPluginInstance::HandleEvent");
  *handled = (PRBool)PlatformHandleEvent(event);
  return NS_OK;
}

NS_METHOD CPluginInstance::GetValue(nsPluginInstanceVariable variable, void *value)
{
  dbgOut1("CPluginInstance::GetValue");
  return NS_ERROR_FAILURE;
}
