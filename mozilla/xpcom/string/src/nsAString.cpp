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

#include "nsAString.h"
#include "nsDependentSubstring.h"
#include "nsDependentString.h"
#include "plstr.h"


int
nsDefaultStringComparator::operator()( const char_type* lhs, const char_type* rhs, PRUint32 aLength ) const
  {
    return nsCharTraits<char_type>::compare(lhs, rhs, aLength);
  }

int
nsDefaultStringComparator::operator()( char_type lhs, char_type rhs) const
  {
    return lhs - rhs;
  } 

NS_COM
int
Compare( const nsAString& lhs, const nsAString& rhs, const nsStringComparator& aComparator )
  {
    typedef nsAString::size_type  size_type;

    if ( &lhs == &rhs )
      return 0;

    size_type lLength = lhs.Length();
    size_type rLength = rhs.Length();
    size_type lengthToCompare = NS_MIN(lLength, rLength);

    nsAString::const_iterator leftIter, rightIter;
    lhs.BeginReading(leftIter);
    rhs.BeginReading(rightIter);

    for (;;)
      {
        size_type lengthAvailable = size_type( NS_MIN(leftIter.size_forward(), rightIter.size_forward()) );

        if ( lengthAvailable > lengthToCompare )
          lengthAvailable = lengthToCompare;

        {
          int result;
            // Note: |result| should be declared in this |if| expression, but some compilers don't like that
          if ( (result = aComparator(leftIter.get(), rightIter.get(), lengthAvailable)) != 0 )
            return result;
        }

        if ( !(lengthToCompare -= lengthAvailable) )
          break;

        leftIter.advance( PRInt32(lengthAvailable) );
        rightIter.advance( PRInt32(lengthAvailable) );
      }

    if ( lLength < rLength )
      return -1;
    else if ( rLength < lLength )
      return 1;
    else
      return 0;
  }

const nsAString::shared_buffer_handle_type*
nsAString::GetSharedBufferHandle() const
  {
    return 0;
  }

const nsAString::buffer_handle_type*
nsAString::GetFlatBufferHandle() const
  {
    return GetSharedBufferHandle();
  }

const nsAString::buffer_handle_type*
nsAString::GetBufferHandle() const
  {
    return GetSharedBufferHandle();
  }

PRUint32
nsAString::GetImplementationFlags() const
  {
    return 0;
  }


PRBool
nsAString::IsVoid() const
  {
    return PR_FALSE;
  }

void
nsAString::SetIsVoid( PRBool )
  {
    // |SetIsVoid| is ignored by default
  }

PRBool
nsAString::Equals( const char_type* rhs, const nsStringComparator& aComparator ) const
  {
    return Equals(nsDependentString(rhs), aComparator);
  }



nsAString::char_type
nsAString::First() const
  {
    NS_ASSERTION(Length()>0, "|First()| on an empty string");

    const_iterator iter;
    return *BeginReading(iter);
  }

nsAString::char_type
nsAString::Last() const
  {
    NS_ASSERTION(Length()>0, "|Last()| on an empty string");

    const_iterator iter;

    if ( !IsEmpty() )
      {
        EndReading(iter);
        iter.advance(-1);
      }

    return *iter; // Note: this has undefined results if |IsEmpty()|
  }

nsAString::size_type
nsAString::CountChar( char_type c ) const
  {
      /*
        re-write this to use a counting sink
       */

    size_type result = 0;
    size_type lengthToExamine = Length();

    const_iterator iter;
    for ( BeginReading(iter); ; )
      {
        PRInt32 lengthToExamineInThisFragment = iter.size_forward();
        const char_type* fromBegin = iter.get();
        result += size_type(NS_COUNT(fromBegin, fromBegin+lengthToExamineInThisFragment, c));
        if ( !(lengthToExamine -= lengthToExamineInThisFragment) )
          return result;
        iter.advance(lengthToExamineInThisFragment);
      }
      // never reached; quiets warnings
    return 0;
  }

