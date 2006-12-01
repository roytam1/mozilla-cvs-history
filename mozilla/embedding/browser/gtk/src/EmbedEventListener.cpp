/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 tw=80 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Christopher Blizzard. Portions created by Christopher Blizzard are Copyright (C) Christopher Blizzard.  All Rights Reserved.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Christopher Blizzard <blizzard@mozilla.org>
 *   Oleg Romashin <romaxa@gmail.com>
 *   Tomaz Noleto <tnoleto@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <nsCOMPtr.h>
#include <nsIDOMMouseEvent.h>

#include <nsIDOMNSEvent.h>
#include "nsIDOMKeyEvent.h"
#include "nsIDOMUIEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"

#include "EmbedEventListener.h"
#include "EmbedPrivate.h"
#include "gtkmozembed_internal.h"

static PRInt32 sLongPressTimer = 0, mLongMPressDelay = 1000;
static PRInt32 sX = 0, sY = 0;
static PRBool  sMPressed = PR_FALSE, sIsScrolling = PR_FALSE;
static char* gFavLocation = NULL;

EmbedEventListener::EmbedEventListener(void)
{
  mOwner = nsnull;
}

EmbedEventListener::~EmbedEventListener()
{
}

NS_IMPL_ADDREF(EmbedEventListener)
NS_IMPL_RELEASE(EmbedEventListener)
NS_INTERFACE_MAP_BEGIN(EmbedEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMUIListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
NS_INTERFACE_MAP_END

nsresult
EmbedEventListener::Init(EmbedPrivate *aOwner)
{
  mOwner = aOwner;
  mCtxInfo = new EmbedContextMenuInfo(aOwner);
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::HandleLink (nsIDOMNode* node)
{
  nsresult result;

  nsCOMPtr<nsIDOMElement> linkElement;
  linkElement = do_QueryInterface (node);
  if (!linkElement) return NS_ERROR_FAILURE;

  nsString name;
  result = GetLinkAttribute(linkElement, "rel", &name);
  if (NS_FAILED(result)) return NS_ERROR_FAILURE;

  nsString link;
  result = GetLinkAttribute(linkElement, "href", &link);
  if (NS_FAILED (result) || !link.Length()) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMDocument> domDoc;
  result = node->GetOwnerDocument(getter_AddRefs(domDoc));
  if (NS_FAILED(result) || !domDoc) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOM3Node> domnode = do_QueryInterface(domDoc);
  if(!domnode) return NS_ERROR_FAILURE;

  nsString spec;
  domnode->GetBaseURI(spec);

  nsCString cSpec;
  NS_UTF16ToCString(spec, NS_CSTRING_ENCODING_UTF8, cSpec);

  nsCOMPtr<nsIURI> baseURI;
  result = NewURI(getter_AddRefs(baseURI), cSpec.get());
  if ((NS_FAILED(result) || !baseURI) return NS_ERROR_FAILURE;

  nsCString linkstring;
  NS_UTF16ToCString(link, NS_CSTRING_ENCODING_UTF8, linkstring);
  nsCString url;
  result = baseURI->Resolve (linkstring, url);
  if (NS_FAILED (result)) return NS_ERROR_FAILURE;

  nsString type;
  result = GetLinkAttribute(linkElement, "type", &type);
  if (NS_FAILED(result)) return NS_ERROR_FAILURE;

  nsCString cType;
  NS_UTF16ToCString(type, NS_CSTRING_ENCODING_UTF8, cType);

  nsString title;
  result = GetLinkAttribute(linkElement, "title", &title);
  if (NS_FAILED(result)) return NS_ERROR_FAILURE;

  nsCString cTitle;
  NS_UTF16ToCString(title, NS_CSTRING_ENCODING_UTF8, cTitle);

  nsCString cName;
  NS_UTF16ToCString(name, NS_CSTRING_ENCODING_UTF8, cName);

  if (!g_ascii_strcasecmp(cName.get(),"SHORTCUT ICON") ||
      !g_ascii_strcasecmp(cName.get(),"ICON")) {

    mOwner->mNeedFav = PR_FALSE;
    this->GetFaviconFromURI(url.get());
  }
  else {

    const gchar *navi_title = cTitle.get();
    if (*navi_title == '\0')
      navi_title = NULL;

    const gchar *navi_type = cType.get();
    if (*navi_type == '\0')
      navi_type = NULL;

    if (!g_ascii_strcasecmp(cName.get(), "ALTERNATE") &&
        !g_ascii_strcasecmp(cType.get(), "application/rss+xml")) {
    }
    else {
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::HandleEvent(nsIDOMEvent* aDOMEvent)
{
  nsString eventType;
  aDOMEvent->GetType(eventType);
  if (eventType.EqualsLiteral ("focus"))
    if (mCtxInfo->GetFormControlType(aDOMEvent)) {
      if (mCtxInfo->mEmbedCtxType & GTK_MOZ_EMBED_CTX_INPUT) {
        gint return_val = FALSE;
        gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                        moz_embed_signals[DOM_FOCUS],
                        (void *)aDOMEvent, &return_val);
        if (return_val) {
          aDOMEvent->StopPropagation();
          aDOMEvent->PreventDefault();
        }
      }
    }

  if (eventType.EqualsLiteral ("DOMLinkAdded") && mOwner->mNeedFav) {

    nsresult result;	
    nsCOMPtr<nsIDOMEventTarget> eventTarget;

    aDOMEvent->GetTarget(getter_AddRefs(eventTarget));
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(eventTarget, &result);
    if (NS_FAILED(result) || !node) 
      return NS_ERROR_FAILURE;
    HandleLink (node);
  }
  else if (mOwner->mNeedFav) {
    mOwner->mNeedFav = PR_FALSE;
    nsCString favicon_url = mOwner->mPrePath + nsCString("/favicon.ico");
    this->GetFaviconFromURI(favicon_url.get());
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::KeyDown(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMKeyEvent> keyEvent;
  keyEvent = do_QueryInterface(aDOMEvent);
  if (!keyEvent)
    return NS_OK;
  // Return FALSE to this function to mark the event as not
  // consumed...
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_KEY_DOWN],
                  (void *)keyEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::KeyUp(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMKeyEvent> keyEvent;
  keyEvent = do_QueryInterface(aDOMEvent);
  if (!keyEvent)
    return NS_OK;
  // return FALSE to this function to mark this event as not
  // consumed...
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_KEY_UP],
                  (void *)keyEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  } else {
    //mCtxInfo->UpdateContextData(aDOMEvent);
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::KeyPress(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMKeyEvent> keyEvent;
  keyEvent = do_QueryInterface(aDOMEvent);
  if (!keyEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_KEY_PRESS],
                  (void *)keyEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

static gboolean
sLongMPress(void *aOwningWidget)
{
  // Return TRUE from your signal handler to mark the event as consumed.
  if (!sMPressed || sIsScrolling)
    return FALSE;
  sMPressed = PR_FALSE;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(aOwningWidget),
                  moz_embed_signals[DOM_MOUSE_LONG_PRESS],
                  (void *)0, &return_val);
  if (return_val) {
    sMPressed = PR_FALSE;
  }
  return FALSE;
}

NS_IMETHODIMP
EmbedEventListener::MouseDown(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;

  // handling event internally, first.
  HandleSelection(mouseEvent);

  // Return TRUE from your signal handler to mark the event as consumed.
  sMPressed = PR_TRUE;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_DOWN],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    sMPressed = PR_FALSE;
    if (sLongPressTimer)
      g_source_remove (sLongPressTimer);
//FIXME
//    aDOMEvent->StopPropagation();
//    aDOMEvent->PreventDefault();
  } else {
    sLongPressTimer = g_timeout_add(mLongMPressDelay, sLongMPress, mOwner->mOwningWidget);
    ((nsIDOMMouseEvent*)mouseEvent)->GetScreenX(&sX);
    ((nsIDOMMouseEvent*)mouseEvent)->GetScreenY(&sY);        
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseUp(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;

  // handling event internally, first.
  HandleSelection(mouseEvent);

  // Return TRUE from your signal handler to mark the event as consumed.
  if (sLongPressTimer)
    g_source_remove (sLongPressTimer);
  sMPressed = PR_FALSE;
  mOwner->mOpenBlock = sIsScrolling;
  sIsScrolling = sMPressed;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_UP],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseClick(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  sMPressed = PR_FALSE;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_CLICK],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseDblClick(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  if (sLongPressTimer)
    g_source_remove (sLongPressTimer);
  sMPressed = PR_FALSE;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_DBL_CLICK],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  } 
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseOver(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_OVER],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  } else {
    //mCtxInfo->UpdateContextData(aDOMEvent);
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseOut(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_OUT],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::Activate(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
  if (!uiEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_ACTIVATE],
                  (void *)uiEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::FocusIn(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
  if (!uiEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_FOCUS_IN],
                  (void *)uiEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::FocusOut(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
  if (!uiEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_FOCUS_OUT],
                  (void *)uiEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseMove(nsIDOMEvent* aDOMEvent)
{
  if (mSelCon)
    mSelCon->SetDisplaySelection (nsISelectionController::SELECTION_ON);

  if (sMPressed && 
      gtk_signal_handler_pending(GTK_OBJECT(mOwner->mOwningWidget),
                                 moz_embed_signals[DOM_MOUSE_SCROLL], TRUE)) {
    // Return TRUE from your signal handler to mark the event as consumed.
    nsCOMPtr <nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aDOMEvent);
    if (!mouseEvent)
      return NS_OK;
    PRInt32  newX, newY, subX, subY;
    ((nsIDOMMouseEvent*)mouseEvent)->GetScreenX(&newX);
    ((nsIDOMMouseEvent*)mouseEvent)->GetScreenY(&newY);
    subX = newX - sX;
    subY = newY - sY;
    nsresult rv = NS_OK;
    if (ABS(subX) > 10 || ABS(subY) > 10 || (sIsScrolling && sMPressed)) {
      if (!sIsScrolling) {
        gint return_val = FALSE;
        gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                        moz_embed_signals[DOM_MOUSE_SCROLL],
                        (void *)mouseEvent, &return_val);
        if (!return_val) {
          sIsScrolling = PR_TRUE;
          if (mCtxInfo)
            rv = mCtxInfo->GetElementForScroll(aDOMEvent);
        } else {
          sMPressed = PR_FALSE;
          sIsScrolling = PR_FALSE;
        }
      }
      if (sIsScrolling) 
      {
        if (sLongPressTimer)
          g_source_remove (sLongPressTimer);
        if (mCtxInfo->mNSHHTMLElementSc) {
          PRInt32 x, y;
          mCtxInfo->mNSHHTMLElementSc->GetScrollTop(&y);
          mCtxInfo->mNSHHTMLElementSc->GetScrollLeft(&x);
#ifdef MOZ_SCROLL_TOP_LEFT_HACK
          rv = mCtxInfo->mNSHHTMLElementSc->ScrollTopLeft (y - subY, x - subX);
#endif
        } else {
          rv = NS_ERROR_UNEXPECTED;
        }
        if (rv == NS_ERROR_UNEXPECTED) {
          nsCOMPtr<nsIDOMWindow> DOMWindow;
          nsIWebBrowser *webBrowser = nsnull;
          gtk_moz_embed_get_nsIWebBrowser(mOwner->mOwningWidget, &webBrowser);
          webBrowser->GetContentDOMWindow(getter_AddRefs(DOMWindow));
          DOMWindow->ScrollBy(-subX, -subY);
        }
      }
      sX = newX;
      sY = newY;
      sIsScrolling = sMPressed;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::DragMove(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP 
EmbedEventListener::Focus(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP 
EmbedEventListener::Blur(nsIDOMEvent* aEvent)
{
  nsresult rv;
  if (mSelCon)
    rv = mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);

  return NS_OK;
 }

NS_IMETHODIMP 
EmbedEventListener::HandleSelection(nsIDOMMouseEvent* aDOMMouseEvent)
{
  nsresult rv;
  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aDOMMouseEvent));

  nsCOMPtr<nsIDOMEventTarget> target;
  rv = nsevent->GetOriginalTarget(getter_AddRefs(target));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDOMNode> eventNode = do_QueryInterface(target);
  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = eventNode->GetOwnerDocument(getter_AddRefs(domDoc));  
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  nsIPresShell *presShell = doc->GetShellAt(0);

  mSelCon = do_QueryInterface(presShell);

  nsString eventType;
  rv = aDOMMouseEvent->GetType(eventType);
  if (NS_FAILED(rv))
    return rv;

  PRInt32 clickCount;
  rv = aDOMMouseEvent->GetDetail(&clickCount);
  if (NS_FAILED(rv))
    return rv;  

  mCtxInfo->UpdateContextData(aDOMMouseEvent);

  if (eventType.EqualsLiteral("mousedown")) {
    if (clickCount == 1) {
      if (!(mCtxInfo->mEmbedCtxType & GTK_MOZ_EMBED_CTX_XUL))
        rv = mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_OFF);
    }
    return rv;
  }

  if (eventType.EqualsLiteral("mouseup")) {
    rv = mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
    if (clickCount == 1) {
      nsCOMPtr<nsISelection> domSel;
      mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, 
                            getter_AddRefs(domSel));
      rv = domSel->RemoveAllRanges();
      return rv;
    }
  }

  return rv;
}

nsresult
EmbedEventListener::GetLinkAttribute (nsCOMPtr<nsIDOMElement>& linkElement,
                                      const char *name,
                                      nsString *value)
{
  nsString n_name;
  nsCString c_name(name);
  NS_CStringToUTF16(c_name, NS_CSTRING_ENCODING_UTF8, n_name);

  return linkElement->GetAttribute(n_name, *value);
}

nsresult
EmbedEventListener::NewURI (nsIURI **result,
                            const char *spec)
{
  nsresult rv;
  nsCString cSpec(spec);
  nsCOMPtr<nsIIOService> ioService;
  rv = GetIOService (getter_AddRefs (ioService));
  if (NS_FAILED(rv))
    return rv;

  rv = ioService->NewURI (cSpec, nsnull, nsnull, result);
  return rv;
}

nsresult
EmbedEventListener::GetIOService(nsIIOService **ioService)
{
  nsresult rv;

  nsCOMPtr<nsIServiceManager> mgr;
  NS_GetServiceManager (getter_AddRefs (mgr));
  if (!mgr) return NS_ERROR_FAILURE;

  rv = mgr->GetServiceByContractID ("@mozilla.org/network/io-service;1",
                                    NS_GET_IID (nsIIOService),
                                    (void **)ioService);
  return rv;
}


void
EmbedEventListener::GeneratePixBuf()
{
  GdkPixbuf *pixbuf = NULL;
  pixbuf = gdk_pixbuf_new_from_file(::gFavLocation, NULL);
  if(!pixbuf) {
    gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget), 
                    moz_embed_signals[ICON_CHANGED],
                    NULL );

    // remove the wrong favicon
    // FIXME: need better impl...
    nsCOMPtr<nsILocalFile> faviconFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);

    if (!faviconFile) {
      NS_Free(::gFavLocation);
      return;
    }

    nsCString faviconLocation(::gFavLocation);
    faviconFile->InitWithNativePath(faviconLocation);
    faviconFile->Remove(FALSE);
    NS_Free(::gFavLocation);
    return;
  }
  
  // now send the signal to eal then eal send another signal to UI
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget), 
                  moz_embed_signals[ICON_CHANGED],
                  pixbuf );
  //mOwner->mNeedFav = PR_FALSE;
  NS_Free(::gFavLocation);
}

void
EmbedEventListener::GetFaviconFromURI(const char* aURI)
{
  gchar *file_name = NS_strdup(aURI);
  gchar *favicon_uri = NS_strdup(aURI);

  gint i = 0;
  gint rv = 0;

  nsCOMPtr<nsIWebBrowserPersist> persist = do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID);
  if (!persist) {
    NS_Free(file_name);
    NS_Free(favicon_uri);
    return;
  }
  persist->SetProgressListener(this);

  while (file_name[i] != '\0') {
    if (file_name[i] == '/' || file_name[i] == ':')
      file_name[i] = '_';
    i++;
  }

  nsCString fileName(file_name);

  nsCOMPtr<nsILocalFile> favicon_dir = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);

  if (!favicon_dir) {
    NS_Free(favicon_uri);
    NS_Free(file_name);
    return;
  }

  nsCString faviconDir("~/.mozilla/favicon");
  favicon_dir->InitWithNativePath(faviconDir);

  PRBool isExist;
  rv = favicon_dir->Exists(&isExist);
  if (NS_SUCCEEDED(rv) && !isExist) {
    rv = favicon_dir->Create(nsIFile::DIRECTORY_TYPE,0775);
    if (NS_FAILED(rv)) {
      NS_Free(file_name);
      NS_Free(favicon_uri);
      return;
    }
  }

  nsCAutoString favicon_path("~/.mozilla/favicon");
  nsCOMPtr<nsILocalFile> target_file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
  if (!target_file) {
    NS_Free(file_name);
    NS_Free(favicon_uri);
    return;
  }
  target_file->InitWithNativePath(favicon_path);
  target_file->Append(NS_ConvertUTF8toUTF16(fileName));

  nsString path;
  target_file->GetPath(path);
  ::gFavLocation = NS_strdup(NS_ConvertUTF16toUTF8(path).get());
  nsCOMPtr<nsIIOService> ios (do_GetService(NS_IOSERVICE_CONTRACTID));
  if (!ios) {
    NS_Free(file_name);
    NS_Free(favicon_uri);
    NS_Free(::gFavLocation);
    return;
  }

  nsCOMPtr<nsIURI> uri;

  rv = ios->NewURI(nsDependentCString(favicon_uri), "", nsnull, getter_AddRefs(uri));
  if (!uri) {
    NS_Free(file_name);
    NS_Free(favicon_uri);
    NS_Free(::gFavLocation);
    return;
  }
  NS_Free(file_name);
  NS_Free(favicon_uri);

  // save the favicon if the icon does not exist
  rv = target_file->Exists(&isExist);
  if (NS_SUCCEEDED(rv) && !isExist) {
    rv = persist->SaveURI(uri, nsnull, nsnull, nsnull, "", target_file);
    if (NS_FAILED(rv)) {
      return;
    }
  }
  else {
    GeneratePixBuf();
  }

}

