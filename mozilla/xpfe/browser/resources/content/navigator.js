/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

  var appCore = null;
  var defaultStatus = "default status text";
  var explicitURL = false;

/*
// pinkerton 9/21/99
// disabling d&d code for pre-alpha thang
  function BeginDragPersonalToolbar ( event )
  {
    //XXX we rely on a capturer to already have determined which item the mouse was over
    //XXX and have set an attribute.
    
    // if the click is on the toolbar proper, ignore it. We only care about clicks on
    // items.
    var toolbar = document.getElementById("PersonalToolbar");
    if ( event.target == toolbar )
      return true;  // continue propagating the event
    
    var dragStarted = false;
    var dragService = Components.classes["component://netscape/widget/dragservice"].getService();
    if ( dragService ) dragService = dragService.QueryInterface(Components.interfaces.nsIDragService);
    if ( dragService ) {
      var trans = Components.classes["component://netscape/widget/transferable"].createInstance();
      if ( trans ) trans = trans.QueryInterface(Components.interfaces.nsITransferable);
      if ( trans ) {
        trans.addDataFlavor("moz/toolbaritem");
        var genData = Components.classes["component://netscape/supports-wstring"].createInstance();
        var data = null;
        if ( genData ) data = genData.QueryInterface(Components.interfaces.nsISupportsWString);
        if ( data ) {
        
		var id = event.target.getAttribute("id");
		data.data = id;

		dump("ID: " + id + "\n");

		var database = toolbar.database;
		var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
		if (rdf)   rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);
		if ((!rdf) || (!database))	return(false);

		// make sure its a bookmark, bookmark separator, or bookmark folder
		var src = rdf.GetResource(id, true);
		var prop = rdf.GetResource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type", true);
		var target = database.GetTarget(src, prop, true);
		if (target)	target = target.QueryInterface(Components.interfaces.nsIRDFResource);
		if (target)	target = target.Value;
		if ((!target) || (target == ""))	return(false);

		dump("Type: '" + target + "'\n");

		if ((target != "http://home.netscape.com/NC-rdf#BookmarkSeparator") &&
		   (target != "http://home.netscape.com/NC-rdf#Bookmark") &&
		   (target != "http://home.netscape.com/NC-rdf#Folder"))	return(false);

          trans.setTransferData ( "moz/toolbaritem", genData, id.length*2 );  // double byte data (19*2)
          var transArray = Components.classes["component://netscape/supports-array"].createInstance();
          if ( transArray ) transArray = transArray.QueryInterface(Components.interfaces.nsISupportsArray);
          if ( transArray ) {
            // put it into the transferable as an |nsISupports|
            var genTrans = trans.QueryInterface(Components.interfaces.nsISupports);
            transArray.AppendElement(genTrans);
            var nsIDragService = Components.interfaces.nsIDragService;
            dragService.invokeDragSession ( transArray, null, nsIDragService.DRAGDROP_ACTION_COPY + 
                                                nsIDragService.DRAGDROP_ACTION_MOVE );
            dragStarted = true;
          }
        } // if data object
      } // if transferable
    } // if drag service

    return !dragStarted;  // don't propagate the event if a drag has begun

  } // BeginDragPersonalToolbar
  
  
  function DropPersonalToolbar ( event )
  { 
    var dropAccepted = false;
    
    var dragService = Components.classes["component://netscape/widget/dragservice"].getService();
    if ( dragService ) dragService = dragService.QueryInterface(Components.interfaces.nsIDragService);
    if ( dragService ) {
      var dragSession = dragService.getCurrentSession();
      if ( dragSession ) {
        var trans = Components.classes["component://netscape/widget/transferable"].createInstance();
        if ( trans ) trans = trans.QueryInterface(Components.interfaces.nsITransferable);
        if ( trans ) {

		// get references to various services/resources once up-front
		var personalToolbarRes = null;
		
		var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
		if (rdf)   rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);
		if (rdf)
		{
			personalToolbarRes = rdf.GetResource("NC:PersonalToolbarFolder");
		}
		var rdfc = Components.classes["component://netscape/rdf/container"].getService();
		if (rdfc)  rdfc = rdfc.QueryInterface(Components.interfaces.nsIRDFContainer);

          trans.addDataFlavor("moz/toolbaritem");
          for ( var i = 0; i < dragSession.numDropItems; ++i ) {
            dragSession.getData ( trans, i );
            var dataObj = new Object();
            var bestFlavor = new Object();
            var len = new Object();
            trans.getAnyTransferData ( bestFlavor, dataObj, len );
            if ( dataObj ) dataObj = dataObj.value.QueryInterface(Components.interfaces.nsISupportsWString);
            if ( dataObj ) {
            
              // remember len is in bytes, not chars
              var id = dataObj.data.substring(0, len.value / 2);
              dump("ID: '" + id + "'\n");
              
		var objectRes = rdf.GetResource(id, true);

              dragSession.canDrop = true;
              var dropAccepted = true;
              
		var toolbar = document.getElementById("PersonalToolbar");
		var database = toolbar.database;
		if (database && rdf && rdfc && personalToolbarRes && objectRes)
		{
			
			rdfc.Init(database, personalToolbarRes);

			// Note: RDF Sequences are one-based, not zero-based

			// XXX figure out the REAL index to insert at;
			// for the moment, insert it as the first element (pos=1)
			var newIndex = 1;
	
			var currentIndex = rdfc.IndexOf(objectRes);
			if (currentIndex > 0)
			{
				dump("Element '" + id + "' was at position # " + currentIndex + "\n");
//				rdfc.RemoveElement(objectRes, true);
				dump("Element '" + id + "' removed from position # " + currentIndex + "\n");
			}
//			rdfc.InsertElementAt(objectRes, newIndex, true);
			dump("Element '" + id + "' re-inserted at new position # " + newIndex + ".\n");
		}

            } 
          
          } // foreach drag item
      
        } // if transferable
      } // if dragsession
    } // if dragservice
    
    return !dropAccepted;  // don't propagate the event if a drag was accepted

  } // DropPersonalToolbar
  
  
  function DragOverPersonalToolbar ( event )
  {
    var validFlavor = false;

    var dragService = Components.classes["component://netscape/widget/dragservice"].getService();
    if ( dragService ) dragService = dragService.QueryInterface(Components.interfaces.nsIDragService);
    if ( dragService ) {
      var dragSession = dragService.getCurrentSession();
      if ( dragSession ) {
        if ( dragSession.isDataFlavorSupported("moz/toolbaritem") )
          validFlavor = true;
        //XXX other flavors here...
      }
    }

    // touch the attribute to trigger the repaint with the drop feedback.
    if ( validFlavor ) {
      //XXX this is really slow and likes to refresh N times per second.
      var toolbar = document.getElementById("PersonalToolbar");
      toolbar.setAttribute ( "tb-triggerrepaint", 0 );
    }

    return true;

  } // DragOverPersonalToolbar
*/

