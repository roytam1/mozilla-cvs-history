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

#ifndef gtkmozembed_h
#define gtkmozembed_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#define GTK_TYPE_MOZ_EMBED             (gtk_moz_embed_get_type())
#define GTK_MOZ_EMBED(obj)             GTK_CHECK_CAST((obj), GTK_TYPE_MOZ_EMBED, GtkMozEmbed)
#define GTK_MOZ_EMBED_CLASS(klass)     GTK_CHEK_CLASS_CAST((klass), GTK_TYPE_MOZ_EMBED, GtkMozEmbedClass)
#define GTK_IS_MOZ_EMBED(obj)          GTK_CHECK_TYPE((obj), GTK_TYPE_MOZ_EMBED)
#define GTK_IS_MOZ_EMBED_CLASS(klass)  GTK_CHECK_CLASS_TYPE((klass), GTK_TYPE_MOZ_EMBED)

typedef struct _GtkMozEmbed      GtkMozEmbed;
typedef struct _GtkMozEmbedClass GtkMozEmbedClass;

struct _GtkMozEmbed
{
  GtkBin    bin;
  void     *data;
};

struct _GtkMozEmbedClass
{
  GtkBinClass parent_class;

  void (* link_message) (GtkMozEmbed *embed);
  void (* js_status)    (GtkMozEmbed *embed);
  void (* location)     (GtkMozEmbed *embed);
  void (* title)        (GtkMozEmbed *embed);
  void (* progress)     (GtkMozEmbed *embed, gint maxprogress, gint curprogress);
  void (* net_status)   (GtkMozEmbed *embed, gint status);
  void (* net_start)    (GtkMozEmbed *embed);
  void (* net_stop)     (GtkMozEmbed *embed);
};

extern GtkType      gtk_moz_embed_get_type         (void);
extern GtkWidget   *gtk_moz_embed_new              (void);
extern void         gtk_moz_embed_load_url         (GtkMozEmbed *embed, const char *url);
extern void         gtk_moz_embed_stop_load        (GtkMozEmbed *embed);
extern char        *gtk_moz_embed_get_link_message (GtkMozEmbed *embed);
extern char        *gtk_moz_embed_get_js_status    (GtkMozEmbed *embed);
extern char        *gtk_moz_embed_get_title        (GtkMozEmbed *embed);
extern char        *gtk_moz_embed_get_location     (GtkMozEmbed *embed);

/* These are straight out of nsIWebProgress.h */

enum { gtk_moz_embed_flag_net_start = 1,
       gtk_moz_embed_flag_net_stop = 2,
       gtk_moz_embed_flag_net_dns = 4,
       gtk_moz_embed_flag_net_connecting = 8,
       gtk_moz_embed_flag_net_redirecting = 16,
       gtk_moz_embed_flag_net_negotiating = 32,
       gtk_moz_embed_flag_net_transferring = 64,
       gtk_moz_embed_flag_net_failedDNS = 4096,
       gtk_moz_embed_flag_net_failedConnect = 8192,
       gtk_moz_embed_flag_net_failedTransfer = 16384,
       gtk_moz_embed_flag_net_failedTimeout = 32768,
       gtk_moz_embed_flag_net_userCancelled = 65536,
       gtk_moz_embed_flag_win_start = 1048576,
       gtk_moz_embed_flag_win_stop = 2097152 };


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* gtkmozembed_h */
