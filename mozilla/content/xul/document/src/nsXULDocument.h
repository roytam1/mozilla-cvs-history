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
 */

#ifndef nsXULDocument_h__
#define nsXULDocument_h__

#include "nsCOMPtr.h"
#include "nsElementMap.h"
#include "nsForwardReference.h"
#include "nsIArena.h"
#include "nsICSSLoader.h"
#include "nsIContent.h"
#include "nsIDOMEventCapturer.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMSelection.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIHTMLCSSStyleSheet.h"
#include "nsIHTMLContentContainer.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIJSScriptObject.h"
#include "nsILineBreakerFactory.h"
#include "nsINameSpaceManager.h"
#include "nsIParser.h"
#include "nsIPrincipal.h"
#include "nsIRDFDataSource.h"
#include "nsIScriptObjectOwner.h"
#include "nsIStreamLoadableDocument.h"
#include "nsISupportsArray.h"
#include "nsIURI.h"
#include "nsIWordBreakerFactory.h"
#include "nsIXULDocument.h"
#include "nsIXULPrototypeDocument.h"
#include "nsRDFDOMNodeList.h"
#include "nsTime.h"
#include "nsVoidArray.h"
#include "nsWeakPtr.h"
#include "nsWeakReference.h"
#include "nsIUnicharStreamLoader.h"

class nsIAtom;
class nsIHTMLElementFactory;
class nsILoadGroup;
class nsIRDFResource;
class nsIRDFService;
class nsIScriptContextOwner;
class nsITimer;
class nsIUnicharStreamLoader;
class nsIXMLElementFactory;
class nsIXULContentUtils;
class nsIXULPrototypeCache;
#if 0 // XXXbe save me, scc (need NSCAP_FORWARD_DECL(nsXULPrototypeScript))
class nsIXULPrototypeScript;
#else
#include "nsXULElement.h"
#endif

struct JSObject;
struct PRLogModuleInfo;

/**
 * The XUL document class
 */
