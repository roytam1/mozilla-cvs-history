/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is The Browser Profile Migrator.
 *
 * The Initial Developer of the Original Code is Ben Goodger.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Ben Goodger <ben@bengoodger.com>
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

#include "nsBrowserProfileMigratorUtils.h"
#include "nsCRT.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIObserverService.h"
#include "nsIPrefLocalizedString.h"
#include "nsIPrefService.h"
#include "nsIProfile.h"
#include "nsIProfileInternal.h"
#include "nsIRegistry.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsISupportsPrimitives.h"
#include "nsSeamonkeyProfileMigrator.h"
#include "nsVoidArray.h"

///////////////////////////////////////////////////////////////////////////////
// nsSeamonkeyProfileMigrator

#define FILE_NAME_BOOKMARKS       NS_LITERAL_STRING("bookmarks.html")
#define FILE_NAME_COOKIES         NS_LITERAL_STRING("cookies.txt")
#define FILE_NAME_CERT8DB         NS_LITERAL_STRING("cert8.db")
#define FILE_NAME_KEY3DB          NS_LITERAL_STRING("key3.db")
#define FILE_NAME_SECMODDB        NS_LITERAL_STRING("secmod.db")
#define FILE_NAME_HISTORY         NS_LITERAL_STRING("history.dat")
#define FILE_NAME_MIMETYPES       NS_LITERAL_STRING("mimeTypes.rdf")
#define FILE_NAME_DOWNLOADS       NS_LITERAL_STRING("downloads.rdf")
#define FILE_NAME_PREFS           NS_LITERAL_STRING("prefs.js")
#define FILE_NAME_SEARCH          NS_LITERAL_STRING("search.rdf")
#define FILE_NAME_USERCONTENT     NS_LITERAL_STRING("userContent.css")
#define DIR_NAME_CHROME           NS_LITERAL_STRING("chrome")

NS_IMPL_ISUPPORTS1(nsSeamonkeyProfileMigrator, nsIBrowserProfileMigrator)

static nsIObserverService* sObserverService = nsnull;

nsSeamonkeyProfileMigrator::nsSeamonkeyProfileMigrator()
{
  CallGetService("@mozilla.org/observer-service;1", &sObserverService);
}

nsSeamonkeyProfileMigrator::~nsSeamonkeyProfileMigrator()
{
  NS_IF_RELEASE(sObserverService);
}

///////////////////////////////////////////////////////////////////////////////
// nsIBrowserProfileMigrator

NS_IMETHODIMP
nsSeamonkeyProfileMigrator::Migrate(PRUint32 aItems, PRBool aReplace, const PRUnichar* aProfile)
{
  nsresult rv = NS_OK;

  NOTIFY_OBSERVERS(MIGRATION_STARTED, nsnull);

  CreateTemplateProfile(aProfile);

  GetSourceProfile(aProfile);

  if (aReplace) {
    COPY_DATA(CopyPreferences,  aReplace, nsIBrowserProfileMigrator::SETTINGS,  NS_LITERAL_STRING("settings").get());
    COPY_DATA(CopyCookies,      aReplace, nsIBrowserProfileMigrator::COOKIES,   NS_LITERAL_STRING("cookies").get());
    COPY_DATA(CopyHistory,      aReplace, nsIBrowserProfileMigrator::HISTORY,   NS_LITERAL_STRING("history").get());
    COPY_DATA(CopyPasswords,    aReplace, nsIBrowserProfileMigrator::PASSWORDS, NS_LITERAL_STRING("passwords").get());
    COPY_DATA(CopyOtherData,    aReplace, nsIBrowserProfileMigrator::OTHERDATA, NS_LITERAL_STRING("otherdata").get());
  }
  COPY_DATA(CopyBookmarks,    aReplace, nsIBrowserProfileMigrator::BOOKMARKS, NS_LITERAL_STRING("bookmarks").get());

  NOTIFY_OBSERVERS(MIGRATION_ENDED, nsnull);

  return rv;
}

