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

/*
 * Global NNTP-related functions and error conditions
 */

#ifndef MKNEWS_H
#define MKNEWS_H

#ifndef MKGETURL_H
#include "mkgeturl.h"
#endif /* MKGETURL_H */

#define MK_NNTP_RESPONSE_HELP 100

#define MK_NNTP_RESPONSE_POSTING_ALLOWED 200
#define MK_NNTP_RESPONSE_POSTING_DENIED 201

#define MK_NNTP_RESPONSE_DISCONTINUED 400

#define MK_NNTP_RESPONSE_COMMAND_UNKNOWN 500
#define MK_NNTP_RESPONSE_SYNTAX_ERROR 501
#define MK_NNTP_RESPONSE_PERMISSION_DENIED 502
#define MK_NNTP_RESPONSE_SERVER_ERROR 503

#define MK_NNTP_RESPONSE_ARTICLE_BOTH 220
#define MK_NNTP_RESPONSE_ARTICLE_HEAD 221
#define MK_NNTP_RESPONSE_ARTICLE_BODY 222
#define MK_NNTP_RESPONSE_ARTICLE_NONE 223
#define MK_NNTP_RESPONSE_ARTICLE_NO_GROUP 412
#define MK_NNTP_RESPONSE_ARTICLE_NO_CURRENT 420
#define MK_NNTP_RESPONSE_ARTICLE_NONEXIST 423
#define MK_NNTP_RESPONSE_ARTICLE_NOTFOUND 430

#define MK_NNTP_RESPONSE_GROUP_SELECTED   211
#define MK_NNTP_RESPONSE_GROUP_NO_GROUP   411

#define MK_NNTP_RESPONSE_IHAVE_OK         235
#define MK_NNTP_RESPONSE_IHAVE_ARTICLE    335
#define MK_NNTP_RESPONSE_IHAVE_NOT_WANTED 435
#define MK_NNTP_RESPONSE_IHAVE_FAILED     436
#define MK_NNTP_RESPONSE_IHAVE_REJECTED   437

#define MK_NNTP_RESPONSE_LAST_OK          223
#define MK_NNTP_RESPONSE_LAST_NO_GROUP    412
#define MK_NNTP_RESPONSE_LAST_NO_CURRENT  420
#define MK_NNTP_RESPONSE_LAST_NO_ARTICLE  422

#define MK_NNTP_RESPONSE_LIST_OK          215

#define MK_NNTP_RESPONSE_NEWGROUPS_OK     231

#define MK_NNTP_RESPONSE_NEWNEWS_OK       230

#define MK_NNTP_RESPONSE_NEXT_OK          223
#define MK_NNTP_RESPONSE_NEXT_NO_GROUP    412
#define MK_NNTP_RESPONSE_NEXT_NO_CURRENT  420
#define MK_NNTP_RESPONSE_NEXT_NO_ARTICLE  421

#define MK_NNTP_RESPONSE_POST_OK          240
#define MK_NNTP_RESPONSE_POST_SEND_NOW    340
#define MK_NNTP_RESPONSE_POST_DENIED      440
#define MK_NNTP_RESPONSE_POST_FAILED      441

#define MK_NNTP_RESPONSE_QUIT_OK          205

#define MK_NNTP_RESPONSE_SLAVE_OK         202

#define MK_NNTP_RESPONSE_CHECK_NO_ARTICLE 238
#define MK_NNTP_RESPONSE_CHECK_NO_ACCEPT  400
#define MK_NNTP_RESPONSE_CHECK_LATER      431
#define MK_NNTP_RESPONSE_CHECK_DONT_SEND  438
#define MK_NNTP_RESPONSE_CHECK_DENIED     480
#define MK_NNTP_RESPONSE_CHECK_ERROR      500

#define MK_NNTP_RESPONSE_XHDR_OK          221
#define MK_NNTP_RESPONSE_XHDR_NO_GROUP    412
#define MK_NNTP_RESPONSE_XHDR_NO_CURRENT  420
#define MK_NNTP_RESPONSE_XHDR_NO_ARTICLE  430
#define MK_NNTP_RESPONSE_XHDR_DENIED      502

#define MK_NNTP_RESPONSE_XOVER_OK         224
#define MK_NNTP_RESPONSE_XOVER_NO_GROUP   412
#define MK_NNTP_RESPONSE_XOVER_NO_CURRENT 420
#define MK_NNTP_RESPONSE_XOVER_DENIED     502

#define MK_NNTP_RESPONSE_XPAT_OK          221
#define MK_NNTP_RESPONSE_XPAT_NO_ARTICLE  430
#define MK_NNTP_RESPONSE_XPAT_DENIED      502

#define MK_NNTP_RESPONSE_AUTHINFO_OK      281
#define MK_NNTP_RESPONSE_AUTHINFO_CONT    381
#define MK_NNTP_RESPONSE_AUTHINFO_REQUIRE 480
#define MK_NNTP_RESPONSE_AUTHINFO_REJECT  482
#define MK_NNTP_RESPONSE_AUTHINFO_DENIED  502

#define MK_NNTP_RESPONSE_

#define MK_NNTP_RESPONSE_AUTHINFO_SIMPLE_OK      250
#define MK_NNTP_RESPONSE_AUTHINFO_SIMPLE_CONT    350
#define MK_NNTP_RESPONSE_AUTHINFO_SIMPLE_REQUIRE 450
#define MK_NNTP_RESPONSE_AUTHINFO_SIMPLE_REJECT  452



#define MK_NNTP_RESPONSE_TYPE_INFO    1
#define MK_NNTP_RESPONSE_TYPE_OK      2
#define MK_NNTP_RESPONSE_TYPE_CONT    3
#define MK_NNTP_RESPONSE_TYPE_CANNOT  4
#define MK_NNTP_RESPONSE_TYPE_ERROR   5

#define MK_NNTP_RESPONSE_TYPE(x) (x/100)
extern CONST char * NET_MKGetNewsHost ();
extern int net_InitializeNewsFeData (ActiveEntry * cur_entry);
extern void NET_InitNewsProtocol(void);

#endif /* MKNEWS_H */
