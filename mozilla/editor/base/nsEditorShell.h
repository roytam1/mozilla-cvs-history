/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef nsEditorAppCore_h___
#define nsEditorAppCore_h___

//#include "nsAppCores.h"

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsISupports.h"

#include "nsIEditorShell.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsIDOMSelectionListener.h"
#ifdef NECKO
#include "nsIPrompt.h"
#else
#include "nsINetSupport.h"
#endif
#include "nsIStreamObserver.h"
#include "nsIDOMDocument.h"
#include "nsVoidArray.h"
#include "nsTextServicesCID.h"
#include "nsIEditorSpellCheck.h"
#include "nsISpellChecker.h"

class nsIBrowserWindow;
class nsIWebShell;
class nsIScriptContext;
class nsIDOMWindow;
class nsIDOMElement;
class nsIDOMNode;
class nsIURI;
class nsIWebShellWindow;
class nsIPresShell;
class nsIHTMLEditor;
class nsITextEditor;
class nsIOutputStream;



#define NS_EDITORAPPCORE_CID                          \
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

    nsEditorShell();
    virtual ~nsEditorShell();

    NS_DECL_ISUPPORTS
    
  //  NS_IMETHOD    GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
    NS_IMETHOD    Init();
  //  NS_IMETHOD    GetId(nsString& aId);	// { return nsBaseAppCore::GetId(aId); } 
  //  NS_IMETHOD    SetDocumentCharset(const nsString& aCharset);	//  { return nsBaseAppCore::SetDocumentCharset(aCharset); } 


    /* nsIEditorShell interface */

	  NS_IMETHOD GetEditorDocument(nsIDOMDocument * *aEditorDocument);
	  NS_IMETHOD GetEditorSelection(nsIDOMSelection * *aEditorSelection);

    NS_IMETHOD GetDocumentStatus(PRInt32 *aDocumentStatus);

	  NS_IMETHOD GetWrapColumn(PRInt32 *aWrapColumn);
	  NS_IMETHOD SetWrapColumn(PRInt32 aWrapColumn);

	  NS_IMETHOD SetEditorType(const PRUnichar *editorType);

	  NS_IMETHOD SetToolbarWindow(nsIDOMWindow *win);
	  NS_IMETHOD SetContentWindow(nsIDOMWindow *win);
	  NS_IMETHOD SetWebShellWindow(nsIDOMWindow *win);
	  NS_IMETHOD LoadUrl(const PRUnichar *url);

	  /* void NewWindow (); */
	  NS_IMETHOD NewWindow();
	  NS_IMETHOD Open();
	  NS_IMETHOD Save();
	  NS_IMETHOD SaveAs();
	  NS_IMETHOD CloseWindow();
	  NS_IMETHOD Print();
	  NS_IMETHOD Exit();

	  NS_IMETHOD Undo();
	  NS_IMETHOD Redo();
	  NS_IMETHOD Cut();
	  NS_IMETHOD Copy();
	  NS_IMETHOD Paste();

	  NS_IMETHOD PasteAsQuotation();
	  NS_IMETHOD PasteAsCitedQuotation(const PRUnichar *cite);
	  NS_IMETHOD InsertAsQuotation(const PRUnichar *quotedText);
	  NS_IMETHOD InsertAsCitedQuotation(const PRUnichar *quotedText, const PRUnichar *cite);

	  NS_IMETHOD SelectAll();
	  NS_IMETHOD DeleteSelection(PRInt32 direction);

	  /* void Find (); */
	  NS_IMETHOD Find();
	  NS_IMETHOD FindNext();

	  /* void InsertText (in wstring textToInsert); */
	  NS_IMETHOD InsertText(const PRUnichar *textToInsert);
	  NS_IMETHOD InsertSource(const PRUnichar *sourceToInsert);
    NS_IMETHOD InsertBreak();
	  NS_IMETHOD InsertLink();
	  NS_IMETHOD InsertImage();
	  NS_IMETHOD InsertList(const PRUnichar *listType);

	  /* void Indent (in string indent); */
	  NS_IMETHOD Indent(const PRUnichar *indent);
	  NS_IMETHOD Align(const PRUnichar *align);

    /* nsIDOMElement GetSelectedElement (in wstring tagName); */
	  NS_IMETHOD GetSelectedElement(const PRUnichar *tagName, nsIDOMElement **_retval);
	  NS_IMETHOD CreateElementWithDefaults(const PRUnichar *tagName, nsIDOMElement **_retval);
	  NS_IMETHOD InsertElement(nsIDOMElement *element, PRBool deleteSelection);
    NS_IMETHOD SaveHLineSettings(nsIDOMElement* aElement);
	  NS_IMETHOD InsertLinkAroundSelection(nsIDOMElement *anchorElement);
	  NS_IMETHOD SelectElement(nsIDOMElement *element);
	  NS_IMETHOD SetSelectionAfterElement(nsIDOMElement *element);

	  /* void SetParagraphFormat (in string value); */
	  NS_IMETHOD SetParagraphFormat(PRUnichar *value);
	  NS_IMETHOD GetParagraphFormat(PRUnichar * *aParagraphFormat);

	  /* void SetTextProperty (in string prop, in string attr, in string value); */
	  NS_IMETHOD SetTextProperty(const PRUnichar *prop, const PRUnichar *attr, const PRUnichar *value);
	  NS_IMETHOD GetTextProperty(const PRUnichar *prop, const PRUnichar *attr, const PRUnichar *value, PRUnichar **firstHas, PRUnichar **anyHas, PRUnichar **allHas);
	  NS_IMETHOD RemoveTextProperty(const PRUnichar *prop, const PRUnichar *attr);

	  /* void SetBodyAttribute (in string attr, in string value); */
	  NS_IMETHOD SetBodyAttribute(const PRUnichar *attr, const PRUnichar *value);
	  NS_IMETHOD SetBackgroundColor(const PRUnichar *color);

	  NS_IMETHOD ApplyStyleSheet(const PRUnichar *url);

    /* Get the contents, for output or other uses */
    NS_IMETHOD GetContentsAs(const PRUnichar *format, PRUint32 flags, PRUnichar **contentsAs);

    /* Debugging: dump content tree to stdout */
    NS_IMETHOD DumpContentTree();

	  /* string GetLocalFileURL (in nsIDOMWindow parent, in string filterType); */
	  NS_IMETHOD GetLocalFileURL(nsIDOMWindow *parent, const PRUnichar *filterType, PRUnichar **_retval);

	  /* void BeginBatchChanges (); */
	  NS_IMETHOD BeginBatchChanges();
	  NS_IMETHOD EndBatchChanges();

	  /* void RunUnitTests (); */
	  NS_IMETHOD RunUnitTests();

    /* void BeginLogging (); */
	  NS_IMETHOD StartLogging(nsIFileSpec *logFile);
	  NS_IMETHOD StopLogging();


    /* Spell check interface */
	  NS_IMETHOD StartSpellChecking(PRUnichar **_retval);
	  NS_IMETHOD GetNextMisspelledWord(PRUnichar **_retval);
	  NS_IMETHOD GetSuggestedWord(PRUnichar **_retval);
	  NS_IMETHOD CheckCurrentWord(const PRUnichar *suggestedWord, PRBool *_retval);
	  NS_IMETHOD ReplaceWord(const PRUnichar *misspelledWord, const PRUnichar *replaceWord, PRBool allOccurrences);
	  NS_IMETHOD IgnoreWordAllOccurrences(const PRUnichar *word);
	  NS_IMETHOD AddWordToDictionary(const PRUnichar *word);
	  NS_IMETHOD RemoveWordFromDictionary(const PRUnichar *word);
	  NS_IMETHOD GetPersonalDictionaryWord(const PRUnichar *word, PRUnichar **_retval);
	  NS_IMETHOD CloseSpellChecking();


    // nsIDocumentLoaderObserver
    NS_IMETHOD OnStartDocumentLoad(nsIDocumentLoader* loader, nsIURI* aURL, const char* aCommand);

