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

#include "nsDefaultSOAPEncoder.h"
#include "nsSOAPUtils.h"
#include "nsSOAPParameter.h"
#include "nsISOAPAttachments.h"
#include "nsXPIDLString.h"
#include "nsIDOMDocument.h"
#include "nsIDOMText.h"
#include "nsCOMPtr.h"
#include "nsISchema.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsISupportsPrimitives.h"
#include "nsIDOMParser.h"
#include "nsSOAPUtils.h"
#include "nsISOAPEncoding.h"
#include "nsISOAPEncoder.h"
#include "nsISOAPDecoder.h"
#include "prprf.h"
#include "prdtoa.h"
#include "nsReadableUtils.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMAttr.h"

NS_NAMED_LITERAL_STRING(kOne,"1");
NS_NAMED_LITERAL_STRING(kZero,"0");

NS_NAMED_LITERAL_STRING(kEmpty,"");

NS_NAMED_LITERAL_STRING(kSchemaNamespaceURI,"http://www.w3.org/2001/XMLSchema");
NS_NAMED_LITERAL_STRING(kSchemaDatatypesNamespaceURI,"http://www.w3.org/2001/XMLSchema-datatypes");
NS_NAMED_LITERAL_STRING(kSchemaTypeAttribute,"type");
NS_NAMED_LITERAL_STRING(kSOAPArrayTypeAttribute,"arrayType");

NS_NAMED_LITERAL_STRING(kAnyTypeSchemaType, "anyType");
NS_NAMED_LITERAL_STRING(kAnySimpleTypeSchemaType, "anySimpleType");
NS_NAMED_LITERAL_STRING(kArraySOAPType, "Array");

NS_NAMED_LITERAL_STRING(kStringSchemaType, "string");
NS_NAMED_LITERAL_STRING(kBooleanSchemaType, "boolean");
NS_NAMED_LITERAL_STRING(kFloatSchemaType, "float");
NS_NAMED_LITERAL_STRING(kDoubleSchemaType, "double");
NS_NAMED_LITERAL_STRING(kLongSchemaType, "long");
NS_NAMED_LITERAL_STRING(kIntSchemaType, "int");
NS_NAMED_LITERAL_STRING(kShortSchemaType, "short");
NS_NAMED_LITERAL_STRING(kByteSchemaType, "byte");
NS_NAMED_LITERAL_STRING(kUnsignedLongSchemaType, "unsignedLong");
NS_NAMED_LITERAL_STRING(kUnsignedIntSchemaType, "unsignedInt");
NS_NAMED_LITERAL_STRING(kUnsignedShortSchemaType, "unsignedShort");
NS_NAMED_LITERAL_STRING(kUnsignedByteSchemaType, "unsignedByte");

NS_NAMED_LITERAL_STRING(kDurationSchemaType, "duration");
NS_NAMED_LITERAL_STRING(kDateTimeSchemaType, "dateTime");
NS_NAMED_LITERAL_STRING(kTimeSchemaType, "time");
NS_NAMED_LITERAL_STRING(kDateSchemaType, "date");
NS_NAMED_LITERAL_STRING(kGYearMonthSchemaType, "gYearMonth");
NS_NAMED_LITERAL_STRING(kGYearSchemaType, "gYear");
NS_NAMED_LITERAL_STRING(kGMonthDaySchemaType, "gMonthDay");
NS_NAMED_LITERAL_STRING(kGDaySchemaType, "gDay");
NS_NAMED_LITERAL_STRING(kGMonthSchemaType, "gMonth");
NS_NAMED_LITERAL_STRING(kHexBinarySchemaType, "hexBinary");
NS_NAMED_LITERAL_STRING(kBase64BinarySchemaType, "base64Binary");
NS_NAMED_LITERAL_STRING(kAnyURISchemaType, "anyURI");
NS_NAMED_LITERAL_STRING(kQNameSchemaType, "QName");
NS_NAMED_LITERAL_STRING(kNOTATIONSchemaType, "NOTATION");
NS_NAMED_LITERAL_STRING(kNormalizedStringSchemaType, "normalizedString");
NS_NAMED_LITERAL_STRING(kTokenSchemaType, "token");
NS_NAMED_LITERAL_STRING(kLanguageSchemaType, "language");
NS_NAMED_LITERAL_STRING(kNMTOKENSchemaType, "NMTOKEN");
NS_NAMED_LITERAL_STRING(kNMTOKENSSchemaType, "NMTOKENS");
NS_NAMED_LITERAL_STRING(kNameSchemaType, "Name");
NS_NAMED_LITERAL_STRING(kNCNameSchemaType, "NCName");
NS_NAMED_LITERAL_STRING(kIDSchemaType, "ID");
NS_NAMED_LITERAL_STRING(kIDREFSchemaType, "IDREF");
NS_NAMED_LITERAL_STRING(kIDREFSSchemaType, "IDREFS");
NS_NAMED_LITERAL_STRING(kENTITYSchemaType, "ENTITY");
NS_NAMED_LITERAL_STRING(kENTITIESSchemaType, "ENTITIES");
NS_NAMED_LITERAL_STRING(kDecimalSchemaType, "decimal");
NS_NAMED_LITERAL_STRING(kIntegerSchemaType, "integer");
NS_NAMED_LITERAL_STRING(kNonPositiveIntegerSchemaType, "nonPositiveInteger");
NS_NAMED_LITERAL_STRING(kNegativeIntegerSchemaType, "negativeInteger");
NS_NAMED_LITERAL_STRING(kNonNegativeIntegerSchemaType, "nonNegativeInteger");
NS_NAMED_LITERAL_STRING(kPositiveIntegerSchemaType, "positiveInteger");

