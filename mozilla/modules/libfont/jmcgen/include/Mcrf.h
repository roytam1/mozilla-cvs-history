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
 * Source date: 9 Apr 1997 21:45:13 GMT
 * netscape/fonts/crf module C header file
 * Generated by jmc version 1.8 -- DO NOT EDIT
 ******************************************************************************/

#ifndef _Mcrf_H_
#define _Mcrf_H_

#include "jmc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * crf
 ******************************************************************************/

/* The type of the crf interface. */
struct crfInterface;

/* The public type of a crf instance. */
typedef struct crf {
	const struct crfInterface*	vtable;
} crf;

/* The inteface ID of the crf interface. */
#ifndef JMC_INIT_crf_ID
extern EXTERN_C_WITHOUT_EXTERN const JMCInterfaceID crf_ID;
#else
EXTERN_C const JMCInterfaceID crf_ID = { 0x472d303c, 0x3c0b096b, 0x7609161a, 0x402b5c49 };
#endif /* JMC_INIT_crf_ID */
/*******************************************************************************
 * crf Operations
 ******************************************************************************/

#define crf_getInterface(self, a, exception)	\
	(((self)->vtable->getInterface)(self, crf_getInterface_op, a, exception))

#define crf_addRef(self, exception)	\
	(((self)->vtable->addRef)(self, crf_addRef_op, exception))

#define crf_release(self, exception)	\
	(((self)->vtable->release)(self, crf_release_op, exception))

#define crf_hashCode(self, exception)	\
	(((self)->vtable->hashCode)(self, crf_hashCode_op, exception))

#define crf_equals(self, a, exception)	\
	(((self)->vtable->equals)(self, crf_equals_op, a, exception))

#define crf_clone(self, exception)	\
	(((self)->vtable->clone)(self, crf_clone_op, exception))

#define crf_toString(self, exception)	\
	(((self)->vtable->toString)(self, crf_toString_op, exception))

#define crf_finalize(self, exception)	\
	(((self)->vtable->finalize)(self, crf_finalize_op, exception))

#define crf_GetMatchInfo(self, exception)	\
	(((self)->vtable->GetMatchInfo)(self, crf_GetMatchInfo_op, exception))

#define crf_GetPointSize(self, exception)	\
	(((self)->vtable->GetPointSize)(self, crf_GetPointSize_op, exception))

#define crf_GetMaxWidth(self, exception)	\
	(((self)->vtable->GetMaxWidth)(self, crf_GetMaxWidth_op, exception))

#define crf_GetFontAscent(self, exception)	\
	(((self)->vtable->GetFontAscent)(self, crf_GetFontAscent_op, exception))

#define crf_GetFontDescent(self, exception)	\
	(((self)->vtable->GetFontDescent)(self, crf_GetFontDescent_op, exception))

#define crf_GetMaxLeftBearing(self, exception)	\
	(((self)->vtable->GetMaxLeftBearing)(self, crf_GetMaxLeftBearing_op, exception))

#define crf_GetMaxRightBearing(self, exception)	\
	(((self)->vtable->GetMaxRightBearing)(self, crf_GetMaxRightBearing_op, exception))

#define crf_SetTransformMatrix(self, a, a_length, exception)	\
	(((self)->vtable->SetTransformMatrix)(self, crf_SetTransformMatrix_op, a, a_length, exception))

#define crf_GetTransformMatrix(self, exception)	\
	(((self)->vtable->GetTransformMatrix)(self, crf_GetTransformMatrix_op, exception))

#define crf_MeasureText(self, a, b, c, c_length, d, d_length, exception)	\
	(((self)->vtable->MeasureText)(self, crf_MeasureText_op, a, b, c, c_length, d, d_length, exception))

#define crf_MeasureBoundingBox(self, a, b, c, c_length, d, exception)	\
	(((self)->vtable->MeasureBoundingBox)(self, crf_MeasureBoundingBox_op, a, b, c, c_length, d, exception))

