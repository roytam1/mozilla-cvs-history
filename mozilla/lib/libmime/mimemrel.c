/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/* mimemrel.c --- definition of the MimeMultipartRelated class (see mimei.h)
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

/* Thoughts on how to implement this:

   = if the type of this multipart/related is not text/html, then treat
     it the same as multipart/mixed.

   = For each part in this multipart/related
     = if this part is not the "top" part
       = then save this part to a tmp file or a memory object,
         kind-of like what we do for multipart/alternative sub-parts.
         If this is an object we're blocked on (see below) send its data along.
     = else
       = emit this part (remember, it's of type text/html)
       = at some point, layout may load a URL for <IMG SRC="cid:xxxx">.
         we intercept that.
         = if one of our cached parts has that cid, return the data for it.
         = else, "block", the same way the image library blocks layout when it
           doesn't yet have the size of the image.
       = at some point, layout may load a URL for <IMG SRC="relative/yyy">.
         we need to intercept that too.
         = expand the URL, and compare it to our cached objects.
           if it matches, return it.
         = else block on it.

   = once we get to the end, if we have any sub-part references that we're
     still blocked on, map over them:
     = if they're cid: references, close them ("broken image" results.)
     = if they're URLs, then load them in the normal way.

  --------------------------------------------------

  Ok, that's fairly complicated.  How about an approach where we go through
  all the parts first, and don't emit until the end?

   = if the type of this multipart/related is not text/html, then treat
     it the same as multipart/mixed.

   = For each part in this multipart/related
       = save this part to a tmp file or a memory object,
         like what we do for multipart/alternative sub-parts.

   = Emit the "top" part (the text/html one)
     = intercept all calls to NET_GetURL, to allow us to rewrite the URL.
       (hook into netlib, or only into imglib's calls to GetURL?)
       (make sure we're behaving in a context-local way.)

       = when a URL is loaded, look through our cached parts for a match.
         = if we find one, map that URL to a "cid:" URL
         = else, let it load normally

     = at some point, layout may load a URL for <IMG SRC="cid:xxxx">.
       it will do this either because that's what was in the HTML, or because
       that's how we "rewrote" the URLs when we intercepted NET_GetURL.

       = if one of our cached parts has the requested cid, return the data
         for it.
       = else, generate a "broken image"

   = free all the cached data

  --------------------------------------------------

  How hard would be an approach where we rewrite the HTML?
  (Looks like it's not much easier, and might be more error-prone.)

   = if the type of this multipart/related is not text/html, then treat
     it the same as multipart/mixed.

   = For each part in this multipart/related
       = save this part to a tmp file or a memory object,
         like what we do for multipart/alternative sub-parts.

   = Parse the "top" part, and emit slightly different HTML:
     = for each <IMG SRC>, <IMG LOWSRC>, <A HREF>?  Any others?
       = look through our cached parts for a matching URL
         = if we find one, map that URL to a "cid:" URL
         = else, let it load normally

     = at some point, layout may load a URL for <IMG SRC="cid:xxxx">.
       = if one of our cached parts has the requested cid, return the data
         for it.
       = else, generate a "broken image"

   = free all the cached data
 */



#include "mimemrel.h"

extern int MK_UNABLE_TO_OPEN_TMP_FILE;

#define MIME_SUPERCLASS mimeMultipartClass
MimeDefClass(MimeMultipartRelated, MimeMultipartRelatedClass,
			 mimeMultipartRelatedClass, &MIME_SUPERCLASS);


/* Stupid utility function.  Really ought to be part of the standard string
   package if you ask me...*/

static char* mime_strnchr(char* str, char c, int length)
{
	int i;
	for (i=0 ; i<length ; i++) {
		if (*str == c) return str;
		str++;
	}
	return NULL;
}


static int
MimeMultipartRelated_initialize(MimeObject* obj)
{
	MimeMultipartRelated* relobj = (MimeMultipartRelated*) obj;
	relobj->base_url = MimeHeaders_get(obj->headers, "Content-Base",
									   FALSE, FALSE);

	/* I used to have code here to test if the type was text/html.  Then I
	   added multipart/alternative as being OK, too.  Then I found that the
	   VCard spec seems to talk about having the first part of a
	   multipart/related be an application/directory.  At that point, I decided
	   to punt.  We handle anything as the first part, and stomp on the HTML it
	   generates to adjust tags to point into the other parts.  This probably
	   works out to something reasonable in most cases. */

	relobj->hash = XP_HashTableNew(20, XP_StringHash,
								   (XP_HashCompFunction) strcmp);
	if (!relobj->hash) return MK_OUT_OF_MEMORY;

	return ((MimeObjectClass*)&MIME_SUPERCLASS)->initialize(obj);
}

