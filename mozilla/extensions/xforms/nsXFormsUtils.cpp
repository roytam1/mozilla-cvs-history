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
#include "nsIModelElementPrivate.h"
#include "nsIXFormsModelElement.h"
#include "nsIXFormsControl.h"

#include "nsIXFormsContextControl.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsDataHashtable.h"

#include "nsAutoPtr.h"
#include "nsXFormsXPathAnalyzer.h"
#include "nsXFormsXPathParser.h"
#include "nsXFormsXPathNode.h"
#include "nsXFormsMDGSet.h"
#include "nsIDOMXPathExpression.h"
#include "nsArray.h"

#include "nsIScriptSecurityManager.h"
#include "nsIPermissionManager.h"
#include "nsServiceManagerUtils.h"

#define CANCELABLE 0x01
#define BUBBLES    0x02

const EventData sXFormsEventsEntries[41] = {
  { "xforms-model-construct",      PR_FALSE, PR_TRUE  },
  { "xforms-model-construct-done", PR_FALSE, PR_TRUE  },
  { "xforms-ready",                PR_FALSE, PR_TRUE  },
  { "xforms-model-destruct",       PR_FALSE, PR_TRUE  },
  { "xforms-previous",             PR_TRUE,  PR_FALSE },
  { "xforms-next",                 PR_TRUE,  PR_FALSE },
  { "xforms-focus",                PR_TRUE,  PR_FALSE },
  { "xforms-help",                 PR_TRUE,  PR_TRUE  },
  { "xforms-hint",                 PR_TRUE,  PR_TRUE  },
  { "xforms-rebuild",              PR_TRUE,  PR_TRUE  },
  { "xforms-refresh",              PR_TRUE,  PR_TRUE  },
  { "xforms-revalidate",           PR_TRUE,  PR_TRUE  },
  { "xforms-recalculate",          PR_TRUE,  PR_TRUE  },
  { "xforms-reset",                PR_TRUE,  PR_TRUE  },
  { "xforms-submit",               PR_TRUE,  PR_TRUE  },
  { "DOMActivate",                 PR_TRUE,  PR_TRUE  },
  { "xforms-value-changed",        PR_FALSE, PR_TRUE  },
  { "xforms-select",               PR_FALSE, PR_TRUE  },
  { "xforms-deselect",             PR_FALSE, PR_TRUE  },
  { "xforms-scroll-first",         PR_FALSE, PR_TRUE  },
  { "xforms-scroll-last",          PR_FALSE, PR_TRUE  },
  { "xforms-insert",               PR_FALSE, PR_TRUE  },
  { "xforms-delete",               PR_FALSE, PR_TRUE  },
  { "xforms-valid",                PR_FALSE, PR_TRUE  },
  { "xforms-invalid",              PR_FALSE, PR_TRUE  },
  { "DOMFocusIn",                  PR_FALSE, PR_TRUE  },
  { "DOMFocusOut",                 PR_FALSE, PR_TRUE  },
  { "xforms-readonly",             PR_FALSE, PR_TRUE  },
  { "xforms-readwrite",            PR_FALSE, PR_TRUE  },
  { "xforms-required",             PR_FALSE, PR_TRUE  },
  { "xforms-optional",             PR_FALSE, PR_TRUE  },
  { "xforms-enabled",              PR_FALSE, PR_TRUE  },
  { "xforms-disabled",             PR_FALSE, PR_TRUE  },
  { "xforms-in-range",             PR_FALSE, PR_TRUE  },
  { "xforms-out-of-range",         PR_FALSE, PR_TRUE  },
  { "xforms-submit-done",          PR_FALSE, PR_TRUE  },
  { "xforms-submit-error",         PR_FALSE, PR_TRUE  },
  { "xforms-binding-exception",    PR_FALSE, PR_TRUE  },
  { "xforms-link-exception",       PR_FALSE, PR_TRUE  },
  { "xforms-link-error",           PR_FALSE, PR_TRUE  },
  { "xforms-compute-exception",    PR_FALSE, PR_TRUE  }
};

