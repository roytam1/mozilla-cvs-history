/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 */


#include "CParserContext.h"
#include "nsToken.h"

MOZ_DECL_CTOR_COUNTER(CParserContext);
/**
 * Your friendly little constructor. Ok, it's not the friendly, but the only guy
 * using it is the parser.
 * @update	gess7/23/98
 * @param   aScanner
 * @param   aKey
 * @param   aListener
 */
CParserContext::CParserContext(nsScanner* aScanner,void* aKey,nsIStreamObserver* aListener) :
  mSourceType()
  //,mTokenDeque(gTokenDeallocator)
{
  MOZ_COUNT_CTOR(CParserContext);

  mScanner=aScanner;
  mKey=aKey;
  mPrevContext=0;
  mListener=aListener;
  NS_IF_ADDREF(mListener);
  mParseMode=eParseMode_unknown;
  mAutoDetectStatus=eUnknownDetect;
  mTransferBuffer=0;
  mDTD=0;
  mTransferBufferSize=eTransferBufferSize;
  mParserEnabled=PR_TRUE;
  mStreamListenerState=eNone;
  mMultipart=PR_TRUE;
  mContextType=eCTNone;
}


/**
 * Destructor for parser context
 * NOTE: DO NOT destroy the dtd here.
 * @update	gess7/11/98
 */
CParserContext::~CParserContext(){

  MOZ_COUNT_DTOR(CParserContext);

  if(mScanner)
    delete mScanner;

  if(mTransferBuffer)
    delete [] mTransferBuffer;

  NS_IF_RELEASE(mDTD);

  //Remember that it's ok to simply ingore the PrevContext.

}

