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

#ifndef nsIAppStartupNotifier_h___
#define nsIAppStartupNotifier_h___

#include "nsIObserver.h"

/*
 Some components need to be run at the startup of mozilla or embedding - to 
 start new services etc.

 This interface provides a generic way to start up arbitrary components 
 without requiring them to hack into main1() (or into NS_InitEmbedding) as 
 it's currently being done for services such as wallet, command line handlers 
 etc.

 We will have a category called "app-startup" which components register 
 themselves in using the CategoryManager.

 Components can also (optionally) add the word "service," as a prefix 
 to the "value" they pass in during a call to AddCategoryEntry() as
 shown below:

    categoryManager->AddCategoryEntry(APPSTARTUP_CATEGORY, "testcomp",
                        "service," NS_WALLETSERVICE_CONTRACTID
                        PR_TRUE, PR_TRUE,
                        getter_Copies(previous));

 Presence of the "service" keyword indicates the components desire to 
 be started as a service. When the "service" keyword is not present
 we just do a do_CreateInstance.

 When mozilla starts (and when NS_InitEmbedding()) is invoked
 we create an instance of the AppStartupNotifier component (which 
 implements nsIObserver) and invoke it's Observe() method. 

 Observe()  will enumerate the components registered into the
 APPSTARTUP_CATEGORY and notify them that startup has begun
 and release them.
*/

#include "nsString.h"

#define NS_APPSTARTUPNOTIFIER_CONTRACTID "@mozilla.org/embedcomp/appstartup-notifier;1"
#define NS_APPSTARTUPNOTIFIER_CLASSNAME  "AppStartup Notifier"

#define APPSTARTUP_CATEGORY "app-startup"
#define APPSTARTUP_TOPIC (NS_LITERAL_STRING(APPSTARTUP_CATEGORY).get())


/*
 Please note that there's not a new interface in this file.
 We're just leveraging nsIObserver instead of creating a
 new one

 This file exists solely to provide the defines above
*/

#endif /* nsIAppStartupNotifier_h___ */

