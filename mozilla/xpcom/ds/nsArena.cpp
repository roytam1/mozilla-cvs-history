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

#include "nsArena.h"
#include "nsCRT.h"

NS_IMETHODIMP
ArenaImpl::Init(PRUint32 aBlockSize)
{
  if (aBlockSize < NS_MIN_ARENA_BLOCK_SIZE) {
    aBlockSize = NS_DEFAULT_ARENA_BLOCK_SIZE;
  }
  PL_INIT_ARENA_POOL(&mPool, "nsIArena", aBlockSize);
  mBlockSize = aBlockSize;
  return NS_OK;
}

NS_IMPL_ISUPPORTS(ArenaImpl, nsIArena::GetIID())

ArenaImpl::~ArenaImpl()
{
  PL_FinishArenaPool(&mPool);
}

NS_IMETHODIMP_(void*)
ArenaImpl::Alloc(PRUint32 size)
{
  // Adjust size so that it's a multiple of sizeof(double)
  PRUint32 align = size & (sizeof(double) - 1);
  if (0 != align) {
    size += sizeof(double) - align;
  }

  void* p;
  PL_ARENA_ALLOCATE(p, &mPool, size);
  return p;
}

NS_METHOD
ArenaImpl::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;
  
  ArenaImpl* it = new ArenaImpl();
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(it);
  nsresult rv = it->QueryInterface(aIID, aResult);
  NS_RELEASE(it);
  return rv;
}

NS_BASE nsresult NS_NewHeapArena(nsIArena** aInstancePtrResult,
                                 PRUint32 aArenaBlockSize)
{
  nsresult rv;
  nsIArena* arena;
  rv = ArenaImpl::Create(NULL, nsIArena::GetIID(), (void**)&arena);
  if (NS_FAILED(rv)) return rv;
    
  rv = arena->Init(aArenaBlockSize);
  if (NS_FAILED(rv)) {
    NS_RELEASE(arena);
    return rv;
  }    
  
  *aInstancePtrResult = arena;
  return rv;
}
