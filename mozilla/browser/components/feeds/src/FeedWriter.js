# -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Feed Writer.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <beng@google.com>
#   Jeff Walden <jwalden+code@mit.edu>
#   Asaf Romano <mozilla.mano@sent.com>
#   Robert Sayre <sayrer@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK ***** */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

function LOG(str) {
  var prefB = 
    Cc["@mozilla.org/preferences-service;1"].
    getService(Ci.nsIPrefBranch);

  var shouldLog = false;
  try {
    shouldLog = prefB.getBoolPref("feeds.log");
  } 
  catch (ex) {
  }

  if (shouldLog)
    dump("*** Feeds: " + str + "\n");
}

/**
 * Wrapper function for nsIIOService::newURI.
 * @param aURLSpec
 *        The URL string from which to create an nsIURI.
 * @returns an nsIURI object, or null if the creation of the URI failed.
 */
function makeURI(aURLSpec, aCharset) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  try {
    return ios.newURI(aURLSpec, aCharset, null);
  } catch (ex) { }

  return null;
}

const XML_NS = "http://www.w3.org/XML/1998/namespace"
const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const AAA_NS = "http://www.w3.org/2005/07/aaa";
const TYPE_MAYBE_FEED = "application/vnd.mozilla.maybe.feed";
const URI_BUNDLE = "chrome://browser/locale/feeds/subscribe.properties";

const PREF_SELECTED_APP = "browser.feeds.handlers.application";
const PREF_SELECTED_WEB = "browser.feeds.handlers.webservice";
const PREF_SELECTED_ACTION = "browser.feeds.handler";
const PREF_SELECTED_READER = "browser.feeds.handler.default";
const PREF_SHOW_FIRST_RUN_UI = "browser.feeds.showFirstRunUI";

const FW_CLASSID = Components.ID("{49bb6593-3aff-4eb3-a068-2712c28bd58e}");
const FW_CLASSNAME = "Feed Writer";
const FW_CONTRACTID = "@mozilla.org/browser/feeds/result-writer;1";

const TITLE_ID = "feedTitleText";
const SUBTITLE_ID = "feedSubtitleText";

const ICON_DATAURL_PREFIX = "data:image/x-icon;base64,";