NS_IMETHODIMP
nsSeamonkeyProfileMigrator::GetSourceHasMultipleProfiles(PRBool* aResult)
{
  nsCOMPtr<nsISupportsArray> profiles;
  GetSourceProfiles(getter_AddRefs(profiles));

  if (profiles) {
    PRUint32 count;
    profiles->Count(&count);
    *aResult = count > 1;
  }
  else
    *aResult = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsSeamonkeyProfileMigrator::GetSourceProfiles(nsISupportsArray** aResult)
{
  if (!mProfileNames && !mProfileLocations) {
    nsresult rv = NS_NewISupportsArray(getter_AddRefs(mProfileNames));
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewISupportsArray(getter_AddRefs(mProfileLocations));
    if (NS_FAILED(rv)) return rv;

    GetProfileDataFromSeamonkeyRegistry(mProfileNames, mProfileLocations);
  }
  
  NS_IF_ADDREF(*aResult = mProfileNames);
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// nsSeamonkeyProfileMigrator

nsresult
nsSeamonkeyProfileMigrator::GetSourceProfile(const PRUnichar* aProfile)
{
  PRUint32 count;
  mProfileNames->Count(&count);
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsISupportsString> str(do_QueryElementAt(mProfileNames, i));
    nsXPIDLString profileName;
    str->GetData(profileName);
    if (profileName.Equals(aProfile)) {
      mSourceProfile = do_QueryElementAt(mProfileLocations, i);
      break;
    }
  }

  return NS_OK;
}

nsresult
nsSeamonkeyProfileMigrator::GetProfileDataFromSeamonkeyRegistry(nsISupportsArray* aProfileNames,
                                                                nsISupportsArray* aProfileLocations)
{
  nsresult rv = NS_OK;

  // Find the Seamonkey Registry
  nsCOMPtr<nsIProperties> fileLocator(do_GetService("@mozilla.org/file/directory_service;1"));
  nsCOMPtr<nsILocalFile> seamonkeyRegistry;
  fileLocator->Get(NS_WIN_APPDATA_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(seamonkeyRegistry));

  seamonkeyRegistry->Append(NS_LITERAL_STRING("Mozilla"));
  seamonkeyRegistry->Append(NS_LITERAL_STRING("registry.dat"));

  // Open It
  nsCOMPtr<nsIRegistry> reg(do_CreateInstance("@mozilla.org/registry;1"));
  reg->Open(seamonkeyRegistry);

  nsRegistryKey profilesTree;
  rv = reg->GetKey(nsIRegistry::Common, NS_LITERAL_STRING("Profiles").get(), &profilesTree);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIEnumerator> keys;
  reg->EnumerateSubtrees(profilesTree, getter_AddRefs(keys));

  keys->First();
  while (keys->IsDone() != NS_OK) {
    nsCOMPtr<nsISupports> key;
    keys->CurrentItem(getter_AddRefs(key));

    nsCOMPtr<nsIRegistryNode> node(do_QueryInterface(key));

    nsRegistryKey profile;
    node->GetKey(&profile);

    // "migrated" is "yes" for all valid Seamonkey profiles. It is only "no"
    // for 4.x profiles. 
    nsXPIDLString isMigrated;
    reg->GetString(profile, NS_LITERAL_STRING("migrated").get(), getter_Copies(isMigrated));

    if (isMigrated.Equals(NS_LITERAL_STRING("no"))) {
      keys->Next();
      continue;
    }

    // Get the profile name and add it to the names array
    nsXPIDLString profileName;
    node->GetName(getter_Copies(profileName));

    nsCOMPtr<nsISupportsString> profileNameString(do_CreateInstance("@mozilla.org/supports-string;1"));
    profileNameString->SetData(profileName);
    mProfileNames->AppendElement(profileNameString);

    // Get the profile location and add it to the locations array
    nsXPIDLString directory;
    reg->GetString(profile, NS_LITERAL_STRING("directory").get(), getter_Copies(directory));

    nsCOMPtr<nsILocalFile> dir;
#ifdef XP_MACOSX
    rv = NS_NewNativeLocalFile(nsCString(), PR_TRUE, getter_AddRefs(dir));
    if (NS_FAILED(rv)) return rv;
    dir->SetPersistentDescriptor(NS_LossyConvertUCS2toASCII(directory));
#else
    rv = NS_NewLocalFile(directory, PR_TRUE, getter_AddRefs(dir));
    if (NS_FAILED(rv)) return rv;
#endif

    PRBool exists;
    dir->Exists(&exists);

    if (exists)
      mProfileLocations->AppendElement(dir);

    keys->Next();
  }
}

#define F(a) nsSeamonkeyProfileMigrator::a

#define MAKEPREFTRANSFORM(pref, newpref, getmethod, setmethod) \
  { pref, newpref, F(Get##getmethod), F(Set##setmethod), PR_FALSE, -1 }

#define MAKESAMETYPEPREFTRANSFORM(pref, method) \
  { pref, 0, F(Get##method), F(Set##method), PR_FALSE, -1 }


static 
nsSeamonkeyProfileMigrator::PREFTRANSFORM gTransforms[] = {
  MAKESAMETYPEPREFTRANSFORM("signon.SignonFileName",                    String),
  MAKESAMETYPEPREFTRANSFORM("browser.startup.homepage",                 WString),
  MAKESAMETYPEPREFTRANSFORM("browser.history_expire_days",              Int),
  MAKESAMETYPEPREFTRANSFORM("browser.tabs.autoHide",                    Bool),
  MAKESAMETYPEPREFTRANSFORM("browser.tabs.loadInBackground",            Bool),
  MAKESAMETYPEPREFTRANSFORM("browser.enable_automatic_image_resizing",  Bool),
  MAKESAMETYPEPREFTRANSFORM("network.cookie.warnAboutCookies",          Bool),
  MAKESAMETYPEPREFTRANSFORM("network.cookie.lifetime.enabled",          Bool),
  MAKESAMETYPEPREFTRANSFORM("network.cookie.lifetime.behavior",         Int),
  MAKESAMETYPEPREFTRANSFORM("dom.disable_open_during_load",             Bool),
  MAKESAMETYPEPREFTRANSFORM("signon.rememberSignons",                   Bool),
  MAKESAMETYPEPREFTRANSFORM("security.enable_ssl2",                     Bool),
  MAKESAMETYPEPREFTRANSFORM("security.enable_ssl3",                     Bool),
  MAKESAMETYPEPREFTRANSFORM("security.enable_tls",                      Bool),
  MAKESAMETYPEPREFTRANSFORM("security.warn_entering_secure",            Bool),
  MAKESAMETYPEPREFTRANSFORM("security.warn_entering_weak",              Bool),
  MAKESAMETYPEPREFTRANSFORM("security.warn_leaving_secure",             Bool),
  MAKESAMETYPEPREFTRANSFORM("security.warn_submit_insecure",            Bool),
  MAKESAMETYPEPREFTRANSFORM("security.warn_viewing_mixed",              Bool),
  MAKESAMETYPEPREFTRANSFORM("security.default_personal_cert",           String),
  MAKESAMETYPEPREFTRANSFORM("security.OSCP.enabled",                    Int),
  MAKESAMETYPEPREFTRANSFORM("security.OSCP.signingCA",                  String),
  MAKESAMETYPEPREFTRANSFORM("security.OSCP.URL",                        String),
  MAKESAMETYPEPREFTRANSFORM("security.enable_java",                     Bool),
  MAKESAMETYPEPREFTRANSFORM("javascript.enabled",                       Bool),
  MAKESAMETYPEPREFTRANSFORM("dom.disable_window_move_resize",           Bool),
  MAKESAMETYPEPREFTRANSFORM("dom.disable_window_flip",                  Bool),
  MAKESAMETYPEPREFTRANSFORM("dom.disable_window_open_feature.status",   Bool),
  MAKESAMETYPEPREFTRANSFORM("dom.disable_window_status_change",         Bool),
  MAKESAMETYPEPREFTRANSFORM("dom.disable_image_src_set",                Bool),
  MAKESAMETYPEPREFTRANSFORM("accessibility.typeaheadfind.autostart",    Bool),
  MAKESAMETYPEPREFTRANSFORM("accessibility.typeaheadfind.linksonly",    Bool),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.type",                       Int),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.http",                       String),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.http_port",                  Int),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.ftp",                        String),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.ftp_port",                   Int),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.ssl",                        String),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.ssl_port",                   Int),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.socks",                      String),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.socks_port",                 Int),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.gopher",                     String),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.gopher_port",                Int),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.no_proxies_on",              String),
  MAKESAMETYPEPREFTRANSFORM("network.proxy.autoconfig_url",             String),
  MAKESAMETYPEPREFTRANSFORM("browser.display.foreground_color",         String),
  MAKESAMETYPEPREFTRANSFORM("browser.display.background_color",         String),
  MAKESAMETYPEPREFTRANSFORM("browser.anchor_color",                     String),
  MAKESAMETYPEPREFTRANSFORM("browser.visited_color",                    String),
  MAKESAMETYPEPREFTRANSFORM("browser.underline_anchors",                Bool),
  MAKESAMETYPEPREFTRANSFORM("browser.display.use_system_colors",        Bool),
  MAKESAMETYPEPREFTRANSFORM("browser.display.use_document_colors",      Bool),
  MAKESAMETYPEPREFTRANSFORM("browser.display.screen_resolution",        Int),
  MAKESAMETYPEPREFTRANSFORM("browser.display.use_document_fonts",       Bool),

  MAKEPREFTRANSFORM("network.image.imageBehavior",      0, Int, Image),
  MAKEPREFTRANSFORM("network.cookie.cookieBehavior",    0, Int, Cookie),
  MAKEPREFTRANSFORM("browser.downloadmanager.behavior", 0, Int, DownloadManager),

  MAKEPREFTRANSFORM("wallet.captureForms", "formfill.enabled", Bool, Bool)
};

