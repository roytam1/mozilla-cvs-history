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

#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsGfxCIID.h"
#include "nsFontMetricsPh.h"
#include "nsRenderingContextPh.h"
#include "nsImagePh.h"
#include "nsDeviceContextPh.h"
#include "nsRegionPh.h"
#include "nsBlender.h"
#include "nsDeviceContextSpecPh.h"
#include "nsDeviceContextSpecFactoryP.h"

#include "nsPhGfxLog.h"

static NS_DEFINE_IID(kCFontMetrics, NS_FONT_METRICS_CID);
static NS_DEFINE_IID(kCRenderingContext, NS_RENDERING_CONTEXT_CID);
static NS_DEFINE_IID(kCImage, NS_IMAGE_CID);
static NS_DEFINE_IID(kCDeviceContext, NS_DEVICE_CONTEXT_CID);
static NS_DEFINE_IID(kCRegion, NS_REGION_CID);
static NS_DEFINE_IID(kCBlender, NS_BLENDER_CID);

static NS_DEFINE_IID(kCDeviceContextSpec, NS_DEVICE_CONTEXT_SPEC_CID);
static NS_DEFINE_IID(kCDeviceContextSpecFactory, NS_DEVICE_CONTEXT_SPEC_FACTORY_CID);

static NS_DEFINE_IID(kCDrawingSurface, NS_DRAWING_SURFACE_CID);

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);

class nsGfxFactoryPh : public nsIFactory
{   
  public:   
    // nsISupports methods   
    NS_IMETHOD QueryInterface(const nsIID &aIID,    
                                       void **aResult);   
    NS_IMETHOD_(nsrefcnt) AddRef(void);   
    NS_IMETHOD_(nsrefcnt) Release(void);   

    // nsIFactory methods   
    NS_IMETHOD CreateInstance(nsISupports *aOuter,   
                                       const nsIID &aIID,   
                                       void **aResult);   

    NS_IMETHOD LockFactory(PRBool aLock);   

    nsGfxFactoryPh(const nsCID &aClass);   
    ~nsGfxFactoryPh();   

  private:   
    nsrefcnt  mRefCnt;   
    nsCID     mClassID;
};   

nsGfxFactoryPh::nsGfxFactoryPh(const nsCID &aClass)   
{   
  mRefCnt = 0;
  mClassID = aClass;
}   

nsGfxFactoryPh::~nsGfxFactoryPh()   
{   
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::~nsGfxFactoryPh Destructor\n"));

  NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");   
}   

nsresult nsGfxFactoryPh::QueryInterface(const nsIID &aIID,   
                                         void **aResult)   
{   
  if (aResult == NULL) {   
    return NS_ERROR_NULL_POINTER;   
  }   

  // Always NULL result, in case of failure   
  *aResult = NULL;   

  if (aIID.Equals(kISupportsIID)) {   
    *aResult = (void *)(nsISupports*)this;   
  } else if (aIID.Equals(kIFactoryIID)) {   
    *aResult = (void *)(nsIFactory*)this;   
  }   

  if (*aResult == NULL) {   
    return NS_NOINTERFACE;   
  }   

  AddRef(); // Increase reference count for caller   
  return NS_OK;   
}   

nsrefcnt nsGfxFactoryPh::AddRef()   
{   
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::AddRef\n"));
  return ++mRefCnt;   
}   

nsrefcnt nsGfxFactoryPh::Release()   
{   
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::Release\n"));

  if (--mRefCnt == 0) {   
    delete this;   
    return 0; // Don't access mRefCnt after deleting!   
  }   
  return mRefCnt;   
}  

nsresult nsGfxFactoryPh::CreateInstance(nsISupports *aOuter,  
                                          const nsIID &aIID,  
                                          void **aResult)  
{  
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::CreateInstance\n"));

  if (aResult == NULL) {  
    return NS_ERROR_NULL_POINTER;  
  }  

  *aResult = NULL;  
  
  nsISupports *inst = nsnull;

  if (mClassID.Equals(kCFontMetrics))
  {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::CreateInstance asking for nsFontMetricsPh.\n"));
    nsFontMetricsPh* fm;
    NS_NEWXPCOM(fm, nsFontMetricsPh);
    inst = (nsISupports *)fm;
  }
  else if (mClassID.Equals(kCDeviceContext)) {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::CreateInstance asking for nsDeviceContextPh.\n"));
    nsDeviceContextPh* dc;
    NS_NEWXPCOM(dc, nsDeviceContextPh);
    inst = (nsISupports *)dc;
  }
  else if (mClassID.Equals(kCRenderingContext)) {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::CreateInstance asking for nsRenderingContextPh.\n"));
    nsRenderingContextPh*  rc;
    NS_NEWXPCOM(rc, nsRenderingContextPh);
    inst = (nsISupports *)((nsIRenderingContext*)rc);
  }
  else if (mClassID.Equals(kCImage)) {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::CreateInstance asking for nsImagePh.\n"));
    nsImagePh* image;
    NS_NEWXPCOM(image, nsImagePh);
    inst = (nsISupports *)image;
  }
  else if (mClassID.Equals(kCRegion)) {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::CreateInstance asking for nsRegionPh.\n"));
    nsRegionPh*  region;
    NS_NEWXPCOM(region, nsRegionPh);
    inst = (nsISupports *)region;
  }
  else if (mClassID.Equals(kCBlender)) {
    inst = (nsISupports *)new nsBlender;
  }
  else if (mClassID.Equals(kCDeviceContextSpec)) {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::CreateInstance asking for nsDeviceContextSpecPh.\n"));
    nsDeviceContextSpecPh* dcs;
    NS_NEWXPCOM(dcs, nsDeviceContextSpecPh);
    inst = (nsISupports *)dcs;
  }
  else if (mClassID.Equals(kCDeviceContextSpecFactory)) {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::CreateInstance asking for nsDeviceContextSpecFactoryPh.\n"));
    nsDeviceContextSpecFactoryPh* dcs;
    NS_NEWXPCOM(dcs, nsDeviceContextSpecFactoryPh);
    inst = (nsISupports *)dcs;
  }

  if (inst == NULL)
  {  
    PR_LOG(PhGfxLog, PR_LOG_ERROR,("nsGfxFactoryPh::CreateInstance Failed.\n"));
    return NS_ERROR_OUT_OF_MEMORY;  
  }  

  nsresult res = inst->QueryInterface(aIID, aResult);

  if (res != NS_OK) {  
    // We didn't get the right interface, so clean up  
    PR_LOG(PhGfxLog, PR_LOG_ERROR,("nsGfxFactoryPh::CreateInstance Did not get the right interface, Failed.\n"));
    delete inst;  
  }  
//  else {
//    inst->Release();
//  }

  return res;  
}  

nsresult nsGfxFactoryPh::LockFactory(PRBool aLock)  
{  
 PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::LockFactory - Not Implmented\n"));

  // Not implemented in simplest case.  
  return NS_OK;
}  

// return the proper factory to the caller
extern "C" NS_GFXNONXP nsresult NSGetFactory(nsISupports* servMgr,
                                             const nsCID &aClass,
                                             const char *aClassName,
                                             const char *aProgID,
                                             nsIFactory **aFactory)
{
 PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsGfxFactoryPh::NSGetFactory\n"));

  if (nsnull == aFactory) {
    return NS_ERROR_NULL_POINTER;
  }

  *aFactory = new nsGfxFactoryPh(aClass);

  if (nsnull == aFactory) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return (*aFactory)->QueryInterface(kIFactoryIID, (void**)aFactory);
}