PRInt32
nsAString::FindChar( char_type aChar, PRUint32 aOffset ) const
  {
    const_iterator iter, done_searching;
    BeginReading(iter).advance( PRInt32(aOffset) );
    EndReading(done_searching);

    size_type lengthSearched = 0;
    while ( iter != done_searching )
      {
        PRInt32 fragmentLength = iter.size_forward();
        const char_type* charFoundAt = nsCharTraits<char_type>::find(iter.get(), fragmentLength, aChar);
        if ( charFoundAt )
          return lengthSearched + (charFoundAt-iter.get()) + aOffset;

        lengthSearched += fragmentLength;
        iter.advance(fragmentLength);
      }

    return -1;
  }

PRBool
nsAString::IsDependentOn( const self_type& aString ) const
  {
    const_fragment_type f1;
    const char_type* s1 = GetReadableFragment(f1, kFirstFragment);
    while ( s1 )
      {
        const_fragment_type f2;
        const char_type* s2 = aString.GetReadableFragment(f2, kFirstFragment);
        while ( s2 )
          {
              // if it _isn't_ the case that
              //  one fragment starts after the other ends,
              //  or ends before the other starts,
              // then, they conflict:
              // !(f2.mStart>=f1.mEnd || f2.mEnd<=f1.mStart)
              //
              // Simplified, that gives us:
            if ( f2.mStart < f1.mEnd && f2.mEnd > f1.mStart )
              return PR_TRUE;

            s2 = aString.GetReadableFragment(f2, kNextFragment);
          }
        s1 = GetReadableFragment(f1, kNextFragment);
      }
    return PR_FALSE;
  }

  //
  // |Assign()|
  //

void
nsAString::do_AssignFromReadable( const self_type& aReadable )
    /*
      ...we need to check whether the string that's being assigned into |this| somehow references |this|.
      E.g.,
      
        ... writable& w ...
        ... readable& r ...
        
        w = r + w;

      In this example, you can see that unless the characters promised by |w| in |r+w| are resolved before
      anything starts getting copied into |w|, there will be trouble.  They will be overritten by the contents
      of |r| before being retrieved to be appended.

      We could have a really tricky solution where we tell the promise to resolve _just_ the data promised
      by |this|, but this should be a rare case, since clients with more local knowledge will know that, e.g.,
      in the case above, |Insert| could have special behavior with significantly better performance.  Since
      it's a rare case anyway, we should just do the simplest thing that could possibly work, resolve the
      entire promise.  If we measure and this turns out to show up on performance radar, we then have the
      option to fix either the callers or this mechanism.
    */
  {
      // self-assign is a no-op
    if ( this == &aReadable)
      return;

    if ( !aReadable.IsDependentOn(*this) )
      UncheckedAssignFromReadable(aReadable);
    else
      {
        size_type length = aReadable.Length();
        char_type* buffer = new char_type[length];
        if ( buffer )
          {
            // Note: not exception safe.  We need something to manage temporary buffers like this

            const_iterator fromBegin, fromEnd;
            char_type* toBegin = buffer;
            copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), toBegin);
            UncheckedAssignFromReadable(Substring(buffer, buffer + length));
            delete[] buffer;
          }
        // else assert?
      }
  }

void
nsAString::UncheckedAssignFromReadable( const self_type& aReadable )
  {
    SetLength(0);
    if ( !aReadable.IsEmpty() )
      {
        SetLength(aReadable.Length());
          // first setting the length to |0| avoids copying characters only to be overwritten later
          //  in the case where the implementation decides to re-allocate

        const_iterator fromBegin, fromEnd;
        iterator toBegin;
        copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), BeginWriting(toBegin));
      }
  }

void
nsAString::do_AssignFromElementPtr( const char_type* aPtr )
  {
    do_AssignFromReadable(nsDependentString(aPtr));
  }

void
nsAString::do_AssignFromElementPtrLength( const char_type* aPtr, size_type aLength )
  {
    do_AssignFromReadable(Substring(aPtr, aPtr+aLength));
  }

void
nsAString::do_AssignFromElement( char_type aChar )
  {
    UncheckedAssignFromReadable(Substring(&aChar, &aChar+1));
  }



  //
  // |Append()|
  //

