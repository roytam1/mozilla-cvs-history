
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

#include <ctype.h>
#include <string.h> 
#include <stdlib.h>
#include "nsString.h"
#include "nsDebug.h"
#include "nsDeque.h"

#ifndef RICKG_TESTBED
#include "prdtoa.h"
#include "nsISizeOfHandler.h"
#endif 


static const char* kPossibleNull = "Error: possible unintended null in string";
static const char* kNullPointerError = "Error: unexpected null ptr";
static const char* kWhitespace="\b\t\r\n ";


static void Subsume(nsStr& aDest,nsStr& aSource){
  if(aSource.mStr && aSource.mLength) {
    if(aSource.mOwnsBuffer){
      nsStr::Destroy(aDest);
      aDest.mStr=aSource.mStr;
      aDest.mLength=aSource.mLength;
      aDest.mCharSize=aSource.mCharSize;
      aDest.mCapacity=aSource.mCapacity;
      aDest.mOwnsBuffer=aSource.mOwnsBuffer;
      aSource.mOwnsBuffer=PR_FALSE;
      aSource.mStr=0;
    }
    else{
      nsStr::StrAssign(aDest,aSource,0,aSource.mLength);
    }
  } 
  else nsStr::Truncate(aDest,0);
}


/**
 * Default constructor. 
 */
nsString::nsString() {
  Initialize(*this,eTwoByte);
}

#if 0
/**
 * This constructor accepts an ascii string
 * @update  gess 1/4/99
 * @param   aCString is a ptr to a 1-byte cstr
 * @param   aLength tells us how many chars to copy from given CString
 */
nsString::nsString(const char* aCString,PRInt32 aCount){  
  Initialize(*this,eTwoByte);
  AssignWithConversion(aCString,aCount);
}
#endif

#ifdef NEW_STRING_APIS
nsString::nsString(const PRUnichar* aString) {
  Initialize(*this,eTwoByte);
  Assign(aString);
}
#endif

/**
 * This constructor accepts a unicode string
 * @update  gess 1/4/99
 * @param   aString is a ptr to a unichar string
 * @param   aLength tells us how many chars to copy from given aString
 */
nsString::nsString(const PRUnichar* aString,PRInt32 aCount) {  
  Initialize(*this,eTwoByte);
  Assign(aString,aCount);
}

#if 0
/**
 * This constructor works for all other nsSTr derivatives
 * @update  gess 1/4/99
 * @param   reference to another nsCString
 */
nsString::nsString(const nsStr &aString)  {
  Initialize(*this,eTwoByte);
  StrAssign(*this,aString,0,aString.mLength);
}
#endif

/**
 * This is our copy constructor 
 * @update  gess 1/4/99
 * @param   reference to another nsString
 */
nsString::nsString(const nsString& aString) {
  Initialize(*this,eTwoByte);
  StrAssign(*this,aString,0,aString.mLength);
}

/**
 * construct from subsumeable string
 * @update  gess 1/4/99
 * @param   reference to a subsumeString
 */
