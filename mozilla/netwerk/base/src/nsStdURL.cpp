/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Original Author: Gagan Saksena <gagan@netscape.com>
 *
 * Contributor(s): 
 */

#include "nsIIOService.h"
#include "nsURLHelper.h"
#include "nsStdURL.h"
#include "nsStdURLParser.h"
#include "nscore.h"
#include "nsCRT.h"
#include "nsString.h"
#include "prmem.h"
#include "prprf.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsILocalFile.h"
#include "nsEscape.h"

#if defined(XP_PC) && !defined(XP_OS2)
#include <windows.h>
#endif

static NS_DEFINE_CID(kStdURLCID, NS_STANDARDURL_CID);
static NS_DEFINE_CID(kThisStdURLImplementationCID,
                     NS_THIS_STANDARDURL_IMPLEMENTATION_CID);
static NS_DEFINE_CID(kStdURLParserCID, NS_STANDARDURLPARSER_CID);
static NS_DEFINE_CID(kAuthURLParserCID, NS_AUTHORITYURLPARSER_CID);
static NS_DEFINE_CID(kNoAuthURLParserCID, NS_NOAUTHORITYURLPARSER_CID);

// Global objects. Released from the module shudown routine.
nsIURLParser * nsStdURL::gStdURLParser = NULL;
nsIURLParser * nsStdURL::gAuthURLParser = NULL;
nsIURLParser * nsStdURL::gNoAuthURLParser = NULL;

#if defined (XP_MAC)
static void SwapSlashColon(char * s)
{
	while (*s)
	{
		if (*s == '/')
			*s++ = ':';
		else if (*s == ':')
			*s++ = '/';
		else
			*s++;
	}
} 
#endif

