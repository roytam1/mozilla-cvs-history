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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */
#include "nsCOMPtr.h"
#include "nsXMLContentSink.h"
#include "nsIElementFactory.h"
#include "nsIParser.h"
#include "nsIUnicharInputStream.h"
#include "nsIDocument.h"
#include "nsIXMLDocument.h"
#include "nsIXMLContent.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIURL.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIWebShell.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIContent.h"
#include "nsITextContent.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsIDOMComment.h"
#include "nsIDOMCDATASection.h"
#include "nsIHTMLContent.h"
#include "nsHTMLEntities.h"
#include "nsHTMLParts.h"
#include "nsVoidArray.h"
#include "nsCRT.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"
#include "nsIHTMLContentContainer.h"
#include "nsHTMLAtoms.h"
#include "nsLayoutAtoms.h"
#include "nsLayoutCID.h"
#include "nsIScriptContext.h"
#include "nsINameSpace.h"
#include "nsINameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIContentViewer.h"
#include "jsapi.h" // for JSVERSION_* and JS_VersionToString
#include "prtime.h"
#include "prlog.h"
#include "prmem.h"
#ifdef XSL
#include "nsXSLContentSink.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsISupports.h"
#include "nsParserCIID.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#endif

// XXX misnamed header file, but oh well
#include "nsHTMLTokens.h"

static char kNameSpaceSeparator = ':';
static char kNameSpaceDef[] = "xmlns";
static char kStyleSheetPI[] = "xml-stylesheet";

#ifdef XSL
static char kXSLType[] = "text/xsl";
#endif

static NS_DEFINE_CID(kNameSpaceManagerCID,       NS_NAMESPACEMANAGER_CID);

static NS_DEFINE_IID(kIXMLContentSinkIID, NS_IXMLCONTENT_SINK_IID);
static NS_DEFINE_IID(kIXMLContentIID, NS_IXMLCONTENT_IID);
static NS_DEFINE_IID(kIHTMLContentContainerIID, NS_IHTMLCONTENTCONTAINER_IID);
static NS_DEFINE_IID(kIXMLDocumentIID, NS_IXMLDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMCommentIID, NS_IDOMCOMMENT_IID);
static NS_DEFINE_IID(kIScrollableViewIID, NS_ISCROLLABLEVIEW_IID);
static NS_DEFINE_IID(kIDOMNodeIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDOMCDATASectionIID, NS_IDOMCDATASECTION_IID);
#ifdef XSL
static NS_DEFINE_IID(kIDOMDocumentIID, NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMElementIID, NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIContentIID, NS_ICONTENT_IID);
static NS_DEFINE_IID(kIObserverIID, NS_IOBSERVER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_IID(kIStreamListenerIID, NS_ISTREAMLISTENER_IID);
#endif

nsINameSpaceManager* nsXMLContentSink::gNameSpaceManager = nsnull;
PRUint32 nsXMLContentSink::gRefCnt = 0;

// XXX Open Issues:
// 1) html:style - Should we allow inline style? If so, the content
//    sink needs to process the tag and invoke the CSS parser.
// 2) html:base - Should we allow a base tag? If so, the content
//    sink needs to maintain the base when resolving URLs for
//    loaded scripts and style sheets. Should it be allowed anywhere?
// 3) what's not allowed - We need to figure out which HTML tags
//    (prefixed with a HTML namespace qualifier) are explicitly not
//    allowed (if any).
// 4) factoring code with nsHTMLContentSink - There's some amount of
//    common code between this and the HTML content sink. This will
//    increase as we support more and more HTML elements. How can code
//    from the code be factored?

nsresult
NS_NewXMLContentSink(nsIXMLContentSink** aResult,
                     nsIDocument* aDoc,
                     nsIURI* aURL,
                     nsIWebShell* aWebShell)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsXMLContentSink* it;
  NS_NEWXPCOM(it, nsXMLContentSink);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult rv = it->Init(aDoc, aURL, aWebShell);
  if (NS_OK != rv) {
    delete it;
    return rv;
  }
  return it->QueryInterface(kIXMLContentSinkIID, (void **)aResult);
}

nsXMLContentSink::nsXMLContentSink()
{
  NS_INIT_REFCNT();
  gRefCnt++;
  if (gRefCnt == 1) {
     nsresult rv = nsServiceManager::GetService(kNameSpaceManagerCID,
                                          NS_GET_IID(nsINameSpaceManager),
                                          (nsISupports**) &gNameSpaceManager);
     NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get namespace manager");
  }

  mDocument = nsnull;
  mDocumentURL = nsnull;
  mDocumentBaseURL = nsnull;
  mWebShell = nsnull;
  mParser = nsnull;
  mRootElement = nsnull;
  mDocElement = nsnull;
  mContentStack = nsnull;
  mNameSpaceStack = nsnull;
  mText = nsnull;
  mTextLength = 0;
  mTextSize = 0;
  mConstrainSize = PR_TRUE;
  mInScript = PR_FALSE;
  mStyleSheetCount = 0;
  mCSSLoader       = nsnull;
#ifdef XSL
  mXSLTransformMediator = nsnull;
#endif
}

nsXMLContentSink::~nsXMLContentSink()
{
  gRefCnt--;
  if (gRefCnt == 0) {
    NS_IF_RELEASE(gNameSpaceManager);
  }

  NS_IF_RELEASE(mDocument);
  NS_IF_RELEASE(mDocumentURL);
  NS_IF_RELEASE(mDocumentBaseURL);
  NS_IF_RELEASE(mWebShell);
  NS_IF_RELEASE(mParser);
  NS_IF_RELEASE(mRootElement);
  NS_IF_RELEASE(mDocElement);
  if (nsnull != mNameSpaceStack) {
    // There shouldn't be any here except in an error condition
    PRInt32 index = mNameSpaceStack->Count();
    while (0 < index--) {
      nsINameSpace* nameSpace = (nsINameSpace*)mNameSpaceStack->ElementAt(index);
      NS_RELEASE(nameSpace);
    }
    delete mNameSpaceStack;
  }
  if (nsnull != mText) {
    PR_FREEIF(mText);
  }
  NS_IF_RELEASE(mCSSLoader);
#ifdef XSL
  NS_IF_RELEASE(mXSLTransformMediator);
#endif
}

nsresult
nsXMLContentSink::Init(nsIDocument* aDoc,
                       nsIURI* aURL,
                       nsIWebShell* aContainer)
{
  NS_PRECONDITION(nsnull != aDoc, "null ptr");
  NS_PRECONDITION(nsnull != aURL, "null ptr");
  if ((nsnull == aDoc) || (nsnull == aURL)) {
    return NS_ERROR_NULL_POINTER;
  }

  mDocument = aDoc;
  NS_ADDREF(aDoc);
  mDocumentURL = aURL;
  NS_ADDREF(aURL);
  mDocumentBaseURL = aURL;
  NS_ADDREF(aURL);
  mWebShell = aContainer;
  NS_IF_ADDREF(aContainer);

  mState = eXMLContentSinkState_InProlog;
  mDocElement = nsnull;
  mRootElement = nsnull;

  // XXX this presumes HTTP header info is alread set in document
  // XXX if it isn't we need to set it here...
  mDocument->GetHeaderData(nsHTMLAtoms::headerDefaultStyle, mPreferredStyle);

  nsIHTMLContentContainer* htmlContainer = nsnull;
  if (NS_SUCCEEDED(aDoc->QueryInterface(kIHTMLContentContainerIID, (void**)&htmlContainer))) {
    htmlContainer->GetCSSLoader(mCSSLoader);
    NS_RELEASE(htmlContainer);
  }

  return aDoc->GetNodeInfoManager(*getter_AddRefs(mNodeInfoManager));
}