void
nsAString::do_AppendFromReadable( const self_type& aReadable )
  {
    if ( !aReadable.IsDependentOn(*this) )
      UncheckedAppendFromReadable(aReadable);
    else
      {
        size_type length = aReadable.Length();
        char_type* buffer = new char_type[length];
        if ( buffer )
          {
            const_iterator fromBegin, fromEnd;
            char_type* toBegin = buffer;
            copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), toBegin);
            UncheckedAppendFromReadable(Substring(buffer, buffer + length));
            delete[] buffer;
          }
        // else assert?
      }
  }

void
nsAString::UncheckedAppendFromReadable( const self_type& aReadable)
  {
    size_type oldLength = this->Length();
    SetLength(oldLength + aReadable.Length());

    const_iterator fromBegin, fromEnd;
    iterator toBegin;
    copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), BeginWriting(toBegin).advance( PRInt32(oldLength) ) );
  }


void
nsAString::do_AppendFromElementPtr( const char_type* aPtr )
  {
    do_AppendFromReadable(nsDependentString(aPtr));
  }

void
nsAString::do_AppendFromElementPtrLength( const char_type* aPtr, size_type aLength )
  {
    do_AppendFromReadable(Substring(aPtr, aPtr+aLength));
  }

void
nsAString::do_AppendFromElement( char_type aChar )
  {
    UncheckedAppendFromReadable(Substring(&aChar, &aChar + 1));
  }



  //
  // |Insert()|
  //

void
nsAString::do_InsertFromReadable( const self_type& aReadable, index_type atPosition )
  {
    if ( !aReadable.IsDependentOn(*this) )
      UncheckedInsertFromReadable(aReadable, atPosition);
    else
      {
        size_type length = aReadable.Length();
        char_type* buffer = new char_type[length];
        if ( buffer )
          {
            const_iterator fromBegin, fromEnd;
            char_type* toBegin = buffer;
            copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), toBegin);
            UncheckedInsertFromReadable(Substring(buffer, buffer + length), atPosition);
            delete[] buffer;
          }
        // else assert
      }
  }

void
nsAString::UncheckedInsertFromReadable( const self_type& aReadable, index_type atPosition )
  {
    size_type oldLength = this->Length();
    SetLength(oldLength + aReadable.Length());

    const_iterator fromBegin, fromEnd;
    iterator toBegin;
    if ( atPosition < oldLength )
      copy_string_backward(this->BeginReading(fromBegin).advance(PRInt32(atPosition)), this->BeginReading(fromEnd).advance(PRInt32(oldLength)), EndWriting(toBegin));
    else
      atPosition = oldLength;
    copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), BeginWriting(toBegin).advance(PRInt32(atPosition)));
  }

void
nsAString::do_InsertFromElementPtr( const char_type* aPtr, index_type atPosition )
  {
    do_InsertFromReadable(nsDependentString(aPtr), atPosition);
  }

void
nsAString::do_InsertFromElementPtrLength( const char_type* aPtr, index_type atPosition, size_type aLength )
  {
    do_InsertFromReadable(Substring(aPtr, aPtr+aLength), atPosition);
  }

void
nsAString::do_InsertFromElement( char_type aChar, index_type atPosition )
  {
    UncheckedInsertFromReadable(Substring(&aChar, &aChar+1), atPosition);
  }



  //
  // |Cut()|
  //

void
nsAString::Cut( index_type cutStart, size_type cutLength )
  {
    size_type myLength = this->Length();
    cutLength = NS_MIN(cutLength, myLength-cutStart);
    index_type cutEnd = cutStart + cutLength;

    const_iterator fromBegin, fromEnd;
    iterator toBegin;
    if ( cutEnd < myLength )
      copy_string(this->BeginReading(fromBegin).advance(PRInt32(cutEnd)), this->EndReading(fromEnd), BeginWriting(toBegin).advance(PRInt32(cutStart)));
    SetLength(myLength-cutLength);
  }



  //
  // |Replace()|
  //

