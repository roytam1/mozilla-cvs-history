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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsXUtils.h"

#include <unistd.h>
#include <string.h>

#if defined(__osf__) && !defined(_XOPEN_SOURCE_EXTENDED)
/*
** DEC's compiler requires _XOPEN_SOURCE_EXTENDED to be defined in
** order for it to see the prototype for usleep in unistd.h, but if
** we define that the build breaks long before getting here.  So
** put the prototype here explicitly.
*/
int usleep(useconds_t);
#endif

//////////////////////////////////////////////////////////////////
/* static */ void
nsXUtils::XFlashWindow(Display *       aDisplay,
                       Window          aWindow,
                       unsigned int    aTimes,
                       unsigned long   aInterval,
                       XRectangle *    aArea)
{
  Window       root_window = 0;
  Window       child_window = 0;
  GC           gc;
  int          x;
  int          y;
  unsigned int width;
  unsigned int height;
  unsigned int border_width;
  unsigned int depth;
  int          root_x;
  int          root_y;
  unsigned int i;
  XGCValues    gcv;
  
  XGetGeometry(aDisplay,
               aWindow,
               &root_window,
               &x,
               &y,
               &width,
               &height,
               &border_width,
               &depth);
  
  XTranslateCoordinates(aDisplay, 
                        aWindow,
                        root_window, 
                        0, 
                        0,
                        &root_x,
                        &root_y,
                        &child_window);
  
  memset(&gcv, 0, sizeof(XGCValues));
  
  gcv.function = GXxor;
  gcv.foreground = WhitePixel(aDisplay, DefaultScreen(aDisplay));
  gcv.subwindow_mode = IncludeInferiors;
  
  if (gcv.foreground == 0)
    gcv.foreground = 1;
  
  gc = XCreateGC(aDisplay,
                 root_window,
                 GCFunction | GCForeground | GCSubwindowMode, 
                 &gcv);
  
  XGrabServer(aDisplay);

  // If an area is given, use that.  Notice how out of whack coordinates
  // and dimentsions are not checked!!!
  if (aArea)
  {
	root_x += aArea->x;
	root_y += aArea->y;

	width = aArea->width;
	height = aArea->height;
  }

  // Need to do this twice so that the XOR effect can replace 
  // the original window contents.
  for (i = 0; i < aTimes * 2; i++)
  {
	XFillRectangle(aDisplay,
				   root_window,
				   gc,
				   root_x,
				   root_y,
				   width,
				   height);
	
	XSync(aDisplay, False);
	
	usleep(aInterval);
  }
  
  
  XFreeGC(aDisplay, gc);  
  
  XUngrabServer(aDisplay);
}
/*****************************************************************/
