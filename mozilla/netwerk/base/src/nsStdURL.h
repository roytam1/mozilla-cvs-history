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
#include "nsAgg.h"
#include "nsCRT.h"
#include "nsString.h" // REMOVE Later!!

#define NS_THIS_STANDARDURL_IMPLEMENTATION_CID       \
{ /* e3939dc8-29ab-11d3-8cce-0060b0fc14a3 */         \
    0xe3939dc8,                                      \
    0x29ab,                                          \
    0x11d3,                                          \
    {0x8c, 0xce, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

class nsStdURL : public nsIURL
{
public:
    ////////////////////////////////////////////////////////////////////////////
    // nsStdURL methods:

    nsStdURL(const char* i_Spec, nsISupports* outer=nsnull);
    nsStdURL(const nsStdURL& i_URL); 
    virtual ~nsStdURL();

    nsStdURL&   operator =(const nsStdURL& otherURL); 
    PRBool      operator ==(const nsStdURL& otherURL) const;

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    NS_DECL_AGGREGATED

    ////////////////////////////////////////////////////////////////////////////
    // nsIURI methods:
    NS_DECL_NSIURI

    ////////////////////////////////////////////////////////////////////////////
    // nsIURL methods:
    NS_DECL_NSIURL

    /* todo move this to protected later */
    nsresult ParsePath(void);

protected:
    nsresult Parse(void);
    nsresult ReconstructPath(void);
    nsresult ReconstructSpec(void);

    // Some handy functions 
    nsresult DupString(char* *o_Dest, const char* i_Src);
    nsresult ExtractString(char* i_Src, char* *o_Dest, PRUint32 length);
protected:

    char*       mScheme;
    char*       mPreHost;
    char*       mHost;
    PRInt32     mPort;
    char*       mPath;

    char*       mDirectory;
    char*       mFileName;
    char*       mParam;
    char*       mQuery;
    char*       mRef;

    char*       mSpec; 

};

inline NS_METHOD
nsStdURL::GetSpec(char* *o_Spec)
{
    return DupString(o_Spec, mSpec);
}

inline NS_METHOD
nsStdURL::GetScheme(char* *o_Scheme)
{
    return DupString(o_Scheme, mScheme);
}

inline NS_METHOD
nsStdURL::GetPreHost(char* *o_PreHost)
{
    return DupString(o_PreHost, mPreHost);
}

inline NS_METHOD
nsStdURL::GetHost(char* *o_Host)
{
    return DupString(o_Host, mHost);
}

inline NS_METHOD
nsStdURL::GetPath(char* *o_Path)
{
    return DupString(o_Path, mPath);
}

inline NS_METHOD
nsStdURL::GetDirectory(char* *o_Directory)
{
    return DupString(o_Directory, mDirectory);
}

inline NS_METHOD
nsStdURL::GetFileName(char* *o_FileName)
{
    return DupString(o_FileName, mFileName);
}

inline NS_METHOD
nsStdURL::GetFileExtension(char* *o_FileExtension)
{
    if (!o_FileExtension)
        return NS_ERROR_NULL_POINTER;

    char *dot = mFileName;
    if (dot) {
        // find the last dot
        while (*dot) dot++;
        while ( (dot != mFileName) && (*dot != '.') ) dot--; // goto the dot.
        if (*dot == '.') {
            nsCAutoString ext(dot+1);
            *o_FileExtension = ext.ToNewCString();
            if (!*o_FileExtension) return NS_ERROR_OUT_OF_MEMORY;
        } else {
            *o_FileExtension = nsnull;
        }
    }
    return NS_OK;
}

inline NS_METHOD
nsStdURL::SetFileExtension(const char* FileExtension)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

inline NS_METHOD
nsStdURL::GetRef(char* *o_Ref)
{
    return DupString(o_Ref, mRef);
}

inline NS_METHOD
nsStdURL::GetParam(char **o_Param)
{
    return DupString(o_Param, mParam);
}

inline NS_METHOD
nsStdURL::GetQuery(char* *o_Query)
{
    return DupString(o_Query, mQuery);
}

inline NS_METHOD
nsStdURL::SetScheme(const char* i_Scheme)
{
    if (mScheme) nsCRT::free(mScheme);
    nsresult status = DupString(&mScheme, i_Scheme);
    ReconstructSpec();
    return status;
}

inline NS_METHOD
nsStdURL::SetPreHost(const char* i_PreHost)
{
    if (mPreHost) nsCRT::free(mPreHost);
    nsresult status = DupString(&mPreHost, i_PreHost);
    ReconstructSpec();
    return status;
}

inline NS_METHOD
nsStdURL::SetHost(const char* i_Host)
{
    if (mHost) nsCRT::free(mHost);
    nsresult status = DupString(&mHost, i_Host);
    ReconstructSpec();
    return status;
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
nsStdURL::SetPort(PRInt32 aPort)
{
    mPort = aPort;
    ReconstructSpec();
    return NS_OK;
}

#endif // nsStdURL_h__