XP_Bool mime_multipart_related_nukehash(XP_HashTable table, const void* key,
										void* value, void* closure)
{
	XP_FREE((char*) key);
	XP_FREE((char*) value);
	return TRUE;
}

static void
MimeMultipartRelated_finalize (MimeObject *obj)
{
	MimeMultipartRelated* relobj = (MimeMultipartRelated*) obj;
	FREEIF(relobj->base_url);
	FREEIF(relobj->curtag);
	if (relobj->hash) {
		XP_MapRemhash(relobj->hash, mime_multipart_related_nukehash, NULL);
		XP_HashTableDestroy(relobj->hash);
		relobj->hash = NULL;
	}
	if (relobj->file_stream) {
		XP_FileClose(relobj->file_stream);
		relobj->file_stream = NULL;
	}
	if (relobj->file_buffer_name) {
		XP_FileRemove(relobj->file_buffer_name, xpTemporary);
		XP_FREE(relobj->file_buffer_name);
		relobj->file_buffer_name = 0;
	}
	((MimeObjectClass*)&MIME_SUPERCLASS)->finalize(obj);
}

char * escape_unescaped_percents(const char *incomingURL);

/* This routine is only necessary because the mailbox URL fed to us 
   by the winfe can contain spaces and '>'s in it. It's a hack. */
static char *
escape_for_mrel_subst(char *inURL)
{
	char *output, *inC, *outC, *temp;

	/* XP_STRLEN asserts the presence of a string in inURL */
	int size = XP_STRLEN(inURL) + 1;

	for(inC = inURL; *inC; inC++)
		if ((*inC == ' ') || (*inC == '>')) 
			size += 2; /* space -> '%20', '>' -> '%3E', etc. */

	output = XP_ALLOC(size);
	if (output)
	{
		/* Walk through the source string, copying all chars
			except for spaces, which get escaped. */
		inC = inURL;
		outC = output;
		while(*inC)
		{
			if (*inC == ' ')
			{
				*outC++ = '%'; *outC++ = '2'; *outC++ = '0';
			}
			else if (*inC == '>')
			{
				*outC++ = '%'; *outC++ = '3'; *outC++ = 'E';
			}
			else
				*outC++ = *inC;

			inC++;
		}
		*outC = '\0';
	
		temp = escape_unescaped_percents(output);
		if (temp)
		{
			FREEIF(output);
			output = temp;
		}
	}
	return output;
}