void
nsAString::do_ReplaceFromReadable( index_type cutStart, size_type cutLength, const self_type& aReadable )
  {
    if ( !aReadable.IsDependentOn(*this) )
      UncheckedReplaceFromReadable(cutStart, cutLength, aReadable);
    else
      {
        size_type length = aReadable.Length();
        char_type* buffer = new char_type[length];
        if ( buffer )
          {
            const_iterator fromBegin, fromEnd;
            char_type* toBegin = buffer;
            copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), toBegin);
            UncheckedReplaceFromReadable(cutStart, cutLength, Substring(buffer, buffer + length));
            delete[] buffer;
          }
        // else assert?
      }
  }

void
nsAString::UncheckedReplaceFromReadable( index_type cutStart, size_type cutLength, const self_type& aReplacement )
  {
    size_type oldLength = this->Length();

    cutStart = NS_MIN(cutStart, oldLength);
    cutLength = NS_MIN(cutLength, oldLength-cutStart);
    index_type cutEnd = cutStart + cutLength;

    size_type replacementLength = aReplacement.Length();
    index_type replacementEnd = cutStart + replacementLength;

    size_type newLength = oldLength - cutLength + replacementLength;

    const_iterator fromBegin, fromEnd;
    iterator toBegin;
    if ( cutLength > replacementLength )
      copy_string(this->BeginReading(fromBegin).advance(PRInt32(cutEnd)), this->EndReading(fromEnd), BeginWriting(toBegin).advance(PRInt32(replacementEnd)));
    SetLength(newLength);
    if ( cutLength < replacementLength )
      copy_string_backward(this->BeginReading(fromBegin).advance(PRInt32(cutEnd)), this->BeginReading(fromEnd).advance(PRInt32(oldLength)), EndWriting(toBegin));

    copy_string(aReplacement.BeginReading(fromBegin), aReplacement.EndReading(fromEnd), BeginWriting(toBegin).advance(PRInt32(cutStart)));
  }



int
nsDefaultCStringComparator::operator()( const char_type* lhs, const char_type* rhs, PRUint32 aLength ) const
  {
    return nsCharTraits<char_type>::compare(lhs, rhs, aLength);
  }

PRBool
nsDefaultCStringComparator::operator()( char_type lhs, char_type rhs ) const
  {
    return lhs - rhs;
  }

int
nsCaseInsensitiveCStringComparator::operator()( const char_type* lhs, const char_type* rhs, PRUint32 aLength ) const
  {
    PRInt32 result=PRInt32(PL_strncasecmp(lhs, rhs, aLength));
    //Egads. PL_strncasecmp is returning *very* negative numbers.
    //Some folks expect -1,0,1, so let's temper its enthusiasm.
    if (result<0) 
      result=-1;
    return result;
  }

PRBool
nsCaseInsensitiveCStringComparator::operator()( char lhs, char rhs ) const
  {
    if (lhs == rhs) return 0;
    
    lhs = tolower(lhs);
    rhs = tolower(rhs);

    return lhs - rhs;
  }

NS_COM
int
Compare( const nsACString& lhs, const nsACString& rhs, const nsCStringComparator& aComparator )
  {
    typedef nsACString::size_type   size_type;

    if ( &lhs == &rhs )
      return 0;

    size_type lLength = lhs.Length();
    size_type rLength = rhs.Length();
    size_type lengthToCompare = NS_MIN(lLength, rLength);

    nsACString::const_iterator leftIter, rightIter;
    lhs.BeginReading(leftIter);
    rhs.BeginReading(rightIter);

    for (;;)
      {
        size_type lengthAvailable = size_type( NS_MIN(leftIter.size_forward(), rightIter.size_forward()) );

        if ( lengthAvailable > lengthToCompare )
          lengthAvailable = lengthToCompare;

        {
          int result;
            // Note: |result| should be declared in this |if| expression, but some compilers don't like that
          if ( (result = aComparator(leftIter.get(), rightIter.get(), lengthAvailable)) != 0 )
            return result;
        }

        if ( !(lengthToCompare -= lengthAvailable) )
          break;

        leftIter.advance( PRInt32(lengthAvailable) );
        rightIter.advance( PRInt32(lengthAvailable) );
      }

    if ( lLength < rLength )
      return -1;
    else if ( rLength < lLength )
      return 1;
    else
      return 0;
  }