#ifndef XSL

NS_IMPL_ADDREF(nsXMLContentSink)
NS_IMPL_RELEASE(nsXMLContentSink)

#else

NS_IMPL_THREADSAFE_ADDREF(nsXMLContentSink)
NS_IMPL_THREADSAFE_RELEASE(nsXMLContentSink)

#endif

NS_INTERFACE_MAP_BEGIN(nsXMLContentSink)
	NS_INTERFACE_MAP_ENTRY(nsIXMLContentSink)
	NS_INTERFACE_MAP_ENTRY(nsIContentSink)
#ifdef XSL
	NS_INTERFACE_MAP_ENTRY(nsIObserver)
	NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
#endif
	NS_INTERFACE_MAP_ENTRY(nsIStreamLoaderObserver)
	NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXMLContentSink)
NS_INTERFACE_MAP_END

  // nsIContentSink
NS_IMETHODIMP
nsXMLContentSink::WillBuildModel(void)
{
  // Notify document that the load is beginning
  mDocument->BeginLoad();
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::DidBuildModel(PRInt32 aQualityLevel)
{
  // XXX this is silly; who cares?
  PRInt32 i, ns = mDocument->GetNumberOfShells();
  for (i = 0; i < ns; i++) {
    nsCOMPtr<nsIPresShell> shell( dont_AddRef(mDocument->GetShellAt(i)) );
    if (shell) {
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if(vm) {
        vm->SetQuality(nsContentQuality(aQualityLevel));
      }
    }
  }

#ifndef XSL
  StartLayoutProcess();
#else
  nsresult rv;
  if (mXSLTransformMediator) {
    rv = SetupTransformMediator();
  }

  if (!mXSLTransformMediator || NS_FAILED(rv)) {
    mDocument->SetRootContent(mDocElement);
    StartLayoutProcess();
  }
#endif

  // Drop our reference to the parser to get rid of a circular
  // reference.
  NS_IF_RELEASE(mParser);

  return NS_OK;
}

void
nsXMLContentSink::StartLayoutProcess()
{
  StartLayout();

  // XXX Should scroll to ref when that makes sense
  // ScrollToRef();

  mDocument->EndLoad();
}

#ifdef XSL
// The observe method is called on completion of the transform.  The nsISupports argument is an
// nsIDOMElement interface to the root node of the output content model.
NS_IMETHODIMP
nsXMLContentSink::Observe(nsISupports *aSubject, const PRUnichar *aTopic, const PRUnichar *someData)
{
  nsIContent* content;
  nsresult rv = NS_OK;

  // Set the output content model on the document
  rv = aSubject->QueryInterface(kIContentIID, (void **) &content);
  if (NS_SUCCEEDED(rv)) {
    mDocument->SetRootContent(content);
    NS_RELEASE(content);
  }
  else
    mDocument->SetRootContent(mDocElement);

  // Start the layout process
  StartLayoutProcess();

  // Reset the observer on the transform mediator
  mXSLTransformMediator->SetTransformObserver(nsnull);

  return rv;
}


// Provide the transform mediator with the source document's content
// model and the output document, and register the XML content sink
// as the transform observer.  The transform mediator will call
// the nsIObserver::Observe() method on the transform observer once
// the transform is completed.  The nsISupports pointer to the Observe
// method will be an nsIDOMElement pointer to the root node of the output
// content model.
nsresult
nsXMLContentSink::SetupTransformMediator()
{
  nsIDOMElement* source;
  nsIDOMDocument* currentDoc;
  nsresult rv = NS_OK;

  rv = mDocElement->QueryInterface(kIDOMElementIID, (void **) &source);
  if (NS_SUCCEEDED(rv)) {
    mXSLTransformMediator->SetSourceContentModel(source);
    rv = mDocument->QueryInterface(kIDOMDocumentIID, (void **) &currentDoc);
    if (NS_SUCCEEDED(rv)) {
      mXSLTransformMediator->SetCurrentDocument(currentDoc);
      mXSLTransformMediator->SetTransformObserver(this);
      NS_RELEASE(currentDoc);
    }
    NS_RELEASE(source);
  }

  return rv;
}
#endif

NS_IMETHODIMP
nsXMLContentSink::WillInterrupt(void)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::WillResume(void)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::SetParser(nsIParser* aParser)
{
  NS_IF_RELEASE(mParser);
  mParser = aParser;
  NS_IF_ADDREF(mParser);

  return NS_OK;
}

// XXX Code copied from nsHTMLContentSink. It should be shared.
void
nsXMLContentSink::GetAttributeValueAt(const nsIParserNode& aNode,
                                      PRInt32 aIndex,
                                      nsString& aResult)
{
  // Copy value
  const nsString& value = aNode.GetValueAt(aIndex);
  aResult.Truncate();
  aResult.Append(value);

  // Strip quotes if present
  PRUnichar first = aResult.First();
  if ((first == '\"') || (first == '\'')) {
    if (aResult.Last() == first) {
      aResult.Cut(0, 1);
      PRInt32 pos = aResult.Length() - 1;
      if (pos >= 0) {
        aResult.Cut(pos, 1);
      }
    } else {
      // Mismatched quotes - leave them in
    }
  }

  if (mParser) {
    nsCOMPtr<nsIDTD> dtd;

    nsresult rv = mParser->GetDTD(getter_AddRefs(dtd));

    if (NS_SUCCEEDED(rv)) {

      // Reduce any entities
      // XXX Note: as coded today, this will only convert well formed
      // entities.  This may not be compatible enough.
      // XXX there is a table in navigator that translates some numeric entities
      // should we be doing that? If so then it needs to live in two places (bad)
      // so we should add a translate numeric entity method from the parser...
      char cbuf[100];
      PRInt32 index = 0;
      while (index < aResult.Length()) {
        // If we have the start of an entity (and it's not at the end of
        // our string) then translate the entity into it's unicode value.
        if ((aResult.CharAt(index++) == '&') && (index < aResult.Length())) {
          PRInt32 start = index - 1;
          PRUnichar e = aResult.CharAt(index);
          if (e == '#') {
            // Convert a numeric character reference
            index++;
            char* cp = cbuf;
            char* limit = cp + sizeof(cbuf) - 1;
            PRBool ok = PR_FALSE;
            PRInt32 slen = aResult.Length();
            while ((index < slen) && (cp < limit)) {
              PRUnichar ch = aResult.CharAt(index);
              if (ch == ';') {
                index++;
                ok = PR_TRUE;
                break;
              }
              if ((ch >= '0') && (ch <= '9')) {
                *cp++ = char(ch);
                index++;
                continue;
              }
              break;
            }
            if (!ok || (cp == cbuf)) {
              continue;
            }
            *cp = '\0';
            if (cp - cbuf > 5) {
              continue;
            }
            PRInt32 ch = PRInt32( ::atoi(cbuf) );
            if (ch > 65535) {
              continue;
            }

            // Remove entity from string and replace it with the integer
            // value.
            aResult.Cut(start, index - start);
            aResult.Insert(PRUnichar(ch), start);
            index = start + 1;
          }
          else if (((e >= 'A') && (e <= 'Z')) ||
                   ((e >= 'a') && (e <= 'z'))) {
            // Convert a named entity
            index++;
            char* cp = cbuf;
            char* limit = cp + sizeof(cbuf) - 1;
            *cp++ = char(e);
            PRBool ok = PR_FALSE;
            PRInt32 slen = aResult.Length();
            while ((index < slen) && (cp < limit)) {
              PRUnichar ch = aResult.CharAt(index);
              if (ch == ';') {
                index++;
                ok = PR_TRUE;
                break;
              }
              if (((ch >= '0') && (ch <= '9')) ||
                  ((ch >= 'A') && (ch <= 'Z')) ||
                  ((ch >= 'a') && (ch <= 'z'))) {
                *cp++ = char(ch);
                index++;
                continue;
              }
              break;
            }
            if (!ok || (cp == cbuf)) {
              continue;
            }
            *cp = '\0';
            PRInt32 ch;
            nsAutoString str; str.AssignWithConversion(cbuf);
            dtd->ConvertEntityToUnicode(str, &ch);
            if (ch < 0) {
              continue;
            }

            // Remove entity from string and replace it with the integer
            // value.
            aResult.Cut(start, index - start);
            aResult.Insert(PRUnichar(ch), start);
            index = start + 1;
          }
          else if (e == '{') {
            // Convert a script entity
            // XXX write me!
          }
        }
      }
    }
  }
}

// XXX Code copied from nsHTMLContentSink. It should be shared.
nsresult
nsXMLContentSink::AddAttributes(const nsIParserNode& aNode,
                                nsIContent* aContent,
                                PRBool aIsHTML)
{
  // Add tag attributes to the content attributes
  nsAutoString name, v;
  PRInt32 ac = aNode.GetAttributeCount();
  for (PRInt32 i = 0; i < ac; i++) {
    // Get upper-cased key
    const nsString& key = aNode.GetKeyAt(i);
    name.Truncate();
    name.Append(key);

    nsIAtom* nameSpacePrefix = CutNameSpacePrefix(name);
    nsIAtom* nameAtom = NS_NewAtom(name);
    PRInt32 nameSpaceID = (nsnull == nameSpacePrefix) ? kNameSpaceID_None : GetNameSpaceId(nameSpacePrefix);
    if (kNameSpaceID_Unknown == nameSpaceID) {
      nameSpaceID = kNameSpaceID_None;  // XXX is this correct? or is it a bad document?
    }
    if ((kNameSpaceID_XMLNS == nameSpaceID) && aIsHTML) {
      NS_RELEASE(nameAtom);
      name.InsertWithConversion("xmlns:", 0);
      nameAtom = NS_NewAtom(name);
      nameSpaceID = kNameSpaceID_HTML;  // XXX this is wrong, but necessary until HTML can store other namespaces for attrs
    }

    nsAutoString value;
    if (NS_CONTENT_ATTR_NOT_THERE ==
        aContent->GetAttribute(nameSpaceID, nameAtom, value)) {
      // Get value and remove mandatory quotes
      GetAttributeValueAt(aNode, i, v);

      // Add attribute to content
      aContent->SetAttribute(nameSpaceID, nameAtom, v, PR_FALSE);
    }
    NS_RELEASE(nameAtom);
    NS_IF_RELEASE(nameSpacePrefix);
  }

  // Give autoloading links a chance to fire
  if (mWebShell) {
    nsCOMPtr<nsIXMLContent> xmlcontent(do_QueryInterface(aContent));
    if (xmlcontent) {
      nsresult rv = xmlcontent->MaybeTriggerAutoLink(mWebShell);
      if (rv == NS_XML_AUTOLINK_REPLACE ||
          rv == NS_XML_AUTOLINK_UNDEFINED) {
        // If we do not terminate the parse, we just keep generating link trigger
        // events. We want to parse only up to the first replace link, and stop.
        mParser->Terminate();
      }
    }
  }

  return NS_OK;
}

void
nsXMLContentSink::PushNameSpacesFrom(const nsIParserNode& aNode)
{
  nsAutoString k, uri, prefix;
  PRInt32 ac = aNode.GetAttributeCount();
  PRInt32 offset;
  nsINameSpace* nameSpace = nsnull;

  if ((nsnull != mNameSpaceStack) && (0 < mNameSpaceStack->Count())) {
    nameSpace = (nsINameSpace*)mNameSpaceStack->ElementAt(mNameSpaceStack->Count() - 1);
    NS_ADDREF(nameSpace);
  }
  else {
    nsINameSpaceManager* manager = nsnull;
    mDocument->GetNameSpaceManager(manager);
    NS_ASSERTION(nsnull != manager, "no name space manager in document");
    if (nsnull != manager) {
      manager->CreateRootNameSpace(nameSpace);
      NS_RELEASE(manager);
    }
  }

  if (nsnull != nameSpace) {
    for (PRInt32 i = 0; i < ac; i++) {
      const nsString& key = aNode.GetKeyAt(i);
      k.Truncate();
      k.Append(key);
      // Look for "xmlns" at the start of the attribute name
      offset = k.Find(kNameSpaceDef);
      if (0 == offset) {
        if (k.Length() == (sizeof(kNameSpaceDef)-1)) {
          // If there's nothing left, this is a default namespace
          prefix.Truncate();
        }
        else {
          PRUnichar next = k.CharAt(sizeof(kNameSpaceDef)-1);
          // If the next character is a :, there is a namespace prefix
          if (':' == next) {
            prefix.Truncate();
            k.Right(prefix, k.Length()-sizeof(kNameSpaceDef));
          }
          else {
            continue;
          }
        }

        // Get the attribute value (the URI for the namespace)
        GetAttributeValueAt(aNode, i, uri);

        // Open a local namespace
        nsIAtom* prefixAtom = ((0 < prefix.Length()) ? NS_NewAtom(prefix) : nsnull);
        nsINameSpace* child = nsnull;
        nameSpace->CreateChildNameSpace(prefixAtom, uri, child);
        if (nsnull != child) {
          NS_RELEASE(nameSpace);
          nameSpace = child;
        }
        NS_IF_RELEASE(prefixAtom);
      }
    }
    if (nsnull == mNameSpaceStack) {
      mNameSpaceStack = new nsVoidArray();
    }
    mNameSpaceStack->AppendElement(nameSpace);
  }
}

nsIAtom*  nsXMLContentSink::CutNameSpacePrefix(nsString& aString)
{
  nsAutoString  prefix;
  PRInt32 nsoffset = aString.FindChar(kNameSpaceSeparator);
  if (-1 != nsoffset) {
    aString.Left(prefix, nsoffset);
    aString.Cut(0, nsoffset+1);
  }
  if (0 < prefix.Length()) {
    return NS_NewAtom(prefix);
  }
  return nsnull;
}

NS_IMETHODIMP
nsXMLContentSink::OpenContainer(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;
  nsAutoString tag;
  nsCOMPtr<nsIAtom> nameSpacePrefix;
  PRInt32 nameSpaceID = kNameSpaceID_Unknown;
  PRBool isHTML = PR_FALSE;
  PRBool pushContent = PR_TRUE;
  nsCOMPtr<nsIContent> content;

  // XXX Hopefully the parser will flag this before we get
  // here. If we're in the epilog, there should be no
  // new elements
  PR_ASSERT(eXMLContentSinkState_InEpilog != mState);

  FlushText();

  mState = eXMLContentSinkState_InDocumentElement;

  tag.Assign(aNode.GetText());
  nameSpacePrefix = getter_AddRefs(CutNameSpacePrefix(tag));
  nsCOMPtr<nsIAtom> tagAtom = dont_AddRef(NS_NewAtom(tag));

  // We must register namespace declarations found in the attribute list
  // of an element before creating the element. This is because the
  // namespace prefix for an element might be declared within the attribute
  // list.
  PushNameSpacesFrom(aNode);

  nameSpaceID = GetNameSpaceId(nameSpacePrefix);

  nsCOMPtr<nsINodeInfo> nodeInfo;

  mNodeInfoManager->GetNodeInfo(tagAtom, nameSpacePrefix, nameSpaceID,
                                *getter_AddRefs(nodeInfo));

  isHTML = IsHTMLNameSpace(nameSpaceID);

  if (isHTML) {
    if (nsHTMLAtoms::script == tagAtom.get()) {
      result = ProcessStartSCRIPTTag(aNode);
    }
    
    nsCOMPtr<nsIHTMLContent> htmlContent;
    result = NS_CreateHTMLElement(getter_AddRefs(htmlContent), nodeInfo);
    content = do_QueryInterface(htmlContent);
  }
  else {
    // The first step here is to see if someone has provided their
    // own content element implementation (e.g., XUL or MathML).  
    // This is done based off a progid/namespace scheme.
    nsCOMPtr<nsIElementFactory> elementFactory;

    // This should *not* be done for every node, only when we find
    // a new namespace!!! -- jst
    GetElementFactory(nameSpaceID, getter_AddRefs(elementFactory));
    if (elementFactory) {
      // Create the content element using the element factory.
      elementFactory->CreateInstanceByTag(nodeInfo, getter_AddRefs(content));
    }
    else {
      nsCOMPtr<nsIXMLContent> xmlContent;
      result = NS_NewXMLElement(getter_AddRefs(xmlContent), nodeInfo);
    
      content = do_QueryInterface(xmlContent);
    }
  }
  
  if (NS_OK == result) {
    PRInt32 id;
    mDocument->GetAndIncrementContentID(&id);
    content->SetContentID(id);
    content->SetDocument(mDocument, PR_FALSE, PR_TRUE);

    // Set the attributes on the new content element
    result = AddAttributes(aNode, content, isHTML);
    if (NS_OK == result) {
      // If this is the document element
      if (nsnull == mDocElement) {
        mDocElement = content;
        NS_ADDREF(mDocElement);

        // For XSL, we need to wait till after the transform
        // to set the root content object.  Hence, the following
        // ifndef.
#ifndef XSL
        mDocument->SetRootContent(mDocElement);
#endif
      }
      else {
        nsCOMPtr<nsIContent> parent = getter_AddRefs(GetCurrentContent());

        parent->AppendChildTo(content, PR_FALSE);
      }
      if (pushContent) {
        PushContent(content);
      }
    }
  }

  return result;
}

NS_IMETHODIMP
nsXMLContentSink::CloseContainer(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;
  nsAutoString tag;
  nsCOMPtr<nsIAtom> nameSpacePrefix;
  PRInt32 nameSpaceID = kNameSpaceID_Unknown;
  PRBool isHTML = PR_FALSE;
  PRBool popContent = PR_TRUE;

  // XXX Hopefully the parser will flag this before we get
  // here. If we're in the prolog or epilog, there should be
  // no close tags for elements.
  PR_ASSERT(eXMLContentSinkState_InDocumentElement == mState);

  tag.Assign(aNode.GetText());

  nameSpacePrefix = getter_AddRefs(CutNameSpacePrefix(tag));
  nameSpaceID = GetNameSpaceId(nameSpacePrefix);
  isHTML = IsHTMLNameSpace(nameSpaceID);

  if (!mInScript) {
    FlushText();
  }

  if (isHTML) {
    nsIAtom* tagAtom = NS_NewAtom(tag);
    if (nsHTMLAtoms::script == tagAtom) {
      result = ProcessEndSCRIPTTag(aNode);
    }
    NS_RELEASE(tagAtom);
  }

  nsCOMPtr<nsIContent> content;
  if (popContent) {
    content = getter_AddRefs(PopContent());
    if (content) {
      if (mDocElement == content.get()) {
        mState = eXMLContentSinkState_InEpilog;
      }
    }
    else {
      // XXX Again, the parser should catch unmatched tags and
      // we should never get here.
      PR_ASSERT(0);
    }
  }
  nsINameSpace* nameSpace = PopNameSpaces();
  if (content) {
    nsCOMPtr<nsIXMLContent> xmlContent = do_QueryInterface(content);
    if (xmlContent) {
      xmlContent->SetContainingNameSpace(nameSpace);
    }
  }
  NS_IF_RELEASE(nameSpace);

  return result;
}

NS_IMETHODIMP
nsXMLContentSink::AddLeaf(const nsIParserNode& aNode)
{
  // XXX For now, all leaf content is character data
  // XXX make sure to push/pop name spaces here too (for attributes)
  switch (aNode.GetTokenType()) {
    case eToken_text:
    case eToken_whitespace:
    case eToken_newline:
      AddText(aNode.GetText());
      break;

    case eToken_cdatasection:
      /*
       * If we're inside a <html:script> tag we add the data as text so that
       * the script can be processed.
       *
       * -- jst@citec.fi
       */
      if (mInScript) {
        AddText(aNode.GetText());
      } else {
        AddCDATASection(aNode);
      }

      break;

    case eToken_entity:
    {
      nsAutoString tmp;
      PRInt32 unicode = aNode.TranslateToUnicodeStr(tmp);
      if (unicode < 0) {
        return AddText(aNode.GetText());
      }
      return AddText(tmp);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::NotifyError(const nsParserError* aError)
{
  return NS_OK;
}

// nsIXMLContentSink
NS_IMETHODIMP
nsXMLContentSink::AddXMLDecl(const nsIParserNode& aNode)
{
  // XXX We'll ignore it for now
  printf("nsXMLContentSink::AddXMLDecl\n");
  return NS_OK;
}

nsresult
nsXMLContentSink::AddContentAsLeaf(nsIContent *aContent)
{
  nsresult result = NS_OK;

  if (eXMLContentSinkState_InProlog == mState) {
    result = mDocument->AppendToProlog(aContent);
  }
  else if (eXMLContentSinkState_InEpilog == mState) {
    result = mDocument->AppendToEpilog(aContent);
  }
  else {
    nsCOMPtr<nsIContent> parent = getter_AddRefs(GetCurrentContent());

    if (nsnull != parent) {
      result = parent->AppendChildTo(aContent, PR_FALSE);
    }
  }

  return result;
}

NS_IMETHODIMP
nsXMLContentSink::AddComment(const nsIParserNode& aNode)
{
  FlushText();

  nsAutoString text;
  nsIContent *comment;
  nsIDOMComment *domComment;
  nsresult result = NS_OK;

  text.Assign(aNode.GetText());
  result = NS_NewCommentNode(&comment);
  if (NS_OK == result) {
    result = comment->QueryInterface(kIDOMCommentIID, (void **)&domComment);
    if (NS_OK == result) {
      domComment->AppendData(text);
      NS_RELEASE(domComment);

      comment->SetDocument(mDocument, PR_FALSE, PR_TRUE);
      result = AddContentAsLeaf(comment);
    }
    NS_RELEASE(comment);
  }

  return result;
}

NS_IMETHODIMP
nsXMLContentSink::AddCDATASection(const nsIParserNode& aNode)
{
  FlushText();

  nsAutoString text;
  nsIContent *cdata;
  nsIDOMCDATASection *domCDATA;
  nsresult result = NS_OK;

  text.Assign(aNode.GetText());
  result = NS_NewXMLCDATASection(&cdata);
  if (NS_OK == result) {
    result = cdata->QueryInterface(kIDOMCDATASectionIID, (void **)&domCDATA);
    if (NS_OK == result) {
      domCDATA->AppendData(text);
      NS_RELEASE(domCDATA);

      cdata->SetDocument(mDocument, PR_FALSE, PR_TRUE);
      result = AddContentAsLeaf(cdata);
    }
    NS_RELEASE(cdata);
  }

  return result;
}

// This method starts at aOffSet in aStr and tries to find aChar. It keeps 
// skipping whitespace till it finds aChar or some other non-whitespace character.  If
// it finds aChar, it returns aChar's offset.  If it finds some other non-whitespace character
// or runs into the end of the string, it returns -1.
static PRInt32
FindWhileSkippingWhitespace(nsString& aStr, PRUnichar aChar, PRInt32 aOffset)
{
  PRInt32 i = aOffset;
  PRUnichar ch = aStr.CharAt(i);
  PRInt32 index = -1;

  while (ch == '\n' || ch == '\t' || ch == '\r') {
    ch = aStr.CharAt(++i);
  }

  if (ch == aChar)
    index = i;

  return index;
}

static nsresult
GetQuotedAttributeValue(nsString& aSource,
                        const nsString& aAttribute,
                        nsString& aValue)
{  
  PRInt32 startOfAttribute = 0;     // Index into aSource where the attribute name starts
  PRInt32 startOfValue = 0;         // Index into aSource where the attribute value starts
  PRInt32 posnOfValueDelimeter = 0; 
  nsresult result = NS_ERROR_FAILURE;

  // While there are more characters to look at
  while (startOfAttribute != -1) {
    // Find the attribute starting at offset
    startOfAttribute = aSource.Find(aAttribute, PR_FALSE, startOfAttribute);
    // If attribute found
    if (startOfAttribute != -1) { 
      // Find the '=' character while skipping whitespace      
      startOfValue = FindWhileSkippingWhitespace(aSource, '=', startOfAttribute + aAttribute.Length());
      // If '=' found
      if (startOfValue != -1) {        
        PRUnichar delimeter = kQuote;
        // Find the quote or apostrophe while skipping whitespace
        posnOfValueDelimeter = FindWhileSkippingWhitespace(aSource, kQuote, startOfValue + 1);
        if (posnOfValueDelimeter == -1) {
          posnOfValueDelimeter = FindWhileSkippingWhitespace(aSource, kApostrophe, startOfValue + 1);
          delimeter = kApostrophe;
        }        
        // If quote or apostrophe found
        if (posnOfValueDelimeter != -1) {
          startOfValue = posnOfValueDelimeter + 1;
          // Find the ending quote or apostrophe
          posnOfValueDelimeter = aSource.FindChar(delimeter, PR_FALSE, startOfValue);
          // If found
          if (posnOfValueDelimeter != -1) {
            // Set the value of the attibute and exit the loop            
            // The attribute value starts at startOfValue and ends at (posnOfValueDelimeter - 1)
            aSource.Mid(aValue, startOfValue, posnOfValueDelimeter - startOfValue);
            result = NS_OK;
            break;
          }          
          else {
            // Try to find the attribute in the remainder of the string
            startOfAttribute++;
            continue;
          } // Endif found  
        }
        else {
          // Try to find the attribute in the remainder of the string
          startOfAttribute++;
          continue;
        } // Endif quote or apostrophe found
      } 
      else {
        // Try to find the attribute in the remainder of the string
        startOfAttribute++;
        continue;
      } // Endif '=' found
    } // Endif attribute found
  } // End while
  
  return result;
}

static void
ParseProcessingInstruction(const nsString& aText,
                           nsString& aTarget,
                           nsString& aData)
{
  PRInt32 offset;

  aTarget.Truncate();
  aData.Truncate();

  offset = aText.FindCharInSet(" \n\r\t");
  if (-1 != offset) {
    aText.Mid(aTarget, 2, offset-2);
    aText.Mid(aData, offset+1, aText.Length()-offset-3);
  }
}

static void SplitMimeType(const nsString& aValue, nsString& aType, nsString& aParams)
{
  aType.Truncate();
  aParams.Truncate();
  PRInt32 semiIndex = aValue.FindChar(PRUnichar(';'));
  if (-1 != semiIndex) {
    aValue.Left(aType, semiIndex);
    aValue.Right(aParams, (aValue.Length() - semiIndex) - 1);
    aParams.StripWhitespace();
  }
  else {
    aType = aValue;
  }
  aType.StripWhitespace();
}

#ifdef XSL
nsresult
nsXMLContentSink::CreateStyleSheetURL(nsIURI** aUrl,
                                      const nsAutoString& aHref)
{
  nsresult result = NS_OK;
  result = NS_NewURI(aUrl, aHref, mDocumentBaseURL);
  return result;

}

// Create an XML parser and an XSL content sink and start parsing
// the XSL stylesheet located at the given URL.
nsresult
nsXMLContentSink::LoadXSLStyleSheet(nsIURI* aUrl, const nsString& aType)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIParser> parser;

  static NS_DEFINE_IID(kCParserIID, NS_IPARSER_IID);
  static NS_DEFINE_IID(kCParserCID, NS_PARSER_IID);

  // Create the XML parser
  rv = nsComponentManager::CreateInstance(kCParserCID,
                                    nsnull,
                                    kCParserIID,
                                    getter_AddRefs(parser));

  if (NS_FAILED(rv)) return rv;

  // Create a transform mediator
  rv = NS_NewTransformMediator(&mXSLTransformMediator, aType);
  if (NS_FAILED(rv)) return rv;


  // Enable the transform mediator. It will start the transform
  // as soon as it has enough state to do so.  The state needed is
  // the source content model, the style content model, the current
  // document, and an observer.  The XML and XSL content sinks provide
  // this state by calling the various setters on nsITransformMediator.
  mXSLTransformMediator->SetEnabled(PR_TRUE);

  // The XML document owns the transform mediator.  Give the mediator to
  // the XML document.
  nsCOMPtr<nsIXMLDocument> xmlDoc;
  rv = mDocument->QueryInterface(kIXMLDocumentIID, (void**) getter_AddRefs(xmlDoc));
  if (NS_FAILED(rv)) return rv;
  xmlDoc->SetTransformMediator(mXSLTransformMediator);

  // Create the XSL content sink
  nsCOMPtr<nsIXMLContentSink> sink;
  rv = NS_NewXSLContentSink(getter_AddRefs(sink), mXSLTransformMediator, mDocument, aUrl, mWebShell);
  if (NS_FAILED(rv)) return rv;

  // Hook up the content sink to the parser's output and ask the parser
  // to start parsing the URL specified by aURL.
  parser->SetContentSink(sink);
  nsAutoString utf8(NS_ConvertASCIItoUCS2("UTF-8"));
  mDocument->SetDocumentCharacterSet(utf8);
  parser->SetDocumentCharset(utf8, kCharsetFromDocTypeDefault);
  parser->Parse(aUrl);

  // Set the parser as the stream listener and start the URL load
  nsCOMPtr<nsIStreamListener> sl;
  rv = mParser->QueryInterface(kIStreamListenerIID, (void**)getter_AddRefs(sl));
  if (NS_FAILED(rv)) return rv;

  rv = NS_OpenURI(sl, nsnull, aUrl);

  return rv;
}

nsresult
nsXMLContentSink::ProcessStyleLink(nsIContent* aElement,
                                   const nsString& aHref, PRBool aAlternate,
                                   const nsString& aTitle, const nsString& aType,
                                   const nsString& aMedia)
{
  nsresult rv = NS_OK;

  if (aType.EqualsIgnoreCase(kXSLType))
    rv = ProcessXSLStyleLink(aElement, aHref, aAlternate, aTitle, aType, aMedia);
  else
    rv = ProcessCSSStyleLink(aElement, aHref, aAlternate, aTitle, aType, aMedia);

  return rv;
}

nsresult
nsXMLContentSink::ProcessXSLStyleLink(nsIContent* aElement,
                                   const nsString& aHref, PRBool aAlternate,
                                   const nsString& aTitle, const nsString& aType,
                                   const nsString& aMedia)
{
  nsresult rv = NS_OK;
  nsIURI* url;

  rv = CreateStyleSheetURL(&url, aHref);
  if (NS_SUCCEEDED(rv)) {
    rv = LoadXSLStyleSheet(url, aType);
    NS_RELEASE(url);
  }

  return rv;
}
#endif

nsresult
nsXMLContentSink::ProcessCSSStyleLink(nsIContent* aElement,
                                   const nsString& aHref, PRBool aAlternate,
                                   const nsString& aTitle, const nsString& aType,
                                   const nsString& aMedia)
{
  nsresult result = NS_OK;

  if (aAlternate) { // if alternate, does it have title?
    if (0 == aTitle.Length()) { // alternates must have title
      return NS_OK; //return without error, for now
    }
  }

  nsAutoString  mimeType;
  nsAutoString  params;
  SplitMimeType(aType, mimeType, params);

  if ((0 == mimeType.Length()) || mimeType.EqualsIgnoreCase("text/css")) {
    nsIURI* url = nsnull;
// XXX we need to get passed in the nsILoadGroup here!
//    nsILoadGroup* group = mDocument->GetDocumentLoadGroup();
    result = NS_NewURI(&url, aHref, mDocumentBaseURL/*, group*/);
    if (NS_OK != result) {
      return NS_OK; // The URL is bad, move along, don't propogate the error (for now)
    }

    PRBool blockParser = PR_FALSE;
    if (! aAlternate) {
      if (0 < aTitle.Length()) {  // possibly preferred sheet
        if (0 == mPreferredStyle.Length()) {
          mPreferredStyle = aTitle;
          mCSSLoader->SetPreferredSheet(aTitle);
          mDocument->SetHeaderData(nsHTMLAtoms::headerDefaultStyle, aTitle);
        }
      }
      else {  // persistent sheet, block
        blockParser = PR_TRUE;
      }
    }

    PRBool doneLoading;
    result = mCSSLoader->LoadStyleLink(aElement, url, aTitle, aMedia, kNameSpaceID_Unknown,
                                       mStyleSheetCount++,
                                       ((blockParser) ? mParser : nsnull),
                                       doneLoading, nsnull);
    NS_RELEASE(url);
    if (NS_SUCCEEDED(result) && blockParser && (! doneLoading)) {
      result = NS_ERROR_HTMLPARSER_BLOCK;
    }
  }
  return result;
}

NS_IMETHODIMP
nsXMLContentSink::AddProcessingInstruction(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;
  nsAutoString text, target, data;
  nsIContent* node;

  FlushText();

  text.Assign(aNode.GetText());
  ParseProcessingInstruction(text, target, data);
  result = NS_NewXMLProcessingInstruction(&node, target, data);
  if (NS_OK == result) {
    node->SetDocument(mDocument, PR_FALSE, PR_TRUE);
    result = AddContentAsLeaf(node);
  }

  if (NS_OK == result) {
    nsAutoString type, href, title, media, alternate;

    // If it's a stylesheet PI...
    if (target.EqualsWithConversion(kStyleSheetPI)) {
      result = GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("href"), href);
      // If there was an error or there's no href, we can't do
      // anything with this PI
      if ((NS_OK != result) || (0 == href.Length())) {
        return result;
      }
      result = GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("type"), type);
      if (NS_FAILED(result)) {
        type.AssignWithConversion("text/css");  // Default the type attribute to the mime type for CSS
      }
      result = GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("title"), title);
      if (NS_SUCCEEDED(result)) {
        title.CompressWhitespace();
      }
      result = GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("media"), media);
      if (NS_SUCCEEDED(result)) {
        media.ToLowerCase();
      }
      result = GetQuotedAttributeValue(text, NS_ConvertASCIItoUCS2("alternate"), alternate);      
#ifndef XSL
      result = ProcessCSSStyleLink(node, href, alternate.EqualsWithConversion("yes"),
                                title, type, media);
#else
      result = ProcessStyleLink(node, href, alternate.EqualsWithConversion("yes"),
                                title, type, media);
#endif
    }
  }

  return result;
}


