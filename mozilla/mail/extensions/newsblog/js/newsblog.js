/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is the News&Blog Feed Downloader
 *
 *
 * Contributor(s):
 *  Myk Melez <myk@mozilla.org) (Original Author)
 *  David Bienvenu <bienvenu@nventure.com> 
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

var gExternalScriptsLoaded = false;

var nsNewsBlogFeedDownloader =
{
  downloadFeed: function(aUrl, aFolder, aQuickMode, aTitle, aUrlListener, aMsgWindow)
  {
    if (!gExternalScriptsLoaded)
      loadScripts();

    // we might just pull all these args out of the aFolder DB, instead of passing them in...
    var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"]
        .getService(Components.interfaces.nsIRDFService);
    id = rdf.GetResource(aUrl);
    feed = new Feed(id);
    feed.urlListener = aUrlListener;
    feed.folder = aFolder;
    feed.msgWindow = aMsgWindow;
    feed.download();
  },
 
  QueryInterface: function(aIID)
  {
    if (aIID.equals(Components.interfaces.nsINewsBlogFeedDownloader) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
    return null;
  }
}

var nsNewsBlogAcctMgrExtension = 
{ 
  name: "newsblog",
  chromePackageName: "messenger-newsblog",
  showPanel: function (server)
  {
    return server.type == "rss";
  },
  QueryInterface: function(aIID)
  {
    if (aIID.equals(Components.interfaces.nsIMsgAccountManagerExtension) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
    return null;
  }  
}

var nsNewsBlogFeedDownloaderModule =
{
  getClassObject: function(aCompMgr, aCID, aIID)
  {
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this.mObjects) 
      if (aCID.equals(this.mObjects[key].CID))
        return this.mObjects[key].factory;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  mObjects: 
  {
    feedDownloader: 
    { 
      CID: Components.ID("{5c124537-adca-4456-b2b5-641ab687d1f6}"),
      contractID: "@mozilla.org/newsblog-feed-downloader;1",
      className: "News+Blog Feed Downloader",
      factory: 
      {
        createInstance: function (aOuter, aIID) 
        {
          if (aOuter != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
          if (!aIID.equals(Components.interfaces.nsINewsBlogFeedDownloader) &&
              !aIID.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

          // return the singleton
          return nsNewsBlogFeedDownloader.QueryInterface(aIID);
        }       
      } // factory
    }, // feed downloader
    
    nsNewsBlogAcctMgrExtension: 
    { 
      CID: Components.ID("{E109C05F-D304-4ca5-8C44-6DE1BFAF1F74}"),
      contractID: "@mozilla.org/accountmanager/extension;1?name=newsblog",
      className: "News+Blog Account Manager Extension",
      factory: 
      {
        createInstance: function (aOuter, aIID) 
        {
          if (aOuter != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
          if (!aIID.equals(Components.interfaces.nsIMsgAccountManagerExtension) &&
              !aIID.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

          // return the singleton
          return nsNewsBlogAcctMgrExtension.QueryInterface(aIID);
        }       
      } // factory
    } // account manager extension
  },

  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType)
  {        
    aCompMgr = aCompMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    for (var key in this.mObjects) 
    {
      var obj = this.mObjects[key];
      aCompMgr.registerFactoryLocation(obj.CID, obj.className, obj.contractID, aFileSpec, aLocation, aType);
    }

    // we also need to do special account extension registration
    var catman = Components.classes["@mozilla.org/categorymanager;1"].getService(Components.interfaces.nsICategoryManager);
    catman.addCategoryEntry("mailnews-accountmanager-extensions",
                            "newsblog account manager extension",
                            "@mozilla.org/accountmanager/extension;1?name=newsblog", true, true);
  },

  unregisterSelf: function(aCompMgr, aFileSpec, aLocation)
  {
    aCompMgr = aCompMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    for (var key in this.mObjects) 
    {
      var obj = this.mObjects[key];
      aCompMgr.unregisterFactoryLocation(obj.CID, aFileSpec);
    }

    // unregister the account manager extension
    catman = Components.classes["@mozilla.org/categorymanager;1"].getService(Components.interfaces.nsICategoryManager);
    catman.deleteCategoryEntry("mailnews-accountmanager-extensions",
                               "@mozilla.org/accountmanager/extension;1?name=newsblog", true);
  },

  canUnload: function(aCompMgr)
  {
    return true;
  }
};

function NSGetModule(aCompMgr, aFileSpec)
{
  return nsNewsBlogFeedDownloaderModule;
}

function loadScripts()
{
  var scriptLoader =  Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
                     .createInstance(Components.interfaces.mozIJSSubScriptLoader);
  if (scriptLoader)
  { 
    scriptLoader.loadSubScript("chrome://messenger-newsblog/content/Feed.js");
    scriptLoader.loadSubScript("chrome://messenger-newsblog/content/FeedItem.js");
    scriptLoader.loadSubScript("chrome://messenger-newsblog/content/file-utils.js");
    scriptLoader.loadSubScript("chrome://messenger-newsblog/content/utils.js");
  }

  gExternalScriptsLoaded = true;
}
