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

// SYNTAX HINTS:  dashes are delimiters.  Use underscores instead.
//  The first character after a period must be alphabetic.

pref("network.search.url","http://cgi.netscape.com/cgi-bin/url_search.cgi?search=");

pref("keyword.URL", "http://keyword.netscape.com/keyword/");
pref("keyword.enabled", false);
pref("general.useragent.locale", "en-US");
pref("general.milestone", "");

pref("general.startup.browser",             true);
pref("general.startup.mail",                false);
pref("general.startup.news",                false);
pref("general.startup.editor",              false);
pref("general.startup.calendar",            false);

pref("general.always_load_images",          true);
pref("general.always_load_movies",          true);
pref("general.always_load_sounds",          true);
pref("general.title_tips",                  true);

pref("general.help_source.site",            1); // 0 = Netscape, 1 = installed, 2 = custom
pref("general.help_source.url",             "");

pref("general.fullcircle_enable",           true);
pref("general.fullcircle_collect_ns_data",  false);

pref("browser.enable_style_sheets",         true);
// 0 = blank, 1 = home (browser.startup.homepage), 2 = last
pref("browser.startup.page",                1);     
pref("browser.startup.homepage",	   "http://www.mozilla.org/");
pref("browser.startup.homepage_override",   true);
pref("browser.startup.autoload_homepage",   true);
pref("browser.startup.agreed_to_licence",   false);
pref("browser.startup.license_version",     0);
pref("browser.startup.default_window",      1); // start up browser
pref("browser.cache.disk_cache_size",       7680);
pref("browser.cache.enable",                true);
pref("browser.cache.memory_cache_size",     1024);
pref("browser.cache.disk_cache_ssl",        false);
pref("browser.foreground_color",            "#000000");
pref("browser.background_color",            "#C0C0C0");
pref("browser.anchor_color",                "#0000EE");
pref("browser.visited_color",               "#551A8B");
pref("browser.chrome.show_directory_buttons",   true);
pref("browser.chrome.toolbar_style",        2);
pref("browser.chrome.advanced_toolbar",     false);
pref("browser.chrome.show_toolbar",         true);
pref("browser.chrome.show_status_bar",      true);
pref("browser.chrome.show_url_bar",         true);
pref("browser.chrome.show_security_bar",    true);
pref("browser.chrome.button_style",         0);

pref("browser.background_option",           0); // DEFAULT_BACKGROUND
pref("browser.link_expiration",             9);
pref("browser.cache.check_doc_frequency",   0);

pref("browser.delay_images",                false);
pref("browser.underline_anchors",           true);
pref("browser.never_expire",                false);
pref("browser.display_while_loading",       true);
pref("browser.custom_link_color",           false);
pref("browser.custom_visited_color",        false);
pref("browser.custom_text_color",           false);
pref("browser.use_document_colors",         true);
pref("browser.ldapfile_location",       "");
pref("browser.print_background",            false);
pref("browser.prefs_window.modeless",       false);
pref("browser.prefs_window_rect",           "-1,-1,-1,-1");
pref("browser.find_window_rect",            "-1,-1,-1,-1");
pref("browser.bookmark_window_rect",        "-1,-1,-1,-1");
pref("browser.download_window_rect",        "-1,-1,-1,-1");
pref("browser.wfe.ignore_def_check",false);
pref("browser.wfe.use_windows_colors",true);
pref("browser.startup_mode",1);
pref("browser.show_about_as_stupid_modal_window", false);


// various default search settings
pref("browser.search.defaulturl", "http://search.netscape.com/cgi-bin/search?search=");
pref("browser.search.opensidebarsearchpanel", true);
pref("browser.search.powermode", 0);

localDefPref("browser.bookmark_location",       "");
localDefPref("browser.addressbook_location",    "");
localDefPref("browser.socksfile_location",      "");
localDefPref("browser.ldapfile_location",       "");

pref("browser.url_history.URL_1", "");
pref("browser.url_history.URL_2", "");
pref("browser.url_history.URL_3", "");
pref("browser.url_history.URL_4", "");
pref("browser.url_history.URL_5", "");
pref("browser.url_history.URL_6", "");
pref("browser.url_history.URL_7", "");
pref("browser.url_history.URL_8", "");
pref("browser.url_history.URL_9", "");
pref("browser.url_history.URL_10", "");
pref("browser.url_history.URL_11", "");
pref("browser.url_history.URL_12", "");
pref("browser.url_history.URL_13", "");
pref("browser.url_history.URL_14", "");
pref("browser.url_history.URL_15", "");

