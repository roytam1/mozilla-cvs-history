/*
 * (C) Copyright The MITRE Corporation 1999  All rights reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * The program provided "as is" without any warranty express or
 * implied, including the warranty of non-infringement and the implied
 * warranties of merchantibility and fitness for a particular purpose.
 * The Copyright owner will not be liable for any damages suffered by
 * you as a result of using the Program. In no event will the Copyright
 * owner be liable for any special, indirect or consequential damages or
 * lost profits even if the Copyright owner has been advised of the
 * possibility of their occurrence.
 *
 * Contributor(s):
 *
 * Tom Kneeland
 *    -- original author.
 *
 * Keith Visco <kvisco@ziplink.net>
 * Larry Fitzpatrick
 *
 */

#ifndef txString_h__
#define txString_h__

#include "baseutils.h"

#include "TxObject.h"
#include <iostream.h>
#include "nsString.h"

#ifdef TX_EXE
class txCaseInsensitiveStringComparator
: public nsStringComparator
{
public:
    virtual int operator()(const char_type*, const char_type*, PRUint32 aLength) const;
    virtual int operator()(char_type, char_type) const;
};
#endif

class String
: public TxObject
{
public:
    /*
     * Default constructor.
     */
    String();

    /*
     * Copying constructor.
     */
    String(const String& aSource);

#ifdef TX_EXE
    /*
     * Constructor, allocates a buffer and copies the supplied string buffer.
     * If aLength is zero it computes the length from the supplied string.
     */
    explicit String(const PRUnichar* aSource, PRUint32 aLength = 0);
#endif
    explicit String(const nsAString& aSource);
    ~String();

    /*
     * Append aSource to this string.
     */
    void Append(PRUnichar aSource);
    void Append(const String& aSource);
    void Append(const PRUnichar* aSource, PRUint32 aLength);
    void Append(const nsAString& aSource);

    /*
     * Insert aSource at aOffset in this string.
     */
    void insert(PRUint32 aOffset, PRUnichar aSource);
    void insert(PRUint32 aOffset, const String& aSource);

    /*
     * Replace characters starting at aOffset with aSource.
     */
    void replace(PRUint32 aOffset, PRUnichar aSource);
    void replace(PRUint32 aOffset, const String& aSource);

    /*
     * Delete aCount characters starting at aOffset.
     */
    void Cut(PRUint32 aOffset, PRUint32 aCount);

    /*
     * Returns the character at aIndex. Caller needs to check the
     * index for out-of-bounds errors.
     */
    PRUnichar CharAt(PRUint32 aIndex) const;

    /*
     * Returns index of first occurrence of aData.
     */
    PRInt32 indexOf(PRUnichar aData,
                    PRInt32 aOffset = 0) const;
    PRInt32 indexOf(const String& aData, PRInt32 aOffset = 0) const;

    /*
     * Returns index of last occurrence of aData.
     */
    PRInt32 RFindChar(PRUnichar aData,
                        PRInt32 aOffset = -1) const;

    /*
     * Check equality between strings.
     */
    MBool Equals(const String& aData) const;

    /*
     * Check equality (ignoring case) between strings.
     */
    MBool isEqualIgnoreCase(const String& aData) const;

    /*
     * Check whether the string is empty.
     */
    MBool IsEmpty() const;

    /*
     * Return the length of the string.
     */
    PRUint32 Length() const;

    /*
     * Returns a substring starting at start
     * Note: the dest String is cleared before use
     */
    String& subString(PRUint32 aStart, String& aDest) const;

    /*
     * Returns the subString starting at start and ending at end
     * Note: the dest String is cleared before use
     */
    String& subString(PRUint32 aStart, PRUint32 aEnd,
                      String& aDest) const;

    /*
     * Convert string to lowercase.
     */
    void toLowerCase();

    /*
     * Convert string to uppercase.
     */
    void toUpperCase();

    /*
     * Shorten the string to aLength.
     */
    void Truncate(PRUint32 aLength = 0);

    /*
     * Return a reference to this string's nsString.
     */
    operator nsAString&();

    /*
     * Return a const reference to this string's nsString.
     */
    operator const nsAString&() const;

private:
    nsString mString;

    friend ostream& operator << (ostream& aOutput, const String& aSource);

// XXX NOT IMPLEMENTED
    explicit String(PRUint32 aSize);
    explicit String(char* aSource);
    explicit String(char aSource);
    void Append(char* aSource);
    void Append(char aSource);
    MBool Equals(char* aSource);
    MBool Equals(char aSource);
// XXX NOT IMPLEMENTED

// XXX DEPRECATED
public:
    nsString& getNSString();
    const nsString& getConstNSString() const;
// XXX DEPRECATED
};

/*
 * Translate PRUnichars to Chars and output to the provided stream.
 */
ostream& operator << (ostream& aOutput, const String& aSource);

// txMozillaString.h contains all inline implementations for the 
// Mozilla module.
#include "txMozillaString.h"

#endif // txString_h__
