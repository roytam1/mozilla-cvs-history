/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsString.h"
#include "nsIUnicodeEncoder.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetConverterManager2.h"
#include "nsReadableUtils.h"
#include "nsITextToSubURI.h"
#include "nsIServiceManager.h"
#include "nsUConvDll.h"
#include "nsEscape.h"
#include "prmem.h"
#include "nsTextToSubURI.h"
#include "nsCRT.h"

static NS_DEFINE_CID(kITextToSubURIIID, NS_ITEXTTOSUBURI_IID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

nsTextToSubURI::nsTextToSubURI()
{
    NS_INIT_ISUPPORTS();
}
nsTextToSubURI::~nsTextToSubURI()
{
}

NS_IMPL_ISUPPORTS1(nsTextToSubURI, nsITextToSubURI)

NS_IMETHODIMP  nsTextToSubURI::ConvertAndEscape(
  const char *charset, const PRUnichar *text, char **_retval) 
{
  if(nsnull == _retval)
    return NS_ERROR_NULL_POINTER;
  *_retval = nsnull;
  nsAutoString charsetStr; charsetStr.AssignWithConversion(charset);
  nsIUnicodeEncoder *encoder = nsnull;
  nsresult rv = NS_OK;
  
  // Get Charset, get the encoder.
  nsICharsetConverterManager * ccm = nsnull;
  rv = nsServiceManager::GetService(kCharsetConverterManagerCID ,
                                    NS_GET_IID(nsICharsetConverterManager),
                                    (nsISupports**)&ccm);
  if(NS_SUCCEEDED(rv) && (nsnull != ccm)) {
     rv = ccm->GetUnicodeEncoder(&charsetStr, &encoder);
     nsServiceManager::ReleaseService( kCharsetConverterManagerCID, ccm);
     if (NS_SUCCEEDED(rv)) {
       rv = encoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, (PRUnichar)'?');
       if(NS_SUCCEEDED(rv))
       {
          char buf[256];
          char *pBuf = buf;
          PRInt32 ulen = nsCRT::strlen(text);
          PRInt32 outlen = 0;
          if(NS_SUCCEEDED(rv = encoder->GetMaxLength(text, ulen, &outlen))) 
          {
             if(outlen >= 256) {
                pBuf = (char*)PR_Malloc(outlen+1);
             }
             if(nsnull == pBuf) {
                outlen = 255;
                pBuf = buf;
             }
             if(NS_SUCCEEDED(rv = encoder->Convert(text,&ulen, pBuf, &outlen))) {
                pBuf[outlen] = '\0';
                *_retval = nsEscape(pBuf, url_XPAlphas);
                if(nsnull == *_retval)
                  rv = NS_ERROR_OUT_OF_MEMORY;
             }
          }
          if(pBuf != buf)
             PR_Free(pBuf);
       }
       NS_IF_RELEASE(encoder);
     }
  }
  
  return rv;
}

NS_IMETHODIMP  nsTextToSubURI::UnEscapeAndConvert(
  const char *charset, const char *text, PRUnichar **_retval) 
{
  if(nsnull == _retval)
    return NS_ERROR_NULL_POINTER;
  *_retval = nsnull;
  nsresult rv = NS_OK;
  
  // unescape the string, unescape changes the input
  char *unescaped = nsCRT::strdup((char *) text);
  if (nsnull == unescaped)
    return NS_ERROR_OUT_OF_MEMORY;
  unescaped = nsUnescape(unescaped);
  NS_ASSERTION(unescaped, "nsUnescape returned null");

  // Convert from the charset to unicode
  nsCOMPtr<nsICharsetConverterManager> ccm = 
           do_GetService(kCharsetConverterManagerCID, &rv); 
  if (NS_SUCCEEDED(rv)) {
    nsAutoString charsetStr; charsetStr.AssignWithConversion(charset);
    nsIUnicodeDecoder *decoder;
    rv = ccm->GetUnicodeDecoder(&charsetStr, &decoder);
    if (NS_SUCCEEDED(rv)) {
      PRUnichar *pBuf = nsnull;
      PRInt32 len = strlen(unescaped);
      PRInt32 outlen = 0;
      if (NS_SUCCEEDED(rv = decoder->GetMaxLength(unescaped, len, &outlen))) {
        pBuf = (PRUnichar *) PR_Malloc((outlen+1)*sizeof(PRUnichar*));
        if (nsnull == pBuf)
          rv = NS_ERROR_OUT_OF_MEMORY;
        else {
          if (NS_SUCCEEDED(rv = decoder->Convert(unescaped, &len, pBuf, &outlen))) {
            pBuf[outlen] = 0;
            *_retval = pBuf;
          }
        }
      }
      NS_IF_RELEASE(decoder);
    }
  }
  PR_FREEIF(unescaped);

  return rv;
}

