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
 * Contributor(s): 
 */
#ifndef nsEditorShell_h___
#define nsEditorShell_h___

//#include "nsAppCores.h"

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"//XXX adding weak ref knowledge
#include "nsString.h"
#include "nsISupports.h"

#include "nsIEditorShell.h"
#include "nsIEditorController.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsIDOMSelectionListener.h"
#include "nsIDOMEventReceiver.h"
#include "nsIPrompt.h"
#include "nsIStreamObserver.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventListener.h"
#include "nsVoidArray.h"
#include "nsTextServicesCID.h"
#include "nsIEditorSpellCheck.h"
#include "nsISpellChecker.h"
#include "nsInterfaceState.h"
#include "nsIHTMLEditor.h"
#include "nsIStringBundle.h"
#include "nsICSSStyleSheet.h"

// Parser Observation
#include "nsEditorParserObserver.h"

class nsIDocShell;
class nsIScriptContext;
class nsIDOMWindow;
class nsIDOMElement;
class nsIDOMNode;
class nsIURI;
class nsIPresShell;
class nsIOutputStream;
class nsISupportsArray;
class nsIStringBundleService;
class nsIStringBundle;
class nsIStyleSheet;

#define NS_EDITORSHELL_CID                            \
{ /* {} */                                            \
    0x9afff72b, 0xca9a, 0x11d2,                       \
    { 0x96, 0xc9, 0x0, 0x60, 0xb0, 0xfb, 0x99, 0x56 } \
}

////////////////////////////////////////////////////////////////////////////////
// nsEditorShell:
////////////////////////////////////////////////////////////////////////////////

class nsEditorShell :   public nsIEditorShell,
                        public nsIEditorSpellCheck,
                        public nsIDocumentLoaderObserver
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

    /* Declare all methods in the nsIEditorSpellCheck interface */
    NS_DECL_NSIEDITORSPELLCHECK

    // nsIDocumentLoaderObserver
    NS_DECL_NSIDOCUMENTLOADEROBSERVER

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
      eCantEditOther = 3
  	} ECantEditReason;
  	
    nsresult   			DoEditorMode(nsIDocShell *aDocShell);
    // nuke any existing editor in the editorShell, thus preparing it to edit
    // a(nother) document.
    nsresult        ResetEditingState();
    nsresult  			InstantiateEditor(nsIDOMDocument *aDoc, nsIPresShell *aPresShell);
		nsresult    	  PrepareDocumentForEditing(nsIDocumentLoader* aLoader, nsIURI *aUrl);
    nsresult  			ScrollSelectionIntoView();
    nsresult        TransferDocumentStateListeners();
    nsresult  			RemoveOneProperty(const nsString& aProp, const nsString& aAttr);
		nsresult        DoFind(PRBool aFindNext);
    // To allow executing JavaScript commands from C++ via nsIEditorControler interface
    nsresult        DoControllerCommand(nsString& aCommand);

    void            Alert(const nsString& aTitle, const nsString& aMsg);
    // Bring up a Yes/No dialog WE REALLY NEED A Yes/No/Cancel dialog and would like to set our own caption as well!
    PRBool          Confirm(const nsString& aTitle, const nsString& aQuestion);
    // Return value: No=0, Yes=1, Cancel=2
    // aYesString and aNoString are optional:
    // if null, then "Yes" and "No" are used
    EConfirmResult  ConfirmWithCancel(const nsString& aTitle, const nsString& aQuestion,
                                     const nsString *aYesString, const nsString *aNoString);

		// this returns an AddReffed nsIScriptContext. You must relase it.
		nsIScriptContext*  GetScriptContext(nsIDOMWindow * aWin);

    // Get a string from the string bundle file
    void            GetBundleString(const nsString& name, nsString &outString);
    
    // Get the text of the <title> tag
    nsresult        GetDocumentTitleString(nsString& title);

    nsresult        DeleteSuggestedWordList();

    // Get the current document title an use it as part of the window title
    // Uses "(Untitled)" for empty title
    nsresult        UpdateWindowTitle();

    // does the document being loaded contain subframes?
    nsresult        DocumentContainsFrames(nsIDocumentLoader* aLoader, PRBool& outHasFrames);
    // is the document being loaded the root of a frameset, or a non-frameset doc?
    nsresult        DocumentIsRootDoc(nsIDocumentLoader* aLoader, PRBool& outIsRoot);
    
    nsCOMPtr<nsIHTMLEditor>	 	    mEditor;         // this can be either an HTML or plain text (or other?) editor
    nsCOMPtr<nsISupports>         mSearchContext;  // context used for search and replace. Owned by the appshell.
    nsCOMPtr<nsISpellChecker>     mSpellChecker;

    // Let UI detect and process double click on elements for AdvancedProperties
    //  (see nsEditorShellMouseEventListener)
    nsCOMPtr<nsIDOMEventListener> mMouseListenerP;
    // We need this to add mMouseListenerP to document
    nsresult GetDocumentEventReceiver(nsIDOMEventReceiver **aEventReceiver);

    PRBool mMailCompose;

    // this is a holding pen for doc state listeners. They will be registered with
    // the editor when that gets created.
    nsCOMPtr<nsISupportsArray>  mDocStateListeners;		// contents are nsISupports

    // Pointer to localized strings used for UI
    nsCOMPtr<nsIStringBundle>   mStringBundle;

    // Pointer to extra style sheets we load/unload
    //  for various Edit Modes or for Paragraph Marks
    nsCOMPtr<nsICSSStyleSheet>  mEditModeStyleSheet;
    nsCOMPtr<nsICSSStyleSheet>  mAllTagsModeStyleSheet;
    nsCOMPtr<nsICSSStyleSheet>  mParagraphMarksStyleSheet;

    // The override style sheet that we never unload while editing
    nsCOMPtr<nsICSSStyleSheet>  mBaseStyleSheet;
    
    // Saves the current display mode to reload style sheets
    //   after loading a url
    PRInt32 mDisplayMode;
    
    nsIDOMWindow            *mWebShellWindow;				// weak reference
    //nsIDOMWindow            *mContentWindow;				// weak reference
    nsWeakPtr                mContentWindow;        // weak reference

    nsEditorParserObserver  *mParserObserver;       // we hold the owning ref to this.
    nsInterfaceState        *mStateMaintainer;      // we hold the owning ref to this.

    nsIEditorController     *mEditorController;     // temporary weak ref to the editor controller
    nsIDocShell             *mDocShell;						  // weak reference

    // The webshell that contains the document being edited.
    // Don't assume that webshell directly contains the document being edited;
    // if we are in a frameset, this assumption is false.
    nsIDocShell         *mContentAreaDocShell;	// weak reference

    PRPackedBool        mCloseWindowWhenLoaded; // error on load. Close window when loaded
    ECantEditReason     mCantEditReason;
    
		EEditorType					mEditorType;
		nsString						mEditorTypeString;	    // string which describes which editor type will be instantiated (lowercased)
    PRInt32             mWrapColumn;            // can't actually set this 'til the editor is created, so we may have to hold on to it for a while

    nsStringArray       mSuggestedWordList;
    PRInt32             mSuggestedWordIndex;

    nsStringArray       mDictionaryList;
    PRInt32             mDictionaryIndex;
    
};

#endif // nsEditorShell_h___
