/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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


#include "nsProxyEvent.h"
#include "nsProxyObjectManager.h"
#include "nsProxyEventPrivate.h"

#include "nsRepository.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"

#include "nsIAllocator.h"
#include "nsHashtable.h"


#include "nsIInterfaceInfoManager.h"
#include "xptcall.h"



static uint32 zero_methods_descriptor;



//////////////////////////////////////////////////////////////////////////////////////////////////
//  nsProxyEventClass
//////////////////////////////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kProxyEventClassIID, NS_PROXYEVENT_CLASS_IID);

NS_IMPL_ISUPPORTS(nsProxyEventClass, kProxyEventClassIID)

// static
nsProxyEventClass*
nsProxyEventClass::GetNewOrUsedClass(REFNSIID aIID)
{
    /* find in our hash table */
    
    nsProxyObjectManager *manager = nsProxyObjectManager::GetInstance();
    nsHashtable *iidToClassMap =  manager->GetIIDToProxyClassMap();


    nsProxyEventClass* clazz = NULL;
    nsIDKey key(aIID);

    if(iidToClassMap->Exists(&key))
    {
        clazz = (nsProxyEventClass*) iidToClassMap->Get(&key);
        NS_ADDREF(clazz);
    }
    else
    {
        nsIInterfaceInfoManager* iimgr;
        if(NULL != (iimgr = XPTI_GetInterfaceInfoManager()))
        {
            nsIInterfaceInfo* info;
            if(NS_SUCCEEDED(iimgr->GetInfoForIID(&aIID, &info)))
            {
                /* 
                   Check to see if IsISupportsDescendent 
                */
                nsIInterfaceInfo* oldest = info;
                nsIInterfaceInfo* parent;

                NS_ADDREF(oldest);
                while(NS_SUCCEEDED(oldest->GetParent(&parent)))
                {
                    NS_RELEASE(oldest);
                    oldest = parent;
                }

                PRBool IsISupportsDescendent = PR_FALSE;
                nsID* iid;
                if(NS_SUCCEEDED(oldest->GetIID(&iid))) 
                {
                    IsISupportsDescendent = iid->Equals(nsISupports::GetIID());
                    nsAllocator::Free(iid);
                }
                NS_RELEASE(oldest);
               
                NS_ASSERTION(IsISupportsDescendent,"!IsISupportsDescendent");

                if (IsISupportsDescendent)  
                {
                    clazz = new nsProxyEventClass(aIID, info);
                    if(!clazz->mDescriptors)
                        NS_RELEASE(clazz);  // sets clazz to NULL
                }
                NS_RELEASE(info);
            }
            NS_RELEASE(iimgr);
        }
    }
    return clazz;
}


nsProxyEventClass::nsProxyEventClass(REFNSIID aIID, nsIInterfaceInfo* aInfo)
: mInfo(aInfo),
  mIID(aIID),
  mDescriptors(NULL)
{
    NS_ADDREF(mInfo);

    NS_INIT_REFCNT();
    NS_ADDREF_THIS();

    /* add use to the used classes */
    nsIDKey key(aIID);

    nsProxyObjectManager *manager = nsProxyObjectManager::GetInstance();
    nsHashtable *iidToClassMap =  manager->GetIIDToProxyClassMap();

    iidToClassMap->Put(&key, this);

    uint16 methodCount;
    if(NS_SUCCEEDED(mInfo->GetMethodCount(&methodCount)))
    {
        if(methodCount)
        {
            int wordCount = (methodCount/32)+1;
            if(NULL != (mDescriptors = new uint32[wordCount]))
            {
                int i;
                // init flags to 0;
                for(i = wordCount-1; i >= 0; i--)
                    mDescriptors[i] = 0;
            }
        }
        else
        {
            mDescriptors = &zero_methods_descriptor;
        }
    }
}

nsProxyEventClass::~nsProxyEventClass()
{
    if(mDescriptors && mDescriptors != &zero_methods_descriptor)
        delete [] mDescriptors;

    nsIDKey key(mIID);
    
    nsProxyObjectManager *manager = nsProxyObjectManager::GetInstance();
    nsHashtable *iidToClassMap =  manager->GetIIDToProxyClassMap();
    iidToClassMap->Remove(&key);
    
    NS_RELEASE(mInfo);
}

