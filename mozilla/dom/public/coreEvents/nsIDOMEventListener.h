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

#ifndef nsIDOMEventListener_h__
#define nsIDOMEventListener_h__

#include "nsIDOMEvent.h"
#include "nsISupports.h"

/*
 * Event listener interface.
 */

#define NS_IDOMEVENTLISTENER_IID \
{ /* df31c120-ded6-11d1-bd85-00805f8ae3f4 */ \
0xdf31c120, 0xded6, 0x11d1, \
{0xbd, 0x85, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

class nsIDOMEventListener : public nsISupports
{
public:

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDOMEVENTLISTENER_IID)

 /**
  * Processes all events excepting mouse and key events. 
  * @param anEvent the event to process. @see nsIDOMEvent.h for event types.
  */

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) = 0;
};

#endif // nsIDOMEventListener_h__
