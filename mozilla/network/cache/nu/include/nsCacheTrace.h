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
#ifndef _nsCacheTrace_H_
#define _nsCacheTrace_H_
/* 
* nsCacheTrace
*
* Gagan Saksena
* 02/02/98
* 
*/
#ifndef CRLF
#	define CRLF "\r\n"
#endif

#include "prtypes.h"
#include "prlog.h"

class nsCacheTrace 
{

public:
		nsCacheTrace(void);
    static void Enable(PRBool bEnable);
    static PRBool IsEnabled(void);
    static void	Trace(const char* msg);
    static void	Traceln(const char* msg);
    static void Use(char* buffer);
    static char* m_TraceBuffer;
private:
    PRBool m_bEnabled;
};

inline void nsCacheTrace::Trace(const char* msg) 
{
	PR_ASSERT(msg);
    //Do log stuff here TODO
}

inline void nsCacheTrace::Traceln(const char* msg)
{
	Trace(msg);
	Trace(CRLF);
}

inline void nsCacheTrace::Use(char* buffer) 
{
	m_TraceBuffer = buffer;
}
#endif