static const EventData sEventDefaultsEntries[] = {
  //UIEvents already in sXFormsEvents
  
  //MouseEvent
  { "click",                       PR_TRUE,  PR_TRUE  },
  { "mousedown",                   PR_TRUE,  PR_TRUE  },
  { "mouseup",                     PR_TRUE,  PR_TRUE  },
  { "mouseover",                   PR_TRUE,  PR_TRUE  },
  { "mousemove",                   PR_FALSE, PR_TRUE  },
  { "mouseout",                    PR_TRUE,  PR_TRUE  },
  //MutationEvent
  { "DOMSubtreeModified",          PR_FALSE, PR_TRUE  },
  { "DOMNodeInserted",             PR_FALSE, PR_TRUE  },
  { "DOMNodeRemoved",              PR_FALSE, PR_TRUE  },
  { "DOMNodeRemovedFromDocument",  PR_FALSE, PR_FALSE },
  { "DOMNodeInsertedIntoDocument", PR_FALSE, PR_FALSE },
  { "DOMAttrModified",             PR_FALSE, PR_TRUE  },
  { "DOMCharacterDataModified",    PR_FALSE, PR_TRUE  },
  //HTMLEvents
  { "load",                        PR_FALSE, PR_FALSE },
  { "unload",                      PR_FALSE, PR_FALSE },
  { "abort",                       PR_FALSE, PR_TRUE  },
  { "error",                       PR_FALSE, PR_TRUE  },
  { "select",                      PR_FALSE, PR_TRUE  },
  { "change",                      PR_FALSE, PR_TRUE  },
  { "submit",                      PR_TRUE,  PR_TRUE  },
  { "reset",                       PR_FALSE, PR_TRUE  },
  { "focus",                       PR_FALSE, PR_FALSE },
  { "blur",                        PR_FALSE, PR_FALSE },
  { "resize",                      PR_FALSE, PR_TRUE  },
  { "scroll",                      PR_FALSE, PR_TRUE  }
};

static nsDataHashtable<nsStringHashKey,PRUint32> sXFormsEvents;
static nsDataHashtable<nsStringHashKey,PRUint32> sEventDefaults;

/* static */ nsresult
nsXFormsUtils::Init()
{
  if (!sXFormsEvents.Init())
    return NS_ERROR_FAILURE;

  unsigned int i;

  for (i = 0; i < NS_ARRAY_LENGTH(sXFormsEventsEntries); ++i) {
    PRUint32 flag = 0;
    if (sXFormsEventsEntries[i].canCancel)
      flag |= CANCELABLE;
    if (sXFormsEventsEntries[i].canBubble)
      flag |= BUBBLES;
    sXFormsEvents.Put(NS_ConvertUTF8toUTF16(sXFormsEventsEntries[i].name),
                                            flag);
  }

  if (!sEventDefaults.Init())
    return NS_ERROR_FAILURE;
  for (i = 0; i < NS_ARRAY_LENGTH(sEventDefaultsEntries); ++i) {
    PRUint32 flag = 0;
    if (sEventDefaultsEntries[i].canCancel)
      flag |= CANCELABLE;
    if (sEventDefaultsEntries[i].canBubble)
      flag |= BUBBLES;
    sEventDefaults.Put(NS_ConvertUTF8toUTF16(sEventDefaultsEntries[i].name),
                                             flag);
  }
  return NS_OK;
}

/* static */ PRBool
nsXFormsUtils::GetParentModel(nsIDOMElement           *aBindElement,
                              nsIModelElementPrivate **aModel)
{
  PRBool res = PR_TRUE;
  nsCOMPtr<nsIDOMNode> modelWrapper;

  // Walk up the tree looking for the containing model.
  aBindElement->GetParentNode(getter_AddRefs(modelWrapper));

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

    // Model is not the immediate parent, this is a reference to a nested
    // (invalid) bind
    res = PR_FALSE;
  }
  *aModel = nsnull;
  nsCOMPtr<nsIModelElementPrivate> model = do_QueryInterface(modelWrapper);
  model.swap(*aModel);

  return res;
}

