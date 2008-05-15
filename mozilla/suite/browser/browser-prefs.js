/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 *   Benjamin Smedberg <bsmedberg@covad.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/* The prefs in this file are specific to the seamonkey (toolkit) browser.
 * Generic default prefs that would be useful to embedders belong in
 * modules/libpref/src/init/all.js
 */

pref("startup.homepage_override_url","chrome://navigator-region/locale/region.properties");
pref("general.skins.selectedSkin", "classic/1.0");

pref("browser.chromeURL","chrome://navigator/content/navigator.xul");
pref("browser.hiddenWindowChromeURL", "chrome://navigator/content/hiddenWindow.xul");

pref("general.startup.browser",             true);
pref("general.startup.mail",                false);
pref("general.startup.news",                false);
pref("general.startup.editor",              false);
pref("general.startup.compose",             false);
pref("general.startup.addressbook",         false);

pref("general.open_location.last_url",      "");
pref("general.open_location.last_window_choice", 0);

pref("general.smoothScroll", false);
pref("general.autoScroll", true);

#expand pref("general.useragent.extra.__MOZ_APP_NAME__", "__MOZ_APP_DISPLAYNAME__/__MOZ_APP_VERSION__");

// 0 = blank, 1 = home (browser.startup.homepage), 2 = last
pref("browser.startup.page",                1);
pref("browser.startup.homepage",	   "chrome://navigator-region/locale/region.properties");
pref("browser.startup.homepage.count", 1);

// disable this until it can be disabled on a per-docshell basis (see bug 319368)
pref("browser.send_pings", false);

pref("browser.urlbar.autoFill", false);
pref("browser.urlbar.showPopup", true);
pref("browser.urlbar.showSearch", true);
pref("browser.urlbar.matchOnlyTyped", false);

pref("browser.chrome.site_icons", true);
pref("browser.chrome.favicons", false);
pref("browser.chrome.image_icons.max_size", 1024);

// 0 = never, 1 = when in cache, 2 = always
pref("browser.chrome.load_toolbar_icons", 0);

pref("browser.toolbars.showbutton.bookmarks", true);
pref("browser.toolbars.showbutton.go",      false);
pref("browser.toolbars.showbutton.home",    true);
pref("browser.toolbars.showbutton.print",   true);
pref("browser.toolbars.showbutton.search",  true);

pref("browser.download.progressDnldDialog.keepAlive", true); // keep the dnload progress dialog up after dnload is complete
pref("browser.download.progressDnldDialog.enable_launch_reveal_buttons", true);
pref("browser.download.progressDnlgDialog.dontAskForLaunch", false);
pref("browser.download.finished_download_sound", false);
pref("browser.download.finished_download_alert", false);
pref("browser.download.finished_sound_url", "");
pref("browser.download.autoDownload", false);
pref("browser.download.lastLocation", true);

// various default search settings
pref("browser.search.defaulturl", "chrome://navigator-region/locale/region.properties");
pref("browser.search.opensidebarsearchpanel", true);
pref("browser.search.opentabforcontextsearch", false);
pref("browser.search.last_search_category", "NC:SearchCategory?category=urn:search:category:1");
pref("browser.search.mode", 0);
// basic search popup constraint: minimum sherlock plugin version displayed
// (note: must be a string representation of a float or it'll default to 0.0)
pref("browser.search.basic.min_ver", "0.0");
pref("browser.urlbar.autocomplete.enabled", true);
pref("browser.urlbar.clickSelectsAll", true);
// when clickSelectsAll=true, does it also apply when the click is past end of text?
pref("browser.urlbar.clickAtEndSelects", true);

pref("browser.search.param.Google.1.custom", "chrome://navigator/content/searchconfig.properties");
pref("browser.search.param.Google.1.default", "chrome://navigator/content/searchconfig.properties");

pref("browser.history.grouping", "day");
pref("browser.sessionhistory.max_entries", 50);

// Tabbed browser
pref("browser.tabs.loadDivertedInBackground", false);
pref("browser.tabs.loadInBackground", false);
pref("browser.tabs.opentabfor.middleclick", false);
pref("browser.tabs.opentabfor.urlbar", false);
pref("browser.tabs.tooltippreview.enable", true);
pref("browser.tabs.tooltippreview.width", 300);
pref("browser.tabs.undoclose.depth", 3);
pref("browser.tabs.autoHide", true);
pref("browser.tabs.forceHide", false);
pref("browser.tabs.warnOnClose", true);
pref("browser.tabs.warnOnCloseOther", true);
// 0 = append, 1 = replace
pref("browser.tabs.loadGroup", 1);

