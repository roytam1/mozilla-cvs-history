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

#ifndef __nsXtEventHandler_h      
#define __nsXtEventHandler_h

#include "Xm/Xm.h"

class nsIWidget;
class nsIMenuItem;

void nsXtWidget_KeyPressMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b);
void nsXtWidget_KeyReleaseMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b);
void nsXtWidget_ExposureMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b);
void nsXtWidget_ButtonPressMask_EventHandler(Widget w,XtPointer p, XEvent * event, Boolean * b);
void nsXtWidget_ButtonReleaseMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b);
void nsXtWidget_ButtonMotionMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b);
void nsXtWidget_MotionMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b);
void nsXtWidget_EnterMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b);
void nsXtWidget_LeaveMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b);
//----------------------------------------------------

void nsXtWidget_FSBCancel_Callback(Widget w, XtPointer p, XtPointer call_data);
void nsXtWidget_FSBOk_Callback(Widget w, XtPointer p, XtPointer call_data);

//----------------------------------------------------
void nsXtWidget_Focus_Callback(Widget w, XtPointer p, XtPointer call_data);
void nsXtWidget_Scrollbar_Callback(Widget w, XtPointer p, XtPointer call_data);
void nsXtWidget_Toggle_Callback(Widget w, XtPointer p, XtPointer call_data);
void nsXtWidget_Toggle_ArmCallback(Widget w, XtPointer p, XtPointer call_data);
void nsXtWidget_Toggle_DisArmCallback(Widget w, XtPointer p, XtPointer call_data);
void nsXtWidget_RadioButton_ArmCallback(Widget w, XtPointer p, XtPointer call_data);
void nsXtWidget_RadioButton_DisArmCallback(Widget w, XtPointer p, XtPointer call_data);

void nsXtWidget_Text_Callback(Widget w, XtPointer p, XtPointer call_data);
void nsXtWidget_Resize_Callback(Widget w, XtPointer p, XtPointer call_data);
void nsXtWidget_Expose_Callback(Widget w, XtPointer p, XtPointer call_data);

void nsXtWidget_Refresh_Callback(XtPointer call_data);

void nsXtWidget_ResetResize_Callback(XtPointer call_data);

void nsXtWidget_Menu_Callback(Widget w, XtPointer p, XtPointer call_data);

#endif  // __nsXtEventHandler.h





