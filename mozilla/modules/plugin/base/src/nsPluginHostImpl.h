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

#ifndef nsPluginHostImpl_h__
#define nsPluginHostImpl_h__

#include "xp_core.h"
#include "nsIPluginManager.h"
#include "nsIPluginHost.h"
#include "nsINetworkManager.h"
#include "nsCRT.h"
#include "prlink.h"
#include "nsIMalloc.h"

class ns4xPlugin;

class nsPluginTag
{
public:
  nsPluginTag();
  ~nsPluginTag();

  nsPluginTag   *mNext;
  char          *mName;
  char          *mDescription;
  char          *mMimeType;
  char          *mMimeDescription;
  char          *mExtensions;
  PRInt32       mVariants;
  char          **mMimeTypeArray;
  char          **mMimeDescriptionArray;
  char          **mExtensionsArray;
  PRLibrary     *mLibrary;
  nsIPlugin     *mEntryPoint;
  ns4xPlugin    *mAdapter;
  PRUint32      mFlags;
};

#define NS_PLUGIN_FLAG_ENABLED    0x0001    //is this plugin enabled?
#define NS_PLUGIN_FLAG_OLDSCHOOL  0x0002    //is this a pre-xpcom plugin?

class nsPluginHostImpl : public nsIPluginManager, public nsIPluginHost,
                         public nsINetworkManager
{
public:
  nsPluginHostImpl();
  ~nsPluginHostImpl();

  void* operator new(size_t sz) {
    void* rv = new char[sz];
    nsCRT::zero(rv, sz);
    return rv;
  }

  NS_DECL_ISUPPORTS

  //nsIPluginManager interface

  NS_IMETHOD
  ReloadPlugins(PRBool reloadPages);

  NS_IMETHOD
  UserAgent(const char* *resultingAgentString);

  NS_IMETHOD
  GetValue(nsPluginManagerVariable variable, void *value);

  NS_IMETHOD
  SetValue(nsPluginManagerVariable variable, void *value);

  //nsINetworkManager interface

  NS_IMETHOD
  GetURL(nsISupports* peer, const char* url, const char* target,
         void* notifyData = NULL, const char* altHost = NULL,
         const char* referrer = NULL, PRBool forceJSEnabled = PR_FALSE);

  NS_IMETHOD
  PostURL(nsISupports* peer, const char* url, const char* target,
          PRUint32 postDataLen, const char* postData,
          PRBool isFile = PR_FALSE, void* notifyData = NULL,
          const char* altHost = NULL, const char* referrer = NULL,
          PRBool forceJSEnabled = PR_FALSE,
          PRUint32 postHeadersLength = 0, const char* postHeaders = NULL);

  //nsIPluginHost interface

  NS_IMETHOD
  Init(void);

  NS_IMETHOD
  LoadPlugins(void);

  NS_IMETHOD
  InstantiatePlugin(const char *aMimeType, nsIURL *aURL, nsIPluginInstance ** aPluginInst);

  NS_IMETHOD
  InstantiatePlugin(const char *aMimeType, nsIPluginInstance ** aPluginInst,
                    nsPluginWindow *aWindow, nsString& aURL);

  NS_IMETHOD
  NewPluginStream(const nsString& aURL, nsIPluginInstance *aInstance, void *aNotifyData);

  NS_IMETHOD
  NewPluginStream(const nsString& aURL, nsIPluginInstance **aInstance, nsPluginWindow *aWindow);

  //nsIFactory interface

  NS_IMETHOD CreateInstance(nsISupports *aOuter,
                            REFNSIID aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock);

private:
  char        *mPluginPath;
  nsPluginTag *mPlugins;
  nsIMalloc   *mMalloc;
};

#endif