pref("browser.uriloader", true); // turn new uri loading on by default

pref("browser.personal_toolbar_button.min_chars", 15);
pref("browser.personal_toolbar_button.max_chars", 30);

pref("browser.PICS.ratings_enabled", false);
pref("browser.PICS.pages_must_be_rated", false);
pref("browser.PICS.disable_for_this_session", false);
pref("browser.PICS.reenable_for_this_session", false);
pref("browser.PICS.service.http___home_netscape_com_default_rating.service_enabled", true);
pref("browser.PICS.service.http___home_netscape_com_default_rating.s", 0);

// gfx widgets
pref("nglayout.widget.mode", 2);
pref("nglayout.widget.gfxscrollbars", true);

// Smart Browsing prefs
pref("browser.related.enabled", true);
pref("browser.related.autoload", 1);  // 0 = Always, 1 = After first use, 2 = Never
pref("browser.related.provider", "http://www-rl.netscape.com/wtgn?");
pref("browser.related.detailsProvider", "http://cgi.netscape.com/cgi-bin/rlcgi.cgi?URL=");
pref("browser.related.disabledForDomains", "");
pref("browser.goBrowsing.enabled", true);


// The NavCenter preferences
localDefPref("browser.navcenter.dockstyle", 1); // 1 = left, 2 = right, 3 = top, 4 = bottom
localDefPref("browser.navcenter.docked.tree.visible", false);
localDefPref("browser.navcenter.docked.selector.visible", true);
localDefPref("browser.navcenter.docked.tree.width", 250); // Percent of parent window consumed by docked nav center
localDefPref("browser.navcenter.floating.rect", "20, 20, 400, 600"); // Window dimensions when floating

localDefPref("ghist.expires.pos",          4);
localDefPref("ghist.expires.width",        1400);
localDefPref("ghist.firstvisit.pos",       2);
localDefPref("ghist.firstvisit.width",     1400);
localDefPref("ghist.lastvisit.pos",        3);
localDefPref("ghist.lastvisit.width",      1400);
localDefPref("ghist.location.pos",         1);
localDefPref("ghist.location.width",       2400);
pref("ghist.show_value",           0);
pref("ghist.sort_descending",      false);
pref("ghist.sortby",               3);  // eGH_LastDateSort
localDefPref("ghist.title.pos",            0);
localDefPref("ghist.title.width",          2400);
localDefPref("ghist.visiblecolumns",       6);
localDefPref("ghist.visitcount.pos",       5);
localDefPref("ghist.visitcount.width",     1000);
localDefPref("ghist.window_rect",          "0,0,0,0");

pref("javascript.enabled",                  true);
pref("javascript.allow.mailnews",           true);
pref("javascript.allow.signing",            true);
pref("javascript.reflect_preferences",      false);     // for PE

// advanced prefs
pref("advanced.always_load_images",         true);
pref("advanced.java.allow",                 true);
pref("css.allow",                           true);
pref("advanced.mailftp",                    true);

pref("offline.startup_state",            0);
pref("offline.send.unsent_messages",            0);
pref("offline.prompt_synch_on_exit",            true);
pref("offline.news.download.use_days",          0);

pref("network.dnsAttempt",              0);
pref("network.tcptimeout",                  0);         // use default
pref("network.tcpbufsize",                  0);         //
pref("network.use_async_dns",               true);
pref("network.dnsCacheExpiration",          900); // in seconds
pref("network.enableUrlMatch",              true);
pref("network.max_connections",             4);
pref("network.speed_over_ui",               true);
pref("network.file_sort_method",            0);     // NAME 0, TYPE 1, SIZE 2, DATE 3
pref("network.ftp.passive",		    true);
pref("network.hosts.smtp_server",           "mail");
pref("network.hosts.pop_server",            "mail");

// sspitzer:  change this back to "news" when we get to beta.
// for now, set this to news.mozilla.org because you can only
// post to the server specified by this pref.
pref("network.hosts.nntp_server",           "news.mozilla.org");

