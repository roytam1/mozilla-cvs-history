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

#include "nsTransferable.h"
#include "nsIDataFlavor.h"
#include "nsDataFlavor.h"
#include "nsWidgetsCID.h"
#include "nsISupportsArray.h"
#include "nsRepository.h"

#include "nsDataObj.h"

#include "DDCOMM.h"

static NS_DEFINE_IID(kITransferableIID,  NS_ITRANSFERABLE_IID);
static NS_DEFINE_IID(kIDataFlavorIID,    NS_IDATAFLAVOR_IID);
static NS_DEFINE_IID(kCDataFlavorCID,    NS_DATAFLAVOR_CID);

NS_IMPL_ADDREF(nsTransferable)
NS_IMPL_RELEASE(nsTransferable)

//-------------------------------------------------------------------------
//
// Transferable constructor
//
//-------------------------------------------------------------------------
nsTransferable::nsTransferable()
{
  NS_INIT_REFCNT();
  mDataObj  = nsnull;
  mStrCache = nsnull;
  mDataPtr  = nsnull;

  nsresult rv = NS_NewISupportsArray(&mDFList);

}

//-------------------------------------------------------------------------
//
// Transferable destructor
//
//-------------------------------------------------------------------------
nsTransferable::~nsTransferable()
{
  if (mStrCache) {
    delete mStrCache;
  }
  if (mDataPtr) {
    delete[] mDataPtr;
  }
  
}

/**
 * @param aIID The name of the class implementing the method
 * @param _classiiddef The name of the #define symbol that defines the IID
 * for the class (e.g. NS_ISUPPORTS_IID)
 * 
*/ 
nsresult nsTransferable::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{

  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv = NS_NOINTERFACE;

  if (aIID.Equals(kITransferableIID)) {
    *aInstancePtr = (void*) ((nsITransferable*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return rv;
}


/**
  * 
  *
  */
NS_IMETHODIMP nsTransferable::GetTransferDataFlavors(nsISupportsArray ** aDataFlavorList)
{
  *aDataFlavorList = mDFList;
  NS_ADDREF(mDFList);
  return NS_OK;
}

/**
  * 
  *
  */
NS_IMETHODIMP nsTransferable::IsDataFlavorSupported(nsIDataFlavor * aDataFlavor)
{
  nsAutoString  mimeInQuestion;
  aDataFlavor->GetMimeType(mimeInQuestion);

  PRUint32 i;
  for (i=0;i<mDFList->Count();i++) {
    nsIDataFlavor * df;
    nsISupports * supports = mDFList->ElementAt(i);
    if (NS_OK == supports->QueryInterface(kIDataFlavorIID, (void **)&df)) {
      nsAutoString mime;
      df->GetMimeType(mime);
      if (mimeInQuestion.Equals(mime)) {
        return NS_OK;
      }
      NS_RELEASE(df);
    }
    NS_RELEASE(supports);
  }
  return NS_ERROR_FAILURE;
}

/**
  * 
  *
  */
NS_IMETHODIMP nsTransferable::GetTransferData(nsIDataFlavor * aDataFlavor, void ** aData, PRUint32 * aDataLen)
{
  if (mDataPtr) {
    delete[] mDataPtr;
  }

  nsAutoString  mimeInQuestion;
  aDataFlavor->GetMimeType(mimeInQuestion);

  if (mimeInQuestion.Equals(kTextMime)) {
    char * str = mStrCache->ToNewCString();
    *aDataLen = mStrCache->Length()+1;

    mDataPtr = (void *)str;
    *aData = mDataPtr; 
  } else if (mimeInQuestion.Equals(kHTMLMime)) {

  }

  return NS_OK;
}

/**
  * 
  *
  */
NS_IMETHODIMP nsTransferable::SetTransferData(nsIDataFlavor * aDataFlavor, void * aData, PRUint32 aDataLen)
{
  if (aData == nsnull) {
    return NS_OK;
  }

  nsAutoString  mimeInQuestion;
  aDataFlavor->GetMimeType(mimeInQuestion);

  if (mimeInQuestion.Equals(kTextMime)) {
    if (nsnull == mStrCache) {
      mStrCache = new nsString();
    }
    mStrCache->SetString((char *)aData, aDataLen-1);
  } else if (mimeInQuestion.Equals(kHTMLMime)) {

  }

  return NS_OK;
}

/**
  * 
  *
  */
NS_IMETHODIMP nsTransferable::SetTransferString(const nsString & aStr)
{
  if (!mStrCache) {
    mStrCache = new nsString(aStr);
  } else {
    *mStrCache = aStr;
  }

  return NS_OK;
}

/**
  * 
  *
  */
NS_IMETHODIMP nsTransferable::GetTransferString(nsString & aStr)
{
  if (!mStrCache) {
    mStrCache = new nsString();
  }
  aStr = *mStrCache;

  return NS_OK;
}

/**
  * 
  *
  */
NS_IMETHODIMP nsTransferable::AddDataFlavor(const nsString & aMimeType, const nsString & aHumanPresentableName)
{
  nsIDataFlavor * df;
  nsresult rv = nsComponentManager::CreateInstance(kCDataFlavorCID, nsnull, kIDataFlavorIID, (void**) &df);
  if (nsnull == df) {
    return rv;
  }

  df->Init(aMimeType, aHumanPresentableName);
  mDFList->AppendElement(df);
  
  return rv;
}

/**
  * 
  *
  */
NS_IMETHODIMP nsTransferable::IsLargeDataSet()
{
  if (mStrCache) {
    return (mStrCache->Length() > 1024?NS_OK:NS_ERROR_FAILURE);
  }

  return NS_ERROR_FAILURE;
}

