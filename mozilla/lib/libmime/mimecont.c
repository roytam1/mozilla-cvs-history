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


#include "mimecont.h"

#define MIME_SUPERCLASS mimeObjectClass
MimeDefClass(MimeContainer, MimeContainerClass,
			 mimeContainerClass, &MIME_SUPERCLASS);

static int MimeContainer_initialize (MimeObject *);
static void MimeContainer_finalize (MimeObject *);
static int MimeContainer_add_child (MimeObject *, MimeObject *);
static int MimeContainer_parse_eof (MimeObject *, XP_Bool);
static int MimeContainer_parse_end (MimeObject *, XP_Bool);
static XP_Bool MimeContainer_displayable_inline_p (MimeObjectClass *class,
												   MimeHeaders *hdrs);

#if defined(DEBUG) && defined(XP_UNIX)
static int MimeContainer_debug_print (MimeObject *, FILE *, int32 depth);
#endif

static int
MimeContainerClassInitialize(MimeContainerClass *class)
{
  MimeObjectClass *oclass = (MimeObjectClass *) &class->object;

  XP_ASSERT(!oclass->class_initialized);
  oclass->initialize  = MimeContainer_initialize;
  oclass->finalize    = MimeContainer_finalize;
  oclass->parse_eof   = MimeContainer_parse_eof;
  oclass->parse_end   = MimeContainer_parse_end;
  oclass->displayable_inline_p = MimeContainer_displayable_inline_p;
  class->add_child    = MimeContainer_add_child;

#if defined(DEBUG) && defined(XP_UNIX)
  oclass->debug_print = MimeContainer_debug_print;
#endif
  return 0;
}


static int
MimeContainer_initialize (MimeObject *object)
{
  /* This is an abstract class; it shouldn't be directly instanciated. */
  XP_ASSERT(object->class != (MimeObjectClass *) &mimeContainerClass);

  return ((MimeObjectClass*)&MIME_SUPERCLASS)->initialize(object);
}

static void
MimeContainer_finalize (MimeObject *object)
{
  MimeContainer *cont = (MimeContainer *) object;

  /* Do this first so that children have their parse_eof methods called
	 in forward order (0-N) but are destroyed in backward order (N-0)
   */
  if (!object->closed_p)
	object->class->parse_eof (object, FALSE);
  if (!object->parsed_p)
	object->class->parse_end (object, FALSE);

  if (cont->children)
	{
	  int i;
	  for (i = cont->nchildren-1; i >= 0; i--)
		{
		  MimeObject *kid = cont->children[i];
		  if (kid)
			mime_free(kid);
		  cont->children[i] = 0;
		}
	  FREEIF(cont->children);
	  cont->nchildren = 0;
	}
  ((MimeObjectClass*)&MIME_SUPERCLASS)->finalize(object);
}

static int
MimeContainer_parse_eof (MimeObject *object, XP_Bool abort_p)
{
  MimeContainer *cont = (MimeContainer *) object;
  int status;

  /* We must run all of this object's parent methods first, to get all the
	 data flushed down its stream, so that the children's parse_eof methods
	 can access it.  We do not access *this* object again after doing this,
	 only its children.
   */
  status = ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_eof(object, abort_p);
  if (status < 0) return status;

  if (cont->children)
	{
	  int i;
	  for (i = 0; i < cont->nchildren; i++)
		{
		  MimeObject *kid = cont->children[i];
		  if (kid && !kid->closed_p)
			{
			  int status = kid->class->parse_eof(kid, abort_p);
			  if (status < 0) return status;
			}
		}
	}
  return 0;
}

static int
MimeContainer_parse_end (MimeObject *object, XP_Bool abort_p)
{
  MimeContainer *cont = (MimeContainer *) object;
  int status;

  /* We must run all of this object's parent methods first, to get all the
	 data flushed down its stream, so that the children's parse_eof methods
	 can access it.  We do not access *this* object again after doing this,
	 only its children.
   */
  status = ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_end(object, abort_p);
  if (status < 0) return status;

  if (cont->children)
	{
	  int i;
	  for (i = 0; i < cont->nchildren; i++)
		{
		  MimeObject *kid = cont->children[i];
		  if (kid && !kid->parsed_p)
			{
			  int status = kid->class->parse_end(kid, abort_p);
			  if (status < 0) return status;
			}
		}
	}
  return 0;
}

static int
MimeContainer_add_child (MimeObject *parent, MimeObject *child)
{
  MimeContainer *cont = (MimeContainer *) parent;
  MimeObject **old_kids, **new_kids;

  XP_ASSERT(parent && child);
  if (!parent || !child) return -1;

  old_kids = cont->children;
  new_kids = XP_ALLOC(sizeof(MimeObject *) * (cont->nchildren + 1));
  if (!new_kids) return MK_OUT_OF_MEMORY;
  
  if (cont->nchildren > 0)
	XP_MEMCPY(new_kids, old_kids, sizeof(MimeObject *) * cont->nchildren);
  new_kids[cont->nchildren] = child;
  XP_FREE(old_kids);
  cont->children = new_kids;
  cont->nchildren++;

  child->parent = parent;

  /* Copy this object's options into the child. */
  child->options = parent->options;

  return 0;
}

static XP_Bool
MimeContainer_displayable_inline_p (MimeObjectClass *class, MimeHeaders *hdrs)
{
  return TRUE;
}


#if defined(DEBUG) && defined(XP_UNIX)
static int
MimeContainer_debug_print (MimeObject *obj, FILE *stream, int32 depth)
{
  MimeContainer *cont = (MimeContainer *) obj;
  int i;
  char *addr = mime_part_address(obj);
  for (i=0; i < depth; i++)
	fprintf(stream, "  ");
  fprintf(stream, "<%s %s (%d kid%s) 0x%08X>\n",
		  obj->class->class_name,
		  addr ? addr : "???",
		  cont->nchildren, (cont->nchildren == 1 ? "" : "s"),
		  (uint32) cont);
  FREEIF(addr);

/*
  if (cont->nchildren > 0)
	fprintf(stream, "\n");
 */

  for (i = 0; i < cont->nchildren; i++)
	{
	  MimeObject *kid = cont->children[i];
	  int status = kid->class->debug_print (kid, stream, depth+1);
	  if (status < 0) return status;
	}

/*
  if (cont->nchildren > 0)
	fprintf(stream, "\n");
 */

  return 0;
}
#endif
