
function toNavigator()
{
	CycleWindow('navigator:browser', 'chrome://navigator/content/');
}

function toMessengerWindow()
{
	toOpenWindowByType("mail:3pane", "chrome://messenger/content/");
}


function toAddressBook() 
{
	toOpenWindowByType("mail:addressbook", "chrome://addressbook/content/addressbook.xul");
}

function toNewsgroups() 
{

        dump ("Sorry, command not implemented.\n");

}

// Set up a lame hack to avoid opening two bookmarks.
// Could otherwise happen with two Ctrl-B's in a row.
var gDisableHistory = false;
function enableHistory() {
  gDisableHistory = false;
}

function toHistory()
{
  // Use a single sidebar history dialog

  var cwindowManager = Components.classes['component://netscape/rdf/datasource?name=window-mediator'].getService();
  var iwindowManager = Components.interfaces.nsIWindowMediator;
  var windowManager  = cwindowManager.QueryInterface(iwindowManager);

  var historyWindow = windowManager.getMostRecentWindow('history:manager');

  if (historyWindow) {
    //debug("Reuse existing history window");
    historyWindow.focus();
  } else {
    //debug("Open a new history dialog");

    if (true == gDisableHistory) {
      //debug("Recently opened one. Wait a little bit.");
      return;
    }
    gDisableHistory = true;

    window.open( "chrome://history/content/", "_blank", "chrome,menubar,resizable,scrollbars" );
    setTimeout(enableHistory, 2000);
  }

}

function toJavaConsole()
{
	try{
		var cid =
			Components.classes['component://netscape/oji/jvm-mgr'];
		var iid = Components.interfaces.nsIJVMManager;
		var jvmMgr = cid.getService(iid);
		jvmMgr.ShowJavaConsole();
	} catch(e) {
		
	}
}

function toOpenWindowByType( inType, uri )
{
	var windowManager = Components.classes['component://netscape/rdf/datasource?name=window-mediator'].getService();

	var	windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);

	var topWindow = windowManagerInterface.getMostRecentWindow( inType );
	
	if ( topWindow )
		topWindow.focus();
	else
		window.open(uri, "_blank", "chrome,menubar,toolbar,resizable");
}


function OpenBrowserWindow()
{
  dump("In OpenBrowserWindw()...\n");
  var handler = Components.classes['component://netscape/appshell/component/browser/cmdhandler'];
  handler = handler.getService();
  handler = handler.QueryInterface(Components.interfaces.nsICmdLineHandler);
                    startpage = handler.defaultArgs;
  var startpage = handler.defaultArgs;
  var url = handler.chromeUrlForTask;
  window.openDialog(url, "_blank", "chrome,all,dialog=no", startpage );
}


function CycleWindow( inType, inChromeURL )
{
	var windowManager = Components.classes['component://netscape/rdf/datasource?name=window-mediator'].getService();
	dump("got window Manager \n");
	var	windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    dump("got interface \n");
    
    var desiredWindow = null;
    
	var topWindowOfType = windowManagerInterface.getMostRecentWindow( inType );
	var topWindow = windowManagerInterface.getMostRecentWindow( null );
	dump( "got windows \n");
	
	dump( "topWindowOfType = " + topWindowOfType + "\n");
	if ( topWindowOfType == null )
	{
	  if ( inType == "navigator:browser" )
      OpenBrowserWindow();
    else if ( inType == "composer:html" ) /* open editor window */
      NewEditorWindow();
    else
    {
        /* what to do here? */
    }
    
		return;
	}
	
	if ( topWindowOfType != topWindow )
	{
		dump( "first not top so give focus \n");
		topWindowOfType.focus();
		return;
	}
	
	var enumerator = windowManagerInterface.getEnumerator( inType );
	firstWindow = windowManagerInterface.convertISupportsToDOMWindow ( enumerator.GetNext() );
	if ( firstWindow == topWindowOfType )
	{
		dump( "top most window is first window \n");
		firstWindow = null;
	}
	else
	{
		dump("find topmost window \n");
		while ( enumerator.HasMoreElements() )
		{
			var nextWindow = windowManagerInterface.convertISupportsToDOMWindow ( enumerator.GetNext() );
			if ( nextWindow == topWindowOfType )
				break;
		}	
	}
	desiredWindow = firstWindow;
	if ( enumerator.HasMoreElements() )
	{
		dump( "Give focus to next window in the list \n");
		desiredWindow = windowManagerInterface.convertISupportsToDOMWindow ( enumerator.GetNext() );		
	}
	
	if ( desiredWindow )
	{
		desiredWindow.focus();
		dump("focusing window \n");
	}
	else
	{
		dump("open window \n");
	  if ( inType == "navigator:browser" )
      window.OpenBrowserWindow();
    else if ( inType == "composer:html" ) /* open editor window */
      NewEditorWindow();
    else
    {
        /* what to do here? */
    }
	}
}

