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
#include "nsIBookmarksService.h"
#include "nsIBrowserProfileMigrator.h"
#include "nsIBrowserHistory.h"
#include "nsICookieManager2.h"
#include "nsIGlobalHistory.h"
#include "nsIInputStream.h"
#include "nsILineInputStream.h"
#include "nsILocalFile.h"
#include "nsINIParser.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
#include "nsIPrefLocalizedString.h"
#include "nsIPrefService.h"
#include "nsIProperties.h"
#include "nsIRDFContainer.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsISupportsPrimitives.h"
#include "nsNetUtil.h"
#include "nsOperaProfileMigrator.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#ifdef XP_WIN
#include <windows.h>
#endif

static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
#define MIGRATION_BUNDLE "chrome://browser/locale/migration/migration.properties"

///////////////////////////////////////////////////////////////////////////////
// nsBrowserProfileMigrator

NS_IMPL_ISUPPORTS1(nsOperaProfileMigrator, nsIBrowserProfileMigrator)

static nsIObserverService* sObserverService = nsnull;

nsOperaProfileMigrator::nsOperaProfileMigrator()
{
  CallGetService("@mozilla.org/observer-service;1", &sObserverService);
}

nsOperaProfileMigrator::~nsOperaProfileMigrator()
{
  NS_IF_RELEASE(sObserverService);
}

NS_IMETHODIMP
nsOperaProfileMigrator::Migrate(PRUint32 aItems, PRBool aReplace, const PRUnichar* aProfile)
{
  nsresult rv = NS_OK;

  if (!mOperaProfile)
    GetOperaProfile(aProfile, getter_AddRefs(mOperaProfile));

  NOTIFY_OBSERVERS(MIGRATION_STARTED, nsnull);

  COPY_DATA(CopyPreferences,  aReplace, nsIBrowserProfileMigrator::SETTINGS,  NS_LITERAL_STRING("settings").get());
  COPY_DATA(CopyCookies,      aReplace, nsIBrowserProfileMigrator::COOKIES,   NS_LITERAL_STRING("cookies").get());
  COPY_DATA(CopyHistory,      aReplace, nsIBrowserProfileMigrator::HISTORY,   NS_LITERAL_STRING("history").get());
  COPY_DATA(CopyPasswords,    aReplace, nsIBrowserProfileMigrator::PASSWORDS, NS_LITERAL_STRING("passwords").get());
  COPY_DATA(CopyBookmarks,    aReplace, nsIBrowserProfileMigrator::BOOKMARKS, NS_LITERAL_STRING("bookmarks").get());
  COPY_DATA(CopyOtherData,    aReplace, nsIBrowserProfileMigrator::OTHERDATA, NS_LITERAL_STRING("otherdata").get());

  NOTIFY_OBSERVERS(MIGRATION_ENDED, nsnull);

  return rv;
}

