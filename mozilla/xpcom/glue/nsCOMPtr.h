/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsCOMPtr_h___
#define nsCOMPtr_h___



  // Wrapping includes can speed up compiles (see "Large Scale C++ Software Design")
#ifndef nsDebug_h___
  #include "nsDebug.h"
    // for |NS_PRECONDITION|
#endif

#ifndef nsISupports_h___
  #include "nsISupports.h"
    // for |nsresult|, |NS_ADDREF|, et al
#endif

/*
	Public things defined in this file:

																												T* rawTptr;
		class nsCOMPtr<T>																		nsCOMPtr<T> smartTptr;

		null_nsCOMPtr()																			smartTptr = null_nsCOMPtr();

		do_QueryInterface( nsISupports* )										smartTptr = do_QueryInterface(other_ptr);
		do_QueryInterface( nsISupports*, nsresult* )				smartTptr = do_QueryInterface(other_ptr, &status);

		dont_QueryInterface( T* )														smartTptr = dont_QueryInterface(rawTptr);

		getter_AddRefs( nsCOMPtr<T>& )
		getter_AddRefs( T* )
		dont_AddRef( T* )

		CallQueryInterface( nsISupports*, T** )
		CallQueryInterface( nsISupports*, nsCOMPtr<T>* )
*/

/*
	Having problems?
	
  See the User Manual at:
    <http://www.meer.net/ScottCollins/doc/nsCOMPtr.html>, or
    <http://www.mozilla.org/projects/xpcom/nsCOMPtr.html>
*/




/*
  TO DO...
  
    + Improve internal documentation
      + mention *&
      + alternatives for comparison
      + do_QueryInterface
*/

/*
  WARNING:
    This file defines several macros for internal use only.  These macros begin with the
    prefix |NSCAP_|.  Do not use these macros in your own code.  They are for internal use
    only for cross-platform compatibility, and are subject to change without notice.
*/


  /*
    Set up some |#define|s to turn off a couple of troublesome C++ features.
    Interestingly, none of the compilers barf on template stuff.  These are set up automatically
    by the autoconf system for all Unixes.  (Temporarily, I hope) I have to define them
    myself for Mac and Windows.
  */

	// under Metrowerks (Mac), we don't have autoconf yet
#ifdef __MWERKS__
	#define HAVE_CPP_USING
	#define HAVE_CPP_EXPLICIT
	#define HAVE_CPP_NEW_CASTS
	#define HAVE_CPP_BOOL
#endif

	// under VC++ (Windows), we don't have autoconf yet
#ifdef _MSC_VER
	#define HAVE_CPP_EXPLICIT
	#define HAVE_CPP_USING
	#define HAVE_CPP_NEW_CASTS

  #if (_MSC_VER<1100)
		  // before 5.0, VC++ couldn't handle explicit
    #undef HAVE_CPP_EXPLICIT
  #elif (_MSC_VER==1100)
      // VC++5.0 has an internal compiler error (sometimes) without this
    #undef HAVE_CPP_USING
  #endif

	#define NSCAP_FEATURE_INLINE_STARTASSIGNMENT
		// under VC++, we win by inlining StartAssignment
#endif

#define NSCAP_FEATURE_ALLOW_RAW_POINTERS
#define NSCAP_FEATURE_ALLOW_COMPARISONS
#define NSCAP_FEATURE_FACTOR_DESTRUCTOR


	/*
		If the compiler doesn't support |explicit|, we'll just make it go away, trusting
		that the builds under compilers that do have it will keep us on the straight and narrow.
	*/
#ifndef HAVE_CPP_EXPLICIT
  #define explicit
#endif

#ifdef HAVE_CPP_BOOL
	typedef bool NSCAP_BOOL;
#else
	typedef PRBool NSCAP_BOOL;
#endif

#ifdef HAVE_CPP_NEW_CASTS
	#define NSCAP_STATIC_CAST(T,x)	static_cast<T>(x)
	#define NSCAP_REINTERPRET_CAST(T,x)	reinterpret_cast<T>(x)
#else
	#define NSCAP_STATIC_CAST(T,x) ((T)(x))
	#define NSCAP_REINTERPRET_CAST(T,x) ((T)(x))
#endif

#ifdef NSCAP_FEATURE_DEBUG_MACROS
  #define NSCAP_ADDREF(ptr)    NS_ADDREF(ptr)
  #define NSCAP_RELEASE(ptr)   NS_RELEASE(ptr)
#else
  #define NSCAP_ADDREF(ptr)    (ptr)->AddRef()
  #define NSCAP_RELEASE(ptr)   (ptr)->Release()
