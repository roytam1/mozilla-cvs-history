// vim:set ts=2 sw=2 et cindent:

#include <stdlib.h>
#include "nsStrRef.h"

//-----------------------------------------------------------------------------
// nsStrFragment methods:

nsStrFragment::char_type
nsStrFragment::Last() const
{
  char_type last;

  if (mLength > 0)
  {
    last = mData[mLength - 1];
  }
  else
  {
    NS_ERROR("|Last()| on an empty string");
    last = char_type(0); // BOGUS
  }

  return last;
}

PRUint32
nsStrFragment::CountChar(char_type c) const
{
  PRUint32 result = 0;

  const char_type *start = mData;
  const char_type *end = mData + mLength;

  while (start && start != end)
  {
    start = char_traits::find_char(start, end, c);
    if (start)
    {
      ++result;
      ++start;
    }
  }
  return result;
}

PRInt32
nsStrFragment::FindChar(char_type c, PRUint32 offset) const
{
  const char_type *p =  char_traits::find_char(mData + offset,
                                               mData + mLength, c);
  if (p)
    return p - mData;

  return -1; // XXX kNotFound
}

PRBool
nsStrFragment::Equals(const self_type &ref) const
{
  return mLength == ref.mLength && 
    char_traits::case_sensitive_comparator(mData, ref.mData, mLength) == 0;
}

PRBool
nsStrFragment::Equals(const self_type &ref, comparator_func comp) const
{
  return mLength == ref.mLength &&
    comp(mData, ref.mData, mLength) == 0;
}

PRInt32
nsStrFragment::CompareTo(const self_type &ref) const
{
  if (mLength > ref.mLength)
    return 1;

  if (mLength < ref.mLength)
    return -1;

  return char_traits::case_sensitive_comparator(mData, ref.mData, mLength);
}

PRInt32
nsStrFragment::CompareTo(const self_type &ref, comparator_func comp) const
{
  if (mLength > ref.mLength)
    return 1;

  if (mLength < ref.mLength)
    return -1;

  return comp(mData, ref.mData, mLength);
}

//-----------------------------------------------------------------------------
// reference counted buffer methods:

inline void StrIncrementRef(void *data)
{
  PRInt32 *buf = ((PRInt32 *) data) - 1;
  PR_AtomicIncrement(buf);
}

inline void StrDecrementRef(void *data)
{
  PRInt32 *buf = ((PRInt32 *) data) - 1;
  if (PR_AtomicDecrement(buf) == 0)
  {
    printf(">>> calling free on %p [%s]\n", data, (const char *) data);
    free(buf);
  }
}

inline void *StrAllocateSharedBuf(size_t n)
{
  PRInt32 *buf = (PRInt32 *) malloc(n + sizeof(PRUint32));
  if (buf)
  {
    buf[0] = 1;
    buf++;
  }
  return buf; 
}

//-----------------------------------------------------------------------------
// nsStrRef methods:

nsStrRef::~nsStrRef()
{
  if (mData)
    ReleaseData();
}

void
nsStrRef::ReleaseData()
{
  NS_ASSERTION(mData, "should not call ReleaseData with null mData");

  if (!(mFlags & F_DEPEND))
  {
    if (mFlags & F_SHARED)
    {
      StrDecrementRef(mData);
    }
    else
    {
      printf(">>> calling free on %p [%s]\n", mData, mData);
      free(mData);
    }
  }
}

void
nsStrRef::Adopt(char_type *data, PRUint32 dataLen)
{
  if (mData)
    ReleaseData();

  if (dataLen == PR_UINT32_MAX)
    dataLen = char_traits::length_of(data);

  mData = data;
  mLength = dataLen;
  mFlags = 0;
}

void
nsStrRef::Assign(const char_type *data, PRUint32 dataLen)
{
  if (dataLen == PR_UINT32_MAX)
    dataLen = char_traits::length_of(data);

  // do not release mData until we've created the new buffer.  this allows for
  // the case where data might reference part of mData.

  char_type *buf =
      (char_type *) StrAllocateSharedBuf((dataLen + 1) * sizeof(char_type));
  if (buf)
  {
    char_traits::copy(buf, data, dataLen);
    buf[dataLen] = 0;

    // okay, now we can safely release mData
    if (mData)
      ReleaseData();

    mData = buf;
    mLength = dataLen;
    mFlags = F_SHARED;

    printf(">>> new buffer at %p [%s]\n", mData, mData);
  }
}

