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
 *  Allan Beaufour <abeaufour@novell.com>
 *  Darin Fisher <darin@meer.net>
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

#include "nsXFormsModelElement.h"
#include "nsIXTFGenericElementWrapper.h"
#include "nsMemory.h"
#include "nsIDOMElement.h"
#include "nsIDOM3Node.h"
#include "nsIDOMNodeList.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsXFormsAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsINodeInfo.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMXPathResult.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMXPathExpression.h"
#include "nsIScriptGlobalObject.h"
#include "nsIContent.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIXFormsControl.h"
#include "nsXFormsTypes.h"
#include "nsXFormsXPathParser.h"
#include "nsXFormsXPathAnalyzer.h"
#include "nsIInstanceElementPrivate.h"
#include "nsXFormsUtils.h"

#include "nsISchemaLoader.h"
#include "nsISchema.h"
#include "nsAutoPtr.h"

//------------------------------------------------------------------------------

// Helper function for using XPath to locate an <xsd:schema> element by
// matching its "id" attribute.  This is necessary since <xsd:schema> is
// treated as an ordinary XML data node without an "ID" attribute.
static void
GetSchemaElementById(nsIDOMElement *contextNode,
                     const nsString &id,
                     nsIDOMElement **resultNode)
{
  // search for an element with the given "id" attribute, and then verify
  // that the element is in the XML Schema namespace.

  nsAutoString expr;
  expr.AssignLiteral("//*[@id=\"");
  expr.Append(id);
  expr.AppendLiteral("\"]");

  nsCOMPtr<nsIDOMXPathResult> xpRes =
      nsXFormsUtils::EvaluateXPath(expr,
                                   contextNode,
                                   contextNode,
                                   nsIDOMXPathResult::FIRST_ORDERED_NODE_TYPE);
  if (xpRes) {
    nsCOMPtr<nsIDOMNode> node;
    xpRes->GetSingleNodeValue(getter_AddRefs(node));
    if (node) {
      nsAutoString ns;
      node->GetNamespaceURI(ns);
      if (ns.EqualsLiteral(NS_NAMESPACE_XML_SCHEMA))
        CallQueryInterface(node, resultNode);
    }
  }
}

//------------------------------------------------------------------------------

static void
DeleteVoidArray(void    *aObject,
                nsIAtom *aPropertyName,
                void    *aPropertyValue,
                void    *aData)
{
  delete NS_STATIC_CAST(nsVoidArray *, aPropertyValue);
}

static nsresult
AddToModelList(nsIDOMDocument *domDoc, nsXFormsModelElement *model)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);

  nsVoidArray *models =
      NS_STATIC_CAST(nsVoidArray *,
                     doc->GetProperty(nsXFormsAtoms::modelListProperty));
  if (!models) {
    models = new nsVoidArray(16);
    if (!models)
      return NS_ERROR_OUT_OF_MEMORY;
    doc->SetProperty(nsXFormsAtoms::modelListProperty, models, DeleteVoidArray);
  }
  models->AppendElement(model);
  return NS_OK;
}

static void
RemoveFromModelList(nsIDOMDocument *domDoc, nsXFormsModelElement *model)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);

  nsVoidArray *models =
      NS_STATIC_CAST(nsVoidArray *,
                     doc->GetProperty(nsXFormsAtoms::modelListProperty));
  if (models)
    models->RemoveElement(model);
}

static const nsVoidArray *
GetModelList(nsIDOMDocument *domDoc)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);

  return NS_STATIC_CAST(nsVoidArray *,
                        doc->GetProperty(nsXFormsAtoms::modelListProperty));
}

//------------------------------------------------------------------------------

static const nsIID sScriptingIIDs[] = {
  NS_IDOMELEMENT_IID,
  NS_IDOMEVENTTARGET_IID,
  NS_IDOM3NODE_IID,
  NS_IXFORMSMODELELEMENT_IID
};

static nsIAtom* sModelPropsList[eModel__count];

