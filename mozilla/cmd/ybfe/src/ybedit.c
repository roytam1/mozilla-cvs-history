/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/* 
   ybedit.c --- yb functions for fe
                  specific editor stuff.
*/

#include "xp_core.h"
#include "structs.h"
#include "ntypes.h"
#include "edttypes.h"
#include "edt.h"

void
FE_DisplayTextCaret(MWContext* context,
		    int loc,
		    LO_TextStruct* text_data,
		    int char_offset)
{
}

void
FE_DisplayImageCaret(MWContext* context,
		     LO_ImageStruct* pImageData,
		     ED_CaretObjectPosition caretPos)
{
}

void
FE_DisplayGenericCaret(MWContext* context,
		       LO_Any* pLoAny,
		       ED_CaretObjectPosition caretPos)
{
}

Bool
FE_GetCaretPosition(MWContext* context,
		    LO_Position* where,
		    int32* caretX,
		    int32* caretYLow,
		    int32* caretYHigh)
{
}

void
FE_DestroyCaret(MWContext* pContext)
{
}

void
FE_ShowCaret(MWContext* pContext)
{
}

void
FE_DocumentChanged(MWContext* context,
		   int32 iStartY,
		   int32 iHeight)
{
}

MWContext*
FE_CreateNewEditWindow(MWContext* pContext,
		       URL_Struct* pURL)
{
}

char*
FE_URLToLocalName(char* url)
{
}

void
FE_EditorDocumentLoaded(MWContext* context)
{
}

void
FE_GetDocAndWindowPosition(MWContext * context,
			   int32 *pX,
			   int32 *pY,
			   int32 *pWidth,
			   int32 *pHeight)
{
}

void
FE_SetNewDocumentProperties(MWContext* context)
{
}

Bool
FE_CheckAndSaveDocument(MWContext* context)
{
}

Bool
FE_CheckAndAutoSaveDocument(MWContext *context)
{
}

void 
FE_FinishedSave(MWContext* context,
		int status,
		char *pDestURL,
		int iFileNumber)
{
}

char *
XP_BackupFileName (const char *url)
{
}

Bool
XP_ConvertUrlToLocalFile (const char *url,
			  char **localName)
{
}

void
FE_ImageLoadDialog(MWContext* context)
{
}

void
FE_ImageLoadDialogDestroy(MWContext* context)
{
}

void
FE_DisplayAddRowOrColBorder(MWContext * pMWContext,
			    XP_Rect *pRect,
			    XP_Bool bErase)
{
}

void
FE_DisplayEntireTableOrCell(MWContext * pMWContext,
			    LO_Element * pLoElement)
{
}
