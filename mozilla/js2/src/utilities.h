// -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
// Copyright (C) 1998 Netscape Communications Corporation. All
// Rights Reserved.

#ifndef utilities_h
#define utilities_h

#include <memory>
#include <string>
#include <iostream>
#include "systemtypes.h"

using std::size_t;
using std::ptrdiff_t;
using std::string;
using std::istream;
using std::ostream;
using std::auto_ptr;

namespace JavaScript {


//
// Assertions
//

	#ifdef DEBUG
	 void Assert(const char *s, const char *file, int line);

	 #define ASSERT(_expr) ((_expr) ? (void)0 : JavaScript::Assert(#_expr, __FILE__, __LINE__))
	 #define NOT_REACHED(_reasonStr) JavaScript::Assert(_reasonStr, __FILE__, __LINE__)
	 #define DEBUG_ONLY(_stmt) _stmt
	#else
	 #define ASSERT(expr)
	 #define NOT_REACHED(reasonStr)
	 #define DEBUG_ONLY(_stmt)
	#endif


//
// Numerics
//

	template<class N> N min(N v1, N v2) {return v1 <= v2 ? v1 : v2;}
	template<class N> N max(N v1, N v2) {return v1 >= v2 ? v1 : v2;}

//
// Bit manipulation
//

	#define JS_BIT(n)       ((uint32)1 << (n))
	#define JS_BITMASK(n)   (JS_BIT(n) - 1)

	uint ceilingLog2(uint32 n);
	uint floorLog2(uint32 n);


//
// Unicode UTF-16 characters and strings
//

	// A string of UTF-16 characters.  Nulls are allowed just like any other character.
	// The string is not null-terminated.
	// Use wstring if char16 is wchar_t.  Otherwise use basic_string<uint16>.
	//
	// Eventually we'll want to use a custom class better suited for JavaScript that generates less
	// code bloat and separates the concepts of a fixed, read-only string from a mutable buffer that
	// is expanding.  For now, though, we use the standard basic_string.
	typedef std::basic_string<char16> String;


	typedef uint32 char16orEOF;		// A type that can hold any char16 plus one special value: ueof.
	const char16orEOF char16eof = static_cast<char16orEOF>(-1);

	// If c is a char16, return it; if c is char16eof, return the character \uFFFF.
	inline char16 char16orEOFToChar16(char16orEOF c) {return static_cast<char16>(c);}

	// Special char16s
	namespace uni {
		const char16 null = '\0';
		const char16 cr = '\r';
		const char16 lf = '\n';
		const char16 ls = 0x2028;
		const char16 ps = 0x2029;
	}
	const uint16 firstFormatChar = 0x200C;	// Lowest Unicode Cf character
	
	inline char16 widen(char ch) {return static_cast<char16>(static_cast<uchar>(ch));}

	// Return a String containing the characters of the null-terminated C string cstr
	// (without the trailing null).
	inline String widenCString(const char *cstr)
	{
		size_t len = std::strlen(cstr);
		const uchar *ucstr = reinterpret_cast<const uchar *>(cstr);
		return String(ucstr, ucstr+len);
	}


	// Widen and append length characters starting at chars to the end of str.
	inline void appendChars(String &str, const char *chars, size_t length)
	{
		const uchar *uchars = reinterpret_cast<const uchar *>(chars);
		str.append(uchars, uchars + length);
	}


	String &operator+=(String &str, const char *cstr);
	String operator+(const String &str, const char *cstr);
	String operator+(const char *cstr, const String &str);
	inline String &operator+=(String &str, char c) {return str += widen(c);}
	inline void clear(String &s) {s.resize(0);}


	class CharInfo {
		uint32 info;					// Word from table a.

		// Unicode character attribute lookup tables
		static const uint8 x[];
		static const uint8 y[];
		static const uint32 a[];
	
