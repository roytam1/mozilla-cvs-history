/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Christopher Blizzard.
 * Portions created by Christopher Blizzard are Copyright (C)
 * Christopher Blizzard.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Christopher Blizzard <blizzard@mozilla.org>
 *   Ramiro Estrugo <ramiro@eazel.com>
 */

#include <stdio.h>

#include "gtkmozembed.h"
#include "gtkmozembedprivate.h"
#include "gtkmozembed_internal.h"

#include "EmbedPrivate.h"
#include "EmbedWindow.h"

// so we can do our get_nsIWebBrowser later...
#include <nsIWebBrowser.h>

// so we can get callbacks from the mozarea
#include <gtkmozarea.h>

// Some "massaged" enum information for the GTK Type System
static GtkFlagValue gtk_moz_embed_progress_flags_values[] = {
  { GTK_MOZ_EMBED_FLAG_START,
    "GTK_MOZ_EMBED_FLAG_START", "start" },
  { GTK_MOZ_EMBED_FLAG_REDIRECTING,
    "GTK_MOZ_EMBED_FLAG_REDIRECTING", "redirecting" },
  { GTK_MOZ_EMBED_FLAG_TRANSFERRING,
    "GTK_MOZ_EMBED_FLAG_TRANSFERRING", "transferring" },
  { GTK_MOZ_EMBED_FLAG_NEGOTIATING,
    "GTK_MOZ_EMBED_FLAG_NEGOTIATING", "negotiating" },
  { GTK_MOZ_EMBED_FLAG_STOP,
    "GTK_MOZ_EMBED_FLAG_STOP", "stop" },
  { GTK_MOZ_EMBED_FLAG_IS_REQUEST,
    "GTK_MOZ_EMBED_FLAG_IS_REQUEST", "is-request" },
  { GTK_MOZ_EMBED_FLAG_IS_DOCUMENT,
    "GTK_MOZ_EMBED_FLAG_IS_DOCUMENT", "is-document" },
  { GTK_MOZ_EMBED_FLAG_IS_NETWORK,
    "GTK_MOZ_EMBED_FLAG_IS_NETWORK", "is-network" },
  { GTK_MOZ_EMBED_FLAG_IS_WINDOW,
    "GTK_MOZ_EMBED_FLAG_IS_WINDOW", "is-window" },
  { 0,
    NULL, NULL }
};

static GtkEnumValue gtk_moz_embed_status_enums_values[] = {
  { GTK_MOZ_EMBED_STATUS_FAILED_DNS,
    "GTK_MOZ_EMBED_STATUS_FAILED_DNS", "failed-dns" },
  { GTK_MOZ_EMBED_STATUS_FAILED_CONNECT,
    "GTK_MOZ_EMBED_STATUS_FAILED_CONNECT", "failed-connect" },
  { GTK_MOZ_EMBED_STATUS_FAILED_TIMEOUT,
    "GTK_MOZ_EMBED_STATUS_FAILED_TIMEOUT", "failed-timeout" },
  { GTK_MOZ_EMBED_STATUS_FAILED_USERCANCELED,
    "GTK_MOZ_EMBED_STATUS_FAILED_USERCANCELED", "failed-usercanceled" },
  { 0,
    NULL, NULL }
};

static GtkFlagValue gtk_moz_embed_reload_flags_values[] = {
  { GTK_MOZ_EMBED_FLAG_RELOADNORMAL,
    "GTK_MOZ_EMBED_FLAG_RELOADNORMAL", "reloadnormal" },
  { GTK_MOZ_EMBED_FLAG_RELOADBYPASSCACHE,
    "GTK_MOZ_EMBED_FLAG_RELOADBYPASSCACHE", "reloadbypasscache" },
  { GTK_MOZ_EMBED_FLAG_RELOADBYPASSPROXY,
    "GTK_MOZ_EMBED_FLAG_RELOADBYPASSPROXY", "reloadbypassproxy" },
  { GTK_MOZ_EMBED_FLAG_RELOADBYPASSPROXYANDCACHE,
    "GTK_MOZ_EMBED_FLAG_RELOADBYPASSPROXYANDCACHE",
    "reloadbypassproxyandcache" },
  { 0,
    NULL, NULL }
};

