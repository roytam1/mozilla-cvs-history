/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Darin Fisher <darin@netscape.com> (original author)
 */

#ifndef nsHttpResponseHead_h__
#define nsHttpResponseHead_h__

#include "nsHttpHeaderArray.h"
#include "nsHttp.h"
#include "nsXPIDLString.h"

//-----------------------------------------------------------------------------
// nsHttpResponseHead represents the status line and headers from an HTTP
// response.
//-----------------------------------------------------------------------------

class nsHttpResponseHead
{
public:
    nsHttpResponseHead() : mStatus(200), mContentLength(-1) {}
   ~nsHttpResponseHead() {}
    
    nsHttpHeaderArray &Headers()        { return mHeaders; }
    nsHttpVersion      Version()        { return mVersion; }
    PRUint32           Status()         { return mStatus; }
    const char        *StatusText()     { return mStatusText; }
    PRInt32            ContentLength()  { return mContentLength; }
    const char        *ContentType()    { return mContentType; }
    const char        *ContentCharset() { return mContentCharset; }

    const char *PeekHeader(nsHttpAtom h)            { return mHeaders.PeekHeader(h); }
    nsresult SetHeader(nsHttpAtom h, const char *v) { return mHeaders.SetHeader(h, v); }
    nsresult GetHeader(nsHttpAtom h, char **v)      { return mHeaders.GetHeader(h, v); }

    nsresult Flatten(nsACString &);

    // parse flattened response head. block must be null terminated. parsing is
    // destructive.
    nsresult Parse(char *block);

    // parse the status line. line must be null terminated.
    nsresult ParseStatusLine(char *line);

    // parse a header line. line must be null terminated. parsing is destructive.
    nsresult ParseHeaderLine(char *line);

    // cache validation support methods
    nsresult ComputeFreshnessLifetime(PRUint32 *);
    nsresult ComputeCurrentAge(PRUint32 now, PRUint32 requestTime, PRUint32 *result);

    // update headers...
    nsresult UpdateHeaders(nsHttpHeaderArray &headers); 

private:
    nsresult ParseVersion(const char *);
    nsresult ParseContentType(char *);

    // these return failure if the header does not exist.
    nsresult ParseDateHeader(nsHttpAtom header, PRUint32 *result);
    nsresult GetAgeValue(PRUint32 *result);
    nsresult GetMaxAgeValue(PRUint32 *result);
    nsresult GetDateValue(PRUint32 *result)         { return ParseDateHeader(nsHttp::Date, result); }
    nsresult GetExpiresValue(PRUint32 *result)      { return ParseDateHeader(nsHttp::Expires, result); }
    nsresult GetLastModifiedValue(PRUint32 *result) { return ParseDateHeader(nsHttp::Last_Modified, result); }

private:
    nsHttpHeaderArray mHeaders;
    nsHttpVersion     mVersion;
    PRUint32          mStatus;
    nsXPIDLCString    mStatusText;
    PRInt32           mContentLength;
    nsXPIDLCString    mContentType;
    nsXPIDLCString    mContentCharset;
};

#endif // nsHttpResponseHead_h__
