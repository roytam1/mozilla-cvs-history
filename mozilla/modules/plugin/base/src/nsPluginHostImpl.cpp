/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *   Sean Echevarria <sean@beatnik.com>
 *   H�kan Waara <hwaara@chello.se>
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

/* nsPluginHostImpl.cpp - bulk of code for managing plugins */

/* XdpX Really really force prlogging*/
#define FORCE_PR_LOG /* Allow logging in the release build */
#define PR_LOGGING 1
#include "prlog.h"

#include "nscore.h"
#include "nsPluginHostImpl.h"
#include "nsPluginProxyImpl.h"
#include <stdio.h>
#include "prio.h"
#include "prmem.h"
#include "ns4xPlugin.h"
#include "nsPluginInstancePeer.h"
#include "nsIPlugin.h"
#include "nsIJVMPlugin.h"
#include "nsIJVMManager.h"
#include "nsIPluginStreamListener.h"
#include "nsIHTTPHeaderListener.h" 
#include "nsIObserverService.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIHttpChannel.h"
#include "nsIUploadChannel.h"
#include "nsIByteRangeRequest.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIURL.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIPref.h"
#include "nsIProtocolProxyService.h"
#include "nsIStreamConverterService.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsIFileStream.h" // for nsISeekableStream
#include "nsNetUtil.h"
#include "nsIProgressEventSink.h"
#include "nsIDocument.h"
#include "nsIScriptablePlugin.h"
#include "nsICachingChannel.h"
#include "nsHashtable.h"
#include "nsIProxyInfo.h"
#include "nsObsoleteModuleLoading.h"
#include "nsIComponentRegistrar.h"
#include "nsPluginLogging.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"

// Friggin' X11 has to "#define None". Lame!
#ifdef None
#undef None
#endif

#include "nsIRegistry.h"
#include "nsEnumeratorUtils.h"
#include "nsISupportsPrimitives.h"
// for the dialog
#include "nsIStringBundle.h"
#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"

#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIPrincipal.h"

#include "nsIServiceManager.h"
#include "nsICookieStorage.h"
#include "nsICookieService.h"
#include "nsIDOMPlugin.h"
#include "nsIDOMMimeType.h"
#include "nsMimeTypes.h"
#include "prprf.h"
#include "plevent.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"
#include "nsIInputStreamTee.h"

#if defined(XP_PC) && !defined(XP_OS2)
#include "windows.h"
#include "winbase.h"
#endif

#include "nsFileSpec.h"

#include "nsPluginDocLoaderFactory.h"
#include "nsIDocumentLoaderFactory.h"

#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"
#include "nsILocalFile.h"
#include "nsIFileChannel.h"

#include "nsPluginSafety.h"

#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"

#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsPluginDirServiceProvider.h"
#include "nsInt64.h"
#include "nsPluginError.h"

#include "nsUnicharUtils.h"

#include "imgILoader.h"
#include "nsDefaultPlugin.h"

#ifdef XP_UNIX
#if defined(MOZ_WIDGET_GTK)
#include <gdk/gdkx.h> // for GDK_DISPLAY()
#elif defined(MOZ_WIDGET_QT)
#include <qwindowdefs.h> // for qt_xdisplay()
#elif defined(MOZ_WIDGET_XLIB)
#include "xlibrgb.h" // for xlib_rgb_get_display()
#endif
#endif

#if defined(XP_MAC) && TARGET_CARBON
#include "nsIClassicPluginFactory.h"
#endif

#if defined(XP_MAC) && TARGET_CARBON
#include "nsIClassicPluginFactory.h"
#endif

// We need this hackery so that we can dynamically register doc
// loaders for the 4.x plugins that we discover.
#if defined(XP_PC)
#define PLUGIN_DLL "gkplugin.dll"
#elif defined(XP_UNIX) || defined(XP_BEOS)
#define PLUGIN_DLL "libgkplugin" MOZ_DLL_SUFFIX
#elif defined(XP_MAC)
#if defined(NS_DEBUG)
#define PLUGIN_DLL "pluginDebug.shlb"
#else
#define PLUGIN_DLL "plugin.shlb"
#endif // ifdef NS_DEBUG
#endif

#define REL_PLUGIN_DLL "rel:" PLUGIN_DLL

// this is the name of the directory which will be created
// to cache temporary files.
#define kPluginTmpDirName NS_LITERAL_CSTRING("plugtmp")

// Version of cached plugin info
// 0.01 first implementation
// 0.02 added caching of CanUnload to fix bug 105935
// 0.03 changed name, description and mime desc from string to bytes, bug 108246
// 0.04 added new mime entry point on Mac, bug 113464
static const char *kPluginInfoVersion = "0.04";
////////////////////////////////////////////////////////////////////////
// CID's && IID's
static NS_DEFINE_IID(kIPluginInstanceIID, NS_IPLUGININSTANCE_IID);
static NS_DEFINE_IID(kIPluginInstancePeerIID, NS_IPLUGININSTANCEPEER_IID); 
static NS_DEFINE_IID(kIPluginStreamInfoIID, NS_IPLUGINSTREAMINFO_IID);
static NS_DEFINE_CID(kPluginCID, NS_PLUGIN_CID);
static NS_DEFINE_IID(kIPluginTagInfo2IID, NS_IPLUGINTAGINFO2_IID); 
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_CID(kProtocolProxyServiceCID, NS_PROTOCOLPROXYSERVICE_CID);
static NS_DEFINE_CID(kCookieServiceCID, NS_COOKIESERVICE_CID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIRequestObserverIID, NS_IREQUESTOBSERVER_IID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kHttpHandlerCID, NS_HTTPPROTOCOLHANDLER_CID);
static NS_DEFINE_CID(kIHttpHeaderVisitorIID, NS_IHTTPHEADERVISITOR_IID);
static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);
static NS_DEFINE_IID(kIFileUtilitiesIID, NS_IFILEUTILITIES_IID);
static NS_DEFINE_IID(kIOutputStreamIID, NS_IOUTPUTSTREAM_IID);
static NS_DEFINE_CID(kRegistryCID, NS_REGISTRY_CID);
static const char kDirectoryServiceContractID[] = "@mozilla.org/file/directory_service;1";
// for the dialog
static NS_DEFINE_IID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kCPluginManagerCID, NS_PLUGINMANAGER_CID);

////////////////////////////////////////////////////////////////////////
// Registry keys for caching plugin info
static const char kPluginsRootKey[] = "software/plugins";
static const char kPluginsNameKey[] = "name";
static const char kPluginsDescKey[] = "description";
static const char kPluginsFilenameKey[] = "filename";
static const char kPluginsFullpathKey[] = "fullpath";
static const char kPluginsModTimeKey[] = "lastModTimeStamp";
static const char kPluginsCanUnload[] = "canUnload";
static const char kPluginsVersionKey[] = "version";
static const char kPluginsMimeTypeKey[] = "mimetype";
static const char kPluginsMimeDescKey[] = "description";
static const char kPluginsMimeExtKey[] = "extension";

#ifdef PLUGIN_LOGGING
PRLogModuleInfo* nsPluginLogging::gNPNLog = nsnull;
PRLogModuleInfo* nsPluginLogging::gNPPLog = nsnull;
PRLogModuleInfo* nsPluginLogging::gPluginLog = nsnull;
#endif

#define PLUGIN_PROPERTIES_URL "chrome://global/locale/downloadProgress.properties"
#define PLUGIN_REGIONAL_URL "chrome://global-region/locale/region.properties"

// #defines for reading prefs and extra search plugin paths from windows registry
#define _MAXKEYVALUE_ 8196
#define _NS_PREF_COMMON_PLUGIN_REG_KEY_ "browser.plugins.registry_plugins_folder_key_location"
#define _NS_COMMON_PLUGIN_KEY_NAME_ "Plugins Folders"

// #defines for plugin cache and prefs
#define NS_PREF_MAX_NUM_CACHED_PLUGINS "browser.plugins.max_num_cached_plugins"
#define DEFAULT_NUMBER_OF_STOPPED_PLUGINS 10

#define MAGIC_REQUEST_CONTEXT 0x01020304

void DisplayNoDefaultPluginDialog(const char *mimeType);
nsresult PostPluginUnloadEvent(PRLibrary * aLibrary);

/**
 * Used in DisplayNoDefaultPlugindialog to prevent showing the dialog twice
 * for the same mimetype.
 */

static nsHashtable *mimeTypesSeen = nsnull;

/**
 * placeholder value for mimeTypesSeen hashtable
 */

static const char *hashValue = "value";

/**
 * Default number of entries in the mimeTypesSeen hashtable
 */ 
#define NS_MIME_TYPES_HASH_NUM (20)


////////////////////////////////////////////////////////////////////////
void DisplayNoDefaultPluginDialog(const char *mimeType)
{
  nsresult rv;

  if (nsnull == mimeTypesSeen) {
    mimeTypesSeen = new nsHashtable(NS_MIME_TYPES_HASH_NUM);
  }
  if ((mimeTypesSeen != nsnull) && (mimeType != nsnull)) {
    nsCStringKey key(mimeType);
    // if we've seen this mimetype before
    if (mimeTypesSeen->Get(&key)) {
      // don't display the dialog
      return;
    }
    else {
      mimeTypesSeen->Put(&key, (void *) hashValue);
    }
  }

  nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
  nsCOMPtr<nsIPrompt> prompt;
  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
  if (wwatch)
    wwatch->GetNewPrompter(0, getter_AddRefs(prompt));

  nsCOMPtr<nsIIOService> io(do_GetService(kIOServiceCID));
  nsCOMPtr<nsIStringBundleService> strings(do_GetService(kStringBundleServiceCID));
  nsCOMPtr<nsIStringBundle> bundle;
  nsCOMPtr<nsIStringBundle> regionalBundle;
  nsCOMPtr<nsIURI> uri;
  PRBool displayDialogPrefValue = PR_FALSE, checkboxState = PR_FALSE;

  if (!prefs || !prompt || !io || !strings) {
    return;
  }

  rv = prefs->GetBoolPref("plugin.display_plugin_downloader_dialog", 
                          &displayDialogPrefValue);
  if (NS_SUCCEEDED(rv)) {
    // if the pref is false, don't display the dialog
    if (!displayDialogPrefValue) {
      return;
    }
  }
  
  // Taken from mozilla\extensions\wallet\src\wallet.cpp
  // WalletLocalize().
  rv = strings->CreateBundle(PLUGIN_PROPERTIES_URL, getter_AddRefs(bundle));
  if (NS_FAILED(rv)) {
    return;
  }
  rv = strings->CreateBundle(PLUGIN_REGIONAL_URL, 
                             getter_AddRefs(regionalBundle));
  if (NS_FAILED(rv)) {
    return;
  }

  PRUnichar *titleUni = nsnull;
  PRUnichar *messageUni = nsnull;
  PRUnichar *checkboxMessageUni = nsnull;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("noDefaultPluginTitle").get(), 
                                 &titleUni);
  if (NS_FAILED(rv)) {
    goto EXIT_DNDPD;
  }
  rv = regionalBundle->GetStringFromName(NS_LITERAL_STRING("noDefaultPluginMessage").get(), 
                                         &messageUni);
  if (NS_FAILED(rv)) {
    goto EXIT_DNDPD;
  }
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("noDefaultPluginCheckboxMessage").get(), 
                                 &checkboxMessageUni);
  if (NS_FAILED(rv)) {
    goto EXIT_DNDPD;
  }

  PRInt32 buttonPressed;
  rv = prompt->ConfirmEx(titleUni, messageUni,
                         nsIPrompt::BUTTON_TITLE_OK * nsIPrompt::BUTTON_POS_0,
                         nsnull, nsnull, nsnull,
                         checkboxMessageUni, &checkboxState, &buttonPressed);

  // if the user checked the checkbox, make it so the dialog doesn't
  // display again.
  if (checkboxState) {
    prefs->SetBoolPref("plugin.display_plugin_downloader_dialog",
                       !checkboxState);
  }
 EXIT_DNDPD:
  nsMemory::Free((void *)titleUni);
  nsMemory::Free((void *)messageUni);
  nsMemory::Free((void *)checkboxMessageUni);
  return;
}


////////////////////////////////////////////////////////////////////////
nsActivePlugin::nsActivePlugin(nsPluginTag* aPluginTag,
                               nsIPluginInstance* aInstance, 
                               const char * url,
                               PRBool aDefaultPlugin)
{
  mNext = nsnull;
  mPeer = nsnull;
  mPluginTag = aPluginTag;

  mURL = PL_strdup(url);
  mInstance = aInstance;
  if(aInstance != nsnull)
  {
    aInstance->GetPeer(&mPeer);
    NS_ADDREF(aInstance);
  }
  mXPConnected = PR_FALSE;
  mDefaultPlugin = aDefaultPlugin;
  mStopped = PR_FALSE;
  mllStopTime = LL_ZERO;
  mStreams = new nsVoidArray();   // create a new stream array
}


////////////////////////////////////////////////////////////////////////
nsActivePlugin::~nsActivePlugin()
{
  mPluginTag = nsnull;

  // free our streams array
  if (mStreams)
  {
    delete mStreams;
    mStreams = nsnull;
  }

  if(mInstance != nsnull)
  {
    if(mPeer)
    {
      nsresult rv = NS_OK;
      nsPluginInstancePeerImpl * peer = (nsPluginInstancePeerImpl *)mPeer;
      nsCOMPtr<nsIPluginInstanceOwner> owner;
      rv = peer->GetOwner(*getter_AddRefs(owner));
      owner->SetInstance(nsnull);
    }

    // now check for cached plugins because they haven't had nsIPluginInstance::Destroy()
    // called yet. For non-cached plugins, nsIPluginInstance::Destroy() is called 
    // in either nsObjectFrame::Destroy() or nsActivePluginList::stopRunning()
    PRBool doCache = PR_TRUE;    
    mInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
    if (doCache) 
      mInstance->Destroy();

    NS_RELEASE(mInstance);
    NS_RELEASE(mPeer);
  }
  PL_strfree(mURL);
}


////////////////////////////////////////////////////////////////////////
void nsActivePlugin::setStopped(PRBool stopped)
{
  mStopped = stopped;
  if(mStopped) // plugin instance is told to stop
  {
    mllStopTime = PR_Now();

    // Since we are "stopped", we should clear our our streams array
    if (mStreams)
      while (mStreams->Count() > 0 )  // simple loop, always removing the first element
      {
        nsIStreamListener * s = (nsIStreamListener *) mStreams->ElementAt(0);  // XXX nasty cast!
        if (s)
        {
          NS_RELEASE(s);    // should cause stream destructions (and prevent leaks!)
          mStreams->RemoveElementAt(0);
        }
      }
  }
  else
    mllStopTime = LL_ZERO;

}


////////////////////////////////////////////////////////////////////////
nsActivePluginList::nsActivePluginList()
{
  mFirst = nsnull;
  mLast = nsnull;
  mCount = 0;
}


////////////////////////////////////////////////////////////////////////
nsActivePluginList::~nsActivePluginList()
{
  if(mFirst == nsnull)
    return;
  shut();
}


////////////////////////////////////////////////////////////////////////
void nsActivePluginList::shut()
{
  if(mFirst == nsnull)
    return;

  for(nsActivePlugin * plugin = mFirst; plugin != nsnull;)
  {
    nsActivePlugin * next = plugin->mNext;

    PRBool unloadLibLater = PR_FALSE;
    remove(plugin, &unloadLibLater);
    NS_ASSERTION(!unloadLibLater, "Plugin doesn't want to be unloaded");
    
    plugin = next;
  }
  mFirst = nsnull;
  mLast = nsnull;
}


////////////////////////////////////////////////////////////////////////
PRInt32 nsActivePluginList::add(nsActivePlugin * plugin)
{
  if (mFirst == nsnull)
  {
    mFirst = plugin;
    mLast = plugin;
    mFirst->mNext = nsnull;
  }
  else
  {
    mLast->mNext = plugin;
    mLast = plugin;
  }
  mLast->mNext = nsnull;
  mCount++;
  return mCount;
}


////////////////////////////////////////////////////////////////////////
PRBool nsActivePluginList::IsLastInstance(nsActivePlugin * plugin)
{
  if(!plugin)
    return PR_FALSE;

  if(!plugin->mPluginTag)
    return PR_FALSE;

  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if((p->mPluginTag == plugin->mPluginTag) && (p != plugin))
      return PR_FALSE;
  }
  return PR_TRUE;
}


////////////////////////////////////////////////////////////////////////
PRBool nsActivePluginList::remove(nsActivePlugin * plugin, PRBool * aUnloadLibraryLater)
{
  if(mFirst == nsnull)
    return PR_FALSE;

  nsActivePlugin * prev = nsnull;
  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if(p == plugin)
    {
      PRBool lastInstance = IsLastInstance(p);

      if(p == mFirst)
        mFirst = p->mNext;
      else
        prev->mNext = p->mNext;

      if((prev != nsnull) && (prev->mNext == nsnull))
        mLast = prev;

      // see if this is going to be the last instance of a plugin
      // if so we should perform nsIPlugin::Shutdown and unload the library
      // by calling nsPluginTag::TryUnloadPlugin()
      if(lastInstance)
      {
        // cache some things as we are going to destroy it right now
        nsPluginTag *pluginTag = p->mPluginTag;
        
        delete p; // plugin instance is destroyed here
        
        if(pluginTag)
        {
          // xpconnected plugins from the old world should postpone unloading library 
          // to avoid crash check, if so add library to the list of unloaded libraries
          if(pluginTag->mXPConnected && (pluginTag->mFlags & NS_PLUGIN_FLAG_OLDSCHOOL))
          {
            pluginTag->mCanUnloadLibrary = PR_FALSE;

            if(aUnloadLibraryLater)
              *aUnloadLibraryLater = PR_TRUE;
          }
        
          pluginTag->TryUnloadPlugin();
        }
        else
          NS_ASSERTION(pluginTag, "pluginTag was not set, plugin not shutdown");

      }
      else
        delete p;

      mCount--;
      return PR_TRUE;
    }
    prev = p;
  }
  return PR_FALSE;
}


////////////////////////////////////////////////////////////////////////
void nsActivePluginList::stopRunning()
{
  if(mFirst == nsnull)
    return;

  PRBool doCallSetWindowAfterDestroy = PR_FALSE;

  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if(!p->mStopped && p->mInstance)
    {
      // then determine if the plugin wants Destroy to be called after
      // Set Window.  This is for bug 50547.
      p->mInstance->GetValue(nsPluginInstanceVariable_CallSetWindowAfterDestroyBool, 
                             (void *) &doCallSetWindowAfterDestroy);
      if (doCallSetWindowAfterDestroy) {
        p->mInstance->Stop();
        p->mInstance->Destroy();
        p->mInstance->SetWindow(nsnull);
      }
      else {
        p->mInstance->SetWindow(nsnull);
        p->mInstance->Stop();
        p->mInstance->Destroy();
      }
      doCallSetWindowAfterDestroy = PR_FALSE;
      p->setStopped(PR_TRUE);
    }
  }
}


////////////////////////////////////////////////////////////////////////
void nsActivePluginList::removeAllStopped()
{
  if(mFirst == nsnull)
    return;

  nsActivePlugin * next = nsnull;

  for(nsActivePlugin * p = mFirst; p != nsnull;)
  {
    next = p->mNext;

    if(p->mStopped)
    {
      // we don't care about unloading library problem for 
      // plugins that are already in the 'stop' state
      PRBool unloadLibLater = PR_FALSE;
      remove(p, &unloadLibLater);
    }

    p = next;
  }
  return;
}


////////////////////////////////////////////////////////////////////////
nsActivePlugin * nsActivePluginList::find(nsIPluginInstance* instance)
{
  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if(p->mInstance == instance)
    {
#ifdef NS_DEBUG
      PRBool doCache = PR_TRUE;
      p->mInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
      NS_ASSERTION(!p->mStopped || doCache, "This plugin is not supposed to be cached!");
#endif
      return p;
    }
  }
  return nsnull;
}

nsActivePlugin * nsActivePluginList::find(const char * mimetype)
{
  PRBool defaultplugin = (PL_strcmp(mimetype, "*") == 0);

  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    // give it some special treatment for the default plugin first
    // because we cannot tell the default plugin by asking peer for a mime type
    if(defaultplugin && p->mDefaultPlugin)
      return p;

    if(!p->mPeer)
      continue;

    nsMIMEType mt;

    nsresult res = p->mPeer->GetMIMEType(&mt);

    if(NS_FAILED(res))
      continue;

    if(PL_strcasecmp(mt, mimetype) == 0)
    {
#ifdef NS_DEBUG
      PRBool doCache = PR_TRUE;
      p->mInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
      NS_ASSERTION(!p->mStopped || doCache, "This plugin is not supposed to be cached!");
#endif
       return p;
    }
  }
  return nsnull;
}


////////////////////////////////////////////////////////////////////////
nsActivePlugin * nsActivePluginList::findStopped(const char * url)
{
  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if(!PL_strcmp(url, p->mURL) && p->mStopped)
    {
#ifdef NS_DEBUG
      PRBool doCache = PR_TRUE;
      p->mInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
      NS_ASSERTION(doCache, "This plugin is not supposed to be cached!");
#endif
       return p;
    }
  }
  return nsnull;
}


////////////////////////////////////////////////////////////////////////
PRUint32 nsActivePluginList::getStoppedCount()
{
  PRUint32 stoppedCount = 0;
  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if(p->mStopped)
      stoppedCount++;
  }
  return stoppedCount;
}


////////////////////////////////////////////////////////////////////////
nsActivePlugin * nsActivePluginList::findOldestStopped()
{
  nsActivePlugin * res = nsnull;
  PRInt64 llTime = LL_MAXINT;
  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if(!p->mStopped)
      continue;

    if(LL_CMP(p->mllStopTime, <, llTime))
    {
      llTime = p->mllStopTime;
      res = p;
    }
  }

#ifdef NS_DEBUG
  if(res)
  {
    PRBool doCache = PR_TRUE;
    res->mInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
    NS_ASSERTION(doCache, "This plugin is not supposed to be cached!");
  }
#endif

  return res;
}


////////////////////////////////////////////////////////////////////////
nsUnusedLibrary::nsUnusedLibrary(PRLibrary * aLibrary)
{
  mLibrary = aLibrary;
}


////////////////////////////////////////////////////////////////////////
nsUnusedLibrary::~nsUnusedLibrary()
{
  if(mLibrary)
    PostPluginUnloadEvent(mLibrary);
}


////////////////////////////////////////////////////////////////////////
nsPluginTag::nsPluginTag()
{
  mNext = nsnull;
  mName = nsnull;
  mDescription = nsnull;
  mVariants = 0;
  mMimeTypeArray = nsnull;
  mMimeDescriptionArray = nsnull;
  mExtensionsArray = nsnull;
  mLibrary = nsnull;
  mCanUnloadLibrary = PR_TRUE;
  mEntryPoint = nsnull;
  mFlags = NS_PLUGIN_FLAG_ENABLED;
  mXPConnected = PR_FALSE;
  mFileName = nsnull;
  mFullPath = nsnull;
}


////////////////////////////////////////////////////////////////////////
inline char* new_str(const char* str)
{
  if(str == nsnull)
    return nsnull;

  char* result = new char[strlen(str) + 1];
  if (result != nsnull)
    return strcpy(result, str);
  return result;
}


////////////////////////////////////////////////////////////////////////
nsPluginTag::nsPluginTag(nsPluginTag* aPluginTag)
{
  mNext = nsnull;
  mName = new_str(aPluginTag->mName);
  mDescription = new_str(aPluginTag->mDescription);
  mVariants = aPluginTag->mVariants;

  mMimeTypeArray = nsnull;
  mMimeDescriptionArray = nsnull;
  mExtensionsArray = nsnull;

  if(aPluginTag->mMimeTypeArray != nsnull)
  {
    mMimeTypeArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++)
      mMimeTypeArray[i] = new_str(aPluginTag->mMimeTypeArray[i]);
  }

  if(aPluginTag->mMimeDescriptionArray != nsnull) 
  {
    mMimeDescriptionArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++)
      mMimeDescriptionArray[i] = new_str(aPluginTag->mMimeDescriptionArray[i]);
  }

  if(aPluginTag->mExtensionsArray != nsnull) 
  {
    mExtensionsArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++)
      mExtensionsArray[i] = new_str(aPluginTag->mExtensionsArray[i]);
  }

  mLibrary = nsnull;
  mCanUnloadLibrary = PR_TRUE;
  mEntryPoint = nsnull;
  mFlags = NS_PLUGIN_FLAG_ENABLED;
  mXPConnected = PR_FALSE;
  mFileName = new_str(aPluginTag->mFileName);
  mFullPath = new_str(aPluginTag->mFullPath);
  printf("mFullPath = %s\n", mFullPath);
}


////////////////////////////////////////////////////////////////////////
nsPluginTag::nsPluginTag(nsPluginInfo* aPluginInfo)
{
  mNext = nsnull;
  mName = new_str(aPluginInfo->fName);
  mDescription = new_str(aPluginInfo->fDescription);
  mVariants = aPluginInfo->fVariantCount;

  mMimeTypeArray = nsnull;
  mMimeDescriptionArray = nsnull;
  mExtensionsArray = nsnull;

  if(aPluginInfo->fMimeTypeArray != nsnull)
  {
    mMimeTypeArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++)
      mMimeTypeArray[i] = new_str(aPluginInfo->fMimeTypeArray[i]);
  }

  if(aPluginInfo->fMimeDescriptionArray != nsnull) 
  {
    mMimeDescriptionArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++)
      mMimeDescriptionArray[i] = new_str(aPluginInfo->fMimeDescriptionArray[i]);
  }

  if(aPluginInfo->fExtensionArray != nsnull) 
  {
    mExtensionsArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++)
      mExtensionsArray[i] = new_str(aPluginInfo->fExtensionArray[i]);
  }

  mFileName = new_str(aPluginInfo->fFileName);
  mFullPath = new_str(aPluginInfo->fFullPath);

  mLibrary = nsnull;
  mCanUnloadLibrary = PR_TRUE;
  mEntryPoint = nsnull;

#if TARGET_CARBON
  mCanUnloadLibrary = !aPluginInfo->fBundle;
