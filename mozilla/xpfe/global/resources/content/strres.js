var strBundleService = null;
var localeService = null;

function srGetAppLocale()
{
  var applicationLocale = null;

  if (!localeService) {
      try {
          localeService = Components.classes["component://netscape/intl/nslocaleservice"].getService();
      
          localeService = localeService.QueryInterface(Components.interfaces.nsILocaleService);
      } catch (ex) {
          dump("\n--** localeService failed: " + ex + "\n");
          return null;
      }
  }
  
  applicationLocale = localeService.GetApplicationLocale();
  if (!applicationLocale) {
    dump("\n--** localeService.GetApplicationLocale failed **--\n");
  }
  return applicationLocale;
}

function srGetStrBundleWithLocale(path, locale)
{
  var strBundle = null;

  if (!strBundleService) {
      try {
          strBundleService =
              Components.classes["component://netscape/intl/stringbundle"].getService(); 
          strBundleService = 
              strBundleService.QueryInterface(Components.interfaces.nsIStringBundleService);
      } catch (ex) {
          dump("\n--** strBundleService failed: " + ex + "\n");
          return null;
      }
  }

  strBundle = strBundleService.CreateBundle(path, locale); 
  if (!strBundle) {
	dump("\n--** strBundle createInstance failed **--\n");
  }
  return strBundle;
}

function srGetStrBundle(path)
{
  var appLocale = srGetAppLocale();
  return srGetStrBundleWithLocale(path, appLocale);
}


function localeSwitching(winType, baseDirectory, providerName)
{
  dump("\n ** Enter localeSwitching() ** \n");
  dump("\n ** winType=" +  winType + " ** \n");
  dump("\n ** baseDirectory=" +  baseDirectory + " ** \n");
  dump("\n ** providerName=" +  providerName + " ** \n");

  //
  var rdf;
  if(document.rdf) {
    rdf = document.rdf;
    dump("\n ** rdf = document.rdf ** \n");
  }
  else if(Components) {
    var isupports = Components.classes['component://netscape/rdf/rdf-service'].getService();
    rdf = isupports.QueryInterface(Components.interfaces.nsIRDFService);
    dump("\n ** rdf = Components... ** \n");
  }
  else {
    dump("can't find nuthin: no document.rdf, no Components. \n");
  }
  //

  var ds = rdf.GetDataSource("rdf:chrome");

  // For M4 builds, use this line instead.
  // var ds = rdf.GetDataSource("resource:/chrome/registry.rdf");
  var srcURL = "chrome://";
  srcURL += winType + "/locale/";
  dump("\n** srcURL=" + srcURL + " **\n");
  var sourceNode = rdf.GetResource(srcURL);
  var baseArc = rdf.GetResource("http://chrome.mozilla.org/rdf#base");
  var nameArc = rdf.GetResource("http://chrome.mozilla.org/rdf#name");
                      
  // Get the old targets
  var oldBaseTarget = ds.GetTarget(sourceNode, baseArc, true);
  dump("\n** oldBaseTarget=" + oldBaseTarget + "**\n");
  var oldNameTarget = ds.GetTarget(sourceNode, nameArc, true);
  dump("\n** oldNameTarget=" + oldNameTarget + "**\n");

  // Get the new targets 
  // file:/u/tao/gila/mozilla-org/html/projects/intl/chrome/
  // da-DK
  if (baseDirectory == "") {
	baseDirectory =  "resource:/chrome/";
  }

  var finalBase = baseDirectory;
  if (baseDirectory != "") {
	finalBase += winType + "/locale/" + providerName + "/";
  }
  dump("\n** finalBase=" + finalBase + "**\n");

  var newBaseTarget = rdf.GetLiteral(finalBase);
  var newNameTarget = rdf.GetLiteral(providerName);
  
  // Unassert the old relationships
  if (baseDirectory != "") {
  	ds.Unassert(sourceNode, baseArc, oldBaseTarget);
  }

  ds.Unassert(sourceNode, nameArc, oldNameTarget);
  
  // Assert the new relationships (note that we want a reassert rather than
  // an unassert followed by an assert, once reassert is implemented)
  if (baseDirectory != "") {
  	ds.Assert(sourceNode, baseArc, newBaseTarget, true);
  }
  ds.Assert(sourceNode, nameArc, newNameTarget, true);
  
  // Flush the modified data source to disk
  // (Note: crashes in M4 builds, so don't use Flush() until fix checked in)
  ds.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource).Flush();

  // Open up a new window to see your new chrome, since changes aren't yet dynamically
  // applied to the current window

  // BrowserOpenWindow('chrome://addressbook/content');
  dump("\n ** Leave localeSwitching() ** \n");
}

function localeTo(baseDirectory, localeName)
{
  dump("\n ** Enter localeTo() ** \n");

  localeSwitching("addressbook", baseDirectory, localeName); 
  localeSwitching("bookmarks", baseDirectory, localeName); 
  localeSwitching("directory", baseDirectory, localeName); 
  localeSwitching("editor", baseDirectory, localeName); 
  localeSwitching("global", baseDirectory, localeName); 
  localeSwitching("history", baseDirectory, localeName); 
  localeSwitching("messenger", baseDirectory, localeName); 
  localeSwitching("messengercompose", baseDirectory, localeName); 
  localeSwitching("navigator", baseDirectory, localeName); 
  localeSwitching("pref", baseDirectory, localeName); 
  localeSwitching("profile", baseDirectory, localeName); 
  localeSwitching("regviewer", baseDirectory, localeName); 
  localeSwitching("related", baseDirectory, localeName); 
  localeSwitching("sidebar", baseDirectory, localeName); 
  localeSwitching("wallet", baseDirectory, localeName); 
  localeSwitching("xpinstall", baseDirectory, localeName); 
  
  dump("\n ** Leave localeTo() ** \n");
}

function selectLocale(event)
{
  try {
    var chromeRegistry = Components.classes["component://netscape/chrome/chrome-registry"].getService();
    if ( chromeRegistry ) {
      chromeRegistry = chromeRegistry.QueryInterface( Components.interfaces.nsIChromeRegistry );
    }
    var node = event.target;
    var langcode = node.getAttribute('data');
    //var old_lang = chromeRegistry.getSelectedLocale("navigator");
	//dump("\n-->old_lang=" + old_lang + "--");
    chromeRegistry.selectLocale(langcode, true);
	dump("\n-->set new lang, langcode=" + langcode + "--");
    var sbundle = srGetStrBundle("chrome://communicator/locale/utilityOverlay.properties");
	var alertstr = sbundle.GetStringFromName("langAlert");
	alert(alertstr);
  }
  catch(e) {
    dump("\n--> strres.js: selectLocale() failed!\n");
    return false;
  }
  return true;	
}