NS_IMETHODIMP
nsOperaProfileMigrator::GetSourceHasMultipleProfiles(PRBool* aResult)
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
nsOperaProfileMigrator::GetSourceProfiles(nsISupportsArray** aResult)
{
  if (!mProfiles) {
    nsresult rv = NS_NewISupportsArray(getter_AddRefs(mProfiles));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProperties> fileLocator(do_GetService("@mozilla.org/file/directory_service;1"));
    nsCOMPtr<nsILocalFile> file;
    fileLocator->Get(NS_WIN_APPDATA_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(file));

    // Opera profile lives under %APP_DATA%\Opera\<operaver>\profile 
    file->Append(NS_LITERAL_STRING("Opera"));

    nsCOMPtr<nsISimpleEnumerator> e;
    file->GetDirectoryEntries(getter_AddRefs(e));

    PRBool hasMore;
    e->HasMoreElements(&hasMore);
    while (hasMore) {
      nsCOMPtr<nsILocalFile> curr;
      e->GetNext(getter_AddRefs(curr));

      PRBool isDirectory = PR_FALSE;
      curr->IsDirectory(&isDirectory);
      if (isDirectory) {
        nsCOMPtr<nsISupportsString> string(do_CreateInstance("@mozilla.org/supports-string;1"));
        nsAutoString leafName;
        curr->GetLeafName(leafName);
        string->SetData(leafName);
        mProfiles->AppendElement(string);
      }

      e->HasMoreElements(&hasMore);
    }
  }

  *aResult = mProfiles;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

#define _OPM(type) nsOperaProfileMigrator::type

static
nsOperaProfileMigrator::PREFTRANSFORM gTransforms[] = {
  { "User Prefs", "Download Directory", _OPM(STRING), "browser.download.defaultFolder", _OPM(SetFile), PR_FALSE, -1 },
  { nsnull, "Enable Cookies", _OPM(INT), "network.cookie.cookieBehavior", _OPM(SetCookieBehavior), PR_FALSE, -1 },
  { nsnull, "Accept Cookies Session Only", _OPM(BOOL), "network.cookie.enableForCurrentSessionOnly", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Allow script to resize window", _OPM(BOOL), "dom.disable_window_move_resize", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Allow script to move window", _OPM(BOOL), "dom.disable_window_move_resize", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Allow script to raise window", _OPM(BOOL), "dom.disable_window_flip", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Allow script to change status", _OPM(BOOL), "dom.disable_window_status_change", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Home URL", _OPM(STRING), "browser.startup.homepage", _OPM(SetWString), PR_FALSE, -1 },
  { nsnull, "Ignore Unrequested Popups", _OPM(BOOL), "dom.disable_open_during_load", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Load Figures", _OPM(BOOL), "network.image.imageBehavior", _OPM(SetImageBehavior), PR_FALSE, -1 },

  { "Visited link", nsnull, _OPM(COLOR), "browser.visited_color", _OPM(SetString), PR_FALSE, -1 },
  { "Link", nsnull, _OPM(COLOR), "browser.anchor_color", _OPM(SetString), PR_FALSE, -1 },
  { nsnull, "Underline", _OPM(BOOL), "browser.underline_anchors", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Expiry", _OPM(INT), "browser.history_expire_days", _OPM(SetInt), PR_FALSE, -1 },

  { "Security Prefs", "Enable SSL v2", _OPM(BOOL), "security.enable_ssl2", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Enable SSL v3", _OPM(BOOL), "security.enable_ssl3", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Enable TLS v1.0", _OPM(BOOL), "security.enable_tls", _OPM(SetBool), PR_FALSE, -1 },

  { "Extensions", "Scripting", _OPM(BOOL), "javascript.enabled", _OPM(SetBool), PR_FALSE, -1 }
};

nsresult 
nsOperaProfileMigrator::SetFile(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  nsCOMPtr<nsILocalFile> lf(do_CreateInstance("@mozilla.org/file/local;1"));
  lf->InitWithNativePath(nsDependentCString(xform->stringValue));
  return aBranch->SetComplexValue(xform->targetPrefName, NS_GET_IID(nsILocalFile), lf);
}

nsresult 
nsOperaProfileMigrator::SetCookieBehavior(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  PRInt32 val = (xform->intValue == 3) ? 0 : (xform->intValue == 0) ? 2 : 1;
  return aBranch->SetIntPref(xform->targetPrefName, val);
}

nsresult 
nsOperaProfileMigrator::SetImageBehavior(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetIntPref(xform->targetPrefName, xform->boolValue ? 0 : 2);
}

nsresult 
nsOperaProfileMigrator::SetBool(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetBoolPref(xform->targetPrefName, xform->intValue);
}

nsresult 
nsOperaProfileMigrator::SetWString(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  nsCOMPtr<nsIPrefLocalizedString> pls(do_CreateInstance("@mozilla.org/pref-localizedstring;1"));
  nsAutoString data; data.AssignWithConversion(xform->stringValue);
  pls->SetData(data.get());
  return aBranch->SetComplexValue(xform->targetPrefName, NS_GET_IID(nsIPrefLocalizedString), pls);
}

nsresult 
nsOperaProfileMigrator::SetInt(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetIntPref(xform->targetPrefName, xform->intValue);
}

nsresult 
nsOperaProfileMigrator::SetString(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetCharPref(xform->targetPrefName, xform->stringValue);
}

nsresult
nsOperaProfileMigrator::CopyPreferences(PRBool aReplace)
{
  nsCOMPtr<nsIFile> operaPrefs;
  mOperaProfile->Clone(getter_AddRefs(operaPrefs));
  operaPrefs->Append(NS_LITERAL_STRING("opera6.ini"));

  nsCAutoString path;
  operaPrefs->GetNativePath(path);
  char* pathCopy = ToNewCString(path);
  if (!pathCopy)
    return NS_ERROR_OUT_OF_MEMORY;
  nsINIParser* parser = new nsINIParser(pathCopy);
  if (!parser)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIPrefBranch> branch(do_GetService(NS_PREFSERVICE_CONTRACTID));

  // Traverse the standard transforms
  PREFTRANSFORM* transform;
  PREFTRANSFORM* end = gTransforms + sizeof(gTransforms)/sizeof(PREFTRANSFORM);

  PRInt32 length;
  char* lastSectionName = nsnull;
  for (transform = gTransforms; transform < end; ++transform) {
    if (transform->sectionName)
      lastSectionName = transform->sectionName;

    if (transform->type == _OPM(COLOR)) {
      char* colorString = (char*)malloc(sizeof(char) * 8);
      if (!colorString)
        return NS_ERROR_OUT_OF_MEMORY;

      nsresult rv = ParseColor(parser, lastSectionName, &colorString);
      if (NS_SUCCEEDED(rv)) {
        transform->stringValue = colorString;
   
        transform->prefHasValue = PR_TRUE;
        transform->prefSetterFunc(transform, branch);
      }
      nsCRT::free(colorString);
    }
    else {
      char* val = nsnull;
      PRInt32 err = parser->GetStringAlloc(lastSectionName, transform->keyName, &val, &length);
      if (err == nsINIParser::OK) {
        nsCAutoString valStr;
        PRInt32 strerr;
        switch (transform->type) {
        case _OPM(STRING):
          transform->stringValue = val;
          break;
        case _OPM(INT):
          valStr = val;
          transform->intValue = valStr.ToInteger(&strerr);
          break;
        case _OPM(BOOL):
          valStr = val;
          transform->boolValue = valStr.ToInteger(&strerr) != 0;
          break;
        }
        transform->prefHasValue = PR_TRUE;
        transform->prefSetterFunc(transform, branch);
      }
    }
  }

  // Copy Proxy Settings

  // Copy User Content Sheet
  if (aReplace)
    CopyUserContentSheet(parser);

  nsCRT::free(pathCopy);

  delete parser;
  parser = nsnull;

  return NS_OK;
}

nsresult
nsOperaProfileMigrator::ParseColor(nsINIParser* aParser, char* aSectionName, char** aResult)
{
#define CHAR_BUF_LENGTH 5
  char rbuf[CHAR_BUF_LENGTH], gbuf[CHAR_BUF_LENGTH], bbuf[CHAR_BUF_LENGTH];
  PRInt32 bufSize = CHAR_BUF_LENGTH;
  PRInt32 r, g, b;

  nsCAutoString valStr;
  PRInt32 err, strerr;
  err = aParser->GetString(aSectionName, "Red", rbuf, &bufSize);
  if (err != nsINIParser::OK) return NS_ERROR_FAILURE;
  valStr = rbuf;
  r = valStr.ToInteger(&strerr);
  
  bufSize = CHAR_BUF_LENGTH;
  err = aParser->GetString(aSectionName, "Green", gbuf, &bufSize);
  if (err != nsINIParser::OK) return NS_ERROR_FAILURE;
  valStr = gbuf;
  g = valStr.ToInteger(&strerr);

  bufSize = CHAR_BUF_LENGTH;
  err = aParser->GetString(aSectionName, "Blue", bbuf, &bufSize);
  if (err != nsINIParser::OK) return NS_ERROR_FAILURE;
  valStr = bbuf;
  b = valStr.ToInteger(&strerr);

  sprintf(*aResult, "#%02X%02X%02X", r, g, b);

  return NS_OK;
}

nsresult 
nsOperaProfileMigrator::CopyUserContentSheet(nsINIParser* aParser)
{
  char* userContentCSS = nsnull;
  PRInt32 size;
  PRInt32 err = aParser->GetStringAlloc("User Prefs", "Local CSS File", &userContentCSS, &size);
  if (err == nsINIParser::OK && userContentCSS) {
    // Copy the file
  }
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyCookies(PRBool aReplace)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIFile> temp;
  mOperaProfile->Clone(getter_AddRefs(temp));
  nsCOMPtr<nsILocalFile> historyFile(do_QueryInterface(temp));

  historyFile->Append(NS_LITERAL_STRING("cookies4.dat"));

  nsCOMPtr<nsIInputStream> fileStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileStream), historyFile);
  if (!fileStream) 
    return NS_ERROR_OUT_OF_MEMORY;

  nsOperaCookieMigrator* ocm = new nsOperaCookieMigrator(fileStream);
  if (!ocm)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = ocm->Migrate();

  if (ocm) {
    delete ocm;
    ocm = nsnull;
  }

  return rv;
}

nsOperaCookieMigrator::nsOperaCookieMigrator(nsIInputStream* aSourceStream) :
  mAppVersion(0), mFileVersion(0), mTagTypeLength(0), mPayloadTypeLength(0), 
  mCookieOpen(PR_FALSE), mCurrHandlingInfo(0)
{
  mStream = do_CreateInstance("@mozilla.org/binaryinputstream;1");
  if (mStream)
    mStream->SetInputStream(aSourceStream);

  mCurrCookie.isSecure = PR_FALSE;
  mCurrCookie.expiryTime = 0;
}

nsresult
nsOperaCookieMigrator::Migrate()
{
  if (!mStream)
    return NS_ERROR_FAILURE;

  nsresult rv;

  rv = ReadHeader();
  if (NS_FAILED(rv)) 
    return NS_OK;

  nsCOMPtr<nsICookieManager2> manager(do_GetService(NS_COOKIEMANAGER_CONTRACTID));
  nsCOMPtr<nsIPermissionManager> permissionManager(do_GetService("@mozilla.org/permissionmanager;1"));

  PRUint8 tag;
  PRUint16 length, segmentLength;

  char* buf = nsnull;
  do {
    if (NS_FAILED(mStream->Read8(&tag))) 
      return NS_OK; // EOF.

    switch (tag) {
    case BEGIN_DOMAIN_SEGMENT:
      mStream->Read16(&length);
      break;
    case DOMAIN_COMPONENT: 
      {
        mStream->Read16(&length);
        
        mStream->ReadBytes(length, &buf);
        buf[length] = '\0';
        mDomainStack.AppendElement((void*)buf);
      }
      break;
    case END_DOMAIN_SEGMENT:
      {
        if (mCurrHandlingInfo)
          AddCookieOverride(permissionManager);

        // Pop the domain stack
        PRUint32 count = mDomainStack.Count();
        if (count > 0)
          mDomainStack.RemoveElementAt(count - 1);
      }
      break;

    case BEGIN_PATH_SEGMENT:
      mStream->Read16(&length);
      break;
    case PATH_COMPONENT:
      {
        mStream->Read16(&length);
        
        mStream->ReadBytes(length, &buf);
        buf[length] = '\0';
        mPathStack.AppendElement((void*)buf);
      }
      break;
    case END_PATH_SEGMENT:
      {
        // Add the last remaining cookie for this path.
        if (mCookieOpen) 
          AddCookie(manager);

        // We receive one "End Path Segment" even if the path stack is empty
        // i.e. telling us that we are done processing cookies for "/"

        // Pop the path stack
        PRUint32 count = mPathStack.Count();
        if (count > 0)
          mPathStack.RemoveElementAt(count - 1);
      }
      break;

    case FILTERING_INFO:
      mStream->Read16(&length);
      mStream->Read8(&mCurrHandlingInfo);
      break;
    case PATH_HANDLING_INFO:
    case THIRD_PARTY_HANDLING_INFO: 
      {
        mStream->Read16(&length);
        PRUint8 temp;
        mStream->Read8(&temp);
      }
      break;

    case BEGIN_COOKIE_SEGMENT:
      {
        // Be sure to save the last cookie before overwriting the buffers
        // with data from subsequent cookies. 
        if (mCookieOpen)
          AddCookie(manager);

        mStream->Read16(&segmentLength);
        mCookieOpen = PR_TRUE;
      }
      break;
    case COOKIE_ID:
      {
        mStream->Read16(&length);
        mStream->ReadBytes(length, &buf);
        buf[length] = '\0';
        mCurrCookie.id.Assign(buf);
      }
      break;
    case COOKIE_DATA:
      {
        mStream->Read16(&length);
        mStream->ReadBytes(length, &buf);
        buf[length] = '\0';
        mCurrCookie.data.Assign(buf);
      }
      break;
    case COOKIE_EXPIRY:
      mStream->Read16(&length);
      mStream->Read32(NS_REINTERPRET_CAST(PRUint32*, &(mCurrCookie.expiryTime)));
      break;
    case COOKIE_SECURE:
      mCurrCookie.isSecure = PR_TRUE;
      break;

    // We don't support any of these fields but we must read them in
    // to advance the stream cursor. 
    case COOKIE_LASTUSED: 
      {
        mStream->Read16(&length);
        PRTime temp;
        mStream->Read32(NS_REINTERPRET_CAST(PRUint32*, &temp));
      }
      break;
    case COOKIE_COMMENT:
    case COOKIE_COMMENT_URL:
    case COOKIE_V1_DOMAIN:
    case COOKIE_V1_PATH:
    case COOKIE_V1_PORT_LIMITATIONS:
      {
        mStream->Read16(&length);
        mStream->ReadBytes(length, &buf);
        buf[length] = '\0';
      }
      break;
    case COOKIE_VERSION: 
      {
        mStream->Read16(&length);
        PRUint8 temp;
        mStream->Read8(&temp);
      }
      break;
    case COOKIE_OTHERFLAG_1:
    case COOKIE_OTHERFLAG_2:
    case COOKIE_OTHERFLAG_3:
    case COOKIE_OTHERFLAG_4:
    case COOKIE_OTHERFLAG_5:
    case COOKIE_OTHERFLAG_6: 
      break;
    }
  }
  while (1);
}

nsresult
nsOperaCookieMigrator::AddCookieOverride(nsIPermissionManager* aManager)
{
  nsresult rv;

  nsXPIDLCString domain;
  SynthesizeDomain(getter_Copies(domain));
  nsCOMPtr<nsIURI> uri(do_CreateInstance("@mozilla.org/network/standard-url;1"));
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
  uri->SetHost(domain);

  rv = aManager->Add(uri, "cookie", 
                     (mCurrHandlingInfo == 1 || mCurrHandlingInfo == 3) ? nsIPermissionManager::ALLOW_ACTION :
                                                                          nsIPermissionManager::DENY_ACTION);

  mCurrHandlingInfo = 0;

  return rv;
}


nsresult
nsOperaCookieMigrator::AddCookie(nsICookieManager2* aManager)
{
  // This is where we use the information gathered in all the other 
  // states to add a cookie to the Firebird Cookie Manager.
  nsXPIDLCString domain;
  SynthesizeDomain(getter_Copies(domain));

  nsXPIDLCString path;
  SynthesizePath(getter_Copies(path));

  mCookieOpen = PR_FALSE;
  
  nsresult rv = aManager->Add(domain, 
                              path, 
                              mCurrCookie.id, 
                              mCurrCookie.data, 
                              mCurrCookie.isSecure, 
                              PR_FALSE, 
                              PRInt64(mCurrCookie.expiryTime));

  mCurrCookie.isSecure = 0;
  mCurrCookie.expiryTime = 0;

  return rv;
}

void
nsOperaCookieMigrator::SynthesizePath(char** aResult)
{
  PRUint32 count = mPathStack.Count();
  nsCAutoString synthesizedPath("/");
  for (PRUint32 i = 0; i < count; ++i) {
    synthesizedPath.Append((char*)mPathStack.ElementAt(i));
    if (i != count-1)
      synthesizedPath.Append("/");
  }
  if (synthesizedPath.IsEmpty())
    synthesizedPath.Assign("/");

  *aResult = ToNewCString(synthesizedPath);
}

void
nsOperaCookieMigrator::SynthesizeDomain(char** aResult)
{
  PRUint32 count = mDomainStack.Count();
  if (count == 0)
    return;

  nsCAutoString synthesizedDomain;
  for (PRInt32 i = (PRInt32)count - 1; i >= 0; --i) {
    synthesizedDomain.Append((char*)mDomainStack.ElementAt((PRUint32)i));
    if (i != 0)
      synthesizedDomain.Append(".");
  }

  *aResult = ToNewCString(synthesizedDomain);
}

nsresult
nsOperaCookieMigrator::ReadHeader()
{
  mStream->Read32(&mAppVersion);
  mStream->Read32(&mFileVersion);

  if (mAppVersion & 0x1000 && mFileVersion & 0x2000) {
    mStream->Read16(&mTagTypeLength);
    mStream->Read16(&mPayloadTypeLength);

    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsOperaProfileMigrator::CopyHistory(PRBool aReplace)
{
  printf("*** copy opera history\n");

  nsCOMPtr<nsIBrowserHistory> hist(do_GetService(NS_GLOBALHISTORY_CONTRACTID));

  nsCOMPtr<nsIFile> temp;
  mOperaProfile->Clone(getter_AddRefs(temp));
  nsCOMPtr<nsILocalFile> historyFile(do_QueryInterface(temp));

  historyFile->Append(NS_LITERAL_STRING("global.dat"));

  nsCOMPtr<nsIInputStream> fileStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileStream), historyFile);
  if (!fileStream) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(fileStream);

  nsAutoString buffer, title, url;
  PRTime lastVisitDate;
  PRBool moreData = PR_FALSE;

  enum { TITLE, URL, LASTVISIT } state = TITLE;

  // Format is "title\nurl\nlastvisitdate"
  do {
    nsresult rv = lineStream->ReadLine(buffer, &moreData);
    if (NS_FAILED(rv))
      return rv;

    switch (state) {
    case TITLE:
      title = buffer;
      state = URL;
      break;
    case URL:
      url = buffer;
      state = LASTVISIT;
      break;
    case LASTVISIT:
      // Opera time format is a second offset, PRTime is a microsecond offset
      PRInt32 err;
      lastVisitDate = buffer.ToInteger(&err);
      
      PRInt64 temp, million;
      LL_I2L(temp, lastVisitDate);
      LL_I2L(million, PR_USEC_PER_SEC);
      LL_MUL(lastVisitDate, temp, million);

      nsCAutoString urlStr; urlStr.AssignWithConversion(url);
      hist->AddPageWithDetails(urlStr.get(), title.get(), lastVisitDate);
      
      state = TITLE;
      break;
    }
  }
  while (moreData);

  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyPasswords(PRBool aReplace)
{
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyBookmarks(PRBool aReplace)
{
  // Find Opera Bookmarks
  nsCOMPtr<nsIFile> operaBookmarks;
  mOperaProfile->Clone(getter_AddRefs(operaBookmarks));
  operaBookmarks->Append(NS_LITERAL_STRING("opera6.adr"));

  nsCOMPtr<nsIInputStream> fileInputStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), operaBookmarks);
  if (!fileInputStream) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsILineInputStream> lineInputStream(do_QueryInterface(fileInputStream));

  nsCOMPtr<nsIBookmarksService> bms(do_GetService("@mozilla.org/browser/bookmarks-service;1"));
  nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(kStringBundleServiceCID));
  nsCOMPtr<nsIStringBundle> bundle;
  bundleService->CreateBundle(MIGRATION_BUNDLE, getter_AddRefs(bundle));

  nsCOMPtr<nsIRDFService> rdf(do_GetService("@mozilla.org/rdf/rdf-service;1"));
  nsCOMPtr<nsIRDFResource> root;
  rdf->GetResource(NS_LITERAL_CSTRING("NC:BookmarksRoot"), 
                   getter_AddRefs(root));
  nsCOMPtr<nsIRDFResource> parentFolder;
  if (!aReplace) {
    nsXPIDLString importedOperaHotlistTitle;
    bundle->GetStringFromName(NS_LITERAL_STRING("importedOperaHotlistTitle").get(), 
                              getter_Copies(importedOperaHotlistTitle));

    bms->CreateFolderInContainer(importedOperaHotlistTitle.get(), 
                                 root, -1, getter_AddRefs(parentFolder));
  }
  else
    parentFolder = root;

  CopySmartKeywords(bms, bundle, parentFolder);

  nsCOMPtr<nsIRDFResource> toolbar;
  bms->GetBookmarksToolbarFolder(getter_AddRefs(toolbar));
  
  if (aReplace)
    ClearToolbarFolder(bms, toolbar);

  return ParseBookmarksFolder(lineInputStream, parentFolder, toolbar, bms);
}

nsresult
nsOperaProfileMigrator::CopySmartKeywords(nsIBookmarksService* aBMS, 
                                          nsIStringBundle* aBundle, 
                                          nsIRDFResource* aParentFolder)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIFile> smartKeywords;
  mOperaProfile->Clone(getter_AddRefs(smartKeywords));
  smartKeywords->Append(NS_LITERAL_STRING("search.ini"));

  nsCAutoString path;
  smartKeywords->GetNativePath(path);
  char* pathCopy = ToNewCString(path);
  if (!pathCopy)
    return NS_ERROR_OUT_OF_MEMORY;

  nsINIParser* parser = new nsINIParser(pathCopy);
  if (!parser)
    return NS_ERROR_OUT_OF_MEMORY;

  nsXPIDLString importedSearchUrlsTitle;
  aBundle->GetStringFromName(NS_LITERAL_STRING("importedOperaSearchUrls").get(), 
                             getter_Copies(importedSearchUrlsTitle));

  nsCOMPtr<nsIRDFResource> keywordsFolder;
  aBMS->CreateFolderInContainer(importedSearchUrlsTitle.get(), 
                                aParentFolder, -1, getter_AddRefs(keywordsFolder));

  PRInt32 sectionIndex = 1;
  char section[35];
  char *name = nsnull, *url = nsnull, *keyword = nsnull;
  PRInt32 keyValueLength = 0;
  do {
    sprintf(section, "Search Engine %d", sectionIndex++);
    PRInt32 err = parser->GetStringAlloc(section, "Name", &name, &keyValueLength);
    if (err != nsINIParser::OK)
      break;

    err = parser->GetStringAlloc(section, "URL", &url, &keyValueLength);
    if (err != nsINIParser::OK)
      continue;
    err = parser->GetStringAlloc(section, "Key", &keyword, &keyValueLength);
    if (err != nsINIParser::OK)
      continue;

    nsAutoString nameStr(NS_ConvertUTF8toUCS2(name));
    nsCOMPtr<nsIRDFResource> itemRes;

    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), url);
    if (!uri)
      return NS_ERROR_OUT_OF_MEMORY;
    nsCAutoString hostCStr;
    uri->GetHost(hostCStr);
    nsAutoString host; host.AssignWithConversion(hostCStr.get());

    const PRUnichar* descStrings[] = { NS_ConvertUTF8toUCS2(keyword).get(), host.get() };
    nsXPIDLString keywordDesc;
    aBundle->FormatStringFromName(NS_LITERAL_STRING("importedSearchUrlDesc").get(),
                                  descStrings, 2, getter_Copies(keywordDesc));

    rv = aBMS->CreateBookmarkInContainer(NS_ConvertUTF8toUCS2(name).get(), 
                                         url, 
                                         NS_ConvertUTF8toUCS2(keyword).get(), 
                                         keywordDesc.get(), 
                                         nsnull, 
                                         keywordsFolder,
                                         -1, 
                                         getter_AddRefs(itemRes));
  }
  while (1);

  return rv;
}

