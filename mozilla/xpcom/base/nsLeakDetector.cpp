/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Patrick C. Beard <beard@netscape.com>
 */

#if defined(GC_LEAK_DETECTOR)

#include "nsLeakDetector.h"
#include "nsComponentManager.h"
#include "nsIGenericFactory.h"
#include "nsILeakDetector.h"

#include <stdio.h>
#include <time.h>

extern "C" {
extern FILE *GC_stdout, *GC_stderr;
extern void GC_gcollect(void);
extern void GC_clear_roots(void);
}

static nsresult nextLeakFile()
{
	if (GC_stderr != NULL)
		fclose(GC_stderr);
	
	// generate a time stamped report name.
	time_t timer;
	time(&timer);
	tm* now = localtime(&timer);
	
	char reportName[256];
	sprintf(reportName, "Leaks%02d%02d%02d",
			now->tm_hour, now->tm_min, now->tm_sec);
	GC_stderr = fopen(reportName, "w");
	
	return NS_OK;
}

class nsLeakDetector : public nsILeakDetector {
public:
	nsLeakDetector();
	virtual ~nsLeakDetector();
	
	NS_DECL_ISUPPORTS	
	NS_DECL_NSILEAKDETECTOR
};

NS_IMPL_ISUPPORTS1(nsLeakDetector, nsILeakDetector)

nsLeakDetector::nsLeakDetector() {}
nsLeakDetector::~nsLeakDetector() {}

NS_METHOD nsLeakDetector::DumpLeaks()
{
	GC_gcollect();

	return nextLeakFile();
}

#define NS_CLEAKDETECTOR_CID_STR "bb1ba360-1dd1-11b2-b30e-aa2314429f54"
#define NS_CLEAKDETECTOR_CID {0xbb1ba360, 0x1dd1, 0x11b2, {0xb3, 0x0e, 0xaa, 0x23, 0x14, 0x42, 0x9f, 0x54}}
#define NS_CLEAKDETECTOR_PROGID "component://netscape/xpcom/leakdetector"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsLeakDetector)

static NS_DEFINE_CID(kCLeakDetectorCID, NS_CLEAKDETECTOR_CID);

nsresult NS_InitLeakDetector()
{
	nsresult rv;

	// open the first leak file.
	rv = nextLeakFile();
	if (rv != NS_OK)
		return rv;

	// create a generic factory for the leak detector.
	nsCOMPtr<nsIGenericFactory> factory;
	rv = NS_NewGenericFactory(getter_AddRefs(factory), &nsLeakDetectorConstructor);
	if (rv != NS_OK)
		return rv;

	// register this factory with the component manager.
	rv = nsComponentManager::RegisterFactory(kCLeakDetectorCID, "LeakDetector", NS_CLEAKDETECTOR_PROGID, factory, PR_TRUE);
	return rv;
}

nsresult NS_ShutdownLeakDetector()
{
	// Run a collection to get unreferenced leaks.
	GC_gcollect();

#if 0
	nextLeakFile();
	if (GC_stdout != NULL) {
		fprintf(GC_stdout, "ShutDown Leaks\n");
		GC_clear_roots();
		GC_gcollect();
	}
#endif

	return NS_OK;	
}

#endif /* defined(GC_LEAK_DETECTOR) */
