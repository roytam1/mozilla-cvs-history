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

/* mimeleaf.c --- definition of the MimeLeaf class (see mimei.h)
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#include "mimeleaf.h"

#define MIME_SUPERCLASS mimeObjectClass
MimeDefClass(MimeLeaf, MimeLeafClass, mimeLeafClass, &MIME_SUPERCLASS);

static int MimeLeaf_initialize (MimeObject *);
static void MimeLeaf_finalize (MimeObject *);
static int MimeLeaf_parse_begin (MimeObject *);
static int MimeLeaf_parse_buffer (char *, int32, MimeObject *);
static int MimeLeaf_parse_line (char *, int32, MimeObject *);
static int MimeLeaf_parse_eof (MimeObject *, XP_Bool);
static XP_Bool MimeLeaf_displayable_inline_p (MimeObjectClass *class,
											  MimeHeaders *hdrs);

static int
MimeLeafClassInitialize(MimeLeafClass *class)
{
  MimeObjectClass *oclass = (MimeObjectClass *) class;
  XP_ASSERT(!oclass->class_initialized);
  oclass->initialize   = MimeLeaf_initialize;
  oclass->finalize     = MimeLeaf_finalize;
  oclass->parse_begin  = MimeLeaf_parse_begin;
  oclass->parse_buffer = MimeLeaf_parse_buffer;
  oclass->parse_line   = MimeLeaf_parse_line;
  oclass->parse_eof    = MimeLeaf_parse_eof;
  oclass->displayable_inline_p = MimeLeaf_displayable_inline_p;

  /* Default `parse_buffer' method is one which line-buffers the now-decoded
	 data and passes it on to `parse_line'.  (We snarf the implementation of
	 this method from our superclass's implementation of `parse_buffer', which
	 inherited it from MimeObject.)
   */
  class->parse_decoded_buffer =
	((MimeObjectClass*)&MIME_SUPERCLASS)->parse_buffer;

  return 0;
}


static int
MimeLeaf_initialize (MimeObject *obj)
{
  /* This is an abstract class; it shouldn't be directly instanciated. */
  XP_ASSERT(obj->class != (MimeObjectClass *) &mimeLeafClass);

  return ((MimeObjectClass*)&MIME_SUPERCLASS)->initialize(obj);
}


static void
MimeLeaf_finalize (MimeObject *object)
{
  MimeLeaf *leaf = (MimeLeaf *)object;
  object->class->parse_eof (object, FALSE);

  /* Free the decoder data, if it's still around.  It was probably freed
	 in MimeLeaf_parse_eof(), but just in case... */
  if (leaf->decoder_data)
	{
	  MimeDecoderDestroy(leaf->decoder_data, TRUE);
	  leaf->decoder_data = 0;
	}

  ((MimeObjectClass*)&MIME_SUPERCLASS)->finalize (object);
}


static int
MimeLeaf_parse_begin (MimeObject *obj)
{
  MimeLeaf *leaf = (MimeLeaf *) obj;
  MimeDecoderData *(*fn) (int (*) (const char*, int32, void*), void*) = 0;

  /* Initialize a decoder if necessary.
   */
  if (!obj->encoding)
	;
  else if (!strcasecomp(obj->encoding, ENCODING_BASE64))
	fn = &MimeB64DecoderInit;
  else if (!strcasecomp(obj->encoding, ENCODING_QUOTED_PRINTABLE))
	fn = &MimeQPDecoderInit;
  else if (!strcasecomp(obj->encoding, ENCODING_UUENCODE) ||
		   !strcasecomp(obj->encoding, ENCODING_UUENCODE2) ||
		   !strcasecomp(obj->encoding, ENCODING_UUENCODE3) ||
		   !strcasecomp(obj->encoding, ENCODING_UUENCODE4))
	fn = &MimeUUDecoderInit;

  if (fn)
	{
	  leaf->decoder_data =
		fn (/* The (int (*) ...) cast is to turn the `void' argument
			   into `MimeObject'. */
			((int (*) (const char *, int32, void *))
			 ((MimeLeafClass *)obj->class)->parse_decoded_buffer),
			obj);

	  if (!leaf->decoder_data)
		return MK_OUT_OF_MEMORY;
	}

  return ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_begin(obj);
}


static int
MimeLeaf_parse_buffer (char *buffer, int32 size, MimeObject *obj)
{
  MimeLeaf *leaf = (MimeLeaf *) obj;

  XP_ASSERT(!obj->closed_p);
  if (obj->closed_p) return -1;

  /* If we're not supposed to write this object, bug out now.
   */
  if (!obj->output_p ||
	  !obj->options ||
	  !obj->options->output_fn)
	return 0;

  if (leaf->decoder_data)
	return MimeDecoderWrite (leaf->decoder_data, buffer, size);
  else
	return ((MimeLeafClass *)obj->class)->parse_decoded_buffer (buffer, size,
																obj);
}

static int
MimeLeaf_parse_line (char *line, int32 length, MimeObject *obj)
{
  XP_ASSERT(0);
  /* This method shouldn't ever be called. */
  return -1;
}


static int
MimeLeaf_parse_eof (MimeObject *obj, XP_Bool abort_p)
{
  MimeLeaf *leaf = (MimeLeaf *) obj;
  if (obj->closed_p) return 0;

  /* Close off the decoder, to cause it to give up any buffered data that
	 it is still holding.
   */
  if (leaf->decoder_data)
	{
	  int status = MimeDecoderDestroy(leaf->decoder_data, FALSE);
	  leaf->decoder_data = 0;
	  if (status < 0) return status;
	}

  /* Now run the superclass's parse_eof, which will force out the line
	 buffer (which we may have just repopulated, above.)
   */
  return ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_eof (obj, abort_p);
}


static XP_Bool
MimeLeaf_displayable_inline_p (MimeObjectClass *class, MimeHeaders *hdrs)
{
  return TRUE;
}