nsresult 
nsSeamonkeyProfileMigrator::SetImage(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  nsresult rv = NS_OK;

  if (xform->prefHasValue)
    rv = aBranch->SetIntPref("network.image.imageBehavior", xform->intValue == 1 ? 0 : xform->intValue);

  return rv;
}

nsresult 
nsSeamonkeyProfileMigrator::SetCookie(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  nsresult rv = NS_OK;

  if (xform->prefHasValue)
    rv = aBranch->SetIntPref("network.cookie.cookieBehavior", xform->intValue == 3 ? 0 : xform->intValue);

  return rv;
}

nsresult 
nsSeamonkeyProfileMigrator::SetDownloadManager(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  nsresult rv = NS_OK;
  
  if (xform->prefHasValue) {
    // Seamonkey's download manager uses a single pref to control behavior:
    // 0 - show download manager window
    // 1 - show individual progress dialogs
    // 2 - show nothing
    //
    // Firebird has only a download manager window, but it can behave like a progress dialog, thus:
    // 0 || 1  -> show downloads window when a download starts
    // 2       -> don't show anything when a download starts
    // 1       -> close the downloads window as if it were a progress window when downloads complete.
    //
    rv |= aBranch->SetBoolPref("browser.download.manager.showWhenStarting", xform->intValue != 2);
    rv |= aBranch->SetBoolPref("browser.download.manager.closeWhenDone", xform->intValue == 1);
  }
  return NS_OK;
}

