/*****************************************************************************************************************
 *  
 * This template provides the basic implementation for nsCString.
 *
 * Contributor(s):
 *   Rick Gessner <rickg@netscape.com>
 * 
 * History:
 *
 *  02.29.2000: Original files (rickg)
 *  03.02.2000: Expand the API's to be a bit more realistic (rickg): 
 *                 1. Flesh out the interface to be compatible with old library 
 *                 2. Enable both nsCString-based interfaces and nsAReadableString interfaces 
 *                 3. Build enough infrastructure to test nsCString(s) and nsImmutableString constructions.
 *  03.03.2000: Finished mapping original nsCString functionality to template form (rickg)
 *  
 *****************************************************************************************************************/


#ifndef _NS_CSTRING_
#define _NS_CSTRING_

#include "nsStringValue.h"


class nsString;

class nsCString: public nsAReadableString {
public:

  //******************************************
  // Ctor's
  //******************************************
  
  nsCString();
  
    //copy constructor
  nsCString(const nsCString& aString,PRInt32 aLength=-1);

    //constructor from nsString
  nsCString(const nsString& aString,PRInt32 aLength=-1);

    //call this version for char*'s....
  nsCString(const char* aString,PRInt32 aLength=-1) ;

    //call this version for a single char
  nsCString(const char aChar);

    //call this version for PRUnichar*'s....
  nsCString(const PRUnichar* aString,PRInt32 aLength=-1) ;

    //call this version for all other ABT versions of readable strings
  nsCString(const nsAReadableString &aString) ;

    //call this version for stack-based string buffers...
  nsCString(const nsStackBuffer<char> &aBuffer);

  virtual ~nsCString();

  
  //******************************************
  // Assigment operators
  //******************************************
  
    
  nsCString& operator=(const nsCString& aString);

  nsCString& operator=(const nsString& aString);

  nsCString& operator=(const char* aString);

  nsCString& operator=(const PRUnichar* aString);

  nsCString& operator=(const char aChar);

  //******************************************
  // Here are the accessor methods...
  //******************************************

  virtual nsresult SetLength(PRUint32 aLength) {
    Truncate(aLength);
    return NS_OK;
  }

  virtual nsresult SetCapacity(PRUint32 aCapacity);

  char* GetBuffer() { return mStringValue.mBuffer;}

  char& operator[](PRUint32 aOffset);  

//  operator const PRUnichar* const() {return mStringValue.mBuffer;}

  PRUint32  Length() const {return mStringValue.mLength;}  
  
  size_t    GetCharSize() const {return sizeof(mStringValue.mBuffer[0]);}

  PRBool    IsUnicode() const {return PR_FALSE;}
  
  PRBool    IsEmpty(void) const {return PRBool(mStringValue.mLength==0);}

  char CharAt(PRUint32 anIndex) const {
    return (anIndex<mStringValue.mLength) ? mStringValue.mBuffer[anIndex] : 0;
  }

  PRBool SetCharAt(char aChar,PRUint32 anIndex);

  char First(void) const {
    return CharAt(0);
  } 
  
  char Last(void) const {
    return CharAt(mStringValue.mLength-1);
  }


    //these aren't the real deal, but serve as a placeholder for us to implement iterators.
  virtual nsAReadableStringIterator* First();
  virtual nsAReadableStringIterator* Last();


  //******************************************
  // Here are the Assignment methods, 
  // and other mutators...
  //******************************************

  virtual nsresult Truncate(PRUint32 anOffset=0);

  /*
   *  This method assign from a stringimpl 
   *  string at aString[anOffset].
   *  
   *  @update  rickg 03.01.2000
   *  @param  aString -- source String to be inserted into this
   *  @param  aLength -- number of PRUnichars to be copied from aCopy
   *  @param  aSrcOffset -- insertion position within this str
   *  @return this
   */
  virtual nsresult Assign(const nsCString& aString,PRInt32 aLength=-1,PRUint32 aSrcOffset=0);

  virtual nsresult Assign(const char* aString,PRInt32 aLength=-1,PRUint32 aSrcOffset=0);

//  virtual nsresult Assign(const PRUnichar* aString,PRInt32 aLength=-1,PRUint32 aSrcOffset=0);

