/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

// ���������������������������������������������������������������������������
//	TBD:
//		-	If this caption is going to resize, we need code to recalc the
//			offscreen world accordingly.
//
// ���������������������������������������������������������������������������

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <PP_Messages.h>
#include <UDrawingState.h>
#include <UDrawingUtils.h>
#include <UGWorld.h>
#include <UMemoryMgr.h>
#include <UTextTraits.h>

#include "COffscreenCaption.h"

// ���������������������������������������������������������������������������
//	�	
// ���������������������������������������������������������������������������

COffscreenCaption::COffscreenCaption(LStream* inStream)
	:	LCaption(inStream)
{
	// The erase color is initialized to -1 because it is an invalid palette
	// index.
	mEraseColor = -1;
}

// ���������������������������������������������������������������������������
//	�	
// ���������������������������������������������������������������������������

COffscreenCaption::COffscreenCaption(
	const SPaneInfo&	inPaneInfo,
	ConstStringPtr		inString,
	ResIDT				inTextTraitsID)
		:	LCaption(inPaneInfo, inString, inTextTraitsID)
{
	// The erase color is initialized to -1 because it is an invalid palette
	// index.
	mEraseColor = -1;
}

// ���������������������������������������������������������������������������
//	�	
// ���������������������������������������������������������������������������

void COffscreenCaption::SetDescriptor(ConstStringPtr inDescriptor)
{
	if (inDescriptor == NULL)
		mText = "\p";
	else
		mText = inDescriptor;
		
	Draw(NULL);
}

void COffscreenCaption::SetDescriptor(const char* inCDescriptor)
{
	if (inCDescriptor == NULL)
		mText = "\p";
	else
		mText = inCDescriptor;
		
	Draw(NULL);
}

// ���������������������������������������������������������������������������
//	�	
// ���������������������������������������������������������������������������

void COffscreenCaption::Draw(RgnHandle inSuperDrawRgnH)
{
	Rect theFrame;
	if ((mVisible == triState_On) && CalcPortFrameRect(theFrame) &&
			((inSuperDrawRgnH == nil) || RectInRgn(&theFrame, inSuperDrawRgnH)) && FocusDraw())
		{
		PortToLocalPoint(topLeft(theFrame));	// Get Frame in Local coords
		PortToLocalPoint(botRight(theFrame));

		if (ExecuteAttachments(msg_DrawOrPrint, &theFrame))
			{
			Boolean bDidDraw = false;

			StColorPenState thePenSaver;
			StColorPenState::Normalize();
			
			// Fail safe offscreen drawing
			StValueChanger<EDebugAction> okayToFail(UDebugging::gDebugThrow, debugAction_Nothing);
			try
				{			
				LGWorld theOffWorld(theFrame, 0, useTempMem);

				if (!theOffWorld.BeginDrawing())
					throw memFullErr;
					
				DrawSelf();
					
				theOffWorld.EndDrawing();
				theOffWorld.CopyImage(GetMacPort(), theFrame, srcCopy);
				bDidDraw = true;
				}
			catch (...)
				{
				// 	& draw onscreen
				}
				
			if (!bDidDraw)
				DrawSelf();
			}
		}
}

// ���������������������������������������������������������������������������
//	�	DrawSelf
//
//	If the erase color has been set, we'll erase the background in that
//	color.  Otherwise we should use the window's colors.	
// ���������������������������������������������������������������������������

void COffscreenCaption::DrawSelf(void)
{
	Rect	theFrame;
	CalcLocalFrameRect(theFrame);
	
	Int16	just = UTextTraits::SetPortTextTraits(mTxtrID);
	
	RGBColor theTextColor;
	::GetForeColor(&theTextColor);
	
	if (mEraseColor != -1)
		::PmBackColor(mEraseColor);
	else
		ApplyForeAndBackColors();

	::EraseRect(&theFrame);
	::RGBForeColor(&theTextColor);
	UTextDrawing::DrawWithJustification((Ptr)&mText[1], mText[0], theFrame, just);
}	
