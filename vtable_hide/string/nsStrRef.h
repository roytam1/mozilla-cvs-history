// vim:set ts=2 sw=2 et cindent:
#ifndef nsStrRef_h__
#define nsStrRef_h__

#include "types.h" // XXX temporary
#include "nsCharTraits.h"

class nsStrRef;
class nsStrDependentRef;
class nsStrAutoRef;
class nsStrFragment;
class nsStrFragmentTuple;

//-----------------------------------------------------------------------------

// container for an immutable string fragment, which is not necessarily
// null-terminated.

class nsStrFragment
{
  public:
    typedef NS_CHAR char_type;
    typedef nsStrFragment self_type;
    typedef nsCharTraits<char_type> char_traits;
    typedef char_traits::comparator_func comparator_func;

    nsStrFragment()
      : mData(NS_CONST_CAST(char_type *, char_traits::empty_string))
      , mLength(0) {}

    nsStrFragment(const char_type *data, PRUint32 length)
      : mData(NS_CONST_CAST(char_type *, data)), mLength(length) {}

    // default copy constructor is appropriate

    const char_type *get() const { return mData; }

    PRUint32 Length() const { return mLength; }

    PRBool IsEmpty() const { return mLength == 0; }

    // character access

    char_type First() const { return mData[0]; }
    char_type Last() const;

    PRUint32 CountChar(char_type c) const;

    PRInt32 FindChar(char_type c, PRUint32 offset = 0) const;

    // comparison

    PRBool Equals(const self_type &ref) const;
    PRBool Equals(const self_type &ref, comparator_func c) const;

    PRInt32 CompareTo(const self_type &ref) const;
    PRInt32 CompareTo(const self_type &ref, comparator_func c) const;

    // returns true if this fragment is dependent on (i.e., overlapping with)
    // the given char sequence.
    PRBool IsDependentOn(const char_type *start, const char_type *end) const
      { return (mData >= start) && (mData < end); }

    // XXX more functions...

  protected:
    
    // helper constructor for subclasses
    nsStrFragment(char_type *data)
      : mData(data) {}
    nsStrFragment(char_type *data, PRUint32 length)
      : mData(data), mLength(length) {}

    //  mData is non-const in this class since subclasses may manipulate it.
    char_type *mData;
    PRUint32   mLength;
};

//-----------------------------------------------------------------------------

// container for a null-terminated, immutable string of characters.  since the
// string data is immutable, this class permits sharing of the underyling
// string data between threads.  assignment between nsStrRef instances is
// optimized to avoid copying.

class nsStrRef : public nsStrFragment
{
  public:
    typedef nsStrRef self_type;
    typedef nsStrFragment fragment_type;
    typedef nsStrFragmentTuple fragment_tuple_type;

    ~nsStrRef();

    // constructors

    nsStrRef()
      : nsStrFragment(), mFlags(F_DEPEND) {}

    nsStrRef(const char_type *data, PRUint32 dataLen = PR_UINT32_MAX)
      : nsStrFragment(nsnull) { Assign(data, dataLen); }

    explicit
    nsStrRef(const self_type &ref)
      : nsStrFragment(nsnull) { Assign(ref); }

    explicit
    nsStrRef(const fragment_type &frag)
      : nsStrFragment(nsnull) { Assign(frag); }

    explicit
    nsStrRef(const fragment_tuple_type &tuple)
      : nsStrFragment(nsnull) { Assign(tuple); }

    // adopt given null-terminated raw string

    void Adopt(char_type *data, PRUint32 dataLen = PR_UINT32_MAX);

    // assign into string reference

    void Assign(const char_type c)
      { Assign(&c, 1); }

    void Assign(const char_type *data, PRUint32 dataLen = PR_UINT32_MAX);

    void Assign(const self_type &ref);

    void Assign(const fragment_type &frag)
      { Assign(frag.get(), frag.Length()); }

    void Assign(const fragment_tuple_type &tuple);

    // assignment operators

