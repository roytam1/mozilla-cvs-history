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
 * $Id$
 */

#include "prmem.h"
#include "prerror.h"

#include "nsNSSComponent.h" // for PIPNSS string bundle calls.
#include "nsCOMPtr.h"
#include "nsISupportsArray.h"
#include "nsILocalFile.h"
#include "nsNSSCertificate.h"
#include "nsPKCS12Blob.h"
#include "nsIX509Cert.h"
#include "nsINSSDialogs.h"
#include "nsString.h"
#include "nsILocaleService.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"

#include "pk11func.h"
#include "certdb.h"
#include "cert.h"
#include "secerr.h"
#include "nssb64.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);
static NS_DEFINE_CID(kDateTimeFormatCID, NS_DATETIMEFORMAT_CID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID);

/*
 * nsNSSCertTrust
 * 
 * Class for maintaining trust flags for an NSS certificate.
 */
class nsNSSCertTrust
{
public:
  nsNSSCertTrust();
  nsNSSCertTrust(unsigned int ssl, unsigned int email, unsigned int objsign);
  nsNSSCertTrust(CERTCertTrust *t);
  virtual ~nsNSSCertTrust();

  /* query */
  PRBool HasCA(PRBool checkSSL = PR_TRUE, 
               PRBool checkEmail = PR_TRUE,  
               PRBool checkObjSign = PR_TRUE);
  PRBool HasPeer(PRBool checkSSL = PR_TRUE, 
                 PRBool checkEmail = PR_TRUE,  
                 PRBool checkObjSign = PR_TRUE);
  PRBool HasUser(PRBool checkSSL = PR_TRUE, 
                 PRBool checkEmail = PR_TRUE,  
                 PRBool checkObjSign = PR_TRUE);
  PRBool HasTrustedCA(PRBool checkSSL = PR_TRUE, 
                      PRBool checkEmail = PR_TRUE,  
                      PRBool checkObjSign = PR_TRUE);

  /* common defaults */
  /* equivalent to "c,c,c" */
  void SetValidCA();
  /* equivalent to "C,C,C" */
  void SetTrustedServerCA();
  /* equivalent to "CT,CT,CT" */
  void SetTrustedCA();
  /* equivalent to "p,p,p" */
  void SetValidPeer();
  /* equivalent to "P,P,P" */
  void SetTrustedPeer();
  /* equivalent to "u,u,u" */
  void SetUser();

  /* general setters */
  /* read: "p, P, c, C, T, u, w" */
  void SetSSLTrust(PRBool peer, PRBool tPeer,
                   PRBool ca,   PRBool tCA, PRBool tClientCA,
                   PRBool user, PRBool warn); 

  void SetEmailTrust(PRBool peer, PRBool tPeer,
                     PRBool ca,   PRBool tCA, PRBool tClientCA,
                     PRBool user, PRBool warn);

  void SetObjSignTrust(PRBool peer, PRBool tPeer,
                       PRBool ca,   PRBool tCA, PRBool tClientCA,
                       PRBool user, PRBool warn);

  /* set c <--> CT */
  void AddCATrust(PRBool ssl, PRBool email, PRBool objSign);
  /* set p <--> P */
  void AddPeerTrust(PRBool ssl, PRBool email, PRBool objSign);

  /* get it (const?) (shallow?) */
  CERTCertTrust * GetTrust() { return &mTrust; }

private:
  void addTrust(unsigned int *t, unsigned int v);
  void removeTrust(unsigned int *t, unsigned int v);
  PRBool hasTrust(unsigned int t, unsigned int v);
  CERTCertTrust mTrust;
};

void
nsNSSCertTrust::AddCATrust(PRBool ssl, PRBool email, PRBool objSign)
{
  if (ssl) {
    addTrust(&mTrust.sslFlags, CERTDB_TRUSTED_CA);
    addTrust(&mTrust.sslFlags, CERTDB_TRUSTED_CLIENT_CA);
  }
  if (email) {
    addTrust(&mTrust.emailFlags, CERTDB_TRUSTED_CA);
    addTrust(&mTrust.emailFlags, CERTDB_TRUSTED_CLIENT_CA);
  }
  if (objSign) {
    addTrust(&mTrust.objectSigningFlags, CERTDB_TRUSTED_CA);
    addTrust(&mTrust.objectSigningFlags, CERTDB_TRUSTED_CLIENT_CA);
  }
}

void
nsNSSCertTrust::AddPeerTrust(PRBool ssl, PRBool email, PRBool objSign)
{
  if (ssl)
    addTrust(&mTrust.sslFlags, CERTDB_TRUSTED);
  if (email)
    addTrust(&mTrust.emailFlags, CERTDB_TRUSTED);
  if (objSign)
    addTrust(&mTrust.objectSigningFlags, CERTDB_TRUSTED);
}

nsNSSCertTrust::nsNSSCertTrust()
{
  memset(&mTrust, 0, sizeof(CERTCertTrust));
}

nsNSSCertTrust::nsNSSCertTrust(unsigned int ssl, 
                               unsigned int email, 
                               unsigned int objsign)
{
  addTrust(&mTrust.sslFlags, ssl);
  addTrust(&mTrust.emailFlags, email);
  addTrust(&mTrust.objectSigningFlags, objsign);
}

nsNSSCertTrust::nsNSSCertTrust(CERTCertTrust *t)
{
  memcpy(&mTrust, t, sizeof(CERTCertTrust));
}

nsNSSCertTrust::~nsNSSCertTrust()
{
}

void
nsNSSCertTrust::SetSSLTrust(PRBool peer, PRBool tPeer,
                            PRBool ca,   PRBool tCA, PRBool tClientCA,
                            PRBool user, PRBool warn)
{
  mTrust.sslFlags = 0;
  if (peer || tPeer)
    addTrust(&mTrust.sslFlags, CERTDB_VALID_PEER);
  if (tPeer)
    addTrust(&mTrust.sslFlags, CERTDB_TRUSTED);
  if (ca || tCA)
    addTrust(&mTrust.sslFlags, CERTDB_VALID_CA);
  if (tClientCA)
    addTrust(&mTrust.sslFlags, CERTDB_TRUSTED_CLIENT_CA);
  if (tCA)
    addTrust(&mTrust.sslFlags, CERTDB_TRUSTED_CA);
  if (user)
    addTrust(&mTrust.sslFlags, CERTDB_USER);
  if (warn)
    addTrust(&mTrust.sslFlags, CERTDB_SEND_WARN);
}

