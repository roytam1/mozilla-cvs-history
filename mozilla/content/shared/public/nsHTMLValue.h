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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
#ifndef nsHTMLValue_h___
#define nsHTMLValue_h___

#include "nscore.h"
#include "nsColor.h"
#include "nsString.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"

#include "nsReadableUtils.h"
#include "nsCRT.h"

class nsCheapStringBufferUtils {
public:
  /**
   * Get the string pointer
   * @param aBuf the buffer
   * @return a pointer to the string
   */
  static const PRUnichar* StrPtr(const PRUnichar* aBuf) {
    NS_ASSERTION(aBuf, "Cannot work on null buffer!");
    return (const PRUnichar*)( ((const char*)aBuf) + sizeof(PRUint32) );
  }
  static PRUnichar* StrPtr(PRUnichar* aBuf) {
    NS_ASSERTION(aBuf, "Cannot work on null buffer!");
    return (PRUnichar*)( ((char*)aBuf) + sizeof(PRUint32) );
  }
  /**
   * Get the string length
   * @param aBuf the buffer
   * @return the string length
   */
  static PRUint32 Length(const PRUnichar* aBuf) {
    NS_ASSERTION(aBuf, "Cannot work on null buffer!");
    return *((PRUint32*)aBuf);
  }
  /**
   * Get a DependentString from a buffer
   *
   * @param aBuf the buffer to get string from
   * @return a DependentString representing this string
   */
  static nsDependentSingleFragmentSubstring GetDependentString(const PRUnichar* aBuf) {
    NS_ASSERTION(aBuf, "Cannot work on null buffer!");
    const PRUnichar* buf = StrPtr(aBuf);
    return Substring(buf, buf + Length(aBuf));
  }
  /**
   * Construct from an AString
   * @param aBuf the buffer to copy to
   * @param aStr the string to construct from
   */
  static void CopyToBuffer(PRUnichar*& aBuf, const nsAString& aStr) {
    PRUint32 len = aStr.Length();
    aBuf = (PRUnichar*)nsMemory::Alloc(sizeof(PRUint32) +
                                       len * sizeof(PRUnichar));
    *((PRUint32*)aBuf) = len;
    CopyUnicodeTo(aStr, 0, StrPtr(aBuf), len);
  }
  /**
   * Construct from another nsCheapStringBuffer
   * @param aBuf the buffer to put into
   * @param aSrc the buffer to construct from
   */
  static void Clone(PRUnichar*& aBuf, const PRUnichar* aSrc) {
    NS_ASSERTION(aSrc, "Cannot work on null buffer!");
    aBuf = (PRUnichar*)nsMemory::Clone(aSrc, sizeof(PRUint32) +
                                             Length(aSrc) * sizeof(PRUnichar));
  }
  /**
   * Free the memory for the buf
   * @param aBuf the buffer to free
   */
  static void Free(PRUnichar* aBuf) {
    NS_ASSERTION(aBuf, "Cannot work on null buffer!");
    nsMemory::Free(aBuf);
  }
  /**
   * Get a hashcode for the buffer
   * @param aBuf the buffer
   * @return the hashcode
   */
  static PRUint32 HashCode(const PRUnichar* aBuf) {
    NS_ASSERTION(aBuf, "Cannot work on null buffer!");
    return nsCRT::BufferHashCode((char*)StrPtr(aBuf),
                                 Length(aBuf)*sizeof(PRUnichar));
  }
};

//
// nsHTMLUnit is two bytes: the class of type, and a specifier to distinguish
// between different things stored as the same type.  Doing
// mUnit & HTMLUNIT_CLASS_MASK should give you the class of type.
//
#define HTMLUNIT_NOSTORE    0x0000
#define HTMLUNIT_STRING     0x0100
#define HTMLUNIT_INTEGER    0x0200
#define HTMLUNIT_PIXEL      0x0400
#define HTMLUNIT_COLOR      0x0800
#define HTMLUNIT_ISUPPORTS  0x1000
#define HTMLUNIT_PERCENT    0x2000
#define HTMLUNIT_CLASS_MASK 0xff00

