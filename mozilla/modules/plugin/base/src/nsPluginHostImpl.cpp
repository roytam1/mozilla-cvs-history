/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsPluginHostImpl.h"
#include <stdio.h>
#include "prio.h"
#include "prmem.h"
#include "ns4xPlugin.h"
#include "nsPluginInstancePeer.h"

#include "nsIPlugin.h"
#include "nsIPluginStreamListener.h"
#include "nsIHTTPHeaderListener.h" 
#include "nsIHTTPHeader.h"
#include "nsIObserverService.h"
#include "nsIHTTPProtocolHandler.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIURL.h"
#include "nsXPIDLString.h"
#include "nsIPref.h"
#include "nsIProxyAutoConfig.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsIHTTPChannel.h"
#include "nsIStreamAsFile.h"
#include "nsIFileStream.h" // for nsIRandomAccessStore
#include "nsNetUtil.h"
#include "nsIProgressEventSink.h"
#include "nsIDocument.h"


#if MOZ_NEW_CACHE
#include "nsICachingChannel.h"
#endif
// Friggin' X11 has to "#define None". Lame!
#ifdef None
#undef None
#endif

#include "nsIRegistry.h"
#include "nsEnumeratorUtils.h"

// for the dialog
#include "nsIStringBundle.h"
#include "nsINetSupportDialogService.h"
#include "nsIPrompt.h"
#include "nsHashtable.h"

#include "nsILocale.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIPrincipal.h"

#include "nsIServiceManager.h"
#include "nsICookieStorage.h"
#include "nsICookieService.h"
#include "nsIDOMPlugin.h"
#include "nsIDOMMimeType.h"
#include "prprf.h"

#if defined(XP_PC) && !defined(XP_OS2)
#include "windows.h"
#include "winbase.h"
#endif

#include "nsSpecialSystemDirectory.h"
#include "nsFileSpec.h"

#include "nsPluginDocLoaderFactory.h"
#include "nsIDocumentLoaderFactory.h"

#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"

#ifdef XP_UNIX
#if defined(MOZ_WIDGET_GTK)
#include <gdk/gdkx.h> // for GDK_DISPLAY()
#elif defined(MOZ_WIDGET_QT)
#include <qwindowdefs.h> // for qt_xdisplay()
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
#define PLUGIN_DLL "PLUGIN_DLL"
#endif

#define REL_PLUGIN_DLL "rel:" PLUGIN_DLL

static NS_DEFINE_IID(kIPluginInstanceIID, NS_IPLUGININSTANCE_IID);
static NS_DEFINE_IID(kIPluginInstancePeerIID, NS_IPLUGININSTANCEPEER_IID); 
static NS_DEFINE_IID(kIPluginStreamInfoIID, NS_IPLUGINSTREAMINFO_IID);
static NS_DEFINE_CID(kPluginCID, NS_PLUGIN_CID);
static NS_DEFINE_IID(kIPluginTagInfo2IID, NS_IPLUGINTAGINFO2_IID); 
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_CID(kCookieServiceCID, NS_COOKIESERVICE_CID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIStreamListenerIID, NS_ISTREAMLISTENER_IID);
static NS_DEFINE_IID(kIStreamObserverIID, NS_ISTREAMOBSERVER_IID);

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kHTTPHandlerCID, NS_IHTTPHANDLER_CID);

static NS_DEFINE_IID(kIFileUtilitiesIID, NS_IFILEUTILITIES_IID);
static NS_DEFINE_IID(kIOutputStreamIID, NS_IOUTPUTSTREAM_IID);

static NS_DEFINE_CID(kRegistryCID, NS_REGISTRY_CID);

// for the dialog
static NS_DEFINE_CID(kNetSupportDialogCID, NS_NETSUPPORTDIALOG_CID);
static NS_DEFINE_IID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

#define PLUGIN_PROPERTIES_URL "chrome://global/locale/downloadProgress.properties"
#define PLUGIN_REGIONAL_URL "chrome://global-region/locale/region.properties"

// #defines for reading prefs and extra search plugin paths from windows registry
#define _MAXKEYVALUE_ 8196
#define _NS_PREF_COMMON_PLUGIN_REG_KEY_ "browser.plugins.registry_plugins_folder_key_location"
#define _NS_COMMON_PLUGIN_KEY_NAME_ "Plugins Folders"

// #defines for plugin cache and prefs
#define NS_PREF_MAX_NUM_CACHED_PLUGINS "browser.plugins.max_num_cached_plugins"
#define DEFAULT_NUMBER_OF_STOPPED_PLUGINS 10