    self_type &operator=(const self_type &ref)
      { Assign(ref); return *this; }

    self_type &operator=(const fragment_type &frag)
      { Assign(frag); return *this; }

    self_type &operator=(const fragment_tuple_type &tuple)
      { Assign(tuple); return *this; }

    // support for voiding a string

    PRBool IsVoid() const { return mFlags & F_ISVOID; }
    void SetIsVoid(PRBool);

  protected:

    // helper constructor for subclasses
    nsStrRef(char_type *data, PRUint32 length, PRUint32 flags)
      : nsStrFragment(data, length), mFlags(flags) {}

    void ReleaseData();

    enum
      {
        F_DEPEND = 0x1, // if set, then we do not own mData
        F_SHARED = 0x2, // if set, then mData can be shared
        F_ISVOID = 0x4  // if set, then the string is void
      };
    PRUint32 mFlags;
    PRUint32 mCapacity;
};

//-----------------------------------------------------------------------------

// container for a null-terminated, immutable string of characters that is
// not owned by the container.  this class is designed to be used as a wrapper
// around raw string literals so as to avoid copying the string literal.

class nsStrDependentRef : public nsStrRef
{
  public:
    typedef nsStrDependentRef self_type;

    // constructors

    explicit
    nsStrDependentRef(const char_type *data)
      { mFlags = F_DEPEND; Rebind(data); }

    nsStrDependentRef(const char_type *data, PRUint32 length)
      { mFlags = F_DEPEND; Rebind(data, length); }

    nsStrDependentRef(const char_type *start, const char_type *end)
      { mFlags = F_DEPEND; Rebind(start, end); }

    void Rebind(const char_type *data)
      { Rebind(data, char_traits::length_of(data)); }

    void Rebind(const char_type *data, PRUint32 length)
      {
        NS_ASSERTION(data,
            "nsStrDependentRef must wrap a non-NULL buffer");
        NS_ASSERTION(!data[length],
            "nsStrDependentRef must wrap only null-terminated buffer");
        mData = NS_CONST_CAST(char_type *, data);
        mLength = length;
      }

    void Rebind(const char_type *start, const char_type *end)
      { Rebind(start, end - start); }

  private:
    // NOT TO BE IMPLEMENTED
    void operator=(const self_type &);
};

#define NS_STR_NAMED_LITERAL(name, val) nsStrDependentRef name(val, sizeof(val)-1)
#define NS_STR_LITERAL(val)             nsStrDependentRef(val, sizeof(val)-1)

//-----------------------------------------------------------------------------

// substring helpers

inline const nsStrFragment
Substring(const nsStrFragment &frag, PRUint32 offset, PRUint32 length)
{
  return nsStrFragment(frag.get() + offset, length);
}

inline const nsStrFragment
Substring(const NS_CHAR *data, PRUint32 length)
{
  return nsStrFragment(data, length);
}

inline const nsStrFragment
Substring(const NS_CHAR *start, const NS_CHAR *end)
{
  return nsStrFragment(start, end - start);
}

inline const nsStrFragment
StringHead(const nsStrFragment &frag, PRUint32 length)
{
  return nsStrFragment(frag.get(), length);
}

inline const nsStrFragment
StringTail(const nsStrFragment &frag, PRUint32 length)
{
  return nsStrFragment(frag.get() + frag.Length() - length, length);
}

//-----------------------------------------------------------------------------

// the fragment tuple represents a concatenation of string fragments.
class nsStrFragmentTuple
{
  public:
    typedef NS_CHAR char_type;
    typedef nsCharTraits<char_type> char_traits;

    nsStrFragmentTuple(const nsStrFragment &a, const nsStrFragment &b)
      : mHead(nsnull)
      , mFragA(a)
      , mFragB(b) {}

    nsStrFragmentTuple(const nsStrFragmentTuple &head, const nsStrFragment &frag)
      : mHead(&head)
      , mFragA(frag) // this fragment is ignored when head != null
      , mFragB(frag)
      {}