void
nsOperaProfileMigrator::ClearToolbarFolder(nsIBookmarksService* aBookmarksService, nsIRDFResource* aToolbarFolder)
{
  // If we're here, it means the user's doing a _replace_ import which means
  // clear out the content of this folder, and replace it with the new content
  nsCOMPtr<nsIRDFContainer> ctr(do_CreateInstance("@mozilla.org/rdf/container;1"));
  nsCOMPtr<nsIRDFDataSource> bmds(do_QueryInterface(aBookmarksService));
  ctr->Init(bmds, aToolbarFolder);

  nsCOMPtr<nsISimpleEnumerator> e;
  ctr->GetElements(getter_AddRefs(e));

  PRBool hasMore;
  e->HasMoreElements(&hasMore);
  while (hasMore) {
    nsCOMPtr<nsIRDFResource> b;
    e->GetNext(getter_AddRefs(b));

    ctr->RemoveElement(b, PR_FALSE);

    e->HasMoreElements(&hasMore);
  }
}

typedef enum { LineType_FOLDER, 
               LineType_BOOKMARK, 
               LineType_SEPARATOR, 
               LineType_NAME, 
               LineType_URL, 
               LineType_KEYWORD,
               LineType_DESCRIPTION,
               LineType_ONTOOLBAR,
               LineType_NL,
               LineType_OTHER } LineType;

