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

#include "nsMessenger.h"
#include "nsMessengerNameSet.h"
#include "nsIScriptNameSetRegistry.h"

#include "nsDOMCID.h"

static NS_DEFINE_IID(kCScriptNameSetRegistryCID, NS_SCRIPT_NAMESET_REGISTRY_CID);

class nsMessengerBootstrap : public nsIAppShellService {
  
public:
  nsMessengerBootstrap(nsIServiceManager *serviceManager);
  virtual ~nsMessengerBootstrap();
  
  NS_DECL_ISUPPORTS
  
  // nsIAppShellService 
  // Initialize() is the only one we care about
  NS_IMETHOD Initialize();

private:
  nsIServiceManager *mServiceManager;

  
private:
  NS_IMETHOD Run(void) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD GetNativeEvent(void *& aEvent,
                            nsIWidget* aWidget,
                            PRBool &aIsInWindow,
                            PRBool &aIsMouseEvent)
    { return NS_ERROR_NOT_IMPLEMENTED; }
  
  NS_IMETHOD DispatchNativeEvent(void * aEvent)
    { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD Shutdown(void)
    { return NS_ERROR_NOT_IMPLEMENTED; }

  NS_IMETHOD CreateTopLevelWindow(nsIWidget * aParent,
                                  nsIURL* aUrl, 
                                  nsString& aControllerIID,
                                  nsIWidget*& aResult, nsIStreamObserver* anObserver,
                                  nsIXULWindowCallbacks *aCallbacks,
                                  PRInt32 aInitialWidth, PRInt32 aInitialHeight)
    { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD CreateDialogWindow(  nsIWidget * aParent,
                                  nsIURL* aUrl, 
                                  nsString& aControllerIID,
                                  nsIWidget*& aResult,
                                  nsIStreamObserver* anObserver,
                                  nsIXULWindowCallbacks *aCallbacks,
                                  PRInt32 aInitialWidth, PRInt32 aInitialHeight)
    { return NS_ERROR_NOT_IMPLEMENTED; }
  
  NS_IMETHOD CloseTopLevelWindow(nsIWidget* aWindow)
    { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD RegisterTopLevelWindow(nsIWidget* aWindow)
    { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD UnregisterTopLevelWindow(nsIWidget* aWindow)
    { return NS_ERROR_NOT_IMPLEMENTED; }

};

NS_IMPL_ISUPPORTS(nsMessengerBootstrap, nsIAppShellService::GetIID())

nsMessengerBootstrap::nsMessengerBootstrap(nsIServiceManager *serviceManager)
  : mServiceManager(serviceManager)
{
  NS_INIT_REFCNT();
  
  if (mServiceManager) NS_ADDREF(mServiceManager);
}

nsMessengerBootstrap::~nsMessengerBootstrap()
{
  NS_IF_RELEASE(mServiceManager);

}

nsresult
nsMessengerBootstrap::Initialize()
{
  nsIScriptNameSetRegistry *registry;
  nsresult rv =
    nsServiceManager::GetService(kCScriptNameSetRegistryCID,
                                 nsIScriptNameSetRegistry::GetIID(),
                                 (nsISupports **)&registry);
  if (NS_SUCCEEDED(rv)) {
    nsMessengerNameSet* nameSet = new nsMessengerNameSet();
    rv = registry->AddExternalNameSet(nameSet);
    /* todo - when to release this service? */
  }

  return rv;
}


nsresult
NS_NewMessengerBootstrap(nsIAppShellService **msgboot,
                         nsIServiceManager *serviceManager)
{
  if (!msgboot) return NS_ERROR_NULL_POINTER;
  if (!serviceManager) return NS_ERROR_NULL_POINTER;
  
  nsMessengerBootstrap *bootstrap =
    new nsMessengerBootstrap(serviceManager);

  if (!bootstrap) return NS_ERROR_OUT_OF_MEMORY;

  
  return bootstrap->QueryInterface(nsIAppShellService::GetIID(),
                                   (void **)msgboot);

}

// nsMessenger implementation

class nsMessenger : public nsIMessenger {

public:
    NS_DECL_ISUPPORTS;
};

NS_IMPL_ISUPPORTS(nsMessenger, nsIMessenger::GetIID())


nsresult
NS_NewMessenger(nsIMessenger **msg)
{
  if (!msg) return NS_ERROR_NULL_POINTER;
  nsMessenger *messenger = 
    new nsMessenger();
  if (!messenger) return NS_ERROR_OUT_OF_MEMORY;
  return messenger->QueryInterface(nsIMessenger::GetIID(),
			           (void**)&msg);
}

