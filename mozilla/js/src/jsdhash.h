/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla JavaScript code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999,2000 Netscape Communications Corporation.
 * All Rights Reserved.
 *
 * Contributor(s): 
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

#ifndef jsdhash_h___
#define jsdhash_h___
/*
 * Double hashing, a la Knuth 6.
 */
#include "jstypes.h"

JS_BEGIN_EXTERN_C

/* Minimum table size, or gross entry count (net is at most .75 loaded). */
#ifndef JS_DHASH_MIN_SIZE
#define JS_DHASH_MIN_SIZE 16
#endif

/*
 * Multiplicative hash uses an unsigned 32 bit integer and the golden ratio,
 * expressed as a fixed-point 32-bit fraction.
 */
#define JS_DHASH_BITS           32
#define JS_DHASH_GOLDEN_RATIO   0x9E3779B9U

/* Primitive and forward-struct typedefs. */
typedef uint32                  JSDHashNumber;
typedef struct JSDHashEntryHdr  JSDHashEntryHdr;
typedef struct JSDHashTable     JSDHashTable;
typedef struct JSDHashTableOps  JSDHashTableOps;

/*
 * Table entry header structure.
 *
 * In order to allow in-line allocation of key and value, we do not declare
 * either here.  Instead, the API uses const void *key as a formal parameter,
 * and asks each entry for its key when necessary via a getKey callback, used
 * when growing or shrinking the table.  Other callback types are defined
 * below and grouped into the JSDHashTableOps structure, for single static
 * initialization per hash table sub-type.
 *
 * Each hash table sub-type should nest the JSDHashEntryHdr structure at the
 * front of its particular entry type.  The keyHash member contains the result
 * of multiplying the hash code returned from the hashKey callback (see below)
 * by JS_DHASH_GOLDEN_RATIO.  Its value is table size invariant.  keyHash is
 * maintained automatically by JS_DHashTableOperate -- users should never set
 * it, and its only uses should be via the entry macros below.
 */
struct JSDHashEntryHdr {
    JSDHashNumber       keyHash;        /* every entry must begin like this */
};

#define JS_DHASH_ENTRY_IS_FREE(entry)   ((entry)->keyHash == 0)
#define JS_DHASH_ENTRY_IS_BUSY(entry)   (!JS_DHASH_ENTRY_IS_FREE(entry))

/*
 * A JSDHashTable is currently 8 words (without the JS_DHASHMETER overhead)
 * on most architectures, and may be allocated on the stack or within another
 * structure or class (see below for the Init and Finish functions to use).
 */
struct JSDHashTable {
    JSDHashTableOps     *ops;           /* virtual operations, see below */
    void                *data;          /* ops- and instance-specific data */
    int16               hashShift;      /* multiplicative hash shift */
    int16               sizeLog2;       /* log2(table size) */
    uint32              sizeMask;       /* JS_BITMASK(log2(table size)) */
    uint32              entrySize;      /* number of bytes in an entry */
    uint32              entryCount;     /* number of entries in table */
    uint32              removedCount;   /* removed entry sentinels in table */
    char                *entryStore;    /* entry storage */
#ifdef JS_DHASHMETER
    struct JSDHashStats {
        uint32          searches;       /* total number of table searches */
        uint32          steps;          /* hash chain links traversed */
        uint32          hits;           /* searches that found key */
        uint32          misses;         /* searches that didn't find key */
        uint32          lookups;        /* number of JS_DHASH_LOOKUPs */
        uint32          addMisses;      /* adds that miss, and do work */
        uint32          addHits;        /* adds that hit an existing entry */
        uint32          addFailures;    /* out-of-memory during add growth */
        uint32          removeHits;     /* removes that hit, and do work */
        uint32          removeMisses;   /* useless removes that miss */
        uint32          removeEnums;    /* removes done by Enumerate */
        uint32          grows;          /* table expansions */
        uint32          shrinks;        /* table contractions */
    } stats;
#endif
};

#ifndef CRT_CALL
#ifdef XP_OS2_VACPP
#define CRT_CALL _Optlink
#else
#define CRT_CALL
#endif
#endif

/*
 * Table space at entryStore is allocated and freed using these callbacks.
 * The allocator should return null on error only (not if called with nbytes
 * equal to 0; but note that jsdhash.c code will never call with 0 nbytes).
 */
