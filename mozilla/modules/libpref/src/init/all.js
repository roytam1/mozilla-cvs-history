/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// SYNTAX HINTS:  dashes are delimiters.  Use underscores instead.
//  The first character after a period must be alphabetic.

pref("general.useragent.misc", "rv:1.0.2");
// Separate the ever-changing version info to avoid merge conflicts

// see <http://www.mozilla.org/build/revised-user-agent-strings.html>
pref("general.useragent.locale", "en");
pref("general.useragent.vendor", "Beonex");
pref("general.useragent.vendorSub", "0.8.1-stable");

pref("network.search.url","http://www.google.com/search?q=");

pref("keyword.URL", "http://www.google.com/search?btnI=I%27m+Feeling+Lucky&q="); //XXX permanent?
pref("keyword.enabled", true);

pref("general.startup.browser",             true);
pref("general.startup.mail",                false);
pref("general.startup.news",                false);
pref("general.startup.editor",              false);
pref("general.startup.compose",             false);
pref("general.startup.addressbook",         false);

pref("general.open_location.last_url",      "");
pref("general.open_location.last_window_choice", 0);

pref("backups.number_of_prefs_copies", 1);

// 0 = blank, 1 = home (browser.startup.homepage), 2 = last
pref("browser.startup.page",                1);
pref("browser.startup.homepage",	   "chrome://navigator-region/locale/region.properties");
pref("browser.startup.homepage.count", 1);
// "browser.startup.homepage_override" was for 4.x
pref("browser.startup.homepage_override.beonex.0-8.1", true);

pref("browser.startup.autoload_homepage",   true);

pref("browser.cache.enable",                true); // see also network.http.use-cache
pref("browser.cache.disk.enable",           true);
pref("browser.cache.disk.capacity",         50000);
pref("browser.cache.memory.enable",         true);
pref("browser.cache.memory.capacity",       4096);
pref("browser.cache.disk_cache_ssl",        false);
// 0 = once-per-session, 1 = each-time, 2 = never, 3 = when-appropriate/automatically
pref("browser.cache.check_doc_frequency",   3);

pref("browser.display.use_document_fonts",  1);  // 0 = never, 1 = quick, 2 = always
pref("browser.display.use_document_colors", true);
pref("browser.display.use_system_colors",   false);
pref("browser.display.foreground_color",    "#000000");
pref("browser.display.background_color",    "#FFFFFF");
pref("browser.display.force_inline_alttext", false); // true = force ALT text for missing images to be layed out inline
// 0 = no external leading,
// 1 = use external leading only when font provides,
// 2 = add extra leading both internal leading and external leading are zero
pref("browser.display.normal_lineheight_calc_control", 2);
pref("browser.display.show_image_placeholders", true); // true = show image placeholders while image is loaded and when image is broken
pref("browser.anchor_color",                "#0000EE");
pref("browser.visited_color",               "#551A8B");
pref("browser.underline_anchors",           true);
pref("browser.blink_allowed",               false);

pref("browser.display.use_focus_colors",    false);
pref("browser.display.focus_background_color", "#117722");
pref("browser.display.focus_text_color",     "#ffffff");
pref("browser.display.focus_ring_width",     1);
pref("browser.display.focus_ring_on_anything", false);

pref("browser.new_find", true);  // temporary
pref("editor.new_find",  true);  // temporary

pref("browser.urlbar.autoFill", false);
pref("browser.urlbar.showPopup", true);
pref("browser.urlbar.showSearch", true);
pref("browser.urlbar.matchOnlyTyped", false);

pref("browser.chrome.site_icons", true);
pref("browser.chrome.favicons", false);

pref("browser.chrome.toolbar_tips",         true);
pref("browser.chrome.toolbar_style",        2);

pref("browser.toolbars.showbutton.bookmarks", false);
pref("browser.toolbars.showbutton.go",      false);
pref("browser.toolbars.showbutton.home",    true);
pref("browser.toolbars.showbutton.mynetscape", false);
pref("browser.toolbars.showbutton.net2phone", false);
pref("browser.toolbars.showbutton.print",   false);
pref("browser.toolbars.showbutton.search",  true);

pref("browser.turbo.enabled", false);
// pref("browser.turbo.singleProfileOnly", false);

pref("browser.helperApps.alwaysAsk.force",  false);
pref("browser.helperApps.neverAsk.saveToDisk", "application%2Foccet-stream");
pref("browser.helperApps.neverAsk.openFile", "");

pref("accessibility.browsewithcaret", false);
pref("accessibility.warn_on_browsewithcaret", true);
pref("accessibility.usetexttospeech", "");
pref("accessibility.usebrailledisplay", "");
pref("accessibility.accesskeycausesactivation", true);

