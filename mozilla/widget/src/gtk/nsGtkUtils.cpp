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

#include <unistd.h>
#include <string.h>

#include "nsGtkUtils.h"

#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>

#if defined(__osf__) && !defined(_XOPEN_SOURCE_EXTENDED)
/*
** DEC's compiler requires _XOPEN_SOURCE_EXTENDED to be defined in
** order for it to see the prototype for usleep in unistd.h, but if
** we define that the build breaks long before getting here.  So
** put the prototype here explicitly.
*/
int usleep(useconds_t);
#endif
#if defined(__QNX__)
#define usleep(s)	sleep(s)
#endif

//////////////////////////////////////////////////////////////////
#if 0
/* staitc */ gint
nsGtkUtils::gdk_query_pointer(GdkWindow * window,
							  gint *      x_out,
							  gint *      y_out)
{
  g_return_val_if_fail(NULL != window, FALSE);
  g_return_val_if_fail(NULL != x_out, FALSE);
  g_return_val_if_fail(NULL != y_out, FALSE);

  Window root;
  Window child;
  int rootx, rooty;
  int winx = 0;
  int winy = 0;
  unsigned int xmask = 0;
  gint result = FALSE;
  
  *x_out = -1;
  *y_out = -1;
  
  result = XQueryPointer(GDK_WINDOW_XDISPLAY(window),
                         GDK_WINDOW_XWINDOW(window),
                         &root, 
                         &child,
                         &rootx,
                         &rooty,
                         &winx,
                         &winy,
                         &xmask);
  
  if (result)
  {
    *x_out = rootx;
    *y_out = rooty;
  }

  return result;
}
#endif
//////////////////////////////////////////////////////////////////
/* static */ void 
nsGtkUtils::gtk_widget_set_color(GtkWidget *  widget,
								 GtkRcFlags   flags,
								 GtkStateType state,
								 GdkColor *   color)
{
  GtkRcStyle * rc_style;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (color != NULL);
  g_return_if_fail (flags == 0);

  rc_style = (GtkRcStyle *) gtk_object_get_data (GTK_OBJECT (widget), 
												 "modify-style");

  if (!rc_style)
  {
	rc_style = gtk_rc_style_new ();

	gtk_widget_modify_style (widget, rc_style);

	gtk_object_set_data (GTK_OBJECT (widget), "modify-style", rc_style);
  }

  if (flags & GTK_RC_FG)
  {
    rc_style->color_flags[state] = GtkRcFlags(rc_style->color_flags[state] | GTK_RC_FG);
    rc_style->fg[state] = *color;
  }

  if (flags & GTK_RC_BG)
  {
    rc_style->color_flags[state] = GtkRcFlags(rc_style->color_flags[state] | GTK_RC_BG);
    rc_style->bg[state] = *color;
  }

  if (flags & GTK_RC_TEXT)
  {
    rc_style->color_flags[state] = GtkRcFlags(rc_style->color_flags[state] | GTK_RC_TEXT);
    rc_style->text[state] = *color;
  }

  if (flags & GTK_RC_BASE)
  {
    rc_style->color_flags[state] = GtkRcFlags(rc_style->color_flags[state] | GTK_RC_BASE);
    rc_style->base[state] = *color;
  }
}
//////////////////////////////////////////////////////////////////
/* static */ GdkModifierType
nsGtkUtils::gdk_keyboard_get_modifiers()
{
  GdkModifierType m = (GdkModifierType) 0;

  gdk_window_get_pointer(NULL,NULL,NULL,&m);

  return m;
}
//////////////////////////////////////////////////////////////////
/* static */ void
nsGtkUtils::gdk_window_flash(GdkWindow *    aGdkWindow,
                             unsigned int   aTimes,
                             unsigned long  aInterval,
                             GdkRectangle * aArea)
{
  gint         x;
  gint         y;
  gint         width;
  gint         height;
  guint        i;
  GdkGC *      gc = 0;
  GdkColor     white;

  gdk_window_get_geometry(aGdkWindow,
                          NULL,
                          NULL,
                          &width,
                          &height,
                          NULL);

  gdk_window_get_origin (aGdkWindow,
                         &x,
                         &y);

  gc = gdk_gc_new(GDK_ROOT_PARENT());

  white.pixel = WhitePixel(gdk_display,DefaultScreen(gdk_display));

  gdk_gc_set_foreground(gc,&white);
  gdk_gc_set_function(gc,GDK_XOR);
  gdk_gc_set_subwindow(gc,GDK_INCLUDE_INFERIORS);
  
  /* 
   * If an area is given, use that.  Notice how out of whack coordinates
   * and dimentsions are not checked!!!
   */
  if (aArea)
  {
    x += aArea->x;
    y += aArea->y;
    
    width = aArea->width;
    height = aArea->height;
  }

  /*
   * Need to do this twice so that the XOR effect can replace 
   * the original window contents.
   */
  for (i = 0; i < aTimes * 2; i++)
  {
    gdk_draw_rectangle(GDK_ROOT_PARENT(),
                       gc,
                       TRUE,
                       x,
                       y,
                       width,
                       height);

    gdk_flush();
    
    usleep(aInterval);
  }

  gdk_gc_destroy(gc);
}
//////////////////////////////////////////////////////////////////
