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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

#ifndef nsXBLKeyHandler_h__
#define nsXBLKeyHandler_h__

#include "nsIDOMKeyListener.h"
#include "nsXBLEventHandler.h"

class nsIXBLBinding;
class nsIDOMEvent;
class nsIContent;
class nsIDOMUIEvent;
class nsIDOMKeyEvent;
class nsIDOMMouseEvent;
class nsIAtom;
class nsIController;
class nsIXBLPrototypeHandler;

class nsXBLKeyHandler : public nsIDOMKeyListener, 
                        public nsXBLEventHandler
{
public:
  nsXBLKeyHandler(nsIDOMEventReceiver* aReceiver,
                  nsIXBLPrototypeHandler* aHandler);

  virtual ~nsXBLKeyHandler();

  // nsIDOMetc.
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; };

  NS_IMETHOD KeyUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD KeyDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aMouseEvent);

  NS_DECL_ISUPPORTS_INHERITED

protected:
  static PRUint32 gRefCnt;
  static nsIAtom* kKeyUpAtom;
  static nsIAtom* kKeyDownAtom;
  static nsIAtom* kKeyPressAtom;

protected:
  // Members
};

extern nsresult
NS_NewXBLKeyHandler(nsIDOMEventReceiver* aEventReceiver,
                    nsIXBLPrototypeHandler* aHandlerElement, 
                    nsXBLKeyHandler** aResult);


#endif
