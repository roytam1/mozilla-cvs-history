/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 *  Ian McGreer <mcgreer@netscape.com>
 *  Javier Delgadillo <javi@netscape.com>
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 *
 */

#include "prmem.h"
#include "prerror.h"
#include "prprf.h"

#include "nsNSSComponent.h" // for PIPNSS string bundle calls.
#include "nsCOMPtr.h"
#include "nsArray.h"
#include "nsNSSCertificate.h"
#include "nsNSSCertValidity.h"
#include "nsPKCS12Blob.h"
#include "nsPK11TokenDB.h"
#include "nsIX509Cert.h"
#include "nsISMimeCert.h"
#include "nsNSSASN1Object.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"
#include "nsILocaleService.h"
#include "nsIURI.h"
#include "nsTime.h"
#include "nsIProxyObjectManager.h"
#include "nsCRT.h"
#include "nsAutoLock.h"
#include "nsUsageArrayHelper.h"
#include "nsICertificateDialogs.h"
#include "nsNSSCertHelper.h"

#include "nspr.h"
extern "C" {
#include "pk11func.h"
#include "certdb.h"
#include "cert.h"
#include "secerr.h"
#include "nssb64.h"
#include "secasn1.h"
#include "secder.h"
}
#include "ssl.h"
#include "ocsp.h"
#include "plbase64.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);
static NS_DEFINE_CID(kDateTimeFormatCID, NS_DATETIMEFORMAT_CID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID);


/* nsNSSCertificate */

NS_IMPL_THREADSAFE_ISUPPORTS2(nsNSSCertificate, nsIX509Cert,
                                                nsISMimeCert)

nsNSSCertificate*
nsNSSCertificate::ConstructFromDER(char *certDER, int derLen)
{
  if (!certDER || !derLen)
    return nsnull;

  CERTCertificate *aCert = CERT_DecodeCertFromPackage(certDER, derLen);
  
  if (!aCert)
    return nsnull;

  if(aCert->dbhandle == nsnull)
  {
    aCert->dbhandle = CERT_GetDefaultCertDB();
  }

  // We don't want the new NSS cert to be dupped, therefore we create our instance
  // with a NULL pointer first, and set the contained cert later.

  nsNSSCertificate *newObject = new nsNSSCertificate(nsnull);
  
  if (!newObject)
  {
    CERT_DestroyCertificate(aCert);
    return nsnull;
  }
  
  newObject->mCert = aCert;
  return newObject;
}

nsNSSCertificate::nsNSSCertificate(CERTCertificate *cert) : 
                                           mPermDelete(PR_FALSE),
                                           mCertType(nsIX509Cert::UNKNOWN_CERT)
{
  NS_INIT_ISUPPORTS();

  if (cert) 
    mCert = CERT_DupCertificate(cert);
  else
    mCert = nsnull;
}

nsNSSCertificate::~nsNSSCertificate()
{
  if (mPermDelete) {
    if (mCertType == nsNSSCertificate::USER_CERT) {
      nsCOMPtr<nsIInterfaceRequestor> cxt = new PipUIContext();
      PK11_DeleteTokenCertAndKey(mCert, cxt);
      CERT_DestroyCertificate(mCert);
    } else if (!PK11_IsReadOnly(mCert->slot)) {
      // If the cert isn't a user cert and it is on an external token, 
      // then we'll just leave it as untrusted, but won't delete it 
      // from the cert db.
      SEC_DeletePermCertificate(mCert);
    }
  } else {
    if (mCert)
      CERT_DestroyCertificate(mCert);
  }
}

nsresult
nsNSSCertificate::SetCertType(PRUint32 aCertType)
{
  mCertType = aCertType;
  return NS_OK;
}

nsresult
nsNSSCertificate::GetCertType(PRUint32 *aCertType)
{
  *aCertType = mCertType;
  return NS_OK;
}

nsresult
nsNSSCertificate::MarkForPermDeletion()
{
  // make sure user is logged in to the token
  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();

  if (PK11_NeedLogin(mCert->slot)
      && !PK11_NeedUserInit(mCert->slot)
      && !PK11_IsInternal(mCert->slot))
  {
    if (SECSuccess != PK11_Authenticate(mCert->slot, PR_TRUE, ctx))
    {
      return NS_ERROR_FAILURE;
    }
  }

  mPermDelete = PR_TRUE;
  return NS_OK;
}

