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

#ifndef _MIMEPBUF_H_
#define _MIMEPBUF_H_

#include "mimei.h"
#include "mimerosetta.h"

/* This file provides the ability to save up the entire contents of a MIME
   object (of arbitrary size), and then emit it all at once later.  The
   buffering is done in an efficient way that works well for both very large
   and very small objects.

   This is used in two places:

   = The implementation of multipart/alternative uses this code to do a
     one-part-lookahead.  As it traverses its children, it moves forward
     until it finds a part which cannot be displayed; and then it displays
     the *previous* part (the last which *could* be displayed.)  This code
     is used to hold the previous part until it is needed.
*/

#ifdef MOZ_SECURITY
HG37486
#endif

/* An opaque object used to represent the buffered data.
 */
typedef struct MimePartBufferData MimePartBufferData;

/* Create an empty part buffer object.
 */
extern MimePartBufferData *MimePartBufferCreate (void);

/* Assert that the buffer is now full (EOF has been reached on the current
   part.)  This will free some resources, but leaves the part in the buffer.
   After calling MimePartBufferReset, the buffer may be used to store a
   different object.
 */
void MimePartBufferClose (MimePartBufferData *data);

/* Reset a part buffer object to the default state, discarding any currently-
   buffered data.
 */
extern void MimePartBufferReset (MimePartBufferData *data);

/* Free the part buffer itself, and discard any buffered data.
 */
extern void MimePartBufferDestroy (MimePartBufferData *data);

/* Push a chunk of a MIME object into the buffer.
 */
extern int MimePartBufferWrite (MimePartBufferData *data,
								const char *buf, PRInt32 size);

/* Read the contents of the buffer back out.  This will invoke the provided
   read_fn with successive chunks of data until the buffer has been drained.
   The provided function may be called once, or multiple times.
 */
extern int
MimePartBufferRead (MimePartBufferData *data,
					int (*read_fn) (char *buf, PRInt32 size, void *closure),
					void *closure);

#endif /* _MIMEPBUF_H_ */
