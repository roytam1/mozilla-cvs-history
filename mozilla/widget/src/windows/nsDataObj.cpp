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

#include "Ddcomm.h"
//#include "DragDrop.h"
#include "nsDataObj.h"
#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsIDataFlavor.h"
#include "nsITransferable.h"
#include "IENUMFE.h"

// interface definitions
static NS_DEFINE_IID(kIDataFlavorIID,    NS_IDATAFLAVOR_IID);

//#include "OLEIDL.h"
#include "OLE2.h"
#include "URLMON.h"

//static int gCounter = 0;

ULONG nsDataObjClassFactory::g_cLock = 0;
ULONG nsDataObj::g_cRef = 0;

EXTERN_C GUID CDECL CLSID_nsDataObj =
	{ 0x1bba7640, 0xdf52, 0x11cf, { 0x82, 0x7b, 0, 0xa0, 0x24, 0x3a, 0xe5, 0x05 } };

/*
 * class nsDataObjClassFactory
 */

nsDataObjClassFactory::nsDataObjClassFactory()
{
	 m_cRef = 0L;
    return;
}

nsDataObjClassFactory::~nsDataObjClassFactory()
{
    return;
}

// IUnknown Interface - see iunknown.h for documentation

STDMETHODIMP nsDataObjClassFactory::QueryInterface(REFIID riid, void** ppv)
{
	 *ppv = NULL;

	 if (IID_IUnknown == riid || IID_IClassFactory == riid)
		  *ppv = this;

	 if (NULL != *ppv) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
	 }

    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) nsDataObjClassFactory::AddRef()
{
	return ++m_cRef;
}


STDMETHODIMP_(ULONG) nsDataObjClassFactory::Release()
{
	if (0L != --m_cRef)
		return m_cRef;

	delete this;
	return 0L;
}

// IClassFactory methods

STDMETHODIMP nsDataObjClassFactory::CreateInstance
	(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
{
	 nsDataObj*          pObj;
	 HRESULT             hr;

    *ppvObj=NULL;
    hr=ResultFromScode(E_OUTOFMEMORY);

    //Verify that a controlling unknown asks for IUnknown
	 if (NULL != pUnkOuter && IID_IUnknown != riid)
		  return ResultFromScode(E_NOINTERFACE);

	 //Create the object.
	 pObj = new nsDataObj();

	 if (NULL == pObj)
        return hr;

	 hr = pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
	 if (FAILED(hr))
		  delete pObj;

    return hr;
}


STDMETHODIMP nsDataObjClassFactory::LockServer(BOOL fLock)
{
	if (fLock) {
		++g_cLock;
	} else if (0 < g_cLock) {
		--g_cLock;
	}

	return NOERROR;
}

/*
 * Class nsDataObj
 */

// construction, destruction

nsDataObj::nsDataObj()
{
	m_cRef	        = 0;
  mTransferable   = nsnull;
  nsresult result = NS_NewISupportsArray(&mDataFlavors);

  m_enumFE = new CEnumFormatEtc(this, 32);
  m_enumFE->AddRef();
  //printf("**************************** Counter %d\n", (++gCounter));
}

nsDataObj::~nsDataObj()
{
	m_cRef = 0;
  m_enumFE->Release();
  //printf("**************************** Counter %d\n", (--gCounter));

}


// IUnknown interface methods - see inknown.h for documentation

STDMETHODIMP nsDataObj::QueryInterface(REFIID riid, void** ppv)
{
	*ppv=NULL;

	if ( (IID_IUnknown == riid) || (IID_IDataObject	== riid) ) {
		*ppv = this;
		AddRef();
		return NOERROR;
	}

	return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) nsDataObj::AddRef()
{
	++g_cRef;
  //printf("AddRef >>>>>>>>>>>>>>>>>> %d on %p\n", (m_cRef+1), this);
	return ++m_cRef;
}


STDMETHODIMP_(ULONG) nsDataObj::Release()
{
  //printf("Release>>>>>>>>>>>>>>>>>> %d on %p\n", (m_cRef-1), this);
	if (0 < g_cRef)
		--g_cRef;

	if (0 != --m_cRef)
		return m_cRef;

	delete this;
	return 0;
}

BOOL nsDataObj::FormatsMatch(const FORMATETC& source, const FORMATETC& target) const
{
	if ((source.cfFormat == target.cfFormat) &&
		 (source.dwAspect  & target.dwAspect)  &&
		 (source.tymed     & target.tymed))       {
		return TRUE;
	} else {
		return FALSE;
	}
}


// IDataObject methods

STDMETHODIMP nsDataObj::GetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  //printf("nsDataObj::GetData\n");
  //printf("  format: %d  Text: %d\n", pFE->cfFormat, CF_TEXT);
  if (nsnull == mTransferable) {
	  return ResultFromScode(DATA_E_FORMATETC);
  }

  PRUint32 dfInx = 0;

  ULONG count;
  FORMATETC fe;
  m_enumFE->Reset();
  while (NOERROR == m_enumFE->Next(1, &fe, &count)) {
    nsIDataFlavor * df;
    nsISupports * supports = mDataFlavors->ElementAt(dfInx);
    if (NS_OK == supports->QueryInterface(kIDataFlavorIID, (void **)&df)) {
		  if (FormatsMatch(fe, *pFE)) {
			  pSTM->pUnkForRelease = NULL;
			  CLIPFORMAT format = pFE->cfFormat;
			  switch(format) {
				  case CF_TEXT:
					  return GetText(df, *pFE, *pSTM);
				  //case CF_BITMAP:
				  //	return GetBitmap(*pFE, *pSTM);
				  //case CF_DIB:
				  //	return GetDib(*pFE, *pSTM);
				  //case CF_METAFILEPICT:
				  //	return GetMetafilePict(*pFE, *pSTM);
				  default:
            //printf("***** nsDataObj::GetData - Unknown format %d\n", format);
					  return GetText(df, *pFE, *pSTM);
            break;
        } //switch
      } // if
      NS_RELEASE(df);
    }
    NS_RELEASE(supports);
    dfInx++;
  } // while

	return ResultFromScode(DATA_E_FORMATETC);
}