class nsXULDocument : public nsIDocument,
                      public nsIXULDocument,
                      public nsIStreamLoadableDocument,
                      public nsIDOMXULDocument,
                      public nsIDOMNSDocument,
                      public nsIDOMEventCapturer,
                      public nsIJSScriptObject,
                      public nsIScriptObjectOwner,
                      public nsIHTMLContentContainer,
                      public nsIUnicharStreamLoaderObserver,
                      public nsSupportsWeakReference
{
public:
    nsXULDocument();
    virtual ~nsXULDocument();

    // nsISupports interface
    NS_DECL_ISUPPORTS
    NS_DECL_NSIUNICHARSTREAMLOADEROBSERVER

    // nsIDocument interface
    virtual nsIArena* GetArena();

    NS_IMETHOD GetContentType(nsString& aContentType) const;

    NS_IMETHOD StartDocumentLoad(const char* aCommand,
                                 nsIChannel* aChannel,
                                 nsILoadGroup* aLoadGroup,
                                 nsISupports* aContainer,
                                 nsIStreamListener **aDocListener);

    virtual const nsString* GetDocumentTitle() const;

    virtual nsIURI* GetDocumentURL() const;

    virtual nsIPrincipal* GetDocumentPrincipal();

    NS_IMETHOD GetDocumentLoadGroup(nsILoadGroup **aGroup) const;

    NS_IMETHOD GetBaseURL(nsIURI*& aURL) const;

    NS_IMETHOD GetDocumentCharacterSet(nsString& oCharSetID);

    NS_IMETHOD SetDocumentCharacterSet(const nsString& aCharSetID);

    NS_IMETHOD GetLineBreaker(nsILineBreaker** aResult) ;
    NS_IMETHOD SetLineBreaker(nsILineBreaker* aLineBreaker) ;
    NS_IMETHOD GetWordBreaker(nsIWordBreaker** aResult) ;
    NS_IMETHOD SetWordBreaker(nsIWordBreaker* aWordBreaker) ;

    NS_IMETHOD GetHeaderData(nsIAtom* aHeaderField, nsString& aData) const;
    NS_IMETHOD SetHeaderData(nsIAtom* aheaderField, const nsString& aData);

    NS_IMETHOD CreateShell(nsIPresContext* aContext,
                           nsIViewManager* aViewManager,
                           nsIStyleSet* aStyleSet,
                           nsIPresShell** aInstancePtrResult);

    virtual PRBool DeleteShell(nsIPresShell* aShell);

    virtual PRInt32 GetNumberOfShells();

    virtual nsIPresShell* GetShellAt(PRInt32 aIndex);

    virtual nsIDocument* GetParentDocument();

    virtual void SetParentDocument(nsIDocument* aParent);

    virtual void AddSubDocument(nsIDocument* aSubDoc);

    virtual PRInt32 GetNumberOfSubDocuments();

    virtual nsIDocument* GetSubDocumentAt(PRInt32 aIndex);

    virtual nsIContent* GetRootContent();

    virtual void SetRootContent(nsIContent* aRoot);

    NS_IMETHOD AppendToProlog(nsIContent* aContent);
    NS_IMETHOD AppendToEpilog(nsIContent* aContent);
    NS_IMETHOD ChildAt(PRInt32 aIndex, nsIContent*& aResult) const;
    NS_IMETHOD IndexOf(nsIContent* aPossibleChild, PRInt32& aIndex) const;
    NS_IMETHOD GetChildCount(PRInt32& aCount);

    virtual PRInt32 GetNumberOfStyleSheets();

    virtual nsIStyleSheet* GetStyleSheetAt(PRInt32 aIndex);

    virtual PRInt32 GetIndexOfStyleSheet(nsIStyleSheet* aSheet);

    virtual void AddStyleSheet(nsIStyleSheet* aSheet);
    virtual void RemoveStyleSheet(nsIStyleSheet* aSheet);
    NS_IMETHOD InsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex, PRBool aNotify);

    virtual void SetStyleSheetDisabledState(nsIStyleSheet* aSheet,
                                            PRBool mDisabled);

    NS_IMETHOD GetCSSLoader(nsICSSLoader*& aLoader);

    virtual nsIScriptContextOwner *GetScriptContextOwner();

    virtual void SetScriptContextOwner(nsIScriptContextOwner *aScriptContextOwner);

    NS_IMETHOD GetNameSpaceManager(nsINameSpaceManager*& aManager);

    virtual void AddObserver(nsIDocumentObserver* aObserver);

    virtual PRBool RemoveObserver(nsIDocumentObserver* aObserver);

    NS_IMETHOD BeginLoad();

    NS_IMETHOD EndLoad();

    NS_IMETHOD ContentChanged(nsIContent* aContent,
                              nsISupports* aSubContent);

    NS_IMETHOD ContentStatesChanged(nsIContent* aContent1, nsIContent* aContent2);

    NS_IMETHOD AttributeChanged(nsIContent* aChild,
                                PRInt32 aNameSpaceID,
                                nsIAtom* aAttribute,
                                PRInt32 aHint); // See nsStyleConsts fot hint values

    NS_IMETHOD ContentAppended(nsIContent* aContainer,
                               PRInt32 aNewIndexInContainer);

    NS_IMETHOD ContentInserted(nsIContent* aContainer,
                               nsIContent* aChild,
                               PRInt32 aIndexInContainer);

    NS_IMETHOD ContentReplaced(nsIContent* aContainer,
                               nsIContent* aOldChild,
                               nsIContent* aNewChild,
                               PRInt32 aIndexInContainer);

    NS_IMETHOD ContentRemoved(nsIContent* aContainer,
                              nsIContent* aChild,
                              PRInt32 aIndexInContainer);

    NS_IMETHOD StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule,
                                PRInt32 aHint); // See nsStyleConsts fot hint values

    NS_IMETHOD StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule);

    NS_IMETHOD StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule);

    NS_IMETHOD GetSelection(nsIDOMSelection** aSelection);

    NS_IMETHOD SelectAll();

    NS_IMETHOD FindNext(const nsString &aSearchStr, PRBool aMatchCase, PRBool aSearchDown, PRBool &aIsFound);

    NS_IMETHOD CreateXIF(nsString & aBuffer, nsIDOMSelection* aSelection);

    NS_IMETHOD ToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode);

    virtual void BeginConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode);

    virtual void ConvertChildrenToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode);

    virtual void FinishConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode);

    virtual PRBool IsInRange(const nsIContent *aStartContent, const nsIContent* aEndContent, const nsIContent* aContent) const;

    virtual PRBool IsBefore(const nsIContent *aNewContent, const nsIContent* aCurrentContent) const;

    virtual PRBool IsInSelection(nsIDOMSelection* aSelection, const nsIContent *aContent) const;

    virtual nsIContent* GetPrevContent(const nsIContent *aContent) const;

    virtual nsIContent* GetNextContent(const nsIContent *aContent) const;

    virtual void SetDisplaySelection(PRBool aToggle);

    virtual PRBool GetDisplaySelection() const;

    NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext,
                              nsEvent* aEvent,
                              nsIDOMEvent** aDOMEvent,
                              PRUint32 aFlags,
                              nsEventStatus* aEventStatus);


    // nsIXMLDocument interface
    NS_IMETHOD GetContentById(const nsString& aName, nsIContent** aContent);
