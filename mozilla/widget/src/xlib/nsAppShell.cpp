/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
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

#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "nsWidget.h"
#include "nsAppShell.h"
#include "nsIWidget.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsITimer.h"

static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);

extern "C"
void
xlib_rgb_init (Display *display, Screen *screen);

extern "C"
Visual *
xlib_rgb_get_visual (void);

extern "C" XVisualInfo *
xlib_rgb_get_visual_info (void);

// this is so that we can get the timers in the base.  most widget
// toolkits do this through some set of globals.  not here though.  we
// don't have that luxury
extern "C" int NS_TimeToNextTimeout(struct timeval *);
extern "C" void NS_ProcessTimeouts(void);


// For debugging.
static char *event_names[] = {
  "",
  "",
  "KeyPress",
  "KeyRelease",
  "ButtonPress",
  "ButtonRelease",
  "MotionNotify",
  "EnterNotify",
  "LeaveNotify",
  "FocusIn",
  "FocusOut",
  "KeymapNotify",
  "Expose",
  "GraphicsExpose",
  "NoExpose",
  "VisibilityNotify",
  "CreateNotify",
  "DestroyNotify",
  "UnmapNotify",
  "MapNotify",
  "MapRequest",
  "ReparentNotify",
  "ConfigureNotify",
  "ConfigureRequest",
  "GravityNotify",
  "ResizeRequest",
  "CirculateNotify",
  "CirculateRequest",
  "PropertyNotify",
  "SelectionClear",
  "SelectionRequest",
  "SelectionNotify",
  "ColormapNotify",
  "ClientMessage",
  "MappingNotify"
};

#define ALL_EVENTS ( KeyPressMask | KeyReleaseMask | ButtonPressMask | \
                     ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | \
                     PointerMotionMask | PointerMotionHintMask | Button1MotionMask | \
                     Button2MotionMask | Button3MotionMask | \
                     Button4MotionMask | Button5MotionMask | ButtonMotionMask | \
                     KeymapStateMask | ExposureMask | VisibilityChangeMask | \
                     StructureNotifyMask | ResizeRedirectMask | \
                     SubstructureNotifyMask | SubstructureRedirectMask | \
                     FocusChangeMask | PropertyChangeMask | \
                     ColormapChangeMask | OwnerGrabButtonMask )

NS_IMPL_ADDREF(nsAppShell)
NS_IMPL_RELEASE(nsAppShell)

nsAppShell::nsAppShell()  
{ 
  NS_INIT_REFCNT();
  mDispatchListener = 0;

  mDisplay = nsnull;
  mScreen = nsnull;
}

nsresult nsAppShell::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult result = NS_NOINTERFACE;

  static NS_DEFINE_IID(kIAppShellIID, NS_IAPPSHELL_IID);
  if (result == NS_NOINTERFACE && aIID.Equals(kIAppShellIID)) {
    *aInstancePtr = (void*) ((nsIAppShell*)this);
    NS_ADDREF_THIS();
    result = NS_OK;
  }
  return result;
}


NS_METHOD nsAppShell::Create(int* argc, char ** argv)
{
  // Open the display
  mDisplay = XOpenDisplay(NULL);

  if (mDisplay == NULL) 
    {
    fprintf(stderr, "%s: Cannot connect to X server %s\n",
            argv[0], 
            XDisplayName(NULL));
    
    exit(1);
  }

  _Xdebug = 1;

  mScreen = DefaultScreenOfDisplay(mDisplay);

  xlib_rgb_init(mDisplay, mScreen);

  printf("nsAppShell::Create(dpy=%p  screen=%p)\
n",
           mDisplay,
           mScreen);

  return NS_OK;
}

NS_METHOD nsAppShell::SetDispatchListener(nsDispatchListener* aDispatchListener) 
{
  mDispatchListener = aDispatchListener;
  return NS_OK;
}

