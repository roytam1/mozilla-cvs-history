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
#include "xp.h"
#include "netutils.h"
#include "mkselect.h"
#include "mktcp.h"
#include "mkgeturl.h"
#include "mkstream.h"

typedef struct _ProxyObj {
    PRBool  past_first_line;
	PRBool  definately_send_headers;
	char    *content_type;
	char    *content_encoding;
} ProxyObj;

PRIVATE int net_proxy_write (NET_StreamClass *stream, CONST char* s, int32 len)
{
	ProxyObj *obj=stream->data_object;
    /* send HTTP headers if not already getting an HTTP doc
     */	
    if(!obj->past_first_line)
      {
		if(obj->definately_send_headers || strncmp(s, "HTTP/", len >= 5 ? 5 : len))
		  {
            write(1, "HTTP/1.0 200 OK\r\n",17);
            write(1, "Content-type: ",14);
            write(1, obj->content_type, PL_strlen(obj->content_type));
            if(obj->content_encoding)
              {
                write(1, "\r\nContent-encoding: ",18);
                write(1, obj->content_encoding, PL_strlen(obj->content_encoding));
              }
            write(1, "\r\nServer: MKLib proxy agent\r\n",29);
            write(1, "\r\n", 2);  /* finish it */
		  }

		obj->past_first_line = PR_TRUE;
      }

    write(1, s, len); 
    return(1);
}

PRIVATE unsigned int net_proxy_WriteReady (NET_StreamClass *stream)
{	
   return(MAX_WRITE_READY);  /* always ready for writing */
}


PRIVATE void net_proxy_complete (NET_StreamClass *stream)
{
	ProxyObj *obj=stream->data_object;	
	FREEIF(obj->content_type);
	FREEIF(obj->content_encoding);
	FREE(obj);
    return;
}

PRIVATE void net_proxy_abort (NET_StreamClass *stream, int status)
{
	ProxyObj *obj=stream->data_object;	
	FREEIF(obj->content_type);
	FREEIF(obj->content_encoding);
	FREE(obj);
    return;
}



PUBLIC NET_StreamClass * 
NET_ProxyConverter(int         format_out,
                   void       *data_obj,
                   URL_Struct *URL_s,
                   MWContext  *window_id)
{
    NET_StreamClass* stream;
    ProxyObj * obj;
    
    TRACEMSG(("Setting up display stream. Have URL: %s \n%s\n", 
									URL_s->address, URL_s->content_type));

    stream = PR_NEW(NET_StreamClass);
    if(stream == NULL) 
        return(NULL);

	obj = PR_NEW(ProxyObj);
    if(obj == NULL) 
	  {
		FREE(stream);
        return(NULL);
	  }

	memset(obj, 0, sizeof(ProxyObj));

	stream->data_object = obj;
	

    stream->name           = "ProxyWriter";
    stream->complete       = (MKStreamCompleteFunc) net_proxy_complete;
    stream->abort          = (MKStreamAbortFunc) net_proxy_abort;
    stream->put_block      = (MKStreamWriteFunc) net_proxy_write;
    stream->is_write_ready = (MKStreamWriteReadyFunc) net_proxy_WriteReady;
    stream->window_id      = window_id;

    TRACEMSG(("Returning stream from display_converter\n"));

    /* send HTTP headers if not already getting an HTTP doc 
	 */
    if(PL_strncasecmp(URL_s->address,"http:",5))
      {
		obj->definately_send_headers = TRUE;
      }
	StrAllocCopy(obj->content_type, URL_s->content_type);
	StrAllocCopy(obj->content_encoding, URL_s->content_encoding);

    return stream;
}
