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

#include "Xm/Xm.h"
#include "nsXtEventHandler.h"
#include "X11/keysym.h"

#include "nsWindow.h"
#include "nsTextWidget.h"
#include "nsCheckButton.h"
#include "nsRadioButton.h"
#include "nsFileWidget.h"
#include "nsGUIEvent.h"

#include "stdio.h"

#define DBG 0

extern XtAppContext gAppContext;


struct nsKeyConverter {
  int vkCode; // Platform independent key code
  XID keysym; // X keysym key code
};

struct nsKeyConverter nsKeycodes[] = { 
  NS_VK_CANCEL,     XK_Cancel, 
  NS_VK_BACK,       XK_BackSpace,
  NS_VK_TAB,        XK_Tab,
  NS_VK_CLEAR,      XK_Clear,
  NS_VK_RETURN,     XK_Return,
  NS_VK_SHIFT,      XK_Shift_Lock,      
//  NS_VK_CONTROL,    XK_Control,
  NS_VK_ALT,        XK_Alt_L,
  NS_VK_ALT,        XK_Alt_R,
  NS_VK_PAUSE,      XK_Pause,
  NS_VK_CAPS_LOCK,  XK_Caps_Lock,
  NS_VK_ESCAPE,     XK_Escape,
  NS_VK_SPACE,      XK_space,
//  NS_VK_PAGE_UP,   XK_PageUp,
//  NS_VK_PAGE_DOWN, XK_PageDown,
  NS_VK_END,        XK_End,
  NS_VK_HOME,       XK_Home,
  NS_VK_LEFT,       XK_Left,
  NS_VK_UP,         XK_Up,
  NS_VK_RIGHT,      XK_Right,
  NS_VK_DOWN,       XK_Down, 
  NS_VK_PRINTSCREEN, XK_Print,
  NS_VK_INSERT,     XK_Insert,
  NS_VK_DELETE,     XK_Delete,

  NS_VK_NUMPAD0,    XK_KP_0, 
  NS_VK_NUMPAD1,    XK_KP_1,
  NS_VK_NUMPAD2,    XK_KP_2,
  NS_VK_NUMPAD3,    XK_KP_3,
  NS_VK_NUMPAD4,    XK_KP_4,
  NS_VK_NUMPAD5,    XK_KP_5,
  NS_VK_NUMPAD6,    XK_KP_6,
  NS_VK_NUMPAD7,    XK_KP_7,
  NS_VK_NUMPAD8,    XK_KP_8,
  NS_VK_NUMPAD9,    XK_KP_9,

  NS_VK_MULTIPLY,   XK_KP_Multiply,
  NS_VK_ADD,        XK_KP_Add,
  NS_VK_SEPARATOR,  XK_KP_Separator,
  NS_VK_SUBTRACT,   XK_KP_Subtract,
  NS_VK_DECIMAL,    XK_KP_Decimal,
  NS_VK_DIVIDE,     XK_KP_Divide,
  NS_VK_F1,         XK_F1,
  NS_VK_F2,         XK_F2,
  NS_VK_F3,         XK_F3,
  NS_VK_F4,         XK_F4,
  NS_VK_F5,         XK_F5,
  NS_VK_F6,         XK_F6,
  NS_VK_F7,         XK_F7,
  NS_VK_F8,         XK_F8,
  NS_VK_F9,         XK_F9,
  NS_VK_F10,        XK_F10,
  NS_VK_F11,        XK_F11,
  NS_VK_F12,        XK_F12,
  NS_VK_F13,        XK_F13,
  NS_VK_F14,        XK_F14,
  NS_VK_F15,        XK_F15,
  NS_VK_F16,        XK_F16,
  NS_VK_F17,        XK_F17,
  NS_VK_F18,        XK_F18,
  NS_VK_F19,        XK_F19,
  NS_VK_F20,        XK_F20,
  NS_VK_F21,        XK_F21,
  NS_VK_F22,        XK_F22,
  NS_VK_F23,        XK_F23,
  NS_VK_F24,        XK_F24,

