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
#include <stdio.h>
#include "nsIUnicharInputStream.h"
#include "nsIURL.h"
#ifdef NECKO
#include "nsIIOService.h"
#include "nsNeckoUtil.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#endif // NECKO
#include "nsCRT.h"
#include "nsString.h"
#include "prprf.h"
#include "prtime.h"

static nsString* ConvertCharacterSetName(const char* aName)
{
  return new nsString(aName);
}

int main(int argc, char** argv)
{
  if (3 != argc) {
    printf("usage: CvtURL url character-set-name\n");
    return -1;
  }

  char* characterSetName = argv[2];
  nsString* cset = ConvertCharacterSetName(characterSetName);
  if (PRInt32(cset) < 0) {
    printf("illegal character set name: '%s'\n", characterSetName);
    return -1;
  }

  // Create url object
  char* urlName = argv[1];
  nsIURI* url;
  nsresult rv;
#ifndef NECKO
  rv = NS_NewURL(&url, urlName);
#else
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsIURI *uri = nsnull;
  rv = service->NewURI(urlName, nsnull, &uri);
  if (NS_FAILED(rv)) return rv;

  rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&url);
  NS_RELEASE(uri);
#endif // NECKO
  if (NS_OK != rv) {
    printf("invalid URL: '%s'\n", urlName);
    return -1;
  }

  // Get an input stream from the url
  nsresult ec;
  nsIInputStream* in;
#ifndef NECKO
  ec = NS_OpenURL(url, &in);
#else
  ec = NS_OpenURI(&in, url);
#endif // NECKO
  if (nsnull == in) {
    printf("open of url('%s') failed: error=%x\n", urlName, ec);
    return -1;
  }

  // Translate the input using the argument character set id into unicode
  nsIUnicharInputStream* uin;
  rv = NS_NewConverterStream(&uin, nsnull, in, 0, cset);
  if (NS_OK != rv) {
    printf("can't create converter input stream: %d\n", rv);
    return -1;
  }

  // Read the input and write some output
  PRTime start = PR_Now();
  PRInt32 count = 0;
  for (;;) {
    PRUnichar buf[1000];
    PRUint32 nb;
    ec = uin->Read(buf, 0, 1000, &nb);
    if (ec < 0) {
      if (ec != NS_BASE_STREAM_EOF) {
        printf("i/o error: %d\n", ec);
      }
      break;
    }
    count += nb;
  }
  PRTime end = PR_Now();
  PRTime conversion, ustoms;
  LL_I2L(ustoms, 1000);
  LL_SUB(conversion, end, start);
  LL_DIV(conversion, conversion, ustoms);
  char buf[500];
  PR_snprintf(buf, sizeof(buf),
              "converting and discarding %d bytes took %lldms",
              count, conversion);
  puts(buf);

  // Release the objects
  in->Release();
  uin->Release();
  url->Release();

  return 0;
}
