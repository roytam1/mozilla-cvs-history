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
/* AUTO-GENERATED. DO NOT EDIT!!! */

#ifndef nsIDOMNSEvent_h__
#define nsIDOMNSEvent_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"


#define NS_IDOMNSEVENT_IID \
{ 0x6f76532f,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMNSEvent : public nsISupports {
public:
  enum {
    EVENT_MOUSEDOWN = 1,
    EVENT_MOUSEUP = 2,
    EVENT_MOUSEOVER = 4,
    EVENT_MOUSEOUT = 8,
    EVENT_MOUSEMOVE = 16,
    EVENT_MOUSEDRAG = 32,
    EVENT_CLICK = 64,
    EVENT_DBLCLICK = 128,
    EVENT_KEYDOWN = 256,
    EVENT_KEYUP = 512,
    EVENT_KEYPRESS = 1024,
    EVENT_DRAGDROP = 2048,
    EVENT_FOCUS = 4096,
    EVENT_BLUR = 8192,
    EVENT_SELECT = 16384,
    EVENT_CHANGE = 32768,
    EVENT_RESET = 65536,
    EVENT_SUBMIT = 131072,
    EVENT_SCROLL = 262144,
    EVENT_LOAD = 524288,
    EVENT_UNLOAD = 1048576,
    EVENT_XFER_DONE = 2097152,
    EVENT_ABORT = 4194304,
    EVENT_ERROR = 8388608,
    EVENT_LOCATE = 16777216,
    EVENT_MOVE = 33554432,
    EVENT_RESIZE = 67108864,
    EVENT_FORWARD = 134217728,
    EVENT_HELP = 268435456,
    EVENT_BACK = 536870912,
    EVENT_ALT_MASK = 1,
    EVENT_CONTROL_MASK = 2,
    EVENT_SHIFT_MASK = 4,
    EVENT_META_MASK = 8
  };

  NS_IMETHOD    GetLayerX(PRInt32* aLayerX)=0;
  NS_IMETHOD    SetLayerX(PRInt32 aLayerX)=0;

  NS_IMETHOD    GetLayerY(PRInt32* aLayerY)=0;
  NS_IMETHOD    SetLayerY(PRInt32 aLayerY)=0;
};


#define NS_DECL_IDOMNSEVENT   \
  NS_IMETHOD    GetLayerX(PRInt32* aLayerX);  \
  NS_IMETHOD    SetLayerX(PRInt32 aLayerX);  \
  NS_IMETHOD    GetLayerY(PRInt32* aLayerY);  \
  NS_IMETHOD    SetLayerY(PRInt32 aLayerY);  \



#define NS_FORWARD_IDOMNSEVENT(_to)  \
  NS_IMETHOD    GetLayerX(PRInt32* aLayerX) { return _to##GetLayerX(aLayerX); } \
  NS_IMETHOD    SetLayerX(PRInt32 aLayerX) { return _to##SetLayerX(aLayerX); } \
  NS_IMETHOD    GetLayerY(PRInt32* aLayerY) { return _to##GetLayerY(aLayerY); } \
  NS_IMETHOD    SetLayerY(PRInt32 aLayerY) { return _to##SetLayerY(aLayerY); } \


#endif // nsIDOMNSEvent_h__
