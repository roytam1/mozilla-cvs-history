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

#include "nsCalStatusCanvas.h"
#include "nsCalUICIID.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kCalStatusCanvasCID, NS_CAL_STATUSCANVAS_CID);

nsCalStatusCanvas :: nsCalStatusCanvas(nsISupports* outer) : nsXPFCCanvas(outer)
{
  NS_INIT_REFCNT();
}

nsCalStatusCanvas :: ~nsCalStatusCanvas()
{
}

nsresult nsCalStatusCanvas::QueryInterface(REFNSIID aIID, void** aInstancePtr)      
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 
  static NS_DEFINE_IID(kClassIID, kCalStatusCanvasCID);
  if (aIID.Equals(kClassIID)) {                                          
    *aInstancePtr = (void*) (nsCalStatusCanvas *)this;                                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                      
  if (aIID.Equals(kISupportsIID)) {                                      
    *aInstancePtr = (void*) (this);                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                      
  return (nsXPFCCanvas::QueryInterface(aIID, aInstancePtr));
}

NS_IMPL_ADDREF(nsCalStatusCanvas)
NS_IMPL_RELEASE(nsCalStatusCanvas)

nsresult nsCalStatusCanvas :: Init()
{
  return NS_OK;
}

