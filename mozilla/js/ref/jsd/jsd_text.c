/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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

/*
 * JavaScript Debugger API - Source Text functions
 */

#include "jsd.h"

#ifdef DEBUG
void JSD_ASSERT_VALID_SOURCE_TEXT(JSDSourceText* jsdsrc)
{
    PR_ASSERT(jsdsrc);
    PR_ASSERT(jsdsrc->url);
}
#endif

/***************************************************************************/
/* XXX add notification */

static void
_clearText(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    if( jsdsrc->text )
        MY_XP_HUGE_FREE(jsdsrc->text);
    jsdsrc->text        = NULL;
    jsdsrc->textLength  = 0;
    jsdsrc->textSpace   = 0;
    jsdsrc->status      = JSD_SOURCE_CLEARED;
    jsdsrc->dirty       = JS_TRUE;
    jsdsrc->alterCount  = jsdc->sourceAlterCount++ ;
    jsdsrc->doingEval   = JS_FALSE;
}    

static JSBool
_appendText(JSDContext* jsdc, JSDSourceText* jsdsrc, 
            const char* text, size_t length)
{
#define MEMBUF_GROW 1000

    uintN neededSize = jsdsrc->textLength + length;

    if( neededSize > jsdsrc->textSpace )
    {
        MY_XP_HUGE_CHAR_PTR pBuf;
        uintN iNewSize;

        /* if this is the first alloc, the req might be all that's needed*/
        if( ! jsdsrc->textSpace )
             iNewSize = length;
        else
             iNewSize = (neededSize * 5 / 4) + MEMBUF_GROW;

        pBuf = (MY_XP_HUGE_CHAR_PTR) MY_XP_HUGE_ALLOC(iNewSize);
        if( pBuf )
        {
            if( jsdsrc->text )
            {
                MY_XP_HUGE_MEMCPY(pBuf, jsdsrc->text, jsdsrc->textLength);
                MY_XP_HUGE_FREE(jsdsrc->text);
            }
            jsdsrc->text = pBuf;
            jsdsrc->textSpace = iNewSize;
        }
        else 
        {
            /* LTNOTE: throw an out of memory exception */
            _clearText( jsdc, jsdsrc );
            jsdsrc->status = JSD_SOURCE_FAILED;
            return JS_FALSE;
        }
    }

    MY_XP_HUGE_MEMCPY(&jsdsrc->text[jsdsrc->textLength], text, length);
    jsdsrc->textLength += length;
    return JS_TRUE;
}

static JSDSourceText*
_newSource(JSDContext* jsdc, const char* url)
{
    JSDSourceText* jsdsrc = (JSDSourceText*)calloc(1,sizeof(JSDSourceText));
    if( ! jsdsrc )
        return NULL;
    
    jsdsrc->url        = (char*) url; /* already a copy */
    jsdsrc->status     = JSD_SOURCE_INITED;
    jsdsrc->dirty      = JS_TRUE;
    jsdsrc->alterCount = jsdc->sourceAlterCount++ ;
            
    return jsdsrc;
}

static void
_destroySource(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    PR_ASSERT(NULL == jsdsrc->text);  /* must _clearText() first */
    MY_XP_FREE(jsdsrc->url);
    MY_XP_FREE(jsdsrc);
}

static void
_removeSource(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    PR_REMOVE_LINK(&jsdsrc->links);
    _clearText(jsdc, jsdsrc);
    _destroySource(jsdc, jsdsrc);
}

static JSDSourceText*
_addSource(JSDContext* jsdc, const char* url)
{
    JSDSourceText* jsdsrc = _newSource(jsdc, url);
    if( ! jsdsrc )
        return NULL;
    PR_INSERT_LINK(&jsdsrc->links, &jsdc->sources);
    return jsdsrc;
}

static void
_moveSourceToFront(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    PR_REMOVE_LINK(&jsdsrc->links);
    PR_INSERT_LINK(&jsdsrc->links, &jsdc->sources);
}

static void
_moveSourceToRemovedList(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    _clearText(jsdc, jsdsrc);
    PR_REMOVE_LINK(&jsdsrc->links);
    PR_INSERT_LINK(&jsdsrc->links, &jsdc->removedSources);
}

static void
_removeSourceFromRemovedList( JSDContext* jsdc, JSDSourceText* jsdsrc )
{
    PR_REMOVE_LINK(&jsdsrc->links);
    _destroySource( jsdc, jsdsrc );
}

static JSBool
_isSourceInSourceList(JSDContext* jsdc, JSDSourceText* jsdsrcToFind)
{
    JSDSourceText *jsdsrc;

    for( jsdsrc = (JSDSourceText*)jsdc->sources.next;
         jsdsrc != (JSDSourceText*)&jsdc->sources;
         jsdsrc = (JSDSourceText*)jsdsrc->links.next ) 
    {
        if( jsdsrc == jsdsrcToFind )
            return JS_TRUE;
    }
    return JS_FALSE;
}

/*  compare strings in a case insensitive manner with a length limit
*/

