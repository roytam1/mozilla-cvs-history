/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla FastLoad code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Brendan Eich <brendan@mozilla.org> (original author)
 */

#ifndef nsFastLoadFile_h___
#define nsFastLoadFile_h___

/**
 * Mozilla FastLoad file format and helper types.
 */

#include "prtypes.h"
#include "pldhash.h"

#include "nsBinaryStream.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsID.h"
#include "nsMemory.h"
#include "nsVoidArray.h"

#include "nsISeekableStream.h"

/**
 * FastLoad file Object ID (OID) is an identifier for multiply and cyclicly
 * connected objects in the serialized graph of all reachable objects.
 *
 * Holy Mixed Metaphors: JS, after Common Lisp, uses #n= to define a "sharp
 * variable" naming an object that's multiply or cyclicly connected, and #n#
 * to stand for a connection to an already-defined object.  We too call any
 * object with multiple references "sharp", and (here it comes) any object
 * with only one reference "dull".
 *
 * Note that only sharp objects require a mapping from OID to FastLoad file
 * offset and other information.  Dull objects can be serialized _in situ_
 * (where they are referenced) and deserialized when their (singular, shared)
 * OID is scanned.
 *
 * We also compress 16-byte XPCOM IDs into 32-bit dense identifiers to save
 * space.  See nsFastLoadFooter, below, for the mapping data structure used to
 * compute an nsID given an NSFastLoadID.
 */
typedef PRUint32 NSFastLoadID;          // nsFastLoadFooter::mIDMap index
typedef PRUint32 NSFastLoadOID;         // nsFastLoadFooter::mObjectMap index

/**
 * A Mozilla FastLoad file is an untagged (in general) stream of objects and
 * primitive-type data.  Small integers are fairly common, and could easily be
 * confused for NSFastLoadIDs and NSFastLoadOIDs.  To help catch bugs where
 * reader and writer code fail to match, we XOR unlikely 32-bit numbers with
 * NSFastLoad*IDs when storing and fetching.  The following unlikely values are
 * irrational numbers ((1-sqrt(5))/2, sqrt(2)-1) represented in fixed point.
 *
 * The reader XORs, converts the ID to an index, and bounds-checks all array
 * accesses that use the index.  Array access code asserts that the index is in
 * bounds, and returns a dummy array element if it isn't.
 */
#define MFL_ID_XOR_KEY  0x9E3779B9      // key XOR'd with ID when serialized
#define MFL_OID_XOR_KEY 0x6A09E667      // key XOR'd with OID when serialized

/**
 * An OID can be tagged to introduce the serialized definition of the object,
 * or to stand for a strong or weak reference to that object.  Thus the high
 * 29 bits actually identify the object, and the low three bits tell whether
 * the object is being defined or just referenced -- and via what inheritance
 * chain or inner object, if necessary.
 *
 * The MFL_QUERY_INTERFACE_TAG bit helps us cope with aggregation and multiple
 * inheritance: object identity follows the XPCOM rule, but a deserializer may
 * need to query for an interface not on the primary inheritance chain ending
 * in the nsISupports whose address uniquely identifies the XPCOM object being
 * referenced or defined.
 */
#define MFL_OBJECT_TAG_BITS     3
#define MFL_OBJECT_TAG_MASK     PR_BITMASK(MFL_OBJECT_TAG_BITS)

#define MFL_OBJECT_DEF_TAG      1U      // object definition follows this OID
#define MFL_WEAK_REF_TAG        2U      // OID weakly refers to a prior object
                                        // NB: do not confuse with nsWeakPtr!
#define MFL_QUERY_INTERFACE_TAG 4U      // QI object to the ID follows this OID
                                        // NB: an NSFastLoadID, not an nsIID!

/**
 * The dull object identifier introduces the definition of all objects that
 * have only one (necessarily strong) ref in the serialization.  The definition
 * appears at the point of reference.
 */
#define MFL_DULL_OBJECT_OID     MFL_OBJECT_DEF_TAG

/**
 * Convert an OID to an index into nsFastLoadFooter::mObjectMap.
 */
#define MFL_OID_TO_SHARP_INDEX(oid)     (((oid) >> MFL_OBJECT_TAG_BITS) - 1)

