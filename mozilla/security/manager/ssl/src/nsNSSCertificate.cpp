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
#include "prprf.h"

#include "nsNSSComponent.h" // for PIPNSS string bundle calls.
#include "nsCOMPtr.h"
#include "nsISupportsArray.h"
#include "nsILocalFile.h"
#include "nsNSSCertificate.h"
#include "nsPKCS12Blob.h"
#include "nsIX509Cert.h"
#include "nsINSSDialogs.h"
#include "nsNSSASN1Object.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"
#include "nsILocaleService.h"
#include "nsIURI.h"

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
  PRBool HasAnyCA();
  PRBool HasAnyUser();
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
  PRBool HasTrustedPeer(PRBool checkSSL = PR_TRUE, 
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
  memset(&mTrust, 0, sizeof(CERTCertTrust));
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
nsNSSCertTrust::HasAnyCA()
{
  if (hasTrust(mTrust.sslFlags, CERTDB_VALID_CA) ||
      hasTrust(mTrust.emailFlags, CERTDB_VALID_CA) ||
      hasTrust(mTrust.objectSigningFlags, CERTDB_VALID_CA))
    return PR_TRUE;
  return PR_FALSE;
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
nsNSSCertTrust::HasAnyUser()
{
  if (hasTrust(mTrust.sslFlags, CERTDB_USER) ||
      hasTrust(mTrust.emailFlags, CERTDB_USER) ||
      hasTrust(mTrust.objectSigningFlags, CERTDB_USER))
    return PR_TRUE;
  return PR_FALSE;
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

PRBool
nsNSSCertTrust::HasTrustedPeer(PRBool checkSSL, 
                               PRBool checkEmail,  
                               PRBool checkObjSign)
{
  if (checkSSL && !(hasTrust(mTrust.sslFlags, CERTDB_TRUSTED)))
    return PR_FALSE;
  if (checkEmail && !(hasTrust(mTrust.emailFlags, CERTDB_TRUSTED)))
    return PR_FALSE;
  if (checkObjSign && 
       !(hasTrust(mTrust.objectSigningFlags, CERTDB_TRUSTED)))
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
nsNSSCertificate::GetIssuerOrganization(PRUnichar **aOrganization)
{
  NS_ENSURE_ARG(aOrganization);
  if (mIssuerOrg.Length() == 0) {
    PRBool failed = PR_TRUE;
    CERTCertificate *issuer;
    issuer = CERT_FindCertIssuer(mCert, PR_Now(), certUsageSSLClient);
    if (issuer) {
      char *org = CERT_GetOrgName(&issuer->subject);
      if (org) {
        mIssuerOrg = NS_ConvertASCIItoUCS2(org);
        failed = PR_FALSE;
      }
    }
    if (failed) {
      nsresult rv;
      nsCOMPtr<nsINSSComponent> nssComponent(
                     do_GetService(kNSSComponentCID, &rv));
      if (NS_FAILED(rv)) return rv;
      if (!issuer) {
        rv = nssComponent->GetPIPNSSBundleString(
                     NS_LITERAL_STRING("UnknownCertIssuer").get(), mIssuerOrg);
      } else { /* !org */
        rv = nssComponent->GetPIPNSSBundleString(
                     NS_LITERAL_STRING("UnknownCertOrg").get(), mIssuerOrg);
      }
      if (NS_FAILED(rv)) return rv;
    }
  }
  *aOrganization = mIssuerOrg.ToNewUnicode();
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
nsNSSCertificate::GetChain(nsISupportsArray **_rvChain)
{
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
#else // workaround here
  CERTCertificate *cert = mCert;
  /* enumerate the chain for scripting purposes */
  nsCOMPtr<nsISupportsArray> array;
  rv = NS_NewISupportsArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) { 
    goto done; 
  }
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Getting chain for \"%s\"\n", mCert->nickname));
  while (cert) {
    nsCOMPtr<nsIX509Cert> pipCert = new nsNSSCertificate(cert);
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("adding %s to chain\n", cert->nickname));
    array->AppendElement(pipCert);
    if (SECITEM_CompareItem(&cert->derIssuer, &cert->derSubject) == SECEqual)
      break;
    cert = CERT_FindCertIssuer(cert, PR_Now(), certUsageSSLClient);
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
    // HACK alert
    // When the trust of a builtin cert is modified, NSS copies it into the
    // cert db.  At this point, it is now "managed" by the user, and should
    // not be listed with the builtins.  However, in the collection code
    // used by PK11_ListCerts, the cert is found in the temp db, where it
    // has been loaded from the token.  Though the trust is correct (grabbed
    // from the cert db), the source is wrong.  I believe this is a safe
    // way to work around this.
    if (mCert->slot && !mCert->isperm) {
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

static nsresult
GetIntValue(SECItem *versionItem, 
            unsigned long *version)
{
  SECStatus srv;

  srv = SEC_ASN1DecodeInteger(versionItem,version);
  if (srv != SECSuccess) {
    NS_ASSERTION(0,"Could not decode version of cert");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

static nsresult
ProcessVersion(SECItem         *versionItem,
               nsINSSComponent *nssComponent,
               nsIASN1PrintableItem **retItem)
{
  nsresult rv;
  nsString text;
  nsCOMPtr<nsIASN1PrintableItem> printableItem = new nsNSSASN1PrintableItem();
  if (printableItem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
 
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpVersion").get(),
                                      text);
  rv = printableItem->SetDisplayName(text.get());
  if (NS_FAILED(rv))
    return rv;

  // Now to figure out what version this certificate is.
  unsigned long version;

  if (versionItem->data) {
    rv = GetIntValue(versionItem, &version);
    if (NS_FAILED(rv))
      return rv;
  } else {
    // If there is no version present in the cert, then rfc2459
    // says we default to v1 (0)
    version = 0;
  }

  switch (version){
  case 0:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpVersion1").get(),
                                             text);
    break;
  case 1:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpVersion2").get(),
                                             text);
    break;
  case 2:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpVersion3").get(),
                                             text);
    break;
  default:
    NS_ASSERTION(0,"Bad value for cert version");
    rv = NS_ERROR_FAILURE;
  }
    
  if (NS_FAILED(rv))
    return rv;

  rv = printableItem->SetDisplayValue(text.get());
  if (NS_FAILED(rv))
    return rv;

  *retItem = printableItem;
  NS_ADDREF(*retItem);
  return NS_OK;
}

nsresult 
ProcessSerialNumberDER(SECItem         *serialItem, 
                       nsINSSComponent *nssComponent,
                       nsIASN1PrintableItem **retItem)
{
  nsresult rv;
  nsString text;
  nsCOMPtr<nsIASN1PrintableItem> printableItem = new nsNSSASN1PrintableItem();

  if (printableItem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSerialNo").get(),
                                           text); 
  if (NS_FAILED(rv))
    return rv;

  rv = printableItem->SetDisplayName(text.get());
  if (NS_FAILED(rv))
    return rv;

  nsXPIDLCString serialNumber;
  serialNumber = CERT_Hexify(serialItem, 1);
  if (serialNumber == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = printableItem->SetDisplayValue(NS_ConvertASCIItoUCS2(serialNumber).get());
  *retItem = printableItem;
  NS_ADDREF(*retItem);
  return rv;
}

static nsresult
GetDefaultOIDFormat(SECItem *oid,
                    nsString &outString)
{
  char buf[300];
  int len, written;
    
  unsigned long val  = oid->data[0];
  unsigned int  i    = val % 40;
  val /= 40;
  written = PR_snprintf(buf, 300, "%lu %u ", val, i);
  if (written < 0)
    return NS_ERROR_FAILURE;	
  len = written;

  val = 0;
  for (i = 1; i < oid->len; ++i) {
    // In this loop, we have to parse a DER formatted 
    // If the first bit is a 1, then the integer is 
    // represented by more than one byte.  If the 
    // first bit is set then we continue on and add
    // the values of the later bytes until we get 
    // a byte without the first bit set.
    unsigned long j;

    j = oid->data[i];
    val = (val << 7) | (j & 0x7f);
    if (j & 0x80)
      continue;
    written = PR_snprintf(&buf[len], sizeof(buf)-len, "%lu ", val);
    if (written < 0)
      return NS_ERROR_FAILURE;

    len += written;
    NS_ASSERTION(len < sizeof(buf), "OID data to big to display in 300 chars.");
    val = 0;      
  }

  outString = NS_ConvertASCIItoUCS2(buf).get();
  return NS_OK; 
}

static nsresult
GetOIDText(SECItem *oid, nsINSSComponent *nssComponent, nsString &text)
{ 
  nsresult rv;
  SECOidTag oidTag = SECOID_FindOIDTag(oid);

  switch (oidTag) {
  case SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpMD2WithRSA").get(),
                                             text);
    break;
  case SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpMD5WithRSA").get(),
                                             text);
    break;
  case SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSHA1WithRSA").get(),
                                             text);
    break;
  case SEC_OID_AVA_COUNTRY_NAME:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAVACountry").get(),
                                             text);
    break;
  case SEC_OID_AVA_COMMON_NAME:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAVACN").get(),
                                             text);
    break;
  case SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAVAOU").get(),
                                             text);
    break;
  case SEC_OID_AVA_ORGANIZATION_NAME:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAVAOrg").get(),
                                             text);
    break;
  case SEC_OID_AVA_LOCALITY:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAVALocality").get(),
                                             text);
    break;
  case SEC_OID_AVA_DN_QUALIFIER:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAVADN").get(),
                                             text);
    break;
  case SEC_OID_AVA_DC:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAVADC").get(),
                                             text);
    break;
  case SEC_OID_AVA_STATE_OR_PROVINCE:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAVAState").get(),
                                             text);
    break;
  case SEC_OID_PKCS1_RSA_ENCRYPTION:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpRSAEncr").get(),
                                             text);
    break;
  case SEC_OID_X509_KEY_USAGE:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpKeyUsage").get(),
                                             text);
    break;
  case SEC_OID_NS_CERT_EXT_CERT_TYPE:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpCertType").get(),
                                             text);
    break;
  case SEC_OID_X509_AUTH_KEY_ID:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAuthKeyID").get(),
                                             text);
    break;
  case SEC_OID_RFC1274_UID:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpUserID").get(),
                                             text);
    break;
  case SEC_OID_PKCS9_EMAIL_ADDRESS:
    rv = nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpPK9Email").get(),
                                             text);
    break;
  default:
    rv = GetDefaultOIDFormat(oid, text);
    if (NS_FAILED(rv))
      return rv;

    const PRUnichar *params[1] = {text.get()};
    nsXPIDLString text2;
    rv = nssComponent->PIPBundleFormatStringFromName(NS_LITERAL_STRING("CertDumpDefOID").get(),
                                                     params, 1,
                                                     getter_Copies(text2));
    text = text2;
    break;
  }
  return rv;  
}

