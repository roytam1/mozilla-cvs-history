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
 */

#include "gtkmozembed.h"
#include <gtk/gtk.h>
#include <string.h>

typedef struct _TestGtkBrowser {
  GtkWidget  *topLevelWindow;
  GtkWidget  *topLevelVBox;
  GtkWidget  *toolbarHBox;
  GtkWidget  *toolbar;
  GtkWidget  *backButton;
  GtkWidget  *stopButton;
  GtkWidget  *forwardButton;
  GtkWidget  *reloadButton;
  GtkWidget  *streamButton;
  GtkWidget  *urlEntry;
  GtkWidget  *mozEmbed;
  GtkWidget  *progressAreaHBox;
  GtkWidget  *progressBar;
  GtkWidget  *statusBar;
  const char *statusMessage;
  int         loadPercent;
  int         bytesLoaded;
  int         maxBytesLoaded;
  char       *tempMessage;
} TestGtkBrowser;

static TestGtkBrowser *new_gtk_browser    (guint32 chromeMask);

static int num_browsers = 0;

// callbacks from the UI
static void     back_clicked_cb    (GtkButton   *button, TestGtkBrowser *browser);
static void     stop_clicked_cb    (GtkButton   *button, TestGtkBrowser *browser);
static void     forward_clicked_cb (GtkButton   *button, TestGtkBrowser *browser);
static void     reload_clicked_cb  (GtkButton   *button, TestGtkBrowser *browser);
static void     stream_clicked_cb  (GtkButton   *button, TestGtkBrowser *browser);
static void     url_activate_cb    (GtkEditable *widget, TestGtkBrowser *browser);
static gboolean delete_cb          (GtkWidget *widget,   GdkEventAny *event,
				    TestGtkBrowser *browser);
static void     destroy_cb         (GtkWidget *widget, TestGtkBrowser *browser);

// callbacks from the widget
static void location_changed_cb  (GtkMozEmbed *embed, TestGtkBrowser *browser);
static void title_changed_cb     (GtkMozEmbed *embed, TestGtkBrowser *browser);
static void load_started_cb      (GtkMozEmbed *embed, TestGtkBrowser *browser);
static void load_finished_cb     (GtkMozEmbed *embed, TestGtkBrowser *browser);
static void net_status_change_cb (GtkMozEmbed *embed, gint flags, TestGtkBrowser *browser);
static void progress_change_cb   (GtkMozEmbed *embed, gint cur, gint max,
				  TestGtkBrowser *browser);
static void link_message_cb      (GtkMozEmbed *embed, TestGtkBrowser *browser);
static void js_status_cb         (GtkMozEmbed *embed, TestGtkBrowser *browser);
static void new_window_cb        (GtkMozEmbed *embed, GtkMozEmbed **retval, guint chromemask,
				  TestGtkBrowser *browser);
static void visibility_cb        (GtkMozEmbed *embed, gboolean visibility, TestGtkBrowser *browser);
static void destroy_brsr_cb      (GtkMozEmbed *embed, TestGtkBrowser *browser);

// some utility functions
static void update_status_bar_text  (TestGtkBrowser *browser);
static void update_temp_message     (TestGtkBrowser *browser, const char *message);
static void update_nav_buttons      (TestGtkBrowser *browser);

int
main(int argc, char **argv)
{
  gtk_init(&argc, &argv);

  TestGtkBrowser *browser = new_gtk_browser(gtk_moz_embed_flag_defaultChrome);

  // set our minimum size
  gtk_widget_set_usize(browser->topLevelWindow, 400, 400);

  gtk_widget_show_all(browser->topLevelWindow);

  if (argc > 1)
    gtk_moz_embed_load_url(GTK_MOZ_EMBED(browser->mozEmbed), argv[1]);

  gtk_main();
}

