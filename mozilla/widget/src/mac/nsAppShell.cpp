/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
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

// 
// nsAppShell
//
// This file contains the default implementation of the application shell. Clients
// may either use this implementation or write their own. If you write your
// own, you must create a message sink to route events to. (The message sink
// interface may change, so this comment must be updated accordingly.)
//

#include "nsAppShell.h"
#include "nsIAppShell.h"

#include "nsMacMessageSink.h"
#include "nsMacMessagePump.h"
#include "nsSelectionMgr.h"
#include "nsToolKit.h"
#include <Quickdraw.h>
#include <Fonts.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Traps.h>
#include <Events.h>
#include <Menus.h>

#include <stdlib.h>

#include "macstdlibextras.h"

PRBool nsAppShell::mInitializedToolbox = PR_FALSE;


//-------------------------------------------------------------------------
//
// nsISupports implementation macro
//
//-------------------------------------------------------------------------
NS_DEFINE_IID(kIAppShellIID, NS_IAPPSHELL_IID);
NS_IMPL_ISUPPORTS(nsAppShell,kIAppShellIID);

NS_IMETHODIMP nsAppShell::SetDispatchListener(nsDispatchListener* aDispatchListener)
{
  mDispatchListener = aDispatchListener;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Create the application shell
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsAppShell::Create(int* argc, char ** argv)
{
	mToolKit = auto_ptr<nsToolkit>( new nsToolkit() );

  // Create the selection manager
  if (!mSelectionMgr)
      NS_NewSelectionMgr(&mSelectionMgr);

	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Enter a message handler loop
//
//-------------------------------------------------------------------------
nsresult nsAppShell::Run()
{
	mMacSink = new nsMacMessageSink();
	mMacPump = auto_ptr<nsMacMessagePump>( new nsMacMessagePump(mToolKit.get(), mMacSink) );
	mMacPump->DoMessagePump();

  //if (mDispatchListener)
    //mDispatchListener->AfterDispatch();

	if (mExitCalled)	// hack: see below
	{
		mRefCnt --;
		if (mRefCnt == 0)
			delete this;
	}

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// nsAppShell constructor
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::Exit()
{
	if (mMacPump.get())
	{
		mMacPump->StopRunning();

		mExitCalled = PR_TRUE;
		mRefCnt ++;			// hack: since the applications are likely to delete us
										// after calling this method (see nsViewerApp::Exit()),
										// we temporarily bump the refCnt to let the message pump
										// exit properly. The object will delete itself afterwards.
	}
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// nsAppShell constructor
//
//-------------------------------------------------------------------------
nsAppShell::nsAppShell()
{ 
  // The toolbox initialization code has moved to NSStdLib (InitializeToolbox)
  
  if (!mInitializedToolbox)
  {
    InitializeMacToolbox();
    mInitializedToolbox = PR_TRUE;
  }
  mRefCnt = 0;
  mExitCalled = PR_FALSE;
  mSelectionMgr = 0;
}

//-------------------------------------------------------------------------
//
// nsAppShell destructor
//
//-------------------------------------------------------------------------
nsAppShell::~nsAppShell()
{
  NS_IF_RELEASE(mSelectionMgr);
}

//-------------------------------------------------------------------------
//
// GetNativeData
//
//-------------------------------------------------------------------------
void* nsAppShell::GetNativeData(PRUint32 aDataType)
{
  if (aDataType == NS_NATIVE_SHELL) 
  	{
    //return mTopLevel;
  	}
  return nsnull;
}


// XXX temporary code for Dialog investigation
nsresult nsAppShell::GetNativeEvent(void *& aEvent, nsIWidget* aWidget, PRBool &aIsInWindow, PRBool &aIsMouseEvent)
{
  aIsInWindow   = PR_FALSE;
  aIsMouseEvent = PR_FALSE;

  return NS_ERROR_FAILURE;
}

nsresult nsAppShell::DispatchNativeEvent(void * aEvent)
{
  return NS_ERROR_FAILURE;
}

NS_METHOD
nsAppShell::GetSelectionMgr(nsISelectionMgr** aSelectionMgr)
{
  *aSelectionMgr = mSelectionMgr;
  NS_IF_ADDREF(mSelectionMgr);
  if (!mSelectionMgr)
    return NS_ERROR_NOT_INITIALIZED;
  return NS_OK;
}