nsresult
nsNSSCertificate::FormatUIStrings(const nsAutoString &nickname, nsAutoString &nickWithSerial, nsAutoString &details)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIProxyObjectManager> proxyman(do_GetService(NS_XPCOMPROXY_CONTRACTID, &rv));
  
  if (NS_FAILED(rv) || !proxyman) {
    return NS_ERROR_FAILURE;
  }
  
  NS_DEFINE_CID(nssComponentCID, NS_NSSCOMPONENT_CID);
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(nssComponentCID, &rv));

  if (NS_FAILED(rv) || !nssComponent) {
    return NS_ERROR_FAILURE;
  }
  
  nsCOMPtr<nsIX509Cert> x509Proxy;
  proxyman->GetProxyForObject( NS_UI_THREAD_EVENTQ,
                               nsIX509Cert::GetIID(),
                               NS_STATIC_CAST(nsIX509Cert*, this),
                               PROXY_SYNC | PROXY_ALWAYS,
                               getter_AddRefs(x509Proxy));

  if (!x509Proxy) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    rv = NS_OK;

    nsAutoString info;
    nsAutoString temp1;

    nickWithSerial.Append(nickname);

    if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoIssuedFor").get(), info))) {
      details.Append(info);
      details.Append(NS_LITERAL_STRING("\n"));
    }

    if (NS_SUCCEEDED(x509Proxy->GetSubjectName(temp1)) && !temp1.IsEmpty()) {
      details.Append(NS_LITERAL_STRING("  "));
      if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSubject").get(), info))) {
        details.Append(info);
        details.Append(NS_LITERAL_STRING(": "));
      }
      details.Append(temp1);
      details.Append(NS_LITERAL_STRING("\n"));
    }

    if (NS_SUCCEEDED(x509Proxy->GetSerialNumber(temp1)) && !temp1.IsEmpty()) {
      details.Append(NS_LITERAL_STRING("  "));
      if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSerialNo").get(), info))) {
        details.Append(info);
        details.Append(NS_LITERAL_STRING(": "));
      }
      details.Append(temp1);

      nickWithSerial.Append(NS_LITERAL_STRING(" ["));
      nickWithSerial.Append(temp1);
      nickWithSerial.Append(NS_LITERAL_STRING("]"));

      details.Append(NS_LITERAL_STRING("\n"));
    }


    {
      nsCOMPtr<nsIX509CertValidity> validity;
      nsCOMPtr<nsIX509CertValidity> originalValidity;
      rv = x509Proxy->GetValidity(getter_AddRefs(originalValidity));
      if (NS_SUCCEEDED(rv) && originalValidity) {
        proxyman->GetProxyForObject( NS_UI_THREAD_EVENTQ,
                                     nsIX509CertValidity::GetIID(),
                                     originalValidity,
                                     PROXY_SYNC | PROXY_ALWAYS,
                                     getter_AddRefs(validity));
      }

      if (validity) {
        details.Append(NS_LITERAL_STRING("  "));
        if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoValid").get(), info))) {
          details.Append(info);
        }

        if (NS_SUCCEEDED(validity->GetNotBeforeLocalTime(temp1)) && !temp1.IsEmpty()) {
          details.Append(NS_LITERAL_STRING(" "));
          if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoFrom").get(), info))) {
            details.Append(info);
          }
          details.Append(NS_LITERAL_STRING(" "));
          details.Append(temp1);
        }

        if (NS_SUCCEEDED(validity->GetNotAfterLocalTime(temp1)) && !temp1.IsEmpty()) {
          details.Append(NS_LITERAL_STRING(" "));
          if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoTo").get(), info))) {
            details.Append(info);
          }
          details.Append(NS_LITERAL_STRING(" "));
          details.Append(temp1);
        }

        details.Append(NS_LITERAL_STRING("\n"));
      }
    }

    PRUint32 tempInt = 0;
    if (NS_SUCCEEDED(x509Proxy->GetPurposes(&tempInt, temp1)) && !temp1.IsEmpty()) {
      details.Append(NS_LITERAL_STRING("  "));
      if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoPurposes").get(), info))) {
        details.Append(info);
      }
      details.Append(NS_LITERAL_STRING(": "));
      details.Append(temp1);
      details.Append(NS_LITERAL_STRING("\n"));
    }

    if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoIssuedBy").get(), info))) {
      details.Append(info);
      details.Append(NS_LITERAL_STRING("\n"));
    }

    if (NS_SUCCEEDED(x509Proxy->GetIssuerName(temp1)) && !temp1.IsEmpty()) {
      details.Append(NS_LITERAL_STRING("  "));
      if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSubject").get(), info))) {
        details.Append(info);
        details.Append(NS_LITERAL_STRING(": "));
      }
      details.Append(temp1);
      details.Append(NS_LITERAL_STRING("\n"));
    }

    /*
      the above produces output the following output:

      Issued to: 
        Subject: $subjectName
        Serial number: $serialNumber
        Valid from: $starting_date to $expriation_date
        Purposes: $purposes
      Issued by:
        Subject: $issuerName
    */
  }
  
  return rv;
}


/* readonly attribute string dbKey; */
NS_IMETHODIMP 
nsNSSCertificate::GetDbKey(char * *aDbKey)
{
  SECItem key;

  NS_ENSURE_ARG(aDbKey);
  *aDbKey = nsnull;
  key.len = NS_NSS_LONG*4+mCert->serialNumber.len+mCert->derIssuer.len;
  key.data = (unsigned char *)nsMemory::Alloc(key.len);
  NS_NSS_PUT_LONG(0,key.data); // later put moduleID
  NS_NSS_PUT_LONG(0,&key.data[NS_NSS_LONG]); // later put slotID
  NS_NSS_PUT_LONG(mCert->serialNumber.len,&key.data[NS_NSS_LONG*2]);
  NS_NSS_PUT_LONG(mCert->derIssuer.len,&key.data[NS_NSS_LONG*3]);
  memcpy(&key.data[NS_NSS_LONG*4], mCert->serialNumber.data,
         mCert->serialNumber.len);
  memcpy(&key.data[NS_NSS_LONG*4+mCert->serialNumber.len],
         mCert->derIssuer.data, mCert->derIssuer.len);
  
  *aDbKey = NSSBase64_EncodeItem(nsnull, nsnull, 0, &key);
  nsMemory::Free(key.data); // SECItem is a 'c' type without a destrutor
  return (*aDbKey) ? NS_OK : NS_ERROR_FAILURE;
}

