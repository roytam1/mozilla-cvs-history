/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Travis Bogard <travis@netscape.com>
 */

#include "nsCAppLoop.h"
#include "nsCPhFilter.h"
#include "nsPhEventLog.h"

//*****************************************************************************
//***    nsCAppLoop: Object Management
//*****************************************************************************

nsCAppLoop::nsCAppLoop() : nsCBaseAppLoop()
{
  PR_LOG(PhEventLog, PR_LOG_DEBUG, ("nsCAppLoop::nsCAppLoop m_PhThreadId=<%d>\n", m_PhThreadId)); 
}

nsCAppLoop::~nsCAppLoop()
{
  PR_LOG(PhEventLog, PR_LOG_DEBUG, ("nsCAppLoop::~nsCAppLoop m_PhThreadId=<%d>\n", m_PhThreadId)); 

}

NS_METHOD nsCAppLoop::Create(nsISupports* aOuter, const nsIID& aIID, 
	void** ppv)
{
  PR_LOG(PhEventLog, PR_LOG_DEBUG, ("nsCAppLoop::Create\n")); 

	NS_ENSURE_ARG_POINTER(ppv);
	NS_ENSURE_NO_AGGREGATION(aOuter);

	nsCAppLoop* app = new nsCAppLoop();
	NS_ENSURE(app, NS_ERROR_OUT_OF_MEMORY);

	NS_ADDREF(app);
	nsresult rv = app->QueryInterface(aIID, ppv);
	NS_RELEASE(app);

  PR_LOG(PhEventLog, PR_LOG_DEBUG, ("nsCAppLoop::Create the end rv=<%d>\n", rv)); 

	return rv;
}

//*****************************************************************************
// nsCAppLoop:: Internal Platform Implementations of nsIEventLoop 
//							(Error checking is ensured above)
//*****************************************************************************   

nsresult nsCAppLoop::PlatformExit(PRInt32 exitCode)
{
  PR_LOG(PhEventLog, PR_LOG_DEBUG, ("nsCAppLoop::PlatformExit exitCode=<%d>\n", exitCode)); 

	PostQuitMessage(exitCode);
	return NS_OK;
}