#define SEPARATOR "\n"

static nsresult
ProcessRawBytes(SECItem *data, nsString &text)
{
  // This function is used to display some DER bytes
  // that we have not added support for decoding.
  // It prints the value of the byte out into a 
  // string that can later be displayed as a byte
  // string.  We place a new line after 24 bytes
  // to break up extermaly long sequence of bytes.
  PRUint32 i;
  char buffer[5];
  for (i=0; i<data->len; i++) {
    PR_snprintf(buffer, 5, "%02x ", data->data[i]);
    text.Append(NS_ConvertASCIItoUCS2(buffer).get());
    if ((i+1)%24 == 0) {
      text.Append(NS_LITERAL_STRING(SEPARATOR).get());
    }
  }
  return NS_OK;
}    

static nsresult
ProcessNSCertTypeExtensions(SECItem  *extData, 
                            nsString &text,
                            nsINSSComponent *nssComponent)
{
  SECItem decoded;
  decoded.data = nsnull;
  decoded.len  = 0;
  SEC_ASN1DecodeItem(nsnull, &decoded, SEC_BitStringTemplate, extData);
  unsigned char nsCertType = decoded.data[0];
  nsString local;
  nsMemory::Free(decoded.data);
  if (nsCertType & NS_CERT_TYPE_SSL_CLIENT) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("VerifySSLClient").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (nsCertType & NS_CERT_TYPE_SSL_SERVER) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("VerifySSLServer").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (nsCertType & NS_CERT_TYPE_EMAIL) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpCertTypeEmail").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (nsCertType & NS_CERT_TYPE_OBJECT_SIGNING) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("VerifyObjSign").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (nsCertType & NS_CERT_TYPE_SSL_CA) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("VerifySSLCA").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (nsCertType & NS_CERT_TYPE_EMAIL_CA) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpEmailCA").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (nsCertType & NS_CERT_TYPE_OBJECT_SIGNING_CA) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("VerifyObjSign").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  return NS_OK;
}

