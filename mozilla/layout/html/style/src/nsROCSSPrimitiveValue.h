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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsROCSSPrimitiveValue_h___
#define nsROCSSPrimitiveValue_h___

#include "nsIDOMCSSPrimitiveValue.h"
#include "gfxtypes.h"
#include "nsUnitConversion.h"
#include "nsIScriptObjectOwner.h"

#include "nsCOMPtr.h"
#include "nsDOMError.h"


class nsROCSSPrimitiveValue : public nsIDOMCSSPrimitiveValue,
                              public nsIScriptObjectOwner
{
public:
  NS_DECL_ISUPPORTS

  // nsIDOMCSSPrimitiveValue
  NS_DECL_IDOMCSSPRIMITIVEVALUE

  // nsIDOMCSSValue
  NS_DECL_IDOMCSSVALUE

  // nsIScriptObjectOwner
  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void* aScriptObject);

  // nsROCSSPrimitiveValue
  nsROCSSPrimitiveValue(nsISupports *aOwner);
  virtual ~nsROCSSPrimitiveValue();

  void SetTwips(nscoord aValue)
  {
    mTwips = aValue;
  }

  void SetString(const char *aString)
  {
    mString.AssignWithConversion(aString);
    mType = CSS_STRING;
  }

  void SetString(const nsString& aString)
  {
    mString.Assign(aString);
    mType = CSS_STRING;
  }

private:
  PRUint16 mType;

  nscoord mTwips;
  nsString mString;

  nsISupports *mOwner;

  void* mScriptObject;
};

#endif /* nsROCSSPrimitiveValue_h___ */

