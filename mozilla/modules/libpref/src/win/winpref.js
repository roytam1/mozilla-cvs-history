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

platform.windows = true;

pref("browser.bookmark_window_showwindow",  1);		// SW_NORMAL
pref("mailnews.folder_window_showwindow",   1);		// SW_NORMAL
pref("mailnews.thread_window_showwindow",   1);		// SW_NORMAL
pref("mailnews.message_window_showwindow",  1);		// SW_NORMAL

pref("browser.bookmark_columns_win",        "");
pref("mailnews.folder_columns_win",         "");
pref("mail.thread_columns_win",             "");
pref("news.thread_columns_win",             "");
pref("category.thread_columns_win",         "");
pref("news.category_columns_win",           "");


pref("font.name.serif.x-western", "Times New Roman");
pref("font.size.serif.x-western", 12);
pref("font.name.sans-serif.x-western", "Arial");
pref("font.size.sans-serif.x-western", 12);
pref("font.name.monospace.x-western", "Courier New");
pref("font.size.monospace.x-western", 10);

pref("font.name.serif.x-unicode", "Times New Roman");
pref("font.size.serif.x-unicode", 12);
pref("font.name.sans-serif.x-unicode", "Arial");
pref("font.size.sans-serif.x-unicode", 12);
pref("font.name.monospace.x-unicode", "Courier New");
pref("font.size.monospace.x-unicode", 10);

pref("intl.font2.win.mimecharset",  "iso-8859-1");
pref("intl.font2.win.prop_font",    "Times New Roman");
pref("intl.font2.win.prop_size",    12);
pref("intl.font2.win.fixed_font",   "Courier New");
pref("intl.font2.win.fixed_size",   10);

pref("intl.font260.win.mimecharset",    "Shift_JIS");
pref("intl.font260.win.prop_font",      "Times New Roman");
pref("intl.font260.win.prop_size",      10);
pref("intl.font260.win.fixed_font",     "Courier New");
pref("intl.font260.win.fixed_size",     10);

pref("intl.font263.win.mimecharset",    "big5");
pref("intl.font263.win.prop_font",  "Times New Roman");
pref("intl.font263.win.prop_size",  12);
pref("intl.font263.win.fixed_font", "Courier New");
pref("intl.font263.win.fixed_size", 10);

pref("intl.font1292.win.mimecharset",   "euc-kr");
pref("intl.font1292.win.prop_font", "Times New Roman");
pref("intl.font1292.win.prop_size", 12);
pref("intl.font1292.win.fixed_font",    "Courier New");
pref("intl.font1292.win.fixed_size",    10);

pref("intl.font264.win.mimecharset",    "gb2312");
pref("intl.font264.win.prop_font",  "Times New Roman");
pref("intl.font264.win.prop_size",  12);
pref("intl.font264.win.fixed_font", "Courier New");
pref("intl.font264.win.fixed_size", 10);

pref("intl.font44.win.mimecharset", "windows-1250");
pref("intl.font44.win.prop_font",   "Times New Roman");
pref("intl.font44.win.prop_size",   12);
pref("intl.font44.win.fixed_font",  "Courier New");
pref("intl.font44.win.fixed_size",  10);

pref("intl.font41.win.mimecharset", "windows-1251");
pref("intl.font41.win.prop_font",   "Times New Roman");
pref("intl.font41.win.prop_size",   12);
pref("intl.font41.win.fixed_font",  "Courier New");
pref("intl.font41.win.fixed_size",  10);

pref("intl.font43.win.mimecharset", "windows-1253");
pref("intl.font43.win.prop_font",   "Times New Roman");
pref("intl.font43.win.prop_size",   12);
pref("intl.font43.win.fixed_font",  "Courier New");
pref("intl.font43.win.fixed_size",  10);

pref("intl.font20.win.mimecharset", "iso-8859-9");
pref("intl.font20.win.prop_font",   "Times New Roman");
pref("intl.font20.win.prop_size",   12);
pref("intl.font20.win.fixed_font",  "Courier New");
pref("intl.font20.win.fixed_size",  10);

pref("intl.font290.win.mimecharset",    "utf-8");
pref("intl.font290.win.prop_font",  "Times New Roman");
pref("intl.font290.win.prop_size",  12);
pref("intl.font290.win.fixed_font", "Courier New");
pref("intl.font290.win.fixed_size", 10);

pref("intl.font254.win.mimecharset",    "x-user-defined");
pref("intl.font254.win.prop_font",  "Times New Roman");
pref("intl.font254.win.prop_size",  12);
pref("intl.font254.win.fixed_font", "Courier New");
pref("intl.font254.win.fixed_size", 10);

pref("taskbar.x",                           -1); 
pref("taskbar.y",                           -1);
pref("taskbar.floating",                    true);
pref("taskbar.horizontal",                  false);
pref("taskbar.ontop",                       true);
pref("taskbar.button_style",                -1);

pref("netinst.profile.show_profile_wizard", true); 

//The following pref is internal to Communicator. Please
//do *not* place it in the docs...
pref("netinst.profile.show_dir_overwrite_msg",  true); 