nsXFormsModelElement::nsXFormsModelElement()
  : mElement(nsnull),
    mSchemaCount(0),
    mSchemaTotal(0),
    mPendingInstanceCount(0),
    mDocumentLoaded(PR_FALSE)
{
}

NS_IMPL_ISUPPORTS_INHERITED5(nsXFormsModelElement,
                             nsXFormsStubElement,
                             nsIXFormsModelElement,
                             nsIModelElementPrivate,
                             nsISchemaLoadListener,
                             nsIWebServiceErrorHandler,
                             nsIDOMEventListener)

NS_IMETHODIMP
nsXFormsModelElement::OnDestroyed()
{
  RemoveModelFromDocument();

  mElement = nsnull;
  mSchemas = nsnull;
  return NS_OK;
}

void
nsXFormsModelElement::RemoveModelFromDocument()
{
  mDocumentLoaded = PR_FALSE;

  nsCOMPtr<nsIDOMDocument> domDoc;
  mElement->GetOwnerDocument(getter_AddRefs(domDoc));
  if (!domDoc)
    return;

  RemoveFromModelList(domDoc, this);

  nsCOMPtr<nsIDOMEventTarget> targ = do_QueryInterface(domDoc);
  if (targ)
    targ->RemoveEventListener(NS_LITERAL_STRING("DOMContentLoaded"), this, PR_TRUE);
}

NS_IMETHODIMP
nsXFormsModelElement::GetScriptingInterfaces(PRUint32 *aCount, nsIID ***aArray)
{
  return nsXFormsUtils::CloneScriptingInterfaces(sScriptingIIDs,
                                                 NS_ARRAY_LENGTH(sScriptingIIDs),
                                                 aCount, aArray);
}