static GtkFlagValue gtk_moz_embed_chrome_flags_values[] = {
  { GTK_MOZ_EMBED_FLAG_DEFAULTCHROME, 
    "GTK_MOZ_EMBED_FLAG_DEFAULTCHROME", "defaultchrome" },
  { GTK_MOZ_EMBED_FLAG_WINDOWBORDERSON, 
    "GTK_MOZ_EMBED_FLAG_WINDOWBORDERSON", "windowborderson" },
  { GTK_MOZ_EMBED_FLAG_WINDOWCLOSEON,
    "GTK_MOZ_EMBED_FLAG_WINDOWCLOSEON", "windowcloseon" },
  { GTK_MOZ_EMBED_FLAG_WINDOWRESIZEON,
    "GTK_MOZ_EMBED_FLAG_WINDOWRESIZEON", "windowresizeon" },
  { GTK_MOZ_EMBED_FLAG_MENUBARON, 
    "GTK_MOZ_EMBED_FLAG_MENUBARON", "menubaron" },
  { GTK_MOZ_EMBED_FLAG_TOOLBARON,
    "GTK_MOZ_EMBED_FLAG_TOOLBARON", "toolbaron" },
  { GTK_MOZ_EMBED_FLAG_LOCATIONBARON,
    "GTK_MOZ_EMBED_FLAG_LOCATIONBARON", "locationbaron" },
  { GTK_MOZ_EMBED_FLAG_STATUSBARON,
    "GTK_MOZ_EMBED_FLAG_STATUSBARON", "statusbaron" },
  { GTK_MOZ_EMBED_FLAG_PERSONALTOOLBARON,
    "GTK_MOZ_EMBED_FLAG_PERSONALTOOLBARON", "personaltoolbaron" },
  { GTK_MOZ_EMBED_FLAG_SCROLLBARSON,
    "GTK_MOZ_EMBED_FLAG_SCROLLBARSON", "scrollbarson" },
  { GTK_MOZ_EMBED_FLAG_TITLEBARON, 
    "GTK_MOZ_EMBED_FLAG_TITLEBARON", "titlebaron" },
  { GTK_MOZ_EMBED_FLAG_EXTRACHROMEON,
    "GTK_MOZ_EMBED_FLAG_EXTRACHROMEON", "extrachromeon" },
  { GTK_MOZ_EMBED_FLAG_ALLCHROME, 
    "GTK_MOZ_EMBED_FLAG_ALLCHROME", "allchrome" },
  { GTK_MOZ_EMBED_FLAG_WINDOWRAISED, 
    "GTK_MOZ_EMBED_FLAG_WINDOWRAISED", "windowraised" },
  { GTK_MOZ_EMBED_FLAG_WINDOWLOWERED,
    "GTK_MOZ_EMBED_FLAG_WINDOWLOWERED", "windowlowered" },
  { GTK_MOZ_EMBED_FLAG_CENTERSCREEN,
    "GTK_MOZ_EMBED_FLAG_CENTERSCREEN", "centerscreen" },
  { GTK_MOZ_EMBED_FLAG_DEPENDENT,
    "GTK_MOZ_EMBED_FLAG_DEPENDENT", "dependent" },
  { GTK_MOZ_EMBED_FLAG_MODAL,
    "GTK_MOZ_EMBED_FLAG_MODAL", "modal" },
  { GTK_MOZ_EMBED_FLAG_OPENASDIALOG,
    "GTK_MOZ_EMBED_FLAG_OPENASDIALOG", "openasdialog" },
  { GTK_MOZ_EMBED_FLAG_OPENASCHROME, 
    "GTK_MOZ_EMBED_FLAG_OPENASCHROME", "openaschrome" },
  { 0,
    NULL, NULL }
};