  virtual nsresult Assign(const nsAReadableString &aString,PRInt32 aLength=-1,PRUint32 aSrcOffset=0);

  virtual nsresult Assign(char aChar);

  /**
   * Functionally equivalent to assign or operator=, and deprePRUnichared!
   * 
   */
  nsresult SetString(const nsCString &aString,PRInt32 aLength=-1) {return Assign(aString,aLength);}
  nsresult SetString(const PRUnichar* aString,PRInt32 aLength=-1) {return Assign(aString,aLength);}
  nsresult SetString(const char* aString,PRInt32 aLength=-1) {return Assign(aString,aLength);}

    //***************************************
    //  Here come the append methods...
    //***************************************

  /*
   *  This method appends a stringimpl starting at aString[aSrcOffset]
   *  
   *  @update  rickg 03.01.2000
   *  @param  aString -- source String to be inserted into this
   *  @param  aLength -- number of PRUnichars to be copied from aCopy
   *  @param  aSrcOffset -- insertion position within this str
   *  @return this
   */
  virtual nsresult Append(const nsCString &aString,PRInt32 aLength=-1,PRUint32 aSrcOffset=0);

  virtual nsresult Append(const nsString &aString,PRInt32 aLength=-1,PRUint32 aSrcOffset=0);

  virtual nsresult Append(const char* aString,PRInt32 aLength=-1,PRUint32 aSrcOffset=0);

//  virtual nsresult Append(const PRUnichar* aString,PRInt32 aLength=-1,PRUint32 aSrcOffset=0);

  virtual nsresult Append(const char aChar) ;

  virtual nsresult Append(const PRUnichar aChar);

  virtual nsresult Append(const nsAReadableString &aString,PRInt32 aLength=-1,PRUint32 aSrcOffset=0);

  virtual nsresult Append(PRInt32 anInteger,PRInt32 aRadix=10);

  virtual nsresult Append(float aFloat);


    //***************************************
    //  Here come a few deletion methods...
    //***************************************

  nsresult Cut(PRUint32 anOffset,PRInt32 aCount);

  nsresult Trim(const char* aTrimSet,PRBool aEliminateLeading=PR_TRUE,PRBool aEliminateTrailing=PR_TRUE,PRBool aIgnoreQuotes=PR_FALSE);

  /**
   *  This method strips PRUnichars in given set from string.
   *  You can control whether PRUnichars are yanked from
   *  start and end of string as well.
   *  
   * @update  rickg 03.01.2000
   *  @param   aEliminateLeading controls stripping of leading ws
   *  @param   aEliminateTrailing controls stripping of trailing ws
   *  @return  this
   */
  nsresult CompressSet(const char* aSet, char aChar,PRBool aEliminateLeading=PR_TRUE,PRBool aEliminateTrailing=PR_TRUE);

  /**
   *  This method compresses whitespace within a string. Multiple ws in a row get compressed 1.
   *  You can also control whether whitespace is yanked from each end.
   *  
   *  @update  rickg 03.01.2000
   *  @param   aEliminateLeading controls stripping of leading ws
   *  @param   aEliminateTrailing controls stripping of trailing ws
   *  @return  this
   */
  nsresult CompressWhitespace( PRBool aEliminateLeading=PR_TRUE,PRBool aEliminateTrailing=PR_TRUE);


    //***************************************
    //  Here come a wad of insert methods...
    //***************************************

  /*
   *  This method inserts n PRUnichars from given string into this
   *  string at str[anOffset].
   *  
   *  @update  rickg 03.01.2000
   *  @param  aString -- source String to be inserted into this
   *  @param  anOffset -- insertion position within this str
   *  @param  aCount -- number of chars to be copied from aCopy
   *  @return this
   */
  nsresult Insert(const nsCString& aString,PRUint32 anOffset=0,PRInt32 aCount=-1);

  nsresult Insert(const char* aString,PRUint32 anOffset=0,PRInt32 aLength=-1);

  nsresult Insert(char aChar,PRUint32 anOffset=0);

  
    //*******************************************
    //  Here come inplace replacement  methods...
    //*******************************************

