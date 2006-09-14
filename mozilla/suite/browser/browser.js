/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *   Blake Ross <blakeross@telocity.com>
 *   Peter Annema <disttsc@bart.nl>
 *   Samir Gehani <sgehani@netscape.com>
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

const nsIWebNavigation = Components.interfaces.nsIWebNavigation;
var gPrintSettingsAreGlobal = true;
var gSavePrintSettings = true;
var gPrintSettings = null;
var gChromeState = null; // chrome state before we went into print preview
var gOldCloseHandler = null; // close handler before we went into print preview
var gInPrintPreviewMode = false;

function getWebNavigation()
{
  try {
    return getBrowser().webNavigation;
  } catch (e) {
    return null;
  }
}

function BrowserReloadWithFlags(reloadFlags)
{
  /* First, we'll try to use the session history object to reload so 
   * that framesets are handled properly. If we're in a special 
   * window (such as view-source) that has no session history, fall 
   * back on using the web navigation's reload method.
   */

  var webNav = getWebNavigation();
  try {
    var sh = webNav.sessionHistory;
    if (sh)
      webNav = sh.QueryInterface(Components.interfaces.nsIWebNavigation);
  } catch (e) {
  }

  try {
    webNav.reload(reloadFlags);
  } catch (e) {
  }
}

function toggleAffectedChrome(aHide)
{
  // chrome to toggle includes:
  //   (*) menubar
  //   (*) navigation bar
  //   (*) personal toolbar
  //   (*) tab browser ``strip''
  //   (*) sidebar

  if (!gChromeState)
    gChromeState = new Object;
  var chrome = new Array;
  var i = 0;
  chrome[i++] = document.getElementById("main-menubar");
  chrome[i++] = document.getElementById("nav-bar");
  chrome[i++] = document.getElementById("PersonalToolbar");

  // sidebar states map as follows:
  //   was-hidden    => hide/show nothing
  //   was-collapsed => hide/show only the splitter
  //   was-shown     => hide/show the splitter and the box
  if (aHide)
  {
    // going into print preview mode
    if (sidebar_is_collapsed())
    {
      gChromeState.sidebar = "was-collapsed";
      chrome[i++] = document.getElementById("sidebar-splitter");
    }
    else if (sidebar_is_hidden())
      gChromeState.sidebar = "was-hidden";
    else 
    {
      gChromeState.sidebar = "was-visible";
      chrome[i++] = document.getElementById("sidebar-box");
      chrome[i++] = document.getElementById("sidebar-splitter");
    }
  }
  else
  {
    // restoring normal mode (i.e., leaving print preview mode)
    if (gChromeState.sidebar == "was-collapsed" ||
        gChromeState.sidebar == "was-visible")
      chrome[i++] = document.getElementById("sidebar-splitter");
    if (gChromeState.sidebar == "was-visible")
      chrome[i++] = document.getElementById("sidebar-box");
  }

  // now that we've figured out which elements we're interested, toggle 'em
  for (i = 0; i < chrome.length; ++i)
  {
    if (aHide)
      chrome[i].hidden = true;
    else
      chrome[i].hidden = false;
  }

  // if we are unhiding and sidebar used to be there rebuild it
  if (!aHide && gChromeState.sidebar == "was-visible")
    SidebarRebuild();
    
  // now deal with the tab browser ``strip'' 
  var theTabbrowser = document.getElementById("content"); 
  if (aHide) // normal mode -> print preview
  {
    gChromeState.hadTabStrip = theTabbrowser.getStripVisibility();
    theTabbrowser.setStripVisibilityTo(false);
  }
  else // print preview -> normal mode
  {
    // tabs were showing before entering print preview
    if (gChromeState.hadTabStrip) 
    {
      theTabbrowser.setStripVisibilityTo(true);
      gChromeState.hadTabStrip = false; // reset
    }
  }
}

