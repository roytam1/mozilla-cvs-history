// -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
//
// The contents of this file are subject to the Netscape Public
// License Version 1.1 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of
// the License at http://www.mozilla.org/NPL/
//
// Software distributed under the License is distributed on an "AS
// IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
// implied. See the License for the specific language governing
// rights and limitations under the License.
//
// The Original Code is the JavaScript 2 Prototype.
//
// The Initial Developer of the Original Code is Netscape
// Communications Corporation.  Portions created by Netscape are
// Copyright (C) 2000 Netscape Communications Corporation. All
// Rights Reserved.

#ifndef gc_allocator_h
#define gc_allocator_h

#include <memory>

#ifndef _WIN32 // Microsoft VC6 bug: standard identifiers should be in std namespace
using std::size_t;
using std::ptrdiff_t;
#endif

namespace JavaScript {
    extern "C" {
        void* GC_malloc(size_t bytes);
        void* GC_malloc_atomic(size_t bytes);
        void GC_free(void* ptr);
        void GC_gcollect(void);

        typedef void (*GC_finalization_proc) (void* obj, void* client_data);
        void GC_register_finalizer(void* obj, GC_finalization_proc proc, void* client_data,
                                   GC_finalization_proc *old_proc, void* *old_client_data);
    }

    #if 0 && !defined(XP_MAC)
    // for platforms where GC doesn't exist yet.
    inline void* GC_malloc(size_t bytes) { return ::operator new(bytes); }
    inline void* GC_malloc_atomic(size_t bytes) { return ::operator new(bytes); }
    inline void GC_free(void* ptr) { operator delete(ptr); }
    inline void GC_gcollect() {}
    inline void GC_register_finalizer(void* obj, GC_finalization_proc proc, void* client_data,
                                      GC_finalization_proc *old_proc, void* *old_client_data) {}
    #endif
    
	/**
	 * General case:  memory for type must be allocated as a conservatively
	 * scanned block of memory.
	 */
	template <class T> struct gc_traits {
		static T* allocate(size_t n) { return static_cast<T*>(GC_malloc(n * sizeof(T))); }
	};
	
	/**
	 * Specializations for blocks of atomic types:  the macro define_atomic_type(_type)
	 * specializes gc_traits<T> for types that need not be scanned by the
	 * GC. Implementors are free to define other types as atomic, if they are
	 * guaranteed not to contain pointers.
	 */
	#define define_atomic_type(_type) 											\
	template <> struct gc_traits<_type> {										\
		static _type* allocate(size_t n)										\
		{																		\
			return static_cast<_type*>(GC_malloc_atomic(n * sizeof(_type)));	\
		}																		\
	};
	
	define_atomic_type(char)
	define_atomic_type(unsigned char)
	define_atomic_type(short)
	define_atomic_type(unsigned short)
	define_atomic_type(int)
	define_atomic_type(unsigned int)
	define_atomic_type(long)
	define_atomic_type(unsigned long)
	define_atomic_type(float)
	define_atomic_type(double)
	
	#undef define_atomic_type
	
	/**
	 * Traits for classes that need to have their destructor called
	 * when reclaimed by the garbage collector.
	 */
	template <class T> struct gc_traits_finalizable {
		static void finalizer(void* obj, void* client_data)
		{
			T* t = static_cast<T*>(obj);
			size_t n = reinterpret_cast<size_t>(client_data);
			for (size_t i = 0; i < n; ++i)
				t[i].~T();
		}
		
		static T* allocate(size_t n)
		{
			T* t = gc_traits<T>::allocate(n);
			GC_finalization_proc old_proc; void* old_client_data;
			GC_register_finalizer(t, &finalizer, reinterpret_cast<void*>(n), &old_proc, &old_client_data);
			return t;
		}
	};
	
	/**
	 * An allocator that can be used to allocate objects in the garbage collected heap.
	 */
	template <class T, class traits = gc_traits<T> > class gc_allocator {
	public:
		typedef T value_type;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef T *pointer;
		typedef const T *const_pointer;
		typedef T &reference;
		typedef const T &const_reference;
		
		gc_allocator() {}
		template<typename U, typename UTraits> gc_allocator(const gc_allocator<U, UTraits>&) {}
		// ~gc_allocator() {}

		static pointer address(reference r) { return &r; }
		static const_pointer address(const_reference r) { return &r; }
				
		static pointer allocate(size_type n, const void* /* hint */ = 0) { return traits::allocate(n); }
		static void deallocate(pointer, size_type) {}
		
		static void construct(pointer p, const T &val) { new(p) T(val);}
		static void destroy(pointer p) { p->~T(); }
		
#if defined(__GNUC__) || defined(_WIN32)
        static size_type max_size() { return size_type(-1) / sizeof(T); }
#else
		static size_type max_size() { return std::numeric_limits<size_type>::max() / sizeof(T); }
#endif

		template<class U> struct rebind { typedef gc_allocator<U> other; };

#ifdef _WIN32
		// raw byte allocator used on some platforms (grrr).
		typedef char _Char[1];
		static char* _Charalloc(size_type n) { return (char*) rebind<_Char>::other::allocate(n); }

        // funky operator required for calling basic_string<T> constructor (grrr).
        template<typename U, typename UTraits> int operator==(const gc_allocator<U, UTraits>&) { return 0; }
#endif

		// void* deallocate used on some platforms (grrr).
		static void deallocate(void*, size_type) {}

		static void collect() { GC_gcollect(); }
	};

    /**
     * Generic base class for objects allocated using a gc_allocator. How they are allocated
     * can be controlled by specializing gc_traits for the specific class.
     */
    template <typename T> class gc_object {
    public:
        void* operator new(size_t) { return gc_allocator<T>::allocate(1, 0); }
        void operator delete(void* /* ptr */) {}
    };

    /**
     * Simpler base class for classes that have no need to specialize allocation behavior.
     */
    class gc_base {
    public:
        void* operator new(size_t n) { return GC_malloc(n); }
        void operator delete(void*) {}
    };
}

#endif /* gc_allocator_h */