// class and instance initialization

static void
gtk_moz_embed_class_init(GtkMozEmbedClass *klass);

static void
gtk_moz_embed_init(GtkMozEmbed *embed);

// GtkObject methods

static void
gtk_moz_embed_destroy(GtkObject *object);

// GtkWidget methods

static void
gtk_moz_embed_realize(GtkWidget *widget);

static void
gtk_moz_embed_unrealize(GtkWidget *widget);

static void
gtk_moz_embed_size_allocate(GtkWidget *widget, GtkAllocation *allocation);

static void
gtk_moz_embed_map(GtkWidget *widget);

static void
gtk_moz_embed_unmap(GtkWidget *widget);

static gint
handle_child_focus_in(GtkWidget     *aWidget,
		      GdkEventFocus *aGdkFocusEvent,
		      GtkMozEmbed   *aEmbed);

static gint
handle_child_focus_out(GtkWidget     *aWidget,
		       GdkEventFocus *aGdkFocusEvent,
		       GtkMozEmbed   *aEmbed);

// signal handlers for tracking the focus and and focus out events on
// the toplevel window.

static void
handle_toplevel_focus_in (GtkMozArea    *aArea,
			  GtkMozEmbed   *aEmbed);
static void
handle_toplevel_focus_out(GtkMozArea    *aArea,
			  GtkMozEmbed   *aEmbed);

// globals for this type of widget

static GtkBinClass *parent_class;

guint moz_embed_signals[LAST_SIGNAL] = { 0 };

// GtkObject + class-related functions

GtkType
gtk_moz_embed_get_type(void)
{
  static GtkType moz_embed_type = 0;
  if (!moz_embed_type)
  {
    static const GtkTypeInfo moz_embed_info =
    {
      "GtkMozEmbed",
      sizeof(GtkMozEmbed),
      sizeof(GtkMozEmbedClass),
      (GtkClassInitFunc)gtk_moz_embed_class_init,
      (GtkObjectInitFunc)gtk_moz_embed_init,
      0,
      0,
      0
    };
    moz_embed_type = gtk_type_unique(GTK_TYPE_BIN, &moz_embed_info);
  }

  return moz_embed_type;
}