static XP_Bool
MimeMultipartRelated_output_child_p(MimeObject *obj, MimeObject* child)
{
	MimeMultipartRelated *relobj = (MimeMultipartRelated *) obj;
	if (relobj->head_loaded) {
		/* This is a child part.  Just remember the mapping between the URL
		   it represents and the part-URL to get it back. */
		char* location = MimeHeaders_get(child->headers, "Content-Location",
										 FALSE, FALSE);
		if (!location) {
			char* tmp = MimeHeaders_get(child->headers, "Content-ID",
										FALSE, FALSE);
			if (tmp) {
				char* tmp2 = tmp;
				if (*tmp2 == '<') {
					int length;
					tmp2++;
					length = XP_STRLEN(tmp2);
					if (length > 0 && tmp2[length - 1] == '>') {
						tmp2[length - 1] = '\0';
					}
				}
				location = PR_smprintf("cid:%s", tmp2);
				XP_FREE(tmp);
			}
		}
		if (location) {
			char* base_url = MimeHeaders_get(child->headers, "Content-Base",
											 FALSE, FALSE);
			char* absolute =
				NET_MakeAbsoluteURL(base_url ? base_url : relobj->base_url,
									location);
			FREEIF(base_url);
			XP_FREE(location);
			if (absolute) {
				char* partnum = mime_part_address(child);
				if (partnum) {
					char* part;
					part = mime_set_url_part(obj->options->url, partnum,
											 FALSE);
					if (part) {
					    /* If there's a space in the url, escape the url.
						   (This happens primarily on Windows and Unix.) */
					    char *temp = part;
					    if (XP_STRCHR(part, ' ') || XP_STRCHR(part, '>') || XP_STRCHR(part, '%'))
						  temp = escape_for_mrel_subst(part);
						XP_Puthash(relobj->hash, absolute, temp);

						/* The value string that is added to the hashtable will be deleted
						   by the hashtable at destruction time. So if we created an
						   escaped string for the hashtable, we have to delete the
						   part URL that was given to us. */
						if (temp != part) XP_FREE(part);
					}
					XP_FREE(partnum);
				}
			}
		}
	} else {
		/* Ah-hah!  We're the head object.  */
		char* base_url;
		relobj->head_loaded = TRUE;
		relobj->headobj = child;
		relobj->buffered_hdrs = MimeHeaders_copy(child->headers);
		base_url = MimeHeaders_get(child->headers, "Content-Base",
								   FALSE, FALSE);
		if (base_url) {
			/* If the head object has a base_url associated with it, use
			   that instead of any base_url that may have been associated
			   with the multipart/related. */
			FREEIF(relobj->base_url);
			relobj->base_url = base_url;
		}

	}
	if (obj->options && !obj->options->write_html_p
#ifdef MIME_DRAFTS
		&& !obj->options->decompose_file_p
#endif /* MIME_DRAFTS */
		)
	  {
		return TRUE;
	  }

	return FALSE;			/* Don't actually parse this child; we'll handle
							   all that at eof time. */
}

static int
MimeMultipartRelated_parse_child_line (MimeObject *obj,
									   char *line, int32 length,
									   XP_Bool first_line_p)
{
	MimeContainer *cont = (MimeContainer *) obj;
	MimeMultipartRelated *relobj = (MimeMultipartRelated *) obj;
	int status;
	MimeObject *kid;

	if (obj->options && !obj->options->write_html_p
#ifdef MIME_DRAFTS
		&& !obj->options->decompose_file_p
#endif /* MIME_DRAFTS */
		)
	  {
		/* Oh, just go do the normal thing... */
		return ((MimeMultipartClass*)&MIME_SUPERCLASS)->
			parse_child_line(obj, line, length, first_line_p);
	  }

	/* Throw it away if this isn't the head object.  (Someday, maybe we'll
	   cache it instead.) */
	XP_ASSERT(cont->nchildren > 0);
	if (cont->nchildren <= 0)
		return -1;
	kid = cont->children[cont->nchildren-1];
	XP_ASSERT(kid);
	if (!kid) return -1;
	if (kid != relobj->headobj) return 0;

	/* Buffer this up (###tw much code duplication from mimemalt.c) */
	/* If we don't yet have a buffer (either memory or file) try and make a
	   memory buffer. */
	if (!relobj->head_buffer && !relobj->file_buffer_name) {
		int target_size = 1024 * 50;       /* try for 50k */
		while (target_size > 0) {
			relobj->head_buffer = (char *) XP_ALLOC(target_size);
			if (relobj->head_buffer) break;  /* got it! */
			target_size -= (1024 * 5);     /* decrease it and try again */
		}

		if (relobj->head_buffer) {
			relobj->head_buffer_size = target_size;
		} else {
			relobj->head_buffer_size = 0;
		}

		relobj->head_buffer_fp = 0;
	}

	/* Ok, if at this point we still don't have either kind of buffer, try and
	   make a file buffer. */
	if (!relobj->head_buffer && !relobj->file_buffer_name) {
		relobj->file_buffer_name = WH_TempName(xpTemporary, "nsma");
		if (!relobj->file_buffer_name) return MK_OUT_OF_MEMORY;

		relobj->file_stream = XP_FileOpen(relobj->file_buffer_name,
										  xpTemporary,
										  XP_FILE_WRITE_BIN);
		if (!relobj->file_stream) {
			return MK_UNABLE_TO_OPEN_TMP_FILE;
		}
	}
  
	XP_ASSERT(relobj->head_buffer || relobj->file_stream);


	/* If this line will fit in the memory buffer, put it there.
	 */
	if (relobj->head_buffer &&
	    relobj->head_buffer_fp + length < relobj->head_buffer_size) {
		XP_MEMCPY(relobj->head_buffer + relobj->head_buffer_fp, line, length);
		relobj->head_buffer_fp += length;
	} else {
		/* Otherwise it won't fit; write it to the file instead. */

		/* If the file isn't open yet, open it, and dump the memory buffer
		   to it. */
		if (!relobj->file_stream) {
			if (!relobj->file_buffer_name) {
				relobj->file_buffer_name = WH_TempName (xpTemporary, "nsma");
			}
			if (!relobj->file_buffer_name) return MK_OUT_OF_MEMORY;

			relobj->file_stream = XP_FileOpen(relobj->file_buffer_name,
											xpTemporary,
											XP_FILE_WRITE_BIN);
			if (!relobj->file_stream) {
				return MK_UNABLE_TO_OPEN_TMP_FILE;
			}

			if (relobj->head_buffer && relobj->head_buffer_fp) {
				status = XP_FileWrite (relobj->head_buffer,
									   relobj->head_buffer_fp,
									   relobj->file_stream);
				if (status < 0) return status;
			}

			FREEIF(relobj->head_buffer);
			relobj->head_buffer_fp = 0;
			relobj->head_buffer_size = 0;
		}

		/* Dump this line to the file. */
		status = XP_FileWrite (line, length, relobj->file_stream);
		if (status < 0) return status;
	}

	return 0;
}