void
nsNSSCertTrust::SetEmailTrust(PRBool peer, PRBool tPeer,
                              PRBool ca,   PRBool tCA, PRBool tClientCA,
                              PRBool user, PRBool warn)
{
  mTrust.emailFlags = 0;
  if (peer || tPeer)
    addTrust(&mTrust.emailFlags, CERTDB_VALID_PEER);
  if (tPeer)
    addTrust(&mTrust.emailFlags, CERTDB_TRUSTED);
  if (ca || tCA)
    addTrust(&mTrust.emailFlags, CERTDB_VALID_CA);
  if (tClientCA)
    addTrust(&mTrust.emailFlags, CERTDB_TRUSTED_CLIENT_CA);
  if (tCA)
    addTrust(&mTrust.emailFlags, CERTDB_TRUSTED_CA);
  if (user)
    addTrust(&mTrust.emailFlags, CERTDB_USER);
  if (warn)
    addTrust(&mTrust.emailFlags, CERTDB_SEND_WARN);
}

void
nsNSSCertTrust::SetObjSignTrust(PRBool peer, PRBool tPeer,
                                PRBool ca,   PRBool tCA, PRBool tClientCA,
                                PRBool user, PRBool warn)
{
  mTrust.objectSigningFlags = 0;
  if (peer || tPeer)
    addTrust(&mTrust.objectSigningFlags, CERTDB_VALID_PEER);
  if (tPeer)
    addTrust(&mTrust.objectSigningFlags, CERTDB_TRUSTED);
  if (ca || tCA)
    addTrust(&mTrust.objectSigningFlags, CERTDB_VALID_CA);
  if (tClientCA)
    addTrust(&mTrust.objectSigningFlags, CERTDB_TRUSTED_CLIENT_CA);
  if (tCA)
    addTrust(&mTrust.objectSigningFlags, CERTDB_TRUSTED_CA);
  if (user)
    addTrust(&mTrust.objectSigningFlags, CERTDB_USER);
  if (warn)
    addTrust(&mTrust.objectSigningFlags, CERTDB_SEND_WARN);
}

void
nsNSSCertTrust::SetValidCA()
{
  SetSSLTrust(PR_FALSE, PR_FALSE,
              PR_TRUE, PR_FALSE, PR_FALSE,
              PR_FALSE, PR_FALSE);
  SetEmailTrust(PR_FALSE, PR_FALSE,
                PR_TRUE, PR_FALSE, PR_FALSE,
                PR_FALSE, PR_FALSE);
  SetObjSignTrust(PR_FALSE, PR_FALSE,
                  PR_TRUE, PR_FALSE, PR_FALSE,
                  PR_FALSE, PR_FALSE);
}

void
nsNSSCertTrust::SetTrustedServerCA()
{
  SetSSLTrust(PR_FALSE, PR_FALSE,
              PR_TRUE, PR_TRUE, PR_FALSE,
              PR_FALSE, PR_FALSE);
  SetEmailTrust(PR_FALSE, PR_FALSE,
                PR_TRUE, PR_TRUE, PR_FALSE,
                PR_FALSE, PR_FALSE);
  SetObjSignTrust(PR_FALSE, PR_FALSE,
                  PR_TRUE, PR_TRUE, PR_FALSE,
                  PR_FALSE, PR_FALSE);
}

void
nsNSSCertTrust::SetTrustedCA()
{
  SetSSLTrust(PR_FALSE, PR_FALSE,
              PR_TRUE, PR_TRUE, PR_TRUE,
              PR_FALSE, PR_FALSE);
  SetEmailTrust(PR_FALSE, PR_FALSE,
                PR_TRUE, PR_TRUE, PR_TRUE,
                PR_FALSE, PR_FALSE);
  SetObjSignTrust(PR_FALSE, PR_FALSE,
                  PR_TRUE, PR_TRUE, PR_TRUE,
                  PR_FALSE, PR_FALSE);
}

void 
nsNSSCertTrust::SetValidPeer()
{
  SetSSLTrust(PR_TRUE, PR_FALSE,
              PR_FALSE, PR_FALSE, PR_FALSE,
              PR_FALSE, PR_FALSE);
  SetEmailTrust(PR_TRUE, PR_FALSE,
                PR_FALSE, PR_FALSE, PR_FALSE,
                PR_FALSE, PR_FALSE);
  SetObjSignTrust(PR_TRUE, PR_FALSE,
                  PR_FALSE, PR_FALSE, PR_FALSE,
                  PR_FALSE, PR_FALSE);
}

void 
nsNSSCertTrust::SetTrustedPeer()
{
  SetSSLTrust(PR_TRUE, PR_TRUE,
              PR_FALSE, PR_FALSE, PR_FALSE,
              PR_FALSE, PR_FALSE);
  SetEmailTrust(PR_TRUE, PR_TRUE,
                PR_FALSE, PR_FALSE, PR_FALSE,
                PR_FALSE, PR_FALSE);
  SetObjSignTrust(PR_TRUE, PR_TRUE,
                  PR_FALSE, PR_FALSE, PR_FALSE,
                  PR_FALSE, PR_FALSE);
}

void
nsNSSCertTrust::SetUser()
{
  SetSSLTrust(PR_FALSE, PR_FALSE,
              PR_FALSE, PR_FALSE, PR_FALSE,
              PR_TRUE, PR_FALSE);
  SetEmailTrust(PR_FALSE, PR_FALSE,
                PR_FALSE, PR_FALSE, PR_FALSE,
                PR_TRUE, PR_FALSE);
  SetObjSignTrust(PR_FALSE, PR_FALSE,
                  PR_FALSE, PR_FALSE, PR_FALSE,
                  PR_TRUE, PR_FALSE);
}