	  public:
		// Enumerated Unicode general category types
		enum Type {
		    Unassigned            = 0,	// Cn
		    UppercaseLetter       = 1,	// Lu
		    LowercaseLetter       = 2,	// Ll
		    TitlecaseLetter       = 3,	// Lt
		    ModifierLetter        = 4,	// Lm
		    OtherLetter           = 5,	// Lo
		    NonSpacingMark        = 6,	// Mn
		    EnclosingMark         = 7,	// Me
		    CombiningSpacingMark  = 8,	// Mc
		    DecimalDigitNumber    = 9,	// Nd
		    LetterNumber          = 10,	// Nl
		    OtherNumber           = 11,	// No
		    SpaceSeparator        = 12,	// Zs
		    LineSeparator         = 13,	// Zl
		    ParagraphSeparator    = 14,	// Zp
		    Control               = 15,	// Cc
		    Format                = 16,	// Cf
		    PrivateUse            = 18,	// Co
		    Surrogate             = 19,	// Cs
		    DashPunctuation       = 20,	// Pd
		    StartPunctuation      = 21,	// Ps
		    EndPunctuation        = 22,	// Pe
		    ConnectorPunctuation  = 23,	// Pc
		    OtherPunctuation      = 24,	// Po
		    MathSymbol            = 25,	// Sm
		    CurrencySymbol        = 26,	// Sc
		    ModifierSymbol        = 27,	// Sk
		    OtherSymbol           = 28	// So
		};

		enum Group {
		    NonIdGroup,			// 0  May not be part of an identifier
		    FormatGroup,		// 1  Format control
		    IdGroup,			// 2  May start or continue a JS identifier (includes $ and _)
		    IdContinueGroup,	// 3  May continue a JS identifier  [IdContinueGroup & -2 == IdGroup]
		    WhiteGroup,			// 4  White space character (but not line break)
		    LineBreakGroup		// 5  Line break character  [LineBreakGroup & -2 == WhiteGroup]
		};

		CharInfo() {}
		CharInfo(char16 c): info(a[y[x[static_cast<uint16>(c)>>6]<<6 | c&0x3F]]) {}
		CharInfo(const CharInfo &ci): info(ci.info) {}

		friend Type cType(const CharInfo &ci) {return static_cast<Type>(ci.info & 0x1F);}
		friend Group cGroup(const CharInfo &ci) {return static_cast<Group>(ci.info >> 16 & 7);}

		friend bool isAlpha(const CharInfo &ci)
		{
			return ((((1 << UppercaseLetter) | (1 << LowercaseLetter) | (1 << TitlecaseLetter) | (1 << ModifierLetter) | (1 << OtherLetter))
					 >> cType(ci)) & 1) != 0;
		}

		friend bool isAlphanumeric(const CharInfo &ci)
		{
			return ((((1 << UppercaseLetter) | (1 << LowercaseLetter) | (1 << TitlecaseLetter) | (1 << ModifierLetter) | (1 << OtherLetter) |
					  (1 << DecimalDigitNumber) | (1 << LetterNumber))
					 >> cType(ci)) & 1) != 0;
		}

		// Return true if this character can start a JavaScript identifier
		friend bool isIdLeading(const CharInfo &ci) {return cGroup(ci) == IdGroup;}
		// Return true if this character can continue a JavaScript identifier
		friend bool isIdContinuing(const CharInfo &ci) {return cGroup(ci) & -2 == IdGroup;}

		// Return true if this character is a Unicode decimal digit (Nd) character
		friend bool isDecimalDigit(const CharInfo &ci) {return cType(ci) == DecimalDigitNumber;}
		// Return true if this character is a Unicode white space or line break character
		friend bool isSpace(const CharInfo &ci) {return cGroup(ci) & -2 == WhiteGroup;}
		// Return true if this character is a Unicode line break character (LF, CR, LS, or PS)
		friend bool isLineBreak(const CharInfo &ci) {return cGroup(ci) == LineBreakGroup;}
		// Return true if this character is a Unicode format control character (Cf)
		friend bool isFormat(const CharInfo &ci) {return cGroup(ci) == FormatGroup;}

		friend bool isUpper(const CharInfo &ci) {return cType(ci) == UppercaseLetter;}
		friend bool isLower(const CharInfo &ci) {return cType(ci) == LowercaseLetter;}

		friend char16 toUpper(char16 c);
		friend char16 toLower(char16 c);
	};
	
	inline bool isASCIIDecimalDigit(char16 c) {return c >= '0' && c <= '9';}
	bool isASCIIHexDigit(char16 c, uint &digit);

	const char16 *skipWhiteSpace(const char16 *str, const char16 *strEnd);


//
// Array auto_ptr's
//

	// An ArrayAutoPtr holds a pointer to an array initialized by new T[x].
	// A regular auto_ptr cannot be used here because it deletes its pointer using
	// delete rather than delete[].
	// An appropriate operator[] is also provided.
	template <typename T>
	class ArrayAutoPtr {
		T *ptr;
		
