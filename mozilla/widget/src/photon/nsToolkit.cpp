/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsToolkit.h"
#include "nsWindow.h"
#include "prmon.h"
#include "prtime.h"
#include "nsGUIEvent.h"
#include <Pt.h>
#include "nsPhWidgetLog.h"


NS_DEFINE_IID(kIToolkitIID,  NS_ITOOLKIT_IID);
NS_IMPL_ISUPPORTS(nsToolkit,kIToolkitIID);

//-------------------------------------------------------------------------
//
// constructor
//
//-------------------------------------------------------------------------
nsToolkit::nsToolkit()  
{
  NS_INIT_REFCNT();
  mWidget = NULL;
}


//-------------------------------------------------------------------------
//
// destructor
//
//-------------------------------------------------------------------------
nsToolkit::~nsToolkit()
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsToolkit::~nsToolkit\n"));
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
NS_METHOD nsToolkit::Init(PRThread *aThread)
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsToolkit::Init this=<%p> aThread=<%p>\n", this, aThread));
  if( aThread )
  {  
    /* No idea what this is for */
  }
  else
  {
    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsToolkit::Init - ERROR: Wants to create a thread!\n"));
  }

  return NS_OK;
}