NS_IMETHODIMP
nsStdURL::InitGlobalObjects()
{
    nsresult rv;
    if (!gStdURLParser) {
        nsCOMPtr<nsIURLParser> urlParser;

        // Get the global instance of standard URLParser
        urlParser = do_GetService(kStdURLParserCID, &rv);
        if (NS_FAILED(rv)) return rv;
        gStdURLParser = urlParser.get();
        NS_ADDREF(gStdURLParser);

        // Get the global instance of Auth URLParser
        urlParser = do_GetService(kAuthURLParserCID, &rv);
        if (NS_FAILED(rv)) return rv;
        gAuthURLParser = urlParser.get();
        NS_ADDREF(gAuthURLParser);

        // Get the global instance of NoAuth URLParser
        urlParser = do_GetService(kNoAuthURLParserCID, &rv);
        if (NS_FAILED(rv)) return rv;
        gNoAuthURLParser = urlParser.get();
        NS_ADDREF(gNoAuthURLParser);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStdURL::ShutdownGlobalObjects()
{
    NS_IF_RELEASE(gNoAuthURLParser);
    NS_IF_RELEASE(gAuthURLParser);
    NS_IF_RELEASE(gStdURLParser);
    return NS_OK;
}

nsStdURL::nsStdURL(nsISupports* outer)
    : mScheme(nsnull),
      mUsername(nsnull),
      mPassword(nsnull),
      mHost(nsnull),
      mPort(-1),
      mDirectory(nsnull),
      mFileBaseName(nsnull),
      mFileExtension(nsnull),
      mParam(nsnull),
      mQuery(nsnull),
      mRef(nsnull),
      mDefaultPort(-1),
      mSchemeType(nsIURI::UNKNOWN)
{
    NS_INIT_AGGREGATED(outer);
    InitGlobalObjects();
    mURLParser = gStdURLParser; // XXX hack - shouldn't be needed, should be calling Init
}

NS_IMETHODIMP
nsStdURL::Init(PRUint32 urlType, PRInt32 defaultPort, 
               const char* initialSpec, nsIURI* baseURI)
{
    nsresult rv;
    switch (urlType) {
      case nsIStandardURL::URLTYPE_STANDARD:
        mURLParser = gStdURLParser;
        break;
      case nsIStandardURL::URLTYPE_AUTHORITY:
        mURLParser = gAuthURLParser;
        break;
      case nsIStandardURL::URLTYPE_NO_AUTHORITY:
        mURLParser = gNoAuthURLParser;
        break;
      default:
        NS_NOTREACHED("bad urlType");
        return NS_ERROR_FAILURE;
    }
    mDefaultPort = defaultPort;

    if (initialSpec == nsnull) return NS_OK;

    nsXPIDLCString resolvedURI;
    if (baseURI) {
        rv = baseURI->Resolve(initialSpec, getter_Copies(resolvedURI));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        resolvedURI = initialSpec;
    }
    return SetSpec(resolvedURI);
}

nsStdURL::nsStdURL(const nsStdURL& otherURL)
    : mPort(otherURL.mPort)
{
    NS_INIT_REFCNT();

    mScheme = otherURL.mScheme ? nsCRT::strdup(otherURL.mScheme) : nsnull;
    mUsername = otherURL.mUsername ? nsCRT::strdup(otherURL.mUsername) : nsnull;
    mPassword = otherURL.mPassword ? nsCRT::strdup(otherURL.mPassword) : nsnull;
    mHost = otherURL.mHost ? nsCRT::strdup(otherURL.mHost) : nsnull;
    mDirectory = otherURL.mDirectory ? nsCRT::strdup(otherURL.mDirectory) : nsnull;
    mFileBaseName = otherURL.mFileBaseName ? nsCRT::strdup(otherURL.mFileBaseName) : nsnull;
    mFileExtension = otherURL.mFileExtension ? nsCRT::strdup(otherURL.mFileExtension) : nsnull;
    mParam = otherURL.mParam ? nsCRT::strdup(otherURL.mParam) : nsnull;
    mQuery = otherURL.mQuery ? nsCRT::strdup(otherURL.mQuery) : nsnull;
    mRef= otherURL.mRef ? nsCRT::strdup(otherURL.mRef) : nsnull;
    mURLParser = otherURL.mURLParser;
    mSchemeType = otherURL.mSchemeType;

    NS_INIT_AGGREGATED(nsnull); // Todo! How?
}

nsStdURL& 
nsStdURL::operator=(const nsStdURL& otherURL)
{
    mScheme = otherURL.mScheme ? nsCRT::strdup(otherURL.mScheme) : nsnull;
    mUsername = otherURL.mUsername ? nsCRT::strdup(otherURL.mUsername) : nsnull;
    mPassword = otherURL.mPassword ? nsCRT::strdup(otherURL.mPassword) : nsnull;
    mHost = otherURL.mHost ? nsCRT::strdup(otherURL.mHost) : nsnull;
    mDirectory = otherURL.mDirectory ? nsCRT::strdup(otherURL.mDirectory) : nsnull;
    mFileBaseName = otherURL.mFileBaseName ? nsCRT::strdup(otherURL.mFileBaseName) : nsnull;
    mFileExtension = otherURL.mFileExtension ? nsCRT::strdup(otherURL.mFileExtension) : nsnull;
    mParam = otherURL.mParam ? nsCRT::strdup(otherURL.mParam) : nsnull;
    mQuery = otherURL.mQuery ? nsCRT::strdup(otherURL.mQuery) : nsnull;
    mRef= otherURL.mRef ? nsCRT::strdup(otherURL.mRef) : nsnull;
    mURLParser = otherURL.mURLParser;
    mSchemeType = otherURL.mSchemeType;

    NS_INIT_AGGREGATED(nsnull); // Todo! How?
    return *this;
}

PRBool
nsStdURL::operator==(const nsStdURL& otherURL) const
{
    PRBool retValue = PR_FALSE;
    ((nsStdURL*)(this))->Equals((nsIURI*)&otherURL,&retValue);
    return retValue;
}

nsStdURL::~nsStdURL()
{
    CRTFREEIF(mScheme);
    CRTFREEIF(mUsername);
    CRTFREEIF(mPassword);
    CRTFREEIF(mHost);
    CRTFREEIF(mDirectory);
    CRTFREEIF(mFileBaseName);
    CRTFREEIF(mFileExtension);
    CRTFREEIF(mParam);
    CRTFREEIF(mQuery);
    CRTFREEIF(mRef);
}

NS_IMPL_AGGREGATED(nsStdURL);

NS_IMETHODIMP
nsStdURL::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "no instance pointer");
    if(!aInstancePtr)
        return NS_ERROR_INVALID_POINTER;

    if (aIID.Equals(NS_GET_IID(nsISupports)))
        *aInstancePtr = GetInner();
    else if (aIID.Equals(kThisStdURLImplementationCID) ||   // used by Equals
            aIID.Equals(NS_GET_IID(nsIURL)) ||
            aIID.Equals(NS_GET_IID(nsIURI)) ||
            aIID.Equals(NS_GET_IID(nsIFileURL)))
        *aInstancePtr = NS_STATIC_CAST(nsIURL*, this);
    else if (aIID.Equals(NS_GET_IID(nsIStandardURL))) {
		*aInstancePtr = NS_STATIC_CAST(nsIStandardURL*, this);
	}
    else {
        *aInstancePtr = nsnull;
        return NS_NOINTERFACE;
    }
    NS_ADDREF((nsISupports*)*aInstancePtr);
    return NS_OK;
}

