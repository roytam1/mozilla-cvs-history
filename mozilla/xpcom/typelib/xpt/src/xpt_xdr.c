/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

/* Implementation of XDR primitives. */

#include "xpt_xdr.h"
#include <nspr.h>
#include <string.h>             /* strchr */

#define ENCODING(cursor)                                                      \
  ((cursor)->state->mode == XPT_ENCODE)

#define CURS_POOL_OFFSET(cursor)                                              \
  ((cursor)->pool == XPT_HEADER                                               \
   ? (cursor)->offset                                                         \
   : (PR_ASSERT((cursor)->state->data_offset),                                \
      (cursor)->offset + (cursor)->state->data_offset))

/* can be used as lvalue */
#define CURS_POINT(cursor)                                                    \
  ((cursor)->state->pool->data[CURS_POOL_OFFSET(cursor)])

#ifdef DEBUG_shaver
#define DBG(x) printf##x
#else
#define DBG(x) (0)
#endif

/* XXX fail if XPT_DATA and !state->data_offset */
#define CHECK_COUNT_(cursor, space)                                           \
 /* if we're in the header, then exceeding the data_offset is illegal */      \
((cursor)->pool == XPT_HEADER ?                                               \
 ((cursor)->offset + (space) > (cursor)->state->data_offset                   \
  ? (DBG(("no space left in HEADER %d + %d > %d\n", (cursor)->offset,         \
          (space), (cursor)->state->data_offset)), PR_FALSE)                  \
  : PR_TRUE) :                                                                \
 /* if we're in the data area and we're about to exceed the allocation */     \
 (CURS_POOL_OFFSET(cursor) + (space) > (cursor)->state->pool->allocated ?     \
  /* then grow if we're in ENCODE mode */                                     \
  (ENCODING(cursor) ? XPT_GrowPool((cursor)->state->pool)                     \
   /* and fail if we're in DECODE mode */                                     \
   : (DBG(("can't extend in DECODE")), PR_FALSE))                             \
  /* otherwise we're OK */                                                    \
  : PR_TRUE))

#define CHECK_COUNT(cursor, space)                                            \
  (CHECK_COUNT_(cursor, space)                                                \
   ? PR_TRUE                                                                  \
   : (fprintf(stderr, "FATAL: can't no room for %d in cursor\n", space),      \
      PR_FALSE))

/* increase the data allocation for the pool by XPT_GROW_CHUNK */
#define XPT_GROW_CHUNK 8192

static PLHashNumber
null_hash(const void *key)
{
    return (PLHashNumber)key;
}
     
XPTState *
XPT_NewXDRState(XPTMode mode, char *data, uint32 len)
{
    XPTState *state;

    state = PR_NEW(XPTState);

    if (!state)
        return NULL;

    state->mode = mode;
    state->pool = PR_NEW(XPTDatapool);
    state->next_cursor[0] = state->next_cursor[1] = 0;
    if (!state->pool)
        goto err_free_state;

    state->pool->count = 0;
    state->pool->offset_map = PL_NewHashTable(32, null_hash, PL_CompareValues,
                                              PL_CompareValues, NULL, NULL);

    if (!state->pool->offset_map)
        goto err_free_pool;
    if (mode == XPT_DECODE) {
        state->pool->data = data;
        state->pool->allocated = len;
    } else {
        state->pool->data = PR_MALLOC(XPT_GROW_CHUNK);
        if (!state->pool->data)
            goto err_free_hash;
        state->pool->allocated = XPT_GROW_CHUNK;
    }

    return state;

 err_free_hash:
    PL_HashTableDestroy(state->pool->offset_map);
 err_free_pool:
    PR_DELETE(state->pool);
 err_free_state:
    PR_DELETE(state);
    return NULL;
}

void
XPT_DestroyXDRState(XPTState *state)
{
    if (state->mode == XPT_ENCODE)
        PR_DELETE(state->pool->data);
    PR_DELETE(state->pool);
    PR_DELETE(state);
}

