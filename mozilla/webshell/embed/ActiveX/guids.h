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
#ifndef GUIDS_H
#define GUIDS_H

#define NS_EXTERN_IID(_name) \
	extern const nsIID _name;

// Class IDs
NS_EXTERN_IID(kEventQueueServiceCID);
NS_EXTERN_IID(kHTMLEditorCID);

// Interface IDs
NS_EXTERN_IID(kIBrowserWindowIID);
NS_EXTERN_IID(kIEventQueueServiceIID);
NS_EXTERN_IID(kIDocumentViewerIID);
NS_EXTERN_IID(kIDOMDocumentIID);
NS_EXTERN_IID(kIDOMNodeIID);
NS_EXTERN_IID(kIDOMElementIID);
NS_EXTERN_IID(kIWebShellContainerIID);
NS_EXTERN_IID(kIStreamObserverIID);
NS_EXTERN_IID(kIDocumentLoaderObserverIID);
NS_EXTERN_IID(kISupportsIID);

#ifdef USE_PLUGIN
NS_EXTERN_IID(kIFactoryIID);
NS_EXTERN_IID(kIPluginIID);
NS_EXTERN_IID(kIPluginInstanceIID);
#endif

#endif