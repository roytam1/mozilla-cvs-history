/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *  Terry Hayes <thayes@netscape.com>
 *  Javier Delgadillo <javi@netscape.com>
 */

/*
 * Dialog services for PIP.
 */
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIPrompt.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDialogParamBlock.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsIPref.h"
#include "nsIInterfaceRequestor.h"
#include "nsIX509Cert.h"
#include "nsIX509CertDB.h"
#include "nsILocaleService.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"
#include "nsIWindowWatcher.h"

#include "nsNSSDialogs.h"
#include "nsPKIParamBlock.h"

#define PIPSTRING_BUNDLE_URL "chrome://pippki/locale/pippki.properties"
#define STRING_BUNDLE_URL    "chrome://communicator/locale/security.properties"

#define ENTER_SITE_PREF      "security.warn_entering_secure"
#define WEAK_SITE_PREF       "security.warn_entering_weak"
#define LEAVE_SITE_PREF      "security.warn_leaving_secure"
#define MIXEDCONTENT_PREF    "security.warn_viewing_mixed"
#define INSECURE_SUBMIT_PREF "security.warn_submit_insecure"

static NS_DEFINE_CID(kCStringBundleServiceCID,  NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kDateTimeFormatCID, NS_DATETIMEFORMAT_CID);

/**
 * Common class that uses the window watcher service to open a
 * standard dialog, with or without a parent context. The params
 * parameter can be an nsISupportsArray so any number of additional
 * arguments can be used.
 */
class nsNSSDialogHelper
{
public:
  const static char *kDefaultOpenWindowParam;
  //The params is going to be either a nsIPKIParamBlock or
  //nsIDialogParamBlock
  static nsresult openDialog(
                  nsIDOMWindowInternal *window,
                  const char *url,
                  nsISupports *params);
};

const char* nsNSSDialogHelper::kDefaultOpenWindowParam = "centerscreen,chrome,modal,titlebar";

nsresult
nsNSSDialogHelper::openDialog(
    nsIDOMWindowInternal *window,
    const char *url,
    nsISupports *params)
{
  nsresult rv;
  NS_WITH_SERVICE(nsIWindowWatcher, windowWatcher, "@mozilla.org/embedcomp/window-watcher;1", &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDOMWindow> newWindow;
  rv = windowWatcher->OpenWindow(window,
                                 url,
                                 "_blank",
                                 nsNSSDialogHelper::kDefaultOpenWindowParam,
                                 params,
                                 getter_AddRefs(newWindow));
  return rv;
}

/* ==== */
static NS_DEFINE_CID(kPKIParamBlockCID, NS_PKIPARAMBLOCK_CID);

nsNSSDialogs::nsNSSDialogs()
{
  NS_INIT_ISUPPORTS();
}

nsNSSDialogs::~nsNSSDialogs()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS7(nsNSSDialogs, nsINSSDialogs, 
                                            nsITokenPasswordDialogs,
                                            nsISecurityWarningDialogs,
                                            nsIBadCertListener,
                                            nsICertificateDialogs,
                                            nsIClientAuthDialogs,
                                            nsITokenDialogs);

nsresult
nsNSSDialogs::Init()
{
  nsresult rv;

  mPref = do_GetService(kPrefCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIStringBundleService> service = do_GetService(kCStringBundleServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = service->CreateBundle(STRING_BUNDLE_URL,
                             getter_AddRefs(mStringBundle));
  if (NS_FAILED(rv)) return rv;

  rv = service->CreateBundle(PIPSTRING_BUNDLE_URL,
                             getter_AddRefs(mPIPStringBundle));
  return rv;
}

nsresult
nsNSSDialogs::SetPassword(nsIInterfaceRequestor *ctx,
                          const PRUnichar *tokenName, PRBool* _canceled)
{
  nsresult rv;

  *_canceled = PR_FALSE;

  // Get the parent window for the dialog
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);

  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1"));
  if (!block) return NS_ERROR_FAILURE;

  // void ChangePassword(in wstring tokenName, out int status);
  rv = block->SetString(1, tokenName);
  if (NS_FAILED(rv)) return rv;

  rv = nsNSSDialogHelper::openDialog(parent,
                                "chrome://pippki/content/changepassword.xul",
                                block);

  if (NS_FAILED(rv)) return rv;

  PRInt32 status;

  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;

  *_canceled = (status == 0)?PR_TRUE:PR_FALSE;

  return rv;
}

nsresult
nsNSSDialogs::GetPassword(nsIInterfaceRequestor *ctx,
                          const PRUnichar *tokenName, 
                          PRUnichar **_password,
                          PRBool* _canceled)
{
  nsresult rv;
  *_canceled = PR_FALSE;
  // Get the parent window for the dialog
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);
  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1"));
  if (!block) return NS_ERROR_FAILURE;
  // Set the token name in the window
  rv = block->SetString(1, tokenName);
  if (NS_FAILED(rv)) return rv;
  // open up the window
  rv = nsNSSDialogHelper::openDialog(parent,
                                     "chrome://pippki/content/getpassword.xul",
                                     block);
  if (NS_FAILED(rv)) return rv;
  // see if user canceled
  PRInt32 status;
  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;
  *_canceled = (status == 0) ? PR_TRUE : PR_FALSE;
  if (!*_canceled) {
    // retrieve the password
    rv = block->GetString(2, _password);
  }
  return rv;
}

/* boolean unknownIssuer (in nsITransportSecurityInfo socketInfo,
                          in nsIX509Cert cert, out addType); */
NS_IMETHODIMP
nsNSSDialogs::UnknownIssuer(nsITransportSecurityInfo *socketInfo,
                            nsIX509Cert *cert, PRInt16 *outAddType,
                            PRBool *_retval)
{
  nsresult rv;
  PRInt32 addType;
  
  *_retval = PR_FALSE;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);

  if (!block)
    return NS_ERROR_FAILURE;

  nsXPIDLString commonName;
  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull, 
                                     "chrome://pippki/content/newserver.xul",
                                     block);

  if (NS_FAILED(rv))
    return rv;

  PRInt32 status;
  nsCOMPtr<nsIDialogParamBlock> dialogBlock = do_QueryInterface(block);
  rv = dialogBlock->GetInt(1, &status);
  if (NS_FAILED(rv))
    return rv; 

  if (status == 0) {
    *_retval = PR_FALSE;
  } else {
    // The user wants to continue, let's figure out
    // what to do with this cert. 
    rv = dialogBlock->GetInt(2, &addType);
    switch (addType) {
      case 0:
        *outAddType = ADD_TRUSTED_PERMANENTLY;
        *_retval    = PR_TRUE;
        break;
      case 1:
        *outAddType = ADD_TRUSTED_FOR_SESSION;
        *_retval    = PR_TRUE;
        break;
      default:
        *outAddType = UNINIT_ADD_FLAG;
        *_retval    = PR_FALSE;
        break;
    } 
  }

  return NS_OK; 
}

