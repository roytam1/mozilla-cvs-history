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

#include "nsXFormsUtils.h"
#include "nsString.h"
#include "nsXFormsAtoms.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsIDOMXPathResult.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMDocument.h"
#include "nsIDOMText.h"
#include "nsIXFormsModelElement.h"

/* static */ nsIDOMNode*
nsXFormsUtils::GetParentModel(nsIDOMElement *aElement)
{
  nsCOMPtr<nsIDOMNode> modelWrapper;

  // Walk up the tree looking for the containing model.
  aElement->GetParentNode(getter_AddRefs(modelWrapper));

  nsAutoString localName, namespaceURI;
  nsCOMPtr<nsIDOMNode> temp;

  while (modelWrapper) {
    modelWrapper->GetLocalName(localName);
    if (localName.EqualsLiteral("model")) {
      modelWrapper->GetNamespaceURI(namespaceURI);
      if (namespaceURI.EqualsLiteral(NS_NAMESPACE_XFORMS))
        break;
    }

    temp.swap(modelWrapper);
    temp->GetParentNode(getter_AddRefs(modelWrapper));
  }

  // We're releasing this reference, but the node is owned by the DOM.
  return modelWrapper;
}

/* static */ nsIDOMNode*
nsXFormsUtils::GetModelAndBind(nsIDOMElement  *aElement,
                               PRUint32        aElementFlags,
                               nsIDOMElement **aBindElement)
{
  *aBindElement = nsnull;
  NS_ENSURE_TRUE(aElement, nsnull);

  nsCOMPtr<nsIDOMDocument> domDoc;
  aElement->GetOwnerDocument(getter_AddRefs(domDoc));
  if (!domDoc)
    return nsnull;

  nsAutoString bindId;
  aElement->GetAttribute(NS_LITERAL_STRING("bind"), bindId);

  if (!bindId.IsEmpty()) {
    // Get the bind element with the given id.
    domDoc->GetElementById(bindId, aBindElement);

    if (*aBindElement)
      return GetParentModel(*aBindElement);
  }

  if (aElementFlags & ELEMENT_WITH_MODEL_ATTR) {
    // If no bind was given, we use model.
    nsAutoString modelId;
    aElement->GetAttribute(NS_LITERAL_STRING("model"), modelId);

    nsCOMPtr<nsIDOMNode> modelWrapper;

    if (modelId.IsEmpty()) {
      // No model given, so use the first one in the document.
      nsCOMPtr<nsIDOMNodeList> nodes;
      domDoc->GetElementsByTagNameNS(NS_LITERAL_STRING(NS_NAMESPACE_XFORMS),
                                     NS_LITERAL_STRING("model"),
                                     getter_AddRefs(nodes));

      if (!nodes)
        return nsnull;

      nodes->Item(0, getter_AddRefs(modelWrapper));
    } else {
      nsCOMPtr<nsIDOMElement> wrapperElement;
      domDoc->GetElementById(modelId, getter_AddRefs(wrapperElement));
      modelWrapper = wrapperElement;
    }

    // We're releasing this reference, but the node is owned by the DOM.
    return modelWrapper;
  }

  // If no bind was given, we assume the given element is a child
  // of the model.
  return GetParentModel(aElement);
}

/* static */ already_AddRefed<nsIDOMXPathResult>
nsXFormsUtils::EvaluateXPath(const nsAString &aExpression,
                             nsIDOMNode      *aContextNode,
                             nsIDOMNode      *aResolverNode,
                             PRUint16         aResultType)
{
  nsCOMPtr<nsIDOMDocument> doc;
  aContextNode->GetOwnerDocument(getter_AddRefs(doc));
  NS_ENSURE_TRUE(doc, nsnull);

  nsCOMPtr<nsIDOMXPathEvaluator> eval = do_QueryInterface(doc);
  NS_ENSURE_TRUE(eval, nsnull);

  nsCOMPtr<nsIDOMXPathNSResolver> resolver;
  eval->CreateNSResolver(aResolverNode, getter_AddRefs(resolver));
  NS_ENSURE_TRUE(resolver, nsnull);

  nsCOMPtr<nsISupports> supResult;
  eval->Evaluate(aExpression, aContextNode, resolver, aResultType, nsnull,
                 getter_AddRefs(supResult));

  nsIDOMXPathResult *result = nsnull;
  if (supResult)
    CallQueryInterface(supResult, &result);  // addrefs
  return result;
}

