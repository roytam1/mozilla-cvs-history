/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*******************************************************************************
 * Source date: 9 Apr 1997 21:45:12 GMT
 * netscape/fonts/cdlm module C header file
 * Generated by jmc version 1.8 -- DO NOT EDIT
 ******************************************************************************/

#ifndef _Mcdlm_H_
#define _Mcdlm_H_

#include "jmc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * cdlm
 ******************************************************************************/

/* The type of the cdlm interface. */
struct cdlmInterface;

/* The public type of a cdlm instance. */
typedef struct cdlm {
	const struct cdlmInterface*	vtable;
} cdlm;

/* The inteface ID of the cdlm interface. */
#ifndef JMC_INIT_cdlm_ID
extern EXTERN_C_WITHOUT_EXTERN const JMCInterfaceID cdlm_ID;
#else
EXTERN_C const JMCInterfaceID cdlm_ID = { 0x3c05681c, 0x011f0f65, 0x7b254505, 0x5573601b };
#endif /* JMC_INIT_cdlm_ID */
/*******************************************************************************
 * cdlm Operations
 ******************************************************************************/

#define cdlm_getInterface(self, a, exception)	\
	(((self)->vtable->getInterface)(self, cdlm_getInterface_op, a, exception))

#define cdlm_addRef(self, exception)	\
	(((self)->vtable->addRef)(self, cdlm_addRef_op, exception))

#define cdlm_release(self, exception)	\
	(((self)->vtable->release)(self, cdlm_release_op, exception))

#define cdlm_hashCode(self, exception)	\
	(((self)->vtable->hashCode)(self, cdlm_hashCode_op, exception))

#define cdlm_equals(self, a, exception)	\
	(((self)->vtable->equals)(self, cdlm_equals_op, a, exception))

#define cdlm_clone(self, exception)	\
	(((self)->vtable->clone)(self, cdlm_clone_op, exception))

#define cdlm_toString(self, exception)	\
	(((self)->vtable->toString)(self, cdlm_toString_op, exception))

#define cdlm_finalize(self, exception)	\
	(((self)->vtable->finalize)(self, cdlm_finalize_op, exception))

#define cdlm_SupportsInterface(self, a, exception)	\
	(((self)->vtable->SupportsInterface)(self, cdlm_SupportsInterface_op, a, exception))

#define cdlm_CreateObject(self, a, b, b_length, exception)	\
	(((self)->vtable->CreateObject)(self, cdlm_CreateObject_op, a, b, b_length, exception))

#define cdlm_OnUnload(self, exception)	\
	(((self)->vtable->OnUnload)(self, cdlm_OnUnload_op, exception))

/*******************************************************************************
 * cdlm Interface
 ******************************************************************************/

struct netscape_jmc_JMCInterfaceID;
struct java_lang_Object;
struct java_lang_String;

struct cdlmInterface {
	void*	(*getInterface)(struct cdlm* self, jint op, const JMCInterfaceID* a, JMCException* *exception);
	void	(*addRef)(struct cdlm* self, jint op, JMCException* *exception);
	void	(*release)(struct cdlm* self, jint op, JMCException* *exception);
	jint	(*hashCode)(struct cdlm* self, jint op, JMCException* *exception);
	jbool	(*equals)(struct cdlm* self, jint op, void* a, JMCException* *exception);
	void*	(*clone)(struct cdlm* self, jint op, JMCException* *exception);
	const char*	(*toString)(struct cdlm* self, jint op, JMCException* *exception);
	void	(*finalize)(struct cdlm* self, jint op, JMCException* *exception);
	jint	(*SupportsInterface)(struct cdlm* self, jint op, const JMCInterfaceID* a, JMCException* *exception);
	void*	(*CreateObject)(struct cdlm* self, jint op, const JMCInterfaceID* a, void** b, jsize b_length, JMCException* *exception);
	jint	(*OnUnload)(struct cdlm* self, jint op, JMCException* *exception);
};

