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
 * The Original Code is Mozilla Schema Validation.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Doron Rosenberg <doronr@us.ibm.com> (original author)
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

#ifndef __nsSchemaValidator_h__
#define __nsSchemaValidator_h__

#include "nsISchemaValidator.h"
#include "nsISchema.h"
#include "nsCOMPtr.h"

 typedef struct Schema_GDay {
   int day;            // day represented (1-31)
   PRBool tz_negative; // is timezone negative
   int tz_hour;        // timezone - hour (0-23) - null if not specified
   int tz_minute;      // timezone - minute (0-59) - null if not specified
 } ;

 typedef struct Schema_GMonth {
   int month;          // month represented (1-12)
   PRBool tz_negative; // is timezone negative
   int tz_hour;        // timezone - hour (0-23) - null if not specified
   int tz_minute;      // timezone - minute (0-59) - null if not specified
 } ;

 typedef struct Schema_GYear {
   long year;          // year 
   PRBool tz_negative; // is timezone negative
   int tz_hour;        // timezone - hour (0-23) - null if not specified
   int tz_minute;      // timezone - minute (0-59) - null if not specified
 } ;

 typedef struct Schema_GYearMonth {
   Schema_GYear gYear;
   Schema_GMonth gMonth;
 } ;

 typedef struct Schema_GMonthDay {
   Schema_GMonth gMonth;
   Schema_GDay gDay;
 } ;

 typedef struct Schema_Duration {
   long years;
   long months;
   long days;
   long hours;
   long minutes;
   long seconds;
   long fractional_seconds;

   Schema_Duration() {
     // -1 means not set
     years = -1;
     months = -1;
     days = -1;
     hours = -1;
     minutes = -1;
     seconds = -1;
     fractional_seconds = -1;
   }
 } ;

/* eced2af3-fde9-4575-b5a4-e1c830b24611 */
#define NS_SCHEMAVALIDATOR_CID \
{ 0xeced2af3, 0xfde9, 0x4575, \
  {0xb5, 0xa4, 0xe1, 0xc8, 0x30, 0xb2, 0x46, 0x11}}     

#define NS_SCHEMAVALIDATOR_CONTRACTID "@mozilla.org/schemavalidator;1"

#define NS_ERROR_SCHEMAVALIDATOR_NO_SCHEMA_LOADED      NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_SCHEMA, 1)
#define NS_ERROR_SCHEMAVALIDATOR_NO_DOM_NODE_LOADED    NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_SCHEMA, 2)
#define NS_ERROR_SCHEMAVALIDATOR_NO_TYPE_FOUND         NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_SCHEMA, 3)
#define NS_ERROR_SCHEMAVALIDATOR_TYPE_NOT_FOUND        NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_SCHEMA, 4)

class nsSchemaValidator : public nsISchemaValidator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCHEMAVALIDATOR

  nsSchemaValidator();