/**
 * beaufour: Section 7.4 in the specification does a really bad job of
 * explaining how to find the model, so the code below is my interpretation of
 * it...
 *
 * @see http://bugzilla.mozilla.org/show_bug.cgi?id=265216
 */

/* static */ nsresult
nsXFormsUtils::GetNodeContext(nsIDOMElement           *aElement,
                              PRUint32                 aElementFlags,
                              nsIModelElementPrivate **aModel,
                              nsIDOMElement          **aBindElement,
                              PRBool                  *aOuterBind,
                              nsIDOMNode             **aContextNode,
                              PRInt32                 *aContextPosition,
                              PRInt32                  *aContextSize)
{
  NS_ENSURE_ARG(aElement);
  NS_ENSURE_ARG(aOuterBind);
  NS_ENSURE_ARG_POINTER(aContextNode);
  NS_ENSURE_ARG_POINTER(aBindElement);
  *aBindElement = nsnull;

  // Find correct model element
  nsCOMPtr<nsIDOMDocument> domDoc;
  aElement->GetOwnerDocument(getter_AddRefs(domDoc));
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

  nsAutoString bindId;
  aElement->GetAttribute(NS_LITERAL_STRING("bind"), bindId);
  if (!bindId.IsEmpty()) {
    // CASE 1: Use @bind
    domDoc->GetElementById(bindId, aBindElement);
    if (!IsXFormsElement(*aBindElement, NS_LITERAL_STRING("bind"))) {
      DispatchEvent(aElement, eEvent_BindingException);
      return NS_ERROR_ABORT;
    }

    // Context size and position are always 1
    if (aContextSize)
      *aContextSize = 1;
    if (aContextPosition)
      *aContextPosition = 1;
    
    return FindBindContext(*aBindElement,
                           aOuterBind,
                           aModel,
                           aContextNode);
  }

  if (aElementFlags & ELEMENT_WITH_MODEL_ATTR) {
    // CASE 2: Use @model
    // If bind did not set model, and the element has a model attribute we use this
    nsAutoString modelId;
    aElement->GetAttribute(NS_LITERAL_STRING("model"), modelId);
    
    if (!modelId.IsEmpty()) {
      nsCOMPtr<nsIDOMElement> modelElement;
      domDoc->GetElementById(modelId, getter_AddRefs(modelElement));
      nsCOMPtr<nsIModelElementPrivate> model = do_QueryInterface(modelElement);
      
      // No element found, or element not a \<model\> element
      if (!model) {
        nsXFormsUtils::DispatchEvent(aElement, eEvent_BindingException);        
        return NS_ERROR_FAILURE;
      }

      NS_ADDREF(*aModel = model);
    }
  }

  // Search for a parent setting context for us
  nsresult rv = FindParentContext(aElement,
                                  aModel,
                                  aContextNode,
                                  aContextPosition,
                                  aContextSize);
  // CASE 3/4: Use parent's model / first model in document.
  // If FindParentContext() does not find a parent context but |aModel| is not
  // set, it sets the model to the first model in the document.
  
  NS_ENSURE_SUCCESS(rv, rv);    

  return NS_OK;
}

/* static */ already_AddRefed<nsIModelElementPrivate>
nsXFormsUtils::GetModel(nsIDOMElement  *aElement,
                        PRUint32        aElementFlags)

{

  nsCOMPtr<nsIModelElementPrivate> model;
  nsCOMPtr<nsIDOMNode> contextNode;
  nsCOMPtr<nsIDOMElement> bind;
  PRBool outerbind;

  GetNodeContext(aElement,
                 aElementFlags,
                 getter_AddRefs(model),
                 getter_AddRefs(bind),
                 &outerbind,
                 getter_AddRefs(contextNode));

  NS_ENSURE_TRUE(model, nsnull);

  nsIModelElementPrivate *result = nsnull;
  if (model)
    CallQueryInterface(model, &result);  // addrefs
  return result;
}

