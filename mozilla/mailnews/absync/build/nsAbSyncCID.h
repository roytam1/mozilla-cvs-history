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

#ifndef nsAbSyncCID_h__
#define nsAbSyncCID_h__

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIComponentManager.h"

//
// Ab Sync Service
//
#define NS_ABSYNC_SERVICE_CID				  \
{ /* {3C21BB39-0A87-11d4-8FD6-00A024A7D144} */      \
    0x3c21bb39, 0xa87, 0x11d4,                      \
  { 0x8f, 0xd6, 0x0, 0xa0, 0x24, 0xa7, 0xd1, 0x44 } }

#define NS_ABSYNC_SERVICE_PROGID			\
  "component://netscape/absync"

//
// Ab Sync Listener
//
#define NS_ABSYNC_LISTENER_CID				  \
{ /* {3C21BB44-0A87-11d4-8FD6-00A024A7D144} */      \
    0x3c21bb44, 0xa87, 0x11d4,                      \
  { 0x8f, 0xd6, 0x0, 0xa0, 0x24, 0xa7, 0xd1, 0x44 } }

#define NS_ABSYNC_LISTENER_PROGID			\
  "component://netscape/absync/listener"

//
// Ab Sync Post Engine
//
#define NS_ABSYNC_POST_ENGINE_CID				  \
{ /* {3C21BB9f-0A87-11d4-8FD6-00A024A7D144} */      \
    0x3c21bb9f, 0xa87, 0x11d4,                      \
  { 0x8f, 0xd6, 0x0, 0xa0, 0x24, 0xa7, 0xd1, 0x44 } }

#define NS_ABSYNC_POST_ENGINE_PROGID			\
  "component://netscape/absync/postengine"


//
// Ab Sync Post Listener
//
#define NS_ABSYNC_POST_LISTENER_CID				  \
{ /* {3C21BBcc-0A87-11d4-8FD6-00A024A7D144} */      \
    0x3c21bbcc, 0xa87, 0x11d4,                      \
  { 0x8f, 0xd6, 0x0, 0xa0, 0x24, 0xa7, 0xd1, 0x44 } }

#define NS_ABSYNC_POST_LISTENER_PROGID			\
  "component://netscape/absync/postlistener"
  
  
#endif // nsAbSyncCID_h__