static nsresult
ProcessKeyUsageExtension(SECItem *extData, nsString &text,
                         nsINSSComponent *nssComponent)
{
  SECItem decoded;
  decoded.data = nsnull;
  decoded.len  = 0;
  SEC_ASN1DecodeItem(nsnull, &decoded, SEC_BitStringTemplate, extData);
  unsigned char keyUsage = decoded.data[0];
  nsString local;
  nsMemory::Free(decoded.data);  
  if (keyUsage & KU_DIGITAL_SIGNATURE) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpKUSign").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (keyUsage & KU_NON_REPUDIATION) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpKUNonRep").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (keyUsage & KU_KEY_ENCIPHERMENT) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpKUEnc").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (keyUsage & KU_DATA_ENCIPHERMENT) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpKUDEnc").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (keyUsage & KU_KEY_AGREEMENT) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpKUKA").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (keyUsage & KU_KEY_CERT_SIGN) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpKUCertSign").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }
  if (keyUsage & KU_CRL_SIGN) {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpKUCRLSign").get(),
                                        local);
    text.Append(local.get());
    text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  }

  return NS_OK;
}

static nsresult
ProcessExtensionData(SECOidTag oidTag, SECItem *extData, 
                     nsString &text, nsINSSComponent *nssComponent)
{
  nsresult rv;
  switch (oidTag) {
  case SEC_OID_NS_CERT_EXT_CERT_TYPE:
    rv = ProcessNSCertTypeExtensions(extData, text, nssComponent);
    break;
  case SEC_OID_X509_KEY_USAGE:
    rv = ProcessKeyUsageExtension(extData, text, nssComponent);
    break;
  default:
    rv = ProcessRawBytes(extData, text);
    break; 
  }
  return rv;
}

static nsresult
ProcessSingleExtension(CERTCertExtension *extension,
                       nsINSSComponent *nssComponent,
                       nsIASN1PrintableItem **retExtension)
{
  nsString text;
  GetOIDText(&extension->id, nssComponent, text);
  nsCOMPtr<nsIASN1PrintableItem>extensionItem = new nsNSSASN1PrintableItem();
  if (extensionItem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  extensionItem->SetDisplayName(text.get());
  SECOidTag oidTag = SECOID_FindOIDTag(&extension->id);
  text.Truncate();
  if (extension->critical.data != nsnull) {
    if (extension->critical.data[0]) {
      nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpCritical").get(),
                                          text);
    } else {
      nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpNonCritical").get(),
                                         text);
    }
  } else {
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpNonCritical").get(),
                                        text);
  }
  text.Append(NS_LITERAL_STRING(SEPARATOR).get());
  nsresult rv = ProcessExtensionData(oidTag, &extension->value, text, 
                                     nssComponent);
  if (NS_FAILED(rv))
    return rv;

  extensionItem->SetDisplayValue(text.get());
  *retExtension = extensionItem;
  NS_ADDREF(*retExtension);
  return NS_OK;
}