// Dialog modality issues
pref("browser.prefWindowModal", true);
pref("browser.show_about_as_stupid_modal_window", false);

pref("browser.download.progressDnldDialog.keepAlive", true); // keep the dnload progress dialog up after dnload is complete
pref("browser.download.progressDnldDialog.enable_launch_reveal_buttons", true);

// various default search settings
pref("browser.search.defaulturl", "chrome://navigator-region/locale/region.properties");
pref("browser.search.opensidebarsearchpanel", true);
pref("browser.search.last_search_category", "NC:SearchCategory?category=urn:search:category:1");
pref("browser.search.mode", 1);
pref("browser.search.powermode", 0);
// basic search popup constraint: minimum sherlock plugin version displayed
// (note: must be a string representation of a float or it'll default to 0.0)
pref("browser.search.basic.min_ver", "0.0");
pref("browser.urlbar.autocomplete.enabled", true);
pref("browser.urlbar.clickSelectsAll", true);

pref("browser.history.last_page_visited", "");
pref("browser.history_expire_days", 9);
pref("browser.history.grouping", "day");
pref("browser.sessionhistory.max_entries", 50);

// Translation service
pref("browser.translation.service", "http://www.teletranslator.com:8120/?AlisUI=frames_ex/moz_home&alis_info=moz&AlisTargetURI=");
pref("browser.translation.serviceDomain", "teletranslator.com");
  
// Platform for Internet Content Selection
pref("browser.PICS.ratings_enabled", false);
pref("browser.PICS.pages_must_be_rated", false);
pref("browser.PICS.disable_for_this_session", false);
pref("browser.PICS.reenable_for_this_session", false);
pref("browser.PICS.service.http___home_netscape_com_default_rating.service_enabled", false);
pref("browser.PICS.service.http___home_netscape_com_default_rating.s", 0);

// loading and rendering of framesets and iframes
pref("browser.frames.enabled", true);

// form submission
pref("browser.forms.submit.backwards_compatible", true);

// Tab browser preferences.
pref("browser.tabs.autoHide", true);
pref("browser.tabs.forceHide", false);
pref("browser.tabs.loadInBackground", true);
pref("browser.tabs.opentabfor.middleclick", true);
pref("browser.tabs.opentabfor.urlbar", false);
pref("browser.tabs.opentabfor.windowopen", true);
pref("browser.tabs.opentabfor.bookmarks", false);

// view source
pref("view_source.syntax_highlight", true);
pref("view_source.wrap_long_lines", false);

// gfx widgets
pref("nglayout.widget.mode", 2);
pref("nglayout.widget.gfxscrollbars", true);

// css2 hover pref
pref("nglayout.events.showHierarchicalHover", false);

// dispatch left clicks only to content in browser (still allows clicks to chrome/xul)
pref("nglayout.events.dispatchLeftClickOnly", true);

// whether or not to use xbl form controls
pref("nglayout.debug.enable_xbl_forms", false);

// size of scrollbar snapping region
pref("slider.snapMultiplier", 6);

// option to choose plug-in finder
pref("application.use_ns_plugin_finder", false);

// Smart Browsing prefs
pref("browser.related.enabled", false);
pref("browser.related.autoload", 1);  // 0 = Always, 1 = After first use, 2 = Never
pref("browser.related.provider", "http://www-rl.netscape.com/wtgn?"); //XXX
pref("browser.related.disabledForDomains", "");
pref("browser.goBrowsing.enabled", false);

// URI fixup prefs
pref("browser.fixup.alternate.enabled", false);
pref("browser.fixup.alternate.prefix", "www.");
pref("browser.fixup.alternate.suffix", ".com");

//Internet Search
pref("browser.search.defaultenginename", "chrome://communicator-region/locale/region.properties");

// Print header customization
// Use the following codes:
// &T - Title
// &U - Document URL
// &D - Date/Time
// &P - Page Number
// &PT - Page Number "of" Page total
// Set each header to a string containing zero or one of these codes
// and the code will be replaced in that string by the corresponding data
pref("print.print_headerleft", "&T");
pref("print.print_headercenter", "");
pref("print.print_headerright", "&U");
pref("print.print_footerleft", "&PT");
pref("print.print_footercenter", "");
pref("print.print_footerright", "&D");
pref("print.show_print_progress", true);

// When this is set to false it means each window has its PrintSettings
// and a change in one browser window does not effect the others
pref("print.use_global_printsettings", true);

// This indicates whether it should use the native dialog or the XP Dialog50
pref("print.use_native_print_dialog", false);

// Save the Printings after each print job
pref("print.save_print_settings", true);