#ifdef XSL
    NS_IMETHOD SetTransformMediator(nsITransformMediator* aMediator);
#endif

    // nsIXULDocument interface
    NS_IMETHOD AddElementForID(const nsString& aID, nsIContent* aElement);
    NS_IMETHOD RemoveElementForID(const nsString& aID, nsIContent* aElement);
    NS_IMETHOD GetElementsForID(const nsString& aID, nsISupportsArray* aElements);
    NS_IMETHOD CreateContents(nsIContent* aElement);
    NS_IMETHOD AddContentModelBuilder(nsIRDFContentModelBuilder* aBuilder);
    NS_IMETHOD GetForm(nsIDOMHTMLFormElement** aForm);
    NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm);
    NS_IMETHOD AddForwardReference(nsForwardReference* aRef);
    NS_IMETHOD ResolveForwardReferences();

    NS_IMETHOD CreateFromPrototype(const char* aCommand,
                                   nsIXULPrototypeDocument* aPrototype);

    // nsIStreamLoadableDocument interface
    NS_IMETHOD LoadFromStream(nsIInputStream& xulStream,
                              nsISupports* aContainer,
                              const char* aCommand );

    // nsIDOMEventCapturer interface
    NS_IMETHOD    CaptureEvent(const nsString& aType);
    NS_IMETHOD    ReleaseEvent(const nsString& aType);

    // nsIDOMEventReceiver interface (yuck. inherited from nsIDOMEventCapturer)
    NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID);
    NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID);
    NS_IMETHOD GetListenerManager(nsIEventListenerManager** aInstancePtrResult);
    NS_IMETHOD GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult);

    // nsIDOMEventTarget interface
    NS_IMETHOD AddEventListener(const nsString& aType, nsIDOMEventListener* aListener,
                                PRBool aUseCapture);
    NS_IMETHOD RemoveEventListener(const nsString& aType, nsIDOMEventListener* aListener,
                                   PRBool aUseCapture);

    // nsIDOMDocument interface
    NS_IMETHOD    GetDoctype(nsIDOMDocumentType** aDoctype);
    NS_IMETHOD    GetImplementation(nsIDOMDOMImplementation** aImplementation);
    NS_IMETHOD    GetDocumentElement(nsIDOMElement** aDocumentElement);

    NS_IMETHOD    CreateElement(const nsString& aTagName, nsIDOMElement** aReturn);
    NS_IMETHOD    CreateDocumentFragment(nsIDOMDocumentFragment** aReturn);
    NS_IMETHOD    CreateTextNode(const nsString& aData, nsIDOMText** aReturn);
    NS_IMETHOD    CreateComment(const nsString& aData, nsIDOMComment** aReturn);
    NS_IMETHOD    CreateCDATASection(const nsString& aData, nsIDOMCDATASection** aReturn);
    NS_IMETHOD    CreateProcessingInstruction(const nsString& aTarget, const nsString& aData, nsIDOMProcessingInstruction** aReturn);
    NS_IMETHOD    CreateAttribute(const nsString& aName, nsIDOMAttr** aReturn);
    NS_IMETHOD    CreateEntityReference(const nsString& aName, nsIDOMEntityReference** aReturn);
    NS_IMETHOD    GetElementsByTagName(const nsString& aTagname, nsIDOMNodeList** aReturn);

    // nsIDOMNSDocument interface
    NS_IMETHOD    GetStyleSheets(nsIDOMStyleSheetCollection** aStyleSheets);
    NS_IMETHOD    CreateElementWithNameSpace(const nsString& aTagName, const nsString& aNameSpace, nsIDOMElement** aResult);
    NS_IMETHOD    CreateRange(nsIDOMRange** aRange);
    NS_IMETHOD    GetWidth(PRInt32* aWidth);
    NS_IMETHOD    GetHeight(PRInt32* aHeight);

    // nsIDOMXULDocument interface
    NS_DECL_IDOMXULDOCUMENT

    // nsIDOMNode interface
    NS_IMETHOD    GetNodeName(nsString& aNodeName);
    NS_IMETHOD    GetNodeValue(nsString& aNodeValue);
    NS_IMETHOD    SetNodeValue(const nsString& aNodeValue);
    NS_IMETHOD    GetNodeType(PRUint16* aNodeType);
    NS_IMETHOD    GetParentNode(nsIDOMNode** aParentNode);
    NS_IMETHOD    GetChildNodes(nsIDOMNodeList** aChildNodes);
    NS_IMETHOD    HasChildNodes(PRBool* aHasChildNodes);
    NS_IMETHOD    GetFirstChild(nsIDOMNode** aFirstChild);
    NS_IMETHOD    GetLastChild(nsIDOMNode** aLastChild);
    NS_IMETHOD    GetPreviousSibling(nsIDOMNode** aPreviousSibling);
    NS_IMETHOD    GetNextSibling(nsIDOMNode** aNextSibling);
    NS_IMETHOD    GetAttributes(nsIDOMNamedNodeMap** aAttributes);
    NS_IMETHOD    GetOwnerDocument(nsIDOMDocument** aOwnerDocument);
    NS_IMETHOD    InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn);
    NS_IMETHOD    ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn);
    NS_IMETHOD    RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);
    NS_IMETHOD    AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn);
    NS_IMETHOD    CloneNode(PRBool aDeep, nsIDOMNode** aReturn);

    // nsIJSScriptObject interface
    virtual PRBool    AddProperty(JSContext *aContext, jsval aID, jsval *aVp);
    virtual PRBool    DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp);
    virtual PRBool    GetProperty(JSContext *aContext, jsval aID, jsval *aVp);
    virtual PRBool    SetProperty(JSContext *aContext, jsval aID, jsval *aVp);
    virtual PRBool    EnumerateProperty(JSContext *aContext);
    virtual PRBool    Resolve(JSContext *aContext, jsval aID);
    virtual PRBool    Convert(JSContext *aContext, jsval aID);
    virtual void      Finalize(JSContext *aContext);

    // nsIScriptObjectOwner interface
    NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
    NS_IMETHOD SetScriptObject(void *aScriptObject);

    // nsIHTMLContentContainer interface
    NS_IMETHOD GetAttributeStyleSheet(nsIHTMLStyleSheet** aResult);
    NS_IMETHOD GetInlineStyleSheet(nsIHTMLCSSStyleSheet** aResult);