#if defined(AIX) || defined(XP_OS2_VACPP)
nsString::nsString(const nsSubsumeStr& aSubsumeStr)  {
  Initialize(*this,eTwoByte);

  nsSubsumeStr temp(aSubsumeStr);  // a temp is needed for the AIX and VAC++ compilers
  Subsume(*this,temp);
#else
nsString::nsString(nsSubsumeStr& aSubsumeStr)  {
  Initialize(*this,eTwoByte);

  Subsume(*this,aSubsumeStr);
#endif /* AIX  || XP_OS2_VACPP */
}

/**
 * Destructor
 * Make sure we call nsStr::Destroy.
 */
nsString::~nsString() {
  nsStr::Destroy(*this);
}

#ifdef NEW_STRING_APIS
const PRUnichar* nsString::GetReadableFragment( nsReadableFragment<PRUnichar>& aFragment, nsFragmentRequest aRequest, PRUint32 aOffset ) const {
  switch ( aRequest ) {
    case kFirstFragment:
    case kLastFragment:
    case kFragmentAt:
      aFragment.mEnd = (aFragment.mStart = mUStr) + mLength;
      return aFragment.mStart + aOffset;
    
    case kPrevFragment:
    case kNextFragment:
    default:
      return 0;
  }
}

PRUnichar* nsString::GetWritableFragment( nsWritableFragment<PRUnichar>& aFragment, nsFragmentRequest aRequest, PRUint32 aOffset ) {
  switch ( aRequest ) {
    case kFirstFragment:
    case kLastFragment:
    case kFragmentAt:
      aFragment.mEnd = (aFragment.mStart = mUStr) + mLength;
      return aFragment.mStart + aOffset;
    
    case kPrevFragment:
    case kNextFragment:
    default:
      return 0;
  }
}

nsString::nsString( const nsAReadableString& aReadable ) {
  Initialize(*this,eTwoByte);
  Assign(aReadable);
}
#endif

void nsString::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const {
  if (aResult) {
    *aResult = sizeof(*this) + mCapacity * mCharSize;
  }
}

/**
 * This method truncates this string to given length.
 *
 * @update  gess 01/04/99
 * @param   anIndex -- new length of string
 * @return  nada
 */
void nsString::SetLength(PRUint32 anIndex) {
  if ( anIndex > mCapacity )
    SetCapacity(anIndex);
  nsStr::Truncate(*this,anIndex);
}


/**
 * Call this method if you want to force the string to a certain capacity
 * @update  gess 1/4/99
 * @param   aLength -- contains new length for mStr
 * @return
 */
void nsString::SetCapacity(PRUint32 aLength) {
  if(aLength) {
    if(aLength>mCapacity) {
      GrowCapacity(*this,aLength);
    }
    AddNullTerminator(*this);
  }
}

/**********************************************************************
  Accessor methods...
 *********************************************************************/


//static char gChar=0;
/** 
 * 
 * @update  gess1/4/99
 * @return  ptr to internal buffer (if 1-byte), otherwise NULL
 */
const char* nsString::GetBuffer(void) const {
  const char* result=(eOneByte==mCharSize) ? mStr : 0;
  return result;
}


/**
 * This method returns the internal unicode buffer. 
 * Now that we've factored the string class, this should never
 * be able to return a 1 byte string.
 *
 * @update  gess1/4/99
 * @return  ptr to internal (2-byte) buffer;
 */
const PRUnichar* nsString::GetUnicode(void)  const {
  const PRUnichar* result=(eOneByte==mCharSize) ? 0 : mUStr;
  return result;
}

#ifndef NEW_STRING_APIS
/**
 * Get nth character.
 */
PRUnichar nsString::operator[](PRUint32 anIndex) const {
  return GetCharAt(*this,anIndex);
}

/**
 * Get nth character.
 */
PRUnichar nsString::CharAt(PRUint32 anIndex) const {
  return GetCharAt(*this,anIndex);
}

/**
 * Get 1st character.
 */
PRUnichar nsString::First(void) const{
  return GetCharAt(*this,0);
}

/**
 * Get last character.
 */
PRUnichar nsString::Last(void) const{
  return GetCharAt(*this,mLength-1);
}
#endif // !defined(NEW_STRING_APIS)

/**
 * set a char inside this string at given index
 * @param aChar is the char you want to write into this string
 * @param anIndex is the ofs where you want to write the given char
 * @return TRUE if successful
 */
PRBool nsString::SetCharAt(PRUnichar aChar,PRUint32 anIndex){
  PRBool result=PR_FALSE;
  if(anIndex<mLength){
    if(eOneByte==mCharSize)
      mStr[anIndex]=char(aChar); 
    else mUStr[anIndex]=aChar;

// SOON!   if(0==aChar) mLength=anIndex;

    result=PR_TRUE;
  }
  return result;
}


/*********************************************************
  append (operator+) METHODS....
 *********************************************************/

#ifndef NEW_STRING_APIS
/**
 * Create a new string by appending given string to this
 * @update  gess 01/04/99
 * @param   aString -- 2nd string to be appended
 * @return  new subsumeable string (this makes things faster by reducing copying)
 */
nsSubsumeStr nsString::operator+(const nsStr& aString){
  nsString temp(*this); //make a temp string the same size as this...
  nsStr::StrAppend(temp,aString,0,aString.mLength);
  return nsSubsumeStr(temp);
}

/**
 * Create a new string by appending given string to this
 * @update  gess 01/04/99
 * @param   aString -- 2nd string to be appended
 * @return  new subsumeable string (this makes things faster by reducing copying)
 */
nsSubsumeStr nsString::operator+(const nsString& aString){
  nsString temp(*this); //make a temp string the same size as this...
  nsStr::StrAppend(temp,aString,0,aString.mLength);
  return nsSubsumeStr(temp);
}

/**
 * create a new string by adding this to the given buffer.
 * @update  gess 01/04/99
 * @param   aCString is a ptr to cstring to be added to this
 * @return  new subsumeable string (this makes things faster by reducing copying)
 */
nsSubsumeStr nsString::operator+(const char* aCString) {
  nsString temp(*this);
  temp.AppendWithConversion(aCString); 
  return nsSubsumeStr(temp);
}


/**
 * create a new string by adding this to the given char.
 * @update  gess 01/04/99
 * @param   aChar is a char to be added to this
 * @return  new subsumeable string (this makes things faster by reducing copying)
 */
nsSubsumeStr nsString::operator+(char aChar) {
  nsString temp(*this);
  temp.Append(char(aChar));
  return nsSubsumeStr(temp);
}

/**
 * create a new string by adding this to the given buffer.
 * @update  gess 01/04/99
 * @param   aString is a ptr to unistring to be added to this
 * @return  new subsumeable string (this makes things faster by reducing copying)
 */
nsSubsumeStr nsString::operator+(const PRUnichar* aString) {
  nsString temp(*this);
  temp.Append(aString);
  return nsSubsumeStr(temp);
}


/**
 * create a new string by adding this to the given char.
 * @update  gess 01/04/99
 * @param   aChar is a unichar to be added to this
 * @return  new subsumeable string (this makes things faster by reducing copying)
 */
nsSubsumeStr nsString::operator+(PRUnichar aChar) {
  nsString temp(*this);
  temp.Append(char(aChar));
  return nsSubsumeStr(temp);
}
#endif // !defined(NEW_STRING_APIS)

/**********************************************************************
  Lexomorphic transforms...
 *********************************************************************/

/**
 * Converts all chars in internal string to lower
 * @update  gess 01/04/99
 */
void nsString::ToLowerCase() {
  nsStr::ChangeCase(*this,PR_FALSE);
}

/**
 * Converts all chars in internal string to upper
 * @update  gess 01/04/99
 */
void nsString::ToUpperCase() {
  nsStr::ChangeCase(*this,PR_TRUE);
}

/**
 * Converts chars in this to uppercase, and
 * stores them in aString
 * @update  gess 01/04/99
 * @param   aOut is a string to contain result
 */
void nsString::ToLowerCase(nsString& aString) const {
  aString=*this;
  nsStr::ChangeCase(aString,PR_FALSE);
}

/**
 * Converts chars in this to uppercase, and
 * stores them in a given output string
 * @update  gess 01/04/99
 * @param   aOut is a string to contain result
 */
void nsString::ToUpperCase(nsString& aString) const {
  aString=*this;
  nsStr::ChangeCase(aString,PR_TRUE);
}

/**
 *  This method is used to remove all occurances of the
 *  characters found in aSet from this string.
 *  
 *  @update rickg 03.23.2000
 *  @param  aChar -- char to be stripped
 *  @param  anOffset -- where in this string to start stripping chars
 *  @return *this 
 */
void
nsString::StripChar(PRUnichar aChar,PRInt32 anOffset){
  if(mLength && (anOffset<PRInt32(mLength))) {
    if(eOneByte==mCharSize) {
      char*  to   = mStr + anOffset;
      char*  from = mStr + anOffset;
      char*  end  = mStr + mLength;

      while (from < end) {
        char theChar = *from++;
        if(aChar!=theChar) {
          *to++ = theChar;
        }
      }
      *to = 0; //add the null
      mLength=to - mStr;
    }
    else {
      PRUnichar*  to   = mUStr + anOffset;
      PRUnichar*  from = mUStr + anOffset;
      PRUnichar*  end  = mUStr + mLength;

      while (from < end) {
        PRUnichar theChar = *from++;
        if(aChar!=theChar) {
          *to++ = theChar;
        }
      }
      *to = 0; //add the null
      mLength=to - mUStr;
    }
  }
}

/**
 *  This method is used to remove all occurances of the
 *  characters found in aSet from this string.
 *  
 *  @update gess 01/04/99
 *  @param  aSet -- characters to be cut from this
 *  @return *this 
 */
void
nsString::StripChars(const char* aSet){
  nsStr::StripChars(*this,aSet);
}


/**
 *  This method strips whitespace throughout the string
 *  
 *  @update  gess 01/04/99
 *  @return  this
 */
void
nsString::StripWhitespace() {
  StripChars(kWhitespace);
}

/**
 *  This method is used to replace all occurances of the
 *  given source char with the given dest char
 *  
 *  @param  
 *  @return *this 
 */
void
nsString::ReplaceChar(PRUnichar aSourceChar, PRUnichar aDestChar) {
  PRUint32 theIndex=0;
  if(eTwoByte==mCharSize){
    for(theIndex=0;theIndex<mLength;theIndex++){
      if(mUStr[theIndex]==aSourceChar) {
        mUStr[theIndex]=aDestChar;
      }//if
    }
  }
  else{
    for(theIndex=0;theIndex<mLength;theIndex++){
      if(mStr[theIndex]==(char)aSourceChar) {
        mStr[theIndex]=(char)aDestChar;
      }//if
    }
  }
}

/**
 *  This method is used to replace all occurances of the
 *  given source char with the given dest char
 *  
 *  @param  
 *  @return *this 
 */
void
nsString::ReplaceChar(const char* aSet, PRUnichar aNewChar){
  if(aSet){
    PRInt32 theIndex=FindCharInSet(aSet,0);
    while(kNotFound<theIndex) {
      if(eTwoByte==mCharSize) 
        mUStr[theIndex]=aNewChar;
      else mStr[theIndex]=(char)aNewChar;
      theIndex=FindCharInSet(aSet,theIndex+1);
    }
  }
}

/**
 *  This method is used to replace all occurances of the
 *  given target with the given replacement
 *  
 *  @param  
 *  @return *this 
 */
void
nsString::ReplaceSubstring(const PRUnichar* aTarget,const PRUnichar* aNewValue){
  if(aTarget && aNewValue) {

    PRInt32 len=nsCRT::strlen(aTarget);
    if(0<len) {
      CBufDescriptor theDesc1(aTarget,PR_TRUE, len+1,len);
      nsAutoString theTarget(theDesc1);

      len=nsCRT::strlen(aNewValue);
      if(0<len) {
        CBufDescriptor theDesc2(aNewValue,PR_TRUE, len+1,len);
        nsAutoString theNewValue(theDesc2);

        ReplaceSubstring(theTarget,theNewValue);
      }
    }
  }
}

/**
 *  This method is used to replace all occurances of the
 *  given target substring with the given replacement substring
 *  
 *  @param aTarget
 *  @param aNewValue
 *  @return *this 
 */
void
nsString::ReplaceSubstring(const nsString& aTarget,const nsString& aNewValue){


  //WARNING: This is not working yet!!!!!

  if(aTarget.mLength && aNewValue.mLength) {
    PRBool isSameLen=(aTarget.mLength==aNewValue.mLength);

    if((isSameLen) && (1==aNewValue.mLength)) {
      ReplaceChar(aTarget.CharAt(0),aNewValue.CharAt(0));
    }
    else {
      PRInt32 theIndex=0;
      while(kNotFound!=(theIndex=nsStr::FindSubstr(*this,aTarget,PR_FALSE,theIndex,mLength))) {
        if(aNewValue.mLength<aTarget.mLength) {
          //Since target is longer than newValue, we should delete a few chars first, then overwrite.
          PRInt32 theDelLen=aTarget.mLength-aNewValue.mLength;
          nsStr::Delete(*this,theIndex,theDelLen);
        }
        else {
          //this is the worst case: the newvalue is larger than the substr it's replacing
          //so we have to insert some characters...
          PRInt32 theInsLen=aNewValue.mLength-aTarget.mLength;
          StrInsert(*this,theIndex,aNewValue,0,theInsLen);
        }
        nsStr::Overwrite(*this,aNewValue,theIndex);
      }
    }
  }
}

/**
 *  This method is used to replace all occurances of the
 *  given source char with the given dest char
 *  
 *  @param  
 *  @return *this 
 */
PRInt32 nsString::CountChar(PRUnichar aChar) {
  PRInt32 theIndex=0;
  PRInt32 theCount=0;
  PRInt32 theLen=(PRInt32)mLength;
  for(theIndex=0;theIndex<theLen;theIndex++){
    PRUnichar theChar=GetCharAt(*this,theIndex);
    if(theChar==aChar)
      theCount++;
  }
  return theCount;
}

/**
 *  This method trims characters found in aTrimSet from
 *  either end of the underlying string.
 *  
 *  @update  gess 3/31/98
 *  @param   aTrimSet -- contains chars to be trimmed from
 *           both ends
 *  @return  this
 */
void
nsString::Trim(const char* aTrimSet, PRBool aEliminateLeading,PRBool aEliminateTrailing,PRBool aIgnoreQuotes){

  if(aTrimSet){
    
    PRUnichar theFirstChar=0;
    PRUnichar theLastChar=0;
    PRBool    theQuotesAreNeeded=PR_FALSE;

    if(aIgnoreQuotes && (mLength>2)) {
      theFirstChar=First();    
      theLastChar=Last();
      if(theFirstChar==theLastChar) {
        if(('\''==theFirstChar) || ('"'==theFirstChar)) {
          Cut(0,1);
          Truncate(mLength-1);
          theQuotesAreNeeded=PR_TRUE;
        }
        else theFirstChar=0;
      }
    }
    
    nsStr::Trim(*this,aTrimSet,aEliminateLeading,aEliminateTrailing);

    if(aIgnoreQuotes && theQuotesAreNeeded) {
      Insert(theFirstChar,0);
      Append(theLastChar);
    }

  }
}

/**
 *  This method strips chars in given set from string.
 *  You can control whether chars are yanked from
 *  start and end of string as well.
 *  
 *  @update  gess 3/31/98
 *  @param   aEliminateLeading controls stripping of leading ws
 *  @param   aEliminateTrailing controls stripping of trailing ws
 *  @return  this
 */
void
nsString::CompressSet(const char* aSet, PRUnichar aChar,PRBool aEliminateLeading,PRBool aEliminateTrailing){
  if(aSet){
    ReplaceChar(aSet,aChar);
    nsStr::CompressSet(*this,aSet,aEliminateLeading,aEliminateTrailing);
  }
}

/**
 *  This method strips whitespace from string.
 *  You can control whether whitespace is yanked from
 *  start and end of string as well.
 *  
 *  @update  gess 3/31/98
 *  @param   aEliminateLeading controls stripping of leading ws
 *  @param   aEliminateTrailing controls stripping of trailing ws
 *  @return  this
 */
void
nsString::CompressWhitespace( PRBool aEliminateLeading,PRBool aEliminateTrailing){
  CompressSet(kWhitespace,' ',aEliminateLeading,aEliminateTrailing);
}

/**********************************************************************
  string conversion methods...
 *********************************************************************/

/**
 * Creates a duplicate clone (ptr) of this string.
 * @update  gess 01/04/99
 * @return  ptr to clone of this string
 */
nsString* nsString::ToNewString() const {
  return new nsString(*this);
}

/**
 * Creates an ascii clone of this string
 * Note that calls to this method should be matched with calls to Recycle().
 * @update  gess 02/24/00
 * @WARNING! Potential i18n issue here, since we're stepping down from 2byte chars to 1byte chars!
 * @return  ptr to new ascii string
 */
char* nsString::ToNewCString() const {

  nsCString temp;
  temp.AssignWithConversion(GetUnicode(), Length());  //construct nsCString with alloc on heap (which we'll steal in a moment)
  temp.SetCapacity(8);    //force it to have an allocated buffer, even if this is empty.
  char* result=temp.mStr; //steal temp's buffer
  temp.mStr=0;            //clear temp's buffer to prevent deallocation
  return result;          //and return the char*
}

/**
 * Creates an UTF8 clone of this string
 * Note that calls to this method should be matched with calls to Recycle().
 * @update  ftang 09/10/99
 * @return  ptr to new UTF8 string
 * http://www.cis.ohio-state.edu/htbin/rfc/rfc2279.html
 */
char* nsString::ToNewUTF8String() const {
  nsCString temp("");
  temp.SetCapacity(8); //ensure that we get an allocated buffer instead of the common empty one.

  // Caculate how many bytes we need
  PRUnichar* p;
  PRInt32 utf8len;
  for(p = this->mUStr, utf8len=0; 0 != (*p);p++)
  {
     if(0x0000 == ((*p) & 0x007F))
        utf8len += 1; // 0000 0000 - 0000 007F
     else if(0x0000 == ((*p) & 0x07FF))
        utf8len += 2; // 0000 0080 - 0000 07FF
     else 
        utf8len += 3; // 0000 0800 - 0000 FFFF
     // Note: Surrogate pair need 4 bytes, but in this caculation
     // we count as 6 bytes. It will wast 2 bytes per surrogate pair
  }

  if((utf8len+1) > 8)
     temp.SetCapacity(utf8len+1); 

  char* result=temp.mStr;
  char* out = result;
  PRUint32 ucs4=0;

  for(p = this->mUStr, utf8len=0; 0 != (*p);p++)
  {
     if(0 == ucs4) {
       if(0x0000 == ((*p) & 0xFF80)) {
          *out++ = (char)*p;
       } else if(0x0000 == ((*p) & 0xF800)) {
          *out++ = 0xC0 | (char)((*p) >> 6);
          *out++ = 0x80 | (char)(0x003F & (*p));
       } else {
          if( 0xD800 == ( 0xFC00 & (*p))) 
          { // D800- DBFF - High Surrogate 
            // N = (H- D800) *400 + 10000 + ...
            ucs4 = 0x10000 | ((0x03FF & (*p)) << 10);
          } else if( 0xDC00 == ( 0xFC00 & (*p))) { 
            // DC00- DFFF - Low Surrogate 
            // error here. We should hit High Surrogate first
            // Do not output any thing in this case
          } else {
            *out++ = 0xE0 | (char)((*p) >> 12);
            *out++ = 0x80 | (char)(0x003F & (*p >> 6));
            *out++ = 0x80 | (char)(0x003F & (*p) );
          }
       }
     } else {
       if( 0xDC00 == (0xFC00 & (*p))) { 
         // DC00- DFFF - Low Surrogate 
         // N += ( L - DC00 )  
         ucs4 |= (0x03FF & (*p));
         // 0001 0000-001F FFFF
         *out++ = 0xF0 | (char)(ucs4 >> 18);
         *out++ = 0x80 | (char)(0x003F & (ucs4 >> 12));
         *out++ = 0x80 | (char)(0x003F & (ucs4 >> 6));
         *out++ = 0x80 | (char)(0x003F & ucs4) ;
       } else {
         // Got a High Surrogate but no low surrogate
         // output nothing.
       }
       ucs4 = 0;
     }
  }
  *out = '\0'; // null terminate
  temp.mStr=0;
  temp.mOwnsBuffer=PR_FALSE;
  
  return result;
}

/**
 * Creates an ascii clone of this string
 * Note that calls to this method should be matched with calls to Recycle().
 * @update  gess 02/24/00
 * @return  ptr to new ascii string
 */
PRUnichar* nsString::ToNewUnicode() const {

  nsString temp(*this);         //construct nsString with alloc on heap (which we'll steal in a moment)
  temp.SetCapacity(8);          //force it to have an allocated buffer, even if this is empty.
  PRUnichar* result=temp.mUStr; //steal temp's buffer
  temp.mStr=0;                  //clear temp's buffer to prevent deallocation
  temp.mOwnsBuffer=PR_FALSE;
  return result;                //and return the PRUnichar* to the caller
}

/**
 * Copies contents of this string into he given buffer
 * Note that if you provide me a buffer that is smaller than the length of
 * this string, only the number of bytes that will fit are copied. 
 *
 * @update  gess 01/04/99
 * @param   aBuf
 * @param   aBufLength -- size of your external buffer (including null)
 * @param   anOffset -- THIS IS NOT USED AT THIS TIME!
 * @return
 */
char* nsString::ToCString(char* aBuf, PRUint32 aBufLength,PRUint32 anOffset) const{
  if(aBuf) {

    // NS_ASSERTION(mLength<=aBufLength,"buffer smaller than string");

    CBufDescriptor theDescr(aBuf,PR_TRUE,aBufLength,0);
    nsCAutoString temp(theDescr);
    nsStr::StrAssign(temp, *this, anOffset, aBufLength-1);
    temp.mStr=0;
  }
  return aBuf;
}

/**
 * Perform string to float conversion.
 * @update  gess 01/04/99
 * @param   aErrorCode will contain error if one occurs
 * @return  float rep of string value
 */
float nsString::ToFloat(PRInt32* aErrorCode) const {
  char buf[100];
  if (mLength > PRInt32(sizeof(buf)-1)) {
    *aErrorCode = (PRInt32) NS_ERROR_ILLEGAL_VALUE;
    return 0.0f;
  }
  char* cp = ToCString(buf, sizeof(buf));
  float f = (float) PR_strtod(cp, &cp);
  if (*cp != 0) {
    *aErrorCode = (PRInt32) NS_ERROR_ILLEGAL_VALUE;
  }
  *aErrorCode = (PRInt32) NS_OK;
  return f;
}


/**
 * Perform decimal numeric string to int conversion.
 * NOTE: In this version, we use the radix you give, even if it's wrong.
 * @update  gess 02/14/00
 * @param   aErrorCode will contain error if one occurs
 * @param   aRadix tells us what base to expect the given string in. kAutoDetect tells us to determine the radix.
 * @return  int rep of string value
 */
PRInt32 nsString::ToInteger(PRInt32* anErrorCode,PRUint32 aRadix) const {
  PRUnichar*  cp=mUStr;
  PRInt32     theRadix = (kAutoDetect==aRadix) ? 10 : aRadix;
  PRInt32     result=0;
  PRBool      negate=PR_FALSE;
  PRUnichar   theChar=0;

  *anErrorCode=NS_ERROR_ILLEGAL_VALUE;
  
  if(cp) {
  
    //begin by skipping over leading chars that shouldn't be part of the number...
    
    PRUnichar*  endcp=cp+mLength;
    PRBool      done=PR_FALSE;
    
    while((cp<endcp) && (!done)){
      theChar=*cp;
      switch(*cp++) {
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
          theRadix=16;
          done=PR_TRUE;
          break;
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
          done=PR_TRUE;
          break;
        case '-': 
          negate=PR_TRUE; //fall through...
          break;
        case 'X': case 'x': 
          theRadix=16;
          break; 
        default:
          break;
      } //switch
    }

    theRadix = (kAutoDetect==aRadix) ? theRadix : aRadix;

      //if you don't have any valid chars, return 0, but set the error;
    if(cp<=endcp) {

      *anErrorCode = NS_OK;

        //now iterate the numeric chars and build our result
      PRUnichar* first=--cp;  //in case we have to back up.

      while(cp<=endcp){
        theChar=*cp++;
        if(('0'<=theChar) && (theChar<='9')){
          result = (theRadix * result) + (theChar-'0');
        }
        else if((theChar>='A') && (theChar<='F')) {
          if(10==theRadix) {
            if(kAutoDetect==aRadix){
              theRadix=16;
              cp=first; //backup
              result=0;
            }
            else {
              *anErrorCode=NS_ERROR_ILLEGAL_VALUE;
              result=0;
              break;
            }
          }
          else {
            result = (theRadix * result) + ((theChar-'A')+10);
          }
        }
        else if((theChar>='a') && (theChar<='f')) {
          if(10==theRadix) {
            if(kAutoDetect==aRadix){
              theRadix=16;
              cp=first; //backup
              result=0;
            }
            else {
              *anErrorCode=NS_ERROR_ILLEGAL_VALUE;
              result=0;
              break;
            }
          }
          else {
            result = (theRadix * result) + ((theChar-'a')+10);
          }
        }
        else if(('X'==theChar) || ('x'==theChar) || ('#'==theChar) || ('+'==theChar)) {
          continue;
        }
        else {
          //we've encountered a char that's not a legal number or sign
          break;
        }
      } //while
      if(negate)
        result=-result;
    } //if
  }
  return result;
}

/**********************************************************************
  String manipulation methods...                
 *********************************************************************/


#ifndef NEW_STRING_APIS
/**
 * assign given nsStr (or derivative) to this one
 * @update  gess 01/04/99
 * @param   aString: nsStr to be appended
 * @return  this
 */
nsString& nsString::Assign(const nsStr& aString,PRInt32 aCount) {
  if(this!=&aString){
    nsStr::Truncate(*this,0);

    if(aCount<0)
      aCount=aString.mLength;
    else aCount=MinInt(aCount,aString.mLength);

    StrAssign(*this,aString,0,aCount);
  }
  return *this;
}
#endif

/**
 * assign given char* to this string
 * @update  gess 01/04/99
 * @param   aCString: buffer to be assigned to this 
 * @param   aCount -- length of given buffer or -1 if you want me to compute length.
 * NOTE:    IFF you pass -1 as aCount, then your buffer must be null terminated.
 *
 * @return  this
 */
void nsString::AssignWithConversion(const char* aCString,PRInt32 aCount) {
  nsStr::Truncate(*this,0);
  if(aCString){
    AppendWithConversion(aCString,aCount);
  }
}

#ifdef NEW_STRING_APIS
void nsString::AssignWithConversion(const char* aCString) {
  nsStr::Truncate(*this,0);
  if(aCString){
    AppendWithConversion(aCString);
  }
}
#endif

#ifndef NEW_STRING_APIS
/**
 * assign given unichar* to this string
 * @update  gess 01/04/99
 * @param   aString: buffer to be assigned to this 
 * @param   aCount -- length of given buffer or -1 if you want me to compute length.
 * NOTE:    IFF you pass -1 as aCount, then your buffer must be null terminated.
 *
 * @return  this
 */
nsString& nsString::Assign(const PRUnichar* aString,PRInt32 aCount) {
  nsStr::Truncate(*this,0);
  if(aString){
    Append(aString,aCount);
  }
  return *this;
}
#endif

/**
 * assign given char to this string
 * @update  gess 01/04/99
 * @param   aChar: char to be assignd to this
 * @return  this
 */
void nsString::AssignWithConversion(char aChar) {
  nsStr::Truncate(*this,0);
  AppendWithConversion(aChar);
}

#ifndef NEW_STRING_APIS
/**
 * assign given unichar to this string
 * @update  gess 01/04/99
 * @param   aChar: char to be assignd to this
 * @return  this
 */
nsString& nsString::Assign(PRUnichar aChar) {
  nsStr::Truncate(*this,0);
  return Append(aChar);
}
#endif

#ifndef NEW_STRING_APIS
/**
 * WARNING! THIS IS A VERY SPECIAL METHOD. 
 * This method "steals" the contents of aSource and hands it to aDest.
 * Ordinarily a copy is made, but not in this version.
 * @update  gess10/30/98
 * @param 
 * @return
 */
#if defined(AIX) || defined(XP_OS2_VACPP)
nsString& nsString::operator=(const nsSubsumeStr& aSubsumeString) {
  nsSubsumeStr temp(aSubsumeString);  // a temp is needed for the AIX and VAC++ compilers
  Subsume(*this,temp);
#else
  nsString& nsString::operator=(nsSubsumeStr& aSubsumeString) {
  Subsume(*this,aSubsumeString);
#endif // AIX || XP_OS2_VACPP
  return *this;
}
#endif

/**
 * append given c-string to this string
 * @update  gess 01/04/99
 * @param   aString : string to be appended to this
 * @param   aCount -- length of given buffer or -1 if you want me to compute length.
 * NOTE:    IFF you pass -1 as aCount, then your buffer must be null terminated.
 *
 * @return  this
 */
void nsString::AppendWithConversion(const char* aCString,PRInt32 aCount) {
  if(aCString && aCount){  //if astring is null or count==0 there's nothing to do
    nsStr temp;
    nsStr::Initialize(temp,eOneByte);
    temp.mStr=(char*)aCString;

    if(0<aCount) {
      temp.mLength=aCount;

      // If this assertion fires, the caller is probably lying about the length of
      //   the passed-in string.  File a bug on the caller.

#ifdef NS_DEBUG
      PRInt32 len=nsStr::FindChar(temp,0,PR_FALSE,0,temp.mLength);
      if(kNotFound<len) {
        NS_WARNING(kPossibleNull);
      }
#endif

    }
    else aCount=temp.mLength=nsCRT::strlen(aCString);
      
    if(0<aCount)
      StrAppend(*this,temp,0,aCount);
  }
}

/**
 * append given char to this string
 * @update  gess 01/04/99
 * @param   aChar: char to be appended 
 * @return  this
 */
void nsString::AppendWithConversion(char aChar) {
  char buf[2]={0,0};
  buf[0]=aChar;

  nsStr temp;
  nsStr::Initialize(temp,eOneByte);
  temp.mStr=buf;
  temp.mLength=1;
  StrAppend(*this,temp,0,1);
}

/**
 * Append the given integer to this string 
 * @update  gess 01/04/99
 * @param   aInteger:
 * @param   aRadix:
 * @return
 */
void nsString::AppendInt(PRInt32 anInteger,PRInt32 aRadix) {

  PRUint32 theInt=(PRUint32)anInteger;

  char buf[]={'0',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  PRInt32 radices[] = {1000000000,268435456};
  PRInt32 mask1=radices[16==aRadix];

  PRInt32 charpos=0;
  if(anInteger<0) {
    theInt*=-1;
    if(10==aRadix) {
      buf[charpos++]='-';
    }
    else theInt=(int)~(theInt-1);
  }

  PRBool isfirst=PR_TRUE;
  while(mask1>=1) {
    PRInt32 theDiv=theInt/mask1;
    if((theDiv) || (!isfirst)) {
      buf[charpos++]="0123456789abcdef"[theDiv];
      isfirst=PR_FALSE;
    }
    theInt-=theDiv*mask1;
    mask1/=aRadix;
  }
  AppendWithConversion(buf);
}


/**
 * Append the given float to this string 
 * @update  gess 01/04/99
 * @param   aFloat:
 * @return
 */
void nsString::AppendFloat(double aFloat){
  char buf[40];
  // *** XX UNCOMMENT THIS LINE
  //PR_snprintf(buf, sizeof(buf), "%g", aFloat);
  sprintf(buf,"%g",aFloat);
  AppendWithConversion(buf);
}


#ifndef NEW_STRING_APIS
/**
 * append given string to this string; 
 * @update  gess 01/04/99
 * @param   aString : string to be appended to this
 * @return  this
 */
#if 0
nsString& nsString::Append(const nsStr& aString,PRInt32 aCount) {

  if(aCount<0)
    aCount=aString.mLength;
  else aCount=MinInt(aCount,aString.mLength);

  if(0<aCount)
    StrAppend(*this,aString,0,aCount);
  return *this;
}
#endif

/**
 * append given string to this string
 * I don't think we need this method now that we have the previous one.
 * @update  gess 01/04/99
 * @param   aString : string to be appended to this
 * @return  this
 */
nsString& nsString::Append(const nsString& aString,PRInt32 aCount) {
  if(aCount<0)
    aCount=aString.mLength;
  else aCount=MinInt(aCount,aString.mLength);
  if(0<aCount)
    StrAppend(*this,aString,0,aCount);
  return *this;
}

/**
 * append given unicode string to this 
 * @update  gess 01/04/99
 * @param   aString : string to be appended to this
 * @param   aCount -- length of given buffer or -1 if you want me to compute length.
 * NOTE:    IFF you pass -1 as aCount, then your buffer must be null terminated.
 *
 * @return  this
 */
nsString& nsString::Append(const PRUnichar* aString,PRInt32 aCount) {
  if(aString && aCount){  //if astring is null or count==0 there's nothing to do
    nsStr temp;
    nsStr::Initialize(temp,eTwoByte);
    temp.mUStr=(PRUnichar*)aString;

    if(0<aCount) {
      temp.mLength=aCount;

      // If this assertion fires, the caller is probably lying about the length of
      //   the passed-in string.  File a bug on the caller.
#ifdef NS_DEBUG
      PRInt32 len=nsStr::FindChar(temp,0,PR_FALSE,0,temp.mLength);
      if(kNotFound<len) {
        NS_WARNING(kPossibleNull);
      }
#endif

    }
    else aCount=temp.mLength=nsCRT::strlen(aString);
      
    if(0<aCount)
      StrAppend(*this,temp,0,aCount);
  }
  return *this;
}

/**
 * append given unichar to this string
 * @update  gess 01/04/99
 * @param   aChar: char to be appended to this
 * @return  this
 */
nsString& nsString::Append(PRUnichar aChar) {
  PRUnichar buf[2]={0,0};
  buf[0]=aChar;

  nsStr temp;
  nsStr::Initialize(temp,eTwoByte);
  temp.mUStr=buf;
  temp.mLength=1;
  StrAppend(*this,temp,0,1);
  return *this;
}
#endif

/*
 *  Copies n characters from this left of this string to given string,
 *  
 *  @update  gess 4/1/98
 *  @param   aDest -- Receiving string
 *  @param   aCount -- number of chars to copy
 *  @return  number of chars copied
 */
PRUint32 nsString::Left(nsString& aDest,PRInt32 aCount) const{

  if(aCount<0)
    aCount=mLength;
  else aCount=MinInt(aCount,mLength);
  nsStr::StrAssign(aDest,*this,0,aCount);

  return aDest.mLength;
}

/*
 *  Copies n characters from this string to from given offset
 *  
 *  @update  gess 4/1/98
 *  @param   aDest -- Receiving string
 *  @param   anOffset -- where copying should begin
 *  @param   aCount -- number of chars to copy
 *  @return  number of chars copied
 */
PRUint32 nsString::Mid(nsString& aDest,PRUint32 anOffset,PRInt32 aCount) const{
  if(aCount<0)
    aCount=mLength;
  else aCount=MinInt(aCount,mLength);
  nsStr::StrAssign(aDest,*this,anOffset,aCount);

  return aDest.mLength;
}

/*
 *  Copies last n characters from this string to given string,
 *  
 *  @update gess 4/1/98
 *  @param  aDest -- Receiving string
 *  @param  aCount -- number of chars to copy
 *  @return number of chars copied
 */
PRUint32 nsString::Right(nsString& aDest,PRInt32 aCount) const{
  PRInt32 offset=MaxInt(mLength-aCount,0);
  return Mid(aDest,offset,aCount);
}



/**
 * Insert a char* into this string at a specified offset.
 *
 * @update  gess4/22/98
 * @param   char* aCString to be inserted into this string
 * @param   anOffset is insert pos in str 
 * @param   aCount -- length of given buffer or -1 if you want me to compute length.
 * NOTE:    IFF you pass -1 as aCount, then your buffer must be null terminated.
 *
 * @return  this
 */
void nsString::InsertWithConversion(const char* aCString,PRUint32 anOffset,PRInt32 aCount){
  if(aCString && aCount){
    nsStr temp;
    nsStr::Initialize(temp,eOneByte);
    temp.mStr=(char*)aCString;

    if(0<aCount) {
      temp.mLength=aCount;

      // If this assertion fires, the caller is probably lying about the length of
      //   the passed-in string.  File a bug on the caller.
#ifdef NS_DEBUG
      PRInt32 len=nsStr::FindChar(temp,0,PR_FALSE,0,temp.mLength);
      if(kNotFound<len) {
        NS_WARNING(kPossibleNull);
      }
#endif

    }
    else aCount=temp.mLength=nsCRT::strlen(aCString);

    if(0<aCount){
      StrInsert(*this,anOffset,temp,0,aCount);
    }
  }
}

#ifndef NEW_STRING_APIS
/*
 *  This method inserts n chars from given string into this
 *  string at str[anOffset].
 *  
 *  @update gess 4/1/98
 *  @param  aString -- source String to be inserted into this
 *  @param  anOffset -- insertion position within this str
 *  @param  aCount -- number of chars to be copied from aCopy
 *  @return this
 */
void nsString::Insert(const nsString& aCopy,PRUint32 anOffset,PRInt32 aCount) {
  StrInsert(*this,anOffset,aCopy,0,aCount);
}


/**
 * Insert a unicode* into this string at a specified offset.
 *
 * @update  gess4/22/98
 * @param   aChar char to be inserted into this string
 * @param   anOffset is insert pos in str 
 * @param   aCount -- length of given buffer or -1 if you want me to compute length.
 * NOTE:    IFF you pass -1 as aCount, then your buffer must be null terminated.
 *
 * @return  this
 */
void nsString::Insert(const PRUnichar* aString,PRUint32 anOffset,PRInt32 aCount){
  if(aString && aCount){
    nsStr temp;
    nsStr::Initialize(temp,eTwoByte);
    temp.mUStr=(PRUnichar*)aString;

    if(0<aCount) {
      temp.mLength=aCount;

      // If this assertion fires, the caller is probably lying about the length of
      //   the passed-in string.  File a bug on the caller.
#ifdef NS_DEBUG
      PRInt32 len=nsStr::FindChar(temp,0,PR_FALSE,0,temp.mLength);
      if(kNotFound<len) {
        NS_WARNING(kPossibleNull);
      }
#endif

    }
    else aCount=temp.mLength=nsCRT::strlen(aString);

    if(0<aCount){
      StrInsert(*this,anOffset,temp,0,aCount);
    }
  }
}


/**
 * Insert a single uni-char into this string at
 * a specified offset.
 *
 * @update  gess4/22/98
 * @param   aChar char to be inserted into this string
 * @param   anOffset is insert pos in str 
 * @return  this
 */
void nsString::Insert(PRUnichar aChar,PRUint32 anOffset){
  PRUnichar theBuffer[2]={0,0};
  theBuffer[0]=aChar;
  nsStr temp;
  nsStr::Initialize(temp,eTwoByte);
  temp.mUStr=theBuffer;
  temp.mLength=1;
  StrInsert(*this,anOffset,temp,0,1);
}
#endif

/*
 *  This method is used to cut characters in this string
 *  starting at anOffset, continuing for aCount chars.
 *  
 *  @update gess 01/04/99
 *  @param  anOffset -- start pos for cut operation
 *  @param  aCount -- number of chars to be cut
 *  @return *this
 */
#ifndef NEW_STRING_APIS
void nsString::Cut(PRUint32 anOffset, PRInt32 aCount) {
  if(0<aCount) {
    nsStr::Delete(*this,anOffset,aCount);
  }
}
#endif

/**********************************************************************
  Searching methods...                
 *********************************************************************/
 
/**
 *  search for given string within this string
 *  
 *  @update  gess 3/25/98
 *  @param   aString - substr to be found
 *  @param   aIgnoreCase tells us whether or not to do caseless compare
 *  @param   anOffset tells us where in this string to start searching
 *  @param   aCount tells us how many iterations to make starting at the given offset
 *  @return  offset in string, or -1 (kNotFound)
 */
PRInt32 nsString::Find(const char* aCString,PRBool aIgnoreCase,PRInt32 anOffset,PRInt32 aCount) const{
  NS_ASSERTION(0!=aCString,kNullPointerError);

  PRInt32 result=kNotFound;
  if(aCString) {
    nsStr temp;
    nsStr::Initialize(temp,eOneByte);
    temp.mLength=nsCRT::strlen(aCString);
    temp.mStr=(char*)aCString;
    result=nsStr::FindSubstr(*this,temp,aIgnoreCase,anOffset,aCount);
  }
  return result;
}

/**
 *  search for given string within this string
 *  
 *  @update  gess 3/25/98
 *  @param   aString - substr to be found
 *  @param   aIgnoreCase tells us whether or not to do caseless compare
 *  @param   anOffset tells us where in this string to start searching
 *  @param   aCount tells us how many iterations to make starting at the given offset
 *  @return  offset in string, or -1 (kNotFound)
 */
PRInt32 nsString::Find(const PRUnichar* aString,PRBool aIgnoreCase,PRInt32 anOffset,PRInt32 aCount) const{
  NS_ASSERTION(0!=aString,kNullPointerError);

  PRInt32 result=kNotFound;
  if(aString) {
    nsStr temp;
    nsStr::Initialize(temp,eTwoByte);
    temp.mLength=nsCRT::strlen(aString);
    temp.mUStr=(PRUnichar*)aString;
    result=nsStr::FindSubstr(*this,temp,aIgnoreCase,anOffset,aCount);
  }
  return result;
}

/**
 *  Search for given string within this string
 *  
 *  @update  gess 3/25/98
 *  @param   aString - substr to be found
 *  @param   aIgnoreCase tells us whether or not to do caseless compare
 *  @param   anOffset tells us where in this string to start searching
 *  @param   aCount tells us how many iterations to make starting at the given offset
 *  @return  offset in string, or -1 (kNotFound)
 */
PRInt32 nsString::Find(const nsStr& aString,PRBool aIgnoreCase,PRInt32 anOffset,PRInt32 aCount) const{
  PRInt32 result=nsStr::FindSubstr(*this,aString,aIgnoreCase,anOffset,aCount);
  return result;
}

/**
 *  search for given string within this string
 *  
 *  @update  gess 3/25/98
 *  @param   aString - substr to be found
 *  @param   aIgnoreCase tells us whether or not to do caseless compare
 *  @param   anOffset tells us where in this string to start searching
 *  @param   aCount tells us how many iterations to make starting at the given offset
 *  @return  offset in string, or -1 (kNotFound)
 */
PRInt32 nsString::Find(const nsString& aString,PRBool aIgnoreCase,PRInt32 anOffset,PRInt32 aCount) const{
  PRInt32 result=nsStr::FindSubstr(*this,aString,aIgnoreCase,anOffset,aCount);
  return result;
}

/**
 *  Search for a given char, starting at given offset
 *  
 *  @update  gess 3/25/98
 *  @param   aChar
 *  @return  offset of found char, or -1 (kNotFound)
 */
#if 0
PRInt32 nsString::Find(PRUnichar aChar,PRInt32 anOffset,PRBool aIgnoreCase) const{
  PRInt32 result=nsStr::FindChar(*this,aChar,aIgnoreCase,anOffset);
  return result;
}
#endif

/**
 *  Search for a given char, starting at given offset
 *  
 *  @update  gess 3/25/98
 *  @param   aChar is the unichar to be sought
 *  @param   aIgnoreCase tells us whether or not to do caseless compare
 *  @param   anOffset tells us where in this string to start searching
 *  @param   aCount tells us how many iterations to make starting at the given offset
 *  @return  offset in string, or -1 (kNotFound)
 */
PRInt32 nsString::FindChar(PRUnichar aChar,PRBool aIgnoreCase,PRInt32 anOffset,PRInt32 aCount) const{
  PRInt32 result=nsStr::FindChar(*this,aChar,aIgnoreCase,anOffset,aCount);
  return result;
}

/**
 *  This method finds the offset of the first char in this string that is
 *  a member of the given charset, starting the search at anOffset
 *  
 *  @update  gess 3/25/98
 *  @param   aCStringSet
 *  @param   anOffset -- where in this string to start searching
 *  @return  
 */
PRInt32 nsString::FindCharInSet(const char* aCStringSet,PRInt32 anOffset) const{
  NS_ASSERTION(0!=aCStringSet,kNullPointerError);

  PRInt32 result=kNotFound;
  if(aCStringSet) {
    nsStr temp;
    nsStr::Initialize(temp,eOneByte);
    temp.mLength=nsCRT::strlen(aCStringSet);
    temp.mStr=(char*)aCStringSet;
    result=nsStr::FindCharInSet(*this,temp,PR_FALSE,anOffset);
  }
  return result;
}

/**
 *  This method finds the offset of the first char in this string that is
 *  a member of the given charset, starting the search at anOffset
 *  
 *  @update  gess 3/25/98
 *  @param   aCStringSet
 *  @param   anOffset -- where in this string to start searching
 *  @return  
 */
PRInt32 nsString::FindCharInSet(const PRUnichar* aStringSet,PRInt32 anOffset) const{
  NS_ASSERTION(0!=aStringSet,kNullPointerError);

  PRInt32 result=kNotFound;
  if(aStringSet) {
    nsStr temp;
    nsStr::Initialize(temp,eTwoByte);
    temp.mLength=nsCRT::strlen(aStringSet);
    temp.mStr=(char*)aStringSet;
    result=nsStr::FindCharInSet(*this,temp,PR_FALSE,anOffset);
  }
  return result;
}

/**
 *  This method finds the offset of the first char in this string that is
 *  a member of the given charset, starting the search at anOffset
 *  
 *  @update  gess 3/25/98
 *  @param   aCStringSet
 *  @param   anOffset -- where in this string to start searching
 *  @return  
 */
PRInt32 nsString::FindCharInSet(const nsStr& aSet,PRInt32 anOffset) const{
  PRInt32 result=nsStr::FindCharInSet(*this,aSet,PR_FALSE,anOffset);
  return result;
}


/**
 *  Reverse search for given string within this string
 *  
 *  @update  gess 3/25/98
 *  @param   aString - substr to be found
 *  @param   aIgnoreCase tells us whether or not to do caseless compare
 *  @param   anOffset tells us where in this string to start searching
 *  @param   aCount tells us how many iterations to make starting at the given offset
 *  @return  offset in string, or -1 (kNotFound)
 */
PRInt32 nsString::RFind(const nsStr& aString,PRBool aIgnoreCase,PRInt32 anOffset,PRInt32 aCount) const{
  PRInt32 result=nsStr::RFindSubstr(*this,aString,aIgnoreCase,anOffset,aCount);
  return result;
}

/**
 *  Reverse search for given string within this string
 *  
 *  @update  gess 3/25/98
 *  @param   aString - substr to be found
 *  @param   aIgnoreCase tells us whether or not to do caseless compare
 *  @param   anOffset tells us where in this string to start searching
 *  @param   aCount tells us how many iterations to make starting at the given offset
 *  @return  offset in string, or -1 (kNotFound)
 */
PRInt32 nsString::RFind(const nsString& aString,PRBool aIgnoreCase,PRInt32 anOffset,PRInt32 aCount) const{
  PRInt32 result=nsStr::RFindSubstr(*this,aString,aIgnoreCase,anOffset,aCount);
  return result;
}

/**
 *  Reverse search for given string within this string
 *  
 *  @update  gess 3/25/98
 *  @param   aString - substr to be found
 *  @param   aIgnoreCase tells us whether or not to do caseless compare
 *  @param   anOffset tells us where in this string to start searching
 *  @param   aCount tells us how many iterations to make starting at the given offset
 *  @return  offset in string, or -1 (kNotFound)
 */
PRInt32 nsString::RFind(const char* aString,PRBool aIgnoreCase,PRInt32 anOffset,PRInt32 aCount) const{
  NS_ASSERTION(0!=aString,kNullPointerError);
 
  PRInt32 result=kNotFound;
  if(aString) {
    nsStr temp;
    nsStr::Initialize(temp,eOneByte);
    temp.mLength=nsCRT::strlen(aString);
    temp.mStr=(char*)aString;
    result=nsStr::RFindSubstr(*this,temp,aIgnoreCase,anOffset,aCount);
  }
  return result;
}


/**
 *  Reverse search for char
 *  
 *  @update  gess 3/25/98
 *  @param   achar
 *  @param   aIgnoreCase
 *  @param   anOffset - tells us where to begin the search
 *  @return  offset of substring or -1
 */
#if 0
PRInt32 nsString::RFind(PRUnichar aChar,PRInt32 anOffset,PRBool aIgnoreCase) const{
  PRInt32 result=nsStr::RFindChar(*this,aChar,aIgnoreCase,anOffset);
  return result;
}
#endif
 
/**
 *  Reverse search for a given char, starting at given offset
 *  
 *  @update  gess 3/25/98
 *  @param   aChar
 *  @param   aIgnoreCase
 *  @param   anOffset
 *  @return  offset of found char, or -1 (kNotFound)
 */
PRInt32 nsString::RFindChar(PRUnichar aChar,PRBool aIgnoreCase,PRInt32 anOffset,PRInt32 aCount) const{
  PRInt32 result=nsStr::RFindChar(*this,aChar,aIgnoreCase,anOffset,aCount);
  return result;
}

/**
 *  Reverse search for char in this string that is also a member of given charset
 *  
 *  @update  gess 3/25/98
 *  @param   aCStringSet
 *  @param   anOffset
 *  @return  offset of found char, or -1 (kNotFound)
 */
PRInt32 nsString::RFindCharInSet(const char* aCStringSet,PRInt32 anOffset) const{
  NS_ASSERTION(0!=aCStringSet,kNullPointerError);

  PRInt32 result=kNotFound;
  if(aCStringSet) {
    nsStr temp;
    nsStr::Initialize(temp,eOneByte);
    temp.mLength=nsCRT::strlen(aCStringSet);
    temp.mStr=(char*)aCStringSet;
    result=nsStr::RFindCharInSet(*this,temp,PR_FALSE,anOffset);
  }
  return result;
}

/**
 *  Reverse search for a first char in this string that is a
 *  member of the given char set
 *  
 *  @update  gess 3/25/98
 *  @param   aSet
 *  @param   aIgnoreCase
 *  @param   anOffset
 *  @return  offset of found char, or -1 (kNotFound)
 */
PRInt32 nsString::RFindCharInSet(const nsStr& aSet,PRInt32 anOffset) const{
  PRInt32 result=nsStr::RFindCharInSet(*this,aSet,PR_FALSE,anOffset);
  return result;
}

/**
 *  Reverse search for a first char in this string that is a
 *  member of the given char set
 *  
 *  @update  gess 3/25/98
 *  @param   aSet
 *  @param   aIgnoreCase
 *  @param   anOffset
 *  @return  offset of found char, or -1 (kNotFound)
 */
PRInt32 nsString::RFindCharInSet(const PRUnichar* aStringSet,PRInt32 anOffset) const{
  NS_ASSERTION(0!=aStringSet,kNullPointerError);

  PRInt32 result=kNotFound;
  if(aStringSet) {
    nsStr temp;
    nsStr::Initialize(temp,eTwoByte);
    temp.mLength=nsCRT::strlen(aStringSet);
    temp.mUStr=(PRUnichar*)aStringSet;
    result=nsStr::RFindCharInSet(*this,temp,PR_FALSE,anOffset);
  }
  return result;
}


/**************************************************************
  COMPARISON METHODS...
 **************************************************************/

/**
 * Compares given cstring to this string. 
 * @update  gess 01/04/99
 * @param   aCString pts to a cstring
 * @param   aIgnoreCase tells us how to treat case
 * @param   aCount tells us how many chars to test; -1 implies full length
 * @return  -1,0,1
 */
PRInt32 nsString::CompareWithConversion(const char *aCString,PRBool aIgnoreCase,PRInt32 aCount) const {
  NS_ASSERTION(0!=aCString,kNullPointerError);

  if(aCString) {
    nsStr temp;
    nsStr::Initialize(temp,eOneByte);

    temp.mLength= (0<aCount) ? aCount : nsCRT::strlen(aCString);

    temp.mStr=(char*)aCString;
    return nsStr::StrCompare(*this,temp,aCount,aIgnoreCase);
  }

  return 0;
}

/**
 * Compares given cstring to this string. 
 * @update  gess 01/04/99
 * @param   aCString pts to a cstring
 * @param   aIgnoreCase tells us how to treat case
 * @param   aCount tells us how many chars to test; -1 implies full length
 * @return  -1,0,1
 */
PRInt32 nsString::CompareWithConversion(const nsString& aString,PRBool aIgnoreCase,PRInt32 aCount) const {
  PRInt32 result=nsStr::StrCompare(*this,aString,aCount,aIgnoreCase);
  return result;
}

/**
 * Compares given unistring to this string. 
 * @update  gess 01/04/99
 * @param   aString pts to a uni-string
 * @param   aIgnoreCase tells us how to treat case
 * @param   aCount tells us how many chars to test; -1 implies full length
 * @return  -1,0,1
 */
PRInt32 nsString::CompareWithConversion(const PRUnichar* aString,PRBool aIgnoreCase,PRInt32 aCount) const {
  NS_ASSERTION(0!=aString,kNullPointerError);

  if(aString) {
    nsStr temp;
    nsStr::Initialize(temp,eTwoByte);
    temp.mLength=nsCRT::strlen(aString);
    temp.mUStr=(PRUnichar*)aString;
    return nsStr::StrCompare(*this,temp,aCount,aIgnoreCase);
  }

   return 0;
}

#ifndef NEW_STRING_APIS
/**
 * Compare given nsStr with this cstring.
 * 
 * @param   aString is an nsStr instance to be compared
 * @param   aIgnoreCase tells us how to treat case
 * @param   aCount tells us how many chars to test; -1 implies full length
 * @return  -1,0,1
 */
PRInt32 nsString::Compare(const nsStr& aString,PRBool aIgnoreCase,PRInt32 aCount) const {
  return nsStr::StrCompare(*this,aString,aCount,aIgnoreCase);
}
#endif

#ifndef NEW_STRING_APIS
/**
 *  Here come a whole bunch of operator functions that are self-explanatory...
 */

PRBool nsString::operator==(const nsString& S) const {return Equals(S);}      
//PRBool nsString::operator==(const nsStr& S) const {return Equals(S);}      
//PRBool nsString::operator==(const char* s) const {return Equals(s);}
PRBool nsString::operator==(const PRUnichar* s) const {return Equals(nsAutoString(s));}

PRBool nsString::operator!=(const nsString& S) const {return PRBool(Compare(S)!=0);}
//PRBool nsString::operator!=(const nsStr& S) const {return PRBool(Compare(S)!=0);}
//PRBool nsString::operator!=(const char* s) const {return PRBool(Compare(s)!=0);}
PRBool nsString::operator!=(const PRUnichar* s) const {return PRBool(Compare(nsAutoString(s))!=0);}

PRBool nsString::operator<(const nsString& S) const {return PRBool(Compare(S)<0);}
//PRBool nsString::operator<(const nsStr& S) const {return PRBool(Compare(S)<0);}
//PRBool nsString::operator<(const char* s) const {return PRBool(Compare(s)<0);}
PRBool nsString::operator<(const PRUnichar* s) const {return PRBool(Compare(nsAutoString(s))<0);}

PRBool nsString::operator>(const nsString& S) const {return PRBool(Compare(S)>0);}
//PRBool nsString::operator>(const nsStr& S) const {return PRBool(Compare(S)>0);}
//PRBool nsString::operator>(const char* s) const {return PRBool(Compare(s)>0);}
PRBool nsString::operator>(const PRUnichar* s) const {return PRBool(Compare(nsAutoString(s))>0);}

PRBool nsString::operator<=(const nsString& S) const {return PRBool(Compare(S)<=0);}
//PRBool nsString::operator<=(const nsStr& S) const {return PRBool(Compare(S)<=0);}
//PRBool nsString::operator<=(const char* s) const {return PRBool(Compare(s)<=0);}
PRBool nsString::operator<=(const PRUnichar* s) const {return PRBool(Compare(nsAutoString(s))<=0);}

PRBool nsString::operator>=(const nsString& S) const {return PRBool(Compare(S)>=0);}
//PRBool nsString::operator>=(const nsStr& S) const {return PRBool(Compare(S)>=0);}
//PRBool nsString::operator>=(const char* s) const {return PRBool(Compare(s)>=0);}
PRBool nsString::operator>=(const PRUnichar* s) const {return PRBool(Compare(nsAutoString(s))>=0);}
#endif // !defined(NEW_STRING_APIS)

PRBool nsString::EqualsIgnoreCase(const nsString& aString) const {
  return EqualsWithConversion(aString,PR_TRUE);
}

PRBool nsString::EqualsIgnoreCase(const char* aString,PRInt32 aLength) const {
  return EqualsWithConversion(aString,PR_TRUE,aLength);
}

#ifndef NEW_STRING_APIS
PRBool nsString::EqualsIgnoreCase(const PRUnichar* s1, const PRUnichar* s2) const {
  return Equals(s1,s2,PR_TRUE);
}
#endif

/**
 * Compare this to given string; note that we compare full strings here.
 * 
 * @update gess 01/04/99
 * @param  aString is the other nsString to be compared to
 * @param   aCount tells us how many chars to test; -1 implies full length
 * @return TRUE if equal
 */
PRBool nsString::EqualsWithConversion(const nsString& aString,PRBool aIgnoreCase,PRInt32 aCount) const {
  PRInt32 theAnswer=nsStr::StrCompare(*this,aString,aCount,aIgnoreCase);
  PRBool  result=PRBool(0==theAnswer);
  return result;

}

/**
 * Compare this to given c-string; note that we compare full strings here.
 *
 * @param  aString is the CString to be compared 
 * @param  aIgnorecase tells us whether to be case sensitive
 * @param   aCount tells us how many chars to test; -1 implies full length
 * @return TRUE if equal
 */
PRBool nsString::EqualsWithConversion(const char* aString,PRBool aIgnoreCase,PRInt32 aCount) const {
  PRInt32 theAnswer=CompareWithConversion(aString,aIgnoreCase,aCount);
  PRBool  result=PRBool(0==theAnswer);
  return result;
}

/**
 * Compare this to given unicode string; note that we compare full strings here.
 *
 * @param  aString is the U-String to be compared 
 * @param  aIgnorecase tells us whether to be case sensitive
 * @param   aCount tells us how many chars to test; -1 implies full length
 * @return TRUE if equal
 */
PRBool nsString::EqualsWithConversion(const PRUnichar* aString,PRBool aIgnoreCase,PRInt32 aCount) const {
  PRInt32 theAnswer=CompareWithConversion(aString,aIgnoreCase,aCount);
  PRBool  result=PRBool(0==theAnswer);
  return result;
}

/**
 * Compare this to given atom; note that we compare full strings here.
 * The optional length argument just lets us know how long the given string is.
 * If you provide a length, it is compared to length of this string as an
 * optimization.
 * 
 * @update gess 01/04/99
 * @param  aString -- unistring to compare to this
 * @param  aLength -- length of given string.
 * @return TRUE if equal
 */
PRBool nsString::EqualsAtom(/*FIX: const */nsIAtom* aAtom,PRBool aIgnoreCase) const{
  NS_ASSERTION(0!=aAtom,kNullPointerError);
  PRBool result=PR_FALSE;
  if(aAtom){
    PRInt32 cmp=0;
    const PRUnichar* unicode;
    if (aAtom->GetUnicode(&unicode) != NS_OK || unicode == nsnull)
        return PR_FALSE;
    if (aIgnoreCase)
      cmp=nsCRT::strcasecmp(mUStr,unicode);
    else
      cmp=nsCRT::strcmp(mUStr,unicode);
    result=PRBool(0==cmp);
  }

   return result;
}

PRBool nsString::EqualsIgnoreCase(/*FIX: const */nsIAtom *aAtom) const {
  return EqualsAtom(aAtom,PR_TRUE);
}

#ifndef NEW_STRING_APIS
/**
 * Compare this to given string; note that we compare full strings here.
 * 
 * @update gess 01/04/99
 * @param  aString is the other nsString to be compared to
 * @param   aCount tells us how many chars to test; -1 implies full length
 * @return TRUE if equal
 */
PRBool nsString::Equals(const nsStr& aString,PRBool aIgnoreCase,PRInt32 aCount) const {
  PRInt32 theAnswer=nsStr::StrCompare(*this,aString,aCount,aIgnoreCase);
  PRBool  result=PRBool(0==theAnswer);
  return result;
}

/**
 * Compare given strings
 * @update gess 7/27/98
 * @param  s1 -- first unichar string to be compared
 * @param  s2 -- second unichar string to be compared
 * @return TRUE if equal
 */
PRBool nsString::Equals(const PRUnichar* s1, const PRUnichar* s2,PRBool aIgnoreCase) const {
  NS_ASSERTION(0!=s1,kNullPointerError);
  NS_ASSERTION(0!=s2,kNullPointerError);
  PRBool  result=PR_FALSE;
  if((s1) && (s2)){
    PRInt32 cmp=(aIgnoreCase) ? nsCRT::strcasecmp(s1,s2) : nsCRT::strcmp(s1,s2);
    result=PRBool(0==cmp);
  }

   return result;
}
#endif

/**
 *  Determine if given char in valid alpha range
 *  
 *  @update  gess 3/31/98
 *  @param   aChar is character to be tested
 *  @return  TRUE if in alpha range
 */
PRBool nsString::IsAlpha(PRUnichar aChar) {
  // XXX i18n
  if (((aChar >= 'A') && (aChar <= 'Z')) || ((aChar >= 'a') && (aChar <= 'z'))) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

/**
 *  Determine if given char is a valid space character
 *  
 *  @update  gess 3/31/98
 *  @param   aChar is character to be tested
 *  @return  TRUE if is valid space char
 */
PRBool nsString::IsSpace(PRUnichar aChar) {
  // XXX i18n
  if ((aChar == ' ') || (aChar == '\r') || (aChar == '\n') || (aChar == '\t')) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

/**
 *  Determine if given buffer contains plain ascii
 *  
 *  @param   aBuffer -- if null, then we test *this, otherwise we test given buffer
 *  @return  TRUE if is all ascii chars, or if strlen==0
 */
PRBool nsString::IsASCII(const PRUnichar* aBuffer) {

  if(!aBuffer) {
    aBuffer=mUStr;
    if(eOneByte==mCharSize)
      return PR_TRUE;
  }
  if(aBuffer) {
    while(*aBuffer) {
      if(*aBuffer>255){
        return PR_FALSE;        
      }
      aBuffer++;
    }
  }
  return PR_TRUE;
}


/**
 *  Determine if given char is valid digit
 *  
 *  @update  gess 3/31/98
 *  @param   aChar is character to be tested
 *  @return  TRUE if char is a valid digit
 */
PRBool nsString::IsDigit(PRUnichar aChar) {
  // XXX i18n
  return PRBool((aChar >= '0') && (aChar <= '9'));
}

#ifndef RICKG_TESTBED
/**************************************************************
  Define the string deallocator class...
 **************************************************************/
class nsStringDeallocator: public nsDequeFunctor{
public:
  virtual void* operator()(void* anObject) {
    nsString* aString= (nsString*)anObject;
    if(aString){
      delete aString;
    }
    return 0;
  }
};

/****************************************************************************
 * This class, appropriately enough, creates and recycles nsString objects..
 ****************************************************************************/

class nsStringRecycler {
public:
  nsStringRecycler() : mDeque(0) {
  }

  ~nsStringRecycler() {
    nsStringDeallocator theDeallocator;
    mDeque.ForEach(theDeallocator); //now delete the strings
  }

  void Recycle(nsString* aString) {
    mDeque.Push(aString);
  }

  nsString* CreateString(void){
    nsString* result=(nsString*)mDeque.Pop();
    if(!result)
      result=new nsString();
    return result;
  }
  nsDeque mDeque;
};
static nsStringRecycler& GetRecycler(void);

/**
 * 
 * @update  gess 01/04/99
 * @param 
 * @return
 */
nsStringRecycler& GetRecycler(void){
  static nsStringRecycler gRecycler;
  return gRecycler;
}
#endif


/**
 * Call this mehod when you're done 
 * @update  gess 01/04/99
 * @param 
 * @return
 */
nsString* nsString::CreateString(void){
  nsString* result=0;
#ifndef RICKG_TESTBED
  GetRecycler().CreateString();
#endif
  return result;
}

/**
 * Call this mehod when you're done 
 * @update  gess 01/04/99
 * @param 
 * @return
 */
void nsString::Recycle(nsString* aString){
#ifndef RICKG_TESTBED
  GetRecycler().Recycle(aString);
#else
  delete aString;
#endif
}

#if 0

/**
 * 
 * @update  gess8/8/98
 * @param 
 * @return
 */
ostream& operator<<(ostream& aStream,const nsString& aString){
  if(eOneByte==aString.mCharSize) {
    aStream<<aString.mStr;
  }
  else{
    PRUint32        theOffset=0;
    const PRUint32  theBufSize=300;
    char            theBuf[theBufSize+1];
    PRUint32        theCount=0;
    PRUint32        theRemains=0;

    while(theOffset<aString.mLength){
      theRemains=aString.mLength-theOffset;
      theCount=(theRemains<theBufSize) ? theRemains : theBufSize;
      aString.ToCString(theBuf,theCount+1,theOffset);
      theBuf[theCount]=0;
      aStream<<theBuf;
      theOffset+=theCount;
    }
  }    
  return aStream;
}
#endif


/**
 * 
 * @update  gess 01/04/99
 * @param 
 * @return
 */
NS_COM int fputs(const nsString& aString, FILE* out)
{
  char buf[100];
  char* cp = buf;
  PRInt32 len = aString.mLength;
  if (len >= PRInt32(sizeof(buf))) {
    cp = aString.ToNewCString();
  } else {
    aString.ToCString(cp, len + 1);
  }
  if(len>0)
    ::fwrite(cp, 1, len, out);
  if (cp != buf) {
    delete[] cp;
  }
  return (int) len;
}
       
/**
 * Dumps the contents of the string to stdout
 * @update  gess 11/15/99
 */
void nsString::DebugDump(void) const {
  
  const char* theBuffer=mStr;
  nsCAutoString temp;

  if(eTwoByte==mCharSize) {  
    nsStr::StrAssign(temp, *this, 0, mLength);  
    theBuffer=temp.GetBuffer();
  }

  if(theBuffer) {
    printf("\n%s",theBuffer);
  }
}
       


/***********************************************************************
  IMPLEMENTATION NOTES: AUTOSTRING...
 ***********************************************************************/


/**
 * Default constructor
 *
 */
nsAutoString::nsAutoString() : nsString() {
  Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
  AddNullTerminator(*this);
}

/**
 * Copy construct from uni-string
 * @param   aString is a ptr to a unistr
 * @param   aLength tells us how many chars to copy from aString
 */
nsAutoString::nsAutoString(const PRUnichar* aString,PRInt32 aLength) : nsString() {
  Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
  AddNullTerminator(*this);
  Append(aString,aLength);
}

nsAutoString::nsAutoString( const nsString& aString )
  : nsString()
{
  Initialize(*this, mBuffer, (sizeof(mBuffer)>>eTwoByte)-1, 0, eTwoByte, PR_FALSE);
  AddNullTerminator(*this);
  Append(aString);
}


/**
 * constructor that uses external buffer
 * @param   aBuffer describes the external buffer
 */
nsAutoString::nsAutoString(const CBufDescriptor& aBuffer) : nsString() {
  if(!aBuffer.mBuffer) {
    Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
  }
  else {
    Initialize(*this,aBuffer.mBuffer,aBuffer.mCapacity,aBuffer.mLength,aBuffer.mCharSize,!aBuffer.mStackBased);
  }
  if(!aBuffer.mIsConst)
    AddNullTerminator(*this);
}

NS_ConvertASCIItoUCS2::NS_ConvertASCIItoUCS2( const char* aCString, PRUint32 aLength )
  {
    Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
    AddNullTerminator(*this);
    AppendWithConversion(aCString,aLength);
  }

NS_ConvertASCIItoUCS2::NS_ConvertASCIItoUCS2( const char* aCString )
  {
    Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
    AddNullTerminator(*this);
    AppendWithConversion(aCString);
  }

NS_ConvertASCIItoUCS2::NS_ConvertASCIItoUCS2( char aChar )
  {
    Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
    AddNullTerminator(*this);
    AppendWithConversion(aChar);
  }

#if 0
#ifdef NEW_STRING_APIS
NS_ConvertASCIItoUCS2::NS_ConvertASCIItoUCS2( const nsAReadableCString& )
  {
    // ...
  }
#else
NS_ConvertASCIItoUCS2::NS_ConvertASCIItoUCS2( const nsCString& )
  {
    // ...
  }
#endif
#endif

#if 0
/**
 * Copy construct from ascii c-string
 * @param   aCString is a ptr to a 1-byte cstr
 * @param   aLength tells us how many chars to copy from aCString
 */
nsAutoString::nsAutoString(const char* aCString,PRInt32 aLength) : nsString() {
  Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
  AddNullTerminator(*this);
  Append(aCString,aLength);
}

/**
 * Copy construct from an nsStr
 * @param   
 */
nsAutoString::nsAutoString(const nsStr& aString) : nsString() {
  Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
  AddNullTerminator(*this);
  Append(aString);
}
#endif

/**
 * Default copy constructor
 */
nsAutoString::nsAutoString(const nsAutoString& aString) : nsString() {
  Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
  AddNullTerminator(*this);
  Append(aString);
}


/**
 * Copy construct from a unichar
 * @param   
 */
nsAutoString::nsAutoString(PRUnichar aChar) : nsString(){
  Initialize(*this,mBuffer,(sizeof(mBuffer)>>eTwoByte)-1,0,eTwoByte,PR_FALSE);
  AddNullTerminator(*this);
  Append(aChar);
}

/**
 * construct from a subsumeable string
 * @update  gess 1/4/99
 * @param   reference to a subsumeString
 */
#if defined(AIX) || defined(XP_OS2_VACPP)
nsAutoString::nsAutoString(const nsSubsumeStr& aSubsumeStr) :nsString() {
  nsSubsumeStr temp(aSubsumeStr);  // a temp is needed for the AIX and VAC++ compilers
  Subsume(*this,temp);
#else
nsAutoString::nsAutoString( nsSubsumeStr& aSubsumeStr) :nsString() {
  Subsume(*this,aSubsumeStr);
#endif // AIX || XP_OS2_VACPP
}

/**
 * deconstruct the autstring
 * @param   
 */
nsAutoString::~nsAutoString(){
}

void nsAutoString::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const {
  if (aResult) {
    *aResult = sizeof(*this) + mCapacity * mCharSize;
  }
}

nsSubsumeStr::nsSubsumeStr() : nsString() {
}

nsSubsumeStr::nsSubsumeStr(nsStr& aString) : nsString() {
  ::Subsume(*this,aString);
}

nsSubsumeStr::nsSubsumeStr(PRUnichar* aString,PRBool assumeOwnership,PRInt32 aLength) : nsString() {
  mUStr=aString;
  mCharSize=eTwoByte;
  mCapacity=mLength=(-1==aLength) ? nsCRT::strlen(aString) : aLength;
  mOwnsBuffer=assumeOwnership;
}
 
nsSubsumeStr::nsSubsumeStr(char* aString,PRBool assumeOwnership,PRInt32 aLength) : nsString() {
  mStr=aString;
  mCharSize=eOneByte;
  mCapacity=mLength=(-1==aLength) ? strlen(aString) : aLength;
  mOwnsBuffer=assumeOwnership;
}

int nsSubsumeStr::Subsume(PRUnichar* aString,PRBool assumeOwnership,PRInt32 aLength) {
  mUStr=aString;
  mCharSize=eTwoByte;
  mCapacity=mLength=(-1==aLength) ? nsCRT::strlen(aString) : aLength;
  mOwnsBuffer=assumeOwnership;
  return 0;
}

