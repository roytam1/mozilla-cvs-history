/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "MPL"); you may not use this file except in
 * compliance with the MPL.  You may obtain a copy of the MPL at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the MPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the MPL
 * for the specific language governing rights and limitations under the
 * MPL.
 *
 * The Initial Developers of this code under the MPL are Owen Taylor
 * <otaylor@redhat.com> and Christopher Blizzard <blizzard@redhat.com>.
 * Portions created by the Initial Developers are Copyright (C) 1999
 * Owen Taylor and Christopher Blizzard.  All Rights Reserved.
 */

#ifndef __GDK_SUPERWIN_H__
#define __GDK_SUPERWIN_H__

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtkobject.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _GdkSuperWin GdkSuperWin;
typedef struct _GdkSuperWinClass GdkSuperWinClass;

#define GDK_TYPE_SUPERWIN            (gdk_superwin_get_type())
#define GDK_SUPERWIN(obj)            (GTK_CHECK_CAST((obj), GDK_TYPE_SUPERWIN, GdkSuperWin))
#define GDK_SUPERWIN_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), GDK_TYPE_SUPERWIN, GdkSuperWinClass))
#define GDK_IS_SUPERWIN(obj)         (GTK_CHECK_TYPE((obj), GDK_TYPE_SUPERWIN))
#define GDK_IS_SUPERWIN_CLASS(klass) (GTK_CHECK_CLASS_TYPE((klass), GDK_TYPE_SUPERWIN))

typedef void (*GdkSuperWinFunc) (GdkSuperWin *super_win,
				 XEvent      *event,
				 gpointer     data);

struct _GdkSuperWin
{
  GtkObject object;
  GdkWindow *shell_window;
  GdkWindow *bin_window;

  /* Private */
  GList *translate_queue;
  
  GdkSuperWinFunc event_func;
  gpointer        func_data;
  GDestroyNotify  notify;

  GdkVisibilityState   visibility;
};

struct _GdkSuperWinClass
{
  GtkObjectClass object_class;
};

GtkType gdk_superwin_get_type(void);

GdkSuperWin *gdk_superwin_new    (GdkWindow      *parent_window,
				  guint           x,
				  guint           y,
				  guint           width,
				  guint           height);

void         gdk_superwin_set_event_func (GdkSuperWin    *superwin,
					  GdkSuperWinFunc event_func,
					  gpointer        func_data,
					  GDestroyNotify  notify);
void         gdk_superwin_scroll (GdkSuperWin *superwin,
				  gint         dx,
				  gint         dy);
void         gdk_superwin_resize (GdkSuperWin *superwin,
				  gint         width,
				  gint         height);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GDK_SUPERWIN_H__ */