NS_IMETHODIMP
nsStdURL::Equals(nsIURI *i_OtherURI, PRBool *o_Equals)
{
    PRBool eq = PR_FALSE;
    if (i_OtherURI) {
        nsStdURL* other = nsnull;
        nsresult rv = i_OtherURI->QueryInterface(kThisStdURLImplementationCID,
                                                 (void**)&other);
        if (NS_FAILED(rv)) {
            *o_Equals = eq;
            return rv;
        }
        // Maybe the directorys are different
        if (nsCRT::strcasecmp(mDirectory, other->mDirectory)==0) {
            // Or the Filebasename?
            if (nsCRT::strcasecmp(mFileBaseName, other->mFileBaseName)==0) {
                // Maybe the Fileextension?
                if (nsCRT::strcasecmp(mFileExtension, other->mFileExtension)==0) {
                    // Or the Host?
                    if (nsCRT::strcasecmp(mHost, other->mHost)==0) {
                        // Or the Scheme?
                        if (nsCRT::strcasecmp(mScheme, other->mScheme)==0) {
                            // Username?
                            if (nsCRT::strcasecmp(mUsername, other->mUsername)==0) {
                                // Password?
                                if (nsCRT::strcasecmp(mPassword, other->mPassword)==0) {
                                    // Param?
                                    if (nsCRT::strcasecmp(mParam, other->mParam)==0) {
                                        // Query?
                                        if (nsCRT::strcasecmp(mQuery, other->mQuery)==0) {
                                            // Ref?
                                            if (nsCRT::strcasecmp(mRef, other->mRef)==0) {
                                                // Port?
                                                PRInt32 myPort = mPort != -1 ? mPort : mDefaultPort;
                                                PRInt32 theirPort = other->mPort != -1 ? other->mPort : other->mDefaultPort;
                                                if (myPort == theirPort) {
                                                    // They are equal!!!!!
                                                    eq = PR_TRUE;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        NS_RELEASE(other);
    }
    *o_Equals = eq;
    return NS_OK;
}

NS_IMETHODIMP
nsStdURL::SchemeIs(PRUint32 i_Scheme, PRBool *o_Equals)
{
    NS_ENSURE_ARG_POINTER(o_Equals);
    if (i_Scheme == nsIURI::UNKNOWN)
        return NS_ERROR_INVALID_ARG;
    *o_Equals = (mSchemeType == i_Scheme);
    return NS_OK;
}

NS_IMETHODIMP
nsStdURL::Clone(nsIURI **o_URI)
{
    nsStdURL* url = new nsStdURL(*this); /// TODO check outer?
    if (url == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    nsresult rv= NS_OK;

    *o_URI = url;
    NS_ADDREF(url);
    return rv;
}

nsresult
nsStdURL::Parse(const char* i_Spec)
{
    // Main parser
    NS_PRECONDITION( (nsnull != i_Spec), "Parse called on empty url!");
    if (!i_Spec)
        return NS_ERROR_MALFORMED_URI;

    NS_PRECONDITION( (nsnull != mURLParser), "Parse called without parser!");
    if (!mURLParser) return NS_ERROR_NULL_POINTER;

    // Parse the spec
    char* ePath = nsnull;
    nsresult rv = mURLParser->ParseAtScheme(i_Spec, &mScheme, &mUsername, 
                                            &mPassword, &mHost, &mPort, 
                                            &ePath);

    if (0 == PL_strcasecmp("about", mScheme))
        mSchemeType = nsIURI::ABOUT;
    else if (0 == PL_strcasecmp("chrome", mScheme))
        mSchemeType = nsIURI::CHROME;
    else if (0 == PL_strcasecmp("file", mScheme))
        mSchemeType = nsIURI::FILE;
    else if (0 == PL_strcasecmp("ftp", mScheme))
        mSchemeType = nsIURI::FTP;
    else if (0 == PL_strcasecmp("http", mScheme))
        mSchemeType = nsIURI::HTTP;
    else if (0 == PL_strcasecmp("https", mScheme))
        mSchemeType = nsIURI::HTTPS;
    else if (0 == PL_strcasecmp("imap", mScheme))
        mSchemeType = nsIURI::IMAP;
    else if (0 == PL_strcasecmp("jar", mScheme))
        mSchemeType = nsIURI::JAR;
    else if (0 == PL_strcasecmp("javascript", mScheme))
        mSchemeType = nsIURI::JAVASCRIPT;
    else if (0 == PL_strcasecmp("mailbox", mScheme))
        mSchemeType = nsIURI::MAILBOX;
    else if (0 == PL_strcasecmp("mailto", mScheme))
        mSchemeType = nsIURI::MAILTO;
    else if (0 == PL_strcasecmp("news", mScheme))
        mSchemeType = nsIURI::NEWS;
    else if (0 == PL_strcasecmp("resource", mScheme))
        mSchemeType = nsIURI::RESOURCE;
    else
        mSchemeType = nsIURI::UNKNOWN;

    if (NS_SUCCEEDED(rv)) {
        // Now parse the path
        rv = mURLParser->ParseAtDirectory(ePath, &mDirectory, &mFileBaseName, 
                                          &mFileExtension, &mParam, 
                                          &mQuery, &mRef);
    }
    CRTFREEIF(ePath);

    return rv;
}

nsresult
nsStdURL::GetString(char** result, char* fromEscapedStr, Format toFormat)
{
    // Given str "foo%20bar", gets "foo bar" if UNESCAPED
    nsresult rv = NS_OK;
    if (toFormat == UNESCAPED) {
        rv = nsURLUnescape(fromEscapedStr, result);
    } else
        rv = DupString(result, fromEscapedStr);
    return rv;
}

nsresult
nsStdURL::AppendString(nsCString& buffer, char * fromUnescapedStr,
                       Format toFormat, PRInt16 mask)
{
    // Given str "foo bar", appends "foo%20bar" to buffer if ESCAPED
    nsresult rv = NS_OK; 

    if (!fromUnescapedStr)
        return NS_ERROR_FAILURE;

    if (toFormat == HOSTESCAPED && strchr(fromUnescapedStr, ':')) {
        buffer += "[";
        buffer += fromUnescapedStr;
        buffer += "]";
    }
    else if (toFormat != UNESCAPED) {
        rv = nsAppendURLEscapedString(buffer, fromUnescapedStr, mask);
    } else {
        buffer += fromUnescapedStr;
    }

    return rv;
}

nsresult
nsStdURL::AppendPreHost(nsCString& buffer, char* i_Username, 
                        char* i_Password, Format toFormat)
{
    nsresult rv = NS_OK;
    if (i_Username)
    {
        rv = AppendString(buffer,i_Username,ESCAPED,
                          nsIIOService::url_Username);
        if (NS_FAILED(rv))
            return rv;
        if (i_Password) 
        {
           buffer += ':';
           rv = AppendString(buffer,i_Password,ESCAPED,
                             nsIIOService::url_Password);
           if (NS_FAILED(rv))
               return rv;
        }
    }
    return rv;
}

nsresult
nsStdURL::AppendFileName(nsCString& buffer, char* i_FileBaseName, 
                         char* i_FileExtension, Format toFormat)
{
    nsresult rv = NS_OK;
    if (i_FileBaseName)
    {
        rv = AppendString(buffer,i_FileBaseName,ESCAPED,
                          nsIIOService::url_FileBaseName);
        if (NS_FAILED(rv))
            return rv;
    }
    if (i_FileExtension) 
    {
        buffer += '.';
        rv = AppendString(buffer,i_FileExtension,ESCAPED,
                          nsIIOService::url_FileExtension);
    }
    return rv;
}

nsresult
nsStdURL::GetSpec(char **o_Spec)
{
    nsresult rv = NS_OK;
    nsCAutoString finalSpec; // guaranteed to be singlebyte.
    if (mScheme)
    {
        rv = AppendString(finalSpec,mScheme,ESCAPED,nsIIOService::url_Scheme);
        finalSpec += "://";
    }

    rv = AppendPreHost(finalSpec,mUsername,mPassword,ESCAPED);
    if (mUsername)
    {
        finalSpec += "@";
    }

    if (mHost)
    {
        rv = AppendString(finalSpec,mHost,HOSTESCAPED,nsIIOService::url_Host);
        if (-1 != mPort && mPort != mDefaultPort)
        {
            char* portBuffer = PR_smprintf(":%d", mPort);
            if (!portBuffer)
                return NS_ERROR_OUT_OF_MEMORY;
            finalSpec += portBuffer;
            PR_smprintf_free(portBuffer);
            portBuffer = 0;
        }
    }
    char* ePath = nsnull;
    rv = GetPath(&ePath);
    if NS_FAILED(rv) {
        CRTFREEIF(ePath);
        return rv;
    }

    if (ePath)
    {
        finalSpec += ePath;
    }
    *o_Spec = finalSpec.ToNewCString();
    CRTFREEIF(ePath);

    return (*o_Spec ? NS_OK : NS_ERROR_OUT_OF_MEMORY);
}

nsresult
nsStdURL::GetPrePath(char **o_Spec)
{
    nsresult rv = NS_OK;
    nsCAutoString finalSpec; // guaranteed to be singlebyte.
    if (mScheme)
    {
        rv = AppendString(finalSpec,mScheme,ESCAPED,nsIIOService::url_Scheme);
        finalSpec += "://";
    }

    rv = AppendPreHost(finalSpec,mUsername,mPassword,ESCAPED);
    if (mUsername)
    {
        finalSpec += "@";
    }

    if (mHost)
    {
        rv = AppendString(finalSpec,mHost,HOSTESCAPED,nsIIOService::url_Host);
        if (-1 != mPort)
        {
            char* portBuffer = PR_smprintf(":%d", mPort);
            if (!portBuffer)
                return NS_ERROR_OUT_OF_MEMORY;
            finalSpec += portBuffer;
            PR_smprintf_free(portBuffer);
            portBuffer = 0;
        }
    }
    *o_Spec = finalSpec.ToNewCString();

    return (*o_Spec ? NS_OK : NS_ERROR_OUT_OF_MEMORY);
}

nsresult
nsStdURL::SetPrePath(const char *i_Spec)
{
    NS_NOTREACHED("nsStdURL::SetPrePath");
    return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_METHOD
nsStdURL::Create(nsISupports *aOuter, 
    REFNSIID aIID, 
    void **aResult)
{
    if (!aResult)
         return NS_ERROR_INVALID_POINTER;

    if (aOuter && !aIID.Equals(NS_GET_IID(nsISupports)))
        return NS_ERROR_INVALID_ARG;

    nsStdURL* url = new nsStdURL(aOuter);
    if (url == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = url->AggregatedQueryInterface(aIID, aResult);
    if (NS_FAILED(rv)) {
        delete url;
        return rv;
    }

    return rv;
}

NS_METHOD
nsStdURL::GetPreHost(char **o_PreHost)
{
    if (!o_PreHost)
        return NS_ERROR_NULL_POINTER;
    
    if (!mUsername) {
        *o_PreHost = nsnull;
    } else {
        nsCAutoString temp;

        nsresult rv = AppendPreHost(temp,mUsername,mPassword,ESCAPED);

        if (NS_FAILED(rv))
            return rv;

        *o_PreHost = temp.ToNewCString();
        if (!*o_PreHost)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStdURL::SetDirectory(const char* i_Directory)
{
    if (!i_Directory)
        return NS_ERROR_NULL_POINTER;
    
    if (mDirectory)
        nsCRT::free(mDirectory);

    nsCAutoString dir;
    if ('/' != *i_Directory)
        dir += "/";
    
    dir += i_Directory;

    // if the last slash is missing then attach it
    if (dir.Last() != '/') {  
        dir += "/";
    }

    CRTFREEIF(mDirectory);
    mDirectory = dir.ToNewCString();
    if (!mDirectory)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsStdURL::SetFileName(const char* i_FileName)
{
    if (!i_FileName)
        return NS_ERROR_NULL_POINTER;
    
    NS_PRECONDITION( (nsnull != mURLParser), "Parse called without parser!");
    if (!mURLParser) return NS_ERROR_NULL_POINTER;

    //If it starts with a / then everything is the path.
    if ('/' == *i_FileName) {
        return SetPath(i_FileName);
    }
 
    // Otherwise concatenate Directory and Filename and the call SetPath
    nsCAutoString dir;
    nsresult status = AppendString(dir,mDirectory,ESCAPED,
                                   nsIIOService::url_Directory);
    dir += i_FileName;
    char *eNewPath = dir.ToNewCString();
    if (!eNewPath) 
        return NS_ERROR_OUT_OF_MEMORY;
    status = SetPath(eNewPath);
    CRTFREEIF(eNewPath);
    return status;
}

NS_IMETHODIMP
nsStdURL::SetParam(const char* i_Param)
{
    CRTFREEIF(mParam);
    return DupString(&mParam, (i_Param && (*i_Param == ';')) ? 
                     (i_Param+1) : i_Param);
}

NS_IMETHODIMP
nsStdURL::SetQuery(const char* i_Query)
{
    CRTFREEIF(mQuery);
    return DupString(&mQuery, (i_Query && (*i_Query == '?')) ? 
                     (i_Query+1) : i_Query);
}

NS_IMETHODIMP
nsStdURL::SetRef(const char* i_Ref)
{
    CRTFREEIF(mRef);
    return DupString(&mRef, (i_Ref && (*i_Ref == '#')) ? 
                     (i_Ref+1) : i_Ref);
}

NS_IMETHODIMP
nsStdURL::Resolve(const char *relativePath, char **result) 
{
    nsresult rv = NS_OK;

    if (!relativePath) return NS_ERROR_NULL_POINTER;

    nsXPIDLCString scheme;
    rv = ExtractURLScheme(relativePath, nsnull, nsnull, getter_Copies(scheme));
    if (NS_SUCCEEDED(rv)) {
        // then aSpec is absolute
        rv = DupString(result, relativePath);
        char* path = PL_strstr(*result,"://");
        if (path) {
            path = PL_strstr((char*)(path+3),"/");
            if (path) 
                CoaleseDirs(path);
        }
        return rv;
    }

    nsCAutoString finalSpec; // guaranteed to be singlebyte.

    // This is another case of an almost absolute URL 
    if (*relativePath == '/' && *(relativePath+1) != '\0' && 
        *(relativePath+1) == '/') {

        if (mScheme)
        {
            rv = AppendString(finalSpec,mScheme,ESCAPED,
                              nsIIOService::url_Scheme);
            finalSpec += ":";
        }

        finalSpec += relativePath;
        *result = finalSpec.ToNewCString();
        if (*result) {
            char* path = PL_strstr(*result,"://");
            if (path) {
                path = PL_strstr((char*)(path+3),"/");
                if (path)
                    CoaleseDirs(path);
            }
            return NS_OK;
        } else
            return NS_ERROR_OUT_OF_MEMORY;
    } 

    const char *start = relativePath;

    if (mScheme)
    {
        rv = AppendString(finalSpec,mScheme,ESCAPED,nsIIOService::url_Scheme);
        finalSpec += "://";
    }

    rv = AppendPreHost(finalSpec,mUsername,mPassword,ESCAPED);
    if (mUsername)
    {
        finalSpec += '@';
    }

    if (mHost)
    {
        rv = AppendString(finalSpec,mHost,HOSTESCAPED,nsIIOService::url_Host);
        if (-1 != mPort)
        {
            char* portBuffer = PR_smprintf(":%d", mPort);
            if (!portBuffer)
                return NS_ERROR_OUT_OF_MEMORY;
            finalSpec += portBuffer;
            PR_smprintf_free(portBuffer);
            portBuffer = 0;
        }
    }

    if (start) {
      switch (*start) 
      {
        case '/':
            finalSpec += (char*)start;
            break;
        case ';': 
            rv = AppendString(finalSpec,mDirectory,ESCAPED,
                              nsIIOService::url_Directory);
            rv = AppendFileName(finalSpec,mFileBaseName,mFileExtension,
                                ESCAPED);
            finalSpec += (char*)start;
            break;
        case '?': 
            rv = AppendString(finalSpec,mDirectory,ESCAPED,
                              nsIIOService::url_Directory);
            rv = AppendFileName(finalSpec,mFileBaseName,mFileExtension,
                                ESCAPED);
            if (mParam)
            {
                finalSpec += ';';
                rv = AppendString(finalSpec,mParam,ESCAPED,
                                  nsIIOService::url_Param);
            }
            finalSpec += (char*)start;
            break;
        case '#':
            rv = AppendString(finalSpec,mDirectory,ESCAPED,
                              nsIIOService::url_Directory);
            rv = AppendFileName(finalSpec,mFileBaseName,mFileExtension,
                                ESCAPED);
            if (mParam)
            {
                finalSpec += ';';
                rv = AppendString(finalSpec,mParam,ESCAPED,
                                  nsIIOService::url_Param);
            }
            if (mQuery)
            {
                finalSpec += '?';
                rv = AppendString(finalSpec,mQuery,ESCAPED,
                                  nsIIOService::url_Query);
            }
            finalSpec += (char*)start;
            break;
        default:
            rv = AppendString(finalSpec,mDirectory,ESCAPED,
                              nsIIOService::url_Directory);
            finalSpec += (char*)start;
      }
    }
    *result = finalSpec.ToNewCString();

    if (*result) {
        char* path = PL_strstr(*result,"://");
        if (path) {
            path = PL_strstr((char*)(path+3),"/");
            if (path)
                CoaleseDirs(path);
        }
        return NS_OK;
    } else
        return NS_ERROR_OUT_OF_MEMORY;
}

nsresult
nsStdURL::GetPath(char** o_Path)
{
    //Take all the elements of the path and construct it
    nsCAutoString path;
    nsresult rv = NS_OK;
    if (mDirectory)
    {
        rv = AppendString(path,mDirectory,ESCAPED,nsIIOService::url_Directory);
        if (NS_FAILED(rv))
            return rv;
    }

    rv = AppendFileName(path,mFileBaseName,mFileExtension, ESCAPED);
    if (NS_FAILED(rv))
        return rv;

    if (mParam)
    {
        path += ';';
        rv = AppendString(path,mParam,ESCAPED,nsIIOService::url_Param);
        if (NS_FAILED(rv))
            return rv;
    }
    if (mQuery)
    {
        path += '?';
        rv = AppendString(path,mQuery,ESCAPED,nsIIOService::url_Query);
        if (NS_FAILED(rv))
            return rv;
    }
    if (mRef)
    {
        path += '#';
        rv = AppendString(path,mRef,ESCAPED,nsIIOService::url_Ref);
        if (NS_FAILED(rv))
            return rv;
    }
    *o_Path = path.ToNewCString();
    return (*o_Path ? NS_OK : NS_ERROR_OUT_OF_MEMORY);
}

nsresult
nsStdURL::GetDirectory(char** o_Directory)
{
    nsCAutoString directory;
    nsresult rv = NS_OK;
    rv = AppendString(directory,mDirectory,ESCAPED,
                      nsIIOService::url_Directory);
    if (NS_FAILED(rv))
        return rv;
    *o_Directory = directory.ToNewCString();
    return (*o_Directory ? NS_OK : NS_ERROR_OUT_OF_MEMORY);
}

NS_METHOD
nsStdURL::SetSpec(const char* i_Spec)
{
    char* eSpec = nsnull;
    nsresult rv = DupString(&eSpec,i_Spec);
    if (NS_FAILED(rv)){
        CRTFREEIF(eSpec);
        return rv;
    }

    // Skip leading spaces and control-characters
    char* fwdPtr= (char*) eSpec;
    while (fwdPtr && (*fwdPtr > '\0') && (*fwdPtr <= ' '))
        fwdPtr++;

    // Strip linebreaks
    char* copyToPtr = 0;
    char* midPtr = fwdPtr;
    while (midPtr && (*midPtr != '\0')) {
        while ((*midPtr == '\r') || (*midPtr == '\n')) { // if linebreak
            if (!copyToPtr)
                copyToPtr = midPtr; // start copying
            midPtr++; // skip linebreak
        }
        if (copyToPtr) { // if copying
            *copyToPtr = *midPtr;
            copyToPtr++;
        }
        midPtr++;
    }
    if (copyToPtr) { // If we removed linebreaks, copyToPtr is the end of the string
        midPtr = copyToPtr;
    }

    // Remove trailing spaces and control-characters
    while ((midPtr-fwdPtr) >= 0) {
        midPtr--;
        if ((*midPtr > ' ') || (*midPtr <= '\0'))  // UTF-8 chars < 0?
            break;
    }
    if (midPtr && (*midPtr != '\0'))
        *(midPtr+1) = '\0'; // Restore trailing null

    // If spec is being rewritten clean up everything-
    CRTFREEIF(mScheme);
    CRTFREEIF(mUsername);
    CRTFREEIF(mPassword);
    CRTFREEIF(mHost);
    mPort = -1;
    CRTFREEIF(mDirectory);
    CRTFREEIF(mFileBaseName);
    CRTFREEIF(mFileExtension);
    CRTFREEIF(mParam);
    CRTFREEIF(mQuery);
    CRTFREEIF(mRef);
    rv = Parse(fwdPtr);
    CRTFREEIF(eSpec);
    return rv;
}

NS_METHOD
nsStdURL::SetPreHost(const char* i_PreHost)
{
    NS_PRECONDITION( (nsnull != mURLParser), "Parse called without parser!");
    if (!mURLParser) return NS_ERROR_NULL_POINTER;

    CRTFREEIF(mUsername);
    CRTFREEIF(mPassword);

    return mURLParser->ParsePreHost(i_PreHost,&mUsername,&mPassword);
}

NS_METHOD
nsStdURL::SetPath(const char* i_Path)
{
    NS_PRECONDITION( (nsnull != mURLParser), "Parse called without parser!");
    if (!mURLParser) return NS_ERROR_NULL_POINTER;

    CRTFREEIF(mDirectory);
    CRTFREEIF(mFileBaseName);
    CRTFREEIF(mFileExtension);
    CRTFREEIF(mParam);
    CRTFREEIF(mQuery);
    CRTFREEIF(mRef);

    return mURLParser->ParseAtDirectory((char*)i_Path, &mDirectory, 
                                        &mFileBaseName, 
                                        &mFileExtension, &mParam, 
                                        &mQuery, &mRef);
}

NS_METHOD
nsStdURL::GetFilePath(char **o_DirFile)
{
    if (!o_DirFile)
        return NS_ERROR_NULL_POINTER;

    nsresult rv = NS_OK;
    nsCAutoString temp;
    if (mDirectory)
    {
        rv = AppendString(temp,mDirectory,ESCAPED,nsIIOService::url_Directory);
    }

    rv = AppendFileName(temp,mFileBaseName,mFileExtension,ESCAPED);
    if (NS_FAILED(rv))
        return rv;

    *o_DirFile = temp.ToNewCString();
    if (!*o_DirFile)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_METHOD
nsStdURL::GetFileName(char **o_FileName)
{
    if (!o_FileName)
        return NS_ERROR_NULL_POINTER;

    nsresult rv = NS_OK;    
    if (mFileBaseName || mFileExtension) {
        nsCAutoString temp;

        rv = AppendFileName(temp,mFileBaseName,mFileExtension,ESCAPED);
        if (NS_FAILED(rv))
            return rv;

        *o_FileName = temp.ToNewCString();
        if (!*o_FileName)
            return NS_ERROR_OUT_OF_MEMORY;
    } else {
        *o_FileName = nsnull;
    }
    return NS_OK;
}

NS_METHOD
nsStdURL::SetFilePath(const char *filePath)
{
    return SetPath(filePath);
}

NS_IMETHODIMP
nsStdURL::GetFile(nsIFile * *aFile)
{
    nsresult rv = NS_OK;

    if (nsCRT::strcasecmp(mScheme, "file") != 0) {
        // if this isn't a file: URL, then we can't return an nsIFile
        return NS_ERROR_FAILURE;
    }

    NS_ASSERTION(mUsername == nsnull, "file: with mUsername");

	// we do not use the path cause it can contain the # char
    nsCAutoString path;
    if (mDirectory)
    {
        rv = AppendString(path,mDirectory,ESCAPED,nsIIOService::url_Directory);
#if defined( XP_MAC )
        // Now Swap the / and colons to convert back to a mac path
        // Do this only on the mDirectory portion - not mFileBaseName or mFileExtension
        SwapSlashColon( (char*)path.GetBuffer() );
#endif
    }

    rv = AppendFileName(path,mFileBaseName,mFileExtension,ESCAPED);

#ifdef XP_PC
    if (path.CharAt(2) == '|')
        path.SetCharAt(':', 2);

    // cut off the leading '/'
    if (path.CharAt(0) == '/')
        path.Cut(0, 1);
    else if (mHost) {
        // Deal with the case where the user type file://C|/ (2 slashes instead of 3).
        // In this case, the C| ends up as the host name (ugh).
        nsCAutoString host(mHost);
        PRInt32 len = host.Length();
        if (len == 2 && host.CharAt(1) == '|') {
             host.SetCharAt(':', 1);
             path.Insert(host, 0);
         }
    }
    path.ReplaceChar('/', '\\');
#endif

    nsUnescape((char*)path.GetBuffer());
#if defined( XP_MAC )
    // wack off leading :'s
    if (path.CharAt(0) == ':')
        path.Cut(0, 1);
#endif
    nsCOMPtr<nsILocalFile> localFile;
    rv = NS_NewLocalFile(path, PR_FALSE, getter_AddRefs(localFile));

    mFile = localFile;
	*aFile = mFile;
	NS_IF_ADDREF(*aFile);
    return rv;
}

NS_IMETHODIMP
nsStdURL::SetFile(nsIFile * aFile)
{
    nsresult rv;

    mFile = aFile;

    // set up this URL to denote the nsIFile
    SetScheme("file");
    CRTFREEIF(mUsername);
    CRTFREEIF(mPassword);
    CRTFREEIF(mHost);
    mPort = -1;

    char* ePath = nsnull;
    nsCAutoString escPath;

    rv = aFile->GetPath(&ePath);
    if (NS_SUCCEEDED(rv)) {
#if defined (XP_PC)
        // Replace \ with / to convert to an url
        char* s = ePath;
	    while (*s)
	    {
#ifndef XP_OS2
            // We need to call IsDBCSLeadByte because
            // Japanese windows can have 0x5C in the sencond byte 
            // of a Japanese character, for example 0x8F 0x5C is
            // one Japanese character
            if(::IsDBCSLeadByte(*s) && *(s+1) != nsnull) {
                s++;
		    } else 
#endif
                if (*s == '\\')
                    *s = '/';
		    s++;
	    }
#endif
#if defined( XP_MAC )
   	    // Swap the / and colons to convert to an url
        SwapSlashColon(ePath);
#endif
        // Escape the path with the directory mask
        rv = nsURLEscape(ePath,nsIIOService::url_Directory+
                         nsIIOService::url_Forced,escPath);
        if (NS_SUCCEEDED(rv)) {
            PRBool dir;
            rv = aFile->IsDirectory(&dir);
            if (NS_SUCCEEDED(rv) && dir && escPath[escPath.Length() - 1] != '/') {
                // make sure we have a trailing slash
                escPath += "/";
            }
            rv = SetPath(escPath.GetBuffer());
        }
    }
    CRTFREEIF(ePath);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