/**
 * Magic "number" at start of a FastLoad file.  Inspired by the PNG "magic"
 * string, which inspired XPCOM's typelib (.xpt) file magic.  Guaranteed to
 * be corrupted by FTP-as-ASCII and other likely errors, meaningful to savvy
 * humans, and ending in ^Z to terminate erroneous text input on Windows.
 */
#define MFL_FILE_MAGIC          "XPCOM\nMozFASL\r\n\032"
#define MFL_FILE_MAGIC_SIZE     16

#define MFL_FILE_VERSION_0      0
#define MFL_FILE_VERSION_1      1000
#define MFL_FILE_VERSION        2       // experimental, muxed doc support

/**
 * Compute Fletcher's 16-bit checksum over aLength bytes starting at aBuffer,
 * with the initial accumulators seeded from aChecksum.
 */
PR_EXTERN(PRUint32)
NS_AccumulateFastLoadChecksum(PRUint32 aChecksum,
                              const char* aBuffer,
                              PRUint32 aLength);

/**
 * Header at the start of a FastLoad file.
 */
struct nsFastLoadHeader {
    char        mMagic[MFL_FILE_MAGIC_SIZE];
    PRUint32    mChecksum;
    PRUint32    mVersion;
    PRUint32    mFooterOffset;
    PRUint32    mFileSize;
};

/**
 * Footer prefix structure (footer header, ugh), after which come arrays of
 * structures or strings counted by these members.
 */
struct nsFastLoadFooterPrefix {
    PRUint32    mNumIDs;
    PRUint32    mNumSharpObjects;
    PRUint32    mNumMuxedDocuments;
    PRUint32    mNumDependencies;
};

struct nsFastLoadSharpObjectInfo {
    PRUint32    mCIDOffset;     // offset of object's NSFastLoadID and data
    PRUint16    mStrongRefCnt;
    PRUint16    mWeakRefCnt;
};

struct nsFastLoadMuxedDocumentInfo {
    const char* mURISpec;
    PRUint32    mInitialSegmentOffset;
};

// Specialize nsVoidArray to avoid gratuitous string copying, yet not leak.
class NS_COM nsFastLoadDependencyArray : public nsVoidArray {
  public:
    ~nsFastLoadDependencyArray() {
        for (PRInt32 i = 0, n = Count(); i < n; i++)
            nsMemory::Free(ElementAt(i));
    }

    /**
     * Append aFileName to this dependency array.  Hand off the memory at
     * aFileName if aCopy is false, otherwise clone it with nsMemory.
     */
    PRBool AppendDependency(const char* aFileName, PRBool aCopy = PR_TRUE) {
        char* s = NS_CONST_CAST(char*, aFileName);
        if (aCopy) {
            s = NS_REINTERPRET_CAST(char*, nsMemory::Clone(s, strlen(s) + 1));
            if (!s) return PR_FALSE;
        }
        PRBool ok = AppendElement(s);
        if (!ok && aCopy)
            nsMemory::Free(s);
        return ok;
    }
};

// forward declarations of opaque types defined in nsFastLoadFile.cpp
struct nsDocumentMapReadEntry;
struct nsDocumentMapWriteEntry;

/**
 * Inherit from the concrete class nsBinaryInputStream, which inherits from
 * abstract nsIObjectInputStream but does not implement its direct methods.
 * Though the names are not as clear as I'd like, this seems to be the best
 * way to share nsBinaryStream.cpp code.
 */
