/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsJPGDecoder.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"

static NS_DEFINE_CID(kJPGDecoderCID, NS_JPGDECODER_CID);

static nsModuleComponentInfo components[] =
{
  { "JPEG Decoder",
     NS_JPGDECODER_CID,
     "@mozilla.org/image/decoder;1?type=image/jpeg",
     JPGDecoder::Create }
};

NS_IMPL_NSGETMODULE(nsJPGModule, components)