nsresult
nsSeamonkeyProfileMigrator::TransformPreferences(const nsAString& aSourcePrefFileName,
                                                 const nsAString& aTargetPrefFileName)
{
  PREFTRANSFORM* transform;
  PREFTRANSFORM* end = gTransforms + sizeof(gTransforms)/sizeof(PREFTRANSFORM);

  // Load the source pref file
  nsCOMPtr<nsIPrefService> psvc(do_GetService(NS_PREFSERVICE_CONTRACTID));
  psvc->ResetPrefs();

  nsCOMPtr<nsIFile> sourcePrefsFile;
  mSourceProfile->Clone(getter_AddRefs(sourcePrefsFile));
  sourcePrefsFile->Append(aSourcePrefFileName);
  psvc->ReadUserPrefs(sourcePrefsFile);

  nsCOMPtr<nsIPrefBranch> branch(do_QueryInterface(psvc));
  for (transform = gTransforms; transform < end; ++transform)
    transform->prefGetterFunc(transform, branch);

  nsVoidArray* fontPrefs = new nsVoidArray();
  if (!fontPrefs)
    return NS_ERROR_OUT_OF_MEMORY;
  ReadFontsBranch(psvc, fontPrefs);

  // Now that we have all the pref data in memory, load the target pref file,
  // and write it back out
  psvc->ResetPrefs();
  for (transform = gTransforms; transform < end; ++transform)
    transform->prefSetterFunc(transform, branch);

  WriteFontsBranch(psvc, fontPrefs);
  delete fontPrefs;
  fontPrefs = nsnull;

  nsCOMPtr<nsIFile> targetPrefsFile;
  mTargetProfile->Clone(getter_AddRefs(targetPrefsFile));
  targetPrefsFile->Append(aTargetPrefFileName);
  psvc->SavePrefFile(targetPrefsFile);

  return NS_OK;
}