const nsACString::shared_buffer_handle_type*
nsACString::GetSharedBufferHandle() const
  {
    return 0;
  }

const nsACString::buffer_handle_type*
nsACString::GetFlatBufferHandle() const
  {
    return GetSharedBufferHandle();
  }

const nsACString::buffer_handle_type*
nsACString::GetBufferHandle() const
  {
    return GetSharedBufferHandle();
  }

PRUint32
nsACString::GetImplementationFlags() const
  {
    return 0;
  }


PRBool
nsACString::IsVoid() const
  {
    return PR_FALSE;
  }

void
nsACString::SetIsVoid( PRBool )
  {
    // |SetIsVoid| is ignored by default
  }

PRBool
nsACString::Equals( const char_type* rhs, const nsCStringComparator& aComparator ) const
  {
    return Equals(nsDependentCString(rhs), aComparator);
  }

nsACString::char_type
nsACString::First() const
  {
    NS_ASSERTION(Length()>0, "|First()| on an empty string");

    const_iterator iter;
    return *BeginReading(iter);
  }

nsACString::char_type
nsACString::Last() const
  {
    NS_ASSERTION(Length()>0, "|Last()| on an empty string");

    const_iterator iter;

    if ( !IsEmpty() )
      {
        EndReading(iter);
        iter.advance(-1);
      }

    return *iter; // Note: this has undefined results if |IsEmpty()|
  }

nsACString::size_type
nsACString::CountChar( char_type c ) const
  {
      /*
        re-write this to use a counting sink
       */

    size_type result = 0;
    size_type lengthToExamine = Length();

    const_iterator iter;
    for ( BeginReading(iter); ; )
      {
        PRInt32 lengthToExamineInThisFragment = iter.size_forward();
        const char_type* fromBegin = iter.get();
        result += size_type(NS_COUNT(fromBegin, fromBegin+lengthToExamineInThisFragment, c));
        if ( !(lengthToExamine -= lengthToExamineInThisFragment) )
          return result;
        iter.advance(lengthToExamineInThisFragment);
      }
      // never reached; quiets warnings
    return 0;
  }

PRInt32
nsACString::FindChar( char_type aChar, PRUint32 aOffset ) const
  {
    const_iterator iter, done_searching;
    BeginReading(iter).advance( PRInt32(aOffset) );
    EndReading(done_searching);

    size_type lengthSearched = 0;
    while ( iter != done_searching )
      {
        PRInt32 fragmentLength = iter.size_forward();
        const char_type* charFoundAt = nsCharTraits<char_type>::find(iter.get(), fragmentLength, aChar);
        if ( charFoundAt )
          return lengthSearched + (charFoundAt-iter.get()) + aOffset;

        lengthSearched += fragmentLength;
        iter.advance(fragmentLength);
      }

    return -1;
  }

PRBool
nsACString::IsDependentOn( const self_type& aString ) const
  {
    const_fragment_type f1;
    const char_type* s1 = GetReadableFragment(f1, kFirstFragment);
    while ( s1 )
      {
        const_fragment_type f2;
        const char_type* s2 = aString.GetReadableFragment(f2, kFirstFragment);
        while ( s2 )
          {
              // if it _isn't_ the case that
              //  one fragment starts after the other ends,
              //  or ends before the other starts,
              // then, they conflict:
              // !(f2.mStart>=f1.mEnd || f2.mEnd<=f1.mStart)
              //
              // Simplified, that gives us:
            if ( f2.mStart < f1.mEnd && f2.mEnd > f1.mStart )
              return PR_TRUE;

            s2 = aString.GetReadableFragment(f2, kNextFragment);
          }
        s1 = GetReadableFragment(f1, kNextFragment);
      }
    return PR_FALSE;
  }

  //
  // |Assign()|
  //

