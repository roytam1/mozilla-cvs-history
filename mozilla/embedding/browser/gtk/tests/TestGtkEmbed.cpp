#include "gtkmozembed.h"
#include <gtk/gtk.h>

GtkWidget *toplevelwindow;
GtkWidget *box;
GtkWidget *entry;
GtkWidget *moz_embed;

static gboolean
destroy_cb(GtkWidget *widget, gpointer data);

static void
activate_cb(GtkEditable *widget, gpointer data);

void
link_message_cb(GtkMozEmbed *embed, gpointer data);

void
js_status_cb(GtkMozEmbed *embed, gpointer data);

void
location_cb(GtkMozEmbed *embed, gpointer data);

void
title_cb(GtkMozEmbed *embed, gpointer data);

int
main(int argc, char **argv)
{
  gtk_init(&argc, &argv);

  toplevelwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect(GTK_OBJECT(toplevelwindow), "delete_event",
		     GTK_SIGNAL_FUNC(destroy_cb), NULL);

  gtk_widget_set_usize(toplevelwindow, 300, 400);

  box = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(toplevelwindow), box);
  gtk_widget_show(box);
  
  entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 1);
  gtk_widget_show(entry);

  gtk_signal_connect(GTK_OBJECT(entry), "activate",
		     GTK_SIGNAL_FUNC(activate_cb), NULL);

  moz_embed = gtk_moz_embed_new();

  gtk_signal_connect(GTK_OBJECT(moz_embed), "link_message",
		     GTK_SIGNAL_FUNC(link_message_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(moz_embed), "js_status",
		     GTK_SIGNAL_FUNC(js_status_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(moz_embed), "location",
		     GTK_SIGNAL_FUNC(location_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(moz_embed), "title",
		     GTK_SIGNAL_FUNC(title_cb), NULL);

  gtk_widget_set_usize(moz_embed, 100, 100);
  gtk_box_pack_end(GTK_BOX(box), moz_embed, TRUE, TRUE, 1);
  gtk_widget_show(moz_embed);

  if (argc > 1)
  {
    gtk_moz_embed_load_url(GTK_WIDGET (moz_embed), argv[1]);
  }

  gtk_widget_show(toplevelwindow);

  gtk_main();
}

gboolean
destroy_cb(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(toplevelwindow);
  gtk_main_quit();
  return TRUE;
}

void
activate_cb(GtkEditable *widget, gpointer data)
{
  gchar *text = gtk_editable_get_chars(widget, 0, -1);
  g_print("activate_cb.  data is %s\n", text);
  gtk_moz_embed_load_url(moz_embed, text);
  g_free(text);
}

void
link_message_cb(GtkMozEmbed *embed, gpointer data)
{
  g_print("link_message_cb\n");
}

void
js_status_cb(GtkMozEmbed *embed, gpointer data)
{
  g_print("js_status_cb\n");
}

void
location_cb(GtkMozEmbed *embed, gpointer data)
{
  g_print("location_cb\n");
}

void
title_cb(GtkMozEmbed *embed, gpointer data)
{
  g_print("title_cb\n");
}