#ifndef NECKO
    NS_IMETHOD OnEndDocumentLoad(nsIDocumentLoader* loader, nsIURI *aUrl, PRInt32 aStatus,
								  nsIDocumentLoaderObserver * aObserver);
#else
    NS_IMETHOD OnEndDocumentLoad(nsIDocumentLoader* loader, nsIChannel* channel, PRInt32 aStatus,
                                  nsIDocumentLoaderObserver * aObserver);
#endif // NECKO

#ifndef NECKO
    NS_IMETHOD OnStartURLLoad(nsIDocumentLoader* loader, nsIURI* aURL, const char* aContentType, 
                           		 nsIContentViewer* aViewer);
#else
    NS_IMETHOD OnStartURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, const char* aContentType,
                                 nsIContentViewer* aViewer);
#endif // NECKO

#ifndef NECKO
    NS_IMETHOD OnProgressURLLoad(nsIDocumentLoader* loader, nsIURI* aURL, PRUint32 aProgress, 
                               PRUint32 aProgressMax);
#else
    NS_IMETHOD OnProgressURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, PRUint32 aProgress,
                               PRUint32 aProgressMax);
#endif // NECKO

#ifndef NECKO
    NS_IMETHOD OnStatusURLLoad(nsIDocumentLoader* loader, nsIURI* aURL, nsString& aMsg);