void
XPT_GetXDRData(XPTState *state, XPTPool pool, char **data, uint32 *len)
{
    if (pool == XPT_HEADER) {
        *data = state->pool->data;
    } else {
        *data = state->pool->data + state->data_offset;
    }
    *len = state->next_cursor[pool];
}

void
XPT_DataOffset(XPTState *state, uint32 *data_offsetp)
{
    if (state->mode == XPT_DECODE)
        XPT_SetDataOffset(state, *data_offsetp);
    else
        *data_offsetp = state->data_offset;
}

void
XPT_SetDataOffset(XPTState *state, uint32 data_offset)
{
   state->data_offset = data_offset;
}

static PRBool
XPT_GrowPool(XPTDatapool *pool)
{
    char *newdata = realloc(pool->data, pool->allocated + XPT_GROW_CHUNK);
    if (!newdata)
        return PR_FALSE;
    pool->data = newdata;
    pool->allocated += XPT_GROW_CHUNK;
    return PR_TRUE;
}

PRBool
XPT_MakeCursor(XPTState *state, XPTPool pool, uint32 len, XPTCursor *cursor)
{
    cursor->state = state;
    cursor->pool = pool;
    cursor->bits = 0;
    cursor->offset = state->next_cursor[pool];

    if (!CHECK_COUNT(cursor, len))        
        return PR_FALSE;

    /* this check should be in CHECK_CURSOR */
    if (pool == XPT_DATA && !state->data_offset) {
        fprintf(stderr, "no data offset for XPT_DATA cursor!\n");
        return PR_FALSE;
    }

    state->next_cursor[pool] += len;

    return PR_TRUE;
}

PRBool
XPT_SeekTo(XPTCursor *cursor, uint32 offset)
{
    /* XXX do some real checking and update len and stuff */
    cursor->offset = offset;
    return PR_TRUE;
}

PRBool
XPT_DoString(XPTCursor *cursor, XPTString **strp)
{
    XPTCursor my_cursor;
    XPTString *str = *strp;
    PRBool already;
    int i;

    XPT_PREAMBLE(cursor, strp, XPT_DATA, str->length + 2, my_cursor,
                 already, XPTString, str);
    
    if (!XPT_Do16(&my_cursor, &str->length))
        goto error;

    if (cursor->state->mode == XPT_DECODE)
        if (!(str->bytes = malloc(str->length)))
            goto error;
    for (i = 0; i < str->length; i++)
        if (!XPT_Do8(&my_cursor, &str->bytes[i]))
            goto error_2;

    return PR_TRUE;

 error_2:
    free(str->bytes);

    XPT_ERROR_HANDLE(str);
}

PRBool
XPT_DoCString(XPTCursor *cursor, char **identp)
{
    XPTCursor my_cursor;
    char *ident = *identp;
    PRBool already;
    XPTMode mode = cursor->state->mode;
    
    XPT_PREAMBLE_NO_ALLOC(cursor, identp, XPT_DATA, strlen(ident) + 1,
                          my_cursor, already);

    if (mode == XPT_DECODE) {
        char *start = &CURS_POINT(&my_cursor), *end;
        int len;

        end = strchr(start, 0); /* find the end of the string */
        if (!end) {
            fprintf(stderr, "didn't find end of string on decode!\n");
            return PR_FALSE;
        }
        len = end - start;

        ident = PR_MALLOC(len + 1);
        if (!ident)
            return PR_FALSE;

        memcpy(ident, start, len);
        ident[len] = 0;
        *identp = ident;

        if (!XPT_SetAddrForOffset(&my_cursor, my_cursor.offset, ident)) {
            PR_DELETE(ident);
            return PR_FALSE;
        }

    } else {
        while(*ident)
            if (!XPT_Do8(&my_cursor, ident++))
                return PR_FALSE;
        if (!XPT_Do8(&my_cursor, ident)) /* write trailing zero */
            return PR_FALSE;
    }
    
    return PR_TRUE;
}

