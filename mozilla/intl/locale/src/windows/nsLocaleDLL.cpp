/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCollationCID.h"
#include "nsCollationWin.h"
#include "nsDateTimeFormatCID.h"
#include "nsDateTimeFormatWin.h"
#include "nsFontPackageService.h"
#include "nsIGenericFactory.h"
#include "nsILocaleService.h"
#include "nsIScriptableDateFormat.h"
#include "nsIServiceManager.h"
#include "nsLanguageAtomService.h"
#include "nsLocaleCID.h"
#include "nsIWin32LocaleImpl.h"

#define MAKE_CTOR(ctor_, iface_, func_)                   \
static NS_IMETHODIMP                                      \
ctor_(nsISupports* aOuter, REFNSIID aIID, void** aResult) \
{                                                         \
  *aResult = nsnull;                                      \
  if (aOuter)                                             \
    return NS_ERROR_NO_AGGREGATION;                       \
  iface_* inst;                                           \
  nsresult rv = func_(&inst);                             \
  if (NS_SUCCEEDED(rv)) {                                 \
    rv = inst->QueryInterface(aIID, aResult);             \
    NS_RELEASE(inst);                                     \
  }                                                       \
  return rv;                                              \
}


MAKE_CTOR(CreateLocaleService, nsILocaleService, NS_NewLocaleService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIWin32LocaleImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDateTimeFormatWin)
//NS_GENERIC_FACTORY_CONSTRUCTOR(nsScriptableDateTimeFormat)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLanguageAtomService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontPackageService)

// The list of components we register
static const nsModuleComponentInfo gComponents[] = {
  { "nsLocaleService component",
    NS_LOCALESERVICE_CID,
    NS_LOCALESERVICE_CONTRACTID,
    CreateLocaleService },

  { "Platform locale",
    NS_WIN32LOCALE_CID,
    NS_WIN32LOCALE_CONTRACTID,
    nsIWin32LocaleImplConstructor },

  { "Collation factory",
    NS_COLLATIONFACTORY_CID,
    NULL,
    nsCollationFactoryConstructor },

  { "Collation",
    NS_COLLATION_CID,
    NULL,
    nsCollationWinConstructor },

  { "Date/Time formatter",
    NS_DATETIMEFORMAT_CID,
    NULL,
    nsDateTimeFormatWinConstructor },

  { "Scriptable Date Format",
    NS_SCRIPTABLEDATEFORMAT_CID,
    NS_SCRIPTABLEDATEFORMAT_CONTRACTID,
    NS_NewScriptableDateFormat },

  { "Language Atom Service",
    NS_LANGUAGEATOMSERVICE_CID,
    NS_LANGUAGEATOMSERVICE_CONTRACTID,
    nsLanguageAtomServiceConstructor },

  { "Font Package Service",
    NS_FONTPACKAGESERVICE_CID,
    NS_FONTPACKAGESERVICE_CONTRACTID,
    nsFontPackageServiceConstructor },
};

NS_IMPL_NSGETMODULE(nsLocaleModule, gComponents)
