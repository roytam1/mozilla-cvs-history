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
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Aaron Reed <aaronr@us.ibm.com>
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

#ifndef nsIXFormsUtilityService_h
#define nsIXFormsUtilityService_h

#include "nsIDOMNode.h"


/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* nsIXFormsUtilityService */
#define NS_IXFORMSUTILITYSERVICE_IID_STR "975fb01f-27a7-4dd2-a598-d00109538594"
#define NS_IXFORMSUTILITYSERVICE_IID \
{ 0x975fb01f, 0x27a7, 0x4dd2, \
  { 0xa5, 0x98, 0xd0, 0x1, 0x9, 0x53, 0x85, 0x94 } }

/**
 * Private interface implemented by the nsXFormsUtilityService in XForms
 * extension.
 */
class NS_NO_VTABLE nsIXFormsUtilityService : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXFORMSUTILITYSERVICE_IID)

  enum {
    STATE_OUT_OF_RANGE,
    STATE_IN_RANGE,
    STATE_NOT_A_RANGE
  };

  /**
   * Return true if instance node that element is bound to is readonly.
   */
  NS_IMETHOD IsReadonly(nsIDOMNode *aElement, PRBool *aState) = 0;

  /**
   * Return true if instance node that element is bound to is relevant.
   */
  NS_IMETHOD IsRelevant(nsIDOMNode *aElement, PRBool *aState) = 0;

  /**
   * Return true if instance node that element is bound to is required.
   */
  NS_IMETHOD IsRequired(nsIDOMNode *aElement, PRBool *aState) = 0;

  /**
   * Return true if instance node that element is bound to is valid.
   */
  NS_IMETHOD IsValid(nsIDOMNode *aElement, PRBool *aState) = 0;

  /**
   * Return constant declared above that indicates whether instance node that
   * element is bound to is out of range, is in range or neither. The last value
   * is used if element can't have in-range or out-of-range state, for exmple,
   * xforms:input.
   */
  NS_IMETHOD IsInRange(nsIDOMNode *aElement, PRUint32 *aState) = 0;

  /**
   * Return value of instance node that element is bound to.
   */
  NS_IMETHOD GetValue(nsIDOMNode *aElement, nsAString& aValue) = 0;

  /**
   * Return @start attribute value of xforms:range element. Failure if
   * given element is not xforms:range.
   */
  NS_IMETHOD GetRangeStart(nsIDOMNode *aElement, nsAString& aValue) = 0;

  /**
   * Return @end attribute value of xforms:range element. Failure if
   * given element is not xforms:range.
   */
  NS_IMETHOD GetRangeEnd(nsIDOMNode *aElement, nsAString& aValue) = 0;

  /**
   * Return @step attribute value of xforms:range element. Failure if
   * given element is not xforms:range.
   */
  NS_IMETHOD GetRangeStep(nsIDOMNode *aElement, nsAString& aValue) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXFormsUtilityService,
                              NS_IXFORMSUTILITYSERVICE_IID)

#endif /* nsIXFormsUtilityService_h */