void
nsStrRef::Assign(const self_type &ref)
{
  // self-assign is a no-op
  if (this == &ref)
    return;

  if (ref.mFlags & F_SHARED)
  {
    // since ref.mData is reference counted, we can safely release our mData.
    ReleaseData();

    mData = ref.mData;
    mLength = ref.mLength;
    mFlags = ref.mFlags;

    StrIncrementRef(mData);
  }
  else
    Assign(ref.mData, ref.mLength);
}

void
nsStrRef::Assign(const fragment_tuple_type &tuple)
{
  PRUint32 len = tuple.Length();

  char_type *buf =
      (char_type *) StrAllocateSharedBuf((len + 1) * sizeof(char_type));
  if (buf)
  {
    tuple.WriteTo(buf, len);
    buf[len] = char_type(0);

    ReleaseData();

    mData = buf;
    mLength = len;
    mFlags = nsStrRef::F_SHARED;
  }
}

void
nsStrRef::SetIsVoid(PRBool val)
{
  if (val)
  {
    ReleaseData();

    mData = NS_CONST_CAST(char_type *, char_traits::empty_string);
    mLength = 0;
    mFlags = F_DEPEND | F_ISVOID;
  }
  else
  {
    mFlags &= ~F_ISVOID;
  }
}

//-----------------------------------------------------------------------------
// nsStrFragmentTuple methods:

PRUint32
nsStrFragmentTuple::Length() const
{
  //
  // fragments are enumerated right to left
  //
  PRUint32 len = mFragB.Length();
  if (mHead)
    len += mHead->Length();
  else
    len += mFragA.Length();
  return len;
}

void
nsStrFragmentTuple::WriteTo(char_type *buf, PRUint32 end) const
{
  // we need to write out data into buf, ending at end.  so our data
  // needs to preceed |end| exactly.  we trust that the buffer was
  // properly sized!

  char_traits::copy(buf + end - mFragB.Length(), mFragB.get(), mFragB.Length());

  end -= mFragB.Length();

  if (mHead)
    mHead->WriteTo(buf, end);
  else
    char_traits::copy(buf + end - mFragA.Length(), mFragA.get(), mFragA.Length());
}

PRBool
nsStrFragmentTuple::IsDependentOn(const char_type *start,
                                  const char_type *end) const
{
  //
  // fragments are enumerated right to left
  //
  PRBool dependent = mFragB.IsDependentOn(start, end);
  if (!dependent)
  {
    if (mHead)
      dependent = mHead->IsDependentOn(start, end);
    else
      dependent = mFragA.IsDependentOn(start ,end);
  }
  return dependent;
}

//-----------------------------------------------------------------------------
// nsStrBuf methods:

void
nsStrBuf::Assign(const char_type *data, PRUint32 dataLen)
{
  if (dataLen == PR_UINT32_MAX)
    dataLen = char_traits::length_of(data);

  Assign(nsStrFragment(data, dataLen));
}

void
nsStrBuf::Assign(const ref_type &ref)
{
  // self-assign is a no-op
  if (this == &ref)
    return;

  // avoid copying if possible... 

  // XXX for some strange reason GCC 3.2.2 requires this cast
  if (NS_STATIC_CAST(const self_type &, ref).mFlags & F_SHARED)
    nsStrRef::Assign(ref);
  else
    Assign(NS_STATIC_CAST(const fragment_type &, ref));
}

void
nsStrBuf::Assign(const fragment_type &frag)
{
  // self-assign is a no-op
  if (this == &frag)
    return;

  PRUint32 len = frag.Length();

  // check if the fragment depends on mData
  if (frag.IsDependentOn(mData, mData + mLength))
  {
    nsStrAutoBuf temp(frag);
    Assign(temp);
    return;
  }

  if (EnsureCapacity(len + 1, PR_FALSE))
  {
    char_traits::copy(mData, frag.get(), len);
    mData[len] = char_type(0);
    mLength = len;
  }
}

void
nsStrBuf::Assign(const fragment_tuple_type &tuple)
{
  PRUint32 len = tuple.Length();

  printf("+++ %s [f=0x%x c=%u len=%u]\n", __PRETTY_FUNCTION__, mFlags, mCapacity, len);

  // check if the fragment tuple depends on mData
  if (tuple.IsDependentOn(mData, mData + mLength))
  {
    nsStrAutoBuf temp(tuple);
    Assign(temp);
    return;
  }

  if (EnsureCapacity(len + 1, PR_FALSE))
  {
    tuple.WriteTo(mData, len);
    mData[len] = char_type(0);
    mLength = len;
  }
}

