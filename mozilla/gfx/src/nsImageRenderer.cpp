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

#include "libimg.h"
#include "ilIImageRenderer.h"
#include "nsIImage.h"
#include "nsIRenderingContext.h"
#include "ni_pixmp.h"
#include "il_util.h"
#include "nsGfxCIID.h"
#include "nsIDeviceContext.h"

static NS_DEFINE_IID(kIImageRendererIID, IL_IIMAGERENDERER_IID);

class ImageRendererImpl : public ilIImageRenderer {
public:
  static IL_ColorSpace *sPseudoColorSpace;
  
  ImageRendererImpl();
  ~ImageRendererImpl();

  NS_DECL_ISUPPORTS

  virtual void NewPixmap(void* aDisplayContext, 
			 PRInt32 aWidth, PRInt32 aHeight, 
			 IL_Pixmap* aImage, IL_Pixmap* aMask);

  virtual void UpdatePixmap(void* aDisplayContext, 
			    IL_Pixmap* aImage, 
			    PRInt32 aXOffset, PRInt32 aYOffset, 
			    PRInt32 aWidth, PRInt32 aHeight);

  virtual void ControlPixmapBits(void* aDisplayContext, 
				 IL_Pixmap* aImage, PRUint32 aControlMsg);

  virtual void DestroyPixmap(void* aDisplayContext, IL_Pixmap* aImage);
  
  virtual void DisplayPixmap(void* aDisplayContext, 
			     IL_Pixmap* aImage, IL_Pixmap* aMask, 
			     PRInt32 aX, PRInt32 aY, 
			     PRInt32 aXOffset, PRInt32 aYOffset, 
			     PRInt32 aWidth, PRInt32 aHeight);

  virtual void DisplayIcon(void* aDisplayContext, 
			   PRInt32 aX, PRInt32 aY, PRUint32 aIconNumber);

  virtual void GetIconDimensions(void* aDisplayContext, 
				 PRInt32 *aWidthPtr, PRInt32 *aHeightPtr, 
				 PRUint32 aIconNumber);
};

IL_ColorSpace *ImageRendererImpl::sPseudoColorSpace = nsnull;

ImageRendererImpl::ImageRendererImpl()
{
  NS_INIT_REFCNT();
}

ImageRendererImpl::~ImageRendererImpl()
{
}

NS_IMPL_ISUPPORTS(ImageRendererImpl, kIImageRendererIID)


void 
ImageRendererImpl::NewPixmap(void* aDisplayContext, 
			     PRInt32 aWidth, PRInt32 aHeight, 
			     IL_Pixmap* aImage, IL_Pixmap* aMask)
{
    nsIRenderingContext *rc = (nsIRenderingContext *)aDisplayContext;
    nsIImage  *img;
    nsresult  rv;

    static NS_DEFINE_IID(kImageCID, NS_IMAGE_CID);
    static NS_DEFINE_IID(kImageIID, NS_IIMAGE_IID);

    rv = NSRepository::CreateInstance(kImageCID, nsnull, kImageIID, (void **)&img);

    if (NS_OK != rv)
        return;

    PRInt32 depth;
    if (aImage->header.color_space->pixmap_depth == 8) {
        depth = 8;
    }
    else {
        depth = 24;
    }

    img->Init(aWidth, aHeight, depth, 
	      (aMask == nsnull) ? nsMaskRequirements_kNoMask : 
	                          nsMaskRequirements_kNeeds1Bit);

    aImage->bits = img->GetBits();
    aImage->client_data = img;
    // We don't need to add a reference here, because we're already holding
    // a reference, because of the call above to the repository to create the
    // nsIImage*
    aImage->header.width = aWidth;
    aImage->header.height = aHeight;
    aImage->header.widthBytes = img->GetLineStride();

    if (aMask) {
        aMask->bits = img->GetAlphaBits();
        aMask->client_data = img;
        // Make sure you add another reference here, because when the mask's
        // pixmap is destroyed the reference will be released
        NS_ADDREF(img);
        aMask->header.width = aWidth;
        aMask->header.height = aHeight;
    }

    if (aImage->header.color_space->pixmap_depth == 8) {
        IL_ColorMap *cmap;
        if (sPseudoColorSpace == nsnull) {
            cmap = IL_NewCubeColorMap(nsnull, 0, 216);
            
            if (cmap != nsnull) {
                sPseudoColorSpace = IL_CreatePseudoColorSpace(cmap, 8, 8);
                
                if (sPseudoColorSpace == nsnull) {
                    IL_DestroyColorMap(cmap);
                    
                    // XXX We should do something here
                    return;
                }
            }
            else {
                // XXX We should do something here
                return;
            }
        }
        IL_AddRefToColorSpace(sPseudoColorSpace);
        IL_ReleaseColorSpace(aImage->header.color_space);
        aImage->header.color_space = sPseudoColorSpace;

        cmap = &sPseudoColorSpace->cmap;
        nsColorMap *nscmap = img->GetColorMap();
        PRUint8 *mapptr = nscmap->Index;
        int i;
                
        for (i=0; i < cmap->num_colors; i++) {
          *mapptr++ = cmap->map[i].red;
          *mapptr++ = cmap->map[i].green;
          *mapptr++ = cmap->map[i].blue;
        }

        nsIDeviceContext  *dx = rc->GetDeviceContext();
        img->ImageUpdated(dx, nsImageUpdateFlags_kColorMapChanged, nsnull);
        NS_IF_RELEASE(dx);
                
        if (aImage->header.transparent_pixel) {
            PRUint8 red, green, blue;
            PRUint8 *lookup_table = (PRUint8 *)aImage->header.color_space->cmap.table;
            red = aImage->header.transparent_pixel->red;
            green = aImage->header.transparent_pixel->green;
            blue = aImage->header.transparent_pixel->blue;
            aImage->header.transparent_pixel->index =  
              lookup_table[((red >> 3) << 10) |
                          ((green >> 3) << 5) |
                          (blue >> 3)];
        }
    }
    else {
        IL_RGBBits colorRGBBits;
        IL_ColorSpace *colorSpace;
        
        colorRGBBits.red_shift = 16;  
        colorRGBBits.red_bits = 8;
        colorRGBBits.green_shift = 8;
        colorRGBBits.green_bits = 8; 
        colorRGBBits.blue_shift = 0; 
        colorRGBBits.blue_bits = 8;  
        colorSpace = IL_CreateTrueColorSpace(&colorRGBBits, 24);
        IL_AddRefToColorSpace(colorSpace);
        IL_ReleaseColorSpace(aImage->header.color_space);
        aImage->header.color_space = colorSpace;
    }
}