NS_NAMED_LITERAL_STRING(kTrue, "true");
NS_NAMED_LITERAL_STRING(kFalse, "false");

NS_NAMED_LITERAL_STRING(kTrueA, "1");
NS_NAMED_LITERAL_STRING(kFalseA, "0");

#define DECLARE_ENCODER(name) \
class ns##name##Encoder : \
  public nsISOAPEncoder, \
  public nsISOAPDecoder \
{\
public:\
  ns##name##Encoder();\
  virtual ~ns##name##Encoder();\
  NS_DECL_ISUPPORTS\
  NS_DECL_NSISOAPENCODER\
  NS_DECL_NSISOAPDECODER\
};\
ns##name##Encoder::ns##name##Encoder() {NS_INIT_ISUPPORTS();}\
ns##name##Encoder::~ns##name##Encoder() {}

// All encoders must be first declared and then registered.
DECLARE_ENCODER(Default)
DECLARE_ENCODER(AnyType)
DECLARE_ENCODER(AnySimpleType)
DECLARE_ENCODER(Array)
DECLARE_ENCODER(String)
DECLARE_ENCODER(Boolean)
DECLARE_ENCODER(Double)
DECLARE_ENCODER(Float)
DECLARE_ENCODER(Long)
DECLARE_ENCODER(Int)
DECLARE_ENCODER(Short)
DECLARE_ENCODER(Byte)
DECLARE_ENCODER(UnsignedLong)
DECLARE_ENCODER(UnsignedInt)
DECLARE_ENCODER(UnsignedShort)
DECLARE_ENCODER(UnsignedByte)

