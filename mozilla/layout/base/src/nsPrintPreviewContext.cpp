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
#include "nsPresContext.h"
#include "nsIDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsIView.h"
#include "nsIWidget.h"
#include "nsGfxCIID.h"

static NS_DEFINE_IID(kIPresContextIID, NS_IPRESCONTEXT_IID);

class PrintPreviewContext : public nsPresContext {
public:
  PrintPreviewContext();
  ~PrintPreviewContext();

  virtual PRBool IsPaginated();
  virtual nscoord GetPageWidth();
  virtual nscoord GetPageHeight();
};

PrintPreviewContext::PrintPreviewContext()
{
  nsresult  res;

  static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);
  static NS_DEFINE_IID(kDeviceContextIID, NS_IDEVICE_CONTEXT_IID);

  res = NSRepository::CreateInstance(kDeviceContextCID, nsnull, kDeviceContextIID, (void **)&mDeviceContext);

  if (NS_OK == res)
  {
    mDeviceContext->Init();

    mDeviceContext->SetDevUnitsToAppUnits(mDeviceContext->GetDevUnitsToTwips());
    mDeviceContext->SetAppUnitsToDevUnits(mDeviceContext->GetTwipsToDevUnits());

    NS_ADDREF(mDeviceContext);
  }
}

PrintPreviewContext::~PrintPreviewContext()
{
  NS_IF_RELEASE(mDeviceContext);
}

PRBool PrintPreviewContext::IsPaginated()
{
  return PR_TRUE;
}

// XXX assumes a 1/2 margin around all sides
nscoord PrintPreviewContext::GetPageWidth()
{
#if 0
  return (nscoord) NS_INCHES_TO_TWIPS(7.5);
#else
  // For testing purposes make the page width 95% of the visible area
  return mVisibleArea.width * 95 / 100;
#endif
}

nscoord PrintPreviewContext::GetPageHeight()
{
#if 0
  return (nscoord) NS_INCHES_TO_TWIPS(10);
#else
  // For testing purposes make the page height 60% of the visible area
  return mVisibleArea.height * 60 / 100;
#endif
}

NS_LAYOUT nsresult
NS_NewPrintPreviewContext(nsIPresContext** aInstancePtrResult)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  PrintPreviewContext *it = new PrintPreviewContext();

  if (it == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kIPresContextIID, (void **) aInstancePtrResult);
}