function UpdateHistory(event)
{
    // This is registered as a capturing "load" event handler. When a
    // document load completes in the content window, we'll be
    // notified here. This allows us to update the global history and
    // set the document's title information.

   //  dump("UpdateHistory: content's location is '" + window.content.location.href + "',\n");
    //dump("                         title is '" + window.content.document.title + "'\n");

	if (window.content.location.href && window.content.location.href != "")
	{
		try
		{
			var history = Components.classes["component://netscape/browser/global-history"].getService();
			if (history) history = history.QueryInterface(Components.interfaces.nsIGlobalHistory);
			if (history) history.SetPageTitle(window.content.location.href, window.content.document.title);
		}
		catch (ex)
		{
			dump("failed to set the page title.\n");
		}
	}
}

function UpdateBookmarksLastVisitedDate(event)
{

	if (window.content.location.href && window.content.location.href != "")
	{
		// if the URL is bookmarked, update its "Last Visited" date
		var bmks = Components.classes["component://netscape/browser/bookmarks-service"].getService();
		if (bmks)	bmks = bmks.QueryInterface(Components.interfaces.nsIBookmarksService);
		try
		{
			if (bmks)	bmks.UpdateBookmarkLastVisitedDate(window.content.location.href);
		}
		catch(ex)
		{
			dump("failed to update bookmark last visited date.\n");
		}
	}
}

  function createBrowserInstance() {
    appCore = Components
                .classes[ "component://netscape/appshell/component/browser/instance" ]
                  .createInstance( Components.interfaces.nsIBrowserInstance );
    if ( !appCore ) {
        alert( "Error creating browser instance\n" );
    }
  }

  function Startup()
  {
    // Position window.
    var win = document.getElementById( "main-window" );
    var x = win.getAttribute( "x" );
    var y = win.getAttribute( "y" );
    // dump(" move to "+x+" "+y+"\n");
    window.moveTo( x, y );

    //  TileWindow();
    // Make sure window fits on screen initially
   // FitToScreen();
	
	// Set default home page
	 var pref = Components.classes['component://netscape/preferences'];
     if (pref)
     {
     	pref = pref.getService();
     	// dump("QIing for pref interface\n");
     	
     	pref = pref.QueryInterface(Components.interfaces.nsIPref);
      	if ( pref )
      	{
         	var homebutton = document.getElementById("home-button")
      		if ( homebutton )
      		{
      			var defaultURL = homebutton.getAttribute("defaultURL" );
      		//	dump("Set default homepage to +"+defaultURL+" \n");
      			pref.SetDefaultCharPref( "browser.startup.homepage",defaultURL);
      		}
      	}
      }

	
    // Create the browser instance component.
    createBrowserInstance();
    if (appCore == null) {
        // Give up.
        window.close();
    }

    // Initialize browser instance..
    appCore.setWebShellWindow(window);
    
    tryToSetContentWindow();
	
    // Add a capturing event listener to the content window so we'll
    // be notified when onloads complete.
    window.addEventListener("load", UpdateHistory, true);
    window.addEventListener("load", UpdateBookmarksLastVisitedDate, true);
	 // Check for window.arguments[0].  If present, go to that url.
    if ( window.arguments && window.arguments[0] ) {
        // Load it using yet another psuedo-onload handler.
        onLoadViaOpenDialog();
    }
   
    sidebarOverlayInit();
  }

  function Shutdown() {
    // Get the current window position/size.
    var x = window.screenX;
    var y = window.screenY;
    var h = window.outerHeight;
    var w = window.outerWidth;
	
    // Store these into the window attributes (for persistence).
    var win = document.getElementById( "main-window" );
    win.setAttribute( "x", x );
    win.setAttribute( "y", y );
    win.setAttribute( "height", h );
    win.setAttribute( "width", w );

    // Close the app core.
    if ( appCore ) {
        appCore.close();
    }
  }

  function onLoadWithArgs() {
    // See if Startup has been run.
    if ( appCore ) {
        // See if load in progress (loading default page).
        if ( document.getElementById("Browser:Throbber").getAttribute("busy") == "true" ) {
            dump( "Stopping load of default initial page\n" );
            appCore.stop();
        }
        dump( "Loading page specified on ShowWindowWithArgs\n" );
        appCore.loadInitialPage();
    } else {
        // onLoad handler timing is not correct yet.
        dump( "onLoadWithArgs not needed yet\n" );
        // Remember that we want this url.
        explicitURL = true;
    }
  }

  function onLoadViaOpenDialog() {
    // See if load in progress (loading default page).
    if ( document.getElementById("Browser:Throbber").getAttribute("busy") == "true" ) {
        dump( "Stopping load of default initial page\n" );
        appCore.stop();
    }
    dump( "Loading page specified via openDialog\n" );
    dump("Check if a view source window \n");
    if( window.arguments[1]=="view-source" )
    {
    	dump(" A view source window \n");
    	var element = document.getElementById("main-window");
    	
    	var preface = element.getAttribute("viewsourcetitlepreface");
    	element.setAttribute( "titlepreface", preface );
    	appCore.isViewSource = true;
    	element.setAttribute("windowtype","Browser:view-source");
    }
    appCore.loadUrl( window.arguments[0] );
  }

  function tryToSetContentWindow() {
    if ( window.content ) {
		var pref = 0;
        dump("Setting content window\n");
        appCore.setContentWindow( window.content );
        // Have browser app core load appropriate initial page.

        if ( !explicitURL ) {
            pref = Components.classes['component://netscape/preferences'];
    
            // if all else fails, use trusty "about:blank" as the start page
            var startpage = "about:blank";  
            if (pref) {
              pref = pref.getService();
            }
	    else {
		dump("failed to get component://netscape/preferences\n");
	    }
            if (pref) {
              pref = pref.QueryInterface(Components.interfaces.nsIPref);
            }
	    else {
		dump("failed to get pref service\n");
	    }
            if (pref) {
              // from mozilla/modules/libpref/src/init/all.js
              // 0 = blank 
              // 1 = home (browser.startup.homepage)
              // 2 = last 
		choice = 1;
		try {
              		choice = pref.GetIntPref("browser.startup.page");
		}
		catch (ex) {
			dump("failed to get the browser.startup.page pref\n");
		}
		dump("browser.startup.page = " + choice + "\n");
    	  switch (choice) {
    		case 0:
                		startpage = "about:blank";
          			break;
    		case 1:
				try {
                			startpage = pref.CopyCharPref("browser.startup.homepage");
				}
				catch (ex) {
					dump("failed to get the browser.startup.homepage pref\n");
					startpage = "about:blank";
				}
          			break;
    		case 2:
                		var history = Components.classes['component://netscape/browser/global-history'];
    			if (history) {
                   			history = history.getService();
    	    		}
    	    		if (history) {
                  			history = history.QueryInterface(Components.interfaces.nsIGlobalHistory);
    	    		}
    	    		if (history) {
    				startpage = history.GetLastPageVisted();
    	    		}
          			break;
       		default:
                		startpage = "about:blank";
    	  }
            }
            else {
		dump("failed to QI pref service\n");
	    }
	    dump("startpage = " + startpage + "\n");
            document.getElementById("args").setAttribute("value", startpage);
        }
        appCore.loadInitialPage();
    } else {
        // Try again.
        dump("Scheduling later attempt to set content window\n");
        window.setTimeout( "tryToSetContentWindow()", 100 );
    }
  }

  function Translate(src, dest, engine)
  {
	var service = "http://levis.alis.com:8080";
	service += "?AlisSourceLang=" + src;
	service += "&AlisTargetLang=" + dest;
	service += "&AlisMTEngine=" + engine;

	// if we're already viewing a translated page, the just get the
	// last argument (which we expect to always be "AlisMTEngine")
	var targetURI = window.content.location.href;
	var targetURIIndex = targetURI.indexOf("AlisTargetURI=");
	if (targetURIIndex >= 0)
	{
		targetURI = targetURI.substring(targetURIIndex + 14);
	}
	service += "&AlisTargetURI=" + targetURI;

	window.content.location.href = service;
  }

  function RefreshUrlbar()
  {
   //Refresh the urlbar bar
    document.getElementById('urlbar').value = window.content.location.href;
  }

  function gotoHistoryIndex(index)
  {
     appCore.gotoHistoryIndex(index);
  }

  function BrowserBack()
  {
     // Get a handle to the back-button
     var bb = document.getElementById("canGoBack");
     // If the button is disabled, don't bother calling in to Appcore
     if ( (bb.getAttribute("disabled")) == "true" ) 
        return;

    if (appCore != null) {
      dump("Going Back\n");
      appCore.back();
    } else {
      dump("BrowserAppCore has not been created!\n");
    }
  }


  function BrowserForward()
  {
     // Get a handle to the back-button
     var fb = document.getElementById("canGoForward");
     // If the button is disabled, don't bother calling in to Appcore
     if ( (fb.getAttribute("disabled")) == "true" ) 
        return;

    if (appCore != null) {
      dump("Going Forward\n");
      appCore.forward();
    } else {
      dump("BrowserAppCore has not been created!\n");
    }
  }



  function BrowserCanStop() {
    var stop = document.getElementById("canStop");
    if ( stop ) {
        var stopDisabled = stop.getAttribute("disabled");
        var stopButton   = document.getElementById( "stop-button" );
        if ( stopButton ) {
            if ( stopDisabled == "true") {
                stopButton.setAttribute( "disabled", "true" );
            } else {
                stopButton.setAttribute( "disabled", "" );
            }
        }
        //Enable/disable the stop menu item
        var stopMenu   = document.getElementById( "menuitem-stop" );
        if ( stopMenu ) {
            if ( stopDisabled == "true") {
                stopMenu.setAttribute( "disabled", "true" );
            } else {
                stopMenu.setAttribute( "disabled", "" );
            }
        }
    }
  }

  function BrowserStop() {
     // Get a handle to the "canStop" broadcast id
     var stopBElem = document.getElementById("canStop");
     if (!stopBElem) {
        dump("Couldn't obtain handle to stop Broadcast element\n");
        return;
	   }

     var canStop = stopBElem.getAttribute("disabled");
     var sb = document.getElementById("stop-button");
     
     if (!sb) {
     	 dump("Could not obtain handle to stop button\n");
	     return;
     }

     // If the stop button is currently disabled, just return
     if ((sb.getAttribute("disabled")) == "true") {
	    return;
     }
	
     //Stop button has just been pressed. Disable it. 
     sb.setAttribute("disabled", "true");

     // Get a handle to the stop menu item.
     var sm = document.getElementById("menuitem-stop");
     if (!sm) {
       dump("Couldn't obtain menu item Stop\n");
     } else {
       // Disable the stop menu-item.
       sm.setAttribute("disabled", "true");
     }
  
     //Call in to BrowserAppcore to stop the current loading
     if (appCore != null) {
        dump("Going to Stop\n");
        appCore.stop();
     } else {
        dump("BrowserAppCore has not been created!\n");
     }
  }
 

  function BrowserReallyReload(reloadType) {
     // Get a handle to the "canReload" broadcast id
     var reloadBElem = document.getElementById("canReload");
     if (!reloadBElem) {
        dump("Couldn't obtain handle to reload Broadcast element\n");
        return;
	   }

     var canreload = reloadBElem.getAttribute("disabled");
     

     // If the reload button is currently disabled, just return
     if ( canreload) {
	     return;
     }
	
     //Call in to BrowserAppcore to reload the current loading
     if (appCore != null) {
        dump("Going to reload\n");
        appCore.reload(reloadType);
     } else {
        dump("BrowserAppCore has not been created!\n");
     }
  }

  function BrowserHome()
  {
   window.content.home();
   RefreshUrlbar();
  }

  function OpenBookmarkURL(node, root)
  {
    if (node.getAttribute('container') == "true") {
      return false;
    }

	var url = node.getAttribute('id');
	try
	{
		var rootNode = document.getElementById(root);
		var ds = null;
		if (rootNode)
		{
			ds = rootNode.database;
		}
		// add support for IE favorites under Win32, and NetPositive URLs under BeOS
		var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
		if (rdf)   rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);
		if (rdf)
		{
			if (ds)
			{
				var src = rdf.GetResource(url, true);
				var prop = rdf.GetResource("http://home.netscape.com/NC-rdf#URL", true);
				var target = ds.GetTarget(src, prop, true);
				if (target)	target = target.QueryInterface(Components.interfaces.nsIRDFLiteral);
				if (target)	target = target.Value;
				if (target)	url = target;
				
			}
		}
	}
	catch(ex)
	{
	}

    // Ignore "NC:" urls.
    if (url.substring(0, 3) == "NC:") {
      return false;
    }

    window.content.location.href = url;
    RefreshUrlbar();
  }