NS_IMETHODIMP
EmbedEventListener::OnStateChange(nsIWebProgress *aWebProgress, 
                                  nsIRequest *aRequest, 
                                  PRUint32 aStateFlags, 
                                  nsresult aStatus)
{
  /* if (!(aStateFlags & (STATE_STOP | STATE_IS_NETWORK | STATE_IS_DOCUMENT))){*/
  if(aStateFlags & STATE_STOP) 
  {
    /* FINISH DOWNLOADING */
    if (NS_SUCCEEDED(aStatus)) {
      GeneratePixBuf();
      return NS_OK;
    }
  }
  else {
  }

  return NS_OK;

}

NS_IMETHODIMP
EmbedEventListener::OnProgressChange(nsIWebProgress *aWebProgress, 
                                     nsIRequest *aRequest, 
                                     PRInt32 aCurSelfProgress, 
                                     PRInt32 aMaxSelfProgress, 
                                     PRInt32 aCurTotalProgress, 
                                     PRInt32 aMaxTotalProgress)
{
  return NS_OK;
}

NS_IMETHODIMP 
EmbedEventListener::OnLocationChange(nsIWebProgress *aWebProgress, 
                                     nsIRequest *aRequest, 
                                     nsIURI *aLocation)
{
  return NS_OK;
}


NS_IMETHODIMP 
EmbedEventListener::OnStatusChange(nsIWebProgress *aWebProgress, 
                                   nsIRequest *aRequest, 
                                   nsresult aStatus, 
                                   const PRUnichar *aMessage)
{
  return NS_OK;
}


NS_IMETHODIMP 
EmbedEventListener::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                     nsIRequest *aRequest, 
                                     PRUint32 aState)
{
  return NS_OK;
}
