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
 * The Original Code is Mozilla XForms support.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@brianryner.com>
 *  Olli Pettay <Olli.Pettay@helsinki.fi>
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

#include "nsXFormsInstanceElement.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOM3Node.h"
#include "nsIDocument.h"
#include "nsMemory.h"
#include "nsXFormsAtoms.h"
#include "nsString.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIXTFGenericElementWrapper.h"
#include "nsXFormsUtils.h"
#include "nsNetUtil.h"

static const char* kLoadAsData = "loadAsData";

NS_IMPL_ISUPPORTS_INHERITED5(nsXFormsInstanceElement,
                             nsXFormsStubElement,
                             nsIInstanceElementPrivate,
                             nsIStreamListener,
                             nsIRequestObserver,
                             nsIInterfaceRequestor,
                             nsIChannelEventSink)

nsXFormsInstanceElement::nsXFormsInstanceElement()
  : mElement(nsnull)
  , mIgnoreAttributeChanges(PR_FALSE)
{
}

NS_IMETHODIMP
nsXFormsInstanceElement::OnDestroyed()
{
  if (mChannel) {
    // better be a good citizen and tell the browser that we don't need this
    // resource anymore.
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
  }
  mListener = nsnull;
  mElement = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::AttributeSet(nsIAtom *aName,
                                      const nsAString &aNewValue)
{
  if (mIgnoreAttributeChanges)
    return NS_OK;

  if (aName == nsXFormsAtoms::src) {
    LoadExternalInstance(aNewValue);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::AttributeRemoved(nsIAtom *aName)
{
  if (mIgnoreAttributeChanges)
    return NS_OK;

  if (aName == nsXFormsAtoms::src) {
    PRBool restart = PR_FALSE;
    if (mChannel) {
      // looks like we are trying to remove the src attribute while we are
      // already trying to load the external instance document.  We'll stop
      // the current load effort.
      restart = PR_TRUE;
      mChannel->Cancel(NS_BINDING_ABORTED);
      mChannel = nsnull;
      mListener = nsnull;
    }
    // We no longer have an external instance to use.  Reset our instance
    // document to whatever inline content we have.
    nsresult rv = CloneInlineInstance();

    // if we had already started to load an external instance document, then
    // as part of that we would have told the model to wait for that external
    // document to load before it finishes the model construction.  Since we
    // aren't loading from an external document any longer, tell the model that
    // there is need to wait for us anymore.
    if (restart) {
      nsCOMPtr<nsIModelElementPrivate> model = GetModel();
      if (model) {
        model->InstanceLoadFinished(PR_TRUE);
      }
    }
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::BeginAddingChildren()
{
  // Ignore attribute changes during document construction.  Attributes will be
  // handled in the DoneAddingChildren.
  mIgnoreAttributeChanges = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::DoneAddingChildren()
{
  // By the time this is called, we should be inserted in the document and have
  // all of our child elements, so this is our first opportunity to create the
  // instance document.

  nsAutoString src;
  mElement->GetAttribute(NS_LITERAL_STRING("src"), src);

  if (src.IsEmpty()) {
    // If we don't have a linked external instance, use our inline data.
    CloneInlineInstance();
  } else {
    LoadExternalInstance(src);
  }

  // Now, observe changes to the "src" attribute.
  mIgnoreAttributeChanges = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::OnCreated(nsIXTFGenericElementWrapper *aWrapper)
{
  aWrapper->SetNotificationMask(nsIXTFElement::NOTIFY_ATTRIBUTE_SET |
                                nsIXTFElement::NOTIFY_ATTRIBUTE_REMOVED |
                                nsIXTFElement::NOTIFY_PARENT_CHANGED |
                                nsIXTFElement::NOTIFY_BEGIN_ADDING_CHILDREN |
                                nsIXTFElement::NOTIFY_DONE_ADDING_CHILDREN);

  nsCOMPtr<nsIDOMElement> node;
  aWrapper->GetElementNode(getter_AddRefs(node));

  // It's ok to keep a weak pointer to mElement.  mElement will have an owning
  // reference to this object, so as long as we null out mElement in
  // OnDestroyed, it will always be valid.

  mElement = node;
  NS_ASSERTION(mElement, "Wrapper is not an nsIDOMElement, we'll crash soon");

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::ParentChanged(nsIDOMElement *aNewParent)
{
  if (!aNewParent)
    return NS_OK;

  // Once we are set up in the DOM, can find the model and make sure that this
  // instance is on the list of instance elements that model keeps
  nsCOMPtr<nsIModelElementPrivate> model = GetModel();
  NS_ENSURE_TRUE(model, NS_ERROR_FAILURE);
  model->AddInstanceElement(this);
  
  return NS_OK;
}

// nsIInterfaceRequestor

NS_IMETHODIMP
nsXFormsInstanceElement::GetInterface(const nsIID & aIID, void **aResult)
{
  *aResult = nsnull;
  return QueryInterface(aIID, aResult);
}

// nsIChannelEventSink

NS_IMETHODIMP
nsXFormsInstanceElement::OnChannelRedirect(nsIChannel *OldChannel,
                                           nsIChannel *aNewChannel,
                                           PRUint32    aFlags)
{
  NS_PRECONDITION(aNewChannel, "Redirect without a channel?");
  nsCOMPtr<nsIURI> newURI;
  nsresult rv = aNewChannel->GetURI(getter_AddRefs(newURI));
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_ENSURE_STATE(mElement);
  nsCOMPtr<nsIDOMDocument> domDoc;
  mElement->GetOwnerDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
  NS_ENSURE_STATE(doc);

  if (!nsXFormsUtils::CheckSameOrigin(doc->GetDocumentURI(), newURI)) {
    nsXFormsUtils::ReportError(NS_LITERAL_STRING("instanceLoadOrigin"), domDoc);
    return NS_ERROR_ABORT;
  }

  return NS_OK;
}

// nsIStreamListener

// It is possible that mListener could be null here.  If the document hit a
// parsing error after we've already started to load the external source, then
// Mozilla will destroy all of the elements and the document in order to load
// the parser error page.  This will cause mListener to become null but since
// the channel will hold a nsCOMPtr to the nsXFormsInstanceElement preventing
// it from being freed up, the channel will still be able to call the
// nsIStreamListener functions that we implement here.  And calling
// mChannel->Cancel() is no guarantee that these other notifications won't come
// through if the timing is wrong.  So we need to check for mElement below
// before we handle any of the stream notifications.

NS_IMETHODIMP
nsXFormsInstanceElement::OnStartRequest(nsIRequest *request, nsISupports *ctx)
{
  if (!mElement || !mListener) {
    return NS_OK;
  }
  return mListener->OnStartRequest(request, ctx);
}

NS_IMETHODIMP
nsXFormsInstanceElement::OnDataAvailable(nsIRequest *aRequest,
                                         nsISupports *ctxt,
                                         nsIInputStream *inStr,
                                         PRUint32 sourceOffset,
                                         PRUint32 count)
{
  if (!mElement || !mListener) {
    return NS_OK;
  }
  return mListener->OnDataAvailable(aRequest, ctxt, inStr, sourceOffset, count);
}

NS_IMETHODIMP
nsXFormsInstanceElement::OnStopRequest(nsIRequest *request, nsISupports *ctx,
                                       nsresult status)
{
  if (status == NS_BINDING_ABORTED) {
    // looks like our element has already been destroyed.  No use continuing on.
    return NS_OK;
  }

  mChannel = nsnull;
  NS_ASSERTION(mListener, "No stream listener for document!");
  mListener->OnStopRequest(request, ctx, status);

  PRBool succeeded = NS_SUCCEEDED(status);
  if (!succeeded)
    mDocument = nsnull;

  if (mDocument) {
    nsCOMPtr<nsIDOMElement> docElem;
    mDocument->GetDocumentElement(getter_AddRefs(docElem));
    if (docElem) {
      nsAutoString tagName, namespaceURI;
      docElem->GetTagName(tagName);
      docElem->GetNamespaceURI(namespaceURI);
  
      if (tagName.EqualsLiteral("parsererror") &&
          namespaceURI.EqualsLiteral("http://www.mozilla.org/newlayout/xml/parsererror.xml")) {
        NS_WARNING("resulting instance document could not be parsed");
        succeeded = PR_FALSE;
        mDocument = nsnull;
      }
    }
  }

  nsCOMPtr<nsIModelElementPrivate> model = GetModel();
  if (model) {
    model->InstanceLoadFinished(succeeded);
  }

  mListener = nsnull;
  return NS_OK;
}

// nsIInstanceElementPrivate

NS_IMETHODIMP
nsXFormsInstanceElement::GetDocument(nsIDOMDocument **aDocument)
{
  NS_IF_ADDREF(*aDocument = mDocument);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::SetDocument(nsIDOMDocument *aDocument)
{
  mDocument = aDocument;
  return NS_OK;
}


NS_IMETHODIMP
nsXFormsInstanceElement::BackupOriginalDocument()
{
  nsresult rv = NS_OK;

  // This is called when xforms-ready is received by the model.  By now we know 
  // that the instance document, whether external or inline, is loaded into 
  // mDocument.  Get the root node, clone it, and insert it into our copy of 
  // the document.  This is the magic behind getting xforms-reset to work.
  if(mDocument && mOriginalDocument) {
    nsCOMPtr<nsIDOMNode> newNode;
    nsCOMPtr<nsIDOMElement> instanceRoot;
    rv = mDocument->GetDocumentElement(getter_AddRefs(instanceRoot));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(instanceRoot, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMNode> nodeReturn;
    rv = instanceRoot->CloneNode(PR_TRUE, getter_AddRefs(newNode)); 
    if(NS_SUCCEEDED(rv)) {
      rv = mOriginalDocument->AppendChild(newNode, getter_AddRefs(nodeReturn));
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), 
                       "failed to set up original instance document");
    }
  }
  return rv;
}

NS_IMETHODIMP
nsXFormsInstanceElement::RestoreOriginalDocument()
{
  nsresult rv = NS_OK;

  // This is called when xforms-reset is received by the model.  We assume
  // that the backup of the instance document has been populated and is
  // loaded into mOriginalDocument.  Get the backup's root node, clone it, and 
  // insert it into the live copy of the instance document.  This is the magic 
  // behind getting xforms-reset to work. 
  if(mDocument && mOriginalDocument) {
    nsCOMPtr<nsIDOMNode> newNode, instanceRootNode, nodeReturn;
    nsCOMPtr<nsIDOMElement> instanceRoot;

    // first remove all the old stuff
    rv = mDocument->GetDocumentElement(getter_AddRefs(instanceRoot));
    if(NS_SUCCEEDED(rv)) {
      if(instanceRoot) {
        rv = mDocument->RemoveChild(instanceRoot, getter_AddRefs(nodeReturn));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    // now all of the garbage is out o' there!  Put the original data back
    // into mDocument
    rv = mOriginalDocument->GetDocumentElement(getter_AddRefs(instanceRoot));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(instanceRoot, NS_ERROR_FAILURE);
    instanceRootNode = do_QueryInterface(instanceRoot);

    rv = instanceRootNode->CloneNode(PR_TRUE, getter_AddRefs(newNode)); 
    if(NS_SUCCEEDED(rv)) {
      rv = mDocument->AppendChild(newNode, getter_AddRefs(nodeReturn));
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), 
                       "failed to restore original instance document");
    }
  }
  return rv;
}

NS_IMETHODIMP
nsXFormsInstanceElement::GetElement(nsIDOMElement **aElement)
{
  NS_IF_ADDREF(*aElement = mElement);
  return NS_OK;  
}

// private methods

nsresult
nsXFormsInstanceElement::CloneInlineInstance()
{
  // Clear out our existing instance data
  nsresult rv = CreateInstanceDocument();
  if (NS_FAILED(rv))
    return rv; // don't warn, we might just not be in the document yet

  // look for our first child element (skip over text nodes, etc.)
  nsCOMPtr<nsIDOMNode> child, temp;
  mElement->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 nodeType;
    child->GetNodeType(&nodeType);

    if (nodeType == nsIDOMNode::ELEMENT_NODE)
      break;

    temp.swap(child);
    temp->GetNextSibling(getter_AddRefs(child));
  }

  if (child) {
    nsCOMPtr<nsIDOMNode> newNode;
    rv = mDocument->ImportNode(child, PR_TRUE, getter_AddRefs(newNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNode> nodeReturn;
    rv = mDocument->AppendChild(newNode, getter_AddRefs(nodeReturn));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to append root instance node");
  }

  return rv;
}

void
nsXFormsInstanceElement::LoadExternalInstance(const nsAString &aSrc)
{
  nsresult rv = NS_ERROR_FAILURE;

  PRBool restart = PR_FALSE;
  if (mChannel) {
    // probably hit this condition because someone changed the value of our
    // src attribute while we are already trying to load the previously
    // specified document.  We'll stop the current load effort and kick off the
    // new attempt.
    restart = PR_TRUE;
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
    mListener = nsnull;
  }

  // Clear out our existing instance data
  if (NS_SUCCEEDED(CreateInstanceDocument())) {
    nsCOMPtr<nsIDocument> newDoc = do_QueryInterface(mDocument);

    nsCOMPtr<nsIDOMDocument> domDoc;
    mElement->GetOwnerDocument(getter_AddRefs(domDoc));
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
    if (doc) {
      nsCOMPtr<nsIURI> uri;
      NS_NewURI(getter_AddRefs(uri), aSrc, doc->GetDocumentCharacterSet().get(),
                doc->GetDocumentURI());
      if (uri) {
        if (nsXFormsUtils::CheckSameOrigin(doc->GetDocumentURI(), uri)) {
          nsCOMPtr<nsILoadGroup> loadGroup;
          loadGroup = doc->GetDocumentLoadGroup();
          NS_WARN_IF_FALSE(loadGroup, "No load group!");

          // Using the same load group as the main document and creating
          // the channel with LOAD_NORMAL flag delays the dispatching of
          // the 'load' event until all instance data documents have been loaded.
          NS_NewChannel(getter_AddRefs(mChannel), uri, nsnull, loadGroup,
                        nsnull, nsIRequest::LOAD_NORMAL);

          if (mChannel) {
            rv = newDoc->StartDocumentLoad(kLoadAsData, mChannel, loadGroup, nsnull,
                                           getter_AddRefs(mListener), PR_TRUE);
            if (NS_SUCCEEDED(rv)) {
              mChannel->SetNotificationCallbacks(this);
              rv = mChannel->AsyncOpen(this, nsnull);
            }
          }
        } else {
          nsXFormsUtils::ReportError(NS_LITERAL_STRING("instanceLoadOrigin"),
                                     domDoc);
        }
      }
    }
  }

  nsCOMPtr<nsIModelElementPrivate> model = GetModel();
  if (model) {
    // if this isn't the first time that this instance element has tried
    // to load an external document, then we don't need to tell the model
    // to wait again.  It would screw up the counter it uses.  But if this
    // isn't a restart, then by golly, the model better wait for us!
    if (!restart) {
      model->InstanceLoadStarted();
    }
    if (NS_FAILED(rv)) {
      model->InstanceLoadFinished(PR_FALSE);
    }
  }
}

nsresult
nsXFormsInstanceElement::CreateInstanceDocument()
{
  nsCOMPtr<nsIDOMDocument> doc;
  nsresult rv = mElement->GetOwnerDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!doc) // could be we just aren't inserted yet, so don't warn
    return NS_ERROR_FAILURE;

  // Do not try to load an instance document if the current document is not
  // associated with a DOM window.  This could happen, for example, if some
  // XForms document loaded itself as instance data (which is what the Forms
  // 1.0 testsuite does).
  nsCOMPtr<nsIDocument> d = do_QueryInterface(doc);
  if (d && !d->GetScriptGlobalObject())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMDOMImplementation> domImpl;
  rv = doc->GetImplementation(getter_AddRefs(domImpl));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = domImpl->CreateDocument(EmptyString(), EmptyString(), nsnull,
                               getter_AddRefs(mDocument));
  NS_ENSURE_SUCCESS(rv, rv);

  // I don't know if not being able to create a backup document is worth
  // failing this function.  Since it probably won't be used often, we'll
  // let it slide.  But it probably does mean that things are going south
  // with the browser.
  domImpl->CreateDocument(EmptyString(), EmptyString(), nsnull,
                          getter_AddRefs(mOriginalDocument));
  return rv;
}

already_AddRefed<nsIModelElementPrivate>
nsXFormsInstanceElement::GetModel()
{
  if (!mElement)
  {
    NS_WARNING("The XTF wrapper element has been destroyed");
    return nsnull;
  }

  nsCOMPtr<nsIDOMNode> parentNode;
  mElement->GetParentNode(getter_AddRefs(parentNode));

  nsIModelElementPrivate *model = nsnull;
  if (parentNode)
    CallQueryInterface(parentNode, &model);
  return model;
}

nsresult
NS_NewXFormsInstanceElement(nsIXTFElement **aResult)
{
  *aResult = new nsXFormsInstanceElement();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
