/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
#include "stdafx.h"
#include "guids.h"

// Class IDs
NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
NS_DEFINE_IID(kHTMLEditorCID, NS_HTMLEDITOR_CID);

// Interface IDs
NS_DEFINE_IID(kIBrowserWindowIID, NS_IBROWSER_WINDOW_IID);
NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);
NS_DEFINE_IID(kIDocumentViewerIID, NS_IDOCUMENT_VIEWER_IID);
NS_DEFINE_IID(kIDOMDocumentIID, NS_IDOMDOCUMENT_IID);
NS_DEFINE_IID(kIDOMNodeIID, NS_IDOMNODE_IID);
NS_DEFINE_IID(kIDOMElementIID, NS_IDOMELEMENT_IID);
NS_DEFINE_IID(kIWebShellContainerIID, NS_IWEB_SHELL_CONTAINER_IID);
NS_DEFINE_IID(kIStreamObserverIID, NS_ISTREAMOBSERVER_IID);
NS_DEFINE_IID(kIDocumentLoaderObserverIID, NS_IDOCUMENT_LOADER_OBSERVER_IID);
NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

#ifdef USE_PLUGIN
NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
NS_DEFINE_IID(kIPluginIID, NS_IPLUGIN_IID);
NS_DEFINE_IID(kIPluginInstanceIID, NS_IPLUGININSTANCE_IID);
#endif
