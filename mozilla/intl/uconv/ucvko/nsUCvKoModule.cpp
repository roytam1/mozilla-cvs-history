/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Roy Yokoyama <yokoyama@netscape.com>
 */

#define NS_IMPL_IDS

#include "nspr.h"
#include "nsString.h"
#include "pratom.h"
#include "nsCOMPtr.h"
#include "nsIFactory.h"
#include "nsIRegistry.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIModule.h"
#include "nsUCvKOCID.h"
#include "nsUCvKODll.h"

#include "nsEUCKRToUnicode.h"
#include "nsUnicodeToEUCKR.h"
#include "nsUnicodeToKSC5601.h"
#include "nsUnicodeToX11Johab.h"

//----------------------------------------------------------------------------
// Global functions and data [declaration]

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

#define DECODER_NAME_BASE "Unicode Decoder-"
#define ENCODER_NAME_BASE "Unicode Encoder-"

PRInt32 g_InstanceCount = 0;
PRInt32 g_LockCount = 0;

PRUint16 g_utKSC5601Mapping[] = {
#include "u20kscgl.ut"
};

PRUint16 g_ufKSC5601Mapping[] = {
#include "u20kscgl.uf"
};

PRUint16 g_AsciiMapping[] = {
  0x0001, 0x0004, 0x0005, 0x0008, 0x0000, 0x0000, 0x007F, 0x0000
};
PRUint16 g_HangulNullMapping[] ={
  0x0001, 0x0004, 0x0005, 0x0008, 0x0000, 0xAC00, 0xD7A3, 0xAC00
};

NS_IMPL_NSUCONVERTERREGSELF

NS_UCONV_REG_UNREG(nsEUCKRToUnicode, "EUC-KR", "Unicode" , NS_EUCKRTOUNICODE_CID);
NS_UCONV_REG_UNREG(nsUnicodeToEUCKR, "Unicode", "EUC-KR",  NS_UNICODETOEUCKR_CID);
NS_UCONV_REG_UNREG(nsUnicodeToKSC5601, "Unicode", "ks_c_5601-1987",  NS_UNICODETOKSC5601_CID);
NS_UCONV_REG_UNREG(nsUnicodeToX11Johab, "Unicode", "x-x11johab",  NS_UNICODETOX11JOHAB_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsEUCKRToUnicode);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToEUCKR);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToKSC5601);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToX11Johab);

static nsModuleComponentInfo components[] = 
{
  { 
    DECODER_NAME_BASE "EUC-KR" , NS_EUCKRTOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "EUC-KR",
    nsEUCKRToUnicodeConstructor ,
    nsEUCKRToUnicodeRegSelf , nsEUCKRToUnicodeUnRegSelf 
  },
  { 
    ENCODER_NAME_BASE "EUC-KR" , NS_UNICODETOEUCKR_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "EUC-KR",
    nsUnicodeToEUCKRConstructor, 
    nsUnicodeToEUCKRRegSelf, nsUnicodeToEUCKRUnRegSelf
  },
  { 
    ENCODER_NAME_BASE "ks_c_5601-1987" , NS_UNICODETOKSC5601_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "ks_c_5601-1987",
    nsUnicodeToKSC5601Constructor,
    nsUnicodeToKSC5601RegSelf, nsUnicodeToKSC5601UnRegSelf
  },
  { 
    ENCODER_NAME_BASE "x-x11johab" , NS_UNICODETOX11JOHAB_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-x11johab",
    nsUnicodeToX11JohabConstructor,
    nsUnicodeToX11JohabRegSelf, nsUnicodeToX11JohabUnRegSelf
  }
};

NS_IMPL_NSGETMODULE("nsUCvKoModule", components);