#define crf_DrawText(self, a, b, c, d, e, e_length, exception)	\
	(((self)->vtable->DrawText)(self, crf_DrawText_op, a, b, c, d, e, e_length, exception))

#define crf_PrepareDraw(self, a, exception)	\
	(((self)->vtable->PrepareDraw)(self, crf_PrepareDraw_op, a, exception))

#define crf_EndDraw(self, a, exception)	\
	(((self)->vtable->EndDraw)(self, crf_EndDraw_op, a, exception))

/*******************************************************************************
 * crf Interface
 ******************************************************************************/

struct netscape_jmc_JMCInterfaceID;
struct java_lang_Object;
struct java_lang_String;
struct netscape_fonts_nffmi;
struct netscape_fonts_nfrc;
struct netscape_fonts_BoundingBoxStar;

struct crfInterface {
	void*	(*getInterface)(struct crf* self, jint op, const JMCInterfaceID* a, JMCException* *exception);
	void	(*addRef)(struct crf* self, jint op, JMCException* *exception);
	void	(*release)(struct crf* self, jint op, JMCException* *exception);
	jint	(*hashCode)(struct crf* self, jint op, JMCException* *exception);
	jbool	(*equals)(struct crf* self, jint op, void* a, JMCException* *exception);
	void*	(*clone)(struct crf* self, jint op, JMCException* *exception);
	const char*	(*toString)(struct crf* self, jint op, JMCException* *exception);
	void	(*finalize)(struct crf* self, jint op, JMCException* *exception);
	struct nffmi*	(*GetMatchInfo)(struct crf* self, jint op, JMCException* *exception);
	jdouble	(*GetPointSize)(struct crf* self, jint op, JMCException* *exception);
	jint	(*GetMaxWidth)(struct crf* self, jint op, JMCException* *exception);
	jint	(*GetFontAscent)(struct crf* self, jint op, JMCException* *exception);
	jint	(*GetFontDescent)(struct crf* self, jint op, JMCException* *exception);
	jint	(*GetMaxLeftBearing)(struct crf* self, jint op, JMCException* *exception);
	jint	(*GetMaxRightBearing)(struct crf* self, jint op, JMCException* *exception);
	void	(*SetTransformMatrix)(struct crf* self, jint op, void** a, jsize a_length, JMCException* *exception);
	void*	(*GetTransformMatrix)(struct crf* self, jint op, JMCException* *exception);
	jint	(*MeasureText)(struct crf* self, jint op, struct nfrc* a, jint b, jbyte* c, jsize c_length, jint* d, jsize d_length, JMCException* *exception);
	jint	(*MeasureBoundingBox)(struct crf* self, jint op, struct nfrc* a, jint b, jbyte* c, jsize c_length, struct nf_bounding_box * d, JMCException* *exception);
	jint	(*DrawText)(struct crf* self, jint op, struct nfrc* a, jint b, jint c, jint d, jbyte* e, jsize e_length, JMCException* *exception);
	jint	(*PrepareDraw)(struct crf* self, jint op, struct nfrc* a, JMCException* *exception);
	jint	(*EndDraw)(struct crf* self, jint op, struct nfrc* a, JMCException* *exception);
};

/*******************************************************************************
 * crf Operation IDs
 ******************************************************************************/

typedef enum crfOperations {
	crf_getInterface_op,
	crf_addRef_op,
	crf_release_op,
	crf_hashCode_op,
	crf_equals_op,
	crf_clone_op,
	crf_toString_op,
	crf_finalize_op,
	crf_GetMatchInfo_op,
	crf_GetPointSize_op,
	crf_GetMaxWidth_op,
	crf_GetFontAscent_op,
	crf_GetFontDescent_op,
	crf_GetMaxLeftBearing_op,
	crf_GetMaxRightBearing_op,
	crf_SetTransformMatrix_op,
	crf_GetTransformMatrix_op,
	crf_MeasureText_op,
	crf_MeasureBoundingBox_op,
	crf_DrawText_op,
	crf_PrepareDraw_op,
	crf_EndDraw_op
} crfOperations;

