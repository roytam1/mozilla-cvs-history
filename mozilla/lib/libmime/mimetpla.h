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

/* mimetpla.h --- definition of the MimeInlineTextPlain class (see mimei.h)
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMETPLA_H_
#define _MIMETPLA_H_

#include "mimetext.h"

/* The MimeInlineTextHTML class implements the text/plain MIME content type,
   and is also used for all otherwise-unknown text/ subtypes.
 */

typedef struct MimeInlineTextPlainClass MimeInlineTextPlainClass;
typedef struct MimeInlineTextPlain      MimeInlineTextPlain;

struct MimeInlineTextPlainClass {
  MimeInlineTextClass text;
};

extern MimeInlineTextPlainClass mimeInlineTextPlainClass;

struct MimeInlineTextPlain {
  MimeInlineText text;
};

#endif /* _MIMETPLA_H_ */