/* static */ already_AddRefed<nsIDOMXPathResult>
nsXFormsUtils::EvaluateXPath(const nsAString &aExpression,
                             nsIDOMNode      *aContextNode,
                             nsIDOMNode      *aResolverNode,
                             PRUint16         aResultType,
                             PRInt32          aContextPosition,
                             PRInt32          aContextSize,
                             nsXFormsMDGSet  *aSet)
{
  nsCOMPtr<nsIDOMDocument> doc;
  aContextNode->GetOwnerDocument(getter_AddRefs(doc));
  NS_ENSURE_TRUE(doc, nsnull);

  nsCOMPtr<nsIDOMXPathEvaluator> eval = do_QueryInterface(doc);
  NS_ENSURE_TRUE(eval, nsnull);

  nsCOMPtr<nsIDOMXPathNSResolver> resolver;
  eval->CreateNSResolver(aResolverNode, getter_AddRefs(resolver));
  NS_ENSURE_TRUE(resolver, nsnull);

  nsCOMPtr<nsIDOMXPathExpression> expression;
  eval->CreateExpression(aExpression,
                         resolver,
                         getter_AddRefs(expression));
  NS_ENSURE_TRUE(expression, nsnull);
   
  ///
  /// @todo Evaluate() should use aContextPosition and aContextSize
  nsCOMPtr<nsISupports> supResult;
  expression->Evaluate(aContextNode,
                       aResultType,
                       nsnull,
                       getter_AddRefs(supResult));

  nsIDOMXPathResult *result = nsnull;
  if (supResult) {
    /// @todo beaufour: This is somewhat "hackish". Hopefully, this will
    /// improve when we integrate properly with Transformiix (XXX)
    /// @see http://bugzilla.mozilla.org/show_bug.cgi?id=265212
    if (aSet) {
      nsXFormsXPathParser parser;
      nsXFormsXPathAnalyzer analyzer(eval, resolver);
      nsAutoPtr<nsXFormsXPathNode> xNode(parser.Parse(aExpression));

      nsresult rv = analyzer.Analyze(aContextNode,
                                     xNode,
                                     expression,
                                     &aExpression,
                                     aSet);
      NS_ENSURE_SUCCESS(rv, nsnull);
    }
    CallQueryInterface(supResult, &result);  // addrefs
  }

  return result;
}

/* static */ nsresult
nsXFormsUtils::FindBindContext(nsIDOMElement           *aBindElement,
                               PRBool                  *aOuterBind,
                               nsIModelElementPrivate **aModel,
                               nsIDOMNode             **aContextNode)
{
  NS_ENSURE_ARG_POINTER(aContextNode);
  *aContextNode = nsnull;
  
  // 1) Find the model for the bind
  *aOuterBind = GetParentModel(aBindElement, aModel);
  NS_ENSURE_STATE(*aModel);

  // 2) Find the context node
  nsCOMPtr<nsIXFormsModelElement> modelInt = do_QueryInterface(*aModel);
  NS_ENSURE_STATE(modelInt);

  nsCOMPtr<nsIDOMDocument> instanceDoc;
  modelInt->GetInstanceDocument(NS_LITERAL_STRING(""),
                                getter_AddRefs(instanceDoc));
  NS_ENSURE_STATE(instanceDoc);

  nsIDOMElement* docElement;
  instanceDoc->GetDocumentElement(&docElement); // addrefs
  NS_ENSURE_STATE(docElement);
  *aContextNode = docElement; // addref'ed above

  return NS_OK;
}