void
nsACString::do_AssignFromReadable( const self_type& aReadable )
    /*
      ...we need to check whether the string that's being assigned into |this| somehow references |this|.
      E.g.,
      
        ... writable& w ...
        ... readable& r ...
        
        w = r + w;

      In this example, you can see that unless the characters promised by |w| in |r+w| are resolved before
      anything starts getting copied into |w|, there will be trouble.  They will be overritten by the contents
      of |r| before being retrieved to be appended.

      We could have a really tricky solution where we tell the promise to resolve _just_ the data promised
      by |this|, but this should be a rare case, since clients with more local knowledge will know that, e.g.,
      in the case above, |Insert| could have special behavior with significantly better performance.  Since
      it's a rare case anyway, we should just do the simplest thing that could possibly work, resolve the
      entire promise.  If we measure and this turns out to show up on performance radar, we then have the
      option to fix either the callers or this mechanism.
    */
  {
      // self-assign is a no-op
    if (this == &aReadable)
      return;

    if ( !aReadable.IsDependentOn(*this) )
      UncheckedAssignFromReadable(aReadable);
    else
      {
        size_type length = aReadable.Length();
        char_type* buffer = new char_type[length];
        if ( buffer )
          {
            // Note: not exception safe.  We need something to manage temporary buffers like this

            const_iterator fromBegin, fromEnd;
            char_type* toBegin = buffer;
            copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), toBegin);
            UncheckedAssignFromReadable(Substring(buffer, buffer + length));
            delete[] buffer;
          }
        // else assert?
      }
  }

void
nsACString::UncheckedAssignFromReadable( const self_type& aReadable )
  {
    SetLength(0);
    if ( !aReadable.IsEmpty() )
      {
        SetLength(aReadable.Length());
          // first setting the length to |0| avoids copying characters only to be overwritten later
          //  in the case where the implementation decides to re-allocate

        const_iterator fromBegin, fromEnd;
        iterator toBegin;
        copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), BeginWriting(toBegin));
      }
  }

void
nsACString::do_AssignFromElementPtr( const char_type* aPtr )
  {
    do_AssignFromReadable(nsDependentCString(aPtr));
  }

void
nsACString::do_AssignFromElementPtrLength( const char_type* aPtr, size_type aLength )
  {
    do_AssignFromReadable(Substring(aPtr, aPtr+aLength));
  }

void
nsACString::do_AssignFromElement( char_type aChar )
  {
    UncheckedAssignFromReadable(Substring(&aChar, &aChar+1));
  }



  //
  // |Append()|
  //

void
nsACString::do_AppendFromReadable( const self_type& aReadable )
  {
    if ( !aReadable.IsDependentOn(*this) )
      UncheckedAppendFromReadable(aReadable);
    else
      {
        size_type length = aReadable.Length();
        char_type* buffer = new char_type[length];
        if ( buffer )
          {
            const_iterator fromBegin, fromEnd;
            char_type* toBegin = buffer;
            copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), toBegin);
            UncheckedAppendFromReadable(Substring(buffer, buffer + length));
            delete[] buffer;
          }
        // else assert?
      }
  }

void
nsACString::UncheckedAppendFromReadable( const self_type& aReadable )
  {
    size_type oldLength = this->Length();
    SetLength(oldLength + aReadable.Length());

    const_iterator fromBegin, fromEnd;
    iterator toBegin;
    copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), BeginWriting(toBegin).advance( PRInt32(oldLength) ) );
  }

void
nsACString::do_AppendFromElementPtr( const char_type* aPtr )
  {
    do_AppendFromReadable(nsDependentCString(aPtr));
  }

void
nsACString::do_AppendFromElementPtrLength( const char_type* aPtr, size_type aLength )
  {
    do_AppendFromReadable(Substring(aPtr, aPtr+aLength));
  }

void
nsACString::do_AppendFromElement( char_type aChar )
  {
    UncheckedAppendFromReadable(Substring(&aChar, &aChar + 1));
  }



  //
  // |Insert()|
  //

void
nsACString::do_InsertFromReadable( const self_type& aReadable, index_type atPosition )
  {
    if ( !aReadable.IsDependentOn(*this) )
      UncheckedInsertFromReadable(aReadable, atPosition);
    else
      {
        size_type length = aReadable.Length();
        char_type* buffer = new char_type[length];
        if ( buffer )
          {
            const_iterator fromBegin, fromEnd;
            char_type* toBegin = buffer;
            copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), toBegin);
            UncheckedInsertFromReadable(Substring(buffer, buffer + length), atPosition);
            delete[] buffer;
          }
        // else assert
      }
  }