/* boolean mismatchDomain (in nsITransportSecurityInfo socketInfo, 
                           in wstring targetURL, 
                           in nsIX509Cert cert); */

NS_IMETHODIMP 
nsNSSDialogs::MismatchDomain(nsITransportSecurityInfo *socketInfo, 
                             const PRUnichar *targetURL, 
                             nsIX509Cert *cert, PRBool *_retval) 
{
  nsresult rv;

  *_retval = PR_FALSE;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);

  if (!block)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDialogParamBlock> dialogBlock = do_QueryInterface(block);
  rv = dialogBlock->SetString(1, targetURL);
  if (NS_FAILED(rv))
    return rv;

  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                                 "chrome://pippki/content/domainMismatch.xul",
                                 block);
  if (NS_FAILED(rv))
    return rv;

  PRInt32 status;

  rv = dialogBlock->GetInt(1, &status);
  if (NS_FAILED(rv))
    return rv;

  *_retval = (status) ? PR_TRUE : PR_FALSE;

  return NS_OK;  
}

/* boolean certExpired (in nsITransportSecurityInfo socketInfo, 
                        in nsIX509Cert cert); */
NS_IMETHODIMP 
nsNSSDialogs::CertExpired(nsITransportSecurityInfo *socketInfo, 
                          nsIX509Cert *cert, PRBool *_retval)
{
  nsresult rv;
  PRTime now = PR_Now();
  PRTime notAfter, notBefore, timeToUse;
  nsCOMPtr<nsIX509CertValidity> validity;
  const char *key;

  *_retval = PR_FALSE;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);

  if (!block)
    return NS_ERROR_FAILURE; 
  rv = cert->GetValidity(getter_AddRefs(validity));
  if (NS_FAILED(rv))
    return rv;

  rv = validity->GetNotAfter(&notAfter);
  if (NS_FAILED(rv))
    return rv;

  rv = validity->GetNotBefore(&notBefore);
  if (NS_FAILED(rv))
    return rv;

  if (LL_CMP(now, >, notAfter)) {
    key       = "serverCertExpiredMsg1"; 
    timeToUse = notAfter; 
  } else {
    key = "serverCertNotYetValedMsg1";
    timeToUse = notBefore;
  }

  nsXPIDLString message1;
  PRUnichar *commonName=nsnull;
  nsString formattedDate;

  rv = cert->GetCommonName(&commonName);

  nsIDateTimeFormat *aDateTimeFormat;
  rv = nsComponentManager::CreateInstance(kDateTimeFormatCID, NULL,
                                          NS_GET_IID(nsIDateTimeFormat), 
                                          (void **) &aDateTimeFormat);

  aDateTimeFormat->FormatPRTime(nsnull, kDateFormatShort, 
                                kTimeFormatNoSeconds, timeToUse, 
                                formattedDate);
  PRUnichar *formattedDatePR = formattedDate.ToNewUnicode();
  const PRUnichar *formatStrings[2] = { commonName, formattedDatePR }; 
  nsString keyString = NS_ConvertASCIItoUCS2(key);
  mPIPStringBundle->FormatStringFromName(keyString.GetUnicode(), formatStrings, 
                                         2, getter_Copies(message1));
  
  Recycle(commonName);
  Recycle(formattedDatePR);

  nsCOMPtr<nsIDialogParamBlock> dialogBlock = do_QueryInterface(block);
  rv = dialogBlock->SetString(1,message1); 

  if (NS_FAILED(rv))
    return rv;

  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                             "chrome://pippki/content/serverCertExpired.xul",
                             block);

  PRInt32 status;
  rv = dialogBlock->GetInt(1, &status);
  if (NS_FAILED(rv))
    return rv; 

  *_retval = (status) ? PR_TRUE : PR_FALSE;
  
  return NS_OK;
}

