/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *  Terry Hayes <thayes@netscape.com>
 */

#include "nsMemory.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsIInterfaceRequestor.h"

#include "nsICMSMessage.h"

#include "nsCMSMessage.h"
#include "nsNSSCertificate.h"
#include "nsNSSHelper.h"

#include <string.h>
#include "plbase64.h"
#include "cert.h"
#include "cms.h"

#include "nsIServiceManager.h"
#include "nsIPref.h"

// Standard ISupports implementation
// NOTE: Should these be the thread-safe versions?
NS_IMPL_ISUPPORTS1(nsCMSMessage, nsICMSMessage)

// nsCMSMessage constructor
nsCMSMessage::nsCMSMessage()
: mMessage(0)
{
  // initialize superclass
  NS_INIT_ISUPPORTS();
}

// nsCMSMessage destructor
nsCMSMessage::~nsCMSMessage()
{
  if (mMessage) NSS_CMSMessage_Destroy(mMessage);
}

// nssCMSMessage initializer
nsresult nsCMSMessage::
Init()
{
  nsresult rv = NS_OK;

  mMessage = NSS_CMSMessage_Create(NULL);
  if (!mMessage) rv = NS_ERROR_OUT_OF_MEMORY;

  return rv;
}

/* boolean isSigned(); */
NS_IMETHODIMP nsCMSMessage::
IsSigned(PRBool * _retval)
{
  nsresult rv = NS_OK;

  /* Use location of object as indicator signed state */
  *_retval = (((long)mMessage & 0x80) != 0);

  return rv;
}

/*****
 * nsCMSManager
 *****/

// Standard ISupports implementation
NS_IMPL_ISUPPORTS1(nsCMSManager, nsICMSManager)

// nsCMSManager constructor
nsCMSManager::nsCMSManager()
{
  // initialize superclass
  NS_INIT_ISUPPORTS();
}

// nsCMSMessage destructor
nsCMSManager::~nsCMSManager()
{
}

// nsCMSManager::GetEncryptionCert
NS_IMETHODIMP nsCMSManager::
GetEncryptionCert(char ** _retval)
{
  nsresult rv = NS_OK;
  CERTCertificate *cert = 0;
  nsXPIDLCString nickname;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();

  *_retval = 0;

  static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
  nsCOMPtr<nsIPref> prefs = do_GetService(kPrefCID, &rv);
  if (NS_FAILED(rv)) goto done;

  rv = prefs->GetCharPref("aim.session.certnickname",
                          getter_Copies(nickname));
  if (NS_FAILED(rv)) goto done;

printf("Cert Nickname: %s\n", nickname.get());

  /* Find a good cert in the user's database */
  cert = CERT_FindUserCertByUsage(CERT_GetDefaultCertDB(), (char*)nickname.get(), 
           certUsageEmailRecipient, PR_TRUE, ctx);

  if (!cert) { goto done; }  /* Success, but no value */

  /* Convert the DER to a BASE64 String */
  encode(cert->derCert.data, cert->derCert.len, _retval);

done:
  if (cert) CERT_DestroyCertificate(cert);
  return rv;
}


// nsCMSManager::DecodeCert
nsresult nsCMSManager::
DecodeCert(const char *value, nsIX509Cert ** _retval)
{
  nsresult rv = NS_OK;
  PRInt32 length;
  unsigned char *data = 0;

  *_retval = 0;

  if (!value) { return NS_ERROR_FAILURE; }

  rv = decode(value, &data, &length);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIX509Cert> cert =  new nsNSSCertificate((char *)data, length);

  *_retval = cert;
  NS_IF_ADDREF(*_retval);

  nsCRT::free((char*)data);
  return rv;
}