static int 
strncasecomp (const char* one, const char * two, int n)
{
    const char *pA;
    const char *pB;
    
    for(pA=one, pB=two;; pA++, pB++) 
    {
        int tmp;
        if (pA == one+n) 
            return 0;   
        if (!(*pA && *pB)) 
            return *pA - *pB;
        tmp = MY_XP_TO_LOWER(*pA) - MY_XP_TO_LOWER(*pB);
        if (tmp) 
            return tmp;
    }
}

static char file_url_prefix[]    = "file:";
#define FILE_URL_PREFIX_LEN     (sizeof file_url_prefix - 1)

const char*
jsd_BuildNormalizedURL( const char* url_string )
{
    char *new_url_string;

    if( ! url_string )
        return NULL;

    if (!MY_XP_STRNCASECMP(url_string, file_url_prefix, FILE_URL_PREFIX_LEN) &&
        url_string[FILE_URL_PREFIX_LEN + 0] == '/' &&
        url_string[FILE_URL_PREFIX_LEN + 1] == '/') {
        new_url_string = PR_smprintf("%s%s",
                                     file_url_prefix,
                                     url_string + FILE_URL_PREFIX_LEN + 2);
    } else {
        new_url_string = MY_XP_STRDUP(url_string);
    }
    return new_url_string;
}

/***************************************************************************/

void
jsd_DestroyAllSources( JSDContext* jsdc )
{
    JSDSourceText *jsdsrc;
    JSDSourceText *next;

    for( jsdsrc = (JSDSourceText*)jsdc->sources.next;
         jsdsrc != (JSDSourceText*)&jsdc->sources;
         jsdsrc = next ) 
    {
        next = (JSDSourceText*)jsdsrc->links.next;
        _removeSource( jsdc, jsdsrc );
    }

    for( jsdsrc = (JSDSourceText*)jsdc->removedSources.next;
         jsdsrc != (JSDSourceText*)&jsdc->removedSources;
         jsdsrc = next ) 
    {
        next = (JSDSourceText*)jsdsrc->links.next;
        _removeSourceFromRemovedList( jsdc, jsdsrc );
    }

}

JSDSourceText*
jsd_IterateSources(JSDContext* jsdc, JSDSourceText **iterp)
{
    JSDSourceText *jsdsrc = *iterp;
    
    if( !jsdsrc )
        jsdsrc = (JSDSourceText *)jsdc->sources.next;
    if( jsdsrc == (JSDSourceText *)&jsdc->sources )
        return NULL;
    *iterp = (JSDSourceText *)jsdsrc->links.next;
    return jsdsrc;
}

JSDSourceText*
jsd_FindSourceForURL(JSDContext* jsdc, const char* url)
{
    JSDSourceText *jsdsrc;

    for( jsdsrc = (JSDSourceText *)jsdc->sources.next;
         jsdsrc != (JSDSourceText *)&jsdc->sources;
         jsdsrc = (JSDSourceText *)jsdsrc->links.next )
    {
        if( 0 == strcmp(jsdsrc->url, url) )
            return jsdsrc;
    }
    return NULL;
}

const char*
jsd_GetSourceURL(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    return jsdsrc->url;
}

JSBool
jsd_GetSourceText(JSDContext* jsdc, JSDSourceText* jsdsrc,
                  const char** ppBuf, int* pLen )
{
    *ppBuf = jsdsrc->text;
    *pLen  = jsdsrc->textLength;
    return JS_TRUE;
}

void
jsd_ClearSourceText(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    if( JSD_SOURCE_INITED  != jsdsrc->status &&
        JSD_SOURCE_PARTIAL != jsdsrc->status )
    {
        _clearText(jsdc, jsdsrc);
    }
}

JSDSourceStatus
jsd_GetSourceStatus(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    return jsdsrc->status;
}

JSBool
jsd_IsSourceDirty(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    return jsdsrc->dirty;
}

void
jsd_SetSourceDirty(JSDContext* jsdc, JSDSourceText* jsdsrc, JSBool dirty)
{
    jsdsrc->dirty = dirty;
}

uintN
jsd_GetSourceAlterCount(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    return jsdsrc->alterCount;
}

uintN
jsd_IncrementSourceAlterCount(JSDContext* jsdc, JSDSourceText* jsdsrc)
{
    return jsdsrc->alterCount = jsdc->sourceAlterCount++;
}

/***************************************************************************/

#if defined(DEBUG) && 0
void DEBUG_ITERATE_SOURCES( JSDContext* jsdc )
{
    JSDSourceText* iterp = NULL;
    JSDSourceText* jsdsrc = NULL;
    int dummy;
    
    while( NULL != (jsdsrc = jsd_IterateSources(jsdc, &iterp)) )
    {
        const char*     url;
        const char*     text;
        int             len;
        JSBool          dirty;
        JSDStreamStatus status;
        JSBool          gotSrc;

        url     = JSD_GetSourceURL(jsdc, jsdsrc);
        dirty   = JSD_IsSourceDirty(jsdc, jsdsrc);
        status  = JSD_GetSourceStatus(jsdc, jsdsrc);
        gotSrc  = JSD_GetSourceText(jsdc, jsdsrc, &text, &len );
        
        dummy = 0;  /* gives us a line to set breakpoint... */
    }
}
#else
#define DEBUG_ITERATE_SOURCES(x) ((void)x)
#endif