PRBool
nsNSSCertTrust::HasCA(PRBool checkSSL, 
                      PRBool checkEmail,  
                      PRBool checkObjSign)
{
  if (checkSSL && !hasTrust(mTrust.sslFlags, CERTDB_VALID_CA))
    return PR_FALSE;
  if (checkEmail && !hasTrust(mTrust.emailFlags, CERTDB_VALID_CA))
    return PR_FALSE;
  if (checkObjSign && !hasTrust(mTrust.objectSigningFlags, CERTDB_VALID_CA))
    return PR_FALSE;
  return PR_TRUE;
}

PRBool
nsNSSCertTrust::HasPeer(PRBool checkSSL, 
                        PRBool checkEmail,  
                        PRBool checkObjSign)
{
  if (checkSSL && !hasTrust(mTrust.sslFlags, CERTDB_VALID_PEER))
    return PR_FALSE;
  if (checkEmail && !hasTrust(mTrust.emailFlags, CERTDB_VALID_PEER))
    return PR_FALSE;
  if (checkObjSign && !hasTrust(mTrust.objectSigningFlags, CERTDB_VALID_PEER))
    return PR_FALSE;
  return PR_TRUE;
}
PRBool
nsNSSCertTrust::HasUser(PRBool checkSSL, 
                        PRBool checkEmail,  
                        PRBool checkObjSign)
{
  if (checkSSL && !hasTrust(mTrust.sslFlags, CERTDB_USER))
    return PR_FALSE;
  if (checkEmail && !hasTrust(mTrust.emailFlags, CERTDB_USER))
    return PR_FALSE;
  if (checkObjSign && !hasTrust(mTrust.objectSigningFlags, CERTDB_USER))
    return PR_FALSE;
  return PR_TRUE;
}

PRBool
nsNSSCertTrust::HasTrustedCA(PRBool checkSSL, 
                             PRBool checkEmail,  
                             PRBool checkObjSign)
{
  if (checkSSL && !(hasTrust(mTrust.sslFlags, CERTDB_TRUSTED_CA) ||
                    hasTrust(mTrust.sslFlags, CERTDB_TRUSTED_CLIENT_CA)))
    return PR_FALSE;
  if (checkEmail && !(hasTrust(mTrust.emailFlags, CERTDB_TRUSTED_CA) ||
                      hasTrust(mTrust.emailFlags, CERTDB_TRUSTED_CLIENT_CA)))
    return PR_FALSE;
  if (checkObjSign && 
       !(hasTrust(mTrust.objectSigningFlags, CERTDB_TRUSTED_CA) ||
         hasTrust(mTrust.objectSigningFlags, CERTDB_TRUSTED_CLIENT_CA)))
    return PR_FALSE;
  return PR_TRUE;
}

void
nsNSSCertTrust::addTrust(unsigned int *t, unsigned int v)
{
  *t |= v;
}

PRBool
nsNSSCertTrust::hasTrust(unsigned int t, unsigned int v)
{
  return (t & v);
}

/* Header file */
class nsX509CertValidity : public nsIX509CertValidity
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIX509CERTVALIDITY

  nsX509CertValidity();
  nsX509CertValidity(CERTCertificate *cert);
  virtual ~nsX509CertValidity();
  /* additional members */

private:
  PRTime mNotBefore, mNotAfter;
  PRBool mTimesInitialized;
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsX509CertValidity, nsIX509CertValidity)

nsX509CertValidity::nsX509CertValidity() : mTimesInitialized(PR_FALSE)
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsX509CertValidity::nsX509CertValidity(CERTCertificate *cert) : 
                                           mTimesInitialized(PR_FALSE)
{
  NS_INIT_ISUPPORTS();
  if (cert) {
    SECStatus rv = CERT_GetCertTimes(cert, &mNotBefore, &mNotAfter);
    if (rv == SECSuccess)
      mTimesInitialized = PR_TRUE;
  }
}

nsX509CertValidity::~nsX509CertValidity()
{
  /* destructor code */
}

/* readonly attribute PRTime notBefore; */
NS_IMETHODIMP nsX509CertValidity::GetNotBefore(PRTime *aNotBefore)
{
  NS_ENSURE_ARG(aNotBefore);

  nsresult rv = NS_ERROR_FAILURE;
  if (mTimesInitialized) {
    *aNotBefore = mNotBefore;
    rv = NS_OK;
  }
  return rv;
}

/* readonly attribute PRTime notAfter; */
NS_IMETHODIMP nsX509CertValidity::GetNotAfter(PRTime *aNotAfter)
{
  NS_ENSURE_ARG(aNotAfter);

  nsresult rv = NS_ERROR_FAILURE;
  if (mTimesInitialized) {
    *aNotAfter = mNotAfter;
    rv = NS_OK;
  }
  return rv;
}

/* nsNSSCertificate */

NS_IMPL_THREADSAFE_ISUPPORTS1(nsNSSCertificate, nsIX509Cert)

nsNSSCertificate::nsNSSCertificate(char *certDER, int derLen)
{
  NS_INIT_ISUPPORTS();

  mCert = CERT_DecodeCertFromPackage(certDER, derLen);
}

nsNSSCertificate::nsNSSCertificate(CERTCertificate *cert)
{
  NS_INIT_ISUPPORTS();

  if (cert) 
    mCert = CERT_DupCertificate(cert);
}

nsNSSCertificate::~nsNSSCertificate()
{
  if (mCert)
    CERT_DestroyCertificate(mCert);
}

/* readonly attribute string dbKey; */
NS_IMETHODIMP 
nsNSSCertificate::GetDbKey(char * *aDbKey)
{
  SECStatus srv;
  SECItem key;

  NS_ENSURE_ARG(aDbKey);
  srv = CERT_KeyFromIssuerAndSN(mCert->arena, &mCert->derIssuer,
                                &mCert->serialNumber, &key);
  if (srv != SECSuccess) {
    return NS_ERROR_FAILURE;
  }
  *aDbKey = NSSBase64_EncodeItem(nsnull, nsnull, 0, &key);
  return (*aDbKey) ? NS_OK : NS_ERROR_FAILURE;
}

