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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

#include "nsCOMPtr.h"
#include "nsXBLService.h"
#include "nsIInputStream.h"
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
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsIXMLContentSink.h"
#include "nsLayoutCID.h"
#include "nsXMLDocument.h"
#include "nsHTMLAtoms.h"
#include "nsSupportsArray.h"
#include "nsITextContent.h"

#include "nsIXBLBinding.h"
#include "nsIChromeRegistry.h"
#include "nsIPref.h"

// Static IIDs/CIDs. Try to minimize these.
static NS_DEFINE_CID(kNameSpaceManagerCID,        NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_CID(kXMLDocumentCID,             NS_XMLDOCUMENT_CID);
static NS_DEFINE_CID(kParserCID,                  NS_PARSER_IID); // XXX What's up with this???
static NS_DEFINE_CID(kChromeRegistryCID,          NS_CHROMEREGISTRY_CID);

// nsProxyStream 
// A helper class used for synchronous parsing of URLs.
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


// Implementation /////////////////////////////////////////////////////////////////

// Static member variable initialization
PRUint32 nsXBLService::gRefCnt = 0;
nsSupportsHashtable* nsXBLService::mBindingTable = nsnull;
nsSupportsHashtable* nsXBLService::mScriptAccessTable = nsnull;

nsINameSpaceManager* nsXBLService::gNameSpaceManager = nsnull;
 
nsHashtable* nsXBLService::gClassTable = nsnull;

nsIAtom* nsXBLService::kExtendsAtom = nsnull;
nsIAtom* nsXBLService::kHasChildrenAtom = nsnull;
nsIAtom* nsXBLService::kURIAtom = nsnull;

// Enabled by default. Must be over-ridden to disable
PRBool nsXBLService::gDisableChromeCache = PR_FALSE;
static const char kDisableChromeCachePref[] = "nglayout.debug.disable_xul_cache";

PRInt32 nsXBLService::kNameSpaceID_XBL;

// Implement our nsISupports methods
NS_IMPL_ISUPPORTS1(nsXBLService, nsIXBLService)

// Constructors/Destructors
nsXBLService::nsXBLService(void)
{
  NS_INIT_REFCNT();
  gRefCnt++;
  if (gRefCnt == 1) {
    // Create our binding table.
    mBindingTable = new nsSupportsHashtable();
    mScriptAccessTable = new nsSupportsHashtable();

    // Register the XBL namespace.
    nsresult rv = nsComponentManager::CreateInstance(kNameSpaceManagerCID,
                                                     nsnull,
                                                     NS_GET_IID(nsINameSpaceManager),
                                                     (void**) &gNameSpaceManager);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create namespace manager");
    if (NS_FAILED(rv)) return;

    // XXX This is sure to change. Copied from mozilla/layout/xul/content/src/nsXULAtoms.cpp
    static const char kXBLNameSpaceURI[]
        = "http://www.mozilla.org/xbl";

    rv = gNameSpaceManager->RegisterNameSpace(NS_ConvertASCIItoUCS2(kXBLNameSpaceURI), kNameSpaceID_XBL);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to register XBL namespace");
    if (NS_FAILED(rv)) return;

    // Create our atoms
    kExtendsAtom = NS_NewAtom("extends");
    kHasChildrenAtom = NS_NewAtom("haschildren");
    kURIAtom = NS_NewAtom("uri");

    // Find out if the XUL cache is on or off
    NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_PROGID, &rv);
    if (NS_SUCCEEDED(rv))
      prefs->GetBoolPref(kDisableChromeCachePref, &gDisableChromeCache);

    gClassTable = new nsHashtable();
  }
}

static PRBool PR_CALLBACK DeleteClasses(nsHashKey* aKey, void* aValue, void* closure)
{
  JSClass* c = (JSClass*)aValue;
  nsMemory::Free(c->name);
  delete c;
  return HT_ENUMERATE_NEXT;
}

nsXBLService::~nsXBLService(void)
{
  gRefCnt--;
  if (gRefCnt == 0) {
    delete mBindingTable;
    delete mScriptAccessTable;

    NS_IF_RELEASE(gNameSpaceManager);
    
    // Release our atoms
    NS_RELEASE(kExtendsAtom);
    NS_RELEASE(kHasChildrenAtom);
    NS_RELEASE(kURIAtom);

    // Walk the hashtable and delete the JSClasses
    if (gClassTable)
      gClassTable->Enumerate(DeleteClasses);
    delete gClassTable;
  }
}