private:
  ~nsSchemaValidator();

  // methods dealing with simpletypes
  nsresult ValidateSimpletype(const nsAString & aNodeValue, nsISchemaSimpleType *aSchemaSimpleType, PRBool *aResult);
  nsresult ValidateRestrictionSimpletype(const nsAString & aNodeValue, nsISchemaSimpleType *aSchemaSimpleType, PRBool *aResult);
  nsresult ValidateBuiltinType(const nsAString & aNodeValue, nsISchemaSimpleType *aSchemaSimpleType, PRBool *aResult);

  // methods dealing with validation of built-in types
  nsresult ValidateBuiltinTypeString(const nsAString & aNodeValue,
      PRUint32 aLength, PRUint32 aMinLength, PRUint32 aMaxLength,
      PRBool *aResult);

  nsresult ValidateBuiltinTypeBoolean(const nsAString & aNodeValue,
      PRBool *aResult);

  nsresult ValidateBuiltinTypeGDay(const nsAString & aNodeValue,
    const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
    const nsAString & aMaxInclusive, const nsAString & aMinInclusive,
    PRBool *aResult);
  PRBool IsValidSchemaGDay(const nsAString & aNodeValue, Schema_GDay *aResult);

  PRBool IsValidSchemaGType(const nsAString & aNodeValue,
    long aMinValue, long aMaxValue, int *aResult);

  nsresult ValidateBuiltinTypeGMonth(const nsAString & aNodeValue,
    const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
    const nsAString & aMaxInclusive, const nsAString & aMinInclusive,
    PRBool *aResult);
  PRBool IsValidSchemaGMonth(const nsAString & aNodeValue, Schema_GMonth *aResult);

  nsresult ValidateBuiltinTypeGYear(const nsAString & aNodeValue,
    const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
    const nsAString & aMaxInclusive, const nsAString & aMinInclusive,
    PRBool *aResult);
  PRBool IsValidSchemaGYear(const nsAString & aNodeValue, Schema_GYear *aResult);

  nsresult ValidateBuiltinTypeGYearMonth(const nsAString & aNodeValue,
    const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
    const nsAString & aMaxInclusive, const nsAString & aMinInclusive,
    PRBool *aResult);
  PRBool IsValidSchemaGYearMonth(const nsAString & aNodeValue, Schema_GYearMonth *aYearMonth);

  nsresult ValidateBuiltinTypeGMonthDay(const nsAString & aNodeValue,
    const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
    const nsAString & aMaxInclusive, const nsAString & aMinInclusive,
    PRBool *aResult);
  PRBool IsValidSchemaGMonthDay(const nsAString & aNodeValue, Schema_GMonthDay *aYearMonth);

  nsresult ValidateBuiltinTypeDateTime(const nsAString & aNodeValue,
    const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
    const nsAString & aMaxInclusive, const nsAString & aMinInclusive,
    PRBool *aResult);
  int CompareSchemaDateTime(PRExplodedTime datetime1,
    PRBool isDateTime1Negative, PRTime datetime2,
    PRBool isDateTime2Negative);
  PRBool IsValidSchemaDateTime(const nsAString & aNodeValue, PRTime *aResult);

  nsresult ValidateBuiltinTypeDate(const nsAString & aNodeValue,
    const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
    const nsAString & aMaxInclusive, const nsAString & aMinInclusive,
    PRBool *aResult);
  PRBool IsValidSchemaDate(const nsAString & aNodeValue, PRTime *aResult);

  nsresult ValidateBuiltinTypeTime(const nsAString & aNodeValue,
    const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
    const nsAString & aMaxInclusive, const nsAString & aMinInclusive,
    PRBool *aResult);
  PRBool IsValidSchemaTime(const nsAString & aNodeValue, PRTime *aResult);

  nsresult ValidateBuiltinTypeInteger(const nsAString & aNodeValue, PRUint32 aTotalDigits, const nsAString & aMaxExclusive, 
      const nsAString & aMaxInclusive, const nsAString & aMinInclusive, const nsAString & aMinExclusive, PRBool *aResult);
  int CompareStrings(const nsAString & aString1, const nsAString & aString2);

  nsresult ValidateBuiltinTypeFloat(const nsAString & aNodeValue, PRUint32 aTotalDigits, const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
      const nsAString & aMaxInclusive, const nsAString & aMinInclusive, PRBool *aResult);
  PRBool IsValidSchemaFloat(const nsAString & aNodeValue, float *aResult);

  nsresult ValidateBuiltinTypeByte(const nsAString & aNodeValue, PRUint32 aTotalDigits, const nsAString & aMaxExclusive, 
      const nsAString & aMaxInclusive, const nsAString & aMinInclusive, const nsAString & aMinExclusive, PRBool *aResult);
  PRBool IsValidSchemaByte(const nsAString & aNodeValue, long *aResult);

  nsresult ValidateBuiltinTypeDecimal(const nsAString & aNodeValue, PRUint32 aTotalDigits, const nsAString & aMaxExclusive, const nsAString & aMinExclusive,
      const nsAString & aMaxInclusive, const nsAString & aMinInclusive, PRBool *aResult);
  PRBool IsValidSchemaDecimal(const nsAString & aNodeValue, nsAString & aWholePart, nsAString & aFractionPart);
  int CompareFractionStrings(const nsAString & aString1, const nsAString & aString2);

  nsresult ValidateBuiltinTypeAnyURI(const nsAString & aNodeValue, 
      PRUint32 aLength, PRUint32 aMinLength, PRUint32 aMaxLength,
      PRBool *aResult);
  PRBool IsValidSchemaAnyURI(const nsAString & aString);

  nsresult ValidateBuiltinTypeBase64Binary(const nsAString & aNodeValue, 
      PRUint32 aLength, PRUint32 aMinLength, PRUint32 aMaxLength,
      PRBool *aResult);
  PRBool IsValidSchemaBase64Binary(const nsAString & aString, char** aDecodedString);

  // helper methods
  void DumpBaseType(nsISchemaBuiltinType *aBuiltInType);

protected:
  nsCOMPtr<nsISchema> mSchema;
};

#endif // __nsSchemaValidator_h__
