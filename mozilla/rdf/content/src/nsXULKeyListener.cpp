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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIContent.h"
#include "nsIPrincipal.h"
#include "nsIJSEventListener.h"
#include "nsIDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMXULDocument.h"
#include "nsINSEvent.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsIXULKeyListener.h"
#include "nsIXULDocument.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIXULPrototypeDocument.h"
#include "nsIScriptObjectOwner.h"
#include "nsIXULContentSink.h"
#include "nsRDFCID.h"
#include "nsINameSpaceManager.h"
#include "nsHashtable.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsXPIDLString.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsNetUtil.h"
#include "plstr.h"
#include "nsIWebShell.h"
#include "nsIDocShell.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIPresContext.h"

  enum {
    VK_CANCEL = 3,
    VK_BACK = 8,
    VK_TAB = 9,
    VK_CLEAR = 12,
    VK_RETURN = 13,
    VK_ENTER = 14,
    VK_SHIFT = 16,
    VK_CONTROL = 17,
    VK_ALT = 18,
    VK_PAUSE = 19,
    VK_CAPS_LOCK = 20,
    VK_ESCAPE = 27,
    VK_SPACE = 32,
    VK_PAGE_UP = 33,
    VK_PAGE_DOWN = 34,
    VK_END = 35,
    VK_HOME = 36,
    VK_LEFT = 37,
    VK_UP = 38,
    VK_RIGHT = 39,
    VK_DOWN = 40,
    VK_PRINTSCREEN = 44,
    VK_INSERT = 45,
    VK_DELETE = 46,
    VK_0 = 48,
    VK_1 = 49,
    VK_2 = 50,
    VK_3 = 51,
    VK_4 = 52,
    VK_5 = 53,
    VK_6 = 54,
    VK_7 = 55,
    VK_8 = 56,
    VK_9 = 57,
    VK_SEMICOLON = 59,
    VK_EQUALS = 61,
    VK_A = 65,
    VK_B = 66,
    VK_C = 67,
    VK_D = 68,
    VK_E = 69,
    VK_F = 70,
    VK_G = 71,
    VK_H = 72,
    VK_I = 73,
    VK_J = 74,
    VK_K = 75,
    VK_L = 76,
    VK_M = 77,
    VK_N = 78,
    VK_O = 79,
    VK_P = 80,
    VK_Q = 81,
    VK_R = 82,
    VK_S = 83,
    VK_T = 84,
    VK_U = 85,
    VK_V = 86,
    VK_W = 87,
    VK_X = 88,
    VK_Y = 89,
    VK_Z = 90,
    VK_NUMPAD0 = 96,
    VK_NUMPAD1 = 97,
    VK_NUMPAD2 = 98,
    VK_NUMPAD3 = 99,
    VK_NUMPAD4 = 100,
    VK_NUMPAD5 = 101,
    VK_NUMPAD6 = 102,
    VK_NUMPAD7 = 103,
    VK_NUMPAD8 = 104,
    VK_NUMPAD9 = 105,
    VK_MULTIPLY = 106,
    VK_ADD = 107,
    VK_SEPARATOR = 108,
    VK_SUBTRACT = 109,
    VK_DECIMAL = 110,
    VK_DIVIDE = 111,
    VK_F1 = 112,
    VK_F2 = 113,
    VK_F3 = 114,
    VK_F4 = 115,
    VK_F5 = 116,
    VK_F6 = 117,
    VK_F7 = 118,
    VK_F8 = 119,
    VK_F9 = 120,
    VK_F10 = 121,
    VK_F11 = 122,
    VK_F12 = 123,
    VK_F13 = 124,
    VK_F14 = 125,
    VK_F15 = 126,
    VK_F16 = 127,
    VK_F17 = 128,
    VK_F18 = 129,
    VK_F19 = 130,
    VK_F20 = 131,
    VK_F21 = 132,
    VK_F22 = 133,
    VK_F23 = 134,
    VK_F24 = 135,
    VK_NUM_LOCK = 144,
    VK_SCROLL_LOCK = 145,
    VK_COMMA = 188,
    VK_PERIOD = 190,
    VK_SLASH = 191,
    VK_BACK_QUOTE = 192,
    VK_OPEN_BRACKET = 219,
    VK_BACK_SLASH = 220,
    VK_CLOSE_BRACKET = 221,
    VK_QUOTE = 222
  };

////////////////////////////////////////////////////////////////////////

static NS_DEFINE_CID(kXULKeyListenerCID,      NS_XULKEYLISTENER_CID);
static NS_DEFINE_CID(kXULDocumentCID, NS_XULDOCUMENT_CID);
static NS_DEFINE_CID(kXULContentSinkCID, NS_XULCONTENTSINK_CID);
static NS_DEFINE_CID(kParserCID,                 NS_PARSER_IID); // XXX

static NS_DEFINE_IID(kIXULKeyListenerIID,     NS_IXULKEYLISTENER_IID);
static NS_DEFINE_IID(kISupportsIID,           NS_ISUPPORTS_IID);

static NS_DEFINE_IID(kIDomNodeIID,            NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDomElementIID,         NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIDomEventListenerIID,   NS_IDOMEVENTLISTENER_IID);

enum eEventType {
  eKeyPress,
  eKeyDown,
  eKeyUp
};

