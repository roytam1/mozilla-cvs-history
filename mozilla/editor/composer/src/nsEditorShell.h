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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
#ifndef nsEditorShell_h___
#define nsEditorShell_h___

//#include "nsAppCores.h"

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"//XXX adding weak ref knowledge
#include "nsString.h"
#include "nsISupports.h"

#include "nsIEditorShell.h"
#include "nsIWebProgressListener.h"
#include "nsIDOMEventListener.h"
#include "nsIURIContentListener.h"
#include "nsIEditorSpellCheck.h"
#include "nsISpellChecker.h"
#include "nsIHTMLEditor.h"
#include "nsIStringBundle.h"
#include "nsICSSStyleSheet.h"
#include "nsISupportsArray.h"
#include "nsIPrintSettings.h"

class nsIDOMDocument;
class nsIDocShell;
class nsIScriptContext;
class nsIDOMWindow;
class nsIDOMWindowInternal;
class nsIDOMElement;
class nsIDOMNode;
class nsIDOMHTMLDocument;
class nsIURI;
class nsIChannel;
class nsIPresShell;
class nsIOutputStream;
class nsISupportsArray;
class nsIStringBundleService;
class nsIStringBundle;
class nsIStyleSheet;
class nsIEditorController;
class nsIDOMEventReceiver;
class nsIDOMEventListener;
class nsISpellChecker;
class nsInterfaceState;
class nsIHTMLEditor;
class nsICSSStyleSheet;
class nsEditorParserObserver;

#define NS_EDITORSHELL_CID                            \
{ /* {} */                                            \
    0x9afff72b, 0xca9a, 0x11d2,                       \
    { 0x96, 0xc9, 0x0, 0x60, 0xb0, 0xfb, 0x99, 0x56 } \
}

////////////////////////////////////////////////////////////////////////////////
// nsEditorShell:
////////////////////////////////////////////////////////////////////////////////