#endif
  mFlags = NS_PLUGIN_FLAG_ENABLED;
  mXPConnected = PR_FALSE;
}



////////////////////////////////////////////////////////////////////////
nsPluginTag::nsPluginTag(const char* aName,
                         const char* aDescription,
                         const char* aFileName,
                         const char* aFullPath,
                         const char* const* aMimeTypes,
                         const char* const* aMimeDescriptions,
                         const char* const* aExtensions,
                         PRInt32 aVariants,
                         PRInt64 aLastModifiedTime,
                         PRBool aCanUnload)
  : mNext(nsnull),
    mVariants(aVariants),
    mMimeTypeArray(nsnull),
    mMimeDescriptionArray(nsnull),
    mExtensionsArray(nsnull),
    mLibrary(nsnull),
    mCanUnloadLibrary(aCanUnload),
    mEntryPoint(nsnull),
    mFlags(0),
    mXPConnected(PR_FALSE),
    mLastModifiedTime(aLastModifiedTime)
{
  mName            = new_str(aName);
  mDescription     = new_str(aDescription);
  mFileName        = new_str(aFileName);
  mFullPath        = new_str(aFullPath);

  if (mVariants) {
    mMimeTypeArray        = new char*[mVariants];
    mMimeDescriptionArray = new char*[mVariants];
    mExtensionsArray      = new char*[mVariants];

    for (PRInt32 i = 0; i < aVariants; ++i) {
      mMimeTypeArray[i]        = new_str(aMimeTypes[i]);
      mMimeDescriptionArray[i] = new_str(aMimeDescriptions[i]);
      mExtensionsArray[i]      = new_str(aExtensions[i]);
    }
  }
}

nsPluginTag::~nsPluginTag()
{
  TryUnloadPlugin(PR_TRUE);

  if (nsnull != mName) {
    delete[] (mName);
    mName = nsnull;
  }

  if (nsnull != mDescription) {
    delete[] (mDescription);
    mDescription = nsnull;
  }

  if (nsnull != mMimeTypeArray) {
    for (int i = 0; i < mVariants; i++)
      delete[] mMimeTypeArray[i];

    delete[] (mMimeTypeArray);
    mMimeTypeArray = nsnull;
  }

  if (nsnull != mMimeDescriptionArray) {
    for (int i = 0; i < mVariants; i++)
      delete[] mMimeDescriptionArray[i];

    delete[] (mMimeDescriptionArray);
    mMimeDescriptionArray = nsnull;
  }

  if (nsnull != mExtensionsArray) {
    for (int i = 0; i < mVariants; i++)
      delete[] mExtensionsArray[i];

    delete[] (mExtensionsArray);
    mExtensionsArray = nsnull;
  }

  if(nsnull != mFileName)
  {
    delete [] mFileName;
    mFileName = nsnull;
  }

  if(nsnull != mFullPath)
  {
    delete [] mFullPath;
    mFullPath = nsnull;
  }

}

//----------------------------------------------------------------------
// helper struct for asynchronous handeling of plugin unloading
struct nsPluginUnloadEvent: public PLEvent {
  nsPluginUnloadEvent (PRLibrary* aLibrary);
 
  void HandleEvent() {
    if (mLibrary)
      NS_TRY_SAFE_CALL_VOID(PR_UnloadLibrary(mLibrary), nsnull);  // put our unload call in a saftey wrapper
    else 
      NS_WARNING("missing library from nsPluginUnloadEvent");
  }

  PRLibrary* mLibrary;
};
nsPluginUnloadEvent::nsPluginUnloadEvent (PRLibrary* aLibrary)
{
  mLibrary = aLibrary;
}
//----------------------------------------------------------------------
// helper static callback functions for plugin unloading PLEvents
static void PR_CALLBACK HandlePluginUnloadPLEvent(nsPluginUnloadEvent* aEvent)
{
  aEvent->HandleEvent();
}
static void PR_CALLBACK DestroyPluginUnloadPLEvent(nsPluginUnloadEvent* aEvent)
{
  delete aEvent;
}

// unload plugin asynchronously if possible, otherwise just unload now
nsresult PostPluginUnloadEvent (PRLibrary* aLibrary)
{
  nsCOMPtr<nsIEventQueueService> eventService(do_GetService(kEventQueueServiceCID));
  if (eventService) {
    nsCOMPtr<nsIEventQueue> eventQueue;  
    eventService->GetThreadEventQueue(PR_GetCurrentThread(), getter_AddRefs(eventQueue));
    if (eventQueue) {
      nsPluginUnloadEvent * ev = new nsPluginUnloadEvent(aLibrary);
      if (ev) {

        PL_InitEvent(ev, nsnull, (PLHandleEventProc) ::HandlePluginUnloadPLEvent, (PLDestroyEventProc) ::DestroyPluginUnloadPLEvent);
        if (NS_SUCCEEDED(eventQueue->PostEvent(ev)))
          return NS_OK;        
        else NS_WARNING("failed to post event onto queue");

      } else NS_WARNING("not able to create plugin unload event");
    } else NS_WARNING("couldn't get event queue");
  } else NS_WARNING("couldn't get event queue service");

  // failure case
  NS_TRY_SAFE_CALL_VOID(PR_UnloadLibrary(aLibrary), nsnull);

  return NS_ERROR_FAILURE;
}


////////////////////////////////////////////////////////////////////////
void nsPluginTag::TryUnloadPlugin(PRBool aForceShutdown)
{
  PRBool isXPCOM = PR_FALSE;
  if (!(mFlags & NS_PLUGIN_FLAG_OLDSCHOOL))
    isXPCOM = PR_TRUE;

  if (isXPCOM && !aForceShutdown) return;

  if (mEntryPoint)
  {
    mEntryPoint->Shutdown();
    mEntryPoint->Release();
    mEntryPoint = nsnull;
  }

  // before we unload check if we are allowed to, see bug #61388
  // also, never unload an XPCOM plugin library
  if (mLibrary && mCanUnloadLibrary && !isXPCOM)
    PostPluginUnloadEvent(mLibrary); // unload the plugin asynchronously by posting a PLEvent 

  // we should zero it anyway, it is going to be unloaded by 
  // CleanUnsedLibraries before we need to call the library 
  // again so the calling code should not be fooled and reload 
  // the library fresh
  mLibrary = nsnull;
}


////////////////////////////////////////////////////////////////////////
PRBool nsPluginTag::Equals(nsPluginTag *aPluginTag)
{
  NS_ENSURE_TRUE(aPluginTag, PR_FALSE);

  if ( (PL_strcmp(mName, aPluginTag->mName) != 0) ||
       (PL_strcmp(mDescription, aPluginTag->mDescription) != 0) ||
       (mVariants != aPluginTag->mVariants) )
    return PR_FALSE;

  if (0 != mVariants && mMimeTypeArray && aPluginTag->mMimeTypeArray)
    for (PRInt32 i = 0; i < mVariants; i++)
      if (PL_strcmp(mMimeTypeArray[i], aPluginTag->mMimeTypeArray[i]) != 0)
        return PR_FALSE;

  return PR_TRUE;
}

////////////////////////////////////////////////////////////////////////
class nsPluginStreamListenerPeer;

class nsPluginStreamInfo : public nsIPluginStreamInfo
{
public:
  nsPluginStreamInfo();
  virtual ~nsPluginStreamInfo();
 
  NS_DECL_ISUPPORTS

  // nsIPluginStreamInfo interface

  NS_IMETHOD
  GetContentType(nsMIMEType* result);

  NS_IMETHOD
  IsSeekable(PRBool* result);

  NS_IMETHOD
  GetLength(PRUint32* result);

  NS_IMETHOD
  GetLastModified(PRUint32* result);

  NS_IMETHOD
  GetURL(const char** result);

  NS_IMETHOD
  RequestRead(nsByteRange* rangeList);
    
  NS_IMETHOD
  GetStreamOffset(PRInt32 *result);

  NS_IMETHOD
  SetStreamOffset(PRInt32 result);

  // local methods

  void
  SetContentType(const nsMIMEType contentType);

  void
  SetSeekable(const PRBool seekable);

  void
  SetLength(const PRUint32 length);

  void
  SetLastModified(const PRUint32 modified);

  void
  SetURL(const char* url);

  void
  SetPluginInstance(nsIPluginInstance * aPluginInstance);

  void
  SetPluginStreamListenerPeer(nsPluginStreamListenerPeer * aPluginStreamListenerPeer);

  void
  MakeByteRangeString(nsByteRange* aRangeList, nsACString &string, PRInt32 *numRequests);

  void
  SetLocalCachedFile(nsIFile *file);

  void
  GetLocalCachedFile(nsIFile** file);

  void
  SetLocalCachedFileStream(nsIOutputStream *stream);

  void
  GetLocalCachedFileStream(nsIOutputStream **stream);

private:

  char* mContentType;
  char* mURL;
  nsCOMPtr<nsIFile> mCachedFile;
  PRBool mSeekable;
  PRUint32 mLength;
  PRUint32 mModified;
  nsIPluginInstance * mPluginInstance;
  nsPluginStreamListenerPeer * mPluginStreamListenerPeer;
  nsCOMPtr<nsIOutputStream> mFileCacheOutputStream;
  PRBool mLocallyCached;
  PRInt32 mStreamOffset;
};


///////////////////////////////////////////////////////////////////////////////////////////////////

class nsPluginStreamListenerPeer : public nsIStreamListener,
                                   public nsIProgressEventSink,
                                   public nsIHttpHeaderVisitor
{
public:
  nsPluginStreamListenerPeer();
  virtual ~nsPluginStreamListenerPeer();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROGRESSEVENTSINK
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIHTTPHEADERVISITOR

  // Called by GetURL and PostURL (via NewStream)
  nsresult Initialize(nsIURI *aURL, 
                      nsIPluginInstance *aInstance, 
                      nsIPluginStreamListener *aListener,
                      PRInt32 requestCount = 1);

  nsresult InitializeEmbeded(nsIURI *aURL, 
                             nsIPluginInstance* aInstance, 
                             nsIPluginInstanceOwner *aOwner = nsnull,
                             nsIPluginHost *aHost = nsnull);

  nsresult InitializeFullPage(nsIPluginInstance *aInstance);

  nsresult OnFileAvailable(nsIFile* aFile);

  nsILoadGroup* GetLoadGroup();

  nsresult SetLocalFile(nsIFile* aFile);

  nsresult ServeStreamAsFile(nsIRequest *request, nsISupports *ctxt,
                              nsresult status);

private:
  nsresult SetUpCache(nsIURI* aURL); // todo: see about removing this...
  nsresult SetUpStreamListener(nsIRequest* request, nsIURI* aURL);
  nsresult SetupPluginCacheFile(nsIChannel* channel);

  nsIURI                  *mURL;

  
  nsIPluginInstanceOwner  *mOwner;
  nsIPluginInstance       *mInstance;
  nsIPluginStreamListener *mPStreamListener;
  nsPluginStreamInfo      *mPluginStreamInfo;
  PRPackedBool             mSetUpListener;

  // Set to PR_TRUE if we request failed (like with a HTTP response of 404)
  PRPackedBool            mRequestFailed;

  /*
   * Set to PR_TRUE after nsIPluginInstancePeer::OnStartBinding() has
   * been called.  Checked in ::OnStopRequest so we can call the
   * plugin's OnStartBinding if, for some reason, it has not already
   * been called.
   */
  PRPackedBool      mStartBinding;
  PRPackedBool      mHaveFiredOnStartRequest;
  // these get passed to the plugin stream listener
  char                    *mMIMEType;
  PRUint32                mLength;
  nsPluginStreamType      mStreamType;
  nsIPluginHost           *mHost;

  // local file which was used to post data and which should be deleted after that
  nsCOMPtr<nsIFile>        mLocalFile;
  nsHashtable             *mDataForwardToRequest;

public:
  PRBool                  mAbort;
  PRInt32                 mPendingRequests;

};

////////////////////////////////////////////////////////////////////////
class nsPluginByteRangeStreamListener : public nsIStreamListener {
public:
  nsPluginByteRangeStreamListener(nsPluginStreamListenerPeer* aPluginStreamListenerPeer);
  virtual ~nsPluginByteRangeStreamListener();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIRequestObserver methods:
  NS_DECL_NSIREQUESTOBSERVER

  // nsIStreamListener methods:
  NS_DECL_NSISTREAMLISTENER

private:
  nsCOMPtr<nsIStreamListener> mStreamConverter;
  nsPluginStreamListenerPeer* mFinalPluginStreamListener;
};

////////////////////////////////////////////////////////////////////////
nsPluginStreamInfo::nsPluginStreamInfo()
{
  NS_INIT_REFCNT();

  mPluginInstance = nsnull;
  mPluginStreamListenerPeer = nsnull;

  mContentType = nsnull;
  mURL = nsnull;
  mSeekable = PR_FALSE;
  mLength = 0;
  mModified = 0;
  mLocallyCached = PR_FALSE;
  mStreamOffset = 0;
}


////////////////////////////////////////////////////////////////////////
nsPluginStreamInfo::~nsPluginStreamInfo()
{
  if(mContentType != nsnull)
  PL_strfree(mContentType);
  if(mURL != nsnull)
    PL_strfree(mURL);

  // close FD of mFileCacheOutputStream if it's still open
  // or we won't be able to remove the cache file
  if (mFileCacheOutputStream)
    mFileCacheOutputStream->Close();

  // ONLY delete our cached file if we created it
  if(mLocallyCached && mCachedFile)
  {
     mCachedFile->Remove(PR_FALSE);
  }

  NS_IF_RELEASE(mPluginInstance);
}

////////////////////////////////////////////////////////////////////////
NS_IMPL_ADDREF(nsPluginStreamInfo)
NS_IMPL_RELEASE(nsPluginStreamInfo)
////////////////////////////////////////////////////////////////////////

