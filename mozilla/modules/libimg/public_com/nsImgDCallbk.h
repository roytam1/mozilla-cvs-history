/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsImgDCallbk_h___
#define nsImgDCallbk_h___

// XXX This file needs to move to libimg/src - dp

#include "nsIImgDCallbk.h"

/*---------------------------------------------*/
/*-----------------class-----------------------*/

class ImgDCallbk : public nsIImgDCallbk
{
public:
  ImgDCallbk(il_container *aContainer) {
    NS_INIT_ISUPPORTS();
    ilContainer = aContainer;
  };

  virtual ~ImgDCallbk();

  NS_DECL_ISUPPORTS

  NS_IMETHOD ImgDCBFlushImage();
  NS_IMETHOD ImgDCBImageSize();
  NS_IMETHOD ImgDCBResetPalette();
  NS_IMETHOD ImgDCBInitTransparentPixel();
  NS_IMETHOD ImgDCBDestroyTransparentPixel();
  NS_IMETHOD ImgDCBSetupColorspaceConverter();
  NS_IMETHOD ImgDCBCreateGreyScaleColorSpace();

  NS_IMETHOD_(void*) ImgDCBSetTimeout(TimeoutCallbackFunction func,
                                      void* closure, PRUint32 msecs);
  NS_IMETHOD ImgDCBClearTimeout(void *timer_id);


  /* callbacks from the decoder */
  NS_IMETHOD ImgDCBHaveHdr(int destwidth, int destheight);
  NS_IMETHOD ImgDCBHaveRow(PRUint8*, PRUint8*,
                           int, int, int, int,
                           PRUint8 , int);

  NS_IMETHOD ImgDCBHaveImageFrame();
  NS_IMETHOD ImgDCBHaveImageAll();
  NS_IMETHOD ImgDCBError();

  NS_IMETHODIMP CreateInstance(const nsCID &aClass,
                               il_container* ic,
                               const nsIID &aIID,
                               void **ppv) ;

  il_container *GetContainer() {
    return ilContainer;
  };

  il_container *SetContainer(il_container *ic) {
    ilContainer=ic;
    return ic;
  };

private:
  il_container* ilContainer;
}; 

#endif /* nsImgDCallbk_h___ */