pref("network.hosts.socks_server",          "");
pref("network.hosts.socks_serverport",      1080);
pref("network.hosts.socks_conf",            "");
pref("network.proxy.autoconfig_url",        "");
pref("network.proxy.type",                  0);
pref("network.proxy.ftp",                   "");
pref("network.proxy.ftp_port",              0);
pref("network.proxy.gopher",                "");
pref("network.proxy.gopher_port",           0);
pref("network.proxy.news",                  "");
pref("network.proxy.news_port",             0);
pref("network.proxy.http",                  "");
pref("network.proxy.http_port",             0);
pref("network.proxy.wais",                  "");
pref("network.proxy.wais_port",             0);
pref("network.proxy.ssl",                   "");
pref("network.proxy.ssl_port",              0);
pref("network.proxy.no_proxies_on",         "");
pref("network.online",                      true); //online/offline
pref("network.prompt_at_startup",           false);//Ask me
pref("network.accept_cookies",              0);     // 0 = Always, 1 = warn, 2 = never
pref("network.foreign_cookies",             0); // 0 = Accept, 1 = Don't accept
pref("network.cookie.cookieBehavior",       0); // 0-Accept, 1-dontAcceptForeign, 2-dontUse
pref("network.cookie.warnAboutCookies",     false);
pref("signon.rememberSignons",              true);
pref("network.sendRefererHeader",           true);
pref("network.enablePad",                   false); // Allow client to do proxy autodiscovery
pref("network.padPacURL",                   ""); // The proxy autodiscovery url
pref("wallet.captureForms",                 true);
pref("wallet.notified",                     false);
pref("wallet.fetchPatches",                 false);
pref("wallet.Server",                       "http://www.mozilla.org/wallet/tables/");
pref("wallet.version",                      "1");
pref("wallet.enabled",                      true);
pref("messages.new_window",                 true); // ML obsolete; use mailnews.message_in_thread_window
pref("intl.accept_languages",               "en");
pref("intl.mailcharset.cyrillic",           "koi8-r");
pref("intl.accept_charsets",                "iso-8859-1,*,utf-8");
pref("intl.auto_detect_encoding",           true);
pref("intl.character_set",                  2);     // CS_LATIN1
pref("intl.font_encoding",                  6);     // CS_MAC_ROMAN

pref("intl.charset_menu.static",           "iso-8859-1, iso-2022-jp, shift_jis, euc-jp");
pref("intl.charset_menu.cache",            "");

pref("browser.enable_webfonts",         true);
pref("browser.use_document_fonts",              1); // 0 = never, 1 = quick, 2 = always

// -- folders (Mac: these are binary aliases.)
localDefPref("browser.download_directory",      "");
localDefPref("browser.cache.directory",         "");
localDefPref("mail.signature_file",             "");
localDefPref("mail.directory",                  "");
localDefPref("mail.cc_file",                    "");
localDefPref("news.cc_file",                    "");

pref("news.fancy_listing",      true);      // obsolete
localDefPref("browser.cache.wfe.directory", null);
pref("browser.wfe.show_value", 1);
pref("browser.blink_allowed", true);
pref("images.dither", "auto");
pref("images.incremental_display", true);
pref("network.wfe.use_async_dns", true);
pref("network.wfe.tcp_connect_timeout",0);
localDefPref("news.directory",                  "");
localDefPref("security.directory",              "");

pref("autoupdate.enabled",              true);
pref("autoupdate.confirm_install",				false);
pref("autoupdate.unsigned_jar_support",  false);

pref("silentdownload.enabled",    true);
pref("silentdownload.directory",  "");
pref("silentdownload.range",      3000);
pref("silentdownload.interval",  10000);


pref("imap.io.mac.logging", false);

pref("browser.editor.disabled", false);

pref("SpellChecker.DefaultLanguage", 0);
pref("SpellChecker.DefaultDialect", 0);

pref("mime.table.allow_add", true);
pref("mime.table.allow_edit", true);
pref("mime.table.allow_remove", true);

//prefs for product registration/activation
pref("browser.registration.enable", false);
pref("browser.registration.url", "");
pref("browser.registration.domain", "");
pref("browser.registration.acceptdomain", "");
pref("browser.registration.mailservername", "");
pref("browser.registration.mailservertype", "");

pref("signed.applets.codebase_principal_support", false);

pref("security.policy.default.barprop.visible.write", "UniversalBrowserWrite");