static void
gtk_moz_embed_class_init(GtkMozEmbedClass *klass)
{
  GtkContainerClass  *container_class;
  GtkBinClass        *bin_class;
  GtkWidgetClass     *widget_class;
  GtkObjectClass     *object_class;
  
  container_class = GTK_CONTAINER_CLASS(klass);
  bin_class       = GTK_BIN_CLASS(klass);
  widget_class    = GTK_WIDGET_CLASS(klass);
  object_class    = GTK_OBJECT_CLASS(klass);

  parent_class = (GtkBinClass *)gtk_type_class(gtk_bin_get_type());

  widget_class->realize = gtk_moz_embed_realize;
  widget_class->unrealize = gtk_moz_embed_unrealize;
  widget_class->size_allocate = gtk_moz_embed_size_allocate;
  widget_class->map = gtk_moz_embed_map;
  widget_class->unmap = gtk_moz_embed_unmap;

  object_class->destroy = gtk_moz_embed_destroy;


  
  // set up our signals

  moz_embed_signals[LINK_MESSAGE] = 
    gtk_signal_new ("link_message",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET(GtkMozEmbedClass, link_message),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE, 0);
  moz_embed_signals[JS_STATUS] =
    gtk_signal_new ("js_status",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET(GtkMozEmbedClass, js_status),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE, 0);
  moz_embed_signals[LOCATION] =
    gtk_signal_new ("location",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET(GtkMozEmbedClass, location),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE, 0);
  moz_embed_signals[TITLE] = 
    gtk_signal_new("title",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, title),
		   gtk_marshal_NONE__NONE,
		   GTK_TYPE_NONE, 0);
  moz_embed_signals[PROGRESS] =
    gtk_signal_new("progress",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, progress),
		   gtk_marshal_NONE__INT_INT,
		   GTK_TYPE_NONE, 2, GTK_TYPE_INT, GTK_TYPE_INT);
  moz_embed_signals[PROGRESS_ALL] = 
    gtk_signal_new("progress_all",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, progress_all),
		   gtk_marshal_NONE__POINTER_INT_INT,
		   GTK_TYPE_NONE, 3, GTK_TYPE_STRING,
		   GTK_TYPE_INT, GTK_TYPE_INT);
  moz_embed_signals[NET_STATE] =
    gtk_signal_new("net_state",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, net_state),
		   gtk_marshal_NONE__INT_INT,
		   GTK_TYPE_NONE, 2, GTK_TYPE_INT, GTK_TYPE_UINT);
  moz_embed_signals[NET_STATE_ALL] =
    gtk_signal_new("net_state_all",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, net_state_all),
		   gtk_marshal_NONE__POINTER_INT_INT,
		   GTK_TYPE_NONE, 3, GTK_TYPE_STRING,
		   GTK_TYPE_INT, GTK_TYPE_UINT);
  moz_embed_signals[NET_START] =
    gtk_signal_new("net_start",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, net_start),
		   gtk_marshal_NONE__NONE,
		   GTK_TYPE_NONE, 0);
  moz_embed_signals[NET_STOP] =
    gtk_signal_new("net_stop",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, net_stop),
		   gtk_marshal_NONE__NONE,
		   GTK_TYPE_NONE, 0);
  moz_embed_signals[NEW_WINDOW] =
    gtk_signal_new("new_window",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, new_window),
		   gtk_marshal_NONE__POINTER_UINT,
		   GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_UINT);
  moz_embed_signals[VISIBILITY] =
    gtk_signal_new("visibility",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, visibility),
		   gtk_marshal_NONE__BOOL,
		   GTK_TYPE_NONE, 1, GTK_TYPE_BOOL);
  moz_embed_signals[DESTROY_BROWSER] =
    gtk_signal_new("destroy_browser",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, destroy_brsr),
		   gtk_marshal_NONE__NONE,
		   GTK_TYPE_NONE, 0);
  moz_embed_signals[OPEN_URI] = 
    gtk_signal_new("open_uri",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, open_uri),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_STRING);
  moz_embed_signals[SIZE_TO] =
    gtk_signal_new("size_to",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, size_to),
		   gtk_marshal_NONE__INT_INT,
		   GTK_TYPE_NONE, 2, GTK_TYPE_INT, GTK_TYPE_INT);
  moz_embed_signals[DOM_KEY_DOWN] =
    gtk_signal_new("dom_key_down",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, dom_key_down),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  moz_embed_signals[DOM_KEY_PRESS] =
    gtk_signal_new("dom_key_press",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, dom_key_press),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  moz_embed_signals[DOM_KEY_UP] =
    gtk_signal_new("dom_key_up",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, dom_key_up),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  moz_embed_signals[DOM_MOUSE_DOWN] =
    gtk_signal_new("dom_mouse_down",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, dom_mouse_down),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  moz_embed_signals[DOM_MOUSE_UP] =
    gtk_signal_new("dom_mouse_up",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, dom_mouse_up),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  moz_embed_signals[DOM_MOUSE_CLICK] =
    gtk_signal_new("dom_mouse_click",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, dom_mouse_click),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  moz_embed_signals[DOM_MOUSE_DBL_CLICK] =
    gtk_signal_new("dom_mouse_dbl_click",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, dom_mouse_dbl_click),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  moz_embed_signals[DOM_MOUSE_OVER] =
    gtk_signal_new("dom_mouse_over",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, dom_mouse_over),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  moz_embed_signals[DOM_MOUSE_OUT] =
    gtk_signal_new("dom_mouse_out",
		   GTK_RUN_LAST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(GtkMozEmbedClass, dom_mouse_out),
		   gtk_marshal_BOOL__POINTER,
		   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);

  gtk_object_class_add_signals(object_class, moz_embed_signals, LAST_SIGNAL);

}

