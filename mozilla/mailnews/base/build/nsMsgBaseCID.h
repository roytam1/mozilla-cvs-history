/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsMessageBaseCID_h__
#define nsMessageBaseCID_h__

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIComponentManager.h"

#define NS_MSGRFC822PARSER_CID                    \
{ /* 26D71620-7421-11d2-804A-006008128C4E */      \
 0x26d71620, 0x7421, 0x11d2,                      \
 {0x80, 0x4a, 0x0, 0x60, 0x8, 0x12, 0x8c, 0x4e}}

#define NS_MSGFOLDEREVENT_CID				              \
{ /* FBFEBE7A-C1DD-11d2-8A40-0060B0FC04D2 */      \
 0xfbfebe7a, 0xc1dd, 0x11d2,                      \
 {0x8a, 0x40, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2}}

#define NS_MSGGROUPRECORD_CID                     \
{ /* a8f54ee0-d292-11d2-b7f6-00805f05ffa5 */      \
 0xa8f54ee0, 0xd292, 0x11d2,                      \
 {0xb7, 0xf6, 0x00, 0x80, 0x5f, 0x05, 0xff, 0xa5}}


#endif // nsMessageBaseCID_h__