static LineType GetLineType(nsAString& aBuffer, nsAString& aData)
{
  if (Substring(aBuffer, 0, 7).Equals(NS_LITERAL_STRING("#FOLDER")))
    return LineType_FOLDER;
  if (Substring(aBuffer, 0, 4).Equals(NS_LITERAL_STRING("#URL")))
    return LineType_BOOKMARK;
  if (Substring(aBuffer, 0, 1).Equals(NS_LITERAL_STRING("-")))
    return LineType_SEPARATOR;
  if (Substring(aBuffer, 1, 5).Equals(NS_LITERAL_STRING("NAME="))) {
    const nsAString& data = Substring(aBuffer, 6, aBuffer.Length() - 6);
    aData = ToNewUnicode(data);
    return LineType_NAME;
  }
  if (Substring(aBuffer, 1, 4).Equals(NS_LITERAL_STRING("URL="))) {
    const nsAString& data = Substring(aBuffer, 5, aBuffer.Length() - 5);
    aData = ToNewUnicode(data);
    return LineType_URL;
  }
  if (Substring(aBuffer, 1, 12).Equals(NS_LITERAL_STRING("DESCRIPTION="))) {
    const nsAString& data = Substring(aBuffer, 13, aBuffer.Length() - 13);
    aData = ToNewUnicode(data);
    return LineType_DESCRIPTION;
  }
  if (Substring(aBuffer, 1, 11).Equals(NS_LITERAL_STRING("SHORT NAME="))) {
    const nsAString& data = Substring(aBuffer, 12, aBuffer.Length() - 12);
    aData = ToNewUnicode(data);
    return LineType_KEYWORD;
  }
  if (Substring(aBuffer, 1, 15).Equals(NS_LITERAL_STRING("ON PERSONALBAR="))) {
    const nsAString& data = Substring(aBuffer, 16, aBuffer.Length() - 16);
    aData = ToNewUnicode(data);
    return LineType_ONTOOLBAR;
  }
  if (aBuffer.IsEmpty())
    return LineType_NL; // Newlines separate bookmarks
  return LineType_OTHER;
}