pref("print.whileInPrintPreview", true);

// Cache old Presentation when going into Print Preview
pref("print.always_cache_old_pres", false);

// Enables you to specify the gap from the edge of the paper to the margin
// this is used by both Printing and Print Preview
pref("print.print_edge_top", 0); // 1/100 of an inch
pref("print.print_edge_left", 0); // 1/100 of an inch
pref("print.print_edge_right", 0); // 1/100 of an inch
pref("print.print_edge_bottom", 0); // 1/100 of an inch

// Default Capability Preferences: Security-Critical! 
// Editing these may create a security risk - be sure you know what you're doing
//pref("capability.policy.default.barprop.visible.set", "UniversalBrowserWrite");

pref("capability.policy.default_policynames", "mailnews");
pref("capability.policy.policynames", "");

pref("capability.policy.default.DOMException.code", "allAccess");
pref("capability.policy.default.DOMException.message", "allAccess");
pref("capability.policy.default.DOMException.name", "allAccess");
pref("capability.policy.default.DOMException.result", "allAccess");
pref("capability.policy.default.DOMException.toString", "allAccess");

pref("capability.policy.default.History.back", "allAccess");
pref("capability.policy.default.History.current", "UniversalBrowserRead");
pref("capability.policy.default.History.forward", "allAccess");
pref("capability.policy.default.History.go", "allAccess");
pref("capability.policy.default.History.item", "UniversalBrowserRead");
pref("capability.policy.default.History.next", "UniversalBrowserRead");
pref("capability.policy.default.History.previous", "UniversalBrowserRead");
pref("capability.policy.default.History.toString", "UniversalBrowserRead");

pref("capability.policy.default.HTMLDocument.close", "allAccess");
pref("capability.policy.default.HTMLDocument.open", "allAccess");

pref("capability.policy.default.Location.hash.set", "allAccess");
pref("capability.policy.default.Location.href.set", "allAccess");
pref("capability.policy.default.Location.reload", "allAccess");
pref("capability.policy.default.Location.replace", "allAccess");

pref("capability.policy.default.Navigator.preference", "allAccess");
pref("capability.policy.default.Navigator.preferenceinternal.get", "UniversalPreferencesRead");
pref("capability.policy.default.Navigator.preferenceinternal.set", "UniversalPreferencesWrite");

pref("capability.policy.default.Window.blur", "allAccess");
pref("capability.policy.default.Window.close", "allAccess");
pref("capability.policy.default.Window.closed", "allAccess");
pref("capability.policy.default.Window.Components", "allAccess");
pref("capability.policy.default.Window.document", "allAccess");
pref("capability.policy.default.Window.focus", "allAccess");
pref("capability.policy.default.Window.frames", "allAccess");
pref("capability.policy.default.Window.fullScreen", "noAccess");
pref("capability.policy.default.Window.history", "allAccess");
pref("capability.policy.default.Window.length", "allAccess");
pref("capability.policy.default.Window.location", "allAccess");
pref("capability.policy.default.Window.opener", "allAccess");
pref("capability.policy.default.Window.parent", "allAccess");
pref("capability.policy.default.Window.self", "allAccess");
pref("capability.policy.default.Window.top", "allAccess");
pref("capability.policy.default.Window.window", "allAccess");

// Restrictions on the DOM for mail/news - see bugs 66938 and 84545
pref("capability.policy.mailnews.sites", "mailbox: imap: news:");

