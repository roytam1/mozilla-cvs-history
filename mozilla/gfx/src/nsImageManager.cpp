/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 */

#include "nsIImageManager.h"
#include "libimg.h"
#include "nsCRT.h"
#include "nsImageNet.h"
#include "nsCOMPtr.h"

static NS_DEFINE_IID(kIImageManagerIID, NS_IIMAGEMANAGER_IID);

class ImageManagerImpl : public nsIImageManager {
public:
  ImageManagerImpl();
  virtual ~ImageManagerImpl();

  nsresult Init();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  virtual void SetCacheSize(PRInt32 aCacheSize);
  virtual PRInt32 GetCacheSize(void);
  virtual PRInt32 ShrinkCache(void);
  NS_IMETHOD FlushCache(void);
 // virtual nsImageType GetImageType(const char *buf, PRInt32 length);

private:
  nsCOMPtr<ilISystemServices> mSS;
};

// The singleton image manager
// This a service on XP_PC , mac and gtk. Need to convert 
// it to a service on all the remaining platforms.
static ImageManagerImpl*   gImageManager = nsnull;

ImageManagerImpl::ImageManagerImpl()
{
  NS_NewImageSystemServices(getter_AddRefs(mSS));
  IL_Init(mSS);
  IL_SetCacheSize(1024L * 1024L);
}

ImageManagerImpl::~ImageManagerImpl()
{
  IL_Shutdown();
  gImageManager = nsnull;
}

NS_IMPL_ISUPPORTS1(ImageManagerImpl, nsIImageManager); 

nsresult 
ImageManagerImpl::Init()
{
  return NS_OK;
}

void  
ImageManagerImpl::SetCacheSize(PRInt32 aCacheSize)
{
  IL_SetCacheSize(aCacheSize);
}

PRInt32 
ImageManagerImpl::GetCacheSize()
{
  return IL_GetCacheSize();
}
 
PRInt32 
ImageManagerImpl::ShrinkCache(void)
{
  return IL_ShrinkCache();
}

NS_IMETHODIMP
ImageManagerImpl::FlushCache(void)
{
  IL_FlushCache();
  return NS_OK;
}

extern "C" NS_GFX_(nsresult)
NS_NewImageManager(nsIImageManager **aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (nsnull == gImageManager) {
    gImageManager = new ImageManagerImpl();
  }
  if (nsnull == gImageManager) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return gImageManager->QueryInterface(kIImageManagerIID,
                                       (void **)aInstancePtrResult);
}

/* This is going to be obsolete, once ImageManagerImpl becomes a service on all platforms */
extern "C" NS_GFX_(void)
NS_FreeImageManager()
{
  /*Do not release it on platforms on which ImageManagerImpl is a service. 
  Need to remove this method, once ImageManagerImpl is converted to a service on all platforms.*/
  NS_IF_RELEASE(gImageManager);
}
