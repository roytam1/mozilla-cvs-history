/**
 * Communicator Shared Utility Library
 * for shared application glue for the Communicator suite of applications
 **/

/**
 * Go into online/offline mode
 **/
function setOfflineStatus(aToggleFlag)
{
  var ioService = nsJSComponentManager.getServiceByID("{9ac9e770-18bc-11d3-9337-00104ba0fd40}", 
                                                      "nsIIOService");
  var broadcaster = document.getElementById("Communicator:WorkMode");
  if (aToggleFlag)
    ioService.offline = !ioService.offline;

  var bundle = srGetStrBundle("chrome://communicator/locale/utilityOverlay.properties");                                                      
  if (ioService.offline && broadcaster)
    {
      broadcaster.setAttribute("offline", "true");
      broadcaster.setAttribute("tooltiptext", bundle.GetStringFromName("offlineTooltip"));
      broadcaster.setAttribute("value", bundle.GetStringFromName("goonline"));
      FillInTooltip(broadcaster);
    }
  else if (broadcaster)
    {
      broadcaster.removeAttribute("offline");
      broadcaster.setAttribute("tooltiptext", bundle.GetStringFromName("onlineTooltip"));
      broadcaster.setAttribute("value", bundle.GetStringFromName("gooffline"));
      FillInTooltip(broadcaster);
    }
}


var goPrefWindow = 0;

function getBrowserURL() {

  try {
    var prefs = Components.classes["component://netscape/preferences"];
    if (prefs) {
      prefs = prefs.getService();
      if (prefs)
        prefs = prefs.QueryInterface(Components.interfaces.nsIPref);
    }
    if (prefs) {
      var url = prefs.CopyCharPref("browser.chromeURL");
      if (url)
        return url;
    }
  } catch(e) {
  }
  return "chrome://navigator/content/navigator.xul";
}

function goPageSetup()
{
}

function goEditCardDialog(abURI, card, okCallback)
{
	window.openDialog("chrome://messenger/content/addressbook/abEditCardDialog.xul",
					  "",
					  "chrome,resizeable=no,modal",
					  {abURI:abURI, card:card, okCallback:okCallback});
}


function goPreferences(id, paneURL, paneID)
{
  var prefWindowModalityPref;
  try {
    var pref = Components.classes["component://netscape/preferences"].getService();
    if( pref ) 
      pref = pref.QueryInterface( Components.interfaces.nsIPref );
    if( pref )
      prefWindowModalityPref = pref.GetBoolPref( "browser.prefWindowModal");
  }
  catch(e) {
    prefWindowModalityPref = true;
  }
  var modality = prefWindowModalityPref ? "yes" : "no";
  
  var prefWindow = window.openDialog("chrome://communicator/content/pref/pref.xul","PrefWindow", "chrome,modal=" + modality+ ",resizable=yes", paneURL, paneID);
}


function goToggleToolbar( id, elementID )
{
	dump( "toggling toolbar "+id+"\n");
	var toolbar = document.getElementById( id );
	var element = document.getElementById( elementID );
	if ( toolbar )
	{
	 	var attribValue = toolbar.getAttribute("hidden") ;
	
		//dump("set hidden to "+!attribValue+"\n");
		if ( attribValue != false )
		{
		//	dump( "Show \n");
			toolbar.setAttribute("hidden", "" );
			if ( element )
				element.setAttribute("checked","true")
		}
		else
		{
		//	dump("hide \n");
			toolbar.setAttribute("hidden", true );
			if ( element )
				element.setAttribute("checked","false")
		}
		document.persist(id, 'hidden');
		document.persist(elementID, 'checked');
	}
}


function goClickThrobber( urlPref )
{
	var url;
	try {
		var pref = Components.classes["component://netscape/preferences"].getService();
		if( pref )
		pref = pref.QueryInterface( Components.interfaces.nsIPref );
		url = pref.getLocalizedUnicharPref(urlPref);
	}

	catch(e) {
		url = null;
	}

	if ( url )
		openTopWin(url);
}


//No longer needed.  Rip this out since we are using openTopWin
function goHelpMenu( url )
{
  /* note that this chrome url should probably change to not have all of the navigator controls */
  /* also, do we want to limit the number of help windows that can be spawned? */
  window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", url );
}