/* readonly attribute string windowTitle; */
NS_IMETHODIMP 
nsNSSCertificate::GetWindowTitle(char * *aWindowTitle)
{
  if (mCert) {
    if (mCert->nickname) {
      *aWindowTitle = PL_strdup(mCert->nickname);
    } else {
      *aWindowTitle = CERT_GetCommonName(&mCert->subject);
    }
  } else {
    NS_ASSERTION(0,"Somehow got nsnull for mCertificate in nsNSSCertificate.");
    *aWindowTitle = nsnull;
  }
  return NS_OK;
}

/*  readonly attribute wstring nickname; */
NS_IMETHODIMP
nsNSSCertificate::GetNickname(PRUnichar **_nickname)
{
  const char *nickname = (mCert->nickname) ? mCert->nickname : "(no nickname)";
  nsAutoString nn = NS_ConvertASCIItoUCS2(nickname);
  *_nickname = nn.ToNewUnicode();
  return NS_OK;
}

/*  readonly attribute wstring emailAddress; */
NS_IMETHODIMP
nsNSSCertificate::GetEmailAddress(PRUnichar **_emailAddress)
{
  const char *email = (mCert->emailAddr) ? mCert->emailAddr : "(no email address)";
  nsAutoString em = NS_ConvertASCIItoUCS2(email);
  *_emailAddress = em.ToNewUnicode();
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetCommonName(PRUnichar **aCommonName)
{
  NS_ENSURE_ARG(aCommonName);
  *aCommonName = nsnull;
  if (mCert) {
    char *commonName = CERT_GetCommonName(&mCert->subject);
    if (commonName) {
      nsAutoString cn = NS_ConvertASCIItoUCS2(commonName);
      *aCommonName = cn.ToNewUnicode();
    } /*else {
      *aCommonName = NS_LITERAL_STRING("<not set>").get(), 
    }*/
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetOrganization(PRUnichar **aOrganization)
{
  NS_ENSURE_ARG(aOrganization);
  *aOrganization = nsnull;
  if (mCert) {
    char *organization = CERT_GetOrgName(&mCert->subject);
    if (organization) {
      nsAutoString org = NS_ConvertASCIItoUCS2(organization);
      *aOrganization = org.ToNewUnicode();
    } /*else {
      *aOrganization = NS_LITERAL_STRING("<not set>").get(), 
    }*/
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetOrganizationalUnit(PRUnichar **aOrganizationalUnit)
{
  NS_ENSURE_ARG(aOrganizationalUnit);
  *aOrganizationalUnit = nsnull;
  if (mCert) {
    char *orgunit = CERT_GetOrgUnitName(&mCert->subject);
    if (orgunit) {
      nsAutoString ou = NS_ConvertASCIItoUCS2(orgunit);
      *aOrganizationalUnit = ou.ToNewUnicode();
    } /*else {
      *aOrganizationalUnit = NS_LITERAL_STRING("<not set>").get(), 
    }*/
  }
  return NS_OK;
}

/* 
 * nsIEnumerator getChain(); 
 */
NS_IMETHODIMP
nsNSSCertificate::GetChain(nsIEnumerator **_rvChain)
{
  nsresult rv;
  CERTCertListNode *node;
  /* Get the cert chain from NSS */
  CERTCertList *nssChain;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Getting chain for \"%s\"\n", mCert->nickname));
  nssChain = CERT_GetCertChainFromCert(mCert, PR_Now(), certUsageSSLClient);
  if (!nssChain)
    return NS_ERROR_FAILURE;
  /* enumerate the chain for scripting purposes */
  nsCOMPtr<nsISupportsArray> array;
  rv = NS_NewISupportsArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) { 
    goto done; 
  }
  for (node = CERT_LIST_HEAD(nssChain);
       !CERT_LIST_END(node, nssChain);
       node = CERT_LIST_NEXT(node)) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("adding %s to chain\n", node->cert->nickname));
    nsCOMPtr<nsIX509Cert> cert = new nsNSSCertificate(node->cert);
    array->AppendElement(cert);
  }
  rv = array->Enumerate(_rvChain);
done:
  if (nssChain)
    CERT_DestroyCertList(nssChain);
  return rv;
}