/* static */ already_AddRefed<nsIDOMNode>
nsXFormsUtils::FindBindContext(nsIDOMElement         *aElement,
                               nsIXFormsModelElement *aModel)
{
  // Figure out the context node for this <bind>.
  // The outermost bind has the root element of the [first] instance document
  // as its context node.  For inner binds, the first node in the nodeset
  // of the parent is used as the context node.

  nsCOMPtr<nsIDOMNode> modelNode = do_QueryInterface(aModel);

  nsCOMPtr<nsIDOMNode> parentNode;
  aElement->GetParentNode(getter_AddRefs(parentNode));
  if (!parentNode)
    return nsnull; // illegal, a bind must be contained in a model.

  if (parentNode == modelNode) {
    // This is the outermost bind.
    nsCOMPtr<nsIDOMDocument> instanceDoc;
    aModel->GetInstanceDocument(NS_LITERAL_STRING(""),
                                getter_AddRefs(instanceDoc));
    if (!instanceDoc)
      return nsnull;

    nsIDOMElement *docElement = nsnull;
    instanceDoc->GetDocumentElement(&docElement);  // addrefs
    return docElement;
  }

  // The context node is the first node in the nodeset of the parent bind.
  nsCOMPtr<nsIDOMXPathResult> parentNodeSet =
    EvaluateNodeset(nsCOMPtr<nsIDOMElement>(do_QueryInterface(parentNode)),
                    nsIDOMXPathResult::FIRST_ORDERED_NODE_TYPE);

  nsIDOMNode *nodeResult = nsnull;
  if (parentNodeSet)
    parentNodeSet->GetSingleNodeValue(&nodeResult);  // addrefs

  return nodeResult;
}

/* static */ already_AddRefed<nsIDOMXPathResult>
nsXFormsUtils::EvaluateNodeset(nsIDOMElement *aElement, PRUint16 aResultType)
{
  // The first thing we need to do is ensure that we have a nodeset attribute.
  // If we don't, we walk up and check for it on outer bind elements.
  // While we're walking up, we also locate the model element.

  nsCOMPtr<nsIXFormsModelElement> model;
  nsCOMPtr<nsIDOMElement> bindElement, element = aElement;
  nsCOMPtr<nsIDOMNode> temp;
  nsAutoString nodeset;

  while (element) {
    model = do_QueryInterface(element);
    if (model)
      break;

    if (!bindElement) {
      element->GetAttribute(NS_LITERAL_STRING("nodeset"), nodeset);
      if (!nodeset.IsEmpty()) {
        // Verify that this is a <bind> element;
        nsAutoString value;
        element->GetNamespaceURI(value);
        if (value.EqualsLiteral(NS_NAMESPACE_XFORMS)) {
          element->GetLocalName(value);
          if (value.EqualsLiteral("bind")) {
            bindElement = element;
          }
        }
      }
    }

    element->GetParentNode(getter_AddRefs(temp));
    element = do_QueryInterface(temp);
  }

  if (!bindElement || !model)
    return nsnull;

  nsCOMPtr<nsIDOMNode> contextNode = FindBindContext(bindElement, model);
  if (!contextNode)
    return nsnull;

  return EvaluateXPath(nodeset, contextNode, bindElement, aResultType);
}

