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
 *   Rick Gessner <rickg@netscape.com> (original author)
 *   Scott Collins <scc@mozilla.org>
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

/* nsStr.h --- the underlying buffer for rickg's original string implementations;
    these classes will be replaced by the new shared-buffer string (see bug #53065)
 */

#ifndef _nsStr
#define _nsStr

#ifndef nsStringDefines_h___
#include "nsStringDefines.h"
#endif


/***********************************************************************
  MODULE NOTES:

    1. There are two philosophies to building string classes:
        A. Hide the underlying buffer & offer API's allow indirect iteration
        B. Reveal underlying buffer, risk corruption, but gain performance
        
       We chose the option B for performance reasons. 

    2  Our internal buffer always holds capacity+1 bytes.

    The nsStr struct is a simple structure (no methods) that contains
    the necessary info to be described as a string. This simple struct
    is manipulated by the static methods provided in this class. 
    (Which effectively makes this a library that works on structs).

    There are also object-based versions called nsString and nsAutoString
    which use nsStr but makes it look at feel like an object. 

 ***********************************************************************/

/***********************************************************************
  ASSUMPTIONS:

    1. nsStrings and nsAutoString are always null terminated. However,
       since it maintains a length byte, you can store NULL's inside
       the string. Just be careful passing such buffers to 3rd party
       API's that assume that NULL always terminate the buffer.

    2. nsCStrings can be upsampled into nsString without data loss

    3. Char searching is faster than string searching. Use char interfaces
       if your needs will allow it.

    4. It's easy to use the stack for nsAutostring buffer storage (fast too!).
       See the CBufDescriptor class in this file.

    5. If you don't provide the optional count argument to Append() and Insert(),
       the method will assume that the given buffer is terminated by the first
       NULL it encounters.

    6. Downsampling from nsString to nsCString can be lossy -- avoid it if possible!

    7. Calls to ToNewCString() and ToNewUnicode() should be matched with calls to nsMemory::Free().

 ***********************************************************************/


/**********************************************************************************
  
 AND NOW FOR SOME GENERAL DOCUMENTATION ON STRING USAGE...

    The fundamental datatype in the string library is nsStr. It's a structure that
    provides the buffer storage and meta-info. It also provides a C-style library
    of functions for direct manipulation (for those of you who prefer K&R to Bjarne). 

    Here's a diagram of the class hierarchy: 

      nsStr
        |___nsString
        |      |
        |      ------nsAutoString
        |
        |___nsCString
               |
               ------nsCAutoString

    Why so many string classes? The 4 variants give you the control you need to 
    determine the best class for your purpose. There are 2 dimensions to this 
    flexibility: 1) stack vs. heap; and 2) 1-byte chars vs. 2-byte chars.

    Note: While nsAutoString and nsCAutoString begin life using stack-based storage,
          they may not stay that way. Like all nsString classes, autostrings will
          automatically grow to contain the data you provide. When autostrings
          grow beyond their intrinsic buffer, they switch to heap based allocations.
          (We avoid alloca to avoid considerable platform difficulties; see the 
           GNU documentation for more details).

    I should also briefly mention that all the string classes use a "memory agent" 
    object to perform memory operations. This class proxies the standard nsMemory
    for actual memory calls, but knows the structure of nsStr making heap operations
    more localized.
    

  CHOOSING A STRING CLASS:

    In order to choose a string class for you purpose, use this handy table:

                        heap-based    stack-based
                    -----------------------------------
        ascii data  |   nsCString     nsCAutoString   |
                    |----------------------------------
      unicode data  |    nsString      nsAutoString   |
                    ----------------------------------- 
    
  
    Note: The i18n folks will stenuously object if we get too carried away with the
          use of nsCString's that pass interface boundaries. Try to limit your
          use of these to external interfaces that demand them, or for your own
          private purposes in cases where they'll never be seen by humans. 

    
  --- FAQ ---

    Q. When should I use nsCString instead of nsString?

    A. You should really try to stick with nsString, so that we stay as unicode
       compliant as possible. But there are cases where an interface you use requires
       a char*. In such cases, it's fair to use nsCString. 

    Q. I know that my string is going to be a certain size. Can I pre-size my nsString?

    A. Yup, here's how:

         {
           nsString mBuffer;
           mBuffer.SetCapacity(aReasonableSize);
         }

    Q. Should nsAutoString or nsCAutoString ever live on the heap?  

    A. That would be counterproductive. The point of nsAutoStrings is to preallocate your
       buffers, and to auto-destroy the string when it goes out of scope. 

    Q. I already have a char*. Can I use the nsString functionality on that buffer?

    A. Yes you can -- by using an intermediate class called CBufDescriptor.
       The CBufDescriptor class is used to tell nsString about an external buffer (heap or stack) to use
       instead of it's own internal buffers. Here's an example:
         
         {
           char theBuffer[256];
           CBufDescritor theBufDecriptor( theBuffer, PR_TRUE, sizeof(theBuffer), 0);
           nsCAutoString s3( theBufDescriptor );
           s3="HELLO, my name is inigo montoya, you killed my father, prepare to die!.";
         }

         The assignment statment to s3 will cause the given string to be written to your
         stack-based buffer via the normal nsString/nsCString interfaces. Cool, huh?  
         Note however that just like any other nsStringXXX use, if you write more data 
         than will fit in the buffer, a visit to the heap manager will be in order. 
     

    Q. What is the simplest way to get from a char* to PRUnichar*?
    
    A. The simplest way is by construction (or assignment): 

        {
          char* theBuf = "hello there";
          nsAutoString foo(theBuf);
        }

       If you don't want the char* to be copied into the nsAutoString, the use a 
       CBufDescriptor instead.


 **********************************************************************************/