static TestGtkBrowser *
new_gtk_browser(guint32 chromeMask)
{
  TestGtkBrowser *browser = 0;

  num_browsers++;

  browser = g_new0(TestGtkBrowser, 1);

  // create our new toplevel window
  browser->topLevelWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  // new vbox
  browser->topLevelVBox = gtk_vbox_new(FALSE, 0);
  // add it to the toplevel window
  gtk_container_add(GTK_CONTAINER(browser->topLevelWindow), browser->topLevelVBox);
  // create the hbox that will contain the toolbar and the url text entry bar
  browser->toolbarHBox = gtk_hbox_new(FALSE, 0);
  // add that hbox to the vbox
  gtk_box_pack_start(GTK_BOX(browser->topLevelVBox), browser->toolbarHBox,
		   FALSE, // expand
		   FALSE, // fill
		   0);    // padding
  // new horiz toolbar with buttons + icons
  browser->toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
				     GTK_TOOLBAR_BOTH);
  // add it to the hbox
  gtk_box_pack_start(GTK_BOX(browser->toolbarHBox), browser->toolbar,
		   FALSE, // expand
		   FALSE, // fill
		   0);    // padding
  // new back button
  browser->backButton = gtk_toolbar_append_item(GTK_TOOLBAR(browser->toolbar),
						"Back",
						"Go Back",
						"Go Back",
						0, // XXX replace with icon
						GTK_SIGNAL_FUNC(back_clicked_cb),
						browser);
  // new stop button
  browser->stopButton = gtk_toolbar_append_item(GTK_TOOLBAR(browser->toolbar),
						"Stop",
						"Stop",
						"Stop",
						0, // XXX replace with icon
						GTK_SIGNAL_FUNC(stop_clicked_cb),
						browser);
  // new forward button
  browser->forwardButton = gtk_toolbar_append_item(GTK_TOOLBAR(browser->toolbar),
						   "Forward",
						   "Forward",
						   "Forward",
						   0, // XXX replace with icon
						   GTK_SIGNAL_FUNC(forward_clicked_cb),
						   browser);
  // new reload button
  browser->reloadButton = gtk_toolbar_append_item(GTK_TOOLBAR(browser->toolbar),
						  "Reload",
						  "Reload",
						  "Reload",
						  0, // XXX replace with icon
						  GTK_SIGNAL_FUNC(reload_clicked_cb),
						  browser);
  // new stream button
  browser->streamButton = gtk_toolbar_append_item(GTK_TOOLBAR(browser->toolbar),
						  "Stream",
						  "Stream",
						  "Stream",
						  0, // XXX replace with icon
						  GTK_SIGNAL_FUNC(stream_clicked_cb),
						  browser);
  // create the url text entry
  browser->urlEntry = gtk_entry_new();
  // add it to the hbox
  gtk_box_pack_start(GTK_BOX(browser->toolbarHBox), browser->urlEntry,
		   TRUE, // expand
		   TRUE, // fill
		   0);    // padding
  // create our new gtk moz embed widget
  browser->mozEmbed = gtk_moz_embed_new();
  // add it to the toplevel vbox
  gtk_box_pack_start(GTK_BOX(browser->topLevelVBox), browser->mozEmbed,
			   TRUE, // expand
			   TRUE, // fill
			   0);   // padding
  // create the new hbox for the progress area
  browser->progressAreaHBox = gtk_hbox_new(FALSE, 0);
  // add it to the vbox
  gtk_box_pack_start(GTK_BOX(browser->topLevelVBox), browser->progressAreaHBox,
		   FALSE, // expand
		   FALSE, // fill
		   0);   // padding
  // create our new progress bar
  browser->progressBar = gtk_progress_bar_new();
  // add it to the hbox
  gtk_box_pack_start(GTK_BOX(browser->progressAreaHBox), browser->progressBar,
		   FALSE, // expand
		   FALSE, // fill
		   0);   // padding
  // create our status areaa
  browser->statusBar = gtk_statusbar_new();
  // add it to the hbox
  gtk_box_pack_start(GTK_BOX(browser->progressAreaHBox), browser->statusBar,
		   TRUE, // expand
		   TRUE, // fill
		   0);   // padding
  
  // by default none of the buttons are marked as sensitive.
  gtk_widget_set_sensitive(browser->backButton, FALSE);
  gtk_widget_set_sensitive(browser->stopButton, FALSE);
  gtk_widget_set_sensitive(browser->forwardButton, FALSE);
  gtk_widget_set_sensitive(browser->reloadButton, FALSE);

  // catch the destruction of the toplevel window
  gtk_signal_connect(GTK_OBJECT(browser->topLevelWindow), "delete_event",
		     GTK_SIGNAL_FUNC(delete_cb), browser);
  // hook up the activate signal to the right callback
  gtk_signal_connect(GTK_OBJECT(browser->urlEntry), "activate",
		     GTK_SIGNAL_FUNC(url_activate_cb), browser);
  // hook up the location change to update the urlEntry
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "location",
		     GTK_SIGNAL_FUNC(location_changed_cb), browser);
  // hook up the title change to update the window title
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "title",
		     GTK_SIGNAL_FUNC(title_changed_cb), browser);
  // hook up the start and stop signals
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "net_start",
		     GTK_SIGNAL_FUNC(load_started_cb), browser);
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "net_stop",
		     GTK_SIGNAL_FUNC(load_finished_cb), browser);
  // hook up to the change in network status
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "net_status",
		     GTK_SIGNAL_FUNC(net_status_change_cb), browser);
  // hookup to changes in progress
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "progress",
		     GTK_SIGNAL_FUNC(progress_change_cb), browser);
  // hookup to changes in over-link message
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "link_message",
		     GTK_SIGNAL_FUNC(link_message_cb), browser);
  // hookup to changes in js status message
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "js_status",
		     GTK_SIGNAL_FUNC(js_status_cb), browser);
  // hookup to see whenever a new window is requested
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "new_window",
		     GTK_SIGNAL_FUNC(new_window_cb), browser);
  // hookup to any requested visibility changes
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "visibility",
		     GTK_SIGNAL_FUNC(visibility_cb), browser);
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "destroy_browser",
		     GTK_SIGNAL_FUNC(destroy_brsr_cb), browser);
  // hookup to when the window is destroyed
  gtk_signal_connect(GTK_OBJECT(browser->mozEmbed), "destroy",
		     GTK_SIGNAL_FUNC(destroy_cb), browser);

  // set the chrome type so it's stored in the object
  gtk_moz_embed_set_chrome_mask(GTK_MOZ_EMBED(browser->mozEmbed), chromeMask);

  return browser;
}

