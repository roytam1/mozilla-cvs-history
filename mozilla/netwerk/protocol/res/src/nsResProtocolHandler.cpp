/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsResProtocolHandler.h"
#include "nsAutoLock.h"
#include "nsIURL.h"
#include "nsIComponentManager.h"
#include "nsResChannel.h"

static NS_DEFINE_CID(kStandardURLCID, NS_STANDARDURL_CID);

////////////////////////////////////////////////////////////////////////////////

nsResProtocolHandler::nsResProtocolHandler()
    : mLock(nsnull), mSubstitutions(32)
{
    NS_INIT_REFCNT();
}

nsresult
nsResProtocolHandler::SetSpecialDir(const char* name, nsSpecialSystemDirectory::SystemDirectories sysDir)
{
    nsSpecialSystemDirectory dir(sysDir);
    nsFileURL fileURL(dir);

    return AppendSubstitution(name, fileURL.GetURLString());
}

nsresult
nsResProtocolHandler::Init()
{
    nsresult rv;

    mLock = PR_NewLock();
    if (mLock == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    // set up initial mappings
    rv = SetSpecialDir("ProgramDir", nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
    if (NS_FAILED(rv)) return rv;

    // make "res:///" == "resource:/"
    rv = SetSpecialDir("", nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("CurrentDir", nsSpecialSystemDirectory::OS_CurrentWorkingDirectory);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("CurrentDrive", nsSpecialSystemDirectory::OS_DriveDirectory);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("TempDir", nsSpecialSystemDirectory::OS_TemporaryDirectory);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("ComponentsDir", nsSpecialSystemDirectory::XPCOM_CurrentProcessComponentDirectory);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("SystemDir",
#ifdef XP_MAC
                       nsSpecialSystemDirectory::Mac_SystemDirectory
#elif XP_PC
                       nsSpecialSystemDirectory::Win_SystemDirectory
#else
                       nsSpecialSystemDirectory::Unix_LibDirectory  // XXX ???
#endif
                       );
    if (NS_FAILED(rv)) return rv;

    // Set up the "Resource" root to point to the old resource location 
    // such that:
    //     resource://<path>  ==  res://Resource/<path>
#ifdef XP_PC

    // XXX why is this one inconsistent -- Mac Unix and BeOS seem to look inthe current process dir
    rv = SetSpecialDir("Resource", nsSpecialSystemDirectory::XPCOM_CurrentProcessComponentRegistry);
    if (NS_FAILED(rv)) return rv;

#elif defined (XP_MAC)

    rv = SetSpecialDir("Resource", nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
    if (NS_FAILED(rv)) return rv;

#elif defined(XP_UNIX)
    
    // first add $MOZILLA_FIVE_HOME if it exists
    char* path = PR_GetEnv("MOZILLA_FIVE_HOME");
    if (path) {
        char* fileURL = PR_smprintf("file:///%s", path);
        PR_Free(path);
        rv = AppendSubstitution("Resource", fileURL);
        PR_Free(fileURL);
        if (NS_FAILED(rv)) return rv;
    }

    // then add pwd
    char homepath[MAXPATHLEN];
    FILE* pp;
    if (!(pp = popen("pwd", "r"))) {
        NS_WARNING("res protocol: can't open pwd");
        return NS_ERROR_FAILURE;
    }
    if (fgets(homepath, MAXPATHLEN, pp)) {
        homepath[PL_strlen(homepath)-1] = 0;
    }
    else {
        NS_WARNING("res protocol: can't get homepath");

        pclose(pp);
        return NS_ERROR_FAILURE;
    }
    pclose(pp);
    char* fileURL = PR_smprintf("file:///%s", homepath);
    rv = AppendSubstitution("Resource", fileURL);
    PR_Free(fileURL);
    if (NS_FAILED(rv)) return rv;

#elif defined(XP_BEOS)

    // first add $MOZILLA_FIVE_HOME if it exists
    const char *moz5 = getenv("MOZILLA_FIVE_HOME");
    if (moz5) {
        char* fileURL = PR_smprintf("file:///%s", moz5);
        rv = AppendSubstitution("Resource", fileURL);
        PR_Free(fileURL);
        if (NS_FAILED(rv)) return rv;
    }

    // then add pwd
    static char buf[MAXPATHLEN];
    int32 cookie = 0;
    image_info info;
    char *p;
    *buf = 0;
    if (get_next_image_info(0, &cookie, &info) != B_OK) {
        NS_WARNING("res protocol: get_next_image_info failed");
        return NS_ERROR_FAILURE;
    }
    strcpy(buf, info.name);
    if ((p = strrchr(buf, '/')) == 0) {
        NS_WARNING("res protocol: no slash in working dir");
        return NS_ERROR_FAILURE;
    }
    *p = 0;
    char* fileURL = PR_smprintf("file:///%s", buf);
    rv = AppendSubstitution("Resource", fileURL);
    if (NS_FAILED(rv)) return rv;

#endif

    return rv;
}

static PR_CALLBACK PRBool
DeleteCStringArray(nsHashKey *aKey, void *aData, void* closure)
{
    nsCStringArray* array = NS_STATIC_CAST(nsCStringArray*, aData);
    if (array)
        delete array;
    return PR_TRUE;
}

nsResProtocolHandler::~nsResProtocolHandler()
{
    if (mLock)
        PR_DestroyLock(mLock);
    mSubstitutions.Enumerate(DeleteCStringArray, nsnull);
}

NS_IMPL_ISUPPORTS2(nsResProtocolHandler, nsIResProtocolHandler, nsIProtocolHandler)

NS_METHOD
nsResProtocolHandler::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsResProtocolHandler* ph = new nsResProtocolHandler();
    if (ph == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ph);
    nsresult rv = ph->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = ph->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(ph);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP
nsResProtocolHandler::GetScheme(char* *result)
{
    *result = nsCRT::strdup("res");
    if (*result == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        // no port for res: URLs
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::NewURI(const char *aSpec, nsIURI *aBaseURI,
                             nsIURI **result)
{
    nsresult rv;

    // Res: URLs (currently) have no additional structure beyond that provided by standard
    // URLs, so there is no "outer" given to CreateInstance 

    nsIURI* url;
    if (aBaseURI) {
        rv = aBaseURI->Clone(&url);
        if (NS_FAILED(rv)) return rv;
        rv = url->SetRelativePath(aSpec);
    }
    else {
        rv = nsComponentManager::CreateInstance(kStandardURLCID, nsnull,
                                                NS_GET_IID(nsIURI),
                                                (void**)&url);
        if (NS_FAILED(rv)) return rv;
        rv = url->SetSpec((char*)aSpec);
    }

    if (NS_FAILED(rv)) {
        NS_RELEASE(url);
        return rv;
    }

    *result = url;
    return rv;
}

NS_IMETHODIMP
nsResProtocolHandler::NewChannel(const char* command, nsIURI* uri,
                                 nsILoadGroup* aLoadGroup,
                                 nsIInterfaceRequestor* notificationCallbacks,
                                 nsLoadFlags loadAttributes,
                                 nsIURI* originalURI,
                                 PRUint32 bufferSegmentSize,
                                 PRUint32 bufferMaxSize,
                                 nsIChannel* *result)
{
    nsresult rv;
    
    nsResChannel* channel;
    rv = nsResChannel::Create(nsnull, NS_GET_IID(nsIResChannel), (void**)&channel);
    if (NS_FAILED(rv)) return rv;

    rv = channel->Init(this, command, uri, aLoadGroup, notificationCallbacks,
                       loadAttributes, originalURI, bufferSegmentSize, bufferMaxSize);
    if (NS_FAILED(rv)) {
        NS_RELEASE(channel);
        return rv;
    }

    *result = channel;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

nsresult
nsResProtocolHandler::RawGetSubstitutions(const char *root, nsCStringArray* *result)
{
    nsStringKey key(root);
    nsCStringArray* strings = (nsCStringArray*)mSubstitutions.Get(&key);
    if (strings == nsnull) {
        strings = new nsCStringArray();
        if (strings == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        (void)mSubstitutions.Put(&key, strings);
    }
    *result = strings;
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::PrependSubstitution(const char *root, const char *url)
{
    nsresult rv;
    nsAutoLock lock(mLock);

    nsCStringArray* strings;
    rv = RawGetSubstitutions(root, &strings);
    if (NS_FAILED(rv)) return rv;

    nsCString urlStr(url);
    PRBool ok = strings->InsertCStringAt(urlStr, 0);
    if (!ok) return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::AppendSubstitution(const char *root, const char *url)
{
    nsresult rv;
    nsAutoLock lock(mLock);

    nsCStringArray* strings;
    rv = RawGetSubstitutions(root, &strings);
    if (NS_FAILED(rv)) return rv;

    nsCString urlStr(url);
    PRBool ok = strings->AppendCString(urlStr);
    if (!ok) return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::RemoveSubstitution(const char *root, const char *url)
{
    nsresult rv;
    nsAutoLock lock(mLock);

    nsCStringArray* strings;
    rv = RawGetSubstitutions(root, &strings);
    if (NS_FAILED(rv)) return rv;

    nsCString urlStr(url);
    PRBool ok = strings->RemoveCString(urlStr);
    if (!ok) return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::GetSubstitutions(const char *root, nsCStringArray* *result)
{
    nsresult rv;
    nsAutoLock lock(mLock);

    nsCStringArray* strings;
    rv = RawGetSubstitutions(root, &strings);
    if (NS_FAILED(rv)) return rv;

    // XXX should put this in nsCStringArray::Clone
    nsCStringArray* subst = new nsCStringArray();
    if (subst == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    PRInt32 len = strings->Count();
    for (PRInt32 i = 0; i < len; i++) {
        PRBool ok = subst->AppendCString(*(*strings)[i]);
        if (!ok) {
            delete subst;
            return NS_ERROR_FAILURE;
        }
    }

    *result = subst;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
