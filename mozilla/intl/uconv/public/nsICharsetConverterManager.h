/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Morten Nilsen <morten@nilsen.com>
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

#ifndef nsICharsetConverterManager_h___
#define nsICharsetConverterManager_h___

#include "nsString.h"
#include "nsError.h"
#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"

#define NS_ICHARSETCONVERTERMANAGER_IID \
  {0x3c1c0161, 0x9bd0, 0x11d3, { 0x9d, 0x9, 0x0, 0x50, 0x4, 0x0, 0x7, 0xb2}}

// XXX change to NS_CHARSETCONVERTERMANAGER_CID
#define NS_ICHARSETCONVERTERMANAGER_CID \
  {0x3c1c0163, 0x9bd0, 0x11d3, { 0x9d, 0x9, 0x0, 0x50, 0x4, 0x0, 0x7, 0xb2}}

// XXX change to NS_CHARSETCONVERTERMANAGER_PID
#define NS_CHARSETCONVERTERMANAGER_CONTRACTID "@mozilla.org/charset-converter-manager;1"

#define NS_REGISTRY_UCONV_BASE          "software/netscape/intl/uconv/"
// XXX change "xuconv" to "uconv" when the new enc&dec trees are in place
#define NS_DATA_BUNDLE_REGISTRY_KEY     "software/netscape/intl/xuconv/data/"
#define NS_TITLE_BUNDLE_REGISTRY_KEY    "software/netscape/intl/xuconv/titles/"

#define NS_ERROR_UCONV_NOCONV \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_UCONV, 0x01)

#define NS_ERROR_USING_FALLBACK_LOCALE \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_UCONV, 0x02)

#ifdef DEBUG
#define REGSELF_PRINTF(x,y)                                             \
  printf("RegSelf %s to %s converter complete\n",                       \
         x, y)                                                         
#else
#define REGSELF_PRINTF(x,y)
#endif

struct nsConverterRegistryInfo {
  const char *fromCharset;
  const char *toCharset;
  nsCID cid;
};

#define NS_CONVERTER_REGISTRY_START \
  static const nsConverterRegistryInfo gConverterRegistryInfo[] = {

#define NS_CONVERTER_REGISTRY_END \
  };


#define NS_IMPL_NSUCONVERTERREGSELF                                     \
static NS_IMETHODIMP                                                    \
nsUConverterRegSelf(nsIComponentManager *aCompMgr,                      \
                    nsIFile *aPath,                                     \
                    const char* registryLocation,                       \
                    const char* componentType,                          \
                    const nsModuleComponentInfo *info)                  \
{                                                                       \
  nsresult res = NS_OK;                                                 \
  nsRegistryKey key;                                                    \
  char buff[1024];                                                      \
  PRBool isOpen = PR_FALSE;                                             \
  nsCOMPtr<nsIRegistry> registry =                                      \
           do_GetService(NS_REGISTRY_CONTRACTID, &res);                 \
  if (NS_FAILED(res))                                                   \
    return res;                                                         \
  res = registry->IsOpen(&isOpen);                                      \
  if (NS_FAILED(res))                                                   \
    return res;                                                         \
  if(! isOpen) {                                                        \
    res = registry->OpenWellKnownRegistry(                              \
        nsIRegistry::ApplicationComponentRegistry);                     \
    if (NS_FAILED(res))                                                 \
      return res;                                                       \
  }                                                                     \
  int i;                                                                \
  for (i=0; i<sizeof(gConverterRegistryInfo)/sizeof(gConverterRegistryInfo[0]); i++) { \
      char * cid_string;                                                    \
      cid_string = gConverterRegistryInfo[i].cid.ToString();                \
      sprintf(buff, "%s/%s", "software/netscape/intl/uconv", cid_string);   \
      nsCRT::free(cid_string);                                              \
      res = registry->AddSubtree(nsIRegistry::Common, buff, &key);        \
      if (NS_FAILED(res))                                                   \
        continue;                                                           \
      res = registry->SetStringUTF8(key, "source",                        \
                                    gConverterRegistryInfo[i].fromCharset);\
      if (NS_FAILED(res))                                                   \
        continue;                                                           \
      res = registry->SetStringUTF8(key, "destination",                   \
                                    gConverterRegistryInfo[i].toCharset); \
      if (NS_FAILED(res))                                                   \
        continue;                                                           \
      REGSELF_PRINTF(gConverterRegistryInfo[i].fromCharset,                 \
                     gConverterRegistryInfo[i].toCharset);                  \
  }                                                                         \
  return res;                                                               \
} \
\
static NS_IMETHODIMP \
nsUConverterUnregSelf(nsIComponentManager *aCompMgr,                        \
                      nsIFile *aPath,                                       \
                      const char*,                                          \
                      const nsModuleComponentInfo *info)                    \
{ \
  return NS_OK; \
}


#define NS_UCONV_REG_UNREG(_From, _To, _CID )               \
  {                                                                         \
    _From,                                                                  \
    _To,                                                                    \
    _CID,                                                                   \
  },
  
/**
 * Interface for a Manager of Charset Converters.
 * 
 * This Manager's data is a cache of the stuff available directly through 
 * Registry and Extensible String Bundles. Plus a set of convenient APIs.
 * 
 * Note: The term "Charset" used in the classes, interfaces and file names 
 * should be read as "Coded Character Set". I am saying "charset" only for 
 * length considerations: it is a much shorter word. This convention is for 
 * source-code only, in the attached documents I will be either using the 
 * full expression or I'll specify a different convention.
 *
 * A DECODER converts from a random encoding into Unicode.
 * An ENCODER converts from Unicode into a random encoding.
 * All our data structures and APIs are divided like that.
 * However, when you have a charset data, you may have 3 cases:
 * a) the data is charset-dependet, but it is common for encoders and decoders
 * b) the data is different for the two of them, thus needing different APIs
 * and different "aProp" identifying it.
 * c) the data is relevant only for one: encoder or decoder; its nature making 
 * the distinction.
 *
 * @created         15/Nov/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsICharsetConverterManager : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ICHARSETCONVERTERMANAGER_IID)

  NS_IMETHOD GetUnicodeEncoder(const nsString * aDest, 
      nsIUnicodeEncoder ** aResult) = 0;
  NS_IMETHOD GetUnicodeDecoder(const nsString * aSrc, 
      nsIUnicodeDecoder ** aResult) = 0;

  NS_IMETHOD GetCharsetLangGroup(nsString * aCharset, nsIAtom ** aResult) = 0;
};

#endif /* nsICharsetConverterManager_h___ */
