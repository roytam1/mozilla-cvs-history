/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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

#define NS_IMPL_IDS
#include "pratom.h"
#include "nsIFactory.h"
#include "nsIServiceManager.h"
#include "nsRepository.h"
//#include "nsIObserver.h"
#include "nsIURL.h"
#ifdef NECKO
#include "nsIIOService.h"
#include "nsIURI.h"
#include "nsIServiceManager.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#endif // NECKO
#include "nsPICSElementObserver.h"
#include "nsString.h"
#include "nsIPICS.h"
#include "nspics.h"
#include "nsIWebShellServices.h"
#include "plstr.h"
#include "prenv.h"

//static NS_DEFINE_IID(kIObserverIID, NS_IOBSERVER_IID);
//static NS_DEFINE_IID(kObserverCID, NS_OBSERVER_CID);
static NS_DEFINE_IID(kIPICSElementObserverIID, NS_IPICSELEMENTOBSERVER_IID);
static NS_DEFINE_IID(kIElementObserverIID,     NS_IELEMENTOBSERVER_IID);
static NS_DEFINE_IID(kIObserverIID,            NS_IOBSERVER_IID);
static NS_DEFINE_IID(kISupportsIID,            NS_ISUPPORTS_IID);

static NS_DEFINE_IID(kIPICSIID,                NS_IPICS_IID);
static NS_DEFINE_IID(kPICSCID,                 NS_PICS_CID);




////////////////////////////////////////////////////////////////////////////////
// nsPICSElementObserver Implementation


NS_IMPL_ADDREF(nsPICSElementObserver)                       \
NS_IMPL_RELEASE(nsPICSElementObserver)    

NS_PICS nsresult NS_NewPICSElementObserver(nsIObserver** anObserver)
{
    if (anObserver == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    } 
    
    nsPICSElementObserver* it = new nsPICSElementObserver();

    if (it == 0) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return it->QueryInterface(kIPICSElementObserverIID, (void **) anObserver);
}

nsPICSElementObserver::nsPICSElementObserver()
{
    NS_INIT_REFCNT();
}

nsPICSElementObserver::~nsPICSElementObserver(void)
{

}

NS_IMETHODIMP nsPICSElementObserver::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{

  if( NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  *aInstancePtr = NULL;

  if( aIID.Equals ( kIPICSElementObserverIID )) {
    *aInstancePtr = (void*) ((nsIElementObserver*) this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if( aIID.Equals ( kIElementObserverIID )) {
    *aInstancePtr = (void*) ((nsIElementObserver*) this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if( aIID.Equals ( kIObserverIID )) {
    *aInstancePtr = (void*) ((nsIObserver*) this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if( aIID.Equals ( kISupportsIID )) {
    *aInstancePtr = (void*) (this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

const char* nsPICSElementObserver::GetTagNameAt(PRUint32 aTagIndex)
{
  if (aTagIndex == 0) {
    return "META";
  } if (aTagIndex == 1) {
    return "BODY";
  }else {
    return nsnull;
  }
}

NS_IMETHODIMP nsPICSElementObserver::Notify(PRUint32 aDocumentID, eHTMLTags aTag, 
                    PRUint32 numOfAttributes, const PRUnichar* nameArray[], 
                    const PRUnichar* valueArray[]) 
{
  nsresult rv;
  int status;
  nsIWebShellServices* ws;
//  nsString theURL(aSpec);
// char* url = aSpec.ToNewCString();
  nsIURL* uaURL = nsnull;
//  rv = NS_NewURL(&uaURL, nsString(aSpec));
 
  if(aTag == eHTMLTag_meta) {
    if(numOfAttributes >= 2) {
      const nsString& theValue1=valueArray[0];
      char *val1 = theValue1.ToNewCString();
      if(theValue1.EqualsIgnoreCase("\"PICS-LABEL\"")) {
        printf("\nReceived notification for a PICS-LABEl\n");
        const nsString& theValue2=valueArray[1];
        char *label = theValue2.ToNewCString();
        if (valueArray[numOfAttributes]) {
          const nsString& theURLValue=valueArray[numOfAttributes];
#ifndef NECKO
          rv = NS_NewURL(&uaURL, theURLValue);
#else
          NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
          if (NS_FAILED(rv)) return rv;

          nsIURI *uri = nsnull;
          const char *uriStr = theURLValue.GetBuffer();
          rv = service->NewURI(uriStr, nsnull, &uri);
          if (NS_FAILED(rv)) return rv;

          rv = uri->QueryInterface(nsIURL::GetIID(), (void**)&uaURL);
          NS_RELEASE(uri);
          if (NS_FAILED(rv)) return rv;
#endif // NECKO
        }
        nsIPICS *pics = NULL;
        rv = nsRepository::CreateInstance(kPICSCID,
								        NULL,
								        kIPICSIID,
								        (void **) &pics);
        if(rv == NS_OK) {
          pics->GetWebShell(aDocumentID, ws);
          if(ws) {
            status = pics->ProcessPICSLabel(label);
            if(uaURL)
              pics->SetNotified(ws, uaURL, PR_TRUE);

            if(status) {
              if(ws) {
                char * text = PR_GetEnv("NGLAYOUT_HOME");
                nsString mtemplateURL = text ? text : "resource:/res/samples/picstest1.html";
              //  ws->LoadURL(mtemplateURL, nsnull, nsnull);
                nsCharsetSource s;
                ws->SetRendering(PR_TRUE);
                ws->StopDocumentLoad();
                ws->LoadDocument("resource:/res/samples/picstest1.html", nsnull, s);
              }
            }
          }
        } 
      }
    }
  }
  return NS_OK;
    
}

NS_IMETHODIMP nsPICSElementObserver::Observe(nsISupports*, const PRUnichar*, const PRUnichar*) 
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////////////
// nsPICSElementObserverFactory Implementation

static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
NS_IMPL_ISUPPORTS(nsPICSElementObserverFactory, kIFactoryIID);

nsPICSElementObserverFactory::nsPICSElementObserverFactory(void)
{
    NS_INIT_REFCNT();
}

nsPICSElementObserverFactory::~nsPICSElementObserverFactory(void)
{

}

nsresult
nsPICSElementObserverFactory::CreateInstance(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (! aResult)
        return NS_ERROR_NULL_POINTER;
    
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    *aResult = nsnull;

    nsresult rv;
    nsIObserver* inst = nsnull;

    if (NS_FAILED(rv = NS_NewPICSElementObserver(&inst)))
        return rv;

    if (!inst)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = inst->QueryInterface(aIID, aResult);

    if (NS_FAILED(rv)) {
        *aResult = NULL;
    }
    return rv;
}

nsresult
nsPICSElementObserverFactory::LockFactory(PRBool aLock)
{
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////////////
