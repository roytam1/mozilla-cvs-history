/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/* Some simple smoke tests of the typelib loader. */

#include "nscore.h"

#include "nsISupports.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "xptinfo.h"

#include <stdio.h>

#include "../src/nsInterfaceInfo.h"

// This file expects the nsInterfaceInfoManager to be able to discover
// .xpt files corresponding to those in xpcom/idl.  Currently this
// means setting XPTDIR in the environment to some directory
// containing these files.

int main (int argc, char **argv) {
    int i;
    nsIID *iid1, *iid2, *iid3, *iid4, *iid5, *iid6;
    char *name1, *name2, *name3, *name4, *name5, *name6;
    nsIInterfaceInfo *info1, *info2, *info3, *info4, *info5, *info6;;

    nsIInterfaceInfoManager *iim = XPTI_GetInterfaceInfoManager();

    fprintf(stderr, "\ngetting iid for 'nsISupports'\n");
    iim->GetIIDForName("nsISupports", &iid1);
    iim->GetNameForIID(iid1, &name1);
    fprintf(stderr, "%s iid %s\n", name1, iid1->ToString());

    fprintf(stderr, "\ngetting iid for 'nsIBaseStream'\n");
    iim->GetIIDForName("nsIBaseStream", &iid2);
    iim->GetNameForIID(iid2, &name2);
    fprintf(stderr, "%s iid %s\n", name2, iid2->ToString());

    fprintf(stderr, "iid: %s, name: %s\n", iid1->ToString(), name1);
    fprintf(stderr, "iid: %s, name: %s\n", iid2->ToString(), name2);

    fprintf(stderr, "\ngetting info for iid2 from above\n");
    iim->GetInfoForIID(iid2, &info2);
#ifdef DEBUG
    ((nsInterfaceInfo *)info2)->print(stderr);
#endif

    fprintf(stderr, "\ngetting iid for 'nsIInputStream'\n");
    iim->GetIIDForName("nsIInputStream", &iid3);
    iim->GetNameForIID(iid3, &name3);
    fprintf(stderr, "%s iid %s\n", name3, iid2->ToString());
    iim->GetInfoForIID(iid3, &info3);
#ifdef DEBUG
    ((nsInterfaceInfo *)info3)->print(stderr);
#endif

    fprintf(stderr, "\ngetting info for name 'nsIBidirectionalEnumerator'\n");
    iim->GetInfoForName("nsIBidirectionalEnumerator", &info4);
#ifdef DEBUG
    ((nsInterfaceInfo *)info4)->print(stderr);
#endif

    fprintf(stderr, "\nparams work?\n");
    fprintf(stderr, "\ngetting info for name 'nsIServiceManager'\n");
    iim->GetInfoForName("nsIServiceManager", &info5);
#ifdef DEBUG
    ((nsInterfaceInfo *)info5)->print(stderr);
#endif
    
    uint16 methodcount;
    info5->GetMethodCount(&methodcount);
    const nsXPTMethodInfo *mi;
    for (i = 0; i < methodcount; i++) {
        info5->GetMethodInfo(i, &mi);
        char *methodname;
        fprintf(stderr, "method %d, name %s\n", i, mi->GetName());
    }

    // 7 is GetServiceWithListener, which has juicy params.
    info5->GetMethodInfo(7, &mi);
    uint8 paramcount = mi->GetParamCount();

    nsXPTParamInfo param2 = mi->GetParam(2);
    // should be IID for nsIShutdownListener
    nsIID *nsISL;
    info5->GetIIDForParam(&param2, &nsISL);
//      const nsIID *nsISL = param2.GetInterfaceIID(info5);
    fprintf(stderr, "iid assoc'd with param 2 of method 7 of GetServiceWithListener - %s\n", nsISL->ToString());
    // if we look up the name?
    char *nsISLname;
    iim->GetNameForIID(nsISL, &nsISLname);
    fprintf(stderr, "which is called %s\n", nsISLname);

    fprintf(stderr, "\nhow about one defined in a different typelib\n");
    nsXPTParamInfo param3 = mi->GetParam(3);
    // should be IID for nsIShutdownListener
    nsIID *nsISS;
    info5->GetIIDForParam(&param3, &nsISS);
//      const nsIID *nsISS = param3.GetInterfaceIID(info5);
    fprintf(stderr, "iid assoc'd with param 3 of method 7 of GetServiceWithListener - %s\n", nsISS->ToString());
    // if we look up the name?
    char *nsISSname;
    iim->GetNameForIID(nsISS, &nsISSname);
    fprintf(stderr, "which is called %s\n", nsISSname);

    return 0;
}    