function showPrintPreviewToolbar()
{
  toggleAffectedChrome(true);
  const kXULNS = 
    "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

  var printPreviewTB = document.createElementNS(kXULNS, "toolbar");
  printPreviewTB.setAttribute("printpreview", true);
  printPreviewTB.setAttribute("id", "print-preview-toolbar");

  var navTB = document.getElementById("nav-bar");
  navTB.parentNode.appendChild(printPreviewTB);
}

function BrowserExitPrintPreview()
{
  gInPrintPreviewMode = false;

  var browser = getBrowser();
  browser.setAttribute("handleCtrlPageUpDown", "true");

  // exit print preview galley mode in content area
  var ifreq = _content.QueryInterface(
    Components.interfaces.nsIInterfaceRequestor);
  var webBrowserPrint = ifreq.getInterface(
    Components.interfaces.nsIWebBrowserPrint);     
  webBrowserPrint.exitPrintPreview(); 
  _content.focus();

  // remove the print preview toolbar
  var navTB = document.getElementById("nav-bar");
  var printPreviewTB = document.getElementById("print-preview-toolbar");
  navTB.parentNode.removeChild(printPreviewTB);

  // restore chrome to original state
  toggleAffectedChrome(false);

  // restore old onclose handler if we found one before previewing
  var mainWin = document.getElementById("main-window");
  mainWin.setAttribute("onclose", gOldCloseHandler);
}

function GetPrintSettings(webBrowserPrint)
{
  var prevPS = gPrintSettings;

  try {
    if (gPrintSettings == null) {
      var pref = Components.classes["@mozilla.org/preferences-service;1"]
                           .getService(Components.interfaces.nsIPrefBranch);
      if (pref) {
        gPrintSettingsAreGlobal = pref.getBoolPref("print.use_global_printsettings", false);
        gSavePrintSettings = pref.getBoolPref("print.save_print_settings", false);
      }

      if (gPrintSettingsAreGlobal) {
        gPrintSettings = webBrowserPrint.globalPrintSettings;        
        if (gSavePrintSettings) {
          webBrowserPrint.initPrintSettingsFromPrefs(gPrintSettings, false, gPrintSettings.kInitSaveNativeData);
        }
      } else {
        gPrintSettings = webBrowserPrint.newPrintSettings;
      }
    }
  } catch (e) {
    dump("GetPrintSettings "+e);
  }

  return gPrintSettings;
}

function BrowserPrintPreview()
{
  gInPrintPreviewMode = true;

  var browser = getBrowser();
  browser.setAttribute("handleCtrlPageUpDown", "false");

  var mainWin = document.getElementById("main-window");

  // save previous close handler to restoreon exiting print preview mode
  if (mainWin.hasAttribute("onclose"))
    gOldCloseHandler = mainWin.getAttribute("onclose");
  else
    gOldCloseHandler = null;
  mainWin.setAttribute("onclose", "BrowserExitPrintPreview(); return false;");
 
  try {
    var ifreq = _content.QueryInterface(
      Components.interfaces.nsIInterfaceRequestor);
    var webBrowserPrint = ifreq.getInterface(
      Components.interfaces.nsIWebBrowserPrint);     
    if (webBrowserPrint) {
      gPrintSettings = GetPrintSettings(webBrowserPrint);
      webBrowserPrint.printPreview(gPrintSettings, null);
    }

    // show the toolbar after we go into print preview mode so
    // that we can initialize the toolbar with total num pages
    showPrintPreviewToolbar();

    _content.focus();
  } catch (e) {
    // Pressing cancel is expressed as an NS_ERROR_ABORT return value,
    // causing an exception to be thrown which we catch here.
    // Unfortunately this will also consume helpful failures, so add a
    // dump(e); // if you need to debug
  }
}


