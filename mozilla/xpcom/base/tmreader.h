/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is tmreader.h/tmreader.c code, released
 * July 7, 2000.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Brendan Eich, 7-July-2000
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 */
#ifndef tmreader_h___
#define tmreader_h___

#include "prtypes.h"
#include "plhash.h"
#include "nsTraceMalloc.h"

PR_BEGIN_EXTERN_C

typedef struct tmreader     tmreader;
typedef struct tmevent      tmevent;
typedef struct tmgraphedge  tmgraphedge;
typedef struct tmgraphnode  tmgraphnode;
typedef struct tmcallsite   tmcallsite;

struct tmevent {
    char            type;
    uint32          serial;
    union {
        char        *libname;
        struct {
            uint32  library;
            char    *name;
        } method;
        struct {
            uint32  parent;
            uint32  method;
            uint32  offset;
        } site;
        struct {
            uint32  oldsize;
            uint32  size;
        } alloc;
        struct {
            nsTMStats tmstats;
            uint32  calltree_maxkids_parent;
            uint32  calltree_maxstack_top;
        } stats;
    } u;
};

typedef struct tmcounts {
    int32           direct;     /* things allocated by this node's code */
    int32           total;      /* direct + things from all descendents */
} tmcounts;

struct tmgraphnode {
    PLHashEntry     entry;      /* key is serial or name, value must be name */
    tmgraphedge     *in;
    tmgraphedge     *out;
    tmgraphnode     *up;        /* parent in supergraph, e.g., JS for JS_*() */
    tmgraphnode     *down;      /* subgraph kids, declining bytes.total order */
    tmgraphnode     *next;      /* next kid in supergraph node's down list */
    int             low;        /* 0 or lowest current tree walk level */
    tmcounts        bytes;      /* bytes (direct and total) allocated */
    tmcounts        allocs;     /* number of allocations */
    double          sqsum;      /* sum of squared bytes.direct */
    int             sort;       /* sorted index in node table, -1 if no table */
};

#define tmgraphnode_name(node)  ((char*) (node)->entry.value)

#define tmlibrary_serial(lib)   ((uint32) (lib)->entry.key)
#define tmcomponent_name(comp)  ((const char*) (comp)->entry.key)

struct tmgraphedge {
    tmgraphedge     *next;
    tmgraphnode     *node;
    tmcounts        bytes;
};

struct tmcallsite {
    PLHashEntry     entry;
    tmcallsite      *parent;
    tmcallsite      *siblings;
    tmcallsite      *kids;
    tmgraphnode     *method;
    uint32          offset;
    tmcounts        bytes;
    tmcounts        allocs;
};

struct tmreader {
    const char      *program;
    void            *data;
    PLHashTable     *libraries;
    PLHashTable     *components;
    PLHashTable     *methods;
    PLHashTable     *callsites;
    tmcallsite      calltree_root;
};

typedef void (*tmeventhandler)(tmreader *tmr, tmevent *event);

extern tmreader     *tmreader_new(const char *program, void *data);
extern void         tmreader_destroy(tmreader *tmr);
extern int          tmreader_loop(tmreader *tmr, const char *filename,
                                  tmeventhandler eventhandler);
extern tmcallsite   *tmreader_get_callsite(tmreader *tmr, uint32 serial);

PR_END_EXTERN_C

#endif /* tmreader_h___ */
