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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Original Author(s):
 *   Chris Waterson <waterson@netscape.com>
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 03/27/2000   IBM Corp.       Added PR_CALLBACK for Optlink
 *                               use in OS2
 */

/*

  Implementation for a XUL content element.

  TO DO

  1. Make sure to compute class information: GetClasses(), HasClass().

 */

#include "jsapi.h"      // for JS_AddNamedRoot and JS_RemoveRootRT
#include "nsCOMPtr.h"
#include "nsDOMCID.h"
#include "nsDOMError.h"
#include "nsDOMEvent.h"
#include "nsForwardReference.h"
#include "nsHTMLValue.h"
#include "nsHashtable.h"
#include "nsIAtom.h"
#include "nsIDOMAttr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIEventStateManager.h"
#include "nsIHTMLContentContainer.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIJSScriptObject.h"
#include "nsIMutableStyleContext.h"
#include "nsINameSpace.h"
#include "nsINameSpaceManager.h"
#include "nsIPresShell.h"
#include "nsIPrincipal.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFContentModelBuilder.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIServiceManager.h"
#include "nsIStyleContext.h"
#include "nsIStyleRule.h"
#include "nsIStyleSheet.h"
#include "nsIStyledContent.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsIXMLContent.h"
#include "nsIXULContent.h"
#include "nsIXULContentUtils.h"
#include "nsIXULDocument.h"
#include "nsIXULPopupListener.h"
#include "nsIXULPrototypeDocument.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIXBLService.h"
#include "nsLayoutCID.h"
#include "nsRDFCID.h"
#include "nsRDFDOMNodeList.h"
#include "nsStyleConsts.h"
#include "nsXPIDLString.h"
#include "nsXULAttributes.h"
#include "nsXULControllers.h"
#include "nsXULTreeElement.h"
#include "nsXULMenuListElement.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsXULDocument.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMCSSStyleDeclaration.h"

// Used for the temporary DOM Level2 hack
#include "nsIPref.h"
static PRBool kStrictDOMLevel2;

#include "prlog.h"
#include "rdf.h"
#include "rdfutil.h"

#include "nsIControllers.h"

// The XUL interfaces implemented by the RDF content node.
#include "nsIDOMXULElement.h"

// The XUL doc interface
#include "nsIDOMXULDocument.h"

#include "nsISizeOfHandler.h"

class nsIWebShell;

// XXX This is sure to change. Copied from mozilla/layout/xul/content/src/nsXULAtoms.cpp
#define XUL_NAMESPACE_URI "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
static const char kXULNameSpaceURI[] = XUL_NAMESPACE_URI;
static const char kRDFNameSpaceURI[] = RDF_NAMESPACE_URI;
// End of XUL interface includes

//----------------------------------------------------------------------

static NS_DEFINE_IID(kIContentIID,                NS_ICONTENT_IID);
static NS_DEFINE_IID(kIDOMElementIID,             NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIDOMEventReceiverIID,       NS_IDOMEVENTRECEIVER_IID);
static NS_DEFINE_IID(kIDOMNodeIID,                NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDOMNodeListIID,            NS_IDOMNODELIST_IID);
static NS_DEFINE_IID(kIDocumentIID,               NS_IDOCUMENT_IID);
static NS_DEFINE_IID(kIEventListenerManagerIID,   NS_IEVENTLISTENERMANAGER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID,         NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kINameSpaceManagerIID,       NS_INAMESPACEMANAGER_IID);
static NS_DEFINE_IID(kIPrivateDOMEventIID,        NS_IPRIVATEDOMEVENT_IID);
static NS_DEFINE_IID(kIRDFCompositeDataSourceIID, NS_IRDFCOMPOSITEDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFDocumentIID,            NS_IRDFDOCUMENT_IID);
static NS_DEFINE_IID(kIRDFServiceIID,             NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kIScriptObjectOwnerIID,      NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kISupportsIID,               NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIXMLContentIID,             NS_IXMLCONTENT_IID);

static NS_DEFINE_CID(kEventListenerManagerCID,    NS_EVENTLISTENERMANAGER_CID);
static NS_DEFINE_IID(kIDOMEventTargetIID,         NS_IDOMEVENTTARGET_IID);
static NS_DEFINE_CID(kNameSpaceManagerCID,        NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_CID(kRDFServiceCID,              NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kXULContentUtilsCID,         NS_XULCONTENTUTILS_CID);

static NS_DEFINE_IID(kIXULPopupListenerIID,       NS_IXULPOPUPLISTENER_IID);
static NS_DEFINE_CID(kXULPopupListenerCID,        NS_XULPOPUPLISTENER_CID);

//----------------------------------------------------------------------

static JSRuntime* gScriptRuntime;
static PRInt32 gScriptRuntimeRefcnt;

static nsresult
AddJSGCRoot(JSContext* cx, void* aScriptObjectRef, const char* aName)
{
    PRBool ok;
    ok = JS_AddNamedRoot(cx, aScriptObjectRef, aName);
    if (! ok) return NS_ERROR_OUT_OF_MEMORY;

    if (gScriptRuntimeRefcnt++ == 0) {
        gScriptRuntime = JS_GetRuntime(cx);
    }

    return NS_OK;
}

static nsresult
RemoveJSGCRoot(void* aScriptObjectRef)
{
    JS_RemoveRootRT(gScriptRuntime, aScriptObjectRef);

    if (--gScriptRuntimeRefcnt == 0) {
        gScriptRuntime = nsnull;
    }

    return NS_OK;
}

//----------------------------------------------------------------------

struct XULBroadcastListener
{
    nsVoidArray* mAttributeList;
    nsIDOMElement* mListener;

    XULBroadcastListener(const nsString& aAttribute, nsIDOMElement* aListener)
    : mAttributeList(nsnull)
    {
        mListener = aListener; // WEAK REFERENCE
        if (!aAttribute.EqualsWithConversion("*")) {
            mAttributeList = new nsVoidArray();
            mAttributeList->AppendElement((void*)(new nsString(aAttribute)));
        }

        // For the "*" case we leave the attribute list nulled out, and this means
        // we're observing all attribute changes.
    }

    ~XULBroadcastListener()
    {
        // Release all the attribute strings.
        if (mAttributeList) {
            PRInt32 count = mAttributeList->Count();
            for (PRInt32 i = 0; i < count; i++) {
                nsString* str = (nsString*)(mAttributeList->ElementAt(i));
                delete str;
            }

            delete mAttributeList;
        }
    }

    PRBool IsEmpty()
    {
        if (ObservingEverything())
            return PR_FALSE;

        PRInt32 count = mAttributeList->Count();
        return (count == 0);
    }

    void RemoveAttribute(const nsString& aString)
    {
        if (ObservingEverything())
            return;

        if (mAttributeList) {
            PRInt32 count = mAttributeList->Count();
            for (PRInt32 i = 0; i < count; i++) {
                nsString* str = (nsString*)(mAttributeList->ElementAt(i));
                if (*str == aString) {
                    mAttributeList->RemoveElementAt(i);
                    delete str;
                    break;
                }
            }
        }
    }

    PRBool ObservingEverything()
    {
        return (mAttributeList == nsnull);
    }

    PRBool ObservingAttribute(const nsString& aString)
    {
        if (ObservingEverything())
            return PR_TRUE;

        if (mAttributeList) {
            PRInt32 count = mAttributeList->Count();
            for (PRInt32 i = 0; i < count; i++) {
                nsString* str = (nsString*)(mAttributeList->ElementAt(i));
                if (*str == aString)
                    return PR_TRUE;
            }
        }

        return PR_FALSE;
    }
};

//----------------------------------------------------------------------

nsrefcnt             nsXULElement::gRefCnt;
nsIRDFService*       nsXULElement::gRDFService;
nsINameSpaceManager* nsXULElement::gNameSpaceManager;
nsIXULContentUtils*  nsXULElement::gXULUtils;
PRInt32              nsXULElement::kNameSpaceID_RDF;
PRInt32              nsXULElement::kNameSpaceID_XUL;

nsIAtom*             nsXULElement::kClassAtom;
nsIAtom*             nsXULElement::kContextAtom;
nsIAtom*             nsXULElement::kHeightAtom;
nsIAtom*             nsXULElement::kHiddenAtom;
nsIAtom*             nsXULElement::kIdAtom;
nsIAtom*             nsXULElement::kObservesAtom;
nsIAtom*             nsXULElement::kOpenAtom;
nsIAtom*             nsXULElement::kPopupAtom;
nsIAtom*             nsXULElement::kMenuPopupAtom;
nsIAtom*             nsXULElement::kRefAtom;
nsIAtom*             nsXULElement::kSelectedAtom;
nsIAtom*             nsXULElement::kStyleAtom;
nsIAtom*             nsXULElement::kTooltipAtom;
nsIAtom*             nsXULElement::kTreeAtom;
nsIAtom*             nsXULElement::kTreeCellAtom;
nsIAtom*             nsXULElement::kTreeChildrenAtom;
nsIAtom*             nsXULElement::kTreeColAtom;
nsIAtom*             nsXULElement::kTreeItemAtom;
nsIAtom*             nsXULElement::kTreeRowAtom;
nsIAtom*             nsXULElement::kValueAtom;
nsIAtom*             nsXULElement::kWidthAtom;
nsIAtom*             nsXULElement::kWindowAtom;
nsIAtom*             nsXULElement::kMenuListAtom;
nsIAtom*             nsXULElement::kMenuAtom;
nsIAtom*             nsXULElement::kPopupSetAtom;
nsIAtom*             nsXULElement::kBrowserAtom;
nsIAtom*             nsXULElement::kEditorAtom;
nsIAtom*             nsXULElement::kIFrameAtom;

#ifdef XUL_PROTOTYPE_ATTRIBUTE_METERING
PRUint32             nsXULPrototypeAttribute::gNumElements;
PRUint32             nsXULPrototypeAttribute::gNumAttributes;
PRUint32             nsXULPrototypeAttribute::gNumEventHandlers;
PRUint32             nsXULPrototypeAttribute::gNumCacheTests;
PRUint32             nsXULPrototypeAttribute::gNumCacheHits;
PRUint32             nsXULPrototypeAttribute::gNumCacheSets;
PRUint32             nsXULPrototypeAttribute::gNumCacheFills;
#endif

//----------------------------------------------------------------------
// nsXULElement


nsXULElement::nsXULElement()
    : mPrototype(nsnull),
      mDocument(nsnull),
      mParent(nsnull),
      mChildren(nsnull),
      mScriptObject(nsnull),
      mLazyState(0),
      mIsAnonymous(PR_FALSE),
      mSlots(nsnull)
{
    NS_INIT_REFCNT();
    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumElements);
}


nsresult
nsXULElement::Init()
{
    if (gRefCnt++ == 0) {
        nsresult rv;

// Temporary hack that tells if some new DOM Level 2 features are on or off
        kStrictDOMLevel2 = PR_FALSE; // Default in case of failure
        NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_PROGID, &rv);
        if (NS_SUCCEEDED(rv)) {
            prefs->GetBoolPref("temp.DOMLevel2update.enabled", &kStrictDOMLevel2);
        }
// End of temp hack.


        rv = nsServiceManager::GetService(kRDFServiceCID,
                                          kIRDFServiceIID,
                                          (nsISupports**) &gRDFService);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
        if (NS_FAILED(rv)) return rv;
       
        kClassAtom          = NS_NewAtom("class");
        kContextAtom        = NS_NewAtom("context");
        kHeightAtom         = NS_NewAtom("height");
        kHiddenAtom         = NS_NewAtom("hidden");
        kIdAtom             = NS_NewAtom("id");
        kObservesAtom       = NS_NewAtom("observes");
        kOpenAtom           = NS_NewAtom("open");
        kPopupAtom          = NS_NewAtom("popup");
        kMenuPopupAtom      = NS_NewAtom("menupopup");
        kRefAtom            = NS_NewAtom("ref");
        kSelectedAtom       = NS_NewAtom("selected");
        kStyleAtom          = NS_NewAtom("style");
        kTooltipAtom        = NS_NewAtom("tooltip");
        kTreeAtom           = NS_NewAtom("tree");
        kTreeCellAtom       = NS_NewAtom("treecell");
        kTreeChildrenAtom   = NS_NewAtom("treechildren");
        kTreeColAtom        = NS_NewAtom("treecol");
        kTreeItemAtom       = NS_NewAtom("treeitem");
        kTreeRowAtom        = NS_NewAtom("treerow");
        kValueAtom          = NS_NewAtom("value");
        kWidthAtom          = NS_NewAtom("width");
        kWindowAtom         = NS_NewAtom("window");
        kMenuListAtom       = NS_NewAtom("menulist");
        kMenuAtom           = NS_NewAtom("menu");
        kPopupSetAtom       = NS_NewAtom("popupset");
        kBrowserAtom        = NS_NewAtom("browser");
        kIFrameAtom         = NS_NewAtom("iframe");
        kEditorAtom         = NS_NewAtom("editor");

        rv = nsComponentManager::CreateInstance(kNameSpaceManagerCID,
                                                nsnull,
                                                kINameSpaceManagerIID,
                                                (void**) &gNameSpaceManager);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create namespace manager");
        if (NS_FAILED(rv)) return rv;

        if (gNameSpaceManager) {
            gNameSpaceManager->RegisterNameSpace(NS_ConvertASCIItoUCS2(kRDFNameSpaceURI), kNameSpaceID_RDF);
            gNameSpaceManager->RegisterNameSpace(NS_ConvertASCIItoUCS2(kXULNameSpaceURI), kNameSpaceID_XUL);
        }

        rv = nsServiceManager::GetService(kXULContentUtilsCID,
                                          NS_GET_IID(nsIXULContentUtils),
                                          (nsISupports**) &gXULUtils);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get XUL content utils");
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsXULElement::~nsXULElement()
{
    delete mSlots;

    //NS_IF_RELEASE(mDocument); // not refcounted
    //NS_IF_RELEASE(mParent)    // not refcounted

    if (mChildren) {
        // Force child's parent to be null. This ensures that we don't
        // have dangling pointers if a child gets leaked.
        PRUint32 cnt;
        mChildren->Count(&cnt);
        for (PRInt32 i = cnt - 1; i >= 0; --i) {
            nsISupports* isupports = mChildren->ElementAt(i);
            nsCOMPtr<nsIContent> child = do_QueryInterface(isupports);
            NS_RELEASE(isupports);

            child->SetParent(nsnull);
        }
    }

    // Clean up shared statics
    if (--gRefCnt == 0) {
        if (gRDFService) {
            nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
            gRDFService = nsnull;
        }
        
        NS_IF_RELEASE(kClassAtom);
        NS_IF_RELEASE(kContextAtom);
        NS_IF_RELEASE(kHeightAtom);
        NS_IF_RELEASE(kHiddenAtom);
        NS_IF_RELEASE(kIdAtom);
        NS_IF_RELEASE(kObservesAtom);
        NS_IF_RELEASE(kOpenAtom);
        NS_IF_RELEASE(kPopupAtom);
        NS_IF_RELEASE(kMenuPopupAtom);
        NS_IF_RELEASE(kRefAtom);
        NS_IF_RELEASE(kSelectedAtom);
        NS_IF_RELEASE(kStyleAtom);
        NS_IF_RELEASE(kTooltipAtom);
        NS_IF_RELEASE(kTreeAtom);
        NS_IF_RELEASE(kTreeCellAtom);
        NS_IF_RELEASE(kTreeChildrenAtom);
        NS_IF_RELEASE(kTreeColAtom);
        NS_IF_RELEASE(kTreeItemAtom);
        NS_IF_RELEASE(kTreeRowAtom);
        NS_IF_RELEASE(kValueAtom);
        NS_IF_RELEASE(kWidthAtom);
        NS_IF_RELEASE(kWindowAtom);
        NS_IF_RELEASE(kMenuListAtom);
        NS_IF_RELEASE(kMenuAtom);
        NS_IF_RELEASE(kPopupSetAtom);
        NS_IF_RELEASE(kBrowserAtom);
        NS_IF_RELEASE(kIFrameAtom);
        NS_IF_RELEASE(kEditorAtom);

        NS_IF_RELEASE(gNameSpaceManager);

        if (gXULUtils) {
            nsServiceManager::ReleaseService(kXULContentUtilsCID, gXULUtils);
            gXULUtils = nsnull;
        }
    }
}


nsresult
nsXULElement::Create(nsXULPrototypeElement* aPrototype,
                     nsIDocument* aDocument,
                     PRBool aIsScriptable,
                     nsIContent** aResult)
{
    // Create an nsXULElement from a prototype
    NS_PRECONDITION(aPrototype != nsnull, "null ptr");
    if (! aPrototype)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aDocument != nsnull, "null ptr");
    if (! aDocument)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsXULElement* element = new nsXULElement();
    if (! element)
        return NS_ERROR_OUT_OF_MEMORY;

    // anchor the element so an early return will clean up properly.
    nsCOMPtr<nsIContent> anchor =
        do_QueryInterface(NS_REINTERPRET_CAST(nsIStyledContent*, element));

    nsresult rv;
    rv = element->Init();
    if (NS_FAILED(rv)) return rv;

    element->mPrototype = aPrototype;
    element->mDocument = aDocument;

    if (aIsScriptable) {
        // Check each attribute on the prototype to see if we need to do
        // any additional processing and hookup that would otherwise be
        // done 'automagically' by SetAttribute().
        for (PRInt32 i = 0; i < aPrototype->mNumAttributes; ++i) {
            nsXULPrototypeAttribute* attr = &(aPrototype->mAttributes[i]);

            if (attr->mNodeInfo->NamespaceEquals(kNameSpaceID_None)) {
                // Check for an event handler
                nsIID iid;
                PRBool found;
                nsCOMPtr<nsIAtom> name;
                attr->mNodeInfo->GetNameAtom(*getter_AddRefs(name));
                rv = gXULUtils->GetEventHandlerIID(name, &iid, &found);
                if (NS_FAILED(rv)) return rv;

                if (found) {
                    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumEventHandlers);
                    rv = element->AddScriptEventListener(name, attr->mValue, iid);
                    if (NS_FAILED(rv)) return rv;
                }

                // Check for popup attributes
                if (attr->mNodeInfo->Equals(kPopupAtom) ||
                    attr->mNodeInfo->Equals(kTooltipAtom) ||
                    attr->mNodeInfo->Equals(kContextAtom)) {
                    rv = element->AddPopupListener(name);
                    if (NS_FAILED(rv)) return rv;
                }
            }
        }
    }

    *aResult = NS_REINTERPRET_CAST(nsIStyledContent*, element);
    NS_ADDREF(*aResult);
    return NS_OK;
}