  NS_VK_COMMA,      XK_comma,
  NS_VK_PERIOD,     XK_period, 
  NS_VK_SLASH,      XK_slash, 
//  NS_VK_BACK_QUOTE, XK_backquote, 
  NS_VK_OPEN_BRACKET, XK_bracketleft, 
  NS_VK_CLOSE_BRACKET, XK_bracketright, 
  NS_VK_QUOTE, XK_quotedbl
  
}; 


int nsConvertKey(XID keysym)
{
 int i;
 int length = sizeof(nsKeycodes) / sizeof(struct nsKeyConverter);
 for (i = 0; i < length; i++) {
   if (keysym == nsKeycodes[i].keysym)
     return(nsKeycodes[i].vkCode);
 }

 return((int)keysym);
}


//==============================================================
void nsXtWidget_InitNSEvent(XEvent   * anXEv,
                            XtPointer  p,
                            nsGUIEvent &anEvent,
                            PRUint32   aEventType) 
{
  anEvent.message = aEventType;
  anEvent.widget  = (nsWindow *) p;
  anEvent.eventStructType = NS_GUI_EVENT;

  if (anXEv != NULL) {
    anEvent.point.x = anXEv->xbutton.x;
    anEvent.point.y = anXEv->xbutton.y;
  }

  anEvent.time = 0; //TBD

}

//==============================================================
void nsXtWidget_InitNSMouseEvent(XEvent   * anXEv,
                                 XtPointer  p,
                                 nsMouseEvent &anEvent,
                                 PRUint32   aEventType) 
{
  // Do base initialization
  nsXtWidget_InitNSEvent(anXEv, p, anEvent, aEventType);

  if (anXEv != NULL) { // Do Mouse Event specific intialization
    anEvent.time       = anXEv->xbutton.time;
    anEvent.isShift    = anXEv->xbutton.state & ShiftMask;
    anEvent.isControl  = anXEv->xbutton.state & ControlMask;
    anEvent.isAlt      = PR_FALSE; // Fix later
    anEvent.clickCount = 1; // Fix for double-clicks
    anEvent.eventStructType = NS_MOUSE_EVENT;
  }

  //anEvent.isAlt      = GetKeyState(VK_LMENU) < 0    || GetKeyState(VK_RMENU) < 0;
  ////anEvent.clickCount = (aEventType == NS_MOUSE_LEFT_DOUBLECLICK ||
                      //aEventType == NS_MOUSE_LEFT_DOUBLECLICK)? 2:1;

}

//==============================================================
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define INTERSECTS(r1_x1,r1_x2,r1_y1,r1_y2,r2_x1,r2_x2,r2_y1,r2_y2) \
        !((r2_x2 <= r1_x1) ||\
          (r2_y2 <= r1_y1) ||\
          (r2_x1 >= r1_x2) ||\
          (r2_y1 >= r1_y2))

//==============================================================
typedef struct COLLAPSE_INFO {
    Window win;
    nsRect *r;
} CollapseInfo;

//==============================================================
static void expandDamageRect(nsRect *drect, XEvent *xev, Boolean debug, char*str)
{
/*    int x1 = xev->xexpose.x;
    int y1 = xev->xexpose.y;
    int x2 = x1 + xev->xexpose.width;
    int y2 = y1 + xev->xexpose.height;

    if (debug) {
        printf("   %s: collapsing (%d,%d %dx%d) into (%d,%d %dx%d) ->>",
               str, x1, y1, xev->xexpose.width, xev->xexpose.height,
               drect->x, drect->y, drect->width - drect->x, drect->height - drect->y); 
    } 
    
    drect->x = MIN(x1, drect->x);
    drect->y = MIN(y1, drect->y);
    drect->width = MAX(x2, drect->width);
    drect->height = MAX(y2, drect->height);
    
    if (debug) {
        printf("(%d,%d %dx%d) %s\n",
               drect->x, drect->y, drect->width - drect->x, drect->height - drect->y);
    }*/
}

