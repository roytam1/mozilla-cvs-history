/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 *
 * Contributors:
 *     Douglas Turner <dougt@netscape.com>
 */

#ifndef nsTopProgressNotifier_h__
#define nsTopProgressNotifier_h__

#include "nsIXPInstallProgressNotifier.h"
#include "nsVector.h"


class nsTopProgressNotifier : public nsIXPInstallProgressNotifier
{
    public:

        nsTopProgressNotifier();
        virtual ~nsTopProgressNotifier();

        long RegisterNotifier(nsIXPInstallProgressNotifier * newNotifier);
        void UnregisterNotifier(long id);

        void BeforeJavascriptEvaluation(void);
        void AfterJavascriptEvaluation(void);
        void InstallStarted(const char* UIPackageName);
        long ItemScheduled(const  char* message );
        void InstallFinalization(const char* message, long itemNum, long totNum );
        void InstallAborted(void);
   
   private:
        nsVector *mNotifiers;

};

#endif