protected:
    // Implementation methods
    friend nsresult
    NS_NewXULDocument(nsIXULDocument** aResult);

    nsresult Init(void);
    nsresult StartLayout(void);

    nsresult OpenWidgetItem(nsIContent* aElement);
    nsresult CloseWidgetItem(nsIContent* aElement);
    nsresult RebuildWidgetItem(nsIContent* aElement);

    nsresult
    AddSubtreeToDocument(nsIContent* aElement);

    nsresult
    RemoveSubtreeFromDocument(nsIContent* aElement);

    nsresult
    AddElementToMap(nsIContent* aElement);

    nsresult
    RemoveElementFromMap(nsIContent* aElement);

    static PRIntn
    RemoveElementsFromMapByContent(const nsString& aID,
                                   nsIContent* aElement,
                                   void* aClosure);

    static nsresult
    GetElementsByTagName(nsIDOMNode* aNode,
                         const nsString& aTagName,
                         nsRDFDOMNodeList* aElements);

    static nsresult
    GetElementsByAttribute(nsIDOMNode* aNode,
                           const nsString& aAttribute,
                           const nsString& aValue,
                           nsRDFDOMNodeList* aElements);

    nsresult
    ParseTagString(const nsString& aTagName, nsIAtom*& aName, PRInt32& aNameSpaceID);

    NS_IMETHOD PrepareStyleSheets(nsIURI* anURL);

    void SetDocumentURLAndGroup(nsIURI* anURL);
    void SetIsPopup(PRBool isPopup) { mIsPopup = isPopup; };

    nsresult CreateElement(PRInt32 aNameSpaceID,
                           nsIAtom* aTag,
                           nsIContent** aResult);

    nsresult PrepareToLoad(nsISupports* aContainer,
                           const char* aCommand,
                           nsIChannel* aChannel,
                           nsILoadGroup* aLoadGroup,
                           nsIParser** aResult);

    nsresult
    PrepareToLoadPrototype(nsIURI* aURI,
                           const char* aCommand,
                           nsIPrincipal* aDocumentPrincipal,
                           nsIParser** aResult);

    nsresult ApplyPersistentAttributes();
    nsresult ApplyPersistentAttributesToElements(nsIRDFResource* aResource, nsISupportsArray* aElements);

