//-------- global variables

var gBrowser;

function init()
{
  // Initialize the Help window
  //  "window.arguments[0]" is undefined or context string

  // move to right end of screen
  var width = document.documentElement.getAttribute("width");
  var height = document.documentElement.getAttribute("height");
  window.moveTo(screen.availWidth-width, (screen.availHeight-height)/2);

  gBrowser = document.getElementById("help-content");
  var sessionHistory =  Components.classes["@mozilla.org/browser/shistory;1"]
                                  .createInstance(Components.interfaces.nsISHistory);

  getWebNavigation().sessionHistory = sessionHistory;

  if ("argument" in window && window.arguments.length >= 1) {
    browser.loadURI(window.arguments[0]);
  } else {
    goHome(); // should be able to do browser.goHome();
  }
}

function getWebNavigation()
{
  return gBrowser.webNavigation;
}

function loadURI(aURI)
{
  const nsIWebNavigation = Components.interfaces.nsIWebNavigation;
  getWebNavigation().loadURI(aURI, nsIWebNavigation.LOAD_FLAGS_NONE);
}

function goBack()
{
  var webNavigation = getWebNavigation();
  if (webNavigation.canGoBack)
    webNavigation.goBack();
}

function goForward()
{
  var webNavigation = getWebNavigation();
  if (webNavigation.canGoForward)
    webNavigation.goForward();
}

function goHome() {
  // load "Welcome" page
  loadURI("chrome://help/locale/maincont.html");
}

function print()
{
  try {
    _content.print();
  } catch (e) {
  }
}