/* readonly attribute string windowTitle; */
NS_IMETHODIMP 
nsNSSCertificate::GetWindowTitle(char * *aWindowTitle)
{
  NS_ENSURE_ARG(aWindowTitle);
  if (mCert) {
    if (mCert->nickname) {
      *aWindowTitle = PL_strdup(mCert->nickname);
    } else {
      *aWindowTitle = CERT_GetCommonName(&mCert->subject);
      if (!*aWindowTitle) {
        *aWindowTitle = PL_strdup(mCert->subjectName);
      }
    }
  } else {
    NS_ASSERTION(0,"Somehow got nsnull for mCertificate in nsNSSCertificate.");
    *aWindowTitle = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetNickname(nsAString &_nickname)
{
  const char *nickname = (mCert->nickname) ? mCert->nickname : "(no nickname)";
  _nickname = NS_ConvertUTF8toUCS2(nickname);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetEmailAddress(nsAString &_emailAddress)
{
  const char *email = (mCert->emailAddr) ? mCert->emailAddr : "(no email address)";
  _emailAddress = NS_ConvertUTF8toUCS2(email);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetCommonName(nsAString &aCommonName)
{
  aCommonName.Truncate();
  if (mCert) {
    char *commonName = CERT_GetCommonName(&mCert->subject);
    if (commonName) {
      aCommonName = NS_ConvertUTF8toUCS2(commonName);
      PORT_Free(commonName);
    } /*else {
      *aCommonName = ToNewUnicode(NS_LITERAL_STRING("<not set>")), 
    }*/
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetOrganization(nsAString &aOrganization)
{
  aOrganization.Truncate();
  if (mCert) {
    char *organization = CERT_GetOrgName(&mCert->subject);
    if (organization) {
      aOrganization = NS_ConvertUTF8toUCS2(organization);
      PORT_Free(organization);
    } /*else {
      *aOrganization = ToNewUnicode(NS_LITERAL_STRING("<not set>")), 
    }*/
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetIssuerCommonName(nsAString &aCommonName)
{
  aCommonName.Truncate();
  if (mCert) {
    char *commonName = CERT_GetCommonName(&mCert->issuer);
    if (commonName) {
      aCommonName = NS_ConvertUTF8toUCS2(commonName);
      PORT_Free(commonName);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetIssuerOrganization(nsAString &aOrganization)
{
  aOrganization.Truncate();
  if (mCert) {
    char *organization = CERT_GetOrgName(&mCert->issuer);
    if (organization) {
      aOrganization = NS_ConvertUTF8toUCS2(organization);
      PORT_Free(organization);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetIssuerOrganizationUnit(nsAString &aOrganizationUnit)
{
  aOrganizationUnit.Truncate();
  if (mCert) {
    char *organizationUnit = CERT_GetOrgUnitName(&mCert->issuer);
    if (organizationUnit) {
      aOrganizationUnit = NS_ConvertUTF8toUCS2(organizationUnit);
      PORT_Free(organizationUnit);
    }
  }
  return NS_OK;
}

/* readonly attribute nsIX509Cert issuer; */
NS_IMETHODIMP 
nsNSSCertificate::GetIssuer(nsIX509Cert * *aIssuer)
{
  NS_ENSURE_ARG(aIssuer);
  *aIssuer = nsnull;
  CERTCertificate *issuer;
  issuer = CERT_FindCertIssuer(mCert, PR_Now(), certUsageSSLClient);
  if (issuer) {
    nsCOMPtr<nsIX509Cert> cert = new nsNSSCertificate(issuer);
    *aIssuer = cert;
    NS_ADDREF(*aIssuer);
    CERT_DestroyCertificate(issuer);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetOrganizationalUnit(nsAString &aOrganizationalUnit)
{
  aOrganizationalUnit.Truncate();
  if (mCert) {
    char *orgunit = CERT_GetOrgUnitName(&mCert->subject);
    if (orgunit) {
      aOrganizationalUnit = NS_ConvertUTF8toUCS2(orgunit);
      PORT_Free(orgunit);
    } /*else {
      *aOrganizationalUnit = ToNewUnicode(NS_LITERAL_STRING("<not set>")), 
    }*/
  }
  return NS_OK;
}

/* 
 * nsIEnumerator getChain(); 
 */
NS_IMETHODIMP
nsNSSCertificate::GetChain(nsIArray **_rvChain)
{
  NS_ENSURE_ARG(_rvChain);
  nsresult rv;
  /* Get the cert chain from NSS */
  CERTCertList *nssChain = NULL;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Getting chain for \"%s\"\n", mCert->nickname));
  // XXX This function is buggy - if it can't find the issuer, it crashes
  //     on a null pointer.  Will have to wait until it is fixed in NSS.
#ifdef NSS_CHAIN_BUG_FIXED
  nssChain = CERT_GetCertChainFromCert(mCert, PR_Now(), certUsageSSLClient);
  if (!nssChain)
    return NS_ERROR_FAILURE;
  /* enumerate the chain for scripting purposes */
  nsCOMPtr<nsIMutableArray> array;
  rv = NS_NewArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) { 
    goto done; 
  }
  for (node = CERT_LIST_HEAD(nssChain);
       !CERT_LIST_END(node, nssChain);
       node = CERT_LIST_NEXT(node)) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("adding %s to chain\n", node->cert->nickname));
    nsCOMPtr<nsIX509Cert> cert = new nsNSSCertificate(node->cert);
    array->AppendElement(cert, PR_FALSE);
  }
#else // workaround here
  CERTCertificate *cert = nsnull;
  /* enumerate the chain for scripting purposes */
  nsCOMPtr<nsIMutableArray> array;
  rv = NS_NewArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) { 
    goto done; 
  }
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Getting chain for \"%s\"\n", mCert->nickname));
  cert = CERT_DupCertificate(mCert);
  while (cert) {
    nsCOMPtr<nsIX509Cert> pipCert = new nsNSSCertificate(cert);
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("adding %s to chain\n", cert->nickname));
    array->AppendElement(pipCert, PR_FALSE);
    PRBool wantToBreak = PR_FALSE;
    CERTCertificate *next_cert = nsnull;
    if (SECITEM_CompareItem(&cert->derIssuer, &cert->derSubject) == SECEqual) {
      wantToBreak = PR_TRUE;
    }
    else {
      next_cert = CERT_FindCertIssuer(cert, PR_Now(), certUsageSSLClient);
    }
    CERT_DestroyCertificate(cert);
    if (wantToBreak) {
      break;
    }
    cert = next_cert;
  }
#endif // NSS_CHAIN_BUG_FIXED
  *_rvChain = array;
  NS_IF_ADDREF(*_rvChain);
  rv = NS_OK;
done:
  if (nssChain)
    CERT_DestroyCertList(nssChain);
  return rv;
}

NS_IMETHODIMP
nsNSSCertificate::GetSubjectName(nsAString &_subjectName)
{
  _subjectName.Truncate();
  if (mCert->subjectName) {
    _subjectName = NS_ConvertUTF8toUCS2(mCert->subjectName);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNSSCertificate::GetIssuerName(nsAString &_issuerName)
{
  _issuerName.Truncate();
  if (mCert->issuerName) {
    _issuerName = NS_ConvertUTF8toUCS2(mCert->issuerName);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNSSCertificate::GetSerialNumber(nsAString &_serialNumber)
{
  _serialNumber.Truncate();
  nsXPIDLCString tmpstr; tmpstr.Adopt(CERT_Hexify(&mCert->serialNumber, 1));
  if (tmpstr.get()) {
    _serialNumber = NS_ConvertASCIItoUCS2(tmpstr);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNSSCertificate::GetSha1Fingerprint(nsAString &_sha1Fingerprint)
{
  _sha1Fingerprint.Truncate();
  unsigned char fingerprint[20];
  SECItem fpItem;
  memset(fingerprint, 0, sizeof fingerprint);
  PK11_HashBuf(SEC_OID_SHA1, fingerprint, 
               mCert->derCert.data, mCert->derCert.len);
  fpItem.data = fingerprint;
  fpItem.len = SHA1_LENGTH;
  nsXPIDLCString fpStr; fpStr.Adopt(CERT_Hexify(&fpItem, 1));
  if (fpStr.get()) {
    _sha1Fingerprint = NS_ConvertASCIItoUCS2(fpStr);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNSSCertificate::GetMd5Fingerprint(nsAString &_md5Fingerprint)
{
  _md5Fingerprint.Truncate();
  unsigned char fingerprint[20];
  SECItem fpItem;
  memset(fingerprint, 0, sizeof fingerprint);
  PK11_HashBuf(SEC_OID_MD5, fingerprint, 
               mCert->derCert.data, mCert->derCert.len);
  fpItem.data = fingerprint;
  fpItem.len = MD5_LENGTH;
  nsXPIDLCString fpStr; fpStr.Adopt(CERT_Hexify(&fpItem, 1));
  if (fpStr.get()) {
    _md5Fingerprint = NS_ConvertASCIItoUCS2(fpStr);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNSSCertificate::GetIssuedDate(nsAString &_issuedDate)
{
  _issuedDate.Truncate();
  nsresult rv;
  PRTime beforeTime;
  nsCOMPtr<nsIX509CertValidity> validity;
  rv = this->GetValidity(getter_AddRefs(validity));
  if (NS_FAILED(rv)) return rv;
  rv = validity->GetNotBefore(&beforeTime);
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIDateTimeFormat> dateFormatter =
     do_CreateInstance(kDateTimeFormatCID, &rv);
  if (NS_FAILED(rv)) return rv;
  nsAutoString date;
  dateFormatter->FormatPRTime(nsnull, kDateFormatShort, kTimeFormatNone,
                              beforeTime, date);
  _issuedDate = date;
  return NS_OK;
}

nsresult
nsNSSCertificate::GetSortableDate(PRTime aTime, nsAString &_aSortableDate)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(aTime, PR_GMTParameters, &explodedTime);
  char datebuf[20]; // 4 + 2 + 2 + 2 + 2 + 2 + 1 = 15
  if (0 != PR_FormatTime(datebuf, sizeof(datebuf), "%Y%m%d%H%M%S", &explodedTime)) {
    _aSortableDate = NS_ConvertASCIItoUCS2(nsDependentCString(datebuf));
    return NS_OK;
  }
  else
    return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsNSSCertificate::GetIssuedDateSortable(nsAString &_issuedDate)
{
  _issuedDate.Truncate();
  nsresult rv;
  PRTime beforeTime;
  nsCOMPtr<nsIX509CertValidity> validity;
  rv = this->GetValidity(getter_AddRefs(validity));
  if (NS_FAILED(rv)) return rv;
  rv = validity->GetNotBefore(&beforeTime);
  if (NS_FAILED(rv)) return rv;
  return GetSortableDate(beforeTime, _issuedDate);
}

NS_IMETHODIMP
nsNSSCertificate::GetExpiresDate(nsAString &_expiresDate)
{
  _expiresDate.Truncate();
  nsresult rv;
  PRTime afterTime;
  nsCOMPtr<nsIX509CertValidity> validity;
  rv = this->GetValidity(getter_AddRefs(validity));
  if (NS_FAILED(rv)) return rv;
  rv = validity->GetNotAfter(&afterTime);
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIDateTimeFormat> dateFormatter =
     do_CreateInstance(kDateTimeFormatCID, &rv);
  if (NS_FAILED(rv)) return rv;
  nsAutoString date;
  dateFormatter->FormatPRTime(nsnull, kDateFormatShort, kTimeFormatNone,
                              afterTime, date);
  _expiresDate = date;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetExpiresDateSortable(nsAString &_expiresDate)
{
  _expiresDate.Truncate();
  nsresult rv;
  PRTime afterTime;
  nsCOMPtr<nsIX509CertValidity> validity;
  rv = this->GetValidity(getter_AddRefs(validity));
  if (NS_FAILED(rv)) return rv;
  rv = validity->GetNotAfter(&afterTime);
  if (NS_FAILED(rv)) return rv;
  return GetSortableDate(afterTime, _expiresDate);
}

NS_IMETHODIMP
nsNSSCertificate::GetTokenName(nsAString &aTokenName)
{
  aTokenName.Truncate();
  if (mCert) {
    // HACK alert
    // When the trust of a builtin cert is modified, NSS copies it into the
    // cert db.  At this point, it is now "managed" by the user, and should
    // not be listed with the builtins.  However, in the collection code
    // used by PK11_ListCerts, the cert is found in the temp db, where it
    // has been loaded from the token.  Though the trust is correct (grabbed
    // from the cert db), the source is wrong.  I believe this is a safe
    // way to work around this.
    if (mCert->slot) {
      char *token = PK11_GetTokenName(mCert->slot);
      if (token) {
        aTokenName = NS_ConvertUTF8toUCS2(token);
      }
    } else {
      nsresult rv;
      nsAutoString tok;
      nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
      if (NS_FAILED(rv)) return rv;
      rv = nssComponent->GetPIPNSSBundleString(
                                NS_LITERAL_STRING("InternalToken").get(), tok);
      if (NS_SUCCEEDED(rv))
        aTokenName = tok;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetUsesOCSP(PRBool *aUsesOCSP)
{
  nsCOMPtr<nsIPref> prefService = do_GetService(NS_PREF_CONTRACTID);
 
  PRInt32 ocspEnabled; 
  prefService->GetIntPref("security.OCSP.enabled", &ocspEnabled);
  if (ocspEnabled == 2) {
    *aUsesOCSP = PR_TRUE;
  } else if (ocspEnabled == 1) {
    nsXPIDLCString ocspLocation;
    ocspLocation.Adopt(CERT_GetOCSPAuthorityInfoAccessLocation(mCert));
    *aUsesOCSP = (ocspLocation) ? PR_TRUE : PR_FALSE;
  } else {
    *aUsesOCSP = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetRawDER(PRUint32 *aLength, PRUint8 **aArray)
{
  if (mCert) {
    *aArray = (PRUint8 *)mCert->derCert.data;
    *aLength = mCert->derCert.len;
    return NS_OK;
  }
  *aLength = 0;
  return NS_ERROR_FAILURE;
}

CERTCertificate *
nsNSSCertificate::GetCert()
{
  return (mCert) ? CERT_DupCertificate(mCert) : nsnull;
}

NS_IMETHODIMP
nsNSSCertificate::GetValidity(nsIX509CertValidity **aValidity)
{
  NS_ENSURE_ARG(aValidity);
  nsX509CertValidity *validity = new nsX509CertValidity(mCert);
  if (nsnull == validity)
   return  NS_ERROR_OUT_OF_MEMORY; 

  NS_ADDREF(validity);
  *aValidity = NS_STATIC_CAST(nsIX509CertValidity*, validity);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::VerifyForUsage(PRUint32 usage, PRUint32 *verificationResult)
{
  NS_ENSURE_ARG(verificationResult);

  SECCertUsage nss_usage;
  
  switch (usage)
  {
    case CERT_USAGE_SSLClient:
      nss_usage = certUsageSSLClient;
      break;

    case CERT_USAGE_SSLServer:
      nss_usage = certUsageSSLServer;
      break;

    case CERT_USAGE_SSLServerWithStepUp:
      nss_usage = certUsageSSLServerWithStepUp;
      break;

    case CERT_USAGE_SSLCA:
      nss_usage = certUsageSSLCA;
      break;

    case CERT_USAGE_EmailSigner:
      nss_usage = certUsageEmailSigner;
      break;

    case CERT_USAGE_EmailRecipient:
      nss_usage = certUsageEmailRecipient;
      break;

    case CERT_USAGE_ObjectSigner:
      nss_usage = certUsageObjectSigner;
      break;

    case CERT_USAGE_UserCertImport:
      nss_usage = certUsageUserCertImport;
      break;

    case CERT_USAGE_VerifyCA:
      nss_usage = certUsageVerifyCA;
      break;

    case CERT_USAGE_ProtectedObjectSigner:
      nss_usage = certUsageProtectedObjectSigner;
      break;

    case CERT_USAGE_StatusResponder:
      nss_usage = certUsageStatusResponder;
      break;

    case CERT_USAGE_AnyCA:
      nss_usage = certUsageAnyCA;
      break;

    default:
      return NS_ERROR_FAILURE;
  }

  CERTCertDBHandle *defaultcertdb = CERT_GetDefaultCertDB();

  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         nss_usage, NULL) == SECSuccess)
  {
    *verificationResult = VERIFIED_OK;
  }
  else
  {
    int err = PR_GetError();

    // this list was cloned from verifyFailed

    switch (err)
    {
      case SEC_ERROR_INADEQUATE_KEY_USAGE:
      case SEC_ERROR_INADEQUATE_CERT_TYPE:
        *verificationResult = USAGE_NOT_ALLOWED;
        break;

      case SEC_ERROR_REVOKED_CERTIFICATE:
        *verificationResult = CERT_REVOKED;
        break;

      case SEC_ERROR_EXPIRED_CERTIFICATE:
        *verificationResult = CERT_EXPIRED;
        break;
        
      case SEC_ERROR_UNTRUSTED_CERT:
        *verificationResult = CERT_NOT_TRUSTED;
        break;
        
      case SEC_ERROR_UNTRUSTED_ISSUER:
        *verificationResult = ISSUER_NOT_TRUSTED;
        break;
        
      case SEC_ERROR_UNKNOWN_ISSUER:
        *verificationResult = ISSUER_UNKNOWN;
        break;
        
      case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
        *verificationResult = INVALID_CA;
        break;
        
      case SEC_ERROR_CERT_USAGES_INVALID:
      default:
        *verificationResult = NOT_VERIFIED_UNKNOWN; 
        break;
    }
  }
  
  return NS_OK;  
}


/*
 * void getUsages(out PRUint32 verified,
 *                out PRUint32 count, 
 *                [retval, array, size_is(count)] out wstring usages);
 */
NS_IMETHODIMP
nsNSSCertificate::GetUsages(PRUint32 *_verified,
                            PRUint32 *_count,
                            PRUnichar ***_usages)
{
  nsresult rv;
  PRUnichar *tmpUsages[13];
  char *suffix = "";
  PRUint32 tmpCount;
  nsUsageArrayHelper uah(mCert);
  rv = uah.GetUsageArray(suffix, 13, _verified, &tmpCount, tmpUsages);
  if (tmpCount > 0) {
    *_usages = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * tmpCount);
    for (PRUint32 i=0; i<tmpCount; i++) {
      (*_usages)[i] = tmpUsages[i];
    }
    *_count = tmpCount;
    return NS_OK;
  }
  *_usages = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *));
  *_count = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetPurposes(PRUint32   *_verified,
                              nsAString &_purposes)
{
  nsresult rv;
  PRUnichar *tmpUsages[13];
  char *suffix = "_p";
  PRUint32 tmpCount;
  nsUsageArrayHelper uah(mCert);
  rv = uah.GetUsageArray(suffix, 13, _verified, &tmpCount, tmpUsages);
  _purposes.Truncate();
  for (PRUint32 i=0; i<tmpCount; i++) {
    if (i>0) _purposes.Append(NS_LITERAL_STRING(","));
    _purposes.Append(tmpUsages[i]);
    nsMemory::Free(tmpUsages[i]);
  }
  return NS_OK;
}

static nsresult
ProcessSECAlgorithmID(SECAlgorithmID *algID,
                      nsINSSComponent *nssComponent,
                      nsIASN1Sequence **retSequence)
{
  nsCOMPtr<nsIASN1Sequence> sequence = new nsNSSASN1Sequence();
  if (sequence == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  *retSequence = nsnull;
  nsString text;
  GetOIDText(&algID->algorithm, nssComponent, text);
  if (!algID->parameters.len || algID->parameters.data[0] == nsIASN1Object::ASN1_NULL) {
    sequence->SetDisplayValue(text);
    sequence->SetProcessObjects(PR_FALSE);
  } else {
    nsCOMPtr<nsIASN1PrintableItem> printableItem = new nsNSSASN1PrintableItem();
    printableItem->SetDisplayValue(text);
    nsCOMPtr<nsIMutableArray> asn1Objects;
    sequence->GetASN1Objects(getter_AddRefs(asn1Objects));
    asn1Objects->AppendElement(printableItem, PR_FALSE);
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAlgID").get(),
                                        text);
    printableItem->SetDisplayName(text);
    printableItem = new nsNSSASN1PrintableItem();
    asn1Objects->AppendElement(printableItem, PR_FALSE);
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpParams").get(),
                                        text);
    printableItem->SetDisplayName(text); 
    ProcessRawBytes(&algID->parameters,text);
    printableItem->SetDisplayValue(text);
  }
  *retSequence = sequence;
  NS_ADDREF(*retSequence);
  return NS_OK;
}

static nsresult
ProcessTime(PRTime dispTime, const PRUnichar *displayName, 
            nsIASN1Sequence *parentSequence)
{
  nsresult rv;
  nsCOMPtr<nsIDateTimeFormat> dateFormatter =
     do_CreateInstance(kDateTimeFormatCID, &rv);
  if (NS_FAILED(rv)) 
    return rv;

  nsString text;
  nsString tempString;

  PRExplodedTime explodedTime;
  PR_ExplodeTime(dispTime, PR_LocalTimeParameters, &explodedTime);

  dateFormatter->FormatPRExplodedTime(nsnull, kDateFormatShort, kTimeFormatSecondsForce24Hour,
                              &explodedTime, tempString);

  text.Append(tempString);
  text.Append(NS_LITERAL_STRING("\n("));

  PRExplodedTime explodedTimeGMT;
  PR_ExplodeTime(dispTime, PR_GMTParameters, &explodedTimeGMT);

  dateFormatter->FormatPRExplodedTime(nsnull, kDateFormatShort, kTimeFormatSecondsForce24Hour,
                              &explodedTimeGMT, tempString);

  text.Append(tempString);
  text.Append(NS_LITERAL_STRING(" GMT)"));

  nsCOMPtr<nsIASN1PrintableItem> printableItem = new nsNSSASN1PrintableItem();
  if (printableItem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  printableItem->SetDisplayValue(text);
  printableItem->SetDisplayName(nsDependentString(displayName));
  nsCOMPtr<nsIMutableArray> asn1Objects;
  parentSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  asn1Objects->AppendElement(printableItem, PR_FALSE);
  return NS_OK;
}

static nsresult
ProcessSubjectPublicKeyInfo(CERTSubjectPublicKeyInfo *spki, 
                            nsIASN1Sequence *parentSequence,
                            nsINSSComponent *nssComponent)
{
  nsCOMPtr<nsIASN1Sequence> spkiSequence = new nsNSSASN1Sequence();

  if (spkiSequence == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  nsString text;
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSPKI").get(),
                                      text);
  spkiSequence->SetDisplayName(text);

  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSPKIAlg").get(),
                                      text);
  nsCOMPtr<nsIASN1Sequence> sequenceItem;
  nsresult rv = ProcessSECAlgorithmID(&spki->algorithm, nssComponent,
                                      getter_AddRefs(sequenceItem));
  if (NS_FAILED(rv))
    return rv;
  sequenceItem->SetDisplayName(text);
  nsCOMPtr<nsIMutableArray> asn1Objects;
  spkiSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  asn1Objects->AppendElement(sequenceItem, PR_FALSE);

  // The subjectPublicKey field is encoded as a bit string.
  // ProcessRawBytes expects the lenght to be in bytes, so 
  // let's convert the lenght into a temporary SECItem.
  SECItem data;
  data.data = spki->subjectPublicKey.data;
  data.len  = spki->subjectPublicKey.len / 8;
  text.Truncate();
  ProcessRawBytes(&data, text);
  nsCOMPtr<nsIASN1PrintableItem> printableItem = new nsNSSASN1PrintableItem();
  if (printableItem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  printableItem->SetDisplayValue(text);
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSubjPubKey").get(),
                                      text);
  printableItem->SetDisplayName(text);
  asn1Objects->AppendElement(printableItem, PR_FALSE);
  
  parentSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  asn1Objects->AppendElement(spkiSequence, PR_FALSE);
  return NS_OK;
}

static nsresult
ProcessExtensions(CERTCertExtension **extensions, 
                  nsIASN1Sequence *parentSequence, 
                  nsINSSComponent *nssComponent)
{
  nsCOMPtr<nsIASN1Sequence> extensionSequence = new nsNSSASN1Sequence;
  if (extensionSequence == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  nsString text;
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpExtensions").get(),
                                      text);
  extensionSequence->SetDisplayName(text);
  PRInt32 i;
  nsresult rv;
  nsCOMPtr<nsIASN1PrintableItem> newExtension;
  nsCOMPtr<nsIMutableArray> asn1Objects;
  extensionSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  for (i=0; extensions[i] != nsnull; i++) {
    rv = ProcessSingleExtension(extensions[i], nssComponent,
                                getter_AddRefs(newExtension));
    if (NS_FAILED(rv))
      return rv;

    asn1Objects->AppendElement(newExtension, PR_FALSE);
  }
  parentSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  asn1Objects->AppendElement(extensionSequence, PR_FALSE);
  return NS_OK;
}

static nsresult
ProcessName(CERTName *name, nsINSSComponent *nssComponent, PRUnichar **value)
{
  CERTRDN** rdns;
  CERTRDN** rdn;
  CERTAVA** avas;
  CERTAVA* ava;
  SECItem *decodeItem = nsnull;
  nsString finalString;

  rdns = name->rdns;

  nsString type;
  nsresult rv;
  const PRUnichar *params[2];
  nsString avavalue;
  nsXPIDLString temp;
  CERTRDN **lastRdn;
  lastRdn = rdns;


  /* find last RDN */
  lastRdn = rdns;
  while (*lastRdn) lastRdn++;
  // The above whille loop will put us at the last member
  // of the array which is a NULL pointer.  So let's back
  // up one spot so that we have the last non-NULL entry in 
  // the array in preparation for traversing the 
  // RDN's (Relative Distinguished Name) in reverse oder.
  lastRdn--;
   
  /*
   * Loop over name contents in _reverse_ RDN order appending to string
   * When building the Ascii string, NSS loops over these entries in 
   * reverse order, so I will as well.  The difference is that NSS
   * will always place them in a one line string separated by commas,
   * where I want each entry on a single line.  I can't just use a comma
   * as my delimitter because it is a valid character to have in the 
   * value portion of the AVA and could cause trouble when parsing.
   */
  for (rdn = lastRdn; rdn >= rdns; rdn--) {
    avas = (*rdn)->avas;
    while ((ava = *avas++) != 0) {
      rv = GetOIDText(&ava->type, nssComponent, type);
      if (NS_FAILED(rv))
        return rv;

      //This function returns a string in UTF8 format.
      decodeItem = CERT_DecodeAVAValue(&ava->value);
      if(!decodeItem) {
         return NS_ERROR_FAILURE;
      }
      avavalue.AssignWithConversion((char*)decodeItem->data, decodeItem->len);

      SECITEM_FreeItem(decodeItem, PR_TRUE);
      params[0] = type.get();
      params[1] = avavalue.get();
      nssComponent->PIPBundleFormatStringFromName(NS_LITERAL_STRING("AVATemplate").get(),
                                                  params, 2, 
                                                  getter_Copies(temp));
      finalString += temp + NS_LITERAL_STRING("\n");
    }
  }
  *value = ToNewUnicode(finalString);    
  return NS_OK;
}

nsresult
nsNSSCertificate::CreateTBSCertificateASN1Struct(nsIASN1Sequence **retSequence,
                                                 nsINSSComponent *nssComponent)
{
  //
  //   TBSCertificate  ::=  SEQUENCE  {
  //        version         [0]  EXPLICIT Version DEFAULT v1,
  //        serialNumber         CertificateSerialNumber,
  //        signature            AlgorithmIdentifier,
  //        issuer               Name,
  //        validity             Validity,
  //        subject              Name,
  //        subjectPublicKeyInfo SubjectPublicKeyInfo,
  //        issuerUniqueID  [1]  IMPLICIT UniqueIdentifier OPTIONAL,
  //                             -- If present, version shall be v2 or v3
  //        subjectUniqueID [2]  IMPLICIT UniqueIdentifier OPTIONAL,
  //                             -- If present, version shall be v2 or v3
  //        extensions      [3]  EXPLICIT Extensions OPTIONAL
  //                            -- If present, version shall be v3
  //        }
  //
  // This is the ASN1 structure we should be dealing with at this point.
  // The code in this method will assert this is the structure we're dealing
  // and then add more user friendly text for that field.
  nsCOMPtr<nsIASN1Sequence> sequence = new nsNSSASN1Sequence();
  if (sequence == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  nsString text;
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpCertificate").get(),
                                      text);
  sequence->SetDisplayName(text);
  nsCOMPtr<nsIASN1PrintableItem> printableItem;
  
  nsCOMPtr<nsIMutableArray> asn1Objects;
  sequence->GetASN1Objects(getter_AddRefs(asn1Objects));

  nsresult rv = ProcessVersion(&mCert->version, nssComponent,
                               getter_AddRefs(printableItem));
  if (NS_FAILED(rv))
    return rv;

  asn1Objects->AppendElement(printableItem, PR_FALSE);
  
  rv = ProcessSerialNumberDER(&mCert->serialNumber, nssComponent,
                              getter_AddRefs(printableItem));

  if (NS_FAILED(rv))
    return rv;
  asn1Objects->AppendElement(printableItem, PR_FALSE);

  nsCOMPtr<nsIASN1Sequence> algID;
  rv = ProcessSECAlgorithmID(&mCert->signature,
                             nssComponent, getter_AddRefs(algID));
  if (NS_FAILED(rv))
    return rv;

  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSigAlg").get(),
                                      text);
  algID->SetDisplayName(text);
  asn1Objects->AppendElement(algID, PR_FALSE);

  nsXPIDLString value;
  ProcessName(&mCert->issuer, nssComponent, getter_Copies(value));

  printableItem = new nsNSSASN1PrintableItem();
  if (printableItem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  printableItem->SetDisplayValue(value);
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpIssuer").get(),
                                      text);
  printableItem->SetDisplayName(text);
  asn1Objects->AppendElement(printableItem, PR_FALSE);
  
  nsCOMPtr<nsIASN1Sequence> validitySequence = new nsNSSASN1Sequence();
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpValidity").get(),
                                      text);
  validitySequence->SetDisplayName(text);
  asn1Objects->AppendElement(validitySequence, PR_FALSE);
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpNotBefore").get(),
                                      text);
  nsCOMPtr<nsIX509CertValidity> validityData;
  GetValidity(getter_AddRefs(validityData));
  PRTime notBefore, notAfter;

  validityData->GetNotBefore(&notBefore);
  validityData->GetNotAfter(&notAfter);
  validityData = 0;
  rv = ProcessTime(notBefore, text.get(), validitySequence);
  if (NS_FAILED(rv))
    return rv;

  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpNotAfter").get(),
                                      text);
  rv = ProcessTime(notAfter, text.get(), validitySequence);
  if (NS_FAILED(rv))
    return rv;

  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSubject").get(),
                                      text);

  printableItem = new nsNSSASN1PrintableItem();
  if (printableItem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  printableItem->SetDisplayName(text);
  ProcessName(&mCert->subject, nssComponent,getter_Copies(value));
  printableItem->SetDisplayValue(value);
  asn1Objects->AppendElement(printableItem, PR_FALSE);

  rv = ProcessSubjectPublicKeyInfo(&mCert->subjectPublicKeyInfo, sequence,
                                   nssComponent); 
  if (NS_FAILED(rv))
    return rv;
 
  SECItem data; 
  // Is there an issuerUniqueID?
  if (mCert->issuerID.data != nsnull) {
    // The issuerID is encoded as a bit string.
    // The function ProcessRawBytes expects the
    // length to be in bytes, so let's convert the
    // length in a temporary SECItem
    data.data = mCert->issuerID.data;
    data.len  = mCert->issuerID.len / 8;

    ProcessRawBytes(&data, text);
    printableItem = new nsNSSASN1PrintableItem();
    if (printableItem == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;

    printableItem->SetDisplayValue(text);
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpIssuerUniqueID").get(),
                                         text);
    printableItem->SetDisplayName(text);
    asn1Objects->AppendElement(printableItem, PR_FALSE);
  }

  if (mCert->subjectID.data) {
    // The subjectID is encoded as a bit string.
    // The function ProcessRawBytes expects the
    // length to be in bytes, so let's convert the
    // length in a temporary SECItem
    data.data = mCert->issuerID.data;
    data.len  = mCert->issuerID.len / 8;

    ProcessRawBytes(&data, text);
    printableItem = new nsNSSASN1PrintableItem();
    if (printableItem == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;

    printableItem->SetDisplayValue(text);
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSubjectUniqueID").get(),
                                         text);
    printableItem->SetDisplayName(text);
    asn1Objects->AppendElement(printableItem, PR_FALSE);

  }
  if (mCert->extensions) {
    rv = ProcessExtensions(mCert->extensions, sequence, nssComponent);
    if (NS_FAILED(rv))
      return rv;
  }
  *retSequence = sequence;
  NS_ADDREF(*retSequence);  
  return NS_OK;
}

#ifdef DEBUG_javi
void
DumpASN1Object(nsIASN1Object *object, unsigned int level)
{
  nsAutoString dispNameU, dispValU;
  unsigned int i;
  nsCOMPtr<nsIMutableArray> asn1Objects;
  nsCOMPtr<nsISupports> isupports;
  nsCOMPtr<nsIASN1Object> currObject;
  PRBool processObjects;
  PRUint32 numObjects;

  for (i=0; i<level; i++)
    printf ("  ");

  object->GetDisplayName(dispNameU);
  nsCOMPtr<nsIASN1Sequence> sequence(do_QueryInterface(object));
  if (sequence) {
    printf ("%s ", NS_ConvertUCS2toUTF8(dispNameU).get());
    sequence->GetProcessObjects(&processObjects);
    if (processObjects) {
      printf("\n");
      sequence->GetASN1Objects(getter_AddRefs(asn1Objects));
      asn1Objects->Count(&numObjects);
      for (i=0; i<numObjects;i++) {
        isupports = dont_AddRef(asn1Objects->ElementAt(i));
        currObject = do_QueryInterface(isupports);
        DumpASN1Object(currObject, level+1);    
      }
    } else { 
      object->GetDisplayValue(dispValU);
      printf("= %s\n", NS_ConvertUCS2toUTF8(dispValU).get()); 
    }
  } else { 
    object->GetDisplayValue(dispValU);
    printf("%s = %s\n",NS_ConvertUCS2toUTF8(dispNameU).get(), 
                       NS_ConvertUCS2toUTF8(dispValU).get()); 
  }
}
#endif

nsresult
nsNSSCertificate::CreateASN1Struct()
{
  nsCOMPtr<nsIASN1Sequence> sequence = new nsNSSASN1Sequence();

  mASN1Structure = sequence; 
  if (mASN1Structure == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIMutableArray> asn1Objects;
  sequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  nsXPIDLCString title;
  GetWindowTitle(getter_Copies(title));
  
  mASN1Structure->SetDisplayName(NS_ConvertASCIItoUCS2(title));
  // This sequence will be contain the tbsCertificate, signatureAlgorithm,
  // and signatureValue.
  nsresult rv;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  rv = CreateTBSCertificateASN1Struct(getter_AddRefs(sequence),
                                      nssComponent);
  if (NS_FAILED(rv))
    return rv;

  asn1Objects->AppendElement(sequence, PR_FALSE);
  nsCOMPtr<nsIASN1Sequence> algID;

  rv = ProcessSECAlgorithmID(&mCert->signatureWrap.signatureAlgorithm, 
                             nssComponent, getter_AddRefs(algID));
  if (NS_FAILED(rv))
    return rv;
  nsString text;
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSigAlg").get(),
                                      text);
  algID->SetDisplayName(text);
  asn1Objects->AppendElement(algID, PR_FALSE);
  nsCOMPtr<nsIASN1PrintableItem>printableItem = new nsNSSASN1PrintableItem();
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpCertSig").get(),
                                      text);
  printableItem->SetDisplayName(text);
  // The signatureWrap is encoded as a bit string.
  // The function ProcessRawBytes expects the
  // length to be in bytes, so let's convert the
  // length in a temporary SECItem
  SECItem temp;
  temp.data = mCert->signatureWrap.signature.data;
  temp.len  = mCert->signatureWrap.signature.len / 8;
  text.Truncate();
  ProcessRawBytes(&temp,text);
  printableItem->SetDisplayValue(text);
  asn1Objects->AppendElement(printableItem, PR_FALSE);
  return NS_OK;
}

/* readonly attribute nsIASN1Object ASN1Structure; */
NS_IMETHODIMP 
nsNSSCertificate::GetASN1Structure(nsIASN1Object * *aASN1Structure)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(aASN1Structure);
  if (mASN1Structure == nsnull) {
    // First create the recursive structure os ASN1Objects
    // which tells us the layout of the cert.
    rv = CreateASN1Struct();
    if (NS_FAILED(rv)) {
      return rv;
    }
#ifdef DEBUG_javi
    DumpASN1Object(mASN1Structure, 0);
#endif
  }
  *aASN1Structure = mASN1Structure;
  NS_IF_ADDREF(*aASN1Structure);
  return rv;
}

NS_IMETHODIMP
nsNSSCertificate::IsSameCert(nsIX509Cert *other, PRBool *result)
{
  NS_ENSURE_ARG(other);
  NS_ENSURE_ARG(result);

  nsNSSCertificate *other2 = NS_STATIC_CAST(nsNSSCertificate*, other);
  if (!other2)
    return NS_ERROR_FAILURE;
  
  *result = (mCert == other2->mCert);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::SaveSMimeProfile()
{
  if (SECSuccess != CERT_SaveSMimeProfile(mCert, nsnull, nsnull))
    return NS_ERROR_FAILURE;
  else
    return NS_OK;
}


char* nsNSSCertificate::defaultServerNickname(CERTCertificate* cert)
{
  char* nickname = nsnull;
  int count;
  PRBool conflict;
  char* servername = nsnull;
  
  servername = CERT_GetCommonName(&cert->subject);
  if (servername == NULL) {
    return nsnull;
  }
   
  count = 1;
  while (1) {
    if (count == 1) {
      nickname = PR_smprintf("%s", servername);
    }
    else {
      nickname = PR_smprintf("%s #%d", servername, count);
    }
    if (nickname == NULL) {
      break;
    }

    conflict = SEC_CertNicknameConflict(nickname, &cert->derSubject,
                                        cert->dbhandle);
    if (conflict == PR_SUCCESS) {
      break;
    }
    PR_Free(nickname);
    count++;
  }
  PR_FREEIF(servername);
  return nickname;
}