protected:
    // pseudo constants
    static PRInt32 gRefCnt;

    static nsIAtom*  kAttributeAtom;
    static nsIAtom*  kCommandUpdaterAtom;
    static nsIAtom*  kDataSourcesAtom;
    static nsIAtom*  kElementAtom;
    static nsIAtom*  kIdAtom;
    static nsIAtom*  kKeysetAtom;
    static nsIAtom*  kObservesAtom;
    static nsIAtom*  kOpenAtom;
    static nsIAtom*  kOverlayAtom;
    static nsIAtom*  kPersistAtom;
    static nsIAtom*  kPositionAtom;
    static nsIAtom*  kRefAtom;
    static nsIAtom*  kRuleAtom;
    static nsIAtom*  kTemplateAtom;

    static nsIAtom** kIdentityAttrs[];

    static nsIRDFService* gRDFService;
    static nsIRDFResource* kNC_persist;
    static nsIRDFResource* kNC_attribute;
    static nsIRDFResource* kNC_value;

    static nsIHTMLElementFactory* gHTMLElementFactory;
    static nsIXMLElementFactory* gXMLElementFactory;

    static nsINameSpaceManager* gNameSpaceManager;
    static PRInt32 kNameSpaceID_XUL;

    static nsIXULContentUtils* gXULUtils;
    static nsIXULPrototypeCache* gXULCache;

    static PRLogModuleInfo* gXULLog;

    nsIContent*
    FindContent(const nsIContent* aStartNode,
                const nsIContent* aTest1,
                const nsIContent* aTest2) const;

    nsresult
    Persist(nsIContent* aElement, PRInt32 aNameSpaceID, nsIAtom* aAttribute);

    nsresult
    DestroyForwardReferences();

    // IMPORTANT: The ownership implicit in the following member variables has been
    // explicitly checked and set using nsCOMPtr for owning pointers and raw COM interface
    // pointers for weak (ie, non owning) references. If you add any members to this
    // class, please make the ownership explicit (pinkerton, scc).
    // NOTE, THIS IS STILL IN PROGRESS, TALK TO PINK OR SCC BEFORE CHANGING

    nsCOMPtr<nsIArena>         mArena;
    nsVoidArray                mObservers;
    nsAutoString               mDocumentTitle;
    nsCOMPtr<nsIURI>           mDocumentURL;        // [OWNER] ??? compare with loader
    nsWeakPtr                  mDocumentLoadGroup;  // [WEAK] leads to loader
    nsCOMPtr<nsIPrincipal>     mDocumentPrincipal;  // [OWNER]
    nsCOMPtr<nsIContent>       mRootContent;        // [OWNER]
    nsIDocument*               mParentDocument;     // [WEAK]
    nsIScriptContextOwner*     mScriptContextOwner; // [WEAK] it owns me! (indirectly)
    void*                      mScriptObject;       // ????
    nsXULDocument*             mNextSrcLoadWaiter;  // [OWNER] but not COMPtr
    nsString                   mCharSetID;
    nsVoidArray                mStyleSheets;
    nsCOMPtr<nsIDOMSelection>  mSelection;          // [OWNER]
    PRBool                     mDisplaySelection;
    nsVoidArray                mPresShells;
    nsCOMPtr<nsIEventListenerManager> mListenerManager;   // [OWNER]
    nsCOMPtr<nsINameSpaceManager>     mNameSpaceManager;  // [OWNER]
    nsCOMPtr<nsIHTMLStyleSheet>       mAttrStyleSheet;    // [OWNER]
    nsCOMPtr<nsIHTMLCSSStyleSheet>    mInlineStyleSheet;  // [OWNER]
    nsCOMPtr<nsICSSLoader>            mCSSLoader;         // [OWNER]
    nsElementMap               mElementMap;
    nsCOMPtr<nsISupportsArray> mBuilders;        // [OWNER] of array, elements shouldn't own this, but they do
    nsCOMPtr<nsIRDFDataSource>          mLocalStore;
    nsCOMPtr<nsILineBreaker>            mLineBreaker;    // [OWNER] 
    nsCOMPtr<nsIWordBreaker>            mWordBreaker;    // [OWNER] 
    nsString                   mCommand;
    nsVoidArray                mSubDocuments;     // [OWNER] of subelements
    PRBool                     mIsPopup;
    nsCOMPtr<nsIDOMHTMLFormElement>     mHiddenForm;   // [OWNER] of this content element
    nsCOMPtr<nsIDOMXULCommandDispatcher>     mCommandDispatcher; // [OWNER] of the focus tracker

    nsVoidArray mForwardReferences;
    nsForwardReference::Phase mResolutionPhase;

    // The following are pointers into the content model which provide access to
    // the objects triggering either a popup or a tooltip. These are marked as
    // [OWNER] only because someone could, through DOM calls, delete the object from the
    // content model while the popup/tooltip was visible. If we didn't have a reference
    // to it, the object would go away and we'd be left pointing to garbage. This
    // does not introduce cycles into the ownership model because this is still
    // parent/child ownership. Just wanted the reader to know hyatt and I had thought about
    // this (pinkerton).
    nsCOMPtr<nsIDOMNode>    mPopupNode;            // [OWNER] element triggering the popup
    nsCOMPtr<nsIDOMNode>    mTooltipNode;          // [OWNER] element triggering the tooltip

    /**
     * Context stack, which maintains the state of the Builder and allows
     * it to be interrupted.
     */
    class ContextStack {
    protected:
        struct Entry {
            nsXULPrototypeElement* mPrototype;
            nsIContent*            mElement;
            PRInt32                mIndex;
            Entry*                 mNext;
        };

        Entry* mTop;
        PRInt32 mDepth;

    public:
        ContextStack();
        ~ContextStack();

        PRInt32 Depth() { return mDepth; }

        nsresult Push(nsXULPrototypeElement* aPrototype, nsIContent* aElement);
        nsresult Pop();
        nsresult Peek(nsXULPrototypeElement** aPrototype, nsIContent** aElement, PRInt32* aIndex);

        nsresult SetTopIndex(PRInt32 aIndex);

        PRBool IsInsideXULTemplate();
    };

    friend class ContextStack;
    ContextStack mContextStack;

    enum State { eState_Master, eState_Overlay };
    State mState;

    /**
     * An array of overlay nsIURIs that have yet to be resolved. The
     * order of the array is significant: overlays at the _end_ of the
     * array are resolved before overlays earlier in the array (i.e.,
     * it is a stack).
     */
    nsCOMPtr<nsISupportsArray> mUnloadedOverlays;

    /**
     * Load the transcluded script at the specified URI. If the
     * prototype construction must 'block' until the load has
     * completed, aBlock will be set to true.
     */
    nsresult LoadScript(nsXULPrototypeScript *aScriptProto, PRBool* aBlock);

    /**
     * Execute the precompiled script object scoped by this XUL document's
     * containing window object, and using its associated script context.
     */
    nsresult ExecuteScript(JSObject* aScriptObject);

    /**
     * Create a delegate content model element from a prototype.
     */
    nsresult CreateElement(nsXULPrototypeElement* aPrototype, nsIContent** aResult);

    /**
     * Create a temporary 'overlay' element to which content nodes
     * can be attached for later resolution.
     */
    nsresult CreateOverlayElement(nsXULPrototypeElement* aPrototype, nsIContent** aResult);

    /**
     * Add attributes from the prototype to the element.
     */
    nsresult AddAttributes(nsXULPrototypeElement* aPrototype, nsIContent* aElement);

    /**
     * The prototype-script of the current transcluded script that is being
     * loaded.  For document.write('<script src="nestedwrite.js"><\/script>')
     * to work, these need to be in a stack element type, and we need to hold
     * the top of stack here.
     */
    nsXULPrototypeScript* mCurrentScriptProto;

	/**
	 * A "dummy" channel that is used as a placeholder to signal document load
	 * completion.
	 */
	nsCOMPtr<nsIChannel> mPlaceholderChannel;
	
    /**
     * Create a XUL template builder on the specified node if a 'datasources'
     * attribute is present.
     */
    static nsresult
    CheckTemplateBuilder(nsIContent* aElement);

    /**
     * Do hookup for <xul:observes> tag
     */
    nsresult HookupObserver(nsIContent* aElement);

    /**
     * Add the current prototype's style sheets to the document.
     */
    nsresult AddPrototypeSheets();

    /**
     * Used to resolve broadcaster references
     */
    class BroadcasterHookup : public nsForwardReference
    {
    protected:
        nsXULDocument* mDocument;              // [WEAK]
        nsCOMPtr<nsIContent> mObservesElement; // [OWNER]
        PRBool mResolved;

    public:
        BroadcasterHookup(nsXULDocument* aDocument, nsIContent* aObservesElement) :
            mDocument(aDocument),
            mObservesElement(aObservesElement),
            mResolved(PR_FALSE) {}

        virtual ~BroadcasterHookup();

        virtual Phase GetPhase() { return eHookup; }
        virtual Result Resolve();
    };

    friend class BroadcasterHookup;


    /**
     * Used to hook up overlays
     */
    class OverlayForwardReference : public nsForwardReference
    {
    protected:
        nsXULDocument* mDocument;      // [WEAK]
        nsCOMPtr<nsIContent> mOverlay; // [OWNER]
        PRBool mResolved;

        nsresult Merge(nsIContent* aTargetNode, nsIContent* aOverlayNode);

    public:
        OverlayForwardReference(nsXULDocument* aDocument, nsIContent* aOverlay)
            : mDocument(aDocument), mOverlay(aOverlay), mResolved(PR_FALSE) {}

        virtual ~OverlayForwardReference();

        virtual Phase GetPhase() { return eConstruction; }
        virtual Result Resolve();
    };

    friend class OverlayForwardReference;


    static
    nsresult
    CheckBroadcasterHookup(nsXULDocument* aDocument,
                           nsIContent* aElement,
                           PRBool* aNeedsHookup,
                           PRBool* aDidResolve);

    static
    nsresult
    InsertElement(nsIContent* aParent, nsIContent* aChild);

    static
    PRBool
    IsChromeURI(nsIURI* aURI);

    /**
     * The current prototype that we are walking to construct the
     * content model.
     */
    nsCOMPtr<nsIXULPrototypeDocument> mCurrentPrototype;

    /**
     * The master document (outermost, .xul) prototype, from which
     * all subdocuments get their security principals.
     */
    nsCOMPtr<nsIXULPrototypeDocument> mMasterPrototype;

    /**
     * Owning references to all of the prototype documents that were
     * used to construct this document.
     */
    nsCOMPtr<nsISupportsArray> mPrototypes;

    /**
     * Prepare to walk the current prototype.
     */
    nsresult PrepareToWalk();

    /**
     * Add overlays from the chrome registry to the set of unprocessed
     * overlays still to do.
     */
    nsresult AddChromeOverlays();

    /**
     * Resume (or initiate) an interrupted (or newly prepared)
     * prototype walk.
     */
    nsresult ResumeWalk();

#if defined(DEBUG_waterson) || defined(DEBUG_hyatt)
    // timing
    nsTime mLoadStart;
#endif

    class CachedChromeLoader : public nsIStreamObserver
    {
    protected:
        nsXULDocument* mDocument;
        PRBool mLoading;

    public:
        CachedChromeLoader(nsXULDocument* aDocument);
        virtual ~CachedChromeLoader();

        // nsISupports interface
        NS_DECL_ISUPPORTS

        // nsIStreamObserver interface
        NS_DECL_NSISTREAMOBSERVER
    };

    friend class CachedChromeLoader;
};



#endif // nsXULDocument_h__
