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

#ifndef __GTK_MOZAREA_H__
#define __GTK_MOZAREA_H__

#include <gtk/gtkwindow.h>
#include "gdksuperwin.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _GtkMozArea GtkMozArea;
typedef struct _GtkMozAreaClass GtkMozAreaClass;

#define GTK_TYPE_MOZAREA                  (gtk_mozarea_get_type ())
#define GTK_MOZAREA(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_MOZAREA, GtkMozArea))
#define GTK_MOZAREA_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_MOZAREA, GtkMozAreaClass))
#define GTK_IS_MOZAREA(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_MOZAREA))
#define GTK_IS_MOZAREA_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MOZAREA))

struct _GtkMozArea
{
  GtkWidget widget;
  GdkSuperWin *superwin;
};
  
struct _GtkMozAreaClass
{
  GtkWindowClass window_class;
};

GtkType    gtk_mozarea_get_type (void);
GtkWidget *gtk_mozarea_new ();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_MOZAREA_H__ */
