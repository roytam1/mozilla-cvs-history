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
 * Author:
 *   Adam Lock <adamlock@netscape.com>
 *
 * Contributor(s): 
 */

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__1339B542_3453_11D2_93B9_000000000000__INCLUDED_)
#define AFX_STDAFX_H__1339B542_3453_11D2_93B9_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "jstypes.h"
#include "prtypes.h"

// Mozilla headers

#include "xp_core.h"
#include "jscompat.h"

#include "prthread.h"
#include "prprf.h"
#include "plevent.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsWidgetsCID.h"
#include "nsGfxCIID.h"
#include "nsViewsCID.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsICookieService.h"

#include "nsIHTTPChannel.h"

#include "nsIPref.h"
#include "nsIURL.h"
#include "nsIBaseWindow.h"
#include "nsIWebBrowser.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserSetup.h"
#include "nsIWebNavigation.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsIDocumentLoader.h"
#include "nsIContentViewer.h"
#include "nsIContentViewerEdit.h"
#include "nsIInterfaceRequestor.h"
#include "nsIPresShell.h"
#include "nsCOMPtr.h"
#include "nsISelection.h"
#include "nsIPresContext.h"
#include "nsIPrompt.h"

#include "nsEditorCID.h"
#include "nsIEditor.h"
#include "nsIHtmlEditor.h"

#include "nsIDocument.h"
#include "nsIDocumentObserver.h"
#include "nsIStreamListener.h"
#include "nsUnitConversion.h"
#include "nsVoidArray.h"
#include "nsCRT.h"

#include "nsIDocumentViewer.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMEventReceiver.h"

#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED
#define _ATL_STATIC_REGISTRY
#define _ATL_DEBUG_INTERFACES

// ATL headers
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <docobj.h>
#include <winsock2.h>

// STL headers
#include <vector>
#include <list>
#include <string>

// New winsock2.h doesn't define this anymore
typedef long int32;

// Turn off warnings about debug symbols for templates being too long
#pragma warning(disable : 4786)

// Mozilla control headers
#include "resource.h"

#include "ActiveXTypes.h"
#include "BrowserDiagnostics.h"
#include "IOleCommandTargetImpl.h"
#include "DHTMLCmdIds.h"

#include "MozillaControl.h"
#include "PropertyList.h"
#include "PropertyBag.h"
#include "ItemContainer.h"
#include "ControlSite.h"
#include "ControlSiteIPFrame.h"

#include "IEHtmlDocument.h"
#include "CPMozillaControl.h"
#include "MozillaBrowser.h"
#include "WebBrowserContainer.h"
#include "DropTarget.h"
#include "guids.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__1339B542_3453_11D2_93B9_000000000000__INCLUDED)
