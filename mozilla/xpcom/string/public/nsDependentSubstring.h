/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Scott Collins <scc@mozilla.org> (original author)
 */

#ifndef nsDependentSubstring_h___
#define nsDependentSubstring_h___

#ifndef nsAString_h___
#include "nsAString.h"
#endif

#ifndef nsASingleFragmentString_h___
#include "nsASingleFragmentString.h"
#endif

#ifndef nsStringTraits_h___
#include "nsStringTraits.h"
#endif



  //
  // nsDependentSubstring
  //

class NS_COM nsDependentSubstring
    : public nsAString
    /*
      NOT FOR USE BY HUMANS (mostly)

      ...not unlike |nsDependentConcatenation|.  Instances of this class exist only as anonymous
      temporary results from |Substring()|.  Like |nsDependentConcatenation|, this class only
      holds a pointer, no string data of its own.  It does its magic by overriding and forwarding
      calls to |GetReadableFragment()|.
    */
  {
    public:
      typedef nsDependentSubstring self_type;

    protected:
      virtual const char_type* GetReadableFragment( const_fragment_type&, nsFragmentRequest, PRUint32 ) const;
      virtual       char_type* GetWritableFragment(       fragment_type&, nsFragmentRequest, PRUint32 ) { return 0; }

    public:
      nsDependentSubstring( const abstract_string_type& aString, PRUint32 aStartPos, PRUint32 aLength );
      nsDependentSubstring( const const_iterator& aStart, const const_iterator& aEnd );

      // nsDependentSubstring( const self_type& ); // auto-generated copy-constructor should be OK
      // ~nsDependentSubstring();                           // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const self_type& );        // we're immutable, you can't assign into a substring

    public:
      virtual PRUint32 Length() const;

    private:
      const abstract_string_type&  mString;
      PRUint32                     mStartPos;
      PRUint32                     mLength;
  };

class NS_COM nsDependentCSubstring
    : public nsACString
    /*
      NOT FOR USE BY HUMANS (mostly)

      ...not unlike |nsDependentConcatenation|.  Instances of this class exist only as anonymous
      temporary results from |Substring()|.  Like |nsDependentConcatenation|, this class only
      holds a pointer, no string data of its own.  It does its magic by overriding and forwarding
      calls to |GetReadableFragment()|.
    */
  {
    public:
      typedef nsDependentCSubstring self_type;

    protected:
      virtual const char_type* GetReadableFragment( const_fragment_type&, nsFragmentRequest, PRUint32 ) const;
      virtual       char_type* GetWritableFragment(       fragment_type&, nsFragmentRequest, PRUint32 ) { return 0; }

    public:
      nsDependentCSubstring( const abstract_string_type& aString, PRUint32 aStartPos, PRUint32 aLength );
      nsDependentCSubstring( const const_iterator& aStart, const const_iterator& aEnd );

      // nsDependentCSubstring( const self_type& ); // auto-generated copy-constructor should be OK
      // ~nsDependentCSubstring();                            // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const self_type& );         // we're immutable, you can't assign into a substring

    public:
      virtual PRUint32 Length() const;

    private:
      const abstract_string_type&  mString;
      PRUint32                     mStartPos;
      PRUint32                     mLength;
  };


class NS_COM nsDependentSingleFragmentSubstring
      : public nsASingleFragmentString
  {
    public:
      typedef nsDependentSingleFragmentSubstring self_type;
      typedef nsASingleFragmentString   abstract_single_fragment_type;

      void
      Rebind( const char_type* aStartPtr, const char_type* aEndPtr );

      void
      Rebind( const abstract_single_fragment_type& aString, const PRUint32 aStartPos, const PRUint32 aLength );

      nsDependentSingleFragmentSubstring( const char_type* aStartPtr, const char_type* aEndPtr ) { Rebind(aStartPtr, aEndPtr); }
      nsDependentSingleFragmentSubstring( const abstract_single_fragment_type& aString, const PRUint32 aStartPos, const PRUint32 aLength ) { Rebind(aString, aStartPos, aLength); }

      // nsDependentSingleFragmentSubstring( const self_type& );                  // auto-generated copy-constructor OK
      // ~nsDependentSingleFragmentSubstring();                                   // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const self_type& );                                       // we're immutable, so no copy-assignment operator

    public:
      virtual const buffer_handle_type* GetBufferHandle() const                 { return NS_REINTERPRET_CAST(const buffer_handle_type*, &mHandle); }

    private:
      const_buffer_handle_type mHandle;
  };


class NS_COM nsDependentSingleFragmentCSubstring
      : public nsASingleFragmentCString
  {
    public:
      typedef nsDependentSingleFragmentCSubstring self_type;
      typedef nsASingleFragmentCString  abstract_single_fragment_type;

      void
      Rebind( const char_type* aStartPtr, const char_type* aEndPtr );

      void
      Rebind( const abstract_single_fragment_type& aString, const PRUint32 aStartPos, const PRUint32 aLength );

      nsDependentSingleFragmentCSubstring( const char_type* aStartPtr, const char_type* aEndPtr ) { Rebind(aStartPtr, aEndPtr); }
      nsDependentSingleFragmentCSubstring( const abstract_single_fragment_type& aString, const PRUint32 aStartPos, const PRUint32 aLength ) { Rebind(aString, aStartPos, aLength); }

      // nsDependentSingleFragmentCSubstring( const self_type& );                  // auto-generated copy-constructor OK
      // ~nsDependentSingleFragmentCSubstring();                                   // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const self_type& );                                       // we're immutable, so no copy-assignment operator

    public:
      virtual const buffer_handle_type* GetBufferHandle() const                 { return NS_REINTERPRET_CAST(const buffer_handle_type*, &mHandle); }

    private:
      const_buffer_handle_type mHandle;
  };