typedef void *
(* CRT_CALL JSDHashAllocTable)(JSDHashTable *table, uint32 nbytes);

typedef void
(* CRT_CALL JSDHashFreeTable) (JSDHashTable *table, void *ptr);

/*
 * When a table grows or shrinks, each entry is queried for its key using this
 * callback.  NB: in that event, entry is not in table any longer; it's in the
 * old entryStore vector, which is due to be freed once all entries have been
 * moved via moveEntry callbacks.
 */
typedef const void *
(* CRT_CALL JSDHashGetKey)    (JSDHashTable *table, JSDHashEntryHdr *entry);

/*
 * Compute the hash code for a given key to be looked up, added, or removed
 * from table.  A hash code may have any JSDHashNumber value.
 */
typedef JSDHashNumber
(* CRT_CALL JSDHashHashKey)   (JSDHashTable *table, const void *key);

/*
 * Compare the key identifying entry in table with the provided key parameter.
 * Return JS_TRUE if keys match, JS_FALSE otherwise.
 */
typedef JSBool
(* CRT_CALL JSDHashMatchEntry)(JSDHashTable *table,
                               const JSDHashEntryHdr *entry,
                               const void *key);

/*
 * Copy the data starting at from to the new entry storage at to.  Do not add
 * reference counts for any strong references in the entry, however, as this
 * is a "move" operation: the old entry storage at from will be freed without
 * any reference-decrementing callback shortly.
 */
typedef void
(* CRT_CALL JSDHashMoveEntry)(JSDHashTable *table,
                              const JSDHashEntryHdr *from,
                              JSDHashEntryHdr *to);

/*
 * Clear the entry and drop any strong references it holds.  This callback is
 * invoked during a JS_DHASH_REMOVE operation (see below for operation codes),
 * but only if the given key is found in the table.
 */
typedef void
(* CRT_CALL JSDHashClearEntry)(JSDHashTable *table, JSDHashEntryHdr *entry);

/*
 * Called when a table (whether allocated dynamically by itself, or nested in
 * a larger structure, or allocated on the stack) is finished.  This callback
 * allows table->ops-specific code to finalize table->data.
 */
typedef void
(* CRT_CALL JSDHashFinalize)  (JSDHashTable *table);

/* Finally, the "vtable" structure for JSDHashTable. */
struct JSDHashTableOps {
    JSDHashAllocTable   allocTable;
    JSDHashFreeTable    freeTable;
    JSDHashGetKey       getKey;
    JSDHashHashKey      hashKey;
    JSDHashMatchEntry   matchEntry;
    JSDHashMoveEntry    moveEntry;
    JSDHashClearEntry   clearEntry;
    JSDHashFinalize     finalize;
};

/*
 * Default implementations for some of the above ops.
 */
extern JS_PUBLIC_API(void *)
JS_DHashAllocTable(JSDHashTable *table, uint32 nbytes);

extern JS_PUBLIC_API(void)
JS_DHashFreeTable(JSDHashTable *table, void *ptr);

extern JS_PUBLIC_API(JSDHashNumber)
JS_DHashStringKey(JSDHashTable *table, const void *key);

extern JS_PUBLIC_API(void)
JS_DHashFinalizeStub(JSDHashTable *table);

/*
 * Dynamically allocate a new JSDHashTable using malloc, initialize it using
 * JS_DHashTableInit, and return its address.  Return null on malloc failure.
 * Note that the entry storage at table->entryStore will be allocated using
 * the ops->allocTable callback.
 */
extern JS_PUBLIC_API(JSDHashTable *)
JS_NewDHashTable(JSDHashTableOps *ops, void *data, uint32 entrySize,
                 uint32 capacity);

/*
 * Finalize table's data, free its entry storage (via table->ops->freeTable),
 * and return the memory starting at table to the malloc heap.
 */
extern JS_PUBLIC_API(void)
JS_DHashTableDestroy(JSDHashTable *table);

/*
 * Initialize table with ops, data, entrySize, and capacity.  Capacity is a
 * guess for the smallest table size at which the table will usually be less
 * than 75% loaded (the table will grow or shrink as needed; capacity serves
 * only to avoid inevitable early growth from JS_DHASH_MIN_SIZE).
 */
