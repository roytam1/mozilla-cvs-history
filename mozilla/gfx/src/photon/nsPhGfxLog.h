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
 
/*
 * Used for Logging in the gfx/src/photon directory.
 */ 

#include <prlog.h>
#include <signal.h>

extern PRLogModuleInfo *PhGfxLog;
extern unsigned char PhGfxLogState;
#undef PR_LOG

#define PR_LOG(_module,_level,_args) \
PR_BEGIN_MACRO \
  if (_module == nsnull) { \
    printf("nsPhGfxLog: defining PhGfxLog\n"); \
    PhGfxLog = PR_NewLogModule("PhGfxLog");   \
    PR_SetLogFile("logfile.txt"); \
  } \
  if ((_module)->level >= (_level)) \
    PR_LogPrint _args ;\
  else if (PhGfxLogState) \
    PR_LogPrint _args ;\
PR_END_MACRO