// nsCMSManager::SendMessage
nsresult nsCMSManager::
SendMessage(const char *msg, const char *base64Cert, char ** _retval)
{
  nsresult rv = NS_OK;
  CERTCertificate *cert = 0;
  NSSCMSMessage *cmsMsg = 0;
  unsigned char *certDER = 0;
  PRInt32 derLen;
  NSSCMSEnvelopedData *env;
  NSSCMSContentInfo *cinfo;
  NSSCMSRecipientInfo *rcpt;
  SECItem item;
  SECItem output;
  PLArenaPool *arena = PORT_NewArena(1024);
  SECStatus s;

  /* Step 0. Create a CMS Message */
  cmsMsg = NSS_CMSMessage_Create(NULL);
  if (!cmsMsg) { rv = NS_ERROR_FAILURE; goto done; }

  /* Step 1.  Import the certificate into NSS */
  rv = decode(base64Cert, &certDER, &derLen);
  if (NS_FAILED(rv)) goto done;

  cert = CERT_DecodeCertFromPackage((char *)certDER, derLen);
  if (!cert) { rv = NS_ERROR_FAILURE; goto done; }

#if 0
  cert->dbhandle = CERT_GetDefaultCertDB();  /* work-around */
#endif

  /* Step 2.  Get a signature cert */

  /* Step 3. Build inner (signature) content */

  /* Step 4. Build outer (enveloped) content */
  env = NSS_CMSEnvelopedData_Create(cmsMsg, SEC_OID_DES_EDE3_CBC, 0);
  if (!env) { rv = NS_ERROR_FAILURE; goto done; }

  cinfo = NSS_CMSEnvelopedData_GetContentInfo(env);
  item.data = (unsigned char *)msg;
  item.len = strlen(msg);  /* XPCOM equiv?? */
  s = NSS_CMSContentInfo_SetContent_Data(cmsMsg, cinfo, 0, PR_FALSE);
  if (s != SECSuccess) { rv = NS_ERROR_FAILURE; goto done; }

  rcpt = NSS_CMSRecipientInfo_Create(cmsMsg, cert);
  if (!rcpt) { rv = NS_ERROR_FAILURE; goto done; }

  s = NSS_CMSEnvelopedData_AddRecipient(env, rcpt);
  if (s != SECSuccess) { rv = NS_ERROR_FAILURE; goto done; }

  /* Step 5. Add content to message */
  cinfo = NSS_CMSMessage_GetContentInfo(cmsMsg);
  s = NSS_CMSContentInfo_SetContent_EnvelopedData(cmsMsg, cinfo, env);
  if (s != SECSuccess) { rv = NS_ERROR_FAILURE; goto done; }
  
  /* Step 6. Encode */
  NSSCMSEncoderContext *ecx;

  output.data = 0; output.len = 0;
  ecx = NSS_CMSEncoder_Start(cmsMsg, 0, 0, &output, arena,
            0, 0, 0, 0, 0, 0);
  if (!ecx) { rv = NS_ERROR_FAILURE; goto done; }

  s = NSS_CMSEncoder_Update(ecx, msg, strlen(msg));
  if (s != SECSuccess) { rv = NS_ERROR_FAILURE; goto done; }

  s = NSS_CMSEncoder_Finish(ecx);
  if (s != SECSuccess) { rv = NS_ERROR_FAILURE; goto done; }

  /* Step 7. Base64 encode and return the result */
  rv = encode(output.data, output.len, _retval);

done:
  if (certDER) nsCRT::free((char *)certDER);
  if (cert) CERT_DestroyCertificate(cert);
  if (cmsMsg) NSS_CMSMessage_Destroy(cmsMsg);
  if (arena) PORT_FreeArena(arena, PR_FALSE);  /* PR_FALSE? */

  return rv;
}

/*
 * nsCMSManager::ReceiveMessage
 */
nsresult nsCMSManager::
ReceiveMessage(const char *msg, char **_retval)
{
  nsresult rv = NS_OK;
  NSSCMSDecoderContext *dcx;
  unsigned char *der = 0;
  PRInt32 derLen;
  NSSCMSMessage *cmsMsg = 0;
  SECItem *content;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();

  /* Step 1. Decode the base64 wrapper */
  rv = decode(msg, &der, &derLen);
  if (NS_FAILED(rv)) goto done;

  dcx = NSS_CMSDecoder_Start(0, 0, 0, /* pw */ 0, ctx, /* key */ 0, 0);
  if (!dcx) { rv = NS_ERROR_FAILURE; goto done; }

  (void)NSS_CMSDecoder_Update(dcx, (char *)der, derLen);
  cmsMsg = NSS_CMSDecoder_Finish(dcx);
  if (!cmsMsg) { rv = NS_ERROR_FAILURE; goto done; } /* Memory leak on dcx?? */

  content = NSS_CMSMessage_GetContent(cmsMsg);
  if (!content) { rv = NS_ERROR_FAILURE; goto done; }

  /* Copy the data */
  *_retval = (char*)malloc(content->len+1);
  memcpy(*_retval, content->data, content->len);
  (*_retval)[content->len] = 0;

done:
  if (der) free(der);
  if (cmsMsg) NSS_CMSMessage_Destroy(cmsMsg);

  return rv;
}

nsresult nsCMSManager::
encode(const unsigned char *data, PRInt32 dataLen, char **_retval)
{
    nsresult rv = NS_OK;

    *_retval = PL_Base64Encode((const char *)data, dataLen, NULL);
    if (!*_retval) { rv = NS_ERROR_OUT_OF_MEMORY; goto loser; }

loser:
    return rv;
}

nsresult nsCMSManager::
decode(const char *data, unsigned char **result, PRInt32 * _retval)
{
    nsresult rv = NS_OK;
    PRUint32 len = PL_strlen(data);
    int adjust = 0;

    /* Compute length adjustment */
    if (data[len-1] == '=') {
      adjust++;
      if (data[len-2] == '=') adjust++;
    }

    *result = (unsigned char *)PL_Base64Decode(data, len, NULL);
    if (!*result) { rv = NS_ERROR_ILLEGAL_VALUE; goto loser; }

    *_retval = (len*3)/4 - adjust;

loser:
    return rv;
}