/*  readonly attribute wstring subjectName; */
NS_IMETHODIMP
nsNSSCertificate::GetSubjectName(PRUnichar **_subjectName)
{
  if (mCert->subjectName) {
    nsAutoString sn = NS_ConvertASCIItoUCS2(mCert->subjectName);
    *_subjectName = sn.ToNewUnicode();
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/*  readonly attribute wstring issuerName; */
NS_IMETHODIMP
nsNSSCertificate::GetIssuerName(PRUnichar **_issuerName)
{
  if (mCert->issuerName) {
    nsAutoString in = NS_ConvertASCIItoUCS2(mCert->issuerName);
    *_issuerName = in.ToNewUnicode();
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/*  readonly attribute wstring serialNumber; */
NS_IMETHODIMP
nsNSSCertificate::GetSerialNumber(PRUnichar **_serialNumber)
{
  char *tmpstr = CERT_Hexify(&mCert->serialNumber, 1);
  if (tmpstr) {
    nsAutoString sn = NS_ConvertASCIItoUCS2(tmpstr);
    *_serialNumber = sn.ToNewUnicode();
    PR_Free(tmpstr);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/*  readonly attribute wstring rsaPubModulus; */
NS_IMETHODIMP
nsNSSCertificate::GetRsaPubModulus(PRUnichar **_rsaPubModulus)
{
  //char *tmpstr = CERT_Hexify(&mCert->serialNumber, 1);
  char *tmpstr = PL_strdup("not yet implemented");
  if (tmpstr) {
    nsAutoString rsap = NS_ConvertASCIItoUCS2(tmpstr);
    *_rsaPubModulus = rsap.ToNewUnicode();
    PR_Free(tmpstr);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/*  readonly attribute wstring sha1Fingerprint; */
NS_IMETHODIMP
nsNSSCertificate::GetSha1Fingerprint(PRUnichar **_sha1Fingerprint)
{
  unsigned char fingerprint[20];
  char *fpStr = NULL;
  SECItem fpItem;
  memset(fingerprint, 0, sizeof fingerprint);
  PK11_HashBuf(SEC_OID_SHA1, fingerprint, 
               mCert->derCert.data, mCert->derCert.len);
  fpItem.data = fingerprint;
  fpItem.len = SHA1_LENGTH;
  fpStr = CERT_Hexify(&fpItem, 1);
  if (fpStr) {
    nsAutoString sha1str = NS_ConvertASCIItoUCS2(fpStr);
    *_sha1Fingerprint = sha1str.ToNewUnicode();
    PR_Free(fpStr);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/*  readonly attribute wstring md5Fingerprint; */
NS_IMETHODIMP
nsNSSCertificate::GetMd5Fingerprint(PRUnichar **_md5Fingerprint)
{
  unsigned char fingerprint[20];
  char *fpStr = NULL;
  SECItem fpItem;
  memset(fingerprint, 0, sizeof fingerprint);
  PK11_HashBuf(SEC_OID_MD5, fingerprint, 
               mCert->derCert.data, mCert->derCert.len);
  fpItem.data = fingerprint;
  fpItem.len = MD5_LENGTH;
  fpStr = CERT_Hexify(&fpItem, 1);
  if (fpStr) {
    nsAutoString md5str = NS_ConvertASCIItoUCS2(fpStr);
    *_md5Fingerprint = md5str.ToNewUnicode();
    PR_Free(fpStr);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/* readonly attribute wstring issuedDate; */
NS_IMETHODIMP
nsNSSCertificate::GetIssuedDate(PRUnichar **_issuedDate)
{
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
  *_issuedDate = date.ToNewUnicode();
  return NS_OK;
}

/* readonly attribute wstring expiresDate; */
NS_IMETHODIMP
nsNSSCertificate::GetExpiresDate(PRUnichar **_expiresDate)
{
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
  *_expiresDate = date.ToNewUnicode();
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificate::GetTokenName(PRUnichar **aTokenName)
{
  NS_ENSURE_ARG(aTokenName);
  *aTokenName = nsnull;
  if (mCert) {
    if (mCert->slot) {
      char *token = PK11_GetTokenName(mCert->slot);
      if (token) {
        nsAutoString tok = NS_ConvertASCIItoUCS2(token);
        *aTokenName = tok.ToNewUnicode();
      }
    } else {
      nsresult rv;
      nsAutoString tok;
      nsCOMPtr<nsINSSComponent> nssComponent(
		                        do_GetService(kNSSComponentCID, &rv));
      if (NS_FAILED(rv)) return rv;
      rv = nssComponent->GetPIPNSSBundleString(
                                NS_LITERAL_STRING("InternalToken").get(), tok);
      if (!NS_FAILED(rv))
        *aTokenName = tok.ToNewUnicode();
    }
  }
  return NS_OK;
}

/* [noscript] long getRawDER (out charPtr result) */
NS_IMETHODIMP
nsNSSCertificate::GetRawDER(char **result, PRUint32 *_retval)
{
  if (mCert) {
    *result = (char *)mCert->derCert.data;
    *_retval = mCert->derCert.len;
    return NS_OK;
  }
  *_retval = 0;
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

PRBool
nsNSSCertificate::verifyFailed(PRUint32 *_verified)
{
  int err = PR_GetError();
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("not verified because: %d\n", err));
  switch (err) {
  /* For these cases, verify only failed for the particular usage */
  case SEC_ERROR_INADEQUATE_KEY_USAGE:
  case SEC_ERROR_INADEQUATE_CERT_TYPE:
    return PR_FALSE;
  /* These are the cases that have individual error messages */
  case SEC_ERROR_REVOKED_CERTIFICATE:
    *_verified = nsNSSCertificate::CERT_REVOKED; break;
  case SEC_ERROR_EXPIRED_CERTIFICATE:
    *_verified = nsNSSCertificate::CERT_EXPIRED; break;
  case SEC_ERROR_UNTRUSTED_CERT:
    *_verified = nsNSSCertificate::CERT_NOT_TRUSTED; break;
  case SEC_ERROR_UNTRUSTED_ISSUER:
    *_verified = nsNSSCertificate::ISSUER_NOT_TRUSTED; break;
  case SEC_ERROR_UNKNOWN_ISSUER:
    *_verified = nsNSSCertificate::ISSUER_UNKNOWN; break;
  case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
    // XXX are there other error for this?
    *_verified = nsNSSCertificate::INVALID_CA; break;
  case SEC_ERROR_CERT_USAGES_INVALID: // XXX what is this?
  // there are some OCSP errors from PSM 1.x to add here
  default:
    *_verified = nsNSSCertificate::NOT_VERIFIED_UNKNOWN; break;
  }
  return PR_TRUE;
}

nsresult
nsNSSCertificate::GetUsageArray(char     *suffix,
                                PRUint32 *_verified,
                                PRUint32 *_count,
                                PRUnichar **tmpUsages)
{
  nsresult rv;
  int tmpCount = 0;
  CERTCertDBHandle *defaultcertdb = CERT_GetDefaultCertDB();
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv)) return rv; 
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageSSLClient, NULL) == SECSuccess) {
    // add client to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifySSLClient").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageSSLServer, NULL) == SECSuccess) {
    // add server to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifySSLServer").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageSSLServerWithStepUp, NULL) == SECSuccess) {
    // add stepup to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifySSLStepUp").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageEmailSigner, NULL) == SECSuccess) {
    // add signer to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifyEmailSigner").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageEmailRecipient, NULL) == SECSuccess) {
    // add recipient to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifyEmailRecip").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageObjectSigner, NULL) == SECSuccess) {
    // add objsigner to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifyObjSign").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
#if 0
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageProtectedObjectSigner, NULL) == SECSuccess) {
    // add protected objsigner to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifyProtectObjSign").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageUserCertImport, NULL) == SECSuccess) {
    // add user import to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifyUserImport").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
#endif
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageSSLCA, NULL) == SECSuccess) {
    // add SSL CA to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifySSLCA").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
#if 0
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageVerifyCA, NULL) == SECSuccess) {
    // add verify CA to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifyCAVerifier").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
#endif
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageStatusResponder, NULL) == SECSuccess) {
    // add status responder to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifyStatusResponder").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
#if 0
  if (CERT_VerifyCertNow(defaultcertdb, mCert, PR_TRUE, 
                         certUsageAnyCA, NULL) == SECSuccess) {
    // add any CA to usage
    nsAutoString verifyDesc;
    nsAutoString typestr(NS_LITERAL_STRING("VerifyAnyCA").get());
    typestr.AppendWithConversion(suffix);
    rv = nssComponent->GetPIPNSSBundleString(typestr.GetUnicode(), verifyDesc);
    tmpUsages[tmpCount++] = verifyDesc.ToNewUnicode();
  } else if (verifyFailed(_verified)) goto verify_failed;
#endif
  *_count = tmpCount;
  *_verified = nsNSSCertificate::VERIFIED_OK;
  return NS_OK;
verify_failed:
  *_count = 0;
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
  rv = GetUsageArray(suffix, _verified, &tmpCount, tmpUsages);
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

/* void getPurposes(out PRUint32 verified, out wstring purposes); */
NS_IMETHODIMP
nsNSSCertificate::GetPurposes(PRUint32   *_verified,
                              PRUnichar **_purposes)
{
  nsresult rv;
  PRUnichar *tmpUsages[13];
  char *suffix = "_p";
  PRUint32 tmpCount;
  rv = GetUsageArray(suffix, _verified, &tmpCount, tmpUsages);
  nsAutoString porpoises;
  for (PRUint32 i=0; i<tmpCount; i++) {
    if (i>0) porpoises.AppendWithConversion(",");
    porpoises.Append(tmpUsages[i]);
    nsMemory::Free(tmpUsages[i]);
  }
  if (_purposes != NULL) {  // skip it for verify-only
    *_purposes = porpoises.ToNewUnicode();
  }
  return NS_OK;
}

/* void view (); */
NS_IMETHODIMP 
nsNSSCertificate::View()
{
  nsresult rv;

  nsCOMPtr<nsICertificateDialogs> certDialogs;
  rv = ::getNSSDialogs(getter_AddRefs(certDialogs),
                       NS_GET_IID(nsICertificateDialogs));
  return certDialogs->ViewCert(this);
}

/* nsNSSCertificateDB */

NS_IMPL_ISUPPORTS1(nsNSSCertificateDB, nsIX509CertDB)

nsNSSCertificateDB::nsNSSCertificateDB()
{
  NS_INIT_ISUPPORTS();
}

nsNSSCertificateDB::~nsNSSCertificateDB()
{
}

/*  nsIX509Cert getCertByNickname(in nsIPK11Token aToken,
 *                                in wstring aNickname);
 */
NS_IMETHODIMP
nsNSSCertificateDB::GetCertByNickname(nsIPK11Token *aToken,
                                      const PRUnichar *nickname,
                                      nsIX509Cert **_rvCert)
{
  CERTCertificate *cert = NULL;
  char *asciiname = NULL;
  asciiname = NS_CONST_CAST(char*, NS_ConvertUCS2toUTF8(nickname).get());
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Getting \"%s\"\n", asciiname));
#if 0
  // what it should be, but for now...
  if (aToken) {
    cert = PK11_FindCertFromNickname(asciiname, NULL);
  } else {
    cert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(), asciiname);
  }
#endif
  cert = PK11_FindCertFromNickname(asciiname, NULL);
  if (!cert) {
    cert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(), asciiname);
  }
  if (cert) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("got it\n"));
    nsCOMPtr<nsIX509Cert> pCert = new nsNSSCertificate(cert);
    *_rvCert = pCert;
    NS_ADDREF(*_rvCert);
    return NS_OK;
  }
  *_rvCert = nsnull;
  return NS_ERROR_FAILURE;
}

/* nsIX509Cert getCertByDBKey(in string aDBkey, in nsIPK11Token aToken); */
NS_IMETHODIMP 
nsNSSCertificateDB::GetCertByDBKey(const char *aDBkey, nsIPK11Token *aToken,
                                   nsIX509Cert **_cert)
{
  SECItem keyItem = {siBuffer, nsnull, 0};
  SECItem *dummy;
  *_cert = nsnull; 
  if (!aDBkey) return NS_ERROR_FAILURE;
  dummy = NSSBase64_DecodeBuffer(nsnull, &keyItem, aDBkey,
                                 (PRUint32)PL_strlen(aDBkey)); 
  // In the future, this should actually look on the token.  But for now,
  // take it for granted that the cert has been loaded into the temp db.
  CERTCertificate *cert = CERT_FindCertByKey(CERT_GetDefaultCertDB(),
                                             &keyItem);
  PR_FREEIF(keyItem.data);
  if (cert) {
    nsNSSCertificate *nssCert = new nsNSSCertificate(cert);
    if (nssCert == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(nssCert);
    *_cert = NS_STATIC_CAST(nsIX509Cert*, nssCert);
  }
  return NS_OK;
}

/*
 * void getCertNicknames(in nsIPK11Token aToken, 
 *                       in unsigned long aType,
 *                       out unsigned long count,
 *                       [array, size_is(count)] out wstring certNameList);
 */
NS_IMETHODIMP 
nsNSSCertificateDB::GetCertNicknames(nsIPK11Token *aToken, 
                                     PRUint32      aType,
                                     PRUint32     *_count,
                                     PRUnichar  ***_certNames)
{
  nsresult rv = NS_ERROR_FAILURE;
  /*
   * obtain the cert list from NSS
   */
  CERTCertList *certList = NULL;
  PK11CertListType pk11type;
#if 0
  // this would seem right, but it didn't work...
  // oh, I know why - bonks out on internal slot certs
  if (aType == nsIX509Cert::USER_CERT)
    pk11type = PK11CertListUser;
  else 
#endif
    pk11type = PK11CertListUnique;
  certList = PK11_ListCerts(pk11type, NULL);
  if (!certList)
    goto cleanup;
  /*
   * get list of cert names from list of certs
   * XXX also cull the list (NSS only distinguishes based on user/non-user
   */
  getCertNames(certList, aType, _count, _certNames);
  rv = NS_OK;
  /*
   * finish up
   */
cleanup:
  if (certList)
    CERT_DestroyCertList(certList);
  return rv;
}

/*
 *  [noscript] unsigned long getCertsByType(in unsigned long aType,
 *                                          in nsCertCompareFunc aCertCmpFn,
 *                                          out nsISupportsArray certs);
 */
PRBool 
nsNSSCertificateDB::GetCertsByType(PRUint32           aType,
                                   nsCertCompareFunc  aCertCmpFn,
                                   nsISupportsArray **_certs)
{
  CERTCertList *certList = NULL;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("GetCertsByType"));
  nsCOMPtr<nsISupportsArray> certarray;
  nsresult rv = NS_NewISupportsArray(getter_AddRefs(certarray));
  if (NS_FAILED(rv)) return PR_FALSE;
  certList = PK11_ListCerts(PK11CertListUnique, NULL);
  CERTCertListNode *node;
  int i, count = 0;
  for (node = CERT_LIST_HEAD(certList);
       !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    if (getCertType(node->cert) == aType) {
      nsCOMPtr<nsIX509Cert> pipCert = new nsNSSCertificate(node->cert);
      if (pipCert) {
        for (i=0; i<count; i++) {
          nsCOMPtr<nsISupports> isupport = 
                                      getter_AddRefs(certarray->ElementAt(i));
          nsCOMPtr<nsIX509Cert> cert = do_QueryInterface(isupport);
	  if ((*aCertCmpFn)(pipCert, cert) < 0) {
            certarray->InsertElementAt(pipCert, i);
	    break;
	  }
	}
	if (i == count) certarray->AppendElement(pipCert);
	count++;
      }
    }
  }
  *_certs = certarray;
  NS_ADDREF(*_certs);
  if (certList)
    CERT_DestroyCertList(certList);
  return PR_TRUE;
}

/*
 * [noscript] void importCertificate (in nsIX509Cert cert, 
 *                                    in unsigned long type,
 *                                    in unsigned long trust, 
 *                                    in wchar tokenName); 
 */
NS_IMETHODIMP 
nsNSSCertificateDB::ImportCertificate(nsIX509Cert *cert, 
                                      PRUint32 type,
                                      PRUint32 trusted,
                                      const PRUnichar *nickname)
{
  SECStatus srv = SECFailure;
  nsresult nsrv;
  CERTCertificate *tmpCert = NULL;
  nsNSSCertTrust trust;
  char *nick;
  SECItem der;
  switch (type) {
  case nsIX509Cert::CA_CERT:
    trust.SetValidCA();
    trust.AddCATrust(trusted & nsIX509CertDB::TRUSTED_SSL,
                     trusted & nsIX509CertDB::TRUSTED_EMAIL,
                     trusted & nsIX509CertDB::TRUSTED_OBJSIGN);
    break;
  default:
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  nsrv = cert->GetRawDER((char **)&der.data, &der.len);
  if (nsrv != NS_OK)
    return NS_ERROR_FAILURE;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Creating temp cert\n"));
  tmpCert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &der,
                                    NULL, PR_FALSE, PR_TRUE);
  if (!tmpCert) goto done;
  if (nickname) {
    nick = NS_CONST_CAST(char*, NS_ConvertUCS2toUTF8(nickname).get());
  } else {
    nick = CERT_MakeCANickname(tmpCert);
  }
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Created nick \"%s\"\n", nick));
  /* XXX check to see if cert is perm (it shouldn't be, but NSS asserts if
     it is */
  /* XXX this is an ugly peek into NSS */
  if (tmpCert->isperm) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Cert was already in db %s\n", nick));
    return NS_ERROR_FAILURE;
  }
  srv = CERT_AddTempCertToPerm(tmpCert, nick, trust.GetTrust());