/* static */ nsresult
nsXFormsUtils::EvaluateNodeBinding(nsIDOMElement           *aElement,
                                   PRUint32                 aElementFlags,
                                   const nsString          &aBindingAttr,
                                   const nsString          &aDefaultRef,
                                   PRUint16                 aResultType,
                                   nsIModelElementPrivate **aModel,
                                   nsIDOMXPathResult      **aResult,
                                   nsIMutableArray         *aDeps)
{
  if (!aElement || !aModel || !aResult) {
    return NS_OK;
  }

  *aModel = nsnull;
  *aResult = nsnull;

  nsCOMPtr<nsIDOMNode>    contextNode;
  nsCOMPtr<nsIDOMElement> bindElement;
  PRBool outerBind;
  PRInt32 contextPosition;
  PRInt32 contextSize;
  nsresult rv = GetNodeContext(aElement,
                               aElementFlags,
                               aModel,
                               getter_AddRefs(bindElement),
                               &outerBind,
                               getter_AddRefs(contextNode),
                               &contextPosition,
                               &contextSize);

  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString expr;
  if (bindElement) {
    if (!outerBind) {
      // "When you refer to @id on a nested bind it returns an emtpy nodeset
      // because it has no meaning. The XForms WG will assign meaning in the
      // future."
      // @see http://www.w3.org/MarkUp/Group/2004/11/f2f/2004Nov11#resolution6

      return NS_OK;
    } else {
      // If there is a (outer) bind element, we retrive its nodeset.
      bindElement->GetAttribute(NS_LITERAL_STRING("nodeset"), expr);
    }
  } else {
    // If there's no bind element, we expect there to be a |aBindingAttr| attribute.
    aElement->GetAttribute(aBindingAttr, expr);

    if (expr.IsEmpty())
    {
      if (aDefaultRef.IsEmpty())
        return NS_OK;

      expr.Assign(aDefaultRef);
    } 

    if (!contextNode) {
      nsCOMPtr<nsIDOMDocument> instanceDoc;
      nsCOMPtr<nsIXFormsModelElement> model = do_QueryInterface(*aModel);
      
      NS_ENSURE_STATE(model); // The referenced model is not actually a model element, or does not exist.
    
      model->GetInstanceDocument(NS_LITERAL_STRING(""),
                                 getter_AddRefs(instanceDoc));
    
      NS_ENSURE_STATE(instanceDoc);
    
      nsCOMPtr<nsIDOMElement> docElement;
      instanceDoc->GetDocumentElement(getter_AddRefs(docElement));
      contextNode = docElement;
    
      if (!contextNode) {
        return NS_OK;   // this will happen if the doc is still loading
      }
    }
  }

  // Evaluate |expr|
  nsXFormsMDGSet set;
  nsCOMPtr<nsIDOMXPathResult> res = EvaluateXPath(expr,
                                                  contextNode,
                                                  aElement,
                                                  aResultType,
                                                  contextSize,
                                                  contextPosition,
                                                  aDeps ? &set : nsnull);

  if (res && aDeps) {
    for (PRInt32 i = 0; i < set.Count(); ++i) {
      aDeps->AppendElement(set.GetNode(i), PR_FALSE);
    }
  }

  res.swap(*aResult); // exchanges ref

  return NS_OK;
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

///
/// @todo Use this consistently, or delete? (XXX)
/* static */ PRBool
nsXFormsUtils::GetSingleNodeBindingValue(nsIDOMElement* aElement,
                                         nsString& aValue)
{
  if (!aElement)
    return PR_FALSE;
  nsCOMPtr<nsIModelElementPrivate> model;
  nsCOMPtr<nsIDOMXPathResult> result;
  
  nsresult rv = EvaluateNodeBinding(aElement,
                                    nsXFormsUtils::ELEMENT_WITH_MODEL_ATTR,
                                    NS_LITERAL_STRING("ref"),
                                    EmptyString(),
                                    nsIDOMXPathResult::FIRST_ORDERED_NODE_TYPE,
                                    getter_AddRefs(model),
                                    getter_AddRefs(result));

  if (NS_FAILED(rv) || !result)
    return PR_FALSE;

  nsCOMPtr<nsIDOMNode> singleNode;
  result->GetSingleNodeValue(getter_AddRefs(singleNode));
  if (!singleNode)
    return PR_FALSE;

  nsXFormsUtils::GetNodeValue(singleNode, aValue);
  return PR_TRUE;
}

/* static */ nsresult
nsXFormsUtils::DispatchEvent(nsIDOMNode* aTarget, nsXFormsEvent aEvent)
{
  if (!aTarget)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMDocument> domDoc;
  aTarget->GetOwnerDocument(getter_AddRefs(domDoc));

  nsCOMPtr<nsIDOMDocumentEvent> doc = do_QueryInterface(domDoc);

  nsCOMPtr<nsIDOMEvent> event;
  doc->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  const EventData *data = &sXFormsEventsEntries[aEvent];
  event->InitEvent(NS_ConvertUTF8toUTF16(data->name),
                   data->canBubble, data->canCancel);

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(aTarget);
  PRBool cancelled;
  return target->DispatchEvent(event, &cancelled);
}

/* static */ PRBool
nsXFormsUtils::IsXFormsEvent(const nsAString& aEvent,
                             PRBool& aCancelable,
                             PRBool& aBubbles)
{
  PRUint32 flag = 0;
  if (!sXFormsEvents.Get(aEvent, &flag))
    return PR_FALSE;
  aCancelable = (flag & CANCELABLE) ? PR_TRUE : PR_FALSE;
  aBubbles = (flag & BUBBLES) ? PR_TRUE : PR_FALSE;
  return PR_TRUE;
}

/* static */ void
nsXFormsUtils::GetEventDefaults(const nsAString& aEvent,
                                PRBool& aCancelable,
                                PRBool& aBubbles)
{
  PRUint32 flag = 0;
  if (!sEventDefaults.Get(aEvent, &flag))
    return;
  aCancelable = (flag & CANCELABLE) ? PR_TRUE : PR_FALSE;
  aBubbles = (flag & BUBBLES) ? PR_TRUE : PR_FALSE;
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

/* static */ nsresult
nsXFormsUtils::FindParentContext(nsIDOMElement           *aElement,
                                 nsIModelElementPrivate **aModel,
                                 nsIDOMNode             **aContextNode,
                                 PRInt32                 *aContextPosition,
                                 PRInt32                 *aContextSize)
{
  NS_ENSURE_ARG(aElement);
  NS_ENSURE_ARG_POINTER(aModel);
  NS_ENSURE_ARG_POINTER(aContextNode);

  nsCOMPtr<nsIDOMNode> elementNode = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(elementNode, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNode> curNode;
  nsresult rv = elementNode->GetParentNode(getter_AddRefs(curNode));
  NS_ENSURE_SUCCESS(rv, NS_OK);

  // If a model is set, get its ID
  nsAutoString childModelID;
  if (*aModel) {
    nsCOMPtr<nsIDOMElement> modelElement = do_QueryInterface(*aModel);
    NS_ENSURE_TRUE(modelElement, NS_ERROR_FAILURE);
    modelElement->GetAttribute(NS_LITERAL_STRING("id"), childModelID);
  }

  // Find our context:
  // Iterate over all parents and find first one that implements nsIXFormsContextControl,
  // and has the same model as us.
  nsCOMPtr<nsIDOMNode> temp;  
  nsAutoString contextModelID;
  while (curNode) {
    nsCOMPtr<nsIXFormsContextControl> contextControl = do_QueryInterface(curNode);
    nsCOMPtr<nsIDOMElement> cElement = do_QueryInterface(curNode);
    if (contextControl && cElement) {
      PRInt32 cSize;
      PRInt32 cPosition;
      nsCOMPtr<nsIDOMNode> tempNode;
      rv = contextControl->GetContext(contextModelID, getter_AddRefs(tempNode), &cPosition, &cSize);
      NS_ENSURE_SUCCESS(rv, rv);
      // If the call failed, it means that we _have_ a parent which sets the
      // context but it is invalid, ie. the XPath expression could have
      // generated an error.

      if (tempNode && (childModelID.IsEmpty()
                       || childModelID.Equals(contextModelID))) {
        NS_ADDREF(*aContextNode = tempNode);
        if (aContextSize) 
          *aContextSize = cSize;
        if (aContextPosition)
          *aContextPosition = cPosition;
        break;
      }
    }
    // Next ancestor
    temp.swap(curNode);
    rv = temp->GetParentNode(getter_AddRefs(curNode));
    NS_ENSURE_SUCCESS(rv, NS_OK);
  }

  // Child had no model set, set it
  if (!*aModel) {
    nsCOMPtr<nsIDOMDocument> domDoc;
    nsresult rv = aElement->GetOwnerDocument(getter_AddRefs(domDoc));
    NS_ENSURE_SUCCESS(rv, rv);
    
    NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

    nsCOMPtr<nsIModelElementPrivate> model;
    if (!*aContextNode || contextModelID.IsEmpty()) {
      // We have either not found a context node, or we have found one where
      // the model ID is empty. That means we use the first model in document
      nsCOMPtr<nsIDOMNodeList> nodes;
      domDoc->GetElementsByTagNameNS(NS_LITERAL_STRING(NS_NAMESPACE_XFORMS),
                                     NS_LITERAL_STRING("model"),
                                     getter_AddRefs(nodes));
      // No model element in document!
      NS_ENSURE_STATE(nodes);

      nsCOMPtr<nsIDOMNode> modelNode;
      nodes->Item(0, getter_AddRefs(modelNode));
      model = do_QueryInterface(modelNode);
    } else {
      // Get the model with the correct ID
      nsCOMPtr<nsIDOMElement> modelElement;
      domDoc->GetElementById(contextModelID, getter_AddRefs(modelElement));
      model = do_QueryInterface(modelElement);
    }    
    if (!model) {
      DispatchEvent(aElement, eEvent_BindingException);
      return NS_ERROR_ABORT;
    }
    NS_ADDREF(*aModel = model);
  }
  
  return NS_OK;
}

/* static */ PRBool
nsXFormsUtils::CheckSameOrigin(nsIURI *aBaseURI, nsIURI *aTestURI)
{
  nsresult rv;

  // check to see if we're allowed to load this URI
  nsCOMPtr<nsIScriptSecurityManager> secMan =
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
  if (secMan) {
    rv = secMan->CheckSameOriginURI(aBaseURI, aTestURI);
    if (NS_SUCCEEDED(rv))
      return PR_TRUE;
  }

  // else, check with the permission manager to see if this host is
  // permitted to access sites from other domains.

  nsCOMPtr<nsIPermissionManager> permMgr =
      do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  PRUint32 perm;
  rv = permMgr->TestPermission(aBaseURI, "xforms-load", &perm);
  if (NS_SUCCEEDED(rv) && perm == nsIPermissionManager::ALLOW_ACTION)
    return PR_TRUE; 

  return PR_FALSE;
}

/*static*/ PRBool
nsXFormsUtils::IsXFormsElement(nsIDOMNode* aNode, const nsAString& aName)
{
  if (aNode) {
    PRUint16 nodeType;
    aNode->GetNodeType(&nodeType);
    if (nodeType == nsIDOMNode::ELEMENT_NODE) {
      nsAutoString name;
      aNode->GetLocalName(name);
      if (name.Equals(aName)) {
        nsAutoString ns;
        aNode->GetNamespaceURI(ns);
        if (ns.EqualsLiteral(NS_NAMESPACE_XFORMS))
          return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

/* static */ PRBool
nsXFormsUtils::IsLabelElement(nsIDOMNode *aElement)
{
  nsAutoString value;
  aElement->GetLocalName(value);
  if (value.EqualsLiteral("label")) {
    aElement->GetNamespaceURI(value);
    return value.EqualsLiteral(NS_NAMESPACE_XFORMS);
  }

  return PR_FALSE;
}