    // computes the aggregate string length
    PRUint32 Length() const;

    // writes the aggregate string to the given buffer.  bufLen is assumed to
    // be equal to or greater than the value returned by the Length() method.
    // the string written to |buf| is not null-terminated.
    void WriteTo(char_type *buf, PRUint32 bufLen) const;

    // returns true if this tuple is dependent on (i.e., overlapping with)
    // the given char sequence.
    PRBool IsDependentOn(const char_type *start, const char_type *end) const;

  protected:
    const nsStrFragmentTuple *mHead;
    const nsStrFragment      &mFragA;
    const nsStrFragment      &mFragB;
};

inline const nsStrFragmentTuple
operator+(const nsStrFragment &a, const nsStrFragment &b)
{
  return nsStrFragmentTuple(a, b);
}

inline const nsStrFragmentTuple
operator+(const nsStrFragmentTuple &a, const nsStrFragment &b)
{
  return nsStrFragmentTuple(a, b);
}

//-----------------------------------------------------------------------------

#if 0

// a container for a null-terminated, mutable string buffer.  this class
// provides high-level string editing functions.

class nsStrBuf : public nsStrRef
{
  public:
    typedef nsStrRef ref_type;
    typedef nsStrBuf self_type;

    // constructors

    nsStrBuf()
      : nsStrRef(), mCapacity(0) {}

    explicit
    nsStrBuf(const char_type *data)
      : nsStrRef(), mCapacity(0) { Assign(data); }

    nsStrBuf(const char_type *data, PRUint32 dataLen)
      : nsStrRef(), mCapacity(0) { Assign(data, dataLen); }

    explicit
    nsStrBuf(const ref_type &ref)
      : nsStrRef(), mCapacity(0) { Assign(ref); }

    explicit
    nsStrBuf(const fragment_type &frag)
      : nsStrRef(), mCapacity(0) { Assign(frag); }

    explicit
    nsStrBuf(const fragment_tuple_type &tuple)
      : nsStrRef(), mCapacity(0) { Assign(tuple); }

    // assign into string buffer

    void Assign(const char_type c)
      { Assign(&c, 1); }

    void Assign(const char_type *data, PRUint32 dataLen = PR_UINT32_MAX);

    void Assign(const ref_type &ref);

    void Assign(const fragment_type &frag);

    void Assign(const fragment_tuple_type &tuple);

    // assignment operators

    self_type &operator=(const self_type &buf)
      { Assign(buf); return *this; }

    self_type &operator=(const ref_type &ref)
      { Assign(ref); return *this; }

    self_type &operator=(const fragment_type &frag)
      { Assign(frag); return *this; }

    self_type &operator=(const fragment_tuple_type &tuple)
      { Assign(tuple); return *this; }

    // replace part of string buffer

    void Replace(PRUint32 cutOffset, PRUint32 cutLength, char_type c)
      { Replace(cutOffset, cutLength, &c, 1); }

    void Replace(PRUint32 cutOffset, PRUint32 cutLength,
                 const char_type *data, PRUint32 dataLen = PR_UINT32_MAX);

    void Replace(PRUint32 cutOffset, PRUint32 cutLength,
                 const fragment_type &frag);

    void Replace(PRUint32 cutOffset, PRUint32 cutLength,
                 const fragment_tuple_type &tuple);

    // append to string buffer

    void Append(char_type c)
      { Replace(mLength, 0, &c, 1); }

    void Append(const char_type *data, PRUint32 dataLen = PR_UINT32_MAX)
      { Replace(mLength, 0, data, dataLen); }

    void Append(const fragment_type &frag)
      { Replace(mLength, 0, frag); }

    void Append(const fragment_tuple_type &tuple)
      { Replace(mLength, 0, tuple); }

    self_type &operator+=(char_type c)
      { Append(c); return *this; }

    self_type &operator+=(const char_type *data)
      { Append(data); return *this; }