void
nsACString::UncheckedInsertFromReadable( const self_type& aReadable, index_type atPosition )
  {
    size_type oldLength = this->Length();
    SetLength(oldLength + aReadable.Length());

    const_iterator fromBegin, fromEnd;
    iterator toBegin;
    if ( atPosition < oldLength )
      copy_string_backward(this->BeginReading(fromBegin).advance(PRInt32(atPosition)), this->BeginReading(fromEnd).advance(PRInt32(oldLength)), EndWriting(toBegin));
    else
      atPosition = oldLength;
    copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), BeginWriting(toBegin).advance(PRInt32(atPosition)));
  }

void
nsACString::do_InsertFromElementPtr( const char_type* aPtr, index_type atPosition )
  {
    do_InsertFromReadable(nsDependentCString(aPtr), atPosition);
  }

void
nsACString::do_InsertFromElementPtrLength( const char_type* aPtr, index_type atPosition, size_type aLength )
  {
    do_InsertFromReadable(Substring(aPtr, aPtr+aLength), atPosition);
  }

void
nsACString::do_InsertFromElement( char_type aChar, index_type atPosition )
  {
    UncheckedInsertFromReadable(Substring(&aChar, &aChar+1), atPosition);
  }



  //
  // |Cut()|
  //

void
nsACString::Cut( index_type cutStart, size_type cutLength )
  {
    size_type myLength = this->Length();
    cutLength = NS_MIN(cutLength, myLength-cutStart);
    index_type cutEnd = cutStart + cutLength;

    const_iterator fromBegin, fromEnd;
    iterator toBegin;
    if ( cutEnd < myLength )
      copy_string(this->BeginReading(fromBegin).advance(PRInt32(cutEnd)), this->EndReading(fromEnd), BeginWriting(toBegin).advance(PRInt32(cutStart)));
    SetLength(myLength-cutLength);
  }



  //
  // |Replace()|
  //

void
nsACString::do_ReplaceFromReadable( index_type cutStart, size_type cutLength, const self_type& aReadable )
  {
    if ( !aReadable.IsDependentOn(*this) )
      UncheckedReplaceFromReadable(cutStart, cutLength, aReadable);
    else
      {
        size_type length = aReadable.Length();
        char_type* buffer = new char_type[length];
        if ( buffer )
          {
            const_iterator fromBegin, fromEnd;
            char_type* toBegin = buffer;
            copy_string(aReadable.BeginReading(fromBegin), aReadable.EndReading(fromEnd), toBegin);
            UncheckedReplaceFromReadable(cutStart, cutLength, Substring(buffer, buffer + length));
            delete[] buffer;
          }
        // else assert?
      }
  }

void
nsACString::UncheckedReplaceFromReadable( index_type cutStart, size_type cutLength, const self_type& aReplacement )
  {
    size_type oldLength = this->Length();

    cutStart = NS_MIN(cutStart, oldLength);
    cutLength = NS_MIN(cutLength, oldLength-cutStart);
    index_type cutEnd = cutStart + cutLength;

    size_type replacementLength = aReplacement.Length();
    index_type replacementEnd = cutStart + replacementLength;

    size_type newLength = oldLength - cutLength + replacementLength;

    const_iterator fromBegin, fromEnd;
    iterator toBegin;
    if ( cutLength > replacementLength )
      copy_string(this->BeginReading(fromBegin).advance(PRInt32(cutEnd)), this->EndReading(fromEnd), BeginWriting(toBegin).advance(PRInt32(replacementEnd)));
    SetLength(newLength);
    if ( cutLength < replacementLength )
      copy_string_backward(this->BeginReading(fromBegin).advance(PRInt32(cutEnd)), this->BeginReading(fromEnd).advance(PRInt32(oldLength)), EndWriting(toBegin));

    copy_string(aReplacement.BeginReading(fromBegin), aReplacement.EndReading(fromEnd), BeginWriting(toBegin).advance(PRInt32(cutStart)));
  }