/*******************************************************************************
 * Writing your C implementation: "crf.h"
 * *****************************************************************************
 * You must create a header file named "crf.h" that implements
 * the struct crfImpl, including the struct crfImplHeader
 * as it's first field:
 * 
 * 		#include "Mcrf.h" // generated header
 * 
 * 		struct crfImpl {
 * 			crfImplHeader	header;
 * 			<your instance data>
 * 		};
 * 
 * This header file will get included by the generated module implementation.
 ******************************************************************************/

/* Forward reference to the user-defined instance struct: */
typedef struct crfImpl	crfImpl;


/* This struct must be included as the first field of your instance struct: */
typedef struct crfImplHeader {
	const struct crfInterface*	vtablecrf;
	jint		refcount;
} crfImplHeader;

/*******************************************************************************
 * Instance Casting Macros
 * These macros get your back to the top of your instance, crf,
 * given a pointer to one of its interfaces.
 ******************************************************************************/

#undef  crfImpl2nfrf
#define crfImpl2nfrf(crfImplPtr) \
	((nfrf*)((char*)(crfImplPtr) + offsetof(crfImplHeader, vtablecrf)))

#undef  nfrf2crfImpl
#define nfrf2crfImpl(nfrfPtr) \
	((crfImpl*)((char*)(nfrfPtr) - offsetof(crfImplHeader, vtablecrf)))

#undef  crfImpl2crf
#define crfImpl2crf(crfImplPtr) \
	((crf*)((char*)(crfImplPtr) + offsetof(crfImplHeader, vtablecrf)))

#undef  crf2crfImpl
#define crf2crfImpl(crfPtr) \
	((crfImpl*)((char*)(crfPtr) - offsetof(crfImplHeader, vtablecrf)))

/*******************************************************************************
 * Operations you must implement
 ******************************************************************************/


extern JMC_PUBLIC_API(void*)
_crf_getBackwardCompatibleInterface(struct crf* self, const JMCInterfaceID* iid,
	JMCException* *exception);

extern JMC_PUBLIC_API(void)
_crf_init(struct crf* self, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_crf_getInterface(struct crf* self, jint op, const JMCInterfaceID* a, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_crf_addRef(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_crf_release(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_hashCode(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jbool)
_crf_equals(struct crf* self, jint op, void* a, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_crf_clone(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(const char*)
_crf_toString(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_crf_finalize(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(struct nffmi*)
_crf_GetMatchInfo(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jdouble)
_crf_GetPointSize(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_GetMaxWidth(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_GetFontAscent(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_GetFontDescent(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_GetMaxLeftBearing(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_GetMaxRightBearing(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_crf_SetTransformMatrix(struct crf* self, jint op, void* * a, jsize a_length, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_crf_GetTransformMatrix(struct crf* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_MeasureText(struct crf* self, jint op, struct nfrc* a, jint b, jbyte * c, jsize c_length, jint * d, jsize d_length, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_MeasureBoundingBox(struct crf* self, jint op, struct nfrc* a, jint b, jbyte * c, jsize c_length, struct nf_bounding_box * d, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_DrawText(struct crf* self, jint op, struct nfrc* a, jint b, jint c, jint d, jbyte * e, jsize e_length, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_PrepareDraw(struct crf* self, jint op, struct nfrc* a, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_crf_EndDraw(struct crf* self, jint op, struct nfrc* a, JMCException* *exception);

/*******************************************************************************
 * Factory Operations
 ******************************************************************************/

JMC_PUBLIC_API(crf*)
crfFactory_Create(JMCException* *exception);

/******************************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* _Mcrf_H_ */