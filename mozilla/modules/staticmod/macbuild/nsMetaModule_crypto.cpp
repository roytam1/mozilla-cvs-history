/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *   Christopher Seawood <cls@seawood.org> 
 *   Doug Turner <dougt@netscape.com>
 */

#include "nsError.h"
#include "nsIModule.h"
#include "nsIFile.h"
#include "nsIGenericFactory.h"
#include "prmem.h"

#define META_DESTRUCTOR_FUNC MetaModuleDestructorCrypto

#define NS_METAMODULE_NAME "nsMetaModuleCrypto"
#define NS_METAMODULE_DESC "Meta Component Crypto"
#define NS_METAMODULE_CID \
{ 0xcccc3240, 0x1dd1, 0x11b2, { 0xb2, 0x11, 0xc0, 0x85, 0x1f, 0xe2, 0xa6, 0xf6 } }
#define NS_METAMODULE_CONTRACTID "@mozilla.org/metamodule_crypto;1"


/*
 * NO USER EDITABLE PORTIONS AFTER THIS POINT
 *
 */

#define REGISTER_MODULE_USING(mod) { \
    nsCOMPtr<nsIModule> module; \
    mod##(aCompMgr, aPath, getter_AddRefs(module)); \
    module->RegisterSelf(aCompMgr, aPath, aRegistryLocation, aComponentType); \
}

struct nsModuleComponentInfoContainer {
    nsModuleComponentInfo		*list;
    PRUint32                    count;
};

extern "C" nsresult PKI_NSGetModule(nsIComponentManager *servMgr, nsIFile* aPath, nsIModule** return_cobj);
extern nsModuleComponentInfo*  PKI_NSGM_comps; extern PRUint32 PKI_NSGM_comp_count;
extern "C" nsresult NSS_NSGetModule(nsIComponentManager *servMgr, nsIFile* aPath, nsIModule** return_cobj);
extern nsModuleComponentInfo*  NSS_NSGM_comps; extern PRUint32 NSS_NSGM_comp_count;

static nsresult
NS_RegisterMetaModules(nsIComponentManager *aCompMgr,
                       nsIFile *aPath,
                       const char *aRegistryLocation,
                       const char *aComponentType)
{
    nsresult rv = NS_OK;
    
     REGISTER_MODULE_USING(PKI_NSGetModule);
     REGISTER_MODULE_USING(NSS_NSGetModule);
    
        {};
    
    return rv;
}

void META_DESTRUCTOR_FUNC(nsIModule *self, nsModuleComponentInfo *components)
{
    PR_Free(components);
}


class nsMetaModuleImpl : public nsISupports
{
public:
    nsMetaModuleImpl();
    virtual ~nsMetaModuleImpl();
    NS_DECL_ISUPPORTS
};

nsMetaModuleImpl::nsMetaModuleImpl()
{
    NS_INIT_ISUPPORTS();
}

nsMetaModuleImpl::~nsMetaModuleImpl()
{
}

NS_IMPL_ISUPPORTS1(nsMetaModuleImpl, nsISupports);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMetaModuleImpl)

static NS_METHOD nsMetaModuleRegistrationProc(nsIComponentManager *aCompMgr,
                                          nsIFile *aPath,
                                          const char *registryLocation,
                                          const char *componentType,
                                          const nsModuleComponentInfo *info)
{
    NS_RegisterMetaModules(aCompMgr, aPath, registryLocation, componentType);
    return NS_OK;
}

static NS_METHOD nsMetaModuleUnregistrationProc(nsIComponentManager *aCompMgr,
                                            nsIFile *aPath,
                                            const char *registryLocation,
                                            const nsModuleComponentInfo *info)
{
    return NS_OK;
}


static nsModuleComponentInfo components[] =
{
  { NS_METAMODULE_DESC,
    NS_METAMODULE_CID, 
    NS_METAMODULE_CONTRACTID, 
    nsMetaModuleImplConstructor,
    nsMetaModuleRegistrationProc,
    nsMetaModuleUnregistrationProc
  },
};

static nsModuleComponentInfoContainer componentsList[] = {
    { components, sizeof(components)/sizeof(components[0]) },
     { PKI_NSGM_comps, PKI_NSGM_comp_count },
     { NSS_NSGM_comps, NSS_NSGM_comp_count }, 
    { nsnull, 0 }
};

extern "C" NS_EXPORT nsresult NSGetModule(nsIComponentManager *servMgr,      
                                          nsIFile* location,                 
                                          nsIModule** result)                
{                                                                            
    nsModuleComponentInfo *outList = nsnull;
    nsModuleComponentInfoContainer *inList = componentsList;
    PRUint32 count = 0, i = 0, k=0, msize = sizeof(nsModuleComponentInfo);

    while (inList[i].list != nsnull) {
        count += inList[i].count;
        i++;
    }

    outList = (nsModuleComponentInfo *) PR_Calloc(count, sizeof(nsModuleComponentInfo));

    i = 0; k =0;
    while (inList[i].list != nsnull) {
        memcpy(&outList[k], inList[i].list, msize * inList[i].count);
        k+= inList[i].count;
        i++;
    }
    return NS_NewGenericModule(NS_METAMODULE_NAME, count, outList, nsnull, result);
}
