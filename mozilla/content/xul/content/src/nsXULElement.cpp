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
 * Contributor(s):
 *   Chris Waterson <waterson@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Peter Annema <disttsc@bart.nl>
 *   Brendan Eich <brendan@mozilla.org>
 *   Mike Shaver <shaver@mozilla.org>
 *   Ben Goodger <ben@netscape.com>
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

  1. Possibly clean up the attribute walking code (e.g., in
     SetDocument, GetAttrCount, etc.) by extracting into an iterator.

 */

#include "jsapi.h"      // for JS_AddNamedRoot and JS_RemoveRootRT
#include "jsxdrapi.h"
#include "nsCOMPtr.h"
#include "nsDOMCID.h"
#include "nsDOMError.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsHTMLValue.h"
#include "nsHashtable.h"
#include "nsIAtom.h"
#include "nsIDOMAttr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMPaintListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMXULListener.h"
#include "nsIDOMScrollListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIEventStateManager.h"
#include "nsIFastLoadService.h"
#include "nsIHTMLContentContainer.h"
#include "nsIHTMLStyleSheet.h"
#include "nsINameSpace.h"
#include "nsINameSpaceManager.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIPresShell.h"
#include "nsIPrincipal.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIServiceManager.h"
#include "nsIStyleRule.h"
#include "nsIStyleSheet.h"
#include "nsIStyledContent.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"
#include "nsIXULContent.h"
#include "nsIXULDocument.h"
#include "nsIXULPopupListener.h"
#include "nsIXULPrototypeDocument.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIXBLService.h"
#include "nsLayoutCID.h"
#include "nsContentCID.h"
#include "nsRDFCID.h"
#include "nsRDFDOMNodeList.h"
#include "nsStyleConsts.h"
#include "nsXPIDLString.h"
#include "nsXULAttributes.h"
#include "nsXULControllers.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsXULDocument.h"
#include "nsRuleWalker.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsXULAtoms.h"
#include "nsIListBoxObject.h"
#include "nsContentUtils.h"
#include "nsGenericElement.h"
#include "nsContentList.h"
#include "nsMutationEvent.h"
#include "nsIDOMMutationEvent.h"
#include "nsPIDOMWindow.h"

#include "prlog.h"
#include "rdf.h"

#include "nsIControllers.h"

// The XUL interfaces implemented by the RDF content node.
#include "nsIDOMXULElement.h"

// The XUL doc interface
#include "nsIDOMXULDocument.h"

#include "nsReadableUtils.h"
#include "nsITimelineService.h"
#include "nsIFrame.h"

class nsIDocShell;

// Global object maintenance
nsICSSParser* nsXULPrototypeElement::sCSSParser = nsnull;
nsIXULPrototypeCache* nsXULPrototypeScript::sXULPrototypeCache = nsnull;
nsIXBLService * nsXULElement::gXBLService = nsnull;

//----------------------------------------------------------------------

static NS_DEFINE_CID(kEventListenerManagerCID,    NS_EVENTLISTENERMANAGER_CID);
static NS_DEFINE_CID(kRDFServiceCID,              NS_RDFSERVICE_CID);

static NS_DEFINE_CID(kXULPopupListenerCID,        NS_XULPOPUPLISTENER_CID);

//----------------------------------------------------------------------

#if 0 /* || defined(DEBUG_shaver) || defined(DEBUG_waterson) */
#define DEBUG_ATTRIBUTE_STATS
#endif

#ifdef DEBUG_ATTRIBUTE_STATS
#include <execinfo.h>

static struct {
    PRUint32 GetAttributes;
    PRUint32 UnsetAttr;
    PRUint32 Create;
    PRUint32 Total;
} gFaults;
#endif

#include "nsIJSRuntimeService.h"
static nsIJSRuntimeService* gJSRuntimeService = nsnull;
static JSRuntime* gScriptRuntime = nsnull;
static PRInt32 gScriptRuntimeRefcnt = 0;

static nsresult
AddJSGCRoot(void* aScriptObjectRef, const char* aName)
{
    if (++gScriptRuntimeRefcnt == 1 || !gScriptRuntime) {
        CallGetService("@mozilla.org/js/xpc/RuntimeService;1",
                       &gJSRuntimeService);
        if (! gJSRuntimeService) {
            NS_NOTREACHED("couldn't add GC root");
            return NS_ERROR_FAILURE;
        }

        gJSRuntimeService->GetRuntime(&gScriptRuntime);
        if (! gScriptRuntime) {
            NS_NOTREACHED("couldn't add GC root");
            return NS_ERROR_FAILURE;
        }
    }

    PRBool ok;
    ok = ::JS_AddNamedRootRT(gScriptRuntime, aScriptObjectRef, aName);
    if (! ok) {
        NS_NOTREACHED("couldn't add GC root");
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

static nsresult
RemoveJSGCRoot(void* aScriptObjectRef)
{
    if (! gScriptRuntime) {
        NS_NOTREACHED("couldn't remove GC root");
        return NS_ERROR_FAILURE;
    }

    ::JS_RemoveRootRT(gScriptRuntime, aScriptObjectRef);

    if (--gScriptRuntimeRefcnt == 0) {
        NS_RELEASE(gJSRuntimeService);
        gScriptRuntime = nsnull;
    }

    return NS_OK;
}

//----------------------------------------------------------------------

struct EventHandlerMapEntry {
    const char*  mAttributeName;
    nsIAtom*     mAttributeAtom;
    const nsIID* mHandlerIID;
};

static EventHandlerMapEntry kEventHandlerMap[] = {
    { "onclick",         nsnull, &NS_GET_IID(nsIDOMMouseListener)       },
    { "ondblclick",      nsnull, &NS_GET_IID(nsIDOMMouseListener)       },
    { "onmousedown",     nsnull, &NS_GET_IID(nsIDOMMouseListener)       },
    { "onmouseup",       nsnull, &NS_GET_IID(nsIDOMMouseListener)       },
    { "onmouseover",     nsnull, &NS_GET_IID(nsIDOMMouseListener)       },
    { "onmouseout",      nsnull, &NS_GET_IID(nsIDOMMouseListener)       },

    { "onmousemove",     nsnull, &NS_GET_IID(nsIDOMMouseMotionListener) },

    { "onkeydown",       nsnull, &NS_GET_IID(nsIDOMKeyListener)         },
    { "onkeyup",         nsnull, &NS_GET_IID(nsIDOMKeyListener)         },
    { "onkeypress",      nsnull, &NS_GET_IID(nsIDOMKeyListener)         },

    { "onload",          nsnull, &NS_GET_IID(nsIDOMLoadListener)        },
    { "onunload",        nsnull, &NS_GET_IID(nsIDOMLoadListener)        },
    { "onabort",         nsnull, &NS_GET_IID(nsIDOMLoadListener)        },
    { "onerror",         nsnull, &NS_GET_IID(nsIDOMLoadListener)        },

    { "onpopupshowing",    nsnull, &NS_GET_IID(nsIDOMXULListener)        },
    { "onpopupshown",      nsnull, &NS_GET_IID(nsIDOMXULListener)        },
    { "onpopuphiding" ,    nsnull, &NS_GET_IID(nsIDOMXULListener)        },
    { "onpopuphidden",     nsnull, &NS_GET_IID(nsIDOMXULListener)        },
    { "onclose",           nsnull, &NS_GET_IID(nsIDOMXULListener)        },
    { "oncommand",         nsnull, &NS_GET_IID(nsIDOMXULListener)        },
    { "onbroadcast",       nsnull, &NS_GET_IID(nsIDOMXULListener)        },
    { "oncommandupdate",   nsnull, &NS_GET_IID(nsIDOMXULListener)        },

    { "onoverflow",       nsnull, &NS_GET_IID(nsIDOMScrollListener)     },
    { "onunderflow",      nsnull, &NS_GET_IID(nsIDOMScrollListener)     },
    { "onoverflowchanged",nsnull, &NS_GET_IID(nsIDOMScrollListener)     },

    { "onfocus",         nsnull, &NS_GET_IID(nsIDOMFocusListener)       },
    { "onblur",          nsnull, &NS_GET_IID(nsIDOMFocusListener)       },

    { "onsubmit",        nsnull, &NS_GET_IID(nsIDOMFormListener)        },
    { "onreset",         nsnull, &NS_GET_IID(nsIDOMFormListener)        },
    { "onchange",        nsnull, &NS_GET_IID(nsIDOMFormListener)        },
    { "onselect",        nsnull, &NS_GET_IID(nsIDOMFormListener)        },
    { "oninput",         nsnull, &NS_GET_IID(nsIDOMFormListener)        },

    { "onpaint",         nsnull, &NS_GET_IID(nsIDOMPaintListener)       },

    { "ondragenter",     nsnull, &NS_GET_IID(nsIDOMDragListener)        },
    { "ondragover",      nsnull, &NS_GET_IID(nsIDOMDragListener)        },
    { "ondragexit",      nsnull, &NS_GET_IID(nsIDOMDragListener)        },
    { "ondragdrop",      nsnull, &NS_GET_IID(nsIDOMDragListener)        },
    { "ondraggesture",   nsnull, &NS_GET_IID(nsIDOMDragListener)        },

    { "oncontextmenu",   nsnull, &NS_GET_IID(nsIDOMContextMenuListener) },

    { nsnull,            nsnull, nsnull                                 }
};


// XXX This function is called for every attribute on every element for
// XXX which we SetDocument, among other places.  A linear search might
// XXX not be what we want.
static nsresult
GetEventHandlerIID(nsIAtom* aName, nsIID* aIID, PRBool* aFound)
{
    *aFound = PR_FALSE;

    EventHandlerMapEntry* entry = kEventHandlerMap;
    while (entry->mAttributeAtom) {
        if (entry->mAttributeAtom == aName) {
            *aIID = *entry->mHandlerIID;
            *aFound = PR_TRUE;
            break;
        }
        ++entry;
    }

    return NS_OK;
}

static void
InitEventHandlerMap()
{
    EventHandlerMapEntry* entry = kEventHandlerMap;
    while (entry->mAttributeName) {
        entry->mAttributeAtom = NS_NewAtom(entry->mAttributeName);
        ++entry;
    }
}


static void
FinishEventHandlerMap()
{
    EventHandlerMapEntry* entry = kEventHandlerMap;
    while (entry->mAttributeName) {
        NS_IF_RELEASE(entry->mAttributeAtom);
        ++entry;
    }
}

//----------------------------------------------------------------------

static PRBool HasMutationListeners(nsIContent* aContent, PRUint32 aType)
{
  nsCOMPtr<nsIDocument> doc;
  aContent->GetDocument(*getter_AddRefs(doc));
  if (!doc)
    return PR_FALSE;

  nsCOMPtr<nsIScriptGlobalObject> global;
  doc->GetScriptGlobalObject(getter_AddRefs(global));
  if (!global)
    return PR_FALSE;

  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(global));
  if (!window)
    return PR_FALSE;

  PRBool set;
  window->HasMutationListeners(aType, &set);
  if (!set)
    return PR_FALSE;

  // We know a mutation listener is registered, but it might not
  // be in our chain.  Check quickly to see.
  nsCOMPtr<nsIContent> curr = aContent;
  nsCOMPtr<nsIEventListenerManager> manager;

  while (curr) {
    nsCOMPtr<nsIDOMEventReceiver> rec(do_QueryInterface(curr));
    if (rec) {
      rec->GetListenerManager(getter_AddRefs(manager));
      if (manager) {
        PRBool hasMutationListeners = PR_FALSE;
        manager->HasMutationListeners(&hasMutationListeners);
        if (hasMutationListeners)
          return PR_TRUE;
      }
    }

    nsCOMPtr<nsIContent> prev = curr;
    prev->GetParent(*getter_AddRefs(curr));
  }

  nsCOMPtr<nsIDOMEventReceiver> rec(do_QueryInterface(doc));
  if (rec) {
    rec->GetListenerManager(getter_AddRefs(manager));
    if (manager) {
      PRBool hasMutationListeners = PR_FALSE;
      manager->HasMutationListeners(&hasMutationListeners);
      if (hasMutationListeners)
        return PR_TRUE;
    }
  }

  rec = do_QueryInterface(window);
  if (rec) {
    rec->GetListenerManager(getter_AddRefs(manager));
    if (manager) {
      PRBool hasMutationListeners = PR_FALSE;
      manager->HasMutationListeners(&hasMutationListeners);
      if (hasMutationListeners)
        return PR_TRUE;
    }
  }

  return PR_FALSE;
}

//----------------------------------------------------------------------

static nsChangeHint
StyleHintFor(nsINodeInfo* aNodeInfo)
{
    nsCOMPtr<nsIAtom> tagName;
    aNodeInfo->GetNameAtom(*getter_AddRefs(tagName));
    if ((tagName == nsXULAtoms::broadcaster) ||
        (tagName == nsXULAtoms::command) ||
        (tagName == nsXULAtoms::key)) {
        return NS_STYLE_HINT_NONE;
    }

    return NS_STYLE_HINT_UNKNOWN;
}


//----------------------------------------------------------------------

nsrefcnt             nsXULElement::gRefCnt;
nsIRDFService*       nsXULElement::gRDFService;

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
//

nsXULElement::nsXULElement()
    : mPrototype(nsnull),
      mDocument(nsnull),
      mParent(nsnull),
      mBindingParent(nsnull),
      mSlots(nsnull)
{
    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumElements);
}


nsresult
nsXULElement::Init()
{
    if (gRefCnt++ == 0) {
        nsresult rv;

        rv = CallGetService(kRDFServiceCID, &gRDFService);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
        if (NS_FAILED(rv)) return rv;

        InitEventHandlerMap();
    }

    return NS_OK;
}