/* static */ already_AddRefed<nsIDOMXPathResult>
nsXFormsUtils::EvaluateNodeBinding(nsIDOMElement  *aElement,
                                   PRUint32        aElementFlags,
                                   const nsString &aDefaultRef,
                                   PRUint16        aResultType,
                                   nsIDOMNode    **aModel,
                                   nsIDOMElement **aBind)
{
  // A control may be attached to a model by either using the 'bind'
  // attribute to give the id of a bind element, or using the 'model'
  // attribute to give the id of a model.  If neither of these are given,
  // the control belongs to the first model in the document.

  *aBind = nsnull;

  NS_IF_ADDREF(*aModel = GetModelAndBind(aElement, aElementFlags, aBind));
  if (!*aModel)
    return nsnull;

  // If there is a bind element, we just evaluate its nodeset.
  if (*aBind)
    return EvaluateNodeset(*aBind, aResultType);

  // If not, we expect there to be a ref attribute.
  nsAutoString expr;
  aElement->GetAttribute(NS_LITERAL_STRING("ref"), expr);

  if (expr.IsEmpty())
  {
    if (aDefaultRef.IsEmpty())
      return nsnull;
    expr.Assign(aDefaultRef);
  } 

  // Get the instance data and evaluate the xpath expression.
  // XXXfixme when xpath extensions are implemented (instance())
  nsCOMPtr<nsIDOMDocument> instanceDoc;
  nsCOMPtr<nsIXFormsModelElement> model = do_QueryInterface(*aModel);
  if (!model) {
    // The referenced model is not actually a model element, or does not exist.
    return nsnull;
  }

  model->GetInstanceDocument(NS_LITERAL_STRING(""),
                             getter_AddRefs(instanceDoc));

  if (!instanceDoc)
    return nsnull;

  nsCOMPtr<nsIDOMElement> docElement;
  instanceDoc->GetDocumentElement(getter_AddRefs(docElement));
  if (!docElement)
    return nsnull;   // this will happen if the doc is still loading

  return EvaluateXPath(expr, docElement, aElement, aResultType);
}

/* static */ void
nsXFormsUtils::GetNodeValue(nsIDOMNode* aDataNode, nsString& aNodeValue)
{
  PRUint16 nodeType;
  aDataNode->GetNodeType(&nodeType);

  switch(nodeType) {
  case nsIDOMNode::ATTRIBUTE_NODE:
  case nsIDOMNode::TEXT_NODE:
    // "Returns the string-value of the node."
    aDataNode->GetNodeValue(aNodeValue);
    return;

  case nsIDOMNode::ELEMENT_NODE:
    {
      // "If text child nodes are present, returns the string-value of the
      // first text child node.  Otherwise, returns "" (the empty string)".

      // Find the first child text node.
      nsCOMPtr<nsIDOMNodeList> childNodes;
      aDataNode->GetChildNodes(getter_AddRefs(childNodes));

      if (childNodes) {
        nsCOMPtr<nsIDOMNode> child;
        PRUint32 childCount;
        childNodes->GetLength(&childCount);

        for (PRUint32 i = 0; i < childCount; ++i) {
          childNodes->Item(i, getter_AddRefs(child));
          NS_ASSERTION(child, "DOMNodeList length is wrong!");

          child->GetNodeType(&nodeType);
          if (nodeType == nsIDOMNode::TEXT_NODE) {
            child->GetNodeValue(aNodeValue);
            return;
          }
        }
      }

      // No child text nodes.  Return an empty string.
    }
    break;
          
  default:
    // namespace, processing instruction, comment, XPath root node
    NS_WARNING("String value for this node type is not defined");
  }

  aNodeValue.Truncate(0);
}

