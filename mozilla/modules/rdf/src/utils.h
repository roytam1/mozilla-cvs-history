/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef	_RDF_UTILS_H_
#define	_RDF_UTILS_H_

#include "rdf.h"
#include "rdf-int.h"
#include "htrdf.h"
#include "prprf.h"
#include "xp.h"
#include "glue.h"
#include <stdarg.h>
#include <ctype.h>




/* utils.c data structures and defines */

#define CONTAINER_FLAG 0x01
#define LOCKED_FLAG    0x02



/* utils.c function prototypes */

XP_BEGIN_PROTOS

RDF_Resource	getMCFFrtop (char* furl);
void		addToResourceList (RDFFile f, RDF_Resource u);
void		addToAssertionList (RDFFile f, Assertion as);
void		removeFromAssertionList(RDFFile f, Assertion as);
void		ht_fprintf(PRFileDesc *file, const char *fmt, ...);
void		ht_rjcprintf(PRFileDesc *file, const char *fmt, const char *data);
char *		makeDBURL(char* name);
PLHashNumber	idenHash (const void *key);
int		idenEqual (const void *v1, const void *v2);
PRBool		inverseTV (PRBool tv);
char *		append2Strings (const char* str1, const char* str2);
void		stringAppendBase (char* dest, const char* addition);
void		stringAppend (char* dest, const char* addition);
int16		charSearch (const char c, const char* data);
PRBool		endsWith (const char* pattern, const char* uuid);

PRBool		substring (const char* pattern, const char* data);
int			compareStrings(char *s1, char *s2);
int16		revCharSearch (const char c, const char* data);
PRBool		urlEquals (const char* url1, const char* url2);
PRBool		isSeparator (RDF_Resource r);
void		setContainerp (RDF_Resource r, PRBool val);
PRBool		containerp (RDF_Resource r);
uint8		resourceType (RDF_Resource r);
void		setResourceType (RDF_Resource r, uint8 val);
char *		resourceID(RDF_Resource r);
char *		makeResourceName (RDF_Resource node);
char* opTypeToString (RDF_EventType opType) ;
void traceNotify (char* event, RDF_Resource u, RDF_Resource s, void* v, RDF_ValueType type) ;

PRBool		IsUTF8Text(const char* utf8, int32 len);
PRBool		IsUTF8String(const char* utf8);

void		AddCookieResource(char* name, char* path, char* host, char* expires);
void		RDF_ReadCookies(char * filename);
PRBool		CookieUnassert (RDFT r, RDF_Resource u, RDF_Resource s, void* v, RDF_ValueType type);

RDF_Cursor	CookieGetSlotValues(RDFT rdf, RDF_Resource u, RDF_Resource s, RDF_ValueType type, PRBool inversep, PRBool tv);
void *		CookieGetNextValue(RDFT rdf, RDF_Cursor c);
RDF_Error	CookieDisposeCursor(RDFT rdf, RDF_Cursor c);
PRBool		CookieAssert(RDFT rdf, RDF_Resource u, RDF_Resource s, void *v, RDF_ValueType type, PRBool tv);
void *		CookieGetSlotValue(RDFT rdf, RDF_Resource u, RDF_Resource s, RDF_ValueType type, PRBool inversep, PRBool tv);

XP_END_PROTOS

#endif