	  public:
		explicit ArrayAutoPtr(T *p = 0): ptr(p) {}
		ArrayAutoPtr(ArrayAutoPtr &a): ptr(a.ptr) {a.ptr = 0;}
		ArrayAutoPtr &operator=(ArrayAutoPtr &a) {reset(a.release());}
		~ArrayAutoPtr() {delete[] ptr;}
		
		T &operator*() const {return *ptr;}
		T &operator->() const {return *ptr;}
		template<class N> T &operator[](N i) const {return ptr[i];}
		T *get() const {return ptr;}
		T *release() {T *p = ptr; ptr = 0; return p;}
		void reset(T *p = 0) {delete[] ptr; ptr = p;}
	};
	
	typedef ArrayAutoPtr<char> CharAutoPtr;


//
// Growable arrays
//

	// private
	template <typename T>
	class ProtoArrayBuffer {
	  protected:
	    T *buffer;
	    int32 length;
	    int32 bufferSize;

	    void append(const T *elts, int32 nElts, T *cache);
	};


	// private
	template <typename T>
	void ProtoArrayBuffer<T>::append(const T *elts, int32 nElts, T *cache)
	{
	    assert(nElts >= 0);
	    int32 newLength = length + nElts;
	    if (newLength > bufferSize) {
	        // Allocate a new buffer and copy the current buffer's contents there.
	        int32 newBufferSize = newLength + bufferSize;
	        auto_ptr<T> newBuffer = new T[newBufferSize];
	        T *p = buffer;
	        T *pLimit = old + length;
	        T *q = newBuffer.get();
	        while (p != pLimit)
	            *q++ = *p++;
	        if (buffer != cache)
	            delete buffer;
	        buffer = newBuffer.release();
	        bufferSize = newBufferSize;
	    }
	    length = newLength;
	}


	// An ArrayBuffer represents an array of elements of type T.  The ArrayBuffer contains
	// storage for a fixed size array of cacheSize elements; if this size is exceeded, the
	// ArrayBuffer allocates the array from the heap.
	// Use append to append nElts elements to the end of the ArrayBuffer.
	template <typename T, int32 cacheSize>
	class ArrayBuffer: public ProtoArrayBuffer<T> {
	    T cache[cacheSize];

	  public:
	    ArrayBuffer() {buffer = &cache; length = cacheSize; bufferSize = cacheSize;}
	    ~ArrayBuffer() {if (buffer != &cache) delete buffer;}
	    
	    int32 size() const {return length;}
	    T *front() const {return buffer;}
	    void append(const T *elts, int32 nElts) {ProtoArrayBuffer<T>::append(elts, nElts, cache);}
	};



//
// Algorithms
//

	// Assign zero to every element between first inclusive and last exclusive.
	// This is equivalent ot fill(first, last, 0) but may be more efficient.
	template<class For>
	inline void zero(For first, For last)
	{
		while (first != last)
			*first++ = 0;
	}


	// Assign zero to n elements starting at first.
	// This is equivalent ot fill_n(first, n, 0) but may be more efficient.
	template<class For, class Size>
	inline void zero_n(For first, Size n)
	{
		while (n--)
			*first++ = 0;
	}


//
// C++ I/O
//


	// A class to remember the format of an ostream so that a function may modify it internally
	// without changing it for the caller.
	class SaveFormat {
		ostream &o;
		std::ios_base::fmtflags flags;
		char fill;
	  public:
		explicit SaveFormat(ostream &out);
		~SaveFormat();
	};


	void showChar(ostream &out, char16 ch);

	template<class In>
	void showString(ostream &out, In begin, In end)
	{
		while (begin != end)
			showChar(out, *begin++);
	}
	void showString(ostream &out, const String &str);


//
// Exceptions
//

	// A JavaScript exception (other than out-of-memory, for which we use the standard C++
	// exception bad_alloc).
	struct Exception {
		enum Kind {
			SyntaxError
		};
		
		Kind kind;						// The exception's kind
		String message;					// The detailed message
		String sourceFile;				// A description of the source code that caused the error
		uint32 lineNum;					// One-based source line number; 0 if unknown
		uint32 charPos;					// Zero-based character offset within the line
		String sourceLine;				// The text of the source line

		Exception(Kind kind, const String &message): kind(kind), message(message), lineNum(0) {}
		Exception(Kind kind, const String &message, const String &sourceFile, uint32 lineNum, uint32 charPos, const String sourceLine):
			kind(kind), message(message), sourceFile(sourceFile), lineNum(lineNum), charPos(charPos), sourceLine(sourceLine) {}
			
		const char *kindString() const;
		String fullMessage() const;
	};
}
#endif