#define REGISTER_ENCODER(name) \
{\
  ns##name##Encoder *handler = new ns##name##Encoder();\
  SetEncoder(kSchemaNamespaceURI, k##name##SchemaType, handler); \
  SetEncoder(kSchemaDatatypesNamespaceURI, k##name##SchemaType, handler); \
  SetDecoder(kSchemaNamespaceURI, k##name##SchemaType, handler); \
  SetDecoder(kSchemaDatatypesNamespaceURI, k##name##SchemaType, handler); \
}

nsDefaultSOAPEncoder::nsDefaultSOAPEncoder(): nsSOAPEncoding(nsSOAPUtils::kSOAPEncodingURI, nsnull) 
{
  {
    nsDefaultEncoder *handler = new nsDefaultEncoder();
    SetDefaultEncoder(handler);
    SetDefaultDecoder(handler);
  }
  REGISTER_ENCODER(AnyType)
  REGISTER_ENCODER(AnySimpleType)
  {
    nsArrayEncoder *handler = new nsArrayEncoder();
    SetEncoder(nsSOAPUtils::kSOAPEncodingURI, kArraySOAPType, handler); 
    SetDecoder(nsSOAPUtils::kSOAPEncodingURI, kArraySOAPType, handler); 
  }
  REGISTER_ENCODER(String)
  REGISTER_ENCODER(Boolean)
  REGISTER_ENCODER(Double)
  REGISTER_ENCODER(Float)
  REGISTER_ENCODER(Long)
  REGISTER_ENCODER(Int)
  REGISTER_ENCODER(Short)
  REGISTER_ENCODER(Byte)
  REGISTER_ENCODER(UnsignedLong)
  REGISTER_ENCODER(UnsignedInt)
  REGISTER_ENCODER(UnsignedShort)
  REGISTER_ENCODER(UnsignedByte)
}
//  Here is the implementation of the encoders.
static nsresult EncodeSimpleValue(
		                          const nsAReadableString & aValue, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsIDOMElement* aDestination,
					  nsIDOMElement** _retval)
{
  nsCOMPtr<nsIDOMDocument>document;
  nsresult rc = aDestination->GetOwnerDocument(getter_AddRefs(document));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsIDOMElement>element;
  rc = document->CreateElementNS(aNamespaceURI,
		                 aName, 
				 _retval);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsIDOMNode>ignore;
  rc = aDestination->AppendChild(*_retval, getter_AddRefs(ignore));
  if (NS_FAILED(rc)) return rc;
  if (!aValue.IsEmpty()) {
    nsCOMPtr<nsIDOMText> text;
    rc = document->CreateTextNode(aValue, getter_AddRefs(text));
    if (NS_FAILED(rc)) return rc;
    return (*_retval)->AppendChild(text, getter_AddRefs(ignore));
  }
  return rc;
}

//  Default

NS_IMETHODIMP nsDefaultEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsCOMPtr<nsISOAPEncoder> encoder;
  if (aSchemaType) {
    nsCOMPtr<nsISchemaType> lookupType = aSchemaType;
    do {
      nsAutoString schemaType;
      nsAutoString schemaURI;
      nsresult rc = lookupType->GetName(schemaType);
      if (NS_FAILED(rc)) return rc;
      rc = lookupType->GetTargetNamespace(schemaURI);
      if (NS_FAILED(rc)) return rc;
      rc = aEncoding->GetEncoder(schemaType, schemaURI, getter_AddRefs(encoder));
      if (NS_FAILED(rc)) return rc;
      if (encoder) break;
      PRUint16 typevalue;
      rc = lookupType->GetSchemaType(&typevalue);
      if (NS_FAILED(rc)) return rc;
      if (typevalue == nsISchemaType::SCHEMA_TYPE_COMPLEX) {
        nsCOMPtr<nsISchemaComplexType> oldtype = do_QueryInterface(lookupType);
        oldtype->GetBaseType(getter_AddRefs(lookupType));
      }
      else {
	break;
      }
    } while(lookupType);
  }
  if (!encoder) {
    PRUint16 typevalue;
    if (aSchemaType) {
      nsresult rc = aSchemaType->GetSchemaType(&typevalue);
      if (NS_FAILED(rc)) return rc;
    }
    else {
      typevalue = nsISchemaType::SCHEMA_TYPE_COMPLEX;
    }
    nsAutoString schemaType;
    if (typevalue == nsISchemaType::SCHEMA_TYPE_COMPLEX) {
      schemaType.Assign(kAnyTypeSchemaType);
    }
    else {
      schemaType.Assign(kAnySimpleTypeSchemaType);
    }
    nsresult rc = aEncoding->GetEncoder(schemaType, kSchemaDatatypesNamespaceURI, getter_AddRefs(encoder));
    if (NS_FAILED(rc)) return rc;
  }
  if (encoder) {
    return encoder->Encode(aEncoding, aSource, aNamespaceURI, aName, aSchemaType, aAttachments, aDestination, aReturnValue);
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAnyTypeEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsAutoString nativeSchemaType;
  nsAutoString nativeSchemaURI;
  PRBool mustBeSimple = PR_FALSE;
  PRBool mustBeComplex = PR_FALSE;
  if (aSchemaType) {
    PRUint16 typevalue;
    nsresult rc = aSchemaType->GetSchemaType(&typevalue);
    if (NS_FAILED(rc)) return rc;
    if (typevalue == nsISchemaType::SCHEMA_TYPE_COMPLEX) {
      mustBeComplex = PR_TRUE;
    }
    else {
      mustBeSimple = PR_TRUE;
    }
  }
  PRUint16 typevalue;
  nativeSchemaURI.Assign(kSchemaDatatypesNamespaceURI);
  aSource->GetDataType(&typevalue);
  switch(typevalue) {
    case nsIDataType::VTYPE_INT8:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kByteSchemaType);
      break;
    case nsIDataType::VTYPE_INT16:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kShortSchemaType);
      break;
    case nsIDataType::VTYPE_INT32:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kIntSchemaType);
      break;
    case nsIDataType::VTYPE_INT64:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kLongSchemaType);
      break;
    case nsIDataType::VTYPE_UINT8:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kUnsignedByteSchemaType);
      break;
    case nsIDataType::VTYPE_UINT16:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kUnsignedShortSchemaType);
      break;
    case nsIDataType::VTYPE_UINT32:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kUnsignedIntSchemaType);
      break;
    case nsIDataType::VTYPE_UINT64:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kUnsignedLongSchemaType);
        break;
    case nsIDataType::VTYPE_FLOAT:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kFloatSchemaType);
      break;
    case nsIDataType::VTYPE_DOUBLE:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kDoubleSchemaType);
      break;
    case nsIDataType::VTYPE_BOOL:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kBooleanSchemaType);
      break;
    case nsIDataType::VTYPE_ARRAY:
      if (mustBeSimple) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kArraySOAPType);
      nativeSchemaURI.Assign(nsSOAPUtils::kSOAPEncodingURI);
      break;