#ifdef DEBUG_javi
void
DumpASN1Object(nsIASN1Object *object, unsigned int level)
{
  PRUnichar *dispNameU, *dispValU;
  unsigned int i;
  nsCOMPtr<nsISupportsArray> asn1Objects;
  nsCOMPtr<nsISupports> isupports;
  nsCOMPtr<nsIASN1Object> currObject;
  PRBool processObjects;
  PRUint32 numObjects;

  for (i=0; i<level; i++)
    printf ("  ");

  object->GetDisplayName(&dispNameU);
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
      object->GetDisplayValue(&dispValU);
      printf("= %s\n", NS_ConvertUCS2toUTF8(dispValU).get()); 
      PR_Free(dispValU);
    }
  } else { 
    object->GetDisplayValue(&dispValU);
    printf("%s = %s\n",NS_ConvertUCS2toUTF8(dispNameU).get(), 
                       NS_ConvertUCS2toUTF8(dispValU).get()); 
    PR_Free(dispValU);
  }
  PR_Free(dispNameU);
}
#endif

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
  if (algID->parameters.data[0] == nsIASN1Object::ASN1_NULL) {
    sequence->SetDisplayValue(text.get());
    sequence->SetProcessObjects(PR_FALSE);
  } else {
    nsCOMPtr<nsIASN1PrintableItem> printableItem = new nsNSSASN1PrintableItem();
    printableItem->SetDisplayValue(text.get());
    nsCOMPtr<nsISupportsArray>asn1Objects;
    sequence->GetASN1Objects(getter_AddRefs(asn1Objects));
    asn1Objects->AppendElement(printableItem);
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpAlgID").get(),
                                        text);
    printableItem->SetDisplayName(text.get());
    printableItem = new nsNSSASN1PrintableItem();
    asn1Objects->AppendElement(printableItem);
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpParams").get(),
                                        text);
    printableItem->SetDisplayName(text.get()); 
    ProcessRawBytes(&algID->parameters,text);
    printableItem->SetDisplayValue(text.get());
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
  dateFormatter->FormatPRTime(nsnull, kDateFormatShort, kTimeFormatNone,
                              dispTime, text);
  nsCOMPtr<nsIASN1PrintableItem> printableItem = new nsNSSASN1PrintableItem();
  if (printableItem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  printableItem->SetDisplayValue(text.get());
  printableItem->SetDisplayName(displayName);
  nsCOMPtr<nsISupportsArray> asn1Objects;
  parentSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  asn1Objects->AppendElement(printableItem);
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
  spkiSequence->SetDisplayName(text.get());

  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSPKIAlg").get(),
                                      text);
  nsCOMPtr<nsIASN1Sequence> sequenceItem;
  nsresult rv = ProcessSECAlgorithmID(&spki->algorithm, nssComponent,
                                      getter_AddRefs(sequenceItem));
  if (NS_FAILED(rv))
    return rv;
  sequenceItem->SetDisplayName(text.get());
  nsCOMPtr<nsISupportsArray> asn1Objects;
  spkiSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  asn1Objects->AppendElement(sequenceItem);

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

  printableItem->SetDisplayValue(text.get());
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSubjPubKey").get(),
                                      text);
  printableItem->SetDisplayName(text.get());
  asn1Objects->AppendElement(printableItem);
  
  parentSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  asn1Objects->AppendElement(spkiSequence);
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
  extensionSequence->SetDisplayName(text.get());
  PRInt32 i;
  nsresult rv;
  nsCOMPtr<nsIASN1PrintableItem> newExtension;
  nsCOMPtr<nsISupportsArray> asn1Objects;
  extensionSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  for (i=0; extensions[i] != nsnull; i++) {
    rv = ProcessSingleExtension(extensions[i], nssComponent,
                                getter_AddRefs(newExtension));
    if (NS_FAILED(rv))
      return rv;

    asn1Objects->AppendElement(newExtension);
  }
  parentSequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  asn1Objects->AppendElement(extensionSequence);
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
      finalString.Append(temp.get());
      finalString.Append(NS_LITERAL_STRING("\n").get());
    }
  }
  *value = finalString.ToNewUnicode();    
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
  sequence->SetDisplayName(text.get());
  nsCOMPtr<nsIASN1PrintableItem> printableItem;
  
  nsCOMPtr<nsISupportsArray> asn1Objects;
  sequence->GetASN1Objects(getter_AddRefs(asn1Objects));

  nsresult rv = ProcessVersion(&mCert->version, nssComponent,
                               getter_AddRefs(printableItem));
  if (NS_FAILED(rv))
    return rv;

  asn1Objects->AppendElement(printableItem);
  
  rv = ProcessSerialNumberDER(&mCert->serialNumber, nssComponent,
                              getter_AddRefs(printableItem));

  if (NS_FAILED(rv))
    return rv;
  asn1Objects->AppendElement(printableItem); 

  nsCOMPtr<nsIASN1Sequence> algID;
  rv = ProcessSECAlgorithmID(&mCert->signature,
                             nssComponent, getter_AddRefs(algID));
  if (NS_FAILED(rv))
    return rv;

  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSigAlg").get(),
                                      text);
  algID->SetDisplayName(text.get());
  asn1Objects->AppendElement(algID);

  nsXPIDLString value;
  ProcessName(&mCert->issuer, nssComponent, getter_Copies(value));

  printableItem = new nsNSSASN1PrintableItem();
  if (printableItem == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  printableItem->SetDisplayValue(value);
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpIssuer").get(),
                                      text);
  printableItem->SetDisplayName(text.get());
  asn1Objects->AppendElement(printableItem);
  
  nsCOMPtr<nsIASN1Sequence> validitySequence = new nsNSSASN1Sequence();
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpValidity").get(),
                                      text);
  validitySequence->SetDisplayName(text.get());
  asn1Objects->AppendElement(validitySequence);
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

  printableItem->SetDisplayName(text.get());
  ProcessName(&mCert->subject, nssComponent,getter_Copies(value));
  printableItem->SetDisplayValue(value);
  asn1Objects->AppendElement(printableItem);

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

    printableItem->SetDisplayValue(text.get());
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpIssuerUniqueID").get(),
                                         text);
    printableItem->SetDisplayName(text.get());
    asn1Objects->AppendElement(printableItem);
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

    printableItem->SetDisplayValue(text.get());
    nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSubjectUniqueID").get(),
                                         text);
    printableItem->SetDisplayName(text.get());
    asn1Objects->AppendElement(printableItem);

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

nsresult
nsNSSCertificate::CreateASN1Struct()
{
  nsCOMPtr<nsIASN1Sequence> sequence = new nsNSSASN1Sequence();

  mASN1Structure = sequence; 
  if (mASN1Structure == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsISupportsArray> asn1Objects;
  sequence->GetASN1Objects(getter_AddRefs(asn1Objects));
  nsXPIDLCString title;
  GetWindowTitle(getter_Copies(title));
  
  mASN1Structure->SetDisplayName(NS_ConvertASCIItoUCS2(title).get());
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

  asn1Objects->AppendElement(sequence);
  nsCOMPtr<nsIASN1Sequence> algID;

  rv = ProcessSECAlgorithmID(&mCert->signatureWrap.signatureAlgorithm, 
                             nssComponent, getter_AddRefs(algID));
  if (NS_FAILED(rv))
    return rv;
  nsString text;
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSigAlg").get(),
                                      text);
  algID->SetDisplayName(text.get());
  asn1Objects->AppendElement(algID);
  nsCOMPtr<nsIASN1PrintableItem>printableItem = new nsNSSASN1PrintableItem();
  nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpCertSig").get(),
                                      text);
  printableItem->SetDisplayName(text.get());
  // The signatureWrap is encoded as a bit string.
  // The function ProcessRawBytes expects the
  // length to be in bytes, so let's convert the
  // length in a temporary SECItem
  SECItem temp;
  temp.data = mCert->signatureWrap.signature.data;
  temp.len  = mCert->signatureWrap.signature.len / 8;
  text.Truncate();
  ProcessRawBytes(&temp,text);
  printableItem->SetDisplayValue(text.get());
  asn1Objects->AppendElement(printableItem);
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
  nsCOMPtr<nsIInterfaceRequestor> cxt = new PipUIContext();
  certList = PK11_ListCerts(PK11CertListUnique, cxt);
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

