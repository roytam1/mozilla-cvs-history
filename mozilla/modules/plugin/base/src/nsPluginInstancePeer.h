/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsPluginInstancePeer_h___
#define nsPluginInstancePeer_h___

#include "nsIPluginInstancePeer.h"
#include "nsIPluginTagInfo.h"
#include "nsIPluginInstanceOwner.h"

class nsPluginInstancePeerImpl : public nsIPluginInstancePeer, public nsIPluginTagInfo
{
public:
  nsPluginInstancePeerImpl();
  ~nsPluginInstancePeerImpl();

  NS_DECL_ISUPPORTS

  //nsIPluginInstancePeer interface

  NS_IMETHOD
  GetValue(nsPluginInstancePeerVariable variable, void *value);

  NS_IMETHOD
  GetMIMEType(nsMIMEType *result);

  NS_IMETHOD
  GetMode(nsPluginMode *result);

  NS_IMETHOD
  NewStream(nsMIMEType type, const char* target, nsIOutputStream* *result);

  NS_IMETHOD
  ShowStatus(const char* message);

  NS_IMETHOD
  SetWindowSize(PRUint32 width, PRUint32 height);

  //nsIPluginTagInfo interface

  NS_IMETHOD
  GetAttributes(PRUint16& n, const char*const*& names, const char*const*& values);

  NS_IMETHOD
  GetAttribute(const char* name, const char* *result);

  //locals

  nsresult Initialize(nsIPluginInstanceOwner *aInstance,
                      const nsMIMEType aMimeType);

  nsresult GetOwner(nsIPluginInstanceOwner *&aOwner);

private:
  nsIPluginInstance       *mInstance; //we don't add a ref to this
  nsIPluginInstanceOwner  *mOwner;    //we don't add a ref to this
  nsMIMEType              mMIMEType;
};

#endif