static void
gtk_moz_embed_init(GtkMozEmbed *embed)
{
  EmbedPrivate *priv = new EmbedPrivate();
  embed->data = priv;
}

GtkWidget *
gtk_moz_embed_new(void)
{
  return GTK_WIDGET(gtk_type_new(gtk_moz_embed_get_type()));
}

// GtkObject methods

static void
gtk_moz_embed_destroy(GtkObject *object)
{
  GtkMozEmbed  *embed;
  EmbedPrivate *embedPrivate;
  
  g_return_if_fail(object != NULL);
  g_return_if_fail(GTK_IS_MOZ_EMBED(object));

  embed = GTK_MOZ_EMBED(object);
  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate) {
    delete embedPrivate;
    embed->data = NULL;
  }
}

// GtkWidget methods

static void
gtk_moz_embed_realize(GtkWidget *widget)
{
  GtkMozEmbed    *embed;
  EmbedPrivate   *embedPrivate;
  GdkWindowAttr   attributes;
  gint            attributes_mask;
  
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_EMBED(widget));

  embed = GTK_MOZ_EMBED(widget);
  embedPrivate = (EmbedPrivate *)embed->data;

  GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, embed);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

  // initialize the widget now that we have a window
  nsresult rv;
  rv = embedPrivate->Init(embed);
  g_return_if_fail(NS_SUCCEEDED(rv));
  rv = embedPrivate->Realize();
  g_return_if_fail(NS_SUCCEEDED(rv));

  if (embedPrivate->mURI.Length())
    embedPrivate->LoadCurrentURI();


  // connect to the focus out event for the child
  GtkWidget *child_widget = GTK_BIN(widget)->child;
  gtk_signal_connect_while_alive(GTK_OBJECT(child_widget),
				 "focus_out_event",
				 GTK_SIGNAL_FUNC(handle_child_focus_out),
				 embed,
				 GTK_OBJECT(child_widget));
  gtk_signal_connect_while_alive(GTK_OBJECT(child_widget),
				 "focus_in_event",
				 GTK_SIGNAL_FUNC(handle_child_focus_in),
				 embed,
				 GTK_OBJECT(child_widget));

  // connect to the toplevel focus out events for the child
  GtkMozArea *mozarea = GTK_MOZAREA(child_widget);
  gtk_signal_connect_while_alive(GTK_OBJECT(mozarea),
				 "toplevel_focus_in",
				 GTK_SIGNAL_FUNC(handle_toplevel_focus_in),
				 embed,
				 GTK_OBJECT(mozarea));

  gtk_signal_connect_while_alive(GTK_OBJECT(mozarea),
				 "toplevel_focus_out",
				 GTK_SIGNAL_FUNC(handle_toplevel_focus_out),
				 embed,
				 GTK_OBJECT(mozarea));
}

static void
gtk_moz_embed_unrealize(GtkWidget *widget)
{
  GtkMozEmbed  *embed;
  EmbedPrivate *embedPrivate;
  
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_EMBED(widget));

  embed = GTK_MOZ_EMBED(widget);
  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate) {
    embedPrivate->Unrealize();
  }
}

static void
gtk_moz_embed_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkMozEmbed  *embed;
  EmbedPrivate *embedPrivate;
  
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_EMBED(widget));

  embed = GTK_MOZ_EMBED(widget);
  embedPrivate = (EmbedPrivate *)embed->data;
  
  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED(widget))
  {
    gdk_window_move_resize(widget->window,
			   allocation->x, allocation->y,
			   allocation->width, allocation->height);
    embedPrivate->Resize(allocation->width, allocation->height);
  }
}

