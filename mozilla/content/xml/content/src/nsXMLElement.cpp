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


static inline nsresult MakeURI(const nsACString &aSpec, nsIURI *aBase, nsIURI **aURI)
{
  nsresult rv;
  static NS_DEFINE_CID(ioServCID,NS_IOSERVICE_CID);
  nsCOMPtr<nsIIOService> service(do_GetService(ioServCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  return service->NewURI(aSpec,nsnull,aBase,aURI);
}

NS_IMETHODIMP
nsXMLElement::GetXMLBaseURI(nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  *aURI = nsnull;
  
  nsresult rv;

  nsAutoString base;
  nsCOMPtr<nsIContent> content(do_QueryInterface(NS_STATIC_CAST(nsIXMLContent*,this),&rv));
  while (NS_SUCCEEDED(rv) && content) {
    nsAutoString value;
    rv = content->GetAttr(kNameSpaceID_XML,nsHTMLAtoms::base,value);
    PRInt32 value_len;
    if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
      PRInt32 colon = value.FindChar(':');
      PRInt32 slash = value.FindChar('/');
      if (colon > 0 && !( slash >= 0 && slash < colon)) {
        // Yay, we have absolute path!
        // The complex looking if above is to make sure that we do not erroneously
        // think a value of "./this:that" would have a scheme of "./that"

        NS_ConvertUCS2toUTF8 str(value);
      
        rv = MakeURI(str,nsnull,aURI);
        if (NS_FAILED(rv))
          break;

        if (!base.IsEmpty()) { // XXXdarin base is always empty
          str = NS_ConvertUCS2toUTF8(base);
          nsCAutoString resolvedStr;
          rv = (*aURI)->Resolve(str, resolvedStr);
          if (NS_FAILED(rv)) break;
          rv = (*aURI)->SetSpec(resolvedStr);
        }
        break;

      } else if ((value_len = value.Length()) > 0) {        
        if (!base.IsEmpty()) {
          if (base[0] == '/') {
            // Do nothing, we are waiting for a scheme starting value
          } else {
            // We do not want to add double / delimiters (although the user is free to do so)
            if (value[value_len - 1] != '/')
              value.Append(PRUnichar('/'));
            base.Insert(value, 0);
          }
        } else {
          if (value[value_len - 1] != '/')
            value.Append(PRUnichar('/')); // Add delimiter/make sure we treat this as dir
          base = value;
        }
      }
    } // if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
    nsCOMPtr<nsIContent> parent;
    rv = content->GetParent(*getter_AddRefs(parent));
    content = parent;
  } // while

  if (NS_SUCCEEDED(rv)) {
    if (!*aURI && mDocument) {
      nsCOMPtr<nsIURI> docBase;
      mDocument->GetBaseURL(*getter_AddRefs(docBase));
      if (!docBase) {
        mDocument->GetDocumentURL(getter_AddRefs(docBase));
      }
      if (base.IsEmpty()) {
        *aURI = docBase.get();    
        NS_IF_ADDREF(*aURI);  // nsCOMPtr releases this once
      } else {
        NS_ConvertUCS2toUTF8 str(base);
        rv = MakeURI(str,docBase,aURI);
      }
    }

    // Finally do a security check, almost the same as nsDocument::SetBaseURL()
    if (*aURI) {
      nsCOMPtr<nsIScriptSecurityManager> securityManager = 
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIURI> docURI;
        mDocument->GetDocumentURL(getter_AddRefs(docURI));
        rv = securityManager->CheckLoadURI(docURI, *aURI, nsIScriptSecurityManager::STANDARD);
        if (NS_FAILED(rv)) {
          // Now we need to get the "closest" allowed base URI
          NS_RELEASE(*aURI);

          if (content) { // content is the last content we tried above
            nsCOMPtr<nsIContent> parent;
            content->GetParent(*getter_AddRefs(parent));
            content = parent;
            while (content) {
              nsCOMPtr<nsIXMLContent> xml(do_QueryInterface(content));
              if (xml) {
                return xml->GetXMLBaseURI(aURI);
              }
              content->GetParent(*getter_AddRefs(parent));
              content = parent;
            }
          }
          
          nsCOMPtr<nsIURI> docBase;
          mDocument->GetBaseURL(*getter_AddRefs(docBase));
          if (!docBase) {
            mDocument->GetDocumentURL(getter_AddRefs(docBase));
          }

          *aURI = docBase.get();
          NS_IF_ADDREF(*aURI);
          rv = NS_OK;
        }
      }
    }
  }

  if (NS_FAILED(rv)) {
    NS_IF_RELEASE(*aURI);
  }

  return rv;
}

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


