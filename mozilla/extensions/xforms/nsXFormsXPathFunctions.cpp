/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 *  Aaron Reed <aaronr@us.ibm.com>
 *  Merle Sterling <msterlin@us.ibm.com>
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

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "nsXFormsXPathFunctions.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsStringAPI.h"
#include "nsXFormsUtils.h"
#include "prprf.h"
#include "txDouble.h"
#include "txIFunctionEvaluationContext.h"
#include "txINodeSet.h"
#include "nsIClassInfoImpl.h"
#include "nsIXFormsActionModuleElement.h"
#include "nsIXFormsContextInfo.h"
#include "prrng.h"
#include "nsIXFormsControl.h"
#include "nsIInstanceElementPrivate.h"
#include "nsISchemaValidator.h"
#include "cert.h"
#include "prmem.h"
#include "plbase64.h"
#include "nsICryptoHash.h"
#include "nsICryptoHMAC.h"
#include "nsIKeyModule.h"
#include "nsServiceManagerUtils.h"

#define NS_NAMESPACE_XFORMS "http://www.w3.org/2002/xforms"

static const txdpun nanMask = TX_DOUBLE_NaN;
#define kNaN (nanMask.d)

NS_IMPL_ISUPPORTS1_CI(nsXFormsXPathFunctions, nsIXFormsXPathFunctions)