void
back_clicked_cb (GtkButton *button, TestGtkBrowser *browser)
{
  gtk_moz_embed_go_back(GTK_MOZ_EMBED(browser->mozEmbed));
}

void
stop_clicked_cb (GtkButton *button, TestGtkBrowser *browser)
{
  g_print("stop_clicked_cb\n");
  gtk_moz_embed_stop_load(GTK_MOZ_EMBED(browser->mozEmbed));
}

void
forward_clicked_cb (GtkButton *button, TestGtkBrowser *browser)
{
  g_print("forward_clicked_cb\n");
  gtk_moz_embed_go_forward(GTK_MOZ_EMBED(browser->mozEmbed));
}

void
reload_clicked_cb  (GtkButton *button, TestGtkBrowser *browser)
{
  g_print("reload_clicked_cb\n");
  gtk_moz_embed_reload(GTK_MOZ_EMBED(browser->mozEmbed), gtk_moz_embed_flag_reloadNormal);
}

void 
stream_clicked_cb  (GtkButton   *button, TestGtkBrowser *browser)
{
  char *data;
  char *data2;
  data = "<html>Hi";
  data2 = " there</html>\n";
  g_print("stream_clicked_cb\n");
  gtk_moz_embed_open_stream(GTK_MOZ_EMBED(browser->mozEmbed), "file://", "text/html");
  gtk_moz_embed_append_data(GTK_MOZ_EMBED(browser->mozEmbed), data, strlen(data));
  gtk_moz_embed_append_data(GTK_MOZ_EMBED(browser->mozEmbed), data2, strlen(data2));
  gtk_moz_embed_close_stream(GTK_MOZ_EMBED(browser->mozEmbed));
}

void
url_activate_cb    (GtkEditable *widget, TestGtkBrowser *browser)
{
  gchar *text = gtk_editable_get_chars(widget, 0, -1);
  g_print("loading url %s\n", text);
  gtk_moz_embed_load_url(GTK_MOZ_EMBED(browser->mozEmbed), text);
  g_free(text);
}

gboolean
delete_cb(GtkWidget *widget, GdkEventAny *event, TestGtkBrowser *browser)
{
  g_print("delete_cb\n");
  gtk_widget_destroy(widget);
  return TRUE;
}

