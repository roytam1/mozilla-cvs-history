/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef __ftpCore_h___
#define __ftpCore_h___

#include "nsError.h"
 
//////////////////////////////
//// FTP CODES      RANGE: 20-30
//////////////////////////////
#define NS_ERROR_FTP_LOGIN \
    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 21)

#define NS_ERROR_FTP_MODE \
    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 22)

#define NS_ERROR_FTP_CWD \
    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 23)

#define NS_ERROR_FTP_PASV \
    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 24)

#define NS_ERROR_FTP_DEL_DIR \
    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 25)

#define NS_ERROR_FTP_MKDIR \
    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 26)

#endif // __ftpCore_h___