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
 */
#include "nsSignedStub.h"
#include "prmem.h"
#include "plstr.h"
#include "mimexpcom.h"
#include "mimecth.h"
#include "mimeobj.h"
#include "nsCRT.h"

static int MimeInlineTextSIGNEDStub_parse_line (char *, PRInt32, MimeObject *);
static int MimeInlineTextSIGNEDStub_parse_eof (MimeObject *, PRBool);
static int MimeInlineTextSIGNEDStub_parse_begin (MimeObject *obj);

 /* This is the object definition. Note: we will set the superclass 
    to NULL and manually set this on the class creation */

MimeDefClass(MimeInlineTextSIGNEDStub, MimeInlineTextSIGNEDStubClass, mimeInlineTextSIGNEDStubClass, NULL);  

extern "C" char *
MIME_GetContentType(void)
{
  return SIGNED_CONTENT_TYPE;
}


extern "C" MimeObjectClass *
MIME_CreateContentTypeHandlerClass(const char *content_type, 
                                   contentTypeHandlerInitStruct *initStruct)
{
  MimeObjectClass *clazz = (MimeObjectClass *)&mimeInlineTextSIGNEDStubClass;
  /* 
   * Must set the superclass by hand.
   */
  if (!COM_GetmimeInlineTextClass())
    return NULL;

  clazz->superclass = (MimeObjectClass *)COM_GetmimeInlineTextClass();
  initStruct->force_inline_display = PR_TRUE;
  return clazz;
}

static int
MimeInlineTextSIGNEDStubClassInitialize(MimeInlineTextSIGNEDStubClass *clazz)
{
  MimeObjectClass *oclass = (MimeObjectClass *) clazz;
  PR_ASSERT(!oclass->class_initialized);
  oclass->parse_begin = MimeInlineTextSIGNEDStub_parse_begin;
  oclass->parse_line  = MimeInlineTextSIGNEDStub_parse_line;
  oclass->parse_eof   = MimeInlineTextSIGNEDStub_parse_eof;

  return 0;
}

int
GenerateMessage(char** html) 
{
  *html = nsCRT::strdup("\
<BR><text=\"#000000\" bgcolor=\"#FFFFFF\" link=\"#FF0000\" vlink=\"#800080\" alink=\"#0000FF\">\
<center><table BORDER=1 ><tr>\
<td><CENTER>This messages is possibly <B>SIGNED</B>. Mozilla Mail does not support signed mail.</CENTER></td>\
</tr>\
</table></center><BR>");

  return 0;
}

static int
MimeInlineTextSIGNEDStub_parse_begin(MimeObject *obj)
{
  MimeInlineTextSIGNEDStubClass *clazz;
  int status = ((MimeObjectClass*)COM_GetmimeLeafClass())->parse_begin(obj);
  
  if (status < 0) 
    return status;
  
  if (!obj->output_p) 
    return 0;
  if (!obj->options || !obj->options->write_html_p) 
    return 0;
  
  /* This is a fine place to write out any HTML before the real meat begins. */  

  // Initialize the clazz variable...
  clazz = ((MimeInlineTextSIGNEDStubClass *) obj->clazz);
  return 0;
}

static int
MimeInlineTextSIGNEDStub_parse_line(char *line, PRInt32 length, MimeObject *obj)
{
 /* 
  * This routine gets fed each line of data, one at a time. We just buffer
  * it all up, to be dealt with all at once at the end. 
  */  
  if (!obj->output_p) 
    return 0;
  if (!obj->options || !obj->options->output_fn) 
    return 0;

  if (!obj->options->write_html_p) 
  {
    return COM_MimeObject_write(obj, line, length, TRUE);
  }
  
  return 0;
}

static int
MimeInlineTextSIGNEDStub_parse_eof (MimeObject *obj, PRBool abort_p)
{
  int status = 0;
  char* html = NULL;
  
  if (obj->closed_p) return 0;
  
  /* Run parent method first, to flush out any buffered data. */
  status = ((MimeObjectClass*)COM_GetmimeInlineTextClass())->parse_eof(obj, abort_p);
  if (status < 0) 
    return status;
  
  status = GenerateMessage(&html);
  if (status < 0) 
    return status;
  
  status = COM_MimeObject_write(obj, html, PL_strlen(html), TRUE);
  PR_FREEIF(html);
  if (status < 0) 
    return status;
 
  return 0;
}
