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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Roy Yokoyama <yokoyama@netscape.com>
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
#include "nsCRT.h"

#include "nsEUCKRToUnicode.h"
#include "nsUnicodeToEUCKR.h"
#include "nsUnicodeToKSC5601.h"
#include "nsUnicodeToX11Johab.h"
#include "nsJohabToUnicode.h"
#include "nsUnicodeToJohab.h"
#include "nsUnicodeToJohabNoAscii.h"
#include "nsCP949ToUnicode.h"
#include "nsUnicodeToCP949.h"
#include "nsISO2022KRToUnicode.h"

//----------------------------------------------------------------------------
// Global functions and data [declaration]

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

#define DECODER_NAME_BASE "Unicode Decoder-"
#define ENCODER_NAME_BASE "Unicode Encoder-"

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

NS_CONVERTER_REGISTRY_START
NS_UCONV_REG_UNREG("EUC-KR", "Unicode" , NS_EUCKRTOUNICODE_CID)
NS_UCONV_REG_UNREG("Unicode", "EUC-KR",  NS_UNICODETOEUCKR_CID)
NS_UCONV_REG_UNREG("Unicode", "ks_c_5601-1987",  NS_UNICODETOKSC5601_CID)
NS_UCONV_REG_UNREG("Unicode", "x-x11johab",  NS_UNICODETOX11JOHAB_CID)
NS_UCONV_REG_UNREG("x-johab", "Unicode" , NS_JOHABTOUNICODE_CID)
NS_UCONV_REG_UNREG("Unicode", "x-johab",  NS_UNICODETOJOHAB_CID)
NS_UCONV_REG_UNREG("Unicode", "x-johab-noascii",  NS_UNICODETOJOHABNOASCII_CID)
NS_UCONV_REG_UNREG("x-windows-949", "Unicode" , NS_CP949TOUNICODE_CID)
NS_UCONV_REG_UNREG("Unicode", "x-windows-949",  NS_UNICODETOCP949_CID)
NS_UCONV_REG_UNREG("ISO-2022-KR", "Unicode" , NS_ISO2022KRTOUNICODE_CID)
NS_CONVERTER_REGISTRY_END

NS_IMPL_NSUCONVERTERREGSELF

NS_GENERIC_FACTORY_CONSTRUCTOR(nsEUCKRToUnicode);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToEUCKR);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToKSC5601);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToX11Johab);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJohabToUnicode);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToJohab);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToJohabNoAscii);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCP949ToUnicode);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeToCP949);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsISO2022KRToUnicode);

static const nsModuleComponentInfo components[] = 
{
  { 
    DECODER_NAME_BASE "EUC-KR" , NS_EUCKRTOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "EUC-KR",
    nsEUCKRToUnicodeConstructor ,
    // global converter registration
    nsUConverterRegSelf, nsUConverterUnregSelf,
  },
  { 
    ENCODER_NAME_BASE "EUC-KR" , NS_UNICODETOEUCKR_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "EUC-KR",
    nsUnicodeToEUCKRConstructor, 
  },
  { 
    ENCODER_NAME_BASE "ks_c_5601-1987" , NS_UNICODETOKSC5601_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "ks_c_5601-1987",
    nsUnicodeToKSC5601Constructor,
  },
  { 
    ENCODER_NAME_BASE "x-x11johab" , NS_UNICODETOX11JOHAB_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-x11johab",
    nsUnicodeToX11JohabConstructor,
  },
  { 
    DECODER_NAME_BASE "x-johab" , NS_JOHABTOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "x-johab",
    nsJohabToUnicodeConstructor ,
  },
  { 
    ENCODER_NAME_BASE "x-johab" , NS_UNICODETOJOHAB_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-johab",
    nsUnicodeToJohabConstructor,
  },
  { 
    ENCODER_NAME_BASE "x-johab-noascii", NS_UNICODETOJOHABNOASCII_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-johab-noascii",
    nsUnicodeToJohabNoAsciiConstructor,
  },
  { 
    DECODER_NAME_BASE "x-windows-949" , NS_CP949TOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "x-windows-949",
    nsCP949ToUnicodeConstructor ,
  },
  { 
    ENCODER_NAME_BASE "x-windows-949" , NS_UNICODETOCP949_CID, 
    NS_UNICODEENCODER_CONTRACTID_BASE "x-windows-949",
    nsUnicodeToCP949Constructor,
  },
  { 
    DECODER_NAME_BASE "ISO-2022-KR" , NS_ISO2022KRTOUNICODE_CID, 
    NS_UNICODEDECODER_CONTRACTID_BASE "ISO-2022-KR",
    nsISO2022KRToUnicodeConstructor ,
  }
};

NS_IMPL_NSGETMODULE(nsUCvKoModule, components);