#if 0
      {
	nsIID* iid;
	nsCOMPtr<nsISupports> ptr;
	nsresult rv = foo->GetAsInterface(&iid, getter_AddRefs(ptr));
	if (compare_IIDs(iid, NS_GET_IID(nsIPropertyBag)) == 0) {
          schemaType.Assign(kAnyTypeSchemaType);
	  break;
	}
      }
      {
	nsCOMPtr<nsISupports> ptr;
	nsresult rv = foo->GetAsISupports(getter_AddRefs(ptr));
      }
#endif
    case nsIDataType::VTYPE_VOID:
    case nsIDataType::VTYPE_EMPTY:
//  Empty may be either simple or complex.
      break;
    case nsIDataType::VTYPE_INTERFACE_IS:
    case nsIDataType::VTYPE_INTERFACE:
      if (mustBeSimple) return NS_ERROR_ILLEGAL_VALUE;
      break;
    case nsIDataType::VTYPE_ID:
      if (mustBeComplex) return NS_ERROR_ILLEGAL_VALUE;
      nativeSchemaType.Assign(kAnySimpleTypeSchemaType);
      break;
//  case nsIDataType::VTYPE_CHAR_STR:
//  case nsIDataType::VTYPE_WCHAR_STR:
//  case nsIDataType::VTYPE_CHAR:
//  case nsIDataType::VTYPE_WCHAR:
//  case nsIDataType::VTYPE_ASTRING:
    default:
      nativeSchemaType.Assign(kStringSchemaType);
  }
  if (!nativeSchemaType.IsEmpty()) {
    nsCOMPtr<nsISOAPEncoder> encoder;
    nsresult rc = aEncoding->GetEncoder(nativeSchemaType, nativeSchemaURI, getter_AddRefs(encoder));
    if (NS_FAILED(rc)) return rc;
    if (encoder) {
      nsresult rc = encoder->Encode(aEncoding, aSource, aNamespaceURI, aName, aSchemaType, aAttachments, aDestination, aReturnValue);
      if (NS_FAILED(rc)) return rc;
      if (*aReturnValue	//  If we are not schema-controlled, then add a type attribute as a hint about what we did unless unnamed.
        && !aSchemaType
        && !aName.IsEmpty()) {
        nsAutoString type;
        rc = nsSOAPUtils::MakeNamespacePrefixFixed(*aReturnValue, nativeSchemaURI, type);
        if (NS_FAILED(rc)) return rc;
        type.Append(nsSOAPUtils::kQualifiedSeparator);
        type.Append(nativeSchemaType);
        rc = (*aReturnValue)->SetAttributeNS(kSchemaDatatypesNamespaceURI, kSchemaTypeAttribute, type);
      }
      return rc;
    }
  }