char *
default_nickname(CERTCertificate *cert, nsIInterfaceRequestor* ctx)
{   
  nsresult rv;
  char *username = NULL;
  char *caname = NULL;
  char *nickname = NULL;
  char *tmp = NULL;
  int count;
  char *nickFmt=NULL, *nickFmtWithNum = NULL;
  CERTCertificate *dummycert;
  PK11SlotInfo *slot=NULL;
  CK_OBJECT_HANDLE keyHandle;
  nsAutoString tmpNickFmt;
  nsAutoString tmpNickFmtWithNum;

  CERTCertDBHandle *defaultcertdb = CERT_GetDefaultCertDB();
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv)) goto loser; 

  username = CERT_GetCommonName(&cert->subject);
  if ( username == NULL ) 
    username = PL_strdup("");

  if ( username == NULL ) 
    goto loser;
    
  caname = CERT_GetOrgName(&cert->issuer);
  if ( caname == NULL ) 
    caname = PL_strdup("");
  
  if ( caname == NULL ) 
    goto loser;
  
  count = 1;
  nssComponent->GetPIPNSSBundleString(
                              NS_LITERAL_STRING("nick_template").get(),
                              tmpNickFmt);
  nickFmt = tmpNickFmt.ToNewUTF8String();

  nssComponent->GetPIPNSSBundleString(
                              NS_LITERAL_STRING("nick_template_with_num").get(),
                              tmpNickFmtWithNum);
  nickFmtWithNum = tmpNickFmtWithNum.ToNewUTF8String();


  nickname = PR_smprintf(nickFmt, username, caname);
  /*
   * We need to see if the private key exists on a token, if it does
   * then we need to check for nicknames that already exist on the smart
   * card.
   */
  slot = PK11_KeyForCertExists(cert, &keyHandle, ctx);
  if (slot == NULL) {
    goto loser;
  }
  if (!PK11_IsInternal(slot)) {
    tmp = PR_smprintf("%s:%s", PK11_GetTokenName(slot), nickname);
    PR_Free(nickname);
    nickname = tmp;
    tmp = NULL;
  }
  tmp = nickname;
  while ( 1 ) {	
    if ( count > 1 ) {
      nickname = PR_smprintf("%s #%d", tmp, count);
    }
  
    if ( nickname == NULL ) 
      goto loser;
 
    if (PK11_IsInternal(slot)) {
      /* look up the nickname to make sure it isn't in use already */
      dummycert = CERT_FindCertByNickname(defaultcertdb, nickname);
      
    } else {
      /*
       * Check the cert against others that already live on the smart 
       * card.
       */
      dummycert = PK11_FindCertFromNickname(nickname, ctx);
      if (dummycert != NULL) {
	/*
	 * Make sure the subject names are different.
	 */ 
	if (CERT_CompareName(&cert->subject, &dummycert->subject) == SECEqual)
	{
	  /*
	   * There is another certificate with the same nickname and
	   * the same subject name on the smart card, so let's use this
	   * nickname.
	   */
	  CERT_DestroyCertificate(dummycert);
	  dummycert = NULL;
	}
      }
    }
    if ( dummycert == NULL ) 
      goto done;
    
    /* found a cert, destroy it and loop */
    CERT_DestroyCertificate(dummycert);
    if (tmp != nickname) PR_Free(nickname);
    count++;
  } /* end of while(1) */
    
loser:
  if ( nickname ) {
    PR_Free(nickname);
  }
  nickname = NULL;
done:
  if ( caname ) {
    PR_Free(caname);
  }
  if ( username )  {
    PR_Free(username);
  }
  if (slot != NULL) {
      PK11_FreeSlot(slot);
      if (nickname != NULL) {
	      tmp = nickname;
	      nickname = strchr(tmp, ':');
	      if (nickname != NULL) {
	        nickname++;
	        nickname = PL_strdup(nickname);
	        PR_Free(tmp);
	      } else {
	        nickname = tmp;
	        tmp = NULL;
	      }
      }
    }
    PR_FREEIF(tmp);
    return(nickname);
}
static SECStatus PR_CALLBACK
collect_certs(void *arg, SECItem **certs, int numcerts)
{
    CERTDERCerts *collectArgs;
    SECItem *cert;
    SECStatus rv;

    collectArgs = (CERTDERCerts *)arg;

    collectArgs->numcerts = numcerts;
    collectArgs->rawCerts = (SECItem *) PORT_ArenaZAlloc(collectArgs->arena,
                                           sizeof(SECItem) * numcerts);
    if ( collectArgs->rawCerts == NULL )
      return(SECFailure);
    cert = collectArgs->rawCerts;

    while ( numcerts-- ) {
        rv = SECITEM_CopyItem(collectArgs->arena, cert, *certs);
        if ( rv == SECFailure )
          return(SECFailure);
        cert++;
        certs++;
    }

    return (SECSuccess);
}

NS_IMETHODIMP 
nsNSSCertificateDB::ImportUserCertificate(char *data, PRUint32 length, nsIInterfaceRequestor *ctx)
{
  PK11SlotInfo *slot;
  char * nickname = NULL;
  SECStatus sec_rv;
  int numCACerts;
	SECItem *CACerts;
	CERTDERCerts * collectArgs;
	PRArenaPool *arena;
	CERTCertificate * cert=NULL;

  arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if ( arena == NULL ) 
    goto loser;

  collectArgs = (CERTDERCerts *)PORT_ArenaZAlloc(arena, sizeof(CERTDERCerts));
  if ( collectArgs == NULL ) 
    goto loser;

  collectArgs->arena = arena;
  sec_rv = CERT_DecodeCertPackage(data, length, collect_certs, 
			      (void *)collectArgs);
  if (sec_rv != PR_SUCCESS)
    goto loser;

  cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), collectArgs->rawCerts,
                	       (char *)NULL, PR_FALSE, PR_TRUE);
  if (!cert)
    goto loser;

  slot = PK11_KeyForCertExists(cert, NULL, ctx);
  if ( slot == NULL ) {
    goto loser;
  }
  PK11_FreeSlot(slot);

  /* pick a nickname for the cert */
  if (cert->subjectList && cert->subjectList->entry && 
    cert->subjectList->entry->nickname) {
  	nickname = cert->subjectList->entry->nickname;
  } else {
    nickname = default_nickname(cert, ctx);
  }

  /* user wants to import the cert */
  slot = PK11_ImportCertForKey(cert, nickname, ctx);
  if (!slot) 
      goto loser;
  PK11_FreeSlot(slot);
  numCACerts = collectArgs->numcerts - 1;

  if (numCACerts) {
  	CACerts = collectArgs->rawCerts+1;
    sec_rv = CERT_ImportCAChain(CACerts, numCACerts, certUsageUserCertImport);
  }
  