  /**
   *  swaps occurence of target for another
   *  
   *  @return  this
   */
  nsresult ReplaceSubstring( const nsCString& aTarget,const nsCString& aNewValue);

  nsresult ReplaceChar(char anOldChar, char aNewChar);

  nsresult ReplaceChar(const char* aSet,char aNewPRUnichar);

  nsresult ReplaceSubstring(const char* aTarget,const char* aNewValue);


  //*******************************************
    //  Here come the stripPRUnichar methods...
    //*******************************************

  /**
   *  This method is used to remove all occurances of the
   *  given char(s) from this string.
   *  
   *  @update  rickg 03.01.2000
   *  @param  aChar -- PRUnichar to be stripped
   *  @param  anOffset -- where in this string to start stripping PRUnichars
   *  @return *this 
   */
  nsresult StripChar(char aChar,PRUint32 anOffset=0);

  nsresult StripChar(PRInt32 anInt,PRUint32 anOffset=0);

  nsresult StripChars(const char* aSet);

  nsresult StripWhitespace();

    //**************************************************
    //  Here are some methods that extract substrings...
    //**************************************************

  nsresult Left(nsCString& aCopy,PRInt32 aCount) const;

  nsresult Mid(nsCString& aCopy,PRUint32 anOffset,PRInt32 aCount) const;

  nsresult Right(nsCString& aCopy,PRInt32 aCount) const;


    //*******************************************
    //  Here come the operator+=() methods...
    //*******************************************

  nsCString& operator+=(const nsCString& aString);

  nsCString& operator+=(const PRUnichar* aString);
  
  nsCString& operator+=(const char* aString);
  
  nsCString& operator+=(const PRUnichar aChar);

  nsCString& operator+=(const char aChar);
  
  nsCString& operator+=(const int anInt);


  /***********************************
    Comparison methods...
   ***********************************/

  PRBool operator==(const nsCString& aString) const {return Equals(aString);}      
  PRBool operator==(const PRUnichar* aString) const {return Equals(aString);}
  PRBool operator==(const char* aString) const {return Equals(aString);}

  PRBool operator!=(const nsCString& aString) const {return PRBool(Compare(aString)!=0);}
  PRBool operator!=(const PRUnichar* aString) const {return PRBool(Compare(aString)!=0);}
  PRBool operator!=(const char* aString) const {return PRBool(Compare(aString)!=0);}

  PRBool operator<(const nsCString& aString) const {return PRBool(Compare(aString)<0);}
  PRBool operator<(const PRUnichar* aString) const {return PRBool(Compare(aString)<0);}
  PRBool operator<(const char* aString) const {return PRBool(Compare(aString)<0);}

  PRBool operator>(const nsCString& aString) const {return PRBool(Compare(aString)>0);}
  PRBool operator>(const PRUnichar* aString) const {return PRBool(Compare(aString)>0);}
  PRBool operator>(const char* aString) const {return PRBool(Compare(aString)>0);}

  PRBool operator<=(const nsCString& aString) const {return PRBool(Compare(aString)<=0);}
  PRBool operator<=(const PRUnichar* aString) const {return PRBool(Compare(aString)<=0);}
  PRBool operator<=(const char* aString) const {return PRBool(Compare(aString)<=0);}

  PRBool operator>=(const nsCString& aString) const {return PRBool(Compare(aString)>=0);}
  PRBool operator>=(const PRUnichar* aString) const {return PRBool(Compare(aString)>=0);}
  PRBool operator>=(const char* aString) const {return PRBool(Compare(aString)>=0);}


  PRInt32  Compare(const nsCString &aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 aCount=-1) const;

  PRInt32  Compare(const PRUnichar* aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 aCount=-1) const;
  
  PRInt32  Compare(const char* aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 aCount=-1) const ;

  PRBool  Equals(const nsCString &aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 aCount=-1) const;

  PRBool  Equals(const nsString &aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 aCount=-1) const;

  PRBool  Equals(const PRUnichar* aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 aCount=-1) const;

  PRBool  Equals(const char* aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 aCount=-1) const;

