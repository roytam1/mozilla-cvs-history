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


#ifndef nsIDOMLoadListener_h__
#define nsIDOMLoadListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"

/*
 * Mouse up/down/move event listener
 *
 */
#define NS_IDOMLOADLISTENER_IID \
{ /* f2b05200-ded5-11d1-bd85-00805f8ae3f4 */ \
0xf2b05200, 0xded5, 0x11d1, \
{0xbd, 0x85, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

class nsIDOMLoadListener : public nsIDOMEventListener {

public:

  /**
  * Processes a page or image load event
  * @param aMouseEvent @see nsIDOMEvent.h 
  * @returns whether the event was consumed or ignored. @see nsresult
  */
  virtual nsresult Load(const nsIDOMEvent* aEvent) = 0;

  /**
   * Processes a page unload event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   */
  virtual nsresult Unload(const nsIDOMEvent* aEvent) = 0;

  /**
   * Processes a load abort event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   *
   */
  virtual nsresult Abort(const nsIDOMEvent* aEvent) = 0;

  /**
   * Processes an load error event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   */
  virtual nsresult Error(const nsIDOMEvent* aEvent) = 0;

};

#endif // nsIDOMLoadListener_h__
