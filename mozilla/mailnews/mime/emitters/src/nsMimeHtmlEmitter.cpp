/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 *    Henrik Gemal <gemal@gemal.dk>
 */
#include "nsCOMPtr.h"
#include "stdio.h"
#include "nsMimeRebuffer.h"
#include "nsMimeHtmlEmitter.h"
#include "plstr.h"
#include "nsMailHeaders.h"
#include "nscore.h"
#include "nsEmitterUtils.h"
#include "nsEscape.h"
#include "nsIMimeStreamConverter.h"
#include "nsIMsgWindow.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsXPIDLString.h"
#include "nsMimeTypes.h"
#include "prtime.h"

#include "nsIMimeConverter.h"
#include "nsMsgMimeCID.h"
#include "nsDateTimeFormatCID.h"

static NS_DEFINE_CID(kCMimeConverterCID, NS_MIME_CONVERTER_CID);
static NS_DEFINE_CID(kDateTimeFormatCID,    NS_DATETIMEFORMAT_CID);

/*
 * nsMimeHtmlEmitter definitions....
 */
nsMimeHtmlDisplayEmitter::nsMimeHtmlDisplayEmitter()
{
  mFirst = PR_TRUE;
  mSkipAttachment = PR_FALSE; 
}

nsMimeHtmlDisplayEmitter::~nsMimeHtmlDisplayEmitter(void)
{
}

nsresult nsMimeHtmlDisplayEmitter::Init()
{
  return NS_OK;
}

PRBool nsMimeHtmlDisplayEmitter::BroadCastHeadersAndAttachments()
{
  // try to get a header sink if there is one....
  nsCOMPtr<nsIMsgHeaderSink> headerSink; 
  nsresult rv = GetHeaderSink(getter_AddRefs(headerSink));
  if (NS_SUCCEEDED(rv) && headerSink && mDocHeader)
    return PR_TRUE;
  else
    return PR_FALSE;
}

nsresult 
nsMimeHtmlDisplayEmitter::WriteHeaderFieldHTMLPrefix()
{
  if (!BroadCastHeadersAndAttachments() || (mFormat == nsMimeOutput::nsMimeMessagePrintOutput))
    return nsMimeBaseEmitter::WriteHeaderFieldHTMLPrefix();
  else
    return NS_OK;
}

nsresult
nsMimeHtmlDisplayEmitter::WriteHeaderFieldHTML(const char *field, const char *value)
{
  if (!BroadCastHeadersAndAttachments() || (mFormat == nsMimeOutput::nsMimeMessagePrintOutput))
    return nsMimeBaseEmitter::WriteHeaderFieldHTML(field, value);
  else
    return NS_OK;
}

nsresult
nsMimeHtmlDisplayEmitter::WriteHeaderFieldHTMLPostfix()
{
  if (!BroadCastHeadersAndAttachments() || (mFormat == nsMimeOutput::nsMimeMessagePrintOutput))
    return nsMimeBaseEmitter::WriteHeaderFieldHTMLPostfix();
  else
    return NS_OK;
}


nsresult
nsMimeHtmlDisplayEmitter::GetHeaderSink(nsIMsgHeaderSink ** aHeaderSink)
{
  nsresult rv = NS_OK;
  if ( (mChannel) && (!mHeaderSink) )
  {
    nsCOMPtr<nsIURI> uri;
    mChannel->GetURI(getter_AddRefs(uri));
    if (uri)
    {
      nsCOMPtr<nsIMsgMailNewsUrl> msgurl (do_QueryInterface(uri));
      if (msgurl)
      {
        nsCOMPtr<nsIMsgWindow> msgWindow;
        msgurl->GetMsgWindow(getter_AddRefs(msgWindow));
        if (msgWindow)
          msgWindow->GetMsgHeaderSink(getter_AddRefs(mHeaderSink));
      }
    }
  }

  *aHeaderSink = mHeaderSink;
  NS_IF_ADDREF(*aHeaderSink);
  return rv;
}

