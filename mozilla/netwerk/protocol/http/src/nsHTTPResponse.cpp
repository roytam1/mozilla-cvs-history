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
 * Contributor(s): 
 */

#include "prlog.h"
#include "prprf.h"
#include "nsHTTPResponse.h"
#include "nsIInputStream.h"
#include "nsIURL.h"
#include "nsIHTTPChannel.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsHTTPAtoms.h"

#if defined(PR_LOGGING)
extern PRLogModuleInfo* gHTTPLog;
#endif /* PR_LOGGING */

// When deciding heuristically whether or not to validate an HTTP response with
// the server, this constant can be used to tune how conservative the algorithm
// is.  For example, when set to 0.10, a document is considered stale when the
// document's age is one tenth of the time between when the document was last
// modified and when it was served.
#define HEURISTIC_STALENESS_FACTOR 0.10

nsHTTPResponse::nsHTTPResponse()
{
    NS_INIT_REFCNT();

    mStatus = 0;
    mServerVersion = HTTP_UNKNOWN;

    // The content length is unknown...
    mContentLength = -1;
}

nsHTTPResponse::~nsHTTPResponse()
{
}

NS_IMPL_ISUPPORTS(nsHTTPResponse, NS_GET_IID(nsISupports))


nsresult nsHTTPResponse::GetCharset(char* *o_Charset)
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(o_Charset);

    // Check if status header has been parsed yet
    if (mCharset.Length() == 0)
        return NS_ERROR_NOT_AVAILABLE;

    *o_Charset = mCharset.ToNewCString();
    if (!*o_Charset)
        rv = NS_ERROR_OUT_OF_MEMORY;

    return rv;
}

nsresult nsHTTPResponse::SetCharset(const char* i_Charset)
{
    mCharset = i_Charset;
    return NS_OK;
}

nsresult nsHTTPResponse::GetContentType(char* *o_ContentType)
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(o_ContentType);

    // Check if status header has been parsed yet
    if (mContentType.Length() == 0)
        return NS_ERROR_NOT_AVAILABLE;

    *o_ContentType = mContentType.ToNewCString();
    if (!*o_ContentType)
        rv = NS_ERROR_OUT_OF_MEMORY;

    return rv;
}

nsresult nsHTTPResponse::SetContentType(const char* i_ContentType)
{
    nsCAutoString cType(i_ContentType);
    cType.ToLowerCase();
    mContentType = cType.GetBuffer();
    return NS_OK;
}

nsresult nsHTTPResponse::GetContentLength(PRInt32* o_ContentLength)
{
    NS_ENSURE_ARG_POINTER(o_ContentLength);

    // Check if content-length header was received yet
    if (mContentLength == -1)
        return NS_ERROR_NOT_AVAILABLE;

    *o_ContentLength = mContentLength;
    return NS_OK;
}
    
nsresult nsHTTPResponse::SetContentLength(PRInt32 i_ContentLength)
{
    mContentLength = i_ContentLength;
    return NS_OK;
}

