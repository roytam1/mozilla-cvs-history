/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef __msg_NewsHost__
#define __msg_NewsHost__ 1

#include "nsINNTPHost.h"

/* some platforms (like Windows and Mac) use a map file, because of
 * file name length limitations. */
#ifndef XP_UNIX
#if defined(XP_MAC) || defined(XP_WIN) 
#define USE_NEWSRC_MAP_FILE
#else
#error do_you_need_a_newsrc_map_file
#endif /* XP_MAC || XP_WIN */
#endif /* XP_UNIX */

NS_BEGIN_EXTERN_C

nsresult NS_NewNNTPHost(nsINNTPHost **aInstancePtrResult, const char *name, PRUint32 port);

NS_END_EXTERN_C

#endif /* __msg_NewsHost__ */