//==============================================================
static Bool checkForExpose(Display *dpy, XEvent *evt, XtPointer client_data) 
{
    CollapseInfo *cinfo = (CollapseInfo*)client_data; 

    if ((evt->type == Expose && evt->xexpose.window == cinfo->win &&
         INTERSECTS(cinfo->r->x, cinfo->r->width, cinfo->r->y, cinfo->r->height,
                    evt->xexpose.x, evt->xexpose.y, 
                    evt->xexpose.x + evt->xexpose.width, 
                    evt->xexpose.y + evt->xexpose.height)) ||
  (evt->type == GraphicsExpose && evt->xgraphicsexpose.drawable == cinfo->win &&
         INTERSECTS(cinfo->r->x, cinfo->r->width, cinfo->r->y, cinfo->r->height,
                    evt->xgraphicsexpose.x, evt->xgraphicsexpose.y, 
                    evt->xgraphicsexpose.x + evt->xgraphicsexpose.width, 
                    evt->xgraphicsexpose.y + evt->xgraphicsexpose.height))) {

        return True;
    }
    return False;
}


//==============================================================
void nsXtWidget_ExposureMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b)
{

  if (DBG) fprintf(stderr, "In nsXtWidget_ExposureMask_EventHandler Type %d (%d,%d)\n", event->type, Expose, GraphicsExpose);
  nsWindow * widgetWindow = (nsWindow *) p ;

  nsPaintEvent pevent;
  nsRect       rect;
  nsXtWidget_InitNSEvent(event, p, pevent, NS_PAINT);
  pevent.rect = (nsRect *)&rect;
  XEvent xev;

  rect.x      = event->xexpose.x;
  rect.y      = event->xexpose.y;
  rect.width  = event->xexpose.width;
  rect.height = event->xexpose.height;

  //printf("Expose (%d %d %d %d)\n", event->xexpose.x, event->xexpose.y, 
                                   //event->xexpose.width, event->xexpose.height);

  if (widgetWindow->GetResized())
   return;

  if (event->type == NoExpose) {
    return;
  }

    Display* display = XtDisplay(w);
    Window   window = XtWindow(w);

    XSync(display, FALSE);

    while (XCheckTypedWindowEvent(display, window, Expose, &xev) == TRUE) {
      rect.x      = xev.xexpose.x;
      rect.y      = xev.xexpose.y;
      rect.width  = xev.xexpose.width;
      rect.height = xev.xexpose.height;
      //printf("rect %d %d %d %d\n", rect.x, rect.y, rect.width, rect.height);
      //fe_expose_eh(drawing_area, (XtPointer)context, &xev);
    }

  if (DBG) printf("Calling OnPaint (%d %d %d %d)\n", rect.x, rect.y, rect.width, rect.height);
  widgetWindow->OnPaint(pevent);

}

//==============================================================
void nsXtWidget_ButtonPressMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b)
{
  nsWindow * widgetWindow = (nsWindow *) p ;
  nsMouseEvent mevent;
  nsXtWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_LEFT_BUTTON_DOWN);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsXtWidget_ButtonReleaseMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b)
{
  nsWindow * widgetWindow = (nsWindow *) p ;
  nsMouseEvent mevent;
  nsXtWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_LEFT_BUTTON_UP);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsXtWidget_ButtonMotionMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b)
{
  //if (DBG) fprintf(stderr, "nsXtWidget_ButtonMotionMask_EventHandler\n");
  nsPaintEvent pevent ;
  nsWindow * widgetWindow = (nsWindow *) p ;
  nsMouseEvent mevent;
  nsXtWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_MOVE);
  widgetWindow->DispatchMouseEvent(mevent);

}

//==============================================================
void nsXtWidget_MotionMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b)
{
  //if (DBG) fprintf(stderr, "nsXtWidget_ButtonMotionMask_EventHandler\n");
  nsWindow * widgetWindow = (nsWindow *) p ;
  nsMouseEvent mevent;
  nsXtWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_MOVE);
  widgetWindow->DispatchMouseEvent(mevent);

}