// lets new tab/window load something different than first window
// -1 - use navigator startup preference
//  0 - loads blank page
//  1 - loads home page
//  2 - loads last page visited
pref("browser.tabs.loadOnNewTab", 0);
pref("browser.windows.loadOnNewWindow", 1);

// external link handling in tabbed browsers. values from nsIBrowserDOMWindow.
// 0=default window, 1=current window/tab, 2=new window, 3=new tab in most recent window
pref("browser.link.open_external", 2); // open externally-launched links in a new window

// handle links targeting new windows
pref("browser.link.open_newwindow", 2);

// 0: no restrictions - divert everything
// 1: don't divert window.open at all
// 2: don't divert window.open with features
pref("browser.link.open_newwindow.restriction", 0); 

// Translation service
pref("browser.translation.service", "chrome://navigator-region/locale/region.properties");
pref("browser.translation.serviceDomain", "chrome://navigator-region/locale/region.properties");
  
// Smart Browsing prefs
pref("browser.related.provider", "http://www-rl.netscape.com/wtgn?");
pref("browser.related.disabledForDomains", "");
pref("keyword.enabled", true);

//Internet Search
pref("browser.search.defaultenginename", "chrome://communicator-region/locale/region.properties");

// 0 goes back
// 1 act like pgup
// 2 and other values, nothing
pref("browser.backspace_action", 0);

pref("browser.zoom.full", true);

pref("javascript.options.showInConsole",    true);

pref("offline.startup_state",            0);
pref("offline.send.unsent_messages",            0);
pref("offline.download.download_messages",  0);

pref("signon.rememberSignons",              true);
pref("signon.expireMasterPassword",         false);

pref("browser.formfill.enable",             true);
pref("wallet.captureForms",                 false);
pref("wallet.enabled",                      true);
pref("wallet.crypto",                       false);
pref("wallet.crypto.autocompleteoverride",  false); // Ignore 'autocomplete=off' - available only when wallet.crypto is enabled. 
pref("wallet.namePanel.hide",               false);
pref("wallet.addressPanel.hide",            false);
pref("wallet.phonePanel.hide",              false);
pref("wallet.creditPanel.hide",             false);
pref("wallet.employPanel.hide",             false);
pref("wallet.miscPanel.hide",               false);

// -- folders (Mac: these are binary aliases.)
pref("mail.signature_file",             "");
pref("mail.directory",                  "");
pref("news.directory",                  "");
pref("spellchecker.dictionaries.download.url", "chrome://branding/locale/brand.properties");

// this will automatically enable inline spellchecking (if it is available) for
// editable elements in HTML
// 0 = spellcheck nothing
// 1 = check multi-line controls [default]
// 2 = check multi/single line controls
pref("layout.spellcheckDefault", 1);

// special TypeAheadFind settings
pref("accessibility.typeaheadfind.flashBar", 0);

#ifdef XP_WIN
pref("browser.preferences.instantApply", false);
#else
pref("browser.preferences.instantApply", true);
#endif
#ifdef XP_MACOSX
pref("browser.preferences.animateFadeIn", true);
#else
pref("browser.preferences.animateFadeIn", false);
#endif

pref("browser.download.show_plugins_in_list", true);
pref("browser.download.hide_plugins_without_extensions", true);