NS_IMETHODIMP
nsXMLContentSink::AddDocTypeDecl(const nsIParserNode& aNode, PRInt32 aMode)
{
  printf("nsXMLContentSink::AddDocTypeDecl\n");
  return NS_OK;
}

nsresult
nsXMLContentSink::FlushText(PRBool aCreateTextNode, PRBool* aDidFlush)
{
  nsresult rv = NS_OK;
  PRBool didFlush = PR_FALSE;
  if (0 != mTextLength) {
    if (aCreateTextNode) {
      nsIContent* content;
      rv = NS_NewTextNode(&content);
      if (NS_OK == rv) {
        // Set the content's document
        content->SetDocument(mDocument, PR_FALSE, PR_TRUE);

        // Set the text in the text node
        static NS_DEFINE_IID(kITextContentIID, NS_ITEXT_CONTENT_IID);
        nsITextContent* text = nsnull;
        content->QueryInterface(kITextContentIID, (void**) &text);
        text->SetText(mText, mTextLength, PR_FALSE);
        NS_RELEASE(text);

        // Add text to its parent
        AddContentAsLeaf(content);
        NS_RELEASE(content);
      }
    }
    mTextLength = 0;
    didFlush = PR_TRUE;
  }
  if (nsnull != aDidFlush) {
    *aDidFlush = didFlush;
  }
  return rv;
}