nsresult
nsXULElement::Create(nsINodeInfo *aNodeInfo, nsIContent** aResult)
{
    // Create an nsXULElement with the specified namespace and tag.
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsXULElement* element = new nsXULElement();
    if (! element)
        return NS_ERROR_OUT_OF_MEMORY;

    // anchor the element so an early return will clean up properly.
    nsCOMPtr<nsIContent> anchor =
        do_QueryInterface(NS_REINTERPRET_CAST(nsIStyledContent*, element));

    nsresult rv;
    rv = element->Init();
    if (NS_FAILED(rv)) return rv;

    rv = element->EnsureSlots();
    if (NS_FAILED(rv)) return rv;

    element->mSlots->mNodeInfo    = aNodeInfo;

    *aResult = NS_REINTERPRET_CAST(nsIStyledContent*, element);
    NS_ADDREF(*aResult);
    return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports interface

NS_IMPL_ADDREF(nsXULElement);
NS_IMPL_RELEASE(nsXULElement);

NS_IMETHODIMP 
nsXULElement::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;
    *result = nsnull;

    nsresult rv;

    if (iid.Equals(NS_GET_IID(nsIStyledContent)) ||
        iid.Equals(kIContentIID) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIStyledContent*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIXMLContent))) {
        *result = NS_STATIC_CAST(nsIXMLContent*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIXULContent))) {
        *result = NS_STATIC_CAST(nsIXULContent*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMXULElement)) ||
             iid.Equals(kIDOMElementIID) ||
             iid.Equals(kIDOMNodeIID)) {
        *result = NS_STATIC_CAST(nsIDOMElement*, this);
    }
    else if (iid.Equals(kIScriptObjectOwnerIID)) {
        *result = NS_STATIC_CAST(nsIScriptObjectOwner*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIScriptEventHandlerOwner))) {
        *result = NS_STATIC_CAST(nsIScriptEventHandlerOwner*, this);
    }
    else if (iid.Equals(kIDOMEventReceiverIID)) {
        *result = NS_STATIC_CAST(nsIDOMEventReceiver*, this);
    }
    else if (iid.Equals(kIDOMEventTargetIID)) {
        *result = NS_STATIC_CAST(nsIDOMEventTarget*, this);
    }
    else if (iid.Equals(kIJSScriptObjectIID)) {
        *result = NS_STATIC_CAST(nsIJSScriptObject*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIStyleRule))) {
        *result = NS_STATIC_CAST(nsIStyleRule*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIChromeEventHandler))) {
        *result = NS_STATIC_CAST(nsIChromeEventHandler*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMXULMenuListElement)) &&
             (NodeInfo()->NamespaceEquals(kNameSpaceID_XUL))) {
      nsCOMPtr<nsIAtom> tag;
      PRInt32 dummy;
      NS_WITH_SERVICE(nsIXBLService, xblService, "component://netscape/xbl", &rv);
      xblService->ResolveTag(NS_STATIC_CAST(nsIStyledContent*, this), &dummy, getter_AddRefs(tag));
      if (tag.get() == kMenuListAtom) {
        // We delegate XULMenuListElement APIs to an aggregate object
        if (! InnerXULElement()) {
            rv = EnsureSlots();
            if (NS_FAILED(rv)) return rv;

            if ((mSlots->mInnerXULElement = new nsXULMenuListElement(this)) == nsnull)
                return NS_ERROR_OUT_OF_MEMORY;
        }

        return InnerXULElement()->QueryInterface(iid, result);
      }
      else
        return NS_NOINTERFACE;
    }
    else if ((iid.Equals(NS_GET_IID(nsIDOMXULTreeElement)) ||
              iid.Equals(NS_GET_IID(nsIXULTreeContent))) &&
             (NodeInfo()->NamespaceEquals(kNameSpaceID_XUL))){
      nsCOMPtr<nsIAtom> tag;
      PRInt32 dummy;
      NS_WITH_SERVICE(nsIXBLService, xblService, "component://netscape/xbl", &rv);
      xblService->ResolveTag(NS_STATIC_CAST(nsIStyledContent*, this), &dummy, getter_AddRefs(tag));
      if (tag.get() == kTreeAtom) {
        // We delegate XULTreeElement APIs to an aggregate object
        if (! InnerXULElement()) {
            rv = EnsureSlots();
            if (NS_FAILED(rv)) return rv;

            if ((mSlots->mInnerXULElement = new nsXULTreeElement(this)) == nsnull)
                return NS_ERROR_OUT_OF_MEMORY;
        }

        return InnerXULElement()->QueryInterface(iid, result);
      }
      else
        return NS_NOINTERFACE;
    }
    else {
        *result = nsnull;
        return NS_NOINTERFACE;
    }

    // if we get here, we know one of the above IIDs was ok.
    NS_ADDREF(this);
    return NS_OK;
}

//----------------------------------------------------------------------
// nsIDOMNode interface

NS_IMETHODIMP
nsXULElement::GetNodeName(nsString& aNodeName)
{
    return NodeInfo()->GetQualifiedName(aNodeName);
}


