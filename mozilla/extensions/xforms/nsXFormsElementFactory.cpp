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

#include "nsXFormsElementFactory.h"
#include "nsXFormsModelElement.h"
#include "nsXFormsInstanceElement.h"
#include "nsXFormsSubmissionElement.h"
#include "nsXFormsStubElement.h"
#include "nsString.h"

NS_HIDDEN_(nsresult) NS_NewXFormsInputElement(nsIXTFElement **aElement);
NS_HIDDEN_(nsresult) NS_NewXFormsOutputElement(nsIXTFElement **aElement);

NS_IMPL_ISUPPORTS1(nsXFormsElementFactory, nsIXTFElementFactory)

NS_IMETHODIMP
nsXFormsElementFactory::CreateElement(const nsAString& aTagName,
                                      nsIXTFElement **aElement)
{
  if (aTagName.EqualsLiteral("model"))
    return NS_NewXFormsModelElement(aElement);
  if (aTagName.EqualsLiteral("instance"))
    return NS_NewXFormsInstanceElement(aElement);
  if (aTagName.EqualsLiteral("bind"))
    return NS_NewXFormsStubElement(aElement);
  if (aTagName.EqualsLiteral("input"))
    return NS_NewXFormsInputElement(aElement);
  if (aTagName.EqualsLiteral("output"))
    return NS_NewXFormsOutputElement(aElement);
  if (aTagName.EqualsLiteral("submission"))
    return NS_NewXFormsSubmissionElement(aElement);

  *aElement = nsnull;
  return NS_ERROR_FAILURE;
}