STDMETHODIMP nsDataObj::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  //printf("nsDataObj::GetDataHere\n");
	//if (m_dragDrop) {
	//	return m_dragDrop->GetDataHere(pFE, pSTM);
	//} else {
		return ResultFromScode(E_FAIL);
	//}
}


STDMETHODIMP nsDataObj::QueryGetData(LPFORMATETC pFE)
{
  //printf("nsDataObj::QueryGetData  ");
  //printf("format: %d  Text: %d\n", pFE->cfFormat, CF_TEXT);

  PRUint32 dfInx = 0;

  ULONG count;
  FORMATETC fe;
  m_enumFE->Reset();
  while (NOERROR == m_enumFE->Next(1, &fe, &count)) {
    if (fe.cfFormat == pFE->cfFormat) {
      return S_OK;
    }
  }
  //printf("***** nsDataObj::QueryGetData - Unknown format %d\n", pFE->cfFormat);
	return ResultFromScode(E_FAIL);
}

STDMETHODIMP nsDataObj::GetCanonicalFormatEtc
	 (LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
  //printf("nsDataObj::GetCanonicalFormatEtc\n");
	//if (m_dragDrop) {
	//	return m_dragDrop->GetCanonicalFormatEtc(pFEIn, pFEOut);
	//} else {
		return ResultFromScode(E_FAIL);
	//}
}


STDMETHODIMP nsDataObj::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease)
{
  //printf("nsDataObj::SetData\n");
  return ResultFromScode(E_FAIL);
	//return m_dragDrop->SetData(pFE, pSTM, fRelease);
}


STDMETHODIMP nsDataObj::EnumFormatEtc(DWORD dwDir, LPENUMFORMATETC *ppEnum)
{
  //printf("nsDataObj::EnumFormatEtc\n");

  switch (dwDir) {
    case DATADIR_GET: {
       m_enumFE->Clone(ppEnum);
    } break;
    case DATADIR_SET:
        *ppEnum=NULL;
        break;
    default:
        *ppEnum=NULL;
        break;
  } // switch
  if (NULL==*ppEnum)
    return ResultFromScode(E_FAIL);
  else
    (*ppEnum)->AddRef();

  return NOERROR;

}

STDMETHODIMP nsDataObj::DAdvise(LPFORMATETC pFE, DWORD dwFlags,
										  LPADVISESINK pIAdviseSink, DWORD* pdwConn)
{
  //printf("nsDataObj::DAdvise\n");
	return ResultFromScode(E_FAIL);
}


STDMETHODIMP nsDataObj::DUnadvise(DWORD dwConn)
{
  //printf("nsDataObj::DUnadvise\n");
	return ResultFromScode(E_FAIL);
}

STDMETHODIMP nsDataObj::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
  //printf("nsDataObj::EnumDAdvise\n");
	return ResultFromScode(E_FAIL);
}

// other methods

ULONG nsDataObj::GetCumRefCount()
{
	return g_cRef;
}

ULONG nsDataObj::GetRefCount() const
{
	return m_cRef;
}

// GetData and SetData helper functions

HRESULT nsDataObj::AddSetFormat(FORMATETC& aFE)
{
  //m_setFormats[m_numSetFormats++] = aFE;
	return ResultFromScode(S_OK);
}

HRESULT nsDataObj::AddGetFormat(FORMATETC& aFE)
{
  //m_getFormats[m_numGetFormats++] = aFE;

	return ResultFromScode(S_OK);
}

HRESULT nsDataObj::GetBitmap(FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_NOTIMPL);
}

HRESULT nsDataObj::GetDib(FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_NOTIMPL);
}

