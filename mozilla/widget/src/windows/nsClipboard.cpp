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

#include "nsClipboard.h"
#include <windows.h>
#include <OLE2.h>
#include <SHLOBJ.H>

#include "nsCOMPtr.h"
#include "nsDataObj.h"
#include "nsISupportsArray.h"
#include "nsIClipboardOwner.h"
#include "nsIDataFlavor.h"
#include "nsIFormatConverter.h"
#include "nsIGenericTransferable.h"
#include "nsITransferable.h"

#include "nsIWidget.h"
#include "nsIComponentManager.h"
#include "nsWidgetsCID.h"

#include "DDCOMM.h"

#include "nsVoidArray.h"
#include "nsFileSpec.h"

// interface definitions
static NS_DEFINE_IID(kIDataFlavorIID,    NS_IDATAFLAVOR_IID);

//static NS_DEFINE_IID(kIWidgetIID,        NS_IWIDGET_IID);
//static NS_DEFINE_IID(kWindowCID,         NS_WINDOW_CID);

NS_IMPL_ADDREF_INHERITED(nsClipboard, nsBaseClipboard)
NS_IMPL_RELEASE_INHERITED(nsClipboard, nsBaseClipboard)

//-------------------------------------------------------------------------
//
// nsClipboard constructor
//
//-------------------------------------------------------------------------
nsClipboard::nsClipboard() : nsBaseClipboard()
{
  //NS_INIT_REFCNT();
  mDataObj        = nsnull;
  mIgnoreEmptyNotification = PR_FALSE;
  mWindow         = nsnull;

  // Create a Native window for the shell container...
  //nsresult rv = nsComponentManager::CreateInstance(kWindowCID, nsnull, kIWidgetIID, (void**)&mWindow);
  //mWindow->Show(PR_FALSE);
  //mWindow->Resize(1,1,PR_FALSE);
}

//-------------------------------------------------------------------------
// nsClipboard destructor
//-------------------------------------------------------------------------
nsClipboard::~nsClipboard()
{
  //NS_IF_RELEASE(mWindow);

  //EmptyClipboard();
  if (nsnull != mDataObj) {
    mDataObj->Release();
  }

}