nsresult nsMimeHtmlDisplayEmitter::WriteHTMLHeaders()
{
  if (mDocHeader)
  {
    UtilityWriteCRLF("<html>");
    UtilityWriteCRLF("<head>");

    // mscott --> we should refer to the style sheet used in msg display...this one is wrong i think.
    // Stylesheet info!
    UtilityWriteCRLF("<link rel=\"important stylesheet\" href=\"chrome://messenger/skin/mailheader.css\">");

    UtilityWriteCRLF("</head>");
    UtilityWriteCRLF("<body>");
  }

  // if we aren't broadcasting headers OR printing...just do whatever
  // our base class does...
  if (mFormat == nsMimeOutput::nsMimeMessagePrintOutput)
  {
    return nsMimeBaseEmitter::WriteHTMLHeaders();
  }
  else if (!BroadCastHeadersAndAttachments() || !mDocHeader)
  {
    // This needs to be here to correct the output format if we are
    // not going to broadcast headers to the XUL document.
    if (mFormat == nsMimeOutput::nsMimeMessageBodyDisplay)
      mFormat = nsMimeOutput::nsMimeMessagePrintOutput;

    return nsMimeBaseEmitter::WriteHTMLHeaders();
  }
  else
    mFirstHeaders = PR_FALSE;
 
  PRBool bFromNewsgroups = PR_FALSE;
  for (PRInt32 j=0; j<mHeaderArray->Count(); j++)
  {
    headerInfoType *headerInfo = (headerInfoType *)mHeaderArray->ElementAt(j);
    if (!(headerInfo && headerInfo->name && *headerInfo->name))
      continue;

    if (!nsCRT::strcasecmp("Newsgroups", headerInfo->name))
    {
      bFromNewsgroups = PR_TRUE;
	  break;
    }
  }

  // try to get a header sink if there is one....
  nsCOMPtr<nsIMsgHeaderSink> headerSink; 
  nsresult rv = GetHeaderSink(getter_AddRefs(headerSink));

  if (headerSink)
    headerSink->OnStartHeaders();

  // We are going to iterate over all the known headers,
  // and broadcast them to the header sink. However, we need to 
  // convert our UTF-8 header values into unicode before 
  // broadcasting them....
  nsXPIDLString unicodeHeaderValue;

  for (PRInt32 i=0; i<mHeaderArray->Count(); i++)
  {
    headerInfoType *headerInfo = (headerInfoType *)mHeaderArray->ElementAt(i);
    if ( (!headerInfo) || (!headerInfo->name) || (!(*headerInfo->name)) ||
      (!headerInfo->value) || (!(*headerInfo->value)))
      continue;

    if (headerSink)
    {
      char buffer[128];
      const char * headerValue = headerInfo->value;

      if (nsCRT::strcasecmp("Date", headerInfo->name) == 0)
      {
        nsXPIDLString formattedDate;
        nsresult rv = GenerateDateString(headerInfo->value, getter_Copies(formattedDate));
        if (NS_SUCCEEDED(rv))
          headerSink->HandleHeader(headerInfo->name, formattedDate, bFromNewsgroups);
        // let's try some fancy date formatting...
        PRExplodedTime explode;
        PRTime dateTime;
        PR_ParseTimeString(headerInfo->value, PR_FALSE, &dateTime);

        PR_ExplodeTime( dateTime, PR_LocalTimeParameters, &explode);
        PR_FormatTime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M %p", &explode);
        headerValue = buffer;
      }
      else
      {
        // Convert UTF-8 to UCS2
        *((PRUnichar **)getter_Copies(unicodeHeaderValue)) = nsXPIDLString::Copy(NS_ConvertUTF8toUCS2(headerValue).GetUnicode());

        if (NS_SUCCEEDED(rv))
          headerSink->HandleHeader(headerInfo->name, unicodeHeaderValue, bFromNewsgroups);
      }
    }
  }

  if (headerSink)
    headerSink->OnEndHeaders();
  return NS_OK;
}