uint32
XPT_GetOffsetForAddr(XPTCursor *cursor, void *addr)
{
    return (uint32)PL_HashTableLookup(cursor->state->pool->offset_map, addr);
}

PRBool
XPT_SetOffsetForAddr(XPTCursor *cursor, void *addr, uint32 offset)
{
    return PL_HashTableAdd(cursor->state->pool->offset_map,
                           addr, (void *)offset) != NULL;
}

PRBool
XPT_SetAddrForOffset(XPTCursor *cursor, void *addr)
{
    return PL_HashTableAdd(cursor->state->pool->offset_map,
                           (void *)cursor->offset, addr) != NULL;
}

void *
XPT_GetAddrForOffset(XPTCursor *cursor)
{
    return PL_HashTableLookup(cursor->state->pool->offset_map,
                              (void *)cursor->offset);
}

PRBool
XPT_CheckForRepeat(XPTCursor *cursor, void **addrp, XPTPool pool, int len,
                   XPTCursor *new_cursor, PRBool *already)
{
    void *last = *addrp;

    *already = PR_FALSE;
    new_cursor->state = cursor->state;
    new_cursor->pool = pool;
    new_cursor->bits = 0;

    if (cursor->state->mode == XPT_DECODE) {

        last = XPT_GetAddrForOffset(new_cursor);

        if (last) {
            *already = PR_TRUE;
            *addrp = last;
        }

    } else {

        new_cursor->offset = XPT_GetOffsetForAddr(new_cursor, last);
        if (new_cursor->offset) {
            *already = PR_TRUE;
            return PR_TRUE;
        }

        /* haven't already found it, so allocate room for it. */
        if (!XPT_MakeCursor(cursor->state, pool, len, new_cursor) ||
            !XPT_SetOffsetForAddr(new_cursor, *addrp, new_cursor->offset))
            return PR_FALSE;
    }
    return PR_TRUE;
}


/*
 * When we're writing an IID, we have to do it in a magic order.  From the
 * typelib file spec:
 *
 *   "For example, this IID:
 *     {00112233-4455-6677-8899-aabbccddeeff}
 *   is converted to the 128-bit value
 *     0x00112233445566778899aabbccddeeff
 *   Note that the byte storage order corresponds to the layout of the nsIID
 *   C-struct on a big-endian architecture."
 *
 * (http://www.mozilla.org/scriptable/typelib_file.html#iid)
 */
PRBool
XPT_DoIID(XPTCursor *cursor, nsID *iidp)
{
    int i;

    if (!XPT_Do32(cursor, &iidp->m0) ||
        !XPT_Do16(cursor, &iidp->m1) ||
        !XPT_Do16(cursor, &iidp->m2))
        return PR_FALSE;

    for (i = 0; i < 8; i++)
        if (!XPT_Do8(cursor, &iidp->m3[i]))
            return PR_FALSE;

    return PR_TRUE;
}

PRBool
XPT_Do64(XPTCursor *cursor, PRInt64 *u64p)
{
    return XPT_Do32(cursor, (uint32 *)u64p) &&
        XPT_Do32(cursor, ((uint32 *)u64p) + 1);
}

/*
 * When we're writing 32- or 16-bit quantities, we write a byte at a time to
 * avoid alignment issues.  Someone could come and optimize this to detect
 * well-aligned cases and do a single store, if they cared.  I might care
 * later.
 */
