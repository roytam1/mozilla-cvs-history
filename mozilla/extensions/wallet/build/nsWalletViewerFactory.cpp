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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsWalletPreview.h"
#include "nsSignonViewer.h"
#include "nsCookieViewer.h"
#include "nsWalletEditor.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(WalletPreviewImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(SignonViewerImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(CookieViewerImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(WalletEditorImpl)

// The list of components we register
static nsModuleComponentInfo components[] = {
    { "WalletPreview World Component", NS_WALLETPREVIEW_CID,
      "component://netscape/walletpreview/walletpreview-world", WalletPreviewImplConstructor },
    { "SignonViewer World Component", NS_SIGNONVIEWER_CID,
      "component://netscape/signonviewer/signonviewer-world", SignonViewerImplConstructor },
    { "CookieViewer World Component", NS_COOKIEVIEWER_CID,
      "component://netscape/cookieviewer/cookieviewer-world", CookieViewerImplConstructor },
    { "WalletEditor World Component", NS_WALLETEDITOR_CID,
      "component://netscape/walleteditor/walleteditor-world", WalletEditorImplConstructor },
};

NS_IMPL_NSGETMODULE("nsWalletViewerModule", components)