HRESULT nsDataObj::GetText(nsIDataFlavor * aDF, FORMATETC& aFE, STGMEDIUM& aSTG)
{
  char     * data;
  PRUint32   len;

  // NOTE: Transferable keeps ownership of the memory
  mTransferable->GetTransferData(aDF, (void **)&data, &len);
  if (0 == len) {
	  return ResultFromScode(E_FAIL);
  }

  HGLOBAL     hGlobalMemory = NULL;
  PSTR        pGlobalMemory = NULL;

  aSTG.tymed          = TYMED_HGLOBAL;
  aSTG.pUnkForRelease = NULL;

  //***************
  // NOTE: On Win98 it allocates to the nearest DWORD boundary 
  // alloc 4 gets you 8
  // alloc 6 gets you 8
  // etc.
  // The GHND will zero fill, external windows apps expect 
  // text string to be zero terminated.
  // 
  // So if we are copying CF_TEXT or CF_UNICODE we need to add
  // the null terminator because the transferable only gives up the text
  // and no terminating zero.
  // So if the length of the text is modulo 8 we need to add extra space
  // for terminating zero

  DWORD allocLen = (DWORD)len;

  if (CF_TEXT        == aFE.cfFormat ||
      CF_UNICODETEXT == aFE.cfFormat) {
    if (len % 8 == 0) {
      allocLen++; // note: this by actually add more than one byte.
    }
  }

  // GHND zeroes the memory
  hGlobalMemory = (HGLOBAL)::GlobalAlloc(GHND, allocLen); 
  //DWORD newSize = ::GlobalSize(hGlobalMemory);

  // Copy text to Global Memory Area
  if (hGlobalMemory != NULL) {
    //PSTR pstr = (PSTR)::GlobalLock(hGlobalMemory);
    char * pstr = (char *)::GlobalLock(hGlobalMemory);
    //PSTR pstr = pGlobalMemory;

    // need to use memcpy here
    char* s = data;
    PRUint32 inx;
    for (inx=0; inx < len; inx++) {
	    *pstr++ = *s++;
    }

    // Put data on Clipboard
    BOOL status = ::GlobalUnlock(hGlobalMemory);
  }

  aSTG.hGlobal = hGlobalMemory;

	return ResultFromScode(S_OK);
}

HRESULT nsDataObj::GetMetafilePict(FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_NOTIMPL);
}

HRESULT nsDataObj::SetBitmap(FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_NOTIMPL);
}
HRESULT nsDataObj::SetDib   (FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_FAIL);
}

HRESULT nsDataObj::SetText  (FORMATETC& aFE, STGMEDIUM& aSTG)
{
  if (aFE.cfFormat == CF_TEXT && aFE.tymed ==  TYMED_HGLOBAL) {
		HGLOBAL hString = (HGLOBAL)aSTG.hGlobal;
		if(hString == NULL)
			return(FALSE);

		// get a pointer to the actual bytes
		char *  pString = (char *) GlobalLock(hString);    
		if(!pString)
			return(FALSE);

		//tmpUrl = XP_STRDUP(pString);

		GlobalUnlock(hString);
    nsAutoString str(pString);

    /*  nsEventStatus status;
      nsDragDropEvent event;
      ((nsWindow *)mWindow)->InitEvent(event, NS_DRAGDROP_EVENT);
      event.mType      = nsDragDropEventStatus_eDrop;
      event.mIsFileURL = PR_FALSE;
      event.mURL       = (PRUnichar *)str.GetUnicode();          
      mWindow->DispatchEvent(&event, status);
      */
    /*typedef struct tagKBDEV {
    WORD	vkCode;
    WORD	scanCode;
    DWORD	flags;
    DWORD	time;
    DWORD	dwExtraInfo;
    } KBDEV, FAR *LPKBDEV, *PKBDEV;
    
    KBDEV kbDev[1];
    for (int i=0;i<strlen(pString);i++) {
      kbDev.vkCode = VkKeyScan(pString[i]);
      kbDev.scanCode = OemKeyScan(pString[i]);
      kbDev.flags = 0;
      KeyboardEventEx(&kbDev, 1, sizeof(KBDEV), KEYF_IGNORETIME);
      kbDev.flags = KEYEVENTF_KEYUP;
      KeyboardEventEx(&kbDev, 1, sizeof(KBDEV), KEYF_IGNORETIME);
    }*/
  }
	return ResultFromScode(E_FAIL);
}

HRESULT nsDataObj::SetMetafilePict (FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_FAIL);
}



CLSID nsDataObj::GetClassID() const
{
	return CLSID_nsDataObj;
}

/**
  * Registers a the DataFlavor/FE pair
  *
  */
void nsDataObj::AddDataFlavor(nsIDataFlavor * aDataFlavor, LPFORMATETC aFE)
{
  // These two lists are the mapping to and from data flavors and FEs
  // Later, OLE will tell us it's needs a certain type of FORMATETC (text, unicode, etc)
  // so we will look up data flavor that corresponds to the FE
  // and then ask the transferable for that type of data
  mDataFlavors->AppendElement(aDataFlavor);
  m_enumFE->AddFE(aFE);

}

/**
  * Sets the transferable object
  *
  */
void nsDataObj::SetTransferable(nsITransferable * aTransferable)
{
  if (nsnull != mTransferable) {
    NS_RELEASE(mTransferable);
  }
  mTransferable = aTransferable;
  if (nsnull == mTransferable) {
    return;
  }

  NS_ADDREF(mTransferable);

  return;
}
