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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef nsICalendarShell_h___
#define nsICalendarShell_h___

#include "nsISupports.h"
#include "nsIApplicationShell.h"
#include "nscore.h"
#include "nsIAppShell.h"
#include "nsICalendarUser.h"

#include "capi.h"
#include "nscal.h"

class nsICollectedData;

//fe35e400-ea8d-11d1-9244-00805f8a7ab6
#define NS_ICAL_SHELL_IID   \
{ 0xfe35e400, 0xea8d, 0x11d1,    \
{ 0x92, 0x44, 0x00, 0x80, 0x5f, 0x8a, 0x7a, 0xb6 } }

// Interface to the application shell.
class nsICalendarShell : public nsIApplicationShell,
                         public nsIAppShell 
{

public:

  NS_IMETHOD Logon() = 0;
  NS_IMETHOD Logoff() = 0;
  NS_IMETHOD LoadUI() = 0;
  NS_IMETHOD LoadPreferences() = 0;
  NS_IMETHOD ParseCommandLine() = 0;
  NS_IMETHOD ExecuteCommandScript(nsString aScript) = 0;


  NS_IMETHOD SetCAPISession(CAPISession aCAPISession) = 0;
  NS_IMETHOD_(CAPISession) GetCAPISession() = 0;

  NS_IMETHOD SetCAPIHandle(CAPIHandle aCAPIHandle) = 0;
  NS_IMETHOD_(CAPIHandle) GetCAPIHandle() = 0;

  NS_IMETHOD SetCAPIPassword(char * aPassword) = 0;
  NS_IMETHOD_(char *) GetCAPIPassword() = 0;

  NS_IMETHOD ReceiveCallback(nsICollectedData& aReply) = 0;

  NS_IMETHOD_(nsEventStatus) HandleEvent(nsGUIEvent *aEvent) = 0 ;

  NS_IMETHOD GetLoggedInUser(nsICalendarUser** LoggInUser) = 0;

};

#endif /* nsICalendarShell_h___ */