NS_IMETHODIMP
nsXFormsModelElement::WillChangeDocument(nsIDOMDocument* aNewDocument)
{
  RemoveModelFromDocument();
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::DocumentChanged(nsIDOMDocument* aNewDocument)
{
  if (!aNewDocument)
    return NS_OK;

  AddToModelList(aNewDocument, this);

  nsCOMPtr<nsIDOMEventTarget> targ = do_QueryInterface(aNewDocument);
  if (targ)
    targ->AddEventListener(NS_LITERAL_STRING("DOMContentLoaded"), this, PR_TRUE);
  
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::DoneAddingChildren()
{
  // We wait until all children are added to dispatch xforms-model-construct,
  // since the model may have an action handler for this event.

  nsresult rv = nsXFormsUtils::DispatchEvent(mElement, eEvent_ModelConstruct);
  NS_ENSURE_SUCCESS(rv, rv);

  // xforms-model-construct is not cancellable, so always proceed.
  // We continue here rather than doing this in HandleEvent since we know
  // it only makes sense to perform this default action once.

  // (XForms 4.2.1)
  // 1. load xml schemas

  nsAutoString schemaList;
  mElement->GetAttribute(NS_LITERAL_STRING("schema"), schemaList);
  if (!schemaList.IsEmpty()) {
    NS_ENSURE_TRUE(mSchemas, NS_ERROR_FAILURE);
    // Parse the whitespace-separated list.
    nsCOMPtr<nsIContent> content = do_QueryInterface(mElement);
    nsRefPtr<nsIURI> baseURI = content->GetBaseURI();

    nsCStringArray schemas;
    schemas.ParseString(NS_ConvertUTF16toUTF8(schemaList).get(), " \t\r\n");

    // Increase by 1 to prevent OnLoad from calling FinishConstruction
    mSchemaTotal = schemas.Count();

    for (PRInt32 i=0; i<mSchemaTotal; ++i) {
      nsCOMPtr<nsIURI> newURI;
      NS_NewURI(getter_AddRefs(newURI), *schemas[i], nsnull, baseURI);
      nsCOMPtr<nsIURL> newURL = do_QueryInterface(newURI);
      if (!newURL) {
        rv = NS_ERROR_UNEXPECTED;
      } else {
        // This code is copied from nsXMLEventsManager for extracting an
        // element ID from an xsd:anyURI link.
        nsCAutoString ref;
        newURL->GetRef(ref);
        newURL->SetRef(EmptyCString());
        PRBool equals = PR_FALSE;
        newURL->Equals(baseURI, &equals);
        if (equals) {
          // We will not be able to locate the <xsd:schema> element using the
          // getElementById function defined on our document when <xsd:schema>
          // is treated as an ordinary XML data node.  So, we employ XPath to
          // locate it for us.

          NS_ConvertUTF8toUTF16 id(ref);

          nsCOMPtr<nsIDOMElement> el;
          GetSchemaElementById(mElement, id, getter_AddRefs(el));
          if (!el) {
            // Perhaps the <xsd:schema> element appears after the <xforms:model>
            // element in the document, so we'll defer loading it until the
            // document has finished loading.
            mPendingInlineSchemas.AppendString(id);
          } else {
            nsCOMPtr<nsISchema> schema;
            // no need to observe errors via the callback.  instead, rely on
            // this method returning a failure code when it encounters errors.
            rv = mSchemas->ProcessSchemaElement(nsnull, el,
                                                getter_AddRefs(schema));
            if (NS_SUCCEEDED(rv))
              mSchemaCount++;
          }
        } else {
          nsCAutoString uriSpec;
          newURI->GetSpec(uriSpec);
          rv = mSchemas->LoadAsync(NS_ConvertUTF8toUTF16(uriSpec), this);
        }
      }
      if (NS_FAILED(rv)) {
        // this is a fatal error
        nsXFormsUtils::DispatchEvent(mElement, eEvent_LinkException);
        return NS_OK;
      }
    }
  }

  // 2. construct an XPath data model from inline or external initial instance
  // data.  This is done by our child instance elements as they are inserted
  // into the document, and all of the instances will be processed by this
  // point.

  // schema and external instance data loads should delay document onload

  if (IsComplete()) {
    // No need to fire refresh event if we assume that all UI controls
    // appear later in the document.
    NS_ASSERTION(!mDocumentLoaded, "document should not be loaded yet");
    return FinishConstruction();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::HandleDefault(nsIDOMEvent *aEvent, PRBool *aHandled)
{
  *aHandled = PR_TRUE;

  nsAutoString type;
  aEvent->GetType(type);

  if (type.EqualsLiteral("xforms-refresh")) {
    Refresh();
  } else if (type.EqualsLiteral("xforms-revalidate")) {
    Revalidate();
  } else if (type.EqualsLiteral("xforms-recalculate")) {
    Recalculate();
  } else if (type.EqualsLiteral("xforms-rebuild")) {
    Rebuild();
  } else if (type.EqualsLiteral("xforms-reset")) {
#ifdef DEBUG
    printf("nsXFormsModelElement::Reset()\n");
#endif    
  } else {
    *aHandled = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::OnCreated(nsIXTFGenericElementWrapper *aWrapper)
{
  aWrapper->SetNotificationMask(nsIXTFElement::NOTIFY_WILL_CHANGE_DOCUMENT |
                                nsIXTFElement::NOTIFY_DOCUMENT_CHANGED |
                                nsIXTFElement::NOTIFY_DONE_ADDING_CHILDREN |
                                nsIXTFElement::NOTIFY_HANDLE_DEFAULT);

  nsCOMPtr<nsIDOMElement> node;
  aWrapper->GetElementNode(getter_AddRefs(node));

  // It's ok to keep a weak pointer to mElement.  mElement will have an
  // owning reference to this object, so as long as we null out mElement in
  // OnDestroyed, it will always be valid.

  mElement = node;
  NS_ASSERTION(mElement, "Wrapper is not an nsIDOMElement, we'll crash soon");

  nsresult rv = mMDG.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mSchemas = do_GetService(NS_SCHEMALOADER_CONTRACTID);

  return NS_OK;
}

// nsIXFormsModelElement

NS_IMETHODIMP
nsXFormsModelElement::GetInstanceDocument(const nsAString& aInstanceID,
                                          nsIDOMDocument **aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);

  *aDocument = FindInstanceDocument(aInstanceID).get();  // transfer reference
  return *aDocument ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXFormsModelElement::Rebuild()
{
#ifdef DEBUG
  printf("nsXFormsModelElement::Rebuild()\n");
#endif

  // TODO: Clear graph and re-attach elements

  // 1 . Clear graph
  // mMDG.Clear();

  // 2. Re-attach all elements

  // 3. Rebuild graph
  return mMDG.Rebuild();
}

NS_IMETHODIMP
nsXFormsModelElement::Recalculate()
{
#ifdef DEBUG
  printf("nsXFormsModelElement::Recalculate()\n");
#endif
  
  nsAutoPtr<nsXFormsMDGSet> changedNodes(new nsXFormsMDGSet());
  // TODO: Handle changed nodes. That is, dispatch events, etc.

  nsXFormsMDGSet* ptr = changedNodes.get();

  return mMDG.Recalculate(&ptr);
}

NS_IMETHODIMP
nsXFormsModelElement::Revalidate()
{
#ifdef DEBUG
  printf("nsXFormsModelElement::Revalidate()\n");
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Refresh()
{
#ifdef DEBUG
  printf("nsXFormsModelElement::Refresh()\n");
#endif

  // refresh all of our form controls
  PRInt32 controlCount = mFormControls.Count();
  for (PRInt32 i = 0; i < controlCount; ++i) {
    NS_STATIC_CAST(nsIXFormsControl*, mFormControls[i])->Refresh();
  }

  return NS_OK;
}

// nsISchemaLoadListener

NS_IMETHODIMP
nsXFormsModelElement::OnLoad(nsISchema* aSchema)
{
  mSchemaCount++;

  if (IsComplete()) {
    nsresult rv = FinishConstruction();
    NS_ENSURE_SUCCESS(rv, rv);

    nsXFormsUtils::DispatchEvent(mElement, eEvent_Refresh);

    MaybeNotifyCompletion();
  }

  return NS_OK;
}

// nsIWebServiceErrorHandler

NS_IMETHODIMP
nsXFormsModelElement::OnError(nsresult aStatus,
                              const nsAString &aStatusMessage)
{
  nsXFormsUtils::DispatchEvent(mElement, eEvent_LinkException);
  return NS_OK;
}

// nsIDOMEventListener

NS_IMETHODIMP
nsXFormsModelElement::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);
  if (!type.EqualsLiteral("DOMContentLoaded"))
    return NS_OK;

  mDocumentLoaded = PR_TRUE;

  if (mPendingInlineSchemas.Count() > 0) {
    nsCOMPtr<nsIDOMElement> el;
    nsresult rv;
    for (PRInt32 i=0; i<mPendingInlineSchemas.Count(); ++i) {
      GetSchemaElementById(mElement, *mPendingInlineSchemas[i],
                           getter_AddRefs(el));
      if (!el) {
        rv = NS_ERROR_UNEXPECTED;
      } else {
        nsCOMPtr<nsISchema> schema;
        // no need to observe errors via the callback.  instead, rely on
        // this method returning a failure code when it encounters errors.
        rv = mSchemas->ProcessSchemaElement(nsnull, el,
                                            getter_AddRefs(schema));
        if (NS_SUCCEEDED(rv))
          mSchemaCount++;
      }
      if (NS_FAILED(rv)) {
        // this is a fatal error
        nsXFormsUtils::DispatchEvent(mElement, eEvent_LinkException);
        return NS_OK;
      }
    }
    if (IsComplete()) {
      rv = FinishConstruction();
      NS_ENSURE_SUCCESS(rv, rv);
      nsXFormsUtils::DispatchEvent(mElement, eEvent_Refresh);
    }
    mPendingInlineSchemas.Clear();
  }

  // We may still be waiting on external documents to load.
  MaybeNotifyCompletion();

  return NS_OK;
}

// nsIModelElementPrivate

NS_IMETHODIMP
nsXFormsModelElement::AddFormControl(nsIXFormsControl *aControl)
{
  if (mFormControls.IndexOf(aControl) == -1)
    mFormControls.AppendElement(aControl);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::RemoveFormControl(nsIXFormsControl *aControl)
{
  mFormControls.RemoveElement(aControl);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::GetTypeForControl(nsIXFormsControl  *aControl,
                                        nsISchemaType    **aType)
{
  *aType = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::InstanceLoadStarted()
{
  ++mPendingInstanceCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::InstanceLoadFinished(PRBool aSuccess)
{
  --mPendingInstanceCount;
  if (!aSuccess) {
    nsXFormsUtils::DispatchEvent(mElement, eEvent_LinkException);
  } else if (IsComplete()) {
    nsresult rv = FinishConstruction();
    if (NS_SUCCEEDED(rv)) {
      nsXFormsUtils::DispatchEvent(mElement, eEvent_Refresh);
      MaybeNotifyCompletion();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::FindInstanceElement(const nsAString &aID,
                                          nsIInstanceElementPrivate **aElement)
{
  *aElement = nsnull;

  nsCOMPtr<nsIDOMNodeList> children;
  mElement->GetChildNodes(getter_AddRefs(children));

  if (!children)
    return NS_OK;

  PRUint32 childCount = 0;
  children->GetLength(&childCount);

  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIDOMElement> element;
  nsAutoString id;

  for (PRUint32 i = 0; i < childCount; ++i) {
    children->Item(i, getter_AddRefs(node));
    NS_ASSERTION(node, "incorrect NodeList length?");

    element = do_QueryInterface(node);
    if (!element)
      continue;

    element->GetAttribute(NS_LITERAL_STRING("id"), id);
    if (aID.IsEmpty() || aID.Equals(id)) {
      CallQueryInterface(element, aElement);
      if (*aElement)
        break;
    }
  }

  return NS_OK;
}

// internal methods

already_AddRefed<nsIDOMDocument>
nsXFormsModelElement::FindInstanceDocument(const nsAString &aID)
{
  nsCOMPtr<nsIInstanceElementPrivate> instance;
  nsXFormsModelElement::FindInstanceElement(aID, getter_AddRefs(instance));

  nsIDOMDocument *doc = nsnull;
  if (instance)
    instance->GetDocument(&doc); // addrefs

  return doc;
}

nsresult
nsXFormsModelElement::FinishConstruction()
{
  // 3. if applicable, initialize P3P

  // 4. construct instance data from initial instance data.  apply all
  // <bind> elements in document order.

  // we get the instance data from our instance child nodes

  nsCOMPtr<nsIDOMDocument> firstInstanceDoc =
    FindInstanceDocument(EmptyString());
  if (!firstInstanceDoc)
    return NS_OK;

  nsCOMPtr<nsIDOMElement> firstInstanceRoot;
  firstInstanceDoc->GetDocumentElement(getter_AddRefs(firstInstanceRoot));

  nsresult rv;
  nsCOMPtr<nsIDOMXPathEvaluator> xpath = 
           do_CreateInstance("@mozilla.org/dom/xforms-xpath-evaluator;1", &rv);
  NS_ENSURE_TRUE(xpath, rv);
  
  nsCOMPtr<nsIDOMNodeList> children;
  mElement->GetChildNodes(getter_AddRefs(children));

  PRUint32 childCount = 0;
  if (children)
    children->GetLength(&childCount);

  nsAutoString namespaceURI, localName;

  for (PRUint32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDOMNode> child;
    children->Item(i, getter_AddRefs(child));
    NS_ASSERTION(child, "there can't be null items in the NodeList!");

    child->GetLocalName(localName);
    if (localName.EqualsLiteral("bind")) {
      child->GetNamespaceURI(namespaceURI);
      if (namespaceURI.EqualsLiteral(NS_NAMESPACE_XFORMS)) {
        if (!ProcessBind(xpath, firstInstanceRoot, 1, 1,
                         nsCOMPtr<nsIDOMElement>(do_QueryInterface(child)))) {
          nsXFormsUtils::DispatchEvent(mElement, eEvent_BindingException);
          return NS_OK;
        }
      }
    }
  }

  // 5. dispatch xforms-rebuild, xforms-recalculate, xforms-revalidate

  nsXFormsUtils::DispatchEvent(mElement, eEvent_Rebuild);
  nsXFormsUtils::DispatchEvent(mElement, eEvent_Recalculate);
  nsXFormsUtils::DispatchEvent(mElement, eEvent_Revalidate);

  // We're done initializing this model.

  return NS_OK;
}

void
nsXFormsModelElement::MaybeNotifyCompletion()
{
  nsCOMPtr<nsIDOMDocument> domDoc;
  mElement->GetOwnerDocument(getter_AddRefs(domDoc));

  const nsVoidArray *models = GetModelList(domDoc);
  if (!models) {
    NS_NOTREACHED("no model list property");
    return;
  }
  
  PRInt32 i;

  // Nothing to be done if any model is incomplete or hasn't seen
  // DOMContentLoaded.
  for (i = 0; i < models->Count(); ++i) {
    nsXFormsModelElement *model =
        NS_STATIC_CAST(nsXFormsModelElement *, models->ElementAt(i));
    if (!model->mDocumentLoaded || !model->IsComplete())
      return;
  }

  // Okay, dispatch xforms-model-construct-done and xforms-ready events!
  for (i = 0; i < models->Count(); ++i) {
    nsXFormsModelElement *model =
        NS_STATIC_CAST(nsXFormsModelElement *, models->ElementAt(i));
    nsXFormsUtils::DispatchEvent(model->mElement, eEvent_ModelConstructDone);
    nsXFormsUtils::DispatchEvent(model->mElement, eEvent_Ready);
  }
}

static void
ReleaseExpr(void    *aElement,
            nsIAtom *aPropertyName,
            void    *aPropertyValue,
            void    *aData)
{
  nsIDOMXPathExpression *expr = NS_STATIC_CAST(nsIDOMXPathExpression*,
                                               aPropertyValue);

  NS_RELEASE(expr);
}

PRBool
nsXFormsModelElement::ProcessBind(nsIDOMXPathEvaluator *aEvaluator,
                                  nsIDOMNode           *aContextNode,
                                  PRInt32              aContextPosition,
                                  PRInt32              aContextSize,
                                  nsIDOMElement        *aBindElement)
{
  // Get the model item properties specified by this <bind>.
  nsCOMPtr<nsIDOMXPathExpression> props[eModel__count];
  nsAutoString exprStrings[eModel__count];
  PRInt32 propCount = 0;
  nsresult rv = NS_OK;
  nsAutoString attrStr;

  nsCOMPtr<nsIDOMXPathNSResolver> resolver;
  aEvaluator->CreateNSResolver(aBindElement, getter_AddRefs(resolver));

  for (int i = 0; i < eModel__count; ++i) {
    sModelPropsList[i]->ToString(attrStr);

    aBindElement->GetAttribute(attrStr, exprStrings[i]);
    if (!exprStrings[i].IsEmpty()) {
      rv = aEvaluator->CreateExpression(exprStrings[i], resolver,
                                        getter_AddRefs(props[i]));
      if (NS_FAILED(rv)) {
        nsXFormsUtils::DispatchEvent(mElement, eEvent_ComputeException);
        return PR_FALSE;
      }

      ++propCount;
    }
  }

  // Find the nodeset that this bind applies to.
  nsCOMPtr<nsIDOMXPathResult> result;

  nsAutoString expr;
  aBindElement->GetAttribute(NS_LITERAL_STRING("nodeset"), expr);
  if (expr.IsEmpty()) {
    expr = NS_LITERAL_STRING(".");
  }
  ///
  /// @todo use aContextSize and aContextPosition in evaluation (XXX)
  rv = aEvaluator->Evaluate(expr, aContextNode, resolver,
                            nsIDOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE,
                            nsnull, getter_AddRefs(result));

  if (NS_FAILED(rv)) {
    nsXFormsUtils::DispatchEvent(mElement, eEvent_BindingException);
    return PR_FALSE; // dispatch a binding exception
  }

  NS_ENSURE_TRUE(result, PR_FALSE);

  PRUint32 snapLen;
  rv = result->GetSnapshotLength(&snapLen);
  NS_ENSURE_SUCCESS(rv, rv);
  

  // Iterate over resultset
  nsXFormsMDGSet set;
  nsCOMPtr<nsIDOMNode> node;
  PRInt32 contextPosition = 1;
  PRUint32 snapItem;
  for (snapItem = 0; snapItem < snapLen; ++snapItem) {
    rv = result->SnapshotItem(snapItem, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!node) {
      NS_WARNING("nsXFormsModelElement::ProcessBind(): Empty node in result set.");
      continue;
    }
    

    // Apply MIPs
    nsXFormsXPathParser parser;
    nsXFormsXPathAnalyzer analyzer(aEvaluator, resolver);
    
    // We must check whether the properties already exist on the node.
    for (int j = 0; j < eModel__count; ++j) {
      if (props[j]) {
        nsCOMPtr<nsIContent> content = do_QueryInterface(node, &rv);

        if (NS_FAILED(rv)) {
          NS_WARNING("nsXFormsModelElement::ProcessBind(): Node is not IContent!\n");
          continue;
        }

        nsIDOMXPathExpression *expr = props[j];
        NS_ADDREF(expr);

        // Set property
        rv = content->SetProperty(sModelPropsList[j], expr, ReleaseExpr);
        if (rv == NS_PROPTABLE_PROP_OVERWRITTEN) {
          return PR_FALSE;
        }
        
        // Get node dependencies
        nsAutoPtr<nsXFormsXPathNode> xNode(parser.Parse(exprStrings[j]));
        set.Clear();
        rv = analyzer.Analyze(node, xNode, expr, &exprStrings[j], &set);
        NS_ENSURE_SUCCESS(rv, rv);
        
        // Insert into MDG
        rv = mMDG.AddMIP((ModelItemPropName) j, expr, &set, parser.UsesDynamicFunc(),
                         node, contextPosition++, snapLen);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  // Now evaluate any child <bind> elements.
  nsCOMPtr<nsIDOMNode> childContext;
  result->SnapshotItem(0, getter_AddRefs(childContext));

  nsCOMPtr<nsIDOMNodeList> children;
  aBindElement->GetChildNodes(getter_AddRefs(children));
  if (children) {
    PRUint32 childCount = 0;
    children->GetLength(&childCount);

    nsCOMPtr<nsIDOMNode> child;
    nsAutoString value;

    for (PRUint32 k = 0; k < childCount; ++k) {
      children->Item(k, getter_AddRefs(child));
      if (child) {
        child->GetLocalName(value);
        if (!value.EqualsLiteral("bind"))
          continue;

        child->GetNamespaceURI(value);
        if (!value.EqualsLiteral(NS_NAMESPACE_XFORMS))
          continue;

        if (!ProcessBind(aEvaluator, node,
                         snapItem + 1, snapLen,
                         nsCOMPtr<nsIDOMElement>(do_QueryInterface(child))))
          return PR_FALSE;

      }
    }
  }

  return PR_TRUE;
}

/* static */ void
nsXFormsModelElement::Startup()
{
  sModelPropsList[eModel_type] = nsXFormsAtoms::type;
  sModelPropsList[eModel_readonly] = nsXFormsAtoms::readonly;
  sModelPropsList[eModel_required] = nsXFormsAtoms::required;
  sModelPropsList[eModel_relevant] = nsXFormsAtoms::relevant;
  sModelPropsList[eModel_calculate] = nsXFormsAtoms::calculate;
  sModelPropsList[eModel_constraint] = nsXFormsAtoms::constraint;
  sModelPropsList[eModel_p3ptype] = nsXFormsAtoms::p3ptype;
}

nsresult
NS_NewXFormsModelElement(nsIXTFElement **aResult)
{
  *aResult = new nsXFormsModelElement();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