loser:
  if (arena) {
    PORT_FreeArena(arena, PR_FALSE);
  }
  CERT_DestroyCertificate(cert);
  return (sec_rv) ? NS_ERROR_FAILURE : NS_OK;
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
  if (cert->slot) {
    // To delete a cert of a slot (builtin, most likely), mark it as
    // completely untrusted.
    nsNSSCertTrust trust(0, 0, 0);
    srv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), 
                               cert, trust.GetTrust());
  } else {
    srv = SEC_DeletePermCertificate(cert);
  }
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
  nsNSSCertificate *pipCert = NS_STATIC_CAST(nsNSSCertificate *, cert);
  CERTCertificate *nsscert = pipCert->GetCert();
  if (type == nsIX509Cert::CA_CERT) {
    // always start with untrusted and move up
    trust.SetValidCA();
    trust.AddCATrust(trusted & nsIX509CertDB::TRUSTED_SSL,
                     trusted & nsIX509CertDB::TRUSTED_EMAIL,
                     trusted & nsIX509CertDB::TRUSTED_OBJSIGN);
    srv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), 
                               nsscert,
                               trust.GetTrust());
  } else if (type == nsIX509Cert::SERVER_CERT) {
    // always start with untrusted and move up
    trust.SetValidPeer();
    trust.AddPeerTrust(trusted & nsIX509CertDB::TRUSTED_SSL, 0, 0);
    srv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), 
                               nsscert,
                               trust.GetTrust());
  } else {
    // ignore user and email certs
    return NS_OK;
  }
  return (srv) ? NS_ERROR_FAILURE : NS_OK;
}

/*
 * boolean getCertTrust(in nsIX509Cert cert,
 *                      in unsigned long certType,
 *                      in unsigned long trustType);
 */
NS_IMETHODIMP 
nsNSSCertificateDB::GetCertTrust(nsIX509Cert *cert, 
                                 PRUint32 certType,
                                 PRUint32 trustType,
                                 PRBool *_isTrusted)
{
  SECStatus srv;
  nsNSSCertificate *pipCert = NS_STATIC_CAST(nsNSSCertificate *, cert);
  CERTCertificate *nsscert = pipCert->GetCert();
  CERTCertTrust nsstrust;
  srv = CERT_GetCertTrust(nsscert, &nsstrust);
  nsNSSCertTrust trust(&nsstrust);
  if (certType == nsIX509Cert::CA_CERT) {
    if (trustType & nsIX509CertDB::TRUSTED_SSL) {
      *_isTrusted = trust.HasTrustedCA(PR_TRUE, PR_FALSE, PR_FALSE);
    } else if (trustType & nsIX509CertDB::TRUSTED_EMAIL) {
      *_isTrusted = trust.HasTrustedCA(PR_FALSE, PR_TRUE, PR_FALSE);
    } else if (trustType & nsIX509CertDB::TRUSTED_OBJSIGN) {
      *_isTrusted = trust.HasTrustedCA(PR_FALSE, PR_FALSE, PR_TRUE);
    } else {
      return NS_ERROR_FAILURE;
    }
  } else if (certType == nsIX509Cert::SERVER_CERT) {
    if (trustType & nsIX509CertDB::TRUSTED_SSL) {
      *_isTrusted = trust.HasTrustedPeer(PR_TRUE, PR_FALSE, PR_FALSE);
    } else if (trustType & nsIX509CertDB::TRUSTED_EMAIL) {
      *_isTrusted = trust.HasTrustedPeer(PR_FALSE, PR_TRUE, PR_FALSE);
    } else if (trustType & nsIX509CertDB::TRUSTED_OBJSIGN) {
      *_isTrusted = trust.HasTrustedPeer(PR_FALSE, PR_FALSE, PR_TRUE);
    } else {
      return NS_ERROR_FAILURE;
    }
  } /* user or email, ignore */
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

/* Header file */
class nsOCSPResponder : public nsIOCSPResponder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOCSPRESPONDER

  nsOCSPResponder();
  nsOCSPResponder(const PRUnichar*, const PRUnichar*);
  virtual ~nsOCSPResponder();
  /* additional members */
  static PRInt32 CmpCAName(nsIOCSPResponder *a, nsIOCSPResponder *b);
  static PRInt32 CompareEntries(nsIOCSPResponder *a, nsIOCSPResponder *b);
  static PRBool IncludeCert(CERTCertificate *aCert);
private:
  nsString mCA;
  nsString mURL;
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsOCSPResponder, nsIOCSPResponder)

nsOCSPResponder::nsOCSPResponder()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsOCSPResponder::nsOCSPResponder(const PRUnichar * aCA, const PRUnichar * aURL)
{
  NS_INIT_ISUPPORTS();
  mCA.Assign(aCA);
  mURL.Assign(aURL);
}

nsOCSPResponder::~nsOCSPResponder()
{
  /* destructor code */
}

/* readonly attribute */
NS_IMETHODIMP nsOCSPResponder::GetResponseSigner(PRUnichar** aCA)
{
  NS_ENSURE_ARG(aCA);
  *aCA = mCA.ToNewUnicode();
  return NS_OK;
}

/* readonly attribute */
NS_IMETHODIMP nsOCSPResponder::GetServiceURL(PRUnichar** aURL)
{
  NS_ENSURE_ARG(aURL);
  *aURL = mURL.ToNewUnicode();
  return NS_OK;
}

PRBool nsOCSPResponder::IncludeCert(CERTCertificate *aCert)
{
  CERTCertTrust *trust;
  char *nickname;

  trust = aCert->trust;
  nickname = aCert->nickname;

  if ( ( ( trust->sslFlags & CERTDB_INVISIBLE_CA ) ||
         (trust->emailFlags & CERTDB_INVISIBLE_CA ) ||
         (trust->objectSigningFlags & CERTDB_INVISIBLE_CA ) ) ||
       nickname == NULL) {
      return PR_FALSE;
  }
  if ((trust->sslFlags & CERTDB_VALID_CA) ||
      (trust->emailFlags & CERTDB_VALID_CA) ||
      (trust->objectSigningFlags & CERTDB_VALID_CA)) {
      return PR_TRUE;
  }
  return PR_FALSE;
}