  PRBool  EqualsIgnoreCase(const nsCString &aString) const {return Equals(aString,PR_TRUE);}
  PRBool  EqualsIgnoreCase(const PRUnichar* aString,PRInt32 aCount=-1) const {return Equals(aString,PR_TRUE);}
  PRBool  EqualsIgnoreCase(const char* s1, const char* s2) const;

  // PRBool  Equals(/*FIX: const */nsIAtom* anAtom,PRBool aIgnoreCase) const;   


  /***************************************
    These are string searching methods...
   ***************************************/

  /**
   *  search for given string within this string
   *  
   *  @update  rickg 03.01.2000
   *  @param   aString - substr to be found
   *  @param   aIgnoreCase tells us whether or not to do caseless compare
   *  @param   anOffset tells us where in this string to start searching
   *  @param   aRepCount tells us how many iterations to make starting at the given offset
   *  @return  offset in string, or -1 (kNotFound)
   */
  PRInt32 Find(const nsCString& aTarget,PRBool aIgnoreCase=PR_FALSE,PRUint32 anOffset=0,PRInt32 aRepCount=-1) const;
  PRInt32 Find(const nsString& aTarget,PRBool aIgnoreCase=PR_FALSE,PRUint32 anOffset=0,PRInt32 aRepCount=-1) const;
  PRInt32 Find(const PRUnichar* aString,PRBool aIgnoreCase=PR_FALSE,PRUint32 anOffset=0,PRInt32 aRepCount=-1) const;
  PRInt32 Find(const char* aString,PRBool aIgnoreCase=PR_FALSE,PRUint32 anOffset=0,PRInt32 aRepCount=-1) const;
  PRInt32 FindChar(char aChar,PRBool aIgnoreCase=PR_FALSE,PRInt32 anOffset=-1,PRInt32 aRepCount=-1) const ;
  
  PRInt32 FindCharInSet(const nsCString& aString,PRUint32 anOffset=0) const;
  PRInt32 FindCharInSet(const nsString& aString,PRUint32 anOffset=0) const;
  PRInt32 FindCharInSet(const char *aString,PRUint32 anOffset=0) const;

  PRInt32 RFind(const nsCString& aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 anOffset=-1,PRInt32 aRepCount=-1) const;
  PRInt32 RFind(const char* aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 anOffset=-1,PRInt32 aRepCount=-1) const;
  PRInt32 RFindChar(char aChar,PRBool aIgnoreCase=PR_FALSE,PRInt32 anOffset=-1,PRInt32 aRepCount=-1) const;

  PRInt32 RFindCharInSet(const nsCString& aString,PRInt32 anOffset=-1) const ;
  PRInt32 RFindCharInSet(const char* aString,PRInt32 anOffset=-1) const;

  /***************************************
    These convert to a different type
   ***************************************/

  static void  Recycle(nsCString* aString);

  static nsCString* CreateString(void);

  nsCString* ToNewString() const;

  char* ToNewCString() const;

  char* ToNewUTF8String() const;

  PRUnichar* ToNewUnicode() const;

  char* ToCString(char* aBuf,PRUint32 aBufLength,PRUint32 anOffset=0) const;


  //******************************************
  // Utility methods
  //******************************************

  PRInt32 CountChar(char aChar);
  
    //This will not work correctly for any unicode set other than ascii
  void DebugDump(void) const;

  /**
   * Perform string to float conversion.
   * @param   aErrorCode will contain error if one occurs
   * @return  float rep of string value
   */
  float ToFloat(PRInt32* aErrorCode) const;

  /**
   * Perform string to int conversion.
   * @param   aErrorCode will contain error if one occurs
   * @param   aRadix tells us which radix to assume; kAutoDetect tells us to determine the radix for you.
   * @return  int rep of string value, and possible (out) error code
   */
  PRInt32 ToInteger(PRInt32* anErrorCode,PRUint32 aRadix=kAutoDetect) const;

  void ToLowerCase();

  void ToLowerCase(nsCString &aString) const;

  void ToUpperCase();

  void ToUpperCase(nsCString &aString) const;


protected:
  
  nsStringValueImpl<char>  mStringValue;

  friend class nsString;
};



/*****************************************************************
  Now we declare the nsCAutoString class
 *****************************************************************/


class nsCAutoString : public nsCString {
public:
  