nsresult
nsNSSDialogs::AlertEnteringSecure(nsIInterfaceRequestor *ctx)
{
  nsresult rv;

  rv = AlertDialog(ctx, ENTER_SITE_PREF, 
                   NS_LITERAL_STRING("EnterSiteMessage").get());

  return rv;
}

nsresult
nsNSSDialogs::AlertEnteringWeak(nsIInterfaceRequestor *ctx)
{
  nsresult rv;

  rv = AlertDialog(ctx, WEAK_SITE_PREF,
                   NS_LITERAL_STRING("WeakSiteMessage").get());

  return rv;
}

nsresult
nsNSSDialogs::AlertLeavingSecure(nsIInterfaceRequestor *ctx)
{
  nsresult rv;

  rv = AlertDialog(ctx, LEAVE_SITE_PREF, 
                   NS_LITERAL_STRING("LeaveSiteMessage").get());

  return rv;
}


nsresult
nsNSSDialogs::AlertMixedMode(nsIInterfaceRequestor *ctx)
{
  nsresult rv;

  rv = AlertDialog(ctx, MIXEDCONTENT_PREF, 
                   NS_LITERAL_STRING("MixedContentMessage").get());

  return rv;
}


nsresult
nsNSSDialogs::AlertDialog(nsIInterfaceRequestor *ctx, const char *prefName,
                          const PRUnichar *dialogMessageName)
{
  nsresult rv;

  // Get user's preference for this alert
  PRBool prefValue;
  rv = mPref->GetBoolPref(prefName, &prefValue);
  if (NS_FAILED(rv)) prefValue = PR_TRUE;

  // Stop if alert is not requested
  if (!prefValue) return NS_OK;

  // Get Prompt to use
  nsCOMPtr<nsIPrompt> prompt = do_GetInterface(ctx);
  if (!prompt) return NS_ERROR_FAILURE;

  // Get messages strings from localization file
  nsXPIDLString windowTitle, message, dontShowAgain;

  mStringBundle->GetStringFromName(NS_LITERAL_STRING("Title").get(),
                                   getter_Copies(windowTitle));
  mStringBundle->GetStringFromName(dialogMessageName,
                                   getter_Copies(message));
  mStringBundle->GetStringFromName(NS_LITERAL_STRING("DontShowAgain").get(),
                                   getter_Copies(dontShowAgain));
  if (!windowTitle || !message || !dontShowAgain) return NS_ERROR_FAILURE;
      
  rv = prompt->AlertCheck(windowTitle, message, dontShowAgain, &prefValue);
  if (NS_FAILED(rv)) return rv;
      
  if (!prefValue) {
    mPref->SetBoolPref(prefName, PR_FALSE);
  }

  return rv;
}

