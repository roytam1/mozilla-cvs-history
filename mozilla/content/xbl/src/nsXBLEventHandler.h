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

#ifndef nsXBLEventHandler_h__
#define nsXBLEventHandler_h__

#include "nsIDOMEventReceiver.h"

class nsIXBLBinding;
class nsIDOMEvent;
class nsIContent;
class nsIDOMUIEvent;
class nsIDOMKeyEvent;
class nsIDOMMouseEvent;
class nsIAtom;
class nsIController;
class nsIXBLPrototypeHandler;

// XXX This should be broken up into subclasses for each listener IID type, so we
// can cut down on the bloat of the handlers.
class nsXBLEventHandler : public nsISupports
{
public:
  nsXBLEventHandler(nsIDOMEventReceiver* aReceiver, nsIXBLPrototypeHandler* aHandler);
  virtual ~nsXBLEventHandler();
  
  NS_DECL_ISUPPORTS

public:
  void SetNextHandler(nsXBLEventHandler* aHandler) {
    mNextHandler = aHandler;
  }

  void RemoveEventHandlers();

  void MarkForDeath() {
    if (mNextHandler) mNextHandler->MarkForDeath(); mProtoHandler = nsnull; mEventReceiver = nsnull;
  }

  static nsresult GetTextData(nsIContent *aParent, nsAWritableString& aResult);

protected:
  static PRUint32 gRefCnt;
  static nsIAtom* kKeyAtom;
  static nsIAtom* kKeyCodeAtom;
  static nsIAtom* kCharCodeAtom;
  static nsIAtom* kActionAtom;
  static nsIAtom* kCommandAtom;
  static nsIAtom* kClickCountAtom;
  static nsIAtom* kButtonAtom;
  static nsIAtom* kModifiersAtom;

protected:
  nsIDOMEventReceiver* mEventReceiver; // Both of these refs are weak.
  nsCOMPtr<nsIXBLPrototypeHandler> mProtoHandler;

  nsXBLEventHandler* mNextHandler; // Handlers are chained for easy unloading later.
};

extern nsresult
NS_NewXBLEventHandler(nsIDOMEventReceiver* aEventReceiver, nsIXBLPrototypeHandler* aHandlerElement, 
                      nsXBLEventHandler** aResult);
#endif
