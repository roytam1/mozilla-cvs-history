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
 *   IBM Corp.
 */
#ifndef nsHTMLDocument_h___
#define nsHTMLDocument_h___

#include "nsDocument.h"
#include "nsMarkupDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIHTMLContentContainer.h"
#include "nsHashtable.h"
#include "jsapi.h"
#include "rdf.h"
#include "nsRDFCID.h"
#include "nsIRDFService.h"

class nsBaseContentList;
class nsContentList;
class nsIHTMLStyleSheet;
class nsIHTMLCSSStyleSheet;
class nsIParser;
class nsICSSLoader;

class nsHTMLDocument : public nsMarkupDocument,
                       public nsIHTMLDocument,
                       public nsIDOMHTMLDocument,
                       public nsIDOMNSHTMLDocument,
                       public nsIHTMLContentContainer
{
public:
  nsHTMLDocument();
  virtual ~nsHTMLDocument();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  NS_IMETHOD Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup);

  NS_IMETHOD GetContentType(nsAWritableString& aContentType) const;

  NS_IMETHOD CreateShell(nsIPresContext* aContext,
                         nsIViewManager* aViewManager,
                         nsIStyleSet* aStyleSet,
                         nsIPresShell** aInstancePtrResult);

  NS_IMETHOD StartDocumentLoad(const char* aCommand,
                               nsIChannel* aChannel,
                               nsILoadGroup* aLoadGroup,
                               nsISupports* aContainer,
                               nsIStreamListener **aDocListener,
                               PRBool aReset = PR_TRUE);

  NS_IMETHOD StopDocumentLoad();

  NS_IMETHOD EndLoad();

  NS_IMETHOD AddImageMap(nsIDOMHTMLMapElement* aMap);

  NS_IMETHOD RemoveImageMap(nsIDOMHTMLMapElement* aMap);

  NS_IMETHOD GetImageMap(const nsString& aMapName,
                         nsIDOMHTMLMapElement** aResult);

  NS_IMETHOD GetAttributeStyleSheet(nsIHTMLStyleSheet** aStyleSheet);
  NS_IMETHOD GetInlineStyleSheet(nsIHTMLCSSStyleSheet** aStyleSheet);
  NS_IMETHOD GetCSSLoader(nsICSSLoader*& aLoader);

  NS_IMETHOD GetBaseURL(nsIURI*& aURL) const;
  NS_IMETHOD GetBaseTarget(nsAWritableString& aTarget);
  NS_IMETHOD SetBaseTarget(const nsAReadableString& aTarget);

  NS_IMETHOD SetLastModified(const nsAReadableString& aLastModified);
  NS_IMETHOD SetReferrer(const nsAReadableString& aReferrer);

  NS_IMETHOD GetDTDMode(nsDTDMode& aMode);
  NS_IMETHOD SetDTDMode(nsDTDMode aMode);

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
  NS_IMETHOD AttributeChanged(nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aHint);
  NS_IMETHOD AttributeWillChange(nsIContent* aChild,
                                 PRInt32 aNameSpaceID,
                                 nsIAtom* aAttribute);

  NS_IMETHOD FlushPendingNotifications(PRBool aFlushReflows = PR_TRUE);

  // nsIDOMDocument interface
  NS_DECL_NSIDOMDOCUMENT

  // nsIDOMNode interface
  NS_DECL_NSIDOMNODE

  // nsIDOM3Node interface
  NS_DECL_NSIDOM3NODE

  // nsIDOMHTMLDocument interface
  NS_DECL_NSIDOMHTMLDOCUMENT

  // nsIDOMNSHTMLDocument interface
  NS_DECL_NSIDOMNSHTMLDOCUMENT

  /*
   * Returns true if document.domain was set for this document
   */
  NS_IMETHOD WasDomainSet(PRBool* aDomainWasSet);

  NS_IMETHOD ResolveName(const nsAReadableString& aName,
                         nsIDOMHTMLFormElement *aForm,
                         nsISupports **aResult);

protected:
  nsresult GetPixelDimensions(nsIPresShell* aShell,
                              PRInt32* aWidth,
                              PRInt32* aHeight);

  nsresult RegisterNamedItems(nsIContent *aContent);
  nsresult UnregisterNamedItems(nsIContent *aContent);
  nsresult AddToNameTable(const nsAReadableString& aName,
                          nsIContent *aContent);
  nsresult AddToIdTable(const nsAReadableString& aId, nsIContent *aContent,
                        PRBool aPutInTable);
  nsresult RemoveFromNameTable(const nsAReadableString& aName,
                               nsIContent *aContent);
  nsresult RemoveFromIdTable(nsIContent *aContent);

  void InvalidateHashTables();
  nsresult PrePopulateHashTables();

  nsIContent *MatchId(nsIContent *aContent, const nsAReadableString& aId);
  void FindNamedItems(const nsAReadableString& aName, nsIContent *aContent,
                      nsBaseContentList& aList);

  virtual void InternalAddStyleSheet(nsIStyleSheet* aSheet);
  virtual void InternalInsertStyleSheetAt(nsIStyleSheet* aSheet,
                                          PRInt32 aIndex);
  static PRBool MatchLinks(nsIContent *aContent, nsString* aData);
  static PRBool MatchAnchors(nsIContent *aContent, nsString* aData);
  static PRBool MatchLayers(nsIContent *aContent, nsString* aData);
  static PRBool MatchNameAttribute(nsIContent* aContent, nsString* aData);

  nsresult GetSourceDocumentURL(JSContext* cx, nsIURI** sourceURL);

  PRBool GetBodyContent();
  nsresult GetBodyElement(nsIDOMHTMLBodyElement** aBody);

  NS_IMETHOD GetDomainURI(nsIURI **uri);

  nsresult WriteCommon(const nsAReadableString& aText,
                       PRBool aNewlineTerminate);
  nsresult ScriptWriteCommon(PRBool aNewlineTerminate);
  nsresult OpenCommon(nsIURI* aUrl);

  nsIHTMLStyleSheet*    mAttrStyleSheet;
  nsIHTMLCSSStyleSheet* mStyleAttrStyleSheet;
  nsIURI*     mBaseURL;
  nsString*   mBaseTarget;
  nsString*   mLastModified;
  nsString*   mReferrer;
  nsDTDMode mDTDMode;
  nsVoidArray mImageMaps;

  nsContentList *mImages;
  nsContentList *mApplets;
  nsContentList *mEmbeds;
  nsContentList *mLinks;
  nsContentList *mAnchors;
  nsContentList *mForms;
  nsContentList *mLayers;
  
  nsIParser *mParser;

//ahmed 12-2
#ifdef IBMBIDI
  PRInt32  mTexttype;
#endif
  
  static nsrefcnt gRefCntRDFService;
  static nsIRDFService* gRDF;

  PRUint32 mIsWriting : 1;
  PRUint32 mWriteLevel : 31;

  nsIDOMNode * mBodyContent;

  /*
   * Bug 13871: Frameset spoofing - find out if document.domain was set
   */
  PRBool       mDomainWasSet;

  nsHashtable mNameHashTable;
  nsHashtable mIdHashTable;
};

#endif /* nsHTMLDocument_h___ */