static PRBool statefulCharset(const char *charset)
{
  if (!nsCRT::strncasecmp(charset, "ISO-2022-", sizeof("ISO-2022-")-1) ||
      !nsCRT::strcasecmp(charset, "UTF-7") ||
      !nsCRT::strcasecmp(charset, "HZ-GB-2312"))
    return PR_TRUE;

  return PR_FALSE;
}

nsresult nsTextToSubURI::convertURItoUnicode(const nsAFlatCString &aCharset,
                                             const nsAFlatCString &aURI, 
                                             PRBool aIRI, 
                                             nsAString &_retval)
{
  nsresult rv = NS_OK;

  // check for 7bit encoding the data may not be ASCII after we decode
  PRBool isStatefulCharset = statefulCharset(aCharset.get());

  if (!isStatefulCharset && IsASCII(aURI)) {
    _retval.Assign(NS_ConvertASCIItoUCS2(aURI));
    return rv;
  }

  if (!isStatefulCharset && aIRI) {
    NS_ConvertUTF8toUCS2 ucs2(aURI);
    if (aURI.Equals(NS_ConvertUCS2toUTF8(ucs2))) {
      _retval.Assign(ucs2);
      return rv;
    }
  }

  nsCOMPtr<nsICharsetConverterManager2> charsetConverterManager;

  charsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAtom> charsetAtom;
  rv = charsetConverterManager->GetCharsetAtom2(aCharset.get(), getter_AddRefs(charsetAtom));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;
  rv = charsetConverterManager->GetUnicodeDecoder(charsetAtom, 
                                                  getter_AddRefs(unicodeDecoder));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 srcLen = aURI.Length();
  PRInt32 dstLen;
  rv = unicodeDecoder->GetMaxLength(aURI.get(), srcLen, &dstLen);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar *ustr = (PRUnichar *) nsMemory::Alloc(dstLen * sizeof(PRUnichar));
  NS_ENSURE_TRUE(ustr, NS_ERROR_OUT_OF_MEMORY);

  rv = unicodeDecoder->Convert(aURI.get(), &srcLen, ustr, &dstLen);

  if (NS_SUCCEEDED(rv))
    _retval.Assign(ustr, dstLen);
  
  nsMemory::Free(ustr);

  return rv;
}

NS_IMETHODIMP  nsTextToSubURI::UnEscapeURIForUI(const nsACString & aCharset, 
                                                const nsACString &aURIFragment, 
                                                nsAString &_retval)
{
  nsCAutoString unescapedSpec(aURIFragment);
  NS_UnescapeURL(unescapedSpec);

  return convertURItoUnicode(PromiseFlatCString(aCharset), unescapedSpec, PR_TRUE, _retval);
}

NS_IMETHODIMP  nsTextToSubURI::UnEscapeNonAsciiURI(const nsACString & aCharset, 
                                                   const nsACString &aURIFragment, 
                                                   nsAString &_retval)
{
  nsCAutoString unescapedSpec;
  NS_UnescapeURL(PromiseFlatCString(aURIFragment),
                 esc_AlwaysCopy | esc_OnlyNonASCII, unescapedSpec);

  return convertURItoUnicode(PromiseFlatCString(aCharset), unescapedSpec, PR_TRUE, _retval);
}

//----------------------------------------------------------------------
