/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 */

const nsIFilePicker     = Components.interfaces.nsIFilePicker;
const nsIWindowMediator = Components.interfaces.nsIWindowMediator;
const nsIPrefService    = Components.interfaces.nsIPrefService;
const nsIPrefLocalizedString = Components.interfaces.nsIPrefLocalizedString;

const FILEPICKER_CONTRACTID     = "@mozilla.org/filepicker;1";
const WINDOWMEDIATOR_CONTRACTID = "@mozilla.org/appshell/window-mediator;1";
const PREFSERVICE_CONTRACTID    = "@mozilla.org/preferences-service;1";

function Startup()
{
  var clearHistButton = document.getElementById("browserClearHistory");
  try {
    var urlBarHist = Components.classes["@mozilla.org/browser/urlbarhistory;1"]
                               .getService(Components.interfaces.nsIUrlbarHistory);
    var isBtnLocked = parent.hPrefWindow.getPrefIsLocked(clearHistButton.getAttribute("prefstring"));
    var globalHistory = Components.classes["@mozilla.org/browser/global-history;1"]
                                  .getService(Components.interfaces.nsIBrowserHistory);
    clearHistButton.disabled = ( urlBarHist.count == 0 && globalHistory.count == 0) || isBtnLocked;
  }
  catch(ex) {
  }
}

function selectFile()
{
  var fp = Components.classes[FILEPICKER_CONTRACTID]
                     .createInstance(nsIFilePicker);

  var prefutilitiesBundle = document.getElementById("bundle_prefutilities");
  var title = prefutilitiesBundle.getString("choosehomepage");
  fp.init(window, title, nsIFilePicker.modeOpen);
  fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText |
                   nsIFilePicker.filterXML | nsIFilePicker.filterHTML |
                   nsIFilePicker.filterImages);

  var ret = fp.show();
  if (ret == nsIFilePicker.returnOK) {
    var folderField = document.getElementById("browserStartupHomepage");
    folderField.value = fp.fileURL.spec;
  }
}

function setHomePageToCurrentPage()
{
  var windowManager = Components.classes[WINDOWMEDIATOR_CONTRACTID]
                                .getService(nsIWindowMediator);

  var browserWindow = windowManager.getMostRecentWindow("navigator:browser");
  if (browserWindow) {
    var browser = browserWindow.document.getElementById("content");
    var url = browser.webNavigation.currentURI.spec;
    if (url) {
      var homePageField = document.getElementById("browserStartupHomepage");
      homePageField.value = url;
    }
  }
}

function setHomePageToDefaultPage()
{
  var prefService = Components.classes[PREFSERVICE_CONTRACTID]
                              .getService(nsIPrefService);
  var pref = prefService.getDefaultBranch(null);
  var url = pref.getComplexValue("browser.startup.homepage",
                                  nsIPrefLocalizedString).data;
  var homePageField = document.getElementById("browserStartupHomepage");
  homePageField.value = url;
}

function prefClearGlobalHistory()
{
  var globalHistory = Components.classes["@mozilla.org/browser/global-history;1"]
                                .getService(Components.interfaces.nsIBrowserHistory);
  globalHistory.removeAllPages();
  var urlBarHistory = Components.classes["@mozilla.org/browser/urlbarhistory;1"]
                                .getService(Components.interfaces.nsIUrlbarHistory);
  urlBarHistory.clearHistory();
}