nsresult nsHTTPResponse::GetStatus(PRUint32* o_Value)
{
    nsresult rv = NS_OK;

    if (o_Value) {
        *o_Value = mStatus;
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

nsresult nsHTTPResponse::GetStatusString(char* *o_String)
{
    nsresult rv = NS_OK;

    if (o_String) {
        *o_String = mStatusString.ToNewCString();
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

nsresult nsHTTPResponse::GetServer(char* *o_String)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

// Finally our own methods...

nsresult nsHTTPResponse::SetHeader(nsIAtom* i_Header, const char* i_Value)
{
    return mHeaders.SetHeader(i_Header, i_Value);
}


nsresult nsHTTPResponse::GetHeader(nsIAtom* i_Header, char* *o_Value)
{
    return mHeaders.GetHeader(i_Header, o_Value);
}

nsresult nsHTTPResponse::SetServerVersion(const char* i_Version)
{
  nsresult rv = NS_OK;

  PRInt32 err, offset;
  float version;
  nsCString str(i_Version);

  // Make sure the version string is prefixed by 'HTTP/'
  offset = str.Find("HTTP/");

  // Malformed Version string - Not prefixed by 'HTTP/'.
  if (0 != offset) {
    mServerVersion = HTTP_UNKNOWN;
    return NS_ERROR_FAILURE;
  }

  // Chop off the leading 'HTTP/'
  str.Cut(0, 5);
  version = str.ToFloat(&err);

  // Malformed Version string - Version number not a number!
  if ((PRInt32)NS_OK != err) {
    mServerVersion = HTTP_UNKNOWN;
    return NS_ERROR_FAILURE;
  }

  // At least HTTP/1.1
  if (version >= 1.1f) {
    mServerVersion = HTTP_ONE_ONE;
  } 
  //
  // At least HTTP/1.0.  Some 1.0 servers actually send 0.0 as their
  // version :-(
  //
  else if ((version >= 1.0f) || (version == 0.0f)) {
    mServerVersion = HTTP_ONE_ZERO;
  }
  //
  // HTTP/0.9.  In this case, the version string has been supplied internally
  // since 0.9 responses do not contain a status line!
  //
  else if (version == 0.9f) {
    mServerVersion = HTTP_ZERO_NINE;
  }
  // No idea what the version is.  It must be malformed!
  else {
    mServerVersion = HTTP_UNKNOWN;
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

nsresult nsHTTPResponse::GetServerVersion(HTTPVersion* aResult)
{
  *aResult = mServerVersion;

  return NS_OK;
}


nsresult nsHTTPResponse::SetStatusString(const char* i_Status)
{
    nsresult rv = NS_OK;

    NS_ASSERTION(mStatusString.Length() == 0, "Overwriting status string!");
    mStatusString = i_Status;

    return rv;
}

nsresult nsHTTPResponse::GetHeaderEnumerator(nsISimpleEnumerator** aResult)
{
    return mHeaders.GetEnumerator(aResult);
}

nsresult nsHTTPResponse::ParseStatusLine(nsCString& aStatusLine)
{
    //
    // The Status Line has the following: format:
    //    HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    //
    nsresult rv;
    const char *token;
    nsCAutoString str;
    PRInt32 offset, error;

    //
    // Parse the HTTP-Version:: "HTTP" "/" 1*DIGIT "." 1*DIGIT
    //

    offset = aStatusLine.FindChar(' ');
    (void) aStatusLine.Left(str, offset);

    if (str.Length() == 0) {
      //
      // This is a HTTP/0.9 response...
      // Pretend that the status line and headers have been consumed.
      //
      rv = SetServerVersion("HTTP/0.9");
      return rv;
    } 

    token = str.GetBuffer();
    rv = SetServerVersion(token);
    if (NS_FAILED(rv)) return rv;

    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("\tParseStatusLine [this=%x].\tHTTP-Version: %s\n",
            this, token));

    aStatusLine.Cut(0, offset+1);

    //
    // Parse the Status-Code:: 3DIGIT
    //
    PRInt32 statusCode;

    offset = aStatusLine.FindChar(' ');
    (void) aStatusLine.Left(str, offset);
    if (3 != str.Length()) {
        // The status line is bogus...
        return NS_ERROR_FAILURE;
    }

    statusCode = str.ToInteger(&error);
    if (NS_FAILED(error)) return NS_ERROR_FAILURE;

    SetStatus(statusCode);

    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("\tParseStatusLine [this=%x].\tStatus-Code: %d\n",
            this, statusCode));

    aStatusLine.Cut(0, offset+1);

    //
    // Parse the Reason-Phrase:: *<TEXT excluding CR,LF>
    //
    token = aStatusLine.GetBuffer();
    SetStatusString(token);

    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("\tParseStatusLine [this=%x].\tReason-Phrase: %s\n",
            this, token));

    aStatusLine.Truncate();
  
    return NS_OK;
}

nsresult nsHTTPResponse::ParseHeader(nsCString& aHeaderString)
{
    nsresult rv;

    //
    // Extract the key field - everything up to the ':'
    // The header name is case-insensitive...
    //
    PRInt32 colonOffset;
    nsCAutoString headerKey;
    nsCOMPtr<nsIAtom> headerAtom;

    colonOffset = aHeaderString.FindChar(':');
    if (kNotFound == colonOffset) {
        //
        // The header is malformed... Just clear it.
        //
        aHeaderString.Truncate();
        return NS_ERROR_FAILURE;
    }
    (void) aHeaderString.Left(headerKey, colonOffset);
    headerKey.ToLowerCase();
    //
    // Extract the value field - everything past the ':'
    // Trim any leading or trailing whitespace...
    //
    aHeaderString.Cut(0, colonOffset+1);
    aHeaderString.Trim(" ");

    headerAtom = NS_NewAtom(headerKey.GetBuffer());
    if (headerAtom) {
        rv = ProcessHeader(headerAtom, aHeaderString);
    } else {
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    aHeaderString.Truncate();

    return rv;
}

nsresult nsHTTPResponse::ProcessHeader(nsIAtom* aHeader, nsCString& aValue)
{
    nsresult rv;

    //
    // When the Content-Type response header is processed, the Content-Type
    // and Charset information must be set into the nsHTTPChannel...
    //
    if (nsHTTPAtoms::Content_Type == aHeader) {
        nsCAutoString buffer;
        PRInt32 semicolon;

        // Set the content-type in the HTTPChannel...
        semicolon = aValue.FindChar(';');
        if (kNotFound != semicolon) {
            aValue.Left(buffer, semicolon);
            SetContentType(buffer.GetBuffer());

            // Does the Content-Type contain a charset attribute?
            aValue.Mid(buffer, semicolon+1, -1);
            buffer.Trim(" ");
            if (0 == buffer.Find("charset=", PR_TRUE)) {
                //
                // Set the charset in the HTTPChannel...
                //
                // XXX: Currently, the charset is *everything* past the "charset="
                //      This includes comments :-(
                //
                buffer.Cut(0, 8);
                SetCharset(buffer.GetBuffer());
            }
        } 
        else {
            SetContentType(aValue.GetBuffer());
        }
    }
    //
    // When the Content-Length response header is processed, set the
    // ContentLength in the response ...
    //
    else if (nsHTTPAtoms::Content_Length == aHeader) {
        PRInt32 length, status;

        length = aValue.ToInteger(&status);
        rv = (nsresult)status;

        if (NS_SUCCEEDED(rv)) {
            SetContentLength(length);
        }
    }

    //
    // Set the response header...
    //
    rv = SetHeader(aHeader, aValue.GetBuffer());

    return rv;
}

// Convert PRTime to unix-style time_t, i.e. seconds since the epoch
static PRUint32
convertPRTimeToSeconds(PRTime aTime64)
{
    double fpTime;
    LL_L2D(fpTime, aTime64);
    return (PRUint32)(fpTime * 1e-6 + 0.5);
}

// Parse an http header which has a value that is a date string,
// return the result as a PRTime
nsresult nsHTTPResponse::ParseDateHeader(nsIAtom *aAtom,
                                         PRTime *aResultTime,
                                         PRBool *aHeaderIsPresent)
{
    *aHeaderIsPresent = PR_FALSE;

    nsXPIDLCString header;
    GetHeader(aAtom, getter_Copies(header));

    if (!header)
        return NS_OK;
    *aHeaderIsPresent = PR_TRUE;

    PRStatus status;
    status = PR_ParseTimeString((const char*)header, PR_TRUE, aResultTime);
    if (status != PR_SUCCESS)
        return NS_ERROR_FAILURE;
    
    return NS_OK;
}

// Parse an http header which has a value that is a date string,
// return the result as a unix-style time_t, i.e. seconds since the epoch
nsresult nsHTTPResponse::ParseDateHeader(nsIAtom *aAtom,
                                         PRUint32 *aResultTime,
                                         PRBool *aHeaderIsPresent)
{
    nsresult rv;
    PRTime time64;

    rv = ParseDateHeader(aAtom, &time64, aHeaderIsPresent);
    if (NS_FAILED(rv)) return rv;
    *aResultTime = convertPRTimeToSeconds(time64);

    return NS_OK;
}

// Return the value of the (HTTP 1.1) max-age directive, which itself is a
// component of the Cache-Control response header
nsresult nsHTTPResponse::GetMaxAge(PRUint32* aMaxAge, PRBool* aMaxAgeIsPresent)
{
    *aMaxAgeIsPresent = PR_FALSE;

    char *cacheControlHeader;
    GetHeader(nsHTTPAtoms::Cache_Control, &cacheControlHeader);
    if (!cacheControlHeader)
        return NS_OK;

    nsCAutoString header(cacheControlHeader);
    nsAllocator::Free(cacheControlHeader);
    
    PRInt32 offset;
    offset = header.Find("max-age=", PR_TRUE);
    if (offset == kNotFound)
        return NS_OK;

    *aMaxAge = (PRUint32)atol(header.GetBuffer() + offset + 8);
    *aMaxAgeIsPresent = PR_TRUE;
    return NS_OK;
}


// Check to see if a (cached) HTTP response is stale and, therefore,
// must be revalidated with the origin server.
//
// Staleness can occur for one of several reasons:
//    + The response appears older than permitted by either the Expires
//      response header or the (HTTP 1.1) max-age response directive.
//    + The (HTTP 1.1) no-cache directive appears in the response headers
//    + The cached response is heuristically stale
//    + The response headers appear corrupted or malformed
//
PRBool nsHTTPResponse::IsStale(PRBool aUseHeuristicExpiration)
{
    nsresult rv;

    // Check for the no-cache directive within the Cache-Control response
    // header. Weirdly enough, the presence of the no-cache directive does not
    // indicate that the HTTP response can't be cached.  Rather, it means that
    // the response must always be revalidated with the origin server.  See
    // section 14.9.1 of RFC2616.
    //
    char *cacheControlHeader;
    GetHeader(nsHTTPAtoms::Cache_Control, &cacheControlHeader);
    if (cacheControlHeader) {
        nsCAutoString header(cacheControlHeader);
        nsAllocator::Free(cacheControlHeader);
        if (header.Find("no-cache", PR_TRUE) != kNotFound)
            return PR_TRUE;
    }

    // Get the value of the 'Date:' header
    PRUint32 date;
    PRBool dateHeaderIsPresent;
    rv = ParseDateHeader(nsHTTPAtoms::Date, &date, &dateHeaderIsPresent);

    // Check for corrupted, missing or malformed 'Date:' header
    if (NS_FAILED(rv) || !dateHeaderIsPresent || !date)
        return PR_TRUE;

    // Get the value of the 'max-age' directive from the 'Cache-Control:' header
    PRUint32 maxAge;
    PRBool maxAgeIsPresent;
    rv = GetMaxAge(&maxAge, &maxAgeIsPresent);
    if (NS_FAILED(rv)) return PR_TRUE; // Corrupted or malformed headers ?

    // The below code calculates the age of the response, i.e. the number of
    // seconds that has elapsed since the document was supplied by the origin
    // server.  The HTTP 1.1 spec provides a somewhat more pessimistic
    // computation that makes use of the 'Age:' header, if it is present, thus
    // potentially reducing the effects of clock skew if all proxies on the
    // path from the origin server to the client are HTTP1.1-compliant.
    //
    // However, I've used the simpler and more traditional formula: 
    //   age = current_time - date_header
    //

    PRUint32 now = convertPRTimeToSeconds(PR_Now());
    PRUint32 currentAge;

    // Sometimes current time appears to be before the time that the document was
    // sent due to clock skew between the client and the server, so sanity check it
    if (now > date)
        currentAge = now - date;
    else
        currentAge = 0;

    // Compute document freshness and compare to minimum permitted
    if (maxAgeIsPresent) {
        // The Max-Age directive takes priority over Expires, so if max-age is present
        // in a response, the calculation is simply: 
        if (currentAge < maxAge)
            return PR_FALSE;
    } else {

        // Get the value of the 'Expires:' header
        PRUint32 expires;
        PRBool expiresHeaderIsPresent;
        rv = ParseDateHeader(nsHTTPAtoms::Expires, &expires, &expiresHeaderIsPresent);
        if (NS_FAILED(rv)) return PR_TRUE; // Corrupted or malformed headers ?

        if (expiresHeaderIsPresent) {
            // Otherwise, if Expires is present in the response, the calculation is: 
            if (currentAge < expires - date)
                return PR_FALSE;
        }
    }

    // At this point, there are no protocol-defined means to determine whether or
    // not the HTTP response can be considered stale.  Hence, we resort to a heuristic
    // approach.

    // Check if the document's age is older than a specified fraction of time
    if (aUseHeuristicExpiration) {

        // Get the value of the 'LastModified:' header

        PRUint32 lastModified;
        PRBool lastModifiedHeaderIsPresent;
        rv = ParseDateHeader(nsHTTPAtoms::Last_Modified, &lastModified,
                             &lastModifiedHeaderIsPresent);

        // Check for corrupted, missing or malformed 'LastModified:' header
        if (NS_FAILED(rv) || !lastModifiedHeaderIsPresent || !lastModified)
            return PR_TRUE;

        if (lastModified > date) {
            // Weird: document's last-modified date is after it was sent from the server
            return PR_TRUE;
        }

        PRUint32 heuristicThresholdAge;
        heuristicThresholdAge = (PRUint32)((date - lastModified) * HEURISTIC_STALENESS_FACTOR);
        
        if (currentAge < heuristicThresholdAge)
            return PR_FALSE;
    }

    return PR_TRUE;
}

// This routine is used to reconstruct the HTTP response headers that will be
// stored in the cache.  However, these are not exactly the same as the headers
// that were received:
//    + All hop-by-hop headers are removed, leaving only the end-to-end headers
//    + The set-cookie header is removed since we do not usually want to replay
//      setting of cookies when using a cached response
//    + Header format is canonicalized, with extraneous whitespace deleted and
//      multiple headers with the same field name coalesced into one header
//      with comma-separated values.
//    + Headers may appear in a different order than they were transmitted
//
nsresult nsHTTPResponse::EmitHeaders(nsCString& aResponseBuffer)
{
    nsresult rv;

    //
    // Write the status line
    //
    // i.e. HTTP/x.y SP Status-Code SP Reason-Phrase CRLF
    //

    char *versionString;
    if (mServerVersion == HTTP_ZERO_NINE)
        versionString = "0.9";
    else if (mServerVersion == HTTP_ONE_ZERO)
        versionString = "1.0";
    else if (mServerVersion == HTTP_ONE_ONE)
        versionString = "1.1";
    else
        versionString = "?.?";

    char *statusLine;
    statusLine = PR_smprintf("HTTP/%s %3d %s", versionString, mStatus, (char*)mStatusString);
    if (!statusLine)
        return NS_ERROR_OUT_OF_MEMORY;

    aResponseBuffer = statusLine;
    aResponseBuffer.Append(CRLF);

    PR_smprintf_free(statusLine);

    //
    // Write the response headers, if any...
    //
    // i.e. field-name ":" [field-value] CRLF
    //
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    rv = mHeaders.GetEnumerator(getter_AddRefs(enumerator));
    if (NS_FAILED(rv)) return rv;

    PRBool bMoreHeaders;
    nsXPIDLCString autoBuffer;
    nsAutoString autoString;
    nsCOMPtr<nsISupports>   item;
    nsCOMPtr<nsIHTTPHeader> header;
    nsCOMPtr<nsIAtom>       headerAtom;

    while (1) {
        enumerator->HasMoreElements(&bMoreHeaders);
        if (!bMoreHeaders)
            break;

        enumerator->GetNext(getter_AddRefs(item));
        header = do_QueryInterface(item);

        NS_ASSERTION(header, "Bad HTTP header.");
        if (!header)
            return NS_ERROR_FAILURE;

        header->GetField(getter_AddRefs(headerAtom));
        nsIAtom* headerAtomRaw = headerAtom;
            
        if (headerAtomRaw == nsHTTPAtoms::Connection          ||
            headerAtomRaw == nsHTTPAtoms::Keep_Alive          ||
            headerAtomRaw == nsHTTPAtoms::Proxy_Authenticate  ||
            headerAtomRaw == nsHTTPAtoms::Proxy_Authorization ||
            headerAtomRaw == nsHTTPAtoms::TE                  ||
            headerAtomRaw == nsHTTPAtoms::Trailer             ||
            headerAtomRaw == nsHTTPAtoms::Transfer_Encoding   ||
            headerAtomRaw == nsHTTPAtoms::Upgrade             ||
            headerAtomRaw == nsHTTPAtoms::Set_Cookie)
            continue;
                    
        header->GetValue(getter_Copies(autoBuffer));

        headerAtom->ToString(autoString);
        autoString.Append(": ");
        autoString.Append(autoBuffer);
        autoString.Append(CRLF);
        aResponseBuffer.Append(autoString);
    }

    return NS_OK;
}

// Digest a single string that contains all the HTTP response headers,
// including the status line.
nsresult nsHTTPResponse::ParseHeaders(nsCString& aAllHeaders)
{
    PRInt32 beginLineOffset, endLineOffset;
    nsCString lineBuffer;
    nsresult rv;
    
    beginLineOffset = endLineOffset = 0;

    while (1) {
        endLineOffset = aAllHeaders.Find("\r", PR_FALSE, beginLineOffset);
        if (endLineOffset == kNotFound)
            return NS_OK;
        aAllHeaders.Mid(lineBuffer, beginLineOffset, endLineOffset - beginLineOffset);
        if (beginLineOffset == 0)
            rv = ParseStatusLine(lineBuffer);
        else
            rv = ParseHeader(lineBuffer);
        if (NS_FAILED(rv)) return rv;
        beginLineOffset = endLineOffset + 2; // Skip past CRLF
    }
}
