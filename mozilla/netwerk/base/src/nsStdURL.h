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

#ifndef nsStdURL_h__
#define nsStdURL_h__

#include "nsIURL.h"
#include "nsIURLParser.h"
#include "nsURLHelper.h"
#include "nsAgg.h"
#include "nsCRT.h"
#include "nsString.h" // REMOVE Later!!
#include "nsCOMPtr.h"
#include "nsIFile.h"

#define NS_THIS_STANDARDURL_IMPLEMENTATION_CID       \
{ /* e3939dc8-29ab-11d3-8cce-0060b0fc14a3 */         \
    0xe3939dc8,                                      \
    0x29ab,                                          \
    0x11d3,                                          \
    {0x8c, 0xce, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

class nsStdURL : public nsIFileURL
{
public:
    ///////////////////////////////////////////////////////////////////////////
    // nsStdURL methods:

    nsStdURL();
    nsStdURL(const char* i_Spec, nsISupports* outer=nsnull);
    nsStdURL(const nsStdURL& i_URL); 
    virtual ~nsStdURL();

    nsStdURL&   operator =(const nsStdURL& otherURL); 
    PRBool      operator ==(const nsStdURL& otherURL) const;

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    NS_DECL_AGGREGATED
    NS_DECL_NSIURI
    NS_DECL_NSIURL
    NS_DECL_NSIFILEURL

protected:
    enum Format { ESCAPED, UNESCAPED };
    nsresult Parse(const char* i_Spec);
    nsresult AppendString(nsCString& buffer, char* fromUnescapedStr, 
                          Format toFormat, PRInt16 mask);
    nsresult GetString(char** result, char* fromEscapedStr, 
                       Format toFormat);
    nsresult AppendPreHost(nsCString& buffer, char* i_Username, 
                           char* i_Password, Format toFormat);
    nsresult AppendFileName(nsCString& buffer, char* i_FileBaseName, 
                           char* i_FileExtension, Format toFormat);

protected:

    char*       mScheme;
    char*       mUsername;
    char*       mPassword;
    char*       mHost;
    PRInt32     mPort;

    char*       mDirectory;
    char*       mFileBaseName;
    char*       mFileExtension;
    char*       mParam;
    char*       mQuery;
    char*       mRef;

    nsCOMPtr<nsIURLParser> mURLParser;

    // If a file was given to SetFile, then this instance variable holds it.
    // If GetFile is called, we synthesize one and cache it here.
    nsCOMPtr<nsIFile>   mFile;
};

inline NS_METHOD
nsStdURL::GetScheme(char* *o_Scheme)
{
    return GetString(o_Scheme, mScheme, UNESCAPED);
}

inline NS_METHOD
nsStdURL::GetUsername(char* *o_Username)
{
    return GetString(o_Username, mUsername, UNESCAPED);
}

inline NS_METHOD
nsStdURL::GetPassword(char* *o_Password)
{
    return GetString(o_Password, mPassword, UNESCAPED);
}

inline NS_METHOD
nsStdURL::GetHost(char* *o_Host)
{
    return GetString(o_Host, mHost, UNESCAPED);
}

inline NS_METHOD
nsStdURL::GetPort(PRInt32 *aPort)
{
    if (aPort)
    {
        *aPort = mPort;
        return NS_OK;
    }
    return NS_ERROR_NULL_POINTER;
}

inline NS_METHOD
nsStdURL::GetFileBaseName(char* *o_FileBaseName)
{
    return GetString(o_FileBaseName, mFileBaseName, UNESCAPED);
}

inline NS_METHOD
nsStdURL::GetFileExtension(char* *o_FileExtension)
{
    return GetString(o_FileExtension, mFileExtension, UNESCAPED);
}

inline NS_METHOD
nsStdURL::GetParam(char **o_Param)
{
    return GetString(o_Param, mParam, UNESCAPED);
}

inline NS_METHOD
nsStdURL::GetQuery(char* *o_Query)
{
    return GetString(o_Query, mQuery, UNESCAPED);
}

inline NS_METHOD
nsStdURL::GetRef(char* *o_Ref)
{
    return GetString(o_Ref, mRef, UNESCAPED);
}

inline NS_METHOD
nsStdURL::SetScheme(const char* i_Scheme)
{
    CRTFREEIF(mScheme);
    nsresult rv = DupString(&mScheme, i_Scheme);
    ToLowerCase(mScheme);
    return rv;
}

inline NS_METHOD
nsStdURL::SetUsername(const char* i_Username)
{
    CRTFREEIF(mUsername);
    return DupString(&mUsername, i_Username);
}

inline NS_METHOD
nsStdURL::SetPassword(const char* i_Password)
{
    CRTFREEIF(mPassword);
    return DupString(&mPassword, i_Password);
}

inline NS_METHOD
nsStdURL::SetHost(const char* i_Host)
{
    CRTFREEIF(mHost);
    nsresult rv = DupString(&mHost, i_Host);
    ToLowerCase(mHost);
    return rv;
}

inline NS_METHOD
nsStdURL::SetPort(PRInt32 aPort)
{
    mPort = aPort;
    return NS_OK;
}

inline NS_METHOD
nsStdURL::SetFileBaseName(const char* i_FileBaseName)
{
    CRTFREEIF(mFileBaseName);
    return DupString(&mFileBaseName, i_FileBaseName);
}

inline NS_METHOD
nsStdURL::SetFileExtension(const char* i_FileExtension)
{
    CRTFREEIF(mFileExtension);
    return DupString(&mFileExtension, i_FileExtension);
}

#endif // nsStdURL_h__

