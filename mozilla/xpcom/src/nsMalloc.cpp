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

////////////////////////////////////////////////////////////////////////////////
// Implementation of nsIMalloc using NSPR
////////////////////////////////////////////////////////////////////////////////

#include "nsMalloc.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIMallocIID, NS_IMALLOC_IID);

nsMalloc::nsMalloc(nsISupports* outer)
{
    NS_INIT_AGGREGATED(outer);
}

nsMalloc::~nsMalloc(void)
{
}

NS_IMPL_AGGREGATED(nsMalloc);

NS_METHOD
nsMalloc::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr) 
{
    if (NULL == aInstancePtr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    }                                                                      
    if (aIID.Equals(kIMallocIID) || 
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) this; 
        AddRef(); 
        return NS_OK; 
    } 
    return NS_NOINTERFACE;
}

NS_COM NS_METHOD
nsMalloc::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    if (outer && !aIID.Equals(kISupportsIID))
        return NS_NOINTERFACE;   // XXX right error?
    nsMalloc* mm = new nsMalloc(outer);
    if (mm == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    mm->AddRef();
    if (aIID.Equals(kISupportsIID))
        *aInstancePtr = mm->GetInner();
    else
        *aInstancePtr = mm;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

NS_METHOD_(void*)
nsMalloc::Alloc(PRUint32 size)
{
    return PR_Malloc(size);
}

NS_METHOD_(void*)
nsMalloc::Realloc(void* ptr, PRUint32 size)
{
    return PR_Realloc(ptr, size);
}

NS_METHOD_(void)
nsMalloc::Free(void* ptr)
{
    PR_Free(ptr);
}

NS_METHOD_(PRInt32)
nsMalloc::GetSize(void* ptr)
{
    return -1;
}

NS_METHOD_(PRBool)
nsMalloc::DidAlloc(void* ptr)
{
    return PR_TRUE;
}

// For the definition of CallCacheFlushers()
#ifdef XP_MAC
#    include "MacMemAllocator.h"
#endif

NS_METHOD_(void)
nsMalloc::HeapMinimize(void)
{
#ifdef XP_MAC
    // something wonderful
    CallCacheFlushers(0x7fffffff);
#endif
}

////////////////////////////////////////////////////////////////////////////////