#define NS_ACCUMULATION_BUFFER_SIZE 4096

NS_IMETHODIMP
nsXMLContentSink::AddCharacterData(const nsIParserNode& aNode)
{
  return AddText(aNode.GetText());
}

nsresult
nsXMLContentSink::AddText(const nsString& aString)
{
  PRInt32 addLen = aString.Length();
  if (0 == addLen) {
    return NS_OK;
  }

  // Create buffer when we first need it
  if (0 == mTextSize) {
    mText = (PRUnichar *) PR_MALLOC(sizeof(PRUnichar) * NS_ACCUMULATION_BUFFER_SIZE);
    if (nsnull == mText) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTextSize = NS_ACCUMULATION_BUFFER_SIZE;
  }

  // Copy data from string into our buffer; flush buffer when it fills up
  PRInt32 offset = 0;
  while (0 != addLen) {
    PRInt32 amount = mTextSize - mTextLength;
    if (amount > addLen) {
      amount = addLen;
    }
    if (0 == amount) {
      if (mConstrainSize) {
        nsresult rv = FlushText();
        if (NS_OK != rv) {
          return rv;
        }
      }
      else {
        mTextSize += addLen;
        mText = (PRUnichar *) PR_REALLOC(mText, sizeof(PRUnichar) * mTextSize);
        if (nsnull == mText) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }
    }
    memcpy(&mText[mTextLength], aString.GetUnicode() + offset,
           sizeof(PRUnichar) * amount);
    mTextLength += amount;
    offset += amount;
    addLen -= amount;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::AddUnparsedEntity(const nsIParserNode& aNode)
{
  printf("nsXMLContentSink::AddUnparsedEntity\n");
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::AddNotation(const nsIParserNode& aNode)
{
  printf("nsXMLContentSink::AddNotation\n");
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::AddEntityReference(const nsIParserNode& aNode)
{
  printf("nsXMLContentSink::AddEntityReference\n");
  return NS_OK;
}

PRInt32
nsXMLContentSink::GetNameSpaceId(nsIAtom* aPrefix)
{
  PRInt32 id = (nsnull == aPrefix) ? kNameSpaceID_None : kNameSpaceID_Unknown;
  if ((nsnull != mNameSpaceStack) && (0 < mNameSpaceStack->Count())) {
    PRInt32 index = mNameSpaceStack->Count() - 1;
    nsINameSpace* nameSpace = (nsINameSpace*)mNameSpaceStack->ElementAt(index);
    nameSpace->FindNameSpaceID(aPrefix, id);
  }

  return id;
}

nsINameSpace*
nsXMLContentSink::PopNameSpaces()
{
  if ((nsnull != mNameSpaceStack) && (0 < mNameSpaceStack->Count())) {
    PRInt32 index = mNameSpaceStack->Count() - 1;
    nsINameSpace* nameSpace = (nsINameSpace*)mNameSpaceStack->ElementAt(index);
    mNameSpaceStack->RemoveElementAt(index);
    return nameSpace;
  }

  return nsnull;
}

PRBool
nsXMLContentSink::IsHTMLNameSpace(PRInt32 aID)
{
  return PRBool(kNameSpaceID_HTML == aID);
}

nsIContent*
nsXMLContentSink::GetCurrentContent()
{
  if (nsnull != mContentStack) {
    PRUint32 count;
    mContentStack->Count(&count);
    PR_ASSERT(count);
    if (count) {
      return (nsIContent *)mContentStack->ElementAt(count-1);
    }
  }
  return nsnull;
}

PRInt32
nsXMLContentSink::PushContent(nsIContent *aContent)
{
  PRUint32 count;
  if (nsnull == mContentStack) {
    NS_NewISupportsArray(getter_AddRefs(mContentStack));
  }

  mContentStack->AppendElement(aContent);
  mContentStack->Count(&count);

  return count;
}

nsIContent*
nsXMLContentSink::PopContent()
{  
  nsIContent* content = nsnull;
  if (nsnull != mContentStack) {
    PRUint32 index, count;
    mContentStack->Count(&count);
    index =  count - 1;
    content = (nsIContent *)mContentStack->ElementAt(index);
    mContentStack->RemoveElementAt(index);
  }

  // The caller should NS_RELEASE the returned content object.
  return content;
}

void
nsXMLContentSink::StartLayout()
{
  PRInt32 i, ns = mDocument->GetNumberOfShells();
  for (i = 0; i < ns; i++) {
    nsIPresShell* shell = mDocument->GetShellAt(i);
    if (nsnull != shell) {
      // Make shell an observer for next time
      shell->BeginObservingDocument();

      // Resize-reflow this time
      nsCOMPtr<nsIPresContext> cx;
      shell->GetPresContext(getter_AddRefs(cx));
      nsRect r;
      cx->GetVisibleArea(r);
      shell->InitialReflow(r.width, r.height);

      // Now trigger a refresh
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if (vm) {
        RefreshIfEnabled(vm);
      }

      NS_RELEASE(shell);
    }
  }

  // If the document we are loading has a reference or it is a top level
  // frameset document, disable the scroll bars on the views.
  char* ref = nsnull;
  nsIURL* url;
  nsresult rv = mDocumentURL->QueryInterface(NS_GET_IID(nsIURL), (void**)&url);
  if (NS_SUCCEEDED(rv)) {
    rv = url->GetRef(&ref);
    NS_RELEASE(url);
  }
  PRBool topLevelFrameset = PR_FALSE;
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mWebShell));
  if (docShellAsItem) {
    nsCOMPtr<nsIDocShellTreeItem> root;
    docShellAsItem->GetSameTypeRootTreeItem(getter_AddRefs(root));
    if(docShellAsItem.get() == root.get()) {
      topLevelFrameset = PR_TRUE;
    }
  }

  if ((nsnull != ref) || topLevelFrameset) {
    // XXX support more than one presentation-shell here

    // Get initial scroll preference and save it away; disable the
    // scroll bars.
    ns = mDocument->GetNumberOfShells();
    for (i = 0; i < ns; i++) {
      nsIPresShell* shell = mDocument->GetShellAt(i);
      if (nsnull != shell) {
        nsCOMPtr<nsIViewManager>vm;
        shell->GetViewManager(getter_AddRefs(vm));
        if (vm) {
          nsIView* rootView = nsnull;
          vm->GetRootView(rootView);
          if (nsnull != rootView) {
            nsIScrollableView* sview = nsnull;
            rootView->QueryInterface(kIScrollableViewIID, (void**) &sview);
            if (nsnull != sview) {
              if (topLevelFrameset)
                mOriginalScrollPreference = nsScrollPreference_kNeverScroll;
              else
                sview->GetScrollPreference(mOriginalScrollPreference);
              sview->SetScrollPreference(nsScrollPreference_kNeverScroll);
            }
          }
        }
        NS_RELEASE(shell);
      }
    }
    // XXX who actually uses ref here anyway?
    nsCRT::free(ref);
  }
}

NS_IMETHODIMP
nsXMLContentSink::ResumeParsing()
{
  if (mParser) {
    mParser->EnableParser(PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::EvaluateScript(nsString& aScript, PRUint32 aLineNo, const char* aVersion)
{
  nsresult rv = NS_OK;

  if (0 < aScript.Length()) {
    nsCOMPtr<nsIScriptGlobalObject> scriptGlobal;
    mDocument->GetScriptGlobalObject(getter_AddRefs(scriptGlobal));
    if (scriptGlobal) {
      nsCOMPtr<nsIScriptContext> context;
      NS_ENSURE_SUCCESS(scriptGlobal->GetContext(getter_AddRefs(context)),
         NS_ERROR_FAILURE);

      nsIURI* docURL = mDocument->GetDocumentURL();
      char* url;
      if (docURL) {
        rv = docURL->GetSpec(&url);
      }

      nsCOMPtr<nsIPrincipal> principal;
      if (NS_SUCCEEDED(rv)) {
        rv = mDocument->GetPrincipal(getter_AddRefs(principal));
        NS_ASSERTION(principal, "principal required for document");
      }

      if (NS_SUCCEEDED(rv)) {
        nsAutoString val;
        PRBool isUndefined;

        (void) context->EvaluateString(aScript, nsnull, principal, url, aLineNo, aVersion,
                                       val, &isUndefined);

        NS_IF_RELEASE(docURL);

        nsCRT::free(url);
      }
    }
  }

  return rv;
}

nsresult
nsXMLContentSink::ProcessEndSCRIPTTag(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;
  if (mInScript) {
    nsAutoString script;
    script.Assign(mText, mTextLength);
    result = EvaluateScript(script, mScriptLineNo, mScriptLanguageVersion);
    FlushText(PR_FALSE);
    mInScript = PR_FALSE;
  }

  return result;
}

#define SCRIPT_BUF_SIZE 1024

// XXX Stolen from nsHTMLContentSink. Needs to be shared.
// XXXbe share also with nsRDFParserUtils.cpp and nsHTMLContentSink.cpp
// Returns PR_TRUE if the language name is a version of JavaScript and
// PR_FALSE otherwise
static PRBool
IsJavaScriptLanguage(const nsString& aName, const char* *aVersion)
{
  JSVersion version = JSVERSION_UNKNOWN;

  if (aName.EqualsIgnoreCase("JavaScript") ||
      aName.EqualsIgnoreCase("LiveScript") ||
      aName.EqualsIgnoreCase("Mocha")) {
    version = JSVERSION_DEFAULT;
  }
  else if (aName.EqualsIgnoreCase("JavaScript1.1")) {
    version = JSVERSION_1_1;
  }
  else if (aName.EqualsIgnoreCase("JavaScript1.2")) {
    version = JSVERSION_1_2;
  }
  else if (aName.EqualsIgnoreCase("JavaScript1.3")) {
    version = JSVERSION_1_3;
  }
  else if (aName.EqualsIgnoreCase("JavaScript1.4")) {
    version = JSVERSION_1_4;
  }
  else if (aName.EqualsIgnoreCase("JavaScript1.5")) {
    version = JSVERSION_1_5;
  }
  if (version == JSVERSION_UNKNOWN)
    return PR_FALSE;
  *aVersion = JS_VersionToString(version);
  return PR_TRUE;
}

NS_IMETHODIMP
nsXMLContentSink::OnStreamComplete(nsIStreamLoader* aLoader,
                                   nsISupports* context,
                                   nsresult aStatus,
                                   PRUint32 stringLen,
                                   const char* string)
{
  nsresult rv = NS_OK;
  nsString aData; aData.AssignWithConversion(string, stringLen);

  if (NS_OK == aStatus) {
    rv = EvaluateScript(aData, 0, mScriptLanguageVersion);
    if (NS_FAILED(rv)) return rv;
  }

  rv = ResumeParsing();
  if (NS_FAILED(rv)) return rv;

  // We added a reference when the loader was created. This
  // release should destroy it.
  NS_RELEASE(aLoader);

  return rv;
}

nsresult
nsXMLContentSink::ProcessStartSCRIPTTag(const nsIParserNode& aNode)
{
  nsresult rv = NS_OK;
  PRBool isJavaScript = PR_TRUE;
  const char* jsVersionString = nsnull;
  PRInt32 i, ac = aNode.GetAttributeCount();

  // Look for SRC attribute and look for a LANGUAGE attribute
  nsAutoString src;
  for (i = 0; i < ac; i++) {
    const nsString& key = aNode.GetKeyAt(i);
    if (key.EqualsIgnoreCase("src")) {
      GetAttributeValueAt(aNode, i, src);
    }
    else if (key.EqualsIgnoreCase("type")) {
      nsAutoString  type;

      GetAttributeValueAt(aNode, i, type);
      nsAutoString  mimeType;
      nsAutoString  params;
      SplitMimeType(type, mimeType, params);

      isJavaScript = mimeType.EqualsIgnoreCase("text/javascript");
      if (isJavaScript) {
        JSVersion jsVersion = JSVERSION_DEFAULT;
        if (params.Find("version=", PR_TRUE) == 0) {
          if (params.Length() != 11 || params[8] != '1' || params[9] != '.')
            jsVersion = JSVERSION_UNKNOWN;
          else switch (params[10]) {
            case '0': jsVersion = JSVERSION_1_0; break;
            case '1': jsVersion = JSVERSION_1_1; break;
            case '2': jsVersion = JSVERSION_1_2; break;
            case '3': jsVersion = JSVERSION_1_3; break;
            case '4': jsVersion = JSVERSION_1_4; break;
            case '5': jsVersion = JSVERSION_1_5; break;
            default:  jsVersion = JSVERSION_UNKNOWN;
          }
        }
        jsVersionString = JS_VersionToString(jsVersion);
      }
    }
    else if (key.EqualsIgnoreCase("language")) {
      nsAutoString  lang;

      GetAttributeValueAt(aNode, i, lang);
      isJavaScript = IsJavaScriptLanguage(lang, &jsVersionString);
    }
  }

  // Don't process scripts that aren't JavaScript
  if (isJavaScript) {
    mScriptLanguageVersion = jsVersionString;

    // If there is a SRC attribute...
    if (src.Length() > 0) {
      // Use the SRC attribute value to load the URL
      nsIURI* url = nsnull;
      nsAutoString absURL;
    // XXX we need to get passed in the nsILoadGroup here!
//      nsILoadGroup* group = mDocument->GetDocumentLoadGroup();
      rv = NS_NewURI(&url, src, mDocumentBaseURL);
      if (NS_OK != rv) {
        return rv;
      }

      // Check that this page is allowed to load this URI.
      NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager, 
                      NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
      if (NS_FAILED(rv)) 
          return rv;
      rv = securityManager->CheckLoadURI(mDocumentBaseURL, url, PR_FALSE);
      if (NS_FAILED(rv)) 
          return rv;

      nsIStreamLoader* loader;
      nsCOMPtr<nsILoadGroup> loadGroup;

      mDocument->GetDocumentLoadGroup(getter_AddRefs(loadGroup));
      rv = NS_NewStreamLoader(&loader, url, this, nsnull, loadGroup);
      NS_RELEASE(url);
      if (NS_OK == rv) {
        rv = NS_ERROR_HTMLPARSER_BLOCK;
      }
    }
    else {
      // Wait until we get the script content
      mInScript = PR_TRUE;
      mConstrainSize = PR_FALSE;
      mScriptLineNo = (PRUint32)aNode.GetSourceLineNumber();
    }
  }

  return rv;
}

nsresult
nsXMLContentSink::RefreshIfEnabled(nsIViewManager* vm)
{
  if (vm) {
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(mWebShell));
    nsCOMPtr<nsIContentViewer> contentViewer;
    nsresult rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
    if (NS_SUCCEEDED(rv) && (contentViewer != nsnull)) {
      PRBool enabled;
      contentViewer->GetEnableRendering(&enabled);
      if (enabled) {
        vm->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
      }
    }
  }
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
//
//   XML Element Factory
//

class XMLElementFactoryImpl : public nsIElementFactory
{
protected:
  XMLElementFactoryImpl();
  virtual ~XMLElementFactoryImpl();

public:
  friend
  nsresult
  NS_NewXMLElementFactory(nsIElementFactory** aResult);

  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsIElementFactory interface
  NS_IMETHOD CreateInstanceByTag(nsINodeInfo *aNodeInfo, nsIContent** aResult);

};


XMLElementFactoryImpl::XMLElementFactoryImpl()
{
  NS_INIT_REFCNT();
}

XMLElementFactoryImpl::~XMLElementFactoryImpl()
{
}


NS_IMPL_ISUPPORTS(XMLElementFactoryImpl, NS_GET_IID(nsIElementFactory));


nsresult
NS_NewXMLElementFactory(nsIElementFactory** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  XMLElementFactoryImpl* result = new XMLElementFactoryImpl();
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *aResult = result;
  return NS_OK;
}



NS_IMETHODIMP
XMLElementFactoryImpl::CreateInstanceByTag(nsINodeInfo *aNodeInfo,
                                           nsIContent** aResult)
{
  nsCOMPtr<nsIXMLContent> xmlContent;
  nsresult rv = NS_NewXMLElement(getter_AddRefs(xmlContent), aNodeInfo);
  nsCOMPtr<nsIContent> result = do_QueryInterface(xmlContent);
  *aResult = result;
  NS_IF_ADDREF(*aResult);
  return rv; 
}

void 
nsXMLContentSink::GetElementFactory(PRInt32 aNameSpaceID, nsIElementFactory** aResult)
{
  nsresult rv;
  nsAutoString nameSpace;
  gNameSpaceManager->GetNameSpaceURI(aNameSpaceID, nameSpace);

  nsCAutoString progID = NS_ELEMENT_FACTORY_PROGID_PREFIX;
  progID.AppendWithConversion(nameSpace);

  // Retrieve the appropriate factory.
  NS_WITH_SERVICE(nsIElementFactory, elementFactory, progID, &rv);

  *aResult = elementFactory;
  NS_IF_ADDREF(*aResult);
}