////////////////////////////////////////////////////////////////////////
// KeyListenerImpl
//
//   This is the key listener implementation for keybinding
//
class nsXULKeyListenerImpl : public nsIXULKeyListener,
                           public nsIDOMKeyListener
{
public:
    nsXULKeyListenerImpl(void);
    virtual ~nsXULKeyListenerImpl(void);

public:
    // nsISupports
    NS_DECL_ISUPPORTS

    // nsIXULKeyListener
    NS_IMETHOD Init(
      nsIDOMElement  * aElement,
      nsIDOMDocument * aDocument);

    // nsIDOMKeyListener

    /**
     * Processes a key pressed event
     * @param aKeyEvent @see nsIDOMEvent.h
     * @returns whether the event was consumed or ignored. @see nsresult
     */
    virtual nsresult KeyDown(nsIDOMEvent* aKeyEvent);

    /**
     * Processes a key release event
     * @param aKeyEvent @see nsIDOMEvent.h
     * @returns whether the event was consumed or ignored. @see nsresult
     */
    virtual nsresult KeyUp(nsIDOMEvent* aKeyEvent);

    /**
     * Processes a key typed event
     * @param aKeyEvent @see nsIDOMEvent.h
     * @returns whether the event was consumed or ignored. @see nsresult
     *
     */
    virtual nsresult KeyPress(nsIDOMEvent* aKeyEvent);

    // nsIDOMEventListener
    virtual nsresult HandleEvent(nsIDOMEvent* anEvent) { return NS_OK; };

protected:
    class nsIURIKey : public nsHashKey {
    protected:
        nsCOMPtr<nsIURI> mKey;

    public:
        nsIURIKey(nsIURI* key) : mKey(key) {}
        ~nsIURIKey(void) {}

        PRUint32 HashValue(void) const {
            nsXPIDLCString spec;
            mKey->GetSpec(getter_Copies(spec));
            return (PRUint32) PL_HashString(spec);
        }

        PRBool Equals(const nsHashKey *aKey) const {
            PRBool eq;
            mKey->Equals( ((nsIURIKey*) aKey)->mKey, &eq );
            return eq;
        }

        nsHashKey *Clone(void) const {
            return new nsIURIKey(mKey);
        }
    };

private:
    nsresult DoKey(nsIDOMEvent* aKeyEvent, eEventType aEventType);
    inline PRBool   IsMatchingKeyCode(const PRUint32 theChar, const nsString &keyName);
    inline PRBool   IsMatchingCharCode(const nsString &theChar, const nsString &keyName);

    NS_IMETHOD GetKeyBindingDocument(nsCAutoString& aURLStr, nsIDOMXULDocument** aResult);
    NS_IMETHOD LoadKeyBindingDocument(nsIURI* aURI, nsIDOMXULDocument** aResult);
    NS_IMETHOD LocateAndExecuteKeyBinding(nsIDOMKeyEvent* aKeyEvent, eEventType aEventType,
                                nsIDOMXULDocument* aDocument, PRBool& aHandled);
    NS_IMETHOD HandleEventUsingKeyset(nsIDOMElement* aKeysetElement, nsIDOMKeyEvent* aEvent, eEventType aEventType,
                                      nsIDOMXULDocument* aDocument, PRBool& aHandledFlag);

    nsIDOMElement* element; // Weak reference. The element will go away first.
    nsIDOMXULDocument* mDOMDocument; // Weak reference.

    static PRUint32 gRefCnt;
    static nsSupportsHashtable* mKeyBindingTable;

    // The "xul key" modifier can be any of the known modifiers:
    enum {
        xulKeyNone, xulKeyShift, xulKeyControl, xulKeyAlt, xulKeyMeta
    } mXULKeyModifier;
};

PRUint32 nsXULKeyListenerImpl::gRefCnt = 0;
nsSupportsHashtable* nsXULKeyListenerImpl::mKeyBindingTable = nsnull;

class nsProxyStream : public nsIInputStream
{
private:
  const char* mBuffer;
  PRUint32    mSize;
  PRUint32    mIndex;

public:
  nsProxyStream(void) : mBuffer(nsnull)
  {
      NS_INIT_REFCNT();
  }

  virtual ~nsProxyStream(void) {
  }

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIBaseStream
  NS_IMETHOD Close(void) {
      return NS_OK;
  }

  // nsIInputStream
  NS_IMETHOD Available(PRUint32 *aLength) {
      *aLength = mSize - mIndex;
      return NS_OK;
  }

  NS_IMETHOD Read(char* aBuf, PRUint32 aCount, PRUint32 *aReadCount) {
      PRUint32 readCount = 0;
      while (mIndex < mSize && aCount > 0) {
          *aBuf = mBuffer[mIndex];
          aBuf++;
          mIndex++;
          readCount++;
          aCount--;
      }
      *aReadCount = readCount;
      return NS_OK;
  }

  // Implementation
  void SetBuffer(const char* aBuffer, PRUint32 aSize) {
      mBuffer = aBuffer;
      mSize = aSize;
      mIndex = 0;
  }
};

NS_IMPL_ISUPPORTS(nsProxyStream, NS_GET_IID(nsIInputStream));

////////////////////////////////////////////////////////////////////////


nsXULKeyListenerImpl::nsXULKeyListenerImpl(void)
{
  NS_INIT_REFCNT();
  gRefCnt++;
  if (gRefCnt == 1) {
    mKeyBindingTable = new nsSupportsHashtable();
  }
}

nsXULKeyListenerImpl::~nsXULKeyListenerImpl(void)
{
  gRefCnt--;
  if (gRefCnt == 0)
    delete mKeyBindingTable;
}

NS_IMPL_ADDREF(nsXULKeyListenerImpl)
NS_IMPL_RELEASE(nsXULKeyListenerImpl)