    self_type &operator+=(const fragment_type &frag)
      { Append(frag); return *this; }

    self_type &operator+=(const fragment_tuple_type &tuple)
      { Append(tuple); return *this; }

    // insert into string buffer

    void Insert(PRUint32 offset, char_type c)
      { Replace(offset, 0, &c, 1); }

    void Insert(PRUint32 offset, const char_type *data,
                PRUint32 dataLen = PR_UINT32_MAX)
      { Replace(offset, 0, data, dataLen); }

    void Insert(PRUint32 offset, const fragment_type &frag)
      { Replace(offset, 0, frag); }

    void Insert(PRUint32 offset, const fragment_tuple_type &tuple)
      { Replace(offset, 0, tuple); }

    // cut out section of string buffer

    void Cut(PRUint32 cutOffset, PRUint32 cutLength)
      { Replace(cutOffset, cutLength,
                nsStrFragment(char_traits::empty_string, PRUint32(0))); }

    void SetCapacity(PRUint32 capacity)
      { EnsureCapacity(capacity, PR_TRUE); }

    void SetLength(PRUint32 length);

    void Truncate(PRUint32 length)
      {
        NS_ASSERTION(length <= mLength, "|Trunate()| cannot make a string longer");
        SetLength(length);
      }

  protected:
    
    // helper constructor for subclasses
    nsStrBuf(char_type *data, PRUint32 length, PRUint32 flags, PRUint32 capacity)
      : nsStrRef(data, length, flags), mCapacity(capacity) {}

    PRBool EnsureCapacity(PRUint32 capacity, PRBool preserveData);

    // additional values for mFlags
    enum
      {
        F_CAPACITYSET = 0x10 // if set, then mCapacity applies to mData
      };

    // holds the current size of the buffer at mData
    PRUint32 mCapacity;
};
#endif

//-----------------------------------------------------------------------------

// subclass of nsStrBuf that makes use of fixed storage for small strings.
// this class is designed to be allocated on the stack.

class nsStrAutoBuf : public nsStrBuf
{
  public:
    typedef nsStrRef ref_type;
    typedef nsStrBuf self_type;

    // constructors

    nsStrAutoBuf()
      : nsStrBuf(mFixed, 0, F_DEPEND | F_CAPACITYSET, sizeof(mFixed))
      { mFixed[0] = char_type(0); }

    explicit
    nsStrAutoBuf(const char_type *data)
      : nsStrBuf(mFixed, 0, F_DEPEND | F_CAPACITYSET, sizeof(mFixed))
      { Assign(data); }

    nsStrAutoBuf(const char_type *data, PRUint32 dataLen)
      : nsStrBuf(mFixed, 0, F_DEPEND | F_CAPACITYSET, sizeof(mFixed))
      { Assign(data, dataLen); }

    explicit
    nsStrAutoBuf(const ref_type &ref)
      : nsStrBuf(mFixed, 0, F_DEPEND | F_CAPACITYSET, sizeof(mFixed))
      { Assign(ref); }

    explicit
    nsStrAutoBuf(const fragment_type &frag)
      : nsStrBuf(mFixed, 0, F_DEPEND | F_CAPACITYSET, sizeof(mFixed))
      { Assign(frag); }

    explicit
    nsStrAutoBuf(const fragment_tuple_type &tuple)
      : nsStrBuf(mFixed, 0, F_DEPEND | F_CAPACITYSET, sizeof(mFixed))
      { Assign(tuple); }

    // assign

    self_type &operator=(const self_type &buf)
      { Assign(buf); return *this; }

    self_type &operator=(const ref_type &ref)
      { Assign(ref); return *this; }

    self_type &operator=(const fragment_type &frag)
      { Assign(frag); return *this; }

    self_type &operator=(const fragment_tuple_type &tuple)
      { Assign(tuple); return *this; }

  protected:
    char_type mFixed[64]; // this is in use if F_DEPEND is set.
};

//-----------------------------------------------------------------------------

#endif // nsStrRef_h__