class NS_COM nsFastLoadFileReader
    : public nsBinaryInputStream,
      public nsISeekableStream
{
  public:
    nsFastLoadFileReader(nsIInputStream *aStream)
      : nsBinaryInputStream(aStream),
        mCurrentDocumentMapEntry(nsnull) {
        NS_INIT_REFCNT();
    }

    virtual ~nsFastLoadFileReader() { }

    PRUint32 GetChecksum() { return mHeader.mChecksum; }

    nsresult StartMuxedDocument(nsISupports* aURI, const char* aURISpec);
    nsresult SelectMuxedDocument(nsISupports* aURI);
    nsresult EndMuxedDocument(nsISupports* aURI);

  private:
    // nsISupports methods
    NS_DECL_ISUPPORTS_INHERITED

    // overridden nsIObjectInputStream methods
    NS_IMETHOD ReadObject(PRBool aIsStrongRef, nsISupports* *_retval);
    NS_IMETHOD ReadID(nsID *aResult);

    // nsISeekableStream methods
    NS_DECL_NSISEEKABLESTREAM

    // Override Read so we can demultiplex a document interleaved with others.
    NS_IMETHOD Read(char* aBuffer, PRUint32 aCount, PRUint32 *aBytesRead);

    nsresult ReadHeader(nsFastLoadHeader *aHeader);

    /**
     * In-memory representation of an indexed nsFastLoadSharpObjectInfo record.
     */
    struct nsFastLoadSharpObjectEntry : public nsFastLoadSharpObjectInfo {
        nsCOMPtr<nsISupports>   mObject;
    };

    /**
     * In-memory representation of the FastLoad file footer.
     */
    struct nsFastLoadFooter : public nsFastLoadFooterPrefix {
        nsFastLoadFooter()
          : mIDMap(nsnull),
            mObjectMap(nsnull) {
            mDocumentMap.ops = mURIMap.ops = nsnull;
        }

        ~nsFastLoadFooter() {
            delete[] mIDMap;
            delete[] mObjectMap;
            if (mDocumentMap.ops)
                PL_DHashTableFinish(&mDocumentMap);
            if (mURIMap.ops)
                PL_DHashTableFinish(&mURIMap);
        }

        const nsID& GetID(NSFastLoadID aFastId) const {
            static nsID dummy;
            PRUint32 index = aFastId - 1;
            NS_ASSERTION(index < mNumIDs, "aFastId out of range");
            if (index >= mNumIDs)
                return dummy;
            return mIDMap[index];
        }

        nsFastLoadSharpObjectEntry&
        GetSharpObjectEntry(NSFastLoadOID aOID) const {
            static nsFastLoadSharpObjectEntry dummy;
            PRUint32 index = MFL_OID_TO_SHARP_INDEX(aOID);
            NS_ASSERTION(index < mNumSharpObjects, "aOID out of range");
            if (index >= mNumSharpObjects)
                return dummy;
            return mObjectMap[index];
        }

        const char* GetDependency(PRUint32 aIndex) const {
            static const char dummy[] = "no such file";
            NS_ASSERTION(aIndex < mNumDependencies, "aIndex out of range");
            if (aIndex >= mNumDependencies)
                return dummy;
            PRInt32 index = aIndex;
            return NS_REINTERPRET_CAST(const char*,
                                       mDependencies.ElementAt(PRInt32(index)));
        }

        PRBool AppendDependency(const char* aFileName, PRBool aCopy = PR_TRUE) {
            return mDependencies.AppendDependency(aFileName, aCopy);
        }

        // Map from dense, zero-based, uint32 NSFastLoadID to 16-byte nsID.
        nsID* mIDMap;

        // Map from dense, zero-based MFL_OID_TO_SHARP_INDEX(oid) to sharp
        // object offset and refcnt information.
        nsFastLoadSharpObjectEntry* mObjectMap;

        // Map from URI spec string to nsDocumentMapReadEntry, which helps us
        // demultiplex a document's objects from among the interleaved object
        // stream segments in the FastLoad file.
        PLDHashTable mDocumentMap;

        // Fast mapping from URI object pointer to mDocumentMap entry, valid
        // only while the muxed document is loading.
        PLDHashTable mURIMap;

        // List of source filename dependencies that should trigger regeneration
        // of the FastLoad file.
        nsFastLoadDependencyArray mDependencies;
    };

    nsresult ReadFooter(nsFastLoadFooter *aFooter);
    nsresult ReadFooterPrefix(nsFastLoadFooterPrefix *aFooterPrefix);
    nsresult ReadSlowID(nsID *aID);
    nsresult ReadFastID(NSFastLoadID *aID);
    nsresult ReadSharpObjectInfo(nsFastLoadSharpObjectInfo *aInfo);
    nsresult ReadMuxedDocumentInfo(nsFastLoadMuxedDocumentInfo *aInfo);
    nsresult DeserializeObject(nsISupports* *aObject);

    nsresult   Open();
    NS_IMETHOD Close();

  protected:
    nsFastLoadHeader mHeader;
    nsFastLoadFooter mFooter;

    nsDocumentMapReadEntry* mCurrentDocumentMapEntry;
};

NS_COM nsresult
NS_NewFastLoadFileReader(nsIObjectInputStream* *aResult,
                         nsIInputStream* aSrcStream);