static int
real_write(MimeMultipartRelated* relobj, char* buf, int32 size)
{
  MimeObject* obj = (MimeObject*) relobj;
  void* closure = relobj->real_output_closure;

#ifdef MIME_DRAFTS
  if ( obj->options &&
	   obj->options->decompose_file_p &&
	   obj->options->decompose_file_output_fn )
	{
	  return obj->options->decompose_file_output_fn 
		(buf, size, obj->options->stream_closure);
	}
  else
#endif /* MIME_DRAFTS */
	{
	  if (!closure) {
		MimeObject* obj = (MimeObject*) relobj;
		closure = obj->options->stream_closure;
	  }
	  return relobj->real_output_fn(buf, size, closure);
	}
}


static int
push_tag(MimeMultipartRelated* relobj, const char* buf, int32 size)
{
	if (size + relobj->curtag_length > relobj->curtag_max) {
		relobj->curtag_max += 2 * size;
		if (relobj->curtag_max < 1024) relobj->curtag_max = 1024;
		if (!relobj->curtag) {
			relobj->curtag = (char*) XP_ALLOC(relobj->curtag_max);
		} else {
			relobj->curtag = (char*) XP_REALLOC(relobj->curtag,
												relobj->curtag_max);
		}
		if (!relobj->curtag) return MK_OUT_OF_MEMORY;
	}
	XP_MEMCPY(relobj->curtag + relobj->curtag_length, buf, size);
	relobj->curtag_length += size;
	return 0;
}

