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
 *   Jerry Kirk    <Jerry.Kirk@NexwareCorp.com>
 */

#ifndef nsCPlatformBaseLoop_h__
#define nsCPlatformBaseLoop_h__

#include "PhT.h"

#include "nsCBaseLoop.h"

class nsCPlatformBaseLoop : public nsCBaseLoop 
{
protected:
	nsCPlatformBaseLoop(nsEventLoopType type);
	virtual ~nsCPlatformBaseLoop();

	// Internal Platform Implementations of nsIEventLoop 
	// (Error checking is ensured above)
	// We can over-ride these at a high level because they are the same
	// across all types of event loops.
	nsresult PlatformGetNextEvent(void* platformFilterData, void* platformEventData);
	nsresult PlatformPeekNextEvent(void* FilterData, void* platformEventData, 
		PRBool fRemoveElement);
	nsresult PlatformTranslateEvent(void* platformEventData);
	nsresult PlatformDispatchEvent(void* platformEventData);
	nsresult PlatformSendLoopEvent(void* platformEventData, PRInt32* result);
	nsresult PlatformPostLoopEvent(void* platformEventData);
	
	nsNativeEventDataType PlatformGetEventType();
	nsNativeFilterDataType PlatformGetFilterType();
	PRInt32 PlatformGetReturnCode(void* platformEventData);
	
protected:
	pthread_t m_PhThreadId;
};

#endif /* nsCAppLoop_h__ */
