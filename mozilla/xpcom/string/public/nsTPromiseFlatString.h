/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
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
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by IBM Corporation are Copyright (C) 2003
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
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

#ifndef nsTPromiseFlatString_h___
#define nsTPromiseFlatString_h___

#ifndef nsTString_h___
#include "nsTString.h"
#endif

  /**
   * XXX fix docs XXX
   *
   * WARNING:
   *
   * Try to avoid flat strings.  |PromiseFlat[C]String| will help you as a last resort,
   * and this may be necessary when dealing with legacy or OS calls, but in general,
   * requiring a zero-terminated contiguous hunk of characters kills many of the performance
   * wins the string classes offer.  Write your own code to use |nsA[C]String&|s for parameters.
   * Write your string proccessing algorithms to exploit iterators.  If you do this, you
   * will benefit from being able to chain operations without copying or allocating and your
   * code will be significantly more efficient.  Remember, a function that takes an
   * |const nsA[C]String&| can always be passed a raw character pointer by wrapping it (for free)
   * in a |nsLocal[C]String|.  But a function that takes a character pointer always has the
   * potential to force allocation and copying.
   *
   *
   * How to use it:
   *
   * Like all `promises', a |nsPromiseFlat[C]String| doesn't own the characters it promises.
   * You must never use it to promise characters out of a string with a shorter lifespan.
   * The typical use will be something like this
   *
   *   SomeOSFunction( PromiseFlatCString(aCString).get() ); // GOOD
   *
   * Here's a BAD use:
   *
   *  const char* buffer = PromiseFlatCString(aCString).get();
   *  SomeOSFunction(buffer); // BAD!! |buffer| is a dangling pointer
   *
   * A |nsPromiseFlat[C]String| doesn't support non-|const| access (you can't use it to make
   * changes back into the original string).  To help you avoid that, the only way to make
   * one is with the function |PromiseFlat[C]String|, which produce a |const| instance.
   * ``What if I need to keep a promise around for a little while?'' you might ask.
   * In that case, you can keep a reference, like so
   *
   *   const nsPromiseFlatString& flat = PromiseFlatString(aString);
   *     // this reference holds the anonymous temporary alive, but remember, it must _still_
   *     //   have a lifetime shorter than that of |aString|
   *
   *  SomeOSFunction(flat.get());
   *  SomeOtherOSFunction(flat.get());
   *
   *
   * How does it work?
   *
   * A |nsPromiseFlat[C]String| is just a wrapper for another string.  If you apply it to
   * a string that happens to be flat, your promise is just a reference to that other string
   * and all calls are forwarded through to it.  If you apply it to a non-flat string,
   * then a temporary flat string is created for you, by allocating and copying.  In the 
   * event that you end up assigning the result into a sharing string (e.g., |nsTString|),
   * the right thing happens.
   */

template <class CharT>
class NS_COM nsTPromiseFlatString : public nsTString<CharT>
  {
    public:

      typedef CharT                                              char_type;

      typedef nsTPromiseFlatString<char_type>                    self_type;
      typedef nsTString<char_type>                               string_type;
      typedef nsTStringBase<char_type>                           string_base_type;

      typedef typename string_base_type::string_tuple_type       string_tuple_type;
      typedef typename string_base_type::abstract_string_type    abstract_string_type;

    private:

      void Init( const string_base_type& );
      void Init( const abstract_string_type& );

        // NOT TO BE IMPLEMENTED
      void operator=( const self_type& );

    public:

      explicit
      nsTPromiseFlatString( const string_base_type& str )
        : string_type()
        {
          Init(str);
        }

      explicit
      nsTPromiseFlatString( const abstract_string_type& readable )
        : string_type()
        {
          Init(readable);
        }

      explicit
      nsTPromiseFlatString( const string_tuple_type& tuple )
        : string_type()
        {
          // nothing else to do here except assign the value of the tuple
          // into ourselves.
          Assign(tuple);
        }
  };


inline
const nsTPromiseFlatString<PRUnichar>
PromiseFlatString( const nsTAString<PRUnichar>& str )
  {
    return nsTPromiseFlatString<PRUnichar>(str);
  }

  // e.g., PromiseFlatString(Substring(s))
inline
const nsTPromiseFlatString<PRUnichar>
PromiseFlatString( const nsTStringBase<PRUnichar>& frag )
  {
    return nsTPromiseFlatString<PRUnichar>(frag);
  }

  // e.g., PromiseFlatString(a + b)
inline
const nsTPromiseFlatString<PRUnichar>
PromiseFlatString( const nsTStringTuple<PRUnichar>& tuple )
  {
    return nsTPromiseFlatString<PRUnichar>(tuple);
  }


inline
const nsTPromiseFlatString<char>
PromiseFlatCString( const nsTAString<char>& str )
  {
    return nsTPromiseFlatString<char>(str);
  }

  // e.g., PromiseFlatString(Substring(s))
inline
const nsTPromiseFlatString<char>
PromiseFlatCString( const nsTStringBase<char>& str )
  {
    return nsTPromiseFlatString<char>(str);
  }

  // e.g., PromiseFlatString(a + b)
inline
const nsTPromiseFlatString<char>
PromiseFlatCString( const nsTStringTuple<char>& tuple )
  {
    return nsTPromiseFlatString<char>(tuple);
  }


#endif /* !defined(nsTPromiseFlatString_h___) */