//  Implement complex types with property bags here.
  return NS_ERROR_NOT_IMPLEMENTED;
}

//  AnySimpleType

NS_IMETHODIMP nsAnySimpleTypeEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  nsAutoString value;
  rc = aSource->GetAsAString(value);
  if (NS_FAILED(rc)) return rc;
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kAnySimpleTypeSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  Array

NS_IMETHODIMP nsArrayEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  PRUint16 type;
  nsIID iid;
  PRUint32 count;
  void* array;
  nsresult rc = aSource->GetAsArray(&type, &iid, &count, &array); // First, get the array, if any.
  if (NS_FAILED(rc)) return rc;
  if (aName.IsEmpty()) {  //  Now create the element to hold the array
    rc = EncodeSimpleValue(kEmpty,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kArraySOAPType,
		       aDestination,
		       aReturnValue);
  }
  else {
    rc = EncodeSimpleValue(kEmpty,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
  }
  if (NS_FAILED(rc)) return rc;

//  Here, we ought to find the real array type from the schema or the
//  native type of the array, and attach it as the arrayType attribute
//  and use it when encoding the array elements.  Not complete.
  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID, &rc);
  if (NS_FAILED(rc)) return rc;
  switch(type) {
#define DO_SIMPLE_ARRAY(XPType, SOAPType, Format, Source) \
      {\
        nsAutoString value;\
        rc = nsSOAPUtils::MakeNamespacePrefixFixed(*aReturnValue, kSchemaNamespaceURI, value);\
        if (NS_FAILED(rc)) return rc;\
        value.Append(nsSOAPUtils::kQualifiedSeparator);\
        value.Append(k##SOAPType##SchemaType);\
        rc = (*aReturnValue)->SetAttributeNS(nsSOAPUtils::kSOAPEncodingURI, kSOAPArrayTypeAttribute, value);\
        if (NS_FAILED(rc)) return rc;\
	XPType* values = NS_STATIC_CAST(XPType*, array);\
	nsCOMPtr<nsIDOMElement> dummy;\
        for (PRUint32 i = 0; i < count; i++) {\
          char* ptr = PR_smprintf(Format,Source);\
          if (!ptr) return NS_ERROR_OUT_OF_MEMORY;\
          nsAutoString value;\
          value.Assign(NS_ConvertUTF8toUCS2(nsDependentCString(ptr)).get());\
          PR_smprintf_free(ptr);\
          rc = EncodeSimpleValue(value,\
		       nsSOAPUtils::kSOAPEncodingURI,\
		       k##SOAPType##SchemaType,\
		       *aReturnValue,\
		       getter_AddRefs(dummy));\
          if (NS_FAILED(rc)) return rc;\
        }\
	return rc;\
      }
    case nsIDataType::VTYPE_INT8:
      DO_SIMPLE_ARRAY(PRUint8, Byte, "%hd", (PRInt16)(signed char)values[i]);
    case nsIDataType::VTYPE_INT16:
      DO_SIMPLE_ARRAY(PRInt16, Short, "%hd", values[i]);
    case nsIDataType::VTYPE_INT32:
      DO_SIMPLE_ARRAY(PRInt32, Int, "%ld", values[i]);
    case nsIDataType::VTYPE_INT64:
      DO_SIMPLE_ARRAY(PRInt64, Long, "%lld", values[i]);
    case nsIDataType::VTYPE_UINT8:
      DO_SIMPLE_ARRAY(PRUint8, UnsignedByte, "%hu", (PRUint16)values[i]);
    case nsIDataType::VTYPE_UINT16:
      DO_SIMPLE_ARRAY(PRUint16, UnsignedShort, "%hu", values[i]);
    case nsIDataType::VTYPE_UINT32:
      DO_SIMPLE_ARRAY(PRUint32, UnsignedInt, "%lu", values[i]);
    case nsIDataType::VTYPE_UINT64:
      DO_SIMPLE_ARRAY(PRUint64, UnsignedLong, "%llu", values[i]);
    case nsIDataType::VTYPE_FLOAT:
      DO_SIMPLE_ARRAY(float, Float, "%f", values[i]);
    case nsIDataType::VTYPE_DOUBLE:
      DO_SIMPLE_ARRAY(double, Double, "%lf", values[i]);
    case nsIDataType::VTYPE_BOOL:
      DO_SIMPLE_ARRAY(PRBool, Boolean, "%hu", (PRUint16)values[i]);
    case nsIDataType::VTYPE_CHAR_STR:
      DO_SIMPLE_ARRAY(char*, String, "%s", values[i])
    case nsIDataType::VTYPE_WCHAR_STR:
      DO_SIMPLE_ARRAY(PRUnichar*, String, "%s", NS_ConvertUCS2toUTF8(values[i]).get())
    case nsIDataType::VTYPE_CHAR:
      DO_SIMPLE_ARRAY(char, String, "%c", values[i])
    case nsIDataType::VTYPE_ASTRING:
      DO_SIMPLE_ARRAY(nsAString, String, "%s", NS_ConvertUCS2toUTF8(values[i]).get())
//  Don't support these array types just now (needs more work).
    case nsIDataType::VTYPE_WCHAR:
    case nsIDataType::VTYPE_ID:
    case nsIDataType::VTYPE_ARRAY:
    case nsIDataType::VTYPE_VOID:
    case nsIDataType::VTYPE_EMPTY:
    case nsIDataType::VTYPE_INTERFACE_IS:
    case nsIDataType::VTYPE_INTERFACE:
      break;
  }
  return NS_ERROR_ILLEGAL_VALUE;
}

