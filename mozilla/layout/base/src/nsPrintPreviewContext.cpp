/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsPresContext.h"
#include "nsIPrintPreviewContext.h"
#include "nsIDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsIView.h"
#include "nsIWidget.h"
#include "nsGfxCIID.h"
#include "nsLayoutAtoms.h"
#include "prlog.h"
#include "nsIPrintSettings.h"

class PrintPreviewContext : public nsPresContext, nsIPrintPreviewContext {
public:
  PrintPreviewContext();
  ~PrintPreviewContext();

//Interfaces for addref and release and queryinterface
//NOTE macro used is for classes that inherit from 
// another class. Only the base class should use NS_DECL_ISUPPORTS
  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD GetImageLoadFlags(nsLoadFlags& aLoadFlags);
  NS_IMETHOD GetMedium(nsIAtom** aMedium);
  NS_IMETHOD IsPaginated(PRBool* aResult);
  NS_IMETHOD SetPaginatedScrolling(PRBool aResult) { mCanPaginatedScroll = aResult; return NS_OK; }
  NS_IMETHOD GetPaginatedScrolling(PRBool* aResult);
  NS_IMETHOD GetPageDim(nsRect* aActualRect, nsRect* aAdjRect);
  NS_IMETHOD SetPageDim(nsRect* aRect);
  NS_IMETHOD SetPrintSettings(nsIPrintSettings* aPS);
  NS_IMETHOD GetPrintSettings(nsIPrintSettings** aPS);
  NS_IMETHOD GetScaledPixelsToTwips(float* aScale) const;
  NS_IMETHOD SetScalingOfTwips(PRBool aOn) { mDoScaledTwips = aOn; return NS_OK; }

protected:
  nsRect mPageDim;
  PRBool mCanPaginatedScroll;
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  PRPackedBool mDoScaledTwips;
};

PrintPreviewContext::PrintPreviewContext() :
  mPageDim(-1,-1,-1,-1),
  mCanPaginatedScroll(PR_TRUE),
  mDoScaledTwips(PR_TRUE)
{
  SetBackgroundImageDraw(PR_FALSE);
  SetBackgroundColorDraw(PR_FALSE);
}

PrintPreviewContext::~PrintPreviewContext()
{
}

NS_IMPL_ADDREF_INHERITED(PrintPreviewContext,nsPresContext)
NS_IMPL_RELEASE_INHERITED(PrintPreviewContext,nsPresContext)

//---------------------------------------------------------
NS_IMETHODIMP
PrintPreviewContext::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
 
  if (aIID.Equals(NS_GET_IID(nsIPrintPreviewContext))) {
    *aInstancePtr = (void *)((nsIPrintPreviewContext*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return nsPresContext::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP
PrintPreviewContext::GetMedium(nsIAtom** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsLayoutAtoms::print;
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP 
PrintPreviewContext::GetImageLoadFlags(nsLoadFlags& aLoadFlags)
{
  aLoadFlags = nsIRequest::LOAD_FROM_CACHE | nsIRequest::VALIDATE_NEVER | nsIRequest::LOAD_NORMAL;
  return NS_OK;
}

NS_IMETHODIMP
PrintPreviewContext::IsPaginated(PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
PrintPreviewContext::GetPaginatedScrolling(PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = mCanPaginatedScroll;
  return NS_OK;
}

NS_IMETHODIMP
PrintPreviewContext::GetPageDim(nsRect* aActualRect, nsRect* aAdjRect)
{
  NS_ENSURE_ARG_POINTER(aActualRect);
  NS_ENSURE_ARG_POINTER(aAdjRect);

  PRInt32 width,height;
  if (NS_SUCCEEDED(mDeviceContext->GetDeviceSurfaceDimensions(width, height))) {
    aActualRect->SetRect(0, 0, width, height);
  }
  *aAdjRect = mPageDim;
  return NS_OK;
}

NS_IMETHODIMP
PrintPreviewContext::SetPageDim(nsRect* aPageDim)
{
  NS_ENSURE_ARG_POINTER(aPageDim);
  mPageDim = *aPageDim;
  return NS_OK;
}

NS_IMETHODIMP 
PrintPreviewContext::SetPrintSettings(nsIPrintSettings * aPrintSettings)
{
  NS_ENSURE_ARG_POINTER(aPrintSettings);
  mPrintSettings = aPrintSettings;
  return NS_OK;
}

NS_IMETHODIMP 
PrintPreviewContext::GetPrintSettings(nsIPrintSettings * *aPrintSettings)
{
  NS_ENSURE_ARG_POINTER(aPrintSettings);

  *aPrintSettings = mPrintSettings;
  NS_IF_ADDREF(*aPrintSettings);

  return NS_OK;
}

//---------------------------------------------------------
// The difference between this and GetScaledPixelsToTwips in nsPresContext
// is here we check to see if we are scaling twips
NS_IMETHODIMP
PrintPreviewContext::GetScaledPixelsToTwips(float* aResult) const
{
  NS_PRECONDITION(aResult, "null out param");

  float scale = 1.0f;
  if (mDeviceContext)
  {
    float p2t;
    mDeviceContext->GetDevUnitsToAppUnits(p2t);
    if (mDoScaledTwips) {
      mDeviceContext->GetCanonicalPixelScale(scale);
      scale = p2t * scale;
    } else {
      scale = p2t;
    }
  }
  *aResult = scale;
  return NS_OK;
}

NS_EXPORT nsresult
NS_NewPrintPreviewContext(nsIPrintPreviewContext** aInstancePtrResult)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  PrintPreviewContext *it = new PrintPreviewContext();

  if (it == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(NS_GET_IID(nsIPrintPreviewContext), (void **) aInstancePtrResult);
}
