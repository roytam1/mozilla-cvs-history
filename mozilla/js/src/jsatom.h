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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
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

#ifndef jsatom_h___
#define jsatom_h___
/*
 * JS atom table.
 */
#include <stddef.h>
#include "jstypes.h"
#include "jshash.h" /* Added by JSIFY */
#include "jsapi.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#ifdef JS_THREADSAFE
#include "jslock.h"
#endif

JS_BEGIN_EXTERN_C

#define ATOM_NOCOPY     0x01            /* don't copy atom string bytes */
#define ATOM_TMPSTR     0x02            /* internal, to avoid extra string */
#define ATOM_MARK       0x04            /* atom is reachable via GC */
#define ATOM_PINNED     0x08            /* atom is pinned against GC */

struct JSAtom {
    JSHashEntry         entry;          /* key is jsval, value keyword info */
    uint8               flags;          /* flags, PINNED and/or MARK for now */
    int8                kwindex;        /* keyword index, -1 if not keyword */
    jsatomid            number;         /* atom serial number and hash code */
};

#define ATOM_KEY(atom)           ((jsval)(atom)->entry.key)
#define ATOM_IS_OBJECT(atom)     JSVAL_IS_OBJECT(ATOM_KEY(atom))
#define ATOM_TO_OBJECT(atom)     JSVAL_TO_OBJECT(ATOM_KEY(atom))
#define ATOM_IS_INT(atom)        JSVAL_IS_INT(ATOM_KEY(atom))
#define ATOM_TO_INT(atom)        JSVAL_TO_INT(ATOM_KEY(atom))
#define ATOM_IS_DOUBLE(atom)     JSVAL_IS_DOUBLE(ATOM_KEY(atom))
#define ATOM_TO_DOUBLE(atom)     JSVAL_TO_DOUBLE(ATOM_KEY(atom))
#define ATOM_IS_STRING(atom)     JSVAL_IS_STRING(ATOM_KEY(atom))
#define ATOM_TO_STRING(atom)     JSVAL_TO_STRING(ATOM_KEY(atom))
#define ATOM_IS_BOOLEAN(atom)    JSVAL_IS_BOOLEAN(ATOM_KEY(atom))
#define ATOM_TO_BOOLEAN(atom)    JSVAL_TO_BOOLEAN(ATOM_KEY(atom))
#define ATOM_BYTES(atom)         JS_GetStringBytes(ATOM_TO_STRING(atom))

struct JSAtomListElement {
    JSHashEntry         entry;
};

#define ALE_ATOM(ale)   ((JSAtom *) (ale)->entry.key)
#define ALE_INDEX(ale)  ((jsatomid) (ale)->entry.value)
#define ALE_NODE(ale)   ((JSParseNode *) (ale)->entry.value)
#define ALE_NEXT(ale)   ((JSAtomListElement *) (ale)->entry.next)

#define ALE_SET_ATOM(ale,atom)  ((ale)->entry.key = (const void *)(atom))
#define ALE_SET_INDEX(ale,index)((ale)->entry.value = (void *)(index))
#define ALE_SET_NODE(ale,pn)    ((ale)->entry.value = (void *)(pn))
#define ALE_SET_NEXT(ale,link)  ((ale)->entry.next = (JSHashEntry *)(link))

struct JSAtomList {
    JSAtomListElement   *list;          /* literals indexed for mapping */
    JSHashTable         *table;         /* hash table if list gets too long */
    jsuint              count;          /* count of indexed literals */
};

#define ATOM_LIST_INIT(al)  ((al)->list = NULL, (al)->table = NULL,           \
                             (al)->count = 0)

#define ATOM_LIST_SEARCH(_ale,_al,_atom)                                      \
    JS_BEGIN_MACRO                                                            \
        JSHashEntry **_hep;                                                   \
        ATOM_LIST_LOOKUP(_ale, _hep, _al, _atom);                             \
    JS_END_MACRO

#define ATOM_LIST_LOOKUP(_ale,_hep,_al,_atom)                                 \
    JS_BEGIN_MACRO                                                            \
        if ((_al)->table) {                                                   \
            _hep = JS_HashTableRawLookup((_al)->table, _atom->number, _atom); \
            _ale = *_hep ? (JSAtomListElement *) *_hep : NULL;                \
        } else {                                                              \
            JSAtomListElement **_alep = &(_al)->list;                         \
            _hep = NULL;                                                      \
            while ((_ale = *_alep) != NULL) {                                 \
                if (ALE_ATOM(_ale) == (_atom)) {                              \
                    /* Hit, move atom's element to the front of the list. */  \
                    *_alep = ALE_NEXT(_ale);                                  \
                    ALE_SET_NEXT(_ale, (_al)->list);                          \
                    (_al)->list = _ale;                                       \
                    break;                                                    \
                }                                                             \
                _alep = (JSAtomListElement **)&_ale->entry.next;              \
            }                                                                 \
        }                                                                     \
    JS_END_MACRO

struct JSAtomMap {
    JSAtom              **vector;       /* array of ptrs to indexed atoms */
    jsatomid            length;         /* count of (to-be-)indexed atoms */
};

struct JSAtomState {
    JSRuntime           *runtime;       /* runtime that owns us */
    JSHashTable         *table;         /* hash table containing all atoms */
    jsatomid            number;         /* one beyond greatest atom number */

    /* Type names and value literals. */
    JSAtom              *typeAtoms[JSTYPE_LIMIT];
    JSAtom              *booleanAtoms[2];
    JSAtom              *nullAtom;