  nsCAutoString() : nsCString() {
    memset(mInternalBuffer,0,sizeof(mInternalBuffer));
    mStringValue.mCapacity=kDefaultStringSize;
    mStringValue.mBuffer=mInternalBuffer;
  }
  
    //call this version nsAutoString derivatives...
  nsCAutoString(const nsCAutoString& aString,PRInt32 aLength=-1) : nsCString() {
    memset(mInternalBuffer,0,sizeof(mInternalBuffer));
    mStringValue.mCapacity=kDefaultStringSize;
    mStringValue.mBuffer=mInternalBuffer;
    Assign(aString,aLength);
  }

  //call this version for nsCString,nsCString and the autostrings
  nsCAutoString(const nsCString& aString,PRInt32 aLength=-1) : nsCString() {
    memset(mInternalBuffer,0,sizeof(mInternalBuffer));
    mStringValue.mCapacity=kDefaultStringSize;
    mStringValue.mBuffer=mInternalBuffer;
    Assign(aString,aLength);
  }

    //call this version for char*'s....
  nsCAutoString(const char* aString,PRInt32 aLength=-1) : nsCString() {
    memset(mInternalBuffer,0,sizeof(mInternalBuffer));

    nsCString theString(aString);
    mStringValue.mCapacity=kDefaultStringSize;
    mStringValue.mBuffer=mInternalBuffer;
    Assign(theString,aLength);
  }

    //call this version for a single char of type char...
  nsCAutoString(const char aChar) : nsCString() {
    char theBuffer[]={aChar,0};

    memset(mInternalBuffer,0,sizeof(mInternalBuffer));
    mStringValue.mCapacity=kDefaultStringSize;
    mStringValue.mBuffer=mInternalBuffer;

    nsCString theString(theBuffer,1);
    Assign(theString,1,0);
  }


    //call this version for PRUnichar*'s....
  nsCAutoString(const PRUnichar* aString,PRInt32 aLength=-1) : nsCString() {
    memset(mInternalBuffer,0,sizeof(mInternalBuffer));
    mStringValue.mCapacity=kDefaultStringSize;
    mStringValue.mBuffer=mInternalBuffer;
    nsStringValueImpl<PRUnichar> theString(const_cast<PRUnichar*>(aString),aLength);
    SVAssign<char,PRUnichar>(mStringValue,theString,theString.mLength,0);
  }


    //call this version for all other ABT versions of readable strings
  nsCAutoString(const nsAReadableString &aString) : nsCString() {
    memset(mInternalBuffer,0,sizeof(mInternalBuffer));
    mStringValue.mCapacity=kDefaultStringSize;
    mStringValue.mBuffer=mInternalBuffer;
    Assign(aString);
  }

  nsCAutoString(const nsStackBuffer<char> &aBuffer) : nsCString() {

    memset(mInternalBuffer,0,sizeof(mInternalBuffer));

    mStringValue.mRefCount=2;
    mStringValue.mLength=aBuffer.mLength;
    mStringValue.mCapacity=aBuffer.mCapacity;
    mStringValue.mBuffer=aBuffer.mBuffer;
    
  }


  virtual ~nsCAutoString() { }

  
  nsCAutoString& operator=(const nsCAutoString& aCopy) {
    if(aCopy.mStringValue.mBuffer!=mStringValue.mBuffer) {
      Assign(aCopy);
    }
    return *this;
  }
  
  nsCAutoString& operator=(const nsCString& aString) {
    Assign(aString);
    return *this;
  }

  nsCAutoString& operator=(const char* aString) {
    if(mStringValue.mBuffer!=aString) {
      nsCString theStringValue(const_cast<char*>(aString));
      Assign(aString);
    }
    return *this;
  } 

  nsCAutoString& operator=(const PRUnichar* aString) {
    nsStringValueImpl<PRUnichar> theStringValue(const_cast<PRUnichar*>(aString));
    SVAssign<char,PRUnichar>(mStringValue,theStringValue,theStringValue.mLength,0);
    return *this;
  }

  nsCAutoString& operator=(const char aChar) {
    Assign(aChar);
    return *this;
  } 

protected:
  char  mInternalBuffer[kDefaultStringSize+1];
};


#endif

