/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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


#ifndef nsPIDOMWindow_h__
#define nsPIDOMWindow_h__

#include "nsISupports.h"
#include "nsIDOMLocation.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindowInternal.h"
#include "nsIChromeEventHandler.h"
#include "nsIDOMDocument.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"

// Popup control state enum. The values in this enum must go from most
// permissive to least permissive so that it's safe to push state in
// all situations. Pushing popup state onto the stack never makes the
// current popup state less permissive (see
// nsGlobalWindow::PushPopupControlState()).
enum PopupControlState {
  openAllowed = 0,  // open that window without worries
  openControlled,   // it's a popup, but allow it
  openAbused,       // it's a popup. disallow it, but allow domain override.
  openOverridden    // disallow window open
};

// permissible values for GetOpenAllow
enum OpenAllowValue {
  allowNot = 0,     // the window opening is denied
  allowNoAbuse,     // allowed: not a popup
  allowSelf,        // allowed: it's the same window (_self, _top, et.al.)
  allowExtant,      // allowed: an already open window
  allowWhitelisted  // allowed: it's whitelisted or popup blocking is disabled
};

class nsIDocShell;
class nsIFocusController;
struct nsTimeout;

#define NS_PIDOMWINDOW_IID \
{ 0x91671056, 0xa8ed, 0x4ad3, \
 { 0x8f, 0x19, 0x69, 0xf5, 0x48, 0xbb, 0x9c, 0x4a } }

class nsPIDOMWindow : public nsIDOMWindowInternal
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_PIDOMWINDOW_IID)

  virtual nsPIDOMWindow* GetPrivateRoot() = 0;

  virtual nsresult GetObjectProperty(const PRUnichar* aProperty,
                                     nsISupports** aObject) = 0;

  // This is private because activate/deactivate events are not part
  // of the DOM spec.
  virtual nsresult Activate() = 0;
  virtual nsresult Deactivate() = 0;

  nsIChromeEventHandler* GetChromeEventHandler()
  {  
    return mChromeEventHandler;
  }

  PRBool HasMutationListeners(PRUint32 aMutationEventType)
  {
    return (mMutationBits & aMutationEventType) != 0;
  }

  void SetMutationListeners(PRUint32 aType) { mMutationBits |= aType; }

  virtual nsIFocusController* GetRootFocusController() = 0;

  // GetExtantDocument provides a backdoor to the DOM GetDocument accessor
  nsIDOMDocument* GetExtantDocument() { return mDocument; }

  // Internal getter/setter for the frame element, this version of the
  // getter crosses chrome boundaries whereas the public scriptable
  // one doesn't for security reasons.
  nsIDOMElement* GetFrameElementInternal() { return mFrameElement; }
  void SetFrameElementInternal(nsIDOMElement *aFrameElement)
  {
    mFrameElement = aFrameElement;
  }

  PRBool IsLoadingOrRunningTimeout() const
  {
    return !mIsDocumentLoaded || mRunningTimeout;
  }

  PRBool IsHandlingResizeEvent() const
  {
    return mIsHandlingResizeEvent;
  }

  virtual void SetOpenerScriptURL(nsIURI* aURI) = 0;

  virtual PopupControlState PushPopupControlState(PopupControlState aState) const = 0;
  virtual void PopPopupControlState(PopupControlState state) const = 0;
  virtual PopupControlState GetPopupControlState() const = 0;
  virtual OpenAllowValue GetOpenAllow(const nsAString &aName) = 0;

  // Clear all pending timeouts and intervals.
  virtual void ClearAllTimeouts() = 0;
  
protected:
  nsPIDOMWindow()
    : mRunningTimeout(nsnull), mMutationBits(0), mIsDocumentLoaded(PR_FALSE),
      mIsHandlingResizeEvent(PR_FALSE)
  {
  }

  nsCOMPtr<nsIChromeEventHandler> mChromeEventHandler; // strong
  nsCOMPtr<nsIDOMDocument> mDocument; // strong
  nsIDOMElement *mFrameElement; // weak
  nsCOMPtr<nsIURI> mOpenerScriptURL; // strong; used to determine whether to clear scope
  nsTimeout             *mRunningTimeout;

  PRUint32               mMutationBits;

  PRPackedBool           mIsDocumentLoaded;
  PRPackedBool           mIsHandlingResizeEvent;
};


#ifdef _IMPL_NS_LAYOUT
PopupControlState
PushPopupControlState(PopupControlState aState, PRBool aForce);

void
PopPopupControlState(PopupControlState aState);

#define NS_AUTO_POPUP_STATE_PUSHER nsAutoPopupStatePusherInternal
#else
#define NS_AUTO_POPUP_STATE_PUSHER nsAutoPopupStatePusherExternal
#endif

// Helper class that helps with pushing and popping popup control
// state. Note that this class looks different from within code that's
// part of the layout library than it does in code outside the layout
// library.  We give the two object layouts different names so the symbols
// don't conflict, but code should always use the name
// |nsAutoPopupStatePusher|.
class NS_AUTO_POPUP_STATE_PUSHER
{
public:
#ifdef _IMPL_NS_LAYOUT
  NS_AUTO_POPUP_STATE_PUSHER(PopupControlState aState, PRBool aForce = PR_FALSE)
    : mOldState(::PushPopupControlState(aState, aForce))
  {
  }

  ~NS_AUTO_POPUP_STATE_PUSHER()
  {
    PopPopupControlState(mOldState);
  }
#else
  NS_AUTO_POPUP_STATE_PUSHER(nsPIDOMWindow *aWindow, PopupControlState aState)
    : mWindow(aWindow), mOldState(openAbused)
  {
    if (aWindow) {
      mOldState = aWindow->PushPopupControlState(aState);
    }
  }

  ~NS_AUTO_POPUP_STATE_PUSHER()
  {
    if (mWindow) {
      mWindow->PopPopupControlState(mOldState);
    }
  }
#endif

protected:
#ifndef _IMPL_NS_LAYOUT
  nsCOMPtr<nsPIDOMWindow> mWindow;
#endif
  PopupControlState mOldState;

private:
  // Hide so that this class can only be stack-allocated
  static void* operator new(size_t /*size*/) CPP_THROW_NEW { return nsnull; }
  static void operator delete(void* /*memory*/) {}
};

#define nsAutoPopupStatePusher NS_AUTO_POPUP_STATE_PUSHER

#endif // nsPIDOMWindow_h__