function OpenSearch(tabName, searchStr)
{
	dump("OpenSearch searchStr: '" + searchStr + "'\n\n");

	window.openDialog("resource:/res/samples/search.xul", "SearchWindow", "dialog=no,close,chrome,resizable", tabName, searchStr);
}

  function BrowserNewWindow()
  {
    OpenBrowserWindow();
  }

  function BrowserEditPage(url)
  {
    window.openDialog( "chrome://editor/content", "_blank", "chrome,all,dialog=no", url );
  }
  
  function BrowserNewEditorWindow()
  {
    // Open editor window to default page.
    BrowserEditPage( "resource:/res/html/empty_doc.html" );
  }
  
  function BrowserOpenWindow()
  {
    //opens a window where users can select a web location to open
    window.openDialog( "chrome://navigator/content/openLocation.xul", "_blank", "chrome,modal", appCore );
  }
  
  function createInstance( progid, iidName ) {
      var iid = eval( "Components.interfaces." + iidName );
      return Components.classes[ progid ].createInstance( iid );
  }

  function createInstanceById( cid, iidName ) {
      var iid = eval( "Components.interfaces." + iidName );
      return Components.classesByID[ cid ].createInstance( iid );
  }

  function getService( progid, iidName ) {
      var iid = eval( "Components.interfaces." + iidName );
      return Components.classes[ progid ].getService( iid );
  }

  function getServiceById( cid, iidName ) {
      var iid = eval( "Components.interfaces." + iidName );
      return Components.classesByID[ cid ].getService( iid );
  }

  function openNewWindowWith( url ) {
    var newWin = window.openDialog( "chrome://navigator/content/navigator.xul", "_blank", "chrome,all,dialog=no", url );

    // Fix new window.    
    newWin.saveFileAndPos = true;
  }
  
  function BrowserOpenFileWindow()
  {
    // Get filespecwithui component.            
    var fileSpec = createInstance( "component://netscape/filespecwithui", "nsIFileSpecWithUI" );
    var url = fileSpec.chooseFile( "Open File" );
    if ( url != "" ) {
        openNewWindowWith( url );
    }
  }

  function OpenFile(url) {
    // Obsolete (called from C++ code that is no longer called).
    dump( "OpenFile called?\n" );
    openNewWindowWith( url );
  }

  function BrowserCopy()
  {
    if (appCore != null) {
	    dump("Copying\n");
      appCore.copy();
    } else {
      dump("BrowserAppCore has not been created!\n");
    }
  }


  function BrowserAddBookmark(url,title)
  {
    var bmks = Components.classes["component://netscape/browser/bookmarks-service"].getService();
    bmks = bmks.QueryInterface(Components.interfaces.nsIBookmarksService);
    bmks.AddBookmark(url, title);
  }

  function BrowserEditBookmarks()
  { 
    window.open("chrome://bookmarks/content/", "BookmarksWindow", "chrome,menubar,resizable,scrollbars");
  }

  function BrowserPrintPreview()
  {
    // Borrowing this method to show how to 
    // dynamically change icons
    dump("BrowserPrintPreview\n");
    if (appCore != null) {
	    dump("Changing Icons\n");
      appCore.printPreview();
    } else {
      dump("BrowserAppCore has not been created!\n");
    }
  }

  function BrowserPrint()
  {
    // Borrowing this method to show how to 
    // dynamically change icons
    if (appCore != null) {
      appCore.print();
    }
  }

  function BrowserSetDefaultCharacterSet(aCharset)
  {
    if (appCore != null) {
      appCore.SetDocumentCharset(aCharset);
      window.content.location.reload();
    } else {
      dump("BrowserAppCore has not been created!\n");
    }
  }

  function BrowserClose()
  {
    // This code replicates stuff in Shutdown().  It is here because
    // window.screenX and window.screenY have real values.  We need
    // to fix this eventually but by replicating the code here, we
    // provide a means of saving position (it just requires that the
    // user close the window via File->Close (vs. close box).

    // Get the current window position/size.
    var x = window.screenX;
    var y = window.screenY;
    var h = window.outerHeight;
    var w = window.outerWidth;

    // Store these into the window attributes (for persistence).
    var win = document.getElementById( "main-window" );
    win.setAttribute( "x", x );
    win.setAttribute( "y", y );
    win.setAttribute( "height", h );
    win.setAttribute( "width", w );

  	 window.close();
  }



  function BrowserSelectAll() {
    if (appCore != null) {
        appCore.selectAll();
    } else {
        dump("BrowserAppCore has not been created!\n");
    }
  }

  function BrowserFind() {
    if (appCore != null) {
        appCore.find();      
    } else {
        dump("BrowserAppCore has not been created!\n");
    }
  }

  function BrowserFindAgain() {
    if (appCore != null) {
        appCore.findNext();      
    } else {
        dump("BrowserAppCore has not been created!\n");
    }
  }

  function BrowserLoadURL()
  {
	if (appCore == null)
	{
		dump("BrowserAppCore has not been initialized\n");
		return;
	}

    // rjc: added support for URL shortcuts (3/30/1999)
    try {
      var bmks = Components.classes["component://netscape/browser/bookmarks-service"].getService();
      bmks = bmks.QueryInterface(Components.interfaces.nsIBookmarksService);

      var shortcutURL = bmks.FindShortcut(document.getElementById('urlbar').value);

      dump("FindShortcut: in='" + document.getElementById('urlbar').value + "'  out='" + shortcutURL + "'\n");

      if ((shortcutURL != null) && (shortcutURL != "")) {
        document.getElementById('urlbar').value = shortcutURL;
      }
    }
    catch (ex) {
      // stifle any exceptions so we're sure to load the URL.
    }

	appCore.loadUrl(document.getElementById('urlbar').value);
      
  }

  function WalletEditor()
  {
  	window.open("chrome://wallet/content/WalletEditor.xul","walletEditor","modal,chrome,height=504,width=436"); 
    
  }

  function WalletSafeFillin()
  {
    if (appCore != null) {
      dump("Wallet Safe Fillin\n");
      appCore.walletPreview(window, window.content);
    } else {
      dump("BrowserAppCore has not been created!\n");
    }
  }

  function WalletChangePassword()
  {
    if (appCore != null) {
      dump("Wallet Change Password\n");
      appCore.walletChangePassword();
    } else {
      dump("BrowserAppCore has not been created!\n");
    }
  }


  function WalletQuickFillin()
  {
    if (appCore != null) {
      dump("Wallet Quick Fillin\n");
      appCore.walletQuickFillin(window.content);
    } else {
      dump("BrowserAppCore has not been created!\n");
    }
  }

  function WalletRequestToCapture()
  {
    if (appCore != null) {
      dump("Wallet Request To Capture\n");
      appCore.walletRequestToCapture(window.content);
    } else {
      dump("BrowserAppCore has not been created!\n");
    }
  }

  function WalletSamples()
  {
    window.content.location.href= 'http://people.netscape.com/morse/wallet/samples/';
  }

  function SignonViewer()
  {
      window.open("chrome://wallet/content/SignonViewer.xul","SSViewer","modal,chrome,height=504,width=436"); 
  }

  function CookieViewer()
  {
   window.open("chrome://wallet/content/CookieViewer.xul","CookieViewer","modal,chrome,height=504,width=436"); 
  }

  function OpenMessenger()
  {
	window.open("chrome://messenger/content/", "_blank", "chrome,menubar,toolbar,resizable");
  }

  function OpenAddressbook()
  {
	window.open("chrome://addressbook/content/", "_blank", "chrome,menubar,toolbar,resizable");
  }

  function MsgNewMessage()
  {
    // Open message compose window.
    window.openDialog( "chrome://messengercompose/content/", "_blank", "chrome,all,dialog=no", "" );
  }
  
  function BrowserViewSource()
  {
	dump("BrowserViewSource(); \n ");
   //  window.openDialog( "chrome://navigator/content/viewSource.xul", "_blank", "chrome,all,dialog=no", window.content.location );
     // Use a browser window to view source
    window.openDialog( "chrome://navigator/content/", "_blank", "chrome,menubar,status,dialog=no", window.content.location,"view-source" );

  }


        var bindCount = 0;
        function onStatus() {
            var status = document.getElementById("Browser:Status");
            if ( status ) {
                var text = status.getAttribute("value");
                if ( text == "" ) {
                    text = defaultStatus;
                }
                var statusText = document.getElementById("statusText");
                if ( statusText ) {
                    statusText.setAttribute( "value", text );
                }
            } else {
                dump("Can't find status broadcaster!\n");
            }
        }
        
        function onProtocolClick()
        {
            //TODO: need to hookup to page info
            var status = document.getElementById("Browser:Status");
            if ( status ) {
			    //var msg = "Hey, don't touch me there!";
				//status.setAttribute("value",msg);
            }
        }
        function onProtocolChange() 
        {
            var protocolIcon = document.getElementById("Browser:ProtocolIcon");
            var uri = protocolIcon.getAttribute("uri");
            var icon = document.getElementById("protocol-icon");
 
            icon.setAttribute("src",uri);
        }

        function doTests() {
        }

		var startTime = 0;
        function onProgress() {
            var throbber = document.getElementById("Browser:Throbber");
            var meter    = document.getElementById("Browser:LoadingProgress");
            if ( throbber && meter ) {
                var busy = throbber.getAttribute("busy");
                var wasBusy = meter.getAttribute("mode") == "undetermined" ? "true" : "false";
                if ( busy == "true" ) {
                    if ( wasBusy == "false" ) {
                        // Remember when loading commenced.
    				    startTime = (new Date()).getTime();
                        // Turn progress meter on.
                        meter.setAttribute("mode","undetermined");
                    }
                    // Update status bar.
                } else if ( busy == "false" && wasBusy == "true" ) {
                    // Record page loading time.
                    var status = document.getElementById("Browser:Status");
                    if ( status ) {
						var elapsed = ( (new Date()).getTime() - startTime ) / 1000;
						var msg = "Document: Done (" + elapsed + " secs)";
						dump( msg + "\n" );
                        status.setAttribute("value",msg);
                        defaultStatus = msg;
                    }
                    // Turn progress meter off.
                    meter.setAttribute("mode","normal");
                }
            }
        }
        function dumpProgress() {
            var broadcaster = document.getElementById("Browser:LoadingProgress");
            var meter       = document.getElementById("meter");
            dump( "bindCount=" + bindCount + "\n" );
            dump( "broadcaster mode=" + broadcaster.getAttribute("mode") + "\n" );
            dump( "broadcaster value=" + broadcaster.getAttribute("value") + "\n" );
            dump( "meter mode=" + meter.getAttribute("mode") + "\n" );
            dump( "meter value=" + meter.getAttribute("value") + "\n" );
        }