typedef struct {
  char*         prefName;
  PRInt32       type;
  union {
    char*       stringValue;
    PRInt32     intValue;
    PRBool      boolValue;
    PRUnichar*  wstringValue;
  };
} FontPref;

void
nsSeamonkeyProfileMigrator::ReadFontsBranch(nsIPrefService* aPrefService, 
                                            nsVoidArray* aPrefs)
{
  // Enumerate the branch
  nsCOMPtr<nsIPrefBranch> branch;
  aPrefService->GetBranch("font.", getter_AddRefs(branch));

  PRUint32 count;
  char** prefs = nsnull;
  nsresult rv = branch->GetChildList("", &count, &prefs);
  if (NS_FAILED(rv)) return;

  for (PRUint32 i = 0; i < count; ++i) {
    // Save each pref's value into an array
    char* currPref = prefs[i];
    PRInt32 type;
    branch->GetPrefType(currPref, &type);
    FontPref* pref = new FontPref;
    pref->prefName = currPref;
    pref->type = type;
    switch (type) {
    case nsIPrefBranch::PREF_STRING:
      rv = branch->GetCharPref(currPref, &pref->stringValue);
      break;
    case nsIPrefBranch::PREF_BOOL:
      rv = branch->GetBoolPref(currPref, &pref->boolValue);
      break;
    case nsIPrefBranch::PREF_INT:
      rv = branch->GetIntPref(currPref, &pref->intValue);
      break;
    case nsIPrefBranch::PREF_INVALID:
      {
        nsCOMPtr<nsIPrefLocalizedString> str;
        rv = branch->GetComplexValue(currPref, 
                                    NS_GET_IID(nsIPrefLocalizedString), 
                                    getter_AddRefs(str));
        if (NS_SUCCEEDED(rv) && str)
          str->ToString(&pref->wstringValue);
      }
      break;
    }

    if (NS_SUCCEEDED(rv))
      aPrefs->AppendElement((void*)pref);
  }
}

void
nsSeamonkeyProfileMigrator::WriteFontsBranch(nsIPrefService* aPrefService,
                                             nsVoidArray* aPrefs)
{
  nsresult rv;

  // Enumerate the branch
  nsCOMPtr<nsIPrefBranch> branch;
  aPrefService->GetBranch("font.", getter_AddRefs(branch));

  PRUint32 count = aPrefs->Count();
  for (PRUint32 i = 0; i < count; ++i) {
    FontPref* pref = (FontPref*)aPrefs->ElementAt(i);
    switch (pref->type) {
    case nsIPrefBranch::PREF_STRING:
      rv = branch->SetCharPref(pref->prefName, pref->stringValue);
      nsCRT::free(pref->stringValue);
      pref->stringValue = nsnull;
      break;
    case nsIPrefBranch::PREF_BOOL:
      rv = branch->SetBoolPref(pref->prefName, pref->boolValue);
      break;
    case nsIPrefBranch::PREF_INT:
      rv = branch->SetIntPref(pref->prefName, pref->intValue);
      break;
    case nsIPrefBranch::PREF_INVALID:
      nsCOMPtr<nsIPrefLocalizedString> pls(do_CreateInstance("@mozilla.org/pref-localizedstring;1"));
      pls->SetData(pref->wstringValue);
      rv = branch->SetComplexValue(pref->prefName, 
                                   NS_GET_IID(nsIPrefLocalizedString),
                                   pls);
      nsCRT::free(pref->wstringValue);
      pref->wstringValue = nsnull;
      break;
    }
    nsCRT::free(pref->prefName);
    pref->prefName = nsnull;
    free(pref);
    pref = nsnull;
  }
  aPrefs->Clear();
}