PRBool
XPT_Do32(XPTCursor *cursor, uint32 *u32p)
{
    union {
        uint8 b8[4];
        uint32 b32;
    } u;

    if (!CHECK_COUNT(cursor, 4))
        return PR_FALSE;

    if (ENCODING(cursor)) {
        u.b32 = XPT_SWAB32(*u32p);
        CURS_POINT(cursor) = u.b8[0];
        cursor->offset++;
        CURS_POINT(cursor) = u.b8[1];
        cursor->offset++;
        CURS_POINT(cursor) = u.b8[2];
        cursor->offset++;
        CURS_POINT(cursor) = u.b8[3];
    } else {
        u.b8[0] = CURS_POINT(cursor);
        cursor->offset++;
        u.b8[1] = CURS_POINT(cursor);
        cursor->offset++;
        u.b8[2] = CURS_POINT(cursor);
        cursor->offset++;
        u.b8[3] = CURS_POINT(cursor);
        *u32p = XPT_SWAB32(u.b32);
    }        
    cursor->offset++;
    return PR_TRUE;
}

PRBool
XPT_Do16(XPTCursor *cursor, uint16 *u16p)
{
    union {
        uint8 b8[2];
        uint16 b16;
    } u;

    if (!CHECK_COUNT(cursor, 2))
        return PR_FALSE;

    if (ENCODING(cursor)) {
        u.b16 = XPT_SWAB16(*u16p);
        CURS_POINT(cursor) = u.b8[0];
        cursor->offset++;
        CURS_POINT(cursor) = u.b8[1];
    } else {
        u.b8[0] = CURS_POINT(cursor);
        cursor->offset++;
        u.b8[1] = CURS_POINT(cursor);
        *u16p = XPT_SWAB16(u.b16);
    }
    cursor->offset++;

    return PR_TRUE;
}

PRBool
XPT_Do8(XPTCursor *cursor, uint8 *u8p)
{
    if (!CHECK_COUNT(cursor, 1))
        return PR_FALSE;
    if (cursor->state->mode == XPT_ENCODE)
        CURS_POINT(cursor) = *u8p;
    else
        *u8p = CURS_POINT(cursor);

    cursor->offset++;

    return PR_TRUE;
}

static PRBool
do_bit(XPTCursor *cursor, uint8 *u8p, int bitno)
{
    return PR_FALSE;
#if 0
    int bit_value, delta, new_value;
    XPTDatapool *pool = cursor->pool;

    if (cursor->state->mode == XPT_ENCODE) {
        bit_value = (*u8p & 1) << (bitno);   /* 7 = 0100 0000, 6 = 0010 0000 */
        if (bit_value) {
            delta = pool->bit + (bitno) - 7;
            new_value = delta >= 0 ? bit_value >> delta : bit_value << -delta;
            pool->data[pool->count] |= new_value;
        }
    } else {
        bit_value = pool->data[pool->count] & (1 << (7 - pool->bit));
        *u8p = bit_value >> (7 - pool->bit);
    }
    if (++pool->bit == 8) {
        pool->count++;
        pool->bit = 0;
    }

    return CHECK_COUNT(cursor);
#endif
}

PRBool
XPT_DoBits(XPTCursor *cursor, uint8 *u8p, int nbits)
{

#define DO_BIT(cursor, u8p, nbits)                                            \
    if (!do_bit(cursor, u8p, nbits))                                          \
       return PR_FALSE;

    switch(nbits) {
      case 7:
        DO_BIT(cursor, u8p, 7);
      case 6:
        DO_BIT(cursor, u8p, 6);
      case 5:
        DO_BIT(cursor, u8p, 5);
      case 4:
        DO_BIT(cursor, u8p, 4);
      case 3:
        DO_BIT(cursor, u8p, 3);
      case 2:
        DO_BIT(cursor, u8p, 2);
      case 1:
        DO_BIT(cursor, u8p, 1);
      default:;
    };

#undef DO_BIT

    return PR_TRUE;
}

int
XPT_FlushBits(XPTCursor *cursor)
{
    int skipped = 8 - cursor->bits;

    return 0;
#if 0
    cursor->bits = 0;
    cursor->offset++;

    if (!CHECK_COUNT(cursor))
        return -1;

    return skipped == 8 ? 0 : skipped;
#endif
}