// initial web feed readers list - add enough entries for locales to add theirs
pref("browser.contentHandlers.types.0.title", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.0.uri", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.0.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.1.title", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.1.uri", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.1.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.2.title", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.2.uri", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.2.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.3.title", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.3.uri", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.3.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.4.title", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.4.uri", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.4.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.5.title", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.5.uri", "chrome://navigator-region/locale/region.properties");
pref("browser.contentHandlers.types.5.type", "application/vnd.mozilla.maybe.feed");

pref("browser.feeds.handler", "ask");
pref("browser.videoFeeds.handler", "ask");
pref("browser.audioFeeds.handler", "ask");

pref("shell.checkDefaultClient", true);

pref("app.releaseNotesURL", "chrome://branding/locale/brand.properties");
pref("app.vendorURL", "chrome://branding/locale/brand.properties");

// App-specific update preferences

// Whether or not app updates are enabled
pref("app.update.enabled", true);
 
// This preference turns on app.update.mode and allows automatic download and
// install to take place. We use a separate boolean toggle for this to make
// the UI easier to construct.
pref("app.update.auto", true);

// Defines how the Application Update Service notifies the user about updates:
//
// AUM Set to:        Minor Releases:     Major Releases:
// 0                  download no prompt  download no prompt
// 1                  download no prompt  download no prompt if no incompatibilities
// 2                  download no prompt  prompt
//
// See chart in nsUpdateService.js.in for more details
//
pref("app.update.mode", 1);

// If set to true, the Update Service will present no UI for any event.
pref("app.update.silent", false);

// Update service URL:
pref("app.update.url", "https://aus2-community.mozilla.org/update/1/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/update.xml");
// URL user can browse to manually if for some reason all update installation
// attempts fail.
pref("app.update.url.manual", "http://www.seamonkey-project.org/");
// A default value for the "More information about this update" link
// supplied in the "An update is available" page of the update wizard. 
pref("app.update.url.details", "chrome://communicator-region/locale/region.properties");
 
// User-settable override to app.update.url for testing purposes.
//pref("app.update.url.override", "");
 
// Interval: Time between checks for a new version (in seconds)
//           default=1 day
pref("app.update.interval", 86400);
// Interval: Time before prompting the user again to restart to install the
//           latest download (in seconds) default=1 day
pref("app.update.nagTimer.restart", 86400);
// Interval: When all registered timers should be checked (in milliseconds)
//           default=10 minutes
pref("app.update.timer", 600000);
// Give the user x seconds to react before showing the big UI. default=12 hrs
pref("app.update.promptWaitTime", 43200);
// Show the Update Checking/Ready UI when the user was idle for x seconds
pref("app.update.idletime", 60);
 
// Whether or not we show a dialog box informing the user that the update was
// successfully applied. At the moment suite doesn't want this dialog by
// default
pref("app.update.showInstalledUI", false);

// 0 = suppress prompting for incompatibilities if there are updates available
//     to newer versions of installed addons that resolve them.
// 1 = suppress prompting for incompatibilities only if there are VersionInfo
//     updates available to installed addons that resolve them, not newer
//     versions.
pref("app.update.incompatible.mode", 0);

// Extension preferences

// Developers can set this to |true| if they are constantly changing files in their 
// extensions directory so that the extension system does not constantly think that
// their extensions are being updated and thus reregistered every time the app is
// started.
pref("extensions.ignoreMTimeChanges", false);
// Enables some extra Extension System Logging (can reduce performance)
pref("extensions.logging.enabled", false);
// Hides the install button in the add-ons mgr
pref("extensions.hideInstallButton", true);

// Blocklist preferences
pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/2/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/");
pref("extensions.blocklist.detailsURL", "http://%LOCALE%.www.mozilla.com/%LOCALE%/blocklist/");

// Symmetric (can be overridden by individual extensions) update preferences.
// e.g.
//  extensions.{GUID}.update.enabled
//  extensions.{GUID}.update.url
//  extensions.{GUID}.update.interval
//  .. etc ..
//
pref("extensions.update.enabled", true);
pref("extensions.update.url", "https://versioncheck.addons.mozilla.org/update/VersionCheck.php?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%");
pref("extensions.update.interval", 86400);  // Check for updates to Extensions and 
                                            // Themes every day

// Preferences for the Get Add-ons pane
pref("extensions.getAddons.showPane", true);
pref("extensions.getAddons.browseAddons", "https://%LOCALE%.add-ons.mozilla.org/%LOCALE%/%APP%");
pref("extensions.getAddons.maxResults", 5);
pref("extensions.getAddons.recommended.browseURL", "https://%LOCALE%.add-ons.mozilla.org/%LOCALE%/%APP%/recommended");
pref("extensions.getAddons.recommended.url", "https://services.addons.mozilla.org/%LOCALE%/%APP%/api/%API_VERSION%/list/featured/all/10/%OS%/%VERSION%");
pref("extensions.getAddons.search.browseURL", "https://%LOCALE%.add-ons.mozilla.org/%LOCALE%/%APP%/search?q=%TERMS%");
pref("extensions.getAddons.search.url", "https://services.addons.mozilla.org/%LOCALE%/%APP%/api/%API_VERSION%/search/%TERMS%/all/10/%OS%/%VERSION%");

// Non-symmetric (not shared by extensions) extension-specific [update] preferences
pref("extensions.getMoreExtensionsURL", "chrome://branding/locale/brand.properties");
pref("extensions.getMoreThemesURL", "chrome://branding/locale/brand.properties");
pref("extensions.getMorePluginsURL", "chrome://branding/locale/brand.properties");
pref("extensions.getMoreLocalesURL", "chrome://branding/locale/brand.properties");
pref("extensions.dss.enabled", false);          // Dynamic Skin Switching
pref("extensions.dss.switchPending", false);    // Non-dynamic switch pending after next
                                                // restart.

pref("xpinstall.dialog.confirm", "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul");

pref("xpinstall.dialog.progress.skin", "chrome://mozapps/content/extensions/extensions.xul?view=installs");
pref("xpinstall.dialog.progress.chrome", "chrome://mozapps/content/extensions/extensions.xul?view=installs");
pref("xpinstall.dialog.progress.type.skin", "Extension:Manager");
pref("xpinstall.dialog.progress.type.chrome", "Extension:Manager");

pref("xpinstall.whitelist.add.103", "addons.mozilla.org");

pref("xpinstall.whitelist.add", "update.mozilla.org");
pref("xpinstall.blacklist.add", "");

// Customizable toolbar stuff
pref("custtoolbar.personal_toolbar_folder", "");

pref("sidebar.customize.all_panels.url", "http://sidebar-rdf.netscape.com/%LOCALE%/sidebar-rdf/%SIDEBAR_VERSION%/all-panels.rdf");
pref("sidebar.customize.directory.url", "http://dmoz.org/Netscape/Sidebar/");
pref("sidebar.customize.more_panels.url", "http://dmoz.org/Netscape/Sidebar/");
pref("sidebar.num_tabs_in_view", 8);

pref("browser.throbber.url","chrome://navigator-region/locale/region.properties");

// pref to control the alert notification 
pref("alerts.slideIncrement", 1);
pref("alerts.slideIncrementTime", 10);
pref("alerts.totalOpenTime", 4000);

// 0 opens the download manager
// 1 opens a progress dialog
// 2 and other values, no download manager, no progress dialog. 
pref("browser.downloadmanager.behavior", 0);

pref("privacy.popups.sound_enabled",              false);
pref("privacy.popups.sound_url",                  "");
pref("privacy.popups.statusbar_icon_enabled",     true);
pref("privacy.popups.prefill_whitelist",          false);
pref("privacy.popups.remove_blacklist",           true);
pref("privacy.popups.showBrowserMessage",         true);

// sanitize (clear private data) options
// XXX: turn on sanitize for downloads once we support toolkit' download manager
pref("privacy.item.history",     true);
pref("privacy.item.formdata",    true);
pref("privacy.item.passwords",   false);
pref("privacy.item.downloads",   false);
pref("privacy.item.cookies",     false);
pref("privacy.item.cache",       true);
pref("privacy.item.sessions",    true);
pref("privacy.item.offlineApps", false);

pref("privacy.sanitize.sanitizeOnShutdown", false);
pref("privacy.sanitize.promptOnSanitize", true);

// Show XUL error pages instead of alerts for errors
pref("browser.xul.error_pages.enabled", true);
pref("browser.xul.error_pages.expert_bad_cert", false);

// Setting this pref to |true| forces BiDi UI menu items and keyboard shortcuts
// to be exposed. By default, only expose it for bidi-associated system locales.
pref("bidi.browser.ui", false);

// block popup windows
pref("dom.disable_open_during_load",   true);
// prevent JS from moving/resizing existing windows
pref("dom.disable_window_move_resize", true);
// prevent JS from raising or lowering windows
pref("dom.disable_window_flip",        true);
// prevent JS from disabling or replacing context menus
pref("dom.event.contextmenu.enabled",  true);

// Disables default plugin for Plugin Finder Service
pref("plugin.default_plugin_disabled", true);
// plugin finder service url
pref("pfs.datasource.url", "https://pfs.mozilla.org/plugins/PluginFinderService.php?mimetype=%PLUGIN_MIMETYPE%&appID=%APP_ID%&appVersion=%APP_VERSION%&clientOS=%CLIENT_OS%&chromeLocale=%CHROME_LOCALE%");
pref("plugins.hide_infobar_for_missing_plugin", false);

#ifdef XP_MACOSX
// determines the behavior upon starting a download.
//  0 - open the download manager
//  1 - open a progress dialog
//  2 - do nothing

pref("browser.downloadmanager.behavior", 1);

// Turn on click-and-hold contextual menus
pref("ui.click_hold_context_menus", true);
#endif

#ifndef XP_MACOSX
#ifdef XP_UNIX
// For the download dialog
pref("browser.download.progressDnldDialog.enable_launch_reveal_buttons", false);
pref("browser.urlbar.clickSelectsAll", false);

// 0 goes back
// 1 act like pgup
// 2 and other values, nothing
pref("browser.backspace_action", 2);

pref("general.autoScroll", false);

pref("layout.word_select.stop_at_punctuation", false);
#endif
#endif

// The breakpad report server to link to in about:crashes
pref("breakpad.reportURL", "http://crash-stats.mozilla.com/report/index/");