// CmpByCAName
//
// Compare two responders their token name.  Returns -1, 0, 1 as
// in strcmp.  No token name (null) is treated as >.
PRInt32 nsOCSPResponder::CmpCAName(nsIOCSPResponder *a, nsIOCSPResponder *b)
{
  PRInt32 cmp1;
  nsXPIDLString aTok, bTok;
  a->GetResponseSigner(getter_Copies(aTok));
  b->GetResponseSigner(getter_Copies(bTok));
  if (aTok != nsnull && bTok != nsnull) {
    nsAutoString aStr(aTok);
    cmp1 = aStr.CompareWithConversion(bTok);
  } else {
    cmp1 = (aTok == nsnull) ? 1 : -1;
  }
  return cmp1;
}

// ocsp_compare_entries
//
// Compare two responders.  Returns -1, 0, 1 as
// in strcmp.  Entries with urls come before those without urls.
PRInt32 nsOCSPResponder::CompareEntries(nsIOCSPResponder *a, nsIOCSPResponder *b)
{
  nsXPIDLString aURL, bURL;
  nsAutoString aURLAuto, bURLAuto;

  a->GetServiceURL(getter_Copies(aURL));
  aURLAuto.Assign(aURL);
  b->GetServiceURL(getter_Copies(bURL));
  bURLAuto.Assign(bURL);

  if (aURLAuto.Length() > 0 ) {
    if (bURLAuto.Length() > 0) {
      return nsOCSPResponder::CmpCAName(a, b);
    } else {
      return -1;
    }
  } else {
    if (bURLAuto.Length() > 0) {
      return 1;
    } else {
      return nsOCSPResponder::CmpCAName(a, b);
    }
  }
}

static SECStatus PR_CALLBACK 
GetOCSPResponders (CERTCertificate *aCert,
                   SECItem         *aDBKey,
                   void            *aArg)
{
  nsISupportsArray *array = NS_STATIC_CAST(nsISupportsArray*, aArg);
  PRUnichar* nn = nsnull;
  PRUnichar* url = nsnull;
  char *serviceURL = nsnull;
  char *nickname = nsnull;
  PRUint32 i, count;
  nsresult rv;

  // Are we interested in this cert //
  if (!nsOCSPResponder::IncludeCert(aCert)) {
    return SECSuccess;
  }

  // Get the AIA and nickname //
  serviceURL = CERT_GetOCSPAuthorityInfoAccessLocation(aCert);
  if (serviceURL) {
	url = NS_ConvertASCIItoUCS2(serviceURL).ToNewUnicode();
  }

  nickname = aCert->nickname;
  nn = NS_ConvertASCIItoUCS2(nickname).ToNewUnicode();

  nsCOMPtr<nsIOCSPResponder> new_entry = new nsOCSPResponder(nn, url);

  // Sort the items according to nickname //
  rv = array->Count(&count);
  for (i=0; i < count; ++i) {
    nsCOMPtr<nsISupports> isupport = getter_AddRefs(array->ElementAt(i));
    nsCOMPtr<nsIOCSPResponder> entry = do_QueryInterface(isupport);
    if (nsOCSPResponder::CompareEntries(new_entry, entry) < 0) {
      array->InsertElementAt(new_entry, i);
      break;
    }
  }
  if (i == count) {
    array->AppendElement(new_entry);
  }
  return SECSuccess;
}

/*
 * getOCSPResponders
 *
 * Export a set of certs and keys from the database to a PKCS#12 file.
*/
NS_IMETHODIMP 
nsNSSCertificateDB::GetOCSPResponders(nsISupportsArray ** aResponders)
{
  SECStatus sec_rv;
  nsCOMPtr<nsISupportsArray> respondersArray;
  nsresult rv = NS_NewISupportsArray(getter_AddRefs(respondersArray));
  if (NS_FAILED(rv)) {
    return rv;
  }

  sec_rv = SEC_TraversePermCerts(CERT_GetDefaultCertDB(),
                              ::GetOCSPResponders,
                              respondersArray);
  if (sec_rv == SECSuccess) {
    sec_rv = PK11_TraverseSlotCerts(::GetOCSPResponders,
                                 respondersArray,
                                 nsnull);
  }
  if (sec_rv != SECSuccess) {
    goto loser;
  }

  *aResponders = respondersArray;
  NS_IF_ADDREF(*aResponders);
  return NS_OK;
loser:
  return NS_ERROR_FAILURE;
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
    if (trust.HasAnyUser())
      return nsIX509Cert::USER_CERT;
    if (trust.HasAnyCA())
      return nsIX509Cert::CA_CERT;
    if (trust.HasPeer(PR_TRUE, PR_FALSE, PR_FALSE))
      return nsIX509Cert::SERVER_CERT;
  }
  if (email && trust.HasPeer(PR_FALSE, PR_FALSE, PR_TRUE))
    return nsIX509Cert::EMAIL_CERT;
  return nsIX509Cert::UNKNOWN_CERT;
}


NS_IMETHODIMP 
nsNSSCertificateDB::ImportCrl (char *aData, PRUint32 aLength, nsIURI * aURI, PRUint32 aType)
{
  PRArenaPool *arena = NULL;
  CERTCertificate *caCert;
  SECItem derName = { siBuffer, NULL, 0 };
  SECItem derCrl;
  CERTSignedData sd;
  SECStatus sec_rv;
  CERTSignedCrl *crl;
  nsXPIDLCString url;
  aURI->GetSpec(getter_Copies(url));
  arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (!arena) {
    goto loser;
  }
  memset(&sd, 0, sizeof(sd));

  derCrl.data = (unsigned char*)aData;
  derCrl.len = aLength;
  sec_rv = CERT_KeyFromDERCrl(arena, &derCrl, &derName);
  if (sec_rv != SECSuccess) {
    goto loser;
  }

  caCert = CERT_FindCertByName(CERT_GetDefaultCertDB(), &derName);
  if (!caCert) {
    if (aType == SEC_KRL_TYPE){
      goto loser;
    }
  } else {
    sec_rv = SEC_ASN1DecodeItem(arena,
                            &sd, CERT_SignedDataTemplate, 
                            &derCrl);
    if (sec_rv != SECSuccess) {
      goto loser;
    }
    sec_rv = CERT_VerifySignedData(&sd, caCert, PR_Now(),
                               nsnull);
    if (sec_rv != SECSuccess) {
      goto loser;
    }
  }
    
  crl = SEC_NewCrl(CERT_GetDefaultCertDB(), (char*)url.get(), &derCrl,
                   aType);
  if (!crl) {
    goto loser;
  }
  SSL_ClearSessionCache();
  SEC_DestroyCrl(crl);
  return NS_OK;
loser:
  return NS_ERROR_FAILURE;;
}