nsProxyEventObject*
nsProxyEventClass::CallQueryInterfaceOnProxy(nsProxyEventObject* self, REFNSIID aIID)
{

    nsISupports* aInstancePtr;
    
    // The functions we will call: QueryInterface(REFNSIID aIID, void** aInstancePtr)

    nsXPTCMiniVariant *var = new nsXPTCMiniVariant[2];
    if (var == nsnull) return nsnull;
    
    (&var[0])->val.p     = (void*)&aIID;
    (&var[1])->val.p     = &aInstancePtr;

    nsIInterfaceInfoManager *iim = XPTI_GetInterfaceInfoManager();
    nsIInterfaceInfo *nsISupportsInfo;
    const nsXPTMethodInfo *mi;

    iim->GetInfoForName("nsISupports", &nsISupportsInfo);
    nsISupportsInfo->GetMethodInfo(0, &mi); // 0 is QueryInterface

    nsresult rv = self->CallMethod(0, mi, var);

    aInstancePtr =  (nsISupports*) *((void**)var[1].val.p);
    
    delete [] var;

    if (rv == NS_OK)
    {
        nsProxyEventObject* proxyObj = nsProxyEventObject::GetNewOrUsedProxy(self->GetQueue(), aInstancePtr, aIID);
        if(proxyObj)
        {
            return proxyObj;
        }
    }    
    
    return nsnull;
}


/***************************************************************************/
// This 'ProxyEventClassIdentity' class and singleton allow us to figure out if
// any given nsISupports* is implemented by a nsProxy object. This is done
// using a QueryInterface call on the interface pointer with our ID. If
// that call returns NS_OK and the pointer is to our singleton, then the
// interface must be implemented by a nsProxy object. NOTE: the
// 'ProxyEventClassIdentity' object is not a real XPCOM object and should not be
// used for anything else (hence it is declared in this implementation file).

/* eea90d45-b059-11d2-915e-c12b696c9333 */
#define NS_PROXYEVENT_IDENTITY_CLASS_IID \
{ 0xeea90d45, 0xb059, 0x11d2,                       \
  { 0x91, 0x5e, 0xc1, 0x2b, 0x69, 0x6c, 0x93, 0x33 } }

class ProxyEventClassIdentity
{
    // no instance methods...
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_PROXYEVENT_IDENTITY_CLASS_IID)

    static void* GetSingleton()
    {
        static ProxyEventClassIdentity* singleton = NULL;
        if(!singleton)
            singleton = new ProxyEventClassIdentity();
        return (void*) singleton;
    }
};

NS_IMETHODIMP
nsProxyEventClass::DelegatedQueryInterface(nsProxyEventObject* self,
                                          REFNSIID aIID,
                                          void** aInstancePtr)
{
    if(aIID.Equals(nsISupports::GetIID()))
    {
        nsProxyEventObject* root = self->GetRootProxyObject();
        *aInstancePtr = (void*) root;
        NS_ADDREF(root);
        return NS_OK;
    }
    else if(aIID.Equals(self->GetIID()))
    {
        *aInstancePtr = (void*) self;
        NS_ADDREF(self);
        return NS_OK;
    }
    else if(aIID.Equals(ProxyEventClassIdentity::GetIID()))
    {
        *aInstancePtr = ProxyEventClassIdentity::GetSingleton();
        return NS_OK;
    }
    else
    {
        *aInstancePtr = CallQueryInterfaceOnProxy(self, aIID);
        if (*aInstancePtr == nsnull)
            return NS_NOINTERFACE;
        else
            return NS_OK;
        
    }

    *aInstancePtr = NULL;
    return NS_NOINTERFACE;
}



nsProxyEventObject*
nsProxyEventClass::GetRootProxyObject(nsProxyEventObject* anObject)
{
    nsProxyEventObject* result = CallQueryInterfaceOnProxy(anObject, nsISupports::GetIID());
    return result ? result : anObject;
}