function BrowserReload() {
    dump( "Sorry, command not implemented: " + window.event.srcElement + "\n" );
}

function hiddenWindowStartup()
{
	// Disable menus which are not appropriate
	var disabledItems = ['cmd_close', 'Browser:SendPage', 'Browser:EditPage', 'Browser:PrintSetup', 'Browser:PrintPreview',
						 'Browser:Print', 'canGoBack', 'canGoForward', 'Browser:Home', 'Browser:AddBookmark', 'cmd_undo', 
						 'cmd_redo', 'cmd_cut', 'cmd_copy','cmd_paste', 'cmd_delete', 'cmd_selectAll'];
	for ( id in disabledItems )
	{
	//	dump("disabling "+disabledItems[id]+"\n");
		 var broadcaster = document.getElementById( disabledItems[id]);
		 broadcaster.setAttribute("disabled","true");
	}
}

// Tile
function TileWindow()
{
	var xShift = 25;
	var yShift = 50;
	var done = false;
	var windowManager = Components.classes['component://netscape/rdf/datasource?name=window-mediator'].getService();
	dump("got window Manager \n");
	var	windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
	
	var enumerator = windowManagerInterface.GetEnumerator( null );
	
	var xOffset = screen.availLeft;
	var yOffset = screen.availRight;
	do
	{
		var currentWindow = windowManagerInterface.ConvertISupportsToDOMWindow ( enumerator.GetNext() );
		if ( currentWindow.screenX == screenX && currentWindow.screenY == screenY )
		{
			alreadyThere = true;
			break;
		}	
	} while ( enumerator.HasMoreElements() )
	
	if ( alreadyThere )
	{
		enumerator = windowManagerInterface.GetEnumerator( null );
		do
		{
			var currentWindow = windowManagerInterface.ConvertISupportsToDOMWindow ( enumerator.GetNext() );
			if ( currentWindow.screenX == screenX+xOffset*xShift+yOffset*xShift   && currentWindow.screenY == screenY+yShift*xOffset && window != currentWindow )
			{
				xOffset++;
				if ( (screenY+outerHeight  < screen.availHeight) && (screenY+outerHeight+yShift*xOffset > screen.availHeight ) )
				{
					dump(" increment yOffset");
					yOffset++;
					xOffset = 0;
				}
				enumerator = windowManagerInterface.GetEnumerator( null );
			}	
		} while ( enumerator.HasMoreElements() )
	}
	
	if ( xOffset > 0 || yOffset >0 )
	{
		dump( "offsets:"+xOffset+" "+yOffset+"\n");
		dump("Move by ("+ xOffset*xShift + yOffset*xShift +","+ yShift*xOffset +")\n");
		moveBy( xOffset*xShift + yOffset*xShift, yShift*xOffset );
	}
}
// Make sure that a window fits fully on the screen. Will move to preserve size, and then shrink to fit
function FitToScreen()
{
	var moveX = screenX;
	var sizeX = outerWidth;
	var moveY = screenY;
	var sizeY = outerHeight;
	
	dump( " move to ("+moveX+","+moveY+") size to ("+sizeX+","+sizeY+") \n");
	var totalWidth = screenX+outerWidth;
	if ( totalWidth > screen.availWidth )
	{	
		if( outerWidth > screen.availWidth )
		{
			sizeX = screen.availWidth;
			moveX = screen.availLeft;
		}
		else
		{
			moveX = screen.availWidth- outerWidth;
		}
	}
	
	var totalHeight = screenY+outerHeight;
	if ( totalHeight > screen.availHeight )
	{	
		if( outerWidth > screen.availHeight )
		{
			sizeY = screen.availHeight;
			moveY = screen.availTop;
		}
		else
		{
			moveY = screen.availHeight- outerHeight;
		}
	}
	
	
	dump( " move to ("+moveX+","+moveY+") size to ("+sizeX+","+sizeY+") \n");
	if ( (moveY- screenY != 0 ) ||	(moveX-screenX != 0 ) )
		moveTo( moveX,moveY );
	
	// Maintain a minimum size
	if ( sizeY< 100 )
		sizeY = 100;
	if ( sizeX < 100 )
		sizeX = 100; 
	if ( (sizeY- outerHeight != 0 ) ||	(sizeX-outerWidth != 0 ) )
	{
		//outerHeight = sizeY;
		//outerWidth = sizeX;
		resizeTo( sizeX,sizeY );	
	}
}

// Dumps all properties of anObject.
function dumpObject( anObject, prefix ) {
    if ( prefix == null ) {
        prefix = anObject;
    }
    for ( prop in anObject ) {
        dump( prefix + "." + prop + " = " + anObject[prop] + "\n" );
    }
}

// Takes JS expression and dumps "expr="+expr+"\n"
function dumpExpr( expr ) {
    dump( expr+"="+eval(expr)+"\n" );
}