    /* Various built-in or commonly-used atoms. */
    JSAtom              *ArrayAtom;
    JSAtom              *MathAtom;
    JSAtom              *NaNAtom;
    JSAtom              *ObjectAtom;
    JSAtom              *anonymousAtom;
    JSAtom              *argumentsAtom;
    JSAtom              *arityAtom;
    JSAtom              *calleeAtom;
    JSAtom              *callerAtom;
    JSAtom              *classPrototypeAtom;
    JSAtom              *constructorAtom;
    JSAtom              *countAtom;
    JSAtom              *getterAtom;
    JSAtom              *getAtom;
    JSAtom              *indexAtom;
    JSAtom              *inputAtom;
    JSAtom              *lengthAtom;
    JSAtom              *nameAtom;
    JSAtom              *parentAtom;
    JSAtom              *protoAtom;
    JSAtom              *setterAtom;
    JSAtom              *setAtom;
    JSAtom              *toSourceAtom;
    JSAtom              *toStringAtom;
    JSAtom              *valueOfAtom;
    JSAtom              *evalAtom;

#ifdef JS_THREADSAFE
    JSThinLock          lock;
    volatile uint32     tablegen;
#endif
};

/* Well-known predefined strings and their atoms. */
extern char   *js_type_str[];
extern char   *js_boolean_str[];

extern char   js_Array_str[];
extern char   js_Math_str[];
extern char   js_NaN_str[];
extern char   js_Object_str[];
extern char   js_anonymous_str[];
extern char   js_arguments_str[];
extern char   js_arity_str[];
extern char   js_callee_str[];
extern char   js_caller_str[];
extern char   js_class_prototype_str[];
extern char   js_constructor_str[];
extern char   js_count_str[];
extern char   js_eval_str[];
extern char   js_getter_str[];
extern char   js_get_str[];
extern char   js_index_str[];
extern char   js_input_str[];
extern char   js_length_str[];
extern char   js_name_str[];
extern char   js_parent_str[];
extern char   js_proto_str[];
extern char   js_setter_str[];
extern char   js_set_str[];
extern char   js_toSource_str[];
extern char   js_toString_str[];
extern char   js_toLocaleString_str[];
extern char   js_valueOf_str[];

/*
 * Initialize atom state.  Return true on success, false with an out of
 * memory error report on failure.
 */
extern JSBool
js_InitAtomState(JSContext *cx, JSAtomState *state);

/*
 * Free and clear atom state.
 */
extern void
js_FreeAtomState(JSContext *cx, JSAtomState *state);

/*
 * Atom garbage collection hooks.
 */
typedef void
(*JSGCThingMarker)(JSRuntime *rt, void *thing);

extern void
js_MarkAtomState(JSAtomState *state, JSGCThingMarker mark);

extern void
js_SweepAtomState(JSAtomState *state, uintN gcflags);

extern void
js_UnpinPinnedAtoms(JSAtomState *state);

/*
 * Find or create the atom for an object.  If we create a new atom, give it the
 * type indicated in flags.  Return 0 on failure to allocate memory.
 */
extern JSAtom *
js_AtomizeObject(JSContext *cx, JSObject *obj, uintN flags);

/*
 * Find or create the atom for a Boolean value.  If we create a new atom, give
 * it the type indicated in flags.  Return 0 on failure to allocate memory.
 */
extern JSAtom *
js_AtomizeBoolean(JSContext *cx, JSBool b, uintN flags);

/*
 * Find or create the atom for an integer value.  If we create a new atom, give
 * it the type indicated in flags.  Return 0 on failure to allocate memory.
 */
extern JSAtom *
js_AtomizeInt(JSContext *cx, jsint i, uintN flags);

/*
 * Find or create the atom for a double value.  If we create a new atom, give
 * it the type indicated in flags.  Return 0 on failure to allocate memory.
 */
extern JSAtom *
js_AtomizeDouble(JSContext *cx, jsdouble d, uintN flags);

/*
 * Find or create the atom for a string.  If we create a new atom, give it the
 * type indicated in flags.  Return 0 on failure to allocate memory.
 */
extern JSAtom *
js_AtomizeString(JSContext *cx, JSString *str, uintN flags);

extern JS_FRIEND_API(JSAtom *)
js_Atomize(JSContext *cx, const char *bytes, size_t length, uintN flags);

extern JS_FRIEND_API(JSAtom *)
js_AtomizeChars(JSContext *cx, const jschar *chars, size_t length, uintN flags);

/*
 * This variant handles all value tag types.
 */
extern JSAtom *
js_AtomizeValue(JSContext *cx, jsval value, uintN flags);

/*
 * Convert v to an atomized string.
 */
extern JSAtom *
js_ValueToStringAtom(JSContext *cx, jsval v);

/*
 * Assign atom an index and insert it on al.
 */
extern JSAtomListElement *
js_IndexAtom(JSContext *cx, JSAtom *atom, JSAtomList *al);

/*
 * Get the atom with index i from map.
 */
extern JS_FRIEND_API(JSAtom *)
js_GetAtom(JSContext *cx, JSAtomMap *map, jsatomid i);

/*
 * For all unmapped atoms recorded in al, add a mapping from the atom's index
 * to its address.  The GC must not run until all indexed atoms in atomLists
 * have been mapped by scripts connected to live objects (Function and Script
 * class objects have scripts as/in their private data -- the GC knows about
 * these two classes).
 */
extern JS_FRIEND_API(JSBool)
js_InitAtomMap(JSContext *cx, JSAtomMap *map, JSAtomList *al);

/*
 * Free map->vector and clear map.
 */
extern JS_FRIEND_API(void)
js_FreeAtomMap(JSContext *cx, JSAtomMap *map);

JS_END_EXTERN_C

#endif /* jsatom_h___ */