NS_IMETHODIMP
nsXULElement::GetNodeValue(nsString& aNodeValue)
{
    aNodeValue.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetNodeValue(const nsString& aNodeValue)
{
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}


NS_IMETHODIMP
nsXULElement::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::ELEMENT_NODE;
  return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetParentNode(nsIDOMNode** aParentNode)
{
    if (mParent) {
        return mParent->QueryInterface(kIDOMNodeIID, (void**) aParentNode);
    }
    else if (mDocument) {
        // XXX This is a mess because of our fun multiple inheritance heirarchy
        nsCOMPtr<nsIContent> root = dont_AddRef( mDocument->GetRootContent() );
        nsCOMPtr<nsIContent> thisIContent;
        QueryInterface(kIContentIID, getter_AddRefs(thisIContent));

        if (root == thisIContent) {
            // If we don't have a parent, and we're the root content
            // of the document, DOM says that our parent is the
            // document.
            return mDocument->QueryInterface(kIDOMNodeIID, (void**)aParentNode);
        }
    }

    // A standalone element (i.e. one without a parent or a document)
    *aParentNode = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
    nsresult rv;

    nsRDFDOMNodeList* children;
    rv = nsRDFDOMNodeList::Create(&children);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create DOM node list");
    if (NS_FAILED(rv)) return rv;

    PRInt32 count;
    rv = ChildCount(count);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get child count");
    if (NS_FAILED(rv)) return rv;

    for (PRInt32 i = 0; i < count; ++i) {
        nsCOMPtr<nsIContent> child;
        rv = ChildAt(i, *getter_AddRefs(child));
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get child");
        if (NS_FAILED(rv))
            break;

        nsCOMPtr<nsIDOMNode> domNode;
        rv = child->QueryInterface(kIDOMNodeIID, (void**) getter_AddRefs(domNode));
        if (NS_FAILED(rv)) {
            NS_WARNING("child content doesn't support nsIDOMNode");
            continue;
        }

        rv = children->AppendNode(domNode);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to append node to list");
        if (NS_FAILED(rv))
            break;
    }

    // Create() addref'd for us
    *aChildNodes = children;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetFirstChild(nsIDOMNode** aFirstChild)
{
    nsresult rv;
    nsCOMPtr<nsIContent> child;
    rv = ChildAt(0, *getter_AddRefs(child));

    if (NS_SUCCEEDED(rv) && (child != nsnull)) {
        rv = child->QueryInterface(kIDOMNodeIID, (void**) aFirstChild);
        NS_ASSERTION(NS_SUCCEEDED(rv), "not a DOM node");
        return rv;
    }

    *aFirstChild = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetLastChild(nsIDOMNode** aLastChild)
{
    nsresult rv;
    PRInt32 count;
    rv = ChildCount(count);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get child count");

    if (NS_SUCCEEDED(rv) && (count != 0)) {
        nsCOMPtr<nsIContent> child;
        rv = ChildAt(count - 1, *getter_AddRefs(child));

        NS_ASSERTION(child != nsnull, "no child");

        if (child) {
            rv = child->QueryInterface(kIDOMNodeIID, (void**) aLastChild);
            NS_ASSERTION(NS_SUCCEEDED(rv), "not a DOM node");
            return rv;
        }
    }

    *aLastChild = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetPreviousSibling(nsIDOMNode** aPreviousSibling)
{
    if (nsnull != mParent) {
        PRInt32 pos;
        mParent->IndexOf(NS_STATIC_CAST(nsIStyledContent*, this), pos);
        if (pos > -1) {
            nsCOMPtr<nsIContent> prev;
            mParent->ChildAt(--pos, *getter_AddRefs(prev));
            if (prev) {
                nsresult rv = prev->QueryInterface(kIDOMNodeIID, (void**) aPreviousSibling);
                NS_ASSERTION(NS_SUCCEEDED(rv), "not a DOM node");
                return rv;
            }
        }
    }

    // XXX Nodes that are just below the document (their parent is the
    // document) need to go to the document to find their previous sibling.
    *aPreviousSibling = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetNextSibling(nsIDOMNode** aNextSibling)
{
    if (nsnull != mParent) {
        PRInt32 pos;
        mParent->IndexOf(NS_STATIC_CAST(nsIStyledContent*, this), pos);
        if (pos > -1) {
            nsCOMPtr<nsIContent> next;
            mParent->ChildAt(++pos, *getter_AddRefs(next));
            if (next) {
                nsresult res = next->QueryInterface(kIDOMNodeIID, (void**) aNextSibling);
                NS_ASSERTION(NS_OK == res, "not a DOM Node");
                return res;
            }
        }
    }

    // XXX Nodes that are just below the document (their parent is the
    // document) need to go to the document to find their next sibling.
    *aNextSibling = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
    nsresult rv;
    if (! Attributes()) {
        rv = EnsureSlots();
        if (NS_FAILED(rv)) return rv;

        if (! Attributes()) {
            rv = nsXULAttributes::Create(NS_STATIC_CAST(nsIStyledContent*, this), &(mSlots->mAttributes));
            if (NS_FAILED(rv)) return rv;
        }
    }

    *aAttributes = Attributes();
    NS_ADDREF(*aAttributes);
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
    if (mDocument) {
        return mDocument->QueryInterface(NS_GET_IID(nsIDOMDocument), (void**) aOwnerDocument);
    }
    else {
        *aOwnerDocument = nsnull;
        return NS_OK;
    }
}


NS_IMETHODIMP
nsXULElement::GetNamespaceURI(nsString& aNamespaceURI)
{
    return NodeInfo()->GetNamespaceURI(aNamespaceURI);
}


NS_IMETHODIMP
nsXULElement::GetPrefix(nsString& aPrefix)
{
    return NodeInfo()->GetPrefix(aPrefix);
}


NS_IMETHODIMP
nsXULElement::SetPrefix(const nsString& aPrefix)
{
    // XXX: Validate the prefix string!

    nsINodeInfo *newNodeInfo = nsnull;
    nsCOMPtr<nsIAtom> prefix;

    if (aPrefix.Length()) {
        prefix = dont_AddRef(NS_NewAtom(aPrefix));
        NS_ENSURE_TRUE(prefix, NS_ERROR_OUT_OF_MEMORY);
    }

    nsresult rv = EnsureSlots();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mSlots->mNodeInfo->PrefixChanged(prefix, newNodeInfo);
    NS_ENSURE_SUCCESS(rv, rv);

    mSlots->mNodeInfo = newNodeInfo;

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetLocalName(nsString& aLocalName)
{
    return NodeInfo()->GetLocalName(aLocalName);
}


NS_IMETHODIMP
nsXULElement::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn)
{
    NS_PRECONDITION(aNewChild != nsnull, "null ptr");
    if (! aNewChild)
        return NS_ERROR_NULL_POINTER;

    // aRefChild may be null; that means "append".

    nsresult rv;

    nsCOMPtr<nsIContent> newcontent = do_QueryInterface(aNewChild);
    NS_ASSERTION(newcontent != nsnull, "not an nsIContent");
    if (! newcontent)
        return NS_ERROR_UNEXPECTED;

    // First, check to see if the content was already parented
    // somewhere. If so, remove it.
    nsCOMPtr<nsIContent> oldparent;
    rv = newcontent->GetParent(*getter_AddRefs(oldparent));
    if (NS_FAILED(rv)) return rv;

    if (oldparent) {
        PRInt32 oldindex;
        rv = oldparent->IndexOf(newcontent, oldindex);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to determine index of aNewChild in old parent");
        if (NS_FAILED(rv)) return rv;

        NS_ASSERTION(oldindex >= 0, "old parent didn't think aNewChild was a child");

        if (oldindex >= 0) {
            rv = oldparent->RemoveChildAt(oldindex, PR_TRUE);
            if (NS_FAILED(rv)) return rv;
        }
    }

    // Now, insert the element into the content model under 'this'
    if (aRefChild) {
        nsCOMPtr<nsIContent> refcontent = do_QueryInterface(aRefChild);
        NS_ASSERTION(refcontent != nsnull, "not an nsIContent");
        if (! refcontent)
            return NS_ERROR_UNEXPECTED;

        PRInt32 pos;
        rv = IndexOf(refcontent, pos);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to determine index of aRefChild");
        if (NS_FAILED(rv)) return rv;

        if (pos >= 0) {
            // Because InsertChildAt() only does a "shallow"
            // SetDocument(), we need to ensure that a "deep" one is
            // done now. We do it -before- inserting into the content
            // model, because some frames assume that the document
            // will have been set.
            rv = newcontent->SetDocument(mDocument, PR_TRUE, PR_TRUE);
            if (NS_FAILED(rv)) return rv;

            rv = InsertChildAt(newcontent, pos, PR_TRUE);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to insert aNewChild");
            if (NS_FAILED(rv)) return rv;
        }

        // XXX Hmm. There's a case here that we handle ambiguously, I
        // think. If aRefChild _isn't_ actually one of our kids, then
        // pos == -1, and we'll never insert the new kid. Should we
        // just append it?
    }
    else {
        // Because AppendChildTo() only does a "shallow"
        // SetDocument(), we need to ensure that a "deep" one is done
        // now. We do it -before- appending to the content model,
        // because some frames assume that they can get to the
        // document right away.
        rv = newcontent->SetDocument(mDocument, PR_TRUE, PR_TRUE);
        if (NS_FAILED(rv)) return rv;

        rv = AppendChildTo(newcontent, PR_TRUE);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to append a aNewChild");
        if (NS_FAILED(rv)) return rv;
    }

    NS_ADDREF(aNewChild);
    *aReturn = aNewChild;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
    NS_PRECONDITION(aNewChild != nsnull, "null ptr");
    if (! aNewChild)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aOldChild != nsnull, "null ptr");
    if (! aOldChild)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsCOMPtr<nsIContent> oldelement = do_QueryInterface(aOldChild);
    NS_ASSERTION(oldelement != nsnull, "not an nsIContent");

    if (oldelement) {
        PRInt32 pos;
        rv = IndexOf(oldelement, pos);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to determine index of aOldChild");

        if (NS_SUCCEEDED(rv) && (pos >= 0)) {
            nsCOMPtr<nsIContent> newelement = do_QueryInterface(aNewChild);
            NS_ASSERTION(newelement != nsnull, "not an nsIContent");

            if (newelement) {
                // Because ReplaceChildAt() only does a "shallow"
                // SetDocument(), we need to ensure that a "deep" one
                // is done now. We do it -before- replacing the nodein
                // the content model, because some frames assume that
                // the document will have been set.
                rv = newelement->SetDocument(mDocument, PR_TRUE, PR_TRUE);
                if (NS_FAILED(rv)) return rv;

                rv = ReplaceChildAt(newelement, pos, PR_TRUE);
                NS_ASSERTION(NS_SUCCEEDED(rv), "unable to replace old child");
            }
        }
    }

    NS_ADDREF(aNewChild);
    *aReturn = aNewChild;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
    NS_PRECONDITION(aOldChild != nsnull, "null ptr");
    if (! aOldChild)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsCOMPtr<nsIContent> element = do_QueryInterface(aOldChild);
    NS_ASSERTION(element != nsnull, "not an nsIContent");

    if (element) {
        PRInt32 pos;
        rv = IndexOf(element, pos);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to determine index of aOldChild");

        if (NS_SUCCEEDED(rv) && (pos >= 0)) {
            rv = RemoveChildAt(pos, PR_TRUE);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to remove old child");
        }
    }

    NS_ADDREF(aOldChild);
    *aReturn = aOldChild;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
    return InsertBefore(aNewChild, nsnull, aReturn);
}


NS_IMETHODIMP
nsXULElement::HasChildNodes(PRBool* aReturn)
{
    nsresult rv;
    PRInt32 count;
    if (NS_FAILED(rv = ChildCount(count))) {
        NS_ERROR("unable to count kids");
        return rv;
    }
    *aReturn = (count > 0);
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
    nsresult rv;

    nsCOMPtr<nsIContent> result;

    if (mPrototype) {
        // We haven't "faulted" and become a heavyweight node yet, so
        // we can go ahead and just make another lightweight from our
        // prototype.
        rv = nsXULElement::Create(mPrototype, mDocument, PR_TRUE, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;
    }
    else if (mSlots) {
        // We've faulted: create another heavyweight, and then copy
        // stuff by hand.
        rv = nsXULElement::Create(mSlots->mNodeInfo, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;

        // Copy namespace stuff.
        nsCOMPtr<nsIXMLContent> xmlcontent = do_QueryInterface(result);
        if (xmlcontent) {
            rv = xmlcontent->SetContainingNameSpace(mSlots->mNameSpace);
            if (NS_FAILED(rv)) return rv;
        }

        // Copy attributes, if there are any.
        if (mSlots->mAttributes) {
            PRInt32 count = mSlots->mAttributes->Count();
            for (PRInt32 i = 0; i < count; ++i) {
                nsXULAttribute* attr = mSlots->mAttributes->ElementAt(i);
                NS_ASSERTION(attr != nsnull, "null ptr");
                if (! attr)
                    return NS_ERROR_UNEXPECTED;

                nsAutoString value;
                rv = attr->GetValue(value);
                if (NS_FAILED(rv)) return rv;

                rv = result->SetAttribute(attr->GetNodeInfo(), value,
                                          PR_FALSE);
                if (NS_FAILED(rv)) return rv;
            }
        }

        // XXX TODO: set up RDF generic builder n' stuff if there is a
        // 'datasources' attribute? This is really kind of tricky,
        // because then we'd need to -selectively- copy children that
        // -weren't- generated from RDF. Ugh. Forget it.
    }
    else {
        NS_ERROR("ack! no prototype and no slots!");
        return NS_ERROR_UNEXPECTED;
    }

    if (aDeep && mChildren) {
        // Copy cloned children!
        PRUint32 count;
        rv = mChildren->Count(&count);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = 0; i < PRInt32(count); ++i) {
            nsCOMPtr<nsIContent> child =
                dont_AddRef(NS_STATIC_CAST(nsIContent*, mChildren->ElementAt(i)));

            NS_ASSERTION(child != nsnull, "null ptr");
            if (! child)
                return NS_ERROR_UNEXPECTED;

            nsCOMPtr<nsIDOMNode> domchild = do_QueryInterface(child);
            NS_ASSERTION(domchild != nsnull, "child is not a DOM node");
            if (! domchild)
                return NS_ERROR_UNEXPECTED;
            
            nsCOMPtr<nsIDOMNode> newdomchild;
            rv = domchild->CloneNode(PR_TRUE, getter_AddRefs(newdomchild));
            if (NS_FAILED(rv)) return rv;

            nsCOMPtr<nsIContent> newchild = do_QueryInterface(newdomchild);
            NS_ASSERTION(newchild != nsnull, "newdomchild is not an nsIContent");
            if (! newchild)
                return NS_ERROR_UNEXPECTED;

            rv = result->AppendChildTo(newchild, PR_FALSE);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return CallQueryInterface(result, aReturn);
}


NS_IMETHODIMP
nsXULElement::Normalize()
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULElement::Supports(const nsString& aFeature, const nsString& aVersion,
                       PRBool* aReturn)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}


//----------------------------------------------------------------------
// nsIDOMElement interface

NS_IMETHODIMP
nsXULElement::GetTagName(nsString& aTagName)
{
    return NodeInfo()->GetQualifiedName(aTagName);
}

NS_IMETHODIMP
nsXULElement::GetNodeInfo(nsINodeInfo*& aResult) const
{
    aResult = NodeInfo();
    NS_IF_ADDREF(aResult);

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetAttribute(const nsString& aName, nsString& aReturn)
{
    nsresult rv;
    PRInt32 nameSpaceID;
    nsIAtom* nameAtom;

    if (NS_FAILED(rv = ParseAttributeString(aName, nameAtom, nameSpaceID))) {
        NS_WARNING("unable to parse attribute name");
        return rv;
    }
    if (kNameSpaceID_Unknown == nameSpaceID) {
      nameSpaceID = kNameSpaceID_None;  // ignore unknown prefix XXX is this correct?
    }

    GetAttribute(nameSpaceID, nameAtom, aReturn);
    NS_RELEASE(nameAtom);
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::SetAttribute(const nsString& aName, const nsString& aValue)
{
    if (kStrictDOMLevel2) {
        PRInt32 pos = aName.FindChar(':');
        if (pos >= 0) {
          nsCAutoString tmp; tmp.AssignWithConversion(aName);
          printf ("Possible DOM Error: SetAttribute(\"%s\") called, use SetAttributeNS() in stead!\n", (const char *)tmp);
        }

        nsCOMPtr<nsIAtom> tag(dont_AddRef(NS_NewAtom(aName)));
        return SetAttribute(kNameSpaceID_None, tag, aValue, PR_TRUE);
    }
    nsresult rv;

    PRInt32 nameSpaceID;
    nsCOMPtr<nsIAtom> tag;

    rv = ParseAttributeString(aName, *getter_AddRefs(tag), nameSpaceID);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to parse attribute name");

    if (NS_SUCCEEDED(rv)) {
        rv = SetAttribute(nameSpaceID, tag, aValue, PR_TRUE);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set attribute");
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::RemoveAttribute(const nsString& aName)
{
    if (kStrictDOMLevel2) {
        PRInt32 pos = aName.FindChar(':');
        if (pos >= 0) {
          nsCAutoString tmp; tmp.AssignWithConversion(aName);
          printf ("Possible DOM Error: RemoveAttribute(\"%s\") called, use RemoveAttributeNS() in stead!\n", (const char *)tmp);
        }

        nsCOMPtr<nsIAtom> tag(dont_AddRef(NS_NewAtom(aName)));
        return UnsetAttribute(kNameSpaceID_None, tag, PR_TRUE);
    }

    nsresult rv;

    PRInt32 nameSpaceID;
    nsCOMPtr<nsIAtom> tag;

    rv = ParseAttributeString(aName, *getter_AddRefs(tag), nameSpaceID);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to parse attribute name");

    if (NS_SUCCEEDED(rv)) {
        rv = UnsetAttribute(nameSpaceID, tag, PR_TRUE);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to remove attribute");
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetAttributeNode(const nsString& aName, nsIDOMAttr** aReturn)
{
    if (kStrictDOMLevel2) {
        PRInt32 pos = aName.FindChar(':');
        if (pos >= 0) {
          nsCAutoString tmp; tmp.AssignWithConversion(aName);
          printf ("Possible DOM Error: GetAttributeNode(\"%s\") called, use GetAttributeNodeNS() in stead!\n", (const char *)tmp);
        }
    }

    NS_PRECONDITION(aReturn != nsnull, "null ptr");
    if (! aReturn)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsCOMPtr<nsIDOMNamedNodeMap> map;
    rv = GetAttributes(getter_AddRefs(map));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDOMNode> node;
    rv = map->GetNamedItem(aName, getter_AddRefs(node));
    if (NS_FAILED(rv)) return rv;

    if (node) {
        rv = node->QueryInterface(NS_GET_IID(nsIDOMAttr), (void**) aReturn);
    }
    else {
        *aReturn = nsnull;
        rv = NS_OK;
    }

    return rv;
}


NS_IMETHODIMP
nsXULElement::SetAttributeNode(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn)
{
    NS_PRECONDITION(aNewAttr != nsnull, "null ptr");
    if (! aNewAttr)
        return NS_ERROR_NULL_POINTER;

    NS_NOTYETIMPLEMENTED("write me");

    NS_ADDREF(aNewAttr);
    *aReturn = aNewAttr;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::RemoveAttributeNode(nsIDOMAttr* aOldAttr, nsIDOMAttr** aReturn)
{
    NS_PRECONDITION(aOldAttr != nsnull, "null ptr");
    if (! aOldAttr)
        return NS_ERROR_NULL_POINTER;

    NS_NOTYETIMPLEMENTED("write me");

    NS_ADDREF(aOldAttr);
    *aReturn = aOldAttr;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetElementsByTagName(const nsString& aName, nsIDOMNodeList** aReturn)
{
    if (kStrictDOMLevel2) { 
        PRInt32 pos = aName.FindChar(':');
        if (pos >= 0) {
          nsCAutoString tmp; tmp.AssignWithConversion(aName);
          printf ("Possible DOM Error: GetElementsByTagName(\"%s\") called, use GetElementsByTagNameNS() in stead!\n", (const char *)tmp);
        }
    }

    nsresult rv;

    nsRDFDOMNodeList* elements;
    rv = nsRDFDOMNodeList::Create(&elements);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create node list");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDOMNode> domElement;
    rv = QueryInterface(NS_GET_IID(nsIDOMNode), getter_AddRefs(domElement));
    if (NS_SUCCEEDED(rv)) {
        GetElementsByTagName(domElement, aName, elements);
    }

    // transfer ownership to caller
    *aReturn = elements;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetAttributeNS(const nsString& aNamespaceURI,
                             const nsString& aLocalName, nsString& aReturn)
{
    nsCOMPtr<nsIAtom> name(dont_AddRef(NS_NewAtom(aLocalName)));
    PRInt32 nsid;

    gNameSpaceManager->GetNameSpaceID(aNamespaceURI, nsid);

    if (nsid == kNameSpaceID_Unknown) {
        // Unkonwn namespace means no attr...

        aReturn.Truncate();
        return NS_OK;
    }

    GetAttribute(nsid, name, aReturn);

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetAttributeNS(const nsString& aNamespaceURI,
                             const nsString& aQualifiedName,
                             const nsString& aValue)
{
    nsCOMPtr<nsINodeInfoManager> nimgr;
    nsresult rv = NodeInfo()->GetNodeInfoManager(*getter_AddRefs(nimgr));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsINodeInfo> ni;
    rv = nimgr->GetNodeInfo(aQualifiedName, aNamespaceURI, *getter_AddRefs(ni));
    NS_ENSURE_SUCCESS(rv, rv);

    return SetAttribute(ni, aValue, PR_TRUE);
}

NS_IMETHODIMP
nsXULElement::RemoveAttributeNS(const nsString& aNamespaceURI,
                                const nsString& aLocalName)
{
    PRInt32 nameSpaceId;
    nsCOMPtr<nsIAtom> tag = dont_AddRef(NS_NewAtom(aLocalName));

    gNameSpaceManager->GetNameSpaceID(aNamespaceURI, nameSpaceId);

    nsresult rv = UnsetAttribute(nameSpaceId, tag, PR_TRUE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to remove attribute");

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetAttributeNodeNS(const nsString& aNamespaceURI,
                                 const nsString& aLocalName,
                                 nsIDOMAttr** aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);

    nsresult rv;

    nsCOMPtr<nsIDOMNamedNodeMap> map;
    rv = GetAttributes(getter_AddRefs(map));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDOMNode> node;
    rv = map->GetNamedItemNS(aNamespaceURI, aLocalName, getter_AddRefs(node));
    if (NS_FAILED(rv)) return rv;

    if (node) {
        rv = node->QueryInterface(NS_GET_IID(nsIDOMAttr), (void**) aReturn);
    }
    else {
        *aReturn = nsnull;
        rv = NS_OK;
    }

    return rv;
}

NS_IMETHODIMP
nsXULElement::SetAttributeNodeNS(nsIDOMAttr* aNewAttr,
                                 nsIDOMAttr** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULElement::GetElementsByTagNameNS(const nsString& aNamespaceURI,
                                     const nsString& aLocalName,
                                     nsIDOMNodeList** aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);

    PRInt32 nameSpaceId = kNameSpaceID_Unknown;

    nsRDFDOMNodeList* elements;
    nsresult rv = nsRDFDOMNodeList::Create(&elements);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNodeList> kungFuGrip;
    kungFuGrip = dont_AddRef(NS_STATIC_CAST(nsIDOMNodeList *, elements));

    if (!aNamespaceURI.EqualsWithConversion("*")) {
        gNameSpaceManager->GetNameSpaceID(aNamespaceURI, nameSpaceId);

        if (nameSpaceId == kNameSpaceID_Unknown) {
            // Unkonwn namespace means no matches, we return an empty list...

            *aReturn = elements;
            NS_ADDREF(*aReturn);

            return NS_OK;
        }
    }

    rv = nsXULDocument::GetElementsByTagName(NS_STATIC_CAST(nsIStyledContent *,
                                                            this), aLocalName,
                                             nameSpaceId, elements);
    NS_ENSURE_SUCCESS(rv, rv);

    *aReturn = elements;
    NS_ADDREF(*aReturn);

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::HasAttribute(const nsString& aName, PRBool* aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);

    nsCOMPtr<nsIAtom> name;
    PRInt32 nsid;

    nsresult rv = ParseAttributeString(aName, *getter_AddRefs(name), nsid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString tmp;
    rv = GetAttribute(nsid, name, tmp);

    *aReturn = rv == NS_CONTENT_ATTR_NOT_THERE ? PR_FALSE : PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::HasAttributeNS(const nsString& aNamespaceURI,
                             const nsString& aLocalName, PRBool* aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);

    nsCOMPtr<nsIAtom> name(dont_AddRef(NS_NewAtom(aLocalName)));
    PRInt32 nsid;

    gNameSpaceManager->GetNameSpaceID(aNamespaceURI, nsid);

    if (nsid == kNameSpaceID_Unknown) {
        // Unkonwn namespace means no attr...

        *aReturn = PR_FALSE;
        return NS_OK;
    }

    nsAutoString tmp;
    nsresult rv = GetAttribute(nsid, name, tmp);

    *aReturn = rv == NS_CONTENT_ATTR_NOT_THERE ? PR_FALSE : PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetElementsByAttribute(const nsString& aAttribute,
                                       const nsString& aValue,
                                       nsIDOMNodeList** aReturn)
{
    nsresult rv;
    nsRDFDOMNodeList* elements;
    rv = nsRDFDOMNodeList::Create(&elements);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create node list");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDOMNode> domElement;
    rv = QueryInterface(NS_GET_IID(nsIDOMNode), getter_AddRefs(domElement));
    if (NS_SUCCEEDED(rv)) {
        GetElementsByAttribute(domElement, aAttribute, aValue, elements);
    }

    *aReturn = elements;
    return NS_OK;
}


//----------------------------------------------------------------------
// nsIXMLContent interface

NS_IMETHODIMP
nsXULElement::SetContainingNameSpace(nsINameSpace* aNameSpace)
{
    nsresult rv;

    rv = EnsureSlots();
    if (NS_FAILED(rv)) return rv;

    mSlots->mNameSpace = dont_QueryInterface(aNameSpace);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetContainingNameSpace(nsINameSpace*& aNameSpace) const
{
    nsresult rv;

    if (NameSpace()) {
        // If we have a namespace, return it.
        aNameSpace = NameSpace();
        NS_ADDREF(aNameSpace);
        return NS_OK;
    }

    // Next, try our parent.
    nsCOMPtr<nsIContent> parent( dont_QueryInterface(mParent) );
    while (parent) {
        nsCOMPtr<nsIXMLContent> xml( do_QueryInterface(parent) );
        if (xml)
            return xml->GetContainingNameSpace(aNameSpace);

        nsCOMPtr<nsIContent> temp = parent;
        rv = temp->GetParent(*getter_AddRefs(parent));
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get parent");
        if (NS_FAILED(rv)) return rv;
    }

    // Allright, we walked all the way to the top of our containment
    // hierarchy and couldn't find a parent that supported
    // nsIXMLContent. If we're in a document, try to doc's root
    // element.
    if (mDocument) {
        nsCOMPtr<nsIContent> docroot
            = dont_AddRef( mDocument->GetRootContent() );

        // Wow! Nasty cast to get an unambiguous, non-const
        // nsISupports pointer. We want to make sure that we're not
        // the docroot (this would otherwise spin off into infinity).
        nsISupports* me = NS_STATIC_CAST(nsIStyledContent*, NS_CONST_CAST(nsXULElement*, this));

        if (docroot && !::SameCOMIdentity(docroot, me)) {
            nsCOMPtr<nsIXMLContent> xml( do_QueryInterface(docroot) );
            if (xml)
                return xml->GetContainingNameSpace(aNameSpace);
        }
    }

    aNameSpace = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetNameSpacePrefix(nsIAtom* aNameSpacePrefix)
{
    nsresult rv;

    rv = EnsureSlots();
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsINodeInfo> newNodeInfo;
    mSlots->mNodeInfo->PrefixChanged(aNameSpacePrefix,
                                     *getter_AddRefs(newNodeInfo));

    mSlots->mNodeInfo = newNodeInfo;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetNameSpacePrefix(nsIAtom*& aNameSpacePrefix) const
{
    return NodeInfo()->GetPrefixAtom(aNameSpacePrefix);
}

NS_IMETHODIMP
nsXULElement::MaybeTriggerAutoLink(nsIWebShell *aShell)
{
  return NS_OK;
}


//----------------------------------------------------------------------
// nsIXULContent interface

NS_IMETHODIMP
nsXULElement::PeekChildCount(PRInt32& aCount) const
{
    if (mChildren) {
        PRUint32 cnt;

        nsresult rv;
        rv = mChildren->Count(&cnt);
        if (NS_FAILED(rv)) return rv;

        aCount = PRInt32(cnt);
    }
    else {
        aCount = 0;
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetAnonymousState(PRBool& aState)
{
  aState = mIsAnonymous;
  return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetAnonymousState(PRBool aState)
{
  mIsAnonymous = aState;
  return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetLazyState(PRInt32 aFlags)
{
    mLazyState |= aFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::ClearLazyState(PRInt32 aFlags)
{
    mLazyState &= ~aFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetLazyState(PRInt32 aFlag, PRBool& aResult)
{
    aResult = (mLazyState & aFlag) ? PR_TRUE : PR_FALSE;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::AddScriptEventListener(nsIAtom* aName, const nsString& aValue, REFNSIID aIID)
{
    if (! mDocument)
        return NS_OK; // XXX

    nsresult rv;
    nsCOMPtr<nsIScriptContext> context;
    nsCOMPtr<nsIScriptGlobalObject> global;
    {
        mDocument->GetScriptGlobalObject(getter_AddRefs(global));

        // This can happen normally as part of teardown code.
        if (! global)
            return NS_OK;

        rv = global->GetContext(getter_AddRefs(context));
        if (NS_FAILED(rv)) return rv;
    }

    if (NodeInfo()->Equals(kWindowAtom)) {
        nsCOMPtr<nsIDOMEventReceiver> receiver = do_QueryInterface(global);
        if (! receiver)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIEventListenerManager> manager;
        rv = receiver->GetListenerManager(getter_AddRefs(manager));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIScriptObjectOwner> owner = do_QueryInterface(global);
            
        rv = manager->AddScriptEventListener(context, owner, aName, aValue, aIID, PR_FALSE);
    }
    else {
        nsCOMPtr<nsIEventListenerManager> manager;
        rv = GetListenerManager(getter_AddRefs(manager));
        if (NS_FAILED(rv)) return rv;

        rv = manager->AddScriptEventListener(context, this, aName, aValue, aIID, PR_TRUE);
    }

    return rv;
}


NS_IMETHODIMP
nsXULElement::ForceElementToOwnResource(PRBool aForce)
{
    nsresult rv;

    rv = EnsureSlots();
    if (NS_FAILED(rv)) return rv;

    if (aForce) {
        rv = GetResource(getter_AddRefs(mSlots->mOwnedResource));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        // drop reference
        mSlots->mOwnedResource = nsnull;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::InitTemplateRoot(nsIRDFCompositeDataSource* aDatabase,
                               nsIXULTemplateBuilder* aBuilder)
{
    // Sanity check
    NS_PRECONDITION(Database() == nsnull, "already initialized");
    if (Database())
        return NS_ERROR_ALREADY_INITIALIZED;

    nsresult rv;
    rv = EnsureSlots();
    if (NS_FAILED(rv)) return rv;

    mSlots->mDatabase = aDatabase;
    mSlots->mBuilder = aBuilder;
    return NS_OK;
}


//----------------------------------------------------------------------
// nsIDOMEventReceiver interface

NS_IMETHODIMP
nsXULElement::AddEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
    nsIEventListenerManager *manager;

    if (NS_OK == GetListenerManager(&manager)) {
        manager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
        NS_RELEASE(manager);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULElement::RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
    if (mListenerManager) {
        mListenerManager->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULElement::AddEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                                 PRBool aUseCapture)
{
  nsIEventListenerManager *manager;

  if (NS_OK == GetListenerManager(&manager)) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    manager->AddEventListenerByType(aListener, aType, flags);
    NS_RELEASE(manager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULElement::RemoveEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                                    PRBool aUseCapture)
{
  if (mListenerManager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    mListenerManager->RemoveEventListenerByType(aListener, aType, flags);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULElement::DispatchEvent(nsIDOMEvent* aEvent)
{
  // Obtain a presentation context
  PRInt32 count = mDocument->GetNumberOfShells();
  if (count == 0)
    return NS_OK;

  nsCOMPtr<nsIPresShell> shell = getter_AddRefs(mDocument->GetShellAt(0));
  
  // Retrieve the context
  nsCOMPtr<nsIPresContext> aPresContext;
  shell->GetPresContext(getter_AddRefs(aPresContext));

  nsCOMPtr<nsIEventStateManager> esm;
  if (NS_SUCCEEDED(aPresContext->GetEventStateManager(getter_AddRefs(esm)))) {
    return esm->DispatchNewEvent(NS_STATIC_CAST(nsIStyledContent*, this), aEvent);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULElement::GetListenerManager(nsIEventListenerManager** aResult)
{
    if (! mListenerManager) {
        nsresult rv;

        rv = nsComponentManager::CreateInstance(kEventListenerManagerCID,
                                                nsnull,
                                                kIEventListenerManagerIID,
                                                getter_AddRefs(mListenerManager));
        if (NS_FAILED(rv)) return rv;
    }

    *aResult = mListenerManager;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetNewListenerManager(nsIEventListenerManager **aResult)
{
    return nsComponentManager::CreateInstance(kEventListenerManagerCID,
                                        nsnull,
                                        kIEventListenerManagerIID,
                                        (void**) aResult);
}

NS_IMETHODIMP
nsXULElement::HandleEvent(nsIDOMEvent *aEvent)
{
  return DispatchEvent(aEvent);
}


//----------------------------------------------------------------------
// nsIScriptObjectOwner interface

NS_IMETHODIMP 
nsXULElement::GetScriptObject(nsIScriptContext* aContext, void** aScriptObject)
{
    nsresult rv = NS_OK;

    if (! mScriptObject) {
        // The actual script object that we create will depend on our
        // tag...
        nsresult (*PR_CALLBACK fn)(nsIScriptContext* aContext, nsISupports* aSupports, nsISupports* aParent, void** aReturn);

        nsCOMPtr<nsIAtom> tag;
        PRInt32 dummy;
        NS_WITH_SERVICE(nsIXBLService, xblService, "component://netscape/xbl", &rv);
        xblService->ResolveTag(NS_STATIC_CAST(nsIStyledContent*, this), &dummy, getter_AddRefs(tag));

        const char* rootname;
        if (tag.get() == kTreeAtom) {
            fn = NS_NewScriptXULTreeElement;
            rootname = "nsXULTreeElement::mScriptObject";
        }
        else if (tag.get() == kMenuListAtom) {
            fn = NS_NewScriptXULMenuListElement;
            rootname = "nsXULMenuListElement::mScriptObject";
        }
        else {
            fn = NS_NewScriptXULElement;
            rootname = "nsXULElement::mScriptObject";
        }

        // Create the script object; N.B. that if |mDocument| is null,
        // the script object's |parent| will refer to the class's
        // ctor. This is distinctly different from an element that
        // lives "in" the document when its script object is created.
        rv = fn(aContext, (nsIDOMXULElement*) this, mDocument, (void**) &mScriptObject);

        // Ensure that a reference exists to this element
        aContext->AddNamedReference((void*) &mScriptObject, mScriptObject, rootname);

        // See if we have a frame.
        if (mDocument) {
          nsCOMPtr<nsIPresShell> shell = getter_AddRefs(mDocument->GetShellAt(0));
          if (shell) {
            nsIFrame* frame;
            shell->GetPrimaryFrameFor(NS_STATIC_CAST(nsIStyledContent*, this), &frame);
            if (!frame) {
              // We must ensure that the XBL Binding is installed before we hand
              // back this object.
              nsCOMPtr<nsIBindingManager> bindingManager;
              mDocument->GetBindingManager(getter_AddRefs(bindingManager));
              nsCOMPtr<nsIXBLBinding> binding;
              bindingManager->GetBinding(NS_STATIC_CAST(nsIStyledContent*, this), getter_AddRefs(binding));
              if (!binding) {
                nsCOMPtr<nsIScriptGlobalObject> global;
                mDocument->GetScriptGlobalObject(getter_AddRefs(global));
                nsCOMPtr<nsIDOMViewCSS> viewCSS(do_QueryInterface(global));
                if (viewCSS) {
                  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
                  nsAutoString empty;
                  viewCSS->GetComputedStyle(this, empty, getter_AddRefs(cssDecl));
                  if (cssDecl) {
                    nsAutoString behavior; behavior.AssignWithConversion("behavior");
                    nsAutoString value;
                    cssDecl->GetPropertyValue(behavior, value);
                    if (!value.IsEmpty()) {
                      // We have a binding that must be installed.
                      xblService->LoadBindings(NS_STATIC_CAST(nsIStyledContent*, this), value, PR_FALSE);
                    }
                  }
                }
              }
            }
          }
        }
    }

    *aScriptObject = mScriptObject;
    
    return rv;
}

NS_IMETHODIMP 
nsXULElement::SetScriptObject(void *aScriptObject)
{
    // XXX Drop reference to previous object if there was one?
    mScriptObject = aScriptObject;
    return NS_OK;
}


//----------------------------------------------------------------------
// nsIScriptEventHandlerOwner interface

NS_IMETHODIMP
nsXULElement::GetCompiledEventHandler(nsIAtom *aName, void** aHandler)
{
    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheTests);
    *aHandler = nsnull;
    if (mPrototype) {
        for (PRInt32 i = 0; i < mPrototype->mNumAttributes; ++i) {
            nsXULPrototypeAttribute* attr = &(mPrototype->mAttributes[i]);

            if (attr->mNodeInfo->Equals(aName, kNameSpaceID_None)) {
                XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheHits);
                *aHandler = attr->mEventHandler;
                break;
            }
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::CompileEventHandler(nsIScriptContext* aContext,
                                  void* aTarget,
                                  nsIAtom *aName,
                                  const nsString& aBody,
                                  void** aHandler)
{
    nsresult rv;

    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheSets);

    nsCOMPtr<nsIScriptContext> context;
    JSObject* scopeObject;
    PRBool shared;

    if (mPrototype) {
        // It'll be shared amonst the instances of the prototype
        shared = PR_TRUE;

        // Use the prototype document's special context
        nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mDocument);
        NS_ASSERTION(xuldoc != nsnull, "mDocument is not an nsIXULDocument");
        if (! xuldoc)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIXULPrototypeDocument> protodoc;
        rv = xuldoc->GetMasterPrototype(getter_AddRefs(protodoc));
        if (NS_FAILED(rv)) return rv;

        NS_ASSERTION(protodoc != nsnull, "xul document has no prototype");
        if (! protodoc)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner = do_QueryInterface(protodoc);
        nsCOMPtr<nsIScriptGlobalObject> global;
        globalOwner->GetScriptGlobalObject(getter_AddRefs(global));
        NS_ASSERTION(global != nsnull, "prototype doc does not have a script global");
        if (! global)
            return NS_ERROR_UNEXPECTED;

        rv = global->GetContext(getter_AddRefs(context));
        if (NS_FAILED(rv)) return rv;

        // Use the prototype script's special scope object
        nsCOMPtr<nsIScriptObjectOwner> owner = do_QueryInterface(global);
        if (! owner)
            return NS_ERROR_UNEXPECTED;
    
        rv = owner->GetScriptObject(context, (void**) &scopeObject);
        if (NS_FAILED(rv)) return rv;
    }
    else {
        // We don't have a prototype; do a one-off compile.
        shared = PR_FALSE;
        context = aContext;
        scopeObject = NS_REINTERPRET_CAST(JSObject*, aTarget);
    }

    NS_ASSERTION(context != nsnull, "no script context");
    if (! context)
        return NS_ERROR_UNEXPECTED;

    // Compile the event handler
    rv = context->CompileEventHandler(scopeObject, aName, aBody, shared, aHandler);
    if (NS_FAILED(rv)) return rv;

    if (shared) {
        // If it's a shared handler, we need to bind the shared
        // function object to the real target.
        rv = aContext->BindCompiledEventHandler(aTarget, aName, *aHandler);
        if (NS_FAILED(rv)) return rv;
    }

    if (mPrototype) {
        // Remember the compiled event handler
        for (PRInt32 i = 0; i < mPrototype->mNumAttributes; ++i) {
            nsXULPrototypeAttribute* attr = &(mPrototype->mAttributes[i]);

            if (attr->mNodeInfo->Equals(aName, kNameSpaceID_None)) {
                XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheFills);
                attr->mEventHandler = *aHandler;

                JSContext *cx = (JSContext*) context->GetNativeContext();
                if (!cx)
                    return NS_ERROR_UNEXPECTED;

                rv = AddJSGCRoot(cx, &attr->mEventHandler, "nsXULPrototypeAttribute::mEventHandler");
                if (NS_FAILED(rv)) return rv;

                break;
            }
        }
    }

    return NS_OK;
}


//----------------------------------------------------------------------
// nsIJSScriptObject interface

PRBool
nsXULElement::AddProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
    return PR_TRUE;
}

PRBool
nsXULElement::DeleteProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
    return PR_TRUE;
}

PRBool
nsXULElement::GetProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
    return PR_TRUE;
}

PRBool
nsXULElement::SetProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
    // XXXwaterson do the event handlers here!
    return PR_TRUE;
}

PRBool
nsXULElement::EnumerateProperty(JSContext *aContext, JSObject *aObj)
{
    return PR_TRUE;
}


PRBool
nsXULElement::Resolve(JSContext *aContext, JSObject *aObj, jsval aID)
{
    return PR_TRUE;
}


PRBool
nsXULElement::Convert(JSContext *aContext, JSObject *aObj, jsval aID)
{
    return PR_TRUE;
}


void
nsXULElement::Finalize(JSContext *aContext, JSObject *aObj)
{
}


//----------------------------------------------------------------------
//
// nsIContent interface
//

NS_IMETHODIMP
nsXULElement::GetDocument(nsIDocument*& aResult) const
{
    aResult = mDocument;
    NS_IF_ADDREF(aResult);
    return NS_OK;
}
  
NS_IMETHODIMP
nsXULElement::SetDocument(nsIDocument* aDocument, PRBool aDeep, PRBool aCompileEventHandlers)
{
    nsresult rv;

    if (aDocument != mDocument) {
        nsCOMPtr<nsIXULDocument> rdfDoc;
        if (mDocument) {
            // Release the named reference to the script object so it can
            // be garbage collected.
            if (mScriptObject) {
                nsCOMPtr<nsIScriptGlobalObject> global;
                mDocument->GetScriptGlobalObject(getter_AddRefs(global));
                if (global) {
                    nsCOMPtr<nsIScriptContext> context;
                    global->GetContext(getter_AddRefs(context));
                    if (context) {
                        context->RemoveReference((void*) &mScriptObject, mScriptObject);
                    }
                }
            }
        }

        if (mDocument) {
          nsCOMPtr<nsIBindingManager> bindingManager;
          mDocument->GetBindingManager(getter_AddRefs(bindingManager));
          nsCOMPtr<nsIXBLBinding> binding;
          bindingManager->GetBinding(NS_STATIC_CAST(nsIStyledContent*, this), getter_AddRefs(binding));
          if (binding) {
            binding->ChangeDocument(mDocument, aDocument);
            bindingManager->SetBinding(NS_STATIC_CAST(nsIStyledContent*, this), nsnull);
            if (aDocument) {
              nsCOMPtr<nsIBindingManager> otherManager;
              aDocument->GetBindingManager(getter_AddRefs(otherManager));
              otherManager->SetBinding(NS_STATIC_CAST(nsIStyledContent*, this), binding);
            }
          }
        }

        mDocument = aDocument; // not refcounted

        if (mBoxObject) {
          nsCOMPtr<nsPIBoxObject> privateBox(do_QueryInterface(mBoxObject));
          if (privateBox)
            privateBox->SetDocument(mDocument);
        }

        if (mDocument) {
            // Add a named reference to the script object.
            if (mScriptObject) {
                nsCOMPtr<nsIScriptGlobalObject> global;
                mDocument->GetScriptGlobalObject(getter_AddRefs(global));
                if (global) {
                    nsCOMPtr<nsIScriptContext> context;
                    global->GetContext(getter_AddRefs(context));
                    if (context) {
                        context->AddNamedReference((void*) &mScriptObject, mScriptObject, "nsXULElement::mScriptObject");
                    }
                }
            }

            // When we SetDocument(), we're either adding an element
            // into the document that wasn't there before, or we're
            // moving the element from one document to
            // another. Regardless, we need to (re-)initialize several
            // attributes that are dependant on the document. Do that
            // now.
            PRInt32 count;
            GetAttributeCount(count);

            for (PRInt32 i = 0; i < count; ++i) {
                PRInt32 nameSpaceID;
                nsCOMPtr<nsIAtom> attr, prefix;
                GetAttributeNameAt(i, nameSpaceID, *getter_AddRefs(attr), *getter_AddRefs(prefix));

                PRBool reset = PR_FALSE;

                if (nameSpaceID == kNameSpaceID_None) {
                    if (aCompileEventHandlers) {
                        nsIID iid;
                        rv = gXULUtils->GetEventHandlerIID(attr, &iid, &reset);
                        if (NS_FAILED(rv)) return rv;
                    }

                    if (! reset) {
                        if ((attr.get() == kPopupAtom) ||
                            (attr.get() == kTooltipAtom) ||
                            (attr.get() == kContextAtom) ||
                            (attr.get() == kStyleAtom)) {
                            reset = PR_TRUE;
                        }
                    }
                }

                if (reset) {
                    nsAutoString value;
                    rv = GetAttribute(nameSpaceID, attr, value);
                    if (NS_FAILED(rv)) return rv;

                    rv = SetAttribute(nameSpaceID, attr, value, PR_FALSE);
                    if (NS_FAILED(rv)) return rv;
                }
            }
        }
    }

    if (aDeep && mChildren) {
        PRUint32 cnt;
        rv = mChildren->Count(&cnt);
        if (NS_FAILED(rv)) return rv;
        for (PRInt32 i = cnt - 1; i >= 0; --i) {
            // XXX this entire block could be more rigorous about
            // dealing with failure.
            nsCOMPtr<nsISupports> isupports = dont_AddRef( mChildren->ElementAt(i) );

            NS_ASSERTION(isupports != nsnull, "null ptr");
            if (! isupports)
                continue;

            nsCOMPtr<nsIContent> child = do_QueryInterface(isupports);
            NS_ASSERTION(child != nsnull, "not an nsIContent");
            if (! child)
                continue;

            child->SetDocument(aDocument, aDeep, aCompileEventHandlers);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetParent(nsIContent*& aResult) const
{
    aResult = mParent;
    NS_IF_ADDREF(mParent);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetParent(nsIContent* aParent)
{
    mParent = aParent; // no refcount
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::CanContainChildren(PRBool& aResult) const
{
    // XXX Hmm -- not sure if this is unilaterally true...
    aResult = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::ChildCount(PRInt32& aResult) const
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated())) {
        aResult = 0;
        return rv;
    }

    return PeekChildCount(aResult);
}

NS_IMETHODIMP
nsXULElement::ChildAt(PRInt32 aIndex, nsIContent*& aResult) const
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated())) {
        aResult = nsnull;
        return rv;
    }

    aResult = nsnull;
    if (! mChildren)
        return NS_OK;

    nsCOMPtr<nsISupports> isupports = dont_AddRef( mChildren->ElementAt(aIndex) );
    if (! isupports)
        return NS_OK; // It's okay to ask for an element off the end.

    nsIContent* content;
    rv = isupports->QueryInterface(kIContentIID, (void**) &content);
    if (NS_FAILED(rv)) return rv;

    aResult = content; // take the AddRef() from the QI
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated())) {
        aResult = -1;
        return rv;
    }

    aResult = (mChildren) ? (mChildren->IndexOf(aPossibleChild)) : (-1);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify)
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated()))
        return rv;

    NS_PRECONDITION(nsnull != aKid, "null ptr");

    if (! mChildren) {
        rv = NS_NewISupportsArray(getter_AddRefs(mChildren));
        if (NS_FAILED(rv)) return rv;
    }

    // Make sure that we're not trying to insert the same child
    // twice. If we do, the DOM APIs (e.g., GetNextSibling()), will
    // freak out.
    NS_ASSERTION(mChildren->IndexOf(aKid) < 0, "element is already a child");

    PRBool insertOk = mChildren->InsertElementAt(aKid, aIndex);/* XXX fix up void array api to use nsresult's*/
    if (insertOk) {
        aKid->SetParent(NS_STATIC_CAST(nsIStyledContent*, this));
        //nsRange::OwnerChildInserted(this, aIndex);

        // N.B. that this is "shallow"!
        aKid->SetDocument(mDocument, PR_FALSE, PR_TRUE);

        if (aNotify && mDocument) {
                mDocument->ContentInserted(NS_STATIC_CAST(nsIStyledContent*, this), aKid, aIndex);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify)
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated()))
        return rv;

    NS_PRECONDITION(nsnull != mChildren, "illegal value");
    if (! mChildren)
        return NS_ERROR_ILLEGAL_VALUE;

    NS_PRECONDITION(nsnull != aKid, "null ptr");
    if (! aKid)
        return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsISupports> isupports = dont_AddRef( mChildren->ElementAt(aIndex) );
    if (! isupports)
        return NS_OK; // XXX No kid at specified index; just silently ignore?

    nsCOMPtr<nsIContent> oldKid = do_QueryInterface(isupports);
    NS_ASSERTION(oldKid != nsnull, "old kid not nsIContent");
    if (! oldKid)
        return NS_ERROR_FAILURE;

    if (oldKid.get() == aKid)
        return NS_OK;

    PRBool replaceOk = mChildren->ReplaceElementAt(aKid, aIndex);
    if (replaceOk) {
        aKid->SetParent(NS_STATIC_CAST(nsIStyledContent*, this));
        //nsRange::OwnerChildReplaced(this, aIndex, oldKid);

        // N.B. that we only do a "shallow" SetDocument()
        // here. Callers beware!
        aKid->SetDocument(mDocument, PR_FALSE, PR_TRUE);

        if (aNotify && mDocument) {
            mDocument->ContentReplaced(NS_STATIC_CAST(nsIStyledContent*, this), oldKid, aKid, aIndex);
        }

        // This will cause the script object to be unrooted for each
        // element in the subtree.
        oldKid->SetDocument(nsnull, PR_TRUE, PR_TRUE);

        // We've got no mo' parent.
        oldKid->SetParent(nsnull);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::AppendChildTo(nsIContent* aKid, PRBool aNotify)
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated()))
        return rv;

    NS_PRECONDITION((nsnull != aKid) && (aKid != NS_STATIC_CAST(nsIStyledContent*, this)), "null ptr");

    if (!mChildren) {
        rv = NS_NewISupportsArray(getter_AddRefs(mChildren));
        if (NS_FAILED(rv)) return rv;
    }

    PRBool appendOk = mChildren->AppendElement(aKid);
    if (appendOk) {
        aKid->SetParent(NS_STATIC_CAST(nsIStyledContent*, this));
        // ranges don't need adjustment since new child is at end of list

        // N.B. that this is only "shallow". Callers beware!
        aKid->SetDocument(mDocument, PR_FALSE, PR_TRUE);

        if (aNotify && mDocument) {
            PRUint32 cnt;
            rv = mChildren->Count(&cnt);
            if (NS_FAILED(rv)) return rv;
            
            mDocument->ContentAppended(NS_STATIC_CAST(nsIStyledContent*, this), cnt - 1);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::RemoveChildAt(PRInt32 aIndex, PRBool aNotify)
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated()))
        return rv;

    NS_PRECONDITION(mChildren != nsnull, "illegal value");
    if (! mChildren)
        return NS_ERROR_ILLEGAL_VALUE;

    nsCOMPtr<nsISupports> isupports = dont_AddRef( mChildren->ElementAt(aIndex) );
    if (! isupports)
        return NS_OK; // XXX No kid at specified index; just silently ignore?

    nsCOMPtr<nsIContent> oldKid = do_QueryInterface(isupports);
    NS_ASSERTION(oldKid != nsnull, "old kid not nsIContent");
    if (! oldKid)
        return NS_ERROR_FAILURE;

    // On the removal of a <treeitem>, <treechildren>, or <treecell> element,
    // the possibility exists that some of the items in the removed subtree
    // are selected (and therefore need to be deselected). We need to account for this.
    nsCOMPtr<nsIAtom> tag;
    oldKid->GetTag(*getter_AddRefs(tag));
    if (tag && (tag.get() == kTreeChildrenAtom || tag.get() == kTreeItemAtom ||
                tag.get() == kTreeCellAtom)) {
      // This is the nasty case. We have (potentially) a slew of selected items
      // and cells going away.
      // First, retrieve the tree.
      nsCOMPtr<nsIDOMXULTreeElement> treeElement;
      rv = GetParentTree(getter_AddRefs(treeElement));
      if (treeElement) {
        nsCOMPtr<nsIDOMNodeList> itemList;
        treeElement->GetSelectedItems(getter_AddRefs(itemList));

        nsCOMPtr<nsIDOMNode> parentKid = do_QueryInterface(oldKid);
        PRBool fireSelectionHandler = PR_FALSE;
        if (itemList) {
          // Iterate over all of the items and find out if they are contained inside
          // the removed subtree.
          PRUint32 length;
          itemList->GetLength(&length);
          for (PRUint32 i = 0; i < length; i++) {
            nsCOMPtr<nsIDOMNode> node;
            itemList->Item(i, getter_AddRefs(node));
            if (IsAncestor(parentKid, node)) {
              nsCOMPtr<nsIContent> content = do_QueryInterface(node);
              content->UnsetAttribute(kNameSpaceID_None, kSelectedAtom, PR_FALSE);
              length--;
              i--;
              fireSelectionHandler = PR_TRUE;
            }
          }
        }

        if (fireSelectionHandler) {
          nsCOMPtr<nsIXULTreeContent> tree = do_QueryInterface(treeElement);
          if (tree) {
            tree->FireOnSelectHandler();
          }
        }
      }
    }

    if (oldKid) {
        nsIDocument* doc = mDocument;
        PRBool removeOk = mChildren->RemoveElementAt(aIndex);
        //nsRange::OwnerChildRemoved(this, aIndex, oldKid);
        if (aNotify && removeOk && mDocument) {
            doc->ContentRemoved(NS_STATIC_CAST(nsIStyledContent*, this), oldKid, aIndex);
        }

        // This will cause the script object to be unrooted for each
        // element in the subtree.
        oldKid->SetDocument(nsnull, PR_TRUE, PR_TRUE);

        // We've got no mo' parent.
        oldKid->SetParent(nsnull);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::IsSynthetic(PRBool& aResult)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsXULElement::GetNameSpaceID(PRInt32& aNameSpaceID) const
{
    return NodeInfo()->GetNamespaceID(aNameSpaceID);
}

NS_IMETHODIMP
nsXULElement::GetTag(nsIAtom*& aResult) const
{
    return NodeInfo()->GetNameAtom(aResult);
}

NS_IMETHODIMP 
nsXULElement::ParseAttributeString(const nsString& aStr, 
                                     nsIAtom*& aName, 
                                     PRInt32& aNameSpaceID)
{
static char kNameSpaceSeparator = ':';

    nsAutoString prefix;
    nsAutoString name(aStr);
    PRInt32 nsoffset = name.FindChar(kNameSpaceSeparator);
    if (-1 != nsoffset) {
        name.Left(prefix, nsoffset);
        name.Cut(0, nsoffset+1);
    }

    // Figure out the namespace ID, defaulting to none if there is no
    // namespace prefix.
    aNameSpaceID = kNameSpaceID_None;
    if (0 < prefix.Length()) {
        nsCOMPtr<nsIAtom> nameSpaceAtom = dont_AddRef(NS_NewAtom(prefix));
        if (! nameSpaceAtom)
            return NS_ERROR_FAILURE;

        nsresult rv;
        nsCOMPtr<nsINameSpace> ns;
        rv = GetContainingNameSpace(*getter_AddRefs(ns));
        if (NS_FAILED(rv)) return rv;

        if (ns) {
            rv = ns->FindNameSpaceID(nameSpaceAtom, aNameSpaceID);
            if (NS_FAILED(rv)) return rv;
        }
    }

    aName = NS_NewAtom(name);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetNameSpacePrefixFromId(PRInt32 aNameSpaceID, 
                                         nsIAtom*& aPrefix)
{
    nsresult rv;

    nsCOMPtr<nsINameSpace> ns;
    rv = GetContainingNameSpace(*getter_AddRefs(ns));
    if (NS_FAILED(rv)) return rv;

    if (ns) {
        return ns->FindNameSpacePrefix(aNameSpaceID, aPrefix);
    }

    aPrefix = nsnull;
    return NS_OK;
}



// XXX attribute code swiped from nsGenericContainerElement
// this class could probably just use nsGenericContainerElement
// needed to maintain attribute namespace ID as well as ordering
NS_IMETHODIMP 
nsXULElement::SetAttribute(PRInt32 aNameSpaceID,
                           nsIAtom* aName, 
                           const nsString& aValue,
                           PRBool aNotify)
{
    NS_ASSERTION(kNameSpaceID_Unknown != aNameSpaceID, "must have name space ID");
    if (kNameSpaceID_Unknown == aNameSpaceID)
        return NS_ERROR_ILLEGAL_VALUE;

    NS_ASSERTION(nsnull != aName, "must have attribute name");
    if (nsnull == aName)
        return NS_ERROR_NULL_POINTER;

    nsresult rv = NS_OK;


    if (! Attributes()) {
        rv = EnsureSlots();
        if (NS_FAILED(rv)) return rv;

        // Since EnsureSlots() may have triggered mSlots->mAttributes construction,
        // we need to check _again_ before creating attributes.
        if (! Attributes()) {
            rv = nsXULAttributes::Create(NS_STATIC_CAST(nsIStyledContent*, this), &(mSlots->mAttributes));
            if (NS_FAILED(rv)) return rv;
        }
    }

    // XXX Class and Style attribute setting should be checking for the XUL namespace!

    // Check to see if the CLASS attribute is being set.  If so, we need to rebuild our
    // class list.
    if ((aNameSpaceID == kNameSpaceID_None) && (aName == kClassAtom)) {
        Attributes()->UpdateClassList(aValue);
    }

    // Check to see if the STYLE attribute is being set.  If so, we need to create a new
    // style rule based off the value of this attribute, and we need to let the document
    // know about the StyleRule change.
    if ((aNameSpaceID == kNameSpaceID_None) && (aName == kStyleAtom) && (mDocument != nsnull)) {
        nsCOMPtr <nsIURI> docURL;
        mDocument->GetBaseURL(*getter_AddRefs(docURL));
        Attributes()->UpdateStyleRule(docURL, aValue);
        // XXX Some kind of special document update might need to happen here.
    }

    // Need to check for the SELECTED attribute
    // being set.  If we're a <treeitem>, <treerow>, or <treecell>, the act of
    // setting these attributes forces us to update our selected arrays.
    nsCOMPtr<nsIAtom> tag;
    GetTag(*getter_AddRefs(tag));
    if (mDocument && (aNameSpaceID == kNameSpaceID_None)) {
      // See if we're a treeitem atom.
      nsCOMPtr<nsIRDFNodeList> nodeList;
      if (tag && (tag.get() == kTreeItemAtom) && (aName == kSelectedAtom)) {
        nsCOMPtr<nsIDOMXULTreeElement> treeElement;
        GetParentTree(getter_AddRefs(treeElement));
        if (treeElement) {
          nsCOMPtr<nsIDOMNodeList> nodes;
          treeElement->GetSelectedItems(getter_AddRefs(nodes));
          nodeList = do_QueryInterface(nodes);
        }
      }
      
      if (nodeList) {
        // Append this node to the list.
        nodeList->AppendNode(this);
      }
    }

   
    // Check to see if the POPUP attribute is being set.  If so, we need to attach
    // a new instance of our popup handler to the node.
    if (mDocument && (aNameSpaceID == kNameSpaceID_None) && 
        (aName == kPopupAtom || aName == kTooltipAtom || aName == kContextAtom))
    {
        AddPopupListener(aName);
    }

    // XXX need to check if they're changing an event handler: if so, then we need
    // to unhook the old one.
    
    nsXULAttribute* attr;
    PRInt32 i = 0;
    PRInt32 count = Attributes()->Count();
    while (i < count) {
        attr = Attributes()->ElementAt(i);
        if (attr->GetNodeInfo()->Equals(aName, aNameSpaceID))
            break;
        i++;
    }

    if (i < count) {
        attr->SetValueInternal(aValue);
    }
    else {
        // didn't find it

        nsCOMPtr<nsIAtom> prefix;
        GetNameSpacePrefixFromId(aNameSpaceID, *getter_AddRefs(prefix));

        nsCOMPtr<nsINodeInfoManager> nimgr;
        NodeInfo()->GetNodeInfoManager(*getter_AddRefs(nimgr));
        NS_ENSURE_TRUE(nimgr, NS_ERROR_FAILURE);

        nsCOMPtr<nsINodeInfo> nodeInfo;
        nimgr->GetNodeInfo(aName, prefix, aNameSpaceID,
                           *getter_AddRefs(nodeInfo));

        rv = nsXULAttribute::Create(NS_STATIC_CAST(nsIStyledContent*, this),
                                    nodeInfo, aValue, &attr);
        if (NS_FAILED(rv)) return rv;

        // transfer ownership here... 
        Attributes()->AppendElement(attr);
    }

    // Check to see if this is an event handler, and add a script
    // listener if necessary.
    {
        nsIID iid;
        PRBool found;
        rv = gXULUtils->GetEventHandlerIID(aName, &iid, &found);
        if (NS_FAILED(rv)) return rv;

        if (found) {
            rv = AddScriptEventListener(aName, aValue, iid);
            if (NS_FAILED(rv)) return rv;
        }
    }

    // Notify any broadcasters that are listening to this node.
    if (BroadcastListeners())
    {
        nsAutoString attribute;
        aName->ToString(attribute);
        count = BroadcastListeners()->Count();
        for (i = 0; i < count; i++) {
            XULBroadcastListener* xulListener =
                NS_REINTERPRET_CAST(XULBroadcastListener*, BroadcastListeners()->ElementAt(i));

            if (xulListener->ObservingAttribute(attribute) && 
               (aName != kIdAtom)) {
                // XXX Should have a function that knows which attributes are special.
                // First we set the attribute in the observer.
                xulListener->mListener->SetAttribute(attribute, aValue);
                ExecuteOnBroadcastHandler(xulListener->mListener, attribute);
            }
        }
    }

    if (NS_SUCCEEDED(rv) && mDocument) {
      nsCOMPtr<nsIBindingManager> bindingManager;
      mDocument->GetBindingManager(getter_AddRefs(bindingManager));
      nsCOMPtr<nsIXBLBinding> binding;
      bindingManager->GetBinding(NS_STATIC_CAST(nsIStyledContent*, this), getter_AddRefs(binding));
      if (binding)
        binding->AttributeChanged(aName, aNameSpaceID, PR_FALSE);
      
      if (aNotify)
        mDocument->AttributeChanged(NS_STATIC_CAST(nsIStyledContent*, this), aNameSpaceID, aName, NS_STYLE_HINT_UNKNOWN);
    }

    return rv;
}

NS_IMETHODIMP
nsXULElement::SetAttribute(nsINodeInfo* aNodeInfo, 
                           const nsString& aValue,
                           PRBool aNotify)
{
  NS_ENSURE_ARG_POINTER(aNodeInfo);

  nsCOMPtr<nsIAtom> atom;
  PRInt32 nsid;

  aNodeInfo->GetNameAtom(*getter_AddRefs(atom));
  aNodeInfo->GetNamespaceID(nsid);

  // We still rely on the old way of setting the attribute.

  return SetAttribute(nsid, atom, aValue, aNotify);
}

NS_IMETHODIMP
nsXULElement::GetAttribute(PRInt32 aNameSpaceID,
                           nsIAtom* aName,
                           nsString& aResult) const
{
    nsCOMPtr<nsIAtom> prefix;
    return GetAttribute(aNameSpaceID, aName, *getter_AddRefs(prefix), aResult);
}

NS_IMETHODIMP
nsXULElement::GetAttribute(PRInt32 aNameSpaceID,
                           nsIAtom* aName,
                           nsIAtom*& aPrefix,
                           nsString& aResult) const
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    if (nsnull == aName) {
        return NS_ERROR_NULL_POINTER;
    }

    nsresult rv = NS_CONTENT_ATTR_NOT_THERE;

    if (mSlots && mSlots->mAttributes) {
        PRInt32 count = Attributes()->Count();
        for (PRInt32 i = 0; i < count; i++) {
            nsXULAttribute* attr = NS_REINTERPRET_CAST(nsXULAttribute*, Attributes()->ElementAt(i));
            nsINodeInfo *ni = attr->GetNodeInfo();
            if ((ni->NamespaceEquals(aNameSpaceID) ||
                 (aNameSpaceID == kNameSpaceID_Unknown) ||
                 (aNameSpaceID == kNameSpaceID_None)) && ni->Equals(aName)) {
                ni->GetPrefixAtom(aPrefix);
                attr->GetValue(aResult);
                rv = aResult.Length() ? NS_CONTENT_ATTR_HAS_VALUE : NS_CONTENT_ATTR_NO_VALUE;
                break;
            }
        }
    }
    else if (mPrototype) {
        PRInt32 count = mPrototype->mNumAttributes;
        for (PRInt32 i = 0; i < count; i++) {
            nsXULPrototypeAttribute* attr = &(mPrototype->mAttributes[i]);
            nsINodeInfo *ni = attr->mNodeInfo;
            if ((ni->NamespaceEquals(aNameSpaceID) ||
                 (aNameSpaceID == kNameSpaceID_Unknown) ||
                 (aNameSpaceID == kNameSpaceID_None)) && ni->Equals(aName)) {
                ni->GetPrefixAtom(aPrefix);
                aResult = attr->mValue;
                rv = aResult.Length() ? NS_CONTENT_ATTR_HAS_VALUE : NS_CONTENT_ATTR_NO_VALUE;
                break;
            }
        }
    }
    else {
        aResult.Truncate();
    }

    return rv;
}

NS_IMETHODIMP
nsXULElement::UnsetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName, PRBool aNotify)
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    if (nsnull == aName)
        return NS_ERROR_NULL_POINTER;

    // If we're unsetting an attribute, we actually need to do the
    // copy _first_ so that we can remove the value in the heavyweight
    // element.
    nsresult rv;
    rv = EnsureSlots();
    if (NS_FAILED(rv)) return rv;

    // It's possible that somebody has tried to 'unset' an attribute
    // on an element with _no_ attributes, in which case we'll have
    // paid the cost to make the thing heavyweight, but might still
    // not have created an 'mAttributes' in the slots. Test here, as
    // later code will dereference it...
    if (! Attributes())
        return NS_OK;

    // Check to see if the CLASS attribute is being unset.  If so, we need to
    // delete our class list.
    // XXXbe fuse common (mDocument && aNameSpaceId == kNameSpaceID_None)
    if (mDocument &&
        (aNameSpaceID == kNameSpaceID_None) &&
        (aName == kClassAtom)) {
        Attributes()->UpdateClassList(nsAutoString());
    }
    
    if (mDocument &&
        (aNameSpaceID == kNameSpaceID_None) &&
        aName == kStyleAtom) {

        nsCOMPtr <nsIURI> docURL;
        mDocument->GetBaseURL(*getter_AddRefs(docURL));

        Attributes()->UpdateStyleRule(docURL, nsAutoString());
        // XXX Some kind of special document update might need to happen here.
    }

    // Need to check for the SELECTED attribute
    // being unset.  If we're a <treeitem>, <treerow>, or <treecell>, the act of
    // unsetting these attributes forces us to update our selected arrays.
    nsCOMPtr<nsIAtom> tag;
    GetTag(*getter_AddRefs(tag));
    if (aNameSpaceID == kNameSpaceID_None) {
        // See if we're a treeitem atom.
        // XXX Forgive me father, for I know exactly what I do, and I'm
        // doing it anyway.  Need to make an nsIRDFNodeList interface that
        // I can QI to for additions and removals of nodes.  For now
        // do an evil cast.
        nsCOMPtr<nsIRDFNodeList> nodeList;
        if (tag && (tag.get() == kTreeItemAtom) && (aName == kSelectedAtom)) {
            nsCOMPtr<nsIDOMXULTreeElement> treeElement;
            GetParentTree(getter_AddRefs(treeElement));
            if (treeElement) {
                nsCOMPtr<nsIDOMNodeList> nodes;
                treeElement->GetSelectedItems(getter_AddRefs(nodes));
                nodeList = do_QueryInterface(nodes);
            }
        }
        
        if (nodeList) {
            // Remove this node from the list.
            nodeList->RemoveNode(this);
        }
    }

    // XXX Know how to remove POPUP event listeners when an attribute is unset?

    nsAutoString oldValue;

    rv = NS_OK;
    PRBool successful = PR_FALSE;
    if (Attributes()) {
        PRInt32 count = Attributes()->Count();
        PRInt32 i;
        for (i = 0; i < count; i++) {
            nsXULAttribute* attr = NS_REINTERPRET_CAST(nsXULAttribute*, Attributes()->ElementAt(i));
            if (attr->GetNodeInfo()->Equals(aName, aNameSpaceID)) {
                attr->GetValue(oldValue);
                Attributes()->RemoveElementAt(i);
                NS_RELEASE(attr);
                successful = PR_TRUE;
                break;
            }
        }
    }

    // XUL Only. Find out if we have a broadcast listener for this element.
    if (successful) {
        // Check to see if the OBSERVES attribute is being unset.  If so, we
        // need to remove ourselves completely.
        if (mDocument &&
            (aNameSpaceID == kNameSpaceID_None) && 
            (aName == kObservesAtom))
        {
            // Do a getElementById to retrieve the broadcaster.
            nsCOMPtr<nsIDOMElement> broadcaster;
            nsCOMPtr<nsIDOMXULDocument> domDoc = do_QueryInterface(mDocument);
            domDoc->GetElementById(oldValue, getter_AddRefs(broadcaster));
            if (broadcaster) {
                nsCOMPtr<nsIDOMXULElement> xulBroadcaster = do_QueryInterface(broadcaster);
                if (xulBroadcaster) {
                    xulBroadcaster->RemoveBroadcastListener(NS_ConvertASCIItoUCS2("*"), this);
                }
            }
        }

        if (BroadcastListeners()) {
            PRInt32 count = BroadcastListeners()->Count();
            for (PRInt32 i = 0; i < count; i++) {
                XULBroadcastListener* xulListener =
                    NS_REINTERPRET_CAST(XULBroadcastListener*, BroadcastListeners()->ElementAt(i));

                nsAutoString str;
                aName->ToString(str);
                if (xulListener->ObservingAttribute(str) && 
                   (aName != kIdAtom)) {
                    // XXX Should have a function that knows which attributes are special.
                    // Unset the attribute in the broadcast listener.
                    nsCOMPtr<nsIDOMElement> element;
                    element = do_QueryInterface(xulListener->mListener);
                    if (element)
                        element->RemoveAttribute(str);
                }
            }
        }
     
        // Notify document
        if (NS_SUCCEEDED(rv) && mDocument) {
          nsCOMPtr<nsIBindingManager> bindingManager;
          mDocument->GetBindingManager(getter_AddRefs(bindingManager));
          nsCOMPtr<nsIXBLBinding> binding;
          bindingManager->GetBinding(NS_STATIC_CAST(nsIStyledContent*, this), getter_AddRefs(binding));
          if (binding)
            binding->AttributeChanged(aName, aNameSpaceID, PR_TRUE);

          if (aNotify)
            mDocument->AttributeChanged(NS_STATIC_CAST(nsIStyledContent*, this),
                                        aNameSpaceID, aName,
                                        NS_STYLE_HINT_UNKNOWN);
        }
    }

    // End XUL Only Code
    return rv;
}

NS_IMETHODIMP
nsXULElement::GetAttributeNameAt(PRInt32 aIndex,
                                 PRInt32& aNameSpaceID,
                                 nsIAtom*& aName,
                                 nsIAtom*& aPrefix) const
{
    if (Attributes()) {
        nsXULAttribute* attr = NS_REINTERPRET_CAST(nsXULAttribute*, Attributes()->ElementAt(aIndex));
        if (nsnull != attr) {
            attr->GetNodeInfo()->GetNamespaceID(aNameSpaceID);
            attr->GetNodeInfo()->GetNameAtom(aName);
            attr->GetNodeInfo()->GetPrefixAtom(aPrefix);
            return NS_OK;
        }
    }
    else if (mPrototype) {
        if (aIndex >= 0 && aIndex < mPrototype->mNumAttributes) {
            nsXULPrototypeAttribute* attr = &(mPrototype->mAttributes[aIndex]);
            attr->mNodeInfo->GetNamespaceID(aNameSpaceID);
            attr->mNodeInfo->GetNameAtom(aName);
            attr->mNodeInfo->GetPrefixAtom(aPrefix);
            return NS_OK;
        }
    }

    aNameSpaceID = kNameSpaceID_None;
    aName = nsnull;
    aPrefix = nsnull;
    return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
nsXULElement::GetAttributeCount(PRInt32& aResult) const
{
    nsresult rv = NS_OK;
    if (Attributes()) {
        aResult = Attributes()->Count();
    }
    else if (mPrototype) {
        aResult = mPrototype->mNumAttributes;
    }
    else {
        aResult = 0;
    }

    return rv;
}


static void
rdf_Indent(FILE* out, PRInt32 aIndent)
{
    for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);
}

NS_IMETHODIMP
nsXULElement::List(FILE* out, PRInt32 aIndent) const
{
    NS_PRECONDITION(mDocument != nsnull, "bad content");

    nsresult rv;
    {
        rdf_Indent(out, aIndent);
        fputs("<XUL", out);
        if (mSlots) fputs("*", out);
        fputs(" ", out);

        PRInt32 namespaceID;
        NodeInfo()->GetNamespaceID(namespaceID);

        if (namespaceID == kNameSpaceID_Unknown) {
            fputs("unknown:", out);
        }
        
        nsAutoString as;
        NodeInfo()->GetQualifiedName(as);
        fputs(as, out);
    }

    {
        PRInt32 nattrs;

        if (NS_SUCCEEDED(rv = GetAttributeCount(nattrs))) {
            for (PRInt32 i = 0; i < nattrs; ++i) {
                nsIAtom* attr = nsnull;
                nsCOMPtr<nsIAtom> prefix;
                PRInt32 nameSpaceID;
                GetAttributeNameAt(i, nameSpaceID, attr, *getter_AddRefs(prefix));

                nsAutoString v;
                GetAttribute(nameSpaceID, attr, v);

                fputs(" ", out);

                nsAutoString s;

                if (prefix) {
                    prefix->ToString(s);

                    fputs(s, out);
                    fputs(":", out);
                }

                attr->ToString(s);
                NS_RELEASE(attr);

                fputs(s, out);
                fputs("=", out);
                fputs(v, out);
            }
        }

        if (NS_FAILED(rv))
            return rv;
    }

    fputs(">\n", out);

    {
        PRInt32 nchildren;
        if (NS_FAILED(rv = ChildCount(nchildren)))
            return rv;

        for (PRInt32 i = 0; i < nchildren; ++i) {
            nsIContent* child;
            if (NS_FAILED(rv = ChildAt(i, child)))
                return rv;

            rv = child->List(out, aIndent + 1);
            NS_RELEASE(child);

            if (NS_FAILED(rv))
                return rv;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::BeginConvertToXIF(nsIXIFConverter* aConverter) const
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULElement::ConvertContentToXIF(nsIXIFConverter* aConverter) const
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULElement::FinishConvertToXIF(nsIXIFConverter* aConverter) const
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULElement::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
    if (!aResult) {
        return NS_ERROR_NULL_POINTER;
    }
    PRUint32 sum = 0;
#ifdef DEBUG
    sum += (PRUint32) sizeof(*this);
#endif
    *aResult = sum;
    return NS_OK;
}



NS_IMETHODIMP 
nsXULElement::HandleDOMEvent(nsIPresContext* aPresContext,
                                       nsEvent* aEvent,
                                       nsIDOMEvent** aDOMEvent,
                                       PRUint32 aFlags,
                                       nsEventStatus* aEventStatus)
{
    nsresult ret = NS_OK;
  
    nsIDOMEvent* domEvent = nsnull;
    if (NS_EVENT_FLAG_INIT & aFlags) {
        aDOMEvent = &domEvent;
        aEvent->flags = aFlags;
        aFlags &= ~(NS_EVENT_FLAG_CANT_BUBBLE | NS_EVENT_FLAG_CANT_CANCEL);
        // In order for the event to have a proper target for events that don't go through
        // the presshell (onselect, oncommand, oncreate, ondestroy) we need to set our target
        // ourselves. Also, key sets and menus don't have frames and therefore need their
        // targets explicitly specified. 
        // 
        // We need this for drag&drop as well since the mouse may have moved into a different
        // frame between the initial mouseDown and the generation of the drag gesture.
        // Obviously, the target should be the content/frame where the mouse was depressed,
        // not one computed by the current mouse location.
        nsAutoString tagName;
        NodeInfo()->GetName(tagName); // Local name only
        if (aEvent->message == NS_MENU_ACTION || aEvent->message == NS_MENU_CREATE ||
            aEvent->message == NS_MENU_DESTROY || aEvent->message == NS_FORM_SELECTED ||
            aEvent->message == NS_XUL_BROADCAST || aEvent->message == NS_XUL_COMMAND_UPDATE ||
            aEvent->message == NS_DRAGDROP_GESTURE ||
            tagName.EqualsWithConversion("menu") || tagName.EqualsWithConversion("menuitem") || tagName.EqualsWithConversion("menulist") ||
            tagName.EqualsWithConversion("menubar") || tagName.EqualsWithConversion("menupopup") || tagName.EqualsWithConversion("key") || tagName.EqualsWithConversion("keyset")) {
            nsCOMPtr<nsIEventListenerManager> listenerManager;
            if (NS_FAILED(ret = GetListenerManager(getter_AddRefs(listenerManager)))) {
                NS_ERROR("Unable to instantiate a listener manager on this event.");
                return ret;
            }
            nsAutoString empty;
            if (NS_FAILED(ret = listenerManager->CreateEvent(aPresContext, aEvent, empty, aDOMEvent))) {
                NS_ERROR("This event will fail without the ability to create the event early.");
                return ret;
            }
            
            // We need to explicitly set the target here, because the
            // DOM implementation will try to compute the target from
            // the frame. If we don't have a frame (e.g., we're a
            // menu), then that breaks.
            nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(domEvent);
            if (privateEvent) {
              privateEvent->SetTarget(this);
            }
            else return NS_ERROR_FAILURE;
        }
    }
  
    // Node capturing stage
    if (NS_EVENT_FLAG_BUBBLE != aFlags) {
        if (mParent) {
            PRBool proceed = PR_TRUE;
            if (mIsAnonymous) {
              PRBool parentState;
              nsCOMPtr<nsIXULContent> parent = do_QueryInterface(mParent);
              if (parent) {
                parent->GetAnonymousState(parentState);
                if (!parentState)
                  proceed = PR_FALSE;
              }
              else proceed = PR_FALSE; // Assume that the HTML Content is not anonymous
                                       // XXX Will need to do better for XBL.
            }

            // Pass off to our parent.
            if (proceed)
              mParent->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                      NS_EVENT_FLAG_CAPTURE, aEventStatus);
        }
        else if (mDocument != nsnull) {
            ret = mDocument->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                            NS_EVENT_FLAG_CAPTURE, aEventStatus);
        }
    }
    

    //Local handling stage
    if (mListenerManager && !(aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
        aEvent->flags |= aFlags;
        mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, this, aFlags, aEventStatus);
        aEvent->flags &= ~aFlags;
    }

    //Bubbling stage
    if (NS_EVENT_FLAG_CAPTURE != aFlags) {
        if (mParent != nsnull) {
          PRBool proceed = PR_TRUE;
          if (mIsAnonymous) {
            PRBool parentState;
            nsCOMPtr<nsIXULContent> parent = do_QueryInterface(mParent);
            if (parent) {
              parent->GetAnonymousState(parentState);
              if (!parentState)
                proceed = PR_FALSE;
            }
            else proceed = PR_FALSE; // Assume that the HTML Content is not anonymous
                                     // XXX Will need to do better for XBL.
          }

          // Pass off to our parent.
          if (proceed)
            // We have a parent. Let them field the event.
            ret = mParent->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                          NS_EVENT_FLAG_BUBBLE, aEventStatus);
      }
        else if (mDocument != nsnull) {
        // We must be the document root. The event should bubble to the
        // document.
        ret = mDocument->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                        NS_EVENT_FLAG_BUBBLE, aEventStatus);
        }
    }

    if (NS_EVENT_FLAG_INIT & aFlags) {
        // We're leaving the DOM event loop so if we created a DOM event,
        // release here.
        if (nsnull != *aDOMEvent) {
            nsrefcnt rc;
            NS_RELEASE2(*aDOMEvent, rc);
            if (0 != rc) {
                // Okay, so someone in the DOM loop (a listener, JS object)
                // still has a ref to the DOM Event but the internal data
                // hasn't been malloc'd.  Force a copy of the data here so the
                // DOM Event is still valid.
                nsIPrivateDOMEvent *privateEvent;
                if (NS_OK == (*aDOMEvent)->QueryInterface(kIPrivateDOMEventIID, (void**)&privateEvent)) {
                    privateEvent->DuplicatePrivateData();
                    NS_RELEASE(privateEvent);
                }
            }
        }
        aDOMEvent = nsnull;
    }
    return ret;
}


NS_IMETHODIMP
nsXULElement::GetContentID(PRUint32* aID)
{
    *aID = mContentId;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetContentID(PRUint32 aID)
{
    mContentId = aID;
    return NS_OK;
}

NS_IMETHODIMP 
nsXULElement::RangeAdd(nsIDOMRange& aRange) 
{  
    // rdf content does not yet support DOM ranges
    return NS_OK;
}

 
NS_IMETHODIMP 
nsXULElement::RangeRemove(nsIDOMRange& aRange) 
{
    // rdf content does not yet support DOM ranges
    return NS_OK;
}                                                                        


NS_IMETHODIMP 
nsXULElement::GetRangeList(nsVoidArray*& aResult) const
{
    // rdf content does not yet support DOM ranges
    aResult = nsnull;
    return NS_OK;
}


//----------------------------------------------------------------------
// nsIDOMXULElement interface

NS_IMETHODIMP
nsXULElement::DoCommand()
{
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::AddBroadcastListener(const nsString& attr, nsIDOMElement* anElement) 
{ 
    // Add ourselves to the array.
    nsresult rv;

    if (! BroadcastListeners()) {
        rv = EnsureSlots();
        if (NS_FAILED(rv)) return rv;

        mSlots->mBroadcastListeners = new nsVoidArray();
        if (! mSlots->mBroadcastListeners)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    BroadcastListeners()->AppendElement(new XULBroadcastListener(attr, anElement));

    // We need to sync up the initial attribute value.
    nsCOMPtr<nsIContent> listener( do_QueryInterface(anElement) );

    if (attr.EqualsWithConversion("*")) {
        // All of the attributes found on this node should be set on the
        // listener.
        if (Attributes()) {
            for (PRInt32 i = Attributes()->Count() - 1; i >= 0; --i) {
                nsXULAttribute* attr = NS_REINTERPRET_CAST(nsXULAttribute*, Attributes()->ElementAt(i));
                nsINodeInfo *ni = attr->GetNodeInfo();
                if (ni->Equals(kIdAtom, kNameSpaceID_None))
                    continue;

                // We aren't the id atom, so it's ok to set us in the listener.
                nsAutoString value;
                attr->GetValue(value);
                listener->SetAttribute(ni, value, PR_TRUE);
            }
        }
    }
    else {
        // Find out if the attribute is even present at all.
        nsCOMPtr<nsIAtom> kAtom = dont_AddRef(NS_NewAtom(attr));

        nsAutoString attrValue;
        nsresult result = GetAttribute(kNameSpaceID_None, kAtom, attrValue);
        PRBool attrPresent = (result == NS_CONTENT_ATTR_NO_VALUE ||
                              result == NS_CONTENT_ATTR_HAS_VALUE);

        if (attrPresent) {
            // Set the attribute 
            anElement->SetAttribute(attr, attrValue);
        }
        else {
            // Unset the attribute
            anElement->RemoveAttribute(attr);
        }
    }

    return NS_OK; 
}


NS_IMETHODIMP
nsXULElement::RemoveBroadcastListener(const nsString& attr, nsIDOMElement* anElement) 
{ 
    if (BroadcastListeners()) {
        // Find the element.
        PRInt32 count = BroadcastListeners()->Count();
        for (PRInt32 i = 0; i < count; i++) {
            XULBroadcastListener* xulListener =
                NS_REINTERPRET_CAST(XULBroadcastListener*, BroadcastListeners()->ElementAt(i));

            if (xulListener->mListener == anElement) {
                if (xulListener->ObservingEverything() || attr.EqualsWithConversion("*")) { 
                    // Do the removal.
                    BroadcastListeners()->RemoveElementAt(i);
                    delete xulListener;
                }
                else {
                    // We're observing specific attributes and removing a specific attribute
                    xulListener->RemoveAttribute(attr);
                    if (xulListener->IsEmpty()) {
                        // Do the removal.
                        BroadcastListeners()->RemoveElementAt(i);
                        delete xulListener;
                    }
                }
                break;
            }
        }
    }

    return NS_OK;
}


// XXX This _should_ be an implementation method, _not_ publicly exposed :-(
NS_IMETHODIMP
nsXULElement::GetResource(nsIRDFResource** aResource)
{
    nsresult rv;

    nsAutoString id;
    rv = GetAttribute(kNameSpaceID_None, kRefAtom, id);
    if (NS_FAILED(rv)) return rv;

    if (rv != NS_CONTENT_ATTR_HAS_VALUE) {
        rv = GetAttribute(kNameSpaceID_None, kIdAtom, id);
        if (NS_FAILED(rv)) return rv;
    }

    if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
        rv = gRDFService->GetUnicodeResource(id.GetUnicode(), aResource);
        if (NS_FAILED(rv)) return rv;
    }
    else {
        *aResource = nsnull;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetDatabase(nsIRDFCompositeDataSource** aDatabase)
{
    *aDatabase = Database();
    NS_IF_ADDREF(*aDatabase);
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetBuilder(nsIXULTemplateBuilder** aBuilder)
{
    *aBuilder = Builder();
    NS_IF_ADDREF(*aBuilder);
    return NS_OK;
}


//----------------------------------------------------------------------
// Implementation methods

nsresult
nsXULElement::EnsureContentsGenerated(void) const
{
    if (mLazyState & nsIXULContent::eChildrenMustBeRebuilt) {
        nsresult rv;

        // Ensure that the element is actually _in_ the document tree;
        // otherwise, somebody is trying to generate children for a node
        // that's not currently in the content model.
        NS_PRECONDITION(mDocument != nsnull, "element not in tree");
        if (!mDocument)
            return NS_ERROR_NOT_INITIALIZED;

        // XXX hack because we can't use "mutable"
        nsXULElement* unconstThis = NS_CONST_CAST(nsXULElement*, this);

        if (! unconstThis->mChildren) {
            rv = NS_NewISupportsArray(getter_AddRefs(unconstThis->mChildren));
            if (NS_FAILED(rv)) return rv;
        }

        // Clear this value *first*, so we can re-enter the nsIContent
        // getters if needed.
        unconstThis->mLazyState &= ~nsIXULContent::eChildrenMustBeRebuilt;

        nsCOMPtr<nsIXULDocument> rdfDoc = do_QueryInterface(mDocument);
        if (! mDocument)
            return NS_OK;

        rv = rdfDoc->CreateContents(NS_STATIC_CAST(nsIStyledContent*, unconstThis));
        NS_ASSERTION(NS_SUCCEEDED(rv), "problem creating kids");
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

    
nsresult
nsXULElement::ExecuteOnBroadcastHandler(nsIDOMElement* anElement, const nsString& attrName)
{
    // Now we execute the onchange handler in the context of the
    // observer. We need to find the observer in order to
    // execute the handler.
    nsCOMPtr<nsIDOMNodeList> nodeList;
    if (NS_SUCCEEDED(anElement->GetElementsByTagName(NS_ConvertASCIItoUCS2("observes"),
                                                     getter_AddRefs(nodeList)))) {
        // We have a node list that contains some observes nodes.
        PRUint32 length;
        nodeList->GetLength(&length);
        for (PRUint32 i = 0; i < length; i++) {
            nsIDOMNode* domNode;
            nodeList->Item(i, &domNode);
            nsCOMPtr<nsIDOMElement> domElement;
            domElement = do_QueryInterface(domNode);
            if (domElement) {
                // We have a domElement. Find out if it was listening to us.
                nsAutoString listeningToID;
                domElement->GetAttribute(NS_ConvertASCIItoUCS2("element"), listeningToID);
                nsAutoString broadcasterID;
                GetAttribute(NS_ConvertASCIItoUCS2("id"), broadcasterID);
                if (listeningToID == broadcasterID) {
                    // We are observing the broadcaster, but is this the right
                    // attribute?
                    nsAutoString listeningToAttribute;
                    domElement->GetAttribute(NS_ConvertASCIItoUCS2("attribute"), listeningToAttribute);
                    if (listeningToAttribute == attrName) {
                        // This is the right observes node.
                        // Execute the onchange event handler
                        nsEvent event;
                        event.eventStructType = NS_EVENT;
                        event.message = NS_XUL_BROADCAST; 
                        ExecuteJSCode(domElement, &event);
                    }
                }
            }
            NS_IF_RELEASE(domNode);
        }
    }

    return NS_OK;
}


nsresult
nsXULElement::ExecuteJSCode(nsIDOMElement* anElement, nsEvent* aEvent)
{ 
    // This code executes in every presentation context in which this
    // document is appearing.
    nsCOMPtr<nsIContent> content;
    content = do_QueryInterface(anElement);
    if (!content)
      return NS_OK;

    nsCOMPtr<nsIDocument> document;
    content->GetDocument(*getter_AddRefs(document));

    if (!document)
      return NS_OK;

    PRInt32 count = document->GetNumberOfShells();
    for (PRInt32 i = 0; i < count; i++) {
        nsIPresShell* shell = document->GetShellAt(i);
        if (nsnull == shell)
            continue;

        // Retrieve the context in which our DOM event will fire.
        nsCOMPtr<nsIPresContext> aPresContext;
        shell->GetPresContext(getter_AddRefs(aPresContext));
    
        NS_RELEASE(shell);

        // Handle the DOM event
        nsEventStatus status = nsEventStatus_eIgnore;
        content->HandleDOMEvent(aPresContext, aEvent, nsnull, NS_EVENT_FLAG_INIT, &status);
    }

    return NS_OK;
}



nsresult
nsXULElement::GetElementsByTagName(nsIDOMNode* aNode,
                                     const nsString& aTagName,
                                     nsRDFDOMNodeList* aElements)
{
    nsresult rv;

    nsCOMPtr<nsIDOMNodeList> children;
    if (NS_FAILED(rv = aNode->GetChildNodes( getter_AddRefs(children) ))) {
        NS_ERROR("unable to get node's children");
        return rv;
    }

    // no kids: terminate the recursion
    if (! children)
        return NS_OK;

    PRUint32 length;
    if (NS_FAILED(children->GetLength(&length))) {
        NS_ERROR("unable to get node list's length");
        return rv;
    }

    for (PRUint32 i = 0; i < length; ++i) {
        nsCOMPtr<nsIDOMNode> child;
        if (NS_FAILED(rv = children->Item(i, getter_AddRefs(child) ))) {
            NS_ERROR("unable to get child from list");
            return rv;
        }

        nsCOMPtr<nsIDOMElement> element;
        element = do_QueryInterface(child);
        if (!element)
          continue;

        if (aTagName.EqualsWithConversion("*")) {
            if (NS_FAILED(rv = aElements->AppendNode(child))) {
                NS_ERROR("unable to append element to node list");
                return rv;
            }
        }
        else {
            nsAutoString name;
            if (NS_FAILED(rv = child->GetNodeName(name))) {
                NS_ERROR("unable to get node name");
                return rv;
            }

            if (aTagName.Equals(name)) {
                if (NS_FAILED(rv = aElements->AppendNode(child))) {
                    NS_ERROR("unable to append element to node list");
                    return rv;
                }
            }
        }

        // Now recursively look for children
        if (NS_FAILED(rv = GetElementsByTagName(child, aTagName, aElements))) {
            NS_ERROR("unable to recursively get elements by tag name");
            return rv;
        }
    }

    return NS_OK;
}

nsresult
nsXULElement::GetElementsByAttribute(nsIDOMNode* aNode,
                                       const nsString& aAttribute,
                                       const nsString& aValue,
                                       nsRDFDOMNodeList* aElements)
{
    nsresult rv;

    nsCOMPtr<nsIDOMNodeList> children;
    if (NS_FAILED(rv = aNode->GetChildNodes( getter_AddRefs(children) ))) {
        NS_ERROR("unable to get node's children");
        return rv;
    }

    // no kids: terminate the recursion
    if (! children)
        return NS_OK;

    PRUint32 length;
    if (NS_FAILED(children->GetLength(&length))) {
        NS_ERROR("unable to get node list's length");
        return rv;
    }

    for (PRUint32 i = 0; i < length; ++i) {
        nsCOMPtr<nsIDOMNode> child;
        if (NS_FAILED(rv = children->Item(i, getter_AddRefs(child) ))) {
            NS_ERROR("unable to get child from list");
            return rv;
        }

        nsCOMPtr<nsIDOMElement> element;
        element = do_QueryInterface(child);
        if (!element)
          continue;

        nsAutoString attrValue;
        if (NS_FAILED(rv = element->GetAttribute(aAttribute, attrValue))) {
            NS_ERROR("unable to get attribute value");
            return rv;
        }

        if ((attrValue == aValue) || (attrValue.Length() > 0 && aValue.EqualsWithConversion("*"))) {
            if (NS_FAILED(rv = aElements->AppendNode(child))) {
                NS_ERROR("unable to append element to node list");
                return rv;
            }
        }
       
        // Now recursively look for children
        if (NS_FAILED(rv = GetElementsByAttribute(child, aAttribute, aValue, aElements))) {
            NS_ERROR("unable to recursively get elements by attribute");
            return rv;
        }
    }

    return NS_OK;
}

// nsIStyledContent Implementation
NS_IMETHODIMP
nsXULElement::GetID(nsIAtom*& aResult) const
{
    aResult = nsnull;

    if (mSlots && mSlots->mAttributes) {
        // Take advantage of the fact that the 'id' attribute will
        // already be atomized.
        PRInt32 count = mSlots->mAttributes->Count();
        for (PRInt32 i = 0; i < count; ++i) {
            nsXULAttribute* attr =
                NS_REINTERPRET_CAST(nsXULAttribute*, mSlots->mAttributes->ElementAt(i));

            if (attr->GetNodeInfo()->Equals(kIdAtom, kNameSpaceID_None)) {
                nsIAtom* result;
                attr->GetValueAsAtom(&result);
                aResult = result; // transfer refcnt
                break;
            }
        }
    }
    else if (mPrototype) {
        PRInt32 count = mPrototype->mNumAttributes;
        for (PRInt32 i = 0; i < count; i++) {
            nsXULPrototypeAttribute* attr = &(mPrototype->mAttributes[i]);
            if (attr->mNodeInfo->Equals(kIdAtom, kNameSpaceID_None)) {
                aResult = NS_NewAtom(attr->mValue);
                break;
            }
        }
    }

    return NS_OK;
}
    
NS_IMETHODIMP
nsXULElement::GetClasses(nsVoidArray& aArray) const
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (Attributes()) {
        rv = Attributes()->GetClasses(aArray);
    }
    else if (mPrototype) {
        rv = nsClassList::GetClasses(mPrototype->mClassList, aArray);
    }
    else {
        aArray.Clear();
    }
    return rv;
}

NS_IMETHODIMP 
nsXULElement::HasClass(nsIAtom* aClass) const
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (Attributes()) {
        rv = Attributes()->HasClass(aClass);
    }
    else if (mPrototype) {
        rv = nsClassList::HasClass(mPrototype->mClassList, aClass) ? NS_OK : NS_COMFALSE;
    }
    return rv;
}

NS_IMETHODIMP
nsXULElement::GetContentStyleRules(nsISupportsArray* aRules)
{
    // For treecols, we support proportional widths using the WIDTH attribute.
    if (NodeInfo()->Equals(kTreeColAtom)) {
        // If the width attribute is set, then we should return ourselves as a style
        // rule.
        nsCOMPtr<nsIAtom> widthAtom = dont_AddRef(NS_NewAtom("width"));
        nsAutoString width;
        GetAttribute(kNameSpaceID_None, widthAtom, width);

        nsCOMPtr<nsIAtom> hiddenAtom = dont_AddRef(NS_NewAtom("hidden"));
        nsAutoString hidden;
        GetAttribute(kNameSpaceID_None, hiddenAtom, hidden);

        if (!width.IsEmpty() || !hidden.IsEmpty()) {
            // XXX This should ultimately be factored out if we find that
            // a bunch of XUL widgets are implementing attributes that need
            // to be mapped into style.  I'm hoping treecol will be the only
            // one that needs to do this though.
            // QI ourselves to be an nsIStyleRule.
            aRules->AppendElement((nsIStyleRule*)this);
        }
    }
    return NS_OK;
}
    
NS_IMETHODIMP
nsXULElement::GetInlineStyleRules(nsISupportsArray* aRules)
{
    // Fetch the cached style rule from the attributes.
    nsresult result = NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIStyleRule> rule;
    if (aRules) {
        if (Attributes()) {
            result = Attributes()->GetInlineStyleRule(*getter_AddRefs(rule));
        }
        else if (mPrototype && mPrototype->mInlineStyleRule) {
            rule = mPrototype->mInlineStyleRule;
            result = NS_OK;
        }
    }
    if (rule) {
        aRules->AppendElement(rule);
    }
    return result;
}

NS_IMETHODIMP
nsXULElement::GetMappedAttributeImpact(const nsIAtom* aAttribute, 
                                         PRInt32& aHint) const
{
    aHint = NS_STYLE_HINT_CONTENT;  // by default, never map attributes to style

    if (NodeInfo()->Equals(kTreeItemAtom)) {
        // Force a framechange if the 'open' atom changes on a <treeitem>
        if (kOpenAtom == aAttribute)
            aHint = NS_STYLE_HINT_FRAMECHANGE;
    }
    else if (NodeInfo()->Equals(kTreeColAtom)) {
        // Ignore 'width' and 'hidden' on a <treecol>
        if (kWidthAtom == aAttribute || kHiddenAtom == aAttribute)
            aHint = NS_STYLE_HINT_REFLOW;
    }
    else if (NodeInfo()->Equals(kWindowAtom)) {
        // Ignore 'width' and 'height' on a <window>
        if (kWidthAtom == aAttribute || kHeightAtom == aAttribute)
            aHint = NS_STYLE_HINT_NONE;
    }

    return NS_OK;
}

// Controllers Methods
NS_IMETHODIMP
nsXULElement::GetControllers(nsIControllers** aResult)
{
    if (! Controllers()) {
        NS_PRECONDITION(mDocument != nsnull, "no document");
        if (! mDocument)
            return NS_ERROR_NOT_INITIALIZED;

        nsresult rv;
        rv = EnsureSlots();
        if (NS_FAILED(rv)) return rv;

        rv = NS_NewXULControllers(nsnull, NS_GET_IID(nsIControllers), getter_AddRefs(mSlots->mControllers));
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a controllers");
        if (NS_FAILED(rv)) return rv;

        // Set the command dispatcher on the new controllers object
        nsCOMPtr<nsIDOMXULDocument> domxuldoc = do_QueryInterface(mDocument);
        NS_ASSERTION(domxuldoc != nsnull, "not an nsIDOMXULDocument");
        if (! domxuldoc)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIDOMXULCommandDispatcher> dispatcher;
        rv = domxuldoc->GetCommandDispatcher(getter_AddRefs(dispatcher));
        if (NS_FAILED(rv)) return rv;

        rv = mSlots->mControllers->SetCommandDispatcher(dispatcher);
        if (NS_FAILED(rv)) return rv;
    }

    *aResult = Controllers();
    NS_IF_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetBoxObject(nsIBoxObject** aResult)
{
  if (mBoxObject) {
    *aResult = mBoxObject;
    NS_ADDREF(*aResult);
    return NS_OK;
  }

  // We need to create our object.
  *aResult = nsnull;
  if (!mDocument)
    return NS_OK;

  nsCOMPtr<nsIPresShell> shell = getter_AddRefs(mDocument->GetShellAt(0));
  if (!shell)
    return NS_OK;

  nsresult rv;
  PRInt32 dummy;
  nsCOMPtr<nsIAtom> tag;
  NS_WITH_SERVICE(nsIXBLService, xblService, "component://netscape/xbl", &rv);
  xblService->ResolveTag(NS_STATIC_CAST(nsIStyledContent*, this), &dummy, getter_AddRefs(tag));
  
  nsCAutoString progID("component://netscape/layout/xul-boxobject");
  if (tag.get() == kBrowserAtom)
    progID += "-browser";
  else if (tag.get() == kEditorAtom)
    progID += "-editor";
  else if (tag.get() == kIFrameAtom)
    progID += "-iframe";
  else if (tag.get() == kMenuAtom)
    progID += "-menu";
  else if (tag.get() == kPopupSetAtom)
    progID += "-popupset";
  else if (tag.get() == kTreeAtom)
    progID += "-tree";

  mBoxObject = do_CreateInstance(progID);
  if (!mBoxObject)
    return NS_OK;

  nsCOMPtr<nsPIBoxObject> privateBox(do_QueryInterface(mBoxObject));
  if (NS_FAILED(rv = privateBox->Init(NS_STATIC_CAST(nsIStyledContent*, this), shell)))
    return rv;

  *aResult = mBoxObject;
  NS_ADDREF(*aResult);
 
  return NS_OK;
}

// Methods for setting/getting attributes from nsIDOMXULElement
nsresult
nsXULElement::GetId(nsString& aId)
{
  GetAttribute(NS_ConvertASCIItoUCS2("id"), aId);
  return NS_OK;
}

nsresult
nsXULElement::SetId(const nsString& aId)
{
  SetAttribute(NS_ConvertASCIItoUCS2("id"), aId);
  return NS_OK;
}

nsresult
nsXULElement::GetClassName(nsString& aClassName)
{
  GetAttribute(NS_ConvertASCIItoUCS2("class"), aClassName);
  return NS_OK;
}

nsresult
nsXULElement::SetClassName(const nsString& aClassName)
{
  SetAttribute(NS_ConvertASCIItoUCS2("class"), aClassName);
  return NS_OK;
}

nsresult    
nsXULElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULElement::GetParentTree(nsIDOMXULTreeElement** aTreeElement)
{
  nsCOMPtr<nsIContent> current;
  GetParent(*getter_AddRefs(current));
  while (current) {
    nsCOMPtr<nsIAtom> tag;
    current->GetTag(*getter_AddRefs(tag));
    if (tag && (tag.get() == kTreeAtom)) {
      nsCOMPtr<nsIDOMXULTreeElement> element = do_QueryInterface(current);
      *aTreeElement = element;
      NS_IF_ADDREF(*aTreeElement);
      return NS_OK;
    }

    nsCOMPtr<nsIContent> parent;
    current->GetParent(*getter_AddRefs(parent));
    current = parent.get();
  }
  return NS_OK;
}

PRBool 
nsXULElement::IsAncestor(nsIDOMNode* aParentNode, nsIDOMNode* aChildNode)
{
  nsCOMPtr<nsIDOMNode> parent = dont_QueryInterface(aChildNode);
  while (parent && (parent.get() != aParentNode)) {
    nsCOMPtr<nsIDOMNode> newParent;
    parent->GetParentNode(getter_AddRefs(newParent));
    parent = newParent;
  }

  if (parent)
    return PR_TRUE;
  return PR_FALSE;
}

NS_IMETHODIMP
nsXULElement::Focus()
{
  // Obtain a presentation context and then call SetFocus.
  PRInt32 count = mDocument->GetNumberOfShells();
  if (count == 0)
    return NS_OK;

  nsCOMPtr<nsIPresShell> shell = getter_AddRefs(mDocument->GetShellAt(0));
  
  // Retrieve the context
  nsCOMPtr<nsIPresContext> aPresContext;
  shell->GetPresContext(getter_AddRefs(aPresContext));

  // Set focus
  return SetFocus(aPresContext);
}

NS_IMETHODIMP
nsXULElement::Blur()
{
  // Obtain a presentation context and then call SetFocus.
  PRInt32 count = mDocument->GetNumberOfShells();
  if (count == 0)
    return NS_OK;

  nsCOMPtr<nsIPresShell> shell = getter_AddRefs(mDocument->GetShellAt(0));
  
  // Retrieve the context
  nsCOMPtr<nsIPresContext> aPresContext;
  shell->GetPresContext(getter_AddRefs(aPresContext));

  // Set focus
  return RemoveFocus(aPresContext);
}

NS_IMETHODIMP
nsXULElement::Click()
{
  nsCOMPtr<nsIDocument> doc; // Strong
  GetDocument(*getter_AddRefs(doc));
  if (doc) {
    PRInt32 numShells = doc->GetNumberOfShells();
    nsCOMPtr<nsIPresShell> shell; // Strong
    nsCOMPtr<nsIPresContext> context;
    nsAutoString tagName;
    PRBool isButton = NodeInfo()->Equals(NS_ConvertASCIItoUCS2("button"));

    for (PRInt32 i=0; i<numShells; i++) {
      shell = getter_AddRefs(doc->GetShellAt(i));
      shell->GetPresContext(getter_AddRefs(context));
        
	    nsEventStatus status = nsEventStatus_eIgnore;
	    nsMouseEvent event;
	    event.eventStructType = NS_GUI_EVENT;
	    event.message = NS_MOUSE_LEFT_CLICK;
      event.isShift = PR_FALSE;
      event.isControl = PR_FALSE;
      event.isAlt = PR_FALSE;
      event.isMeta = PR_FALSE;
      event.clickCount = 0;
      event.widget = nsnull;
      HandleDOMEvent(context, &event, nsnull, NS_EVENT_FLAG_INIT, &status);

      if (isButton) {
        nsMouseEvent evt;
        evt.eventStructType = NS_EVENT;
        evt.message = NS_MENU_ACTION;
        nsEventStatus sts = nsEventStatus_eIgnore;
        HandleDOMEvent(context, &evt, nsnull, NS_EVENT_FLAG_INIT, &sts);
      }
    }
  }
  
  return NS_OK;
}

// nsIFocusableContent interface and helpers

NS_IMETHODIMP
nsXULElement::SetFocus(nsIPresContext* aPresContext)
{
  nsAutoString disabled;
  GetAttribute(NS_ConvertASCIItoUCS2("disabled"), disabled);
  if (disabled.EqualsWithConversion("true"))
    return NS_OK;
 
  nsIEventStateManager* esm;
  if (NS_OK == aPresContext->GetEventStateManager(&esm)) {
    
    esm->SetContentState((nsIStyledContent*)this, NS_EVENT_STATE_FOCUS);
    NS_RELEASE(esm);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsXULElement::RemoveFocus(nsIPresContext* aPresContext)
{
  return NS_OK;
}

// nsIStyleRule interface
NS_IMETHODIMP 
nsXULElement::GetStyleSheet(nsIStyleSheet*& aSheet) const
{
    nsresult rv = NS_OK;
    aSheet = nsnull;
    if (mDocument) {
        nsCOMPtr<nsIHTMLContentContainer> container = do_QueryInterface(mDocument);
        if (container) {
            nsCOMPtr<nsIHTMLStyleSheet> htmlStyleSheet;
            rv = container->GetAttributeStyleSheet(getter_AddRefs(htmlStyleSheet));
            if (NS_FAILED(rv))
                return rv;
            nsCOMPtr<nsIStyleSheet> styleSheet = do_QueryInterface(htmlStyleSheet);
            aSheet = styleSheet;
            NS_IF_ADDREF(aSheet);
        }
    }
    return rv;
}

NS_IMETHODIMP 
nsXULElement::GetStrength(PRInt32& aStrength) const
{
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::MapFontStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext)
{
    return NS_OK;
}

NS_IMETHODIMP 
nsXULElement::MapStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext)
{
    if (NodeInfo()->Equals(kTreeColAtom)) {
        // Should only get called if we had a width attribute set. Retrieve it.
        nsAutoString widthVal;
        nsAutoString hiddenVal;
        GetAttribute(NS_ConvertASCIItoUCS2("width"), widthVal);
        GetAttribute(NS_ConvertASCIItoUCS2("hidden"), hiddenVal);
        if (!hiddenVal.IsEmpty())
          widthVal.AssignWithConversion("0*");

        if (!widthVal.IsEmpty()) {
            PRInt32 intVal;
            float floatVal;
            nsHTMLUnit unit = eHTMLUnit_Null;
            if (ParseNumericValue(widthVal, intVal, floatVal, unit)) {
                // Success. Update the width for the style context.
                nsStylePosition* position = (nsStylePosition*)
                aContext->GetMutableStyleData(eStyleStruct_Position);
                switch (unit) {
                  case eHTMLUnit_Percent:
                    position->mWidth.mUnit = eStyleUnit_Percent;
                    position->mWidth.mValue.mFloat = floatVal;
                    break;

                  case eHTMLUnit_Pixel:
                    float p2t;
                    aPresContext->GetScaledPixelsToTwips(&p2t);
                    position->mWidth.mUnit = eStyleUnit_Coord;
                    position->mWidth.mValue.mInt = NSIntPixelsToTwips(intVal, p2t);
                    break;

                  case eHTMLUnit_Proportional:
                    position->mWidth.mUnit = eStyleUnit_Proportional;
                    position->mWidth.mValue.mInt = intVal;
                    break;
                  default:
                    break;
                }
            }
        }
    }
        
    return NS_OK;
}

void nsXULElement::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  // XXX - implement this if you want the sizes of XUL style rules 
  //       dumped during StyleSize dump
  return;
}

PRBool
nsXULElement::ParseNumericValue(const nsString& aString,
                                PRInt32& aIntValue,
                                float& aFloatValue,
                                nsHTMLUnit& aValueUnit)
{
    nsAutoString tmp(aString);
    tmp.CompressWhitespace(PR_TRUE, PR_TRUE);
    PRInt32 ec, val = tmp.ToInteger(&ec);
    if (NS_OK == ec) {
        if (val < 0) val = 0;
        if (tmp.Last() == '%') {/* XXX not 100% compatible with ebina's code */
            if (val > 100) val = 100;
            aFloatValue = (float(val)/100.0f);
            aValueUnit = eHTMLUnit_Percent;
        }
        else if (tmp.Last() == '*') {
            aIntValue = val;
            aValueUnit = eHTMLUnit_Proportional;
        }
        else {
            aIntValue = val;
            aValueUnit = eHTMLUnit_Pixel;
        }
        return PR_TRUE;
    }
    return PR_FALSE;
}


nsresult
nsXULElement::AddPopupListener(nsIAtom* aName)
{
    // Add a popup listener to the element
    nsresult rv;

    nsCOMPtr<nsIXULPopupListener> popupListener;
    rv = nsComponentManager::CreateInstance(kXULPopupListenerCID,
                                            nsnull,
                                            kIXULPopupListenerIID,
                                            getter_AddRefs(popupListener));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to create an instance of the popup listener object.");
    if (NS_FAILED(rv)) return rv;

    XULPopupType popupType;
    if (aName == kTooltipAtom) {
        popupType = eXULPopupType_tooltip;
    }
    else if (aName == kContextAtom) {
        popupType = eXULPopupType_context;
    }
    else {
        popupType = eXULPopupType_popup;
    }

    // Add a weak reference to the node.
    popupListener->Init(this, popupType);

    // Add the popup as a listener on this element.
    nsCOMPtr<nsIDOMEventListener> eventListener = do_QueryInterface(popupListener);

    if (popupType == eXULPopupType_tooltip) {
        AddEventListener(NS_ConvertASCIItoUCS2("mouseout"), eventListener, PR_FALSE);
        AddEventListener(NS_ConvertASCIItoUCS2("mousemove"), eventListener, PR_FALSE);
    }
    else {
        AddEventListener(NS_ConvertASCIItoUCS2("mousedown"), eventListener, PR_FALSE); 
    }

    return NS_OK;
}

//*****************************************************************************
// nsXULElement::nsIChromeEventHandler
//*****************************************************************************   

NS_IMETHODIMP nsXULElement::HandleChromeEvent(nsIPresContext* aPresContext,
   nsEvent* aEvent, nsIDOMEvent** aDOMEvent, PRUint32 aFlags, 
   nsEventStatus* aEventStatus)
{
  // XXX This is a disgusting hack to prevent the doc from going
  // away until after we've finished handling the event.
  // We will be coming up with a better general solution later.
  nsCOMPtr<nsIDocument> kungFuDeathGrip(mDocument);
  return HandleDOMEvent(aPresContext, aEvent, aDOMEvent, aFlags,aEventStatus);
}

//----------------------------------------------------------------------

nsresult
nsXULElement::EnsureSlots()
{
    // Ensure that the 'mSlots' field is valid. This makes the
    // nsXULElement 'heavyweight'.
    if (mSlots)
        return NS_OK;

    mSlots = new Slots(this);
    if (! mSlots)
        return NS_ERROR_OUT_OF_MEMORY;

    // Copy information from the prototype, if there is one.
    if (! mPrototype)
        return NS_OK;

    nsXULPrototypeElement* proto = mPrototype;
    mPrototype = nsnull;

    mSlots->mNameSpace       = proto->mNameSpace;
    mSlots->mNodeInfo        = proto->mNodeInfo;

    // Copy the attributes, if necessary. Arguably, we are over-eager
    // about copying attributes. But eagerly copying the attributes
    // vastly simplifies the "lookup" and "set" logic, which otherwise
    // would need to do some pretty tricky default logic.
    if (proto->mNumAttributes == 0)
        return NS_OK;

    nsresult rv;
    rv = nsXULAttributes::Create(NS_STATIC_CAST(nsIStyledContent*, this), &(mSlots->mAttributes));
    if (NS_FAILED(rv)) return rv;

    for (PRInt32 i = 0; i < proto->mNumAttributes; ++i) {
        nsXULPrototypeAttribute* protoattr = &(proto->mAttributes[i]);

        // Create a CBufDescriptor to avoid copying the attribute's
        // value just to set it.
        nsXULAttribute* attr;
        rv = nsXULAttribute::Create(NS_STATIC_CAST(nsIStyledContent*, this),
                                    protoattr->mNodeInfo,
                                    protoattr->mValue,
                                    &attr);

        if (NS_FAILED(rv)) return rv;

        // transfer ownership of the nsXULAttribute object
        mSlots->mAttributes->AppendElement(attr);
    }

    mSlots->mAttributes->SetClassList(proto->mClassList);
    mSlots->mAttributes->SetInlineStyleRule(proto->mInlineStyleRule);

    return NS_OK;
}

//----------------------------------------------------------------------
//
// nsXULElement::Slots
//

nsXULElement::Slots::Slots(nsXULElement* aElement)
    : mElement(aElement),
      mBroadcastListeners(nsnull),
      mBroadcaster(nsnull),
      mBuilder(nsnull),
      mAttributes(nsnull),
      mInnerXULElement(nsnull)
{
    MOZ_COUNT_CTOR(nsXULElement::Slots);
}


nsXULElement::Slots::~Slots()
{
    MOZ_COUNT_DTOR(nsXULElement::Slots);

    NS_IF_RELEASE(mAttributes);

    // Release our broadcast listeners
    if (mBroadcastListeners) {
        PRInt32 count = mBroadcastListeners->Count();
        for (PRInt32 i = 0; i < count; i++) {
            XULBroadcastListener* xulListener =
                NS_REINTERPRET_CAST(XULBroadcastListener*, mBroadcastListeners->ElementAt(0));

            mElement->RemoveBroadcastListener(NS_ConvertASCIItoUCS2("*"), xulListener->mListener);
        }

        delete mBroadcastListeners;
    }

    // Delete the aggregated interface, if one exists.
    delete mInnerXULElement;
}


//----------------------------------------------------------------------
//
// nsXULPrototypeAttribute
//

nsXULPrototypeAttribute::~nsXULPrototypeAttribute()
{
    if (mEventHandler)
        RemoveJSGCRoot(&mEventHandler);
}


//----------------------------------------------------------------------
//
// nsXULPrototypeElement
//

nsresult
nsXULPrototypeElement::GetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName, nsString& aValue)
{
    for (PRInt32 i = 0; i < mNumAttributes; ++i) {
        if (mAttributes[i].mNodeInfo->Equals(aName, aNameSpaceID)) {
            aValue = mAttributes[i].mValue;
            return aValue.Length() ? NS_CONTENT_ATTR_HAS_VALUE : NS_CONTENT_ATTR_NO_VALUE;
        }
        
    }
    return NS_CONTENT_ATTR_NOT_THERE;
}


//----------------------------------------------------------------------
//
// nsXULPrototypeScript
//

nsXULPrototypeScript::nsXULPrototypeScript(PRInt32 aLineNo, const char *aVersion)
    : nsXULPrototypeNode(eType_Script, aLineNo),
      mSrcLoading(PR_FALSE),
      mSrcLoadWaiters(nsnull),
      mScriptObject(nsnull),
      mLangVersion(aVersion)
{
    MOZ_COUNT_CTOR(nsXULPrototypeScript);
}


nsXULPrototypeScript::~nsXULPrototypeScript()
{
    if (mScriptObject)
        RemoveJSGCRoot(&mScriptObject);
    MOZ_COUNT_DTOR(nsXULPrototypeScript);
}

nsresult
nsXULPrototypeScript::Compile(const PRUnichar* aText,
                              PRInt32 aTextLength,
                              nsIURI* aURI,
                              PRInt32 aLineNo,
                              nsIDocument* aDocument,
                              nsIXULPrototypeDocument* aPrototypeDocument)
{
    // We'll compile the script using the prototype document's special
    // script object as the parent. This ensures that we won't end up
    // with an uncollectable reference.
    //
    // Compiling it using (for example) the first document's global
    // object would cause JS to keep a reference via the __proto__ or
    // __parent__ pointer to the first document's global. If that
    // happened, our script object would reference the first document,
    // and the first document would indirectly reference the prototype
    // document because it keeps the prototype cache
    // alive. Circularity!
    nsresult rv;

    // Use the prototype document's special context
    nsCOMPtr<nsIScriptContext> context;

    // Use the prototype script's special scope object
    JSObject* scopeObject;

    {
        nsCOMPtr<nsIScriptGlobalObject> global;
        nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner
          = do_QueryInterface(aPrototypeDocument);
        globalOwner->GetScriptGlobalObject(getter_AddRefs(global));
        NS_ASSERTION(global != nsnull, "prototype doc has no script global");
        if (! global)
            return NS_ERROR_UNEXPECTED;

        rv = global->GetContext(getter_AddRefs(context));
        if (NS_FAILED(rv)) return rv;

        NS_ASSERTION(context != nsnull, "no context for script global");
        if (! context)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIScriptObjectOwner> owner = do_QueryInterface(global);
        if (! owner)
            return NS_ERROR_UNEXPECTED;
    
        rv = owner->GetScriptObject(context, (void**) &scopeObject);
        if (NS_FAILED(rv)) return rv;
    }

    // Use the enclosing document's principal
    // XXX is this right? or should we use the protodoc's?
    nsCOMPtr<nsIPrincipal> principal;
    rv = aDocument->GetPrincipal(getter_AddRefs(principal));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString urlspec;
    aURI->GetSpec(getter_Copies(urlspec));

    // Ok, compile it to create a prototype script object!
    rv = context->CompileScript(aText,
                                aTextLength,
                                scopeObject,
                                principal,
                                urlspec,
                                PRUint32(aLineNo),
                                mLangVersion,
                                (void**) &mScriptObject);

    if (NS_FAILED(rv)) return rv;

    // Root the compiled prototype script object.
    JSContext* cx = NS_REINTERPRET_CAST(JSContext*, context->GetNativeContext());
    if (!cx)
        return NS_ERROR_UNEXPECTED;

    rv = AddJSGCRoot(cx, &mScriptObject, "nsXULPrototypeScript::mScriptObject");
    return rv;
}