function toEditor()
{
	CycleWindow('composer:html', 'chrome://editor/content/');
}

function ShowWindowFromResource( node )
{
	var windowManager = Components.classes['component://netscape/rdf/datasource?name=window-mediator'].getService();
	dump("got window Manager \n");
	var	windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    dump("got interface \n");
    
    var desiredWindow = null;
    var url = node.getAttribute('id');
    dump( url +" finding \n" );
	desiredWindow = windowManagerInterface.getWindowForResource( url );
	dump( "got window \n");
	if ( desiredWindow )
	{
		dump("focusing \n");
		desiredWindow.focus();
	}
}

function OpenTaskURL( inURL )
{
	dump("loading "+inURL+"\n");
	
	window.open( inURL );
}

function ShowUpdateFromResource( node )
{
	var url = node.getAttribute('url');
    dump( url +" finding \n" );
        // hack until I get a new interface on xpiflash to do a 
        // look up on the name/url pair.
	OpenTaskURL( "http://www.mozilla.org/binaries.html");
}
/** 
 * WALLET submenu
 */

function CheckForWallet()
{
  // remove wallet functions (unless overruled by the "wallet.enabled" pref)
  try {
    if (!this.pref.GetBoolPref("wallet.enabled")) {
      var element;
      element = document.getElementById("walletSafeFill");
      element.setAttribute("style","display: none;" );
      element.setAttribute("disabled","true" );
      element = document.getElementById("walletQuickFill");
      element.setAttribute("style","display: none;" );
      element.setAttribute("disabled","true" );
      element = document.getElementById("walletCapture");
      element.setAttribute("style","display: none;" );
      element.setAttribute("disabled","true" );
      element = document.getElementById("walletSeparator");
      element.setAttribute("style","display: none;" );
      element.setAttribute("disabled","true" );
      element = document.getElementById("walleteditor");
      element.setAttribute("style","display: none;" );
      element.setAttribute("disabled","true" );
      element = document.getElementById("walletSamples");
      element.setAttribute("style","display: none;" );
      element.setAttribute("disabled","true" );
    }
  } catch(e) {
    dump("wallet.enabled pref is missing from all.js");
  }
}

// perform a wallet action
function WalletAction( action ) 
{

  if (action == "password") {
    /* process "password" independent of appcore so it can be called from mail */
    wallet = Components.classes['component://netscape/wallet'];
    wallet = wallet.getService();
    wallet = wallet.QueryInterface(Components.interfaces.nsIWalletService);
    wallet.WALLET_ChangePassword();
    return;
  }

  if( appCore ) {
    switch( action ) {
      case "safefill":
        appCore.walletPreview(window, window.content);
        break;
//    case "password":
//      appCore.walletChangePassword();
//      break;
      case "quickfill": 
        appCore.walletQuickFillin(window.content);
        break;
      case "capture":
      default:
        appCore.walletRequestToCapture(window.content);
        break;
    }
  }
}  

// display a Wallet Dialog
function WalletDialog( which )
{
  switch( which ) {
    case "signon":
      window.openDialog("chrome://wallet/content/SignonViewer.xul","SSViewer","modal,chrome,height=504,width=436"); 
      break;
    case "cookie":
      window.openDialog("chrome://wallet/content/CookieViewer.xul","CookieViewer","modal,chrome,height=504,width=436"); 
      break;
    case "samples":
      window.content.location.href= 'http://www.mozilla.org/wallet/samples/';
      break;
    case "wallet":
    default:
      window.openDialog("chrome://wallet/content/WalletEditor.xul","walletEditor","modal,chrome,height=504,width=436"); 
      break;
  }
}

function toImport()
{
	window.openDialog( 	"chrome://messenger/content/importDialog.xul", 
						"importDialog", 
						"chrome, modal", 
						{importType: "addressbook"});
}