function FeedWriter() {
}
FeedWriter.prototype = {
  _getPropertyAsBag: function FW__getPropertyAsBag(container, property) {
    return container.fields.getProperty(property).
                     QueryInterface(Ci.nsIPropertyBag2);
  },
  
  _getPropertyAsString: function FW__getPropertyAsString(container, property) {
    try {
      return container.fields.getPropertyAsAString(property);
    }
    catch (e) {
    }
    return "";
  },
  
  _setContentText: function FW__setContentText(id, text) {
    this._contentSandbox.element = this._document.getElementById(id);
    this._contentSandbox.textNode = this._document.createTextNode(text);
    var codeStr =
      "while (element.hasChildNodes()) " +
      "  element.removeChild(element.firstChild);" +
      "element.appendChild(textNode);";
    Cu.evalInSandbox(codeStr, this._contentSandbox);
    this._contentSandbox.element = null;
    this._contentSandbox.textNode = null;
  },

  /**
   * Safely sets the href attribute on an anchor tag, providing the URI 
   * specified can be loaded according to rules. 
   * @param   element
   *          The element to set a URI attribute on
   * @param   attribute
   *          The attribute of the element to set the URI to, e.g. href or src
   * @param   uri
   *          The URI spec to set as the href
   */
  _safeSetURIAttribute: 
  function FW__safeSetURIAttribute(element, attribute, uri) {
    var secman = Cc["@mozilla.org/scriptsecuritymanager;1"].
                 getService(Ci.nsIScriptSecurityManager);    
    const flags = Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT_OR_DATA;
    try {
      secman.checkLoadURIStr(this._window.location.href, uri, flags);
      // checkLoadURIStr will throw if the link URI should not be loaded per 
      // the rules specified in |flags|, so we'll never "linkify" the link...
    }
    catch (e) {
      // Not allowed to load this link because secman.checkLoadURIStr threw
      return;
    }

    this._contentSandbox.element = element;
    this._contentSandbox.uri = uri;
    var codeStr = "element.setAttribute('" + attribute + "', uri);";
    Cu.evalInSandbox(codeStr, this._contentSandbox);
  },

  /**
   * Use this sandbox to run any dom manipulation code on nodes which
   * are already inserted into the content document.
   */
  __contentSandbox: null,
  get _contentSandbox() {
    if (!this.__contentSandbox)
      this.__contentSandbox = new Cu.Sandbox(this._window);

    return this.__contentSandbox;
  },

  /**
   * Calls doCommand for a the given XUL element within the context of the
   * content document.
   *
   * @param aElement
   *        the XUL element to call doCommand() on.
   */
  _safeDoCommand: function FW___safeDoCommand(aElement) {
    this._contentSandbox.element = aElement;
    Cu.evalInSandbox("element.doCommand();", this._contentSandbox);
    this._contentSandbox.element = null;
  },

  get _bundle() {
    var sbs = 
        Cc["@mozilla.org/intl/stringbundle;1"].
        getService(Ci.nsIStringBundleService);
    return sbs.createBundle(URI_BUNDLE);
  },
  
  _getFormattedString: function FW__getFormattedString(key, params) {
    return this._bundle.formatStringFromName(key, params, params.length);
  },
  
  _getString: function FW__getString(key) {
    return this._bundle.GetStringFromName(key);
  },

  /* Magic helper methods to be used instead of xbl properties */
  _getSelectedItemFromMenulist: function FW__getSelectedItemFromList(aList) {
    var node = aList.firstChild.firstChild;
    while (node) {
      if (node.localName == "menuitem" && node.getAttribute("selected") == "true")
        return node;

      node = node.nextSibling;
    }

    return null;
  },

  _setCheckboxCheckedState: function FW__setCheckboxCheckedState(aCheckbox, aValue) {
    // see checkbox.xml, xbl bindings are not applied within the sandbox!
    this._contentSandbox.checkbox = aCheckbox;
    var codeStr;
    var change = (aValue != (aCheckbox.getAttribute('checked') == 'true'));
    if (aValue)
      codeStr = "checkbox.setAttribute('checked', 'true'); ";
    else
      codeStr = "checkbox.removeAttribute('checked'); ";

    if (change) {
      this._contentSandbox.document = this._document;
      codeStr += "var event = document.createEvent('Events'); " +
                 "event.initEvent('CheckboxStateChange', true, true);" +
                 "checkbox.dispatchEvent(event);"
    }

    Cu.evalInSandbox(codeStr, this._contentSandbox);
  },

  /**
   * Writes the feed title into the preview document.
   * @param   container
   *          The feed container
   */
  _setTitleText: function FW__setTitleText(container) {
    if (container.title) {
      var title = container.title.plainText();
      this._setContentText(TITLE_ID, title);
      this._contentSandbox.document = this._document;
      this._contentSandbox.title = title;
      var codeStr = "document.title = title;"
      Cu.evalInSandbox(codeStr, this._contentSandbox);
    }

    var feed = container.QueryInterface(Ci.nsIFeed);
    if (feed && feed.subtitle)
      this._setContentText(SUBTITLE_ID, container.subtitle.plainText());
  },
  
  /**
   * Writes the title image into the preview document if one is present.
   * @param   container
   *          The feed container
   */
  _setTitleImage: function FW__setTitleImage(container) {
    try {
      var parts = container.image;
      
      // Set up the title image (supplied by the feed)
      var feedTitleImage = this._document.getElementById("feedTitleImage");
      this._safeSetURIAttribute(feedTitleImage, "src", 
                                parts.getPropertyAsAString("url"));
      
      // Set up the title image link
      var feedTitleLink = this._document.getElementById("feedTitleLink");

      var titleText = this._getFormattedString("linkTitleTextFormat", 
                                               [parts.getPropertyAsAString("title")]);
      this._contentSandbox.feedTitleLink = feedTitleLink;
      this._contentSandbox.titleText = titleText;
      this._contentSandbox.feedTitleText = this._document.getElementById("feedTitleText");
      this._contentSandbox.titleImageWidth = parseInt(parts.getPropertyAsAString("width")) + 15;

      // Fix the margin on the main title, so that the image doesn't run over
      // the underline
      var codeStr = "feedTitleLink.setAttribute('title', titleText); " +
                    "feedTitleText.style.marginRight = titleImageWidth + 'px';";
      Cu.evalInSandbox(codeStr, this._contentSandbox);
      this._contentSandbox.feedTitleLink = null;
      this._contentSandbox.titleText = null;
      this._contentSandbox.feedTitleText = null;
      this._contentSandbox.titleImageWidth = null;

      this._safeSetURIAttribute(feedTitleLink, "href", 
                                parts.getPropertyAsAString("link"));
    }
    catch (e) {
      LOG("Failed to set Title Image (this is benign): " + e);
    }
  },
  
  /**
   * Writes all entries contained in the feed.
   * @param   container
   *          The container of entries in the feed
   */
  _writeFeedContent: function FW__writeFeedContent(container) {
    // Build the actual feed content
    var feed = container.QueryInterface(Ci.nsIFeed);
    if (feed.items.length == 0)
      return;

    this._contentSandbox.feedContent =
      this._document.getElementById("feedContent");

    for (var i = 0; i < feed.items.length; ++i) {
      var entry = feed.items.queryElementAt(i, Ci.nsIFeedEntry);
      entry.QueryInterface(Ci.nsIFeedContainer);
      
      var entryContainer = this._document.createElementNS(HTML_NS, "div");
      entryContainer.className = "entry";

      // If the entry has a title, make it a link
      if (entry.title) {
        var a = this._document.createElementNS(HTML_NS, "a");
        a.appendChild(this._document.createTextNode(entry.title.plainText()));
      
        // Entries are not required to have links, so entry.link can be null.
        if (entry.link)
          this._safeSetURIAttribute(a, "href", entry.link.spec);

        var title = this._document.createElementNS(HTML_NS, "h3");
        title.appendChild(a);
        entryContainer.appendChild(title);
      }

      var body = this._document.createElementNS(HTML_NS, "div");
      var summary = entry.summary || entry.content;
      var docFragment = null;
      if (summary) {

        if (summary.base)
          body.setAttributeNS(XML_NS, "base", summary.base.spec);
        else
          LOG("no base?");
        docFragment = summary.createDocumentFragment(body);
        if (docFragment)
          body.appendChild(docFragment);

        // If the entry doesn't have a title, append a # permalink
        // See http://scripting.com/rss.xml for an example
        if (!entry.title && entry.link) {
          var a = this._document.createElementNS(HTML_NS, "a");
          a.appendChild(this._document.createTextNode("#"));
          this._safeSetURIAttribute(a, "href", entry.link.spec);
          body.appendChild(this._document.createTextNode(" "));
          body.appendChild(a);
        }

      }
      body.className = "feedEntryContent";
      entryContainer.appendChild(body);

      this._contentSandbox.entryContainer = entryContainer;
      this._contentSandbox.clearDiv =
        this._document.createElementNS(HTML_NS, "div");
      this._contentSandbox.clearDiv.style.clear = "both";
      
      var codeStr = "feedContent.appendChild(entryContainer); " +
                     "feedContent.appendChild(clearDiv);"
      Cu.evalInSandbox(codeStr, this._contentSandbox);
    }

    this._contentSandbox.feedContent = null;
    this._contentSandbox.entryContainer = null;
    this._contentSandbox.clearDiv = null;
  },
  
  /**
   * Gets a valid nsIFeedContainer object from the parsed nsIFeedResult.
   * Displays error information if there was one.
   * @param   result
   *          The parsed feed result
   * @returns A valid nsIFeedContainer object containing the contents of
   *          the feed.
   */
  _getContainer: function FW__getContainer(result) {
    var feedService = 
        Cc["@mozilla.org/browser/feeds/result-service;1"].
        getService(Ci.nsIFeedResultService);

    try {
      var result = 
        feedService.getFeedResult(this._getOriginalURI(this._window));
    }
    catch (e) {
      LOG("Subscribe Preview: feed not available?!");
    }
    
    if (result.bozo) {
      LOG("Subscribe Preview: feed result is bozo?!");
    }

    try {
      var container = result.doc;
      container.title;
    }
    catch (e) {
      LOG("Subscribe Preview: no result.doc? Why didn't the original reload?");
      return null;
    }
    return container;
  },
  
  /**
   * Get the human-readable display name of a file. This could be the 
   * application name.
   * @param   file
   *          A nsIFile to look up the name of
   * @returns The display name of the application represented by the file.
   */
  _getFileDisplayName: function FW__getFileDisplayName(file) {
#ifdef XP_WIN
    if (file instanceof Ci.nsILocalFileWin) {
      try {
        return file.getVersionInfoField("FileDescription");
      }
      catch (e) {
      }
    }
#endif
#ifdef XP_MACOSX
    var lfm = file.QueryInterface(Components.interfaces.nsILocalFileMac_MOZILLA_1_8_BRANCH);
    try {
      return lfm.bundleDisplayName;
    }
    catch (e) {
      // fall through to the file name
    }
#endif
    var ios = 
        Cc["@mozilla.org/network/io-service;1"].
        getService(Ci.nsIIOService);
    var url = ios.newFileURI(file).QueryInterface(Ci.nsIURL);
    return url.fileName;
  },

  /**
   * Get moz-icon url for a file
   * @param   file
   *          A nsIFile to look up the name of
   * @returns moz-icon url of the given file as a string
   */
  _getFileIconURL: function FW__getFileIconURL(file) {
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);
    var fph = ios.getProtocolHandler("file")
                 .QueryInterface(Ci.nsIFileProtocolHandler);
    var urlSpec = fph.getURLSpecFromFile(file);
    return "moz-icon://" + urlSpec + "?size=16";
  },

  /**
   * Helper method to set the selected application and system default
   * reader menuitems details from a file object
   *   @param aMenuItem
   *          The menuitem on which the attributes should be set
   *   @param aFile
   *          the menuitem associated file object
   */
  _initMenuItemWithFile: function(aMenuItem, aFile) {
    this._contentSandbox.menuitem = aMenuItem;
    this._contentSandbox.label = this._getFileDisplayName(aFile);
    this._contentSandbox.image = this._getFileIconURL(aFile);
    var codeStr = "menuitem.setAttribute('label', label); " +
                  "menuitem.setAttribute('title', label); " +
                  "menuitem.setAttribute('image', image);"
    Cu.evalInSandbox(codeStr, this._contentSandbox);
  },

  /**
   * Displays a prompt from which the user may choose a (client) feed reader.
   * @return - true if a feed reader was selected, false otherwise.
   */
  _chooseClientApp: function FW__chooseClientApp() {
    try {
      var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
      fp.init(this._window,
              this._getString("chooseApplicationDialogTitle"),
              Ci.nsIFilePicker.modeOpen);
      fp.appendFilters(Ci.nsIFilePicker.filterApps);

      if (fp.show() == Ci.nsIFilePicker.returnOK) {
        this._selectedApp = fp.file;
        if (this._selectedApp) {
          // XXXben - we need to compare this with the running instance executable
          //          just don't know how to do that via script...
          // XXXmano TBD: can probably add this to nsIShellService
#ifdef XP_WIN
          if (fp.file.leafName != "firefox.exe") {
#else
#ifdef XP_MACOSX
          if (fp.file.leafName != "Firefox.app") {
#else
          if (fp.file.leafName != "firefox-bin") {
#endif
#endif
            this._initMenuItemWithFile(this._contentSandbox.selectedAppMenuItem,
                                       this._selectedApp);

            // Show and select the selected application menuitem
            var codeStr = "selectedAppMenuItem.hidden = false;" +
                          "selectedAppMenuItem.doCommand();"
            Cu.evalInSandbox(codeStr, this._contentSandbox);
            return true;
          }
        }
      }
    }
    catch(ex) { }

    return false;
  },

  _setAlwaysUseCheckedState: function FW__setAlwaysUseCheckedState() {
    var checkbox = this._document.getElementById("alwaysUse");
    if (checkbox) {
      var alwaysUse = false;
      try {
        var prefs = Cc["@mozilla.org/preferences-service;1"].
                    getService(Ci.nsIPrefBranch);
        if (prefs.getCharPref(PREF_SELECTED_ACTION) != "ask")
          alwaysUse = true;
      }
      catch(ex) { }
      this._setCheckboxCheckedState(checkbox, alwaysUse);
    }
  },

  _setAlwaysUseLabel: function FW__setAlwaysUseLabel() {
    var checkbox = this._document.getElementById("alwaysUse");
    if (checkbox) {
      var handlersMenuList = this._document.getElementById("handlersMenuList");
      if (handlersMenuList) {
        var handlerName = this._getSelectedItemFromMenulist(handlersMenuList)
                              .getAttribute("label");
        this._contentSandbox.label = this._getFormattedString("alwaysUse", [handlerName]);
        this._contentSandbox.checkbox = checkbox;
        var codeStr = "checkbox.setAttribute('label', label); " +
                      "checkbox.setAttribute('title', label);";
        Cu.evalInSandbox(codeStr, this._contentSandbox);
      }
    }
  },

  /**
   * See nsIDOMEventListener
   */
  handleEvent: function(event) {
    // see comments in the write method
    event = new XPCNativeWrapper(event);
    if (event.target.ownerDocument != this._document) {
      LOG("FeedWriter.handleEvent: Someone passed the feed writer as a listener to the events of another document!");
      return;
    }

    switch (event.type) {
      case "command" : {
        switch (event.target.id) {
          case "subscribeButton":
            this.subscribe();
            break;
          case "chooseApplicationMenuItem":
            // For keyboard-only users, we only show the file picker once the 
            // subscribe button is pressed. See click event handling for the
            // mouse-case.
            break;
          default:
            this._contentSandbox.element = event.target.parentNode.parentNode;
            this._contentSandbox.label = event.target.getAttribute("label");
            var codeStr = "element.setAttributeNS('" + AAA_NS + "', 'valuenow', label);";
            Cu.evalInSandbox(codeStr, this._contentSandbox);
            this._contentSandbox.element = null;
            this._contentSandbox.label = null;

            this._setAlwaysUseLabel();
        }
        break;
      }
      case "click": {
        if (event.target.id == "chooseApplicationMenuItem") {
          if (!this._chooseClientApp()) {
            // Select the (per-prefs) selected handler if no application was selected
            this._setSelectedHandler();
          }
        }
      }
      case "CheckboxStateChange": {
        // Needed for a11y
        var checkbox = this._document.getElementById("alwaysUse");
        var codeStr;
        this._contentSandbox.element = checkbox;
        if (checkbox.getAttributeNS("", "checked") == "true")
          codeStr = "element.setAttributeNS('" + AAA_NS + "', 'checked', 'true');"
        else
          codeStr = "element.setAttributeNS('" + AAA_NS + "', 'checked', 'false');"

        Cu.evalInSandbox(codeStr, this._contentSandbox);
        this._contentSandbox.element = null;
      }
    }
  },

  _setSelectedHandler: function FW__setSelectedHandler() {
    var prefs =   
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch);

    var handler = "bookmarks";
    try {
      handler = prefs.getCharPref(PREF_SELECTED_READER);
    }
    catch (ex) { }
    
    switch (handler) {
      case "web": {
        var handlersMenuList = this._document.getElementById("handlersMenuList");
        if (handlersMenuList) {
          var url = prefs.getCharPref(PREF_SELECTED_WEB);
          var handlers =
            handlersMenuList.getElementsByAttribute("webhandlerurl", url);
          if (handlers.length == 0) {
            LOG("FeedWriter._setSelectedHandler: selected web handler isn't in the menulist")
            return;
          }

          this._safeDoCommand(handlers[0]);
        }
        break;
      }
      case "client": {
        try {
          this._selectedApp =
           prefs.getComplexValue(PREF_SELECTED_APP, Ci.nsILocalFile);
        }
        catch(ex) {
          this._selectedApp = null;
        }

        if (this._selectedApp) {
          this._initMenuItemWithFile(this._contentSandbox.selectedAppMenuItem,
                                     this._selectedApp);
          var codeStr = "selectedAppMenuItem.hidden = false; " +
                        "selectedAppMenuItem.doCommand(); ";

#ifdef XP_WIN
          // Only show the default reader menuitem if the default reader
          // isn't the selected application
          if (this._defaultSystemReader) {
            var shouldHide =
              this._defaultSystemReader.path == this._selectedApp.path;
            codeStr += "defaultHandlerMenuItem.hidden = " + shouldHide + ";"
          }
#endif
          Cu.evalInSandbox(codeStr, this._contentSandbox);
          break;
        }
      }
      case "bookmarks":
      default: {
        var liveBookmarksMenuItem =
          this._document.getElementById("liveBookmarksMenuItem");
        if (liveBookmarksMenuItem)
          this._safeDoCommand(liveBookmarksMenuItem);
      } 
    }
  },

  _initSubscriptionUI: function FW__initSubscriptionUI() {
    var handlersMenuPopup =
      this._document.getElementById("handlersMenuPopup");
    if (!handlersMenuPopup)
      return;

    var codeStr = "";

    // Last-selected application
    var menuItem = this._document.createElementNS(XUL_NS, "menuitem");
    menuItem.id = "selectedAppMenuItem";
    menuItem.className = "menuitem-iconic";
    menuItem.setAttribute("handlerType", "client");

    // a11y
    menuItem.setAttributeNS("http://www.w3.org/TR/xhtml2",
                            "role", "wairole:listitem");
    menuItem.setAttributeNS(AAA_NS, "selected", "false");
    try {
      var prefs = Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefBranch);
      this._selectedApp = prefs.getComplexValue(PREF_SELECTED_APP,
                                                Ci.nsILocalFile);

      if (this._selectedApp.exists())
        this._initMenuItemWithFile(menuItem, this._selectedApp);
      else {
        // Hide the menuitem if the last selected application doesn't exist
        menuItem.setAttribute("hidden", true);
      }
    }
    catch(ex) {
      // Hide the menuitem until an application is selected
      menuItem.setAttribute("hidden", true);
    }
    this._contentSandbox.handlersMenuPopup = handlersMenuPopup;
    this._contentSandbox.selectedAppMenuItem = menuItem;
    
    codeStr += "handlersMenuPopup.appendChild(selectedAppMenuItem); ";

#ifdef XP_WIN
    // On Windows, also list the default feed reader
    var defaultReader;
    try {
      const WRK = Ci.nsIWindowsRegKey;
      var regKey =
          Cc["@mozilla.org/windows-registry-key;1"].createInstance(WRK);
      regKey.open(WRK.ROOT_KEY_CLASSES_ROOT, 
                  "feed\\shell\\open\\command", WRK.ACCESS_READ);
      var path = regKey.readStringValue("");
      if (path.charAt(0) == "\"") {
        // Everything inside the quotes
        path = path.substr(1);
        path = path.substr(0, path.indexOf("\""));
      }
      else {
        // Everything up to the first space
        path = path.substr(0, path.indexOf(" "));
      }

      this._defaultSystemReader = Cc["@mozilla.org/file/local;1"].
                                  createInstance(Ci.nsILocalFile);
      this._defaultSystemReader.initWithPath(path);

      if (this._defaultSystemReader.exists()) {
        menuItem = this._document.createElementNS(XUL_NS, "menuitem");
        menuItem.id = "defaultHandlerMenuItem";
        menuItem.className = "menuitem-iconic";
        // a11y
        menuItem.setAttributeNS("http://www.w3.org/TR/xhtml2",
                                "role", "wairole:listitem");

        this._initMenuItemWithFile(defaultSystemReaderItem, this._defaultSystemReader);

        // Hide the default reader item if it points to the same application
        // as the last-selected application
        if (this._selectedApp &&
            this._selectedApp.path == this._defaultSystemReader.path)
          menuItem.setAttribute("hidden", "true");

        this._contentSandbox.defaultHandlerMenuItem = menuItem;
        codeStr += "handlersMenuPopup.appendChild(defaultHandlerMenuItem); ";
      }
      else {
        this._defaultSystemReader = null;
      }
    }
    catch (e) {
      LOG("No feed handler registered on system");
      this._defaultSystemReader = null;
    }
#endif

    // "Choose Application..." menuitem
    menuItem = this._document.createElementNS(XUL_NS, "menuitem");
    menuItem.id = "chooseApplicationMenuItem";

    var chooseAppItemLabel = this._getString("chooseApplicationMenuItem");
    menuItem.setAttribute("label", chooseAppItemLabel);
    // a11y
    menuItem.setAttributeNS("http://www.w3.org/TR/xhtml2",
                            "role", "wairole:listitem");
    menuItem.setAttribute("title", chooseAppItemLabel);
    menuItem.addEventListener("click", this, false);

    this._contentSandbox.chooseAppMenuItem = menuItem;
    codeStr += "handlersMenuPopup.appendChild(chooseAppMenuItem); ";

    // separator
    this._contentSandbox.chooseAppSep =
      this._document.createElementNS(XUL_NS, "menuseparator")
    codeStr += "handlersMenuPopup.appendChild(chooseAppSep); ";

    Cu.evalInSandbox(codeStr, this._contentSandbox);

    // List of web handlers
    var wccr = 
      Cc["@mozilla.org/embeddor.implemented/web-content-handler-registrar;1"].
      getService(Ci.nsIWebContentConverterService);
    var handlers = wccr.getContentHandlers(TYPE_MAYBE_FEED, {});
    if (handlers.length != 0) {
      for (var i = 0; i < handlers.length; ++i) {
        menuItem = this._document.createElementNS(XUL_NS, "menuitem");
        menuItem.className = "menuitem-iconic";
        menuItem.setAttribute("label", handlers[i].name);
        menuItem.setAttribute("handlerType", "web");
        menuItem.setAttribute("webhandlerurl", handlers[i].uri);

        // a11y
        menuItem.setAttribute("title", handlers[i].name);
        menuItem.setAttributeNS("http://www.w3.org/TR/xhtml2",
                                "role", "wairole:listitem");
        this._contentSandbox.menuItem = menuItem;
        codeStr = "handlersMenuPopup.appendChild(menuItem);";
        Cu.evalInSandbox(codeStr, this._contentSandbox);

        // For privacy reasons we cannot set the image attribute directly
        // to the icon url, see Bug 358878
        var uri = makeURI(handlers[i].uri);
        if (uri && /^https?/.test(uri.scheme))
          new iconDataURIGenerator(uri.prePath + "/favicon.ico", menuItem,
                                   this._contentSandbox)
      }
      this._contentSandbox.menuItem = null;
    }

    this._setSelectedHandler();

    // "Always use..." checkbox initial state
    this._setAlwaysUseLabel();
    this._setAlwaysUseCheckedState();

    // Syncs xul:checked with aaa:checked, needed for a11y
    var checkbox = this._document.getElementById("alwaysUse");
    checkbox.addEventListener("CheckboxStateChange", this, false);

    // We update the "Always use.." checkbox label whenever the selected item
    // in the list is changed
    handlersMenuPopup.addEventListener("command", this, false);

    // Set up the "Subscribe Now" button
    this._document
        .getElementById("subscribeButton")
        .addEventListener("command", this, false);
    
    // first-run ui
    var showFirstRunUI = true;
    try {
      showFirstRunUI = prefs.getBoolPref(PREF_SHOW_FIRST_RUN_UI);
    }
    catch (ex) { }
    if (showFirstRunUI) {
      this._contentSandbox.header = this._document.getElementById("feedHeader");
      codeStr = "header.setAttribute('firstrun', 'true');"
      Cu.evalInSandbox(codeStr, this._contentSandbox);
      prefs.setBoolPref(PREF_SHOW_FIRST_RUN_UI, false);
    }
  },

  /**
   * Returns the original URI object of the feed and ensures that this
   * component is only ever invoked from the preview document.  
   * @param window 
   *        The window of the document invoking the BrowserFeedWriter
   */
  _getOriginalURI: function FW__getOriginalURI(window) {  
    var chan = 
        window.QueryInterface(Ci.nsIInterfaceRequestor).
        getInterface(Ci.nsIWebNavigation).
        QueryInterface(Ci.nsIDocShell_MOZILLA_1_8_BRANCH).
        currentDocumentChannel;
    const kPrefix = "jar:file:";
    if (chan.URI.spec.substring(0, kPrefix.length) == kPrefix)
      return chan.originalURI;
    else
      return null;
  },

  _window: null,
  _document: null,
  _feedURI: null,

  /**
   * See nsIFeedWriter
   */
  write: function FW_write(window) {
    // Explicitly wrap |window| in an XPCNativeWrapper to make sure
    // it's a real native object! This will throw an exception if we
    // get a non-native object.
    window = new XPCNativeWrapper(window);

    this._feedURI = this._getOriginalURI(window);
     if (!this._feedURI)
      return;
    try {
      this._window = window;
      this._document = window.document;
        
      LOG("Subscribe Preview: feed uri = " + this._window.location.href);
       
      // Set up the displayed handler
      this._initSubscriptionUI();
      var prefs =   
      Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefBranch2);
      prefs.addObserver(PREF_SELECTED_ACTION, this, false);
      prefs.addObserver(PREF_SELECTED_READER, this, false);
      prefs.addObserver(PREF_SELECTED_APP, this, false);
      
       // Set up the feed content
      var container = this._getContainer();
      if (!container)
        return;
      
      this._setTitleText(container);
      
      this._setTitleImage(container);
      
      this._writeFeedContent(container);
    }
    finally {
      this._removeFeedFromCache();
    }
  },
  
  /**
   * See nsIFeedWriter
   */
  close: function FW_close() {
    this._document = null;
    this._window = null;
    var prefs =   
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch2);
    prefs.removeObserver(PREF_SELECTED_ACTION, this);
    prefs.removeObserver(PREF_SELECTED_READER, this);
    prefs.removeObserver(PREF_SELECTED_WEB, this);
    prefs.removeObserver(PREF_SELECTED_APP, this);
    this._removeFeedFromCache();
  },
      
  _removeFeedFromCache: function FW__removeFeedFromCache() {
    if (this._feedURI) {
      var feedService = 
          Cc["@mozilla.org/browser/feeds/result-service;1"].
          getService(Ci.nsIFeedResultService);
      feedService.removeFeedResult(this._feedURI);
      this._feedURI = null;
    }
  },  
  
  subscribe: function FW_subscribe() {
    // Subscribe to the feed using the selected handler and save prefs
    var prefs =   
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch);
    var defaultHandler = "reader";
    var useAsDefault = this._document.getElementById("alwaysUse")
                                     .getAttribute("checked");

    var selectedItem =
      this._getSelectedItemFromMenulist(this._document.getElementById("handlersMenuList"));

    // Show the file picker before subscribing if the
    // choose application menuitem was choosen using the keyboard
    if (selectedItem.id == "chooseApplicationMenuItem") {
      if (!this._chooseClientApp())
        return;
      
      selectedItem = this._getSelectedItemFromMenulist(this._document.getElementById("handlersMenuList"));
    }

    if (selectedItem.hasAttribute("webhandlerurl")) {
      var webURI = selectedItem.getAttribute("webhandlerurl");
      prefs.setCharPref(PREF_SELECTED_READER, "web");
      prefs.setCharPref(PREF_SELECTED_WEB, webURI);

      var wccr = 
        Cc["@mozilla.org/embeddor.implemented/web-content-handler-registrar;1"].
        getService(Ci.nsIWebContentConverterService);
      var handler = wccr.getWebContentHandlerByURI(TYPE_MAYBE_FEED, webURI);
      if (handler) {
        if (useAsDefault)
          wccr.setAutoHandler(TYPE_MAYBE_FEED, handler);

        this._window.location.href =
          handler.getHandlerURI(this._window.location.href);
      }
    }
    else {
      switch (selectedItem.id) {
        case "selectedAppMenuItem":
          prefs.setComplexValue(PREF_SELECTED_APP, Ci.nsILocalFile, 
                                this._selectedApp);
          prefs.setCharPref(PREF_SELECTED_READER, "client");
          break;
#ifdef XP_WIN
        case "defaultHandlerMenuItem":
          prefs.setComplexValue(PREF_SELECTED_APP, Ci.nsILocalFile, 
                                this._defaultSystemReader);
          prefs.setCharPref(PREF_SELECTED_READER, "client");
          break;
#endif
        case "liveBookmarksMenuItem":
          defaultHandler = "bookmarks";
          prefs.setCharPref(PREF_SELECTED_READER, "bookmarks");
          break;
      }
      var feedService = Cc["@mozilla.org/browser/feeds/result-service;1"].
                        getService(Ci.nsIFeedResultService);

      // Pull the title and subtitle out of the document
      var feedTitle = this._document.getElementById(TITLE_ID).textContent;
      var feedSubtitle =
        this._document.getElementById(SUBTITLE_ID).textContent;
      feedService.addToClientReader(this._window.location.href,
                                    feedTitle, feedSubtitle);
    }

    // If "Always use..." is checked, we should set PREF_SELECTED_ACTION
    // to either "reader" (If a web reader or if an application is selected),
    // or to "bookmarks" (if the live bookmarks option is selected).
    // Otherwise, we should set it to "ask"
    if (useAsDefault)
      prefs.setCharPref(PREF_SELECTED_ACTION, defaultHandler);
    else
      prefs.setCharPref(PREF_SELECTED_ACTION, "ask");
  },
  
  /**
   * See nsIObserver
   */
  observe: function FW_observe(subject, topic, data) {
    if (!this._window) {
      // this._window is null unless this.write was called with a trusted
      // window object.
      return;
    }

    if (topic == "nsPref:changed") {
      switch (data) {
        case PREF_SELECTED_READER:
        case PREF_SELECTED_WEB:
        case PREF_SELECTED_APP:
          this._setSelectedHandler();
          break;
        case PREF_SELECTED_ACTION:
          this._setAlwaysUseCheckedState();
      }
    } 
  },  
  
  /**
   * See nsIClassInfo
   */
  getInterfaces: function WCCR_getInterfaces(countRef) {
    var interfaces = 
        [Ci.nsIFeedWriter, Ci.nsIClassInfo, Ci.nsISupports];
    countRef.value = interfaces.length;
    return interfaces;
  },
  getHelperForLanguage: function WCCR_getHelperForLanguage(language) {
    return null;
  },
  contractID: FW_CONTRACTID,
  classDescription: FW_CLASSNAME,
  classID: FW_CLASSID,
  implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
  flags: Ci.nsIClassInfo.DOM_OBJECT,

  QueryInterface: function FW_QueryInterface(iid) {
    if (iid.equals(Ci.nsIFeedWriter) ||
        iid.equals(Ci.nsIClassInfo) ||
        iid.equals(Ci.nsIDOMEventListener) ||
        iid.equals(Ci.nsIObserver) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

function iconDataURIGenerator(aURISpec, aElement, aSandbox) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  var chan = ios.newChannelFromURI(makeURI(aURISpec));
  chan.notificationCallbacks = this;
  chan.asyncOpen(this, null);

  this._channel = chan;
  this._bytes = [];
  this._element = aElement;
  this._contentSandbox = aSandbox;
}
iconDataURIGenerator.prototype = {
  _channel: null,
  _countRead: 0,
  _stream: null,

  QueryInterface: function FW_IDUG_loadQI(aIID) {
    if (aIID.equals(Ci.nsISupports)           ||
        aIID.equals(Ci.nsIRequestObserver)    ||
        aIID.equals(Ci.nsIStreamListener)     ||
        aIID.equals(Ci.nsIChannelEventSink)   ||
        aIID.equals(Ci.nsIInterfaceRequestor) ||
        aIID.equals(Ci.nsIBadCertListener)    ||
        // See bug 358878 comment 11
        aIID.equals(Ci.nsIPrompt)             ||
        // See FIXME comment below
        aIID.equals(Ci.nsIHttpEventSink)      ||
        aIID.equals(Ci.nsIProgressEventSink)  ||
        false)
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  // nsIRequestObserver
  onStartRequest: function FW_IDUG_loadStartR(aRequest, aContext) {
    this._stream = Cc["@mozilla.org/binaryinputstream;1"].
                   createInstance(Ci.nsIBinaryInputStream);
  },

  onStopRequest: function FW_IDUG_loadStopR(aRequest, aContext, aStatusCode) {
    var requestFailed = !Components.isSuccessCode(aStatusCode);
    if (!requestFailed && (aRequest instanceof Ci.nsIHttpChannel))
      requestFailed = !aRequest.requestSucceeded;

    if (!requestFailed && this._countRead != 0) {
      var str = String.fromCharCode.apply(null, this._bytes);
      try {
        var dataURI = ICON_DATAURL_PREFIX +
                      this._element.ownerDocument.defaultView.btoa(str);

        this._contentSandbox.element = this._element;
        this._contentSandbox.dataURI = ICON_DATAURL_PREFIX +
          this._element.ownerDocument.defaultView.btoa(str);
        var codeStr = "element.setAttribute('src', dataURI); ";
        if (this._element.getAttribute("selected") == "true")
          codeStr += "element.parentNode.parentNode.setAttribute('src', dataURI)";

        Cu.evalInSandbox(codeStr, this._contentSandbox);
        this._contentSandbox.element = null;
        this._contentSandbox.dataURI = null;
      }
      catch(ex) {}
    }
    this._channel = null;
    this._element  = null;
  },

  // nsIStreamListener
  onDataAvailable: function FW_IDUG_loadDAvailable(aRequest, aContext,
                                                   aInputStream, aOffset,
                                                   aCount) {
    this._stream.setInputStream(aInputStream);

    // Get a byte array of the data
    this._bytes = this._bytes.concat(this._stream.readByteArray(aCount));
    this._countRead += aCount;
  },

  // nsIChannelEventSink
  onChannelRedirect: function FW_IDUG_loadCRedirect(aOldChannel, aNewChannel,
                                                    aFlags) {
    this._channel = aNewChannel;
  },

  // nsIInterfaceRequestor
  getInterface: function FW_IDUG_load_GI(aIID) {
    return this.QueryInterface(aIID);
  },

  // nsIBadCertListener
  confirmUnknownIssuer: function FW_IDUG_load_CUI(aSocketInfo, aCert,
                                                  aCertAddType) {
    return false;
  },

  confirmMismatchDomain: function FW_IDUG_load_CMD(aSocketInfo, aTargetURL,
                                                   aCert) {
    return false;
  },

  confirmCertExpired: function FW_IDUG_load_CCE(aSocketInfo, aCert) {
    return false;
  },

  notifyCrlNextupdate: function FW_IDUG_load_NCN(aSocketInfo, aTargetURL, aCert) {
  },

  // FIXME: bug 253127
  // nsIHttpEventSink
  onRedirect: function (aChannel, aNewChannel) { },
  // nsIProgressEventSink
  onProgress: function (aRequest, aContext, aProgress, aProgressMax) { },
  onStatus: function (aRequest, aContext, aStatus, aStatusArg) { }
};

var Module = {
  QueryInterface: function M_QueryInterface(iid) {
    if (iid.equals(Ci.nsIModule) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  
  getClassObject: function M_getClassObject(cm, cid, iid) {
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    
    if (cid.equals(FW_CLASSID))
      return new GenericComponentFactory(FeedWriter);
      
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  
  registerSelf: function M_registerSelf(cm, file, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
    
    cr.registerFactoryLocation(FW_CLASSID, FW_CLASSNAME, FW_CONTRACTID,
                               file, location, type);
    
    var catman = 
        Cc["@mozilla.org/categorymanager;1"].
        getService(Ci.nsICategoryManager);
    catman.addCategoryEntry("JavaScript global constructor",
                            "BrowserFeedWriter", FW_CONTRACTID, true, true);
  },
  
  unregisterSelf: function M_unregisterSelf(cm, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
    cr.unregisterFactoryLocation(FW_CLASSID, location);
  },
  
  canUnload: function M_canUnload(cm) {
    return true;
  }
};

function NSGetModule(cm, file) {
  return Module;
}

#include ../../../../toolkit/content/debug.js
#include GenericFactory.js