function openTopWin( url )
{
  /* note that this chrome url should probably change to not have all of the navigator controls,
     but if we do this we need to have the option for chrome controls because goClickThrobber()
     needs to use this function with chrome controls */
  /* also, do we want to limit the number of help windows that can be spawned? */
    dump("SetPrefToCurrentPage("+ url +") \n ");
    if ((url == null) || (url == "")) return;
     
    var windowManager = Components.classes['component://netscape/rdf/datasource?name=window-mediator'].getService();
    var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
 
    var topWindowOfType = windowManagerInterface.getMostRecentWindow( "navigator:browser" );
    if ( topWindowOfType )
    {
        dump("setting page: " + topWindowOfType._content.location.href + "\n");
        topWindowOfType.focus();
		// urn:
		var pos = url.indexOf("urn:");
		dump("\n ** openTopWin, url=" + url + ", pos=" + pos + "\n");
		if (pos == 0) {
        	topWindowOfType._content.location.href = xlateURL(url);
		}
		else
		{
        	topWindowOfType._content.location.href = url;
		}
    }
    else
    {
        dump(" No browser window. Should be disabling this button \n");
        window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", url );
    }
}

function goAboutDialog()
{
  var defaultAboutState = false;
  try {
    var pref = Components.classes["component://netscape/preferences"].getService();
    if( pref )
      pref = pref.QueryInterface( Components.interfaces.nsIPref );
    defaultAboutState = pref.GetBoolPref("browser.show_about_as_stupid_modal_window");
  }
  catch(e) {
    defaultAboutState = false;
  }
  if( defaultAboutState )
  	window.openDialog("chrome:global/content/about.xul", "About", "modal,chrome,resizable=yes,height=450,width=550");
  else 
    window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", 'chrome://global/content/about.html' );
}


function goUpdateGlobalEditMenuItems()
{
	//dump("Updating edit menu items\n");
	goUpdateCommand('cmd_undo');
	goUpdateCommand('cmd_redo');
	goUpdateCommand('cmd_cut');
	goUpdateCommand('cmd_copy');
	goUpdateCommand('cmd_paste');
	goUpdateCommand('cmd_selectAll');
	goUpdateCommand('cmd_delete');
}

// update menu items that rely on the current selection
function goUpdateSelectEditMenuItems()
{
	//dump("Updating select menu items\n");
	goUpdateCommand('cmd_cut');
	goUpdateCommand('cmd_copy');
	goUpdateCommand('cmd_delete');
}

// update menu items that relate to undo/redo
function goUpdateUndoEditMenuItems()
{
	//dump("Updating undo/redo menu items\n");
	goUpdateCommand('cmd_undo');
	goUpdateCommand('cmd_redo');
}

// update menu items that depend on clipboard contents
function goUpdatePasteMenuItems()
{
	//dump("Updating clipboard menu items\n");
	goUpdateCommand('cmd_paste');
}

// This used to be BrowserNewEditorWindow in navigator.js
function NewEditorWindow()
{
  dump("In NewEditorWindow()...\n");
  // Open editor window with blank page
  // Kludge to leverage openDialog non-modal!
  window.openDialog( "chrome://editor/content", "_blank", "chrome,all,dialog=no", "about:blank");
}

function NewEditorFromTemplate()
{
  dump("NOT IMPLEMENTED: Write NewEditorFromTemplate()!\n")
}

function NewEditorFromDraft()
{
  dump("NOT IMPLEMENTED: Write NewEditorFromDraft()!\n")
}


function helpMenuCreate()
{
    //adding the brand string to the about
	var BrandBundle = srGetStrBundle("chrome://global/locale/brand.properties");
	var aboutStrName = BrandBundle.GetStringFromName("aboutStrName");
	var aboutItem = document.getElementById( "releaseName" );
	aboutItem.setAttribute("value", aboutStrName);
	
	//Adding the release url since it will change based on brand
	var BrandRelUrl = BrandBundle.GetStringFromName("releaseUrl");	
	var relCommand =  "openTopWin(\'" + BrandRelUrl + "\')";
	var relItem = document.getElementById( "releaseUrl" );
	relItem.setAttribute("oncommand", relCommand);
}