static void
gtk_moz_embed_map(GtkWidget *widget)
{
  GtkMozEmbed  *embed;
  EmbedPrivate *embedPrivate;
  
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_EMBED(widget));

  embed = GTK_MOZ_EMBED(widget);
  embedPrivate = (EmbedPrivate *)embed->data;

  GTK_WIDGET_SET_FLAGS(widget, GTK_MAPPED);

  embedPrivate->Show();

  gdk_window_show(widget->window);
  
}

static void
gtk_moz_embed_unmap(GtkWidget *widget)
{
  GtkMozEmbed  *embed;
  EmbedPrivate *embedPrivate;
  
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_EMBED(widget));

  embed = GTK_MOZ_EMBED(widget);
  embedPrivate = (EmbedPrivate *)embed->data;

  GTK_WIDGET_UNSET_FLAGS(widget, GTK_MAPPED);

  gdk_window_hide(widget->window);

  embedPrivate->Hide();
}

static gint
handle_child_focus_in(GtkWidget     *aWidget,
		      GdkEventFocus *aGdkFocusEvent,
		      GtkMozEmbed   *aEmbed)
{
  EmbedPrivate *embedPrivate;

  embedPrivate = (EmbedPrivate *)aEmbed->data;

  embedPrivate->ChildFocusIn();

  return FALSE;
}

static gint
handle_child_focus_out(GtkWidget     *aWidget,
		       GdkEventFocus *aGdkFocusEvent,
		       GtkMozEmbed   *aEmbed)
{
  EmbedPrivate *embedPrivate;

  embedPrivate = (EmbedPrivate *)aEmbed->data;

  embedPrivate->ChildFocusOut();
 
  return FALSE;
}

static void
handle_toplevel_focus_in (GtkMozArea    *aArea,
			  GtkMozEmbed   *aEmbed)
{
  EmbedPrivate   *embedPrivate;
  embedPrivate = (EmbedPrivate *)aEmbed->data;

  embedPrivate->TopLevelFocusIn();
}

static void
handle_toplevel_focus_out(GtkMozArea    *aArea,
			  GtkMozEmbed   *aEmbed)
{
  EmbedPrivate   *embedPrivate;
  embedPrivate = (EmbedPrivate *)aEmbed->data;

  embedPrivate->TopLevelFocusOut();
}

// Widget methods

void
gtk_moz_embed_push_startup(void)
{
  EmbedPrivate::PushStartup();
}

void
gtk_moz_embed_pop_startup(void)
{
  EmbedPrivate::PopStartup();
}

void
gtk_moz_embed_set_comp_path(char *aPath)
{
  EmbedPrivate::SetCompPath(aPath);
}

void
gtk_moz_embed_set_profile_path(char *aDir, char *aName)
{
  EmbedPrivate::SetProfilePath(aDir, aName);
}

void
gtk_moz_embed_load_url(GtkMozEmbed *embed, const char *url)
{
  EmbedPrivate *embedPrivate;
  
  g_return_if_fail(embed != NULL);
  g_return_if_fail(GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;

  embedPrivate->SetURI(url);

  // If the widget is realized, load the URI.  If it isn't then we
  // will load it later.
  if (GTK_WIDGET_REALIZED(embed))
    embedPrivate->LoadCurrentURI();
}

void
gtk_moz_embed_stop_load(GtkMozEmbed *embed)
{
  EmbedPrivate *embedPrivate;
  
  g_return_if_fail(embed != NULL);
  g_return_if_fail(GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate->mNavigation)
    embedPrivate->mNavigation->Stop();
}

gboolean
gtk_moz_embed_can_go_back(GtkMozEmbed *embed)
{
  PRBool retval = PR_FALSE;
  EmbedPrivate *embedPrivate;

  g_return_val_if_fail ((embed != NULL), FALSE);
  g_return_val_if_fail (GTK_IS_MOZ_EMBED(embed), FALSE);

  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate->mNavigation)
    embedPrivate->mNavigation->GetCanGoBack(&retval);
  return retval;
}