done:
  if (tmpCert) 
    CERT_DestroyCertificate(tmpCert);
  return (srv) ? NS_ERROR_FAILURE : NS_OK;
}

/*
 * void deleteCertificate(in nsIX509Cert aCert);
 */
NS_IMETHODIMP 
nsNSSCertificateDB::DeleteCertificate(nsIX509Cert *aCert)
{
  nsNSSCertificate *nssCert = NS_STATIC_CAST(nsNSSCertificate*, aCert);
  CERTCertificate *cert = nssCert->GetCert();
  if (!cert) return NS_ERROR_FAILURE;
  SECStatus srv;
#if 0
  // for later, to use tokens ...
  if (getCertType(cert) == nsNSSCertificate::USER_CERT) {
    srv = PK11_DeleteTokenCertAndKey(cert, NULL);
  } else {
    srv = SEC_DeletePermCertificate(cert);
  }
#endif
  srv = SEC_DeletePermCertificate(cert);
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("cert deleted: %d", srv));
  CERT_DestroyCertificate(cert);
  return (srv) ? NS_ERROR_FAILURE : NS_OK;
}

/*
 * void setCertTrust(in nsIX509Cert cert,
 *                   in unsigned long type,
 *                   in unsigned long trust);
 */
NS_IMETHODIMP 
nsNSSCertificateDB::SetCertTrust(nsIX509Cert *cert, 
                                 PRUint32 type,
                                 PRUint32 trusted)
{
  SECStatus srv;
  nsNSSCertTrust trust;
  if (type != nsIX509Cert::CA_CERT) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  ////// UGLY kluge until I decide how cert->GetCert() will work
  PRUnichar *wnn;
  cert->GetNickname(&wnn);
  char *nickname = PL_strdup(NS_ConvertUCS2toUTF8(wnn).get()); 
  CERTCertificate *nsscert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(),
                                                     nickname);
  if (!nsscert) {
    return NS_ERROR_FAILURE;
  }
  ////// end of kluge
  // always start with untrusted and move up
  trust.SetValidCA();
  trust.AddCATrust(trusted & nsIX509CertDB::TRUSTED_SSL,
                   trusted & nsIX509CertDB::TRUSTED_EMAIL,
                   trusted & nsIX509CertDB::TRUSTED_OBJSIGN);
  srv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), 
                             nsscert,
                             trust.GetTrust());
  return (srv) ? NS_ERROR_FAILURE : NS_OK;
}