void
nsStrBuf::Replace(PRUint32 cutOffset, PRUint32 cutLength, const char_type *data, PRUint32 dataLen)
{
  if (dataLen == PR_UINT32_MAX)
    dataLen = char_traits::length_of(data);

  Replace(cutOffset, cutLength, nsStrFragment(data, dataLen));
}

void
nsStrBuf::Replace(PRUint32 cutOffset, PRUint32 cutLength,
                  const fragment_type &frag)
{
  // check if the fragment depends on mData
  if (frag.IsDependentOn(mData, mData + mLength))
  {
    nsStrAutoBuf temp(frag);
    Replace(cutOffset, cutLength, temp);
    return;
  }

  PRUint32 fragLen = frag.Length();

  PRUint32 newLen = mLength - cutLength + fragLen;
  if (EnsureCapacity(newLen + 1, PR_TRUE))
  {
    // make space for new substring.  may require that we move part
    // of the existing string.
    if (fragLen != cutLength && cutOffset + cutLength < mLength)
    {
      PRUint32 from = cutOffset + cutLength;
      PRUint32 fromLen = mLength - from;
      PRUint32 to = cutOffset + fragLen;
      char_traits::move(mData + to, mData + from, fromLen);
    }
                                                 
    if (fragLen)
      char_traits::copy(mData + cutOffset, frag.get(), fragLen);

    mData[newLen] = char_type(0);
    mLength = newLen;
  }
}

void
nsStrBuf::Replace(PRUint32 cutOffset, PRUint32 cutLength,
                  const fragment_tuple_type &tuple)
{
  // check if the fragment depends on mData
  if (tuple.IsDependentOn(mData, mData + mLength))
  {
    nsStrAutoBuf temp(tuple);
    Replace(cutOffset, cutLength, temp);
    return;
  }

  PRUint32 tupleLen = tuple.Length();

  PRUint32 newLen = mLength - cutLength + tupleLen;
  if (EnsureCapacity(newLen + 1, PR_TRUE))
  {
    // make space for new substring.  may require that we move part
    // of the existing string.
    if (tupleLen != cutLength && cutOffset + cutLength < mLength)
    {
      PRUint32 from = cutOffset + cutLength;
      PRUint32 fromLen = mLength - from;
      PRUint32 to = cutOffset + tupleLen;
      char_traits::move(mData + to, mData + from, fromLen);
    }
                                                 
    if (tupleLen)
      tuple.WriteTo(mData + cutOffset, tupleLen);

    mData[newLen] = char_type(0);
    mLength = newLen;
  }
}

void
nsStrBuf::SetLength(PRUint32 len)
{
  EnsureCapacity(len + 1, PR_TRUE);
  mData[len] = 0;
  mLength = len;
}

PRBool
nsStrBuf::EnsureCapacity(PRUint32 capacity, PRBool preserveData)
{
  // EnsureCapacity is called in preparation for writing.  So, if we have a
  // reference to a shared buffer, then we need to go ahead and drop that
  // shared reference.

  if (mFlags & F_SHARED)
  {
    char *buf = (char *) malloc(capacity * sizeof(char_type));
    if (!buf)
      return PR_FALSE;

    if (preserveData)
    {
      PRUint32 maxLength = capacity - 1;
      if (mLength > maxLength)
        mLength = maxLength;
      char_traits::copy(buf, mData, mLength);
      buf[mLength] = char_type(0);
    }

    ReleaseData();

    mData = buf;
    mCapacity = capacity;
    mFlags = F_CAPACITYSET;
  }
  else
  {
    if (!(mFlags & F_CAPACITYSET))
    {
      // maybe someone called nsStrRef::Adopt
      mCapacity = mLength + 1;
    }

    if (capacity > mCapacity)
    {
      printf(">>> increasing capacity\n");

      // use doubling algorithm when forced to increase available capacity
      PRUint32 temp = mCapacity;
      while (temp < capacity)
        temp <<= 1;
      capacity = temp;

      char *buf;
      if (mFlags & F_DEPEND)
      {
        // we cannot realloc the existing buffer because we do not own it.
        buf = (char *) malloc(capacity * sizeof(char_type));
        if (!buf)
          return PR_FALSE;

        if (preserveData)
          char_traits::copy(buf, mData, mLength + 1);

        mFlags &= ~F_DEPEND;
      }
      else
      {
        buf = (char *) realloc(mData, capacity * sizeof(char_type));
        if (!buf)
          return PR_FALSE;
      }

      mData = buf;
      mCapacity = capacity;

      printf("    new capacity = %u\n", mCapacity);
    }
  }
  return PR_TRUE;
}