static nsresult CheckLoadURI(nsIURI *aBaseURI, const nsAString& aURI,
                             nsIURI **aAbsURI)
{
  NS_ConvertUCS2toUTF8 str(aURI);

  *aAbsURI = nsnull;

  nsresult rv;
  rv = MakeURI(str,aBaseURI,aAbsURI);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIScriptSecurityManager> securityManager = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv= securityManager->CheckLoadURI(aBaseURI,
                                         *aAbsURI,
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
        rv = GetXMLBaseURI(getter_AddRefs(base));
        if (NS_FAILED(rv))
          break;

        // href= ?
        rv = nsGenericContainerElement::GetAttr(kNameSpaceID_XLink,
                                                nsHTMLAtoms::href,
                                                value);
        if (rv == NS_CONTENT_ATTR_HAS_VALUE && !value.IsEmpty()) {
          nsCOMPtr<nsIURI> uri;
          rv = CheckLoadURI(base,value,getter_AddRefs(uri));
          if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIPresContext> pc;
            rv = DocShellToPresContext(aShell,getter_AddRefs(pc));
            if (NS_SUCCEEDED(rv)) {
              rv = TriggerLink(pc, verb, base, value,
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
          nsIURI* baseURL = nsnull;
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

          GetXMLBaseURI(&baseURL);

          ret = TriggerLink(aPresContext, verb, baseURL, href, target,
                            PR_TRUE);

          NS_IF_RELEASE(baseURL);
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
        nsIURI* baseURL = nsnull;
        nsGenericContainerElement::GetAttr(kNameSpaceID_XLink,
                                           nsHTMLAtoms::href,
                                           href);
        if (href.IsEmpty()) {
          *aEventStatus = nsEventStatus_eConsumeDoDefault; 
          break;
        }

        GetXMLBaseURI(&baseURL);

        ret = TriggerLink(aPresContext, eLinkVerb_Replace, baseURL, href,
                          target, PR_FALSE);
        
        NS_IF_RELEASE(baseURL);
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

#if 0
NS_IMETHODIMP
nsXMLElement::GetScriptObject(nsIScriptContext* aContext, void** aScriptObject)
{
  nsresult res = NS_OK;

  // XXX Yuck! Reaching into the generic content object isn't good.
  nsDOMSlots *slots = GetDOMSlots();

  if (!slots) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!slots->mScriptObject) {
    nsIDOMScriptObjectFactory *factory;

    res = nsGenericElement::GetScriptObjectFactory(&factory);
    if (NS_FAILED(res)) {
      return res;
    }

    nsAutoString tag;
    mNodeInfo->GetQualifiedName(tag);

    res = factory->NewScriptXMLElement(tag, aContext,
                                       NS_STATIC_CAST(nsIContent *, this),
                                       mParent, (void**)&slots->mScriptObject);
    NS_RELEASE(factory);

    if (mDocument && slots->mScriptObject) {
      aContext->AddNamedReference((void *)&slots->mScriptObject,
                                  slots->mScriptObject,
                                  "nsXMLElement::mScriptObject");

      // See if we have a frame.  
      nsCOMPtr<nsIPresShell> shell;
      mDocument->GetShellAt(0, getter_AddRefs(shell));
      if (shell) {
        nsIFrame* frame;
        shell->GetPrimaryFrameFor(this, &frame);
        if (!frame) {
          // We must ensure that the XBL Binding is installed before we hand
          // back this object.
          nsCOMPtr<nsIBindingManager> bindingManager;
          mDocument->GetBindingManager(getter_AddRefs(bindingManager));
          nsCOMPtr<nsIXBLBinding> binding;
          bindingManager->GetBinding(this, getter_AddRefs(binding));
          if (!binding) {
            nsCOMPtr<nsIScriptGlobalObject> global;
            mDocument->GetScriptGlobalObject(getter_AddRefs(global));
            nsCOMPtr<nsIDOMViewCSS> viewCSS(do_QueryInterface(global));
            if (viewCSS) {
              nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
              nsAutoString empty;
              nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(NS_STATIC_CAST(nsIContent *, this)));
              viewCSS->GetComputedStyle(elt, empty, getter_AddRefs(cssDecl));
              if (cssDecl) {
                nsAutoString behavior;
                behavior.Assign(NS_LITERAL_STRING("-moz-binding"));

                nsAutoString value;
                cssDecl->GetPropertyValue(behavior, value);
                if (!value.IsEmpty()) {
                  // We have a binding that must be installed.
                  nsresult rv;
                  PRBool dummy;
                  nsCOMPtr<nsIXBLService> xblService = 
                           do_GetService("@mozilla.org/xbl;1", &rv);
                  xblService->LoadBindings(this, value, PR_FALSE,
                                           getter_AddRefs(binding), &dummy);
                  if (binding) {
                    binding->ExecuteAttachedHandler();
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  *aScriptObject = slots->mScriptObject;
  return res;
}
#endif

// nsIStyledContent implementation

NS_IMETHODIMP
nsXMLElement::GetID(nsIAtom*& aResult) const
{
  nsresult rv;  
  nsCOMPtr<nsIAtom> atom;
  rv = mNodeInfo->GetIDAttributeAtom(getter_AddRefs(atom));
  
  aResult = nsnull;
  if (NS_SUCCEEDED(rv) && atom) {
    nsAutoString value;
    rv = nsGenericContainerElement::GetAttr(kNameSpaceID_Unknown, atom,
                                            value);
    if (NS_SUCCEEDED(rv))
      aResult = NS_NewAtom(value);
  }

  return rv;
}