#endif

  /*
    WARNING:
      VC++4.2 is very picky.  To compile under VC++4.2, the classes must be defined
      in an order that satisfies:
    
        nsDerivedSafe < nsCOMPtr
        nsDontAddRef < nsCOMPtr
        nsCOMPtr < nsGetterAddRefs

      The other compilers probably won't complain, so please don't reorder these
      classes, on pain of breaking 4.2 compatibility.
  */


template <class T>
class nsDerivedSafe : public T
    /*
      No client should ever see or have to type the name of this class.  It is the
      artifact that makes it a compile-time error to call |AddRef| and |Release|
      on a |nsCOMPtr|.  DO NOT USE THIS TYPE DIRECTLY IN YOUR CODE.

      See |nsCOMPtr::operator->|, |nsCOMPtr::operator*|, et al.
    */
  {
    private:
#ifdef HAVE_CPP_USING
      using T::AddRef;
      using T::Release;
#else
      NS_IMETHOD_(nsrefcnt) AddRef(void);
      NS_IMETHOD_(nsrefcnt) Release(void);
#endif

      void operator delete( void*, size_t );                  // NOT TO BE IMPLEMENTED
        // declaring |operator delete| private makes calling delete on an interface pointer a compile error

      nsDerivedSafe<T>& operator=( const nsDerivedSafe<T>& ); // NOT TO BE IMPLEMENTED
        // you may not call |operator=()| through a dereferenced |nsCOMPtr|, because you'd get the wrong one
  };

#if !defined(HAVE_CPP_USING) && defined(NEED_CPP_UNUSED_IMPLEMENTATIONS)
template <class T>
nsrefcnt
nsDerivedSafe<T>::AddRef()
  {
    return 0;
  }

template <class T>
nsrefcnt
nsDerivedSafe<T>::Release()
  {
    return 0;
  }

#endif




template <class T>
struct nsDontQueryInterface
    /*
      ...

			DO NOT USE THIS TYPE DIRECTLY IN YOUR CODE.  Use |dont_QueryInterface()| instead.
    */
  {
    explicit
    nsDontQueryInterface( T* aRawPtr )
        : mRawPtr(aRawPtr)
      {
        // nothing else to do here
      }

    T* mRawPtr;
  };

template <class T>
inline
const nsDontQueryInterface<T>
dont_QueryInterface( T* aRawPtr )
  {
    return nsDontQueryInterface<T>(aRawPtr);
  }




struct nsQueryInterface
    /*
      ...

			DO NOT USE THIS TYPE DIRECTLY IN YOUR CODE.  Use |do_QueryInterface()| instead.
    */
  {
    explicit
    nsQueryInterface( nsISupports* aRawPtr, nsresult* error = 0 )
        : mRawPtr(aRawPtr),
          mErrorPtr(error)
      {
        // nothing else to do here
      }

    nsISupports* mRawPtr;
    nsresult*    mErrorPtr;
  };

inline
const nsQueryInterface
do_QueryInterface( nsISupports* aRawPtr, nsresult* error = 0 )
  {
    return nsQueryInterface(aRawPtr, error);
  }

inline
const nsQueryInterface
null_nsCOMPtr()
		/*
			You can use this to assign |NULL| into an |nsCOMPtr|, e.g.,

        myPtr = null_nsCOMPtr();
		*/
	{
		typedef nsISupports* nsISupports_Ptr;
		return nsQueryInterface(nsISupports_Ptr(0));
	}




template <class T>
struct nsDontAddRef
    /*
      ...cooperates with |nsCOMPtr| to allow you to assign in a pointer _without_
      |AddRef|ing it.  You would rarely use this directly, but rather through the
      machinery of |getter_AddRefs| in the argument list to functions that |AddRef|
      their results before returning them to the caller.

			DO NOT USE THIS TYPE DIRECTLY IN YOUR CODE.  Use |getter_AddRefs()| instead.

      See also |getter_AddRefs()| and |class nsGetterAddRefs|.
    */
  {
    explicit
    nsDontAddRef( T* aRawPtr )
        : mRawPtr(aRawPtr)
      {
        // nothing else to do here
      }

    T* mRawPtr;
  };

template <class T>
inline
const nsDontAddRef<T>
getter_AddRefs( T* aRawPtr )
    /*
      ...makes typing easier, because it deduces the template type, e.g., 
      you write |dont_AddRef(fooP)| instead of |nsDontAddRef<IFoo>(fooP)|.
    */
	{
		return nsDontAddRef<T>(aRawPtr);
	}

	// This call is now deprecated.  Use |getter_AddRefs()| instead.
template <class T>
inline
const nsDontAddRef<T>
dont_AddRef( T* aRawPtr )
  {
    return nsDontAddRef<T>(aRawPtr);
  }



