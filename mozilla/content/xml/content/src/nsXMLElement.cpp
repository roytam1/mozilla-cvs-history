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

#include "nsXMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsLayoutAtoms.h"
#include "nsIDocument.h"
#include "nsIAtom.h"
#include "nsNetUtil.h"
#include "nsIEventListenerManager.h"
#include "nsIDocShell.h"
#include "nsIEventStateManager.h"
#include "nsIDOMEvent.h"
#include "nsINameSpace.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsNetCID.h"
#include "nsIServiceManager.h"
#include "nsXPIDLString.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIScriptSecurityManager.h"
#include "nsIRefreshURI.h"
#include "nsStyleConsts.h"
#include "nsIPresShell.h"
#include "nsGUIEvent.h"
#include "nsIPresContext.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMViewCSS.h"
#include "nsIXBLService.h"
#include "nsIXBLBinding.h"
#include "nsIBindingManager.h"

nsresult
NS_NewXMLElement(nsIContent** aInstancePtrResult, nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);
  *aInstancePtrResult = nsnull;

  nsXMLElement* it = new nsXMLElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(it);
  nsresult rv = it->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    NS_RELEASE(it);
    return rv;
  }

  *aInstancePtrResult = NS_STATIC_CAST(nsIXMLContent *, it);

  return NS_OK;
}

nsXMLElement::nsXMLElement() : mIsLink(PR_FALSE)
{
}

nsXMLElement::~nsXMLElement()
{
}


// QueryInterface implementation for nsXMLElement
NS_IMETHODIMP 
nsXMLElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_ENSURE_ARG_POINTER(aInstancePtr);
  *aInstancePtr = nsnull;

  nsresult rv = nsGenericContainerElement::QueryInterface(aIID, aInstancePtr);

  if (NS_SUCCEEDED(rv))
    return rv;

  nsISupports *inst = nsnull;

  if (aIID.Equals(NS_GET_IID(nsIDOMNode))) {
    inst = NS_STATIC_CAST(nsIDOMNode *, this);
  } else if (aIID.Equals(NS_GET_IID(nsIDOMElement))) {
    inst = NS_STATIC_CAST(nsIDOMElement *, this);
  } else if (aIID.Equals(NS_GET_IID(nsIXMLContent))) {
    inst = NS_STATIC_CAST(nsIXMLContent *, this);
  } else if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    inst = nsContentUtils::GetClassInfoInstance(eDOMClassInfo_Element_id);
    NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
  } else {
    return PostQueryInterface(aIID, aInstancePtr);
  }

  NS_ADDREF(inst);

  *aInstancePtr = inst;

  return NS_OK;
}


NS_IMPL_ADDREF_INHERITED(nsXMLElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsXMLElement, nsGenericElement)


NS_IMETHODIMP
nsXMLElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                      const nsAString& aValue,
                      PRBool aNotify)
{
  return nsGenericContainerElement::SetAttr(aNameSpaceID, aName, aValue,
                                            aNotify);
}

NS_IMETHODIMP 
nsXMLElement::SetAttr(nsINodeInfo *aNodeInfo,
                      const nsAString& aValue,
                      PRBool aNotify)
{
  NS_ENSURE_ARG_POINTER(aNodeInfo);

  if (aNodeInfo->Equals(nsHTMLAtoms::type, kNameSpaceID_XLink)) { 
    
    // NOTE: This really is a link according to the XLink spec,
    //       we do not need to check other attributes. If there
    //       is no href attribute, then this link is simply
    //       untraversible [XLink 3.2].
    mIsLink = aValue.Equals(NS_LITERAL_STRING("simple"));

    // We will check for actuate="onLoad" in MaybeTriggerAutoLink
  }

  return nsGenericContainerElement::SetAttr(aNodeInfo, aValue, aNotify);
}

static nsresult DocShellToPresContext(nsIDocShell *aShell,
                                      nsIPresContext **aPresContext)
{
  *aPresContext = nsnull;

  nsresult rv;
  nsCOMPtr<nsIDocShell> ds = do_QueryInterface(aShell,&rv);
  if (NS_FAILED(rv))
    return rv;

  return ds->GetPresContext(aPresContext);
}