nsXULElement::~nsXULElement()
{
    if (mPrototype)
        mPrototype->Release();

    delete mSlots;

    //NS_IF_RELEASE(mDocument); // not refcounted
    //NS_IF_RELEASE(mParent)    // not refcounted

    // Force child's parent to be null. This ensures that we don't
    // have dangling pointers if a child gets leaked.
    for (PRInt32 i = mChildren.Count() - 1; i >= 0; --i) {
        nsIContent* child = NS_STATIC_CAST(nsIContent*, mChildren[i]);
        child->SetParent(nsnull);
        NS_RELEASE(child);
    }

    // Clean up shared statics
    if (--gRefCnt == 0) {
        FinishEventHandlerMap();

        NS_IF_RELEASE(gRDFService);
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

    aPrototype->AddRef();

    if (aIsScriptable) {
        // Check each attribute on the prototype to see if we need to do
        // any additional processing and hookup that would otherwise be
        // done 'automagically' by SetAttribute().
        for (PRInt32 i = 0; i < aPrototype->mNumAttributes; ++i)
            element->AddListenerFor(aPrototype->mAttributes[i].mNodeInfo, PR_TRUE);
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

    NS_ASSERTION(aNodeInfo, "need nodeinfo for non-proto Create");
    element->mSlots->mNodeInfo = aNodeInfo;

    *aResult = NS_REINTERPRET_CAST(nsIStyledContent*, element);
    NS_ADDREF(*aResult);

#ifdef DEBUG_ATTRIBUTE_STATS
    {
        gFaults.Create++; gFaults.Total++;
        nsAutoString tagstr;
        element->NodeInfo()->GetQualifiedName(tagstr);
        char *tagcstr = ToNewCString(tagstr);
        fprintf(stderr, "XUL: Heavyweight create of <%s>: %d/%d\n",
                tagcstr, gFaults.Create, gFaults.Total);
        nsMemory::Free(tagcstr);
        void *back[5];
        backtrace(back, sizeof(back) / sizeof(back[0]));
        backtrace_symbols_fd(back, sizeof(back) / sizeof(back[0]), 2);
    }
#endif

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

    if (iid.Equals(NS_GET_IID(nsIStyledContent)) ||
        iid.Equals(NS_GET_IID(nsIContent)) ||
        iid.Equals(NS_GET_IID(nsISupports))) {
        *result = NS_STATIC_CAST(nsIStyledContent*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIXMLContent))) {
        *result = NS_STATIC_CAST(nsIXMLContent*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIXULContent))) {
        *result = NS_STATIC_CAST(nsIXULContent*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMXULElement)) ||
             iid.Equals(NS_GET_IID(nsIDOMElement)) ||
             iid.Equals(NS_GET_IID(nsIDOMNode))) {
        *result = NS_STATIC_CAST(nsIDOMXULElement*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIScriptEventHandlerOwner))) {
        *result = NS_STATIC_CAST(nsIScriptEventHandlerOwner*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMEventReceiver)) ||
             iid.Equals(NS_GET_IID(nsIDOMEventTarget))) {
      nsISupports *inst = NS_STATIC_CAST(nsIDOMEventReceiver *,
                            nsDOMEventRTTearoff::Create(this));
      NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
      NS_ADDREF(inst);
      *result = inst;
      return NS_OK;
    }
    else if (iid.Equals(NS_GET_IID(nsIDOM3EventTarget))) {
      nsISupports *inst = NS_STATIC_CAST(nsIDOM3EventTarget *,
                            nsDOMEventRTTearoff::Create(this));
      NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
      NS_ADDREF(inst);
      *result = inst;
      return NS_OK;
    }
    else if (iid.Equals(NS_GET_IID(nsIChromeEventHandler))) {
        *result = NS_STATIC_CAST(nsIChromeEventHandler*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOM3Node))) {
        nsISupports *inst = new nsNode3Tearoff(this);
        NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
        NS_ADDREF(inst);
        *result = inst;
        return NS_OK;
    }
    else if (iid.Equals(NS_GET_IID(nsIClassInfo))) {
        nsISupports *inst = nsContentUtils::
                GetClassInfoInstance(eDOMClassInfo_XULElement_id);

        NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);

        *result = inst;

        return NS_OK;
    }
    else if (mDocument) {
        nsCOMPtr<nsIBindingManager> manager;
        mDocument->GetBindingManager(getter_AddRefs(manager));
        return manager->GetBindingImplementation(this, iid, result);
    } else {
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
nsXULElement::GetNodeName(nsAString& aNodeName)
{
    return NodeInfo()->GetQualifiedName(aNodeName);
}


NS_IMETHODIMP
nsXULElement::GetNodeValue(nsAString& aNodeValue)
{
    aNodeValue.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetNodeValue(const nsAString& aNodeValue)
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
        return CallQueryInterface(mParent, aParentNode);
    }

    if (mDocument) {
        // XXX This is a mess because of our fun multiple inheritance heirarchy
        nsCOMPtr<nsIContent> root;
        mDocument->GetRootContent(getter_AddRefs(root));
        nsCOMPtr<nsIContent> thisIContent;
        QueryInterface(NS_GET_IID(nsIContent), getter_AddRefs(thisIContent));

        if (root == thisIContent) {
            // If we don't have a parent, and we're the root content
            // of the document, DOM says that our parent is the
            // document.
            return CallQueryInterface(mDocument, aParentNode);
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

        nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(child);
        if (!domNode) {
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

    if (child) {
        rv = CallQueryInterface(child, aFirstChild);
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

        NS_ASSERTION(child, "no child");

        if (child) {
            rv = CallQueryInterface(child, aLastChild);
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
        if (pos > 0) {
            nsCOMPtr<nsIContent> prev;
            mParent->ChildAt(--pos, *getter_AddRefs(prev));
            if (prev) {
                nsresult rv = CallQueryInterface(prev, aPreviousSibling);
                NS_ASSERTION(*aPreviousSibling, "not a DOM node");
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
                nsresult res = CallQueryInterface(next, aNextSibling);
                NS_ASSERTION(*aNextSibling, "not a DOM Node");
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
        // We fault everything, until we can fix nsXULAttributes
#ifdef DEBUG_ATTRIBUTE_STATS
        if (mPrototype) {
            gFaults.GetAttributes++; gFaults.Total++;
            fprintf(stderr, "XUL: Faulting for GetAttributes: %d/%d\n",
                    gFaults.GetAttributes, gFaults.Total);
        }
#endif

        rv = MakeHeavyweight();
        if (NS_FAILED(rv)) return rv;

        if (! Attributes()) {
            nsXULAttributes *attrs;
            rv = nsXULAttributes::Create(NS_STATIC_CAST(nsIStyledContent*, this), &attrs);
            if (NS_FAILED(rv)) return rv;

            mSlots->SetAttributes(attrs);
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
        return CallQueryInterface(mDocument, aOwnerDocument);
    }
    nsCOMPtr<nsIDocument> doc;
    NodeInfo()->GetDocument(*getter_AddRefs(doc));
    if (doc) {
        return CallQueryInterface(doc, aOwnerDocument);
    }
    *aOwnerDocument = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetNamespaceURI(nsAString& aNamespaceURI)
{
    return NodeInfo()->GetNamespaceURI(aNamespaceURI);
}


NS_IMETHODIMP
nsXULElement::GetPrefix(nsAString& aPrefix)
{
    return NodeInfo()->GetPrefix(aPrefix);
}


NS_IMETHODIMP
nsXULElement::SetPrefix(const nsAString& aPrefix)
{
    // XXX: Validate the prefix string!

    nsINodeInfo *newNodeInfo = nsnull;
    nsCOMPtr<nsIAtom> prefix;

    if (!aPrefix.IsEmpty() && !DOMStringIsNull(aPrefix)) {
        prefix = do_GetAtom(aPrefix);
        NS_ENSURE_TRUE(prefix, NS_ERROR_OUT_OF_MEMORY);
    }

    nsresult rv = EnsureSlots();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mSlots->mNodeInfo->PrefixChanged(prefix, newNodeInfo);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(newNodeInfo, "trying to assign null nodeinfo!");
    mSlots->mNodeInfo = newNodeInfo;

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetLocalName(nsAString& aLocalName)
{
    return NodeInfo()->GetLocalName(aLocalName);
}


NS_IMETHODIMP
nsXULElement::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                           nsIDOMNode** aReturn)
{
    NS_PRECONDITION(aNewChild != nsnull, "null ptr");
    if (! aNewChild)
        return NS_ERROR_NULL_POINTER;

    // aRefChild may be null; that means "append".

    nsresult rv = nsContentUtils::CheckSameOrigin(this, aNewChild);
    if (NS_FAILED(rv))
        return rv;

    // We can't insert an ancestor or ourself.
    if (IsAncestor(aNewChild, this)) {
        return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }

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
            // Some frames assume that the document will have been set,
            // so pass in PR_TRUE for aDeepSetDocument here.
            rv = InsertChildAt(newcontent, pos, PR_TRUE, PR_TRUE);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to insert aNewChild");
            if (NS_FAILED(rv)) return rv;
        }

        // XXX Hmm. There's a case here that we handle ambiguously, I
        // think. If aRefChild _isn't_ actually one of our kids, then
        // pos == -1, and we'll never insert the new kid. Should we
        // just append it?
    }
    else {
        // Some frames assume that the document will have been set,
        // so pass in PR_TRUE for aDeepSetDocument here.
        rv = AppendChildTo(newcontent, PR_TRUE, PR_TRUE);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to append a aNewChild");
        if (NS_FAILED(rv)) return rv;
    }

    NS_ADDREF(aNewChild);
    *aReturn = aNewChild;
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                           nsIDOMNode** aReturn)
{
    NS_PRECONDITION(aNewChild != nsnull, "null ptr");
    if (! aNewChild)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aOldChild != nsnull, "null ptr");
    if (! aOldChild)
        return NS_ERROR_NULL_POINTER;

    nsresult rv = nsContentUtils::CheckSameOrigin(this, aNewChild);
    if (NS_FAILED(rv))
        return rv;

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
                // Some frames assume that the document will have been set,
                // so pass in PR_TRUE for aDeepSetDocument here.
                rv = ReplaceChildAt(newelement, pos, PR_TRUE, PR_TRUE);
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
nsXULElement::HasAttributes(PRBool* aReturn)
{
    if ((Attributes() && Attributes()->Count() > 0) ||
        (mPrototype && mPrototype->mNumAttributes > 0))
        *aReturn = PR_TRUE;
    else
        *aReturn = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
    nsresult rv;

    nsCOMPtr<nsIContent> result;

    // If we have a prototype, so will our clone.
    if (mPrototype) {
        rv = nsXULElement::Create(mPrototype, mDocument, PR_TRUE,
                                  getter_AddRefs(result));
    } else {
        NS_ASSERTION(mSlots, "no prototype and no slots!");
        if (!mSlots)
            return NS_ERROR_UNEXPECTED;

        rv = nsXULElement::Create(mSlots->mNodeInfo, getter_AddRefs(result));
        if (NS_SUCCEEDED(rv)) {
            rv = result->SetDocument(mDocument, PR_TRUE, PR_TRUE);
        }
    }
    if (NS_FAILED(rv)) return rv;

    if (mSlots) {
        nsXULAttributes *attrs = mSlots->GetAttributes();
        if (attrs) {
            // Copy attributes
            PRInt32 count = attrs->Count();
            for (PRInt32 i = 0; i < count; ++i) {
                nsXULAttribute* attr = attrs->ElementAt(i);
                NS_ASSERTION(attr != nsnull, "null ptr");
                if (! attr)
                    return NS_ERROR_UNEXPECTED;

                nsAutoString value;
                rv = attr->GetValue(value);
                if (NS_FAILED(rv)) return rv;

                rv = result->SetAttr(attr->GetNodeInfo(), value,
                                     PR_FALSE);
                if (NS_FAILED(rv)) return rv;
            }

            // XXX TODO: set up RDF generic builder n' stuff if there is a
            // 'datasources' attribute? This is really kind of tricky,
            // because then we'd need to -selectively- copy children that
            // -weren't- generated from RDF. Ugh. Forget it.
        }

        // Note that we're _not_ copying mBroadcastListeners,
        // mControllers, mInnerXULElement.
    }

    if (aDeep) {
        // Copy cloned children!
        PRInt32 count = mChildren.Count();
        for (PRInt32 i = 0; i < count; ++i) {
            nsIContent* child = NS_STATIC_CAST(nsIContent*, mChildren[i]);

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

            rv = result->AppendChildTo(newchild, PR_FALSE, PR_FALSE);
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
nsXULElement::IsSupported(const nsAString& aFeature,
                          const nsAString& aVersion,
                          PRBool* aReturn)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}


//----------------------------------------------------------------------
// nsIDOMElement interface

NS_IMETHODIMP
nsXULElement::GetTagName(nsAString& aTagName)
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
nsXULElement::GetAttribute(const nsAString& aName,
                           nsAString& aReturn)
{
    nsresult rv;
    PRInt32 nameSpaceID;
    nsCOMPtr<nsIAtom> nameAtom;
    nsCOMPtr<nsINodeInfo> nodeInfo;

    if (NS_FAILED(rv = NormalizeAttrString(aName,
                                           *getter_AddRefs(nodeInfo)))) {
        NS_WARNING("unable to normalize attribute name");
        return rv;
    }

    nodeInfo->GetNameAtom(*getter_AddRefs(nameAtom));
    nodeInfo->GetNamespaceID(nameSpaceID);

    GetAttr(nameSpaceID, nameAtom, aReturn);
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::SetAttribute(const nsAString& aName,
                           const nsAString& aValue)
{
    nsresult rv;

    nsCOMPtr<nsINodeInfo> ni;

    rv = NormalizeAttrString(aName, *getter_AddRefs(ni));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to normalize attribute name");

    if (NS_SUCCEEDED(rv)) {
        rv = SetAttr(ni, aValue, PR_TRUE);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set attribute");
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::RemoveAttribute(const nsAString& aName)
{
    nsresult rv;

    PRInt32 nameSpaceID;
    nsCOMPtr<nsIAtom> tag;
    nsCOMPtr<nsINodeInfo> ni;

    rv = NormalizeAttrString(aName, *getter_AddRefs(ni));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to parse attribute name");

    if (NS_SUCCEEDED(rv)) {
        ni->GetNameAtom(*getter_AddRefs(tag));
        ni->GetNamespaceID(nameSpaceID);

        rv = UnsetAttr(nameSpaceID, tag, PR_TRUE);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to remove attribute");
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetAttributeNode(const nsAString& aName,
                               nsIDOMAttr** aReturn)
{
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
        return CallQueryInterface(node, aReturn);
    }

    *aReturn = nsnull;

    return NS_OK;
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
nsXULElement::GetElementsByTagName(const nsAString& aTagname,
                                   nsIDOMNodeList** aReturn)
{
    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aTagname);
    NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<nsIContentList> list;
    NS_GetContentList(mDocument, nameAtom, kNameSpaceID_Unknown, this,
                      getter_AddRefs(list));
    NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

    return CallQueryInterface(list, aReturn);
}

NS_IMETHODIMP
nsXULElement::GetAttributeNS(const nsAString& aNamespaceURI,
                             const nsAString& aLocalName,
                             nsAString& aReturn)
{
    nsCOMPtr<nsIAtom> name = do_GetAtom(aLocalName);
    PRInt32 nsid;

    nsContentUtils::GetNSManagerWeakRef()->GetNameSpaceID(aNamespaceURI,
                                                          nsid);

    if (nsid == kNameSpaceID_Unknown) {
        // Unkonwn namespace means no attr...

        aReturn.Truncate();
        return NS_OK;
    }

    GetAttr(nsid, name, aReturn);

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetAttributeNS(const nsAString& aNamespaceURI,
                             const nsAString& aQualifiedName,
                             const nsAString& aValue)
{
    nsCOMPtr<nsINodeInfoManager> nimgr;
    nsresult rv = NodeInfo()->GetNodeInfoManager(*getter_AddRefs(nimgr));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsINodeInfo> ni;
    rv = nimgr->GetNodeInfo(aQualifiedName, aNamespaceURI, *getter_AddRefs(ni));
    NS_ENSURE_SUCCESS(rv, rv);

    return SetAttr(ni, aValue, PR_TRUE);
}

NS_IMETHODIMP
nsXULElement::RemoveAttributeNS(const nsAString& aNamespaceURI,
                                const nsAString& aLocalName)
{
    PRInt32 nameSpaceId;
    nsCOMPtr<nsIAtom> tag = do_GetAtom(aLocalName);

    nsContentUtils::GetNSManagerWeakRef()->GetNameSpaceID(aNamespaceURI,
                                                          nameSpaceId);

    return UnsetAttr(nameSpaceId, tag, PR_TRUE);
}

NS_IMETHODIMP
nsXULElement::GetAttributeNodeNS(const nsAString& aNamespaceURI,
                                 const nsAString& aLocalName,
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
        return CallQueryInterface(node, aReturn);
    }

    *aReturn = nsnull;

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetAttributeNodeNS(nsIDOMAttr* aNewAttr,
                                 nsIDOMAttr** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULElement::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                     const nsAString& aLocalName,
                                     nsIDOMNodeList** aReturn)
{
    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aLocalName);
    NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

    PRInt32 nameSpaceId = kNameSpaceID_Unknown;

    nsCOMPtr<nsIContentList> list;

    if (!aNamespaceURI.Equals(NS_LITERAL_STRING("*"))) {
        nsContentUtils::GetNSManagerWeakRef()->GetNameSpaceID(aNamespaceURI,
                                                              nameSpaceId);

        if (nameSpaceId == kNameSpaceID_Unknown) {
            // Unknown namespace means no matches, we create an empty list...
            NS_GetContentList(mDocument, nsnull, kNameSpaceID_None, nsnull,
                              getter_AddRefs(list));
            NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);
        }
    }

    if (!list) {
        NS_GetContentList(mDocument, nameAtom, nameSpaceId, this,
                          getter_AddRefs(list));
        NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);
    }

    return CallQueryInterface(list, aReturn);
}

NS_IMETHODIMP
nsXULElement::HasAttribute(const nsAString& aName, PRBool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  nsCOMPtr<nsIAtom> name;
  nsCOMPtr<nsINodeInfo> ni;
  PRInt32 nsid;

  nsresult rv = NormalizeAttrString(aName, *getter_AddRefs(ni));
  NS_ENSURE_SUCCESS(rv, rv);

  ni->GetNameAtom(*getter_AddRefs(name));
  ni->GetNamespaceID(nsid);

  *aReturn = HasAttr(nsid, name);
  return NS_OK;
}

NS_IMETHODIMP
nsXULElement::HasAttributeNS(const nsAString& aNamespaceURI,
                             const nsAString& aLocalName,
                             PRBool* aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);

    nsCOMPtr<nsIAtom> name = do_GetAtom(aLocalName);
    PRInt32 nsid;

    nsContentUtils::GetNSManagerWeakRef()->GetNameSpaceID(aNamespaceURI, nsid);

    if (nsid == kNameSpaceID_Unknown) {
        // Unkonwn namespace means no attr...
        *aReturn = PR_FALSE;
        return NS_OK;
    }

    *aReturn = HasAttr(nsid, name);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetElementsByAttribute(const nsAString& aAttribute,
                                     const nsAString& aValue,
                                     nsIDOMNodeList** aReturn)
{
    // XXX This should use nsContentList, but that does not support
    // _two_ strings being passed to the match func.  Ah, the ability
    // to create real closures, where art thou?
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
nsXULElement::MaybeTriggerAutoLink(nsIDocShell *aShell)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetXMLBaseURI(nsIURI **aURI)
{
  // XXX TODO, should share the impl with nsXMLElement
  NS_ENSURE_ARG_POINTER(aURI);
  *aURI=nsnull;
  if (mDocument) {
    mDocument->GetBaseURL(*aURI);
    if (!*aURI) {
      mDocument->GetDocumentURL(aURI);
    }
  }
  return NS_OK;
}

#if 0
NS_IMETHODIMP
nsXULElement::GetBaseURI(nsAString &aURI)
{
  // XXX TODO, should share the impl with nsXMLElement
  aURI.Truncate();
  nsresult rv = NS_OK;
  if (mDocument) {
    nsCOMPtr<nsIDOMDocument> doc(do_QueryInterface(mDocument));
    if (doc) {
      rv = doc->GetBaseURI(aURI);
    }
  }
  return rv;
}
#endif

//----------------------------------------------------------------------
// nsIXULContent interface

NS_IMETHODIMP
nsXULElement::PeekChildCount(PRInt32& aCount) const
{
    aCount = mChildren.Count();
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetLazyState(LazyState aFlags)
{
    nsresult rv = EnsureSlots();
    if (NS_FAILED(rv)) return rv;

    LazyState flags = mSlots->GetLazyState();
    mSlots->SetLazyState(LazyState(flags | aFlags));

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::ClearLazyState(LazyState aFlags)
{
    // No need to clear a flag we've never set.
    if (mSlots) {
        LazyState flags = mSlots->GetLazyState();
        mSlots->SetLazyState(LazyState(flags & ~aFlags));
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetLazyState(LazyState aFlag, PRBool& aResult)
{
    if (mSlots) {
        LazyState flags = mSlots->GetLazyState();
        aResult = flags & aFlag;
    }
    else
        aResult = PR_FALSE;

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::AddScriptEventListener(nsIAtom* aName,
                                     const nsAString& aValue)
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

        if (!context) return NS_OK;
    }

    nsCOMPtr<nsIContent> root;
    mDocument->GetRootContent(getter_AddRefs(root));
    nsCOMPtr<nsIContent> content(do_QueryInterface(NS_STATIC_CAST(nsIStyledContent*, this)));
    if ((!root || root == content) && !NodeInfo()->Equals(nsXULAtoms::overlay)) {
        nsCOMPtr<nsIDOMEventReceiver> receiver = do_QueryInterface(global);
        if (! receiver)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIEventListenerManager> manager;
        rv = receiver->GetListenerManager(getter_AddRefs(manager));
        if (NS_FAILED(rv)) return rv;

        rv = manager->AddScriptEventListener(context, global, aName,
                                             aValue, PR_FALSE);
    }
    else {
        nsCOMPtr<nsIEventListenerManager> manager;
        rv = GetListenerManager(getter_AddRefs(manager));
        if (NS_FAILED(rv)) return rv;

        rv = manager->AddScriptEventListener(context,
                                             NS_STATIC_CAST(nsIContent *,
                                                            this),
                                             aName, aValue, PR_TRUE);
    }

    return rv;
}

NS_IMETHODIMP
nsXULElement::GetListenerManager(nsIEventListenerManager** aResult)
{
    if (!mListenerManager) {
        nsresult rv;
        mListenerManager = do_CreateInstance(kEventListenerManagerCID, &rv);
        if (NS_FAILED(rv))
            return rv;

        mListenerManager->SetListenerTarget(NS_STATIC_CAST(nsIStyledContent*, this));
    }

    *aResult = mListenerManager;
    NS_ADDREF(*aResult);
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::DoneCreatingElement()
{
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
                                  const nsAString& aBody,
                                  void** aHandler)
{
    nsresult rv;
    JSObject* scopeObject;

    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheSets);

    if (mPrototype) {
        // It'll be shared among the instances of the prototype.
        // Use null for the scope object when precompiling shared
        // prototype scripts.
        scopeObject = nsnull;
    }
    else {
        // We don't have a prototype; do a one-off compile.
        NS_ASSERTION(aTarget != nsnull, "no prototype and no target?!");
        scopeObject = NS_REINTERPRET_CAST(JSObject*, aTarget);
    }

    // Compile the event handler
    rv = aContext->CompileEventHandler(scopeObject, aName, aBody, !scopeObject,
                                       aHandler);
    if (NS_FAILED(rv)) return rv;

    if (! scopeObject) {
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

                if (attr->mEventHandler) {
                    JSContext *cx = (JSContext*) aContext->GetNativeContext();
                    if (!cx)
                        return NS_ERROR_UNEXPECTED;

                    rv = AddJSGCRoot(&attr->mEventHandler,
                                     "nsXULPrototypeAttribute::mEventHandler");
                    if (NS_FAILED(rv)) return rv;
                }

                break;
            }
        }
    }

    return NS_OK;
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

nsresult
nsXULElement::AddListenerFor(nsINodeInfo *aNodeInfo,
                             PRBool aCompileEventHandlers)
{
    // If appropriate, add a popup listener and/or compile the event
    // handler. Called when we change the element's document, create a
    // new element, change an attribute's value, etc.
    PRInt32 nameSpaceID;
    aNodeInfo->GetNamespaceID(nameSpaceID);

    if (nameSpaceID == kNameSpaceID_None) {
        nsCOMPtr<nsIAtom> attr;
        aNodeInfo->GetNameAtom(*getter_AddRefs(attr));

        if (attr == nsXULAtoms::menu ||
            attr == nsXULAtoms::contextmenu ||
            // XXXdwh popup and context are deprecated
            attr == nsXULAtoms::popup ||
            attr == nsXULAtoms::context) {
            AddPopupListener(attr);
        }

        if (aCompileEventHandlers) {
            nsIID iid;
            PRBool isHandler = PR_FALSE;
            GetEventHandlerIID(attr, &iid, &isHandler);

            if (isHandler) {
                nsAutoString value;
                GetAttr(nameSpaceID, attr, value);
                AddScriptEventListener(attr, value);
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetDocument(nsIDocument* aDocument, PRBool aDeep, PRBool aCompileEventHandlers)
{
    if (aDocument != mDocument) {
        nsCOMPtr<nsIXULDocument> rdfDoc;

        if (mDocument) {
          // Notify XBL- & nsIAnonymousContentCreator-generated
          // anonymous content that the document is changing.
          nsCOMPtr<nsIBindingManager> bindingManager;
          mDocument->GetBindingManager(getter_AddRefs(bindingManager));
          NS_ASSERTION(bindingManager, "no binding manager");
          if (bindingManager) {
            bindingManager->ChangeDocumentFor(NS_STATIC_CAST(nsIStyledContent*, this), mDocument, aDocument);
          }

          nsIDOMElement* domElement = NS_STATIC_CAST(nsIDOMElement*, this);
          nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(mDocument));
          nsDoc->SetBoxObjectFor(domElement, nsnull);
        }

        // mControllers can own objects that are implemented
        // in JavaScript (such as some implementations of
        // nsIControllers.  These objects prevent their global
        // object's script object from being garbage collected,
        // which means JS continues to hold an owning reference
        // to the GlobalWindowImpl, which owns the document,
        // which owns this content.  That's a cycle, so we break
        // it here.  (It might be better to break this by releasing
        // mDocument in GlobalWindowImpl::SetDocShell, but I'm not
        // sure whether that would fix all possible cycles through
        // mControllers.)
        if (!aDocument && mSlots) {
            mSlots->mControllers = nsnull; // Forces release
        }

        if (mListenerManager)
          mListenerManager->SetListenerTarget(nsnull);
        mListenerManager = nsnull;

        mDocument = aDocument; // not refcounted

        if (mDocument) {
            // When we SetDocument(), we're either adding an element
            // into the document that wasn't there before, or we're
            // moving the element from one document to
            // another. Regardless, we need to (re-)initialize several
            // attributes that are dependant on the document. Do that
            // now.
            PRBool haveLocalAttributes = PR_FALSE;
            nsXULAttributes *attrs = Attributes();
            if (attrs) {
                PRInt32 count = attrs->Count();
                haveLocalAttributes = (count > 0);
                for (PRInt32 i = 0; i < count; i++) {
                    nsXULAttribute *xulattr =
                        NS_REINTERPRET_CAST(nsXULAttribute *,
                                            attrs->ElementAt(i));

                    AddListenerFor(xulattr->GetNodeInfo(),
                                   aCompileEventHandlers);
                }
            }

            if (mPrototype) {
                PRInt32 count = mPrototype->mNumAttributes;
                for (PRInt32 i = 0; i < count; i++) {
                    nsXULPrototypeAttribute *protoattr;
                    protoattr = &(mPrototype->mAttributes[i]);

                    // Don't clobber a locally modified attribute.
                    if (haveLocalAttributes &&
                        FindLocalAttribute(protoattr->mNodeInfo)) {
                        continue;
                    }

                    AddListenerFor(protoattr->mNodeInfo,
                                   aCompileEventHandlers);
                }
            }
        }
    }

    if (aDeep) {
        for (PRInt32 i = mChildren.Count() - 1; i >= 0; --i) {
            nsIContent* child = NS_STATIC_CAST(nsIContent*, mChildren[i]);
            child->SetDocument(aDocument, aDeep, aCompileEventHandlers);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetParent(nsIContent*& aResult) const
{
    aResult = mParent;
    NS_IF_ADDREF(aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetParent(nsIContent* aParent)
{
    mParent = aParent; // no refcount
    if (aParent) {
      nsCOMPtr<nsIContent> bindingPar;
      aParent->GetBindingParent(getter_AddRefs(bindingPar));
      if (bindingPar)
        SetBindingParent(bindingPar);
    }

    return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsXULElement::IsNativeAnonymous() const
{
    return PR_FALSE;
}

NS_IMETHODIMP_(void)
nsXULElement::SetNativeAnonymous(PRBool aAnonymous)
{
    // XXX Need to make this actually do something - bug 165110
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

    nsIContent* content = NS_STATIC_CAST(nsIContent*, mChildren.SafeElementAt(aIndex));
    aResult = content;
    NS_IF_ADDREF(aResult);

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

    aResult = mChildren.IndexOf(aPossibleChild);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify,
                            PRBool aDeepSetDocument)
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated()))
        return rv;

    NS_PRECONDITION(nsnull != aKid, "null ptr");

    // Make sure that we're not trying to insert the same child
    // twice. If we do, the DOM APIs (e.g., GetNextSibling()), will
    // freak out.
    NS_ASSERTION(mChildren.IndexOf(aKid) < 0, "element is already a child");

    if (!mChildren.InsertElementAt(aKid, aIndex))
        return NS_ERROR_FAILURE;

    NS_ADDREF(aKid);
    aKid->SetParent(NS_STATIC_CAST(nsIStyledContent*, this));
    //nsRange::OwnerChildInserted(this, aIndex);

    aKid->SetDocument(mDocument, aDeepSetDocument, PR_TRUE);

    if (mDocument && HasMutationListeners(NS_STATIC_CAST(nsIStyledContent*,this),
                                          NS_EVENT_BITS_MUTATION_NODEINSERTED)) {
      nsCOMPtr<nsIDOMEventTarget> node(do_QueryInterface(aKid));
      nsMutationEvent mutation;
      mutation.eventStructType = NS_MUTATION_EVENT;
      mutation.message = NS_MUTATION_NODEINSERTED;
      mutation.mTarget = node;

      nsCOMPtr<nsIDOMNode> relNode(do_QueryInterface(NS_STATIC_CAST(nsIStyledContent*,this)));
      mutation.mRelatedNode = relNode;

      nsEventStatus status = nsEventStatus_eIgnore;
      aKid->HandleDOMEvent(nsnull, &mutation, nsnull, NS_EVENT_FLAG_INIT, &status);
    }

    if (aNotify && mDocument) {
      mDocument->ContentInserted(NS_STATIC_CAST(nsIStyledContent*, this), aKid, aIndex);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify,
                             PRBool aDeepSetDocument)
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated()))
        return rv;

    NS_PRECONDITION(nsnull != aKid, "null ptr");
    if (! aKid)
        return NS_ERROR_NULL_POINTER;

    nsIContent* oldKid = NS_STATIC_CAST(nsIContent*, mChildren[aIndex]);
    NS_ASSERTION(oldKid != nsnull, "old kid not nsIContent");
    if (! oldKid)
        return NS_ERROR_FAILURE;

    if (oldKid == aKid)
        return NS_OK;

    PRBool replaceOk = mChildren.ReplaceElementAt(aKid, aIndex);
    if (replaceOk) {
        NS_ADDREF(aKid);
        aKid->SetParent(NS_STATIC_CAST(nsIStyledContent*, this));
        //nsRange::OwnerChildReplaced(this, aIndex, oldKid);

        aKid->SetDocument(mDocument, aDeepSetDocument, PR_TRUE);

        if (aNotify && mDocument) {
            mDocument->ContentReplaced(NS_STATIC_CAST(nsIStyledContent*, this), oldKid, aKid, aIndex);
        }

        // This will cause the script object to be unrooted for each
        // element in the subtree.
        oldKid->SetDocument(nsnull, PR_TRUE, PR_TRUE);

        // We've got no mo' parent.
        oldKid->SetParent(nsnull);
        NS_RELEASE(oldKid);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::AppendChildTo(nsIContent* aKid, PRBool aNotify, PRBool aDeepSetDocument)
{
    nsresult rv;
    if (NS_FAILED(rv = EnsureContentsGenerated()))
        return rv;

    NS_PRECONDITION((nsnull != aKid) && (aKid != NS_STATIC_CAST(nsIStyledContent*, this)), "null ptr");

    PRBool appendOk = mChildren.AppendElement(aKid);
    if (appendOk) {
        NS_ADDREF(aKid);
        aKid->SetParent(NS_STATIC_CAST(nsIStyledContent*, this));
        // ranges don't need adjustment since new child is at end of list

        aKid->SetDocument(mDocument, aDeepSetDocument, PR_TRUE);

        if (mDocument && HasMutationListeners(NS_STATIC_CAST(nsIStyledContent*,this),
                                              NS_EVENT_BITS_MUTATION_NODEINSERTED)) {
          nsCOMPtr<nsIDOMEventTarget> node(do_QueryInterface(aKid));
          nsMutationEvent mutation;
          mutation.eventStructType = NS_MUTATION_EVENT;
          mutation.message = NS_MUTATION_NODEINSERTED;
          mutation.mTarget = node;

          nsCOMPtr<nsIDOMNode> relNode(do_QueryInterface(NS_STATIC_CAST(nsIStyledContent*,this)));
          mutation.mRelatedNode = relNode;

          nsEventStatus status = nsEventStatus_eIgnore;
          aKid->HandleDOMEvent(nsnull, &mutation, nsnull, NS_EVENT_FLAG_INIT, &status);
        }

        if (aNotify && mDocument) {
            mDocument->ContentAppended(NS_STATIC_CAST(nsIStyledContent*, this), mChildren.Count() - 1);
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

    nsIContent* oldKid = NS_STATIC_CAST(nsIContent*, mChildren[aIndex]);
    if (! oldKid)
        return NS_ERROR_FAILURE;

    if (HasMutationListeners(NS_STATIC_CAST(nsIStyledContent*,this), NS_EVENT_BITS_MUTATION_NODEREMOVED)) {
      nsCOMPtr<nsIDOMEventTarget> node(do_QueryInterface(oldKid));
      nsMutationEvent mutation;
      mutation.eventStructType = NS_MUTATION_EVENT;
      mutation.message = NS_MUTATION_NODEREMOVED;
      mutation.mTarget = node;

      nsCOMPtr<nsIDOMNode> relNode(do_QueryInterface(NS_STATIC_CAST(nsIStyledContent*,this)));
      mutation.mRelatedNode = relNode;

      nsEventStatus status = nsEventStatus_eIgnore;
      oldKid->HandleDOMEvent(nsnull, &mutation, nsnull, NS_EVENT_FLAG_INIT, &status);
    }

    // On the removal of a <treeitem>, <treechildren>, or <treecell> element,
    // the possibility exists that some of the items in the removed subtree
    // are selected (and therefore need to be deselected). We need to account for this.
    nsCOMPtr<nsIAtom> tag;
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> controlElement;
    nsCOMPtr<nsIListBoxObject> listBox;
    PRBool fireSelectionHandler = PR_FALSE;

    // -1 = do nothing, -2 = null out current item
    // anything else = index to re-set as current
    PRInt32 newCurrentIndex = -1;

    oldKid->GetTag(*getter_AddRefs(tag));
    if (tag == nsXULAtoms::listitem) {
      // This is the nasty case. We have (potentially) a slew of selected items
      // and cells going away.
      // First, retrieve the tree.
      // Check first whether this element IS the tree
      controlElement = do_QueryInterface((nsIDOMXULElement*)this);

      // If it's not, look at our parent
      if (!controlElement)
        rv = GetParentTree(getter_AddRefs(controlElement));
      if (controlElement) {
        nsCOMPtr<nsIDOMNode> parentKid = do_QueryInterface(oldKid);
        // Iterate over all of the items and find out if they are contained inside
        // the removed subtree.
        PRInt32 length;
        controlElement->GetSelectedCount(&length);
        for (PRInt32 i = 0; i < length; i++) {
          nsCOMPtr<nsIDOMXULSelectControlItemElement> node;
          controlElement->GetSelectedItem(i, getter_AddRefs(node));
          nsCOMPtr<nsIDOMNode> selNode(do_QueryInterface(node));
          if (selNode == parentKid && NS_SUCCEEDED(rv = controlElement->RemoveItemFromSelection(node))) {
            length--;
            i--;
            fireSelectionHandler = PR_TRUE;
          }
        }

        nsCOMPtr<nsIDOMXULSelectControlItemElement> curItem;
        controlElement->GetCurrentItem(getter_AddRefs(curItem));
        nsCOMPtr<nsIDOMNode> curNode = do_QueryInterface(curItem);
        if (IsAncestor(parentKid, curNode)) {
            // Current item going away
            nsCOMPtr<nsIBoxObject> box;
            controlElement->GetBoxObject(getter_AddRefs(box));
            listBox = do_QueryInterface(box);
            if (listBox) {
                nsCOMPtr<nsIDOMElement> domElem = do_QueryInterface(parentKid);
                if (domElem)
                    listBox->GetIndexOfItem(domElem, &newCurrentIndex);
            }

            // If any of this fails, we'll just set the current item to null
            if (newCurrentIndex == -1)
              newCurrentIndex = -2;
        }
      }
    }

    if (oldKid) {
        nsIDocument* doc = mDocument;
        PRBool removeOk = mChildren.RemoveElementAt(aIndex);
        //nsRange::OwnerChildRemoved(this, aIndex, oldKid);
        if (aNotify && removeOk && mDocument) {
            doc->ContentRemoved(NS_STATIC_CAST(nsIStyledContent*, this), oldKid, aIndex);
        }

        if (newCurrentIndex == -2)
            controlElement->SetCurrentItem(nsnull);
        else if (newCurrentIndex > -1) {
            // Make sure the index is still valid
            PRInt32 treeRows;
            listBox->GetRowCount(&treeRows);
            if (treeRows > 0) {
                newCurrentIndex = PR_MIN((treeRows - 1), newCurrentIndex);
                nsCOMPtr<nsIDOMElement> newCurrentItem;
                listBox->GetItemAtIndex(newCurrentIndex, getter_AddRefs(newCurrentItem));
                if (newCurrentItem) {
                    nsCOMPtr<nsIDOMXULSelectControlItemElement> xulCurItem = do_QueryInterface(newCurrentItem);
                    if (xulCurItem)
                        controlElement->SetCurrentItem(xulCurItem);
                }
            } else {
                controlElement->SetCurrentItem(nsnull);
            }
        }

        if (fireSelectionHandler) {
          nsCOMPtr<nsIDOMDocumentEvent> doc(do_QueryInterface(mDocument));
          nsCOMPtr<nsIDOMEvent> event;
          doc->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
          if (event) {
            event->InitEvent(NS_LITERAL_STRING("select"), PR_FALSE, PR_TRUE);
            PRBool noDefault;
            nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(NS_STATIC_CAST(nsIContent *, this)));
            NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);
            target->DispatchEvent(event, &noDefault);
          }
        }

        // This will cause the script object to be unrooted for each
        // element in the subtree.
        oldKid->SetDocument(nsnull, PR_TRUE, PR_TRUE);

        // We've got no mo' parent.
        oldKid->SetParent(nsnull);
        NS_RELEASE(oldKid);
    }

    return NS_OK;
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
nsXULElement::NormalizeAttrString(const nsAString& aStr,
                                  nsINodeInfo*& aNodeInfo)
{
    PRInt32 i, count = Attributes() ? Attributes()->Count() : 0;
    NS_ConvertUCS2toUTF8 utf8String(aStr);
    
    for (i = 0; i < count; i++) {
        nsXULAttribute* attr = NS_REINTERPRET_CAST(nsXULAttribute*,
                                                   Attributes()->ElementAt(i));
        nsINodeInfo *ni = attr->GetNodeInfo();
        if (ni->QualifiedNameEquals(utf8String)) {
            aNodeInfo = ni;
            NS_ADDREF(aNodeInfo);

            return NS_OK;
        }
    }

    count = mPrototype ? mPrototype->mNumAttributes : 0;
    for (i = 0; i < count; i++) {
        nsXULPrototypeAttribute* attr = &(mPrototype->mAttributes[i]);

        nsINodeInfo *ni = attr->mNodeInfo;
        if (ni->QualifiedNameEquals(utf8String)) {
            aNodeInfo = ni;
            NS_ADDREF(aNodeInfo);

            return NS_OK;
        }
    }

    nsCOMPtr<nsINodeInfoManager> nimgr;
    NodeInfo()->GetNodeInfoManager(*getter_AddRefs(nimgr));
    NS_ENSURE_TRUE(nimgr, NS_ERROR_FAILURE);

    return nimgr->GetNodeInfo(aStr, nsnull, kNameSpaceID_None, aNodeInfo);
}

void
nsXULElement::UnregisterAccessKey(const nsAString& aOldValue)
{
    // If someone changes the accesskey, unregister the old one
    //
    if (mDocument && !aOldValue.IsEmpty()) {
        nsCOMPtr<nsIPresShell> shell;
        mDocument->GetShellAt(0, getter_AddRefs(shell));

        if (shell) {
            PRBool validElement = PR_TRUE;

            // find out what type of content node this is
            nsCOMPtr<nsIAtom> atom;
            nsresult rv = GetTag(*getter_AddRefs(atom));
            if (NS_SUCCEEDED(rv) && atom) {
                if (atom == nsXULAtoms::label) {
                    // XXXjag a side-effect is that we filter out anonymous <label>s
                    // in e.g. <menu>, <menuitem>, <button>. These <label>s inherit
                    // |accesskey| and would otherwise register themselves, overwriting
                    // the content we really meant to be registered.
                    if (!HasAttr(kNameSpaceID_None, nsXULAtoms::control))
                        validElement = PR_FALSE;
                }
            }

            if (validElement) {
                nsCOMPtr<nsIPresContext> presContext;
                shell->GetPresContext(getter_AddRefs(presContext));

                nsCOMPtr<nsIEventStateManager> esm;
                presContext->GetEventStateManager(getter_AddRefs(esm));

                nsIContent* content = NS_STATIC_CAST(nsIContent*, this);
                esm->UnregisterAccessKey(content, aOldValue.First());
            }
        }
    }
}

// XXX attribute code swiped from nsGenericContainerElement
// this class could probably just use nsGenericContainerElement
// needed to maintain attribute namespace ID as well as ordering
NS_IMETHODIMP
nsXULElement::SetAttr(nsINodeInfo* aNodeInfo,
                      const nsAString& aValue,
                      PRBool aNotify)
{
    NS_ASSERTION(nsnull != aNodeInfo, "must have attribute nodeinfo");
    if (nsnull == aNodeInfo)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    nsCOMPtr<nsIAtom> attrName;
    PRInt32 attrns;

    aNodeInfo->GetNameAtom(*getter_AddRefs(attrName));
    aNodeInfo->GetNamespaceID(attrns);

    // XXXwaterson should likely also be conditioned on aNotify. Do we
    // need to BeginUpdate() here as well?
    if (mDocument) {
        mDocument->AttributeWillChange(this, attrns, attrName);
    }

    rv = EnsureAttributes();
    if (NS_FAILED(rv)) return rv;

    // XXX Class and Style attribute setting should be checking for the XUL namespace!

    // Check to see if the CLASS attribute is being set.  If so, we need to rebuild our
    // class list.
    if (aNodeInfo->Equals(nsXULAtoms::clazz, kNameSpaceID_None)) {
        Attributes()->UpdateClassList(aValue);
    }

    // Check to see if the STYLE attribute is being set.  If so, we need to create a new
    // style rule based off the value of this attribute, and we need to let the document
    // know about the StyleRule change.
    if (aNodeInfo->Equals(nsXULAtoms::style, kNameSpaceID_None) &&
        (mDocument != nsnull)) {
        nsCOMPtr <nsIURI> docURL;
        mDocument->GetBaseURL(*getter_AddRefs(docURL));
        Attributes()->UpdateStyleRule(docURL, aValue);
        // XXX Some kind of special document update might need to happen here.
    }

    nsCOMPtr<nsIAtom> tag;
    GetTag(*getter_AddRefs(tag));

    if (tag == nsXULAtoms::window &&
        aNodeInfo->Equals(nsXULAtoms::hidechrome)) {
      nsAutoString val;
      val.Assign(aValue);
      HideWindowChrome(val.EqualsIgnoreCase("true"));
    }

    // XXX need to check if they're changing an event handler: if so, then we need
    // to unhook the old one.

    nsXULAttribute* attr = FindLocalAttribute(aNodeInfo);
    PRBool modification;
    nsAutoString oldValue;

    if (attr) {
        attr->GetValue(oldValue);
        attr->SetValueInternal(aValue);
        modification = PR_TRUE;
    }
    else {
        // Don't have it locally, but might be shadowing a prototype attribute.
        nsXULPrototypeAttribute *protoattr = FindPrototypeAttribute(aNodeInfo);
        if (protoattr) {
            protoattr->mValue.GetValue(oldValue);
            modification = PR_TRUE;
        } else {
            modification = PR_FALSE;
        }

        rv = nsXULAttribute::Create(NS_STATIC_CAST(nsIStyledContent*, this),
                                    aNodeInfo, aValue, &attr);
        if (NS_FAILED(rv)) return rv;

        // transfer ownership here...
        nsXULAttributes *attrs = mSlots->GetAttributes();
        attrs->AppendElement(attr);
    }

    // Add popup and event listeners
    AddListenerFor(aNodeInfo, PR_TRUE);

    // If the accesskey attribute changes, unregister it here.
    // It will be registered for the new value in the relevant frames.
    // Also see nsAreaFrame, nsBoxFrame and nsTextBoxFrame's AttributeChanged
    if (aNodeInfo->Equals(nsXULAtoms::accesskey, kNameSpaceID_None))
        UnregisterAccessKey(oldValue);

    if (mDocument) {
      nsCOMPtr<nsIBindingManager> bindingManager;
      mDocument->GetBindingManager(getter_AddRefs(bindingManager));
      nsCOMPtr<nsIXBLBinding> binding;
      bindingManager->GetBinding(NS_STATIC_CAST(nsIStyledContent*, this), getter_AddRefs(binding));

      if (binding)
        binding->AttributeChanged(attrName, attrns, PR_FALSE, aNotify);

      if (HasMutationListeners(NS_STATIC_CAST(nsIStyledContent*, this), NS_EVENT_BITS_MUTATION_ATTRMODIFIED)) {
        nsCOMPtr<nsIDOMEventTarget> node(do_QueryInterface(NS_STATIC_CAST(nsIStyledContent*, this)));
        nsMutationEvent mutation;
        mutation.eventStructType = NS_MUTATION_EVENT;
        mutation.message = NS_MUTATION_ATTRMODIFIED;
        mutation.mTarget = node;

        nsAutoString attrName2;
        attrName->ToString(attrName2);
        nsCOMPtr<nsIDOMAttr> attrNode;
        GetAttributeNode(attrName2, getter_AddRefs(attrNode));
        mutation.mRelatedNode = attrNode;

        mutation.mAttrName = attrName;
        if (!oldValue.IsEmpty())
          mutation.mPrevAttrValue = do_GetAtom(oldValue);
        if (!aValue.IsEmpty())
          mutation.mNewAttrValue = do_GetAtom(aValue);
        if (modification)
          mutation.mAttrChange = nsIDOMMutationEvent::MODIFICATION;
        else
          mutation.mAttrChange = nsIDOMMutationEvent::ADDITION;
        nsEventStatus status = nsEventStatus_eIgnore;
        HandleDOMEvent(nsnull, &mutation, nsnull, NS_EVENT_FLAG_INIT, &status);
      }

      if (aNotify) {
        PRInt32 modHint = modification
            ? PRInt32(nsIDOMMutationEvent::MODIFICATION)
            : PRInt32(nsIDOMMutationEvent::ADDITION);

        mDocument->AttributeChanged(this, attrns, attrName, modHint,
                                    StyleHintFor(NodeInfo()));

        // XXXwaterson do we need to mDocument->EndUpdate() here?
      }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetAttr(PRInt32 aNameSpaceID,
                      nsIAtom* aName,
                      const nsAString& aValue,
                      PRBool aNotify)
{
    nsCOMPtr<nsINodeInfoManager> nimgr;

    NodeInfo()->GetNodeInfoManager(*getter_AddRefs(nimgr));
    NS_ENSURE_TRUE(nimgr, NS_ERROR_FAILURE);

    nsCOMPtr<nsINodeInfo> ni;
    nimgr->GetNodeInfo(aName, nsnull, aNameSpaceID, *getter_AddRefs(ni));

    return SetAttr(ni, aValue, aNotify);
}

NS_IMETHODIMP
nsXULElement::GetAttr(PRInt32 aNameSpaceID,
                      nsIAtom* aName,
                      nsAString& aResult) const
{
    nsCOMPtr<nsIAtom> prefix;
    return GetAttr(aNameSpaceID, aName, *getter_AddRefs(prefix), aResult);
}

NS_IMETHODIMP
nsXULElement::GetAttr(PRInt32 aNameSpaceID,
                      nsIAtom* aName,
                      nsIAtom*& aPrefix,
                      nsAString& aResult) const
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    if (nsnull == aName) {
        return NS_ERROR_NULL_POINTER;
    }

    if (mSlots) {
        nsXULAttributes *attrs = mSlots->GetAttributes();
        if (attrs) {
            PRInt32 count = attrs->Count();
            for (PRInt32 i = 0; i < count; i++) {
                nsXULAttribute* attr = NS_STATIC_CAST(nsXULAttribute*,
                                                      attrs->ElementAt(i));

                nsINodeInfo *ni = attr->GetNodeInfo();
                if (ni->Equals(aName, aNameSpaceID)) {
                    ni->GetPrefixAtom(aPrefix);
                    attr->GetValue(aResult);
                    return aResult.IsEmpty() ? NS_CONTENT_ATTR_NO_VALUE : NS_CONTENT_ATTR_HAS_VALUE;
                }
            }
        }
    }

    if (mPrototype) {
        PRInt32 count = mPrototype->mNumAttributes;
        for (PRInt32 i = 0; i < count; i++) {
            nsXULPrototypeAttribute* attr = &(mPrototype->mAttributes[i]);

            nsINodeInfo *ni = attr->mNodeInfo;
            if (ni->Equals(aName, aNameSpaceID)) {
                ni->GetPrefixAtom(aPrefix);
                attr->mValue.GetValue( aResult );
                return aResult.IsEmpty() ? NS_CONTENT_ATTR_NO_VALUE : NS_CONTENT_ATTR_HAS_VALUE;
            }
        }
    }

    // Not found.
    aResult.Truncate();
    return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP_(PRBool)
nsXULElement::HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    if (!aName)
        return PR_FALSE;

    if (mSlots) {
        nsXULAttributes *attrs = mSlots->GetAttributes();
        if (attrs) {
            PRInt32 count = attrs->Count();
            for (PRInt32 i = 0; i < count; i++) {
                nsXULAttribute* attr = NS_STATIC_CAST(nsXULAttribute*,
                                                      attrs->ElementAt(i));

                nsINodeInfo *ni = attr->GetNodeInfo();
                if (ni->Equals(aName, aNameSpaceID))
                    return PR_TRUE;
            }
        }
    }

    if (mPrototype) {
        PRInt32 count = mPrototype->mNumAttributes;
        for (PRInt32 i = 0; i < count; i++) {
            nsXULPrototypeAttribute* attr = &(mPrototype->mAttributes[i]);

            nsINodeInfo *ni = attr->mNodeInfo;
            if (ni->Equals(aName, aNameSpaceID))
                return PR_TRUE;
        }
    }

    return PR_FALSE;
}

NS_IMETHODIMP
nsXULElement::UnsetAttr(PRInt32 aNameSpaceID,
                        nsIAtom* aName, PRBool aNotify)
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    if (nsnull == aName)
        return NS_ERROR_NULL_POINTER;

    // If we don't have any attributes, this is really easy.
    if (!Attributes() && !mPrototype)
        return NS_OK;

    PRInt32 index;
    nsXULAttribute *attr =
        FindLocalAttribute(aNameSpaceID, aName, &index);

    if (mPrototype) {
        // Because It's Hard to maintain a magic ``unset'' value in
        // the local attributes, we'll fault all the attributes,
        // unhook ourselves from the prototype, and then remove the
        // local copy of the attribute that we want to unset. In
        // otherwords, we'll become ``heavyweight''.
        //
        // We can avoid this if:
        //
        // 1. The attribute isn't set _anywhere_; i.e., somebody is
        //    trying to unset an attribute that was never set on the
        //    element.
        //
        // 2. The attribute was added locally; i.e., is not present
        //    on the prototype.
        nsXULPrototypeAttribute *protoattr =
            FindPrototypeAttribute(aNameSpaceID, aName);

        if (protoattr) {
            // We've got an attribute on the prototype, so we need to
            // fully fault and remove the local copy.
            nsresult rv = MakeHeavyweight();
            if (NS_FAILED(rv)) return rv;

#ifdef DEBUG_ATTRIBUTE_STATS
            gFaults.UnsetAttr++; gFaults.Total++;
            fprintf(stderr, "XUL: Faulting for UnsetAttr: %d/%d\n",
                    gFaults.UnsetAttr, gFaults.Total);
#endif

            // Now re-find the local copy so we can properly unset it.
            attr = FindLocalAttribute(aNameSpaceID, aName, &index);
            NS_ASSERTION(attr, "an attribute supposed to be here!");
        }
    }

    // If we get here and there is no local attribute, then we can
    // bail. The attribute isn't present on the prototype, nor is it
    // present locally.
    if (!attr)
        return NS_OK;

    // Deal with modification of magical attributes that side-effect
    // other things.
    //
    // XXXwaterson if aNotify == PR_TRUE, do we want to call
    // nsIDocument::BeginUpdate() now?
    if (aNameSpaceID == kNameSpaceID_None) {
      if (mDocument) {
        if (aName == nsXULAtoms::clazz) {
            // If CLASS is being unset, delete our class list.
            Attributes()->UpdateClassList(nsAutoString());
        } else if (aName == nsXULAtoms::style) {
            nsCOMPtr <nsIURI> docURL;
            mDocument->GetBaseURL(*getter_AddRefs(docURL));
            Attributes()->UpdateStyleRule(docURL, nsAutoString());
            // XXX Some kind of special document update might need to happen here.
        }
      }
    }

    nsCOMPtr<nsIAtom> tag;
    GetTag(*getter_AddRefs(tag));

    if (tag == nsXULAtoms::window &&
        aName == nsXULAtoms::hidechrome)
      HideWindowChrome(PR_FALSE);

    // XXX Know how to remove POPUP event listeners when an attribute is unset?

    nsAutoString oldValue;
    attr->GetValue(oldValue);

    // If the accesskey attribute is removed, unregister it here
    // Also see nsAreaFrame, nsBoxFrame and nsTextBoxFrame's AttributeChanged
    if (aNameSpaceID == kNameSpaceID_None &&
        (aName == nsXULAtoms::accesskey || aName == nsXULAtoms::control))
        UnregisterAccessKey(oldValue);

    // Fire mutation listeners
    if (HasMutationListeners(NS_STATIC_CAST(nsIStyledContent*, this),
                             NS_EVENT_BITS_MUTATION_ATTRMODIFIED)) {
        // XXXwaterson ugh, why do we QI() on ourself?
        nsCOMPtr<nsIDOMEventTarget> node(do_QueryInterface(NS_STATIC_CAST(nsIStyledContent*, this)));
        nsMutationEvent mutation;
        mutation.eventStructType = NS_MUTATION_EVENT;
        mutation.message = NS_MUTATION_ATTRMODIFIED;
        mutation.mTarget = node;

        nsAutoString attrName2;
        aName->ToString(attrName2);
        nsCOMPtr<nsIDOMAttr> attrNode;
        GetAttributeNode(attrName2, getter_AddRefs(attrNode));
        mutation.mRelatedNode = attrNode;

        mutation.mAttrName = aName;
        if (!oldValue.IsEmpty())
            mutation.mPrevAttrValue = do_GetAtom(oldValue);
        mutation.mAttrChange = nsIDOMMutationEvent::REMOVAL;
        nsEventStatus status = nsEventStatus_eIgnore;
        HandleDOMEvent(nsnull, &mutation, nsnull, NS_EVENT_FLAG_INIT, &status);
    }

    // Remove the attriubte from the element.
    Attributes()->RemoveElementAt(index);
    NS_RELEASE(attr);

    // Check to see if the OBSERVES attribute is being unset.  If so, we
    // need to remove our broadcaster goop completely.
    if (mDocument && (aNameSpaceID == kNameSpaceID_None) &&
        (aName == nsXULAtoms::observes || aName == nsXULAtoms::command)) {
        nsCOMPtr<nsIDOMXULDocument> xuldoc = do_QueryInterface(mDocument);
        if (xuldoc) {
            // Do a getElementById to retrieve the broadcaster
            nsCOMPtr<nsIDOMElement> broadcaster;
            nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(mDocument);
            domDoc->GetElementById(oldValue, getter_AddRefs(broadcaster));
            if (broadcaster) {
                xuldoc->RemoveBroadcastListenerFor(broadcaster,
                                                   NS_STATIC_CAST(nsIDOMElement*, this),
                                                   NS_LITERAL_STRING("*"));
            }
        }
    }

    // Notify document
    if (mDocument) {
        nsCOMPtr<nsIBindingManager> bindingManager;
        mDocument->GetBindingManager(getter_AddRefs(bindingManager));
        nsCOMPtr<nsIXBLBinding> binding;
        bindingManager->GetBinding(NS_STATIC_CAST(nsIStyledContent*, this), getter_AddRefs(binding));
        if (binding)
            binding->AttributeChanged(aName, aNameSpaceID, PR_TRUE, aNotify);

        if (aNotify) {
            mDocument->AttributeChanged(this, aNameSpaceID, aName,
                                        nsIDOMMutationEvent::REMOVAL,
                                        StyleHintFor(NodeInfo()));

            // XXXwaterson do we need to mDocument->EndUpdate() here?
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetAttrNameAt(PRInt32 aIndex,
                            PRInt32& aNameSpaceID,
                            nsIAtom*& aName,
                            nsIAtom*& aPrefix) const
{
#ifdef DEBUG_ATTRIBUTE_STATS
    int local = Attributes() ? Attributes()->Count() : 0;
    int proto = mPrototype ? mPrototype->mNumAttributes : 0;
    fprintf(stderr, "GANA: %p[%d] of %d/%d:", (void *)this, aIndex, local, proto);
#endif

    PRBool haveLocalAttributes = PR_FALSE;
    if (Attributes()) {
        haveLocalAttributes = PR_TRUE;
        if (aIndex < Attributes()->Count()) {
            nsXULAttribute* attr = NS_REINTERPRET_CAST(nsXULAttribute*, Attributes()->ElementAt(aIndex));
            if (nsnull != attr) {
                attr->GetNodeInfo()->GetNamespaceID(aNameSpaceID);
                attr->GetNodeInfo()->GetNameAtom(aName);
                attr->GetNodeInfo()->GetPrefixAtom(aPrefix);
#ifdef DEBUG_ATTRIBUTE_STATS
                fprintf(stderr, " local!\n");
#endif
                return NS_OK;
            }
        }
    }

    if (mPrototype) {
        if (haveLocalAttributes)
            aIndex -= Attributes()->Count();

        if (aIndex >= 0 && aIndex < mPrototype->mNumAttributes) {
            PRBool skip;
            nsXULPrototypeAttribute* attr;
            do {
                attr = &(mPrototype->mAttributes[aIndex]);
                skip = haveLocalAttributes && FindLocalAttribute(attr->mNodeInfo);
#ifdef DEBUG_ATTRIBUTE_STATS
                if (skip)
                    fprintf(stderr, " [skip %d/%d]", aIndex, aIndex + local);
#endif
            } while (skip && aIndex++ < mPrototype->mNumAttributes);

            if (aIndex <= mPrototype->mNumAttributes) {
#ifdef DEBUG_ATTRIBUTE_STATS
                fprintf(stderr, " proto[%d]!\n", aIndex);
#endif
                attr->mNodeInfo->GetNamespaceID(aNameSpaceID);
                attr->mNodeInfo->GetNameAtom(aName);
                attr->mNodeInfo->GetPrefixAtom(aPrefix);
                return NS_OK;
            }
            // else, we are out of attrs to return, fall-through
        }
    }

#ifdef DEBUG_ATTRIBUTE_STATS
    fprintf(stderr, " not found\n");
#endif

    aNameSpaceID = kNameSpaceID_None;
    aName = nsnull;
    aPrefix = nsnull;
    return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
nsXULElement::GetAttrCount(PRInt32& aResult) const
{
    aResult = 0;
    PRBool haveLocalAttributes;

    if (Attributes()) {
        aResult = Attributes()->Count();
        haveLocalAttributes = aResult > 0;
    } else {
        haveLocalAttributes = PR_FALSE;
    }

#ifdef DEBUG_ATTRIBUTE_STATS
    int dups = 0;
#endif

    if (mPrototype) {
        for (int i = 0; i < mPrototype->mNumAttributes; i++) {
            if (!haveLocalAttributes ||
                !FindLocalAttribute(mPrototype->mAttributes[i].mNodeInfo)) {
                aResult++;
            } else {
#ifdef DEBUG_ATTRIBUTE_STATS
                if (haveLocalAttributes)
                    dups++;
#endif
            }
        }
    }

#ifdef DEBUG_ATTRIBUTE_STATS
    {
        int local = Attributes() ? Attributes()->Count() : 0;
        int proto = mPrototype ? mPrototype->mNumAttributes : 0;
        nsAutoString tagstr;
        NodeInfo()->GetName(tagstr);
        char *tagcstr = ToNewCString(tagstr);

        fprintf(stderr, "GAC: %p has %d+%d-%d=%d <%s%s>\n", (void *)this,
                local, proto, dups, aResult, mPrototype ? "" : "*", tagcstr);
        nsMemory::Free(tagcstr);
    }
#endif

    return NS_OK;
}


#ifdef DEBUG
static void
rdf_Indent(FILE* out, PRInt32 aIndent)
{
    for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);
}

NS_IMETHODIMP
nsXULElement::List(FILE* out, PRInt32 aIndent) const
{
    NS_PRECONDITION(mDocument != nsnull, "bad content");

    PRInt32 i;

    rdf_Indent(out, aIndent);
    fputs("<XUL", out);
    if (mSlots) fputs("*", out);
    fputs(" ", out);

    nsAutoString as;
    NodeInfo()->GetQualifiedName(as);
    fputs(NS_LossyConvertUCS2toASCII(as).get(), out);

    fprintf(out, "@%p", (void *)this);

    PRInt32 nattrs;
    GetAttrCount(nattrs);

    for (i = 0; i < nattrs; ++i) {
        nsCOMPtr<nsIAtom> attr;
        nsCOMPtr<nsIAtom> prefix;
        PRInt32 nameSpaceID;
        GetAttrNameAt(i, nameSpaceID, *getter_AddRefs(attr), *getter_AddRefs(prefix));

        nsAutoString v;
        GetAttr(nameSpaceID, attr, v);

        fputs(" ", out);

        nsAutoString s;

        if (prefix) {
            prefix->ToString(s);

            fputs(NS_LossyConvertUCS2toASCII(s).get(), out);
            fputs(":", out);
        }

        attr->ToString(s);

        fputs(NS_LossyConvertUCS2toASCII(s).get(), out);
        fputs("=", out);
        fputs(NS_LossyConvertUCS2toASCII(v).get(), out);
    }

    PRInt32 nchildren;
    ChildCount(nchildren);

    if (nchildren) {
        fputs("\n", out);

        for (i = 0; i < nchildren; ++i) {
            nsCOMPtr<nsIContent> child;
            ChildAt(i, *getter_AddRefs(child));

            child->List(out, aIndent + 1);
        }

        rdf_Indent(out, aIndent);
    }
    fputs(">\n", out);

    if (mDocument) {
        nsCOMPtr<nsIBindingManager> bindingManager;
        mDocument->GetBindingManager(getter_AddRefs(bindingManager));
        if (bindingManager) {
            nsCOMPtr<nsIDOMNodeList> anonymousChildren;
            bindingManager->GetAnonymousNodesFor(NS_STATIC_CAST(nsIContent*, NS_CONST_CAST(nsXULElement*, this)),
                                                 getter_AddRefs(anonymousChildren));

            if (anonymousChildren) {
                PRUint32 length;
                anonymousChildren->GetLength(&length);
                if (length) {
                    rdf_Indent(out, aIndent);
                    fputs("anonymous-children<\n", out);

                    for (PRUint32 i2 = 0; i2 < length; ++i2) {
                        nsCOMPtr<nsIDOMNode> node;
                        anonymousChildren->Item(i2, getter_AddRefs(node));
                        nsCOMPtr<nsIContent> child = do_QueryInterface(node);
                        child->List(out, aIndent + 1);
                    }

                    rdf_Indent(out, aIndent);
                    fputs(">\n", out);
                }
            }

            PRBool hasContentList;
            bindingManager->HasContentListFor(NS_STATIC_CAST(nsIContent*, NS_CONST_CAST(nsXULElement*, this)),
                                              &hasContentList);

            if (hasContentList) {
                nsCOMPtr<nsIDOMNodeList> contentList;
                bindingManager->GetContentListFor(NS_STATIC_CAST(nsIContent*, NS_CONST_CAST(nsXULElement*, this)),
                                                  getter_AddRefs(contentList));

                NS_ASSERTION(contentList != nsnull, "oops, binding manager lied");

                PRUint32 length;
                contentList->GetLength(&length);
                if (length) {
                    rdf_Indent(out, aIndent);
                    fputs("content-list<\n", out);

                    for (PRUint32 i2 = 0; i2 < length; ++i2) {
                        nsCOMPtr<nsIDOMNode> node;
                        contentList->Item(i2, getter_AddRefs(node));
                        nsCOMPtr<nsIContent> child = do_QueryInterface(node);
                        child->List(out, aIndent + 1);
                    }

                    rdf_Indent(out, aIndent);
                    fputs(">\n", out);
                }
            }
        }
    }

    return NS_OK;
}
#endif

NS_IMETHODIMP
nsXULElement::HandleDOMEvent(nsIPresContext* aPresContext,
                                       nsEvent* aEvent,
                                       nsIDOMEvent** aDOMEvent,
                                       PRUint32 aFlags,
                                       nsEventStatus* aEventStatus)
{
    nsresult ret = NS_OK;

    PRBool retarget = PR_FALSE;
    PRBool externalDOMEvent = PR_FALSE;
    nsCOMPtr<nsIDOMEventTarget> oldTarget;

    nsIDOMEvent* domEvent = nsnull;
    if (NS_EVENT_FLAG_INIT & aFlags) {
        if (aEvent->message == NS_XUL_COMMAND) {
            // See if we have a command elt.  If so, we execute on the command instead
            // of on our content element.
            nsAutoString command;
            GetAttr(kNameSpaceID_None, nsXULAtoms::command, command);
            if (!command.IsEmpty()) {
                nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(mDocument));
                nsCOMPtr<nsIDOMElement> commandElt;
                domDoc->GetElementById(command, getter_AddRefs(commandElt));
                nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));
                if (commandContent) {
                    return commandContent->HandleDOMEvent(aPresContext, aEvent, nsnull, NS_EVENT_FLAG_INIT, aEventStatus);
                }
                else {
                    NS_WARNING("A XUL element is attached to a command that doesn't exist!\n");
                    return NS_ERROR_FAILURE;
                }
            }
        }
        if (aDOMEvent) {
            if (*aDOMEvent)
              externalDOMEvent = PR_TRUE;
        }
        else
          aDOMEvent = &domEvent;

        aEvent->flags |= aFlags;
        aFlags &= ~(NS_EVENT_FLAG_CANT_BUBBLE | NS_EVENT_FLAG_CANT_CANCEL);
        aFlags |= NS_EVENT_FLAG_BUBBLE | NS_EVENT_FLAG_CAPTURE;

        if (!externalDOMEvent) {
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
            if (aEvent->message == NS_XUL_COMMAND || aEvent->message == NS_XUL_POPUP_SHOWING ||
                aEvent->message == NS_XUL_POPUP_SHOWN || aEvent->message == NS_XUL_POPUP_HIDING ||
                aEvent->message == NS_XUL_POPUP_HIDDEN || aEvent->message == NS_FORM_SELECTED ||
                aEvent->message == NS_XUL_BROADCAST || aEvent->message == NS_XUL_COMMAND_UPDATE ||
                aEvent->message == NS_XUL_CLICK || aEvent->message == NS_DRAGDROP_GESTURE ||
                tagName == NS_LITERAL_STRING("menu") || tagName == NS_LITERAL_STRING("menuitem") ||
                tagName == NS_LITERAL_STRING("menulist") || tagName == NS_LITERAL_STRING("menubar") ||
                tagName == NS_LITERAL_STRING("menupopup") || tagName == NS_LITERAL_STRING("key") ||
                tagName == NS_LITERAL_STRING("keyset")) {

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
                  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(NS_STATIC_CAST(nsIContent *, this)));
                  privateEvent->SetTarget(target);
                }
                else
                  return NS_ERROR_FAILURE;

                // if we are a XUL click, we have the private event set.
                // now switch to a left mouse click for the duration of the event
                if (aEvent->message == NS_XUL_CLICK)
                  aEvent->message = NS_MOUSE_LEFT_CLICK;
            }
        }
    }
    else if (aEvent->message == NS_IMAGE_LOAD)
      return NS_OK; // Don't let these events bubble or be captured.  Just allow them
                    // on the target image.

    // Find out if we're anonymous.
    nsCOMPtr<nsIContent> bindingParent;
    if (*aDOMEvent) {
        (*aDOMEvent)->GetTarget(getter_AddRefs(oldTarget));
        nsCOMPtr<nsIContent> content(do_QueryInterface(oldTarget));
        if (content)
            content->GetBindingParent(getter_AddRefs(bindingParent));
    }
    else
        GetBindingParent(getter_AddRefs(bindingParent));

    if (bindingParent) {
        // We're anonymous.  We may potentially need to retarget
        // our event if our parent is in a different scope.
        if (mParent) {
            nsCOMPtr<nsIContent> parentScope;
            mParent->GetBindingParent(getter_AddRefs(parentScope));
            if (parentScope != bindingParent)
                retarget = PR_TRUE;
        }
    }

    // determine the parent:
    nsCOMPtr<nsIContent> parent;
    if (mDocument) {
        nsCOMPtr<nsIBindingManager> bindingManager;
        mDocument->GetBindingManager(getter_AddRefs(bindingManager));
        if (bindingManager) {
            // we have a binding manager -- do we have an anonymous parent?
            bindingManager->GetInsertionParent(this, getter_AddRefs(parent));
        }
    }
    if (parent) {
        retarget = PR_FALSE;
    }
    else {
        // if we didn't find an anonymous parent, use the explicit one,
        // whether it's null or not...
        parent = mParent;
    }

    if (retarget || (parent != mParent)) {
      if (!*aDOMEvent) {
        // We haven't made a DOMEvent yet.  Force making one now.
        nsCOMPtr<nsIEventListenerManager> listenerManager;
        if (NS_FAILED(ret = GetListenerManager(getter_AddRefs(listenerManager)))) {
          return ret;
        }
        nsAutoString empty;
        if (NS_FAILED(ret = listenerManager->CreateEvent(aPresContext, aEvent, empty, aDOMEvent)))
          return ret;
      }

      if (!*aDOMEvent) {
        return NS_ERROR_FAILURE;
      }
      nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(*aDOMEvent);
      if (!privateEvent) {
        return NS_ERROR_FAILURE;
      }

      (*aDOMEvent)->GetTarget(getter_AddRefs(oldTarget));

      PRBool hasOriginal;
      privateEvent->HasOriginalTarget(&hasOriginal);

      if (!hasOriginal)
        privateEvent->SetOriginalTarget(oldTarget);

      if (retarget) {
          nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mParent);
          privateEvent->SetTarget(target);
      }
    }

    //Capturing stage evaluation
    if (NS_EVENT_FLAG_CAPTURE & aFlags) {
      //Initiate capturing phase.  Special case first call to document
      if (parent) {
        parent->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, aFlags & NS_EVENT_CAPTURE_MASK, aEventStatus);
      }
      else if (mDocument != nsnull) {
          ret = mDocument->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                          aFlags & NS_EVENT_CAPTURE_MASK, aEventStatus);
      }
    }


    if (retarget) {
      // The event originated beneath us, and we performed a retargeting.
      // We need to restore the original target of the event.
      nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(*aDOMEvent);
      if (privateEvent)
        privateEvent->SetTarget(oldTarget);
    }

    //Local handling stage
    if (mListenerManager && !(aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
        aEvent->flags |= aFlags;
        nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(NS_STATIC_CAST(nsIContent *, this)));
        mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, target, aFlags, aEventStatus);
        aEvent->flags &= ~aFlags;
    }

    if (retarget) {
      // The event originated beneath us, and we need to perform a retargeting.
      nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(*aDOMEvent);
      if (privateEvent) {
        nsCOMPtr<nsIDOMEventTarget> parentTarget(do_QueryInterface(mParent));
        privateEvent->SetTarget(parentTarget);
      }
    }

    //Bubbling stage
    if (NS_EVENT_FLAG_BUBBLE & aFlags) {
        if (parent != nsnull) {
            // We have a parent. Let them field the event.
            ret = parent->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                          aFlags & NS_EVENT_BUBBLE_MASK, aEventStatus);
      }
        else if (mDocument != nsnull) {
        // We must be the document root. The event should bubble to the
        // document.
        ret = mDocument->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                        aFlags & NS_EVENT_BUBBLE_MASK, aEventStatus);
        }
    }

    if (retarget) {
      // The event originated beneath us, and we performed a retargeting.
      // We need to restore the original target of the event.
      nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(*aDOMEvent);
      if (privateEvent)
        privateEvent->SetTarget(oldTarget);
    }

    if (NS_EVENT_FLAG_INIT & aFlags) {
      // We're leaving the DOM event loop so if we created a DOM event,
      // release here.  If externalDOMEvent is set the event was passed in
      // and we don't own it
      if (*aDOMEvent && !externalDOMEvent) {
        nsrefcnt rc;
        NS_RELEASE2(*aDOMEvent, rc);
        if (0 != rc) {
          // Okay, so someone in the DOM loop (a listener, JS object)
          // still has a ref to the DOM Event but the internal data
          // hasn't been malloc'd.  Force a copy of the data here so the
          // DOM Event is still valid.
          nsCOMPtr<nsIPrivateDOMEvent> privateEvent =
            do_QueryInterface(*aDOMEvent);
          if (privateEvent) {
            privateEvent->DuplicatePrivateData();
          }
        }
        aDOMEvent = nsnull;
      }
    }

    return ret;
}


NS_IMETHODIMP
nsXULElement::GetContentID(PRUint32* aID)
{
    *aID = 0;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetContentID(PRUint32 aID)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::RangeAdd(nsIDOMRange* aRange)
{
    // rdf content does not yet support DOM ranges
    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::RangeRemove(nsIDOMRange* aRange)
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


// XXX This _should_ be an implementation method, _not_ publicly exposed :-(
NS_IMETHODIMP
nsXULElement::GetResource(nsIRDFResource** aResource)
{
    nsresult rv;

    nsAutoString id;
    rv = GetAttr(kNameSpaceID_None, nsXULAtoms::ref, id);
    if (NS_FAILED(rv)) return rv;

    if (rv != NS_CONTENT_ATTR_HAS_VALUE) {
        rv = GetAttr(kNameSpaceID_None, nsXULAtoms::id, id);
        if (NS_FAILED(rv)) return rv;
    }

    if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
        rv = gRDFService->GetUnicodeResource(id, aResource);
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
    nsCOMPtr<nsIXULTemplateBuilder> builder;
    GetBuilder(getter_AddRefs(builder));

    if (builder)
        builder->GetDatabase(aDatabase);
    else
        *aDatabase = nsnull;

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetBuilder(nsIXULTemplateBuilder** aBuilder)
{
    *aBuilder = nsnull;

    if (mDocument) {
        nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mDocument);
        if (xuldoc)
            xuldoc->GetTemplateBuilderFor(NS_STATIC_CAST(nsIStyledContent*, this), aBuilder);
    }

    return NS_OK;
}


//----------------------------------------------------------------------
// Implementation methods

nsresult
nsXULElement::EnsureContentsGenerated(void) const
{
    if (mSlots && (mSlots->GetLazyState() & nsIXULContent::eChildrenMustBeRebuilt)) {
        // Ensure that the element is actually _in_ the document tree;
        // otherwise, somebody is trying to generate children for a node
        // that's not currently in the content model.
        NS_PRECONDITION(mDocument != nsnull, "element not in tree");
        if (!mDocument)
            return NS_ERROR_NOT_INITIALIZED;

        // XXX hack because we can't use "mutable"
        nsXULElement* unconstThis = NS_CONST_CAST(nsXULElement*, this);

        // Clear this value *first*, so we can re-enter the nsIContent
        // getters if needed.
        unconstThis->ClearLazyState(eChildrenMustBeRebuilt);

        // Walk up our ancestor chain, looking for an element with a
        // XUL content model builder attached to it.
        nsCOMPtr<nsIContent> element
            = do_QueryInterface(NS_STATIC_CAST(nsIStyledContent*, unconstThis));

        do {
            nsCOMPtr<nsIDOMXULElement> xulele = do_QueryInterface(element);
            if (xulele) {
                nsCOMPtr<nsIXULTemplateBuilder> builder;
                xulele->GetBuilder(getter_AddRefs(builder));
                if (builder) {
                    if (HasAttr(kNameSpaceID_None, nsXULAtoms::xulcontentsgenerated)) {
                        unconstThis->ClearLazyState(nsIXULContent::eChildrenMustBeRebuilt);
                        return NS_OK;
                    }

                    return builder->CreateContents(NS_STATIC_CAST(nsIStyledContent*, unconstThis));
                }
            }

            nsCOMPtr<nsIContent> parent;
            element->GetParent(*getter_AddRefs(parent));

            element = parent;
        } while (element);

        NS_ERROR("lazy state set with no XUL content builder in ancestor chain");
        return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
}

nsresult
nsXULElement::GetElementsByAttribute(nsIDOMNode* aNode,
                                       const nsAString& aAttribute,
                                       const nsAString& aValue,
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

        if ((attrValue.Equals(aValue)) || (!attrValue.IsEmpty() && aValue.Equals(NS_LITERAL_STRING("*")))) {
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
    if (mSlots) {
        nsXULAttributes *attrs = mSlots->GetAttributes();
        if (attrs) {
            // Take advantage of the fact that the 'id' attribute will
            // already be atomized.
            PRInt32 count = attrs->Count();
            for (PRInt32 i = 0; i < count; ++i) {
                nsXULAttribute* attr =
                    NS_STATIC_CAST(nsXULAttribute*, attrs->ElementAt(i));

                if (attr->GetNodeInfo()->Equals(nsXULAtoms::id, kNameSpaceID_None)) {
                    attr->GetValueAsAtom(&aResult);
                    return NS_OK;
                }
            }
        }
    }

    if (mPrototype) {
        PRInt32 count = mPrototype->mNumAttributes;
        for (PRInt32 i = 0; i < count; i++) {
            nsXULPrototypeAttribute* attr = &(mPrototype->mAttributes[i]);
            if (attr->mNodeInfo->Equals(nsXULAtoms::id, kNameSpaceID_None)) {
                attr->mValue.GetValueAsAtom(&aResult);
                return NS_OK;
            }
        }
    }

    aResult = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetClasses(nsVoidArray& aArray) const
{
    // XXXwaterson if we decide to lazily fault the class list in
    // EnsureAttributes(), then this will need to be fixed.
    if (Attributes())
        return Attributes()->GetClasses(aArray);

    if (mPrototype)
        return nsClassList::GetClasses(mPrototype->mClassList, aArray);

    aArray.Clear();
    return NS_ERROR_NULL_POINTER; // XXXwaterson kooky error code to return, but...
}

NS_IMETHODIMP_(PRBool)
nsXULElement::HasClass(nsIAtom* aClass, PRBool /*aCaseSensitive*/) const
{
    // XXXwaterson if we decide to lazily fault the class list in
    // EnsureAttributes(), then this will need to be fixed.
    if (Attributes())
        return Attributes()->HasClass(aClass);

    if (mPrototype)
        return nsClassList::HasClass(mPrototype->mClassList, aClass);

    return PR_FALSE;
}

NS_IMETHODIMP
nsXULElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetInlineStyleRule(nsIStyleRule** aStyleRule)
{
    // Fetch the cached style rule from the attributes.
    nsresult result = NS_OK;
    if (Attributes()) {
        result = Attributes()->GetInlineStyleRule(*aStyleRule);
    }
    else if (mPrototype) {
        *aStyleRule = mPrototype->mInlineStyleRule;
        NS_IF_ADDREF(*aStyleRule);
    } else {
        *aStyleRule = nsnull;
    }

    return result;
}

NS_IMETHODIMP
nsXULElement::GetMappedAttributeImpact(const nsIAtom* aAttribute, PRInt32 aModType,
                                       nsChangeHint& aHint) const
{
    aHint = NS_STYLE_HINT_CONTENT;  // by default, never map attributes to style

    if (aAttribute == nsXULAtoms::value &&
        (aModType == nsIDOMMutationEvent::REMOVAL || aModType == nsIDOMMutationEvent::ADDITION)) {
      nsCOMPtr<nsIAtom> tag;
      GetTag(*getter_AddRefs(tag));
      if (tag == nsXULAtoms::label || tag == nsXULAtoms::description)
        // Label and description dynamically morph between a normal block and a cropping single-line
        // XUL text frame.  If the value attribute is being added or removed, then we need to return
        // a hint of frame change.  (See bugzilla bug 95475 for details.)
        aHint = NS_STYLE_HINT_FRAMECHANGE;
      else
        aHint = NS_STYLE_HINT_ATTRCHANGE;
    }
    else if (aAttribute == nsXULAtoms::value || aAttribute == nsXULAtoms::flex ||
        aAttribute == nsXULAtoms::label || aAttribute == nsXULAtoms::mousethrough) {
      // VERY IMPORTANT! This has a huge positive performance impact!
      aHint = NS_STYLE_HINT_ATTRCHANGE;
    }
    else if (NodeInfo()->Equals(nsXULAtoms::window) ||
             NodeInfo()->Equals(nsXULAtoms::page) ||
             NodeInfo()->Equals(nsXULAtoms::dialog) ||
             NodeInfo()->Equals(nsXULAtoms::wizard)) {
        // Ignore 'width', 'height', 'screenX', 'screenY' and 'sizemode' on a <window>
        if (nsXULAtoms::width == aAttribute || nsXULAtoms::height == aAttribute ||
            nsXULAtoms::screenX == aAttribute || nsXULAtoms::screenY == aAttribute ||
            nsXULAtoms::sizemode == aAttribute)
           aHint = NS_STYLE_HINT_NONE;
    } else {
        // if left or top changes we reflow. This will happen in xul containers that
        // manage positioned children such as a bulletinboard.
        if (nsXULAtoms::left == aAttribute || nsXULAtoms::top == aAttribute)
            aHint = NS_STYLE_HINT_REFLOW;
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
  *aResult = nsnull;

  if (!mDocument)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(mDocument));
  return nsDoc->GetBoxObjectFor(NS_STATIC_CAST(nsIDOMElement*, this), aResult);
}

// Methods for setting/getting attributes from nsIDOMXULElement
nsresult
nsXULElement::GetId(nsAString& aId)
{
  GetAttribute(NS_LITERAL_STRING("id"), aId);
  return NS_OK;
}

nsresult
nsXULElement::SetId(const nsAString& aId)
{
  SetAttribute(NS_LITERAL_STRING("id"), aId);
  return NS_OK;
}

nsresult
nsXULElement::GetClassName(nsAString& aClassName)
{
  GetAttribute(NS_LITERAL_STRING("class"), aClassName);
  return NS_OK;
}

nsresult
nsXULElement::SetClassName(const nsAString& aClassName)
{
  SetAttribute(NS_LITERAL_STRING("class"), aClassName);
  return NS_OK;
}

nsresult
nsXULElement::GetAlign(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("align"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetAlign(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("align"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetDir(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("dir"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetDir(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("dir"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetFlex(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("flex"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetFlex(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("flex"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetFlexGroup(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("flexgroup"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetFlexGroup(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("flexgroup"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetOrdinal(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("ordinal"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetOrdinal(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("ordinal"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetOrient(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("orient"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetOrient(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("orient"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetPack(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("pack"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetPack(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("pack"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetHidden(PRBool* aResult)
{
  *aResult = PR_FALSE;
  nsAutoString val;
  GetAttribute(NS_LITERAL_STRING("hidden"), val);
  if (val.EqualsIgnoreCase("true"))
    *aResult = PR_TRUE;
  return NS_OK;
}

nsresult
nsXULElement::SetHidden(PRBool aAttr)
{
  if (aAttr)
    SetAttribute(NS_LITERAL_STRING("hidden"), NS_LITERAL_STRING("true"));
  else
    RemoveAttribute(NS_LITERAL_STRING("hidden"));
  return NS_OK;
}

nsresult
nsXULElement::GetCollapsed(PRBool* aResult)
{
  *aResult = PR_FALSE;
  nsAutoString val;
  GetAttribute(NS_LITERAL_STRING("collapsed"), val);
  if (val.EqualsIgnoreCase("true"))
    *aResult = PR_TRUE;
  return NS_OK;
}

nsresult
nsXULElement::SetCollapsed(PRBool aAttr)
{
 if (aAttr)
    SetAttribute(NS_LITERAL_STRING("collapsed"), NS_LITERAL_STRING("true"));
  else
    RemoveAttribute(NS_LITERAL_STRING("collapsed"));
  return NS_OK;
}

nsresult
nsXULElement::GetAllowEvents(PRBool* aResult)
{
  *aResult = PR_FALSE;
  nsAutoString val;
  GetAttribute(NS_LITERAL_STRING("allowevents"), val);
  if (val.EqualsIgnoreCase("true"))
    *aResult = PR_TRUE;
  return NS_OK;
}

nsresult
nsXULElement::SetAllowEvents(PRBool aAttr)
{
 if (aAttr)
    SetAttribute(NS_LITERAL_STRING("allowevents"), NS_LITERAL_STRING("true"));
  else
    RemoveAttribute(NS_LITERAL_STRING("allowevents"));
  return NS_OK;
}

nsresult
nsXULElement::GetObserves(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("observes"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetObserves(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("observes"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetMenu(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("menu"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetMenu(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("menu"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetContextMenu(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("contextmenu"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetContextMenu(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("contextmenu"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetTooltip(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("tooltip"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetTooltip(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("tooltip"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetWidth(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("width"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetWidth(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("width"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetHeight(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("height"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetHeight(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("height"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetMinWidth(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("minwidth"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetMinWidth(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("minwidth"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetMinHeight(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("minheight"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetMinHeight(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("minheight"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetMaxWidth(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("maxwidth"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetMaxWidth(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("maxwidth"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetMaxHeight(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("maxheight"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetMaxHeight(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("maxheight"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetPersist(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("persist"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetPersist(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("persist"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetLeft(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("left"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetLeft(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("left"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetTop(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("top"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetTop(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("top"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetDatasources(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("datasources"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetDatasources(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("datasources"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetRef(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("ref"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetRef(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("ref"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetTooltipText(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("tooltiptext"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetTooltipText(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("tooltiptext"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetStatusText(nsAString& aAttr)
{
  GetAttribute(NS_LITERAL_STRING("statustext"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::SetStatusText(const nsAString& aAttr)
{
  SetAttribute(NS_LITERAL_STRING("statustext"), aAttr);
  return NS_OK;
}

nsresult
nsXULElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULElement::GetParentTree(nsIDOMXULMultiSelectControlElement** aTreeElement)
{
  nsCOMPtr<nsIContent> current;
  GetParent(*getter_AddRefs(current));
  while (current) {
    nsCOMPtr<nsIAtom> tag;
    current->GetTag(*getter_AddRefs(tag));
    if (tag == nsXULAtoms::listbox) {
      nsCOMPtr<nsIDOMXULMultiSelectControlElement> element = do_QueryInterface(current);
      *aTreeElement = element;
      NS_IF_ADDREF(*aTreeElement);
      return NS_OK;
    }

    nsCOMPtr<nsIContent> parent;
    current->GetParent(*getter_AddRefs(parent));
    current = parent;
  }
  return NS_OK;
}

PRBool
nsXULElement::IsAncestor(nsIDOMNode* aParentNode, nsIDOMNode* aChildNode)
{
  nsCOMPtr<nsIDOMNode> parent = dont_QueryInterface(aChildNode);
  while (parent && (parent != aParentNode)) {
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
  // What kind of crazy tries to focus an element without a doc?
  if (!mDocument)
      return NS_OK;

  // Obtain a presentation context and then call SetFocus.
  PRInt32 count = mDocument->GetNumberOfShells();
  if (count == 0)
    return NS_OK;

  nsCOMPtr<nsIPresShell> shell;
  mDocument->GetShellAt(0, getter_AddRefs(shell));

  // Retrieve the context
  nsCOMPtr<nsIPresContext> aPresContext;
  shell->GetPresContext(getter_AddRefs(aPresContext));

  // Set focus
  return SetFocus(aPresContext);
}

NS_IMETHODIMP
nsXULElement::Blur()
{
  // What kind of crazy tries to blur an element without a doc?
  if (!mDocument)
      return NS_OK;

  // Obtain a presentation context and then call SetFocus.
  PRInt32 count = mDocument->GetNumberOfShells();
  if (count == 0)
    return NS_OK;

  nsCOMPtr<nsIPresShell> shell;
  mDocument->GetShellAt(0, getter_AddRefs(shell));

  // Retrieve the context
  nsCOMPtr<nsIPresContext> aPresContext;
  shell->GetPresContext(getter_AddRefs(aPresContext));

  // Set focus
  return RemoveFocus(aPresContext);
}

NS_IMETHODIMP
nsXULElement::Click()
{
  nsAutoString disabled;
  GetAttribute(NS_LITERAL_STRING("disabled"), disabled);
  if (disabled == NS_LITERAL_STRING("true"))
    return NS_OK;

  nsCOMPtr<nsIDocument> doc; // Strong
  GetDocument(*getter_AddRefs(doc));
  if (doc) {
    PRInt32 numShells = doc->GetNumberOfShells();
    nsCOMPtr<nsIPresShell> shell; // Strong
    nsCOMPtr<nsIPresContext> context;

    for (PRInt32 i=0; i<numShells; i++) {
      doc->GetShellAt(i, getter_AddRefs(shell));
      shell->GetPresContext(getter_AddRefs(context));

      nsMouseEvent eventDown;
      eventDown.eventStructType = NS_MOUSE_EVENT;
      eventDown.message = NS_MOUSE_LEFT_BUTTON_DOWN;
      eventDown.isShift = PR_FALSE;
      eventDown.isControl = PR_FALSE;
      eventDown.isAlt = PR_FALSE;
      eventDown.isMeta = PR_FALSE;
      eventDown.clickCount = 0;
      eventDown.widget = nsnull;

      nsMouseEvent eventUp(eventDown),    // use copy constructor for bit-wise copy
                   eventClick(eventDown);
      eventUp.message = NS_MOUSE_LEFT_BUTTON_UP;
      eventClick.message = NS_XUL_CLICK;

      // send mouse down
      nsEventStatus status = nsEventStatus_eIgnore;
      HandleDOMEvent(context, &eventDown,  nsnull, NS_EVENT_FLAG_INIT, &status);

      // send mouse up
      status = nsEventStatus_eIgnore;  // reset status
      HandleDOMEvent(context, &eventUp,    nsnull, NS_EVENT_FLAG_INIT, &status);

      // send mouse click
      status = nsEventStatus_eIgnore;  // reset status
      HandleDOMEvent(context, &eventClick, nsnull, NS_EVENT_FLAG_INIT, &status);
    }
  }

  // oncommand is fired when an element is clicked...
  return DoCommand();
}

NS_IMETHODIMP
nsXULElement::DoCommand()
{
  nsCOMPtr<nsIDocument> doc;
  GetDocument(*getter_AddRefs(doc));
  if (doc) {
    PRInt32 numShells = doc->GetNumberOfShells();
    nsCOMPtr<nsIPresShell> shell;
    nsCOMPtr<nsIPresContext> context;

    for (PRInt32 i=0; i<numShells; i++) {
      doc->GetShellAt(i, getter_AddRefs(shell));
      shell->GetPresContext(getter_AddRefs(context));

      nsEventStatus status = nsEventStatus_eIgnore;
      nsMouseEvent event;
      event.eventStructType = NS_EVENT;
      event.message = NS_XUL_COMMAND;
      HandleDOMEvent(context, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
    }
  }

  return NS_OK;
}

// nsIFocusableContent interface and helpers

NS_IMETHODIMP
nsXULElement::SetFocus(nsIPresContext* aPresContext)
{
  nsAutoString disabled;
  GetAttribute(NS_LITERAL_STRING("disabled"), disabled);
  if (disabled == NS_LITERAL_STRING("true"))
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

NS_IMETHODIMP
nsXULElement::GetBindingParent(nsIContent** aContent)
{
  *aContent = mBindingParent;
  NS_IF_ADDREF(*aContent);
  return NS_OK;
}

NS_IMETHODIMP
nsXULElement::SetBindingParent(nsIContent* aParent)
{
  mBindingParent = aParent; // [Weak] no addref
  if (mBindingParent) {
    PRInt32 count;
    ChildCount(count);
    for (PRInt32 i = 0; i < count; i++) {
      nsCOMPtr<nsIContent> child;
      ChildAt(i, *getter_AddRefs(child));
      child->SetBindingParent(aParent);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsXULElement::IsContentOfType(PRUint32 aFlags)
{
  return !(aFlags & ~(eELEMENT | eXUL));
}

nsresult
nsXULElement::AddPopupListener(nsIAtom* aName)
{
    // Add a popup listener to the element
    nsresult rv;

    nsCOMPtr<nsIXULPopupListener> popupListener =
        do_CreateInstance(kXULPopupListenerCID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to create an instance of the popup listener object.");
    if (NS_FAILED(rv)) return rv;

    XULPopupType popupType;
    if (aName == nsXULAtoms::context || aName == nsXULAtoms::contextmenu) {
        popupType = eXULPopupType_context;
    }
    else {
        popupType = eXULPopupType_popup;
    }

    // Add a weak reference to the node.
    popupListener->Init(this, popupType);

    // Add the popup as a listener on this element.
    nsCOMPtr<nsIDOMEventListener> eventListener = do_QueryInterface(popupListener);
    nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(NS_STATIC_CAST(nsIContent *, this)));
    NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);
    target->AddEventListener(NS_LITERAL_STRING("mousedown"), eventListener, PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING("contextmenu"), eventListener, PR_FALSE);

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
    if (mSlots)
        return NS_OK;

    mSlots = new Slots(this);
    if (!mSlots)
        return NS_ERROR_OUT_OF_MEMORY;

    // Copy information from the prototype, if there is one.
    if (!mPrototype)
        return NS_OK;

    NS_ASSERTION(mPrototype->mNodeInfo, "prototype has null nodeinfo!");
    // XXX this is broken, we need to get a new nodeinfo from the
    // document's nodeinfo manager!!!
    mSlots->mNodeInfo        = mPrototype->mNodeInfo;

    return NS_OK;
}

nsresult nsXULElement::EnsureAttributes()
{
    nsresult rv = EnsureSlots();
    if (NS_FAILED(rv)) return rv;

    if (mSlots->GetAttributes())
        return NS_OK;

    nsXULAttributes *attrs;
    rv = nsXULAttributes::Create(NS_STATIC_CAST(nsIStyledContent*, this), &attrs);
    if (NS_FAILED(rv)) return rv;

    if (mPrototype) {
        // Copy the class list and the style rule information from the
        // prototype.
        // XXXwaterson N.B. that we might not need to do this until the
        // class or style attribute changes.
        attrs->SetClassList(mPrototype->mClassList);
        attrs->SetInlineStyleRule(mPrototype->mInlineStyleRule);
    }

    mSlots->SetAttributes(attrs);
    return NS_OK;
}

nsXULAttribute *
nsXULElement::FindLocalAttribute(nsINodeInfo *info) const
{
    nsXULAttributes *attrs = Attributes();
    if (attrs) {
        PRInt32 count = attrs->Count();
        for (PRInt32 i = 0; i < count; i++) {
            nsXULAttribute *attr = attrs->ElementAt(i);
            if (attr->GetNodeInfo()->Equals(info))
                return attr;
        }
    }
    return nsnull;
}

nsXULAttribute *
nsXULElement::FindLocalAttribute(PRInt32 aNameSpaceID,
                                 nsIAtom *aName,
                                 PRInt32 *aIndex) const
{
    nsXULAttributes *attrs = Attributes();
    if (!attrs)
        return nsnull;

    PRInt32 count = attrs->Count();
    for (PRInt32 i = 0; i < count; i++) {
        nsXULAttribute *attr = attrs->ElementAt(i);
        if (attr->GetNodeInfo()->Equals(aName, aNameSpaceID)) {
            if (aIndex)
                *aIndex = i;

            return attr;
        }
    }
    return nsnull;
}

nsXULPrototypeAttribute *
nsXULElement::FindPrototypeAttribute(nsINodeInfo *info) const
{
    if (mPrototype) {
        for (PRInt32 i = 0; i < mPrototype->mNumAttributes; i++) {
            nsXULPrototypeAttribute *protoattr = &(mPrototype->mAttributes[i]);
            if (protoattr->mNodeInfo->Equals(info))
                return protoattr;
        }
    }
    return nsnull;
}

nsXULPrototypeAttribute *
nsXULElement::FindPrototypeAttribute(PRInt32 ns, nsIAtom *name) const
{
    if (!mPrototype)
        return nsnull;
    for (PRInt32 i = 0; i < mPrototype->mNumAttributes; i++) {
        nsXULPrototypeAttribute *protoattr = &(mPrototype->mAttributes[i]);
        if (protoattr->mNodeInfo->Equals(name, ns))
            return protoattr;
    }
    return nsnull;
}

nsresult nsXULElement::MakeHeavyweight()
{
    NS_ASSERTION(mPrototype || (mSlots && mSlots->mNodeInfo), "need prototype or nodeinfo");

    if (!mPrototype)
        return NS_OK;           // already heavyweight

    PRBool hadAttributes = mSlots && mSlots->GetAttributes();

    // XXXwaterson EnsureAttributes() will have copy the class list
    // and inline style cruft. If we decide to set that junk lazily,
    // then we'll need to be sure to copy it explicitly, here.
    nsresult rv = EnsureAttributes();
    if (NS_FAILED(rv)) return rv;

    nsXULPrototypeElement* proto = mPrototype;
    mPrototype = nsnull;

    if (proto->mNumAttributes > 0) {
      nsXULAttributes *attrs = mSlots->GetAttributes();
      for (PRInt32 i = 0; i < proto->mNumAttributes; ++i) {
          nsXULPrototypeAttribute* protoattr = &(proto->mAttributes[i]);

          // We might have a local value for this attribute, in which case
          // we don't want to copy the prototype's value.
          // XXXshaver Snapshot the local attrs, so we don't search the ones we
          // XXXshaver just appended from the prototype!
          if (hadAttributes && FindLocalAttribute(protoattr->mNodeInfo))
              continue;

          nsAutoString valueStr;
          protoattr->mValue.GetValue(valueStr);

          nsXULAttribute* attr;
          rv = nsXULAttribute::Create(NS_STATIC_CAST(nsIStyledContent*, this),
                                      protoattr->mNodeInfo,
                                      valueStr,
                                      &attr);

          if (NS_FAILED(rv)) return rv;

          // transfer ownership of the nsXULAttribute object
          attrs->AppendElement(attr);
      }
    }

    proto->Release();
    return NS_OK;
}

nsresult
nsXULElement::HideWindowChrome(PRBool aShouldHide)
{
  PRInt32 shellCount = mDocument->GetNumberOfShells();
  if (shellCount > 0) {
    nsCOMPtr<nsIPresShell> shell;
    mDocument->GetShellAt(0, getter_AddRefs(shell));

    if (shell) {
      nsIContent* content = NS_STATIC_CAST(nsIContent*, this);
      nsIFrame* frame = nsnull;
      shell->GetPrimaryFrameFor(content, &frame);

      nsCOMPtr<nsIPresContext> presContext;
      shell->GetPresContext(getter_AddRefs(presContext));

      if (frame && presContext) {
        nsIView* view = nsnull;
        frame->GetView(presContext, &view);

        if (!view) {
          frame->GetParentWithView(presContext, &frame);
          if (frame)
            frame->GetView(presContext, &view);
        }

        if (view) {
          nsCOMPtr<nsIWidget> widget;
          view->GetWidget(*getter_AddRefs(widget));

          widget->HideWindowChrome(aShouldHide);
        }
      }
    }
  }

  return NS_OK;
}

//----------------------------------------------------------------------
//
// nsXULElement::Slots
//

nsXULElement::Slots::Slots(nsXULElement* aElement)
    : mBits(0)
{
    MOZ_COUNT_CTOR(nsXULElement::Slots);
}


nsXULElement::Slots::~Slots()
{
    MOZ_COUNT_DTOR(nsXULElement::Slots);

    nsXULAttributes *attrs = GetAttributes();
    NS_IF_RELEASE(attrs);
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
nsXULPrototypeElement::Serialize(nsIObjectOutputStream* aStream,
                                 nsIScriptContext* aContext,
                                 nsISupportsArray* aNodeInfos)
{
    nsresult rv;

    // Write basic prototype data
    rv = aStream->Write32(mType);

    // Write Node Info
    PRInt32 index = aNodeInfos->IndexOf(mNodeInfo);
    NS_ASSERTION(index >= 0, "unknown nsINodeInfo index");
    rv |= aStream->Write32(index);

    // Write Attributes
    rv |= aStream->Write32(mNumAttributes);

    nsAutoString attributeValue;
    PRInt32 i;
    for (i = 0; i < mNumAttributes; ++i) {
        index = aNodeInfos->IndexOf(mAttributes[i].mNodeInfo);
        NS_ASSERTION(index >= 0, "unknown nsINodeInfo index");
        rv |= aStream->Write32(index);

        rv |= mAttributes[i].mValue.GetValue(attributeValue);
        rv |= aStream->WriteWStringZ(attributeValue.get());
    }

    // Now write children
    rv |= aStream->Write32(PRUint32(mNumChildren));
    for (i = 0; i < mNumChildren; i++) {
        nsXULPrototypeNode* child = mChildren[i];
        switch (child->mType) {
        case eType_Element:
        case eType_Text:
            rv |= child->Serialize(aStream, aContext, aNodeInfos);
            break;
        case eType_Script:
            rv |= aStream->Write32(child->mType);
            nsXULPrototypeScript* script = NS_STATIC_CAST(nsXULPrototypeScript*, child);

            rv |= aStream->Write8(script->mOutOfLine);
            if (! script->mOutOfLine) {
                rv |= script->Serialize(aStream, aContext, aNodeInfos);
            } else {
                rv |= aStream->WriteCompoundObject(script->mSrcURI,
                                                   NS_GET_IID(nsIURI),
                                                   PR_TRUE);

                if (script->mJSObject) {
                    // This may return NS_OK without muxing script->mSrcURI's
                    // data into the FastLoad file, in the case where that
                    // muxed document is already there (written by a prior
                    // session, or by an earlier FastLoad episode during this
                    // session).
                    rv |= script->SerializeOutOfLine(aStream, aContext);
                }
            }
            break;
        }
    }

    return rv;
}

nsresult
nsXULPrototypeElement::Deserialize(nsIObjectInputStream* aStream,
                                   nsIScriptContext* aContext,
                                   nsIURI* aDocumentURI,
                                   nsISupportsArray* aNodeInfos)
{
    NS_PRECONDITION(aNodeInfos, "missing nodeinfo array");
    nsresult rv;

    // Read Node Info
    PRUint32 number;
    rv = aStream->Read32(&number);
    mNodeInfo = do_QueryElementAt(aNodeInfos, number);
    if (!mNodeInfo)
        return NS_ERROR_UNEXPECTED;

    // Read Attributes
    rv |= aStream->Read32(&number);
    mNumAttributes = PRInt32(number);
	
    PRInt32 i;
    if (mNumAttributes > 0) {
        mAttributes = new nsXULPrototypeAttribute[mNumAttributes];
        if (! mAttributes)
            return NS_ERROR_OUT_OF_MEMORY;

        nsAutoString attributeValue;
        for (i = 0; i < mNumAttributes; ++i) {
            rv |= aStream->Read32(&number);
            mAttributes[i].mNodeInfo = do_QueryElementAt(aNodeInfos, number);
            if (!mAttributes[i].mNodeInfo)
                return NS_ERROR_UNEXPECTED;

            rv |= aStream->ReadString(attributeValue);
            mAttributes[i].mValue.SetValue(attributeValue);
        }

        // Compute the element's class list if the element has a 'class' attribute.
        nsAutoString value;
        if (NS_CONTENT_ATTR_HAS_VALUE ==
                GetAttr(kNameSpaceID_None, nsXULAtoms::clazz, value))
            rv |= nsClassList::ParseClasses(&mClassList, value);

        // Parse the element's 'style' attribute
        if (NS_CONTENT_ATTR_HAS_VALUE ==
                GetAttr(kNameSpaceID_None, nsXULAtoms::style, value)) {
            nsICSSParser* parser = GetCSSParser();

            rv |= parser->ParseStyleAttribute(value, aDocumentURI,
                                              getter_AddRefs(mInlineStyleRule));

            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to parse style rule");
        }
    }

    rv |= aStream->Read32(&number);
    mNumChildren = PRInt32(number);

    if (mNumChildren > 0) {
        mChildren = new nsXULPrototypeNode*[mNumChildren];
        if (! mChildren)
            return NS_ERROR_OUT_OF_MEMORY;

        memset(mChildren, 0, sizeof(nsXULPrototypeNode*) * mNumChildren);

        for (i = 0; i < mNumChildren; i++) {
            rv |= aStream->Read32(&number);
            Type childType = (Type)number;

            nsXULPrototypeNode* child = nsnull;

            switch (childType) {
            case eType_Element:
                child = new nsXULPrototypeElement();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                rv |= child->Deserialize(aStream, aContext, aDocumentURI,
                                         aNodeInfos);
                break;
            case eType_Text:
                child = new nsXULPrototypeText();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                rv |= child->Deserialize(aStream, aContext, aDocumentURI,
                                         aNodeInfos);
                break;
            case eType_Script: {
                // language version obtained during deserialization.
                nsXULPrototypeScript* script = new nsXULPrototypeScript(0, nsnull);
                if (! script)
                    return NS_ERROR_OUT_OF_MEMORY;
                child = script;
                child->mType = childType;

                rv |= aStream->Read8(&script->mOutOfLine);
                if (! script->mOutOfLine) {
                    rv |= script->Deserialize(aStream, aContext,
                                              aDocumentURI, aNodeInfos);
                }
                else {
                    rv |= aStream->ReadObject(PR_TRUE, getter_AddRefs(script->mSrcURI));

                    rv |= script->DeserializeOutOfLine(aStream, aContext);
                }
                break;
            }
            }

            mChildren[i] = child;

            // Oh dear. Something failed during the deserialization.
            // We don't know what.  But likely consequences of failed
            // deserializations included calls to |AbortFastLoads| which
            // shuts down the FastLoadService and closes our streams.
            // If that happens, next time through this loop, we die a messy
            // death. So, let's just fail now, and propagate that failure
            // upward so that the ChromeProtocolHandler knows it can't use
            // a cached chrome channel for this.
            if (NS_FAILED(rv))
                return rv;
        }
    }

    return rv;
}


nsresult
nsXULPrototypeElement::GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, nsAString& aValue)
{
    for (PRInt32 i = 0; i < mNumAttributes; ++i) {
        if (mAttributes[i].mNodeInfo->Equals(aName, aNameSpaceID)) {
            mAttributes[i].mValue.GetValue( aValue );
            return aValue.IsEmpty() ? NS_CONTENT_ATTR_NO_VALUE : NS_CONTENT_ATTR_HAS_VALUE;
        }

    }
    return NS_CONTENT_ATTR_NOT_THERE;
}


//----------------------------------------------------------------------
//
// nsXULPrototypeScript
//

nsXULPrototypeScript::nsXULPrototypeScript(PRUint16 aLineNo, const char *aVersion)
    : nsXULPrototypeNode(eType_Script),
      mLineNo(aLineNo),
      mSrcLoading(PR_FALSE),
      mOutOfLine(PR_TRUE),
      mSrcLoadWaiters(nsnull),
      mJSObject(nsnull),
      mLangVersion(aVersion)
{
    MOZ_COUNT_CTOR(nsXULPrototypeScript);
    AddJSGCRoot(&mJSObject, "nsXULPrototypeScript::mJSObject");
}


nsXULPrototypeScript::~nsXULPrototypeScript()
{
    RemoveJSGCRoot(&mJSObject);
    MOZ_COUNT_DTOR(nsXULPrototypeScript);
}


nsresult
nsXULPrototypeScript::Serialize(nsIObjectOutputStream* aStream,
                                nsIScriptContext* aContext,
                                nsISupportsArray* aNodeInfos)
{
    NS_ASSERTION(!mSrcLoading || mSrcLoadWaiters != nsnull || !mJSObject,
                 "script source still loading when serializing?!");
    if (!mJSObject)
        return NS_ERROR_FAILURE;

    nsresult rv;

    // Write basic prototype data
    aStream->Write16(mLineNo);

    JSContext* cx = NS_REINTERPRET_CAST(JSContext*,
                                        aContext->GetNativeContext());
    JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_ENCODE);
    if (! xdr)
        return NS_ERROR_OUT_OF_MEMORY;
    xdr->userdata = (void*) aStream;

    JSScript *script = NS_REINTERPRET_CAST(JSScript*,
                                           ::JS_GetPrivate(cx, mJSObject));
    if (! ::JS_XDRScript(xdr, &script)) {
        rv = NS_ERROR_FAILURE;  // likely to be a principals serialization error
    } else {
        // Get the encoded JSXDRState data and write it.  The JSXDRState owns
        // this buffer memory and will free it beneath ::JS_XDRDestroy.
        //
        // If an XPCOM object needs to be written in the midst of the JS XDR
        // encoding process, the C++ code called back from the JS engine (e.g.,
        // nsEncodeJSPrincipals in caps/src/nsJSPrincipals.cpp) will flush data
        // from the JSXDRState to aStream, then write the object, then return
        // to JS XDR code with xdr reset so new JS data is encoded at the front
        // of the xdr's data buffer.
        //
        // However many XPCOM objects are interleaved with JS XDR data in the
        // stream, when control returns here from ::JS_XDRScript, we'll have
        // one last buffer of data to write to aStream.

        uint32 size;
        const char* data = NS_REINTERPRET_CAST(const char*,
                                               ::JS_XDRMemGetData(xdr, &size));
        NS_ASSERTION(data, "no decoded JSXDRState data!");

        rv = aStream->Write32(size);
        if (NS_SUCCEEDED(rv))
            rv = aStream->WriteBytes(data, size);
    }

    ::JS_XDRDestroy(xdr);
    if (NS_FAILED(rv)) return rv;

    PRUint32 version = PRUint32(mLangVersion
                                ? ::JS_StringToVersion(mLangVersion)
                                : JSVERSION_DEFAULT);
    rv = aStream->Write32(version);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsXULPrototypeScript::SerializeOutOfLine(nsIObjectOutputStream* aStream,
                                         nsIScriptContext* aContext)
{
    nsIXULPrototypeCache* cache = GetXULCache();
#ifdef NS_DEBUG
    PRBool useXULCache = PR_TRUE;
    cache->GetEnabled(&useXULCache);
    NS_ASSERTION(useXULCache,
                 "writing to the FastLoad file, but the XUL cache is off?");
#endif

    nsCOMPtr<nsIFastLoadService> fastLoadService;
    cache->GetFastLoadService(getter_AddRefs(fastLoadService));

    nsresult rv = NS_OK;
    if (!fastLoadService)
        return rv;

    nsCAutoString urispec;
    rv = mSrcURI->GetAsciiSpec(urispec);
    if (NS_FAILED(rv))
        return rv;

    PRBool exists = PR_FALSE;
    fastLoadService->HasMuxedDocument(urispec.get(), &exists);
    if (exists)
        return rv;

    // Allow callers to pass null for aStream, meaning
    // "use the FastLoad service's default output stream."
    // See nsXULDocument.cpp for one use of this.
    nsCOMPtr<nsIObjectOutputStream> objectOutput = aStream;
    if (! objectOutput)
        fastLoadService->GetOutputStream(getter_AddRefs(objectOutput));

    rv = fastLoadService->
         StartMuxedDocument(mSrcURI, urispec.get(),
                            nsIFastLoadService::NS_FASTLOAD_WRITE);
    NS_ASSERTION(rv != NS_ERROR_NOT_AVAILABLE, "reading FastLoad?!");

    nsCOMPtr<nsIURI> oldURI;
    rv |= fastLoadService->SelectMuxedDocument(mSrcURI, getter_AddRefs(oldURI));
    rv |= Serialize(objectOutput, aContext, nsnull);
    rv |= fastLoadService->EndMuxedDocument(mSrcURI);

    if (oldURI) {
        nsCOMPtr<nsIURI> tempURI;
        rv |= fastLoadService->
              SelectMuxedDocument(oldURI, getter_AddRefs(tempURI));
    }

    if (NS_FAILED(rv))
        cache->AbortFastLoads();
    return rv;
}


nsresult
nsXULPrototypeScript::Deserialize(nsIObjectInputStream* aStream,
                                  nsIScriptContext* aContext,
                                  nsIURI* aDocumentURI,
                                  nsISupportsArray* aNodeInfos)
{
    NS_TIMELINE_MARK_FUNCTION("chrome js deserialize");
    nsresult rv;

    // Read basic prototype data
    aStream->Read16(&mLineNo);

    NS_ASSERTION(!mSrcLoading || mSrcLoadWaiters != nsnull || !mJSObject,
                 "prototype script not well-initialized when deserializing?!");

    PRUint32 size;
    rv = aStream->Read32(&size);
    if (NS_FAILED(rv)) return rv;

    char* data;
    rv = aStream->ReadBytes(size, &data);
    if (NS_SUCCEEDED(rv)) {
        JSContext* cx = NS_REINTERPRET_CAST(JSContext*,
                                            aContext->GetNativeContext());

        JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_DECODE);
        if (! xdr) {
            rv = NS_ERROR_OUT_OF_MEMORY;
        } else {
            xdr->userdata = (void*) aStream;
            ::JS_XDRMemSetData(xdr, data, size);

            JSScript *script = nsnull;
            if (! ::JS_XDRScript(xdr, &script)) {
                rv = NS_ERROR_FAILURE;  // principals deserialization error?
            } else {
                mJSObject = ::JS_NewScriptObject(cx, script);
                if (! mJSObject) {
                    rv = NS_ERROR_OUT_OF_MEMORY;    // certain error
                    ::JS_DestroyScript(cx, script);
                }
            }

            // Update data in case ::JS_XDRScript called back into C++ code to
            // read an XPCOM object.
            //
            // In that case, the serialization process must have flushed a run
            // of counted bytes containing JS data at the point where the XPCOM
            // object starts, after which an encoding C++ callback from the JS
            // XDR code must have written the XPCOM object directly into the
            // nsIObjectOutputStream.
            //
            // The deserialization process will XDR-decode counted bytes up to
            // but not including the XPCOM object, then call back into C++ to
            // read the object, then read more counted bytes and hand them off
            // to the JSXDRState, so more JS data can be decoded.
            //
            // This interleaving of JS XDR data and XPCOM object data may occur
            // several times beneath the call to ::JS_XDRScript, above.  At the
            // end of the day, we need to free (via nsMemory) the data owned by
            // the JSXDRState.  So we steal it back, nulling xdr's buffer so it
            // doesn't get passed to ::JS_free by ::JS_XDRDestroy.

            uint32 junk;
            data = (char*) ::JS_XDRMemGetData(xdr, &junk);
            if (data)
                ::JS_XDRMemSetData(xdr, NULL, 0);
            ::JS_XDRDestroy(xdr);
        }

        // If data is null now, it must have been freed while deserializing an
        // XPCOM object (e.g., a principal) beneath ::JS_XDRScript.
        if (data)
            nsMemory::Free(data);
    }
    if (NS_FAILED(rv)) return rv;

    PRUint32 version;
    rv = aStream->Read32(&version);
    if (NS_FAILED(rv)) return rv;

    mLangVersion = ::JS_VersionToString(JSVersion(version));
    return NS_OK;
}


nsresult
nsXULPrototypeScript::DeserializeOutOfLine(nsIObjectInputStream* aInput,
                                           nsIScriptContext* aContext)
{
    // Keep track of FastLoad failure via rv, so we can
    // AbortFastLoads if things look bad.
    nsresult rv = NS_OK;

    nsIXULPrototypeCache* cache = GetXULCache();
    nsCOMPtr<nsIFastLoadService> fastLoadService;
    cache->GetFastLoadService(getter_AddRefs(fastLoadService));

    // Allow callers to pass null for aInput, meaning
    // "use the FastLoad service's default input stream."
    // See nsXULContentSink.cpp for one use of this.
    nsCOMPtr<nsIObjectInputStream> objectInput = aInput;
    if (! objectInput && fastLoadService)
        fastLoadService->GetInputStream(getter_AddRefs(objectInput));

    if (objectInput) {
        PRBool useXULCache = PR_TRUE;
        if (mSrcURI) {
            // NB: we must check the XUL script cache early, to avoid
            // multiple deserialization attempts for a given script, which
            // would exhaust the multiplexed stream containing the singly
            // serialized script.  Note that nsXULDocument::LoadScript
            // checks the XUL script cache too, in order to handle the
            // serialization case.
            //
            // We need do this only for <script src='strres.js'> and the
            // like, i.e., out-of-line scripts that are included by several
            // different XUL documents multiplexed in the FastLoad file.
            cache->GetEnabled(&useXULCache);

            if (useXULCache) {
                cache->GetScript(mSrcURI, NS_REINTERPRET_CAST(void**, &mJSObject));
            }
        }

        if (! mJSObject) {
            nsCOMPtr<nsIURI> oldURI;

            if (mSrcURI) {
                nsCAutoString spec;
                mSrcURI->GetAsciiSpec(spec);
                rv = fastLoadService->StartMuxedDocument(mSrcURI, spec.get(),
                                                         nsIFastLoadService::NS_FASTLOAD_READ);
                if (NS_SUCCEEDED(rv))
                    rv = fastLoadService->SelectMuxedDocument(mSrcURI, getter_AddRefs(oldURI));
            } else {
                // An inline script: check FastLoad multiplexing direction
                // and skip Deserialize if we're not reading from a
                // muxed stream to get inline objects that are contained in
                // the current document.
                PRInt32 direction;
                fastLoadService->GetDirection(&direction);
                if (direction != nsIFastLoadService::NS_FASTLOAD_READ)
                    rv = NS_ERROR_NOT_AVAILABLE;
            }

            // We do reflect errors into rv, but our caller may want to
            // ignore our return value, because mJSObject will be null
            // after any error, and that suffices to cause the script to
            // be reloaded (from the src= URI, if any) and recompiled.
            // We're better off slow-loading than bailing out due to a
            // FastLoad error.
            if (NS_SUCCEEDED(rv))
                rv = Deserialize(objectInput, aContext, nsnull, nsnull);

            if (NS_SUCCEEDED(rv) && mSrcURI) {
                rv = fastLoadService->EndMuxedDocument(mSrcURI);

                if (NS_SUCCEEDED(rv) && oldURI) {
                    nsCOMPtr<nsIURI> tempURI;
                    rv = fastLoadService->SelectMuxedDocument(oldURI, getter_AddRefs(tempURI));

                    NS_ASSERTION(NS_SUCCEEDED(rv) && (!tempURI || tempURI == mSrcURI),
                                 "not currently deserializing into the script we thought we were!");
                }
            }

            if (NS_SUCCEEDED(rv)) {
                if (useXULCache && mSrcURI) {
                    PRBool isChrome = PR_FALSE;
                    mSrcURI->SchemeIs("chrome", &isChrome);
                    if (isChrome) {
                        cache->PutScript(mSrcURI, NS_REINTERPRET_CAST(void*, mJSObject));
                    }
                }
            } else {
                // If mSrcURI is not in the FastLoad multiplex,
                // rv will be NS_ERROR_NOT_AVAILABLE and we'll try to
                // update the FastLoad file to hold a serialization of
                // this script, once it has finished loading.
                if (rv != NS_ERROR_NOT_AVAILABLE)
                    cache->AbortFastLoads();
            }
        }
    }

    return rv;
}

nsresult
nsXULPrototypeScript::Compile(const PRUnichar* aText,
                              PRInt32 aTextLength,
                              nsIURI* aURI,
                              PRUint16 aLineNo,
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
    // document because it keeps the prototype cache alive. Circularity!
    nsresult rv;

    // Use the prototype document's special context
    nsCOMPtr<nsIScriptContext> context;

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
    }

    // Use the enclosing document's principal
    // XXX is this right? or should we use the protodoc's?
    nsCOMPtr<nsIPrincipal> principal;
    rv = aDocument->GetPrincipal(getter_AddRefs(principal));
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString urlspec;
    aURI->GetSpec(urlspec);

    // Ok, compile it to create a prototype script object!
    rv = context->CompileScript(aText,
                                aTextLength,
                                nsnull,
                                principal,
                                urlspec.get(),
                                aLineNo,
                                mLangVersion,
                                (void**)&mJSObject);

    return rv;
}

//----------------------------------------------------------------------
//
// nsXULPrototypeText
//

nsresult
nsXULPrototypeText::Serialize(nsIObjectOutputStream* aStream,
                              nsIScriptContext* aContext,
                              nsISupportsArray* aNodeInfos)
{
    nsresult rv;

    // Write basic prototype data
    rv = aStream->Write32(mType);

    rv |= aStream->WriteWStringZ(mValue.get());

    return rv;
}

nsresult
nsXULPrototypeText::Deserialize(nsIObjectInputStream* aStream,
                                nsIScriptContext* aContext,
                                nsIURI* aDocumentURI,
                                nsISupportsArray* aNodeInfos)
{
    nsresult rv;

    // Write basic prototype data
    rv = aStream->ReadString(mValue);

    return rv;
}