static int
flush_tag(MimeMultipartRelated* relobj)
{
	int length = relobj->curtag_length;
	char* buf;
	int status;
	
	if (relobj->curtag == NULL || length == 0) return 0;

	status = push_tag(relobj, "", 1);	/* Push on a trailing NULL. */
	if (status < 0) return status;
	buf = relobj->curtag;
	XP_ASSERT(*buf == '<' && buf[length - 1] == '>');
	while (*buf) {
		char c;
		char* absolute;
		char* part_url;
		char* ptr = buf;
		char *ptr2;
		XP_Bool isquote = FALSE;
		while (*ptr && *ptr != '=') ptr++;
		if (*ptr == '=') {
			ptr++;
			if (*ptr == '"') {
				isquote = TRUE;
				/* Take up the double quote and leading space here as well. */
				/* Safe because there's a '>' at the end */
				do {ptr++;} while (XP_IS_SPACE(*ptr));
			}
		}
		status = real_write(relobj, buf, ptr - buf);
		if (status < 0) return status;
		buf = ptr;
		if (!*buf) break;
		if (isquote) 
		{
			ptr = mime_strnchr(buf, '"', length - (buf - relobj->curtag));
		} else {
			for (ptr = buf; *ptr ; ptr++) {
				if (*ptr == '>' || XP_IS_SPACE(*ptr)) break;
			}
			XP_ASSERT(*ptr);
		}
		if (!ptr || !*ptr) break;

		while(buf < ptr)
		{
			/* ### mwelch	For each word in the value string, see if
			                the word is a cid: URL. If so, attempt to
							substitute the appropriate mailbox part URL in
							its place. */
			ptr2=buf; /* walk from the left end rightward */
			while((ptr2<ptr) && (!XP_IS_SPACE(*ptr2)))
				ptr2++;
			/* Compare the beginning of the word with "cid:". Yuck. */
			if (((ptr2 - buf) > 4) && 
				(buf[0]=='c' && buf[1]=='i' && buf[2]=='d' && buf[3]==':'))
			{
				/* Null terminate the word so we can... */
				c = *ptr2;
				*ptr2 = '\0';
				
				/* Construct a URL out of the word. */
				absolute = NET_MakeAbsoluteURL(relobj->base_url, buf);

				/* See if we have a mailbox part URL
				   corresponding to this cid. */
				part_url = absolute ? XP_Gethash(relobj->hash, buf, NULL)
					: NULL;
				FREEIF(absolute);
				
				/*If we found a mailbox part URL, write that out instead.*/
				if (part_url)
				{
					status = real_write(relobj, part_url, XP_STRLEN(part_url));
					if (status < 0) return status;
					buf = ptr2; /* skip over the cid: URL we substituted */
				}
				
				/* Restore the character that we nulled. */
				*ptr2 = c;
			}

			/* Advance to the beginning of the next word, or to
			   the end of the value string. */
			while((ptr2<ptr) && (XP_IS_SPACE(*ptr2)))
				ptr2++;

			/* Write whatever original text remains after
			   cid: URL substitution. */
			status = real_write(relobj, buf, ptr2-buf);
			if (status < 0) return status;
			buf = ptr2;
		}
	}
	if (buf && *buf) {
		status = real_write(relobj, buf, XP_STRLEN(buf));
		if (status < 0) return status;
	}
	relobj->curtag_length = 0;
	return 0;
}



static int
mime_multipart_related_output_fn(char* buf, int32 size, void *stream_closure)
{
	MimeMultipartRelated *relobj = (MimeMultipartRelated *) stream_closure;
	char* ptr;
	int32 delta;
	int status;
	while (size > 0) {
		if (relobj->curtag_length > 0) {
			ptr = mime_strnchr(buf, '>', size);
			if (!ptr) {
				return push_tag(relobj, buf, size);
			}
			delta = ptr - buf + 1;
			status = push_tag(relobj, buf, delta);
			if (status < 0) return status;
			status = flush_tag(relobj);
			if (status < 0) return status;
			buf += delta;
			size -= delta;
		}
		ptr = mime_strnchr(buf, '<', size);
		if (ptr && ptr - buf >= size) ptr = 0;
		if (!ptr) {
			return real_write(relobj, buf, size);
		}
		delta = ptr - buf;
		status = real_write(relobj, buf, delta);
		if (status < 0) return status;
		buf += delta;
		size -= delta;
		XP_ASSERT(relobj->curtag_length == 0);
		status = push_tag(relobj, buf, 1);
		if (status < 0) return status;
		XP_ASSERT(relobj->curtag_length == 1);
		buf++;
		size--;
	}
	return 0;
}