inline
const nsDependentCSubstring
Substring( const nsACString& aString, PRUint32 aStartPos )
  {
    return nsDependentCSubstring(aString, aStartPos, aString.Length() - aStartPos);
  }

inline
const nsDependentSubstring
Substring( const nsAString& aString, PRUint32 aStartPos )
  {
    return nsDependentSubstring(aString, aStartPos, aString.Length() - aStartPos);
  }

inline
const nsDependentCSubstring
Substring( const nsACString& aString, PRUint32 aStartPos, PRUint32 aSubstringLength )
  {
    return nsDependentCSubstring(aString, aStartPos, aSubstringLength);
  }

inline
const nsDependentSubstring
Substring( const nsAString& aString, PRUint32 aStartPos, PRUint32 aSubstringLength )
  {
    return nsDependentSubstring(aString, aStartPos, aSubstringLength);
  }

inline
const nsDependentCSubstring
Substring( const nsACString::const_iterator& aStart, const nsACString::const_iterator& aEnd )
  {
    return nsDependentCSubstring(aStart, aEnd);
  }

inline
const nsDependentSubstring
Substring( const nsAString::const_iterator& aStart, const nsAString::const_iterator& aEnd )
  {
    return nsDependentSubstring(aStart, aEnd);
  }

inline
const nsDependentSingleFragmentCSubstring
Substring( const nsASingleFragmentCString& aString, PRUint32 aStartPos )
  {
    return nsDependentSingleFragmentCSubstring(aString, aStartPos, aString.Length() - aStartPos);
  }

inline
const nsDependentSingleFragmentSubstring
Substring( const nsASingleFragmentString& aString, PRUint32 aStartPos )
  {
    return nsDependentSingleFragmentSubstring(aString, aStartPos, aString.Length() - aStartPos);
  }

inline
const nsDependentSingleFragmentCSubstring
Substring( const nsASingleFragmentCString& aString, PRUint32 aStartPos, PRUint32 aSubstringLength )
  {
    return nsDependentSingleFragmentCSubstring(aString, aStartPos, aSubstringLength);
  }

inline
const nsDependentSingleFragmentSubstring
Substring( const nsASingleFragmentString& aString, PRUint32 aStartPos, PRUint32 aSubstringLength )
  {
    return nsDependentSingleFragmentSubstring(aString, aStartPos, aSubstringLength);
  }

inline
const nsDependentSingleFragmentCSubstring
Substring( const nsASingleFragmentCString::const_char_iterator& aStart, const nsASingleFragmentCString::const_char_iterator& aEnd )
  {
    return nsDependentSingleFragmentCSubstring(aStart, aEnd);
  }

inline
const nsDependentSingleFragmentSubstring
Substring( const nsASingleFragmentString::const_char_iterator& aStart, const nsASingleFragmentString::const_char_iterator& aEnd )
  {
    return nsDependentSingleFragmentSubstring(aStart, aEnd);
  }

inline
const nsDependentCSubstring
StringHead( const nsACString& aString, PRUint32 aCount )
  {
    return nsDependentCSubstring(aString, 0, aCount);
  }

inline
const nsDependentSubstring
StringHead( const nsAString& aString, PRUint32 aCount )
  {
    return nsDependentSubstring(aString, 0, aCount);
  }

inline
const nsDependentSingleFragmentCSubstring
StringHead( const nsASingleFragmentCString& aString, PRUint32 aCount )
  {
    return nsDependentSingleFragmentCSubstring(aString, 0, aCount);
  }

inline
const nsDependentSingleFragmentSubstring
StringHead( const nsASingleFragmentString& aString, PRUint32 aCount )
  {
    return nsDependentSingleFragmentSubstring(aString, 0, aCount);
  }

inline
const nsDependentCSubstring
StringTail( const nsACString& aString, PRUint32 aCount )
  {
    return nsDependentCSubstring(aString, aString.Length() - aCount, aCount);
  }

inline
const nsDependentSubstring
StringTail( const nsAString& aString, PRUint32 aCount )
  {
    return nsDependentSubstring(aString, aString.Length() - aCount, aCount);
  }

inline
const nsDependentSingleFragmentCSubstring
StringTail( const nsASingleFragmentCString& aString, PRUint32 aCount )
  {
    return nsDependentSingleFragmentCSubstring(aString, aString.Length() - aCount, aCount);
  }

inline
const nsDependentSingleFragmentSubstring
StringTail( const nsASingleFragmentString& aString, PRUint32 aCount )
  {
    return nsDependentSingleFragmentSubstring(aString, aString.Length() - aCount, aCount);
  }

#endif /* !defined(nsDependentSubstring_h___) */