static nsresult CheckLoadURI(const nsString& aSpec, nsIURI *aBaseURI,
                             nsIDocument* aDocument, nsIURI **aAbsURI)
{
  *aAbsURI = nsnull;

  nsresult rv;
  rv = nsContentUtils::NewURIWithDocumentCharset(aAbsURI, aSpec, aDocument,
                                                 aBaseURI);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIScriptSecurityManager> securityManager = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = securityManager->CheckLoadURI(aBaseURI, *aAbsURI,
                                         nsIScriptSecurityManager::DISALLOW_FROM_MAIL);
    }
  }

  if (NS_FAILED(rv)) {
    NS_IF_RELEASE(*aAbsURI);
  }

  return rv;
}

static inline nsresult SpecialAutoLoadReturn(nsresult aRv, nsLinkVerb aVerb)
{
  if (NS_SUCCEEDED(aRv)) {
    switch(aVerb) {
      case eLinkVerb_Embed:
        aRv = NS_XML_AUTOLINK_EMBED;
        break;
      case eLinkVerb_New:
        aRv = NS_XML_AUTOLINK_NEW;
        break;
      case eLinkVerb_Replace:
        aRv = NS_XML_AUTOLINK_REPLACE;
        break;
      default:
        aRv = NS_XML_AUTOLINK_UNDEFINED;
        break;
    }
  }
  return aRv;
}

NS_IMETHODIMP
nsXMLElement::MaybeTriggerAutoLink(nsIDocShell *aShell)
{
  NS_ENSURE_ARG_POINTER(aShell);

  nsresult rv = NS_OK;

  if (mIsLink) {
    NS_NAMED_LITERAL_STRING(onloadString, "onLoad");
    do {
      // actuate="onLoad" ?
      nsAutoString value;
      rv = nsGenericContainerElement::GetAttr(kNameSpaceID_XLink,
                                              nsLayoutAtoms::actuate,
                                              value);
      if (rv == NS_CONTENT_ATTR_HAS_VALUE &&
          value.Equals(onloadString)) {

        // Disable in Mail/News for now. We may want a pref to control
        // this at some point.
        nsCOMPtr<nsIDocShellTreeItem> docShellItem(do_QueryInterface(aShell));
        if (docShellItem) {
          nsCOMPtr<nsIDocShellTreeItem> rootItem;
          docShellItem->GetRootTreeItem(getter_AddRefs(rootItem));
          nsCOMPtr<nsIDocShell> docshell(do_QueryInterface(rootItem));
          if (docshell) {
            PRUint32 appType;
            if (NS_SUCCEEDED(docshell->GetAppType(&appType)) &&
                appType == nsIDocShell::APP_TYPE_MAIL) {
              return NS_OK;
            }
          }
        }

        // show= ?
        nsLinkVerb verb = eLinkVerb_Undefined; // basically means same as replace
        rv = nsGenericContainerElement::GetAttr(kNameSpaceID_XLink,
                                                nsLayoutAtoms::show, value);
        if (NS_FAILED(rv))
          break;

        // XXX Should probably do this using atoms 
        if (value.Equals(NS_LITERAL_STRING("new"))) {
          nsCOMPtr<nsIPrefBranch> prefBranch =
            do_GetService(NS_PREFSERVICE_CONTRACTID);

          PRBool boolPref = PR_FALSE;
          if (prefBranch) {
            prefBranch->GetBoolPref("dom.disable_open_during_load", &boolPref);
            if (boolPref) {
              // disabling open during load

              return NS_OK;
            }

            prefBranch->GetBoolPref("browser.block.target_new_window",
                                    &boolPref);
          }
          if (!boolPref) {
            // not blocking new windows
            verb = eLinkVerb_New;
          }
        } else if (value.Equals(NS_LITERAL_STRING("replace"))) {
          // We want to actually stop processing the current document now.
          // We do this by returning the correct value so that the one
          // that called us knows to stop processing.
          verb = eLinkVerb_Replace;
        } else if (value.Equals(NS_LITERAL_STRING("embed"))) {
          // XXX TODO
          break;
        }

        // base
        nsCOMPtr<nsIURI> base;
        rv = GetBaseURL(getter_AddRefs(base));
        if (NS_FAILED(rv))
          break;

        // href= ?
        rv = nsGenericContainerElement::GetAttr(kNameSpaceID_XLink,
                                                nsHTMLAtoms::href,
                                                value);
        if (rv == NS_CONTENT_ATTR_HAS_VALUE && !value.IsEmpty()) {
          nsCOMPtr<nsIURI> uri;
          rv = CheckLoadURI(value, base, mDocument, getter_AddRefs(uri));
          if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIPresContext> pc;
            rv = DocShellToPresContext(aShell, getter_AddRefs(pc));
            if (NS_SUCCEEDED(rv)) {
              rv = TriggerLink(pc, verb, base, uri,
                               NS_LITERAL_STRING(""), PR_TRUE);

              return SpecialAutoLoadReturn(rv,verb);
            }
          }
        } // href
      }
    } while (0);
  }

  return rv;
}