nsresult nsAppShell::Run()
{
  nsresult rv = NS_OK;
  nsIEventQueue *EQueue = nsnull;
  int xlib_fd = -1;
  int queue_fd = -1;
  int max_fd;
  fd_set select_set;
  int select_retval;

  // get the event queue service
  rv = nsServiceManager::GetService(kEventQueueServiceCID,
                                    kIEventQueueServiceIID,
                                    (nsISupports **) &mEventQueueService);
  if (NS_OK != rv) {
    NS_ASSERTION("Could not obtain event queue service", PR_FALSE);
    return rv;
  }
  rv = mEventQueueService->GetThreadEventQueue(PR_GetCurrentThread(), &EQueue);
  // If a queue already present use it.
  if (EQueue)
     goto done;

  // Create the event queue for the thread
  rv = mEventQueueService->CreateThreadEventQueue();
  if (NS_OK != rv) {
    NS_ASSERTION("Could not create the thread event queue", PR_FALSE);
    return rv;
  }
  //Get the event queue for the thread
  rv = mEventQueueService->GetThreadEventQueue(PR_GetCurrentThread(), &EQueue);
  if (NS_OK != rv) {
      NS_ASSERTION("Could not obtain the thread event queue", PR_FALSE);
      return rv;
  }    
 done:
  printf("Getting the xlib connection number.\n");
  xlib_fd = ConnectionNumber(mDisplay);
  queue_fd = EQueue->GetEventQueueSelectFD();
  if (xlib_fd >= queue_fd) {
    max_fd = xlib_fd + 1;
  } else {
    max_fd = queue_fd + 1;
  }
  // process events.
  while (1) {
    XEvent          event;
    struct timeval  cur_time;
    struct timeval *cur_time_ptr;
    int             please_run_timer_queue = 0;
    gettimeofday(&cur_time, NULL);
    FD_ZERO(&select_set);
    // add the queue and the xlib connection to the select set
    FD_SET(queue_fd, &select_set);
    FD_SET(xlib_fd, &select_set);
    if (NS_TimeToNextTimeout(&cur_time) == 0) {
      cur_time_ptr = NULL;
    }
    else {
      cur_time_ptr = &cur_time;
      // check to see if something is waiting right now
      if ((cur_time.tv_sec == 0) && (cur_time.tv_usec == 0)) {
        please_run_timer_queue = 1;
      }
    }
    // note that we are passing in the timeout_ptr from above.
    // if there are no timers, this will be null and this will
    // block until hell freezes over
    select_retval = select(max_fd, &select_set, NULL, NULL, cur_time_ptr);
    if (select_retval == -1) {
      printf("Select returned error.\n");
      return NS_ERROR_FAILURE;
    }
    if (select_retval == 0) {
      // the select timed out, process the timeout queue.
      //      printf("Timer ran out...\n");
      please_run_timer_queue = 1;
    }
    // check to see if there's data avilable for the queue
    if (FD_ISSET(queue_fd, &select_set)) {
      //printf("queue data available.\n");
      EQueue->ProcessPendingEvents();
    }
    // check to see if there's data avilable for
    // xlib
    if (FD_ISSET(xlib_fd, &select_set)) {
      //printf("xlib data available.\n");
      //XNextEvent(mDisplay, &event);
      while (XCheckMaskEvent(mDisplay, ALL_EVENTS, &event)) {
        DispatchEvent(&event);
      }
    }
    if (please_run_timer_queue) {
      //printf("Running timer queue...\n");
      NS_ProcessTimeouts();
    }
    // make sure that any pending X requests are flushed.
    XFlush(mDisplay);
  }

	NS_IF_RELEASE(EQueue);
  return rv;
}

NS_METHOD
nsAppShell::GetNativeEvent(PRBool &aRealEvent, void *&aEvent)
{
  return NS_OK;
}

NS_METHOD
nsAppShell::EventIsForModalWindow(PRBool aRealEvent, void *aEvent,
                            nsIWidget *aWidget, PRBool *aForWindow)
{
  return NS_OK;
}

nsresult nsAppShell::DispatchNativeEvent(PRBool aRealEvent, void *aEvent)
{
  return NS_OK;
}

NS_METHOD nsAppShell::Exit()
{
  return NS_OK;
}

nsAppShell::~nsAppShell()
{
}

void* nsAppShell::GetNativeData(PRUint32 aDataType)
{
  return nsnull;
}

void
nsAppShell::DispatchEvent(XEvent *event)
{
  nsWidget *widget;
  widget = nsWidget::GetWidgetForWindow(event->xany.window);
  // switch on the type of event
  switch (event->type) {
  case Expose:
    HandleExposeEvent(event, widget);
    break;
  case ConfigureNotify:
    // we need to make sure that this is the LAST of the
    // config events.
    while (XCheckWindowEvent(mDisplay, event->xany.window, StructureNotifyMask, event) == True);
    HandleConfigureNotifyEvent(event, widget);
    break;
  case ButtonPress:
  case ButtonRelease:
    HandleButtonEvent(event, widget);
    break;
  case MotionNotify:
    HandleMotionNotifyEvent(event, widget);
    break;
  default:
    printf("Unhandled window event: Window 0x%lx Got a %s event\n",
           event->xany.window, event_names[event->type]);
    break;
  }
}