gboolean
gtk_moz_embed_can_go_forward(GtkMozEmbed *embed)
{
  PRBool retval = PR_FALSE;
  EmbedPrivate *embedPrivate;

  g_return_val_if_fail ((embed != NULL), FALSE);
  g_return_val_if_fail (GTK_IS_MOZ_EMBED(embed), FALSE);

  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate->mNavigation)
    embedPrivate->mNavigation->GetCanGoForward(&retval);
  return retval;
}

void
gtk_moz_embed_go_back(GtkMozEmbed *embed)
{
  EmbedPrivate *embedPrivate;

  g_return_if_fail (embed != NULL);
  g_return_if_fail (GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate->mNavigation)
    embedPrivate->mNavigation->GoBack();
}

void
gtk_moz_embed_go_forward(GtkMozEmbed *embed)
{
  EmbedPrivate *embedPrivate;

  g_return_if_fail (embed != NULL);
  g_return_if_fail (GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate->mNavigation)
    embedPrivate->mNavigation->GoForward();
}

void
gtk_moz_embed_render_data(GtkMozEmbed *embed, const char *data,
			  guint32 len, const char *base_uri,
			  const char *mime_type)
{
  EmbedPrivate *embedPrivate;

  g_return_if_fail (embed != NULL);
  g_return_if_fail (GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;

  embedPrivate->OpenStream(base_uri, mime_type);
  embedPrivate->AppendToStream(data, len);
  embedPrivate->CloseStream();
}

void
gtk_moz_embed_open_stream(GtkMozEmbed *embed, const char *base_uri,
			  const char *mime_type)
{
  EmbedPrivate *embedPrivate;

  g_return_if_fail (embed != NULL);
  g_return_if_fail (GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;

  embedPrivate->OpenStream(base_uri, mime_type);
}

void gtk_moz_embed_append_data(GtkMozEmbed *embed, const char *data,
			       guint32 len)
{
  EmbedPrivate *embedPrivate;

  g_return_if_fail (embed != NULL);
  g_return_if_fail (GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;
  embedPrivate->AppendToStream(data, len);
}

void
gtk_moz_embed_close_stream(GtkMozEmbed *embed)
{
  EmbedPrivate *embedPrivate;

  g_return_if_fail (embed != NULL);
  g_return_if_fail (GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;
  embedPrivate->CloseStream();
}

char *
gtk_moz_embed_get_link_message(GtkMozEmbed *embed)
{
  char *retval = nsnull;
  EmbedPrivate *embedPrivate;

  g_return_val_if_fail ((embed != NULL), (char *)NULL);
  g_return_val_if_fail (GTK_IS_MOZ_EMBED(embed), (char *)NULL);

  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate->mWindow)
    retval = embedPrivate->mWindow->mLinkMessage.ToNewCString();

  return retval;
}

char *
gtk_moz_embed_get_js_status(GtkMozEmbed *embed)
{
  char *retval = nsnull;
  EmbedPrivate *embedPrivate;

  g_return_val_if_fail ((embed != NULL), (char *)NULL);
  g_return_val_if_fail (GTK_IS_MOZ_EMBED(embed), (char *)NULL);

  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate->mWindow)
    retval = embedPrivate->mWindow->mJSStatus.ToNewCString();

  return retval;
}

char *
gtk_moz_embed_get_title(GtkMozEmbed *embed)
{
  char *retval = nsnull;
  EmbedPrivate *embedPrivate;

  g_return_val_if_fail ((embed != NULL), (char *)NULL);
  g_return_val_if_fail (GTK_IS_MOZ_EMBED(embed), (char *)NULL);

  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate->mWindow)
    retval = embedPrivate->mWindow->mTitle.ToNewCString();

  return retval;
}

char *
gtk_moz_embed_get_location(GtkMozEmbed *embed)
{
  char *retval = nsnull;
  EmbedPrivate *embedPrivate;

  g_return_val_if_fail ((embed != NULL), (char *)NULL);
  g_return_val_if_fail (GTK_IS_MOZ_EMBED(embed), (char *)NULL);

  embedPrivate = (EmbedPrivate *)embed->data;
  
  retval = embedPrivate->mURI.ToNewCString();

  return retval;
}

void
gtk_moz_embed_reload(GtkMozEmbed *embed, gint32 flags)
{
  EmbedPrivate *embedPrivate;

  g_return_if_fail (embed != NULL);
  g_return_if_fail (GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;

  if (embedPrivate->mNavigation)
    embedPrivate->mNavigation->Reload(flags);
}

void
gtk_moz_embed_set_chrome_mask(GtkMozEmbed *embed, guint32 flags)
{
  EmbedPrivate *embedPrivate;

  g_return_if_fail (embed != NULL);
  g_return_if_fail (GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;

  embedPrivate->mChromeMask = flags;
}

guint32
gtk_moz_embed_get_chrome_mask(GtkMozEmbed *embed)
{
  EmbedPrivate *embedPrivate;

  g_return_val_if_fail ((embed != NULL), 0);
  g_return_val_if_fail (GTK_IS_MOZ_EMBED(embed), 0);

  embedPrivate = (EmbedPrivate *)embed->data;

  return embedPrivate->mChromeMask;
}

GtkType
gtk_moz_embed_progress_flags_get_type(void)
{
  static GtkType progress_flags_type = 0;
    
  if (!progress_flags_type)
    progress_flags_type =
      gtk_type_register_flags("GtkMozEmbedReloadFlags",
			      gtk_moz_embed_progress_flags_values);
  return progress_flags_type;
}

GtkType
gtk_moz_embed_status_enums_get_type(void)
{
  static GtkType status_enum_type = 0;

  if (!status_enum_type)
    status_enum_type =
      gtk_type_register_enum("GtkMozEmbedStatusFlags",
			     gtk_moz_embed_status_enums_values);
  return status_enum_type;
}

GtkType
gtk_moz_embed_reload_flags_get_type(void)
{
  static GtkType reload_flags_type = 0;

  if (!reload_flags_type)
    reload_flags_type =
      gtk_type_register_flags("GtkMozEmbedReloadFlags",
			      gtk_moz_embed_reload_flags_values);
  return reload_flags_type;
}

GtkType
gtk_moz_embed_chrome_flags_get_type(void)
{
  static GtkType chrome_flags_type = 0;

  if (!chrome_flags_type)
    chrome_flags_type = 
      gtk_type_register_flags("GtkMozEmbedChromeFlags",
			      gtk_moz_embed_chrome_flags_values);
  return chrome_flags_type;
}

void
gtk_moz_embed_get_nsIWebBrowser  (GtkMozEmbed *embed, nsIWebBrowser **retval)
{
  EmbedPrivate *embedPrivate;

  g_return_if_fail (embed != NULL);
  g_return_if_fail (GTK_IS_MOZ_EMBED(embed));

  embedPrivate = (EmbedPrivate *)embed->data;
  
  if (embedPrivate->mWindow)
    embedPrivate->mWindow->GetWebBrowser(retval);
}

PRUnichar *
gtk_moz_embed_get_title_unicode (GtkMozEmbed *embed)
{
  PRUnichar *retval = nsnull;
  EmbedPrivate *embedPrivate;
                   
  g_return_val_if_fail ((embed != NULL), (PRUnichar *)NULL);
  g_return_val_if_fail (GTK_IS_MOZ_EMBED(embed), (PRUnichar *)NULL);
  
  embedPrivate = (EmbedPrivate *)embed->data;
                   
  if (embedPrivate->mWindow)
    retval = embedPrivate->mWindow->mTitle.ToNewUnicode();
                   
  return retval;
}
