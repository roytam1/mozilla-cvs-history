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

/* mimemsg.h --- definition of the MimeMessage class (see mimei.h)
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEMSG_H_
#define _MIMEMSG_H_

#include "mimecont.h"

/* The MimeMessage class implements the message/rfc822 and message/news
   MIME containers, which is to say, mail and news messages.
 */

typedef struct MimeMessageClass MimeMessageClass;
typedef struct MimeMessage      MimeMessage;

struct MimeMessageClass {
  MimeContainerClass container;
};

extern MimeMessageClass mimeMessageClass;

struct MimeMessage {
  MimeContainer container;		/* superclass variables */
  MimeHeaders *hdrs;			/* headers of this message */
  XP_Bool newline_p;			/* whether the last line ended in a newline */
  HG88918
};

#endif /* _MIMEMSG_H_ */