// This function loads a particular XBL file and installs all of the bindings
// onto the element.
NS_IMETHODIMP
nsXBLService::LoadBindings(nsIContent* aContent, const nsString& aURL, PRBool aAugmentFlag) 
{ 
  nsresult rv;

  nsCOMPtr<nsIDocument> document;
  aContent->GetDocument(*getter_AddRefs(document));
  nsCOMPtr<nsIBindingManager> bindingManager;
  document->GetBindingManager(getter_AddRefs(bindingManager));
  
  nsCOMPtr<nsIXBLBinding> binding;
  bindingManager->GetBinding(aContent, getter_AddRefs(binding));
  if (binding && !aAugmentFlag) {
    nsCOMPtr<nsIXBLBinding> styleBinding;
    binding->GetFirstStyleBinding(getter_AddRefs(styleBinding));
    if (styleBinding) {
      // See if the URIs match.
      nsAutoString uri;
      styleBinding->GetBindingURI(uri);
      if (uri.Equals(aURL))
        return NS_OK;
      else FlushStyleBindings(aContent);
    }
  }

  nsCOMPtr<nsIXBLBinding> newBinding;
  nsCAutoString url; url.AssignWithConversion(aURL);
  if (NS_FAILED(rv = GetBinding(url, getter_AddRefs(newBinding)))) {
    NS_ERROR("Failed loading an XBL document for content node.");
    return rv;
  }

  if (!newBinding) {
    nsCAutoString str = "Failed to locate XBL binding. XBL is now using id instead of name to reference bindings. Make sure you have switched over.  The invalid binding name is: ";
    str.AppendWithConversion(aURL);
    NS_ERROR(str);
    return NS_ERROR_FAILURE;
  }

  if (aAugmentFlag) {
    nsCOMPtr<nsIXBLBinding> baseBinding;
    nsCOMPtr<nsIXBLBinding> nextBinding = newBinding;
    do {
      baseBinding = nextBinding;
      baseBinding->GetBaseBinding(getter_AddRefs(nextBinding));
      baseBinding->SetIsStyleBinding(PR_FALSE);
    } while (nextBinding);

    // XXX Handle adjusting the prototype chain! We need to somehow indicate to
    // InstallProperties that the whole chain should just be whacked and rebuilt.
    // We are becoming the new binding.
    bindingManager->SetBinding(aContent, newBinding);
    baseBinding->SetBaseBinding(binding);
  }
  else {
    // We loaded a style binding.  It goes on the end.
    if (binding) {
      // Get the last binding that is in the append layer.
      nsCOMPtr<nsIXBLBinding> rootBinding;
      binding->GetRootBinding(getter_AddRefs(rootBinding));
      rootBinding->SetBaseBinding(newBinding);
    }
    else {
      // Install the binding on the content node.
      bindingManager->SetBinding(aContent, newBinding);
    }
  }

  // Set the binding's bound element.
  newBinding->SetBoundElement(aContent);

  // Tell the binding to build the anonymous content.
  newBinding->GenerateAnonymousContent(aContent);

  // Tell the binding to install event handlers
  newBinding->InstallEventHandlers(aContent);

  // Set up our properties
  newBinding->InstallProperties(aContent);

  return NS_OK; 
}