function BrowserPrintSetup()
{
  var didOK = false;
  try {
    var ifreq = _content.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
    var webBrowserPrint = ifreq.getInterface(Components.interfaces.nsIWebBrowserPrint);     
    if (webBrowserPrint) {
      gPrintSettings = GetPrintSettings(webBrowserPrint);
    }

    didOK = goPageSetup(window, gPrintSettings);  // from utilityOverlay.js
    if (didOK) {  // from utilityOverlay.js

      if (webBrowserPrint) {
        if (gPrintSettingsAreGlobal && gSavePrintSettings) {
          webBrowserPrint.savePrintSettingsToPrefs(gPrintSettings, false, gPrintSettings.kInitSaveNativeData);
        }
        if (webBrowserPrint.doingPrintPreview) {
          webBrowserPrint.printPreview(gPrintSettings, null);
        }
      }
    }
  } catch (e) {
    dump("BrowserPrintSetup "+e);
  }
  return didOK;
}

function BrowserPrint()
{
  try {
    var ifreq = _content.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
    var webBrowserPrint = ifreq.getInterface(Components.interfaces.nsIWebBrowserPrint);     
    if (webBrowserPrint) {
      gPrintSettings = GetPrintSettings(webBrowserPrint);
      webBrowserPrint.print(gPrintSettings, null);
    }
  } catch (e) {
    // Pressing cancel is expressed as an NS_ERROR_ABORT return value,
    // causing an exception to be thrown which we catch here.
    // Unfortunately this will also consume helpful failures, so add a
    // dump(e); // if you need to debug
  }
}

function BrowserSetDefaultCharacterSet(aCharset)
{
  // no longer needed; set when setting Force; see bug 79608
}

function BrowserSetForcedCharacterSet(aCharset)
{
  var docCharset = getBrowser().docShell.QueryInterface(
                            Components.interfaces.nsIDocCharset);
  docCharset.charset = aCharset;
  BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

function BrowserSetForcedDetector()
{
  getBrowser().documentCharsetInfo.forcedDetector = true;
}

function BrowserFind()
{
  var focusedWindow = document.commandDispatcher.focusedWindow;
  if (!focusedWindow || focusedWindow == window)
    focusedWindow = window._content;

  findInPage(getBrowser(), window._content, focusedWindow)
}

function BrowserFindAgain()
{
    var focusedWindow = document.commandDispatcher.focusedWindow;
    if (!focusedWindow || focusedWindow == window)
      focusedWindow = window._content;

  findAgainInPage(getBrowser(), window._content, focusedWindow)
}

function BrowserCanFindAgain()
{
  return canFindAgainInPage();
}

function getMarkupDocumentViewer()
{
  return getBrowser().markupDocumentViewer;
}

/**
 * Content area tooltip.
 * XXX - this must move into XBL binding/equiv! Do not want to pollute
 *       navigator.js with functionality that can be encapsulated into
 *       browser widget. TEMPORARY!
 *
 * NOTE: Any changes to this routine need to be mirrored in ChromeListener::FindTitleText()
 *       (located in mozilla/embedding/browser/webBrowser/nsDocShellTreeOwner.cpp)
 *       which performs the same function, but for embedded clients that
 *       don't use a XUL/JS layer. It is important that the logic of
 *       these two routines be kept more or less in sync.
 *       (pinkerton)
 **/
function FillInHTMLTooltip(tipElement)
{
  var retVal = false;
  if (tipElement.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul")
    return retVal;

  const XLinkNS = "http://www.w3.org/1999/xlink";


  var titleText = null;
  var XLinkTitleText = null;
  
  while (!titleText && !XLinkTitleText && tipElement) {
    if (tipElement.nodeType == Node.ELEMENT_NODE) {
      titleText = tipElement.getAttribute("title");
      XLinkTitleText = tipElement.getAttributeNS(XLinkNS, "title");
    }
    tipElement = tipElement.parentNode;
  }

  var texts = [titleText, XLinkTitleText];
  var tipNode = document.getElementById("aHTMLTooltip");

  for (var i = 0; i < texts.length; ++i) {
    var t = texts[i];
    if (t && t.search(/\S/) >= 0) {
      tipNode.setAttribute("label", t);
      retVal = true;
    }
  }

  return retVal;
}