//==============================================================
void nsXtWidget_EnterMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b)
{
  nsWindow * widgetWindow = (nsWindow *) p ;
  nsMouseEvent mevent;
  nsXtWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_ENTER);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsXtWidget_LeaveMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b)
{
  if (DBG) fprintf(stderr, "***************** nsXtWidget_LeaveMask_EventHandler\n");
  nsWindow * widgetWindow = (nsWindow *) p ;
  nsMouseEvent mevent;
  nsXtWidget_InitNSMouseEvent(event, p, mevent, NS_MOUSE_EXIT);
  widgetWindow->DispatchMouseEvent(mevent);
}

//==============================================================
void nsXtWidget_Focus_Callback(Widget w, XtPointer p, XtPointer call_data)
{
  nsWindow * widgetWindow = (nsWindow *) p ;

  XmAnyCallbackStruct * cbs = (XmAnyCallbackStruct*)call_data;

  nsGUIEvent event;
  nsXtWidget_InitNSEvent(cbs->event, p, event, 
                         cbs->reason == XmCR_FOCUS?NS_GOTFOCUS:NS_LOSTFOCUS);
  widgetWindow->DispatchFocus(event);
  
}

//==============================================================
void nsXtWidget_Toggle_Callback(Widget w, XtPointer p, XtPointer call_data)
{
  nsWindow * widgetWindow = (nsWindow *) p ;
  if (DBG) fprintf(stderr, "***************** nsXtWidget_Scrollbar_Callback\n");

  nsScrollbarEvent sevent;

  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;
  
  if (DBG) fprintf(stderr, "Callback struct 0x%x\n", cbs);fflush(stderr);

  /*nsGUIEvent event;
  nsXtWidget_InitNSEvent(event, p, event, NS_MOUSE_MOVE);
  widgetWindow->DispatchMouseEvent(mevent);
  */
}

//==============================================================
void nsXtWidget_Toggle_ArmCallback(Widget w, XtPointer p, XtPointer call_data)
{
  nsCheckButton * checkBtn = (nsCheckButton *) p ;

  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;
  
  if (DBG) fprintf(stderr, "Callback struct 0x%x\n", cbs);fflush(stderr);
  checkBtn->Armed();

}

//==============================================================
void nsXtWidget_Toggle_DisArmCallback(Widget w, XtPointer p, XtPointer call_data)
{
  if (DBG) fprintf(stderr, "In ***************** nsXtWidget_Scrollbar_Callback\n");
  nsCheckButton * checkBtn = (nsCheckButton *) p ;

  nsScrollbarEvent sevent;

  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;
  
  if (DBG) fprintf(stderr, "Callback struct 0x%x\n", cbs);fflush(stderr);

  checkBtn->DisArmed();
  if (DBG) fprintf(stderr, "Out ***************** nsXtWidget_Scrollbar_Callback\n");
  
}

//==============================================================
void nsXtWidget_RadioButton_ArmCallback(Widget w, XtPointer p, XtPointer call_data)
{
  if (DBG) fprintf(stderr, "In ***************** nsXtWidget_RadioButton_ArmCallback\n");
  nsRadioButton * radioBtn = (nsRadioButton *) p ;

  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;

  if (DBG) fprintf(stderr, "Callback struct 0x%x Set %d\n", cbs, cbs->set);fflush(stderr);
  radioBtn->Armed();

  nsMouseEvent mevent;
  nsXtWidget_InitNSMouseEvent(cbs->event, p, mevent, NS_MOUSE_LEFT_BUTTON_DOWN);
  radioBtn->DispatchMouseEvent(mevent);


}

//==============================================================
void nsXtWidget_RadioButton_DisArmCallback(Widget w, XtPointer p, XtPointer call_data)
{
  if (DBG) fprintf(stderr, "In ***************** nsXtWidget_RadioButton_DisArmCallback\n");
  nsRadioButton * radioBtn = (nsRadioButton *) p ;

  nsScrollbarEvent sevent;

  XmToggleButtonCallbackStruct * cbs = (XmToggleButtonCallbackStruct*)call_data;

  if (DBG) fprintf(stderr, "Callback struct 0x%x  Set %d\n", cbs, cbs->set);fflush(stderr);

  radioBtn->DisArmed();
  if (DBG) fprintf(stderr, "Out ***************** nsXtWidget_Scrollbar_Callback\n");

  nsMouseEvent mevent;
  nsXtWidget_InitNSMouseEvent(cbs->event, p, mevent, NS_MOUSE_LEFT_BUTTON_UP);
  radioBtn->DispatchMouseEvent(mevent);

}