nsresult nsMimeHtmlDisplayEmitter::GenerateDateString(const char * dateString, PRUnichar ** aDateString)
{
  nsAutoString formattedDateString;
  nsresult rv = NS_OK;

  if (!mDateFormater)
    mDateFormater = do_CreateInstance(kDateTimeFormatCID);

  PRTime messageTime;
  PR_ParseTimeString(dateString, PR_FALSE, &messageTime);

  PRTime currentTime = PR_Now();
	PRExplodedTime explodedCurrentTime;
  PR_ExplodeTime(currentTime, PR_LocalTimeParameters, &explodedCurrentTime);
  PRExplodedTime explodedMsgTime;
  PR_ExplodeTime(messageTime, PR_LocalTimeParameters, &explodedMsgTime);

  // if the message is from today, don't show the date, only the time. (i.e. 3:15 pm)
  // if the message is from the last week, show the day of the week.   (i.e. Mon 3:15 pm)
  // in all other cases, show the full date (03/19/01 3:15 pm)
  nsDateFormatSelector dateFormat = kDateFormatShort;
  if (explodedCurrentTime.tm_year == explodedMsgTime.tm_year &&
      explodedCurrentTime.tm_month == explodedMsgTime.tm_month &&
      explodedCurrentTime.tm_mday == explodedMsgTime.tm_mday)
  {
    // same day...
    dateFormat = kDateFormatNone;
  } 
  // the following chunk of code causes us to show a day instead of a number if the message was received
  // within the last 7 days. i.e. Mon 5:10pm. We need to add a preference so folks to can enable this behavior
  // if they want it. 
/*
  else if (LL_CMP(currentTime, >, dateOfMsg))
  {
    PRInt64 microSecondsPerSecond, secondsInDays, microSecondsInDays;
	  LL_I2L(microSecondsPerSecond, PR_USEC_PER_SEC);
    LL_UI2L(secondsInDays, 60 * 60 * 24 * 7); // how many seconds in 7 days.....
	  LL_MUL(microSecondsInDays, secondsInDays, microSecondsPerSecond); // turn that into microseconds

    PRInt64 diff;
    LL_SUB(diff, currentTime, dateOfMsg);
    if (LL_CMP(diff, <=, microSecondsInDays)) // within the same week 
      dateFormat = kDateFormatWeekday;
  }
*/

  if (NS_SUCCEEDED(rv)) 
    rv = mDateFormater->FormatPRTime(nsnull /* nsILocale* locale */,
                                      dateFormat,
                                      kTimeFormatNoSeconds,
                                      messageTime,
                                      formattedDateString);

  if (NS_SUCCEEDED(rv))
    *aDateString = formattedDateString.ToNewUnicode();

  return rv;
}

nsresult
nsMimeHtmlDisplayEmitter::EndHeader()
{
  WriteHTMLHeaders();
  return NS_OK;
}

nsresult
nsMimeHtmlDisplayEmitter::StartAttachment(const char *name, const char *contentType, const char *url,
                                          PRBool aNotDownloaded)
{

  nsresult rv = NS_OK;
  nsCOMPtr<nsIMsgHeaderSink> headerSink; 
  rv = GetHeaderSink(getter_AddRefs(headerSink));
  
  if (headerSink)
  {
    char * escapedUrl = nsEscape(url, url_Path);
    nsXPIDLCString uriString;

    nsCOMPtr<nsIMsgMessageUrl> msgurl (do_QueryInterface(mURL, &rv));
    if (NS_SUCCEEDED(rv))
      rv = msgurl->GetUri(getter_Copies(uriString));

    // we need to convert the attachment name from UTF-8 to unicode before
    // we emit it...
    nsXPIDLString unicodeHeaderValue;
    nsAutoString charset; charset.AssignWithConversion("UTF-8");

    rv = NS_OK;
    if (mUnicodeConverter)
  	  rv = mUnicodeConverter->DecodeMimePartIIStr(NS_ConvertASCIItoUCS2(name), charset,
                                                  getter_Copies(unicodeHeaderValue));
    else {
      nsAutoString attachmentName; attachmentName.AssignWithConversion(name);
      *((PRUnichar **)getter_Copies(unicodeHeaderValue)) =
        nsXPIDLString::Copy(attachmentName.GetUnicode());
    }

    if (NS_FAILED(rv))
    {
      nsAutoString attachmentName; attachmentName.AssignWithConversion(name);
      *((PRUnichar **)getter_Copies(unicodeHeaderValue)) =
        nsXPIDLString::Copy(attachmentName.GetUnicode());
    }

    headerSink->HandleAttachment(contentType, url /* was escapedUrl */, unicodeHeaderValue, uriString, aNotDownloaded);

    nsCRT::free(escapedUrl);
    mSkipAttachment = PR_TRUE;
  }
  else
    // then we need to deal with the attachments in the body by inserting them into a table..
    return StartAttachmentInBody(name, contentType, url);

  return rv;
}