NS_IMETHODIMP 
nsXMLElement::HandleDOMEvent(nsIPresContext* aPresContext,
                             nsEvent* aEvent,
                             nsIDOMEvent** aDOMEvent,
                             PRUint32 aFlags,
                             nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  // Try script event handlers first
  nsresult ret = nsGenericContainerElement::HandleDOMEvent(aPresContext,
                                                           aEvent,
                                                           aDOMEvent,
                                                           aFlags,
                                                           aEventStatus);

  if (mIsLink && (NS_OK == ret) && (nsEventStatus_eIgnore == *aEventStatus) &&
      !(aFlags & NS_EVENT_FLAG_CAPTURE) && !(aFlags & NS_EVENT_FLAG_SYSTEM_EVENT)) {
    switch (aEvent->message) {
    case NS_MOUSE_LEFT_BUTTON_DOWN:
      {
        nsIEventStateManager *stateManager;
        if (NS_OK == aPresContext->GetEventStateManager(&stateManager)) {
          stateManager->SetContentState(this, NS_EVENT_STATE_ACTIVE | NS_EVENT_STATE_FOCUS);
          NS_RELEASE(stateManager);
        }
        *aEventStatus = nsEventStatus_eConsumeDoDefault;
      }
      break;

    case NS_MOUSE_LEFT_CLICK:
      {
        if (nsEventStatus_eConsumeNoDefault != *aEventStatus) {
          nsInputEvent* inputEvent = NS_STATIC_CAST(nsInputEvent*, aEvent);
          if (inputEvent->isControl || inputEvent->isMeta ||
              inputEvent->isAlt || inputEvent->isShift) {
            break;  // let the click go through so we can handle it in JS/XUL
          }
          nsAutoString show, href, target;
          nsLinkVerb verb = eLinkVerb_Undefined; // basically means same as replace
          nsGenericContainerElement::GetAttr(kNameSpaceID_XLink,
                                             nsHTMLAtoms::href,
                                             href);
          if (href.IsEmpty()) {
            *aEventStatus = nsEventStatus_eConsumeDoDefault; 
            break;
          }

          nsGenericContainerElement::GetAttr(kNameSpaceID_XLink,
                                             nsLayoutAtoms::show,
                                             show);

          // XXX Should probably do this using atoms 
          if (show.Equals(NS_LITERAL_STRING("new"))) {
            nsCOMPtr<nsIPrefBranch> prefBranch =
              do_GetService(NS_PREFSERVICE_CONTRACTID);

            PRBool blockNewWindow = PR_FALSE;
            if (prefBranch) {
              prefBranch->GetBoolPref("browser.block.target_new_window",
                                      &blockNewWindow);
            }
            if (!blockNewWindow) {
              verb = eLinkVerb_New;
            }
          } else if (show.Equals(NS_LITERAL_STRING("replace"))) {
            verb = eLinkVerb_Replace;
          } else if (show.Equals(NS_LITERAL_STRING("embed"))) {
            verb = eLinkVerb_Embed;
          }

          nsCOMPtr<nsIURI> baseURL;
          GetBaseURL(getter_AddRefs(baseURL));
          nsCOMPtr<nsIURI> uri;
          ret = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri),
                                                          href,
                                                          mDocument,
                                                          baseURL);
          if (NS_SUCCEEDED(ret)) {
            ret = TriggerLink(aPresContext, verb, baseURL, uri, target,
                              PR_TRUE);
          }

          *aEventStatus = nsEventStatus_eConsumeDoDefault; 
        }
      }
      break;

    case NS_MOUSE_RIGHT_BUTTON_DOWN:
      // XXX Bring up a contextual menu provided by the application
      break;

    case NS_KEY_PRESS:
      if (aEvent->eventStructType == NS_KEY_EVENT) {
        nsKeyEvent* keyEvent = NS_STATIC_CAST(nsKeyEvent*, aEvent);
        if (keyEvent->keyCode == NS_VK_RETURN) {
          nsMouseEvent event;
          nsEventStatus status = nsEventStatus_eIgnore;

          //fire click
          event.message = NS_MOUSE_LEFT_CLICK;
          event.eventStructType = NS_MOUSE_EVENT;
          nsGUIEvent* guiEvent = NS_STATIC_CAST(nsGUIEvent*, aEvent);
          event.widget = guiEvent->widget;
          event.point = aEvent->point;
          event.refPoint = aEvent->refPoint;
          event.clickCount = 1;
          event.isShift = keyEvent->isShift;
          event.isControl = keyEvent->isControl;
          event.isAlt = keyEvent->isAlt;
          event.isMeta = keyEvent->isMeta;

          nsCOMPtr<nsIPresShell> presShell;
          aPresContext->GetShell(getter_AddRefs(presShell));
          if (presShell) {
            ret = presShell->HandleDOMEventWithTarget(this, &event, &status);
          }
        }
      }
      break;

    case NS_MOUSE_ENTER_SYNTH:
      {
        nsAutoString href, target;
        nsGenericContainerElement::GetAttr(kNameSpaceID_XLink,
                                           nsHTMLAtoms::href,
                                           href);
        if (href.IsEmpty()) {
          *aEventStatus = nsEventStatus_eConsumeDoDefault; 
          break;
        }

        nsCOMPtr<nsIURI> baseURL;
        GetBaseURL(getter_AddRefs(baseURL));

        nsCOMPtr<nsIURI> uri;
        ret = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri),
                                                        href,
                                                        mDocument,
                                                        baseURL);
        if (NS_SUCCEEDED(ret)) {
          ret = TriggerLink(aPresContext, eLinkVerb_Replace, baseURL, uri,
                            target, PR_FALSE);
        }
        
        *aEventStatus = nsEventStatus_eConsumeDoDefault; 
      }
      break;

      // XXX this doesn't seem to do anything yet
    case NS_MOUSE_EXIT_SYNTH:
      {
        ret = LeaveLink(aPresContext);
        *aEventStatus = nsEventStatus_eConsumeDoDefault; 
      }
      break;

    default:
      break;
    }
  }

  return ret;
}

NS_IMETHODIMP
nsXMLElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsXMLElement* it = new nsXMLElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(it);
  nsCOMPtr<nsISupports> kungFuDeathGrip(NS_STATIC_CAST(nsIContent *, this));

  nsresult rv = it->Init(mNodeInfo);

  if (NS_FAILED(rv)) {
    NS_RELEASE(it);
    return rv;
  }

  CopyInnerTo(this, it, aDeep);

  rv = CallQueryInterface(it, aReturn);
  NS_RELEASE(it);
  return rv;
}

// nsIStyledContent implementation

NS_IMETHODIMP
nsXMLElement::GetID(nsIAtom** aResult) const
{
  nsIAtom* atom = GetIDAttributeName();

  *aResult = nsnull;
  nsresult rv = NS_OK;
  if (atom) {
    nsAutoString value;
    rv = nsGenericContainerElement::GetAttr(kNameSpaceID_None, atom, value);
    if (NS_SUCCEEDED(rv)) {
      *aResult = NS_NewAtom(value);
    }
  }

  return rv;
}