//==============================================================
void nsXtWidget_Scrollbar_Callback(Widget w, XtPointer p, XtPointer call_data)
{
  nsWindow * widgetWindow = (nsWindow *) p ;
  if (DBG) fprintf(stderr, "***************** nsXtWidget_Scrollbar_Callback\n");

  nsScrollbarEvent sevent;

  XmScrollBarCallbackStruct * cbs = (XmScrollBarCallbackStruct*) call_data;
  if (DBG) fprintf(stderr, "Callback struct 0x%x\n", cbs);fflush(stderr);

  sevent.widget  = (nsWindow *) p;
  if (cbs->event != nsnull) {
    sevent.point.x = cbs->event->xbutton.x;
    sevent.point.y = cbs->event->xbutton.y;
  } else {
    sevent.point.x = 0;
    sevent.point.y = 0;
  }
  sevent.time    = 0; //TBD

  switch (cbs->reason) {

    case XmCR_INCREMENT:
      sevent.message = NS_SCROLLBAR_LINE_NEXT;
      break;

    case XmCR_DECREMENT:
      sevent.message = NS_SCROLLBAR_LINE_PREV;
      break;

    case XmCR_PAGE_INCREMENT:
      sevent.message = NS_SCROLLBAR_PAGE_NEXT;
      break;

    case XmCR_PAGE_DECREMENT:
      sevent.message = NS_SCROLLBAR_PAGE_PREV;
      break;

    case XmCR_DRAG:
      sevent.message = NS_SCROLLBAR_POS;
      break;

    case XmCR_VALUE_CHANGED:
      sevent.message = NS_SCROLLBAR_POS;
      break;

    default:
      if (DBG) fprintf(stderr, "In Default processing for scrollbar reason is [%d]\n", cbs->reason);
      break;
  }
  widgetWindow->OnScroll(sevent, cbs->value);
}



//==============================================================
void nsXtWidget_Expose_Callback(Widget w, XtPointer p, XtPointer call_data)
{

  //if (DBG) 
//fprintf(stderr, "In nsXtWidget_Resize_Callback 0x%x", p);
  nsWindow * widgetWindow = (nsWindow *) p ;
  if (widgetWindow == nsnull) {
    return;
  }

  XmDrawingAreaCallbackStruct * cbs = (XmDrawingAreaCallbackStruct *)call_data;
  XEvent * event = cbs->event;
  nsPaintEvent pevent;
  nsRect       rect;
  nsXtWidget_InitNSEvent(event, p, pevent, NS_PAINT);

  pevent.rect = (nsRect *)&rect;

  widgetWindow->OnPaint(pevent);

  if (DBG) fprintf(stderr, "Out nsXtWidget_ExposureMask_EventHandler\n");


}

//==============================================================
void nsXtWidget_Resize_Callback(Widget w, XtPointer p, XtPointer call_data)
{
}