#else
    NS_IMETHOD OnStatusURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, nsString& aMsg);
#endif // NECKO

#ifndef NECKO
    NS_IMETHOD OnEndURLLoad(nsIDocumentLoader* loader, nsIURI* aURL, PRInt32 aStatus);
#else
    NS_IMETHOD OnEndURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, PRInt32 aStatus);
#endif // NECKO

#ifndef NECKO
    NS_IMETHOD HandleUnknownContentType(nsIDocumentLoader* loader,
                                        nsIURI *aURL,
                                        const char *aContentType,
                                        const char *aCommand );
#else
    NS_IMETHOD HandleUnknownContentType(nsIDocumentLoader* loader,
                                        nsIChannel* channel,
                                        const char *aContentType,
                                        const char *aCommand );
#endif // NECKO

  protected:
    nsCOMPtr<nsISpellChecker> mSpellChecker;
    nsStringArray   mSuggestedWordList;
    PRInt32         mSuggestedWordIndex;
    NS_IMETHOD      DeleteSuggestedWordList();
    nsStringArray   mDictionaryList;
    PRInt32         mDictionaryIndex;

  	typedef enum {
  	  eUninitializedEditorType = 0,
  		ePlainTextEditorType = 1,
  		eHTMLTextEditorType = 2
  	} EEditorType;
  	
    nsIPresShell* 	GetPresShellFor(nsIWebShell* aWebShell);
    NS_IMETHOD 			DoEditorMode(nsIWebShell *aWebShell);
    NS_IMETHOD	 		ExecuteScript(nsIScriptContext * aContext, const nsString& aScript);
    NS_IMETHOD			InstantiateEditor(nsIDOMDocument *aDoc, nsIPresShell *aPresShell);
    NS_IMETHOD			RemoveOneProperty(const nsString& aProp, const nsString& aAttr);
    void 						SetButtonImage(nsIDOMNode * aParentNode, PRInt32 aBtnNum, const nsString &aResName);
		NS_IMETHOD			CreateWindowWithURL(const char* urlStr);
		NS_IMETHOD  	  PrepareDocumentForEditing();
		NS_IMETHOD      DoFind(PRBool aFindNext);
		
		// this returns an AddReffed nsIScriptContext. You must relase it.
		nsIScriptContext*  GetScriptContext(nsIDOMWindow * aWin);

    nsString            mEnableScript;     
    nsString            mDisableScript;     

    nsIDOMWindow       *mToolbarWindow;				// weak reference
    nsIDOMWindow       *mContentWindow;				// weak reference

    nsIWebShellWindow  *mWebShellWin;					// weak reference
    nsIWebShell        *mWebShell;						// weak reference
    nsIWebShell        *mContentAreaWebShell;	// weak reference

		EEditorType					mEditorType;
		nsString						mEditorTypeString;	// string which describes which editor type will be instantiated (lowercased)
    nsCOMPtr<nsISupports>	 	mEditor;						// this can be either an HTML or plain text (or other?) editor

    nsCOMPtr<nsISupports>   mSearchContext;		// context used for search and replace. Owned by the appshell.
    
#if 0
    nsCOMPtr<nsIDOMSelectionListener> mStateMaintainer;
#endif    

    PRInt32 mWrapColumn;      // can't actually set this 'til the editor is created, so we may have to hold on to it for a while
};

#endif // nsEditorAppCore_h___