nsresult
nsNSSDialogs::ConfirmPostToInsecure(nsIInterfaceRequestor *ctx, PRBool* _result)
{
  nsresult rv;

  rv = ConfirmDialog(ctx, INSECURE_SUBMIT_PREF,
                     NS_LITERAL_STRING("PostToInsecureFromInsecure").get(),
                     _result);

  return rv;
}

nsresult
nsNSSDialogs::ConfirmPostToInsecureFromSecure(nsIInterfaceRequestor *ctx, PRBool* _result)
{
  nsresult rv;

  rv = ConfirmDialog(ctx, INSECURE_SUBMIT_PREF,
                     NS_LITERAL_STRING("PostToInsecure").get(), _result);

  return rv;
}

nsresult
nsNSSDialogs::ConfirmDialog(nsIInterfaceRequestor *ctx, const char *prefName,
                            const PRUnichar *messageName, PRBool* _result)
{
  nsresult rv;

  // Get user's preference for this alert
  PRBool prefValue;
  rv = mPref->GetBoolPref(prefName, &prefValue);
  if (NS_FAILED(rv)) prefValue = PR_TRUE;

  // Stop if confirm is not requested
  if (!prefValue) {
    *_result = PR_TRUE;
    return NS_OK;
  }

  // Get Prompt to use
  nsCOMPtr<nsIPrompt> prompt = do_GetInterface(ctx);
  if (!prompt) return NS_ERROR_FAILURE;

  // Get messages strings from localization file
  nsXPIDLString windowTitle, message, dontShowAgain;

  mStringBundle->GetStringFromName(NS_LITERAL_STRING("Title").get(),
                                   getter_Copies(windowTitle));
  mStringBundle->GetStringFromName(messageName,
                                   getter_Copies(message));
  mStringBundle->GetStringFromName(NS_LITERAL_STRING("DontShowAgain").get(),
                                   getter_Copies(dontShowAgain));
  if (!windowTitle || !message || !dontShowAgain) return NS_ERROR_FAILURE;
      
  rv = prompt->ConfirmCheck(windowTitle, message, dontShowAgain, &prefValue, _result);
  if (NS_FAILED(rv)) return rv;
      
  if (!prefValue) {
    mPref->SetBoolPref(prefName, PR_FALSE);
  }

  return rv;
}

/* void downloadCACert (in nsIInterfaceRequestor ctx,
                        in nsIX509Cert cert,
                        out trust,
                        out canceled); */
NS_IMETHODIMP 
nsNSSDialogs::DownloadCACert(nsIInterfaceRequestor *ctx, 
                             nsIX509Cert *cert,
                             PRUint32 *_trust,
                             PRBool *_canceled)
{
  nsresult rv;

  *_canceled = PR_FALSE;

  // Get the parent window for the dialog
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);

  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1"));
  if (!block) return NS_ERROR_FAILURE;

  nsXPIDLString commonName;
  rv = cert->GetCommonName(getter_Copies(commonName));
  if (NS_FAILED(rv))
    return rv;

  rv = block->SetString(1, commonName);
  if (NS_FAILED(rv)) return rv;

  rv = nsNSSDialogHelper::openDialog(parent, 
                                   "chrome://pippki/content/downloadcert.xul",
                                     block);
  if (NS_FAILED(rv)) return rv;

  PRInt32 status;
  PRInt32 ssl, email, objsign;

  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;
  rv = block->GetInt(2, &ssl);
  if (NS_FAILED(rv)) return rv;
  rv = block->GetInt(3, &email);
  if (NS_FAILED(rv)) return rv;
  rv = block->GetInt(4, &objsign);
  if (NS_FAILED(rv)) return rv;
 
  *_trust = nsIX509CertDB::UNTRUSTED;
  *_trust |= (ssl) ? nsIX509CertDB::TRUSTED_SSL : 0;
  *_trust |= (email) ? nsIX509CertDB::TRUSTED_EMAIL : 0;
  *_trust |= (objsign) ? nsIX509CertDB::TRUSTED_OBJSIGN : 0;

  *_canceled = (status == 0)?PR_TRUE:PR_FALSE;

  return rv;
}