pref("security.policy.default.document.createattribute", "sameOrigin");
pref("security.policy.default.document.createcdatasection", "sameOrigin");
pref("security.policy.default.document.createcomment", "sameOrigin");
pref("security.policy.default.document.createdocumentfragment", "sameOrigin");
pref("security.policy.default.document.createelement", "sameOrigin");
pref("security.policy.default.document.createentityreference", "sameOrigin");
pref("security.policy.default.document.createprocessinginstruction", "sameOrigin");
pref("security.policy.default.document.createtextnode", "sameOrigin");
pref("security.policy.default.document.doctype", "sameOrigin");
pref("security.policy.default.document.documentelement", "sameOrigin");
pref("security.policy.default.document.getelementsbytagname", "sameOrigin");
pref("security.policy.default.document.implementation", "sameOrigin");
pref("security.policy.default.htmldocument.anchors", "sameOrigin");
pref("security.policy.default.htmldocument.applets", "sameOrigin");
pref("security.policy.default.htmldocument.body", "sameOrigin");
pref("security.policy.default.htmldocument.cookie", "sameOrigin");
pref("security.policy.default.htmldocument.domain", "sameOrigin");
pref("security.policy.default.htmldocument.forms", "sameOrigin");
pref("security.policy.default.htmldocument.getelementbyid", "sameOrigin");
pref("security.policy.default.htmldocument.getelementsbyname", "sameOrigin");
pref("security.policy.default.htmldocument.links", "sameOrigin");
pref("security.policy.default.htmldocument.referrer", "sameOrigin");
pref("security.policy.default.htmldocument.title", "sameOrigin");
pref("security.policy.default.htmldocument.url", "sameOrigin");
pref("security.policy.default.nshtmldocument.alinkcolor.write", "sameOrigin");
pref("security.policy.default.nshtmldocument.bgcolor.write", "sameOrigin");
pref("security.policy.default.nshtmldocument.captureevents", "sameOrigin");
pref("security.policy.default.nshtmldocument.embeds", "sameOrigin");
pref("security.policy.default.nshtmldocument.fgcolor.write", "sameOrigin");
pref("security.policy.default.nshtmldocument.getselection", "sameOrigin");
pref("security.policy.default.nshtmldocument.lastmodified", "sameOrigin");
pref("security.policy.default.nshtmldocument.layers.write", "sameOrigin");
pref("security.policy.default.nshtmldocument.linkcolor.write", "sameOrigin");
pref("security.policy.default.nshtmldocument.nameditem", "sameOrigin");
pref("security.policy.default.nshtmldocument.open", "sameOrigin");
pref("security.policy.default.nshtmldocument.plugins", "sameOrigin");
pref("security.policy.default.nshtmldocument.releaseevents", "sameOrigin");
pref("security.policy.default.nshtmldocument.routeevent", "sameOrigin");
pref("security.policy.default.nshtmldocument.vlinkcolor.write", "sameOrigin");
pref("security.policy.default.nshtmldocument.write", "sameOrigin");
pref("security.policy.default.nshtmldocument.writeln", "sameOrigin");

pref("security.policy.default.eventtarget.addeventlistener", "sameOrigin");

pref("security.policy.default.history.current.read", "UniversalBrowserRead");
pref("security.policy.default.history.next.read", "UniversalBrowserRead");
pref("security.policy.default.history.previous.read", "UniversalBrowserRead");

pref("security.policy.default.htmlinputelement.value", "sameOrigin");

pref("security.policy.default.htmlimageelement.src", "sameOrigin");
pref("security.policy.default.htmlimageelement.lowsrc", "sameOrigin");

pref("security.policy.default.location.hash.read", "sameOrigin");
pref("security.policy.default.location.host.read", "sameOrigin");
pref("security.policy.default.location.hostname.read", "sameOrigin");
pref("security.policy.default.location.href.read", "sameOrigin");
pref("security.policy.default.location.pathname.read", "sameOrigin");
pref("security.policy.default.location.port.read", "sameOrigin");
pref("security.policy.default.location.protocol.read", "sameOrigin");
pref("security.policy.default.location.search.read", "sameOrigin");
pref("security.policy.default.location.tostring.read", "sameOrigin");

pref("security.policy.default.navigator.preference.read", "UniversalPreferencesRead");
pref("security.policy.default.navigator.preference.write", "UniversalPreferencesWrite");

pref("security.policy.default.node.appendchild", "sameOrigin");
pref("security.policy.default.node.attributes", "sameOrigin");
pref("security.policy.default.node.childnodes", "sameOrigin");
pref("security.policy.default.node.clonenode", "sameOrigin");
pref("security.policy.default.node.firstchild", "sameOrigin");
pref("security.policy.default.node.haschildnodes", "sameOrigin");
pref("security.policy.default.node.insertbefore", "sameOrigin");
pref("security.policy.default.node.lastchild", "sameOrigin");
pref("security.policy.default.node.nextsibling", "sameOrigin");
pref("security.policy.default.node.nodename", "sameOrigin");
pref("security.policy.default.node.nodetype", "sameOrigin");
pref("security.policy.default.node.nodevalue", "sameOrigin");
pref("security.policy.default.node.ownerdocument", "sameOrigin");
pref("security.policy.default.node.parentnode", "sameOrigin");
pref("security.policy.default.node.previoussibling", "sameOrigin");
pref("security.policy.default.node.removechild", "sameOrigin");
pref("security.policy.default.node.replacechild", "sameOrigin");