enum nsHTMLUnit {
  // null, value is not specified: 0x0000
  eHTMLUnit_Null          = HTMLUNIT_NOSTORE,
  // empty, value is not specified: 0x0001
  eHTMLUnit_Empty         = HTMLUNIT_NOSTORE | 1,

  // a string value
  eHTMLUnit_String        = HTMLUNIT_STRING,
  // a color name value
  eHTMLUnit_ColorName     = HTMLUNIT_STRING | 1,

  // a simple int value
  eHTMLUnit_Integer       = HTMLUNIT_INTEGER,
  // value has enumerated meaning
  eHTMLUnit_Enumerated    = HTMLUNIT_INTEGER | 1,
  // value is a relative proportion of some whole
  eHTMLUnit_Proportional  = HTMLUNIT_INTEGER | 2,

  // screen pixels (screen relative measure)
  eHTMLUnit_Pixel         = HTMLUNIT_PIXEL,

  // an RGBA value
  eHTMLUnit_Color         = HTMLUNIT_COLOR,

  // (nsISupports*) a ref counted interface
  eHTMLUnit_ISupports     = HTMLUNIT_ISUPPORTS,

  // (1.0 == 100%) value is percentage of something
  eHTMLUnit_Percent       = HTMLUNIT_PERCENT
};

/**
 * Class which is used to represent the value of an attribute of an
 * HTML element.  The value has a unit which is an nsHTMLUnit;
 * checking the unit is a must before asking for the value in any
 * particular form.
 */
class nsHTMLValue {
public:
  nsHTMLValue(nsHTMLUnit aUnit = eHTMLUnit_Null);
  nsHTMLValue(PRInt32 aValue, nsHTMLUnit aUnit);
  nsHTMLValue(float aValue);
  nsHTMLValue(const nsAString& aValue, nsHTMLUnit aUnit = eHTMLUnit_String);
  nsHTMLValue(nsISupports* aValue);
  nsHTMLValue(nscolor aValue);
  nsHTMLValue(const nsHTMLValue& aCopy);
  ~nsHTMLValue(void);

  nsHTMLValue&  operator=(const nsHTMLValue& aCopy);
  PRBool        operator==(const nsHTMLValue& aOther) const;
  PRBool        operator!=(const nsHTMLValue& aOther) const;
  PRUint32      HashValue(void) const;

  /**
   * Get the unit of this HTMLValue
   * @return the unit of this HTMLValue
   */
  nsHTMLUnit   GetUnit(void) const { return (nsHTMLUnit)mUnit; }

  PRInt32      GetIntValue(void) const;
  PRInt32      GetPixelValue(void) const;
  float        GetPercentValue(void) const;
  nsAString&   GetStringValue(nsAString& aBuffer) const;
  already_AddRefed<nsISupports> GetISupportsValue(void) const;
  nscolor      GetColorValue(void) const;

  /**
   * Reset the string to null type, freeing things in the process if necessary.
   */
  void  Reset(void);
  void  SetIntValue(PRInt32 aValue, nsHTMLUnit aUnit);
  void  SetPixelValue(PRInt32 aValue);
  void  SetPercentValue(float aValue);
  void  SetStringValue(const nsAString& aValue, nsHTMLUnit aUnit = eHTMLUnit_String);
  void  SetISupportsValue(nsISupports* aValue);
  void  SetColorValue(nscolor aValue);
  void  SetEmptyValue(void);

#ifdef DEBUG
  void  AppendToString(nsAString& aBuffer) const;
#endif

protected:
  /**
   * The unit of the value
   * @see nsHTMLUnit
   */
  PRUint32 mUnit;
  /**
   * The actual value.  Please to not be adding more-than-4-byte things to this
   * union.
   */
  union {
    /** Int. */
    PRInt32       mInt;
    /** Float. */
    float         mFloat;
    /** String.  First 4 bytes are the length, non-null-terminated. */
    PRUnichar*    mString;
    /** ISupports.  Strong reference.  */
    nsISupports*  mISupports;
    /** Color. */
    nscolor       mColor;
  } mValue;
private:
  /**
   * Copy into this HTMLValue from aCopy.  Please be aware that if this is an
   * existing HTMLValue and you do not call Reset(), this will leak.
   * @param aCopy the value to copy
   */
  void InitializeFrom(const nsHTMLValue& aCopy);
  /**
   * Helper to set string value (checks for embedded nulls or length); verifies
   * that aUnit is a string type as well.
   * @param aValue the value to set
   * @param aUnit the unit to set
   */
  void SetStringValueInternal(const nsAString& aValue, nsHTMLUnit aUnit);
  /**
   * Get a DependentString from mValue.mString (if the string is stored with
   * length, passes that information to the DependentString).  Do not call this
   * if mValue.mString is null.
   *
   * @return a DependentString representing this string
   */
  nsDependentSingleFragmentSubstring GetDependentString() const;
  /**
   * Get the unit class (HTMLUNIT_*)
   * @return the unit class
   */
  PRUint32 GetUnitClass() const { return mUnit & HTMLUNIT_CLASS_MASK; }
};