/* Header file */
class nsCrlEntry : public nsICrlEntry
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICRLENTRY

  nsCrlEntry();
  nsCrlEntry(const PRUnichar*, const PRUnichar*, const PRUnichar*);
  virtual ~nsCrlEntry();
  /* additional members */
private:
  nsString mName;
  nsString mLastUpdate;
  nsString mNextUpdate;
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsCrlEntry, nsICrlEntry)

nsCrlEntry::nsCrlEntry()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsCrlEntry::nsCrlEntry(const PRUnichar * aName, const PRUnichar * aLastUpdate, const PRUnichar *aNextUpdate)
{
  NS_INIT_ISUPPORTS();
  mName.Assign(aName);
  mLastUpdate.Assign(aLastUpdate);
  mNextUpdate.Assign(aNextUpdate);
}

nsCrlEntry::~nsCrlEntry()
{
  /* destructor code */
}

/* readonly attribute */
NS_IMETHODIMP nsCrlEntry::GetName(PRUnichar** aName)
{
  NS_ENSURE_ARG(aName);
  *aName = mName.ToNewUnicode();
  return NS_OK;
}

/* readonly attribute */
NS_IMETHODIMP nsCrlEntry::GetLastUpdate(PRUnichar** aLastUpdate)
{
  NS_ENSURE_ARG(aLastUpdate);
  *aLastUpdate = mLastUpdate.ToNewUnicode();
  return NS_OK;
}

/* readonly attribute */
NS_IMETHODIMP nsCrlEntry::GetNextUpdate(PRUnichar** aNextUpdate)
{
  NS_ENSURE_ARG(aNextUpdate);
  *aNextUpdate = mNextUpdate.ToNewUnicode();
  return NS_OK;
}

/*
 * getCRLs
 *
 * Export a set of certs and keys from the database to a PKCS#12 file.
*/
NS_IMETHODIMP 
nsNSSCertificateDB::GetCrls(nsISupportsArray ** aCrls)
{
  SECStatus sec_rv;
  CERTCrlHeadNode *head = nsnull;
  CERTCrlNode *node = nsnull;
  CERTCertificate *caCert = nsnull;
  nsAutoString name;
  nsAutoString nextUpdate;
  nsAutoString lastUpdate;
  PRTime tmpDate;
  nsCOMPtr<nsISupportsArray> crlsArray;
  nsresult rv;
  rv = NS_NewISupportsArray(getter_AddRefs(crlsArray));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIDateTimeFormat> dateFormatter =
     do_CreateInstance(kDateTimeFormatCID, &rv);
  if (NS_FAILED(rv)) return rv;

  // Get the list of certs //
  sec_rv = SEC_LookupCrls(CERT_GetDefaultCertDB(), &head, -1);
  if (sec_rv != SECSuccess) {
    goto loser;
  }

  if (head) {
    for (node=head->first; node != nsnull; node = node->next) {
      // Get the information we need here //

      // Name (this is the OU of the CA)
      caCert = CERT_FindCertByName(CERT_GetDefaultCertDB(), &(node->crl->crl.derName));
      if (caCert) {
        char *orgunit = CERT_GetOrgUnitName(&caCert->subject);
        if (orgunit) {
          name = NS_ConvertASCIItoUCS2(orgunit);
        }
      }

      // Last Update time
      sec_rv = DER_UTCTimeToTime(&tmpDate, &(node->crl->crl.lastUpdate));
      if (sec_rv == SECSuccess) {
        dateFormatter->FormatPRTime(nsnull, kDateFormatShort, kTimeFormatNone,
                              tmpDate, lastUpdate);
      }

      // Next update time
      sec_rv = DER_UTCTimeToTime(&tmpDate, &(node->crl->crl.nextUpdate));
      if (sec_rv == SECSuccess) {
        dateFormatter->FormatPRTime(nsnull, kDateFormatShort, kTimeFormatNone,
                              tmpDate, nextUpdate);
      }
      nsCOMPtr<nsICrlEntry> entry = new nsCrlEntry(name.get(), lastUpdate.get(), nextUpdate.get());
      crlsArray->AppendElement(entry);
    }
    PORT_FreeArena(head->arena, PR_FALSE);
  }

  *aCrls = crlsArray;
  NS_IF_ADDREF(*aCrls);
  return NS_OK;
loser:
  return NS_ERROR_FAILURE;;
}

/*
 * deletetCrl
 *
 * Delete a Crl entry from the cert db.
*/
NS_IMETHODIMP 
nsNSSCertificateDB::DeleteCrl(PRUint32 aCrlIndex)
{
  CERTSignedCrl *realCrl = nsnull;
  CERTCrlHeadNode *head = nsnull;
  CERTCrlNode *node = nsnull;
  SECStatus sec_rv;
  PRUint32 i;

  // Get the list of certs //
  sec_rv = SEC_LookupCrls(CERT_GetDefaultCertDB(), &head, -1);
  if (sec_rv != SECSuccess) {
    goto loser;
  }

  if (head) {
    for (i = 0, node=head->first; node != nsnull; i++, node = node->next) {
      if (i != aCrlIndex) {
        continue;
      }
      realCrl = SEC_FindCrlByName(CERT_GetDefaultCertDB(), &(node->crl->crl.derName), node->type);
      SEC_DeletePermCRL(realCrl);
      SEC_DestroyCrl(realCrl);
      SSL_ClearSessionCache();
    }
    PORT_FreeArena(head->arena, PR_FALSE);
  }
  return NS_OK;
loser:
  return NS_ERROR_FAILURE;;
}