/* static */ void
nsXFormsUtils::SetNodeValue(nsIDOMNode* aDataNode, const nsString& aNodeValue)
{
  PRUint16 nodeType;
  aDataNode->GetNodeType(&nodeType);

  switch(nodeType) {
  case nsIDOMNode::ATTRIBUTE_NODE:
    // "The string-value of the attribute is replaced with a string
    // corresponding to the new value."
    aDataNode->SetNodeValue(aNodeValue);
    break;

  case nsIDOMNode::TEXT_NODE:
    // "The text node is replaced with a new one corresponding to the new
    // value".
    {
      nsCOMPtr<nsIDOMDocument> document;
      aDataNode->GetOwnerDocument(getter_AddRefs(document));
      if (!document)
        break;

      nsCOMPtr<nsIDOMText> textNode;
      document->CreateTextNode(aNodeValue, getter_AddRefs(textNode));
      if (!textNode)
        break;

      nsCOMPtr<nsIDOMNode> parentNode;
      aDataNode->GetParentNode(getter_AddRefs(parentNode));
      if (parentNode) {
        nsCOMPtr<nsIDOMNode> childReturn;
        parentNode->ReplaceChild(textNode, aDataNode,
                                 getter_AddRefs(childReturn));
      }

      break;
    }

  case nsIDOMNode::ELEMENT_NODE:
    {
      // "If the element has any child text nodes, the first text node is
      // replaced with one corresponding to the new value."

      // Start by creating a text node for the new value.
      nsCOMPtr<nsIDOMDocument> document;
      aDataNode->GetOwnerDocument(getter_AddRefs(document));
      if (!document)
        break;

      nsCOMPtr<nsIDOMText> textNode;
      document->CreateTextNode(aNodeValue, getter_AddRefs(textNode));
      if (!textNode)
        break;

      // Now find the first child text node.
      nsCOMPtr<nsIDOMNodeList> childNodes;
      aDataNode->GetChildNodes(getter_AddRefs(childNodes));

      if (!childNodes)
        break;

      nsCOMPtr<nsIDOMNode> child, childReturn;
      PRUint32 childCount;
      childNodes->GetLength(&childCount);

      for (PRUint32 i = 0; i < childCount; ++i) {
        childNodes->Item(i, getter_AddRefs(child));
        NS_ASSERTION(child, "DOMNodeList length is wrong!");

        child->GetNodeType(&nodeType);
        if (nodeType == nsIDOMNode::TEXT_NODE) {
          // We found one, replace it with our new text node.
          aDataNode->ReplaceChild(textNode, child,
                                  getter_AddRefs(childReturn));
          return;
        }
      }

      // "If no child text nodes are present, a text node is created,
      // corresponding to the new value, and appended as the first child node."

      // XXX This is a bit vague since "appended as the first child node"
      // implies that there are no child nodes at all, but all we've
      // established is that there are no child _text_nodes.
      // Taking this to mean "inserted as the first child node" until this is
      // clarified.

      aDataNode->GetFirstChild(getter_AddRefs(child));
      if (child)
        aDataNode->InsertBefore(textNode, child, getter_AddRefs(childReturn));
      else
        aDataNode->AppendChild(textNode, getter_AddRefs(childReturn));

    }
    break;
          
  default:
    NS_WARNING("Trying to set node value for unsupported node type");
  }
}


/* static */ nsresult
nsXFormsUtils::CloneScriptingInterfaces(const nsIID *aIIDList,
                                        unsigned int aIIDCount,
                                        PRUint32 *aOutCount,
                                        nsIID ***aOutArray)
{
  nsIID **iids = NS_STATIC_CAST(nsIID**,
                                nsMemory::Alloc(aIIDCount * sizeof(nsIID*)));
  if (!iids) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
 
  for (PRUint32 i = 0; i < aIIDCount; ++i) {
    iids[i] = NS_STATIC_CAST(nsIID*,
                             nsMemory::Clone(&aIIDList[i], sizeof(nsIID)));
 
    if (!iids[i]) {
      for (PRUint32 j = 0; j < i; ++j)
        nsMemory::Free(iids[j]);
      nsMemory::Free(iids);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
 
  *aOutArray = iids;
  *aOutCount = aIIDCount;
  return NS_OK;
}