extern JS_PUBLIC_API(JSBool)
JS_DHashTableInit(JSDHashTable *table, JSDHashTableOps *ops, void *data,
                  uint32 entrySize, uint32 capacity);

/*
 * Finalize table's data, free its entry storage using table->ops->freeTable,
 * and leave its members unchanged from their last live values (which leaves
 * pointers dangling).  If you want to burn cycles clearing table, it's up to
 * your code to call memset.
 */
extern JS_PUBLIC_API(void)
JS_DHashTableFinish(JSDHashTable *table);

/*
 * To consolidate keyHash computation and table grow/shrink code, we use a
 * single entry point for lookup, add, and remove operations.  The operation
 * codes are declared here, along with codes returned by JSDHashEnumerator
 * functions, which control JS_DHashTableEnumerate's behavior.
 */
typedef enum JSDHashOperator {
    JS_DHASH_LOOKUP = 0,        /* lookup entry */
    JS_DHASH_ADD = 1,           /* add entry */
    JS_DHASH_REMOVE = 2,        /* remove entry, or enumerator says remove */
    JS_DHASH_NEXT = 0,          /* enumerator says continue */
    JS_DHASH_STOP = 1           /* enumerator says stop */
} JSDHashOperator;

/*
 * To lookup a key in table, call:
 *
 *  entry = JS_DHashTableOperate(table, key, JS_DHASH_LOOKUP);
 *
 * If JS_DHASH_ENTRY_IS_BUSY(entry) is true, key was found and it identifies
 * entry.  If JS_DHASH_ENTRY_IS_FREE(entry) is true, key was not found.
 *
 * To add an entry identified by key to table, call:
 *
 *  entry = JS_DHashTableOperate(table, key, JS_DHASH_ADD);
 *
 * If entry is null upon return, the table is severely overloaded, and new
 * memory can't be allocated for new entry storage via table->ops->allocTable.
 * Otherwise, entry->keyHash has been set so that JS_DHASH_ENTRY_IS_BUSY(entry)
 * is true, and it is up to the caller to initialize the key and value parts
 * of the entry sub-type, if they have not been set already (i.e. if entry was
 * not already in the table).
 *
 * To remove an entry identified by key from table, call:
 *
 *  (void) JS_DHashTableOperate(table, key, JS_DHASH_REMOVE);
 *
 * If key's entry is found, it is cleared (via table->ops->clearEntry) and
 * the entry is marked so that JS_DHASH_ENTRY_IS_FREE(entry).  This operation
 * returns null unconditionally; you should ignore its return value.
 */
extern JS_PUBLIC_API(JSDHashEntryHdr *)
JS_DHashTableOperate(JSDHashTable *table, const void *key, JSDHashOperator op);

/*
 * Enumerate entries in table using etor:
 *
 *   count = JS_DHashTableEnumerate(table, etor, arg);
 *
 * JS_DHashTableEnumerate calls etor like so:
 *
 *   op = etor(table, entry, number, arg);
 *
 * where number is a zero-based ordinal assigned to live entries according to
 * their order in table->entryStore.
 *
 * The return value, op, is treated as a set of flags.  If op is JS_DHASH_NEXT,
 * then continue enumerating.  If op contains JS_DHASH_REMOVE, then clear (via
 * table->ops->clearEntry) and free entry.  Then we check whether op contains
 * JS_DHASH_STOP; if so, stop enumerating and return the number of live entries
 * that were enumerated so far.  Return the total number of live entries when
 * enumeration completes normally.
 *
 * If etor adds or removes an entry, it must return JS_DHASH_STOP; otherwise,
 * undefined behavior results.
 */
typedef JSDHashOperator
(* CRT_CALL JSDHashEnumerator)(JSDHashTable *table, JSDHashEntryHdr *hdr,
                               uint32 number, void *arg);

extern JS_PUBLIC_API(uint32)
JS_DHashTableEnumerate(JSDHashTable *table, JSDHashEnumerator etor, void *arg);

#ifdef JS_DHASHMETER
#include <stdio.h>

extern JS_PUBLIC_API(void)
JS_DHashTableDumpMeter(JSDHashTable *table, JSDHashEnumerator dump, FILE *fp);
#endif

JS_END_EXTERN_C

#endif /* jsdhash_h___ */
