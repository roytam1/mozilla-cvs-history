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
  PRBool newline_p;			/* whether the last line ended in a newline */
  PRBool crypto_stamped_p;		/* whether the header of this message has been
								   emitted expecting its child to emit HTML
								   which says that it is xlated. */

  PRBool crypto_msg_signed_p;	/* What the emitted xlation-stamp *says*. */
  PRBool crypto_msg_encrypted_p;
  PRBool grabSubject;	/* Should we try to grab the subject of this message */
};

#endif /* _MIMEMSG_H_ */