void
nsAppShell::HandleMotionNotifyEvent(XEvent *event, nsWidget *aWidget)
{
  nsMouseEvent mevent;
  mevent.message = NS_MOUSE_MOVE;
  mevent.widget = aWidget;
  mevent.eventStructType = NS_MOUSE_EVENT;
  mevent.point.x = event->xmotion.x;
  mevent.point.y = event->xmotion.y;
  mevent.time = 0;
  NS_ADDREF(aWidget);
  aWidget->DispatchMouseEvent(mevent);
  NS_RELEASE(aWidget);
}

void
nsAppShell::HandleButtonEvent(XEvent *event, nsWidget *aWidget)
{
  nsMouseEvent mevent;
  PRUint32 eventType = 0;
  printf("Button event for window 0x%lx button %d type %s\n",
         event->xany.window, event->xbutton.button, (event->type == ButtonPress ? "ButtonPress" : "ButtonRelease"));
  switch(event->type) {
  case ButtonPress:
    switch(event->xbutton.button) {
    case 1:
      eventType = NS_MOUSE_LEFT_BUTTON_DOWN;
      break;
    case 2:
      eventType = NS_MOUSE_MIDDLE_BUTTON_DOWN;
      break;
    case 3:
      eventType = NS_MOUSE_RIGHT_BUTTON_DOWN;
      break;
    }
    break;
  case ButtonRelease:
    switch(event->xbutton.button) {
    case 1:
      eventType = NS_MOUSE_LEFT_BUTTON_UP;
      break;
    case 2:
      eventType = NS_MOUSE_MIDDLE_BUTTON_UP;
      break;
    case 3:
      eventType = NS_MOUSE_RIGHT_BUTTON_UP;
      break;
    }
    break;
  }
  mevent.message = eventType;
  mevent.widget = aWidget;
  mevent.eventStructType = NS_MOUSE_EVENT;
  mevent.point.x = event->xbutton.x;
  mevent.point.y = event->xbutton.y;
  // XXX fix this.  We need clicks
  mevent.clickCount = 1;
  mevent.time = 0;
  NS_ADDREF(aWidget);
  aWidget->DispatchMouseEvent(mevent);
  NS_ADDREF(aWidget);
}

void
nsAppShell::HandleExposeEvent(XEvent *event, nsWidget *aWidget)
{
  printf("Expose event for window 0x%lx %d %d %d %d\n", event->xany.window,
         event->xexpose.x, event->xexpose.y, event->xexpose.width, event->xexpose.height);
  nsPaintEvent pevent;
  pevent.message = NS_PAINT;
  pevent.widget = aWidget;
  pevent.eventStructType = NS_PAINT_EVENT;
  pevent.rect = new nsRect (event->xexpose.x, event->xexpose.y,
                            event->xexpose.width, event->xexpose.height);
  // XXX fix this
  pevent.time = 0;
  NS_ADDREF(aWidget);
  aWidget->OnPaint(pevent);
  NS_RELEASE(aWidget);
  delete pevent.rect;
}

void
nsAppShell::HandleConfigureNotifyEvent(XEvent *event, nsWidget *aWidget)
{
  printf("ConfigureNotify event for window 0x%lx %d %d %d %d\n",
         event->xconfigure.window,
         event->xconfigure.x, event->xconfigure.y,
         event->xconfigure.width, event->xconfigure.height);
  nsSizeEvent sevent;
  sevent.message = NS_SIZE;
  sevent.widget = aWidget;
  sevent.eventStructType = NS_SIZE_EVENT;
  sevent.windowSize = new nsRect (event->xconfigure.x, event->xconfigure.y,
                                  event->xconfigure.width, event->xconfigure.height);
  sevent.point.x = event->xconfigure.x;
  sevent.point.y = event->xconfigure.y;
  sevent.mWinWidth = event->xconfigure.width;
  sevent.mWinHeight = event->xconfigure.height;
  // XXX fix this
  sevent.time = 0;
  NS_ADDREF(aWidget);
  aWidget->OnResize(sevent);
  NS_RELEASE(aWidget);
  delete sevent.windowSize;
}