//-------------------------------------------------------------------------
// * @param aIID The name of the class implementing the method
// * @param _classiiddef The name of the #define symbol that defines the IID
// * for the class (e.g. NS_ISUPPORTS_IID)
//-------------------------------------------------------------------------
nsresult nsClipboard::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{

  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv = NS_NOINTERFACE;

  static NS_DEFINE_IID(kIClipboard, NS_ICLIPBOARD_IID);
  if (aIID.Equals(kIClipboard)) {
    *aInstancePtr = (void*) ((nsIClipboard*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return rv;
}

//-------------------------------------------------------------------------
UINT nsClipboard::GetFormat(const nsString & aMimeStr)
{
  UINT format = 0;

  if (aMimeStr.Equals(kTextMime)) {
    format = CF_TEXT;
  } else if (aMimeStr.Equals(kUnicodeMime)) {
    format = CF_UNICODETEXT;
  } else if (aMimeStr.Equals(kJPEGImageMime)) {
    format = CF_DIB;
  } else if (aMimeStr.Equals(kDropFilesMime)) {
    format = CF_HDROP;
  } else {
    char * str = aMimeStr.ToNewCString();
    format = ::RegisterClipboardFormat(str);
    delete[] str;
  }

  return format;
}

//-------------------------------------------------------------------------
nsresult nsClipboard::CreateNativeDataObject(nsITransferable * aTransferable, IDataObject ** aDataObj)
{
  if (nsnull == aTransferable) {
    return NS_ERROR_FAILURE;
  }

  // Create our native DataObject that implements 
  // the OLE IDataObject interface
  nsDataObj * dataObj;
  dataObj = new nsDataObj();
  dataObj->AddRef();

  // No set it up with all the right data flavors & enums
  nsresult res = SetupNativeDataObject(aTransferable, dataObj);
  if (NS_OK == res) {
    *aDataObj = dataObj; 
  } else {
    delete dataObj;
  }
  return res;
}

//-------------------------------------------------------------------------
nsresult nsClipboard::SetupNativeDataObject(nsITransferable * aTransferable, IDataObject * aDataObj)
{
  if (nsnull == aTransferable || nsnull == aDataObj) {
    return NS_ERROR_FAILURE;
  }

  nsDataObj * dObj = NS_STATIC_CAST(nsDataObj *, aDataObj);

  // Now give the Transferable to the DataObject 
  // for getting the data out of it
  dObj->SetTransferable(aTransferable);

  // Get the transferable list of data flavors
  nsISupportsArray * dfList;
  aTransferable->FlavorsTransferableCanExport(&dfList);

  // Walk through flavors that contain data and register them
  // into the DataObj as supported flavors
  PRUint32 i;
  for (i=0;i<dfList->Count();i++) {
    nsIDataFlavor * df;
    nsISupports * supports = dfList->ElementAt(i);
    if (NS_OK == supports->QueryInterface(kIDataFlavorIID, (void **)&df)) {
      nsString mime;
      df->GetMimeType(mime);
      UINT format = GetFormat(mime);

      // check here to see if we can the data back from global member
      // XXX need IStream support, or file support
      FORMATETC fe;
      SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);

      // Now tell the native IDataObject about both the DataFlavor and 
      // the native data format
      dObj->AddDataFlavor(df, &fe);
      NS_RELEASE(df);
    }
    NS_RELEASE(supports);
  }
  // Delete the data flavors list
  NS_RELEASE(dfList);

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsClipboard::SetNativeClipboardData()
{
  mIgnoreEmptyNotification = PR_TRUE;

  // make sure we have a good transferable
  if (nsnull == mTransferable) {
    return NS_ERROR_FAILURE;
  }

  // Clear the native clipboard
  ::OleFlushClipboard();

  // Release the existing DataObject
  if (mDataObj) {
    mDataObj->Release();
  }

  IDataObject * dataObj;
  if (NS_OK == CreateNativeDataObject(mTransferable, &dataObj)) { // this add refs
    // cast our native DataObject to its IDataObject pointer
    // and put it on the clipboard
    mDataObj = (IDataObject *)dataObj;
    ::OleSetClipboard(mDataObj);
  }

  mIgnoreEmptyNotification = PR_FALSE;

  return NS_OK;
}


//-------------------------------------------------------------------------
nsresult nsClipboard::GetGlobalData(HGLOBAL aHGBL, void ** aData, PRUint32 * aLen)
{
  // Allocate a new memory buffer and
  // copy the data from global memory 
  nsresult  result = NS_ERROR_FAILURE;
  LPSTR     lpStr; 
  DWORD     dataSize;
  if (aHGBL != NULL) {
    lpStr       = (LPSTR)::GlobalLock(aHGBL);
    dataSize    = ::GlobalSize(aHGBL);
    *aLen       = dataSize;
    char * data = new char[dataSize];

    char*    ptr  = data;
    LPSTR    pMem = lpStr;
    PRUint32 inx;
    for (inx=0; inx < dataSize; inx++) {
	    *ptr++ = *pMem++;
    }
    ::GlobalUnlock(aHGBL);

    *aData = data;
    result = NS_OK;
  } else {
    // We really shouldn't ever get here
    // but just in case
    *aData = nsnull;
    *aLen  = 0;
    LPVOID lpMsgBuf;

    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    );

    // Display the string.
    MessageBox( NULL, (const char *)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

    // Free the buffer.
    LocalFree( lpMsgBuf );    
  }

  return result;
}

//-------------------------------------------------------------------------
nsresult nsClipboard::GetNativeDataOffClipboard(nsIWidget * aWindow, UINT aFormat, void ** aData, PRUint32 * aLen)
{
  HGLOBAL   hglb; 
  nsresult  result = NS_ERROR_FAILURE;

  HWND nativeWin = nsnull;//(HWND)aWindow->GetNativeData(NS_NATIVE_WINDOW);
  if (::OpenClipboard(nativeWin)) { 
    hglb = ::GetClipboardData(aFormat); 
    result = GetGlobalData(hglb, aData, aLen);
    ::CloseClipboard();
  }
  return result;
}

//-------------------------------------------------------------------------
static HRESULT FillSTGMedium(IDataObject * aDataObject, UINT aFormat, LPFORMATETC pFE, LPSTGMEDIUM pSTM, DWORD aTymed)
{
  SET_FORMATETC(*pFE, aFormat, 0, DVASPECT_CONTENT, -1, aTymed);

  // Starting by querying for the data to see if we can get it as from global memory
  HRESULT hres = S_FALSE;
  if (S_OK == aDataObject->QueryGetData(pFE)) {
    hres = aDataObject->GetData(pFE, pSTM);

#if 1 // for debug
    if (hres == E_INVALIDARG) {
      printf("E_INVALIDARG\n");
    }
    if (hres == E_UNEXPECTED) {
      printf("E_UNEXPECTED\n");
    }
    if (hres == E_OUTOFMEMORY) {
      printf("E_OUTOFMEMORY\n");
    }
    if (hres == DV_E_LINDEX ) {
      printf("DV_E_LINDEX\n");
    }
    if (hres == DV_E_FORMATETC) {
      printf("DV_E_FORMATETC\n");
    } 
    if (hres == DV_E_TYMED) {
      printf("DV_E_TYMED\n");
    } 
    if (hres == DV_E_DVASPECT) {
      printf("DV_E_DVASPECT\n");
    } 
    if (hres == OLE_E_NOTRUNNING) {
      printf("OLE_E_NOTRUNNING\n");
    } 
    if (hres == STG_E_MEDIUMFULL) {
      printf("STG_E_MEDIUMFULL\n");
    } 
    if (hres == S_OK) {
      printf("S_OK\n");
    } 
#endif
  }
  return hres;
}

//-------------------------------------------------------------------------
nsresult nsClipboard::GetNativeDataOffClipboard(IDataObject * aDataObject, UINT aFormat, void ** aData, PRUint32 * aLen)
{
  nsresult result = NS_ERROR_FAILURE;

  if (nsnull == aDataObject) {
    return result;
  }

  UINT    format = aFormat;
  HRESULT hres   = S_FALSE;

  // XXX at the moment we only support global memory transfers
  // It is here where we will add support for native images 
  // and IStream
  FORMATETC fe;
  STGMEDIUM stm;
  hres = FillSTGMedium(aDataObject, format, &fe, &stm, TYMED_HGLOBAL);

  // We can only understand our own aFormats (the MIME types)
  // So if the aFormat match one of the image MIME we need to convert over
  // to a format that windows understands
  /*if (S_OK != hres) {
    nsAutoString mime(kJPEGImageMime);
    UINT jpegFormat = GetFormat(mime);
    if (jpegFormat == format) {
      hres = FillSTGMedium(aDataObject, CF_DIB, &fe, &stm, TYMED_HGLOBAL);
    }
  }*/


  if (S_OK == hres) {
    switch (stm.tymed) {

      case TYMED_HGLOBAL: 
        {
          result = GetGlobalData(stm.hGlobal, aData, aLen);
          switch (fe.cfFormat) {
            case CF_TEXT: 
              {
                char * str = (char *)*aData;
                while (str[*aLen-1] == 0) {
                  (*aLen)--;
                }
              } break;

            case CF_DIB :
              {
                printf("************** DIB was dropped!\n");
              } break;

            case CF_HDROP : 
              {
                HDROP dropFiles = (HDROP)(*aData);

                char fileName[1024];
                UINT numFiles = DragQueryFile(dropFiles, 0xFFFFFFFF, NULL, 0);
                if (numFiles > 0) {
                  nsVoidArray * fileList = new nsVoidArray();
                  for (UINT i=0;i<numFiles;i++) {
                    UINT bytesCopied = ::DragQueryFile(dropFiles, i, fileName, 1024);
                    //nsAutoString name((char *)fileName, bytesCopied-1);
                    nsString name = fileName;
                    printf("name [%s]\n", name.ToNewCString());
                    nsFilePath filePath(name);
                    nsFileSpec * fileSpec = new nsFileSpec(filePath);
                    fileList->AppendElement(fileSpec);
                  }
                  *aLen  = (PRUint32)numFiles;
                  *aData = fileList;
                } else {
                  *aData = nsnull;
                  *aLen = 0;
                }
              } break;

            default:
              break;
          } // switch
        } break;

      default:
        break;
    } //switch
  }

  return result;
}


//-------------------------------------------------------------------------
nsresult nsClipboard::GetDataFromDataObject(IDataObject     * aDataObject,
                                            nsIWidget       * aWindow,
                                            nsITransferable * aTransferable)
{
  nsresult res = NS_ERROR_FAILURE;

  // make sure we have a good transferable
  if (nsnull == aTransferable) {
    return res;
  }
  nsCOMPtr<nsIGenericTransferable> genericTrans(do_QueryInterface(aTransferable));
  if (!genericTrans) {
    return res;
  }

  // Get the transferable list of data flavors
  nsISupportsArray * dfList;
  aTransferable->GetTransferDataFlavors(&dfList);

  // Walk through flavors and see which flavor is on the clipboard them on the native clipboard,
  PRUint32 i;
  for (i=0;i<dfList->Count();i++) {
    nsIDataFlavor * df;
    nsISupports * supports = dfList->ElementAt(i);
    if (NS_OK == supports->QueryInterface(kIDataFlavorIID, (void **)&df)) {
      nsString mime;
      df->GetMimeType(mime);
      UINT format = GetFormat(mime);

      void   * data;
      PRUint32 dataLen;

      if (nsnull != aDataObject) {
        res = GetNativeDataOffClipboard(aDataObject, format, &data, &dataLen);
        if (NS_OK == res) {
          genericTrans->SetTransferData(df, data, dataLen);
        }
      } else if (nsnull != aWindow) {
        res = GetNativeDataOffClipboard(aWindow, format, &data, &dataLen);
        if (NS_OK == res) {
          genericTrans->SetTransferData(df, data, dataLen);
        }
      } 
      NS_IF_RELEASE(df);
    }
    NS_RELEASE(supports);
  }

  return res;

}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsClipboard::GetNativeClipboardData(nsITransferable * aTransferable)
{
  // make sure we have a good transferable
  if (nsnull == aTransferable) {
    return NS_ERROR_FAILURE;
  }

  nsresult res;

  // This makes sure we can use the OLE functionality for the clipboard
  IDataObject * dataObj;
  if (S_OK == ::OleGetClipboard(&dataObj)) {
    // Use OLE IDataObject for clipboard operations
    res = GetDataFromDataObject(dataObj, nsnull, aTransferable);
  } else {
    // do it the old manula way
    res = GetDataFromDataObject(nsnull, mWindow, aTransferable);
  }
  return res;

}


//-------------------------------------------------------------------------
static void PlaceDataOnClipboard(PRUint32 aFormat, char * aData, int aLength)
{
  HGLOBAL     hGlobalMemory;
  PSTR        pGlobalMemory;

  PRInt32 size = aLength + 1;

  if (aLength) {
    // Copy text to Global Memory Area
    hGlobalMemory = (HGLOBAL)::GlobalAlloc(GHND, size);
    if (hGlobalMemory != NULL) {
      pGlobalMemory = (PSTR) ::GlobalLock(hGlobalMemory);

      int i;

      char * s  = aData;
      PRInt32 len = aLength;
      for (i=0;i< len;i++) {
	      *pGlobalMemory++ = *s++;
      }

      // Put data on Clipboard
      ::GlobalUnlock(hGlobalMemory);
      ::SetClipboardData(aFormat, hGlobalMemory);
    }
  }  
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsClipboard::ForceDataToClipboard()
{
  // make sure we have a good transferable
  if (nsnull == mTransferable) {
    return NS_ERROR_FAILURE;
  }

  HWND nativeWin = nsnull;//(HWND)mWindow->GetNativeData(NS_NATIVE_WINDOW);
  ::OpenClipboard(nativeWin);
  ::EmptyClipboard();

  // Get the transferable list of data flavors
  nsISupportsArray * dfList;
  mTransferable->GetTransferDataFlavors(&dfList);

  // Walk through flavors and see which flavor is on the native clipboard,
  PRUint32 i;
  for (i=0;i<dfList->Count();i++) {
    nsIDataFlavor * df;
    nsISupports * supports = dfList->ElementAt(i);
    if (NS_OK == supports->QueryInterface(kIDataFlavorIID, (void **)&df)) {
      nsString mime;
      df->GetMimeType(mime);

      // convert mime to native format identifier
      UINT format = GetFormat(mime);

      void   * data;
      PRUint32 dataLen;

      // Get the data as a bunch-o-bytes from the clipboard
      mTransferable->GetTransferData(df, &data, &dataLen);

      // now place it on the Clipboard
      if (nsnull != data) {
        PlaceDataOnClipboard(format, (char *)data, dataLen);
      }
      NS_RELEASE(df);
    }
    NS_RELEASE(supports);
  }

  ::CloseClipboard();

  return NS_OK;
}