class nsCOMPtr_base
		/*
			...factors implementation for all template versions of |nsCOMPtr|.
		*/
  {
    public:

      nsCOMPtr_base( nsISupports* rawPtr = 0 )
          : mRawPtr(rawPtr)
        {
          // nothing else to do here
        }

#ifdef NSCAP_FEATURE_FACTOR_DESTRUCTOR
		 ~nsCOMPtr_base();
#endif

#if 0
     ~nsCOMPtr_base()
        {
          if ( mRawPtr )
            NSCAP_RELEASE(mRawPtr);
        }
#endif

      NS_EXPORT void    assign_with_AddRef( nsISupports* );
      NS_EXPORT void    assign_with_QueryInterface( nsISupports*, const nsIID&, nsresult* );
      NS_EXPORT void**  begin_assignment();

    protected:
      nsISupports* mRawPtr;
  };



template <class T>
class nsCOMPtr : private nsCOMPtr_base
    /*
      ...
    */
  {
    public:
      typedef T element_type;

#ifndef NSCAP_FEATURE_FACTOR_DESTRUCTOR
		 ~nsCOMPtr()
				{
					if ( mRawPtr )
						NSCAP_RELEASE(mRawPtr);
				}
#endif

      nsCOMPtr()
          // : nsCOMPtr_base(0)
        {
          // nothing else to do here
        }

      nsCOMPtr( const nsQueryInterface& aSmartPtr )
          // : nsCOMPtr_base(0)
        {
          assign_with_QueryInterface(aSmartPtr.mRawPtr, T::GetIID(), aSmartPtr.mErrorPtr);
        }

      nsCOMPtr( const nsDontAddRef<T>& aSmartPtr )
          : nsCOMPtr_base(aSmartPtr.mRawPtr)
        {
          // nothing else to do here
        }

      nsCOMPtr( const nsDontQueryInterface<T>& aSmartPtr )
          : nsCOMPtr_base(aSmartPtr.mRawPtr)
        {
          if ( mRawPtr )
            NSCAP_ADDREF(mRawPtr);
        }

      nsCOMPtr( const nsCOMPtr<T>& aSmartPtr )
          : nsCOMPtr_base(aSmartPtr.mRawPtr)
        {
          if ( mRawPtr )
            NSCAP_ADDREF(mRawPtr);
        }

#ifdef NSCAP_FEATURE_ALLOW_RAW_POINTERS
			nsCOMPtr( T* aRawPtr )
					: nsCOMPtr_base(aRawPtr)
				{
					if ( mRawPtr )
						NSCAP_ADDREF(mRawPtr);
				}

      nsCOMPtr<T>&
      operator=( T* rhs )
        {
          assign_with_AddRef(rhs);
          return *this;
        }
#endif

      nsCOMPtr<T>&
      operator=( const nsQueryInterface& rhs )
        {
          assign_with_QueryInterface(rhs.mRawPtr, T::GetIID(), rhs.mErrorPtr);
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsDontAddRef<T>& rhs )
        {
          if ( mRawPtr )
            NSCAP_RELEASE(mRawPtr);
          mRawPtr = rhs.mRawPtr;
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsDontQueryInterface<T>& rhs )
        {
          assign_with_AddRef(rhs.mRawPtr);
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsCOMPtr<T>& rhs )
        {
          assign_with_AddRef(rhs.mRawPtr);
          return *this;
        }

      nsDerivedSafe<T>*
      get() const
          // returns a |nsDerivedSafe<T>*| to deny clients the use of |AddRef| and |Release|
        {
          return NSCAP_REINTERPRET_CAST(nsDerivedSafe<T>*, mRawPtr);
        }

      nsDerivedSafe<T>*
      operator->() const
          // returns a |nsDerivedSafe<T>*| to deny clients the use of |AddRef| and |Release|
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsCOMPtr with operator->().");
          return get();
        }

      nsDerivedSafe<T>&
      operator*() const
          // returns a |nsDerivedSafe<T>*| to deny clients the use of |AddRef| and |Release|
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsCOMPtr with operator*().");
          return *get();
        }

      operator nsDerivedSafe<T>*() const
        {
          return get();
        }

#if 0
    private:
      friend class nsGetterAddRefs<T>;

      /*
        In a perfect world, the following member function, |StartAssignment|, would be private.
        It is and should be only accessed by the closely related class |nsGetterAddRefs<T>|.

        Unfortunately, some compilers---most notably VC++5.0---fail to grok the
        friend declaration above or in any alternate acceptable form.  So, physically
        it will be public (until our compilers get smarter); but it is not to be
        considered part of the logical public interface.
      */
