/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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


/**
 * MODULE NOTES:
 * @update  gess 4/1/98
 * 
 */


#include "nsTokenHandler.h"
#include "nsDebug.h"
#include "nsIDTD.h"
#include "nsToken.h"
#include "nsIParser.h"

MOZ_DECL_CTOR_COUNTER(CTokenHandler);

/**
 *  
 *  
 *  @update  gess 4/2/98
 *  @param   
 *  @return  
 */
CTokenHandler::CTokenHandler(dispatchFP aFP,PRInt32 aType){

  MOZ_COUNT_CTOR(CTokenHandler);

  mType=aType;
  mFP=aFP;
}


/**
 *  
 *  
 *  @update  gess 4/2/98
 *  @param   
 *  @return  
 */
CTokenHandler::~CTokenHandler(){
  MOZ_COUNT_DTOR(CTokenHandler);
}


/**
 *  
 *  
 *  @update  gess 4/2/98
 *  @param   
 *  @return  
 */
PRInt32 CTokenHandler::GetTokenType(void){
  return mType;
}


/**
 *  
 *  
 *  @update  gess 4/2/98
 *  @param   
 *  @return  
 */
nsresult CTokenHandler::operator()(CToken* aToken,nsIDTD* aDTD){
  nsresult result=NS_OK;
  if((0!=aDTD) && (0!=mFP)) {
     result=(*mFP)(aToken,aDTD);
  }
  return result;
}