/*
 * boolean getCertTrust(in nsIX509Cert cert,
 *                      in unsigned long trustType);
 */
NS_IMETHODIMP 
nsNSSCertificateDB::GetCertTrust(nsIX509Cert *cert, 
                                 PRUint32 trustType,
                                 PRBool *_isTrusted)
{
  SECStatus srv;
  ////// UGLY kluge until I decide how cert->GetCert() will work
  PRUnichar *wnn;
  cert->GetNickname(&wnn);
  char *nickname = PL_strdup(NS_ConvertUCS2toUTF8(wnn).get()); 
  CERTCertificate *nsscert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(),
                                                     nickname);
  if (!nsscert) {
    return NS_ERROR_FAILURE;
  }
  ////// end of kluge
  CERTCertTrust nsstrust;
  srv = CERT_GetCertTrust(nsscert, &nsstrust);
  nsNSSCertTrust trust(&nsstrust);
  if (trustType & nsIX509CertDB::TRUSTED_SSL) {
    *_isTrusted = trust.HasTrustedCA(PR_TRUE, PR_FALSE, PR_FALSE);
  } else if (trustType & nsIX509CertDB::TRUSTED_EMAIL) {
    *_isTrusted = trust.HasTrustedCA(PR_FALSE, PR_TRUE, PR_FALSE);
  } else if (trustType & nsIX509CertDB::TRUSTED_OBJSIGN) {
    *_isTrusted = trust.HasTrustedCA(PR_FALSE, PR_FALSE, PR_TRUE);
  } else {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

/*
 *  void importPKCS12File(in nsIPK11Token aToken,
 *                        in nsILocalFile aFile);
 */
NS_IMETHODIMP 
nsNSSCertificateDB::ImportPKCS12File(nsIPK11Token *aToken, 
                                     nsILocalFile *aFile)
{
  NS_ENSURE_ARG(aFile);
  nsPKCS12Blob blob;
  blob.SetToken(aToken);
  return blob.ImportFromFile(aFile);
}

/*
 * void exportPKCS12File(in nsIPK11Token aToken,
 *                       in nsILocalFile aFile,
 *                       in PRUint32 count,
 *                       [array, size_is(count)] in wstring aCertNames);
 */
NS_IMETHODIMP 
nsNSSCertificateDB::ExportPKCS12File(nsIPK11Token     *aToken, 
                                     nsILocalFile     *aFile,
                                     PRUint32          count,
                                     nsIX509Cert     **certs)
                                     //const PRUnichar **aCertNames)
{
  NS_ENSURE_ARG(aFile);
  nsPKCS12Blob blob;
  if (count == 0) return NS_OK;
  blob.SetToken(aToken);
  //blob.LoadCerts(aCertNames, count);
  //return blob.ExportToFile(aFile);
  return blob.ExportToFile(aFile, certs, count);
}

/*
 * NSS Helper Routines (private to nsNSSCertificateDB)
 */

#define DELIM '\001'

/*
 * GetSortedNameList
 *
 * Converts a CERTCertList to a list of certificate names
 */
void
nsNSSCertificateDB::getCertNames(CERTCertList *certList,
                                 PRUint32      type, 
                                 PRUint32     *_count,
                                 PRUnichar  ***_certNames)
{
  nsresult rv;
  CERTCertListNode *node;
  PRUint32 numcerts = 0, i=0;
  PRUnichar **tmpArray = NULL;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("List of certs %d:\n", type));
  for (node = CERT_LIST_HEAD(certList);
       !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    if (getCertType(node->cert) == type) {
      numcerts++;
    }
  }
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("num certs: %d\n", numcerts));
  int nc = (numcerts == 0) ? 1 : numcerts;
  tmpArray = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * nc);
  if (numcerts == 0) goto finish;
  for (node = CERT_LIST_HEAD(certList);
       !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    if (getCertType(node->cert) == type) {
      nsNSSCertificate pipCert(node->cert);
      char *dbkey = NULL;
      char *namestr = NULL;
      nsAutoString certstr;
      rv = pipCert.GetDbKey(&dbkey);
      nsAutoString keystr = NS_ConvertASCIItoUCS2(dbkey);
      PR_FREEIF(dbkey);
      if (type == nsIX509Cert::EMAIL_CERT) {
        namestr = node->cert->emailAddr;
      } else {
        namestr = node->cert->nickname;
        char *sc = strchr(namestr, ':');
        if (sc) *sc = DELIM;
      }
      nsAutoString certname = NS_ConvertASCIItoUCS2(namestr);
      certstr.AppendWithConversion(DELIM);
      certstr += certname;
      certstr.AppendWithConversion(DELIM);
      certstr += keystr;
      tmpArray[i++] = certstr.ToNewUnicode();
    }
  }
finish:
  *_count = numcerts;
  *_certNames = tmpArray;
}

/* somewhat follows logic of cert_list_include_cert from PSM 1.x */
PRUint32 
nsNSSCertificateDB::getCertType(CERTCertificate *cert)
{
  char *nick = cert->nickname;
  char *email = cert->emailAddr;
  nsNSSCertTrust trust(cert->trust);
  if (nick) {
    if (trust.HasUser())
      return nsIX509Cert::USER_CERT;
    if (trust.HasCA())
      return nsIX509Cert::CA_CERT;
    if (trust.HasPeer(PR_TRUE, PR_FALSE, PR_FALSE))
      return nsIX509Cert::SERVER_CERT;
  }
  if (email && trust.HasPeer(PR_FALSE, PR_FALSE, PR_TRUE))
    return nsIX509Cert::EMAIL_CERT;
  return nsIX509Cert::UNKNOWN_CERT;
}