/*******************************************************************************
 * cdlm Operation IDs
 ******************************************************************************/

typedef enum cdlmOperations {
	cdlm_getInterface_op,
	cdlm_addRef_op,
	cdlm_release_op,
	cdlm_hashCode_op,
	cdlm_equals_op,
	cdlm_clone_op,
	cdlm_toString_op,
	cdlm_finalize_op,
	cdlm_SupportsInterface_op,
	cdlm_CreateObject_op,
	cdlm_OnUnload_op
} cdlmOperations;

/*******************************************************************************
 * Writing your C implementation: "cdlm.h"
 * *****************************************************************************
 * You must create a header file named "cdlm.h" that implements
 * the struct cdlmImpl, including the struct cdlmImplHeader
 * as it's first field:
 * 
 * 		#include "Mcdlm.h" // generated header
 * 
 * 		struct cdlmImpl {
 * 			cdlmImplHeader	header;
 * 			<your instance data>
 * 		};
 * 
 * This header file will get included by the generated module implementation.
 ******************************************************************************/

/* Forward reference to the user-defined instance struct: */
typedef struct cdlmImpl	cdlmImpl;


/* This struct must be included as the first field of your instance struct: */
typedef struct cdlmImplHeader {
	const struct cdlmInterface*	vtablecdlm;
	jint		refcount;
} cdlmImplHeader;

/*******************************************************************************
 * Instance Casting Macros
 * These macros get your back to the top of your instance, cdlm,
 * given a pointer to one of its interfaces.
 ******************************************************************************/

#undef  cdlmImpl2nfdlm
#define cdlmImpl2nfdlm(cdlmImplPtr) \
	((nfdlm*)((char*)(cdlmImplPtr) + offsetof(cdlmImplHeader, vtablecdlm)))

#undef  nfdlm2cdlmImpl
#define nfdlm2cdlmImpl(nfdlmPtr) \
	((cdlmImpl*)((char*)(nfdlmPtr) - offsetof(cdlmImplHeader, vtablecdlm)))

#undef  cdlmImpl2cdlm
#define cdlmImpl2cdlm(cdlmImplPtr) \
	((cdlm*)((char*)(cdlmImplPtr) + offsetof(cdlmImplHeader, vtablecdlm)))

#undef  cdlm2cdlmImpl
#define cdlm2cdlmImpl(cdlmPtr) \
	((cdlmImpl*)((char*)(cdlmPtr) - offsetof(cdlmImplHeader, vtablecdlm)))

/*******************************************************************************
 * Operations you must implement
 ******************************************************************************/


extern JMC_PUBLIC_API(void*)
_cdlm_getBackwardCompatibleInterface(struct cdlm* self, const JMCInterfaceID* iid,
	JMCException* *exception);

extern JMC_PUBLIC_API(void)
_cdlm_init(struct cdlm* self, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_cdlm_getInterface(struct cdlm* self, jint op, const JMCInterfaceID* a, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_cdlm_addRef(struct cdlm* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_cdlm_release(struct cdlm* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_cdlm_hashCode(struct cdlm* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jbool)
_cdlm_equals(struct cdlm* self, jint op, void* a, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_cdlm_clone(struct cdlm* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(const char*)
_cdlm_toString(struct cdlm* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_cdlm_finalize(struct cdlm* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_cdlm_SupportsInterface(struct cdlm* self, jint op, const JMCInterfaceID* a, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_cdlm_CreateObject(struct cdlm* self, jint op, const JMCInterfaceID* a, void* * b, jsize b_length, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_cdlm_OnUnload(struct cdlm* self, jint op, JMCException* *exception);

/*******************************************************************************
 * Factory Operations
 ******************************************************************************/

JMC_PUBLIC_API(cdlm*)
cdlmFactory_Create(JMCException* *exception);

/******************************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* _Mcdlm_H_ */