NS_IMETHODIMP
nsXULKeyListenerImpl::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nsnull;
    if (iid.Equals(NS_GET_IID(nsIXULKeyListener)) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIXULKeyListener*, this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMKeyListener))) {
        *result = NS_STATIC_CAST(nsIDOMKeyListener*, this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    else if (iid.Equals(kIDomEventListenerIID)) {
        *result = (nsIDOMEventListener*)(nsIDOMMouseListener*)this;
        NS_ADDREF_THIS();
        return NS_OK;
    }

    return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsXULKeyListenerImpl::Init(
  nsIDOMElement  * aElement,
  nsIDOMDocument * aDocument)
{
  element = aElement; // Weak reference. Don't addref it.

  nsCOMPtr<nsIDOMXULDocument> xulDoc = do_QueryInterface(aDocument);
  if (!xulDoc)
    return NS_ERROR_FAILURE;

  mDOMDocument = xulDoc; // Weak reference.

  // Set the default for the xul key modifier
#ifdef XP_MAC
  mXULKeyModifier = xulKeyMeta;
#elif XP_UNIX
  mXULKeyModifier = xulKeyAlt;
#else
  mXULKeyModifier = xulKeyControl;
#endif
  return NS_OK;
}

////////////////////////////////////////////////////////////////
// nsIDOMKeyListener

/**
 * Processes a key down event
 * @param aKeyEvent @see nsIDOMEvent.h
 * @returns whether the event was consumed or ignored. @see nsresult
 */
nsresult nsXULKeyListenerImpl::KeyDown(nsIDOMEvent* aKeyEvent)
{
  return DoKey(aKeyEvent, eKeyDown);
}

////////////////////////////////////////////////////////////////
/**
 * Processes a key release event
 * @param aKeyEvent @see nsIDOMEvent.h
 * @returns whether the event was consumed or ignored. @see nsresult
 */
nsresult nsXULKeyListenerImpl::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return DoKey(aKeyEvent, eKeyUp);
}

////////////////////////////////////////////////////////////////
/**
 * Processes a key typed event
 * @param aKeyEvent @see nsIDOMEvent.h
 * @returns whether the event was consumed or ignored. @see nsresult
 *
 */
 //  // Get the main document
 //  // find the keyset
 // iterate over key(s) looking for appropriate handler
nsresult nsXULKeyListenerImpl::KeyPress(nsIDOMEvent* aKeyEvent)
{
  return DoKey(aKeyEvent, eKeyPress);
}

nsresult nsXULKeyListenerImpl::DoKey(nsIDOMEvent* aKeyEvent, eEventType aEventType)
{
  // Check the preventDefault flag.  We don't ever execute a XUL key binding
  // if this flag is set.
  nsCOMPtr<nsIDOMNSUIEvent> evt = do_QueryInterface(aKeyEvent);
  PRBool prevent;
  evt->GetPreventDefault(&prevent);
  if (prevent)
    return NS_OK;

  static PRBool executingKeyBind = PR_FALSE;
  nsresult ret = NS_OK;

  if (executingKeyBind)
    return NS_OK;

  executingKeyBind = PR_TRUE;

  if (!aKeyEvent) {
    executingKeyBind = PR_FALSE;
    return ret;
  }

  if (!mDOMDocument) {
    executingKeyBind = PR_FALSE;
    return ret;
  }

  // Get DOMEvent target
  nsCOMPtr<nsIDOMNode> target = nsnull;
  aKeyEvent->GetTarget(getter_AddRefs(target));

  nsCOMPtr<nsPIDOMWindow> piWindow;

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  // Find a keyset node
  // Get the current focused object from the command dispatcher
  nsCOMPtr<nsIDOMXULCommandDispatcher> commandDispatcher;
  mDOMDocument->GetCommandDispatcher(getter_AddRefs(commandDispatcher));
  nsCOMPtr<nsIDOMElement> focusedElement;
  commandDispatcher->GetFocusedElement(getter_AddRefs(focusedElement));

  nsCOMPtr<nsIDOMWindow> domWindow;
  commandDispatcher->GetFocusedWindow(getter_AddRefs(domWindow));
  piWindow = do_QueryInterface(domWindow);

  nsCAutoString keyFile, platformKeyFile;
 
  nsCOMPtr<nsIDOMXULDocument> document;
  GetKeyBindingDocument(platformKeyFile, getter_AddRefs(document));

  // Locate the key node and execute the JS on a match.
  PRBool handled = PR_FALSE;
  if (document) // Local focused ELEMENT handling stage.
    LocateAndExecuteKeyBinding(keyEvent, aEventType, document, handled);

  if (!handled) {
    GetKeyBindingDocument(keyFile, getter_AddRefs(document));
    if (document) // Local focused ELEMENT handling stage.
      LocateAndExecuteKeyBinding(keyEvent, aEventType, document, handled);
  }

  nsCAutoString browserFile = "chrome://global/content/browserBindings.xul";
  nsCAutoString editorFile = "chrome://global/content/editorBindings.xul";
  nsCAutoString browserPlatformFile = "chrome://global/content/platformBrowserBindings.xul";
  nsCAutoString editorPlatformFile = "chrome://global/content/platformEditorBindings.xul";

  nsresult result;

  if (!handled) {
    while (piWindow && !handled) {
      // See if we have a XUL document. Give it a crack.
      nsCOMPtr<nsIDOMWindow> domWindow = do_QueryInterface(piWindow);
      nsCOMPtr<nsIDOMDocument> windowDoc;
      domWindow->GetDocument(getter_AddRefs(windowDoc));
      nsCOMPtr<nsIDOMXULDocument> xulWindowDoc = do_QueryInterface(windowDoc);
      if (xulWindowDoc) {
        // Give the local key bindings in this XUL file a shot.
        LocateAndExecuteKeyBinding(keyEvent, aEventType, xulWindowDoc, handled);
      }

      if (!handled) {
        // Give the DOM window's associated key binding doc a shot.
        // XXX Check to see if we're in edit mode (how??!)
        nsCOMPtr<nsIDocShell> docShell;
        result = piWindow->GetDocShell(getter_AddRefs(docShell));

        nsCOMPtr<nsIPresShell> presShell;
        if(docShell)
         docShell->GetPresShell(getter_AddRefs(presShell));

        PRBool editorHasBindings = PR_FALSE;
        nsCOMPtr<nsIDOMXULDocument> platformDoc;
        if (presShell) {
          PRBool isEditor;
          if (NS_SUCCEEDED(presShell->GetDisplayNonTextSelection(&isEditor)) && isEditor) {
            editorHasBindings = PR_TRUE;
            GetKeyBindingDocument(editorPlatformFile, getter_AddRefs(platformDoc));
            GetKeyBindingDocument(editorFile, getter_AddRefs(document));
          }
        }
        if (!editorHasBindings) {
          GetKeyBindingDocument(browserPlatformFile, getter_AddRefs(platformDoc));
          GetKeyBindingDocument(browserFile, getter_AddRefs(document));
        }

        if (platformDoc)
          LocateAndExecuteKeyBinding(keyEvent, aEventType, platformDoc, handled);

        if (!handled && document)
          LocateAndExecuteKeyBinding(keyEvent, aEventType, document, handled);
      }

      // Move up to the parent DOM window. Need to use the private API
      // to cross sandboxes
      nsCOMPtr<nsPIDOMWindow> piParent;
      piWindow->GetPrivateParent(getter_AddRefs(piParent));
      piWindow = piParent;
    }
  }

executingKeyBind = PR_FALSE;
  return ret;
}

PRBool nsXULKeyListenerImpl::IsMatchingKeyCode(const PRUint32 theChar, const nsString &keyName)
{
  PRBool ret = PR_FALSE;

  //printf("theChar = %d \n", theChar);
  //printf("keyName = %s \n", keyName.ToNewCString());
  //printf("\n");

  switch ( theChar ) {
    case VK_CANCEL:
      if(keyName == "VK_CANCEL")
        ret = PR_TRUE;
        break;
    case VK_BACK:
      if(keyName == "VK_BACK")
        ret = PR_TRUE;
        break;
    case VK_TAB:
      if(keyName == "VK_TAB")
        ret = PR_TRUE;
        break;
    case VK_CLEAR:
      if(keyName == "VK_CLEAR")
        ret = PR_TRUE;
        break;
    case VK_RETURN:
      if(keyName == "VK_RETURN")
        ret = PR_TRUE;
        break;
    case VK_ENTER:
      if(keyName == "VK_ENTER")
        ret = PR_TRUE;
        break;
    case VK_SHIFT:
      if(keyName == "VK_SHIFT")
        ret = PR_TRUE;
        break;
    case VK_CONTROL:
      if(keyName == "VK_CONTROL")
        ret = PR_TRUE;
        break;
    case VK_ALT:
      if(keyName == "VK_ALT")
        ret = PR_TRUE;
        break;
    case VK_PAUSE:
      if(keyName == "VK_PAUSE")
        ret = PR_TRUE;
        break;
    case VK_CAPS_LOCK:
      if(keyName == "VK_CAPS_LOCK")
        ret = PR_TRUE;
        break;
    case VK_ESCAPE:
      if(keyName == "VK_ESCAPE")
        ret = PR_TRUE;
        break;
    case VK_SPACE:
      if(keyName == "VK_SPACE")
        ret = PR_TRUE;
        break;
    case VK_PAGE_UP:
      if(keyName == "VK_PAGE_UP")
        ret = PR_TRUE;
        break;
    case VK_PAGE_DOWN:
      if(keyName == "VK_PAGE_DOWN")
        ret = PR_TRUE;
        break;
    case VK_END:
      if(keyName == "VK_END")
        ret = PR_TRUE;
        break;
    case VK_HOME:
      if(keyName == "VK_HOME")
        ret = PR_TRUE;
        break;
    case VK_LEFT:
      if(keyName == "VK_LEFT")
        ret = PR_TRUE;
        break;
    case VK_UP:
      if(keyName == "VK_UP")
        ret = PR_TRUE;
        break;
    case VK_RIGHT:
      if(keyName == "VK_RIGHT")
        ret = PR_TRUE;
        break;
    case VK_DOWN:
      if(keyName == "VK_DOWN")
        ret = PR_TRUE;
        break;
    case VK_PRINTSCREEN:
      if(keyName == "VK_PRINTSCREEN")
        ret = PR_TRUE;
        break;
    case VK_INSERT:
      if(keyName == "VK_INSERT")
        ret = PR_TRUE;
        break;
    case VK_DELETE:
      if(keyName == "VK_DELETE")
        ret = PR_TRUE;
        break;
    case VK_0:
      if(keyName == "VK_0")
        ret = PR_TRUE;
        break;
    case VK_1:
      if(keyName == "VK_1")
        ret = PR_TRUE;
        break;
    case VK_2:
      if(keyName == "VK_2")
        ret = PR_TRUE;
        break;
    case VK_3:
      if(keyName == "VK_3")
        ret = PR_TRUE;
        break;
    case VK_4:
      if(keyName == "VK_4")
        ret = PR_TRUE;
        break;
    case VK_5:
      if(keyName == "VK_5")
        ret = PR_TRUE;
        break;
    case VK_6:
      if(keyName == "VK_6")
        ret = PR_TRUE;
        break;
    case VK_7:
      if(keyName == "VK_7")
        ret = PR_TRUE;
        break;
    case VK_8:
      if(keyName == "VK_8")
        ret = PR_TRUE;
        break;
    case VK_9:
      if(keyName == "VK_9")
        ret = PR_TRUE;
        break;
    case VK_SEMICOLON:
      if(keyName == "VK_SEMICOLON")
        ret = PR_TRUE;
        break;
    case VK_EQUALS:
      if(keyName == "VK_EQUALS")
        ret = PR_TRUE;
        break;
    case VK_A:
      if(keyName == "VK_A"  || keyName == "A" || keyName == "a")
        ret = PR_TRUE;
        break;
    case VK_B:
      if(keyName == "VK_B" || keyName == "B" || keyName == "b")
        ret = PR_TRUE;
    break;
    case VK_C:
      if(keyName == "VK_C"  || keyName == "C" || keyName == "c")
        ret = PR_TRUE;
        break;
    case VK_D:
      if(keyName == "VK_D"  || keyName == "D" || keyName == "d")
        ret = PR_TRUE;
        break;
    case VK_E:
      if(keyName == "VK_E"  || keyName == "E" || keyName == "e")
        ret = PR_TRUE;
        break;
    case VK_F:
      if(keyName == "VK_F"  || keyName == "F" || keyName == "f")
        ret = PR_TRUE;
        break;
    case VK_G:
      if(keyName == "VK_G"  || keyName == "G" || keyName == "g")
        ret = PR_TRUE;
        break;
    case VK_H:
      if(keyName == "VK_H"  || keyName == "H" || keyName == "h")
        ret = PR_TRUE;
        break;
    case VK_I:
      if(keyName == "VK_I"  || keyName == "I" || keyName == "i")
        ret = PR_TRUE;
        break;
    case VK_J:
      if(keyName == "VK_J"  || keyName == "J" || keyName == "j")
        ret = PR_TRUE;
        break;
    case VK_K:
      if(keyName == "VK_K"  || keyName == "K" || keyName == "k")
        ret = PR_TRUE;
        break;
    case VK_L:
      if(keyName == "VK_L"  || keyName == "L" || keyName == "l")
        ret = PR_TRUE;
        break;
    case VK_M:
      if(keyName == "VK_M"  || keyName == "M" || keyName == "m")
        ret = PR_TRUE;
        break;
    case VK_N:
      if(keyName == "VK_N"  || keyName == "N" || keyName == "n")
        ret = PR_TRUE;
        break;
    case VK_O:
      if(keyName == "VK_O"  || keyName == "O" || keyName == "o")
        ret = PR_TRUE;
        break;
    case VK_P:
      if(keyName == "VK_P"  || keyName == "P" || keyName == "p")
        ret = PR_TRUE;
        break;
    case VK_Q:
      if(keyName == "VK_Q"  || keyName == "Q" || keyName == "q")
        ret = PR_TRUE;
        break;
    case VK_R:
      if(keyName == "VK_R"  || keyName == "R" || keyName == "r")
        ret = PR_TRUE;
        break;
    case VK_S:
      if(keyName == "VK_S"  || keyName == "S" || keyName == "s")
        ret = PR_TRUE;
        break;
    case VK_T:
      if(keyName == "VK_T"  || keyName == "T" || keyName == "t")
        ret = PR_TRUE;
        break;
    case VK_U:
      if(keyName == "VK_U"  || keyName == "U" || keyName == "u")
        ret = PR_TRUE;
        break;
    case VK_V:
      if(keyName == "VK_V"  || keyName == "V" || keyName == "v")
        ret = PR_TRUE;
        break;
    case VK_W:
      if(keyName == "VK_W"  || keyName == "W" || keyName == "w")
        ret = PR_TRUE;
        break;
    case VK_X:
      if(keyName == "VK_X"  || keyName == "X" || keyName == "x")
        ret = PR_TRUE;
        break;
    case VK_Y:
      if(keyName == "VK_Y"  || keyName == "Y" || keyName == "y")
        ret = PR_TRUE;
        break;
    case VK_Z:
      if(keyName == "VK_Z"  || keyName == "Z" || keyName == "z")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD0:
      if(keyName == "VK_NUMPAD0")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD1:
      if(keyName == "VK_NUMPAD1")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD2:
      if(keyName == "VK_NUMPAD2")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD3:
      if(keyName == "VK_NUMPAD3")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD4:
      if(keyName == "VK_NUMPAD4")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD5:
      if(keyName == "VK_NUMPAD5")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD6:
      if(keyName == "VK_NUMPAD6")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD7:
      if(keyName == "VK_NUMPAD7")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD8:
      if(keyName == "VK_NUMPAD8")
        ret = PR_TRUE;
        break;
    case VK_NUMPAD9:
      if(keyName == "VK_NUMPAD9")
        ret = PR_TRUE;
        break;
    case VK_MULTIPLY:
      if(keyName == "VK_MULTIPLY")
        ret = PR_TRUE;
        break;
    case VK_ADD:
      if(keyName == "VK_ADD")
        ret = PR_TRUE;
        break;
    case VK_SEPARATOR:
      if(keyName == "VK_SEPARATOR")
        ret = PR_TRUE;
        break;
    case VK_SUBTRACT:
      if(keyName == "VK_SUBTRACT")
        ret = PR_TRUE;
        break;
    case VK_DECIMAL:
      if(keyName == "VK_DECIMAL")
        ret = PR_TRUE;
        break;
    case VK_DIVIDE:
      if(keyName == "VK_DIVIDE")
        ret = PR_TRUE;
        break;
    case VK_F1:
      if(keyName == "VK_F1")
        ret = PR_TRUE;
        break;
    case VK_F2:
      if(keyName == "VK_F2")
        ret = PR_TRUE;
        break;
    case VK_F3:
      if(keyName == "VK_F3")
        ret = PR_TRUE;
        break;
    case VK_F4:
      if(keyName == "VK_F4")
        ret = PR_TRUE;
        break;
    case VK_F5:
      if(keyName == "VK_F5")
        ret = PR_TRUE;
        break;
    case VK_F6:
      if(keyName == "VK_F6")
        ret = PR_TRUE;
        break;
    case VK_F7:
      if(keyName == "VK_F7")
        ret = PR_TRUE;
        break;
    case VK_F8:
      if(keyName == "VK_F8")
        ret = PR_TRUE;
        break;
    case VK_F9:
      if(keyName == "VK_F9")
        ret = PR_TRUE;
        break;
    case VK_F10:
      if(keyName == "VK_F10")
        ret = PR_TRUE;
        break;
    case VK_F11:
      if(keyName == "VK_F11")
        ret = PR_TRUE;
        break;
    case VK_F12:
      if(keyName == "VK_F12")
        ret = PR_TRUE;
        break;
    case VK_F13:
      if(keyName == "VK_F13")
        ret = PR_TRUE;
        break;
    case VK_F14:
      if(keyName == "VK_F14")
        ret = PR_TRUE;
        break;
    case VK_F15:
      if(keyName == "VK_F15")
        ret = PR_TRUE;
        break;
    case VK_F16:
      if(keyName == "VK_F16")
        ret = PR_TRUE;
        break;
    case VK_F17:
      if(keyName == "VK_F17")
        ret = PR_TRUE;
        break;
    case VK_F18:
      if(keyName == "VK_F18")
        ret = PR_TRUE;
        break;
    case VK_F19:
      if(keyName == "VK_F19")
        ret = PR_TRUE;
        break;
    case VK_F20:
      if(keyName == "VK_F20")
        ret = PR_TRUE;
        break;
    case VK_F21:
      if(keyName == "VK_F21")
        ret = PR_TRUE;
        break;
    case VK_F22:
      if(keyName == "VK_F22")
        ret = PR_TRUE;
        break;
    case VK_F23:
      if(keyName == "VK_F23")
        ret = PR_TRUE;
        break;
    case VK_F24:
      if(keyName == "VK_F24")
        ret = PR_TRUE;
        break;
    case VK_NUM_LOCK:
      if(keyName == "VK_NUM_LOCK")
        ret = PR_TRUE;
        break;
    case VK_SCROLL_LOCK:
      if(keyName == "VK_SCROLL_LOCK")
        ret = PR_TRUE;
        break;
    case VK_COMMA:
      if(keyName == "VK_COMMA")
        ret = PR_TRUE;
        break;
    case VK_PERIOD:
      if(keyName == "VK_PERIOD")
        ret = PR_TRUE;
        break;
    case VK_SLASH:
      if(keyName == "VK_SLASH")
        ret = PR_TRUE;
        break;
    case VK_BACK_QUOTE:
      if(keyName == "VK_BACK_QUOTE")
        ret = PR_TRUE;
        break;
    case VK_OPEN_BRACKET:
      if(keyName == "VK_OPEN_BRACKET")
        ret = PR_TRUE;
        break;
    case VK_BACK_SLASH:
      if(keyName == "VK_BACK_SLASH")
        ret = PR_TRUE;
        break;
    case VK_CLOSE_BRACKET:
      if(keyName == "VK_CLOSE_BRACKET")
        ret = PR_TRUE;
        break;
    case VK_QUOTE:
      if(keyName == "VK_QUOTE")
        ret = PR_TRUE;
        break;
  }

  return ret;
}

PRBool nsXULKeyListenerImpl::IsMatchingCharCode(const nsString &theChar, const nsString &keyName)
{
  PRBool ret = PR_FALSE;

  //printf("theChar = %s \n", theChar.ToNewCString());
  //printf("keyName = %s \n", keyName.ToNewCString());
  //printf("\n");
  if(theChar == keyName)
    ret = PR_TRUE;

  return ret;
}

NS_IMETHODIMP nsXULKeyListenerImpl::GetKeyBindingDocument(nsCAutoString& aURLStr, nsIDOMXULDocument** aResult)
{
  nsCOMPtr<nsIDOMXULDocument> document;
  if (aURLStr != nsCAutoString("")) {
    nsCOMPtr<nsIURL> uri;
    nsComponentManager::CreateInstance("component://netscape/network/standard-url",
                                          nsnull,
                                          NS_GET_IID(nsIURL),
                                          getter_AddRefs(uri));
    uri->SetSpec(aURLStr);

    // We've got a file.  Check our key binding file cache.
    nsIURIKey key(uri);
    document = dont_AddRef(NS_STATIC_CAST(nsIDOMXULDocument*, mKeyBindingTable->Get(&key)));

    if (!document) {
      LoadKeyBindingDocument(uri, getter_AddRefs(document));
      if (document) {
        // Put the key binding doc into our table.
        mKeyBindingTable->Put(&key, document);
      }
    }
  }

  *aResult = document;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP nsXULKeyListenerImpl::LoadKeyBindingDocument(nsIURI* aURI, nsIDOMXULDocument** aResult)
{
  *aResult = nsnull;

  // Create the XUL document
  nsCOMPtr<nsIDOMXULDocument> doc;
  nsresult rv = nsComponentManager::CreateInstance(kXULDocumentCID, nsnull,
                                                   NS_GET_IID(nsIDOMXULDocument),
                                                   getter_AddRefs(doc));

  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIXULDocument> xulDoc = do_QueryInterface(doc, &rv);
  if (NS_FAILED(rv)) return rv;

  xulDoc->SetIsKeybindingDocument(PR_TRUE);

  // Now we have to synchronously load the key binding file.
  // Create a XUL content sink, a parser, and kick off a load for
  // the overlay.

  nsCOMPtr<nsIChannel> channel;
  rv = NS_OpenURI(getter_AddRefs(channel), aURI, nsnull);
  if (NS_FAILED(rv)) return rv;

  // Create a new prototype document
  nsCOMPtr<nsIXULPrototypeDocument> proto;
  rv = NS_NewXULPrototypeDocument(nsnull, NS_GET_IID(nsIXULPrototypeDocument), getter_AddRefs(proto));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISupports> owner;
  rv = channel->GetOwner(getter_AddRefs(owner));
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIPrincipal> principal = do_QueryInterface(owner);
  proto->SetDocumentPrincipal(principal);

  // Set master and current prototype
  xulDoc->SetMasterPrototype(proto);
  xulDoc->SetCurrentPrototype(proto);


  rv = proto->SetURI(aURI);
  if (NS_FAILED(rv)) return rv;

  xulDoc->SetDocumentURL(aURI);
  xulDoc->PrepareStyleSheets(aURI);

  nsCOMPtr<nsIXULContentSink> sink;
  rv = nsComponentManager::CreateInstance(kXULContentSinkCID,
                                          nsnull,
                                          NS_GET_IID(nsIXULContentSink),
                                          getter_AddRefs(sink));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create XUL content sink");
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDocument> document = do_QueryInterface(doc);
  rv = sink->Init(document, proto);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to initialize content sink");
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIParser> parser;
  rv = nsComponentManager::CreateInstance(kParserCID,
                                          nsnull,
                                          NS_GET_IID(nsIParser),
                                          getter_AddRefs(parser));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create parser");
  if (NS_FAILED(rv)) return rv;

  parser->SetCommand("view");

  nsAutoString utf8("UTF-8");
  parser->SetDocumentCharset(utf8, kCharsetFromDocTypeDefault);
  parser->SetContentSink(sink); // grabs a reference to the parser

  // Now do a blocking synchronous parse of the file.
  nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(parser, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = parser->Parse(aURI);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIInputStream> in;
  PRUint32 sourceOffset = 0;
  rv = channel->OpenInputStream(0, -1, getter_AddRefs(in));

  // If we couldn't open the channel, then just return.
  if (NS_FAILED(rv)) return NS_OK;

  NS_ASSERTION(in != nsnull, "no input stream");
  if (! in) return NS_ERROR_FAILURE;

  rv = NS_ERROR_OUT_OF_MEMORY;
  nsProxyStream* proxy = new nsProxyStream();
  if (! proxy)
    return NS_ERROR_FAILURE;

  listener->OnStartRequest(channel, nsnull);
  while (PR_TRUE) {
    char buf[1024];
    PRUint32 readCount;

    if (NS_FAILED(rv = in->Read(buf, sizeof(buf), &readCount)))
        break; // error

    if (readCount == 0)
        break; // eof

    proxy->SetBuffer(buf, readCount);

    rv = listener->OnDataAvailable(channel, nsnull, proxy, sourceOffset, readCount);
    sourceOffset += readCount;
    if (NS_FAILED(rv))
        break;
  }
  listener->OnStopRequest(channel, nsnull, NS_OK, nsnull);

  // don't leak proxy!
  proxy->Close();
  delete proxy;

  // The document is parsed. We now have a prototype document.
  // Everything worked, so we can just hand this back now.
  *aResult = doc;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsXULKeyListenerImpl::LocateAndExecuteKeyBinding(nsIDOMKeyEvent* aEvent, eEventType aEventType, nsIDOMXULDocument* aDocument,
                                             PRBool& aHandledFlag)
{
  aHandledFlag = PR_FALSE;

  // locate the window element which holds the top level key bindings
  nsCOMPtr<nsIDOMElement> rootElement;
  aDocument->GetDocumentElement(getter_AddRefs(rootElement));
  if (!rootElement)
    return NS_OK;

  //nsAutoString rootName;
  //rootElement->GetNodeName(rootName);
  //printf("Root Node [%s] \n", rootName.ToNewCString()); // this leaks

  nsCOMPtr<nsIDOMNode> currNode;
  rootElement->GetFirstChild(getter_AddRefs(currNode));

  while (currNode) {
    nsAutoString currNodeType;
    nsCOMPtr<nsIDOMElement> currElement(do_QueryInterface(currNode));
    if (currElement) {
      currElement->GetNodeName(currNodeType);
      if (currNodeType.Equals("keyset"))
        return HandleEventUsingKeyset(currElement, aEvent, aEventType, aDocument, aHandledFlag);
    }

    nsCOMPtr<nsIDOMNode> prevNode(currNode);
    prevNode->GetNextSibling(getter_AddRefs(currNode));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULKeyListenerImpl::HandleEventUsingKeyset(nsIDOMElement* aKeysetElement, nsIDOMKeyEvent* aKeyEvent, eEventType aEventType,
                                         nsIDOMXULDocument* aDocument, PRBool& aHandledFlag)
{

  nsAutoString trueString = "true";
  nsAutoString falseString = "false";

#undef DEBUG_XUL_KEYS
#ifdef DEBUG_XUL_KEYS
  if (aEventType == eKeyPress)
  {
      PRUint32 charcode, keycode;
    keyEvent->GetCharCode(&charcode);
    keyEvent->GetKeyCode(&keycode);
    printf("DoKey [%s]: key code 0x%d, char code '%c', ",
           (aEventType == eKeyPress ? "KeyPress" : ""), keycode, charcode);
    PRBool ismod;
    keyEvent->GetShiftKey(&ismod);
    if (ismod) printf("[Shift] ");
    keyEvent->GetCtrlKey(&ismod);
    if (ismod) printf("[Ctrl] ");
    keyEvent->GetAltKey(&ismod);
    if (ismod) printf("[Alt] ");
    keyEvent->GetMetaKey(&ismod);
    if (ismod) printf("[Meta] ");
    printf("\n");
  }
#endif /* DEBUG_XUL_KEYS */

  // Given the DOM node and Key Event
  // Walk the node's children looking for 'key' types

  // XXX Use the key-equivalent CSS3 property to obtain the
  // appropriate modifier for this keyset.
  // TODO. For now it's hardcoded.

  // If the node isn't tagged disabled
  // Compares the received key code to found 'key' types
  // Executes command if found
  // Marks event as consumed

  nsCOMPtr<nsIDOMNode> keyNode;
  aKeysetElement->GetFirstChild(getter_AddRefs(keyNode));
  while (keyNode) {
    nsCOMPtr<nsIDOMElement> keyElement(do_QueryInterface(keyNode));
    if (!keyElement)
      continue;

    nsAutoString property;
    keyElement->GetNodeName(property);
    //printf("keyNodeType [%s] \n", keyNodeType.ToNewCString()); // this leaks

    if (property.Equals("key")) {
      //printf("onkeypress [%s] \n", cmdToExecute.ToNewCString()); // this leaks
      do {
        property = falseString;
        keyElement->GetAttribute(nsAutoString("disabled"), property);
        if (property == trueString) {
          break;
        }

        PRUint32 theChar;
        
        nsAutoString keyName; // This should be phased out for keycode and charcode
        keyElement->GetAttribute(nsAutoString("key"), keyName);
        if ( !keyName.IsEmpty() ) {
            if (aEventType != eKeyPress)
                break;
            aKeyEvent->GetCharCode(&theChar);
        }
        //printf("Found key [%s] \n", keyName.ToNewCString()); // this leaks
        PRBool   gotCharCode = PR_FALSE;
        PRBool   gotKeyCode  = PR_FALSE;
        
        if ( keyName.IsEmpty() )
        {
	        keyElement->GetAttribute(nsAutoString("charcode"), keyName);
	        if(keyName.IsEmpty()) {
	          keyElement->GetAttribute(nsAutoString("keycode"), keyName);
	          if(keyName.IsEmpty()) {
	            // HACK for temporary compatibility
	            if(aEventType == eKeyPress)
	              aKeyEvent->GetCharCode(&theChar);
	            else
	              aKeyEvent->GetKeyCode(&theChar);

	          } else {
	            // We want a keycode
	            aKeyEvent->GetKeyCode(&theChar);
	            gotKeyCode = PR_TRUE;
	          }
	        } else {
	          // We want a charcode
	          aKeyEvent->GetCharCode(&theChar);
	          gotCharCode = PR_TRUE;
	        }
        }

		if(keyName.IsEmpty())
			break;

        char tempChar[2];
        tempChar[0] = theChar;
        tempChar[1] = 0;
        nsAutoString tempChar2 = tempChar;
        //printf("compare key [%s] \n", tempChar2.ToNewCString()); // this leaks
        // NOTE - convert theChar and keyName to upper
        keyName.ToUpperCase();
        tempChar2.ToUpperCase();

        PRBool isMatching;
        if(gotCharCode){
            isMatching = IsMatchingCharCode(tempChar2, keyName);
        } else if(gotKeyCode){
          isMatching = IsMatchingKeyCode(theChar, keyName);
        }

        // HACK for backward compatibility
        if(!gotCharCode && ! gotKeyCode){
          isMatching = IsMatchingCharCode(tempChar2, keyName);
        }

        if (!isMatching) {
          break;
        }

        // This is gross -- we're doing string compares
        // every time we loop over this list!

        // Modifiers in XUL files are tri-state --
        //   true, false, and unspecified.
        // If a modifier is unspecified, we don't check
        // the status of that modifier (always match).

        // Get the attribute for the "xulkey" modifier.
        nsAutoString xproperty = "";
        keyElement->GetAttribute(nsAutoString("xulkey"),
                                 xproperty);

        // Is the modifier key set in the event?
        PRBool isModKey = PR_FALSE;

                  // Check whether the shift key fails to match:
        aKeyEvent->GetShiftKey(&isModKey);
        property = "";
        keyElement->GetAttribute(nsAutoString("shift"), property);
        if ((property == trueString && !isModKey)
            || (property == falseString && isModKey))
            break;
        // and also the xul key, if it's specified to be shift:
        if (xulKeyShift == mXULKeyModifier &&
            ((xproperty == trueString && !isModKey)
             || (xproperty == falseString && isModKey)))
            break;

        // and the control key:
        aKeyEvent->GetCtrlKey(&isModKey);
            property = "";
        keyElement->GetAttribute(nsAutoString("control"), property);
        if ((property == trueString && !isModKey)
          || (property == falseString && isModKey))
          break;
        // and if xul is control:
        if (xulKeyControl == mXULKeyModifier &&
            ((xproperty == trueString && !isModKey)
             || (xproperty == falseString && isModKey)))
            break;

        // and the alt key
        aKeyEvent->GetAltKey(&isModKey);
        property = "";
        keyElement->GetAttribute(nsAutoString("alt"), property);
        if ((property == trueString && !isModKey)
          || (property == falseString && isModKey))
          break;
        // and if xul is alt:
        if (xulKeyAlt == mXULKeyModifier &&
          ((xproperty == trueString && !isModKey)
           || (xproperty == falseString && isModKey)))
          break;

        // and the meta key
        aKeyEvent->GetMetaKey(&isModKey);
        property = "";
        keyElement->GetAttribute(nsAutoString("meta"), property);
        if ((property == trueString && !isModKey)
          || (property == falseString && isModKey))
          break;
        // and if xul is meta:
        if (xulKeyMeta == mXULKeyModifier &&
          ((xproperty == trueString && !isModKey)
           || (xproperty == falseString && isModKey)))
          break;

        // We know we're handling this.
        aHandledFlag = PR_TRUE;

        // Get the cancel attribute.
        nsAutoString cancelValue;
        keyElement->GetAttribute(nsAutoString("cancel"), cancelValue);
        if (cancelValue == "true") {
          return NS_OK;
        }

        // Modifier tests passed so execute onclick command
        nsAutoString cmdToExecute;
        nsAutoString oncommand;
        switch(aEventType) {
          case eKeyPress:
            keyElement->GetAttribute(nsAutoString("onkeypress"), cmdToExecute);
#if defined(DEBUG_saari)
            printf("onkeypress = %s\n",
                               cmdToExecute.ToNewCString());
#endif

            keyElement->GetAttribute(nsAutoString("oncommand"), oncommand);
#if defined(DEBUG_saari)
            printf("oncommand = %s\n", oncommand.ToNewCString());
#endif
          break;
          case eKeyDown:
            keyElement->GetAttribute(nsAutoString("onkeydown"), cmdToExecute);
          break;
          case eKeyUp:
            keyElement->GetAttribute(nsAutoString("onkeyup"), cmdToExecute);
          break;
        }

        // This code executes in every presentation context in which this
        // document is appearing.
        nsCOMPtr<nsIDocument> document = do_QueryInterface(aDocument);
        nsCOMPtr<nsIContent> content = do_QueryInterface(keyElement);
        if (aDocument != mDOMDocument) {
          nsCOMPtr<nsIScriptEventHandlerOwner> handlerOwner = do_QueryInterface(keyElement);
          if (handlerOwner) {
            nsAutoString eventStr;
            switch(aEventType) {
              case eKeyPress:
                eventStr = "onkeypress";
                break;
              case eKeyDown:
                eventStr = "onkeydown";
                break;
              case eKeyUp:
                eventStr = "onkeyup";
                break;
            }

            // Look for a compiled event handler on the key element itself.
            nsCOMPtr<nsIAtom> eventName = getter_AddRefs(NS_NewAtom(eventStr));
            void* handler = nsnull;
            handlerOwner->GetCompiledEventHandler(eventName, &handler);

            nsCOMPtr<nsIScriptGlobalObject> masterGlobalObject;
            nsCOMPtr<nsIDocument> masterDoc = do_QueryInterface(mDOMDocument);
            masterDoc->GetScriptGlobalObject(getter_AddRefs(masterGlobalObject));
            nsCOMPtr<nsIScriptContext> masterContext;
            masterGlobalObject->GetContext(getter_AddRefs(masterContext));

            if (!handler) {
              // It hasn't been compiled before.
              nsCOMPtr<nsIScriptGlobalObject> globalObject;
              document->GetScriptGlobalObject(getter_AddRefs(globalObject));
              if (!globalObject) {
                NS_NewScriptGlobalObject(getter_AddRefs(globalObject));
                document->SetScriptGlobalObject(globalObject);
              }

              nsCOMPtr<nsIScriptContext> context;
              globalObject->GetContext(getter_AddRefs(context));
              if (!context) {
                NS_CreateScriptContext(globalObject, getter_AddRefs(context));
                globalObject->SetNewDocument(aDocument);
              }

              nsCOMPtr<nsIScriptObjectOwner> owner = do_QueryInterface(keyElement);
              void* scriptObject;
              owner->GetScriptObject(context, &scriptObject);

              nsCOMPtr<nsIContent> keyContent = do_QueryInterface(keyElement);
              nsAutoString value;
              keyContent->GetAttribute(kNameSpaceID_None, eventName, value);
              if (value != "") {
                  if (handlerOwner) {
                      handlerOwner->CompileEventHandler(context, scriptObject, eventName, value, &handler);
                  }
                  else {
                      context->CompileEventHandler(scriptObject, eventName, value,
                                                   PR_TRUE, &handler);
                  }
              }
            }

            if (handler) {
              nsCOMPtr<nsIDOMElement> rootElement;
              mDOMDocument->GetDocumentElement(getter_AddRefs(rootElement));
              nsCOMPtr<nsIScriptObjectOwner> owner = do_QueryInterface(rootElement);
              void* scriptObject;
              owner->GetScriptObject(masterContext, &scriptObject);

              masterContext->BindCompiledEventHandler(scriptObject, eventName, handler);

              nsCOMPtr<nsIDOMEventListener> eventListener;
              NS_NewJSEventListener(getter_AddRefs(eventListener), masterContext, owner);
              eventListener->HandleEvent(aKeyEvent);

              masterContext->BindCompiledEventHandler(scriptObject, eventName, nsnull);

              return NS_OK;
            }
          }
        }

        PRInt32 count = document->GetNumberOfShells();
        for (PRInt32 i = 0; i < count; i++) {
          nsCOMPtr<nsIPresContext> aPresContext;
          { /* scope the shell variable (prevents the PresShell
               from outliving its ViewManager if the key event
               happens to delete the window.) */
            // Retrieve the context in which our DOM event will fire.
            nsCOMPtr<nsIPresShell> shell = getter_AddRefs(document->GetShellAt(i));
            shell->GetPresContext(getter_AddRefs(aPresContext));
          }

          // Handle the DOM event
          nsEventStatus status = nsEventStatus_eIgnore;
          nsKeyEvent event;
          event.eventStructType = NS_KEY_EVENT;
          switch (aEventType)
          {
            case eKeyPress:  event.message = NS_KEY_PRESS; break;
            case eKeyDown:   event.message = NS_KEY_DOWN; break;
            default:         event.message = NS_KEY_UP; break;
          }
          aKeyEvent->PreventBubble();
          aKeyEvent->PreventCapture();
          aKeyEvent->PreventDefault();
          content->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
          nsresult ret = NS_ERROR_BASE;

          if (aEventType == eKeyPress) {
            // Also execute the oncommand handler on a key press.
            // Execute the oncommand event handler.
            nsEventStatus stat = nsEventStatus_eIgnore;
            nsMouseEvent evt;
            evt.eventStructType = NS_EVENT;
            evt.message = NS_MENU_ACTION;
            content->HandleDOMEvent(aPresContext, &evt, nsnull, NS_EVENT_FLAG_INIT, &stat);
          }
          // Ok, we got this far and handled the event, so don't continue scanning nodes
          //printf("Keybind executed \n");
          return ret;
        } // end for (PRInt32 i = 0; i < count; i++)
      } while (PR_FALSE); // do { ...
    } // end if (keyNodeType.Equals("key"))

    nsCOMPtr<nsIDOMNode> oldkeyNode(keyNode);
    oldkeyNode->GetNextSibling(getter_AddRefs(keyNode));
  } // end while(keynode)

  return NS_OK;
}

////////////////////////////////////////////////////////////////
nsresult
NS_NewXULKeyListener(nsIXULKeyListener** aListener)
{
  nsXULKeyListenerImpl * listener = new nsXULKeyListenerImpl();
  if (!listener)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(listener);
  *aListener = listener;
  return NS_OK;
}