NS_IMETHODIMP
nsXFormsXPathFunctions::Avg(txINodeSet *aNodeSet, double *aResult)
{
    PRUint32 length;
    nsresult rv = aNodeSet->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    double total = 0;
    PRUint32 i;
    for (i = 0; i < length; ++i) {
        double item;
        rv = aNodeSet->ItemAsNumber(i, &item);
        NS_ENSURE_SUCCESS(rv, rv);

        if (TX_DOUBLE_IS_NaN(item)) {
            // This will make aResult be set to kNaN below.
            i = 0;
            break;
        }

        total += item;
    }

    *aResult = (i > 0) ? (total / i) : kNaN;

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::BooleanFromString(const nsAString & aString,
                                          PRBool *aResult)
{
    *aResult = aString.EqualsLiteral("1") ||
               aString.LowerCaseEqualsLiteral("true");

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::CountNonEmpty(txINodeSet *aNodeSet, double *aResult)
{
    PRUint32 length;
    nsresult rv = aNodeSet->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    double result = 0;
    PRUint32 i;
    for (i = 0; i < length; ++i) {
        nsAutoString item;
        rv = aNodeSet->ItemAsString(i, item);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!item.IsEmpty()) {
            ++result;
        }
    }

    *aResult = result;

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::DaysFromDate(const nsAString &aDateTime,
                                     double *aResult)
{
    PRInt32 result = 0;
    nsresult rv = nsXFormsUtils::GetDaysFromDateTime(aDateTime, &result);
    if (rv == NS_ERROR_ILLEGAL_VALUE) {
        *aResult = kNaN;
        rv = NS_OK;
    }
    else {
        *aResult = result;
    }

    return rv;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::If(PRBool aValue, const nsAString &aIfString,
                           const nsAString &aElseString, nsAString &aResult)
{
    // XXX Avoid evaluating aIfString and aElseString until after checking
    //     aValue. Probably needs vararg support.
    aResult = aValue ? aIfString : aElseString;

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Index(txIFunctionEvaluationContext *aContext,
                              const nsAString &aID, double *aResult)
{
    // Given an element's id as the parameter, need to query the element and
    //   make sure that it is a xforms:repeat node.  Given that, must query
    //   its index.

    nsCOMPtr<nsIXFormsXPathState> state;
    aContext->GetState(getter_AddRefs(state));
    nsCOMPtr<nsIDOMNode> resolverNode;
    state->GetXformsNode(getter_AddRefs(resolverNode));
    NS_ENSURE_TRUE(resolverNode, NS_ERROR_FAILURE);

    // here document is the XForms document
    nsCOMPtr<nsIDOMDocument> document;
    resolverNode->GetOwnerDocument(getter_AddRefs(document));
    NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

    // aID should be the id of a nsIXFormsRepeatElement
    nsCOMPtr<nsIDOMElement> repeatEle;
    nsCOMPtr<nsIDOMElement> resolverEle(do_QueryInterface(resolverNode));
    nsresult rv =
      nsXFormsUtils::GetElementByContextId(resolverEle, aID,
                                           getter_AddRefs(repeatEle));
    NS_ENSURE_SUCCESS(rv, rv);

    // now get the index value from the xforms:repeat.
    PRInt32 index;
    rv = nsXFormsUtils::GetRepeatIndex(repeatEle, &index);
    NS_ENSURE_SUCCESS(rv, rv);

    // repeat's index is 1-based.  If it is 0, then that is still ok since
    // repeat's index can be 0 if uninitialized or if the nodeset that it
    // is bound to is empty (either initially or due to delete remove all
    // of the instance nodes).  If index == -1, then repeatEle isn't an
    // XForms repeat element, so we need to return NaN per spec.
    *aResult = index >= 0 ? index : kNaN;

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Instance(txIFunctionEvaluationContext *aContext,
                                 const nsAString &aInstanceId,
                                 txINodeSet **aResult)
{
    *aResult = nsnull;

    // The state is the node in the XForms document that contained
    //   the expression we are evaluating.  We'll use this to get the
    //   document.  If this isn't here, then something is wrong. Bail.
    nsCOMPtr<nsIXFormsXPathState> state;
    aContext->GetState(getter_AddRefs(state));
    nsCOMPtr<nsIDOMNode> resolverNode;
    state->GetXformsNode(getter_AddRefs(resolverNode));
    NS_ENSURE_TRUE(resolverNode, NS_ERROR_FAILURE);

    // here document is the XForms document
    nsCOMPtr<nsIDOMDocument> document;
    resolverNode->GetOwnerDocument(getter_AddRefs(document));
    NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMElement> instEle;
    nsresult rv = document->GetElementById(aInstanceId,
                                           getter_AddRefs(instEle));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool foundInstance = PR_FALSE;
    if (instEle) {
        nsAutoString localname, namespaceURI;
        instEle->GetLocalName(localname);
        instEle->GetNamespaceURI(namespaceURI);

        foundInstance = localname.EqualsLiteral("instance") &&
                        namespaceURI.EqualsLiteral(NS_NAMESPACE_XFORMS);
    }

    nsCOMPtr<txINodeSet> result =
        do_CreateInstance("@mozilla.org/transformiix-nodeset;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!foundInstance) {
        // We didn't find an instance element with the given id.  Return the
        //   empty result set.
        result.swap(*aResult);

        return NS_OK;
    }

    // Make sure that this element is contained in the same
    //   model as the context node of the expression as per
    //   the XForms 1.0 spec.

    // first step is to get the contextNode passed in to
    //   the evaluation

    nsCOMPtr<nsIDOMNode> xfContextNode;
    rv = aContext->GetContextNode(getter_AddRefs(xfContextNode));
    NS_ENSURE_SUCCESS(rv, rv);

    // now see if the node we found (instEle) and the
    //   context node for the evaluation (xfContextNode) link
    //   back to the same model.
    nsCOMPtr<nsIDOMNode> instNode, modelInstance;
    instNode = do_QueryInterface(instEle);
    rv = nsXFormsUtils::GetModelFromNode(instNode,
                                         getter_AddRefs(modelInstance));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool modelContainsNode =
        nsXFormsUtils::IsNodeAssocWithModel(xfContextNode, modelInstance);

    if (modelContainsNode) {
        // ok, we've found an instance node with the proper id
        //   that fulfills the requirement of being from the
        //   same model as the context node.  Now we need to
        //   return a 'node-set containing just the root
        //   element node of the referenced instance data'.
        //   Wonderful.

        nsCOMPtr<nsIDOMNode> root;
        rv = nsXFormsUtils::GetInstanceDocumentRoot(aInstanceId,
                                                    modelInstance,
                                                    getter_AddRefs(root));
        NS_ENSURE_SUCCESS(rv, rv);

        if (root) {
            result->Add(root);
        }

        result.swap(*aResult);

        return NS_OK;
    }

    // XXX where we need to do the work

    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Max(txINodeSet *aNodeSet, double *aResult)
{
    PRUint32 length;
    nsresult rv = aNodeSet->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    double result = kNaN;
    PRUint32 i;
    for (i = 0; i < length; ++i) {
        double item;
        rv = aNodeSet->ItemAsNumber(i, &item);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!TX_DOUBLE_COMPARE(item, <=, result)) {
            result = item;
        }

        if (TX_DOUBLE_IS_NaN(result)) {
            break;
        }
    }

    *aResult = result;

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Min(txINodeSet *aNodeSet, double *aResult)
{
    PRUint32 length;
    nsresult rv = aNodeSet->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    double result = kNaN;
    PRUint32 i;
    for (i = 0; i < length; ++i) {
        double item;
        rv = aNodeSet->ItemAsNumber(i, &item);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!TX_DOUBLE_COMPARE(item, >=, result)) {
            result = item;
        }

        if (TX_DOUBLE_IS_NaN(result)) {
            break;
        }
    }

    *aResult = result;

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Months(const nsAString & aDuration, double *aResult)
{
    PRInt32 result = 0;
    nsresult rv = nsXFormsUtils::GetMonths(aDuration, &result);
    if (rv == NS_ERROR_ILLEGAL_VALUE) {
        *aResult = kNaN;

        return NS_OK;
    }

    *aResult = result;

    return rv;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Now(nsAString & aResult)
{
    return nsXFormsUtils::GetTime(aResult, true);
}

NS_IMETHODIMP
nsXFormsXPathFunctions::LocalDate(nsAString & aResult)
{
    nsAutoString time;
    nsresult rv = nsXFormsUtils::GetTime(time);
    NS_ENSURE_SUCCESS(rv, rv);

    // since we know that the returned string will be in the format of
    // yyyy-mm-ddThh:mm:ss.ssszzzz, we just need to grab the first 10
    // characters to represent the date and then strip off the time zone
    // information from the end and append it to the string to get our answer
    aResult = Substring(time, 0, 10);
    PRInt32 timeSeparator = time.FindChar(PRUnichar('T'));
    if (timeSeparator == kNotFound) {
      // though this should probably never happen, if this is the case we
      // certainly don't have to worry about timezones.  Just return.
      return NS_OK;
    }

    // Time zone information can be of the format '-hh:ss', '+hh:ss', 'Z' or
    // might be no time zone information at all.
    nsAutoString hms(Substring(time, timeSeparator+1, time.Length()));
    PRInt32 timeZoneSeparator = hms.FindChar(PRUnichar('-'));
    if (timeZoneSeparator == kNotFound) {
      timeZoneSeparator = hms.FindChar(PRUnichar('+'));
      if (timeZoneSeparator == kNotFound) {
        timeZoneSeparator = hms.FindChar(PRUnichar('Z'));
        if (timeZoneSeparator == kNotFound) {
          // no time zone information available
          return NS_OK;
        }
      }
    }

    aResult.Append(Substring(hms, timeZoneSeparator, hms.Length()));
    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::LocalDateTime(nsAString & aResult)
{
    return nsXFormsUtils::GetTime(aResult);
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Property(const nsAString &aProperty,
                                 nsAString &aResult)
{
    // This function can handle "version" and "conformance-level"
    //   which is all that the XForms 1.0 spec is worried about
    if (aProperty.EqualsLiteral("version")) {
        aResult.AssignLiteral("1.0");
    }
    else if (aProperty.EqualsLiteral("conformance-level")) {
        aResult.AssignLiteral("basic");
    }
    else {
        aResult.Truncate();
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Seconds(const nsAString &aDuration, double *aResult)
{
    nsresult rv = nsXFormsUtils::GetSeconds(aDuration, aResult);
    if (rv == NS_ERROR_ILLEGAL_VALUE) {
        *aResult = kNaN;
        rv = NS_OK;
    }

    return rv;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::SecondsFromDateTime(const nsAString &aDateTime,
                                            double *aResult)
{
    nsresult rv = nsXFormsUtils::GetSecondsFromDateTime(aDateTime, aResult);
    if (rv == NS_ERROR_ILLEGAL_VALUE) {
        *aResult = kNaN;
        rv = NS_OK;
    }

    return rv;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Current(txIFunctionEvaluationContext *aContext,
                                txINodeSet **aResult)
{
  *aResult = nsnull;

  // now get the contextNode passed in to the evaluation
  nsCOMPtr<nsIXFormsXPathState> state;
  aContext->GetState(getter_AddRefs(state));
  nsCOMPtr<nsIDOMNode> origContextNode;
  state->GetOriginalContextNode(getter_AddRefs(origContextNode));
  NS_ENSURE_STATE(origContextNode);
  
  nsresult rv;
  nsCOMPtr<txINodeSet> result =
    do_CreateInstance("@mozilla.org/transformiix-nodeset;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  result->Add(origContextNode);
  result.swap(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Event(txIFunctionEvaluationContext *aContext,
                              const nsAString &aName,
                              txINodeSet **aResult)
{
    *aResult = nsnull;
    nsresult rv;

    nsCOMPtr<nsIXFormsXPathState> state;
    aContext->GetState(getter_AddRefs(state));
    nsCOMPtr<nsIDOMNode> xfNode;
    state->GetXformsNode(getter_AddRefs(xfNode));
    NS_ENSURE_TRUE(xfNode, NS_ERROR_FAILURE);

    nsCOMPtr<txINodeSet> result =
        do_CreateInstance("@mozilla.org/transformiix-nodeset;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIXFormsContextInfo> contextInfo;
    nsCOMPtr<nsIXFormsActionModuleElement> actionElt(do_QueryInterface(xfNode));
    if (!actionElt) {
      result.swap(*aResult);
      return NS_OK;
    }

    nsCOMPtr<nsIDOMEvent> domEvent;
    actionElt->GetCurrentEvent(getter_AddRefs(domEvent));
    nsCOMPtr<nsIXFormsDOMEvent> xfEvent(do_QueryInterface(domEvent));
    if (!xfEvent) {
      // Event being called for an nsIDOMEvent that is not an
      // nsIXFormsDOMEvent.
      result.swap(*aResult);
      return NS_OK;
    }

    xfEvent->GetContextInfo(aName, getter_AddRefs(contextInfo));
    if (!contextInfo) {
      // The requested context info property does not exist.
      result.swap(*aResult);
      return NS_OK;
    }

    // Determine the type of context info property.
    PRInt32 resultType;
    contextInfo->GetType(&resultType);

    if (resultType == nsIXFormsContextInfo::NODESET_TYPE) {
      // The context property is a nodeset. Snapshot each individual node
      // in the nodeset and add them one at a time to the txINodeset.
      nsCOMPtr<nsIDOMXPathResult> nodeset;
      contextInfo->GetNodesetValue(getter_AddRefs(nodeset));
      if (nodeset) {
        PRUint32 nodesetSize;
        rv = nodeset->GetSnapshotLength(&nodesetSize);
        NS_ENSURE_SUCCESS(rv, rv);
        for (PRUint32 i=0; i < nodesetSize; ++i) {
          nsCOMPtr<nsIDOMNode> node;
          nodeset->SnapshotItem(i, getter_AddRefs(node));
          result->Add(node);
        }
      }
    } else {
      // The type is a dom node, string, or number. Strings and numbers
      // are encapsulated in a text node.
      nsCOMPtr<nsIDOMNode> node;
      contextInfo->GetNodeValue(getter_AddRefs(node));
      if (node) {
        result->Add(node);
      }
#ifdef DEBUG
      PRInt32 type;
      contextInfo->GetType(&type);
      if (type == nsXFormsContextInfo::STRING_TYPE) {
        nsAutoString str;
        contextInfo->GetStringValue(str);
      } else if (type == nsXFormsContextInfo::NUMBER_TYPE) {
        PRInt32 number;
        contextInfo->GetNumberValue(&number);
      }
#endif
    }

    result.swap(*aResult);

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Power(double aBase, double aExponent, double *aResult)
{
    double result = 0;

    // If base is negative and exponent is not an integral value, or if base
    // is zero and exponent is negative, a domain error occurs, setting the
    // global variable errno to the value EDOM.
    // If the result is too large (ERANGE), we consider the result to be kNaN.
    result = pow(aBase, aExponent);
    if (errno == EDOM || errno == ERANGE) {
      result = kNaN;
    }
    *aResult = result;

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Random(PRBool aSeed, double *aResult)
{
    if (aSeed) {
      // initialize random seed.
      PRUint32 seed = 0;
      PRSize rSize = PR_GetRandomNoise(&seed, sizeof(seed));
      if (rSize) {
        srand (seed);
      }
    }
    *aResult = (rand() / ((double)RAND_MAX + 1.0));

    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Compare(const nsAString &aString1,
                                const nsAString &aString2,
                                double *aResult)
{
    *aResult = aString1.Compare(aString2);
    return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::IsCardNumber(const nsAString & aNumber,
                                     PRBool *aResult)
{
  if (aNumber.IsEmpty()) {
    *aResult = PR_FALSE;
  } else {
    *aResult = nsXFormsUtils::IsCardNumber(aNumber);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::DaysToDate(double aDays, nsAString &aResult)
{
  // This function returns a string containing a lexical xsd:date that
  // corresponds to the number of days passed as the parameter. The aDays
  // parameter represents the difference between the desired date and
  // 1970-01-01.
  aResult.Truncate();

  if (TX_DOUBLE_IS_NaN(aDays))
    return NS_OK;

  // Round total number of days to the nearest whole number.
  PRTime t_days;
  LL_I2L(t_days, floor(aDays+0.5));

  PRTime t_secs, t_secs_per_day, t_usec, usec_per_sec;
  // Calculate total number of seconds in aDays.
  LL_I2L(t_secs_per_day, 86400UL);
  LL_MUL(t_secs, t_days, t_secs_per_day);
  // Convert total seconds to usecs.
  LL_I2L(usec_per_sec, PR_USEC_PER_SEC);
  LL_MUL(t_usec, t_secs, usec_per_sec);

  // Convert the time to xsd:date format.
  PRExplodedTime et;
  PR_ExplodeTime(t_usec, PR_GMTParameters, &et);
  char ctime[60];
  PR_FormatTime(ctime, sizeof(ctime), "%Y-%m-%d", &et);

  aResult.Assign(NS_ConvertASCIItoUTF16(ctime));

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::SecondsToDateTime(double aSeconds, nsAString &aResult)
{
  // This function returns a string containing a lexical xsd:dateTime that
  // corresponds to the number of seconds passed as the parameter. The aSeconds
  // parameter represents the difference between the desired UTC dateTime and
  // 1970-01-01T00:00:00Z.
  aResult.Truncate();

  if (TX_DOUBLE_IS_NaN(aSeconds))
    return NS_OK;

  // Round total number of seconds to the nearest whole number.
  PRTime t_secs;
  LL_I2L(t_secs, floor(aSeconds+0.5));

  // Convert total seconds to usecs.
  PRTime t_usec, usec_per_sec;
  LL_I2L(usec_per_sec, PR_USEC_PER_SEC);
  LL_MUL(t_usec, t_secs, usec_per_sec);

  // Convert the time to xsd:dateTime format.
  PRExplodedTime et;
  PR_ExplodeTime(t_usec, PR_GMTParameters, &et);
  char ctime[60];
  PR_FormatTime(ctime, sizeof(ctime), "%Y-%m-%dT%H:%M:%SZ", &et);

  aResult.Assign(NS_ConvertASCIItoUTF16(ctime));

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::ContextNode(txIFunctionEvaluationContext *aContext,
                                    txINodeSet **aResult)
{
  *aResult = nsnull;

  // Get xforms node that contained the context() expression.
  nsCOMPtr<nsIXFormsXPathState> state;
  aContext->GetState(getter_AddRefs(state));
  nsCOMPtr<nsIDOMNode> xfNode;
  state->GetXformsNode(getter_AddRefs(xfNode));
  NS_ENSURE_TRUE(xfNode, NS_ERROR_FAILURE);

  // Get the context node of the xforms node.
  nsCOMPtr<nsIDOMNode> contextNode;
  PRUint32 contextNodesetSize = 0;
  PRInt32 contextPosition;
  nsCOMPtr<nsIModelElementPrivate> model;
  nsCOMPtr<nsIDOMElement> bindElement;
  nsCOMPtr<nsIXFormsControl> parentControl;
  PRBool outerBind;

  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(xfNode));
  nsresult rv =
    nsXFormsUtils::GetNodeContext(element,
                                  nsXFormsUtils::ELEMENT_WITH_MODEL_ATTR,
                                  getter_AddRefs(model),
                                  getter_AddRefs(bindElement),
                                  &outerBind,
                                  getter_AddRefs(parentControl),
                                  getter_AddRefs(contextNode),
                                  &contextPosition,
                                  (PRInt32*)&contextNodesetSize,
                                  PR_FALSE);

  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<txINodeSet> result =
    do_CreateInstance("@mozilla.org/transformiix-nodeset;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  result->Add(contextNode);
  result.swap(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::AdjustDateTimeToTimezone(const nsAString &aDateTime,
                                                 nsAString &aResult)
{
  // We have three cases to deal with:
  //
  // 1. aDateTime does not include a timezone indicator: 2007-10-07T02:22:00
  //    The schema validator will return a PRTime that was calculated using
  //    PR_LocalTimeParameters and that PRTime value formatted as an
  //    xsd:dateTime is the same 2007-10-07T02:22:00 that was passed in.
  //    We just need to append the local time zone.
  //
  // 2. aDateTime is a UTC (aka GMT) time: 2007-10-07T21:26:43Z
  //    The schema validator will treat aDateTime as GMT and return that as a
  //    PRTime. We convert the GMT time to a time in the local time zone and
  //    append the local time zone.
  //
  // 3. aDateTime includes a time zone component: 2007-10-07T02:22:00-07:00
  //    The schema validator checks if the time zone component is valid, but
  //    does not use it when calculating the PRTime value so this case is
  //    similar to case 1 except that we have to add the given time zone
  //    offset (to get the GMT time of the input aDateTime), convert the GMT
  //    time to the local time zone, and append the local time zone.
  aResult.Truncate();


  nsCOMPtr<nsISchemaValidator> schemaValidator =
    do_CreateInstance("@mozilla.org/schemavalidator;1");
  NS_ENSURE_TRUE(schemaValidator, NS_ERROR_FAILURE);

  PRTime t_dateTime;
  nsresult rv = schemaValidator->ValidateBuiltinTypeDateTime(aDateTime,
                                                             &t_dateTime);

  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  // The dateTime is valid, so get the timeZone information. If there is time
  // zone information we are dealing with case 3 and have a bit more work to
  // do to convert aDateTime to the local time zone.
  nsAutoString timeString, timeZoneString;
  PRInt32 timeSeparator = aDateTime.FindChar(PRUnichar('T'));
  timeString.Append(Substring(aDateTime,
                              timeSeparator + 1,
                              aDateTime.Length() - timeSeparator));
  nsXFormsUtils::GetTimeZone(timeString, timeZoneString);

  PRExplodedTime time;
  char ctime[60];

  if (!timeZoneString.IsEmpty()) {
    // The time zone string will be of the form ('+' | '-') hh ':' mm
    // For example: +05:00, -07:00
    nsAutoString hoursString, minutesString;
    hoursString.Append(Substring(timeZoneString, 1, 2));
    minutesString.Append(Substring(timeZoneString, 4, 2));

    nsresult rv;
    PRInt32 hours = hoursString.ToInteger(&rv);
    PRInt32 minutes = minutesString.ToInteger(&rv);
    PRInt32 tzSecs = (hours * 3600) + (minutes * 60);
    if (timeZoneString.CharAt(0) == '+') {
      // The time zone is relative to GMT so if it is positive, we need to
      // subtract the total number of seconds represented by the time zone;
      // likewise, we add if the time zone is negative.
      tzSecs *= -1;
    }

    PR_ExplodeTime(t_dateTime, PR_LocalTimeParameters, &time);
    // Zero out the gmt and dst information because we don't want
    // PR_NormalizeTime to use the local time zone to get back to
    // GMT before it normalizes (because it would calculate the GMT
    // time relative to the time zone that was part of the input dateTime).
    time.tm_params.tp_gmt_offset = 0;
    time.tm_params.tp_dst_offset = 0;
    // Adjust the total seconds.
    time.tm_sec += tzSecs;
    // Normalize the fields and apply the local time parameters to convert
    // the time to the local time zone.
    PR_NormalizeTime(&time, PR_LocalTimeParameters);
    PR_FormatTime(ctime, sizeof(ctime), "%Y-%m-%dT%H:%M:%S\0", &time);

  } else {
    // This is either a GMT time or no time zone information is available.
    PR_ExplodeTime(t_dateTime, PR_LocalTimeParameters, &time);
    PR_FormatTime(ctime, sizeof(ctime), "%Y-%m-%dT%H:%M:%S\0", &time);
  }

  // Calculate local time zone to append to the result.
  int gmtoffsethour = time.tm_params.tp_gmt_offset / 3600;
  int remainder = time.tm_params.tp_gmt_offset % 3600;
  int gmtoffsetminute = remainder ? remainder / 60 : 0;
  // adjust gmtoffsethour for daylight savings time.
  int dstoffset = time.tm_params.tp_dst_offset / 3600;
  gmtoffsethour += dstoffset;
  if (gmtoffsethour < 0) {
    // Make the gmtoffsethour positive; we'll add the plus or minus
    // to the time zone string.
    gmtoffsethour *= -1;
  }
  
  char zone_location[40];
  const int zoneBufSize = sizeof(zone_location);
  PR_snprintf(zone_location, zoneBufSize, "%c%02d:%02d\0",
              time.tm_params.tp_gmt_offset < 0 ? '-' : '+',
              gmtoffsethour, gmtoffsetminute);

  aResult.AppendLiteral(ctime);
  aResult.Append(NS_ConvertASCIItoUTF16(zone_location));

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Digest(txIFunctionEvaluationContext *aContext,
                               const nsAString &aData,
                               const nsAString &aAlgorithm,
                               const nsAString &aEncoding,
                               nsAString &aResult)
{
  aResult.Truncate();

  PRBool throwException = PR_FALSE;
  
  // Determine the hash algorithm to use.
  PRUint32 hashAlg = 0;

  if (aAlgorithm.EqualsLiteral("MD5")) {
    hashAlg = nsICryptoHash::MD5;
  } else if (aAlgorithm.EqualsLiteral("SHA-1")) {
    hashAlg = nsICryptoHash::SHA1;
  } else if (aAlgorithm.EqualsLiteral("SHA-256")) {
    hashAlg = nsICryptoHash::SHA256;
  } else if (aAlgorithm.EqualsLiteral("SHA-384")) {
    hashAlg = nsICryptoHash::SHA384;
  } else if (aAlgorithm.EqualsLiteral("SHA-512")) {
    hashAlg = nsICryptoHash::SHA512;
  } else {
    // Throw exception.
    throwException = PR_TRUE;
  }

  if (!throwException) {
    // Perform the hash.
    nsresult rv;
  
    nsCOMPtr<nsICryptoHash> hash =
      do_CreateInstance("@mozilla.org/security/hash;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = hash->Init(hashAlg);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsCAutoString data = NS_LossyConvertUTF16toASCII(aData);
    rv = hash->Update(reinterpret_cast<const PRUint8*>(data.get()),
                      data.Length());
    NS_ENSURE_SUCCESS(rv, rv);
 
    // PR_FALSE means return the raw binary data.
    nsCAutoString result;
    rv = hash->Finish(PR_FALSE, result);
    NS_ENSURE_SUCCESS(rv, rv);

    // Encode the result.
    if (aEncoding.IsEmpty() || aEncoding.EqualsLiteral("base64")) {
      char *buffer = PL_Base64Encode((char *)result.get(),
                                     result.Length(), nsnull);
      if (buffer) {
        aResult = ToNewUnicode(NS_ConvertASCIItoUTF16(buffer));
        PR_Free(buffer);
      }
    } else if (aEncoding.EqualsLiteral("hex")) {
      SECItem secItem;
      secItem.data = (unsigned char *)result.get();
      secItem.len = result.Length();
      // 0 means do not add ':' to the hex string.
      char *buffer = CERT_Hexify(&secItem, 0);
      if (buffer) {
        nsCAutoString hexResult(buffer);
        ToLowerCase(hexResult);
        aResult = NS_ConvertASCIItoUTF16(hexResult);
        PORT_Free(buffer);
      }
    } else {
      // Throw exception.
      throwException = PR_TRUE;
    }
  }

  if (throwException) {
    // Get xforms node that contained the digest() expression.
    nsCOMPtr<nsIXFormsXPathState> state;
    aContext->GetState(getter_AddRefs(state));
    nsCOMPtr<nsIDOMNode> xfNode;
    state->GetXformsNode(getter_AddRefs(xfNode));
    NS_ENSURE_TRUE(xfNode, NS_ERROR_FAILURE);

    // If the digest function appears in a computed expression (An XPath
    // expression used by model item properties such as relevant and
    // calculate to include dynamic functionality in XForms), an
    // xforms-compute-exception occurs. If the digest function appears
    // in any other attribute that contains an XPath function, an
    // xforms-binding-exception occurs.
    nsXFormsEvent event = eEvent_BindingException;
    nsAutoString localName, namespaceURI;
    xfNode->GetLocalName(localName);
    if (localName.EqualsLiteral("bind")) {
      xfNode->GetNamespaceURI(namespaceURI);
      if (namespaceURI.EqualsLiteral(NS_NAMESPACE_XFORMS)) {
        event = eEvent_ComputeException;
      }
    }

    // Dispatch the event.
    nsCOMPtr<nsIDOMElement> resolverElement = do_QueryInterface(xfNode);
    nsCOMPtr<nsIModelElementPrivate> modelPriv =
      nsXFormsUtils::GetModel(resolverElement);
    nsCOMPtr<nsIDOMNode> model = do_QueryInterface(modelPriv);
    nsXFormsUtils::DispatchEvent(model, event, nsnull, resolverElement,
                                 nsnull);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsXPathFunctions::Hmac(txIFunctionEvaluationContext *aContext,
                            const nsAString &aKey,
                            const nsAString &aData,
                            const nsAString &aAlgorithm,
                            const nsAString &aEncoding,
                            nsAString &aResult)
{
  aResult.Truncate();

  PRBool throwException = PR_FALSE;
  
  // Determine the hash algorithm to use.
  PRUint32 hashAlg = 0;

  if (aAlgorithm.EqualsLiteral("MD5")) {
    hashAlg = nsICryptoHMAC::MD5;
  } else if (aAlgorithm.EqualsLiteral("SHA-1")) {
    hashAlg = nsICryptoHMAC::SHA1;
  } else if (aAlgorithm.EqualsLiteral("SHA-256")) {
    hashAlg = nsICryptoHMAC::SHA256;
  } else if (aAlgorithm.EqualsLiteral("SHA-384")) {
    hashAlg = nsICryptoHMAC::SHA384;
  } else if (aAlgorithm.EqualsLiteral("SHA-512")) {
    hashAlg = nsICryptoHMAC::SHA512;
  } else {
    // Throw exception.
    throwException = PR_TRUE;
  }

  if (!throwException) {
    // Perform the hash.
    nsresult rv;
  
    nsCOMPtr<nsICryptoHMAC> hmac =
      do_CreateInstance("@mozilla.org/security/hmac;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIKeyObjectFactory> keyObjectFactory(do_GetService(
      "@mozilla.org/security/keyobjectfactory;1", &rv));
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsCAutoString key = NS_LossyConvertUTF16toASCII(aKey);
    nsCOMPtr<nsIKeyObject> keyObject;
    rv = keyObjectFactory->KeyFromString(nsIKeyObject::HMAC, key,
                                         getter_AddRefs(keyObject));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = hmac->Init(hashAlg, keyObject);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsCAutoString data = NS_LossyConvertUTF16toASCII(aData);
    rv = hmac->Update(reinterpret_cast<const PRUint8*>(data.get()),
                      data.Length());
    NS_ENSURE_SUCCESS(rv, rv);

    // PR_FALSE means return the raw binary data.
    nsCAutoString result;
    rv = hmac->Finish(PR_FALSE, result);
    NS_ENSURE_SUCCESS(rv, rv);

    // Encode the result.
    if (aEncoding.IsEmpty() || aEncoding.EqualsLiteral("base64")) {
      char *buffer = PL_Base64Encode((char *)result.get(),
                                     result.Length(), nsnull);
      if (buffer) {
        aResult = ToNewUnicode(NS_ConvertASCIItoUTF16(buffer));
        PR_Free(buffer);
      }
    } else if (aEncoding.EqualsLiteral("hex")) {
      SECItem secItem;
      secItem.data = (unsigned char *)result.get();
      secItem.len = result.Length();
      // 0 means do not add ':' to the hex string.
      char *buffer = CERT_Hexify(&secItem, 0);
      if (buffer) {
        nsCAutoString hexResult(buffer);
        ToLowerCase(hexResult);
        aResult = NS_ConvertASCIItoUTF16(hexResult);
        PORT_Free(buffer);
      }
    } else {
      // Throw exception.
      throwException = PR_TRUE;
    }
  }

  if (throwException) {
    // Get xforms node that contained the digest() expression.
    nsCOMPtr<nsIXFormsXPathState> state;
    aContext->GetState(getter_AddRefs(state));
    nsCOMPtr<nsIDOMNode> xfNode;
    state->GetXformsNode(getter_AddRefs(xfNode));
    NS_ENSURE_TRUE(xfNode, NS_ERROR_FAILURE);

    // If the hmac function appears in a computed expression (An XPath
    // expression used by model item properties such as relevant and
    // calculate to include dynamic functionality in XForms), an
    // xforms-compute-exception occurs. If the hmac function appears
    // in any other attribute that contains an XPath function, an
    // xforms-binding-exception occurs.
    nsXFormsEvent event = eEvent_BindingException;
    nsAutoString localName, namespaceURI;
    xfNode->GetLocalName(localName);
    if (localName.EqualsLiteral("bind")) {
      xfNode->GetNamespaceURI(namespaceURI);
      if (namespaceURI.EqualsLiteral(NS_NAMESPACE_XFORMS)) {
        event = eEvent_ComputeException;
      }
    }

    // Dispatch the event.
    nsCOMPtr<nsIDOMElement> resolverElement = do_QueryInterface(xfNode);
    nsCOMPtr<nsIModelElementPrivate> modelPriv =
      nsXFormsUtils::GetModel(resolverElement);
    nsCOMPtr<nsIDOMNode> model = do_QueryInterface(modelPriv);
    nsXFormsUtils::DispatchEvent(model, eEvent_ComputeException, nsnull,
                                 resolverElement, nsnull);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}