void
destroy_cb         (GtkWidget *widget, TestGtkBrowser *browser)
{
  g_print("destroy_cb\n");
  num_browsers--;
  if (browser->tempMessage)
    g_free(browser->tempMessage);
  if (num_browsers == 0)
    gtk_main_quit();
}

void
location_changed_cb (GtkMozEmbed *embed, TestGtkBrowser *browser)
{
  char *newLocation;
  int   newPosition = 0;
  g_print("location_changed_cb\n");
  newLocation = gtk_moz_embed_get_location(embed);
  if (newLocation)
  {
    gtk_editable_delete_text(GTK_EDITABLE(browser->urlEntry), 0, -1);
    gtk_editable_insert_text(GTK_EDITABLE(browser->urlEntry), newLocation, strlen(newLocation), &newPosition);
    g_free(newLocation);
  }
  else
    g_print("failed to get location!\n");
  // always make sure to clear the tempMessage.  it might have been
  // set from the link before a click and we wouldn't have gotten the
  // callback to unset it.
  update_temp_message(browser, 0);
  // update the nav buttons on a location change
  update_nav_buttons(browser);
}

void
title_changed_cb    (GtkMozEmbed *embed, TestGtkBrowser *browser)
{
  char *newTitle;
  g_print("title_changed_cb\n");
  newTitle = gtk_moz_embed_get_title(embed);
  if (newTitle)
  {
    gtk_window_set_title(GTK_WINDOW(browser->topLevelWindow), newTitle);
    g_free(newTitle);
  }
  
}

void
load_started_cb     (GtkMozEmbed *embed, TestGtkBrowser *browser)
{
  g_print("load_started_cb\n");
  gtk_widget_set_sensitive(browser->stopButton, TRUE);
  gtk_widget_set_sensitive(browser->reloadButton, FALSE);
  browser->loadPercent = 0;
  browser->bytesLoaded = 0;
  browser->maxBytesLoaded = 0;
  update_status_bar_text(browser);
}

void
load_finished_cb    (GtkMozEmbed *embed, TestGtkBrowser *browser)
{
  g_print("load_finished_cb\n");
  gtk_widget_set_sensitive(browser->stopButton, FALSE);
  gtk_widget_set_sensitive(browser->reloadButton, TRUE);
  browser->loadPercent = 0;
  browser->bytesLoaded = 0;
  browser->maxBytesLoaded = 0;
  update_status_bar_text(browser);
  gtk_progress_set_percentage(GTK_PROGRESS(browser->progressBar), 0);
}


void
net_status_change_cb (GtkMozEmbed *embed, gint flags, TestGtkBrowser *browser)
{
  //  g_print("net_status_change_cb %d\n", flags);
  if (flags & gtk_moz_embed_flag_net_dns)
    browser->statusMessage = "Looking up host...";
  else if (flags & gtk_moz_embed_flag_net_connecting)
    browser->statusMessage = "Connecting to site...";
  else if (flags & gtk_moz_embed_flag_net_redirecting)
    browser->statusMessage = "Connecting to site...";
  else if (flags & gtk_moz_embed_flag_net_transferring)
    browser->statusMessage = "Transferring data from site...";
  else if (flags & gtk_moz_embed_flag_net_failedDNS)
    browser->statusMessage = "Site not found.";
  else if (flags & gtk_moz_embed_flag_net_failedConnect)
    browser->statusMessage = "Failed to connect to site.";
  else if (flags & gtk_moz_embed_flag_net_failedTransfer)
    browser->statusMessage =  "Failed to transfer any data from site.";
  else if (flags & gtk_moz_embed_flag_net_userCancelled)
    browser->statusMessage = "User cancelled connecting to site.";

  if (flags & gtk_moz_embed_flag_win_start)
    browser->statusMessage = "Loading site...";
  else if (flags & gtk_moz_embed_flag_win_stop)
    browser->statusMessage = "Done.";

  update_status_bar_text(browser);
  
}

void progress_change_cb   (GtkMozEmbed *embed, gint cur, gint max,
			   TestGtkBrowser *browser)
{
  g_print("progress_change_cb cur %d max %d\n", cur, max);

  if (max == -1)
  {
    gtk_progress_set_activity_mode(GTK_PROGRESS(browser->progressBar), FALSE);
    browser->loadPercent = 0;
    browser->bytesLoaded = cur;
    browser->maxBytesLoaded = 0;
    update_status_bar_text(browser);
  }
  else
  {
    browser->bytesLoaded = cur;
    browser->maxBytesLoaded = max;
    browser->loadPercent = (cur * 100) / max;
    update_status_bar_text(browser);
    gtk_progress_set_percentage(GTK_PROGRESS(browser->progressBar), browser->loadPercent / 100.0);
  }
  
}