nsresult
nsSeamonkeyProfileMigrator::CopyPreferences(PRBool aReplace)
{
  TransformPreferences(FILE_NAME_PREFS, FILE_NAME_PREFS);

  // Security Stuff
  CopyFile(FILE_NAME_CERT8DB, FILE_NAME_CERT8DB);
  CopyFile(FILE_NAME_KEY3DB, FILE_NAME_KEY3DB);
  CopyFile(FILE_NAME_SECMODDB, FILE_NAME_SECMODDB);

  // User MIME Type overrides
  CopyFile(FILE_NAME_MIMETYPES, FILE_NAME_MIMETYPES);

  return CopyUserContentSheet();
}

nsresult 
nsSeamonkeyProfileMigrator::CopyUserContentSheet()
{
  nsCOMPtr<nsIFile> sourceUserContent;
  mSourceProfile->Clone(getter_AddRefs(sourceUserContent));
  sourceUserContent->Append(DIR_NAME_CHROME);
  sourceUserContent->Append(FILE_NAME_USERCONTENT);

  PRBool exists = PR_FALSE;
  sourceUserContent->Exists(&exists);
  if (!exists)
    return NS_OK;

  nsCOMPtr<nsIFile> targetUserContent;
  mTargetProfile->Clone(getter_AddRefs(targetUserContent));
  targetUserContent->Append(DIR_NAME_CHROME);
  nsCOMPtr<nsIFile> targetChromeDir;
  targetUserContent->Clone(getter_AddRefs(targetChromeDir));
  targetUserContent->Append(FILE_NAME_USERCONTENT);

  targetUserContent->Exists(&exists);
  if (exists)
    targetUserContent->Remove(PR_FALSE);

  return sourceUserContent->CopyTo(targetChromeDir, FILE_NAME_USERCONTENT);
}

nsresult
nsSeamonkeyProfileMigrator::CopyCookies(PRBool aReplace)
{
  return CopyFile(FILE_NAME_COOKIES, FILE_NAME_COOKIES);
}

nsresult
nsSeamonkeyProfileMigrator::CopyHistory(PRBool aReplace)
{
  return CopyFile(FILE_NAME_HISTORY, FILE_NAME_HISTORY);
}

nsresult
nsSeamonkeyProfileMigrator::CopyPasswords(PRBool aReplace)
{
  nsresult rv;

  // Find out what the signons file was called, this is stored in a pref
  // in Seamonkey.
  nsCOMPtr<nsIPrefService> psvc(do_GetService(NS_PREFSERVICE_CONTRACTID));
  psvc->ResetPrefs();

  nsCOMPtr<nsIFile> seamonkeyPrefsFile;
  mSourceProfile->Clone(getter_AddRefs(seamonkeyPrefsFile));
  seamonkeyPrefsFile->Append(FILE_NAME_PREFS);
  psvc->ReadUserPrefs(seamonkeyPrefsFile);

  nsXPIDLCString signonsFileName;
  nsCOMPtr<nsIPrefBranch> branch(do_QueryInterface(psvc));
  rv = branch->GetCharPref("signon.SignonFileName", getter_Copies(signonsFileName));

  nsAutoString fileName; fileName.AssignWithConversion(signonsFileName);
  return CopyFile(fileName, fileName);
}

nsresult
nsSeamonkeyProfileMigrator::CopyBookmarks(PRBool aReplace)
{
  if (aReplace)
    return CopyFile(FILE_NAME_BOOKMARKS, FILE_NAME_BOOKMARKS);
  return ImportNetscapeBookmarks(FILE_NAME_BOOKMARKS, 
                                 NS_LITERAL_STRING("importedSeamonkeyBookmarksTitle").get());
}

nsresult
nsSeamonkeyProfileMigrator::CopyOtherData(PRBool aReplace)
{
  return CopyFile(FILE_NAME_DOWNLOADS, FILE_NAME_DOWNLOADS);
}