// Attachment handling routines
// Ok, we are changing the way we handle these now...It used to be that we output 
// HTML to make a clickable link, etc... but now, this should just be informational
// and only show up in quoting
//
nsresult
nsMimeHtmlDisplayEmitter::StartAttachmentInBody(const char *name, const char *contentType, const char *url)
{
  if ( (contentType) &&
        ((!nsCRT::strcmp(contentType, APPLICATION_XPKCS7_MIME)) ||
         (!nsCRT::strcmp(contentType, APPLICATION_XPKCS7_SIGNATURE)) ||
         (!nsCRT::strcmp(contentType, TEXT_VCARD))
        )
     )
  {
    mSkipAttachment = PR_TRUE;
    return NS_OK;
  }
  else
    mSkipAttachment = PR_FALSE;

  if (!mFirst)
    UtilityWrite("<hr width=\"90%\" size=4>");

  mFirst = PR_FALSE;

  UtilityWrite("<CENTER>");
  UtilityWrite("<table border>");
  UtilityWrite("<tr>");
  UtilityWrite("<td>");

  UtilityWrite("<div align=right class=\"headerdisplayname\" style=\"display:inline;\">");

  UtilityWrite(name);

  UtilityWrite("</div>");

  UtilityWrite("</td>");
  UtilityWrite("<td>");
  UtilityWrite("<table border=0>");
  return NS_OK;
}

nsresult
nsMimeHtmlDisplayEmitter::AddAttachmentField(const char *field, const char *value)
{
  if (mSkipAttachment || BroadCastHeadersAndAttachments())
    return NS_OK;

  // Don't let bad things happen
  if ( (!value) || (!*value) )
    return NS_OK;

  // Don't output this ugly header...
  if (!nsCRT::strcmp(field, HEADER_X_MOZILLA_PART_URL))
    return NS_OK;

  char  *newValue = nsEscapeHTML(value);

  UtilityWrite("<tr>");

  UtilityWrite("<td>");
  UtilityWrite("<div align=right class=\"headerdisplayname\" style=\"display:inline;\">");

  UtilityWrite(field);
  UtilityWrite(":");
  UtilityWrite("</div>");
  UtilityWrite("</td>");
  UtilityWrite("<td>");

  UtilityWrite(newValue);

  UtilityWrite("</td>");
  UtilityWrite("</tr>");

  PR_FREEIF(newValue);
  return NS_OK;
}

nsresult
nsMimeHtmlDisplayEmitter::EndAttachment()
{
  mSkipAttachment = PR_FALSE;
  if (BroadCastHeadersAndAttachments())
    return NS_OK;
  
  UtilityWrite("</table>");
  UtilityWrite("</td>");
  UtilityWrite("</tr>");

  UtilityWrite("</table>");
  UtilityWrite("</center>");
  UtilityWrite("<br>");
  return NS_OK;
}

nsresult
nsMimeHtmlDisplayEmitter::EndAllAttachments()
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIMsgHeaderSink> headerSink; 
  rv = GetHeaderSink(getter_AddRefs(headerSink));
  if (headerSink)
	  headerSink->OnEndAllAttachments();
  return rv;
}

nsresult
nsMimeHtmlDisplayEmitter::WriteBody(const char *buf, PRUint32 size, PRUint32 *amountWritten)
{
  Write(buf, size, amountWritten);
  return NS_OK;
}

nsresult
nsMimeHtmlDisplayEmitter::EndBody()
{
  UtilityWriteCRLF("</body>");
  UtilityWriteCRLF("</html>");
  return NS_OK;
}