pref("capability.policy.mailnews.*.attributes.get", "noAccess");
pref("capability.policy.mailnews.*.baseURI.get", "noAccess");
pref("capability.policy.mailnews.*.data.get", "noAccess");
pref("capability.policy.mailnews.*.getAttribute", "noAccess");
pref("capability.policy.mailnews.*.getNamedItem", "noAccess");
pref("capability.policy.mailnews.*.host.get", "noAccess");
pref("capability.policy.mailnews.*.hostname.get", "noAccess");
pref("capability.policy.mailnews.*.href.get", "noAccess");
pref("capability.policy.mailnews.*.innerHTML.get", "noAccess");
pref("capability.policy.mailnews.*.lowSrc.get", "noAccess");
pref("capability.policy.mailnews.*.nodeValue.get", "noAccess");
pref("capability.policy.mailnews.*.pathname.get", "noAccess");
pref("capability.policy.mailnews.*.protocol.get", "noAccess");
pref("capability.policy.mailnews.*.src.get", "noAccess");
pref("capability.policy.mailnews.*.substringData.get", "noAccess");
pref("capability.policy.mailnews.*.text.get", "noAccess");
pref("capability.policy.mailnews.*.title.get", "noAccess");
pref("capability.policy.mailnews.DOMException.toString", "noAccess");
pref("capability.policy.mailnews.HTMLAnchorElement.toString", "noAccess");
pref("capability.policy.mailnews.HTMLDocument.domain", "noAccess");
pref("capability.policy.mailnews.HTMLDocument.URL", "noAccess");
pref("capability.policy.mailnews.Location.toString", "noAccess");
pref("capability.policy.mailnews.Range.toString", "noAccess");
pref("capability.policy.mailnews.Window.blur", "noAccess");
pref("capability.policy.mailnews.Window.focus", "noAccess");
pref("capability.policy.mailnews.Window.innerWidth.set", "noAccess");
pref("capability.policy.mailnews.Window.innerHeight.set", "noAccess");
pref("capability.policy.mailnews.Window.moveBy", "noAccess");
pref("capability.policy.mailnews.Window.moveTo", "noAccess");
pref("capability.policy.mailnews.Window.name.set", "noAccess");
pref("capability.policy.mailnews.Window.outerHeight.set", "noAccess");
pref("capability.policy.mailnews.Window.outerWidth.set", "noAccess");
pref("capability.policy.mailnews.Window.resizeBy", "noAccess");
pref("capability.policy.mailnews.Window.resizeTo", "noAccess");
pref("capability.policy.mailnews.Window.screenX.set", "noAccess");
pref("capability.policy.mailnews.Window.screenY.set", "noAccess");
pref("capability.policy.mailnews.Window.sizeToContent", "noAccess");
pref("capability.policy.mailnews.document.load", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.channel", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.responseXML", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.responseText", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.status", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.statusText", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.abort", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.getAllResponseHeaders", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.getResponseHeader", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.open", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.send", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.setRequestHeader", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.readyState", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.overrideMimeType", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.onload", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.onerror", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.onreadystatechange", "noAccess");
pref("capability.policy.mailnews.XMLSerializer.serializeToString", "noAccess");
pref("capability.policy.mailnews.XMLSerializer.serializeToStream", "noAccess");
pref("capability.policy.mailnews.DOMParser,parseFromString", "noAccess");
pref("capability.policy.mailnews.DOMParser,parseFromStream", "noAccess");
pref("capability.policy.mailnews.SOAPCall.transportURI", "noAccess");
pref("capability.policy.mailnews.SOAPCall.verifySourceHeader", "noAccess");
pref("capability.policy.mailnews.SOAPCall.invoke", "noAccess");
pref("capability.policy.mailnews.SOAPCall.asyncInvoke", "noAccess");
pref("capability.policy.mailnews.SOAPResponse.fault", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.styleURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getAssociatedEncoding", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.setEncoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getEncoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.setDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.setDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.defaultEncoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.defaultDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.schemaCollection", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.encode", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.decode", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.mapSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.unmapSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getInternalSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getExternalSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPFault.element", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultNamespaceURI", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultCode", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultString", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultActor", "noAccess");
pref("capability.policy.mailnews.SOAPFault.detail", "noAccess");
pref("capability.policy.mailnews.SOAPHeaderBlock.actorURI", "noAccess");
pref("capability.policy.mailnews.SOAPHeaderBlock.mustUnderstand", "noAccess");
pref("capability.policy.mailnews.SOAPParameter", "noAccess");
pref("capability.policy.mailnews.SOAPPropertyBagMutator.propertyBag", "noAccess");
pref("capability.policy.mailnews.SOAPPropertyBagMutator.addProperty", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.load", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.loadAsync", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.processSchemaElement", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.onLoad", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.onError", "noAccess");

// Scripts & Windows prefs
pref("browser.block.target_new_window",     false);
pref("dom.disable_cookie_get",              false);
pref("dom.disable_cookie_set",              false);
pref("dom.disable_image_src_set",           false);
pref("dom.disable_open_during_load",        false);
pref("dom.disable_window_flip",             true);
pref("dom.disable_window_move_resize",      true);
pref("dom.disable_window_status_change",    true);

pref("dom.disable_window_open_feature.status",      false);

pref("javascript.enabled",                  true);
pref("javascript.allow.mailnews",           false);
pref("javascript.options.strict",           false);
pref("javascript.options.showInConsole",    true);
pref("javascript.strict_domain_checking",   false);

// advanced prefs
pref("advanced.always_load_images",         true);
pref("security.enable_java",                false);
pref("css.allow",                           true);
pref("advanced.mailftp",                    false);
pref("image.animation_mode",                "normal");

pref("offline.startup_state",            0);
pref("offline.send.unsent_messages",            0);
pref("offline.download.download_messages",  0);
pref("offline.prompt_synch_on_exit",            true);
pref("offline.news.download.use_days",          0);

// If there is ever a security firedrill that requires
// us to block certian ports global, this is the pref 
// to use.  Is is a comma delimited list of port numbers
// for example:
//   pref("network.security.ports.banned", "1,2,3,4,5");
// prevents necko connecting to ports 1-5 unless the protocol
// overrides.

// Prevent using external protocol handlers for these schemes
pref("network.protocol-handler.external.hcp", false);
pref("network.protocol-handler.external.vbscript", false);
pref("network.protocol-handler.external.javascript", false);
pref("network.protocol-handler.external.ms-help", false);
pref("network.protocol-handler.external.vnd.ms.radio", false);
// because we disable unknown protocols by default (see below),
// enable a few well-known and presumably safe ones. please review choices.
// Source: <http://www.w3.org/Addressing/schemes>
// Attention: This disables any possible internal handlers, so don't list
// them here.
pref("network.protocol-handler.external.callto", true); // telephony
pref("network.protocol-handler.external.fax", true); // dito
pref("network.protocol-handler.external.tel", true); // dito
pref("network.protocol-handler.external.h324", true); // dito
pref("network.protocol-handler.external.h323", true); // dito
pref("network.protocol-handler.external.sip", true); // dito
pref("network.protocol-handler.external.freenet", true); // freenetproject.org
pref("network.protocol-handler.external.rtsp", true); // realplayer etc.
pref("network.protocol-handler.external.rtp", true); // dito
pref("network.protocol-handler.external.pnm", true); // dito
pref("network.protocol-handler.external.telnet", true); // shell services
pref("network.protocol-handler.external.ssh", true); // dito
pref("network.protocol-handler.external.tn3270", true); // dito?
pref("network.protocol-handler.external.wais", true);
pref("network.protocol-handler.external.z3950r", true);
pref("network.protocol-handler.external.z3950s", true);
pref("network.protocol-handler.external.dict", true); // dictionary lookup
pref("network.protocol-handler.useSystemDefaults", false); // this is for url schemes. ask the OS. a lot of dangerous crap might be there (Windows Help with potential exploits), but some useful stuff as well (RealPlayer). We try to enable the useful stuff explicitly above. Mozilla doesn't have this pref at all, hardcoded to true.
pref("network.protocols.useSystemDefaults", false); // *headbang* this is not for protocols or url schemes, but mimetypes.

pref("network.hosts.smtp_server",           "mail");
pref("network.hosts.pop_server",            "mail");

// <http>
pref("network.http.version", "1.1");	  // default
// pref("network.http.version", "1.0");   // uncomment this out in case of problems
// pref("network.http.version", "0.9");   // it'll work too if you're crazy
// keep-alive option is effectively obsolete. Nevertheless it'll work with
// some older 1.0 servers:

pref("network.http.proxy.version", "1.1");    // default
// pref("network.http.proxy.version", "1.0"); // uncomment this out in case of problems
                                              // (required if using junkbuster proxy)

// enable caching of http documents
pref("network.http.use-cache", true);

// this preference can be set to override the socket type used for normal
// HTTP traffic.  an empty value indicates the normal TCP/IP socket type.
pref("network.http.default-socket-type", "");

pref("network.http.keep-alive", true); // set it to false in case of problems
pref("network.http.proxy.keep-alive", true);
pref("network.http.keep-alive.timeout", 300);

// limit the absolute number of http connections.
pref("network.http.max-connections", 24);

// limit the absolute number of http connections that can be established per
// host.  if a http proxy server is enabled, then the "server" is the proxy
// server.  Otherwise, "server" is the http origin server.
pref("network.http.max-connections-per-server", 8);

// if network.http.keep-alive is true, and if NOT connecting via a proxy, then
// a new connection will only be attempted if the number of active persistent
// connections to the server is less then max-persistent-connections-per-server.
pref("network.http.max-persistent-connections-per-server", 2);

// if network.http.keep-alive is true, and if connecting via a proxy, then a
// new connection will only be attempted if the number of active persistent
// connections to the proxy is less then max-persistent-connections-per-proxy.
pref("network.http.max-persistent-connections-per-proxy", 4);

// amount of time (in seconds) to suspend pending requests, before spawning a
// new connection, once the limit on the number of persistent connections per
// host has been reached.  however, a new connection will not be created if
// max-connections or max-connections-per-server has also been reached.
pref("network.http.request.max-start-delay", 10);

// http specific network timeouts (XXX currently unused)
pref("network.http.connect.timeout",  30);	// in seconds
pref("network.http.request.timeout", 120);	// in seconds

// Headers
pref("network.http.accept.default", "text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,video/x-mng,image/png,image/jpeg,image/gif;q=0.2,text/css,*/*;q=0.1");


// Controls how and when the referrer header will be sent:
//  0 - never send the referrer
//  1 - send only on HTTP link clicks
//  2 - send for both HTTP link clicks and inlines (images)
//  3 - not currently used
//  4 - send the referrer only to the same home, otherwise none
//  5 - send the referrer to the same host, otherwise send a faked referrer
//  6 - always send a faked referrer (base URL of target server)
pref("network.http.sendRefererHeader", 5);

// If true, the modified referrer will be sent for schemes which
// normally wouldn't send a referrer, such as file: and resource: and imap:
pref("network.http.referrerSchemeOverride", false);

// This pref controls whether or not we send HTTPS referrers
// cross domain.  By default, this is enabled for compatibility
// with other browsers and many websites (see bug 141641).
pref("network.http.sendSecureXSiteReferrer", true);

// Maximum number of consecutive redirects before aborting.
pref("network.http.redirection-limit", 20);

// Enable http compression: comment this out in case of problems with 1.1
pref("network.http.accept-encoding" ,"gzip, deflate, compress;q=0.9");

pref("network.http.pipelining"      , false);
pref("network.http.proxy.pipelining", false);

// Always pipeling the very first request:  this will only work when you are
// absolutely sure the the site or proxy you are browsing to/through support
// pipelining; the default behavior will be that the browser will first make
// a normal, non-pipelined request, then  examine  and remember the responce
// and only the subsequent requests to that site will be pipeline
pref("network.http.pipelining.firstrequest", false);

// Max number of requests in the pipeline
pref("network.http.pipelining.maxrequests" , 4);

pref("network.http.proxy.ssl.connect",true);
// </http>

// This preference controls whether or not internationalized domain names (IDN)
// are handled.  IDN requires a nsIIDNService implementation.
pref("network.enableIDN", true);

// This preference controls whether or not URLs with UTF-8 characters are
// escaped.  Set this preference to TRUE for strict RFC2396 conformance.
pref("network.standard-url.escape-utf8", true);

// Idle timeout for ftp control connections - 5 minute default
pref("network.ftp.idleConnectionTimeout", 300);

// directory listing format - constants are defined in nsIDirectoryListing.idl
// Do not set this to 0...
pref("network.dir.format", 2);

// enables the prefetch service (i.e., prefetching of <link rel="next"> URLs).
pref("network.prefetch-next", false);

// sspitzer:  change this back to "news" when we get to beta.
// for now, set this to news.mozilla.org because you can only
// post to the server specified by this pref.
pref("network.hosts.nntp_server",           "news.mozilla.org");

pref("network.hosts.socks_server",          "");
pref("network.hosts.socks_serverport",      1080);
pref("network.hosts.socks_conf",            "");
pref("network.image.imageBehavior",         0); // 0-Accept, 1-dontAcceptForeign, 2-dontUse
pref("network.image.warnAboutImages",       false);
pref("network.proxy.type",                  0); // 0=Direct, 1=Manual, 2=autoconfig
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
pref("network.proxy.socks",                 "");
pref("network.proxy.socks_port",            0);
pref("network.proxy.socks_version",         5);
pref("network.proxy.no_proxies_on",         "");
pref("network.proxy.autoconfig_url",        "");
pref("network.online",                      true); //online/offline
pref("network.accept_cookies",              0);     // 0 = Always, 1 = warn, 2 = never
pref("network.foreign_cookies",             1); // 0=Accept, 1=Don't accept
pref("network.cookie.cookieBehavior",       1); // 0=Accept, 1=dontAcceptForeign, 2=dontUse, 3=p3p
pref("network.cookie.disableCookieForMailNews", true); // disable all cookies for mail
pref("network.cookie.warnAboutCookies",     false);
pref("network.cookie.lifetime.enabled",     true);
pref("network.cookie.lifetime.behavior",    0); // 0-session, 1-days
pref("network.cookie.lifetime.days",        90);
/* morse, why did you break backwards-compatibility and made the pref more
   inflexible, just to make the implementation of the prefs forntend easier??*/
/* network.cookie.p3plevel consists of 8 characters having the
 * following interpretation:
 *   [0]: first-party cookies when site has no privacy policy
 *   [1]: third-party cookies when site has no privacy policy
 *   [2]: first-party cookies when site uses PII with no user consent
 *   [3]: third-party cookies when site uses PII with no user consent
 *   [4]: first-party cookies when site uses PII with implicit consent only
 *   [5]: third-party cookies when site uses PII with implicit consent only
 *   [6]: first-party cookies when site uses PII with explicit consent
 *   [7]: third-party cookies when site uses PII with explicit consent
 * PII = personally identifiable information
 *
 * each of the eight characters can be one of the following
 *   'a': accept the cookie
 *   'd': accept the cookie but downgrade it to a session cookie
 *   'r': reject the cookie
 */
pref("network.cookie.p3p",                  "drrrrrdr");
pref("network.cookie.p3plevel",             3);  // 0=low, 1=medium, 2=high, 3=custom
// See extensions/cookie/resources/content/p3p.xul for the definitions of low/medium/hi
pref("signon.rememberSignons",              true);
pref("signon.expireMasterPassword",         false);
pref("network.enablePad",                   false); // Allow client to do proxy autodiscovery
pref("network.enableIDN",                   false); // Turn on/off IDN (Internationalized Domain Name) resolution
pref("converter.html2txt.structs",          true); // Output structured phrases (strong, em, code, sub, sup, b, i, u)
pref("converter.html2txt.header_strategy",  1); // 0 = no indention; 1 = indention, increased with header level; 2 = numbering and slight indention
pref("wallet.captureForms",                 true);
pref("wallet.notified",                     false);
pref("wallet.TutorialFromMenu",             "chrome://navigator/locale/navigator.properties");
pref("wallet.Server",                       "chrome://navigator/locale/navigator.properties");
pref("wallet.Samples",                      "chrome://navigator/locale/navigator.properties");
pref("wallet.version",                      "1");
pref("wallet.enabled",                      true);
pref("wallet.crypto",                       false);
pref("wallet.crypto.autocompleteoverride",  false); // Ignore 'autocomplete=off' - available only when wallet.crypto is enabled. 
pref("wallet.namePanel.hide",               false);
pref("wallet.addressPanel.hide",            false);
pref("wallet.phonePanel.hide",              false);
pref("wallet.creditPanel.hide",             false);
pref("wallet.employPanel.hide",             false);
pref("wallet.miscPanel.hide",               false);
pref("imageblocker.enabled",                true);
pref("intl.accept_languages",               "chrome://navigator/locale/navigator.properties");
pref("intl.accept_charsets",                "iso-8859-1,*,utf-8");
pref("intl.collationOption",                "chrome://navigator-platform/locale/navigator.properties");
pref("intl.menuitems.alwaysappendacceskeys","chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.static",     "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more1",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more2",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more3",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more4",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more5",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.mailedit",           "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.cache",      "");
pref("intl.charsetmenu.mailview.cache",     "");
pref("intl.charsetmenu.composer.cache",     "");
pref("intl.charsetmenu.browser.cache.size", 5);
pref("intl.charset.detector",               "chrome://navigator/locale/navigator.properties");
pref("intl.charset.default",                "chrome://navigator-platform/locale/navigator.properties");
pref("intl.content.langcode",               "chrome://communicator-region/locale/region.properties");
pref("intl.locale.matchOS",                 false);
// fallback charset list for Unicode conversion (converting from Unicode)
// currently used for mail send only to handle symbol characters (e.g Euro, trademark, smartquotes)
// for ISO-8859-1
pref("intl.fallbackCharsetList.ISO-8859-1", "windows-1252");
pref("font.language.group",                 "chrome://navigator/locale/navigator.properties");

// -- folders (Mac: these are binary aliases.)
localDefPref("mail.signature_file",             "");
localDefPref("mail.directory",                  "");

pref("images.dither", "auto");
localDefPref("news.directory",                  "");
localDefPref("security.directory",              "");

pref("autoupdate.enabled",              true);

pref("browser.editor.disabled", false);

pref("spellchecker.dictionary", "");

pref("signed.applets.codebase_principal_support", false);
pref("security.checkloaduri", true);
pref("security.xpconnect.plugin.unrestricted", true);

// Modifier key prefs: default to Windows settings,
// menu access key = alt, accelerator key = control.
pref("ui.key.accelKey", 17);
pref("ui.key.menuAccessKey", 18);
pref("ui.key.menuAccessKeyFocuses", false);
pref("ui.key.saveLink.shift", true); // true = shift, false = meta

// Middle-mouse handling
pref("middlemouse.paste", false);
pref("middlemouse.openNewWindow", true);
pref("middlemouse.contentLoadURL", false);
pref("middlemouse.scrollbarPosition", false);

// Clipboard behavior
pref("clipboard.autocopy", false);

// 0=lines, 1=pages, 2=history , 3=text size
pref("mousewheel.withnokey.action",0);
pref("mousewheel.withnokey.numlines",1);	
pref("mousewheel.withnokey.sysnumlines",true);
pref("mousewheel.withcontrolkey.action",0);
pref("mousewheel.withcontrolkey.numlines",1);
pref("mousewheel.withcontrolkey.sysnumlines",true);
pref("mousewheel.withshiftkey.action",0);
pref("mousewheel.withshiftkey.numlines",1);
pref("mousewheel.withshiftkey.sysnumlines",false);
pref("mousewheel.withaltkey.action",2);
pref("mousewheel.withaltkey.numlines",1);
pref("mousewheel.withaltkey.sysnumlines",false);

pref("profile.confirm_automigration",true);

// the amount of time (in seconds) that must elapse
// before we think your mozilla profile is defunct
// and you'd benefit from re-migrating from 4.x
// see bug #137886 for more details
//
// if -1, we never think your profile is defunct
// and users will never see the remigrate UI.
pref("profile.seconds_until_defunct", -1);

// Customizable toolbar stuff
pref("custtoolbar.personal_toolbar_folder", "");

//pref("sidebar.customize.all_panels.url", "chrome://communicator/locale/sidebar/net-panels.rdf");
//pref("sidebar.customize.all_panels.url", "http://sidebar-rdf.netscape.com/%LOCALE%/sidebar-rdf/%SIDEBAR_VERSION%/all-panels.rdf");
pref("sidebar.customize.all_panels.url", "");
//pref("sidebar.customize.directory.url", "http://dmoz.org/Netscape/Sidebar/");
pref("sidebar.customize.directory.url", "http://www.beonex.com/communicator/redir/panel");
pref("sidebar.customize.more_panels.url", "http://www.beonex.com/communicator/redir/panel");
pref("sidebar.num_tabs_in_view", 8);

pref("prefs.converted-to-utf8",false);
// --------------------------------------------------
// IBMBIDI 
// --------------------------------------------------
//
// ------------------
//  Text Direction
// ------------------
// 1 = directionLTRBidi *
// 2 = directionRTLBidi
pref("bidi.direction", 1);
// ------------------
//  Text Type
// ------------------
// 1 = charsettexttypeBidi *
// 2 = logicaltexttypeBidi
// 3 = visualtexttypeBidi
pref("bidi.texttype", 1);
// ------------------
//  Controls Text Mode
// ------------------
// 1 = logicalcontrolstextmodeBidiCmd
// 2 = visiualcontrolstextmodeBidi
// 3 = containercontrolstextmodeBidi *
pref("bidi.controlstextmode", 1);
// ------------------
//  Clipboard Text Mode
// ------------------
//  1 = logicalclipboardtextmodeBidi
// 2 = visiualclipboardtextmodeBidi
// 3 = sourceclipboardtextmodeBidi *
pref("bidi.clipboardtextmode", 3);
// ------------------
//  Numeral Style
// ------------------
// 1 = regularcontextnumeralBidi *
// 2 = hindicontextnumeralBidi
// 3 = arabicnumeralBidi
// 4 = hindinumeralBidi
pref("bidi.numeral", 1);
// ------------------
//  Support Mode
// ------------------
// 1 = mozillaBidisupport *
// 2 = OsBidisupport
// 3 = disableBidisupport
pref("bidi.support", 1);
// ------------------
//  Charset Mode
// ------------------
// 1 = doccharactersetBidi *
// 2 = defaultcharactersetBidi
pref("bidi.characterset", 1);


pref("browser.throbber.url","chrome://navigator-region/locale/region.properties");

// used for double-click word selection behavior. Win will override.
pref("layout.word_select.eat_space_to_next_word", false);
pref("layout.word_select.stop_at_punctuation", true);

// pref to force frames to be resizable
pref("layout.frames.force_resizability", false);

// pref to permit users to make verified SOAP calls by default
pref("capability.policy.default.SOAPCall.invokeVerifySourceHeader", "allAccess");

// pref to control the alert notification 
pref("alerts.slideIncrement", 1);
pref("alerts.slideIncrementTime", 10);
pref("alerts.totalOpenTime", 4000);
pref("alerts.height", 50);

// update notifications prefs
pref("update_notifications.enabled", true);
pref("update_notifications.provider.0.frequency", 1); // number of days
pref("update_notifications.provider.0.datasource", "chrome://communicator-region/locale/region.properties");

// 0 opens the download manager
// 1 opens a progress dialog
// 2 and other values, no download manager, no progress dialog. 
pref("browser.downloadmanager.behavior", 1);

// if true, allow plug-ins to override internal imglib decoder mime types in full-page mode
pref("plugin.override_internal_types", false);
pref("plugin.expose_full_path",false); // if true navigator.plugins reveals full path

// if true, enable XSLT (if installed)
pref("xslt.enabled", true);

// Help Windows NT, 2000, and XP dialup a RAS connection
// when a network address is unreachable.
pref("network.autodial-helper.enabled", false);