//==============================================================
void nsXtWidget_Text_Callback(Widget w, XtPointer p, XtPointer call_data)
{
  if (DBG) fprintf(stderr, "In nsXtWidget_Text_Callback\n");
  nsWindow * widgetWindow = (nsWindow *) p ;
  int len;

  XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *) call_data;

  PasswordData * data;
  XtVaGetValues(w, XmNuserData, &data, NULL);
  if (data == NULL || data->mIgnore) {
    return;
  }

  if (cbs->reason == XmCR_ACTIVATE) {
    printf ("Password: %s\n", data->mPassword.ToNewCString());
    return;
  }

  //printf("start %d  insert %d  len %d  end %d  ptr [%s]\n", 
         //cbs->startPos, cbs->currInsert, cbs->text->length, cbs->endPos, cbs->text->ptr);

  if (cbs->startPos < cbs->currInsert) {   /* backspace */
      cbs->endPos = data->mPassword.Length();       /* delete from here to end */
      data->mPassword.SetLength(cbs->startPos);           /* backspace--terminate */
  printf("[%s]\n", data == NULL?"<null>":data->mPassword.ToNewCString());
      return;
  }

  //if (cbs->text->length > 1) {
      //cbs->doit = False;  /* don't allow "paste" operations */
      //return;             /* make the user *type* the password! */
  //}

  if (cbs->startPos == cbs->currInsert && cbs->currInsert < data->mPassword.Length()) {
    //printf("Inserting [%s] at %d\n", cbs->text->ptr, cbs->currInsert);
    nsString insStr(cbs->text->ptr);
    data->mPassword.Insert(insStr, cbs->currInsert, strlen(cbs->text->ptr));
  } else if (cbs->startPos == cbs->currInsert && cbs->endPos != cbs->startPos) {
    data->mPassword.SetLength(cbs->startPos);
    printf("Setting Length [%s] at %d\n", cbs->text->ptr, cbs->currInsert);
  } else if (cbs->startPos == cbs->currInsert) {   /* backspace */
    data->mPassword.Append(cbs->text->ptr);
    //printf("Appending [%s] at %d\n", cbs->text->ptr, cbs->currInsert);
  } else {
    printf("Shouldn't be here!\n");
  }
  
  for (len = 0; len < cbs->text->length; len++)
    cbs->text->ptr[len] = '*';

  if (DBG) fprintf(stderr, "Out nsXtWidget_Text_Callback\n");
}

//==============================================================
void nsXtWidget_FSBCancel_Callback(Widget w, XtPointer p, XtPointer call_data)
{
  nsFileWidget * widgetWindow = (nsFileWidget *) p ;
  if (p != nsnull) {
    printf("OnCancel\n");
    widgetWindow->OnCancel();
  }
}

//==============================================================
void nsXtWidget_FSBOk_Callback(Widget w, XtPointer p, XtPointer call_data)
{
  nsFileWidget * widgetWindow = (nsFileWidget *) p ;
  if (p != nsnull) {
    widgetWindow->OnOk();
  }
}

void nsXtWidget_InitNSKeyEvent(int aEventType, nsKeyEvent& aKeyEvent, Widget w, XtPointer p, XEvent * event, Boolean * b)
{
  nsWindow * widgetWindow = (nsWindow *) p ;
  Modifiers modout = 0;
  KeySym res;

  nsXtWidget_InitNSEvent(event, p, aKeyEvent, aEventType);

  XKeyEvent* xKeyEvent =  (XKeyEvent*)event; 
  XtTranslateKeycode(xKeyEvent->display,xKeyEvent->keycode, xKeyEvent->state, &modout, &res);
  aKeyEvent.keyCode   = nsConvertKey(res);
  aKeyEvent.time      = xKeyEvent->time; 
  aKeyEvent.isShift   = modout & ShiftMask; 
  aKeyEvent.isControl = modout & ControlMask;
  aKeyEvent.isAlt     = PR_FALSE; // Fix later
// printf("KEY Event type %d %d shift %d control %d \n", aEventType == NS_KEY_DOWN, aKeyEvent.keyCode, aKeyEvent.isShift, aKeyEvent.isControl);
}

void nsXtWidget_KeyPressMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b)
{
  nsKeyEvent kevent;
  nsXtWidget_InitNSKeyEvent(NS_KEY_DOWN, kevent, w, p, event, b);
  nsWindow * widgetWindow = (nsWindow *) p ;
  widgetWindow->OnKey(NS_KEY_DOWN, kevent.keyCode, &kevent);
}

void nsXtWidget_KeyReleaseMask_EventHandler(Widget w, XtPointer p, XEvent * event, Boolean * b)
{
  nsKeyEvent kevent;
  nsXtWidget_InitNSKeyEvent(NS_KEY_UP, kevent, w, p, event, b);
  nsWindow * widgetWindow = (nsWindow *) p ;
  widgetWindow->OnKey(NS_KEY_UP, kevent.keyCode, &kevent);
}

void nsXtWidget_ResetResize_Callback(XtPointer call_data)
{
    nsWindow* widgetWindow = (nsWindow*)call_data;
    widgetWindow->SetResized(PR_FALSE);
}