inline nsDependentSingleFragmentSubstring nsHTMLValue::GetDependentString() const
{
  NS_ASSERTION(GetUnitClass() == HTMLUNIT_STRING,
               "Some dork called GetDependentString() on a non-string!");
  static const PRUnichar blankStr[] = { '\0' };
  return mValue.mString
         ? nsCheapStringBufferUtils::GetDependentString(mValue.mString)
         : Substring(blankStr, blankStr);
}

inline PRInt32 nsHTMLValue::GetIntValue(void) const
{
  NS_ASSERTION(GetUnitClass() == HTMLUNIT_STRING ||
               GetUnitClass() == HTMLUNIT_INTEGER,
               "not an int value");
  PRUint32 unitClass = GetUnitClass();
  if (unitClass == HTMLUNIT_INTEGER) {
    return mValue.mInt;
  }

  if (unitClass == HTMLUNIT_STRING) {
    if (mValue.mString) {
      PRInt32 err=0;
      // XXX this copies. new string APIs will make this better, right?
      nsAutoString str(GetDependentString());
      return str.ToInteger(&err);
    }
  }

  return 0;
}

inline PRInt32 nsHTMLValue::GetPixelValue(void) const
{
  NS_ASSERTION((mUnit == eHTMLUnit_Pixel), "not a pixel value");
  if (mUnit == eHTMLUnit_Pixel) {
    return mValue.mInt;
  }
  return 0;
}

inline float nsHTMLValue::GetPercentValue(void) const
{
  NS_ASSERTION(mUnit == eHTMLUnit_Percent, "not a percent value");
  if (mUnit == eHTMLUnit_Percent) {
    return mValue.mFloat;
  }
  return 0.0f;
}

inline nsAString& nsHTMLValue::GetStringValue(nsAString& aBuffer) const
{
  NS_ASSERTION(GetUnitClass() == HTMLUNIT_STRING || mUnit == eHTMLUnit_Null,
               "not a string value");
  if (GetUnitClass() == HTMLUNIT_STRING && mValue.mString) {
    aBuffer = GetDependentString();
  } else {
    aBuffer.Truncate();
  }
  return aBuffer;
}

inline already_AddRefed<nsISupports> nsHTMLValue::GetISupportsValue(void) const
{
  NS_ASSERTION(mUnit == eHTMLUnit_ISupports, "not an ISupports value");
  if (mUnit == eHTMLUnit_ISupports) {
    nsISupports *result = mValue.mISupports;
    NS_IF_ADDREF(result);
    return result;
  }
  return nsnull;
}

inline nscolor nsHTMLValue::GetColorValue(void) const 
{
  NS_ASSERTION((mUnit == eHTMLUnit_Color) || (mUnit == eHTMLUnit_ColorName), 
               "not a color value");
  if (mUnit == eHTMLUnit_Color) {
    return mValue.mColor;
  }
  if (mUnit == eHTMLUnit_ColorName) {
    nscolor color;
    if (NS_ColorNameToRGB(GetDependentString(), &color)) {
      return color;
    }
  }
  return NS_RGB(0,0,0);
}

inline PRBool nsHTMLValue::operator!=(const nsHTMLValue& aOther) const
{
  return PRBool(! ((*this) == aOther));
}

#endif /* nsHTMLValue_h___ */