/***************************************************************************/

JSDSourceText*
jsd_NewSourceText(JSDContext* jsdc, const char* url)
{
    JSDSourceText* jsdsrc;
    const char* new_url_string;

    JSD_LOCK_SOURCE_TEXT(jsdc);

#ifdef LIVEWIRE
    new_url_string = url; /* we take ownership of alloc'd string */
#else
    new_url_string = jsd_BuildNormalizedURL(url);
#endif
    if( ! new_url_string )
        return NULL;

    jsdsrc = jsd_FindSourceForURL(jsdc, new_url_string);

    if( jsdsrc )
    {
        if( jsdsrc->doingEval )
        {
#ifdef LIVEWIRE
            free((char*)new_url_string);
#endif
            JSD_UNLOCK_SOURCE_TEXT(jsdc);
            return NULL;
        }
        else    
            _moveSourceToRemovedList(jsdc, jsdsrc);
    }

    jsdsrc = _addSource( jsdc, new_url_string );

    JSD_UNLOCK_SOURCE_TEXT(jsdc);

    return jsdsrc;
}

JSDSourceText*
jsd_AppendSourceText(JSDContext* jsdc, 
                     JSDSourceText* jsdsrc,
                     const char* text,       /* *not* zero terminated */
                     size_t length,
                     JSDSourceStatus status)
{
    JSD_LOCK_SOURCE_TEXT(jsdc);

    if( jsdsrc->doingEval )
    {
        JSD_UNLOCK_SOURCE_TEXT(jsdc);
        return NULL;
    }

    if( ! _isSourceInSourceList( jsdc, jsdsrc ) )
    {
        _removeSourceFromRemovedList( jsdc, jsdsrc );
        JSD_UNLOCK_SOURCE_TEXT(jsdc);
        return NULL;
    }

    if( text && length && ! _appendText( jsdc, jsdsrc, text, length ) )
    {
        jsdsrc->dirty  = JS_TRUE;
        jsdsrc->alterCount  = jsdc->sourceAlterCount++ ;
        jsdsrc->status = JSD_SOURCE_FAILED;
        _moveSourceToRemovedList(jsdc, jsdsrc);
        JSD_UNLOCK_SOURCE_TEXT(jsdc);
        return NULL;    
    }

    jsdsrc->dirty  = JS_TRUE;
    jsdsrc->alterCount  = jsdc->sourceAlterCount++ ;
    jsdsrc->status = status;
    DEBUG_ITERATE_SOURCES(jsdc);
    JSD_UNLOCK_SOURCE_TEXT(jsdc);
    return jsdsrc;
}

/* convienence function for adding complete source of url in one call */
JSBool
jsd_AddFullSourceText(JSDContext* jsdc, 
                      const char* text,       /* *not* zero terminated */
                      size_t      length,
                      const char* url)
{
    JSDSourceText* jsdsrc;

    JSD_LOCK_SOURCE_TEXT(jsdc);

    jsdsrc = jsd_NewSourceText(jsdc, url);
    if( jsdsrc )
        jsdsrc = jsd_AppendSourceText(jsdc, jsdsrc,
                                      text, length, JSD_SOURCE_PARTIAL );
    if( jsdsrc )
        jsdsrc = jsd_AppendSourceText(jsdc, jsdsrc,
                                      NULL, 0, JSD_SOURCE_COMPLETED );

    JSD_UNLOCK_SOURCE_TEXT(jsdc);

    return jsdsrc ? JS_TRUE : JS_FALSE;
}

/***************************************************************************/

void
jsd_StartingEvalUsingFilename(JSDContext* jsdc, const char* url)
{
    JSDSourceText* jsdsrc;

    /* NOTE: We leave it locked! */
    JSD_LOCK_SOURCE_TEXT(jsdc); 

    jsdsrc = jsd_FindSourceForURL(jsdc, url);
    if(jsdsrc)
    {
#ifndef JSD_LOWLEVEL_SOURCE
        PR_ASSERT(! jsdsrc->doingEval);
#endif
        jsdsrc->doingEval = JS_TRUE;
    }
}    

void
jsd_FinishedEvalUsingFilename(JSDContext* jsdc, const char* url)
{
    JSDSourceText* jsdsrc;

    /* NOTE: We ASSUME it is locked! */

    jsdsrc = jsd_FindSourceForURL(jsdc, url);
    if(jsdsrc)
    {
#ifndef JSD_LOWLEVEL_SOURCE
        /*
        * when using this low level source addition, this jsdsrc might 
        * not have existed before the eval, but does exist now (without
        * this flag set!)
        */
        PR_ASSERT(jsdsrc->doingEval);
#endif
        jsdsrc->doingEval = JS_FALSE;
    }

    JSD_UNLOCK_SOURCE_TEXT(jsdc);
}    