void 
ImageRendererImpl::UpdatePixmap(void* aDisplayContext, 
				IL_Pixmap* aImage, 
				PRInt32 aXOffset, PRInt32 aYOffset, 
				PRInt32 aWidth, PRInt32 aHeight)
{
    nsIRenderingContext *rc = (nsIRenderingContext *)aDisplayContext;
    nsIImage            *img = (nsIImage *)aImage->client_data;
    nsIDeviceContext    *dx = rc->GetDeviceContext();
    nsRect              drect(aXOffset, aYOffset, aWidth, aHeight);

    img->ImageUpdated(dx, nsImageUpdateFlags_kBitsChanged, &drect);

    NS_IF_RELEASE(dx);
}

void 
ImageRendererImpl::ControlPixmapBits(void* aDisplayContext, 
				     IL_Pixmap* aImage, PRUint32 aControlMsg)
{
    nsIRenderingContext *rc = (nsIRenderingContext *)aDisplayContext;
    nsIImage *img = (nsIImage *)aImage->client_data;

    if (aControlMsg == IL_RELEASE_BITS) {
      nsIDeviceContext  *dx = rc->GetDeviceContext();
      if (nsnull != dx) {
        nsDrawingSurface  surf = dx->GetDrawingSurface(*rc);
        if (nsnull != surf)
          img->Optimize(surf);
        NS_RELEASE(dx);
      }
    }
}

void 
ImageRendererImpl::DestroyPixmap(void* aDisplayContext, IL_Pixmap* aImage)
{
    nsIRenderingContext *rc = (nsIRenderingContext *)aDisplayContext;
    nsIImage *img = (nsIImage *)aImage->client_data;

    aImage->client_data = nsnull;
    if (img) {
        NS_RELEASE(img);
    }
}
  
void 
ImageRendererImpl::DisplayPixmap(void* aDisplayContext, 
				 IL_Pixmap* aImage, IL_Pixmap* aMask, 
				 PRInt32 aX, PRInt32 aY, 
				 PRInt32 aXOffset, PRInt32 aYOffset, 
				 PRInt32 aWidth, PRInt32 aHeight)
{
    nsIRenderingContext *rc = (nsIRenderingContext *)aDisplayContext;
    nsIImage *img = (nsIImage *)aImage->client_data;

    // XXX Need better version of DrawImage
    rc->DrawImage(img, aX, aY);
}

void 
ImageRendererImpl::DisplayIcon(void* aDisplayContext, 
			       PRInt32 aX, PRInt32 aY, PRUint32 aIconNumber)
{
    nsIRenderingContext *rc = (nsIRenderingContext *)aDisplayContext;
}

void 
ImageRendererImpl::GetIconDimensions(void* aDisplayContext, 
				     PRInt32 *aWidthPtr, PRInt32 *aHeightPtr, 
				     PRUint32 aIconNumber)
{
    nsIRenderingContext *rc = (nsIRenderingContext *)aDisplayContext;
}

extern "C" NS_GFX_(nsresult)
NS_NewImageRenderer(ilIImageRenderer  **aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  ilIImageRenderer *renderer = new ImageRendererImpl();
  if (renderer == nsnull) {
        return NS_ERROR_OUT_OF_MEMORY;
  }

  return renderer->QueryInterface(kIImageRendererIID, 
                                  (void **) aInstancePtrResult);
}