void
link_message_cb      (GtkMozEmbed *embed, TestGtkBrowser *browser)
{
  char *message;
  g_print("link_message_cb\n");
  message = gtk_moz_embed_get_link_message(embed);
  if (message && (strlen(message) == 0))
    update_temp_message(browser, 0);
  else
    update_temp_message(browser, message);
  if (message)
    g_free(message);
}

void
js_status_cb (GtkMozEmbed *embed, TestGtkBrowser *browser)
{
 char *message;
  g_print("js_status_cb\n");
  message = gtk_moz_embed_get_js_status(embed);
  if (message && (strlen(message) == 0))
    update_temp_message(browser, 0);
  else
    update_temp_message(browser, message);
  if (message)
    g_free(message);
}

void
new_window_cb (GtkMozEmbed *embed, GtkMozEmbed **newEmbed, guint chromemask, TestGtkBrowser *browser)
{
  g_print("new_window_cb\n");
  g_print("embed is %p chromemask is %d\n", embed, chromemask);
  TestGtkBrowser *newBrowser = new_gtk_browser(chromemask);
  gtk_widget_set_usize(newBrowser->topLevelWindow, 400, 400);
  *newEmbed = GTK_MOZ_EMBED(newBrowser->mozEmbed);
  g_print("new browser is %p\n", *newEmbed);
}

void
visibility_cb (GtkMozEmbed *embed, gboolean visibility, TestGtkBrowser *browser)
{
  g_print("visibility_cb %d\n", visibility);
  if (visibility)
    gtk_widget_show_all(browser->topLevelWindow);
  else
    gtk_widget_hide_all(browser->topLevelWindow);
}

void
destroy_brsr_cb      (GtkMozEmbed *embed, TestGtkBrowser *browser)
{
  g_print("destroy_brsr_cb\n");
  gtk_widget_destroy(browser->topLevelWindow);
}

// utility functions

void
update_status_bar_text(TestGtkBrowser *browser)
{
  gchar message[256];

  gtk_statusbar_pop(GTK_STATUSBAR(browser->statusBar), 1);
  if (browser->tempMessage)
    gtk_statusbar_push(GTK_STATUSBAR(browser->statusBar), 1, browser->tempMessage);
  else
  {
    if (browser->loadPercent)
    {
      g_snprintf(message, 255, "%s (%d%% complete, %d bytes of %d loaded)", browser->statusMessage, browser->loadPercent, browser->bytesLoaded, browser->maxBytesLoaded);
    }
    else if (browser->bytesLoaded)
    {
      g_snprintf(message, 255, "%s (%d bytes loaded)", browser->statusMessage, browser->bytesLoaded);
    }
    else
    {
      g_snprintf(message, 255, "%s", browser->statusMessage);
    }
    gtk_statusbar_push(GTK_STATUSBAR(browser->statusBar), 1, message);
  }
}

void
update_temp_message(TestGtkBrowser *browser, const char *message)
{
  if (browser->tempMessage)
    g_free(browser->tempMessage);
  if (message)
    browser->tempMessage = g_strdup(message);
  else
    browser->tempMessage = 0;
  // now that we've updated the temp message, redraw the status bar
  update_status_bar_text(browser);
}


void
update_nav_buttons      (TestGtkBrowser *browser)
{
  gboolean can_go_back;
  gboolean can_go_forward;
  can_go_back = gtk_moz_embed_can_go_back(GTK_MOZ_EMBED(browser->mozEmbed));
  can_go_forward = gtk_moz_embed_can_go_forward(GTK_MOZ_EMBED(browser->mozEmbed));
  if (can_go_back)
    gtk_widget_set_sensitive(browser->backButton, TRUE);
  else
    gtk_widget_set_sensitive(browser->backButton, FALSE);
  if (can_go_forward)
    gtk_widget_set_sensitive(browser->forwardButton, TRUE);
  else
    gtk_widget_set_sensitive(browser->forwardButton, FALSE);
}