#include "nscore.h"
#include "nsMemory.h"
#include <string.h>
#include <stdio.h>
#include "plhash.h"
#include "nsStrShared.h"

//----------------------------------------------------------------------------------------
#define kDefaultCharSize eTwoByte
#define kRadix10        (10)
#define kRadix16        (16)
#define kAutoDetect     (100)
#define kRadixUnknown   (kAutoDetect+1)
#define IGNORE_CASE     (PR_TRUE)

#define NSSTR_CHARSIZE_BIT   (31)
#define NSSTR_OWNSBUFFER_BIT (30)

#define NSSTR_CHARSIZE_MASK  (1<<NSSTR_CHARSIZE_BIT)
#define NSSTR_OWNSBUFFER_MASK (1<<NSSTR_OWNSBUFFER_BIT)

#define NSSTR_CAPACITY_MASK (~(NSSTR_CHARSIZE_MASK | NSSTR_OWNSBUFFER_MASK))

const   PRInt32 kDefaultStringSize = 64;
const   PRInt32 kNotFound = -1;


//----------------------------------------------------------------------------------------
class nsAString;
class NS_COM CBufDescriptor {
public:
  CBufDescriptor(char* aString,           PRBool aStackBased,PRUint32 aCapacity,PRInt32 aLength=-1);
  CBufDescriptor(const char* aString,     PRBool aStackBased,PRUint32 aCapacity,PRInt32 aLength=-1);
  CBufDescriptor(PRUnichar* aString,      PRBool aStackBased,PRUint32 aCapacity,PRInt32 aLength=-1);
  CBufDescriptor(const PRUnichar* aString,PRBool aStackBased,PRUint32 aCapacity,PRInt32 aLength=-1);

  char*     mBuffer;
  eCharSize mCharSize;
  PRUint32  mCapacity;
  PRInt32   mLength;
  PRBool    mStackBased;
  PRBool    mIsConst;
};

//----------------------------------------------------------------------------------------


struct nsStr {  

protected:
  nsStr() {
#ifdef TRACE_STRINGS
    MOZ_COUNT_CTOR(nsStr);
#endif
  }

