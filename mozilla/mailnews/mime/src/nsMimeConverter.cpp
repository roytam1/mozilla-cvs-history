/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "stdio.h"
#include "mimecom.h"
#include "modmimee.h"
#include "nscore.h"
#include "nsMimeConverter.h"
#include "comi18n.h"
#include "prmem.h"
#include "plstr.h"

NS_IMPL_ISUPPORTS(nsMimeConverter, NS_GET_IID(nsIMimeConverter));

/*
 * nsMimeConverter definitions....
 */

/* 
 * Inherited methods for nsMimeConverter
 */
nsMimeConverter::nsMimeConverter()
{
  /* the following macro is used to initialize the ref counting data */
  NS_INIT_ISUPPORTS();
}

nsMimeConverter::~nsMimeConverter()
{
}

nsresult 
nsMimeConverter::DecodeMimePartIIStr(const nsString& header, 
                                           nsString& charset, 
                                           nsString& decodedString)
{
  char charsetCstr[kMAX_CSNAME+1];
  char *encodedCstr;
  char *decodedCstr;
  nsresult res = NS_OK;

  (void) charset.ToCString(charsetCstr, kMAX_CSNAME+1);
  encodedCstr = header.ToNewCString();
  if (nsnull != encodedCstr) {
    PRUnichar *unichars;
    PRInt32 unicharLength;
    // apply MIME decode.
    decodedCstr = MIME_DecodeMimePartIIStr((const char *) encodedCstr, charsetCstr);
    if (nsnull == decodedCstr) {
      // no decode needed and no default charset was specified
      if (*charsetCstr == '\0') {
        decodedString = header;
      }
      else {
        // no MIME encoded, convert default charset to unicode
        res = MIME_ConvertToUnicode(charsetCstr, (const char *) encodedCstr, 
                                    (void **) &unichars, &unicharLength);      
        if (NS_SUCCEEDED(res)) {
          decodedString.SetString(unichars, unicharLength);
          PR_Free(unichars);
        }
      }
    }
    else {
      // convert MIME charset to unicode
      res = MIME_ConvertToUnicode(charsetCstr, (const char *) decodedCstr, 
                                  (void **) &unichars, &unicharLength);      
      if (NS_SUCCEEDED(res)) {
        charset.SetString(charsetCstr);
        decodedString.SetString(unichars, unicharLength);
        PR_Free(unichars);
      }
      PR_Free(decodedCstr);
    }
	nsAllocator::Free(encodedCstr);
  }
  return res;
}

nsresult
nsMimeConverter::DecodeMimePartIIStr(const char *header, 
                                           char       *charset, 
                                           char **decodedString)
{
  char *retString = MIME_DecodeMimePartIIStr(header, charset); 
  if (retString == NULL)
    return NS_ERROR_FAILURE;
  else
  {
    *decodedString = retString;
    return NS_OK;
  }
}

nsresult
nsMimeConverter::EncodeMimePartIIStr(const char    *header, 
                                           const char    *mailCharset, 
                                           const PRInt32 encodedWordSize, 
                                           char          **encodedString)
{
  char *utf8String;

  // Encoder needs utf-8 string.
  if (MIME_ConvertString(mailCharset, "utf-8", header, &utf8String) != 0)
    return NS_ERROR_FAILURE;

  return EncodeMimePartIIStr_UTF8((const char *) utf8String, mailCharset, encodedWordSize, encodedString);
}

nsresult
nsMimeConverter::EncodeMimePartIIStr_UTF8(const char    *header, 
                                          const char    *mailCharset, 
                                          const PRInt32 encodedWordSize, 
                                          char          **encodedString)
{
  char *retString = MIME_EncodeMimePartIIStr(header, mailCharset, encodedWordSize);
  if (retString == NULL)
    return NS_ERROR_FAILURE;
  else
  {
    *encodedString = retString;
    return NS_OK;
  }
}

nsresult 
nsMimeConverter::B64EncoderInit(int (*output_fn) (const char *buf, PRInt32 size, void *closure), void *closure, 
                   MimeEncoderData **returnEncoderData) 
{
MimeEncoderData   *ptr;

  ptr = MimeB64EncoderInit(output_fn, closure);
  if (ptr)
  {
    *returnEncoderData = ptr;
    return NS_OK;
  }
  else
    return NS_ERROR_OUT_OF_MEMORY;
}

nsresult
nsMimeConverter::QPEncoderInit (int (*output_fn) (const char *buf, 
                      PRInt32 size, void *closure), void *closure, MimeEncoderData ** returnEncoderData) 
{
MimeEncoderData   *ptr;

  ptr = MimeQPEncoderInit(output_fn, closure);
  if (ptr)
  {
    *returnEncoderData = ptr;
    return NS_OK;
  }
  else
    return NS_ERROR_OUT_OF_MEMORY;
}

nsresult
nsMimeConverter::UUEncoderInit (char *filename, int (*output_fn) 
                     (const char *buf, PRInt32 size, void *closure), void *closure, 
                     MimeEncoderData ** returnEncoderData)
{
MimeEncoderData   *ptr;

  ptr = MimeUUEncoderInit(filename, output_fn, closure);
  if (ptr)
  {
    *returnEncoderData = ptr;
    return NS_OK;
  }
  else
    return NS_ERROR_OUT_OF_MEMORY;
}

nsresult
nsMimeConverter::EncoderDestroy(MimeEncoderData *data, PRBool abort_p) 
{
  MimeEncoderDestroy(data, abort_p);
  return NS_OK;
}

nsresult
nsMimeConverter::EncoderWrite (MimeEncoderData *data, const char *buffer, PRInt32 size, PRInt32 *written) 
{
  PRInt32   writeCount;
  writeCount = MimeEncoderWrite(data, buffer, size);
  *written = writeCount;
  return NS_OK;
}
