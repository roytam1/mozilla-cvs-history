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

#include "nsGraphicsStateGTK.h"

//////////////////////////////////////////////////////////////////////////
nsGraphicsState::nsGraphicsState()
{
  mMatrix = nsnull;
  mClipRegion = nsnull;
  mColor = NS_RGB(0, 0, 0);
  mLineStyle = nsLineStyle_kSolid;
  mFontMetrics = nsnull;
}
//////////////////////////////////////////////////////////////////////////
nsGraphicsState::~nsGraphicsState()
{
  NS_IF_RELEASE(mClipRegion);
  NS_IF_RELEASE(mFontMetrics);
}
//////////////////////////////////////////////////////////////////////////
nsGraphicsStatePool::nsGraphicsStatePool()
{
	mFreeList = nsnull;
}
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
//
// Public nsGraphicsStatePool
//
//////////////////////////////////////////////////////////////////////////
/* static */ nsGraphicsState *
nsGraphicsStatePool::GetNewGS()
{
  nsGraphicsStatePool * thePool = PrivateGetPool();

  return thePool->PrivateGetNewGS();
}
//////////////////////////////////////////////////////////////////////////
/* static */ void
nsGraphicsStatePool::ReleaseGS(nsGraphicsState* aGS)
{
  nsGraphicsStatePool * thePool = PrivateGetPool();

  thePool->PrivateReleaseGS(aGS);
}
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//
// Private nsGraphicsStatePool
//
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
nsGraphicsStatePool *
nsGraphicsStatePool::gsThePool = nsnull;

//////////////////////////////////////////////////////////////////////////
nsGraphicsStatePool *
nsGraphicsStatePool::PrivateGetPool()
{
  if (nsnull == gsThePool)
  {
    gsThePool = new nsGraphicsStatePool();
  }

  return gsThePool;
}

//////////////////////////////////////////////////////////////////////////

nsGraphicsStatePool::~nsGraphicsStatePool()
{
	nsGraphicsState* gs = mFreeList;
	while (gs != nsnull) {
		nsGraphicsState* next = gs->mNext;
		delete gs;
		gs = next;
	}
}
//////////////////////////////////////////////////////////////////////////
nsGraphicsState *
nsGraphicsStatePool::PrivateGetNewGS()
{
	nsGraphicsState* gs = mFreeList;
	if (gs != nsnull) {
		mFreeList = gs->mNext;
		return gs;
	}
	return new nsGraphicsState;
}
//////////////////////////////////////////////////////////////////////////
void
nsGraphicsStatePool::PrivateReleaseGS(nsGraphicsState* aGS)
{
  //	aGS->Clear();
	aGS->mNext = mFreeList;
	mFreeList = aGS;
}
//////////////////////////////////////////////////////////////////////////

