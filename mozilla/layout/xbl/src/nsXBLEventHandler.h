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

#include "nsIDOMMouseListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMenuListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMScrollListener.h"

class nsIXBLBinding;
class nsIDOMEvent;
class nsIContent;
class nsIDOMUIEvent;
class nsIDOMKeyEvent;
class nsIDOMMouseEvent;
class nsIAtom;
class nsIController;

class nsXBLEventHandler : public nsIDOMKeyListener, 
                          public nsIDOMMouseListener,
                          public nsIDOMMenuListener,
                          public nsIDOMFocusListener,
                          public nsIDOMScrollListener
{
public:
  nsXBLEventHandler(nsIContent* aBoundElement, nsIContent* aHandlerElement, const nsString& aEventName);
  virtual ~nsXBLEventHandler();
   
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  
  virtual nsresult KeyUp(nsIDOMEvent* aMouseEvent);
  virtual nsresult KeyDown(nsIDOMEvent* aMouseEvent);
  virtual nsresult KeyPress(nsIDOMEvent* aMouseEvent);
   
  virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseClick(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseDblClick(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseOver(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseOut(nsIDOMEvent* aMouseEvent);
  
  virtual nsresult Focus(nsIDOMEvent* aMouseEvent);
  virtual nsresult Blur(nsIDOMEvent* aMouseEvent);

  // menu
  NS_IMETHOD Create(nsIDOMEvent* aEvent);
  NS_IMETHOD Close(nsIDOMEvent* aEvent);
  NS_IMETHOD Destroy(nsIDOMEvent* aEvent);
  NS_IMETHOD Action(nsIDOMEvent* aEvent);
  NS_IMETHOD Broadcast(nsIDOMEvent* aEvent);
  NS_IMETHOD CommandUpdate(nsIDOMEvent* aEvent);

  // scroll
  NS_IMETHOD Overflow(nsIDOMEvent* aEvent);
  NS_IMETHOD Underflow(nsIDOMEvent* aEvent);
  NS_IMETHOD OverflowChanged(nsIDOMEvent* aEvent);

  NS_DECL_ISUPPORTS

public:
  void SetNextHandler(nsXBLEventHandler* aHandler) {
    mNextHandler = aHandler;
  }

  void RemoveEventHandlers();

protected:
  inline PRBool KeyEventMatched(nsIDOMKeyEvent* aKeyEvent);
  inline PRBool MouseEventMatched(nsIDOMMouseEvent* aMouseEvent);
  
  inline PRBool IsMatchingKeyCode(const PRUint32 aChar, const nsString& aKeyName);
  inline PRBool IsMatchingCharCode(const PRUint32 aChar, const nsString& aKeyName);

  NS_IMETHOD GetController(nsIController** aResult);

  NS_IMETHOD ExecuteHandler(const nsString& aEventName, nsIDOMEvent* aEvent);

  static PRUint32 gRefCnt;
  static nsIAtom* kKeyAtom;
  static nsIAtom* kKeyCodeAtom;
  static nsIAtom* kCharCodeAtom;
  static nsIAtom* kPrimaryAtom;
  static nsIAtom* kShiftAtom;
  static nsIAtom* kControlAtom;
  static nsIAtom* kAltAtom;
  static nsIAtom* kMetaAtom;
  static nsIAtom* kValueAtom;
  static nsIAtom* kCommandAtom;
  static nsIAtom* kClickCountAtom;
  static nsIAtom* kButtonAtom;
  
  static nsresult GetTextData(nsIContent *aParent, nsString& aResult);

protected:
  nsIContent* mBoundElement; // Both of these refs are weak.
  nsIContent* mHandlerElement;
  nsAutoString mEventName;

  nsXBLEventHandler* mNextHandler; // Handlers are chained for easy unloading later.
};

extern nsresult
NS_NewXBLEventHandler(nsIContent* aBoundElement, nsIContent* aHandlerElement, 
                      const nsString& aEventName,
                      nsXBLEventHandler** aResult);


#endif