  ~nsStr() {
#ifdef TRACE_STRINGS
    MOZ_COUNT_DTOR(nsStr);
#endif
  }

protected:
  union { 
    char*         mStr;
    PRUnichar*    mUStr;
  };

  PRUint32        mLength;
  PRUint32        mCapacityAndFlags;
  
  inline PRUint32 GetCapacity() const {
    // actual capacity is one less than is stored
    return (mCapacityAndFlags & NSSTR_CAPACITY_MASK);
  }

  inline PRBool GetOwnsBuffer() const {
    return ((mCapacityAndFlags & NSSTR_OWNSBUFFER_MASK) != 0);
  }

  inline eCharSize GetCharSize() const {
    return eCharSize(mCapacityAndFlags >> NSSTR_CHARSIZE_BIT);
  }

private:

  inline void SetInternalCapacity(PRUint32 aCapacity) {
    mCapacityAndFlags = 
      ((mCapacityAndFlags & ~NSSTR_CAPACITY_MASK) |
       (aCapacity         & NSSTR_CAPACITY_MASK));
  }

  inline void SetCharSize(eCharSize aCharSize) {
    mCapacityAndFlags =
      ((mCapacityAndFlags & ~NSSTR_CHARSIZE_MASK) |
       (PRUint32(aCharSize) << NSSTR_CHARSIZE_BIT));
  }

  inline void SetOwnsBuffer(PRBool aOwnsBuffer) {
    mCapacityAndFlags =
      (mCapacityAndFlags & ~NSSTR_OWNSBUFFER_MASK |
       (aOwnsBuffer ? 1 : 0) << NSSTR_OWNSBUFFER_BIT);
  }

  inline PRUnichar GetCharAt(PRUint32 anIndex) const {
    if(anIndex<mLength)  {
      return (eTwoByte==GetCharSize()) ? mUStr[anIndex] : (PRUnichar)mStr[anIndex];
    }//if
    return 0;
  }
  
public:
  friend NS_COM
  char*
  ToNewUTF8String( const nsAString& aSource );
  friend inline void AddNullTerminator(nsStr& aDest);
  friend class nsStrPrivate;
  friend class nsString;
  friend class nsCString;
};




/**************************************************************
  A couple of tiny helper methods used in the string classes.
 **************************************************************/

inline PRInt32 MinInt(PRInt32 anInt1,PRInt32 anInt2){
  return (anInt1<anInt2) ? anInt1 : anInt2;
}

inline PRInt32 MaxInt(PRInt32 anInt1,PRInt32 anInt2){
  return (anInt1<anInt2) ? anInt2 : anInt1;
}

inline void AddNullTerminator(nsStr& aDest) {
  if(eTwoByte==aDest.GetCharSize()) 
    aDest.mUStr[aDest.mLength]=0;
  else aDest.mStr[aDest.mLength]=0;
}

/**
 * Deprecated: don't use |Recycle|, just call |nsMemory::Free| directly
 *
 * Return the given buffer to the heap manager. Calls allocator::Free()
 * @return string length
 */
inline void Recycle( char* aBuffer) { nsMemory::Free(aBuffer); }
inline void Recycle( PRUnichar* aBuffer) { nsMemory::Free(aBuffer); }

#ifdef NS_STR_STATS

class nsStringInfo {
public:
  nsStringInfo(nsStr& str);
  ~nsStringInfo() {}

  static nsStringInfo* GetInfo(nsStr& str);

  static void Seen(nsStr& str);

  static void Report(FILE* out = stdout);
  
  static PRIntn ReportEntry(PLHashEntry *he, PRIntn i, void *arg);

protected:
  nsStr         mStr;
  PRUint32      mCount;
};

#define NSSTR_SEEN(str) nsStringInfo::Seen(str)

#else // !NS_STR_STATS

#define NSSTR_SEEN(str) /* nothing */

#endif // !NS_STR_STATS

#endif // _nsStr