#endif

      T**
      StartAssignment()
        {
#ifndef NSCAP_FEATURE_INLINE_STARTASSIGNMENT
          return NSCAP_REINTERPRET_CAST(T**, begin_assignment());
#else
					if ( mRawPtr )
					  NSCAP_RELEASE(mRawPtr);
					mRawPtr = 0;
					return NSCAP_REINTERPRET_CAST(T**, &mRawPtr);
#endif
        }
  };


template <class T>
class nsGetterAddRefs
    /*
      ...

      This class is designed to be used for anonymous temporary objects in the
      argument list of calls that return COM interface pointers, e.g.,

        nsCOMPtr<IFoo> fooP;
        ...->QueryInterface(iid, getter_AddRefs(fooP))

			DO NOT USE THIS TYPE DIRECTLY IN YOUR CODE.  Use |getter_AddRefs()| instead.

      When initialized with a |nsCOMPtr|, as in the example above, it returns
      a |void**| (or |T**| if needed) that the outer call (|QueryInterface| in this
      case) can fill in.
    */
  {
    public:
      explicit
      nsGetterAddRefs( nsCOMPtr<T>& aSmartPtr )
          : mTargetSmartPtr(aSmartPtr)
        {
          // nothing else to do
        }

      operator void**()
        {
          return NSCAP_REINTERPRET_CAST(void**, mTargetSmartPtr.StartAssignment());
        }

      T*&
      operator*()
        {
          return *(mTargetSmartPtr.StartAssignment());
        }

      operator T**()
        {
          return mTargetSmartPtr.StartAssignment();
        }

    private:
      nsCOMPtr<T>& mTargetSmartPtr;
  };

template <class T>
inline
nsGetterAddRefs<T>
getter_AddRefs( nsCOMPtr<T>& aSmartPtr )
    /*
      Used around a |nsCOMPtr| when 
      ...makes the class |nsGetterAddRefs<T>| invisible.
    */
  {
    return nsGetterAddRefs<T>(aSmartPtr);
  }


#ifdef NSCAP_FEATURE_ALLOW_COMPARISONS

	/*
		Note: can't enable this till I find a suitable replacement for |bool|.
	*/

template <class T, class U>
inline
NSCAP_BOOL
operator==( const nsCOMPtr<T>& lhs, const nsCOMPtr<U>& rhs )
	{
		return NSCAP_STATIC_CAST(const void*, lhs.get()) == NSCAP_STATIC_CAST(const void*, rhs.get());
	}

template <class T, class U>
inline
NSCAP_BOOL
operator==( const nsCOMPtr<T>& lhs, const U* rhs )
	{
		return NSCAP_STATIC_CAST(const void*, lhs.get()) == NSCAP_STATIC_CAST(const void*, rhs);
	}

template <class T, class U>
inline
NSCAP_BOOL
operator==( const U* lhs, const nsCOMPtr<T>& rhs )
	{
		return NSCAP_STATIC_CAST(const void*, lhs) == NSCAP_STATIC_CAST(const void*, rhs.get());
	}

template <class T, class U>
inline
NSCAP_BOOL
operator!=( const nsCOMPtr<T>& lhs, const nsCOMPtr<U>& rhs )
	{
		return NSCAP_STATIC_CAST(const void*, lhs.get()) != NSCAP_STATIC_CAST(const void*, rhs.get());
	}

template <class T, class U>
inline
NSCAP_BOOL
operator!=( const nsCOMPtr<T>& lhs, const U* rhs )
	{
		return NSCAP_STATIC_CAST(const void*, lhs.get()) != NSCAP_STATIC_CAST(const void*, rhs);
	}

template <class T, class U>
inline
NSCAP_BOOL
operator!=( const U* lhs, const nsCOMPtr<T>& rhs )
	{
		return NSCAP_STATIC_CAST(const void*, lhs) != NSCAP_STATIC_CAST(const void*, rhs.get());
	}

inline
NSCAP_BOOL
SameCOMIdentity( nsISupports* lhs, nsISupports* rhs )
	{
		return nsCOMPtr<nsISupports>( do_QueryInterface(lhs) ) == nsCOMPtr<nsISupports>( do_QueryInterface(rhs) );
	}

#endif // defined(NSCAP_FEATURE_ALLOW_COMPARISONS)



template <class SourceType, class DestinationType>
inline
nsresult
CallQueryInterface( SourceType* aSource, nsCOMPtr<DestinationType>* aDestination )
		// a type-safe shortcut for calling the |QueryInterface()| member function
	{
		NS_PRECONDITION(aSource, "null parameter");
		NS_PRECONDITION(aDestination, "null parameter");

		return aSource->QueryInterface(DestinationType::GetIID(), NS_STATIC_CAST(void**, getter_AddRefs(*aDestination)));
	}



#endif // !defined(nsCOMPtr_h___)