typedef enum { EntryType_BOOKMARK, EntryType_FOLDER } EntryType;

nsresult
nsOperaProfileMigrator::ParseBookmarksFolder(nsILineInputStream* aStream, 
                                             nsIRDFResource* aParent, 
                                             nsIRDFResource* aToolbar,
                                             nsIBookmarksService* aBMS)
{
  PRBool moreData = PR_FALSE;
  nsAutoString buffer;
  EntryType entryType = EntryType_BOOKMARK;
  nsAutoString name, keyword, description;
  nsCAutoString url;
  PRBool onToolbar = PR_FALSE;
  do {
    nsresult rv = aStream->ReadLine(buffer, &moreData);
    if (NS_FAILED(rv)) return rv;

    if (!moreData) break;

    nsAutoString data;
    LineType type = GetLineType(buffer, data);
    switch(type) {
    case LineType_FOLDER:
      entryType = EntryType_FOLDER;
      break;
    case LineType_BOOKMARK:
      entryType = EntryType_BOOKMARK;
      break;
    case LineType_SEPARATOR:
      // If we're here, we need to break out of the loop for the current folder, 
      // essentially terminating this instance of ParseBookmarksFolder and return
      // to the calling function, which is either ParseBookmarksFolder for a parent
      // folder, or CopyBookmarks (which means we're done parsing all bookmarks).
      goto done;
    case LineType_NAME:
      name = data;
      break;
    case LineType_URL:
      url.AssignWithConversion(data);
      break;
    case LineType_KEYWORD:
      keyword = data;
      break;
    case LineType_DESCRIPTION:
      description = data;
      break;
    case LineType_ONTOOLBAR:
      if (data.Equals(NS_LITERAL_STRING("YES")))
        onToolbar = PR_TRUE;
      break;
    case LineType_NL: {
      nsCOMPtr<nsIRDFResource> itemRes;
      NS_NAMED_LITERAL_STRING(empty, "");
      if (entryType == EntryType_BOOKMARK) {
        if (!name.IsEmpty() && !url.IsEmpty()) {
          rv = aBMS->CreateBookmarkInContainer(name.get(), 
                                               url.get(), 
                                               keyword.get(), 
                                               description.get(), 
                                               nsnull, 
                                               onToolbar ? aToolbar : aParent, 
                                               -1, 
                                               getter_AddRefs(itemRes));
          name = empty;
          url.AssignWithConversion(empty);
          keyword = empty;
          description = empty;
          if (NS_FAILED(rv)) 
            continue;
        }
      }
      else if (entryType == EntryType_FOLDER) {
        if (!name.IsEmpty()) {
          rv = aBMS->CreateFolderInContainer(name.get(), 
                                             onToolbar ? aToolbar : aParent, 
                                             -1, 
                                             getter_AddRefs(itemRes));
          name = empty;
          if (NS_FAILED(rv)) 
            continue;
          ParseBookmarksFolder(aStream, itemRes, aToolbar, aBMS);
        }
      }
      break;
    }
    case LineType_OTHER:
      break;
    }
  }
  while (1);

done:
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyOtherData(PRBool aReplace)
{
  printf("*** copy opera other data\n");

  nsCOMPtr<nsIFile> temp;
  mOperaProfile->Clone(getter_AddRefs(temp));
  nsCOMPtr<nsILocalFile> historyFile(do_QueryInterface(temp));

  historyFile->Append(NS_LITERAL_STRING("download.dat"));

  nsCOMPtr<nsIInputStream> fileStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileStream), historyFile);
  if (!fileStream) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(fileStream);

  nsAutoString buffer;
  PRBool moreData = PR_FALSE;

  // Format is "title\nurl\nlastvisitdate"
  do {
    nsresult rv = lineStream->ReadLine(buffer, &moreData);
    if (NS_FAILED(rv))
      return rv;

    printf("*** download = %ws\n", buffer.get());
  }
  while (moreData);


  return NS_OK;
}

void
nsOperaProfileMigrator::GetOperaProfile(const PRUnichar* aProfile, nsILocalFile** aFile)
{
  nsCOMPtr<nsIProperties> fileLocator(do_GetService("@mozilla.org/file/directory_service;1"));
  nsCOMPtr<nsILocalFile> file;
  fileLocator->Get(NS_WIN_APPDATA_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(file));

  // Opera profile lives under %APP_DATA%\Opera\<operaver>\profile 
  file->Append(NS_LITERAL_STRING("Opera"));
  file->Append(nsDependentString(aProfile));
  file->Append(NS_LITERAL_STRING("profile"));

  *aFile = file;
  NS_ADDREF(*aFile);
}