/**
 * Inherit from the concrete class nsBinaryInputStream, which inherits from
 * abstract nsIObjectInputStream but does not implement its direct methods.
 * Though the names are not as clear as I'd like, this seems to be the best
 * way to share nsBinaryStream.cpp code.
 */
class NS_COM nsFastLoadFileWriter
    : public nsBinaryOutputStream,
      public nsISeekableStream
{
  public:
    nsFastLoadFileWriter(nsIOutputStream *aStream)
      : nsBinaryOutputStream(aStream),
        mCurrentDocumentMapEntry(nsnull) {
        NS_INIT_REFCNT();
        mIDMap.ops = mObjectMap.ops = mDocumentMap.ops = mURIMap.ops = nsnull;
    }

    virtual ~nsFastLoadFileWriter() {
        if (mIDMap.ops)
            PL_DHashTableFinish(&mIDMap);
        if (mObjectMap.ops)
            PL_DHashTableFinish(&mObjectMap);
        if (mDocumentMap.ops)
            PL_DHashTableFinish(&mDocumentMap);
        if (mURIMap.ops)
            PL_DHashTableFinish(&mURIMap);
    }

    nsresult StartMuxedDocument(nsISupports* aURI, const char* aURISpec);
    nsresult SelectMuxedDocument(nsISupports* aURI);
    nsresult EndMuxedDocument(nsISupports* aURI);

    PRUint32 GetDependencyCount() { return PRUint32(mDependencies.Count()); }

    const char* GetDependency(PRUint32 i) {
        return NS_REINTERPRET_CAST(const char*,
                                   mDependencies.ElementAt(PRInt32(i)));
    }

    PRBool AppendDependency(const char* aFileName) {
        return mDependencies.AppendDependency(aFileName);
    }

  private:
    // nsISupports methods
    NS_DECL_ISUPPORTS_INHERITED

    // overridden nsIObjectOutputStream methods
    NS_IMETHOD WriteObject(nsISupports* aObject, PRBool aIsStrongRef);
    NS_IMETHOD WriteSingleRefObject(nsISupports* aObject);
    NS_IMETHOD WriteCompoundObject(nsISupports* aObject,
                                   const nsIID& aIID,
                                   PRBool aIsStrongRef);
    NS_IMETHOD WriteID(const nsID& aID);

    // nsISeekableStream methods
    NS_DECL_NSISEEKABLESTREAM

    nsresult MapID(const nsID& aSlowID, NSFastLoadID *aResult);

    nsresult WriteHeader(nsFastLoadHeader *aHeader);
    nsresult WriteFooter();
    nsresult WriteFooterPrefix(const nsFastLoadFooterPrefix& aFooterPrefix);
    nsresult WriteSlowID(const nsID& aID);
    nsresult WriteFastID(NSFastLoadID aID);
    nsresult WriteSharpObjectInfo(const nsFastLoadSharpObjectInfo& aInfo);
    nsresult WriteMuxedDocumentInfo(const nsFastLoadMuxedDocumentInfo& aInfo);

    nsresult   Open();
    NS_IMETHOD Close(void);

    nsresult WriteObjectCommon(nsISupports* aObject,
                               PRBool aIsStrongRef,
                               PRUint32 aQITag);

    static PLDHashOperator PR_CALLBACK
    IDMapEnumerate(PLDHashTable *aTable,
                   PLDHashEntryHdr *aHdr,
                   PRUint32 aNumber,
                   void *aData);

    static PLDHashOperator PR_CALLBACK
    ObjectMapEnumerate(PLDHashTable *aTable,
                       PLDHashEntryHdr *aHdr,
                       PRUint32 aNumber,
                       void *aData);

    static PLDHashOperator PR_CALLBACK
    DocumentMapEnumerate(PLDHashTable *aTable,
                         PLDHashEntryHdr *aHdr,
                         PRUint32 aNumber,
                         void *aData);

  protected:
    PLDHashTable mIDMap;
    PLDHashTable mObjectMap;
    PLDHashTable mDocumentMap;
    PLDHashTable mURIMap;

    nsDocumentMapWriteEntry* mCurrentDocumentMapEntry;

    nsFastLoadDependencyArray mDependencies;
};

NS_COM nsresult
NS_NewFastLoadFileWriter(nsIObjectOutputStream* *aResult,
                         nsIOutputStream* aDestStream);

#endif // nsFastLoadFile_h___
