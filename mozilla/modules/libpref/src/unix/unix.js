/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

// The other platforms roll this all into "toolbar mode".
pref("browser.chrome.toolbar_tips", true);
pref("browser.chrome.show_menubar", true);

pref("mail.empty_trash", false);

// Handled differently under Mac/Windows
pref("network.hosts.smtp_server", "localhost");
pref("network.hosts.pop_server", "pop");
pref("mail.check_new_mail", true);
pref("mail.sort_by", 0);
pref("news.sort_by", 0);
pref("browser.startup.license_accepted", "");
pref("browser.cache.memory_cache_size", 3000);
pref("browser.cache.disk_cache_size", 5000);
pref("browser.ncols", 0);
pref("browser.installcmap", false);
pref("mail.signature_file", "~/.signature");
pref("mail.default_fcc", "~/nsmail/Sent");
pref("news.default_fcc", "~/nsmail/Sent");
pref("mailnews.reply_on_top", 0);
pref("mailnews.reply_with_extra_lines", 0);
pref("browser.startup.default_window", 0);
pref("security.warn_accept_cookie", false);
pref("editor.disable_spell_checker", false);
pref("editor.dont_lock_spell_files", true);

// Instead of "delay_images"
pref("browser.autoload_images", true);

// Unix only
pref("mail.use_movemail", true);
pref("mail.use_builtin_movemail", true);
pref("mail.movemail_program", "");
pref("mail.movemail_warn", false);
pref("mail.sash_geometry", "");
pref("news.cache_xover", false);
pref("news.show_first_unread", false);
pref("news.sash_geometry", "");
pref("helpers.global_mime_types_file", "/usr/local/lib/netscape/mime.types");
pref("helpers.global_mailcap_file", "/usr/local/lib/netscape/mailcap");
pref("helpers.private_mime_types_file", "~/.mime.types");
pref("helpers.private_mailcap_file", "~/.mailcap");
pref("applications.telnet", "xterm -e telnet %h %p");
pref("applications.tn3270", "xterm -e tn3270 %h");
pref("applications.rlogin", "xterm -e rlogin %h");
pref("applications.rlogin_with_user", "xterm -e rlogin %h -l %u");
pref("applications.tmp_dir", "/tmp");
// On Solaris/IRIX, this should be "lp"
pref("print.print_command", "lpr");
pref("print.print_reversed", false);
pref("print.print_color", true);
pref("print.print_landscape", false);
pref("print.print_paper_size", 0);

// Not sure what this one does...
pref("browser.fancy_ftp", true);

// Fortezza stuff
pref("fortezza.toggle", 1);
pref("fortezza.timeout", 30);

pref("font.name.serif.x-western", "times");
pref("font.name.sans-serif.x-western", "helvetica");
pref("font.name.monospace.x-western", "courier");

pref("intl.font_charset", "");
pref("intl.font_spec_list", "");
pref("mail.signature_date", 0);

// Outliner column defaults
pref("mail.threadpane.messagepane_height", 400);

pref("taskbar.x", -1);
pref("taskbar.y", -1);
pref("taskbar.floating", false);
pref("taskbar.horizontal", false);
pref("taskbar.ontop", false);
pref("taskbar.button_style", -1);

config("menu.help.item_1.url", "http://home.netscape.com/eng/mozilla/5.0/relnotes/unix-5.0.html");