static int
MimeMultipartRelated_parse_eof (MimeObject *obj, XP_Bool abort_p)
{
	/* OK, all the necessary data has been collected.  We now have to spew out
	   the HTML.  We let it go through all the normal mechanisms (which
	   includes content-encoding handling), and intercept the output data to do
	   translation of the tags.  Whee. */

	MimeMultipartRelated *relobj = (MimeMultipartRelated *) obj;
	int status = 0;
	MimeObject *body;
	char* ct;
	const char* dct;

	status = ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_eof(obj, abort_p);
	if (status < 0) goto FAIL;

	if (!relobj->headobj) return 0;

	ct = (relobj->buffered_hdrs
		  ? MimeHeaders_get (relobj->buffered_hdrs, HEADER_CONTENT_TYPE,
							 TRUE, FALSE)
		  : 0);
	dct = (((MimeMultipartClass *) obj->class)->default_part_type);

	relobj->real_output_fn = obj->options->output_fn;
	relobj->real_output_closure = obj->options->output_closure;

	obj->options->output_fn = mime_multipart_related_output_fn;
	obj->options->output_closure = obj;

	body = mime_create(((ct && *ct) ? ct : (dct ? dct : TEXT_HTML)),
					   relobj->buffered_hdrs, obj->options);
	if (!body) {
		status = MK_OUT_OF_MEMORY;
		goto FAIL;
	}
	status = ((MimeContainerClass *) obj->class)->add_child(obj, body);
	if (status < 0) {
		mime_free(body);
		goto FAIL;
	}

#ifdef MIME_DRAFTS
	if ( obj->options && 
	   obj->options->decompose_file_p &&
	   obj->options->decompose_file_init_fn &&
	   (relobj->file_buffer_name || relobj->head_buffer))
	{
	  status = obj->options->decompose_file_init_fn ( obj->options->stream_closure,
													  relobj->buffered_hdrs );
	  if (status < 0) return status;
	}
#endif /* MIME_DRAFTS */


	/* Now that we've added this new object to our list of children,
	   start its parser going. */
	status = body->class->parse_begin(body);
	if (status < 0) goto FAIL;
	
	if (relobj->head_buffer) {
		/* Read it out of memory. */
		XP_ASSERT(!relobj->file_buffer_name && !relobj->file_stream);

		status = body->class->parse_buffer(relobj->head_buffer,
											   relobj->head_buffer_fp,
											   body);
	} else if (relobj->file_buffer_name) {
		/* Read it off disk.
		 */
		char *buf;
		int32 buf_size = 10 * 1024;  /* 10k; tune this? */

		XP_ASSERT(relobj->head_buffer_size == 0 &&
				  relobj->head_buffer_fp == 0);
		XP_ASSERT(relobj->file_buffer_name);
		if (!relobj->file_buffer_name) {
			status = -1;
			goto FAIL;
		}

		buf = (char *) XP_ALLOC(buf_size);
		if (!buf) {
			status = MK_OUT_OF_MEMORY;
			goto FAIL;
		}

		if (relobj->file_stream) {
			XP_FileClose(relobj->file_stream);
		}
		relobj->file_stream = XP_FileOpen(relobj->file_buffer_name,
										  xpTemporary,
										  XP_FILE_READ_BIN);
		if (!relobj->file_stream) {
			XP_FREE(buf);
			status = MK_UNABLE_TO_OPEN_TMP_FILE;
			goto FAIL;
		}

		while(1) {
			int32 rstatus = XP_FileRead(buf, buf_size - 1,
										relobj->file_stream);
			if (rstatus <= 0) {
				status = rstatus;
				break;
			} else {
				/* It would be really nice to be able to yield here, and let
				   some user events and other input sources get processed.
				   Oh well. */

				status = body->class->parse_buffer(buf, rstatus, body);
				if (status < 0) break;
			}
		}
		XP_FREE(buf);
	}

  if (status < 0) goto FAIL;

  /* Done parsing. */
  status = body->class->parse_eof(body, FALSE);
  if (status < 0) goto FAIL;
  status = body->class->parse_end(body, FALSE);
  if (status < 0) goto FAIL;

FAIL:

#ifdef MIME_DRAFTS
  if ( obj->options &&
	   obj->options->decompose_file_p &&
	   obj->options->decompose_file_close_fn &&
	   (relobj->file_buffer_name || relobj->head_buffer)) {
	status = obj->options->decompose_file_close_fn ( obj->options->stream_closure );
	if (status < 0) return status;
  }
#endif /* MIME_DRAFTS */

  relobj->headobj = NULL;
  obj->options->output_fn = relobj->real_output_fn;
  obj->options->output_closure = relobj->real_output_closure;

  return status;
}	




static int
MimeMultipartRelatedClassInitialize(MimeMultipartRelatedClass *class)
{
	MimeObjectClass    *oclass = (MimeObjectClass *) class;
	MimeMultipartClass *mclass = (MimeMultipartClass *) class;
	XP_ASSERT(!oclass->class_initialized);
	oclass->initialize       = MimeMultipartRelated_initialize;
	oclass->finalize         = MimeMultipartRelated_finalize;
	oclass->parse_eof        = MimeMultipartRelated_parse_eof;
	mclass->output_child_p   = MimeMultipartRelated_output_child_p;
	mclass->parse_child_line = MimeMultipartRelated_parse_child_line;
	return 0;
}
