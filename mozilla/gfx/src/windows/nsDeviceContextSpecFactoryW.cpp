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

#include "nsDeviceContextSpecFactoryW.h"
#include "nsDeviceContextSpecWin.h"
#include <windows.h>
#include <commdlg.h>
#include "nsGfxCIID.h"
#include "plstr.h"

nsDeviceContextSpecFactoryWin :: nsDeviceContextSpecFactoryWin()
{
}

nsDeviceContextSpecFactoryWin :: ~nsDeviceContextSpecFactoryWin()
{
}

static NS_DEFINE_IID(kDeviceContextSpecFactoryIID, NS_IDEVICE_CONTEXT_SPEC_FACTORY_IID);
static NS_DEFINE_IID(kIDeviceContextSpecIID, NS_IDEVICE_CONTEXT_SPEC_IID);
static NS_DEFINE_IID(kDeviceContextSpecCID, NS_DEVICE_CONTEXT_SPEC_CID);

NS_IMPL_QUERY_INTERFACE(nsDeviceContextSpecFactoryWin, kDeviceContextSpecFactoryIID)
NS_IMPL_ADDREF(nsDeviceContextSpecFactoryWin)
NS_IMPL_RELEASE(nsDeviceContextSpecFactoryWin)

NS_IMETHODIMP nsDeviceContextSpecFactoryWin :: Init(void)
{
  return NS_OK;
}

//XXX this method needs to do what the API says...

NS_IMETHODIMP nsDeviceContextSpecFactoryWin :: CreateDeviceContextSpec(nsIDeviceContextSpec *aOldSpec,
                                                                       nsIDeviceContextSpec *&aNewSpec,
                                                                       PRBool aQuiet)
{
  PRINTDLG  prntdlg;
  nsresult  rv = NS_ERROR_FAILURE;

  prntdlg.lStructSize = sizeof(prntdlg);
  prntdlg.hwndOwner = NULL;               //XXX need to find a window here. MMP
  prntdlg.hDevMode = NULL;
  prntdlg.hDevNames = NULL;
  prntdlg.hDC = NULL;
  prntdlg.Flags = PD_ALLPAGES | PD_RETURNIC;
  prntdlg.nFromPage = 0;
  prntdlg.nToPage = 0;
  prntdlg.nMinPage = 0;
  prntdlg.nMaxPage = 0;
  prntdlg.nCopies = 1;
  prntdlg.hInstance = NULL;
  prntdlg.lCustData = 0;
  prntdlg.lpfnPrintHook = NULL;
  prntdlg.lpfnSetupHook = NULL;
  prntdlg.lpPrintTemplateName = NULL;
  prntdlg.lpSetupTemplateName = NULL;
  prntdlg.hPrintTemplate = NULL;
  prntdlg.hSetupTemplate = NULL;

  BOOL res = ::PrintDlg(&prntdlg);

  if (TRUE == res)
  {
    DEVNAMES *devnames = (DEVNAMES *)::GlobalLock(prntdlg.hDevNames);

    char device[200], driver[200];

    //print something...

    PL_strcpy(device, &(((char *)devnames)[devnames->wDeviceOffset]));
    PL_strcpy(driver, &(((char *)devnames)[devnames->wDriverOffset]));

#ifdef NS_DEBUG
printf("printer: driver %s, device %s\n", driver, device);
#endif

    nsIDeviceContextSpec  *devspec = nsnull;

    nsRepository::CreateInstance(kDeviceContextSpecCID, nsnull, kIDeviceContextSpecIID, (void **)&devspec);

    if (nsnull != devspec)
    {
      //XXX need to QI rather than cast... MMP
      if (NS_OK == ((nsDeviceContextSpecWin *)devspec)->Init(driver, device, prntdlg.hDevMode))
      {
        aNewSpec = devspec;
        rv = NS_OK;
      }
    }

    //don't free the DEVMODE because the device context spec now owns it...
    ::GlobalUnlock(prntdlg.hDevNames);
    ::GlobalFree(prntdlg.hDevNames);
  }

  return rv;
}