pref("security.policy.default.window.status", "sameOrigin");

/* 0=lines, 1=pages, 2=history */
pref("mousewheel.withnokey.action",0);
pref("mousewheel.withnokey.numlines",1);	
pref("mousewheel.withnokey.sysnumlines",true);
pref("mousewheel.withcontrolkey.action",1);
pref("mousewheel.withcontrolkey.numlines",1);
pref("mousewheel.withcontrolkey.sysnumlines",false);
pref("mousewheel.withshiftkey.action",0);
pref("mousewheel.withshiftkey.numlines",1);
pref("mousewheel.withshiftkey.sysnumlines",false);
pref("mousewheel.withaltkey.action",2);
pref("mousewheel.withaltkey.numlines",1);
pref("mousewheel.withaltkey.sysnumlines",false);

pref("profile.confirm_automigration",true);

// Customizable toolbar stuff
pref("custtoolbar.personal_toolbar_folder", "");
pref("custtoolbar.has_toolbar_folder", true);
pref("custtoolbar.Browser.Navigation_Toolbar.position", 0);
pref("custtoolbar.Browser.Navigation_Toolbar.showing", true);
pref("custtoolbar.Browser.Navigation_Toolbar.open", true);
pref("custtoolbar.Browser.Location_Toolbar.position", 1);
pref("custtoolbar.Browser.Location_Toolbar.showing", true);
pref("custtoolbar.Browser.Location_Toolbar.open", true);
pref("custtoolbar.Browser.Personal_Toolbar.position", 2);
pref("custtoolbar.Browser.Personal_Toolbar.showing", true);
pref("custtoolbar.Browser.Personal_Toolbar.open", true);
pref("custtoolbar.Messenger.Navigation_Toolbar.position", 0);
pref("custtoolbar.Messenger.Navigation_Toolbar.showing", true);
pref("custtoolbar.Messenger.Navigation_Toolbar.open", true);
pref("custtoolbar.Messenger.Location_Toolbar.position", 1);
pref("custtoolbar.Messenger.Location_Toolbar.showing", true);
pref("custtoolbar.Messenger.Location_Toolbar.open", true);
pref("custtoolbar.Messages.Navigation_Toolbar.position", 0);
pref("custtoolbar.Messages.Navigation_Toolbar.showing", true);
pref("custtoolbar.Messages.Navigation_Toolbar.open", true);
pref("custtoolbar.Messages.Location_Toolbar.position", 1);
pref("custtoolbar.Messages.Location_Toolbar.showing", true);
pref("custtoolbar.Messages.Location_Toolbar.open", true);
pref("custtoolbar.Folders.Navigation_Toolbar.position", 0);
pref("custtoolbar.Folders.Navigation_Toolbar.showing", true);
pref("custtoolbar.Folders.Navigation_Toolbar.open", true);
pref("custtoolbar.Folders.Location_Toolbar.position", 1);
pref("custtoolbar.Folders.Location_Toolbar.showing", true);
pref("custtoolbar.Folders.Location_Toolbar.open", true);
pref("custtoolbar.Address_Book.Address_Book_Toolbar.position", 0);
pref("custtoolbar.Address_Book.Address_Book_Toolbar.showing", true);
pref("custtoolbar.Address_Book.Address_Book_Toolbar.open", true);
pref("custtoolbar.Compose_Message.Message_Toolbar.position", 0);
pref("custtoolbar.Compose_Message.Message_Toolbar.showing", true);
pref("custtoolbar.Compose_Message.Message_Toolbar.open", true);
pref("custtoolbar.Composer.Composition_Toolbar.position", 0);
pref("custtoolbar.Composer.Composition_Toolbar.showing", true);
pref("custtoolbar.Composer.Composition_Toolbar.open", true);
pref("custtoolbar.Composer.Formatting_Toolbar.position", 1);
pref("custtoolbar.Composer.Formatting_Toolbar.showing", true);
pref("custtoolbar.Composer.Formatting_Toolbar.open", true);

pref("sidebar.customize.all_panels.url", "http://sidebar-rdf.netscape.com/%LOCALE%/sidebar-rdf/%SIDEBAR_VERSION%/all-panels.rdf");