// For a given element, returns a flat list of all the anonymous children that need
// frames built.
NS_IMETHODIMP
nsXBLService::GetContentList(nsIContent* aContent, nsISupportsArray** aResult, nsIContent** aParent, 
                             PRBool* aMultipleInsertionPoints)
{ 
  // Iterate over all of the bindings one by one and build up an array
  // of anonymous items.
  *aResult = nsnull;
  *aParent = nsnull;
  *aMultipleInsertionPoints = PR_FALSE;

  nsCOMPtr<nsIDocument> document;
  aContent->GetDocument(*getter_AddRefs(document));
  nsCOMPtr<nsIBindingManager> bindingManager;
  NS_ASSERTION(document, "no document");
  if (!document) return NS_ERROR_FAILURE;
  document->GetBindingManager(getter_AddRefs(bindingManager));
  
  nsCOMPtr<nsIXBLBinding> binding;
  bindingManager->GetBinding(aContent, getter_AddRefs(binding));
    
  while (binding) {
    // Get the anonymous content.
    nsCOMPtr<nsIContent> content;
    binding->GetAnonymousContent(getter_AddRefs(content));
    if (content) {
      PRInt32 childCount;
      content->ChildCount(childCount);
      for (PRInt32 i = 0; i < childCount; i++) {
        nsCOMPtr<nsIContent> anonymousChild;
        content->ChildAt(i, *getter_AddRefs(anonymousChild));
        if (!(*aResult)) 
          NS_NewISupportsArray(aResult); // This call addrefs the array.

        (*aResult)->AppendElement(anonymousChild);
      }

      binding->GetSingleInsertionPoint(aParent, aMultipleInsertionPoints);
      return NS_OK;
    }

    nsCOMPtr<nsIXBLBinding> nextBinding;
    binding->GetBaseBinding(getter_AddRefs(nextBinding));
    binding = nextBinding;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXBLService::FlushStyleBindings(nsIContent* aContent)
{
  nsCOMPtr<nsIDocument> document;
  aContent->GetDocument(*getter_AddRefs(document));
  nsCOMPtr<nsIBindingManager> bindingManager;
  document->GetBindingManager(getter_AddRefs(bindingManager));
  
  nsCOMPtr<nsIXBLBinding> binding;
  bindingManager->GetBinding(aContent, getter_AddRefs(binding));
  
  if (binding) {
    nsCOMPtr<nsIXBLBinding> styleBinding;
    binding->GetFirstStyleBinding(getter_AddRefs(styleBinding));

    if (styleBinding) {
      // Clear out the script references.
      nsCOMPtr<nsIDocument> document;
      aContent->GetDocument(*getter_AddRefs(document));
      styleBinding->ChangeDocument(document, nsnull);
    }

    if (styleBinding == binding) 
      bindingManager->SetBinding(aContent, nsnull); // Flush old style bindings
  }
   
  return NS_OK;
}

NS_IMETHODIMP
nsXBLService::FlushBindingDocuments()
{
  delete mBindingTable;
  mBindingTable = new nsSupportsHashtable();
  delete mScriptAccessTable;
  mScriptAccessTable = new nsSupportsHashtable();
  return NS_OK;
}

NS_IMETHODIMP
nsXBLService::ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID, nsIAtom** aResult)
{
  nsCOMPtr<nsIDocument> document;
  aContent->GetDocument(*getter_AddRefs(document));
  if (document) {
    nsCOMPtr<nsIBindingManager> bindingManager;
    document->GetBindingManager(getter_AddRefs(bindingManager));
  
    if (bindingManager)
      return bindingManager->ResolveTag(aContent, aNameSpaceID, aResult);
  }

  aContent->GetNameSpaceID(*aNameSpaceID);
  aContent->GetTag(*aResult); // Addref happens here.
  return NS_OK;
}

NS_IMETHODIMP
nsXBLService::AllowScripts(nsIContent* aContent, PRBool* aAllowScripts)
{
  nsAutoString uri;
  aContent->GetAttribute(kNameSpaceID_None, kURIAtom, uri);
  
  PRInt32 indx = uri.RFindChar('#');
  if (indx >= 0)
    uri.Truncate(indx);

  nsStringKey key(uri);
  nsCOMPtr<nsIDocument> document;
  document = dont_AddRef(NS_STATIC_CAST(nsIDocument*, mScriptAccessTable->Get(&key)));

  *aAllowScripts = !document;

  return NS_OK;
}

// Internal helper methods ////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsXBLService::GetBinding(const nsCString& aURLStr, nsIXBLBinding** aResult)
{
  *aResult = nsnull;

  if (aURLStr.IsEmpty())
    return NS_ERROR_FAILURE;

  // XXX Obtain the # marker and remove it from the URL.
  nsCAutoString uri(aURLStr);
  PRInt32 indx = uri.RFindChar('#');
  nsCAutoString ref; 
  uri.Right(ref, uri.Length() - (indx + 1));
  uri.Truncate(indx);

  nsCOMPtr<nsIDocument> doc;
  
  GetBindingDocument(uri, getter_AddRefs(doc));
  if (!doc)
    return NS_ERROR_FAILURE;

  // We have a doc. Obtain our specific binding element.
  // Walk the children looking for the binding that matches the ref
  // specified in the URL.
  nsCOMPtr<nsIContent> root = getter_AddRefs(doc->GetRootContent());
  if (!root)
    return NS_ERROR_FAILURE;

  nsAutoString bindingName; bindingName.AssignWithConversion( NS_STATIC_CAST(const char*, ref) );

  PRInt32 count;
  root->ChildCount(count);

  for (PRInt32 i = 0; i < count; i++) {
    nsCOMPtr<nsIContent> child;
    root->ChildAt(i, *getter_AddRefs(child));

    nsAutoString value;
    child->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::id, value);
    
    // If no ref is specified just use this.
    if ((bindingName.IsEmpty()) || (bindingName == value)) {
      nsAutoString url;
      child->GetAttribute(kNameSpaceID_None, kURIAtom, url);
      if (url.IsEmpty())
        child->SetAttribute(kNameSpaceID_None, kURIAtom, NS_ConvertASCIItoUCS2(aURLStr.GetBuffer(), aURLStr.Length()), PR_FALSE);
        
      // Make a new binding
      NS_NewXBLBinding(aResult);

      // Initialize its bound element.
      (*aResult)->SetBindingElement(child);

      // Check for the presence of an extends attribute
      child->GetAttribute(kNameSpaceID_None, kExtendsAtom, value);
      if (!value.IsEmpty()) {
        // See if we are extending a builtin tag.
        nsCOMPtr<nsIAtom> tag;
        PRInt32 dummy;
        (*aResult)->GetBaseTag(&dummy, getter_AddRefs(tag));
        if (!tag) {
          // We have a base class binding. Load it right now.
          nsCOMPtr<nsIXBLBinding> baseBinding;
          nsCAutoString urlCString; urlCString.AssignWithConversion(value);
          GetBinding(urlCString, getter_AddRefs(baseBinding));
          if (!baseBinding)
            return NS_OK; // At least we got the derived class binding loaded.
          (*aResult)->SetBaseBinding(baseBinding);
        }
      }

      break;
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXBLService::GetBindingDocument(const nsCString& aURLStr, nsIDocument** aResult)
{
  nsresult rv;

  *aResult = nsnull;
  
  // We've got a file.  Check our key binding file cache.
  nsStringKey key(aURLStr);
  nsCOMPtr<nsIDocument> document;
  document = dont_AddRef(NS_STATIC_CAST(nsIDocument*, mBindingTable->Get(&key)));

  if (!document) {
    nsCOMPtr<nsIURL> uri;
    nsComponentManager::CreateInstance("component://netscape/network/standard-url",
                                       nsnull,
                                       NS_GET_IID(nsIURL),
                                       getter_AddRefs(uri));
    uri->SetSpec(aURLStr);

    FetchBindingDocument(uri, getter_AddRefs(document));
    if (document) {
      // Put the key binding doc into our table.
      mBindingTable->Put(&key, document);

      nsCOMPtr<nsIChromeRegistry> reg(do_GetService(kChromeRegistryCID, &rv));
      if (NS_SUCCEEDED(rv) && reg) {
        PRBool allow;
        reg->AllowScriptsForSkin(uri, &allow);
        if (!allow)
          mScriptAccessTable->Put(&key, document);
      }
    }
    else return NS_ERROR_FAILURE;
  }

  *aResult = document;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsXBLService::FetchBindingDocument(nsIURI* aURI, nsIDocument** aResult)
{
  // Initialize our out pointer to nsnull
  *aResult = nsnull;

  // Create the XML document
  nsCOMPtr<nsIDocument> doc;
  nsresult rv = nsComponentManager::CreateInstance(kXMLDocumentCID, nsnull,
                                                   NS_GET_IID(nsIDocument),
                                                   getter_AddRefs(doc));

  if (NS_FAILED(rv)) return rv;

  // XXX This is evil, but we're living in layout, so I'm
  // just going to do it.
  nsXMLDocument* xmlDoc = (nsXMLDocument*)(doc.get());

  // Now we have to synchronously load the binding file.
  // Create an XML content sink and a parser. 

  nsCOMPtr<nsIChannel> channel;
  rv = NS_OpenURI(getter_AddRefs(channel), aURI, nsnull);
  if (NS_FAILED(rv)) return rv;

  // Call StartDocumentLoad
  nsCOMPtr<nsIStreamListener> listener;
  if (NS_FAILED(rv = xmlDoc->StartDocumentLoad("view", channel, 
                                               nsnull, nsnull, getter_AddRefs(listener)))) {
    NS_ERROR("Failure to init XBL doc prior to load.");
    return rv;
  }

  // Now do a blocking synchronous parse of the file.
  nsCOMPtr<nsIInputStream> in;
  PRUint32 sourceOffset = 0;
  rv = channel->OpenInputStream(getter_AddRefs(in));

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

  // The XML content sink produces a ridiculous # of content nodes.
  // It generates text nodes even for whitespace.  The following
  // call walks the generated document tree and trims out these
  // nodes.
  nsCOMPtr<nsIContent> root = getter_AddRefs(doc->GetRootContent());
  if (root)
    StripWhitespaceNodes(root);

  return NS_OK;
}

NS_IMETHODIMP 
nsXBLService::StripWhitespaceNodes(nsIContent* aElement)
{
  PRInt32 childCount;
  aElement->ChildCount(childCount);
  for (PRInt32 i = 0; i < childCount; i++) {
    nsCOMPtr<nsIContent> child;
    aElement->ChildAt(i, *getter_AddRefs(child));
    nsCOMPtr<nsITextContent> text = do_QueryInterface(child);
    if (text) {
      nsAutoString result;
      text->CopyText(result);
      result.StripWhitespace();
      if (result.IsEmpty()) {
        // This node contained nothing but whitespace.
        // Remove it from the content model.
        aElement->RemoveChildAt(i, PR_TRUE);
        i--; // Decrement our count, since we just removed this child.
        childCount--; // Also decrement our total count.
      }
    }
    else StripWhitespaceNodes(child);
  }

  return NS_OK;
}

// Creation Routine ///////////////////////////////////////////////////////////////////////

nsresult
NS_NewXBLService(nsIXBLService** aResult)
{
  *aResult = new nsXBLService;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