nsresult nsPluginStreamInfo::QueryInterface(const nsIID& aIID,
                                            void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");

  if (nsnull == aInstancePtrResult)
    return NS_ERROR_NULL_POINTER;

  if (aIID.Equals(kIPluginStreamInfoIID))
  {
    *aInstancePtrResult = (void *)((nsIPluginStreamInfo *)this);
    AddRef();
    return NS_OK;
  }

  if (aIID.Equals(kISupportsIID))
  {
    *aInstancePtrResult = (void *)((nsISupports *)((nsIStreamListener *)this));
    AddRef();
    return NS_OK;
  }

  return NS_NOINTERFACE;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginStreamInfo::GetContentType(nsMIMEType* result)
{
  *result = mContentType;
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginStreamInfo::IsSeekable(PRBool* result)
{
  *result = mSeekable;
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginStreamInfo::GetLength(PRUint32* result)
{
  *result = mLength;
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginStreamInfo::GetLastModified(PRUint32* result)
{
  *result = mModified;
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginStreamInfo::GetURL(const char** result)
{
  *result = mURL;
  return NS_OK;
}



////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::MakeByteRangeString(nsByteRange* aRangeList, nsACString &rangeRequest, PRInt32 *numRequests)
{
  rangeRequest.Truncate();
  *numRequests  = 0;
  //the string should look like this: bytes=500-700,601-999
  if(!aRangeList)
    return;

  PRInt32 requestCnt = 0;
  nsCAutoString string("bytes=");

  for(nsByteRange * range = aRangeList; range != nsnull; range = range->next)
  {
    // XXX zero length?
    if(!range->length)
      continue;

    // XXX needs to be fixed for negative offsets
    string.AppendInt(range->offset);
    string.Append("-");
    string.AppendInt(range->offset + range->length - 1);
    if(range->next)
      string += ",";
    
    requestCnt++;
  }

  // get rid of possible trailing comma
  string.Trim(",", PR_FALSE);

  rangeRequest = string;
  *numRequests  = requestCnt;
  return;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginStreamInfo::RequestRead(nsByteRange* rangeList)
{    
  nsCAutoString rangeString;
  PRInt32 numRequests;

  MakeByteRangeString(rangeList, rangeString, &numRequests);
  
  if(numRequests == 0)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIURI> url;

  rv = NS_NewURI(getter_AddRefs(url), nsDependentCString(mURL));

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), url, nsnull, nsnull, nsnull);
  if (NS_FAILED(rv)) 
    return rv;

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if(!httpChannel)
    return NS_ERROR_FAILURE;


  httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Range"), rangeString);
  

  mPluginStreamListenerPeer->mAbort = PR_TRUE; // instruct old stream listener to cancel
                                               // the request on the next ODA. 

  nsCOMPtr<nsIStreamListener> converter;
  
  if (numRequests == 1) {
      converter = mPluginStreamListenerPeer;

    // set current stream offset equal to the first offset in the range list
    // it will work for single byte range request
    // for multy range we'll reset it in ODA 
    SetStreamOffset(rangeList->offset);
  } else {
    nsPluginByteRangeStreamListener *brrListener = 
      new nsPluginByteRangeStreamListener(mPluginStreamListenerPeer);
    if (brrListener)
      converter = brrListener;
    else
      return NS_ERROR_OUT_OF_MEMORY;
  }

  mPluginStreamListenerPeer->mPendingRequests += numRequests;

   nsCOMPtr<nsISupportsPRUint32> container = do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
   if (NS_FAILED(rv)) return rv;
   rv = container->SetData(MAGIC_REQUEST_CONTEXT);
   if (NS_FAILED(rv)) return rv;

   return channel->AsyncOpen(converter, container);
}

NS_IMETHODIMP
nsPluginStreamInfo::GetStreamOffset(PRInt32 *result)
{
  *result = mStreamOffset;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginStreamInfo::SetStreamOffset(PRInt32 offset)
{ 
  mStreamOffset = offset;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::SetContentType(const nsMIMEType contentType)
{ 
  if(mContentType != nsnull)
    PL_strfree(mContentType);

  mContentType = PL_strdup(contentType);
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::SetSeekable(const PRBool seekable)
{
  mSeekable = seekable;
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::SetLength(const PRUint32 length)
{
  mLength = length;
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::SetLastModified(const PRUint32 modified)
{
  mModified = modified;
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::SetURL(const char* url)
{ 
  if(mURL != nsnull)
    PL_strfree(mURL);

  mURL = PL_strdup(url);
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::SetLocalCachedFile(nsIFile* file)
{ 
  file->Clone(getter_AddRefs(mCachedFile));
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::GetLocalCachedFile(nsIFile** file)
{ 
  *file = mCachedFile;
  NS_IF_ADDREF(*file);
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::SetLocalCachedFileStream(nsIOutputStream *stream) 
{
     mFileCacheOutputStream = stream;
     mLocallyCached = PR_TRUE;
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::GetLocalCachedFileStream(nsIOutputStream **stream) 
{
    if (!stream) return;
    NS_IF_ADDREF(*stream = mFileCacheOutputStream);
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::SetPluginInstance(nsIPluginInstance * aPluginInstance)
{
    NS_IF_ADDREF(mPluginInstance = aPluginInstance);
}


////////////////////////////////////////////////////////////////////////
void
nsPluginStreamInfo::SetPluginStreamListenerPeer(nsPluginStreamListenerPeer * aPluginStreamListenerPeer)
{
    // not addref'd - nsPluginStreamInfo is owned by mPluginStreamListenerPeer
    mPluginStreamListenerPeer = aPluginStreamListenerPeer;
}


///////////////////////////////////////////////////////////////////////////////////////////////////

class nsPluginCacheListener : public nsIStreamListener
{
public:
  nsPluginCacheListener(nsPluginStreamListenerPeer* aListener);
  virtual ~nsPluginCacheListener();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

private:
  nsPluginStreamListenerPeer* mListener;
};


////////////////////////////////////////////////////////////////////////
nsPluginCacheListener::nsPluginCacheListener(nsPluginStreamListenerPeer* aListener)
{
  NS_INIT_REFCNT();

  mListener = aListener;
  NS_ADDREF(mListener);
}


////////////////////////////////////////////////////////////////////////
nsPluginCacheListener::~nsPluginCacheListener()
{
  NS_IF_RELEASE(mListener);
}


////////////////////////////////////////////////////////////////////////
NS_IMPL_ISUPPORTS1(nsPluginCacheListener, nsIStreamListener)
////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginCacheListener::OnStartRequest(nsIRequest *request, nsISupports* ctxt)
{
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP 
nsPluginCacheListener::OnDataAvailable(nsIRequest *request, nsISupports* ctxt, 
                                       nsIInputStream* aIStream, 
                                       PRUint32 sourceOffset, 
                                       PRUint32 aLength)
{

  PRUint32 readlen;
  char* buffer = (char*) PR_Malloc(aLength);

  // if we don't read from the stream, OnStopRequest will never be called
  if(!buffer)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = aIStream->Read(buffer, aLength, &readlen);

  NS_ASSERTION(aLength == readlen, "nsCacheListener->OnDataAvailable: "
               "readlen != aLength");

  PR_Free(buffer);
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP 
nsPluginCacheListener::OnStopRequest(nsIRequest *request, 
                                     nsISupports* aContext, 
                                     nsresult aStatus)
{
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

nsPluginStreamListenerPeer::nsPluginStreamListenerPeer()
{
  NS_INIT_REFCNT();

  mURL = nsnull;
  mOwner = nsnull;
  mInstance = nsnull;
  mPStreamListener = nsnull;
  mPluginStreamInfo = nsnull;
  mSetUpListener = PR_FALSE;
  mHost = nsnull;
  mStreamType = nsPluginStreamType_Normal;
  mStartBinding = PR_FALSE;
  mAbort = PR_FALSE;
  mRequestFailed = PR_FALSE;

  mPendingRequests = 0;
  mHaveFiredOnStartRequest = PR_FALSE;
  mDataForwardToRequest = nsnull;
}


////////////////////////////////////////////////////////////////////////
nsPluginStreamListenerPeer::~nsPluginStreamListenerPeer()
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if(mURL != nsnull) (void)mURL->GetSpec(urlSpec);

  nsCAutoString filePath;
  if(mLocalFile != nsnull) (void)mLocalFile->GetNativePath(filePath);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginStreamListenerPeer::dtor this=%p, url=%s, POST_file=%s\n",this, urlSpec.get(), filePath.get()));

  PR_LogFlush();
#endif

  NS_IF_RELEASE(mURL);
  NS_IF_RELEASE(mOwner);
  NS_IF_RELEASE(mInstance);
  NS_IF_RELEASE(mPStreamListener);
  NS_IF_RELEASE(mHost);
  NS_IF_RELEASE(mPluginStreamInfo);

  // if we have mLocalFile (temp file used to post data) it should be
  // safe to delete it now, and hopefully the owner doesn't hold it.
  if(mLocalFile)
    mLocalFile->Remove(PR_FALSE);

  delete mDataForwardToRequest;
}


////////////////////////////////////////////////////////////////////////
NS_IMPL_ISUPPORTS3(nsPluginStreamListenerPeer,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIHttpHeaderVisitor)
////////////////////////////////////////////////////////////////////////


/* Called as a result of GetURL and PostURL */
////////////////////////////////////////////////////////////////////////
nsresult nsPluginStreamListenerPeer::Initialize(nsIURI *aURL, 
                                                nsIPluginInstance *aInstance,
                                                nsIPluginStreamListener* aListener,
                                                PRInt32 requestCount)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if(aURL != nsnull) (void)aURL->GetAsciiSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginStreamListenerPeer::Initialize instance=%p, url=%s\n", aInstance, urlSpec.get()));

  PR_LogFlush();
#endif

  mURL = aURL;
  NS_ADDREF(mURL);

  mInstance = aInstance;
  NS_ADDREF(mInstance);
  
  mPStreamListener = aListener;
  NS_ADDREF(mPStreamListener);

  mPluginStreamInfo = new nsPluginStreamInfo();
  if (!mPluginStreamInfo)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(mPluginStreamInfo);
  mPluginStreamInfo->SetPluginInstance(aInstance);
  mPluginStreamInfo->SetPluginStreamListenerPeer(this);

  mPendingRequests = requestCount;

  mDataForwardToRequest = new nsHashtable(16, PR_FALSE);
  if (!mDataForwardToRequest) 
      return NS_ERROR_FAILURE;

  return NS_OK;
}


/* 
    Called by NewEmbededPluginStream() - if this is called, we weren't 
    able to load the plugin, so we need to load it later once we figure 
    out the mimetype.  In order to load it later, we need the plugin 
    host and instance owner.
*/
////////////////////////////////////////////////////////////////////////
nsresult nsPluginStreamListenerPeer::InitializeEmbeded(nsIURI *aURL, 
                                                       nsIPluginInstance* aInstance, 
                                                       nsIPluginInstanceOwner *aOwner,
                                                       nsIPluginHost *aHost)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if(aURL != nsnull) (void)aURL->GetSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginStreamListenerPeer::InitializeEmbeded url=%s\n", urlSpec.get()));

  PR_LogFlush();
#endif

  mURL = aURL;
  NS_ADDREF(mURL);

  if(aInstance != nsnull) {
    NS_ASSERTION(mInstance == nsnull, "nsPluginStreamListenerPeer::InitializeEmbeded mInstance != nsnull");
    mInstance = aInstance;
    NS_ADDREF(mInstance);
  } else {
    mOwner = aOwner;
    NS_IF_ADDREF(mOwner);

    mHost = aHost;
    NS_IF_ADDREF(mHost);
  }

  mPluginStreamInfo = new nsPluginStreamInfo();
  if (!mPluginStreamInfo)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(mPluginStreamInfo);
  mPluginStreamInfo->SetPluginInstance(aInstance);
  mPluginStreamInfo->SetPluginStreamListenerPeer(this);

  mDataForwardToRequest = new nsHashtable(16, PR_FALSE);
  if (!mDataForwardToRequest) 
      return NS_ERROR_FAILURE;

  return NS_OK;
}


/* Called by NewFullPagePluginStream() */
////////////////////////////////////////////////////////////////////////
nsresult nsPluginStreamListenerPeer::InitializeFullPage(nsIPluginInstance *aInstance)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginStreamListenerPeer::InitializeFullPage instance=%p\n",aInstance));

  NS_ASSERTION(mInstance == nsnull, "nsPluginStreamListenerPeer::InitializeFullPage mInstance != nsnull");
  mInstance = aInstance;
  NS_ADDREF(mInstance);

  mPluginStreamInfo = new nsPluginStreamInfo();
  if (!mPluginStreamInfo)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(mPluginStreamInfo);
  mPluginStreamInfo->SetPluginInstance(aInstance);
  mPluginStreamInfo->SetPluginStreamListenerPeer(this);

  mDataForwardToRequest = new nsHashtable(16, PR_FALSE);
  if (!mDataForwardToRequest) 
      return NS_ERROR_FAILURE;

  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// SetupPluginCacheFile is called if we have to save the stream to disk.
// the most likely cause for this is either there is no disk cache available
// or the stream is coming from a https server.  
//
// These files will be deleted when the host is destroyed.
//
// TODO? What if we fill up the the dest dir?
nsresult
nsPluginStreamListenerPeer::SetupPluginCacheFile(nsIChannel* channel)
{
    nsresult rv;
    
    // Is this the best place to put this temp file?
    nsCOMPtr<nsIFile> pluginTmp;
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(pluginTmp));
    if (NS_FAILED(rv)) return rv;
    
    rv = pluginTmp->AppendNative(kPluginTmpDirName);
    if (NS_FAILED(rv)) return rv;
    
    (void) pluginTmp->Create(nsIFile::DIRECTORY_TYPE,0777);

    // Get the filename from the channel
    nsCOMPtr<nsIURI> uri;
    rv = channel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
    if(!url)
      return NS_ERROR_FAILURE;

    nsCAutoString filename;
    url->GetFileName(filename);
    if (NS_FAILED(rv)) return rv;

    // Create a file to save our stream into. Should we scramble the name?
    rv = pluginTmp->AppendNative(filename);
    if (NS_FAILED(rv)) return rv;
    
    // Yes, make it unique.
    rv = pluginTmp->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0777); 
    if (NS_FAILED(rv)) return rv;

    // save the file.
    mPluginStreamInfo->SetLocalCachedFile(pluginTmp);
    
    // create a file output stream to write to...
    nsCOMPtr<nsIOutputStream> outstream;
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(outstream), pluginTmp, -1, 00600);
    if (NS_FAILED(rv)) return rv;
    
    mPluginStreamInfo->SetLocalCachedFileStream(outstream);
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginStreamListenerPeer::OnStartRequest(nsIRequest *request, nsISupports* aContext)
{
  nsresult  rv = NS_OK;

  if (mHaveFiredOnStartRequest) {
      return NS_OK;
  }

  mHaveFiredOnStartRequest = PR_TRUE;

  // do a little sanity check to make sure our frame isn't gone
  // by getting the tag type and checking for an error, we can determine if
  // the frame is gone
  if (mOwner) {
    nsCOMPtr<nsIPluginTagInfo2> pti2 = do_QueryInterface(mOwner);
    NS_ENSURE_TRUE(pti2, NS_ERROR_FAILURE);
    nsPluginTagType tagType;  
    if (NS_FAILED(pti2->GetTagType(&tagType)))
      return NS_ERROR_FAILURE;  // something happened to our object frame, so bail!
  }

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  NS_ENSURE_TRUE(channel, NS_ERROR_FAILURE);
  
  // deal with 404 (Not Found) HTTP response,
  // just return, this causes the request to be ignored.
  // Also deal with content-encoding
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  PRBool useCacheAsFile = PR_TRUE;

  if (httpChannel) {
    PRUint32 responseCode = 0;
    rv = httpChannel->GetResponseStatus(&responseCode);
    if (NS_FAILED(rv) || responseCode > 206) { // not normal
      // NPP_Notify() will be called from OnStopRequest
      // in ns4xPluginStreamListener::CleanUpStream
      // return error will cancel this request
      // ...and we also need to tell the plugin that
      mRequestFailed = PR_TRUE;
      return NS_ERROR_FAILURE;
    }

    // Now we look for a content-encoding header.  If we find one,
    // we can't use the cache as a file
    nsCAutoString contentEncoding;
    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Encoding"),
                                        contentEncoding);
    if (NS_SUCCEEDED(rv) &&
        !contentEncoding.Equals("identity",
                                nsCaseInsensitiveCStringComparator())) {
      useCacheAsFile = PR_FALSE;
    }
  }

  if (useCacheAsFile) {
    nsCOMPtr<nsICachingChannel> cacheChannel = do_QueryInterface(channel, &rv);
    if (cacheChannel)
      rv = cacheChannel->SetCacheAsFile(PR_TRUE);
    else {
      // do not cache if this is a file channel,
      // just overwrite rv here
      nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(channel, &rv);
    }
  }

  if (!useCacheAsFile || NS_FAILED(rv)) {
    // The channel doesn't want to do our bidding, lets cache it to disk ourselves
    rv = SetupPluginCacheFile(channel);
    if (NS_FAILED(rv))
      NS_WARNING("No Cache Aval.  Some plugins wont work OR we don't have a URL");
  }

  nsCAutoString aContentType;
  rv = channel->GetContentType(aContentType);
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;

  if (!aContentType.IsEmpty())
    mPluginStreamInfo->SetContentType(aContentType.get());

#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if(aURL != nsnull) (void)aURL->GetSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NOISY,
  ("nsPluginStreamListenerPeer::OnStartRequest this=%p request=%p mime=%s, url=%s\n",
  this, request, aContentType.get(), urlSpec.get()));

  PR_LogFlush();
#endif

  nsPluginWindow    *window = nsnull;

  // if we don't have an nsIPluginInstance (mInstance), it means
  // we weren't able to load a plugin previously because we
  // didn't have the mimetype.  Now that we do (aContentType),
  // we'll try again with SetUpPluginInstance() 
  // which is called by InstantiateEmbededPlugin()
  // NOTE: we don't want to try again if we didn't get the MIME type this time

  if ((nsnull == mInstance) && (nsnull != mOwner) && (!aContentType.IsEmpty()))
  {
    mOwner->GetInstance(mInstance);
    mOwner->GetWindow(window);

    if ((nsnull == mInstance) && (nsnull != mHost) && (nsnull != window))
    {
      // determine if we need to try embedded again. FullPage takes a different code path
      nsPluginMode mode;
      mOwner->GetMode(&mode);
      if (mode == nsPluginMode_Embedded)
        rv = mHost->InstantiateEmbededPlugin(aContentType.get(), aURL, mOwner);
      else
        rv = mHost->SetUpPluginInstance(aContentType.get(), aURL, mOwner);

      if (NS_OK == rv)
      {
        // GetInstance() adds a ref
        mOwner->GetInstance(mInstance);

        if (nsnull != mInstance)
        {
          mInstance->Start();
          mOwner->CreateWidget();

          // If we've got a native window, the let the plugin know
          // about it.
          if (window->window)
            mInstance->SetWindow(window);
        }
      }
    }
  }

  //
  // Set up the stream listener...
  //
  PRInt32 length;

  rv = channel->GetContentLength(&length);

  // it's possible for the server to not send a Content-Length.  We should
  // still work in this case.
  if (NS_FAILED(rv)) {
    mPluginStreamInfo->SetLength(PRUint32(-1));
  }
  else {
    mPluginStreamInfo->SetLength(length);
  }


  rv = SetUpStreamListener(request, aURL);
  if (NS_FAILED(rv)) return rv;

  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginStreamListenerPeer::OnProgress(nsIRequest *request, 
                                                     nsISupports* aContext, 
                                                     PRUint32 aProgress, 
                                                     PRUint32 aProgressMax)
{
  nsresult rv = NS_OK;
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginStreamListenerPeer::OnStatus(nsIRequest *request, 
                                                   nsISupports* aContext,
                                                   nsresult aStatus,
                                                   const PRUnichar* aStatusArg)
{
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
class nsPRUintKey : public nsHashKey {
protected:
    PRUint32 mKey;
public:
    nsPRUintKey(PRUint32 key) : mKey(key) {}

    PRUint32 HashCode(void) const {
        return mKey;
    }

    PRBool Equals(const nsHashKey *aKey) const {
        return mKey == ((const nsPRUintKey *) aKey)->mKey;
    }
    nsHashKey *Clone() const {
        return new nsPRUintKey(mKey);
    }
    PRUint32 GetValue() { return mKey; }
};


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginStreamListenerPeer::OnDataAvailable(nsIRequest *request, 
                                                          nsISupports* aContext, 
                                                          nsIInputStream *aIStream, 
                                                          PRUint32 sourceOffset, 
                                                          PRUint32 aLength)
{
  if(mAbort)
  {
      PRUint32 magicNumber = 0;  // set it to something that is not the magic number.
      nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(aContext);
      if (container)
        container->GetData(&magicNumber);
      
      if (magicNumber != MAGIC_REQUEST_CONTEXT)
      {
        // this is not one of our range requests
        mAbort = PR_FALSE;
        return NS_BINDING_ABORTED;
      }
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsIURI> aURL;
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (!channel) 
    return NS_ERROR_FAILURE;
  
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) 
    return rv;

  if(!mPStreamListener || !mPluginStreamInfo)
    return NS_ERROR_FAILURE;

  nsCAutoString urlString;
  aURL->GetSpec(urlString);
  mPluginStreamInfo->SetURL(urlString.get());
  PLUGIN_LOG(PLUGIN_LOG_NOISY,
  ("nsPluginStreamListenerPeer::OnDataAvailable this=%p request=%p, offset=%d, length=%d, url=%s\n",
  this, request, sourceOffset, aLength, urlString.get()));

  // if the plugin has requested an AsFileOnly stream, then don't 
  // call OnDataAvailable
  if(mStreamType != nsPluginStreamType_AsFileOnly)
  {
    // get the absolute offset of the request, if one exists.
    nsCOMPtr<nsIByteRangeRequest> brr = do_QueryInterface(request);
    PRInt32 absoluteOffset = 0;
    PRInt32 amtForwardToPlugin = 0;
    if (brr) {
        brr->GetStartRange(&absoluteOffset);
        
        // we need to track how much data we have forward on to the plugin.  
        nsPRUintKey key(absoluteOffset);

        if (!mDataForwardToRequest)
            return NS_ERROR_FAILURE;

        if (mDataForwardToRequest->Exists(&key))
            amtForwardToPlugin = NS_PTR_TO_INT32(mDataForwardToRequest->Remove(&key));
    
        mDataForwardToRequest->Put(&key, (void*) (amtForwardToPlugin+aLength));
        mPluginStreamInfo->SetStreamOffset(absoluteOffset + amtForwardToPlugin);
    }

    nsCOMPtr<nsIInputStream> stream = aIStream;

    // if we are caching the file ourselves to disk, we want to 'tee' off
    // the data as the plugin read from the stream.  We do this by the magic
    // of an input stream tee.

    nsCOMPtr<nsIOutputStream> outStream;
    mPluginStreamInfo->GetLocalCachedFileStream(getter_AddRefs(outStream));
    if (outStream) {
        rv = NS_NewInputStreamTee(getter_AddRefs(stream), aIStream, outStream);
        if (NS_FAILED(rv)) 
            return rv;
    }

    rv =  mPStreamListener->OnDataAvailable((nsIPluginStreamInfo*)mPluginStreamInfo, 
                                              stream, 
                                              aLength);

    // if a plugin returns an error, the peer must kill the stream
    //   else the stream and PluginStreamListener leak
    if (NS_FAILED(rv))
      request->Cancel(rv);
  }
  else
  {
    // if we don't read from the stream, OnStopRequest will never be called
    char* buffer = new char[aLength];
    PRUint32 amountRead, amountWrote = 0;
    rv = aIStream->Read(buffer, aLength, &amountRead);
    
    // if we are caching this to disk ourselves, lets write the bytes out.
    nsCOMPtr<nsIOutputStream> outStream;
    mPluginStreamInfo->GetLocalCachedFileStream(getter_AddRefs(outStream));
    while (outStream && amountWrote < amountRead && NS_SUCCEEDED(rv))
      rv = outStream->Write(buffer, amountRead, &amountWrote);
    delete [] buffer;
  }
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginStreamListenerPeer::OnStopRequest(nsIRequest *request, 
                                                        nsISupports* aContext,
                                                        nsresult aStatus)
{
  nsresult rv = NS_OK;

  PLUGIN_LOG(PLUGIN_LOG_NOISY,
  ("nsPluginStreamListenerPeer::OnStopRequest this=%p aStatus=%d request=%p\n",
  this, aStatus, request));
  
  // for ByteRangeRequest we're just updating the mDataForwardToRequest hash and return.
  nsCOMPtr<nsIByteRangeRequest> brr = do_QueryInterface(request);
  if (brr) {
    PRInt32 absoluteOffset = 0;
    brr->GetStartRange(&absoluteOffset);
    
    nsPRUintKey key(absoluteOffset);
    
    // remove the request from our data forwarding count hash.    
    (void) mDataForwardToRequest->Remove(&key);
  

    PLUGIN_LOG(PLUGIN_LOG_NOISY,
    ("                          ::OnStopRequest for ByteRangeRequest Started=%d\n",
    absoluteOffset));
  } else {
    // if this is not byte range request and
    // if we are writting the stream to disk ourselves, lets close it
    nsCOMPtr<nsIOutputStream> outStream;
    mPluginStreamInfo->GetLocalCachedFileStream(getter_AddRefs(outStream));
    if (outStream) {
       outStream->Close();
      mPluginStreamInfo->SetLocalCachedFileStream(nsnull);
    }
  }

  // if we still have pending stuff to do, lets not close the plugin socket.
  if (--mPendingRequests > 0)
      return NS_OK;
  
  // we keep our connections around...
  nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(aContext);
  if (container) {
    PRUint32 magicNumber = 0;  // set it to something that is not the magic number.
    container->GetData(&magicNumber);
    if (magicNumber == MAGIC_REQUEST_CONTEXT) {
      // this is one of our range requests
      return NS_OK;
    }
  }
  
  if(!mPStreamListener)
      return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIURI> aURL;
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (!channel) 
    return NS_ERROR_FAILURE;

  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) 
    return rv;
  
  nsCAutoString urlString;
  rv = aURL->GetAsciiSpec(urlString);
  if (NS_SUCCEEDED(rv)) 
    mPluginStreamInfo->SetURL(urlString.get());
  
  // Set the content type to ensure we don't pass null to the plugin
  nsCAutoString aContentType;
  rv = channel->GetContentType(aContentType);
  if (NS_FAILED(rv)) 
    return rv;

  if (!aContentType.IsEmpty())
    mPluginStreamInfo->SetContentType(aContentType.get());

  // set error status if stream failed so we notify the plugin
  if (mRequestFailed)
    aStatus = NS_ERROR_FAILURE;

  if (NS_FAILED(aStatus)) {
    // on error status cleanup the stream 
    // and return w/o OnFileAvailable()
    mPStreamListener->OnStopBinding((nsIPluginStreamInfo*)mPluginStreamInfo, aStatus);
    return NS_OK;
  }

  nsCOMPtr<nsIFile> localFile;
  mPluginStreamInfo->GetLocalCachedFile(getter_AddRefs(localFile));

  if (!localFile) {
    nsCOMPtr<nsICachingChannel> cacheChannel = do_QueryInterface(request);
    if (cacheChannel) {
      cacheChannel->GetCacheFile(getter_AddRefs(localFile));
    } else {
      // see if it is a file channel.
      nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(request);
      if (fileChannel) {
        fileChannel->GetFile(getter_AddRefs(localFile));
      }
    }
  }
  
  if (localFile) {
    OnFileAvailable(localFile);
  }

  if (mStartBinding)
  {
    // On start binding has been called
    mPStreamListener->OnStopBinding((nsIPluginStreamInfo*)mPluginStreamInfo, aStatus);
  }
  else
  {
    // OnStartBinding hasn't been called, so complete the action.
    mPStreamListener->OnStartBinding((nsIPluginStreamInfo*)mPluginStreamInfo);
    mPStreamListener->OnStopBinding((nsIPluginStreamInfo*)mPluginStreamInfo, aStatus);
  }

  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// private methods for nsPluginStreamListenerPeer
nsresult nsPluginStreamListenerPeer::SetUpCache(nsIURI* aURL)
{
  nsPluginCacheListener* cacheListener = new nsPluginCacheListener(this);
  // XXX: Null LoadGroup?
  return NS_OpenURI(cacheListener, nsnull, aURL, nsnull);
}


////////////////////////////////////////////////////////////////////////
nsresult nsPluginStreamListenerPeer::SetUpStreamListener(nsIRequest *request,
                                                         nsIURI* aURL)
{
  nsresult rv = NS_OK;

  // If we don't yet have a stream listener, we need to get 
  // one from the plugin.
  // NOTE: this should only happen when a stream was NOT created 
  // with GetURL or PostURL (i.e. it's the initial stream we 
  // send to the plugin as determined by the SRC or DATA attribute)
  if(mPStreamListener == nsnull && mInstance != nsnull)	  
    rv = mInstance->NewStream(&mPStreamListener);

  if(rv != NS_OK)
    return rv;

  if(mPStreamListener == nsnull)
    return NS_ERROR_NULL_POINTER;
  

  // get httpChannel to retrieve some info we need for nsIPluginStreamInfo setup
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);

  /*
   * Assumption
   * By the time nsPluginStreamListenerPeer::OnDataAvailable() gets
   * called, all the headers have been read.
   */
  if (httpChannel) 
    httpChannel->VisitResponseHeaders(this);
  
  mSetUpListener = PR_TRUE;
  
  // set seekability (seekable if the stream has a known length and if the
  // http server accepts byte ranges).
  PRBool bSeekable = PR_FALSE;
  PRUint32 length = PRUint32(-1);
  mPluginStreamInfo->GetLength(&length);
  if ((length != PRUint32(-1)) && httpChannel)
  {
    nsCAutoString range;
    if(NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("accept-ranges"), range)))
    {
      if (range.Equals(NS_LITERAL_CSTRING("bytes"), nsCaseInsensitiveCStringComparator()))
        bSeekable = PR_TRUE;
    }
  }
  mPluginStreamInfo->SetSeekable(bSeekable);

  // we require a content len
  // get Last-Modified header for plugin info
  if (httpChannel) 
  {
    nsCAutoString lastModified;
    if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("last-modified"), lastModified)) &&
        !lastModified.IsEmpty())
    {
      PRTime time64;
      PR_ParseTimeString(lastModified.get(), PR_TRUE, &time64);  //convert string time to interger time
 
      // Convert PRTime to unix-style time_t, i.e. seconds since the epoch
      double fpTime;
      LL_L2D(fpTime, time64);
      mPluginStreamInfo->SetLastModified((PRUint32)(fpTime * 1e-6 + 0.5));
    }
  } 

  nsCAutoString urlString;
  aURL->GetAsciiSpec(urlString);
  mPluginStreamInfo->SetURL(urlString.get());

  rv = mPStreamListener->OnStartBinding((nsIPluginStreamInfo*)mPluginStreamInfo);

  mStartBinding = PR_TRUE;

  if (NS_SUCCEEDED(rv)) {
    mPStreamListener->GetStreamType(&mStreamType);
    // if plugin requests stream type StreamType_AsFile or StreamType_AsFileOnly
    // most likely it'll try to check file extension,
    // but our cache does nor support it yet.
    // temp fix is to copy the file+ext into temp location
    // all this code should be deleted when bug 90558 got fixed
#if !defined(CACHE_SUPPOPTS_FILE_EXTENSION)
    if (mStreamType	>= nsPluginStreamType_AsFile && httpChannel) {
      // check out if we already set output stream for this channel
      nsCOMPtr<nsIOutputStream> outStream;
      mPluginStreamInfo->GetLocalCachedFileStream(getter_AddRefs(outStream));
      if (!outStream) {
        SetupPluginCacheFile(httpChannel);
      }
    }
#endif
  }

  return rv;
}


////////////////////////////////////////////////////////////////////////
nsresult
nsPluginStreamListenerPeer::OnFileAvailable(nsIFile* aFile)
{
  nsresult rv;
  if (!mPStreamListener)
    return NS_ERROR_FAILURE;

  nsCAutoString path;
  rv = aFile->GetNativePath(path);
  if (NS_FAILED(rv)) return rv;

  if (path.IsEmpty()) {
    NS_WARNING("empty path");
    return NS_OK;
  }

  rv = mPStreamListener->OnFileAvailable((nsIPluginStreamInfo*)mPluginStreamInfo, path.get());
  return rv;
}


////////////////////////////////////////////////////////////////////////
nsILoadGroup*
nsPluginStreamListenerPeer::GetLoadGroup()
{
  nsILoadGroup* loadGroup = nsnull;
  nsIDocument* doc;
  nsresult rv = mOwner->GetDocument(&doc);
  if (NS_SUCCEEDED(rv)) {
    doc->GetDocumentLoadGroup(&loadGroup);
    NS_RELEASE(doc);
  }
  return loadGroup;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginStreamListenerPeer::VisitHeader(const nsACString &header, const nsACString &value)
{
  nsCOMPtr<nsIHTTPHeaderListener> listener = do_QueryInterface(mPStreamListener);
  if (!listener)
    return NS_ERROR_FAILURE;

  return listener->NewResponseHeader(PromiseFlatCString(header).get(),
                                     PromiseFlatCString(value).get());
}


////////////////////////////////////////////////////////////////////////
nsresult nsPluginStreamListenerPeer::SetLocalFile(nsIFile* aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);

  if(mLocalFile)
  {
    NS_ASSERTION(!mLocalFile, "nsPluginStreamListenerPeer::SetLocalFile -- path already set, cleaning...");
    mLocalFile = nsnull;
  }

  // clone file to prevent it from being changed out from under us
  return aFile->Clone(getter_AddRefs(mLocalFile));
}

/////////////////////////////////////////////////////////////////////////

nsPluginHostImpl::nsPluginHostImpl()
{
  NS_INIT_REFCNT();
  mPluginsLoaded = PR_FALSE;
  mDontShowBadPluginMessage = PR_FALSE;
  mIsDestroyed = PR_FALSE;
  mUnusedLibraries = nsnull;
  mOverrideInternalTypes = PR_FALSE;
  mAllowAlienStarHandler = PR_FALSE;
  
  // check to see if pref is set at startup to let plugins take over in 
  // full page mode for certain image mime types that we handle internally
  nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
  if (prefs) {
    prefs->GetBoolPref("plugin.override_internal_types", &mOverrideInternalTypes);
    prefs->GetBoolPref("plugin.allow_alien_star_handler", &mAllowAlienStarHandler);
  }

  nsCOMPtr<nsIObserverService> obsService = do_GetService("@mozilla.org/observer-service;1");
  if (obsService)
  {
    obsService->AddObserver(this, "quit-application", PR_FALSE);
    obsService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  }

#ifdef PLUGIN_LOGGING
  nsPluginLogging::gNPNLog = PR_NewLogModule(NPN_LOG_NAME);
  nsPluginLogging::gNPPLog = PR_NewLogModule(NPP_LOG_NAME);
  nsPluginLogging::gPluginLog = PR_NewLogModule(PLUGIN_LOG_NAME);
  
  PR_LOG(nsPluginLogging::gNPNLog, PLUGIN_LOG_ALWAYS,("NPN Logging Active!\n"));
  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_ALWAYS,("General Plugin Logging Active! (nsPluginHostImpl::ctor)\n"));
  PR_LOG(nsPluginLogging::gNPPLog, PLUGIN_LOG_ALWAYS,("NPP Logging Active!\n"));
  
  PLUGIN_LOG(PLUGIN_LOG_ALWAYS,("nsPluginHostImpl::ctor\n"));
  PR_LogFlush();
#endif
  mCachedPlugins = nsnull;
}


////////////////////////////////////////////////////////////////////////
nsPluginHostImpl::~nsPluginHostImpl()
{
  PLUGIN_LOG(PLUGIN_LOG_ALWAYS,("nsPluginHostImpl::dtor\n"));

#ifdef NS_DEBUG
  printf("nsPluginHostImpl dtor\n");
#endif
  Destroy();
}

////////////////////////////////////////////////////////////////////////
NS_IMPL_ISUPPORTS7(nsPluginHostImpl,
                   nsIPluginManager,
                   nsIPluginManager2,
                   nsIPluginHost,
                   nsIFileUtilities,
                   nsICookieStorage,
                   nsIObserver,
                   nsPIPluginHost);
////////////////////////////////////////////////////////////////////////
NS_METHOD
nsPluginHostImpl::Create(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  NS_PRECONDITION(aOuter == nsnull, "no aggregation");
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsPluginHostImpl* host = new nsPluginHostImpl();
  if (! host)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;
  NS_ADDREF(host);
  rv = host->QueryInterface(aIID, aResult);
  NS_RELEASE(host);
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::GetValue(nsPluginManagerVariable aVariable, void *aValue)
{
  nsresult rv = NS_OK;

  NS_ENSURE_ARG_POINTER(aValue);

#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(NO_X11)
  if (nsPluginManagerVariable_XDisplay == aVariable) {
    Display** value = NS_REINTERPRET_CAST(Display**, aValue);
#if defined(MOZ_WIDGET_GTK)
    *value = GDK_DISPLAY();
#elif defined(MOZ_WIDGET_QT)
    *value = qt_xdisplay();
#elif defined(MOZ_WIDGET_XLIB)
    *value = xxlib_rgb_get_display(xxlib_find_handle(XXLIBRGB_DEFAULT_HANDLE));
#endif
    if (!(*value)) {
      return NS_ERROR_FAILURE;
    }
  }
#endif
  return rv;
}


////////////////////////////////////////////////////////////////////////
PRBool nsPluginHostImpl::IsRunningPlugin(nsPluginTag * plugin)
{
  if(!plugin)
    return PR_FALSE;

  // we can check for mLibrary to be non-zero and then querry nsIPluginInstancePeer
  // in nsActivePluginList to see if plugin with matching mime type is not stopped
  if(!plugin->mLibrary)
    return PR_FALSE;

  for(int i = 0; i < plugin->mVariants; i++) {
    nsActivePlugin * p = mActivePluginList.find(plugin->mMimeTypeArray[i]);
    if(p && !p->mStopped)
      return PR_TRUE;
  }

  return PR_FALSE;
}


////////////////////////////////////////////////////////////////////////
void nsPluginHostImpl::AddToUnusedLibraryList(PRLibrary * aLibrary)
{
  NS_ASSERTION(aLibrary, "nsPluginHostImpl::AddToUnusedLibraryList: Nothing to add");
  if(!aLibrary)
    return;

  nsUnusedLibrary * unusedLibrary = new nsUnusedLibrary(aLibrary);
  if(unusedLibrary) {
    unusedLibrary->mNext = mUnusedLibraries;
    mUnusedLibraries = unusedLibrary;
  }
}


////////////////////////////////////////////////////////////////////////
// this will unload loaded but no longer needed libs which are
// gathered in mUnusedLibraries list, see bug #61388
void nsPluginHostImpl::CleanUnusedLibraries()
{
  if(!mUnusedLibraries)
    return;

  while (nsnull != mUnusedLibraries) {
    nsUnusedLibrary *temp = mUnusedLibraries->mNext;
    delete mUnusedLibraries;
    mUnusedLibraries = temp;
  }
}


////////////////////////////////////////////////////////////////////////
nsresult nsPluginHostImpl::ReloadPlugins(PRBool reloadPages)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHostImpl::ReloadPlugins Begin reloadPages=%d, active_instance_count=%d\n",
  reloadPages, mActivePluginList.mCount));

  nsresult rv = NS_OK;

  // we are re-scanning plugins. New plugins may have been added, also some
  // plugins may have been removed, so we should probably shut everything down
  // but don't touch running (active and  not stopped) plugins
  if(reloadPages) {
    // check if plugins changed, no need to refresh and reload
    // page if no changes to plugins have been made
    // PR_FALSE instructs not to touch the plugin list, just to
    // look for possible changes
    PRBool pluginschanged = PR_TRUE;
    FindPlugins(PR_FALSE, &pluginschanged);

    // if no changed detected, return an appropriate error code
    if (!pluginschanged)
      return NS_ERROR_PLUGINS_PLUGINSNOTCHANGED;

    // if we have currently running plugins we should set a flag not to
    // unload them from memory, see bug #61388
    // and form a list of libs to be unloaded later
    for(nsPluginTag * p = mPlugins; p != nsnull; p = p->mNext) {
      if(IsRunningPlugin(p) && (p->mFlags & NS_PLUGIN_FLAG_OLDSCHOOL)) {
        p->mCanUnloadLibrary = PR_FALSE;
        AddToUnusedLibraryList(p->mLibrary);
      }
    }

    // then stop any running plugins
    mActivePluginList.stopRunning();
  }

  // clean active plugin list
  mActivePluginList.removeAllStopped();

  // shutdown plugins and kill the list if there are no running plugins
  nsPluginTag * prev = nsnull;
  nsPluginTag * next = nsnull;

  for(nsPluginTag * p = mPlugins; p != nsnull;) {
    next = p->mNext;

    // XXX only remove our plugin from the list if it's not running and not
    // an XPCOM plugin. XPCOM plugins do not get a call to nsIPlugin::Shutdown
    // if plugins are reloaded. This also fixes a crash on UNIX where the call
    // to shutdown would break the ProxyJNI connection to the JRE after a reload.
    // see bug 86591
    if(!IsRunningPlugin(p) && (!p->mEntryPoint || (p->mFlags & NS_PLUGIN_FLAG_OLDSCHOOL))) {
      if(p == mPlugins)
        mPlugins = next;
      else
        prev->mNext = next;

      delete p;
      p = next;
      continue;
    }

    prev = p;
    p = next;
  }

  // set flags
  mPluginsLoaded = PR_FALSE;

  //refresh the component registry first
  nsCOMPtr<nsIServiceManager> servManager;
  NS_GetServiceManager(getter_AddRefs(servManager));
  nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servManager);
  if (!registrar) {
    NS_ASSERTION(0, "No nsIComponentRegistrar from get service");
    return NS_ERROR_FAILURE;
  }
  NS_ASSERTION(registrar, "No nsIComponentRegistrar from get service");
  rv = registrar->AutoRegister(nsnull);

  // load them again
  rv = LoadPlugins();

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHostImpl::ReloadPlugins End active_instance_count=%d\n",
  mActivePluginList.mCount));

  return rv;
}

#define NS_RETURN_UASTRING_SIZE 128


////////////////////////////////////////////////////////////////////////
nsresult nsPluginHostImpl::UserAgent(const char **retstring)
{
  static char resultString[NS_RETURN_UASTRING_SIZE];
  nsresult res;

  nsCOMPtr<nsIHttpProtocolHandler> http = do_GetService(kHttpHandlerCID, &res);
  if (NS_FAILED(res)) 
    return res;

  nsCAutoString uaString;
  res = http->GetUserAgent(uaString);

  if (NS_SUCCEEDED(res)) 
  {
    if(NS_RETURN_UASTRING_SIZE > uaString.Length())
    {
      PL_strcpy(resultString, uaString.get());
      *retstring = resultString;
    }
    else
    {
      *retstring = nsnull;
      res = NS_ERROR_OUT_OF_MEMORY;
    }
  } 
  else
    *retstring = nsnull;

  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsPluginHostImpl::UserAgent return=%s\n", *retstring));

  return res;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::GetURL(nsISupports* pluginInst, 
                     const char* url, 
                     const char* target,
                     nsIPluginStreamListener* streamListener,
                     const char* altHost,
                     const char* referrer,
                     PRBool forceJSEnabled)
{
  return GetURLWithHeaders(pluginInst, url, target, streamListener, 
                           altHost, referrer, forceJSEnabled, nsnull, nsnull);
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::GetURLWithHeaders(nsISupports* pluginInst, 
                     const char* url, 
                     const char* target,
                     nsIPluginStreamListener* streamListener,
                     const char* altHost,
                     const char* referrer,
                     PRBool forceJSEnabled,
                     PRUint32 getHeadersLength, 
                     const char* getHeaders)
{
  nsAutoString      string; string.AssignWithConversion(url);
  nsIPluginInstance *instance;
  nsresult          rv;

  // we can only send a stream back to the plugin (as specified by a 
  // null target) if we also have a nsIPluginStreamListener to talk to also
  if(target == nsnull && streamListener == nsnull)
   return NS_ERROR_ILLEGAL_VALUE;

  rv = pluginInst->QueryInterface(kIPluginInstanceIID, (void **)&instance);

  if (NS_SUCCEEDED(rv))
  {
    if (nsnull != target)
    {
      nsPluginInstancePeerImpl *peer;

      rv = instance->GetPeer(NS_REINTERPRET_CAST(nsIPluginInstancePeer **, &peer));

      if (NS_SUCCEEDED(rv))
      {
        nsCOMPtr<nsIPluginInstanceOwner> owner;

        rv = peer->GetOwner(*getter_AddRefs(owner));

        if (NS_SUCCEEDED(rv))
        {
          if ((0 == PL_strcmp(target, "newwindow")) || 
              (0 == PL_strcmp(target, "_new")))
            target = "_blank";
          else if (0 == PL_strcmp(target, "_current"))
            target = "_self";

          rv = owner->GetURL(url, target, nsnull, 0, (void *) getHeaders, 
                             getHeadersLength);
        }

        NS_RELEASE(peer);
      }
    }

    if (nsnull != streamListener)
      rv = NewPluginURLStream(string, instance, streamListener, nsnull, 
                              PR_FALSE, nsnull, getHeaders, getHeadersLength);

    NS_RELEASE(instance);
  }

  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::PostURL(nsISupports* pluginInst,
                    const char* url,
                    PRUint32 postDataLen, 
                    const char* postData,
                    PRBool isFile,
                    const char* target,
                    nsIPluginStreamListener* streamListener,
                    const char* altHost, 
                    const char* referrer,
                    PRBool forceJSEnabled,
                    PRUint32 postHeadersLength, 
                    const char* postHeaders)
{
  nsAutoString      string; string.AssignWithConversion(url);
  nsIPluginInstance *instance;
  nsresult          rv;
  
  // we can only send a stream back to the plugin (as specified 
  // by a null target) if we also have a nsIPluginStreamListener 
  // to talk to also
  if(target == nsnull && streamListener == nsnull)
   return NS_ERROR_ILLEGAL_VALUE;
  
  rv = pluginInst->QueryInterface(kIPluginInstanceIID, (void **)&instance);

  if (NS_SUCCEEDED(rv))
  {
      char *dataToPost;
      if (isFile) {
        rv = CreateTmpFileToPost(postData, &dataToPost);
        if (NS_FAILED(rv) || !dataToPost) return rv;

      } else {
        PRUint32 newDataToPostLen;
        ParsePostBufferToFixHeaders(postData, postDataLen, &dataToPost, &newDataToPostLen);
        if (!dataToPost) 
          return NS_ERROR_UNEXPECTED;

        // we use nsIStringInputStream::adoptDataa() 
        // in NS_NewPluginPostDataStream to set the stream
        // all new data alloced in  ParsePostBufferToFixHeaders()
        // well be nsMemory::Free()d on destroy the stream 
        postDataLen = newDataToPostLen;
      }

      if (nsnull != target)
        {
          nsPluginInstancePeerImpl *peer;          
          rv = instance->GetPeer(NS_REINTERPRET_CAST(nsIPluginInstancePeer **, &peer));
          
          if (NS_SUCCEEDED(rv))
            {
              nsCOMPtr<nsIPluginInstanceOwner> owner;
              
              rv = peer->GetOwner(*getter_AddRefs(owner));
              
              if (NS_SUCCEEDED(rv))
                {
                  if (!target) {
                    target = "_self";
                  }
                  else {
                    if ((0 == PL_strcmp(target, "newwindow")) || 
                        (0 == PL_strcmp(target, "_new")))
                      target = "_blank";
                    else if (0 == PL_strcmp(target, "_current"))
                      target = "_self";
                  }

                  rv = owner->GetURL(url, target, (void*)dataToPost, postDataLen,
                                     (void*) postHeaders, postHeadersLength, isFile);
                }
              
              NS_RELEASE(peer);
            }
        }
    
      // if we don't have a target, just create a stream.  This does
      // NS_OpenURI()!
      if (streamListener != nsnull)
        rv = NewPluginURLStream(string, instance, streamListener,
                                (const char*)dataToPost, isFile, postDataLen,
                                postHeaders, postHeadersLength);

      NS_RELEASE(instance);
      if (isFile) {
        nsCRT::free(dataToPost);
      }
  }
  
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::RegisterPlugin(REFNSIID aCID,
                                               const char* aPluginName,
                                               const char* aDescription,
                                               const char** aMimeTypes,
                                               const char** aMimeDescriptions,
                                               const char** aFileExtensions,
                                               PRInt32 aCount)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsPluginHostImpl::RegisterPlugin name=%s\n",aPluginName));

  nsCOMPtr<nsIRegistry> registry = do_CreateInstance(kRegistryCID);
  if (! registry)
    return NS_ERROR_FAILURE;

  nsresult rv;
  rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
  if (NS_FAILED(rv)) return rv;

  nsCAutoString path(kPluginsRootKey);
  char* cid = aCID.ToString();
  if (! cid)
    return NS_ERROR_OUT_OF_MEMORY;
  path += '/';
  path += cid;
  nsMemory::Free(cid);

  nsRegistryKey pluginKey;
  rv = registry->AddSubtree(nsIRegistry::Common, path.get(), &pluginKey);
  if (NS_FAILED(rv)) return rv;

  // we use SetBytes instead of SetString to address special character issue, see Mozilla bug 108246
  if(aPluginName)
    registry->SetBytesUTF8(pluginKey, kPluginsNameKey, strlen(aPluginName) + 1, (PRUint8 *)aPluginName);

  if(aDescription)
    registry->SetBytesUTF8(pluginKey, kPluginsDescKey, strlen(aDescription) + 1, (PRUint8 *)aDescription);

  for (PRInt32 i = 0; i < aCount; ++i) {
    nsCAutoString mimepath;
    mimepath.AppendInt(i);

    nsRegistryKey key;
    registry->AddSubtree(pluginKey, mimepath.get(), &key);

    registry->SetStringUTF8(key, kPluginsMimeTypeKey, aMimeTypes[i]);

    if(aMimeDescriptions && aMimeDescriptions[i])
      registry->SetBytesUTF8(key, kPluginsMimeDescKey, 
                             strlen(aMimeDescriptions[i]) + 1, 
                             (PRUint8 *)aMimeDescriptions[i]);

    registry->SetStringUTF8(key, kPluginsMimeExtKey, aFileExtensions[i]);
  }

  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::UnregisterPlugin(REFNSIID aCID)
{
  nsCOMPtr<nsIRegistry> registry = do_CreateInstance(kRegistryCID);
  if (! registry)
    return NS_ERROR_FAILURE;

  nsresult rv;
  rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
  if (NS_FAILED(rv)) return rv;

  nsCAutoString path("software/plugins/");
  char* cid = aCID.ToString();
  if (! cid)
    return NS_ERROR_OUT_OF_MEMORY;

  path += cid;
  nsMemory::Free(cid);

  return registry->RemoveSubtree(nsIRegistry::Common, path.get());
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::BeginWaitCursor(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::EndWaitCursor(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::SupportsURLProtocol(const char* protocol, PRBool *result)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::NotifyStatusChange(nsIPlugin* plugin, nsresult errorStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


/*////////////////////////////////////////////////////////////////////////
 * This method queries the prefs for proxy information.
 * It has been tested and is known to work in the following three cases
 * when no proxy host or port is specified
 * when only the proxy host is specified
 * when only the proxy port is specified
 * This method conforms to the return code specified in 
 * http://developer.netscape.com/docs/manuals/proxy/adminnt/autoconf.htm#1020923
 * with the exception that multiple values are not implemented.
 */

NS_IMETHODIMP nsPluginHostImpl::FindProxyForURL(const char* url, char* *result)
{
  if (!url || !result) {
    return NS_ERROR_INVALID_ARG;
  }
  nsresult res;

  nsCOMPtr<nsIURI> uriIn;
  nsCOMPtr<nsIProtocolProxyService> proxyService;
  nsCOMPtr<nsIIOService> ioService;
  PRBool isProxyEnabled;

  proxyService = do_GetService(kProtocolProxyServiceCID, &res);
  if (NS_FAILED(res) || !proxyService) {
    return res;
  }
  
  if (NS_FAILED(proxyService->GetProxyEnabled(&isProxyEnabled))) {
    return res;
  }

  if (!isProxyEnabled) {
    *result = PL_strdup("DIRECT");
    if (nsnull == *result) {
      res = NS_ERROR_OUT_OF_MEMORY;
    }
    return res;
  }
  
  ioService = do_GetService(kIOServiceCID, &res);
  if (NS_FAILED(res) || !ioService) {
    return res;
  }
  
  // make an nsURI from the argument url
  res = ioService->NewURI(nsDependentCString(url), nsnull, nsnull, getter_AddRefs(uriIn));
  if (NS_FAILED(res)) {
    return res;
  }

  nsCOMPtr<nsIProxyInfo> pi;

  res = proxyService->ExamineForProxy(uriIn, 
                                      getter_AddRefs(pi));
  if (NS_FAILED(res)) {
    return res;
  }

  if (!pi || !pi->Host() || pi->Port() <= 0) {
    *result = PL_strdup("DIRECT");
  } else if (!nsCRT::strcasecmp(pi->Type(), "http")) {
    *result = PR_smprintf("PROXY %s:%d", pi->Host(), pi->Port());
  } else if (!nsCRT::strcasecmp(pi->Type(), "socks4")) {
    *result = PR_smprintf("SOCKS %s:%d", pi->Host(), pi->Port());
  } else if (!nsCRT::strcasecmp(pi->Type(), "socks")) {
    // XXX - this is socks5, but there is no API for us to tell the
    // plugin that fact. SOCKS for now, in case the proxy server
    // speaks SOCKS4 as well. See bug 78176
    // For a long time this was returning an http proxy type, so
    // very little is probably broken by this
    *result = PR_smprintf("SOCKS %s:%d", pi->Host(), pi->Port());
  } else {
    NS_ASSERTION(PR_FALSE, "Unknown proxy type!");
    *result = PL_strdup("DIRECT");
  }

  if (nsnull == *result) {
    res = NS_ERROR_OUT_OF_MEMORY;
  }
  
  return res;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::RegisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::UnregisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::AllocateMenuID(nsIEventHandler* handler, PRBool isSubmenu, PRInt16 *result)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::DeallocateMenuID(nsIEventHandler* handler, PRInt16 menuID)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::HasAllocatedMenuID(nsIEventHandler* handler, PRInt16 menuID, PRBool *result)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::ProcessNextEvent(PRBool *bEventHandled)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::CreateInstance(nsISupports *aOuter,
                                               REFNSIID aIID,
                                               void **aResult)
{
  NS_NOTREACHED("how'd I get here?");
  return NS_ERROR_UNEXPECTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::LockFactory(PRBool aLock)
{
  NS_NOTREACHED("how'd I get here?");
  return NS_ERROR_UNEXPECTED;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::Init(void)
{
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::Destroy(void)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsPluginHostImpl::Destroy Called\n"));

  if (mIsDestroyed)
    return NS_OK;

  mIsDestroyed = PR_TRUE;

  // we should call nsIPluginInstance::Stop and nsIPluginInstance::SetWindow 
  // for those plugins who want it
  mActivePluginList.stopRunning();  

  // at this point nsIPlugin::Shutdown calls will be performed if needed
  mActivePluginList.shut();

  if (nsnull != mPluginPath)
  {
    PR_Free(mPluginPath);
    mPluginPath = nsnull;
  }

  while (nsnull != mPlugins)
  {
    nsPluginTag *temp = mPlugins->mNext;

    // while walking through the list of the plugins see if we still have anything 
    // to shutdown some plugins may have never created an instance but still expect 
    // the shutdown call see bugzilla bug 73071
    // with current logic, no need to do anything special as nsIPlugin::Shutdown 
    // will be performed in the destructor

    delete mPlugins;
    mPlugins = temp;
  }

  // Delete any remaining cached plugins list
  while (mCachedPlugins)
  {
    nsPluginTag *next = mCachedPlugins->mNext;
    delete mCachedPlugins;
    mCachedPlugins = next;
  }

  CleanUnusedLibraries();

  // Lets remove any of the temporary files that we created.
  nsCOMPtr<nsIFile> pluginTmp;
  nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(pluginTmp));
  if (NS_FAILED(rv)) return rv;
    
  rv = pluginTmp->AppendNative(kPluginTmpDirName);
  if (NS_FAILED(rv)) return rv;

  pluginTmp->Remove(PR_TRUE);

  if (mPrivateDirServiceProvider)
  {
    nsCOMPtr<nsIDirectoryService> dirService(do_GetService(kDirectoryServiceContractID, &rv));
    if (NS_SUCCEEDED(rv))
      dirService->UnregisterProvider(mPrivateDirServiceProvider);
    mPrivateDirServiceProvider = nsnull;
  }

  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
/* Called by nsPluginInstanceOwner (nsObjectFrame.cpp - embeded case) */
NS_IMETHODIMP nsPluginHostImpl::InstantiateEmbededPlugin(const char *aMimeType, 
                                                         nsIURI* aURL,
                                                         nsIPluginInstanceOwner *aOwner)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if(aURL != nsnull) (void)aURL->GetAsciiSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHostImpl::InstatiateEmbededPlugin Begin mime=%s, owner=%p, url=%s\n",
        aMimeType, aOwner, urlSpec.get()));

  PR_LogFlush();
#endif

  nsresult  rv;
  nsIPluginInstance *instance = nsnull;
  nsCOMPtr<nsIPluginTagInfo2> pti2;
  nsPluginTagType tagType;
  PRBool isJavaEnabled = PR_TRUE;
  PRBool isJava = PR_FALSE;
  
  rv = aOwner->QueryInterface(kIPluginTagInfo2IID, getter_AddRefs(pti2));
  
  if(rv != NS_OK) {
    return rv;
  }
  
  rv = pti2->GetTagType(&tagType);

  if((rv != NS_OK) || !((tagType == nsPluginTagType_Embed)
                        || (tagType == nsPluginTagType_Applet)
                        || (tagType == nsPluginTagType_Object)))
  {
    return rv;
  }

  if (tagType == nsPluginTagType_Applet || 
      PL_strncasecmp(aMimeType, "application/x-java-vm", 21) == 0 ||
      PL_strncasecmp(aMimeType, "application/x-java-applet", 25) == 0) {
    isJava = PR_TRUE;
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
    // see if java is enabled
    if (prefs) {
      rv = prefs->GetBoolPref("security.enable_java", &isJavaEnabled);
      if (NS_SUCCEEDED(rv)) {
        // if not, don't show this plugin
        if (!isJavaEnabled) {
          return NS_ERROR_FAILURE;
        }
      }
      else {
        // if we were unable to get the pref, assume java is enabled
        // and rely on the "find the plugin or not" logic.
        
        // make sure the value wasn't modified in GetBoolPref
        isJavaEnabled = PR_TRUE;
      }
    }
  }

  // Determine if the scheme of this URL is one we can handle internaly because we should
  // only open the initial stream if it's one that we can handle internally. Otherwise
  // |NS_OpenURI| in |InstantiateEmbededPlugin| may open up a OS protocal registered helper app
  PRBool bCanHandleInternally = PR_FALSE;
  nsCAutoString scheme;
  if (aURL && NS_SUCCEEDED(aURL->GetScheme(scheme))) {
      nsCAutoString contractID(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX);
      contractID += scheme;
      ToLowerCase(contractID);
      nsCOMPtr<nsIProtocolHandler> handler = do_GetService(contractID.get());
      if (handler)
        bCanHandleInternally = PR_TRUE;
  }

  if(FindStoppedPluginForURL(aURL, aOwner) == NS_OK) {

    PLUGIN_LOG(PLUGIN_LOG_NOISY, 
    ("nsPluginHostImpl::InstatiateEmbededPlugin FoundStopped mime=%s\n", aMimeType));

    aOwner->GetInstance(instance);
    if((!aMimeType || !isJava) && bCanHandleInternally)
      rv = NewEmbededPluginStream(aURL, nsnull, instance);

    // notify Java DOM component 
    nsresult res;
    nsCOMPtr<nsIPluginInstanceOwner> javaDOM = 
             do_GetService("@mozilla.org/blackwood/java-dom;1", &res);
    if (NS_SUCCEEDED(res) && javaDOM)
      javaDOM->SetInstance(instance);

    NS_IF_RELEASE(instance);
    return NS_OK;
  }

  // if we don't have a MIME type at this point, we still have one more chance by 
  // opening the stream and seeing if the server hands one back 
  if (!aMimeType)
    return bCanHandleInternally ? NewEmbededPluginStream(aURL, aOwner, nsnull) : NS_ERROR_FAILURE;

  rv = SetUpPluginInstance(aMimeType, aURL, aOwner);

  if(rv == NS_OK)
    rv = aOwner->GetInstance(instance);
  else 
  {
    // We have the mime type either supplied in source or from the header.
    // Let's try to render the default plugin.  See bug 41197
    
    // We were unable to find a plug-in yet we 
    // really do have mime type. Return the error
    // so that the nsObjectFrame can render any 
    // alternate content.

    // but try to load the default plugin first. We need to do this
    // for <embed> tag leaving <object> to play with its alt content.
    // but before we return an error let's see if this is an <embed>
    // tag and try to launch the default plugin

    // but to comply with the spec don't do it for <object> tag
    if(tagType == nsPluginTagType_Object)
      return rv;

    nsresult result;

    result = SetUpDefaultPluginInstance(aMimeType, aURL, aOwner);

    if(result == NS_OK)
      result = aOwner->GetInstance(instance);

    if(result != NS_OK) {
      DisplayNoDefaultPluginDialog(aMimeType);
      return NS_ERROR_FAILURE;
    }

    rv = NS_OK;
  }

  // if we have a failure error, it means we found a plugin for the mimetype,
  // but we had a problem with the entry point
  if(rv == NS_ERROR_FAILURE)
    return rv;

  // if we are here then we have loaded a plugin for this mimetype
  // and it could be the Default plugin
  
  nsPluginWindow    *window = nsnull;

  //we got a plugin built, now stream
  aOwner->GetWindow(window);

  if (nsnull != instance)
  {
    instance->Start();
    aOwner->CreateWidget();

    // If we've got a native window, the let the plugin know about it.
    if (window->window)
      instance->SetWindow(window);

    // create an initial stream with data 
    // don't make the stream if it's a java applet or we don't have SRC or DATA attribute
    PRBool havedata = PR_FALSE;

    nsCOMPtr<nsIPluginTagInfo> pti(do_QueryInterface(aOwner, &rv));
  
    if(pti) {
      const char *value;
      if(tagType == nsPluginTagType_Embed)
        havedata = NS_SUCCEEDED(pti->GetAttribute("SRC", &value));
      if(tagType == nsPluginTagType_Object)
        havedata = NS_SUCCEEDED(pti->GetAttribute("DATA", &value));
    }

    if(havedata && !isJava && bCanHandleInternally)
      rv = NewEmbededPluginStream(aURL, nsnull, instance);

    // notify Java DOM component 
    nsresult res;
    nsCOMPtr<nsIPluginInstanceOwner> javaDOM = 
             do_GetService("@mozilla.org/blackwood/java-dom;1", &res);
    if (NS_SUCCEEDED(res) && javaDOM)
      javaDOM->SetInstance(instance);

    NS_RELEASE(instance);
  }

#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec2;
  if(aURL != nsnull) (void)aURL->GetAsciiSpec(urlSpec2);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHostImpl::InstatiateEmbededPlugin Finished mime=%s, rv=%d, owner=%p, url=%s\n",
        aMimeType, rv, aOwner, urlSpec2.get()));

  PR_LogFlush();
#endif

  return rv;
}


////////////////////////////////////////////////////////////////////////
/* Called by nsPluginViewer.cpp (full-page case) */
NS_IMETHODIMP nsPluginHostImpl::InstantiateFullPagePlugin(const char *aMimeType, 
                                                          nsString& aURLSpec,
                                                          nsIStreamListener *&aStreamListener,
                                                          nsIPluginInstanceOwner *aOwner)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHostImpl::InstatiateFullPagePlugin Begin mime=%s, owner=%p, url=%s\n",
  aMimeType, aOwner, NS_LossyConvertUCS2toASCII(aURLSpec).get()));

  nsresult  rv;
  nsIURI    *url;

  //create a URL so that the instantiator can do file ext.
  //based plugin lookups...
  rv = NS_NewURI(&url, aURLSpec);

  if (rv != NS_OK)
    url = nsnull;

  if(FindStoppedPluginForURL(url, aOwner) == NS_OK) {
    PLUGIN_LOG(PLUGIN_LOG_NOISY,
    ("nsPluginHostImpl::InstatiateFullPagePlugin FoundStopped mime=%s\n",aMimeType));

    nsIPluginInstance* instance;
    aOwner->GetInstance(instance);
    if(!aMimeType || PL_strncasecmp(aMimeType, "application/x-java-vm", 21))
      rv = NewFullPagePluginStream(aStreamListener, instance);
    NS_IF_RELEASE(instance);
    return NS_OK;
  }  

  rv = SetUpPluginInstance(aMimeType, url, aOwner);

  NS_IF_RELEASE(url);

  if (NS_OK == rv)
  {
    nsIPluginInstance *instance = nsnull;
    nsPluginWindow    *window = nsnull;

    aOwner->GetInstance(instance);
    aOwner->GetWindow(window);

    if (nsnull != instance)
    {
      instance->Start();
      aOwner->CreateWidget();

      // If we've got a native window, the let the plugin know about it.
      if (window->window)
        instance->SetWindow(window);

      rv = NewFullPagePluginStream(aStreamListener, instance);

      // If we've got a native window, the let the plugin know about it.
      if (window->window)
        instance->SetWindow(window);

      NS_RELEASE(instance);
    }
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHostImpl::InstatiateFullPagePlugin End mime=%s, rv=%d, owner=%p, url=%s\n",
  aMimeType, rv, aOwner, NS_LossyConvertUCS2toASCII(aURLSpec).get()));

  return rv;
}


////////////////////////////////////////////////////////////////////////
nsresult nsPluginHostImpl::FindStoppedPluginForURL(nsIURI* aURL, 
                                                   nsIPluginInstanceOwner *aOwner)
{
  nsCAutoString url;
  if(!aURL)
    return NS_ERROR_FAILURE;

  (void)aURL->GetAsciiSpec(url);
  
  nsActivePlugin * plugin = mActivePluginList.findStopped(url.get());

  if((plugin != nsnull) && (plugin->mStopped))
  {
    nsIPluginInstance* instance = plugin->mInstance;
    nsPluginWindow    *window = nsnull;
    aOwner->GetWindow(window);

    aOwner->SetInstance(instance);

    // we have to reset the owner and instance in the plugin instance peer
    //instance->GetPeer(&peer);
    ((nsPluginInstancePeerImpl*)plugin->mPeer)->SetOwner(aOwner);

    instance->Start();
    aOwner->CreateWidget();

    // If we've got a native window, the let the plugin know about it.
    if (window->window)
      instance->SetWindow(window);

    plugin->setStopped(PR_FALSE);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}


////////////////////////////////////////////////////////////////////////
void nsPluginHostImpl::AddInstanceToActiveList(nsCOMPtr<nsIPlugin> aPlugin,
                                               nsIPluginInstance* aInstance,
                                               nsIURI* aURL,
                                               PRBool aDefaultPlugin)

{
  nsCAutoString url;

  if(!aURL)
    return;

  (void)aURL->GetSpec(url);

  // find corresponding plugin tag
  // this is legal for xpcom plugins not to have nsIPlugin implemented
  nsPluginTag * pluginTag = nsnull;
  if(aPlugin)
  {
    for(pluginTag = mPlugins; pluginTag != nsnull; pluginTag = pluginTag->mNext)
    {
      if(pluginTag->mEntryPoint == aPlugin)
        break;
    }
    NS_ASSERTION(pluginTag, "Plugin tag not found");
  }
  else
  {
    // we don't need it for xpcom plugins because the only purpose to have it
    // is to be able to postpone unloading library dll in some circumstances
    // which we don't do for xpcom plugins. In case we need it in the future
    // we can probably use the following
    /*
    FindPluginEnabledForType(mimetype, pluginTag);
    */
  }

  nsActivePlugin * plugin = new nsActivePlugin(pluginTag, aInstance, url.get(), aDefaultPlugin);

  if(plugin == nsnull)
    return;

  mActivePluginList.add(plugin);
}


////////////////////////////////////////////////////////////////////////
nsresult nsPluginHostImpl::RegisterPluginMimeTypesWithLayout(nsPluginTag * pluginTag, 
                                                             nsIComponentManager * compManager, 
                                                             nsIFile * path)
{
  NS_ENSURE_ARG_POINTER(pluginTag);
  NS_ENSURE_ARG_POINTER(pluginTag->mMimeTypeArray);
  NS_ENSURE_ARG_POINTER(compManager);

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHostImpl::RegisterPluginMimeTypesWithLayout plugin=%s\n",
  pluginTag->mFileName));

  nsresult rv = NS_OK;
  // what I want to do here is QI for a Component Registration Manager.  Since this 
  // has not been invented yet, QI to the obsolete manager.  Kids, don't do this at home.
  nsCOMPtr<nsIComponentManagerObsolete> obsoleteManager = do_QueryInterface(compManager, &rv);
  if (!obsoleteManager)
    return rv;

  nsCOMPtr<imgILoader> loader;
  if (!mOverrideInternalTypes)
    loader = do_GetService("@mozilla.org/image/loader;1");
    
  for(int i = 0; i < pluginTag->mVariants; i++) {

    // Do not register any doc loader factory content viewers for mime types we do internally
    // Note: This excludes plugins of these mime types from running in full-page mode.
    // This gets around Quicktime's default handling of PNG's
    PRBool bIsSupportedImage = PR_FALSE;
    if (!mOverrideInternalTypes && 
        NS_SUCCEEDED(loader->SupportImageWithMimeType(pluginTag->mMimeTypeArray[i], &bIsSupportedImage)) && 
        bIsSupportedImage)
      continue;
    
    static NS_DEFINE_CID(kPluginDocLoaderFactoryCID, NS_PLUGINDOCLOADERFACTORY_CID);

    nsCAutoString contractid(NS_DOCUMENT_LOADER_FACTORY_CONTRACTID_PREFIX "view;1?type=");
    contractid += pluginTag->mMimeTypeArray[i];


    rv = obsoleteManager->RegisterComponentSpec(kPluginDocLoaderFactoryCID,
                                                "Plugin Loader Stub",
                                                contractid.get(),
                                                path,
                                                PR_TRUE,
                                                PR_FALSE);

    PLUGIN_LOG(PLUGIN_LOG_NOISY,
    ("nsPluginHostImpl::RegisterPluginMimeTypesWithLayout mime=%s, plugin=%s\n",
    pluginTag->mMimeTypeArray[i], pluginTag->mFileName));
  }

  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::SetUpPluginInstance(const char *aMimeType, 
                                                    nsIURI *aURL,
                                                    nsIPluginInstanceOwner *aOwner)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if(aURL != nsnull) (void)aURL->GetSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHostImpl::SetupPluginInstance Begin mime=%s, owner=%p, url=%s\n",
        aMimeType, aOwner, urlSpec.get()));

  PR_LogFlush();
#endif


  nsresult result = NS_ERROR_FAILURE;
  nsIPluginInstance* instance = NULL;
  nsCOMPtr<nsIPlugin> plugin;
  const char* mimetype;

  if(!aURL)
    return NS_ERROR_FAILURE;

  // if don't have a mimetype, check by file extension
  if(!aMimeType)
  {
    char* extension;

    nsCAutoString filename;
    aURL->GetPath(filename);
    extension = PL_strrchr(filename.get(), '.');
    if(extension)
      ++extension;
    else
      return NS_ERROR_FAILURE;

    if(IsPluginEnabledForExtension(extension, mimetype) != NS_OK)
      return NS_ERROR_FAILURE;
  }
  else
    mimetype = aMimeType;

  PRBool isJavaPlugin = PR_FALSE;
  if (aMimeType && 
      (PL_strncasecmp(aMimeType, "application/x-java-vm", 21) == 0 ||
         PL_strncasecmp(aMimeType, "application/x-java-applet", 25) == 0))
  {
    isJavaPlugin = PR_TRUE;
  }

#if (defined(XP_UNIX) && !defined(XP_MACOSX)) || defined(XP_OS2)
  // This is a work-around on Unix for a LiveConnect problem (bug 83698).
  // The problem:
  // The proxy JNI needs to be created by the browser. If it is created by
  // someone else (e.g., a plugin) on a different thread, the proxy JNI will
  // not work, and break LiveConnect.
  // Currently, on Unix, when instantiating a Java plugin instance (by calling
  // InstantiateEmbededPlugin() next), Java plugin will create the proxy JNI
  // if it is not created yet. If that happens, LiveConnect will be broken.
  // Before lazy start JVM was implemented, since at this point the browser
  // already created the proxy JNI buring startup, the problem did not happen.
  // But after the lazy start was implemented, at this point the proxy JNI was
  // not created yet, so the Java plugin created the proxy JNI, and broke
  // liveConnect.
  // On Windows and Mac, Java plugin does not create the proxy JNI, but lets
  // the browser to create it. Hence this is a Unix-only problem.
  //
  // The work-around:
  // The root cause of the problem is in Java plugin's Unix implementation,
  // which should not create the proxy JNI.
  // As a work-around, here we make sure the proxy JNI has been created by the
  // browser, before plugin gets a chance.
  //

  if (isJavaPlugin) {
    // If Java is installed, get proxy JNI. 
    nsCOMPtr<nsIJVMManager> jvmManager = do_GetService(nsIJVMManager::GetCID(),
                                                     &result);
    if (NS_SUCCEEDED(result)) {
      JNIEnv* proxyEnv;
      // Get proxy JNI, if not created yet, create it.
      jvmManager->GetProxyJNI(&proxyEnv);
    }
  }
#endif

  nsCAutoString contractID(
          NS_LITERAL_CSTRING(NS_INLINE_PLUGIN_CONTRACTID_PREFIX) +
          nsDependentCString(mimetype));

  GetPluginFactory(mimetype, getter_AddRefs(plugin));

  result = CallCreateInstance(contractID.get(), &instance);

    // couldn't create an XPCOM plugin, try to create wrapper for a legacy plugin
    if (NS_FAILED(result)) 
    {
      if(plugin)
      { 
#ifdef XP_WIN
        static BOOL firstJavaPlugin = FALSE;
        BOOL restoreOrigDir = FALSE;
        char origDir[_MAX_PATH];
        if (isJavaPlugin && !firstJavaPlugin)
        {
          DWORD dw = ::GetCurrentDirectory(_MAX_PATH, origDir);
          NS_ASSERTION(dw <= _MAX_PATH, "Falied to obtain the current directory, which may leads to incorrect class laoding");
          nsCOMPtr<nsIFile> binDirectory;
          result = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR, 
                                          getter_AddRefs(binDirectory));

          if (NS_SUCCEEDED(result))
          {
              nsCAutoString path;
              binDirectory->GetNativePath(path);
              restoreOrigDir = ::SetCurrentDirectory(path.get());
          }
        }
#endif
        result = plugin->CreateInstance(NULL, kIPluginInstanceIID, (void **)&instance);

#ifdef XP_WIN
        if (!firstJavaPlugin && restoreOrigDir)
        {
          BOOL bCheck = :: SetCurrentDirectory(origDir);
          NS_ASSERTION(bCheck, " Error restoring driectoy");
          firstJavaPlugin = TRUE;
        }
#endif
      }
      if (NS_FAILED(result)) 
      {
        nsCOMPtr<nsIPlugin> bwPlugin = 
                 do_GetService("@mozilla.org/blackwood/pluglet-engine;1", &result);
        if (NS_SUCCEEDED(result)) 
        {
          result = bwPlugin->CreatePluginInstance(NULL,
                                                  kIPluginInstanceIID,
                                                  aMimeType,
                                                  (void **)&instance);
        }
      }
    }

    // neither an XPCOM or legacy plugin could be instantiated, 
    // so return the failure
    if (NS_FAILED(result))
      return result;

    // it is adreffed here
    aOwner->SetInstance(instance);

    nsPluginInstancePeerImpl *peer = new nsPluginInstancePeerImpl();
    if(peer == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;

    // set up the peer for the instance
    peer->Initialize(aOwner, mimetype);   

    nsIPluginInstancePeer *pi;

    result = peer->QueryInterface(kIPluginInstancePeerIID, (void **)&pi);

    if(result != NS_OK)
      return result;

    // tell the plugin instance to initialize itself and pass in the peer.
    instance->Initialize(pi);  // this will not add a ref to the instance (or owner). MMP

    NS_RELEASE(pi);

    // we should addref here
    AddInstanceToActiveList(plugin, instance, aURL, PR_FALSE);

    //release what was addreffed in Create(Plugin)Instance
    NS_RELEASE(instance);

#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec2;
  if(aURL != nsnull) (void)aURL->GetSpec(urlSpec2);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_BASIC,
        ("nsPluginHostImpl::SetupPluginInstance Finished mime=%s, rv=%d, owner=%p, url=%s\n",
        aMimeType, result, aOwner, urlSpec2.get()));

  PR_LogFlush();
#endif

    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
nsresult nsPluginHostImpl::SetUpDefaultPluginInstance(const char *aMimeType, nsIURI *aURL,
                                                      nsIPluginInstanceOwner *aOwner)
{
  nsresult result = NS_ERROR_FAILURE;
  nsIPluginInstance* instance = NULL;
  nsCOMPtr<nsIPlugin> plugin = NULL;
  const char* mimetype;

  if(!aURL)
    return NS_ERROR_FAILURE;

  mimetype = aMimeType;

  GetPluginFactory("*", getter_AddRefs(plugin));

  result = CallCreateInstance(NS_INLINE_PLUGIN_CONTRACTID_PREFIX "*",
                              &instance);

  // couldn't create an XPCOM plugin, try to create wrapper for a legacy plugin
  if (NS_FAILED(result)) 
  {
    if(plugin)
      result = plugin->CreateInstance(NULL, kIPluginInstanceIID, (void **)&instance);
  }

  // neither an XPCOM or legacy plugin could be instantiated, so return the failure
  if(NS_FAILED(result))
    return result;

  // it is adreffed here
  aOwner->SetInstance(instance);

  nsPluginInstancePeerImpl *peer = new nsPluginInstancePeerImpl();
  if(peer == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  // if we don't have a mimetype, check by file extension
  nsXPIDLCString mt;
  if(mimetype == nsnull)
  {
    nsresult res = NS_OK;
    nsCOMPtr<nsIURL> url = do_QueryInterface(aURL);
    if(url)
    {
      nsCAutoString extension;
      url->GetFileExtension(extension);
    
      if(!extension.IsEmpty())
      {
        nsCOMPtr<nsIMIMEService> ms (do_GetService(NS_MIMESERVICE_CONTRACTID, &res));
        if(NS_SUCCEEDED(res) && ms)
        {
          res = ms->GetTypeFromExtension(extension.get(), getter_Copies(mt));
          if(NS_SUCCEEDED(res))
            mimetype = mt;
        }
      }
    }
  }

  // set up the peer for the instance
  peer->Initialize(aOwner, mimetype);   

  nsIPluginInstancePeer *pi;

  result = peer->QueryInterface(kIPluginInstancePeerIID, (void **)&pi);

  if(result != NS_OK)
    return result;

  // tell the plugin instance to initialize itself and pass in the peer.
  instance->Initialize(pi);  // this will not add a ref to the instance (or owner). MMP

  NS_RELEASE(pi);

  // we should addref here
  AddInstanceToActiveList(plugin, instance, aURL, PR_TRUE);

  //release what was addreffed in Create(Plugin)Instance
  NS_RELEASE(instance);

  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginHostImpl::IsPluginEnabledForType(const char* aMimeType)
{
  nsPluginTag *plugins = nsnull;
  PRInt32     variants, cnt;

  LoadPlugins();

  // if we have a mimetype passed in, search the mPlugins linked 
  // list for a match
  if (nsnull != aMimeType)
  {
    plugins = mPlugins;

    while (nsnull != plugins)
    {
      variants = plugins->mVariants;

      for (cnt = 0; cnt < variants; cnt++)
        if (plugins->mMimeTypeArray[cnt] && (0 == PL_strcasecmp(plugins->mMimeTypeArray[cnt], aMimeType)))
          return NS_OK;

      if (cnt < variants)
        break;

      plugins = plugins->mNext;
    }
  }

  return NS_ERROR_FAILURE;
}


////////////////////////////////////////////////////////////////////////
// check comma delimitered extensions
static int CompareExtensions(const char *aExtensionList, const char *aExtension)
{
  if((aExtensionList == nsnull) || (aExtension == nsnull))
    return -1;

  const char *pExt = aExtensionList;
  const char *pComma = strchr(pExt, ',');

  if(pComma == nsnull)
    return PL_strcasecmp(pExt, aExtension);

  while(pComma != nsnull)
  {
    int length = pComma - pExt;
    if(0 == PL_strncasecmp(pExt, aExtension, length))
      return 0;

    pComma++;
    pExt = pComma;
    pComma = strchr(pExt, ',');
  }

  // the last one
  return PL_strcasecmp(pExt, aExtension);
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginHostImpl::IsPluginEnabledForExtension(const char* aExtension, 
                                              const char* &aMimeType)
{
  nsPluginTag *plugins = nsnull;
  PRInt32     variants, cnt;

  LoadPlugins();

  // if we have a mimetype passed in, search the mPlugins linked 
  // list for a match
  if (nsnull != aExtension)
  {
    plugins = mPlugins;

    while (nsnull != plugins)
    {
      variants = plugins->mVariants;

      for (cnt = 0; cnt < variants; cnt++)
      {
        //if (0 == strcmp(plugins->mExtensionsArray[cnt], aExtension))
        // mExtensionsArray[cnt] could be not a single extension but 
        // rather a list separated by commas
        if (0 == CompareExtensions(plugins->mExtensionsArray[cnt], aExtension))
        {
          aMimeType = plugins->mMimeTypeArray[cnt];
          return NS_OK;
        }
      }

      if (cnt < variants)
        break;

      plugins = plugins->mNext;
    }
  }

  return NS_ERROR_FAILURE;
}


////////////////////////////////////////////////////////////////////////
// Utility functions for a charset convertor 
// which converts platform charset to unicode.

static nsresult CreateUnicodeDecoder(nsIUnicodeDecoder **aUnicodeDecoder)
{
  nsresult rv;
  // get the charset
  nsAutoString platformCharset;
  nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = platformCharsetService->GetCharset(kPlatformCharsetSel_FileName, platformCharset);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the decoder
  nsCOMPtr<nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ccm->GetUnicodeDecoder(&platformCharset, aUnicodeDecoder);

  return rv;
}

static nsresult DoCharsetConversion(nsIUnicodeDecoder *aUnicodeDecoder, 
                                     const char* aANSIString, nsAString& aUnicodeString)
{
  NS_ENSURE_TRUE(aUnicodeDecoder, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(aANSIString, NS_ERROR_FAILURE);
  nsresult rv;

  PRInt32 numberOfBytes = strlen(aANSIString);
  PRInt32 outUnicodeLen;
  nsAutoString buffer;
  rv = aUnicodeDecoder->GetMaxLength(aANSIString, numberOfBytes, &outUnicodeLen);
  NS_ENSURE_SUCCESS(rv, rv);
  buffer.SetCapacity(outUnicodeLen);
  rv = aUnicodeDecoder->Convert(aANSIString, &numberOfBytes, (PRUnichar*) buffer.get(), &outUnicodeLen);
  NS_ENSURE_SUCCESS(rv, rv);
  buffer.SetLength(outUnicodeLen);
  aUnicodeString = buffer;

  return rv;
}

////////////////////////////////////////////////////////////////////////

class DOMMimeTypeImpl : public nsIDOMMimeType {
public:
  NS_DECL_ISUPPORTS

  DOMMimeTypeImpl(nsPluginTag* aPluginTag, PRUint32 aMimeTypeIndex)
  {
    NS_INIT_ISUPPORTS();
    (void) CreateUnicodeDecoder(getter_AddRefs(mUnicodeDecoder));
    if (aPluginTag) {
      if (aPluginTag->mMimeDescriptionArray)
        (void) DoCharsetConversion(mUnicodeDecoder,
                                   aPluginTag->mMimeDescriptionArray[aMimeTypeIndex], mDescription);
      if (aPluginTag->mExtensionsArray)
        mSuffixes.AssignWithConversion(aPluginTag->mExtensionsArray[aMimeTypeIndex]);
      if (aPluginTag->mMimeTypeArray)
        mType.AssignWithConversion(aPluginTag->mMimeTypeArray[aMimeTypeIndex]);
    }
  }
  
  virtual ~DOMMimeTypeImpl() {
  }

  NS_METHOD GetDescription(nsAString& aDescription)
  {
    aDescription.Assign(mDescription);
    return NS_OK;
  }

  NS_METHOD GetEnabledPlugin(nsIDOMPlugin** aEnabledPlugin)
  {
    // this has to be implemented by the DOM version.
    *aEnabledPlugin = nsnull;
    return NS_OK;
  }

  NS_METHOD GetSuffixes(nsAString& aSuffixes)
  {
    aSuffixes.Assign(mSuffixes);
    return NS_OK;
  }

  NS_METHOD GetType(nsAString& aType)
  {
    aType.Assign(mType);
    return NS_OK;
  }

private:
  nsString mDescription;
  nsString mSuffixes;
  nsString mType;
  nsCOMPtr<nsIUnicodeDecoder> mUnicodeDecoder;
};


////////////////////////////////////////////////////////////////////////
NS_IMPL_ISUPPORTS1(DOMMimeTypeImpl, nsIDOMMimeType)
////////////////////////////////////////////////////////////////////////
class DOMPluginImpl : public nsIDOMPlugin {
public:
  NS_DECL_ISUPPORTS
  
  DOMPluginImpl(nsPluginTag* aPluginTag) : mPluginTag(aPluginTag)
  {
    NS_INIT_ISUPPORTS();

    (void) CreateUnicodeDecoder(getter_AddRefs(mUnicodeDecoder));
  }
  
  virtual ~DOMPluginImpl() {
  }

  NS_METHOD GetDescription(nsAString& aDescription)
  {
    return DoCharsetConversion(mUnicodeDecoder, mPluginTag.mDescription, aDescription);
  }

  NS_METHOD GetFilename(nsAString& aFilename)
  {
#ifdef XP_MAC
    return DoCharsetConversion(mUnicodeDecoder, mPluginTag.mFullPath, aFilename);
#else        
    return DoCharsetConversion(mUnicodeDecoder, mPluginTag.mFileName, aFilename);
#endif
  }

  NS_METHOD GetName(nsAString& aName)
  {
    return DoCharsetConversion(mUnicodeDecoder, mPluginTag.mName, aName);
  }

  NS_METHOD GetLength(PRUint32* aLength)
  {
    *aLength = mPluginTag.mVariants;
    return NS_OK;
  }

  NS_METHOD Item(PRUint32 aIndex, nsIDOMMimeType** aReturn)
  {
    nsIDOMMimeType* mimeType = new DOMMimeTypeImpl(&mPluginTag, aIndex);
    NS_IF_ADDREF(mimeType);
    *aReturn = mimeType;
    return NS_OK;
  }

  NS_METHOD NamedItem(const nsAString& aName, nsIDOMMimeType** aReturn)
  {
    for (int index = mPluginTag.mVariants - 1; index >= 0; --index) {
      if (aName.Equals(NS_ConvertASCIItoUCS2(mPluginTag.mMimeTypeArray[index])))
        return Item(index, aReturn);
    }
    return NS_OK;
  }

private:
  nsPluginTag mPluginTag;
  nsCOMPtr<nsIUnicodeDecoder> mUnicodeDecoder;
};

////////////////////////////////////////////////////////////////////////
NS_IMPL_ISUPPORTS1(DOMPluginImpl, nsIDOMPlugin)
////////////////////////////////////////////////////////////////////////


NS_IMETHODIMP
nsPluginHostImpl::GetPluginCount(PRUint32* aPluginCount)
{
  LoadPlugins();

  PRUint32 count = 0;

  nsPluginTag* plugin = mPlugins;
  while (plugin != nsnull) {
    ++count;
    plugin = plugin->mNext;
  }

  *aPluginCount = count;

  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginHostImpl::GetPlugins(PRUint32 aPluginCount, 
                             nsIDOMPlugin* aPluginArray[])
{
  LoadPlugins();
  
  nsPluginTag* plugin = mPlugins;
  for (PRUint32 i = 0; i < aPluginCount && plugin != nsnull; 
       i++, plugin = plugin->mNext) {
    nsIDOMPlugin* domPlugin = new DOMPluginImpl(plugin);
    NS_IF_ADDREF(domPlugin);
    aPluginArray[i] = domPlugin;
  }
  
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
nsresult
nsPluginHostImpl::FindPluginEnabledForType(const char* aMimeType, 
                                           nsPluginTag* &aPlugin)
{
  nsPluginTag *plugins = nsnull;
  PRInt32     variants, cnt;
  
  aPlugin = nsnull;
  
  LoadPlugins();
  
  // if we have a mimetype passed in, search the mPlugins 
  // linked list for a match
  if (nsnull != aMimeType) {
    plugins = mPlugins;
    
    while (nsnull != plugins) {
      variants = plugins->mVariants;
      
      for (cnt = 0; cnt < variants; cnt++) {
        if (plugins->mMimeTypeArray[cnt] && (0 == PL_strcasecmp(plugins->mMimeTypeArray[cnt], aMimeType))) {
          aPlugin = plugins;
          return NS_OK;
        }
      }

      if (cnt < variants)
        break;
    
      plugins = plugins->mNext;
    }
  }

  return NS_ERROR_FAILURE;
}

////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::GetPluginFactory(const char *aMimeType, nsIPlugin** aPlugin)
{
  nsresult rv = NS_ERROR_FAILURE;
  *aPlugin = NULL;

  if(!aMimeType)
    return NS_ERROR_ILLEGAL_VALUE;

  // unload any libs that can remain after plugins.refresh(1), see #61388
  CleanUnusedLibraries();

  // If plugins haven't been scanned yet, do so now
  LoadPlugins();

  nsPluginTag* pluginTag;
  if((rv = FindPluginEnabledForType(aMimeType, pluginTag)) == NS_OK)
  {
    PLUGIN_LOG(PLUGIN_LOG_BASIC,
    ("nsPluginHostImpl::GetPluginFactory Begin mime=%s, plugin=%s\n",
    aMimeType, pluginTag->mFileName));

#ifdef NS_DEBUG
    if(aMimeType && pluginTag->mFileName)
      printf("For %s found plugin %s\n", aMimeType, pluginTag->mFileName);
#endif

    if (nsnull == pluginTag->mLibrary)  // if we haven't done this yet
    {
#if 0 /* !defined(XP_MAC) */
      nsFileSpec file(pluginTag->mFileName);
#else
      if (nsnull == pluginTag->mFullPath)
        return NS_ERROR_FAILURE;
      nsFileSpec file(pluginTag->mFullPath);
#endif
      nsPluginFile pluginFile(file);
      PRLibrary* pluginLibrary = NULL;

      if (pluginFile.LoadPlugin(pluginLibrary) != NS_OK || pluginLibrary == NULL)
        return NS_ERROR_FAILURE;

      pluginTag->mLibrary = pluginLibrary;
    }

    nsIPlugin* plugin = pluginTag->mEntryPoint;
    if(plugin == NULL)
    {
      // nsIPlugin* of xpcom plugins can be found thru a call to
      //  nsComponentManager::GetClassObject()
      nsCID clsid;
      nsCAutoString contractID(
              NS_LITERAL_CSTRING(NS_INLINE_PLUGIN_CONTRACTID_PREFIX) +
              nsDependentCString(aMimeType));
      nsresult rv =
          nsComponentManager::ContractIDToClassID(contractID.get(), &clsid);
      if (NS_SUCCEEDED(rv))
      {
        rv = nsComponentManager::GetClassObject(clsid, nsIPlugin::GetIID(), (void**)&plugin);
        if (NS_SUCCEEDED(rv) && plugin)
        {
          // plugin was addref'd by nsComponentManager::GetClassObject
          pluginTag->mEntryPoint = plugin;
          plugin->Initialize();
        }
      }
    }

    if (plugin == NULL)
    {
      // No, this is not a leak. GetGlobalServiceManager() doesn't
      // addref the pointer on the way out. It probably should.
      nsIServiceManagerObsolete* serviceManager;
      nsServiceManager::GetGlobalServiceManager((nsIServiceManager**)&serviceManager);

      // need to get the plugin factory from this plugin.
      nsFactoryProc nsGetFactory = nsnull;
      nsGetFactory = (nsFactoryProc) PR_FindSymbol(pluginTag->mLibrary, "NSGetFactory");
      if(nsGetFactory != nsnull)
      {
        rv = nsGetFactory(serviceManager, kPluginCID, nsnull, nsnull,    // XXX fix ClassName/ContractID
                          (nsIFactory**)&pluginTag->mEntryPoint);
        plugin = pluginTag->mEntryPoint;
        if (plugin != NULL)
          plugin->Initialize();
      }
      else
      {
       // Now lets try to get the entry point from a 4.x plugin
       rv = NS_ERROR_FAILURE;
#if defined(XP_MAC) && TARGET_CARBON
        // on Carbon, first let's see if this is a Classic plugin
        // should we also look for a 'carb' resource?
        if (PR_FindSymbol(pluginTag->mLibrary, "mainRD") != NULL) 
        {
          nsCOMPtr<nsIClassicPluginFactory> factory = 
                   do_GetService(NS_CLASSIC_PLUGIN_FACTORY_CONTRACTID, &rv);
          if (NS_SUCCEEDED(rv)) 
            rv = factory->CreatePlugin(serviceManager, 
                                       pluginTag->mFileName, 
                                       pluginTag->mFullPath,
                                       pluginTag->mLibrary, 
                                       &pluginTag->mEntryPoint);
          if (!pluginTag->mEntryPoint)  // plugin wasn't found
            rv = NS_ERROR_FAILURE;      // setup failure to try normal loading next
        } 
#endif
        if (NS_FAILED(rv))
          rv = ns4xPlugin::CreatePlugin(serviceManager,
                                        pluginTag->mFileName,
                                        pluginTag->mFullPath,
                                        pluginTag->mLibrary,
                                        &pluginTag->mEntryPoint);

        plugin = pluginTag->mEntryPoint;
        pluginTag->mFlags |= NS_PLUGIN_FLAG_OLDSCHOOL;
        // no need to initialize, already done by CreatePlugin()
      }
    }

    if (plugin != nsnull)
    {
      *aPlugin = plugin;
      plugin->AddRef();
      return NS_OK;
    }
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHostImpl::GetPluginFactory End mime=%s, rv=%d, plugin=%p name=%s\n",
  aMimeType, rv, *aPlugin, (pluginTag ? pluginTag->mFileName : "(not found)")));

  return rv;
}


////////////////////////////////////////////////////////////////////////
// XXX called from ScanPluginsDirectory only when told to filter
// currently 'unwanted' plugins are Java, and all other plugins except
// Acrobat, Flash, Quicktime and Shockwave
static PRBool isUnwantedPlugin(nsPluginTag * tag)
{
  if(tag->mFileName == nsnull)
    return PR_TRUE;

  for (PRInt32 i = 0; i < tag->mVariants; ++i) {
    if(nsnull == PL_strcasecmp(tag->mMimeTypeArray[i], "application/pdf"))
      return PR_FALSE;

    if(nsnull == PL_strcasecmp(tag->mMimeTypeArray[i], "application/x-shockwave-flash"))
      return PR_FALSE;

    if(nsnull == PL_strcasecmp(tag->mMimeTypeArray[i],"application/x-director"))
      return PR_FALSE;
  }

  // On Windows, we also want to include the Quicktime plugin from the 4.x directory
  // But because it spans several DLL's, the best check for now is by filename
  if (nsnull != PL_strcasestr(tag->mFileName,"npqtplugin"))
    return PR_FALSE;

  return PR_TRUE;
}

nsPluginTag * nsPluginHostImpl::HaveSamePlugin(nsPluginTag * aPluginTag)
{
  for(nsPluginTag* tag = mPlugins; tag; tag = tag->mNext) {
    if(tag->Equals(aPluginTag))
      return tag;
  }
  return nsnull;
}

PRBool nsPluginHostImpl::IsDuplicatePlugin(nsPluginTag * aPluginTag)
{
  nsPluginTag * tag = HaveSamePlugin(aPluginTag);
  if (tag) {
    // if we got the same plugin, check the full path to see if this is a dup;

    // mFileName contains full path on Windows and Unix and leaf name on Mac
    // if those are not equal, we have the same plugin with  different path,
    // i.e. duplicate, return true
    if (PL_strcmp(tag->mFileName, aPluginTag->mFileName))
      return PR_TRUE;

    // if they are equal, compare mFullPath fields just in case 
    // mFileName contained leaf name only, and if not equal, return true
    if (tag->mFullPath && aPluginTag->mFullPath && PL_strcmp(tag->mFullPath, aPluginTag->mFullPath))
      return PR_TRUE;
  }

  // we do not have it at all, return false
  return PR_FALSE;
}

// Structure for collecting plugin files found during directory scanning
struct pluginFileinDirectory
{
  nsString mFilename;
  PRInt64  mModTime;

  pluginFileinDirectory()
  {
    mModTime = LL_ZERO;
  }
};

// QuickSort callback for comparing the modification time of two files
// if the times are the same, compare the filenames
static int PR_CALLBACK ComparePluginFileInDirectory (const void *v1, const void *v2, void *)
{
  const pluginFileinDirectory* pfd1 = NS_STATIC_CAST(const pluginFileinDirectory*, v1);
  const pluginFileinDirectory* pfd2 = NS_STATIC_CAST(const pluginFileinDirectory*, v2);

  PRInt32 result = 0;
  if (LL_EQ(pfd1->mModTime, pfd2->mModTime))
    result = Compare(pfd1->mFilename, pfd2->mFilename, nsCaseInsensitiveStringComparator());
  else if (LL_CMP(pfd1->mModTime, >, pfd2->mModTime))
    result = -1;
  else 
    result = 1;

  return result;
}

typedef NS_4XPLUGIN_CALLBACK(char *, NP_GETMIMEDESCRIPTION)(void);

static nsresult FixUpPluginInfo(nsPluginInfo &aInfo, nsPluginFile &aPluginFile)
{
#ifndef XP_WIN
  return NS_OK;
#endif

  for (PRUint32 i = 0; i < aInfo.fVariantCount; i++) {
    if (PL_strcmp(aInfo.fMimeTypeArray[i], "*"))
      continue;

    // we got "*" type 
    // check if this is an alien plugin (not our default plugin) 
    // by trying to find a special entry point
    PRLibrary *library = nsnull;
    if (NS_FAILED(aPluginFile.LoadPlugin(library)) || !library)
      return NS_ERROR_FAILURE;

    NP_GETMIMEDESCRIPTION pf = (NP_GETMIMEDESCRIPTION)PR_FindSymbol(library, "NP_GetMIMEDescription");
    if (pf) {
      // if we found it, this is the default plugin, return
      char * mimedescription = pf();
      if (!PL_strncmp(mimedescription, NS_PLUGIN_DEFAULT_MIME_DESCRIPTION, 1))
        return NS_OK;
    }

    // if we are here that means we have an alien plugin 
    // which wants to take over "*" type

    // change its "*" mime type to "[*]"
    PL_strfree(aInfo.fMimeTypeArray[i]);
    aInfo.fMimeTypeArray[i] = PL_strdup("[*]");

    // continue the loop?
  }
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
nsresult nsPluginHostImpl::ScanPluginsDirectory(nsIFile * pluginsDir, 
                                                nsIComponentManager * compManager, 
                                                nsIFile * layoutPath,
                                                PRBool aCreatePluginList,
                                                PRBool * aPluginsChanged,
                                                PRBool checkForUnwantedPlugins)
{
  NS_ENSURE_ARG_POINTER(aPluginsChanged);
  nsresult rv;

  *aPluginsChanged = PR_FALSE;

#ifdef PLUGIN_LOGGING
  nsCAutoString dirPath;
  pluginsDir->GetNativePath(dirPath);
  PLUGIN_LOG(PLUGIN_LOG_BASIC,
  ("nsPluginHostImpl::ScanPluginsDirectory dir=%s\n", dirPath.get()));
#endif

  nsCOMPtr<nsISimpleEnumerator> iter;
  rv = pluginsDir->GetDirectoryEntries(getter_AddRefs(iter));
  if (NS_FAILED(rv))
    return rv;

  // Collect all the files in this directory in a void array we can sort later
  nsAutoVoidArray pluginFilesArray;  // array for sorting files in this directory
  PRBool hasMore;
  while (NS_SUCCEEDED(iter->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = iter->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv))
      continue;
    nsCOMPtr<nsILocalFile> dirEntry(do_QueryInterface(supports, &rv));
    if (NS_FAILED(rv))
      continue;
    nsCAutoString filePath;
    rv = dirEntry->GetNativePath(filePath);
    if (NS_FAILED(rv))
      continue;
    
    nsFileSpec file(filePath.get());
    PRBool wasSymlink;  
    file.ResolveSymlink(wasSymlink);

    if (nsPluginsDir::IsPluginFile(file)) {
      pluginFileinDirectory * item = new pluginFileinDirectory();
      if (!item) 
        return NS_ERROR_OUT_OF_MEMORY;
    
      // Get file mod time
      PRInt64 fileModTime = LL_ZERO;
      dirEntry->GetLastModifiedTime(&fileModTime);

      item->mModTime = fileModTime;
      item->mFilename.AssignWithConversion(file.GetCString());
      pluginFilesArray.AppendElement(item);
    }
  } // end round of up of plugin files

  // now sort the array by file modification time or by filename, if equal
  // put newer plugins first to weed out dups and catch upgrades, see bug 119966
  pluginFilesArray.Sort(ComparePluginFileInDirectory, nsnull);

  // finally, go through the array, looking at each entry and continue processing it
  for (PRInt32 i = 0; i < pluginFilesArray.Count(); i++) {
    pluginFileinDirectory* pfd = NS_STATIC_CAST(pluginFileinDirectory*, pluginFilesArray[i]);
    nsFileSpec file(pfd->mFilename);
    PRInt64 fileModTime = pfd->mModTime;
    delete pfd;

    // Look for it in our cache
    nsPluginTag *pluginTag = RemoveCachedPluginsInfo(file.GetCString());

    if (pluginTag) {
      // If plugin changed, delete cachedPluginTag and dont use cache
      if (LL_NE(fileModTime, pluginTag->mLastModifiedTime)) {
        // Plugins has changed. Dont use cached plugin info.
        delete pluginTag;
        pluginTag = nsnull;

        // plugin file changed, flag this fact
        *aPluginsChanged = PR_TRUE;
      }
      else {
        // if it is unwanted plugin we are checking for, get it back to the cache info list
        // if this is a duplicate plugin, too place it back in the cache info list marking unwantedness
        if((checkForUnwantedPlugins && isUnwantedPlugin(pluginTag)) || IsDuplicatePlugin(pluginTag)) {
          pluginTag->Mark(NS_PLUGIN_FLAG_UNWANTED);
          pluginTag->mNext = mCachedPlugins;
          mCachedPlugins = pluginTag;
        }
      }
    }
    else {
      // plugin file was added, flag this fact
      *aPluginsChanged = PR_TRUE;
    }

    // if we are not creating the list, just continue the loop
    // no need to proceed if changes are detected
    if (!aCreatePluginList) {
      if (*aPluginsChanged)
        return NS_OK;
      else
        continue;
    }

    // if it is not found in cache info list or has been changed, create a new one
    if (!pluginTag) {
      nsPluginFile pluginFile(file);
      PRLibrary* pluginLibrary = nsnull;
      
      // load the plugin's library so we can ask it some questions, but not for Windows
#ifndef XP_WIN
      if (pluginFile.LoadPlugin(pluginLibrary) != NS_OK || pluginLibrary == nsnull)
        continue;
#endif
      
      // create a tag describing this plugin.
      nsPluginInfo info = { sizeof(info) };
      nsresult res = pluginFile.GetPluginInfo(info);
      if(NS_FAILED(res))
        continue;

      // if we don't have mime type -- don't proceed, this is not a plugin
      if(!info.fMimeTypeArray) {
        pluginFile.FreePluginInfo(info);
        continue;
      }
      
      // Check for any potential '*' mime type handlers which are not our 
      // own default plugin and disable them as they will break the plugin 
      // finder service, see Bugzilla bug 132430
      if (!mAllowAlienStarHandler)
        FixUpPluginInfo(info, pluginFile);

      pluginTag = new nsPluginTag(&info);
      pluginFile.FreePluginInfo(info);
      
      if(pluginTag == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
      
      pluginTag->mLibrary = pluginLibrary;
      pluginTag->mLastModifiedTime = fileModTime;

      // if this is unwanted plugin we are checkin for, or this is a duplicate plugin, 
      // add it to our cache info list so we can cache the unwantedness of this plugin 
      // when we sync cached plugins to registry
      if((checkForUnwantedPlugins && isUnwantedPlugin(pluginTag)) || IsDuplicatePlugin(pluginTag)) {
        pluginTag->Mark(NS_PLUGIN_FLAG_UNWANTED);
        pluginTag->mNext = mCachedPlugins;
        mCachedPlugins = pluginTag;
      }
    }

    // set the flag that we want to add this plugin to the list for now
    // and see if it remains after we check several reasons not to do so
    PRBool bAddIt = PR_TRUE;

    // check if this is a specific plugin we don't want
    if(checkForUnwantedPlugins && isUnwantedPlugin(pluginTag))
      bAddIt = PR_FALSE;

    // check if we already have this plugin in the list which
    // is possible if we do refresh
    if(bAddIt) {
      if (HaveSamePlugin(pluginTag)) {
        // we cannot get here if the plugin has just been added
        // and thus |pluginTag| is not from cache, because otherwise
        // it would not be present in the list;
        // so there is no need to delete |pluginTag| -- it _is_ from the cache info list.
        bAddIt = PR_FALSE;
      }
    }

    // so if we still want it -- do it
    if(bAddIt) {
      pluginTag->mNext = mPlugins;
      mPlugins = pluginTag;

      if(layoutPath)
        RegisterPluginMimeTypesWithLayout(pluginTag, compManager, layoutPath);

    }
    else if (!(pluginTag->mFlags & NS_PLUGIN_FLAG_UNWANTED)) {
      // we don't need it, delete it;
      // but don't delete unwanted plugins since they are cached 
      // in the cache info list and will be deleted later
      delete pluginTag;
    }
  }
  return NS_OK;
}

nsresult nsPluginHostImpl::ScanPluginsDirectoryList(nsISimpleEnumerator * dirEnum,
                                                    nsIComponentManager * compManager, 
                                                    nsIFile * layoutPath,
                                                    PRBool aCreatePluginList,
                                                    PRBool * aPluginsChanged,
                                                    PRBool checkForUnwantedPlugins)
{
    PRBool hasMore;
    while (NS_SUCCEEDED(dirEnum->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> supports;
      nsresult rv = dirEnum->GetNext(getter_AddRefs(supports));
      if (NS_FAILED(rv))
        continue;
      nsCOMPtr<nsIFile> nextDir(do_QueryInterface(supports, &rv));
      if (NS_FAILED(rv))
        continue;
      
      // don't pass aPluginsChanged directly to prevent it from been reset
      PRBool pluginschanged = PR_FALSE;
      ScanPluginsDirectory(nextDir, compManager, layoutPath, aCreatePluginList, &pluginschanged, checkForUnwantedPlugins);

      if (pluginschanged)
        *aPluginsChanged = PR_TRUE;

      // if changes are detected and we are not creating the list, do not proceed
      if (!aCreatePluginList && *aPluginsChanged)
        break;
    }
    return NS_OK;
}

NS_IMETHODIMP nsPluginHostImpl::LoadPlugins()
{
  // do not do anything if it is already done
  // use ReloadPlugins() to enforce loading
  if(mPluginsLoaded)
    return NS_OK;

  PRBool pluginschanged;
  return FindPlugins(PR_TRUE, &pluginschanged);
}

#include "nsITimelineService.h"

// if aCreatePluginList is false we will just scan for plugins
// and see if any changes have been made to the plugins.
// This is needed in ReloadPlugins to prevent possible recursive reloads
nsresult nsPluginHostImpl::FindPlugins(PRBool aCreatePluginList, PRBool * aPluginsChanged)
{
  // let's start timing if we are only really creating the plugin list
  if (aCreatePluginList)
  NS_TIMELINE_START_TIMER("LoadPlugins");

  printf("nsPluginHostImpl::FindPlugins here.\n");

#ifdef CALL_SAFETY_ON
  // check preferences on whether or not we want to try safe calls to plugins
  NS_INIT_PLUGIN_SAFE_CALLS;
#endif

  NS_ENSURE_ARG_POINTER(aPluginsChanged);

  *aPluginsChanged = PR_FALSE;
  nsresult rv;
  
  // If the create instance failed, then it automatically disables all
  // our caching functions and we will just end up loading the plugins
  // on startup.
  nsCOMPtr<nsIRegistry> registry = do_CreateInstance(kRegistryCID);
  if (registry) {
    rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);
    if (NS_FAILED(rv)) {
      registry = nsnull;
    }
  }

  // Load cached plugins info
  LoadCachedPluginsInfo(registry);

  // retrieve a path for layout module. Needed for plugin mime types registration
  nsCOMPtr<nsIFile> layoutPath;
  nsCOMPtr<nsIComponentManager> compManager = do_GetService(kComponentManagerCID, &rv);

  // what I want to do here is QI for a Component Registration Manager.  Since this 
  // has not been invented yet, QI to the obsolete manager.  Kids, don't do this at home.
  nsCOMPtr<nsIComponentManagerObsolete> obsoleteManager = do_QueryInterface(compManager); 
  
  if (NS_SUCCEEDED(rv) && compManager && obsoleteManager) 
  {
    PRBool gotLayoutPath;
    if (NS_SUCCEEDED(obsoleteManager->SpecForRegistryLocation(REL_PLUGIN_DLL, getter_AddRefs(layoutPath))))
      gotLayoutPath = PR_TRUE;
    else
      gotLayoutPath = PR_FALSE;
    rv = LoadXPCOMPlugins(compManager, layoutPath);
    if (!gotLayoutPath)
        layoutPath = nsnull;
  }
  
  // Failure here is not a show-stopper so just warn.  
  rv = EnsurePrivateDirServiceProvider();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to register dir service provider.");
  
  nsCOMPtr<nsIProperties> dirService(do_GetService(kDirectoryServiceContractID, &rv));
  if (NS_FAILED(rv))
    return rv;
  
  nsCOMPtr<nsISimpleEnumerator> dirList;
  
  // Scan plugins directories;
  // don't pass aPluginsChanged directly, to prevent its 
  // possible reset in subsequent ScanPluginsDirectory calls
  PRBool pluginschanged = PR_FALSE;

  // 1. Scan the app-defined list of plugin dirs.
  rv = dirService->Get(NS_APP_PLUGINS_DIR_LIST, NS_GET_IID(nsISimpleEnumerator), getter_AddRefs(dirList));
  if (NS_SUCCEEDED(rv)) {
    printf("scanning plugins directory list.\n");
    ScanPluginsDirectoryList(dirList, compManager, layoutPath, aCreatePluginList, &pluginschanged);

    if (pluginschanged)
      *aPluginsChanged = PR_TRUE;

    // if we are just looking for possible changes, 
    // no need to proceed if changes are detected
    if (!aCreatePluginList && *aPluginsChanged) {
      ClearCachedPluginInfoList();
      return NS_OK;
    }
  }
    
  // 2. Scan the system-defined list of plugin dirs
  rv = dirService->Get(NS_OS_PLUGINS_DIR_LIST, NS_GET_IID(nsISimpleEnumerator), getter_AddRefs(dirList));
  if (NS_SUCCEEDED(rv)) {
    ScanPluginsDirectoryList(dirList, compManager, layoutPath, aCreatePluginList, &pluginschanged);

    if (pluginschanged)
      *aPluginsChanged = PR_TRUE;

    // if we are just looking for possible changes, 
    // no need to proceed if changes are detected
    if (!aCreatePluginList && *aPluginsChanged) {
      ClearCachedPluginInfoList();
      return NS_OK;
    }
  }

  mPluginsLoaded = PR_TRUE; // at this point 'some' plugins have been loaded,
                            // the rest is optional
    
#if defined (XP_WIN)
  // 3. Scan the installation paths of our popular plugins if the prefs are enabled
    
  // This table controls the order of scanning
  const char *prefs[] = {NS_WIN_JRE_SCAN_KEY,         nsnull,
                         NS_WIN_ACROBAT_SCAN_KEY,     nsnull,
                         NS_WIN_QUICKTIME_SCAN_KEY,   nsnull,
                         NS_WIN_WMP_SCAN_KEY,         nsnull,
                         NS_WIN_4DOTX_SCAN_KEY,       "1"  /*  second column is flag for 4.x folder */ };

  PRUint32 size = sizeof(prefs) / sizeof(prefs[0]);

  for (PRUint32 i = 0; i < size; i+=2) {
    nsCOMPtr<nsIFile> dirToScan;
    PRBool bExists;
    if (NS_SUCCEEDED(dirService->Get(prefs[i], NS_GET_IID(nsIFile), getter_AddRefs(dirToScan))) &&
        dirToScan &&
        NS_SUCCEEDED(dirToScan->Exists(&bExists)) && 
        bExists) {
      
      PRBool bFilterUnwanted = PR_FALSE;

      // 4.x plugins folder stuff: 
      // Normally we "filter" the 4.x folder through |IsUnwantedPlugin|
      // Check for a pref to see if we want to scan the entire 4.x plugins folder
      if (prefs[i+1]) {
        PRBool bScanEverything;
        bFilterUnwanted = PR_TRUE;  // default to filter 4.x folder
        nsCOMPtr<nsIPref> prefService = do_GetService(NS_PREF_CONTRACTID);
        if (prefService &&
            NS_SUCCEEDED(prefService->GetBoolPref(prefs[i], &bScanEverything)) &&
            bScanEverything)
          bFilterUnwanted = PR_FALSE;

      }
      ScanPluginsDirectory(dirToScan, compManager, layoutPath, aCreatePluginList, &pluginschanged, bFilterUnwanted);

      if (pluginschanged)
        *aPluginsChanged = PR_TRUE;

      // if we are just looking for possible changes, 
      // no need to proceed if changes are detected
      if (!aCreatePluginList && *aPluginsChanged) {
        ClearCachedPluginInfoList();
        return NS_OK;
      }
    }
  }
#endif
   
  // if get to this point and did not detect changes in plugins
  // that means no plugins got updated or added
  // let's see if plugins have been removed
  if (!*aPluginsChanged) {
    // count plugins remained in cache, if there are some, that means some plugins were removed;
    // while counting, we should ignore unwanted plugins which are also present in cache
    PRUint32 cachecount = 0;
    for (nsPluginTag * cachetag = mCachedPlugins; cachetag; cachetag = cachetag->mNext) {
      if (!(cachetag->mFlags & NS_PLUGIN_FLAG_UNWANTED))
        cachecount++;
    }
    // if there is something left in cache, some plugins got removed from the directory
    // and therefor their info did not get removed from the cache info list during directory scan;
    // flag this fact
    if (cachecount > 0) 
      *aPluginsChanged = PR_TRUE;
  }

  // if we are not creating the list, there is no need to proceed
  if (!aCreatePluginList) {
    ClearCachedPluginInfoList();
    return NS_OK;
  }

  // if we are creating the list, it is already done;
  // update the plugins info cache if changes are detected
  if (*aPluginsChanged)
    CachePluginsInfo(registry);

  // No more need for cached plugins. Clear it up.
  ClearCachedPluginInfoList();

  /*
   * XXX Big time hack alert!!!!
   *     Because Real Player 8 installs in the components folder, we must have this one off
   *     scan for nppl3260.dll because XPCOM has shut off nsGetFactory type plugins.
   *     When we stop supporting Real 8 or they fix their installer, this can go away.
   */
  if (aCreatePluginList)
    ScanForRealInComponentsFolder(compManager, layoutPath);

  // reverse our list of plugins 
  nsPluginTag *next,*prev = nsnull;
  for (nsPluginTag *cur = mPlugins; cur; cur = next) {
    next = cur->mNext;
    cur->mNext = prev;
    prev = cur;
  }

  mPlugins = prev;

  NS_TIMELINE_STOP_TIMER("LoadPlugins");
  NS_TIMELINE_MARK_TIMER("LoadPlugins");

  return NS_OK;
}

void nsPluginHostImpl::ClearCachedPluginInfoList() 
{
  while (mCachedPlugins) {
    nsPluginTag *next = mCachedPlugins->mNext;
    delete mCachedPlugins;
    mCachedPlugins = next;
  }
}

////////////////////////////////////////////////////////////////////////

static nsresult
cidToDllname(nsIComponentManager* aComponentManager, nsIRegistry* aRegistry,
             const nsACString &aCID, nsACString &dllName)
{
  // To figure out the filename of the plugin, we'll need to get the
  // plugin's CID, and then navigate through the XPCOM registry to
  // pull out the DLL name to which the CID is registered.
  nsAutoString path(NS_LITERAL_STRING("software/mozilla/XPCOM/classID/") + NS_ConvertASCIItoUCS2(aCID));

  nsRegistryKey cidKey;
  nsresult rv = aRegistry->GetKey(nsIRegistry::Common, path.get(), &cidKey);

  if (NS_SUCCEEDED(rv)) {
    PRUint8* library;
    PRUint32 count;
    // XXX Good grief, what does "GetBytesUTF8()" mean? They're bytes!
    rv = aRegistry->GetBytesUTF8(cidKey, "InprocServer", &count, &library);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIFile> file;
      // what I want to do here is QI for a Component Registration Manager.  Since this 
      // has not been invented yet, QI to the obsolete manager.  Kids, don't do this at home.
      nsCOMPtr<nsIComponentManagerObsolete> obsoleteManager = do_QueryInterface(aComponentManager, &rv);
      if (obsoleteManager)
        rv = obsoleteManager->SpecForRegistryLocation(NS_REINTERPRET_CAST(const char*, library),
                                                      getter_AddRefs(file));

      if (NS_SUCCEEDED(rv)) {
        file->GetNativePath(dllName);
      }

      nsCRT::free(NS_REINTERPRET_CAST(char*, library));
    }
  }

  return rv;
}


static nsresult
LoadXPCOMPlugin(nsIRegistry* aRegistry,
                const char* aFilename,
                nsRegistryKey aPluginKey,
                nsPluginTag** aResult)
{
  nsresult rv;

  // The name, description, MIME types, MIME descriptions, and
  // supported file extensions will all hang off of the plugin's key
  // in the registry. Pull these out now.
  PRUint8 * name=nsnull;
  PRUint32 length;
  aRegistry->GetBytesUTF8(aPluginKey, kPluginsNameKey, &length, &name);

  PRUint8 * description=nsnull;
  aRegistry->GetBytesUTF8(aPluginKey, kPluginsDescKey, &length, &description);

  nsXPIDLCString filename;
  nsXPIDLCString fullpath;
  if (!aFilename) {
    // Look for a filename key
    aRegistry->GetStringUTF8(aPluginKey, kPluginsFilenameKey, getter_Copies(filename));
    aRegistry->GetStringUTF8(aPluginKey, kPluginsFullpathKey, getter_Copies(fullpath));
  }

  PRInt64 lastmod = LL_ZERO;
  aRegistry->GetLongLong(aPluginKey, kPluginsModTimeKey, &lastmod);

  PRInt32 intval = 1;  // Default for canunload is PR_TRUE
  aRegistry->GetInt(aPluginKey, kPluginsCanUnload, &intval);
  PRBool canunload = (intval > 0);

  nsCOMPtr<nsIEnumerator> enumerator;
  rv = aRegistry->EnumerateAllSubtrees(aPluginKey, getter_AddRefs(enumerator));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISimpleEnumerator> subtrees;
  rv = NS_NewAdapterEnumerator(getter_AddRefs(subtrees), enumerator);
  if (NS_FAILED(rv)) return rv;

  char** mimetypes = nsnull;
  char** mimedescriptions = nsnull;
  char** extensions = nsnull;
  PRInt32 count = 0;
  PRInt32 capacity = 0;

  for (;;) {
    PRBool hasmore;
    subtrees->HasMoreElements(&hasmore);
    if (! hasmore)
      break;

    nsCOMPtr<nsISupports> isupports;
    subtrees->GetNext(getter_AddRefs(isupports));

    nsCOMPtr<nsIRegistryNode> node = do_QueryInterface(isupports);
    NS_ASSERTION(node != nsnull, "not an nsIRegistryNode");
    if (! node)
      continue;

    nsRegistryKey key;
    node->GetKey(&key);

    if (count >= capacity) {
      capacity += capacity ? capacity : 4;

      char** newmimetypes        = new char*[capacity];
      char** newmimedescriptions = new char*[capacity];
      char** newextensions       = new char*[capacity];

      if (!newmimetypes || !newmimedescriptions || !newextensions) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        delete[] newmimetypes;
        delete[] newmimedescriptions;
        delete[] newextensions;
        break;
      }

      for (PRInt32 i = 0; i < count; ++i) {
        newmimetypes[i]        = mimetypes[i];
        newmimedescriptions[i] = mimedescriptions[i];
        newextensions[i]       = extensions[i];
      }

      delete[] mimetypes;
      delete[] mimedescriptions;
      delete[] extensions;

      mimetypes        = newmimetypes;
      mimedescriptions = newmimedescriptions;
      extensions       = newextensions;
    }

    aRegistry->GetStringUTF8(key, kPluginsMimeTypeKey, &mimetypes[count]);

    PRUint8 * md;
    aRegistry->GetBytesUTF8(key, kPluginsMimeDescKey, &length, &md);
    mimedescriptions[count] = NS_REINTERPRET_CAST(char *, md);

    aRegistry->GetStringUTF8(key, kPluginsMimeExtKey, &extensions[count]);
    ++count;
  }

  if (NS_SUCCEEDED(rv)) {
    // All done! Create the new nsPluginTag info and send it back.
    nsPluginTag* tag
      = new nsPluginTag(NS_REINTERPRET_CAST(const char *, name),
                        NS_REINTERPRET_CAST(const char *, description),
                        aFilename ? aFilename : filename.get(),
                        fullpath.get(),
                        (const char* const*)mimetypes,
                        (const char* const*)mimedescriptions,
                        (const char* const*)extensions,
                        count, lastmod, canunload);

    if (! tag)
      rv = NS_ERROR_OUT_OF_MEMORY;

    *aResult = tag;
  }

  for (PRInt32 i = 0; i < count; ++i) {
    CRTFREEIF(mimetypes[i]);
    CRTFREEIF(mimedescriptions[i]);
    CRTFREEIF(extensions[i]);
  }

  delete[] mimetypes;
  delete[] mimedescriptions;
  delete[] extensions;
  PR_FREEIF(name);
  PR_FREEIF(description);

  return rv;
}


static nsresult
AddPluginInfoToRegistry(nsIRegistry* registry, nsRegistryKey top,
                        nsPluginTag *tag, const char *keyname)
{
  NS_ENSURE_ARG_POINTER(tag);
  nsRegistryKey pluginKey;
  nsresult rv = registry->AddSubtree(top, keyname, &pluginKey);
  if (NS_FAILED(rv)) return rv;

  // Add filename, name and description
  registry->SetStringUTF8(pluginKey, kPluginsFilenameKey, tag->mFileName);
  if (tag->mFullPath)
    registry->SetStringUTF8(pluginKey, kPluginsFullpathKey, tag->mFullPath);

  // we use SetBytes instead of SetString to address special character issue, see Mozilla bug 108246
  if(tag->mName)
    registry->SetBytesUTF8(pluginKey, kPluginsNameKey, strlen(tag->mName) + 1, (PRUint8 *)tag->mName);
  
  if(tag->mDescription)
    registry->SetBytesUTF8(pluginKey, kPluginsDescKey, strlen(tag->mDescription) + 1, (PRUint8 *)tag->mDescription);

  registry->SetLongLong(pluginKey, kPluginsModTimeKey, &(tag->mLastModifiedTime));
  registry->SetInt(pluginKey, kPluginsCanUnload, tag->mCanUnloadLibrary);

  // Add in each mimetype this plugin supports
  for (int i=0; i<tag->mVariants; i++) {
    char mimetypeKeyName[16];
    nsRegistryKey mimetypeKey;
    PR_snprintf(mimetypeKeyName, sizeof (mimetypeKeyName), "mimetype-%d", i);
    rv = registry->AddSubtree(pluginKey, mimetypeKeyName, &mimetypeKey);
    if (NS_FAILED(rv))
      break;
    registry->SetStringUTF8(mimetypeKey, kPluginsMimeTypeKey, tag->mMimeTypeArray[i]);

    if(tag->mMimeDescriptionArray && tag->mMimeDescriptionArray[i])
      registry->SetBytesUTF8(mimetypeKey, kPluginsMimeDescKey, 
                             strlen(tag->mMimeDescriptionArray[i]) + 1, 
                             (PRUint8 *)tag->mMimeDescriptionArray[i]);

    registry->SetStringUTF8(mimetypeKey, kPluginsMimeExtKey, tag->mExtensionsArray[i]);
  }
  if (NS_FAILED(rv))
    rv = registry->RemoveSubtree(top, keyname);

  return rv;
}


////////////////////////////////////////////////////////////////////////
nsresult
nsPluginHostImpl::LoadXPCOMPlugins(nsIComponentManager* aComponentManager, nsIFile* aPath)
{
  // The "new style" XPCOM plugins have their information stored in
  // the component registry, under the key
  //
  //   nsIRegistry::Common/software/plugins
  //
  // Enumerate through that list now, creating an nsPluginTag for
  // each.
  nsCOMPtr<nsIRegistry> registry = do_CreateInstance(kRegistryCID);
  if (! registry)
    return NS_ERROR_FAILURE;

  nsresult rv;
  rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
  if (NS_FAILED(rv)) return rv;
  
  nsRegistryKey pluginsKey;
  rv = registry->GetSubtree(nsIRegistry::Common, kPluginsRootKey, &pluginsKey);
  if (NS_FAILED(rv)) return rv;

  // XXX get rid nsIEnumerator someday!
  nsCOMPtr<nsIEnumerator> enumerator;
  rv = registry->EnumerateSubtrees(pluginsKey, getter_AddRefs(enumerator));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISimpleEnumerator> plugins;
  rv = NS_NewAdapterEnumerator(getter_AddRefs(plugins), enumerator);
  if (NS_FAILED(rv)) return rv;

  for (;;) {
    PRBool hasMore;
    plugins->HasMoreElements(&hasMore);
    if (! hasMore)
      break;

    nsCOMPtr<nsISupports> isupports;
    plugins->GetNext(getter_AddRefs(isupports));

    nsCOMPtr<nsIRegistryNode> node = do_QueryInterface(isupports);
    NS_ASSERTION(node != nsnull, "not an nsIRegistryNode");
    if (! node)
      continue;

    // Pull out the information for an individual plugin, and link it
    // in to the mPlugins list.
    nsXPIDLCString cid;
    node->GetNameUTF8(getter_Copies(cid));

    nsRegistryKey key;
    node->GetKey(&key);

    nsCAutoString filename;
    rv = cidToDllname(aComponentManager, registry, cid, filename);
    if (NS_FAILED(rv))
      continue;

    nsPluginTag* tag = nsnull;
    rv = LoadXPCOMPlugin(registry, filename.get(), key, &tag);
    if (NS_FAILED(rv))
      continue;

    PRBool bAddIt = PR_TRUE;

    // skip it if we already have it
    if (HaveSamePlugin(tag))
      bAddIt = PR_FALSE;

    if(!bAddIt) {
      if(tag)
        delete tag;
      continue;
    }

    tag->mNext = mPlugins;
    mPlugins = tag;

    // Create an nsIDocumentLoaderFactory wrapper in case we ever see
    // any naked streams.
    RegisterPluginMimeTypesWithLayout(tag, aComponentManager, aPath);
  }

  return NS_OK;
}


nsresult
nsPluginHostImpl::LoadCachedPluginsInfo(nsIRegistry* registry)
{
  // Loads cached plugin info from registry
  //
  // Enumerate through that list now, creating an nsPluginTag for
  // each.
  if (! registry)
    return NS_ERROR_FAILURE;

  nsRegistryKey pluginsKey;
  nsresult rv = registry->GetSubtree(nsIRegistry::Common, kPluginsRootKey, &pluginsKey);
  if (NS_FAILED(rv)) return rv;

  // Make sure we are dealing with the same version number of the plugin info
  nsXPIDLCString version;
  rv = registry->GetStringUTF8(pluginsKey, kPluginsVersionKey, getter_Copies(version));
  if (NS_FAILED(rv) || PL_strcmp(version.get(), kPluginInfoVersion)) {
    // Version mismatch
    // Nuke the subtree
    registry->RemoveSubtree(nsIRegistry::Common, kPluginsRootKey);
    PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_BASIC,
           ("LoadCachedPluginsInfo : Version %s mismatch - Expected %s. Nuking cached info.\n",
            version.get(), kPluginInfoVersion));
    return NS_ERROR_FAILURE;
  }

  // XXX get rid nsIEnumerator someday!
  nsCOMPtr<nsIEnumerator> enumerator;
  rv = registry->EnumerateSubtrees(pluginsKey, getter_AddRefs(enumerator));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISimpleEnumerator> plugins;
  rv = NS_NewAdapterEnumerator(getter_AddRefs(plugins), enumerator);
  if (NS_FAILED(rv)) return rv;

  for (;;) {
    PRBool hasMore;
    plugins->HasMoreElements(&hasMore);
    if (! hasMore)
      break;

    nsCOMPtr<nsISupports> isupports;
    plugins->GetNext(getter_AddRefs(isupports));

    nsCOMPtr<nsIRegistryNode> node = do_QueryInterface(isupports);
    NS_ASSERTION(node != nsnull, "not an nsIRegistryNode");
    if (! node)
      continue;

    // Pull out the information for an individual plugin, and link it
    // in to the mCachedPlugins list.
    nsRegistryKey key;
    node->GetKey(&key);

    nsPluginTag* tag = nsnull;
    rv = LoadXPCOMPlugin(registry, nsnull, key, &tag);
    if (NS_FAILED(rv))
      continue;

    // Mark plugin as loaded from cache
    tag->Mark(NS_PLUGIN_FLAG_FROMCACHE);
    PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_BASIC,
           ("LoadCachedPluginsInfo : Loading Cached plugininfo for %s\n", tag->mFileName));
    tag->mNext = mCachedPlugins;
    mCachedPlugins = tag;
  }

  return NS_OK;
}

nsresult
nsPluginHostImpl::CachePluginsInfo(nsIRegistry* registry)
{
  // Save plugin info from registry

  if (! registry)
    return NS_ERROR_FAILURE;

  // We don't want any old plugins that don't exist anymore hanging around
  // So remove and re-add the root of the plugins info
  nsresult rv = registry->RemoveSubtree(nsIRegistry::Common, kPluginsRootKey);
  
  nsRegistryKey pluginsSubtreeKey;
  rv = registry->AddSubtree(nsIRegistry::Common, kPluginsRootKey, &pluginsSubtreeKey);
  if (NS_FAILED(rv)) return rv;

  rv = registry->SetStringUTF8(pluginsSubtreeKey, kPluginsVersionKey, kPluginInfoVersion);
  if (NS_FAILED(rv)) return rv;

  int count = 0;

  // Store all plugins in the mPlugins list - all plugins currently in use.
  char pluginKeyName[64];
  nsPluginTag *tag;
  for(tag = mPlugins; tag; tag=tag->mNext) {
    // store each plugin info into the registry
    PR_snprintf(pluginKeyName, sizeof(pluginKeyName), "plugin-%d", ++count);
    AddPluginInfoToRegistry(registry, pluginsSubtreeKey, tag, pluginKeyName);
  }

  // Store any unwanted plugins info so that we wont have to load
  // them again the next time around
  tag = mCachedPlugins;
  for(tag = mCachedPlugins; tag; tag=tag->mNext) {
    // Look for unwanted plugins
    if (!(tag->mFlags & NS_PLUGIN_FLAG_UNWANTED))
      continue;

    // store each plugin info into the registry
    PR_snprintf(pluginKeyName, sizeof(pluginKeyName), "plugin-%d", ++count);
    AddPluginInfoToRegistry(registry, pluginsSubtreeKey, tag, pluginKeyName);
  }

  return NS_OK;
}

nsPluginTag *
nsPluginHostImpl::RemoveCachedPluginsInfo(const char *filename)
{
  nsPluginTag **link = &mCachedPlugins;
  for (nsPluginTag *tag = *link; tag; link = &tag->mNext, tag = *link)
  {
    // compare filename or else the mFullPath if it exists. Mac seems to use
    // mFullPath for fullpath and mFileName for just the leafname of fullpath.
    // On win and unix, mFullPath is never used and mFileName is contains the
    // full pathname. All this should move to using nsIFile.
    if (!PL_strcmp(tag->mFileName, filename) ||
        (tag->mFullPath && !PL_strcmp(tag->mFullPath, filename)))
    {
      // Found it. Remove it from our list
      *link = tag->mNext;
      return tag;
    }
  }
  return nsnull;
}

nsresult
nsPluginHostImpl::EnsurePrivateDirServiceProvider()
{
  if (!mPrivateDirServiceProvider)
  {
    nsresult rv;
    nsCOMPtr<nsIDirectoryServiceProvider> provider = new nsPluginDirServiceProvider;
    if (!provider)
      return NS_ERROR_OUT_OF_MEMORY;
    nsCOMPtr<nsIDirectoryService> dirService(do_GetService(kDirectoryServiceContractID, &rv));
    if (NS_FAILED(rv))
      return rv;
    rv = dirService->RegisterProvider(provider);
    if (NS_FAILED(rv))
      return rv;
    mPrivateDirServiceProvider = provider;
  }
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
/* Called by GetURL and PostURL */
NS_IMETHODIMP nsPluginHostImpl::NewPluginURLStream(const nsString& aURL,
                                                   nsIPluginInstance *aInstance,
                                                   nsIPluginStreamListener* aListener,
                                                   const char *aPostData,
                                                   PRBool aIsFile, 
                                                   PRUint32 aPostDataLen, 
                                                   const char *aHeadersData, 
                                                   PRUint32 aHeadersDataLen)
{
  nsCOMPtr<nsIURI> url;
  nsAutoString absUrl;
  nsresult rv;

  if (aURL.Length() <= 0)
    return NS_OK;

  // get the full URL of the document that the plugin is embedded
  //   in to create an absolute url in case aURL is relative
  nsCOMPtr<nsIDocument> doc;
  nsPluginInstancePeerImpl *peer;
  rv = aInstance->GetPeer(NS_REINTERPRET_CAST(nsIPluginInstancePeer **, &peer));
  if (NS_SUCCEEDED(rv) && peer)
  {
    nsCOMPtr<nsIPluginInstanceOwner> owner;
    rv = peer->GetOwner(*getter_AddRefs(owner));
    if (NS_SUCCEEDED(rv) && owner)
    {
      rv = owner->GetDocument(getter_AddRefs(doc));
      if (NS_SUCCEEDED(rv) && doc)
      {
        nsCOMPtr<nsIURI> docURL;
        doc->GetDocumentURL(getter_AddRefs(docURL));
 
        // Create an absolute URL
        rv = NS_MakeAbsoluteURI(absUrl, aURL, docURL);
      }
    }
    NS_RELEASE(peer);
  }

  if (absUrl.IsEmpty())
    absUrl.Assign(aURL);

  rv = NS_NewURI(getter_AddRefs(url), absUrl);

  if (NS_SUCCEEDED(rv))
  {
    nsPluginStreamListenerPeer *listenerPeer = new nsPluginStreamListenerPeer;
    if (listenerPeer == NULL)
      return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(listenerPeer);
    rv = listenerPeer->Initialize(url, aInstance, aListener);

    if (NS_SUCCEEDED(rv)) 
    {
      // it is possible one plugin instance can request several different 
      // streams, if we are serving those streams as files from 
      // temp location (e.g. browser cache is not available)
      // we do not have to delete the files until instance is active,
      // usually temp files are getting deleted when listener peer is released by OnStopRequest(),
      // lets add this listener peer to the to list of stream peers for this instance,
      // it'll delay the release until nsActivePlugin::setStopped() call.
      nsActivePlugin * p = mActivePluginList.find(aInstance);
      if (p && p->mStreams)
      {
        p->mStreams->AppendElement((void *)listenerPeer);
        NS_ADDREF(listenerPeer);
      }

      nsCOMPtr<nsIInterfaceRequestor> callbacks;

      if (doc) 
      {
        // Get the script global object owner and use that as the notification callback
        nsCOMPtr<nsIScriptGlobalObject> global;
        doc->GetScriptGlobalObject(getter_AddRefs(global));

        if (global) 
        {
          nsCOMPtr<nsIScriptGlobalObjectOwner> owner;
          global->GetGlobalObjectOwner(getter_AddRefs(owner));

          callbacks = do_QueryInterface(owner);
        }
      }

      nsCOMPtr<nsIChannel> channel;

      // XXX: Null LoadGroup?
      rv = NS_NewChannel(getter_AddRefs(channel), url, nsnull, nsnull, callbacks);
      if (NS_FAILED(rv)) 
        return rv;

      if (doc) 
      {
        // Set the owner of channel to the document principal...
        nsCOMPtr<nsIPrincipal> principal;
        doc->GetPrincipal(getter_AddRefs(principal));

        channel->SetOwner(principal);
      }

      // deal with headers and post data
      nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
      if(httpChannel) {
        if (aPostData) {
          
          nsCOMPtr<nsIInputStream> postDataStream;
          rv = NS_NewPluginPostDataStream(getter_AddRefs(postDataStream), (const char*)aPostData, 
                                          aPostDataLen, aIsFile);

          if (!postDataStream) 
          {
            NS_RELEASE(aInstance);
            return NS_ERROR_UNEXPECTED;
          }

          // XXX it's a bit of a hack to rewind the postdata stream
          // here but it has to be done in case the post data is
          // being reused multiple times.
          nsCOMPtr<nsISeekableStream> 
          postDataSeekable(do_QueryInterface(postDataStream));
          if (postDataSeekable)
            postDataSeekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);

          nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
          NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");

          uploadChannel->SetUploadStream(postDataStream, nsnull, -1);
        }

        if (aHeadersData) 
          rv = AddHeadersToChannel(aHeadersData, aHeadersDataLen, httpChannel);
      }
      rv = channel->AsyncOpen(listenerPeer, nsnull);
    }
    NS_RELEASE(listenerPeer);
  }
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginHostImpl::AddHeadersToChannel(const char *aHeadersData, 
                                      PRUint32 aHeadersDataLen, 
                                      nsIChannel *aGenericChannel)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIHttpChannel> aChannel = do_QueryInterface(aGenericChannel);
  if (!aChannel) {
    return NS_ERROR_NULL_POINTER;
  }

  // used during the manipulation of the String from the aHeadersData
  nsCAutoString headersString;
  nsCAutoString oneHeader;
  nsCAutoString headerName;
  nsCAutoString headerValue;
  PRInt32 crlf = 0;
  PRInt32 colon = 0;
  
  //
  // Turn the char * buffer into an nsString.
  //
  headersString = aHeadersData;

  //
  // Iterate over the nsString: for each "\r\n" delimeted chunk,
  // add the value as a header to the nsIHTTPChannel
  //
  
  while (PR_TRUE) {
    crlf = headersString.Find("\r\n", PR_TRUE);
    if (-1 == crlf) {
      rv = NS_OK;
      return rv;
    }
    headersString.Mid(oneHeader, 0, crlf);
    headersString.Cut(0, crlf + 2);
    oneHeader.StripWhitespace();
    colon = oneHeader.Find(":");
    if (-1 == colon) {
      rv = NS_ERROR_NULL_POINTER;
      return rv;
    }
    oneHeader.Left(headerName, colon);
    colon++;
    oneHeader.Mid(headerValue, colon, oneHeader.Length() - colon);
    
    //
    // FINALLY: we can set the header!
    // 
    
    rv =aChannel->SetRequestHeader(headerName, headerValue);
    if (NS_FAILED(rv)) {
      rv = NS_ERROR_NULL_POINTER;
      return rv;
    }
  }    
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginHostImpl::StopPluginInstance(nsIPluginInstance* aInstance)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHostImpl::StopPluginInstance called instance=%p\n",aInstance));

  nsActivePlugin * plugin = mActivePluginList.find(aInstance);

  if(plugin != nsnull) {
    plugin->setStopped(PR_TRUE);  // be sure we set the "stop" bit

    // if the plugin does not want to be 'cached' just remove it
    PRBool doCache = PR_TRUE;
    aInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);

    if (!doCache) {
      PRLibrary * library = nsnull;
      if(plugin->mPluginTag)
        library = plugin->mPluginTag->mLibrary;

      PRBool unloadLibLater = PR_FALSE;
      mActivePluginList.remove(plugin, &unloadLibLater);

      if(unloadLibLater)
        AddToUnusedLibraryList(library);
    } 
    else {
      // if it is allowed to be cached simply stop it, but first we should check 
      // if we haven't exceeded the maximum allowed number of cached instances

      // try to get the max cached plugins from a pref or use default
      PRUint32 max_num;
      nsresult rv;
      nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
      if (prefs) rv = prefs->GetIntPref(NS_PREF_MAX_NUM_CACHED_PLUGINS,(int *)&max_num);
      if (NS_FAILED(rv)) max_num = DEFAULT_NUMBER_OF_STOPPED_PLUGINS;

      if(mActivePluginList.getStoppedCount() >= max_num) {
        nsActivePlugin * oldest = mActivePluginList.findOldestStopped();
        if(oldest != nsnull) {
          PRLibrary * library = oldest->mPluginTag->mLibrary;

          PRBool unloadLibLater = PR_FALSE;
          mActivePluginList.remove(oldest, &unloadLibLater);

          if(unloadLibLater)
            AddToUnusedLibraryList(library);
        }
      }
    }
  }
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
/* Called by InstantiateEmbededPlugin() */
nsresult nsPluginHostImpl::NewEmbededPluginStream(nsIURI* aURL,
                                                  nsIPluginInstanceOwner *aOwner,
                                                  nsIPluginInstance* aInstance)
{
  nsPluginStreamListenerPeer  *listener = (nsPluginStreamListenerPeer *)new nsPluginStreamListenerPeer();
  if (listener == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;

  if (!aURL)
    return NS_OK;

  // if we have an instance, everything has been set up
  // if we only have an owner, then we need to pass it in
  // so the listener can set up the instance later after
  // we've determined the mimetype of the stream
  if(aInstance != nsnull)
    rv = listener->InitializeEmbeded(aURL, aInstance);
  else if(aOwner != nsnull)
    rv = listener->InitializeEmbeded(aURL, nsnull, aOwner, (nsIPluginHost *)this);
  else
    rv = NS_ERROR_ILLEGAL_VALUE;

  if (NS_OK == rv) {
    // XXX: Null LoadGroup?
    rv = NS_OpenURI(listener, nsnull, aURL, nsnull);
  }

  //NS_RELEASE(aURL);

  return rv;
}


////////////////////////////////////////////////////////////////////////
/* Called by InstantiateFullPagePlugin() */
nsresult nsPluginHostImpl::NewFullPagePluginStream(nsIStreamListener *&aStreamListener, 
                                                   nsIPluginInstance *aInstance)
{
  nsPluginStreamListenerPeer  *listener = (nsPluginStreamListenerPeer *)new nsPluginStreamListenerPeer();
  if (listener == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;

  rv = listener->InitializeFullPage(aInstance);

  aStreamListener = (nsIStreamListener *)listener;
  NS_IF_ADDREF(listener);

      // add peer to list of stream peers for this instance
    nsActivePlugin * p = mActivePluginList.find(aInstance);
    if (p && p->mStreams)
    {
      p->mStreams->AppendElement((void *)aStreamListener);
      NS_ADDREF(listener);
    }

  return rv;
}


// nsIFileUtilities interface
////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::GetProgramPath(const char* *result)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(result);
  *result = nsnull;
  
  nsCOMPtr<nsIProperties> dirService(do_GetService(kDirectoryServiceContractID, &rv));
  if (NS_FAILED(rv))
    return rv;
  nsCOMPtr<nsILocalFile> programDir;
  rv = dirService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(programDir));
  if (NS_FAILED(rv))
    return rv;
  
  nsCAutoString temp;
  rv = programDir->GetNativePath(temp);
  *result = ToNewCString(temp);
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::GetTempDirPath(const char* *result)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(result);
  *result = nsnull;
  
  nsCOMPtr<nsIProperties> dirService(do_GetService(kDirectoryServiceContractID, &rv));
  if (NS_FAILED(rv))
    return rv;
  nsCOMPtr<nsILocalFile> tempDir;
  rv = dirService->Get(NS_OS_TEMP_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(tempDir));
  if (NS_FAILED(rv))
    return rv;
  
  nsCAutoString temp;
  rv = tempDir->GetNativePath(temp);
  *result = ToNewCString(temp);
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::NewTempFileName(const char* prefix, PRUint32 bufLen, char* resultBuf)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


// nsICookieStorage interface

////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::GetCookie(const char* inCookieURL, void* inOutCookieBuffer, PRUint32& inOutCookieSize)
{
  nsresult rv = NS_ERROR_NOT_IMPLEMENTED;
  nsXPIDLCString cookieString;
  PRUint32 cookieStringLen = 0;
  nsCOMPtr<nsIURI> uriIn;
  
  if ((nsnull == inCookieURL) || (0 >= inOutCookieSize)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIIOService> ioService(do_GetService(kIOServiceCID, &rv));
  
  if (NS_FAILED(rv) || (nsnull == ioService)) {
    return rv;
  }

  nsCOMPtr<nsICookieService> cookieService = 
           do_GetService(kCookieServiceCID, &rv);
  
  if (NS_FAILED(rv) || (nsnull == cookieService)) {
    return NS_ERROR_INVALID_ARG;
  }

  // make an nsURI from the argument url
  rv = ioService->NewURI(nsDependentCString(inCookieURL), nsnull, nsnull, getter_AddRefs(uriIn));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = cookieService->GetCookieString(uriIn, getter_Copies(cookieString));
  
  if (NS_FAILED(rv) || (!cookieString) ||
      (inOutCookieSize <= (cookieStringLen = PL_strlen(cookieString.get())))) {
    return NS_ERROR_FAILURE;
  }

  PL_strcpy((char *) inOutCookieBuffer, cookieString.get());
  inOutCookieSize = cookieStringLen;
  rv = NS_OK;
  
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::SetCookie(const char* inCookieURL, const void* inCookieBuffer, PRUint32 inCookieSize)
{
  nsresult rv = NS_ERROR_NOT_IMPLEMENTED;
  nsCOMPtr<nsIURI> uriIn;
  
  if ((nsnull == inCookieURL) || (nsnull == inCookieBuffer) || 
      (0 >= inCookieSize)) {
    return NS_ERROR_INVALID_ARG;
  }
  
  nsCOMPtr<nsIIOService> ioService(do_GetService(kIOServiceCID, &rv));
  
  if (NS_FAILED(rv) || (nsnull == ioService)) {
    return rv;
  }
  
  nsCOMPtr<nsICookieService> cookieService = 
           do_GetService(kCookieServiceCID, &rv);
  
  if (NS_FAILED(rv) || (nsnull == cookieService)) {
    return NS_ERROR_FAILURE;
  }
  
  // make an nsURI from the argument url
  rv = ioService->NewURI(nsDependentCString(inCookieURL), nsnull, nsnull, getter_AddRefs(uriIn));
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  char * cookie = (char *)inCookieBuffer;
  char c = cookie[inCookieSize];
  cookie[inCookieSize] = '\0';
  rv = cookieService->SetCookieString(uriIn, nsnull, cookie,0); // needs an nsIPrompt parameter
  cookie[inCookieSize] = c;
  
  return rv;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::Observe(nsISupports *aSubject,
                                        const char *aTopic,
                                        const PRUnichar *someData)
{
#ifdef NS_DEBUG
  printf("nsPluginHostImpl::Observe \"%s\"\n", aTopic ? aTopic : "");
#endif
  if (!nsCRT::strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, aTopic) ||
      !nsCRT::strcmp("quit-application", aTopic))
  {
    Destroy();
  }
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsPluginHostImpl::HandleBadPlugin(PRLibrary* aLibrary)
{
  nsresult rv = NS_OK;

  NS_ASSERTION(PR_FALSE, "Plugin performed illegal operation");

  if(mDontShowBadPluginMessage)
    return rv;
  
  nsCOMPtr<nsIPrompt> prompt;
  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
  if (wwatch)
    wwatch->GetNewPrompter(0, getter_AddRefs(prompt));

  nsCOMPtr<nsIIOService> io(do_GetService(kIOServiceCID));
  nsCOMPtr<nsIStringBundleService> strings(do_GetService(kStringBundleServiceCID));

  if (!prompt || !io || !strings)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStringBundle> bundle;
  nsCOMPtr<nsIURI> uri;
  nsCAutoString spec;

  PRInt32 buttonPressed;
  PRBool checkboxState = PR_FALSE;
  
  rv = io->NewURI(NS_LITERAL_CSTRING(PLUGIN_PROPERTIES_URL), nsnull, nsnull, getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return rv;

  rv = uri->GetSpec(spec);
  if (NS_FAILED(rv))
    return rv;

  rv = strings->CreateBundle(spec.get(), getter_AddRefs(bundle));
  if (NS_FAILED(rv))
    return rv;

  PRUnichar *title = nsnull;
  PRUnichar *message = nsnull;
  PRUnichar *checkboxMessage = nsnull;

  rv = bundle->GetStringFromName(NS_LITERAL_STRING("BadPluginTitle").get(), 
                                 &title);
  if (NS_FAILED(rv))
    return rv;

  rv = bundle->GetStringFromName(NS_LITERAL_STRING("BadPluginMessage").get(), 
                                 &message);
  if (NS_FAILED(rv)) {
    nsMemory::Free((void *)title);
    return rv;
  }
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("BadPluginCheckboxMessage").get(), 
                                 &checkboxMessage);
  if (NS_FAILED(rv)) {
    nsMemory::Free((void *)title);
    nsMemory::Free((void *)message);
    return rv;
  }
                               
  // add plugin name to the message
  char * pluginname = nsnull;
  for (nsPluginTag * tag = mPlugins; tag; tag = tag->mNext) {
    if (tag->mLibrary == aLibrary) {
      if (tag->mName)
        pluginname = tag->mName;
      else
        pluginname = tag->mFileName;
    }
  }

  nsAutoString msg;
  msg.AssignWithConversion(pluginname);
  msg.Append(NS_LITERAL_STRING("\n\n"));
  msg.Append(message);

  rv = prompt->ConfirmEx(title, msg.get(),
                         nsIPrompt::BUTTON_TITLE_OK * nsIPrompt::BUTTON_POS_0,
                         nsnull, nsnull, nsnull,
                         checkboxMessage, &checkboxState, &buttonPressed);


  if (checkboxState)
    mDontShowBadPluginMessage = PR_TRUE;

  nsMemory::Free((void *)title);
  nsMemory::Free((void *)message);
  nsMemory::Free((void *)checkboxMessage);
  return rv;
}

// nsPIPluginHost interface

////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP 
nsPluginHostImpl::SetIsScriptableInstance(nsCOMPtr<nsIPluginInstance> aPluginInstance, 
                                        PRBool aScriptable)
{
  nsActivePlugin * p = mActivePluginList.find(aPluginInstance.get());
  if(p == nsnull)
    return NS_ERROR_FAILURE;

  p->mXPConnected = aScriptable;
  if(p->mPluginTag)
    p->mPluginTag->mXPConnected = aScriptable;

  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginHostImpl::ParsePostBufferToFixHeaders(
                            const char *inPostData, PRUint32 inPostDataLen, 
                            char **outPostData, PRUint32 *outPostDataLen)
{
  if (!inPostData || !outPostData || !outPostDataLen)
    return NS_ERROR_NULL_POINTER;
  
  *outPostData = 0;
  *outPostDataLen = 0;

  const char CR = '\r';
  const char LF = '\n';
  const char CRLFCRLF[] = {CR,LF,CR,LF,'\0'}; // C string"\r\n\r\n" 
  const char ContentLenHeader[] = "Content-length";

  nsAutoVoidArray singleLF;
  const char *pSCntlh = 0;// pointer to start of ContentLenHeader in inPostData
  const char *pSod = 0;   // pointer to start of data in inPostData
  const char *pEoh = 0;   // pointer to end of headers in inPostData
  const char *pEod = inPostData + inPostDataLen; // pointer to end of inPostData
  if (*inPostData == LF) {
    // from 4.x spec http://developer.netscape.com/docs/manuals/communicator/plugin/pgfn2.htm#1007754
    // If no custom headers are required, simply add a blank
    // line ('\n') to the beginning of the file or buffer.
    // so *inPostData == '\n' is valid
    pSod = inPostData + 1;
  } else {       
    const char *s = inPostData; //tmp pointer to sourse inPostData
    while (s < pEod) {
      if (!pSCntlh && 
          (*s == 'C' || *s == 'c') && 
          (s + sizeof(ContentLenHeader) - 1 < pEod) &&
          (!PL_strncasecmp(s, ContentLenHeader, sizeof(ContentLenHeader) - 1)))
      {
        // lets assume this is ContentLenHeader for now
        const char *p = pSCntlh = s;
        p += sizeof(ContentLenHeader) - 1;
        // search for first CR or LF == end of ContentLenHeader
        for (; p < pEod; p++) {
          if (*p == CR || *p == LF) {
            // got delimiter, 
            // one more check; if previous char is a digit 
            // most likely pSCntlh points to the start of ContentLenHeader
            if (*(p-1) >= '0' && *(p-1) <= '9') {
              s = p;
            }
            break; //for loop
          }
        }
        if (pSCntlh == s) { // curret ptr is the same 
          pSCntlh = 0; // that was not ContentLenHeader
          break; // there is nothing to parse, break *WHILE LOOP* here
        }
      }

      if (*s == CR) {
        if (pSCntlh && // only if ContentLenHeader is found we are looking for end of headers
            ((s + sizeof(CRLFCRLF)-1) <= pEod) &&
            !memcmp(s, CRLFCRLF, sizeof(CRLFCRLF)-1))
        {
          s += sizeof(CRLFCRLF)-1;
          pEoh = pSod = s; // data stars here
          break;
        }
      } else if (*s == LF) {
        if (*(s-1) != CR) {
          singleLF.AppendElement((void*)s);
        }
        if (pSCntlh && (s+1 < pEod) && (*(s+1) == LF)) {
          s++;
          singleLF.AppendElement((void*)s);
          s++;
          pEoh = pSod = s; // data stars here
          break;
        }
      }
      s++;
    }
  }

  // deal with output buffer
  if (!pSod) { // lets assume whole buffer is a data
    pSod = inPostData;
  }

  PRUint32 newBufferLen = 0;
  PRUint32 dataLen = pEod - pSod;
  PRUint32 headersLen = pEoh ? pSod - inPostData : 0;
  
  char *p; // tmp ptr into new output buf
  if (headersLen) { // we got a headers
    // this function does not make any assumption on correctness 
    // of ContentLenHeader value in this case.

    newBufferLen = dataLen + headersLen;
    // in case there were single LFs in headers
    // reserve an extra space for CR will be added before each single LF
    int cntSingleLF = singleLF.Count();
    newBufferLen += cntSingleLF;

    if (!(*outPostData = p = (char*)nsMemory::Alloc(newBufferLen)))
      return NS_ERROR_OUT_OF_MEMORY;

    // deal with single LF
    const char *s = inPostData;
    if (cntSingleLF) {
      for (int i=0; i<cntSingleLF; i++) {
        const char *plf = (const char*) singleLF.ElementAt(i); // ptr to single LF in headers
        int n = plf - s; // bytes to copy
        if (n) { // for '\n\n' there is nothing to memcpy
          memcpy(p, s, n);
          p += n;
        }
        *p++ = CR;
        s = plf;
        *p++ = *s++;
      }
    }
    // are we done with headers?
    headersLen = pEoh - s;
    if (headersLen) { // not yet
      memcpy(p, s, headersLen); // copy the rest
      p += headersLen;
    }
  } else  if (dataLen) { // no ContentLenHeader is found but there is a data
    // make new output buffer big enough
    // to keep ContentLenHeader+value followed by data
    PRUint32 l = sizeof(ContentLenHeader) + sizeof(CRLFCRLF) + 32;
    newBufferLen = dataLen + l;
    if (!(*outPostData = p = (char*)nsMemory::Alloc(newBufferLen)))
      return NS_ERROR_OUT_OF_MEMORY;
    headersLen = PR_snprintf(p, l,"%s: %ld%s", ContentLenHeader, dataLen, CRLFCRLF);
    if (headersLen == l) { // if PR_snprintf has ate all extra space consider this as an error
      nsMemory::Free(p);
      *outPostData = 0;
      return NS_ERROR_FAILURE;
    }
    p += headersLen;
    newBufferLen = headersLen + dataLen;
  }
  // at this point we've done with headers.
  // there is a possibility that input buffer has only headers info in it
  // which already parsed and copied into output buffer.
  // copy the data
  if (dataLen) {
    memcpy(p, pSod, dataLen);
  }

  *outPostDataLen = newBufferLen;
  
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsPluginHostImpl::CreateTmpFileToPost(const char *postDataURL, char **pTmpFileName) 
{
  *pTmpFileName = 0;
  nsresult rv;
  PRInt64 fileSize;
  nsCAutoString filename;
  // stat file == get size & convert file:///c:/ to c: if needed
  nsCOMPtr<nsILocalFile> inFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_FAILED(rv) || 
    (NS_FAILED(rv = NS_InitFileFromURLSpec(inFile, nsDependentCString(postDataURL))) &&
     NS_FAILED(rv = inFile->InitWithNativePath(nsDependentCString(postDataURL)))) ||
    NS_FAILED(rv = inFile->GetFileSize(&fileSize)) ||
    NS_FAILED(rv = inFile->GetNativePath(filename))
    )
    return rv;
  if (!LL_IS_ZERO(fileSize)) {
    nsCOMPtr<nsIInputStream> inStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inStream), inFile);
    if (NS_FAILED(rv)) return rv;
    
    // Create a temporary file to write the http Content-length: %ld\r\n\" header 
    // and "\r\n" == end of headers for post data to
    nsCOMPtr<nsIFile> tempFile;
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tempFile));
    if (NS_FAILED(rv))
      return rv;
    
    rv = tempFile->AppendNative(kPluginTmpDirName);
    if (NS_FAILED(rv))
      return rv;

    PRBool dirExists;
    tempFile->Exists(&dirExists);
    if (!dirExists)
      (void) tempFile->Create(nsIFile::DIRECTORY_TYPE, 0600);
    
    nsCAutoString inFileName;
    inFile->GetNativeLeafName(inFileName);
    // XXX hack around bug 70083
    inFileName.Insert(NS_LITERAL_CSTRING("post-"), 0);
    rv = tempFile->AppendNative(inFileName);
    
    if (NS_FAILED(rv)) 
      return rv;
    
    // make it unique, and mode == 0600, not world-readable
    rv = tempFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600); 
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIOutputStream> outStream;
    if (NS_SUCCEEDED(rv)) {
      rv = NS_NewLocalFileOutputStream(getter_AddRefs(outStream),
        tempFile,
        (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE),
        0600); // 600 so others can't read our form data
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "Post data file couldn't be created!");
    if (NS_FAILED(rv))
      return rv;

    char buf[1024];
    PRUint32 br, bw;
    PRBool firstRead = PR_TRUE;
    while (1) {
      // Read() mallocs if buffer is null
      rv = inStream->Read(buf, 1024, &br);
      if (NS_FAILED(rv) || (PRInt32)br <= 0) 
        break;
      if (firstRead) {
        // according to the 4.x spec 
        // http://developer.netscape.com/docs/manuals/communicator/plugin/pgfn2.htm#1007707
        //"For protocols in which the headers must be distinguished from the body, 
        // such as HTTP, the buffer or file should contain the headers, followed by
        // a blank line, then the body. If no custom headers are required, simply 
        // add a blank line ('\n') to the beginning of the file or buffer.
    
        char *parsedBuf;
        // assuming first 1K (or what we got) has all headers in,
        // lets parse it through nsPluginHostImpl::ParsePostBufferToFixHeaders()
        ParsePostBufferToFixHeaders((const char *)buf, br, &parsedBuf, &bw);
        rv = outStream->Write(parsedBuf, bw, &br);
        nsMemory::Free(parsedBuf);
        if (NS_FAILED(rv) || (bw != br)) 
          break;

        firstRead = PR_FALSE;
        continue;
      }
      bw = br;
      rv = outStream->Write(buf, bw, &br);
      if (NS_FAILED(rv) || (bw != br)) 
        break;
    }

    inStream->Close();
    outStream->Close();
    if (NS_SUCCEEDED(rv)) {
      nsCAutoString path;
      if (NS_SUCCEEDED(tempFile->GetNativePath(path)))
        *pTmpFileName = ToNewCString(path);
    }
  }
  return rv;
}

nsresult
nsPluginHostImpl::ScanForRealInComponentsFolder(nsIComponentManager * aCompManager, nsIFile * aLayoutPath)
{
  nsresult rv = NS_OK;

#ifdef XP_WIN
  
  // First, lets check if we already have Real. No point in doing this if it's installed correctly
  if (NS_SUCCEEDED(IsPluginEnabledForType("audio/x-pn-realaudio-plugin")))
    return rv;
  
  // Next, maybe the pref wants to override
  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  PRBool bSkipRealPlayerHack = PR_FALSE;
  if (!prefs ||
     (NS_SUCCEEDED(prefs->GetBoolPref("plugin.skip_real_player_hack", &bSkipRealPlayerHack)) &&
     bSkipRealPlayerHack))
  return rv;
    
  // now we need the XPCOM components folder
  nsCOMPtr<nsIFile> RealPlugin;
  if (NS_FAILED(NS_GetSpecialDirectory(NS_XPCOM_COMPONENT_DIR, getter_AddRefs(RealPlugin))) || !RealPlugin)
    return rv;
    
  // make sure the file is actually there
  RealPlugin->AppendNative(NS_LITERAL_CSTRING("nppl3260.dll"));
  PRBool exists;
  nsCAutoString filePath;
  RealPlugin->Exists(&exists);
  if (!exists || NS_FAILED(RealPlugin->GetNativePath(filePath)))
    return rv;

  // now make sure it's a plugin
  nsFileSpec file(filePath.get());
  if (!nsPluginsDir::IsPluginFile(file))
    return rv;
  
  // try to get the mime info and descriptions out of the plugin
  nsPluginFile pluginFile(file);
  nsPluginInfo info = { sizeof(info) };
  if (NS_FAILED(pluginFile.GetPluginInfo(info)))
    return rv;
  
  nsCOMPtr<nsIFile> layoutPath;
  nsCOMPtr<nsIComponentManager> compManager = do_GetService(kComponentManagerCID, &rv);
  
  // finally, create our "plugin tag" and add it to the list
  if (info.fMimeTypeArray) {
    nsPluginTag *pluginTag = new nsPluginTag(&info);
    if (pluginTag) {
      pluginTag->mNext = mPlugins;
      mPlugins = pluginTag;
      
      // last thing we need is to register this plugin with layout so it can be used in full-page mode
      if(aLayoutPath)
        RegisterPluginMimeTypesWithLayout(pluginTag, aCompManager, aLayoutPath);
    }
  }
          
  // free allocated strings in GetPluginInfo
  pluginFile.FreePluginInfo(info);

#endif

  return rv;
}

////////////////////////////////////////////////////////////////////////////////////
nsresult nsPluginStreamListenerPeer::ServeStreamAsFile(nsIRequest *request, 
                                                        nsISupports* aContext,
                                                        nsresult aStatus)
{
  if (!mInstance)
    return NS_ERROR_FAILURE;

  // mInstance->Stop calls mPStreamListener->CleanUpStream(), so stream will be properly clean up 
  mInstance->Stop();
  mInstance->Start();      
  nsPluginInstancePeerImpl *peer;
  mInstance->GetPeer(NS_REINTERPRET_CAST(nsIPluginInstancePeer **, &peer));
  if (peer) {
    nsCOMPtr<nsIPluginInstanceOwner> owner;
    peer->GetOwner(*getter_AddRefs(owner));
    if (owner) {
      nsPluginWindow    *window = nsnull;
      owner->GetWindow(window);
      if (window->window)
        mInstance->SetWindow(window);
    }
    // because we are starting plugin using existing peer 
    // it'll get one extra RefCnt, release it here.
    NS_RELEASE(peer);
  }
  
  mPluginStreamInfo->SetSeekable(0);
  mPStreamListener->OnStartBinding((nsIPluginStreamInfo*)mPluginStreamInfo);
  mPluginStreamInfo->SetStreamOffset(0);

#if !defined(CACHE_SUPPOPTS_FILE_EXTENSION)
  // check out if we already saving the stream into plugins cache
  nsCOMPtr<nsIOutputStream> outStream;
  mPluginStreamInfo->GetLocalCachedFileStream(getter_AddRefs(outStream));
  if (outStream) {
    outStream->Close();
    mPluginStreamInfo->SetLocalCachedFileStream(nsnull);
  }
  
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);
  if (httpChannel) {
    SetupPluginCacheFile(channel);
  }
#endif

  return NS_OK;
}

//////////////////////////////////////////////////////////////////////
NS_IMPL_ISUPPORTS1(nsPluginByteRangeStreamListener, nsIStreamListener)

nsPluginByteRangeStreamListener::nsPluginByteRangeStreamListener(nsPluginStreamListenerPeer* aPluginStreamListenerPeer)
{
  NS_INIT_REFCNT();
  // don't addref
  mFinalPluginStreamListener = aPluginStreamListenerPeer;
}

nsPluginByteRangeStreamListener::~nsPluginByteRangeStreamListener()
{
  mStreamConverter = 0;
}

NS_IMETHODIMP
nsPluginByteRangeStreamListener::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  nsresult rv;

  nsCOMPtr<nsIStreamConverterService> serv = do_GetService(kStreamConverterServiceCID, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = serv->AsyncConvertData(NS_LITERAL_STRING(MULTIPART_BYTERANGES).get(),
                                NS_LITERAL_STRING("*/*").get(),
                                mFinalPluginStreamListener,
                                nsnull,
                                getter_AddRefs(mStreamConverter));
    if (NS_SUCCEEDED(rv)) {
      rv = mStreamConverter->OnStartRequest(request, ctxt);
      if (NS_SUCCEEDED(rv))
        return rv;
    }
    mStreamConverter = 0;
  }

  // if server cannot continue with byte range (206 status) and sending us whole object (200 status)
  // reset this seekable stream & try serve it to plugin instance as a file
  rv = mFinalPluginStreamListener->ServeStreamAsFile(request, ctxt, rv);
  // always use mFinalPluginStreamListener
  mStreamConverter = mFinalPluginStreamListener;
  return rv;
}

NS_IMETHODIMP
nsPluginByteRangeStreamListener::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                              nsresult status)
{
  if (mStreamConverter == mFinalPluginStreamListener) {
    // remove magic number from container
    nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(ctxt);
    if (container) {
      PRUint32 magicNumber = 0;
      container->GetData(&magicNumber);
      if (magicNumber == MAGIC_REQUEST_CONTEXT) {
        // to allow properly finish mFinalPluginStreamListener->OnStopRequest()
        // set it to something that is not the magic number.
        container->SetData(0);
        // also unset mPendingRequests 
        mFinalPluginStreamListener->mPendingRequests = 0;
     
        return mFinalPluginStreamListener->OnStopRequest(request, ctxt, status);
      }
    }
    NS_WARNING("Bad state of nsPluginByteRangeStreamListener");
  }
  
  return mStreamConverter->OnStopRequest(request, ctxt, status);
}

NS_IMETHODIMP
nsPluginByteRangeStreamListener::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                                nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  return mStreamConverter->OnDataAvailable(request, ctxt, inStr, sourceOffset, count);
}
