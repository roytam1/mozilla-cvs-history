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

#ifndef nsDOMEvent_h__
#define nsDOMEvent_h__

#include "nsIDOMUIEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsISupports.h"
#include "nsIPrivateDOMEvent.h"

#include "nsIPresContext.h"
#include "nsPoint.h"
#include "nsGUIEvent.h"
class nsIContent;

class nsIDOMRenderingContext;

class nsDOMEvent : public nsIDOMUIEvent, public nsIDOMNSUIEvent, public nsIPrivateDOMEvent {

public:
  // Note: this enum must be kept in sync with mEventNames in nsDOMEvent.cpp
  enum nsDOMEvents {
    eDOMEvents_mousedown=0,
    eDOMEvents_mouseup,
    eDOMEvents_click,
    eDOMEvents_dblclick,
    eDOMEvents_mouseover,
    eDOMEvents_mouseout,
    eDOMEvents_mousemove,
    eDOMEvents_keydown,
    eDOMEvents_keyup,
    eDOMEvents_keypress,
    eDOMEvents_focus,
    eDOMEvents_blur,
    eDOMEvents_load,
    eDOMEvents_unload,
    eDOMEvents_abort,
    eDOMEvents_error,
    eDOMEvents_submit,
    eDOMEvents_reset,
    eDOMEvents_change,
    eDOMEvents_paint,
	eDOMEvents_text
  };

  nsDOMEvent(nsIPresContext* aPresContext, nsEvent* aEvent);
  virtual ~nsDOMEvent();

  NS_DECL_ISUPPORTS

  // nsIDOMEventInterface
  NS_IMETHOD    GetType(nsString& aType);

  NS_IMETHOD    GetTarget(nsIDOMNode** aTarget);

  NS_IMETHOD    GetCurrentNode(nsIDOMNode** aCurrentNode);

  NS_IMETHOD    GetEventPhase(PRUint16* aEventPhase);

  NS_IMETHOD    PreventBubble();

  NS_IMETHOD    PreventCapture();

  NS_IMETHOD    PreventDefault();

  NS_IMETHOD	  GetText(nsString& aText);

  NS_IMETHOD	  GetCommitText(PRBool* aCommitText);
  NS_IMETHOD	  SetCommitText(PRBool aCommitText);

  NS_IMETHOD    GetScreenX(PRInt32* aScreenX);

  NS_IMETHOD    GetScreenY(PRInt32* aScreenY);

  NS_IMETHOD    GetClientX(PRInt32* aClientX);

  NS_IMETHOD    GetClientY(PRInt32* aClientY);

  NS_IMETHOD    GetAltKey(PRBool* aAltKey);

  NS_IMETHOD    GetCtrlKey(PRBool* aCtrlKey);

  NS_IMETHOD    GetShiftKey(PRBool* aShiftKey);

  NS_IMETHOD    GetMetaKey(PRBool* aMetaKey);

  NS_IMETHOD    GetCharCode(PRUint32* aCharCode);

  NS_IMETHOD    GetKeyCode(PRUint32* aKeyCode);

  NS_IMETHOD    GetButton(PRUint32* aButton);

  // nsIDOMNSEvent interface
  NS_IMETHOD    GetLayerX(PRInt32* aLayerX);

  NS_IMETHOD    GetLayerY(PRInt32* aLayerY);

  NS_IMETHOD    GetPageX(PRInt32* aClientX);

  NS_IMETHOD    GetPageY(PRInt32* aClientY);

  NS_IMETHOD    GetWhich(PRUint32* aKeyCode);

  NS_IMETHOD    GetRc(nsIDOMRenderingContext** aRc);

  // nsIPrivateDOMEvent interface
  NS_IMETHOD    DuplicatePrivateData();
  NS_IMETHOD    SetTarget(nsIDOMNode* aNode);

protected:

  nsEvent* mEvent;
  nsIPresContext* mPresContext;
  nsIDOMNode* mTarget;
  nsString*	mText;
  PRBool	mCommitText;
  const char* GetEventName(PRUint32 aEventType);

};

#endif // nsDOMEvent_h__