NS_IMETHODIMP
nsNSSDialogs::ChooseCertificate(nsIInterfaceRequestor *ctx, const PRUnichar *cn, const PRUnichar *organization, const PRUnichar *issuer, const PRUnichar **certNickList, PRUint32 count, PRUnichar **certNick, PRBool *canceled) 
{
  nsresult rv;
  PRUint32 i;

  *canceled = PR_FALSE;

  // Get the parent window for the dialog
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);

  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1"));
  if (!block) return NS_ERROR_FAILURE;

  // void ChangePassword(in wstring tokenName, out int status);
  rv = block->SetString(1, cn);
  if (NS_FAILED(rv)) return rv;

  // void ChangePassword(in wstring tokenName, out int status);
  rv = block->SetString(2, organization);
  if (NS_FAILED(rv)) return rv;

  // void ChangePassword(in wstring tokenName, out int status);
  rv = block->SetString(3, issuer);
  if (NS_FAILED(rv)) return rv;

  for (i = 0; i < count; i++) {
	  rv = block->SetString(i+4, certNickList[i]);
	  if (NS_FAILED(rv)) return rv;
  }

  rv = block->SetInt(1, count);
  if (NS_FAILED(rv)) return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                                "chrome://pippki/content/clientauthask.xul",
                                block);
  if (NS_FAILED(rv)) return rv;

  PRInt32 status;

  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;

  *canceled = (status == 0)?PR_TRUE:PR_FALSE;
  if (!*canceled) {
    // retrieve the nickname
    rv = block->GetString(1, certNick);
  }
  return rv;
}

/*
 * void setPKCS12FilePassword(in nsIInterfaceRequestor ctx, 
 *                            out wstring password,
 *                            out boolean canceled);
 */
NS_IMETHODIMP 
nsNSSDialogs::SetPKCS12FilePassword(nsIInterfaceRequestor *ctx, 
                                    PRUnichar **_password,
                                    PRBool *_canceled)
{
  nsresult rv;
  *_canceled = PR_FALSE;
  // Get the parent window for the dialog
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);
  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1"));
  if (!block) return NS_ERROR_FAILURE;
  // open up the window
  rv = nsNSSDialogHelper::openDialog(parent,
                                  "chrome://pippki/content/setp12password.xul",
                                  block);
  if (NS_FAILED(rv)) return rv;
  // see if user canceled
  PRInt32 status;
  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;
  *_canceled = (status == 0) ? PR_TRUE : PR_FALSE;
  if (!*_canceled) {
    // retrieve the password
    rv = block->GetString(2, _password);
  }
  return rv;
}

/*
 *  void getPKCS12FilePassword(in nsIInterfaceRequestor ctx, 
 *                             out wstring password,
 *                             out boolean canceled);
 */
NS_IMETHODIMP 
nsNSSDialogs::GetPKCS12FilePassword(nsIInterfaceRequestor *ctx, 
                                    PRUnichar **_password,
                                    PRBool *_canceled)
{
  nsresult rv;
  *_canceled = PR_FALSE;
  // Get the parent window for the dialog
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);
  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1"));
  if (!block) return NS_ERROR_FAILURE;
  // open up the window
  rv = nsNSSDialogHelper::openDialog(parent,
                                  "chrome://pippki/content/getp12password.xul",
                                  block);
  if (NS_FAILED(rv)) return rv;
  // see if user canceled
  PRInt32 status;
  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;
  *_canceled = (status == 0) ? PR_TRUE : PR_FALSE;
  if (!*_canceled) {
    // retrieve the password
    rv = block->GetString(2, _password);
  }
  return rv;
}

/* void viewCert (in nsIX509Cert cert); */
NS_IMETHODIMP 
nsNSSDialogs::ViewCert(nsIX509Cert *cert)
{
  nsresult rv;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);
  if (!block)
    return NS_ERROR_FAILURE;

  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                                     "chrome://pippki/content/certViewer.xul",
                                     block);
  return rv;
}

NS_IMETHODIMP
nsNSSDialogs::ChooseToken(nsIInterfaceRequestor *aCtx, const PRUnichar **aTokenList, PRUint32 aCount, PRUnichar **aTokenChosen, PRBool *aCanceled) {
  nsresult rv;
  PRUint32 i;

  *aCanceled = PR_FALSE;

  // Get the parent window for the dialog
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(aCtx);

  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1"));
  if (!block) return NS_ERROR_FAILURE;

  for (i = 0; i < aCount; i++) {
	  rv = block->SetString(i+1, aTokenList[i]);
	  if (NS_FAILED(rv)) return rv;
  }

  rv = block->SetInt(1, aCount);
  if (NS_FAILED(rv)) return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                                "chrome://pippki/content/choosetoken.xul",
                                block);
  if (NS_FAILED(rv)) return rv;

  PRInt32 status;

  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;

  *aCanceled = (status == 0)?PR_TRUE:PR_FALSE;
  if (!*aCanceled) {
    // retrieve the nickname
    rv = block->GetString(1, aTokenChosen);
  }
  return rv;
}