class nsEditorShell :   public nsIEditorShell,
                        public nsIWebProgressListener,
                        public nsIURIContentListener,
                        public nsSupportsWeakReference
{
  public:

    // These must map onto the button-order for nsICommonDialog::Confirm results
    //  which are rather ugly right now (Cancel in the middle!)
    typedef enum {eYes = 0, eCancel = 1, eNo = 2 } EConfirmResult;

    nsEditorShell();
    virtual ~nsEditorShell();

    NS_DECL_ISUPPORTS

    /* Declare all methods in the nsIEditorShell interface */
    NS_DECL_NSIEDITORSHELL

    // nsIWebProgressListener
    NS_DECL_NSIWEBPROGRESSLISTENER

    // nsIURIContentListener
    NS_DECL_NSIURICONTENTLISTENER
    
  protected:

    typedef enum {
      eUninitializedEditorType = 0,
      ePlainTextEditorType = 1,
      eHTMLTextEditorType = 2
    } EEditorType;


    typedef enum {
      eCantEditNoReason = 0,
      eCantEditFramesets = 1,
      eCantEditMimeType = 2,
      eCantEditFileNotFound = 3,
      eCantEditOther = 9
    } ECantEditReason;
    
    nsresult        DoEditorMode(nsIDocShell *aDocShell);
    // nuke any existing editor in the editorShell, thus preparing it to edit
    // a(nother) document.
    nsresult        ResetEditingState();
    nsresult        InstantiateEditor(nsIDOMDocument *aDoc, nsIPresShell *aPresShell);
    nsresult        PrepareDocumentForEditing(nsIDOMWindow* aDOMWindow, nsIURI *aUrl);
    nsresult        TransferDocumentStateListeners();
    nsresult        RemoveOneProperty(const nsString& aProp, const nsString& aAttr);
    nsresult        DoFind(PRBool aFindNext);
    // To allow executing JavaScript commands from C++ via nsIEditorController interface
    nsresult        DoControllerCommand(const char* aCommand);

    void            Alert(const nsString& aTitle, const nsString& aMsg);
    // Bring up a Yes/No dialog WE REALLY NEED A Yes/No/Cancel dialog and would like to set our own caption as well!
    PRBool          Confirm(const nsString& aTitle, const nsString& aQuestion);
    // Return value: No=0, Yes=1, Cancel=2
    // aYesString and aNoString are optional:
    // if null, then "Yes" and "No" are used
    EConfirmResult  ConfirmWithCancel(const nsString& aTitle, const nsString& aQuestion,
                                     const nsString *aYesString, const nsString *aNoString);

    // Get a string from the string bundle file. If the string is not found
    // this returns an empty string.
    void            GetBundleString(const nsAString &stringName, nsAString &outString);
    
    // Get the text of the <title> tag
    nsresult        GetDocumentTitleString(nsString& title);

    nsresult        GetDocumentURI(nsIDOMDocument *aDoc, nsIURI **aDocumentURI);

    // Helper method which is called at the beginning of a new page load
    nsresult        StartPageLoad(nsIChannel *aChannel);

    // Helper method which is called when an entire page load finishes
    nsresult        EndPageLoad(nsIDOMWindow *aDOMWindow,
                                nsIChannel *aChannel,
                                nsresult aStatus);

    // helper method which is called each time a document (or frame) starts
    // to load.
    nsresult        StartDocumentLoad(nsIDOMWindow *aDOMWindow);
    // helper methods which is called each time a document (or frame) finishes
    // loading.
    nsresult        EndDocumentLoad(nsIDOMWindow *aDOMWindow,
                                    nsIChannel *aChannel,
                                    nsresult aStatus);

    // Check a preference and call NormalizeTable if pref is true
    // Use after deleting or inserting table cells to automatically 
    //  fix rowspan, colspan, and missing cells problems
    nsresult CheckPrefAndNormalizeTable();

    nsCOMPtr<nsIHTMLEditor>         mEditor;         // this can be either an HTML or plain text (or other?) editor
    nsCOMPtr<nsISupports>         mSearchContext;  // context used for search and replace. Owned by the appshell.
    nsCOMPtr<nsISpellChecker>     mSpellChecker;

    nsresult GetDocShellFromContentWindow(nsIDocShell **aDocShell);
    PRBool mMailCompose;

    // These doc listeners are registered with the editor when that gets
    // created. We also keep them in this list so we can register if we have
    // to blow away the editor (e.g. URL reload)
    nsCOMPtr<nsISupportsArray>  mDocStateListeners;    // contents are nsISupports

    // Pointer to localized strings used for UI
    nsCOMPtr<nsIStringBundle>   mStringBundle;

    PRBool  mHTMLSourceMode;
    
    nsIDOMWindowInternal            *mWebShellWindow;        // weak reference
    //nsIDOMWindowInternal            *mContentWindow;        // weak reference
    nsWeakPtr                mContentWindow;        // weak reference

    nsEditorParserObserver  *mParserObserver;       // we hold the owning ref to this.
    nsInterfaceState        *mStateMaintainer;      // we hold the owning ref to this.

    nsIEditorController     *mEditorController;     // temporary weak ref to the editor controller
    nsIEditorController     *mComposerController;   // temporary weak ref to the nsComposerController
    nsIDocShell             *mDocShell;              // weak reference

    // The webshell that contains the document being edited.
    // Don't assume that webshell directly contains the document being edited;
    // if we are in a frameset, this assumption is false.
    nsIDocShell         *mContentAreaDocShell;  // weak reference

    PRPackedBool        mCloseWindowWhenLoaded; // error on load. Close window when loaded
    ECantEditReason     mCantEditReason;
    
    EEditorType         mEditorType;
    nsString            mEditorTypeString;      // string which describes which editor type will be instantiated (lowercased)
    nsCString           mContentMIMEType;       // MIME type of the doc we are editing.
    PRBool              mContentTypeKnown;
    
    PRInt32             mWrapColumn;            // can't actually set this 'til the editor is created, so we may have to hold on to it for a while

    nsCOMPtr<nsIPrintSettings> mPrintSettings;
};

#endif // nsEditorShell_h___