//  String

NS_IMETHODIMP nsStringEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  nsAutoString value;
  rc = aSource->GetAsAString(value);
  if (NS_FAILED(rc)) return rc;
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kStringSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRBool

NS_IMETHODIMP nsBooleanEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRBool b;
  rc = aSource->GetAsBool(&b);
  if (NS_FAILED(rc)) return rc;
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(b ? kTrue : kFalse,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kBooleanSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(b ? kTrue : kFalse,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  Double

NS_IMETHODIMP nsDoubleEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  double f;
  rc = aSource->GetAsDouble(&f);//  Check that double works.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%lf",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kDoubleSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  Float

NS_IMETHODIMP nsFloatEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  float f;
  rc = aSource->GetAsFloat(&f);//  Check that float works.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%f",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kFloatSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRInt64

NS_IMETHODIMP nsLongEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRInt64 f;
  rc = aSource->GetAsInt64(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%lld",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kLongSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRInt32

NS_IMETHODIMP nsIntEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRInt32 f;
  rc = aSource->GetAsInt32(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%d",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kIntSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRInt16

NS_IMETHODIMP nsShortEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRInt16 f;
  rc = aSource->GetAsInt16(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%d",(PRInt32)f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kShortSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  Byte

NS_IMETHODIMP nsByteEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint8 f;
  rc = aSource->GetAsInt8(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%d",(PRInt32)(signed char)f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kByteSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRUint64

NS_IMETHODIMP nsUnsignedLongEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint64 f;
  rc = aSource->GetAsUint64(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%llu",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kLongSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRUint32

NS_IMETHODIMP nsUnsignedIntEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint32 f;
  rc = aSource->GetAsUint32(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%u",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kIntSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRUint16

NS_IMETHODIMP nsUnsignedShortEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint16 f;
  rc = aSource->GetAsUint16(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%u",(PRUint32)f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kShortSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  Unsigned Byte

NS_IMETHODIMP nsUnsignedByteEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint8 f;
  rc = aSource->GetAsUint8(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%u",(PRUint32)f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kByteSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

NS_IMETHODIMP nsDefaultEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsCOMPtr<nsISOAPDecoder> decoder;
  nsCOMPtr<nsISchemaType> type = aSchemaType;
  if (!type) {	//  Where no type has been specified, look one up from schema attribute, if it exists
    nsAutoString explicittype;
    nsresult rc = aSource->GetAttributeNS(kSchemaDatatypesNamespaceURI, kSchemaTypeAttribute, explicittype);
    if (NS_FAILED(rc)) return rc;
    nsCOMPtr<nsISchemaCollection> collection;
    rc = aEncoding->GetSchemaCollection(getter_AddRefs(collection));
    if (NS_FAILED(rc)) return rc;
    nsCOMPtr<nsISchemaElement> element;
    nsAutoString ns;
    nsAutoString name;
    rc = nsSOAPUtils::GetNamespaceURI(aSource, explicittype, ns);
    if (NS_FAILED(rc)) return rc;
    rc = nsSOAPUtils::GetLocalName(explicittype, name);
    if (NS_FAILED(rc)) return rc;
    rc = collection->GetElement(ns, name, getter_AddRefs(element));
    if (NS_FAILED(rc)) return rc;
    if (element) {
      rc = element->GetType(getter_AddRefs(type));
      if (NS_FAILED(rc)) return rc;
    }
  }
  if (type) {
    nsCOMPtr<nsISchemaType> lookupType = type;
    do {
      nsAutoString schemaType;
      nsAutoString schemaURI;
      nsresult rc = lookupType->GetName(schemaType);
      if (NS_FAILED(rc)) return rc;
      rc = lookupType->GetTargetNamespace(schemaURI);
      if (NS_FAILED(rc)) return rc;
      rc = aEncoding->GetDecoder(schemaType, schemaURI, getter_AddRefs(decoder));
      if (NS_FAILED(rc)) return rc;
      if (decoder) break;
      PRUint16 typevalue;
      rc = lookupType->GetSchemaType(&typevalue);
      if (NS_FAILED(rc)) return rc;
      if (typevalue == nsISchemaType::SCHEMA_TYPE_COMPLEX) {
        nsCOMPtr<nsISchemaComplexType> oldtype = do_QueryInterface(lookupType);
        oldtype->GetBaseType(getter_AddRefs(lookupType));
      }
      else {
	break;
      }
    } while(lookupType);
  }
  if (!decoder) {
    PRUint16 typevalue;
    if (type) {
      nsresult rc = type->GetSchemaType(&typevalue);
      if (NS_FAILED(rc)) return rc;
    }
    else {
      typevalue = nsISchemaType::SCHEMA_TYPE_COMPLEX;
    }
    nsAutoString schemaType;
    if (typevalue == nsISchemaType::SCHEMA_TYPE_COMPLEX) {
      schemaType.Assign(kAnyTypeSchemaType);
    }
    else {
      schemaType.Assign(kAnySimpleTypeSchemaType);
    }
    nsresult rc = aEncoding->GetDecoder(schemaType, kSchemaDatatypesNamespaceURI, getter_AddRefs(decoder));
    if (NS_FAILED(rc)) return rc;
  }
  if (decoder) {
    return decoder->Decode(aEncoding, aSource, type, aAttachments, _retval);
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAnyTypeEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  PRBool mustBeSimple = PR_FALSE;
  PRBool mustBeComplex = PR_FALSE;
  if (aSchemaType) {
    PRUint16 typevalue;
    nsresult rc = aSchemaType->GetSchemaType(&typevalue);
    if (NS_FAILED(rc)) return rc;
    if (typevalue == nsISchemaType::SCHEMA_TYPE_COMPLEX) {
      mustBeComplex = PR_TRUE;
    }
    else {
      mustBeSimple = PR_TRUE;
    }
  }
  if (!mustBeComplex) {
    nsAutoString value;
    nsresult rc= nsSOAPUtils::GetElementTextContent(aSource, value);
    if (rc == NS_ERROR_ILLEGAL_VALUE
      && !mustBeSimple) {
      mustBeComplex = PR_TRUE;
    }
    else if (NS_FAILED(rc)) return rc;
    else {
//  Here we have a simple value which has no decoder.  Make it a string.
      nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID, &rc);
      if (NS_FAILED(rc)) return rc;
      rc = p->SetAsAString(value);
      if (NS_FAILED(rc)) return rc;
      *_retval = p;
      NS_ADDREF(*_retval);
      return NS_OK;
    }
  }
//  Here we have a complex value, hopefully a property bag.  Implement it later.
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAnySimpleTypeEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID, &rc);
  if (NS_FAILED(rc)) return rc;
  rc = p->SetAsAString(value);
  if (NS_FAILED(rc)) return rc;
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

//  Incomplete -- becomes very complex due to variant arrays

NS_IMETHODIMP nsArrayEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString ns;
  nsAutoString name;
  nsCOMPtr<nsISchemaType>subtype;
  nsAutoString value;
  nsresult rc = aSource->GetAttributeNS(nsSOAPUtils::kSOAPEncodingURI, kSOAPArrayTypeAttribute, value);
  if (!value.IsEmpty()) {  //  Need to truncate []
    nsCOMPtr<nsISchemaCollection> collection;
    rc = aEncoding->GetSchemaCollection(getter_AddRefs(collection));
    nsCOMPtr<nsISchemaElement> element;
    rc = nsSOAPUtils::GetNamespaceURI(aSource, value, ns);
    if (NS_FAILED(rc)) return rc;
    rc = nsSOAPUtils::GetLocalName(value, name);
    if (NS_FAILED(rc)) return rc;
    rc = collection->GetElement(ns, name, getter_AddRefs(element));
    if (NS_FAILED(rc)) return rc;
    if (element) {
      rc = element->GetType(getter_AddRefs(subtype));
      if (NS_FAILED(rc)) return rc;
    }
  }
  if (ns.Equals(kSchemaNamespaceURI)
    || ns.Equals(kSchemaDatatypesNamespaceURI)) {
    if (name.Equals(kStringSchemaType)) {
    }
    else if (name.Equals(kBooleanSchemaType)) {
    }
    else if (name.Equals(kFloatSchemaType)) {
    }
    else if (name.Equals(kDoubleSchemaType)) {
    }
    else if (name.Equals(kLongSchemaType)) {
    }
    else if (name.Equals(kIntSchemaType)) {
    }
    else if (name.Equals(kShortSchemaType)) {
    }
    else if (name.Equals(kByteSchemaType)) {
    }
    else if (name.Equals(kUnsignedLongSchemaType)) {
    }
    else if (name.Equals(kUnsignedIntSchemaType)) {
    }
    else if (name.Equals(kUnsignedShortSchemaType)) {
    }
    else if (name.Equals(kUnsignedByteSchemaType)) {
    }
    else return NS_ERROR_ILLEGAL_VALUE;
  }
  else if (ns.Equals(nsSOAPUtils::kSOAPEncodingURI))
  {
    if (name.Equals(kArraySOAPType)) {
      return NS_ERROR_ILLEGAL_VALUE;  //  Fix nested arrays later
    }
    else return NS_ERROR_ILLEGAL_VALUE;
  }
  else return NS_ERROR_ILLEGAL_VALUE;
  return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP nsStringEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID, &rc);
  if (NS_FAILED(rc)) return rc;
  rc = p->SetAsAString(value);
  if (NS_FAILED(rc)) return rc;
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsBooleanEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  bool b;
  if (value.Equals(kTrue)
    || value.Equals(kTrueA)) {
    b = PR_TRUE;
  } else if (value.Equals(kFalse)
    || value.Equals(kFalseA)) {
    b = PR_FALSE;
  } else return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsBool(b);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsDoubleEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  double f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %lf %n",&f,&n);
  if (r == 0 || n < value.Length()) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsDouble(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsFloatEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  float f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %f %n",&f,&n);
  if (r == 0 || n < value.Length()) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsFloat(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsLongEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRInt64 f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %lld %n",&f,&n);
  if (r == 0 || n < value.Length()) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsInt64(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsIntEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRInt32 f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %ld %n",&f,&n);
  if (r == 0 || n < value.Length()) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsInt32(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsShortEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRInt16 f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %hd %n",&f,&n);
  if (r == 0 || n < value.Length()) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsInt16(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsByteEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRInt16 f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %hd %n",&f,&n);
  if (r == 0 || n < value.Length() || f < -128 || f > 127) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsInt8((PRUint8)f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsUnsignedLongEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRUint64 f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %llu %n",&f,&n);
  if (r == 0 || n < value.Length()) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsUint64(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsUnsignedIntEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRUint32 f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %lu %n",&f,&n);
  if (r == 0 || n < value.Length()) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsUint32(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsUnsignedShortEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRUint16 f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %hu %n",&f,&n);
  if (r == 0 || n < value.Length()) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsUint16(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsUnsignedByteEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRUint16 f;
  unsigned int n;
  int r = PR_sscanf(NS_ConvertUCS2toUTF8(value).get(), " %hu %n",&f,&n);
  if (r == 0 || n < value.Length() || f > 255) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsUint8((PRUint8)f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}