void DisplayNoDefaultPluginDialog(const char *mimeType);

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
  nsCOMPtr<nsIPrompt> prompt(do_GetService(kNetSupportDialogCID));
  nsCOMPtr<nsIIOService> io(do_GetService(kIOServiceCID));
  nsCOMPtr<nsIStringBundleService> strings(do_GetService(kStringBundleServiceCID));
  nsCOMPtr<nsIStringBundle> bundle;
  nsCOMPtr<nsIStringBundle> regionalBundle;
  nsCOMPtr<nsIURI> uri;
  char *spec = nsnull;
  nsILocale* locale = nsnull;
  PRInt32 buttonPressed;
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
  rv = strings->CreateBundle(PLUGIN_PROPERTIES_URL, locale, getter_AddRefs(bundle));
  if (NS_FAILED(rv)) {
    return;
  }
  rv = strings->CreateBundle(PLUGIN_REGIONAL_URL, locale, 
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

  rv = prompt->UniversalDialog(
                               nsnull, /* title message */
                               titleUni, /* title text in top line of window */
                               messageUni, /* this is the main message */
                               checkboxMessageUni, /* This is the checkbox message */
                               nsnull, /* first button text, becomes OK by default */
                               nsnull, /* second button text, becomes CANCEL by default */
                               nsnull, /* third button text */
                               nsnull, /* fourth button text */
                               nsnull, /* first edit field label */
                               nsnull, /* second edit field label */
                               nsnull, /* first edit field initial and final value */
                               nsnull, /* second edit field initial and final value */
                               nsnull,  /* icon: question mark by default */
                               &checkboxState, /* initial and final value of checkbox */
                               1, /* number of buttons */
                               0, /* number of edit fields */
                               0, /* is first edit field a password field */
                               
                               &buttonPressed);

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

nsActivePlugin::nsActivePlugin(nsCOMPtr<nsIPlugin> aPlugin,
                               nsIPluginInstance* aInstance, 
                               char * url,
                               PRBool aDefaultPlugin)
{
  mNext = nsnull;
  mPeer = nsnull;
  mPlugin = aPlugin;

  mURL = PL_strdup(url);
  mInstance = aInstance;
  if(aInstance != nsnull)
  {
    aInstance->GetPeer(&mPeer);
    NS_ADDREF(aInstance);
  }
  mDefaultPlugin = aDefaultPlugin;
  mStopped = PR_FALSE;
  mllStopTime = LL_ZERO;
}

nsActivePlugin::~nsActivePlugin()
{
  mPlugin = nsnull;
  if(mInstance != nsnull)
  {
    mInstance->Destroy();
    NS_RELEASE(mInstance);
    NS_RELEASE(mPeer);
  }
  PL_strfree(mURL);
}

void nsActivePlugin::setStopped(PRBool stopped)
{
  mStopped = stopped;
  if(mStopped)
    mllStopTime = PR_Now();
  else
    mllStopTime = LL_ZERO;
}

nsActivePluginList::nsActivePluginList()
{
  mFirst = nsnull;
  mLast = nsnull;
  mCount = 0;
}

nsActivePluginList::~nsActivePluginList()
{
  if(mFirst == nsnull)
    return;
  shut();
}

void nsActivePluginList::shut()
{
  if(mFirst == nsnull)
    return;

  for(nsActivePlugin * plugin = mFirst; plugin != nsnull;)
  {
    nsActivePlugin * next = plugin->mNext;
    remove(plugin);
    plugin = next;
  }
  mFirst = nsnull;
  mLast = nsnull;
}

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

PRBool nsActivePluginList::IsLastInstance(nsActivePlugin * plugin)
{
  if(!plugin)
    return PR_FALSE;

  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if((p->mPlugin.get() == plugin->mPlugin.get()) && (p != plugin))
      return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool nsActivePluginList::remove(nsActivePlugin * plugin)
{
  if(mFirst == nsnull)
    return PR_FALSE;

  nsActivePlugin * prev = nsnull;
  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if(p == plugin)
    {
      if(p == mFirst)
        mFirst = p->mNext;
      else
        prev->mNext = p->mNext;

      if((prev != nsnull) && (prev->mNext == nsnull))
        mLast = prev;

      // see if this is going to be the last instance of a plugin
      if(IsLastInstance(p))
      {
        nsIPlugin *nsiplugin = p->mPlugin.get();
        
        delete p; // plugin instance is destroyed here
        
        if(nsiplugin)
          nsiplugin->Shutdown();
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

void nsActivePluginList::stopRunning()
{
  if(mFirst == nsnull)
    return;

  for(nsActivePlugin * p = mFirst; p != nsnull; p = p->mNext)
  {
    if(!p->mStopped && p->mInstance)
    {
      p->mInstance->SetWindow(nsnull);
      p->mInstance->Stop();
      p->setStopped(PR_TRUE);
    }
  }
}

void nsActivePluginList::removeAllStopped()
{
  if(mFirst == nsnull)
    return;

  nsActivePlugin * prev = nsnull;
  nsActivePlugin * next = nsnull;

  for(nsActivePlugin * p = mFirst; p != nsnull;)
  {
    next = p->mNext;

    if(p->mStopped)
    {
      if(p == mFirst)
        mFirst = next;
      else
        prev->mNext = next;

      if(p == mLast)
        mLast = prev;

      delete p;
      mCount--;
      p = next;
      continue;
    }
    prev = p;
    p = next;
  }
  return;
}

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

nsActivePlugin * nsActivePluginList::find(char * mimetype)
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

nsActivePlugin * nsActivePluginList::findStopped(char * url)
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

nsUnloadedLibrary::nsUnloadedLibrary(PRLibrary * aLibrary)
{
  mLibrary = aLibrary;
}

nsUnloadedLibrary::~nsUnloadedLibrary()
{
  if(mLibrary)
    PR_UnloadLibrary(mLibrary);
}

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
  mFileName = nsnull;
}

inline char* new_str(const char* str)
{
  if(str == nsnull)
    return nsnull;

	char* result = new char[strlen(str) + 1];
	if (result != nsnull)
		return strcpy(result, str);
	return result;
}

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
  mFileName = new_str(aPluginTag->mFileName);
}

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

  mLibrary = nsnull;
  mCanUnloadLibrary = PR_TRUE;
  mEntryPoint = nsnull;
  mFlags = NS_PLUGIN_FLAG_ENABLED;
}


nsPluginTag::nsPluginTag(const char* aName,
                         const char* aDescription,
                         const char* aFileName,
                         const char* const* aMimeTypes,
                         const char* const* aMimeDescriptions,
                         const char* const* aExtensions,
                         PRInt32 aVariants)
  : mNext(nsnull),
    mVariants(aVariants),
    mMimeTypeArray(nsnull),
    mMimeDescriptionArray(nsnull),
    mExtensionsArray(nsnull),
    mLibrary(nsnull),
    mCanUnloadLibrary(PR_TRUE),
    mEntryPoint(nsnull),
    mFlags(0)
{
  mName            = new_str(aName);
  mDescription     = new_str(aDescription);
  mFileName        = new_str(aFileName);

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
  NS_IF_RELEASE(mEntryPoint);

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

  if ((nsnull != mLibrary) && mCanUnloadLibrary)
  {
    // before we unload check if we are allowed to, see bug #61388
    PR_UnloadLibrary(mLibrary);
    mLibrary = nsnull;
  }
  
  if(nsnull != mFileName)
  {
    delete [] mFileName;
    mFileName = nsnull;
  }
}

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

private:

	char* mContentType;
	char* mURL;
	PRBool mSeekable;
	PRUint32 mLength;
	PRUint32 mModified;
};

nsPluginStreamInfo::nsPluginStreamInfo()
{
	NS_INIT_REFCNT();

	mContentType = nsnull;
	mURL = nsnull;
	mSeekable = PR_FALSE;
	mLength = 0;
	mModified = 0;
}

nsPluginStreamInfo::~nsPluginStreamInfo()
{
	if(mContentType != nsnull)
		PL_strfree(mContentType);
    if(mURL != nsnull)
		PL_strfree(mURL);
}

NS_IMPL_ADDREF(nsPluginStreamInfo)
NS_IMPL_RELEASE(nsPluginStreamInfo)

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

NS_IMETHODIMP
nsPluginStreamInfo::GetContentType(nsMIMEType* result)
{
	*result = mContentType;
	return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::IsSeekable(PRBool* result)
{
	*result = mSeekable;
	return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::GetLength(PRUint32* result)
{
	*result = mLength;
	return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::GetLastModified(PRUint32* result)
{
	*result = mModified;
	return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::GetURL(const char** result)
{
	*result = mURL;
	return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::RequestRead(nsByteRange* rangeList)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

// local methods

void
nsPluginStreamInfo::SetContentType(const nsMIMEType contentType)
{	
	if(mContentType != nsnull)
		PL_strfree(mContentType);

	mContentType = PL_strdup(contentType);
}

void
nsPluginStreamInfo::SetSeekable(const PRBool seekable)
{
	mSeekable = seekable;
}

void
nsPluginStreamInfo::SetLength(const PRUint32 length)
{
	mLength = length;
}

void
nsPluginStreamInfo::SetLastModified(const PRUint32 modified)
{
	mModified = modified;
}

void
nsPluginStreamInfo::SetURL(const char* url)
{	
	if(mURL != nsnull)
		PL_strfree(mURL);

	mURL = PL_strdup(url);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

class nsPluginStreamListenerPeer : public nsIStreamListener
                                 , public nsIProgressEventSink
{
public:
  nsPluginStreamListenerPeer();
  virtual ~nsPluginStreamListenerPeer();

  NS_DECL_ISUPPORTS

  // nsIProgressEventSink methods:
  NS_DECL_NSIPROGRESSEVENTSINK

  // nsIStreamObserver methods:
  NS_DECL_NSISTREAMOBSERVER

  // nsIStreamListener methods:
  NS_DECL_NSISTREAMLISTENER

  //locals

  // Called by GetURL and PostURL (via NewStream)
  nsresult Initialize(nsIURI *aURL, nsIPluginInstance *aInstance, 
                      nsIPluginStreamListener *aListener);

  nsresult InitializeEmbeded(nsIURI *aURL, nsIPluginInstance* aInstance, 
                             nsIPluginInstanceOwner *aOwner = nsnull,
                             nsIPluginHost *aHost = nsnull);

  nsresult InitializeFullPage(nsIPluginInstance *aInstance);

  nsresult OnFileAvailable(const char* aFilename);

  nsILoadGroup* GetLoadGroup();

  NS_IMETHOD
  ReadHeadersFromChannelAndPostToListener(nsIHTTPChannel *httpChannel,
                                          nsIHTTPHeaderListener *list);
  


private:

  nsresult SetUpCache(nsIURI* aURL);
  nsresult SetUpStreamListener(nsIRequest* request, nsIURI* aURL);

  nsIURI                  *mURL;
  nsIPluginInstanceOwner  *mOwner;
  nsIPluginInstance       *mInstance;

  nsIPluginStreamListener *mPStreamListener;
  nsPluginStreamInfo	  *mPluginStreamInfo;
  PRBool		  mSetUpListener;

  /*

   * Set to PR_TRUE after nsIPluginInstancePeer::OnStartBinding() has
   * been called.  Checked in ::OnStopRequest so we can call the
   * plugin's OnStartBinding if, for some reason, it has not already
   * been called.

   */

  PRBool		  mStartBinding;

  // these get passed to the plugin stream listener
  char                    *mMIMEType;
  PRUint32                mLength;
  nsPluginStreamType      mStreamType;
  nsIPluginHost           *mHost;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class nsPluginCacheListener : public nsIStreamListener
{
public:
  nsPluginCacheListener(nsPluginStreamListenerPeer* aListener);
  virtual ~nsPluginCacheListener();

  NS_DECL_ISUPPORTS

  NS_DECL_NSISTREAMOBSERVER
  NS_DECL_NSISTREAMLISTENER

private:
  nsPluginStreamListenerPeer* mListener;
};

nsPluginCacheListener::nsPluginCacheListener(nsPluginStreamListenerPeer* aListener)
{
  NS_INIT_REFCNT();

  mListener = aListener;
  NS_ADDREF(mListener);
}

nsPluginCacheListener::~nsPluginCacheListener()
{
  NS_IF_RELEASE(mListener);
}

NS_IMPL_ISUPPORTS(nsPluginCacheListener, kIStreamListenerIID);

NS_IMETHODIMP
nsPluginCacheListener::OnStartRequest(nsIRequest *request, nsISupports* ctxt)
{
  return NS_OK;
}

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

NS_IMETHODIMP 
nsPluginCacheListener::OnStopRequest(nsIRequest *request, 
                                     nsISupports* aContext, 
                                     nsresult aStatus, 
                                     const PRUnichar* aMsg)
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
}

nsPluginStreamListenerPeer::~nsPluginStreamListenerPeer()
{
#ifdef NS_DEBUG
  if(mURL != nsnull)
  {
    char* spec;
	(void)mURL->GetSpec(&spec);
	printf("killing stream for %s\n", mURL ? spec : "(unknown URL)");
	nsCRT::free(spec);
  }
#endif

  NS_IF_RELEASE(mURL);
  NS_IF_RELEASE(mOwner);
  NS_IF_RELEASE(mInstance);
  NS_IF_RELEASE(mPStreamListener);
  NS_IF_RELEASE(mHost);
}

NS_IMPL_ADDREF(nsPluginStreamListenerPeer);
NS_IMPL_RELEASE(nsPluginStreamListenerPeer);

nsresult nsPluginStreamListenerPeer::QueryInterface(const nsIID& aIID,
                                                    void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");

  if (nsnull == aInstancePtrResult)
    return NS_ERROR_NULL_POINTER;

  if (aIID.Equals(kIStreamListenerIID))
  {
    *aInstancePtrResult = (void *)((nsIStreamListener *)this);
    AddRef();
    return NS_OK;
  }

  if (aIID.Equals(kIStreamObserverIID))
  {
    *aInstancePtrResult = (void *)((nsIStreamObserver *)this);
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

/* Called as a result of GetURL and PostURL */

nsresult nsPluginStreamListenerPeer::Initialize(nsIURI *aURL, 
                                                nsIPluginInstance *aInstance,
                                                nsIPluginStreamListener* aListener)
{
#ifdef NS_DEBUG
  char* spec;
  (void)aURL->GetSpec(&spec);
  printf("created stream for %s\n", spec);
  nsCRT::free(spec);
#endif

  mURL = aURL;
  NS_ADDREF(mURL);

  mInstance = aInstance;
  NS_ADDREF(mInstance);
  
  mPStreamListener = aListener;
  NS_ADDREF(mPStreamListener);

  mPluginStreamInfo = new nsPluginStreamInfo();

  return NS_OK;
}

/* 
	Called by NewEmbededPluginStream() - if this is called, we weren't 
    able to load the plugin, so we need to load it later once we figure 
    out the mimetype.  In order to load it later, we need the plugin 
    host and instance owner.
*/

nsresult nsPluginStreamListenerPeer::InitializeEmbeded(nsIURI *aURL, 
                                                       nsIPluginInstance* aInstance, 
                                                       nsIPluginInstanceOwner *aOwner,
                                                       nsIPluginHost *aHost)
{
#ifdef NS_DEBUG
  char* spec;
  (void)aURL->GetSpec(&spec);
  printf("created stream for %s\n", spec);
  nsCRT::free(spec);
#endif

  mURL = aURL;
  NS_ADDREF(mURL);

  if(aInstance != nsnull)
  {
    NS_ASSERTION(mInstance == nsnull, "nsPluginStreamListenerPeer::InitializeEmbeded mInstance != nsnull");
	  mInstance = aInstance;
	  NS_ADDREF(mInstance);
  }
  else
  {
	  mOwner = aOwner;
	  NS_IF_ADDREF(mOwner);

	  mHost = aHost;
	  NS_IF_ADDREF(mHost);
  }

  mPluginStreamInfo = new nsPluginStreamInfo();

  return NS_OK;
}

/* Called by NewFullPagePluginStream() */

nsresult nsPluginStreamListenerPeer::InitializeFullPage(nsIPluginInstance *aInstance)
{
#ifdef NS_DEBUG
  printf("created stream for (unknown URL)\n");
  printf("Inside nsPluginStreamListenerPeer::InitializeFullPage...\n");
#endif

  NS_ASSERTION(mInstance == nsnull, "nsPluginStreamListenerPeer::InitializeFullPage mInstance != nsnull");
  mInstance = aInstance;
  NS_ADDREF(mInstance);

  mPluginStreamInfo = new nsPluginStreamInfo();

  return NS_OK;
}


NS_IMETHODIMP
nsPluginStreamListenerPeer::OnStartRequest(nsIRequest *request, nsISupports* aContext)
{
  nsresult  rv = NS_OK;

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);

  if (!channel)
      return NS_ERROR_FAILURE;

#if MOZ_NEW_CACHE
    nsCOMPtr<nsICachingChannel> cacheChannel = do_QueryInterface(channel);
    if (cacheChannel) {
        rv = cacheChannel->SetCacheAsFile(PR_TRUE);
        if (NS_FAILED(rv)) {
           // FIX: Cache must be disabled.  We should try to stream this file
           // to disk ourselves otherwise OnFileAvailable will never be fired.
           NS_ASSERTION(PR_FALSE, "No Disk Cache Aval.  Some plugins wont work.");
        }
    }
#endif

  char* aContentType = nsnull;
  rv = channel->GetContentType(&aContentType);
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;

  if (nsnull != aContentType)
	  mPluginStreamInfo->SetContentType(aContentType);

  nsPluginWindow    *window = nsnull;

  // if we don't have an nsIPluginInstance (mInstance), it means
  // we weren't able to load a plugin previously because we
  // didn't have the mimetype.  Now that we do (aContentType),
  // we'll try again with SetUpPluginInstance() 
  // which is called by InstantiateEmbededPlugin()
  // NOTE: we don't want to try again if we didn't get the MIME type this time

  if ((nsnull == mInstance) && (nsnull != mOwner) && (nsnull != aContentType))
  {
    mOwner->GetInstance(mInstance);
    mOwner->GetWindow(window);

    if ((nsnull == mInstance) && (nsnull != mHost) && (nsnull != window))
    {
      // determine if we need to try embedded again. FullPage takes a different code path
      nsPluginMode mode;
      mOwner->GetMode(&mode);
      if (mode == nsPluginMode_Embedded)
        rv = mHost->InstantiateEmbededPlugin(aContentType, aURL, mOwner);
      else
        rv = mHost->SetUpPluginInstance(aContentType, aURL, mOwner);

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

  nsCRT::free(aContentType);

  //
  // Set up the stream listener...
  //
  PRInt32 length;

  rv = channel->GetContentLength(&length);

  // it's possible for the server to not send a Content-Length.  We should
  // still work in this case.
  if (NS_FAILED(rv)) {
    mPluginStreamInfo->SetLength(-1);
  }
  else {
    mPluginStreamInfo->SetLength(length);
  }


  rv = SetUpStreamListener(request, aURL);
  if (NS_FAILED(rv)) return rv;

  return rv;
}


NS_IMETHODIMP nsPluginStreamListenerPeer::OnProgress(nsIRequest *request, 
                                                     nsISupports* aContext, 
                                                     PRUint32 aProgress, 
                                                     PRUint32 aProgressMax)
{
  nsresult rv = NS_OK;
  return rv;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnStatus(nsIRequest *request, 
                                                   nsISupports* aContext,
                                                   nsresult aStatus,
                                                   const PRUnichar* aStatusArg)
{
  return NS_OK;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnDataAvailable(nsIRequest *request, 
                                                          nsISupports* aContext, 
                                                          nsIInputStream *aIStream, 
                                                          PRUint32 sourceOffset, 
                                                          PRUint32 aLength)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIURI> aURL;
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (!channel) return NS_ERROR_FAILURE;
  
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;

  if(!mPStreamListener)
	  return NS_ERROR_FAILURE;

  char* urlString;
  aURL->GetSpec(&urlString);
  mPluginStreamInfo->SetURL(urlString);
  nsCRT::free(urlString);

  // if the plugin has requested an AsFileOnly stream, then don't 
  // call OnDataAvailable
  if(mStreamType != nsPluginStreamType_AsFileOnly)
  {
    // It's up to the plugin to read from the stream 
    //  If it doesn't, OnStopRequest will never be called
    rv =  mPStreamListener->OnDataAvailable((nsIPluginStreamInfo*)mPluginStreamInfo, aIStream, aLength);
    // if a plugin returns an error, the peer must kill the stream
    //   else the stream and PluginStreamListener leak
    if (NS_FAILED(rv))
      request->Cancel(rv);
  }
  else
  {
    // if we don't read from the stream, OnStopRequest will never be called
    char* buffer = new char[aLength];
    PRUint32 amountRead;
    rv = aIStream->Read(buffer, aLength, &amountRead);
    delete [] buffer;
  }
  return rv;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnStopRequest(nsIRequest *request, 
                                                        nsISupports* aContext,
                                                        nsresult aStatus, 
                                                        const PRUnichar* aMsg)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIURI> aURL;
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (!channel) return NS_ERROR_FAILURE;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;

  if(nsnull != mPStreamListener)
  {
    char* urlString;
    nsCOMPtr<nsIFile> localFile;

#if MOZ_NEW_CACHE
    nsCOMPtr<nsICachingChannel> cacheChannel = do_QueryInterface(channel);
    if (cacheChannel)
        rv = cacheChannel->GetCacheFile(getter_AddRefs(localFile));
#else
    nsCOMPtr<nsIStreamAsFile> streamAsFile = do_QueryInterface(channel);
    if (streamAsFile)
        rv = streamAsFile->GetFile(getter_AddRefs(localFile));
#endif
    if (NS_SUCCEEDED(rv) && localFile)
    {
      char* pathAndFilename;
      rv = localFile->GetPath(&pathAndFilename);
      if (NS_SUCCEEDED(rv))
      {
        OnFileAvailable(pathAndFilename);
        nsMemory::Free(pathAndFilename);
      }
    }

    rv = aURL->GetSpec(&urlString);
    if (NS_SUCCEEDED(rv)) 
    {
      mPluginStreamInfo->SetURL(urlString);
      nsCRT::free(urlString);
    }

    // Set the content type to ensure we don't pass null to the plugin
    char* aContentType = nsnull;
    rv = channel->GetContentType(&aContentType);
    if (NS_FAILED(rv)) return rv;

    if (nsnull != aContentType)
      mPluginStreamInfo->SetContentType(aContentType);

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
  if (aContentType)
    nsCRT::free(aContentType);
  }

  return rv;
}


// private methods for nsPluginStreamListenerPeer

nsresult nsPluginStreamListenerPeer::SetUpCache(nsIURI* aURL)
{
	nsPluginCacheListener* cacheListener = new nsPluginCacheListener(this);
    // XXX: Null LoadGroup?
	return NS_OpenURI(cacheListener, nsnull, aURL, nsnull);
}

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
  

  /*

   * Assumption

   * By the time nsPluginStreamListenerPeer::OnDataAvailable() gets
   * called, all the headers have been read.

   */

  nsCOMPtr<nsIHTTPHeaderListener> headerListener = 
    do_QueryInterface(mPStreamListener);
  if (headerListener) {
    
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
    nsCOMPtr<nsIHTTPChannel>	httpChannel = do_QueryInterface(channel);
    if (httpChannel) {
      ReadHeadersFromChannelAndPostToListener(httpChannel, headerListener);
    }
  }

  
  mSetUpListener = PR_TRUE;
  mPluginStreamInfo->SetSeekable(PR_FALSE);
  
  // get Last-Modified header for plugin info
  
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  nsCOMPtr<nsIHTTPChannel>	theHTTPChannel = do_QueryInterface(channel);
  if (theHTTPChannel) {
     char * lastModified;
     nsCOMPtr<nsIAtom> header(dont_AddRef(NS_NewAtom("last-modified")));

     theHTTPChannel->GetResponseHeader(header, &lastModified);
     if (lastModified) {
       PRTime time64;
       PR_ParseTimeString(lastModified, PR_TRUE, &time64);  //convert string time to interger time
 
       // Convert PRTime to unix-style time_t, i.e. seconds since the epoch
       double fpTime;
       LL_L2D(fpTime, time64);
       mPluginStreamInfo->SetLastModified((PRUint32)(fpTime * 1e-6 + 0.5));
       nsCRT::free(lastModified);
     }
  } 

  char* urlString;
  aURL->GetSpec(&urlString);
  mPluginStreamInfo->SetURL(urlString);
  nsCRT::free(urlString);

  rv = mPStreamListener->OnStartBinding((nsIPluginStreamInfo*)mPluginStreamInfo);

  mStartBinding = PR_TRUE;

  if(rv == NS_OK)
  {
    mPStreamListener->GetStreamType(&mStreamType);
    // check to see if we need to cache the file as well
    if ((mStreamType == nsPluginStreamType_AsFile) || 
        (mStreamType == nsPluginStreamType_AsFileOnly))
    rv = SetUpCache(aURL);
  }

  return rv;
}

nsresult
nsPluginStreamListenerPeer::OnFileAvailable(const char* aFilename)
{
  nsresult rv;
  if (!mPStreamListener)
    return NS_ERROR_FAILURE;

  rv = mPStreamListener->OnFileAvailable((nsIPluginStreamInfo*)mPluginStreamInfo, aFilename);
  return rv;
}

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


NS_IMETHODIMP
nsPluginStreamListenerPeer::
ReadHeadersFromChannelAndPostToListener(nsIHTTPChannel *httpChannel,
                                        nsIHTTPHeaderListener *listener)
{
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsISimpleEnumerator>	enumerator;
  if (NS_FAILED(rv = httpChannel->
                GetResponseHeaderEnumerator(getter_AddRefs(enumerator)))) {
    return rv;
  }
  PRBool			bMoreHeaders;
  nsCOMPtr<nsISupports>   item;
  nsCOMPtr<nsIHTTPHeader>	header;
  char                    *name = nsnull;
  char	                  *val = nsnull;
  
  while (NS_SUCCEEDED(rv = enumerator->HasMoreElements(&bMoreHeaders))
         && (bMoreHeaders == PR_TRUE)) {
    enumerator->GetNext(getter_AddRefs(item));
    header = do_QueryInterface(item);
    NS_ASSERTION(header, "nsPluginHostImpl::ReadHeadersFromChannelAndPostToListener - Bad HTTP header.");
    if (header)	{

      /*

       * Assumption: 

       * The return value from nsIHTTPHeader->{GetFieldName,GetValue}()
       * must be freed.

       */

      header->GetFieldName(&name);
      header->GetValue(&val);
      if (NS_FAILED(rv = listener->NewResponseHeader(name, val))) {
        break;
      }
      nsCRT::free(name); 
      name = nsnull;
      nsCRT::free(val); 
      val = nsnull;
    }
    else {
      rv = NS_ERROR_NULL_POINTER;
      break;
    }
  }
  
  return rv;
}

/////////////////////////////////////////////////////////////////////////

nsPluginHostImpl::nsPluginHostImpl()
{
  NS_INIT_REFCNT();
  mPluginsLoaded = PR_FALSE;
  mDontShowBadPluginMessage = PR_FALSE;
  mIsDestroyed = PR_FALSE;
  mUnloadedLibraries = nsnull;

  nsCOMPtr<nsIObserverService> obsService = do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
  if (obsService)
    obsService->AddObserver(this, NS_LITERAL_STRING("quit-application").get());
}

nsPluginHostImpl::~nsPluginHostImpl()
{
#ifdef NS_DEBUG
printf("killing plugin host\n");
#endif
  Destroy();
  if (nsnull != mPluginPath)
  {
    PR_Free(mPluginPath);
    mPluginPath = nsnull;
  }

  while (nsnull != mPlugins)
  {
    nsPluginTag *temp = mPlugins->mNext;
    delete mPlugins;
    mPlugins = temp;
  }

  CleanUnloadedLibraries();
}

NS_IMPL_ISUPPORTS6(nsPluginHostImpl,
                   nsIPluginManager,
                   nsIPluginManager2,
                   nsIPluginHost,
                   nsIFileUtilities,
                   nsICookieStorage,
                   nsIObserver);

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

NS_IMETHODIMP nsPluginHostImpl::GetValue(nsPluginManagerVariable aVariable, void *aValue)
{
  nsresult rv = NS_OK;

  NS_ENSURE_ARG_POINTER(aValue);

#ifdef XP_UNIX
  if (nsPluginManagerVariable_XDisplay == aVariable) {
    Display** value = NS_REINTERPRET_CAST(Display**, aValue);
#if defined(MOZ_WIDGET_GTK)
    *value = GDK_DISPLAY();
#elif defined(MOZ_WIDGET_QT)
    *value = qt_xdisplay();
#endif
    if (!(*value)) {
      return NS_ERROR_FAILURE;
    }
  }
#endif
  return rv;
}

PRBool nsPluginHostImpl::IsRunningPlugin(nsPluginTag * plugin)
{
  if(!plugin)
    return PR_FALSE;

  // we can check for mLibrary to be non-zero and then querry nsIPluginInstancePeer
  // in nsActivePluginList to see if plugin with matching mime type is not stopped
  if(!plugin->mLibrary)
    return PR_FALSE;

  for(int i = 0; i < plugin->mVariants; i++)
  {
    nsActivePlugin * p = mActivePluginList.find(plugin->mMimeTypeArray[i]);
    if(p && !p->mStopped)
      return PR_TRUE;
  }

  return PR_FALSE;
}

// this will unload loaded but no longer needed libs which are
// gathered in mUnloadedLibraries list, see bug #61388
void nsPluginHostImpl::CleanUnloadedLibraries()
{
  if(!mUnloadedLibraries)
    return;

  while (nsnull != mUnloadedLibraries)
  {
    nsUnloadedLibrary *temp = mUnloadedLibraries->mNext;
    delete mUnloadedLibraries;
    mUnloadedLibraries = temp;
  }
}

nsresult nsPluginHostImpl::ReloadPlugins(PRBool reloadPages)
{
  // XXX don't we want to nuke the old mPlugins right now?
      // we should. Otherwise LoadPlugins will add the same plugins to the list
  // XXX for new-style plugins, we should also call nsIComponentManager::AutoRegister()

  // we are re-scanning plugins. New plugins may have been added, also some
  // plugins may have been removed, so we should probably shut everything down
  // but don't touch running (active and  not stopped) plugins

  if(reloadPages)
  {
    // if we have currently running plugins we should set a flag not to
    // unload them from memory, see bug #61388
    // and form a list of libs to be unloaded later
    for(nsPluginTag * p = mPlugins; p != nsnull; p = p->mNext)
    {
      if(IsRunningPlugin(p))
      {
        p->mCanUnloadLibrary = PR_FALSE;
        nsUnloadedLibrary * unloadedLibrary = new nsUnloadedLibrary(p->mLibrary);
        if(unloadedLibrary)
        {
          unloadedLibrary->mNext = mUnloadedLibraries;
          mUnloadedLibraries = unloadedLibrary;
        }
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

  for(nsPluginTag * p = mPlugins; p != nsnull;)
  {
    next = p->mNext;

    if(!IsRunningPlugin(p))
    {
      if(p == mPlugins)
        mPlugins = next;
      else
        prev->mNext = next;

      if(p->mEntryPoint)
        p->mEntryPoint->Shutdown();

      delete p;
      p = next;
      continue;
    }

    prev = p;
    p = next;
  }

  // set flags
  mPluginsLoaded = PR_FALSE;

  // load them again
  nsresult rv = LoadPlugins();

  return rv;
}

#define NS_RETURN_UASTRING_SIZE 128

nsresult nsPluginHostImpl::UserAgent(const char **retstring)
{
  static char resultString[NS_RETURN_UASTRING_SIZE];
  nsresult res;

  nsCOMPtr<nsIHTTPProtocolHandler> http = do_GetService(kHTTPHandlerCID, &res);
  if (NS_FAILED(res)) 
    return res;

  PRUnichar *UAString = nsnull;
  res = http->GetUserAgent(&UAString);

  if (NS_SUCCEEDED(res)) 
  {
    nsAutoString ua(UAString);
    char * newString = ua.ToNewCString();
    if (!newString) 
    {
      *retstring = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    
    if(NS_RETURN_UASTRING_SIZE > PL_strlen(newString))
    {
      PL_strcpy(resultString, newString);
      *retstring = resultString;
    }
    else
    {
      *retstring = nsnull;
      res = NS_ERROR_OUT_OF_MEMORY;
    }

    nsCRT::free(newString);
  } 
  else
    *retstring = nsnull;

  return res;
}

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
      rv = NewPluginURLStream(string, instance, streamListener,
                              nsnull, nsnull, getHeaders, getHeadersLength);

    NS_RELEASE(instance);
  }

  return rv;
}

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
      nsPluginInstancePeerImpl *peer;

      if (nsnull != target)
        {
          
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
                  rv = owner->GetURL(url, target, (void*)postData, postDataLen,
                                     (void*) postHeaders, postHeadersLength);
                }
              
              NS_RELEASE(peer);
            }
        }
    
      // if we don't have a target, just create a stream.  This does
      // NS_OpenURI()!
      if (streamListener != nsnull)
        rv = NewPluginURLStream(string, instance, streamListener,
                                (void*)postData, postDataLen,
                                postHeaders, postHeadersLength);
      
      NS_RELEASE(instance);
  }
  
  return rv;
}

NS_IMETHODIMP nsPluginHostImpl::RegisterPlugin(REFNSIID aCID,
                                               const char* aPluginName,
                                               const char* aDescription,
                                               const char** aMimeTypes,
                                               const char** aMimeDescriptions,
                                               const char** aFileExtensions,
                                               PRInt32 aCount)
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

  nsRegistryKey pluginKey;
  rv = registry->AddSubtree(nsIRegistry::Common, path, &pluginKey);
  if (NS_FAILED(rv)) return rv;

  registry->SetStringUTF8(pluginKey, "name", aPluginName);
  registry->SetStringUTF8(pluginKey, "description", aDescription);

  for (PRInt32 i = 0; i < aCount; ++i) {
    nsCAutoString mimepath;
    mimepath.AppendInt(i);

    nsRegistryKey key;
    registry->AddSubtree(pluginKey, mimepath, &key);

    registry->SetStringUTF8(key, "mimetype",    aMimeTypes[i]);
    registry->SetStringUTF8(key, "description", aMimeDescriptions[i]);
    registry->SetStringUTF8(key, "extension",   aFileExtensions[i]);
  }

  return NS_OK;
}

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

  return registry->RemoveSubtree(nsIRegistry::Common, path);
}

NS_IMETHODIMP nsPluginHostImpl::BeginWaitCursor(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginHostImpl::EndWaitCursor(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginHostImpl::SupportsURLProtocol(const char* protocol, PRBool *result)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginHostImpl::NotifyStatusChange(nsIPlugin* plugin, nsresult errorStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
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
  const PRInt32 bufLen = 80;
  char buf[bufLen];

  nsCOMPtr<nsIURI> uriIn;
  nsCOMPtr<nsIProxyAutoConfig> pacService;
  nsXPIDLCString protocol; 
  nsXPIDLCString type;
  nsXPIDLCString proxyHost;
  nsXPIDLCString noProxyList;
  PRInt32 proxyPort = -1;
  PRBool useDirect = PR_FALSE;
  PRBool usePrefs = PR_TRUE;
  PRInt32 proxyType = -1;
    
  NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &res);

  if (NS_FAILED(res) || !prefs) {
    return res;
  }

  NS_WITH_SERVICE(nsIIOService, theService, kIOServiceCID, &res);

  if (NS_FAILED(res) || !theService) {
    return res;
  }

  // make an nsURI from the argument url
  res = theService->NewURI(url, nsnull, getter_AddRefs(uriIn));
  if (NS_FAILED(res)) {
    return res;
  }

  // FIRST, see if the ProxyAutoConfig service is activated and enabled
  pacService = do_GetService("@mozilla.org/network/proxy_autoconfig;1");
  if (pacService) {

    // ASSUMPTION: the only time where it's appropriate to NOT look at
    // the prefs is a successful return from PAC.

    if (NS_SUCCEEDED(res = pacService->ProxyForURL(uriIn, 
                                                   getter_Copies(proxyHost), 
                                                   &proxyPort, 
                                                   getter_Copies(type)))) {
      if (PL_strstr(type, "direct")) {
        usePrefs = PR_FALSE;
        useDirect = PR_TRUE;
      }
      else {
        if (proxyHost) {
          usePrefs = PR_FALSE;
          *result = PR_smprintf("PROXY %s:%d", (const char *) proxyHost, 
                                proxyPort);
          if (nsnull == *result) {
            res = NS_ERROR_OUT_OF_MEMORY;
            return res;
          }
        }
      }
    }
  }
  if (usePrefs) {
    // first, see if the prefs say, use "direct connection to the
    // internet".
    // see bug http://bugzilla.mozilla.org/show_bug.cgi?id=48336
    res = prefs->GetIntPref("network.proxy.type", &proxyType);
    if (NS_FAILED(res) || 0 == proxyType) {
      useDirect = PR_TRUE;
    }
  }
  if (usePrefs && !useDirect) {
    // If ProxyAutoConfig is not enabled, look to the prefs.
    
    // If the host for this url is in the "no proxy for" list, just go
    // direct.
    res = prefs->CopyCharPref("network.proxy.no_proxies_on", 
                              getter_Copies(noProxyList));
    if (NS_SUCCEEDED(res) && nsCRT::strlen((const char *) noProxyList) > 0) {
      res = uriIn->GetHost(getter_Copies(proxyHost));
      if (NS_SUCCEEDED(res) && nsCRT::strlen(proxyHost) > 0) {
        // tokenize the noProxyList, and scan each token for
        // the proxyHost.
        // if only one host in noProxyList
        const char *comma = ",";
        char *newStr;
        char *token;
        char *noProxyCopy = nsXPIDLCString::Copy(noProxyList);
        if (!noProxyCopy) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
        
        token = nsCRT::strtok( noProxyCopy, comma, &newStr );   
        while( token != NULL ) {
          if (PL_strstr((const char *) token, (const char *) proxyHost)) {
            useDirect = PR_TRUE;
            break;
          }
          token = nsCRT::strtok( newStr, comma, &newStr );
        }
        nsCRT::free(noProxyCopy);
        
      }
    }
    if (!useDirect) {
      // get the scheme from this nsURI
      res = uriIn->GetScheme(getter_Copies(protocol));
      if (NS_FAILED(res)) {
        return res;
      }
      
      PR_snprintf(buf, bufLen, "network.proxy.%s", (const char *) protocol);
      res = prefs->CopyCharPref(buf, getter_Copies(proxyHost));
      if (NS_SUCCEEDED(res) && nsCRT::strlen((const char *) proxyHost) > 0) {
        PR_snprintf(buf, bufLen, "network.proxy.%s_port", 
                    (const char *) protocol);
        res = prefs->GetIntPref(buf, &proxyPort);
        
        if (NS_SUCCEEDED(res) && (proxyPort>0)) { // currently a bug in IntPref
          // construct the return value
          *result = PR_smprintf("PROXY %s:%d", (const char *) proxyHost, 
                                proxyPort);
          if (nsnull == *result) {
            res = NS_ERROR_OUT_OF_MEMORY;
            return res;
          }
        }
        else { 
          useDirect = PR_TRUE;
        }
      }
      else {
        useDirect = PR_TRUE;
      }
    }
  }
  
  if (useDirect) {
    // construct the return value
    *result = PR_smprintf("DIRECT");
    if (nsnull == *result) {
      res = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return res;
}

NS_IMETHODIMP nsPluginHostImpl::RegisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginHostImpl::UnregisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginHostImpl::AllocateMenuID(nsIEventHandler* handler, PRBool isSubmenu, PRInt16 *result)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginHostImpl::DeallocateMenuID(nsIEventHandler* handler, PRInt16 menuID)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginHostImpl::HasAllocatedMenuID(nsIEventHandler* handler, PRInt16 menuID, PRBool *result)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginHostImpl::ProcessNextEvent(PRBool *bEventHandled)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginHostImpl::CreateInstance(nsISupports *aOuter,
                                               REFNSIID aIID,
                                               void **aResult)
{
  NS_NOTREACHED("how'd I get here?");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsPluginHostImpl::LockFactory(PRBool aLock)
{
  NS_NOTREACHED("how'd I get here?");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsPluginHostImpl::Init(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsPluginHostImpl::Destroy(void)
{
  if (mIsDestroyed)
    return NS_OK;

  mIsDestroyed = PR_TRUE;

  // at this point nsIPlugin::Shutdown calls will be performed if needed
  mActivePluginList.shut();

  return NS_OK;
}

/* Called by nsPluginInstanceOwner (nsObjectFrame.cpp - embeded case) */
NS_IMETHODIMP nsPluginHostImpl::InstantiateEmbededPlugin(const char *aMimeType, 
                                                         nsIURI* aURL,
                                                         nsIPluginInstanceOwner *aOwner)
{
  nsresult  rv;
  nsIPluginInstance *instance = nsnull;
  nsCOMPtr<nsIPluginTagInfo2> pti2 = nsnull;
  nsPluginTagType tagType;
  PRBool isJavaEnabled = PR_TRUE;
  
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

  if (tagType == nsPluginTagType_Applet) {
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

#ifdef NS_DEBUG
  if(aMimeType)
    printf("InstantiateEmbededPlugin for %s\n",aMimeType);
#endif

  if(FindStoppedPluginForURL(aURL, aOwner) == NS_OK)
  {
#ifdef NS_DEBUG
      printf("InstantiateEmbededPlugin find stopped\n");
#endif

	  aOwner->GetInstance(instance);
    if(!aMimeType || PL_strcasecmp(aMimeType, "application/x-java-vm"))
	    rv = NewEmbededPluginStream(aURL, nsnull, instance);

    // notify Java DOM component 
    nsresult res;
    NS_WITH_SERVICE(nsIPluginInstanceOwner, javaDOM, "@mozilla.org/blackwood/java-dom;1", &res);
    if (NS_SUCCEEDED(res) && javaDOM)
      javaDOM->SetInstance(instance);

    NS_IF_RELEASE(instance);
    return NS_OK;
  }

  // if we don't have a MIME type at this point, we still have one more chance by 
  // opening the stream and seeing if the server hands one back 
  if (!aMimeType)
    if (aURL)
    {
       rv = NewEmbededPluginStream(aURL, aOwner, nsnull);
       return rv;
    } else
       return NS_ERROR_FAILURE;

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

      // don't make an initial steam if it's a java applet
      if(!aMimeType || 
         (PL_strcasecmp(aMimeType, "application/x-java-vm") != 0 && 
          PL_strcasecmp(aMimeType, "application/x-java-applet") != 0))
        rv = NewEmbededPluginStream(aURL, nsnull, instance);

      // notify Java DOM component 
      nsresult res;
      NS_WITH_SERVICE(nsIPluginInstanceOwner, javaDOM, "@mozilla.org/blackwood/java-dom;1", &res);
      if (NS_SUCCEEDED(res) && javaDOM)
        javaDOM->SetInstance(instance);

      NS_RELEASE(instance);
    }

#ifdef NS_DEBUG
  printf("InstantiateEmbededPlugin.. returning\n");
#endif
  return rv;
}

/* Called by nsPluginViewer.cpp (full-page case) */

NS_IMETHODIMP nsPluginHostImpl::InstantiateFullPagePlugin(const char *aMimeType, 
                                                          nsString& aURLSpec,
                                                          nsIStreamListener *&aStreamListener,
                                                          nsIPluginInstanceOwner *aOwner)
{
  nsresult  rv;
  nsIURI    *url;
  PRBool isJavaEnabled = PR_TRUE;

#ifdef NS_DEBUG
  printf("InstantiateFullPagePlugin for %s\n",aMimeType);
#endif
  
  //create a URL so that the instantiator can do file ext.
  //based plugin lookups...
  rv = NS_NewURI(&url, aURLSpec);

  if (rv != NS_OK)
    url = nsnull;

  if(FindStoppedPluginForURL(url, aOwner) == NS_OK)
  {
#ifdef NS_DEBUG
      printf("InstantiateFullPagePlugin, got a stopped plugin\n");
#endif

    nsIPluginInstance* instance;
	  aOwner->GetInstance(instance);
    if(!aMimeType || PL_strcasecmp(aMimeType, "application/x-java-vm"))
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

#ifdef NS_DEBUG
    printf("InstantiateFullPagePlugin, got it... now stream\n");
#endif
    //we got a plugin built, now stream

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

      NS_RELEASE(instance);
    }
  }

#ifdef NS_DEBUG
  printf("Falling out of InstantiateFullPagePlugin...\n");
#endif
  return rv;
}

nsresult nsPluginHostImpl::FindStoppedPluginForURL(nsIURI* aURL, 
                                                   nsIPluginInstanceOwner *aOwner)
{
  char* url;
  if(!aURL)
  	return NS_ERROR_FAILURE;
  	
#ifdef NS_DEBUG
  printf("Inside nsPluginHostImpl::FindStoppedPluginForURL...\n");
#endif

  (void)aURL->GetSpec(&url);
  
  nsActivePlugin * plugin = mActivePluginList.findStopped(url);

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
    nsCRT::free(url);
    return NS_OK;
  }
  nsCRT::free(url);
  return NS_ERROR_FAILURE;
}

void nsPluginHostImpl::AddInstanceToActiveList(nsCOMPtr<nsIPlugin> aPlugin,
                                               nsIPluginInstance* aInstance,
                                               nsIURI* aURL,
                                               PRBool aDefaultPlugin)

{
  // first, determine if the plugin wants to be cached
     // Code moved to StopPluginInstance as we still need running
     // plugins to be present in the 'active' list
  
  char* url;

  if(!aURL)
  	return;
  	
  (void)aURL->GetSpec(&url);

  nsActivePlugin * plugin = new nsActivePlugin(aPlugin, aInstance, url, aDefaultPlugin);

  if(plugin == nsnull)
    return;

  mActivePluginList.add(plugin);

  nsCRT::free(url);
}

nsresult nsPluginHostImpl::RegisterPluginMimeTypesWithLayout(nsPluginTag * pluginTag, 
                                                             nsIComponentManager * compManager, 
                                                             nsIFile * path)
{
  NS_ENSURE_ARG_POINTER(pluginTag);
  NS_ENSURE_ARG_POINTER(pluginTag->mMimeTypeArray);
  NS_ENSURE_ARG_POINTER(compManager);

  nsresult rv = NS_OK;

  for(int i = 0; i < pluginTag->mVariants; i++)
  {
    static NS_DEFINE_CID(kPluginDocLoaderFactoryCID, NS_PLUGINDOCLOADERFACTORY_CID);

    nsCAutoString contractid(NS_DOCUMENT_LOADER_FACTORY_CONTRACTID_PREFIX "view;1?type=");
    contractid += pluginTag->mMimeTypeArray[i];

    rv = compManager->RegisterComponentSpec(kPluginDocLoaderFactoryCID,
                                            "Plugin Loader Stub",
                                            contractid,
                                            path,
                                            PR_TRUE,
                                            PR_FALSE);
  }

  return rv;
}

NS_IMETHODIMP nsPluginHostImpl::SetUpPluginInstance(const char *aMimeType, 
                                                    nsIURI *aURL,
                                                    nsIPluginInstanceOwner *aOwner)
{
  nsresult result = NS_ERROR_FAILURE;
  nsIPluginInstance* instance = NULL;
  nsCOMPtr<nsIPlugin> plugin;
  const char* mimetype;
  nsString strContractID; strContractID.AssignWithConversion (NS_INLINE_PLUGIN_CONTRACTID_PREFIX);
  char buf[255];  // todo: need to use a const
		
  if(!aURL)
    return NS_ERROR_FAILURE;

	// if don't have a mimetype, check by file extension
  if(!aMimeType)
  {
    char* extension;

    char* filename;
    aURL->GetPath(&filename);
    extension = PL_strrchr(filename, '.');
    if(extension)
      ++extension;
    else
      return NS_ERROR_FAILURE;

    if(IsPluginEnabledForExtension(extension, mimetype) != NS_OK)
    {
      nsCRT::free(filename);
      return NS_ERROR_FAILURE;
    }
    nsCRT::free(filename);
	}
  else
    mimetype = aMimeType;

    strContractID.AppendWithConversion(mimetype);
    strContractID.ToCString(buf, 255);     // todo: need to use a const

    GetPluginFactory(mimetype, getter_AddRefs(plugin));

    result = nsComponentManager::CreateInstance(buf,
                                                nsnull,
                                                nsIPluginInstance::GetIID(),
                                                (void**)&instance);


    // couldn't create an XPCOM plugin, try to create wrapper for a legacy plugin
    if (NS_FAILED(result)) 
    {
      if(plugin)
        result = plugin->CreateInstance(NULL, kIPluginInstanceIID, (void **)&instance);

      if (NS_FAILED(result)) 
      {
        NS_WITH_SERVICE(nsIPlugin, bwPlugin, "@mozilla.org/blackwood/pluglet-engine;1",&result);
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

    return NS_OK;
}

nsresult nsPluginHostImpl::SetUpDefaultPluginInstance(const char *aMimeType, nsIURI *aURL,
                                                      nsIPluginInstanceOwner *aOwner)
{
  nsresult result = NS_ERROR_FAILURE;
  nsIPluginInstance* instance = NULL;
  nsCOMPtr<nsIPlugin> plugin = NULL;
  const char* mimetype;
  nsString strContractID; strContractID.AssignWithConversion (NS_INLINE_PLUGIN_CONTRACTID_PREFIX);
  char buf[255];  // todo: need to use a const
		
  if(!aURL)
    return NS_ERROR_FAILURE;

  mimetype = aMimeType;

  strContractID.AppendWithConversion("*");
  strContractID.ToCString(buf, 255);     // todo: need to use a const
  
  GetPluginFactory("*", getter_AddRefs(plugin));

  result = nsComponentManager::CreateInstance(buf, nsnull, nsIPluginInstance::GetIID(), (void**)&instance);

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
      nsXPIDLCString extension;
      url->GetFileExtension(getter_Copies(extension));
    
      if(extension)
      {
        nsCOMPtr<nsIMIMEService> ms (do_GetService(NS_MIMESERVICE_CONTRACTID, &res));
        if(NS_SUCCEEDED(res) && ms)
        {
          res = ms->GetTypeFromExtension(extension, getter_Copies(mt));
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
        if (plugins->mMimeTypeArray[cnt] && (0 == strcmp(plugins->mMimeTypeArray[cnt], aMimeType)))
          return NS_OK;

      if (cnt < variants)
        break;

      plugins = plugins->mNext;
    }
  }

  return NS_ERROR_FAILURE;
}

// check comma delimetered extensions
static int CompareExtensions(const char *aExtensionList, const char *aExtension)
{
  if((aExtensionList == nsnull) || (aExtension == nsnull))
    return -1;

  const char *pExt = aExtensionList;
  const char *pComma = strchr(pExt, ',');

  if(pComma == nsnull)
    return strcmp(pExt, aExtension);

  while(pComma != nsnull)
  {
    int length = pComma - pExt;
    if(0 == strncmp(pExt, aExtension, length))
      return 0;

    pComma++;
    pExt = pComma;
    pComma = strchr(pExt, ',');
  }

  // the last one
  return strcmp(pExt, aExtension);
}

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

class DOMMimeTypeImpl : public nsIDOMMimeType {
public:
	NS_DECL_ISUPPORTS

	DOMMimeTypeImpl(nsPluginTag* aPluginTag, PRUint32 aMimeTypeIndex)
	{
		NS_INIT_ISUPPORTS();
		mDescription.AssignWithConversion(aPluginTag->mMimeDescriptionArray[aMimeTypeIndex]);
		mSuffixes.AssignWithConversion(aPluginTag->mExtensionsArray[aMimeTypeIndex]);
		mType.AssignWithConversion(aPluginTag->mMimeTypeArray[aMimeTypeIndex]);
	}
	
	virtual ~DOMMimeTypeImpl() {
	}

	NS_METHOD GetDescription(nsAWritableString& aDescription)
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

	NS_METHOD GetSuffixes(nsAWritableString& aSuffixes)
	{
		aSuffixes.Assign(mSuffixes);
		return NS_OK;
	}

	NS_METHOD GetType(nsAWritableString& aType)
	{
		aType.Assign(mType);
		return NS_OK;
	}

private:
	nsString mDescription;
	nsString mSuffixes;
	nsString mType;
};

NS_IMPL_ISUPPORTS(DOMMimeTypeImpl, nsIDOMMimeType::GetIID());

class DOMPluginImpl : public nsIDOMPlugin {
public:
	NS_DECL_ISUPPORTS
	
	DOMPluginImpl(nsPluginTag* aPluginTag) : mPluginTag(aPluginTag)
	{
		NS_INIT_ISUPPORTS();
	}
	
	virtual ~DOMPluginImpl() {
	}

	NS_METHOD GetDescription(nsAWritableString& aDescription)
	{
		aDescription.Assign(NS_ConvertASCIItoUCS2(mPluginTag.mDescription));
		return NS_OK;
	}

	NS_METHOD GetFilename(nsAWritableString& aFilename)
	{
		aFilename.Assign(NS_ConvertASCIItoUCS2(mPluginTag.mFileName));
		return NS_OK;
	}

	NS_METHOD GetName(nsAWritableString& aName)
	{
		aName.Assign(NS_ConvertASCIItoUCS2(mPluginTag.mName));
		return NS_OK;
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

	NS_METHOD NamedItem(const nsAReadableString& aName, nsIDOMMimeType** aReturn)
	{
		for (int index = mPluginTag.mVariants - 1; index >= 0; --index) {
			if (aName.Equals(NS_ConvertASCIItoUCS2(mPluginTag.mMimeTypeArray[index])))
				return Item(index, aReturn);
		}
		return NS_OK;
	}

private:
	nsPluginTag mPluginTag;
};

NS_IMPL_ISUPPORTS(DOMPluginImpl, nsIDOMPlugin::GetIID());

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
        if (plugins->mMimeTypeArray[cnt] && (0 == strcmp(plugins->mMimeTypeArray[cnt], aMimeType))) {
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

NS_IMETHODIMP nsPluginHostImpl::GetPluginFactory(const char *aMimeType, nsIPlugin** aPlugin)
{
	nsresult rv = NS_ERROR_FAILURE;
	*aPlugin = NULL;

	if(!aMimeType)
		return NS_ERROR_ILLEGAL_VALUE;

  // unload any libs that can remain after plugins.refresh(1), see #61388
  CleanUnloadedLibraries();

	// If plugins haven't been scanned yet, do so now
  LoadPlugins();

	nsPluginTag* pluginTag;
	if((rv = FindPluginEnabledForType(aMimeType, pluginTag)) == NS_OK)
	{
#ifdef NS_DEBUG
    if(aMimeType && pluginTag->mFileName)
      printf("For %s found plugin %s\n", aMimeType, pluginTag->mFileName);
#endif

		if (nsnull == pluginTag->mLibrary)		// if we haven't done this yet
		{
			nsFileSpec file(pluginTag->mFileName);

			nsPluginFile pluginFile(file);
			PRLibrary* pluginLibrary = NULL;

			if (pluginFile.LoadPlugin(pluginLibrary) != NS_OK || pluginLibrary == NULL)
				return NS_ERROR_FAILURE;

			pluginTag->mLibrary = pluginLibrary;

		}

		nsIPlugin* plugin = pluginTag->mEntryPoint;
		if(plugin == NULL)
		{
            // No, this is not a leak. GetGlobalServiceManager() doesn't
            // addref the pointer on the way out. It probably should.
            nsIServiceManager* serviceManager;
            nsServiceManager::GetGlobalServiceManager(&serviceManager);

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
#if defined(XP_MAC) && TARGET_CARBON
                // should we also look for a 'carb' resource?
                if (PR_FindSymbol(pluginTag->mLibrary, "mainRD") != NULL) {
                    NS_WITH_SERVICE(nsIClassicPluginFactory, factory, NS_CLASSIC_PLUGIN_FACTORY_CONTRACTID, &rv);
                    if (NS_SUCCEEDED(rv)) rv = factory->CreatePlugin(serviceManager, pluginTag->mFileName,
                                                                     pluginTag->mLibrary, &pluginTag->mEntryPoint);
                } else
#endif
				rv = ns4xPlugin::CreatePlugin(serviceManager,
                                      pluginTag->mFileName,
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

	return rv;
}

static PRBool areTheSameFileNames(char * name1, char * name2)
{
  if((name1 == nsnull) || (name2 == nsnull))
    return PR_FALSE;

  char * filename1 = PL_strrchr(name1, '\\');
	if(filename1 != nsnull)
		filename1++;
	else
		filename1 = name1;

  char * filename2 = PL_strrchr(name2, '\\');
	if(filename2 != nsnull)
		filename2++;
	else
		filename2 = name2;

  if(PL_strlen(filename1) != PL_strlen(filename2))
    return PR_FALSE;

  // this one MUST be case insensitive for Windows and MUST be case sensitive
  // for Unix. How about Win2000?
  return (nsnull == PL_strncasecmp(filename1, filename2, PL_strlen(filename1)));
}

static PRBool isJavaPlugin(nsPluginTag * tag)
{
  if(tag->mFileName == nsnull)
    return PR_FALSE;

  char * filename = PL_strrchr(tag->mFileName, '\\');

  if(filename != nsnull)
		filename++;
	else
		filename = tag->mFileName;

  return (nsnull == PL_strncasecmp(filename, "npjava", 6));
}

// currently 'unwanted' plugins are Java, and all other plugins except
// RealPlayer, Acrobat, Flash
static PRBool isUnwantedPlugin(nsPluginTag * tag)
{
  if(tag->mFileName == nsnull)
    return PR_TRUE;

  for (PRInt32 i = 0; i < tag->mVariants; ++i) {
    if(nsnull == PL_strcasecmp(tag->mMimeTypeArray[i], "audio/x-pn-realaudio-plugin"))
      return PR_FALSE;

    if(nsnull == PL_strcasecmp(tag->mMimeTypeArray[i], "application/pdf"))
      return PR_FALSE;

    if(nsnull == PL_strcasecmp(tag->mMimeTypeArray[i], "application/x-shockwave-flash"))
      return PR_FALSE;
  }

  return PR_TRUE;
}

nsresult nsPluginHostImpl::ScanPluginsDirectory(nsPluginsDir& pluginsDir, 
                                                nsIComponentManager * compManager, 
                                                nsIFile * layoutPath,
                                                PRBool checkForUnwantedPlugins)
{
  for (nsDirectoryIterator iter(pluginsDir, PR_TRUE); iter.Exists(); iter++) 
  {
		const nsFileSpec& file = iter;
		if (pluginsDir.IsPluginFile(file)) 
    {
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

      nsPluginTag* pluginTag = new nsPluginTag(&info);
      pluginFile.FreePluginInfo(info);

      if(pluginTag == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

      pluginTag->mLibrary = pluginLibrary;

      PRBool bAddIt = PR_TRUE;

      // check if there are specific plugins we don't want
      if(checkForUnwantedPlugins)
      {
        if(isUnwantedPlugin(pluginTag))
          bAddIt = PR_FALSE;
      }

      // check if we already have this plugin in the list which
      // is possible if we do refresh
      if(bAddIt)
      {
        for(nsPluginTag* tag = mPlugins; tag != nsnull; tag = tag->mNext)
        {
          if(areTheSameFileNames(tag->mFileName, pluginTag->mFileName))
          {
            bAddIt = PR_FALSE;
            break;
          }
        }
      }

      // so if we still want it -- do it
      if(bAddIt)
      {
        pluginTag->mNext = mPlugins;
			  mPlugins = pluginTag;

        if(layoutPath)
          RegisterPluginMimeTypesWithLayout(pluginTag, compManager, layoutPath);
      }
      else
        delete pluginTag;
		}
	}
  return NS_OK;
}

NS_IMETHODIMP nsPluginHostImpl::LoadPlugins()
{
  // do not do anything if it is already done
  // use nsPluginHostImpl::ReloadPlugins to enforce loading
  if(mPluginsLoaded)
    return NS_OK;

  // retrieve a path for layout module. Needed for plugin mime types registration
  nsCOMPtr<nsIFile> path;
  PRBool isLayoutPath = PR_FALSE;
  nsresult rv;
  nsCOMPtr<nsIComponentManager> compManager = do_GetService(kComponentManagerCID, &rv);
  if (NS_SUCCEEDED(rv) && compManager) 
  {
    isLayoutPath = NS_SUCCEEDED(compManager->SpecForRegistryLocation(REL_PLUGIN_DLL, getter_AddRefs(path)));
    rv = LoadXPCOMPlugins(compManager, path);
  }

	// scan the 4x plugins directory for eligible legacy plugin libraries

#ifndef XP_WIN // old plugin finding logic

  // scan Mozilla plugins dir
  nsPluginsDir pluginsDir;

  if (pluginsDir.Valid())
  {
    nsCOMPtr<nsIFile> lpath = nsnull;
    if(isLayoutPath)
      lpath = path;

    ScanPluginsDirectory(pluginsDir, compManager, lpath);
  }

#ifdef XP_MAC
  // try to scan old-spelled plugins dir ("Plug-ins") for Mac
  // should we check for duplicate plugins here? We probably should.
  nsPluginsDir pluginsDirMacOld(PLUGINS_DIR_LOCATION_MAC_OLD);
	
  if (pluginsDirMacOld.Valid())
  {
    nsCOMPtr<nsIFile> lpath = nsnull;
    if(isLayoutPath)
      lpath = path;

    ScanPluginsDirectory(pluginsDirMacOld, 
                         compManager, 
                         lpath, 
                         PR_FALSE); // don't check for specific plugins
  }
#endif // XP_MAC

#else //  XP_WIN go for new plugin finding logic on Windows

  // currently we decided to look in both local plugins dir and 
  // that of the previous 4.x installation combining plugins from both places.
  // See bug #21938
  // As of 1.27.00 this selective mechanism is natively supported in Windows 
  // native implementation of nsPluginsDir.

  // first, make a list from MOZ_LOCAL installation
  nsPluginsDir pluginsDirMoz(PLUGINS_DIR_LOCATION_MOZ_LOCAL);

  if (pluginsDirMoz.Valid())
  {
    nsCOMPtr<nsIFile> lpath = nsnull;
    if(isLayoutPath)
      lpath = path;

    ScanPluginsDirectory(pluginsDirMoz, compManager, lpath);
  }

  // now check the 4.x plugins dir and add new files
  // Specifying the last two params as PR_TRUE we make sure that:
  //   1. we search for a match in the list of MOZ_LOCAL plugins, ignore if found, 
  //      add if not found (check for dups)
  //   2. we ignore 4.x Java plugins no matter what and other 
  //      unwanted plugins as per temporary decision described in bug #23856
 	nsPluginsDir pluginsDir4x(PLUGINS_DIR_LOCATION_4DOTX);
	if (pluginsDir4x.Valid())
  {
    nsCOMPtr<nsIFile> lpath = nsnull;
    if(isLayoutPath)
      lpath = path;

    ScanPluginsDirectory(pluginsDir4x, 
                         compManager, 
                         lpath, 
                         PR_TRUE);  // check for specific plugins
  }
#endif

  mPluginsLoaded = PR_TRUE; // at this point 'some' plugins have been loaded,
                            // the rest is optional
 
#ifdef XP_WIN
  // Check the windows registry for extra paths to scan for plugins
  //
  // We are going to get this registry key location from the pref:
  //    browser.plugins.registry_plugins_folder_key_location
  // The key name is "Plugins Folders"
  //
  // So, for example, in winprefs.js put in this line:
  // pref ("browser.plugins.registry_plugins_folder_key_location","Software\\Mozilla\\Common");
  // Then, in HKEY_LOCAL_MACHINE\Software\Mozilla\Common
  // Make a string key "Plugins Folder" who's value is a list of paths sperated by semi-colons

  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  if (!prefs) return NS_OK;     // if we can't get to the prefs, bail
  
  char * regkey;
  rv = prefs->CopyCharPref(_NS_PREF_COMMON_PLUGIN_REG_KEY_,&regkey);
  if (!NS_SUCCEEDED(rv) || regkey == nsnull) return NS_OK; //if pref isn't set, bail
  
  unsigned char valbuf[_MAXKEYVALUE_];
  char* pluginPath;
  HKEY  newkey;
  LONG  result;
  DWORD type   = REG_SZ;
  DWORD length = _MAXKEYVALUE_;
 
  // set up layout path (if not done above)
  nsCOMPtr<nsIFile> lpath = nsnull;
  if(isLayoutPath)
    lpath = path;

  result = RegOpenKeyEx( HKEY_LOCAL_MACHINE, regkey, NULL, KEY_QUERY_VALUE, &newkey );
  
  if ( ERROR_SUCCESS == result ) {
      result = RegQueryValueEx( newkey, _NS_COMMON_PLUGIN_KEY_NAME_, nsnull, &type, valbuf, &length );
      RegCloseKey( newkey );
      if ( ERROR_SUCCESS == result ) {
          // tokenize reg key value by semi-colons
        for ( pluginPath = strtok((char *)&valbuf, ";"); pluginPath; pluginPath = strtok(NULL, ";") ) {
          nsFileSpec winRegPluginPath (pluginPath);
          if (winRegPluginPath.Exists()) {     // check for validity of path first
#ifdef DEBUG
printf("found some more plugins at: %s\n", pluginPath);
#endif
              ScanPluginsDirectory( (nsPluginsDir)winRegPluginPath,
                                                  compManager,
                                                  lpath,
                                                  PR_FALSE);  // check for even unwanted plugins                                                  
            } 
         }  
      }      
  }
  free (regkey);  // clean up

#endif // !XP_WIN

  return NS_OK;
}

static nsresult
LoadXPCOMPlugin(nsIComponentManager* aComponentManager,
                nsIRegistry* aRegistry,
                const char* aCID,
                nsRegistryKey aPluginKey,
                nsPluginTag** aResult)
{
  nsresult rv;

  // The name, description, MIME types, MIME descriptions, and
  // supported file extensions will all hang off of the plugin's key
  // in the registry. Pull these out now.
  nsXPIDLCString name;
  aRegistry->GetStringUTF8(aPluginKey, "name", getter_Copies(name));

  nsXPIDLCString description;
  aRegistry->GetStringUTF8(aPluginKey, "description", getter_Copies(description));

  nsXPIDLCString filename;

  // To figure out the filename of the plugin, we'll need to get the
  // plugin's CID, and then navigate through the XPCOM registry to
  // pull out the DLL name to which the CID is registered.
  nsAutoString path(NS_LITERAL_STRING("software/mozilla/XPCOM/classID/") + NS_ConvertASCIItoUCS2(aCID));

  nsRegistryKey cidKey;
  rv = aRegistry->GetKey(nsIRegistry::Common, path.GetUnicode(), &cidKey);

  if (NS_SUCCEEDED(rv)) {
    PRUint8* library;
    PRUint32 count;
    // XXX Good grief, what does "GetBytesUTF8()" mean? They're bytes!
    rv = aRegistry->GetBytesUTF8(cidKey, "InprocServer", &count, &library);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIFile> file;
      rv = aComponentManager->SpecForRegistryLocation(NS_REINTERPRET_CAST(const char*, library),
                                                      getter_AddRefs(file));

      if (NS_SUCCEEDED(rv)) {
        file->GetPath(getter_Copies(filename));
      }

      nsCRT::free(NS_REINTERPRET_CAST(char*, library));
    }
  }

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

    aRegistry->GetStringUTF8(key, "mimetype",    &mimetypes[count]);
    aRegistry->GetStringUTF8(key, "description", &mimedescriptions[count]);
    aRegistry->GetStringUTF8(key, "extension",   &extensions[count]);
    ++count;
  }

  if (NS_SUCCEEDED(rv)) {
    // All done! Create the new nsPluginTag info and send it back.
    nsPluginTag* tag
      = new nsPluginTag(name.get(),
                        description.get(),
                        filename.get(),
                        (const char* const*)mimetypes,
                        (const char* const*)mimedescriptions,
                        (const char* const*)extensions,
                        count);

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

  return rv;
}

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
  rv = registry->GetSubtree(nsIRegistry::Common, "software/plugins", &pluginsKey);
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

    nsPluginTag* tag = nsnull;
    rv = LoadXPCOMPlugin(aComponentManager, registry, cid, key, &tag);
    if (NS_FAILED(rv))
      continue;

    // skip it if we already have it
    PRBool bAddIt = PR_TRUE;
    for(nsPluginTag* existingtag = mPlugins; existingtag != nsnull; existingtag = existingtag->mNext)
    {
      if(areTheSameFileNames(tag->mFileName, existingtag->mFileName))
      {
        bAddIt = PR_FALSE;
        break;
      }
    }

    if(!bAddIt)
    {
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

/* Called by GetURL and PostURL */

NS_IMETHODIMP nsPluginHostImpl::NewPluginURLStream(const nsString& aURL,
                                                   nsIPluginInstance *aInstance,
                                                   nsIPluginStreamListener* aListener,
                                                   void *aPostData, 
                                                   PRUint32 aPostDataLen, 
                                                   const char *aHeadersData, 
                                                   PRUint32 aHeadersDataLen)
{
  nsCOMPtr<nsIURI> url;
  nsAutoString  absUrl;
  nsresult rv;
  void *newPostData = nsnull;
  PRUint32 newPostDataLen = 0;

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
        nsCOMPtr<nsIURI> docURL( getter_AddRefs(doc->GetDocumentURL()) );
 
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

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIInterfaceRequestor> callbacks;

      if (doc) {
        // Get the script global object owner and use that as the notification callback
        nsCOMPtr<nsIScriptGlobalObject> global;
        doc->GetScriptGlobalObject(getter_AddRefs(global));

        if (global) {
          nsCOMPtr<nsIScriptGlobalObjectOwner> owner;
          global->GetGlobalObjectOwner(getter_AddRefs(owner));

          callbacks = do_QueryInterface(owner);
        }
      }

      nsCOMPtr<nsIChannel> channel;

      // XXX: Null LoadGroup?
      rv = NS_OpenURI(getter_AddRefs(channel), url, nsnull,
                      nsnull, callbacks);
      if (NS_FAILED(rv)) return rv;

      if (doc) {
        // Set the owner of channel to the document principal...
        nsCOMPtr<nsIPrincipal> principal;
        doc->GetPrincipal(getter_AddRefs(principal));

        channel->SetOwner(principal);
      }

      // deal with headers and post data
      nsCOMPtr<nsIHTTPChannel> httpChannel(do_QueryInterface(channel));
      if(httpChannel)
        {

          // figure out if we need to set the post data stream on the
          // channel...  right now, this is only done for http
          // channels.....
          if(aPostData)
            {
              //Make sure there is "r\n\r\n" before the post data
              if (!PL_strstr((const char *) aPostData, "\r\n\r\n")) {
                if (NS_SUCCEEDED(FixPostData(aPostData, aPostDataLen,
                                             &newPostData, &newPostDataLen))) {
                  aPostData = newPostData;
                  aPostDataLen = newPostDataLen;
                }   
              }
              nsCOMPtr<nsIInputStream> postDataStream = nsnull;
              if (aPostData) {
                NS_NewPostDataStream(getter_AddRefs(postDataStream),
                                     PR_FALSE,
                                     (const char *) aPostData, 0);
              }
              
              // XXX it's a bit of a hack to rewind the postdata stream
              // here but it has to be done in case the post data is
              // being reused multiple times.
              
              nsCOMPtr<nsIRandomAccessStore> 
                postDataRandomAccess(do_QueryInterface(postDataStream));
              if (postDataRandomAccess)
                {
                  postDataRandomAccess->Seek(PR_SEEK_SET, 0);
                }
              
              nsCOMPtr<nsIAtom> method(dont_AddRef(NS_NewAtom("POST")));
              httpChannel->SetRequestMethod(method);
              httpChannel->SetUploadStream(postDataStream);

              if (newPostData)
                {
                  delete [] (char *)newPostData;
                  newPostData = nsnull;
                }
            }

          if (aHeadersData) 
            {
              rv = AddHeadersToChannel(aHeadersData, aHeadersDataLen, 
                                       httpChannel);
            }  

        }
       rv = channel->AsyncOpen(listenerPeer, nsnull);
      }
    
    NS_RELEASE(listenerPeer);
  }
  return rv;
}

nsresult
nsPluginHostImpl::FixPostData(void *inPostData, PRUint32 inPostDataLen,
                              void **outPostData, PRUint32 *outPostDataLen)
{
  if ((!inPostData) || (inPostDataLen <= 0) ||
      (!outPostData) || (!outPostDataLen)) {
    return NS_ERROR_NULL_POINTER;
  }

  const char *postData = (const char *)inPostData;
  const char *crlf = nsnull;
  const char *crlfcrlf = "\r\n\r\n";
  const char *t;
  char *newBuf;
  PRInt32 headersLen = 0, dataLen = 0;

  if (!(newBuf = new char[inPostDataLen + 4])) {
    return NS_ERROR_NULL_POINTER;
  }
  nsCRT::memset(newBuf, 0, inPostDataLen + 4);

  if (!(crlf = PL_strstr(postData, "\r\n\n"))) {
    delete [] newBuf;
    return NS_ERROR_NULL_POINTER;
  }
  headersLen = crlf - postData;

  // find the next non-whitespace char
  t = crlf + 3;
  while (*t == '\r' || *t == '\n' || *t == '\t' || *t == ' ' && *t) {
    t++;
  }
  if (*t) {
    // copy the headers
    nsCRT::memcpy(newBuf, postData, headersLen);
    // copy the correct crlfcrlf
    nsCRT::memcpy(newBuf + headersLen, crlfcrlf, 4);
    // copy the rest of the postData
    dataLen = inPostDataLen - (t - postData);
    nsCRT::memcpy(newBuf + headersLen + 4, t, dataLen);
    *outPostDataLen = headersLen + 4 + dataLen;
    *outPostData = newBuf;
  }
  else {
    delete [] newBuf;
    return NS_ERROR_NULL_POINTER;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHostImpl::AddHeadersToChannel(const char *aHeadersData, 
                                      PRUint32 aHeadersDataLen, 
                                      nsIChannel *aGenericChannel)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIHTTPChannel> aChannel = do_QueryInterface(aGenericChannel);
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
  nsIAtom *headerAtom;
  
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
    headerAtom = NS_NewAtom((const char *) headerName);
    if (!headerAtom) {
      rv = NS_ERROR_NULL_POINTER;
      return rv;
    }
    
    //
    // FINALLY: we can set the header!
    // 
    
    rv =aChannel->SetRequestHeader(headerAtom, (const char *) headerValue);
    if (NS_FAILED(rv)) {
      rv = NS_ERROR_NULL_POINTER;
      return rv;
    }
  }    
  return rv;
}

NS_IMETHODIMP
nsPluginHostImpl::StopPluginInstance(nsIPluginInstance* aInstance)
{
  nsActivePlugin * plugin = mActivePluginList.find(aInstance);

  if(plugin != nsnull)
  {
    // if the plugin does not want to be 'cached' just remove it
    PRBool doCache = PR_TRUE;
    aInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
    if (!doCache)
    {
      mActivePluginList.remove(plugin);
    }
    else
    {
      // try to get the max cached plugins from a pref or use default
      PRUint32 max_num;
      nsresult rv;
      nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
      if (prefs) rv = prefs->GetIntPref(NS_PREF_MAX_NUM_CACHED_PLUGINS,(int *)&max_num);
      if (!NS_SUCCEEDED(rv)) max_num = DEFAULT_NUMBER_OF_STOPPED_PLUGINS;

      if(mActivePluginList.getStoppedCount() >= max_num)
      {
        nsActivePlugin * oldest = mActivePluginList.findOldestStopped();
        if(oldest != nsnull)
          mActivePluginList.remove(oldest);
      }
      plugin->setStopped(PR_TRUE);
    }
  }

  return NS_OK;
}

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

  return rv;
}

// nsIFileUtilities interface

NS_IMETHODIMP nsPluginHostImpl::GetProgramPath(const char* *result)
{
	static nsSpecialSystemDirectory programDir(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
	*result = programDir;
	return NS_OK;
}

NS_IMETHODIMP nsPluginHostImpl::GetTempDirPath(const char* *result)
{
	static nsSpecialSystemDirectory tempDir(nsSpecialSystemDirectory::OS_TemporaryDirectory);
	*result = tempDir;
	return NS_OK;
}

NS_IMETHODIMP nsPluginHostImpl::NewTempFileName(const char* prefix, PRUint32 bufLen, char* resultBuf)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

// nsICookieStorage interface

NS_IMETHODIMP nsPluginHostImpl::GetCookie(const char* inCookieURL, void* inOutCookieBuffer, PRUint32& inOutCookieSize)
{
  nsresult rv = NS_ERROR_NOT_IMPLEMENTED;
  nsString cookieString;
  nsCOMPtr<nsIURI> uriIn;
  char *bufPtr;
  
  if ((nsnull == inCookieURL) || (0 >= inOutCookieSize)) {
    return NS_ERROR_INVALID_ARG;
  }

  NS_WITH_SERVICE(nsIIOService, ioService, kIOServiceCID, &rv);
  
  if (NS_FAILED(rv) || (nsnull == ioService)) {
    return rv;
  }

  NS_WITH_SERVICE(nsICookieService, cookieService, kCookieServiceCID, &rv);
  
  if (NS_FAILED(rv) || (nsnull == cookieService)) {
    return NS_ERROR_INVALID_ARG;
  }

  // make an nsURI from the argument url
  rv = ioService->NewURI(inCookieURL, nsnull, getter_AddRefs(uriIn));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = cookieService->GetCookieString(uriIn, cookieString);
  
  if (NS_FAILED(rv) || 
      (inOutCookieSize < cookieString.Length())) {
    return NS_ERROR_FAILURE;
  }
  bufPtr = cookieString.ToCString((char *) inOutCookieBuffer, 
                                  inOutCookieSize);
  if (nsnull == bufPtr) {
    return NS_ERROR_FAILURE;
  }
  inOutCookieSize = cookieString.Length();
  rv = NS_OK;
  
  return rv;
}

NS_IMETHODIMP nsPluginHostImpl::SetCookie(const char* inCookieURL, const void* inCookieBuffer, PRUint32 inCookieSize)
{
  nsresult rv = NS_ERROR_NOT_IMPLEMENTED;
  nsString cookieString;
  nsCOMPtr<nsIURI> uriIn;
  
  if ((nsnull == inCookieURL) || (nsnull == inCookieBuffer) || 
      (0 >= inCookieSize)) {
    return NS_ERROR_INVALID_ARG;
  }
  
  NS_WITH_SERVICE(nsIIOService, ioService, kIOServiceCID, &rv);
  
  if (NS_FAILED(rv) || (nsnull == ioService)) {
    return rv;
  }
  
  NS_WITH_SERVICE(nsICookieService, cookieService, kCookieServiceCID, &rv);
  
  if (NS_FAILED(rv) || (nsnull == cookieService)) {
    return NS_ERROR_FAILURE;
  }
  
  // make an nsURI from the argument url
  rv = ioService->NewURI(inCookieURL, nsnull, getter_AddRefs(uriIn));
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }
  
  cookieString.AssignWithConversion((const char *) inCookieBuffer,(PRInt32) inCookieSize);
  
  rv = cookieService->SetCookieString(uriIn, nsnull, cookieString); // needs an nsHTMLDocument parameter
  
  return rv;
}

NS_IMETHODIMP nsPluginHostImpl::Observe(nsISupports *aSubject,
                                        const PRUnichar *aTopic,
                                        const PRUnichar *someData)
{
  Destroy();
  return NS_OK;
}

NS_IMETHODIMP nsPluginHostImpl::HandleBadPlugin(PRLibrary* aLibrary)
{
  nsresult rv = NS_OK;

  NS_ASSERTION(PR_FALSE, "Plugin performed illegal operation");

  if(mDontShowBadPluginMessage)
    return rv;
  
  nsCOMPtr<nsIPrompt> prompt(do_GetService(kNetSupportDialogCID));
  nsCOMPtr<nsIIOService> io(do_GetService(kIOServiceCID));
  nsCOMPtr<nsIStringBundleService> strings(do_GetService(kStringBundleServiceCID));

  if (!prompt || !io || !strings)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStringBundle> bundle;
  nsCOMPtr<nsIURI> uri;
  char *spec = nsnull;
  nsILocale* locale = nsnull;

  PRInt32 buttonPressed;
  PRBool checkboxState = PR_FALSE;
  
  rv = io->NewURI(PLUGIN_PROPERTIES_URL, nsnull, getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return rv;

  rv = uri->GetSpec(&spec);
  if (NS_FAILED(rv)) 
  {
    nsCRT::free(spec);
    return rv;
  }

  rv = strings->CreateBundle(spec, locale, getter_AddRefs(bundle));
  nsCRT::free(spec);
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
  if (NS_FAILED(rv))
  {
    nsMemory::Free((void *)title);
    return rv;
  }
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("BadPluginCheckboxMessage").get(), 
                                 &checkboxMessage);
  if (NS_FAILED(rv))
  {
    nsMemory::Free((void *)title);
    nsMemory::Free((void *)message);
    return rv;
  }

  rv = prompt->UniversalDialog(nsnull, /* title message */
                               title, /* title text in top line of window */
                               message, /* this is the main message */
                               checkboxMessage, /* This is the checkbox message */
                               nsnull, /* first button text, becomes OK by default */
                               nsnull, /* second button text, becomes CANCEL by default */
                               nsnull, /* third button text */
                               nsnull, /* fourth button text */
                               nsnull, /* first edit field label */
                               nsnull, /* second edit field label */
                               nsnull, /* first edit field initial and final value */
                               nsnull, /* second edit field initial and final value */
                               nsnull,  /* icon: question mark by default */
                               &checkboxState, /* initial and final value of checkbox */
                               1, /* number of buttons */
                               0, /* number of edit fields */
                               0, /* is first edit field a password field */
                               &buttonPressed);

  if (checkboxState)
    mDontShowBadPluginMessage = PR_TRUE;

  nsMemory::Free((void *)title);
  nsMemory::Free((void *)message);
  nsMemory::Free((void *)checkboxMessage);
  return rv;
}
