/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 *
 */

#ifndef _nscanvas_h
#define _nscanvas_h

// nscanvas - this is the NS_CHILD_CID class which contains content.
//            The main code added in here is to do painting.

#include "nswindow.h"

class nsCanvas : public nsWindow
{
 public:
   nsCanvas();

 protected:
   // So we can do the right thing for 'top-level borderless' widgets (popups)
   virtual void RealDoCreate( HWND hwndP, nsWindow *aParent,
                              const nsRect &aRect,
                              EVENT_CALLBACK aHandleEventFunction,
                              nsIDeviceContext *aContext,
                              nsIAppShell *aAppShell,
                              nsWidgetInitData *aInitData,
                              HWND hwndOwner);

   virtual BOOL   SetWindowPos( HWND hwndInsertBehind, long x, long y,
                                long cx, long cy, unsigned long flags);

   virtual PRBool OnReposition( PSWP pSwp);
   virtual PRBool OnPaint();
   virtual PRBool OnKey( MPARAM mp1, MPARAM mp2);
   virtual PRBool OnRealizePalette();
   virtual PRBool DispatchMouseEvent( PRUint32 msg, int clickcount,
                                      MPARAM mp1, MPARAM mp2);
   virtual PCSZ  WindowClass();
   virtual ULONG WindowStyle();

   BOOL mIsTLB;
};

#endif
