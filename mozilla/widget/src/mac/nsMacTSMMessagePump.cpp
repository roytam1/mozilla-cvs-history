/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nscore.h"
#include "nsMacEventHandler.h"
#include "nsMacTSMMessagePump.h"
#include "nsString.h"
#include <Script.h>
#include <TextServices.h>


//-------------------------------------------------------------------------
//
// TSM AE Handler routines UPPs 
//
//-------------------------------------------------------------------------

AEEventHandlerUPP nsMacTSMMessagePump::mPos2OffsetUPP = NULL;
AEEventHandlerUPP nsMacTSMMessagePump::mOffset2PosUPP = NULL;
AEEventHandlerUPP nsMacTSMMessagePump::mUpdateUPP = NULL;

//-------------------------------------------------------------------------
//
// Constructor/Destructors
//
//-------------------------------------------------------------------------
nsMacTSMMessagePump::nsMacTSMMessagePump()
{
	OSErr	err;

	mPos2OffsetUPP = NewAEEventHandlerProc(nsMacTSMMessagePump::PositionToOffsetHandler);
	NS_ASSERTION(mPos2OffsetUPP!=NULL,"nsMacTSMMessagePump::InstallTSMAEHandlers: NewAEEventHandlerProc[Pos2Pffset] failed");

	mOffset2PosUPP = NewAEEventHandlerProc(nsMacTSMMessagePump::OffsetToPositionHandler);
	NS_ASSERTION(mPos2OffsetUPP!=NULL,"nsMacTSMMessagePump::InstallTSMAEHandlers: NewAEEventHandlerProc[Pos2Pffset] failed");

	mUpdateUPP = NewAEEventHandlerProc(nsMacTSMMessagePump::UpdateHandler);
	NS_ASSERTION(mPos2OffsetUPP!=NULL,"nsMacTSMMessagePump::InstallTSMAEHandlers: NewAEEventHandlerProc[Pos2Pffset] failed");

	err = AEInstallEventHandler(kTextServiceClass,kPos2Offset,mPos2OffsetUPP,(long)this,false);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::InstallTSMAEHandlers: AEInstallEventHandlers[Pos2Offset] failed");

	err = AEInstallEventHandler(kTextServiceClass,kOffset2Pos,mOffset2PosUPP,(long)this,false);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::InstallTSMAEHandlers: AEInstallEventHandlers[Offset2Pos] failed");

	err = AEInstallEventHandler(kTextServiceClass,kUpdateActiveInputArea,mUpdateUPP,(long)this,false);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::InstallTSMAEHandlers: AEInstallEventHandlers[Update] failed");

}

nsMacTSMMessagePump::~nsMacTSMMessagePump()
{
	OSErr	err;
	
	err = AERemoveEventHandler(kTextServiceClass,kPos2Offset,mPos2OffsetUPP,false);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::InstallTSMAEHandlers: AEInstallEventHandlers[Pos2Offset] failed");

	err = AERemoveEventHandler(kTextServiceClass,kOffset2Pos,mOffset2PosUPP,false);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::InstallTSMAEHandlers: AEInstallEventHandlers[Offset2Pos] failed");

	err = AERemoveEventHandler(kTextServiceClass,kUpdateActiveInputArea,mUpdateUPP,false);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::InstallTSMAEHandlers: AEInstallEventHandlers[Update] failed");

#if TARGET_CARBON
 	(void)DisposeAEEventHandlerUPP(mPos2OffsetUPP);
 	(void)DisposeAEEventHandlerUPP(mOffset2PosUPP);
 	(void)DisposeAEEventHandlerUPP(mUpdateUPP);
#else
	(void)DisposeRoutineDescriptor(mPos2OffsetUPP);
	(void)DisposeRoutineDescriptor(mOffset2PosUPP);
	(void)DisposeRoutineDescriptor(mUpdateUPP);
#endif

}

//-------------------------------------------------------------------------
//
// TSM AE Handler routines
//
//-------------------------------------------------------------------------

pascal OSErr nsMacTSMMessagePump::PositionToOffsetHandler(const AppleEvent *theAppleEvent, AppleEvent *reply, UInt32 handlerRefcon)
{
	OSErr 				err;
	DescType			returnedType;
	nsMacEventHandler*	eventHandler;	
	Size				actualSize;
	Point				thePoint;
	long				offset;
	short				regionClass;

	//
	// Extract the nsMacEventHandler for this TSMDocument.  It's stored as the TSMDocument's
	//	refcon
	//
	err = AEGetParamPtr(theAppleEvent,keyAETSMDocumentRefcon,typeLongInteger,&returnedType,
						&eventHandler,sizeof(eventHandler),&actualSize);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::PositionToOffsetHandler: AEGetParamPtr[TSMRefcon] failed");
	if (err!=noErr) 
		return err;
	
	//
	// Extract the Position parameter.
	//
	err = AEGetParamPtr(theAppleEvent,keyAECurrentPoint,typeQDPoint,&returnedType,
						&thePoint,sizeof(thePoint),&actualSize);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::PositionToOffsetHandler: AGGetParamPtr[Point] failed");
	if (err!=noErr) 
		return err;

	//
	// pass the request to the widget system
	//
	offset = eventHandler->HandlePositionToOffset(thePoint,&regionClass);
	
	//
	// build up the AE reply (need offset and region class)
	//
	err = AEPutParamPtr(reply,keyAEOffset,typeLongInteger,&offset,sizeof(offset));
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::PositionToOffsetHandler: AEPutParamPtr failed");
	if (err!=noErr) 
		return err;
	
	err = AEPutParamPtr(reply,keyAERegionClass,typeShortInteger,&regionClass,sizeof(regionClass));
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::PositionToOffsetHandler: AEPutParamPtr failed");
	if (err!=noErr) 
		return err;

	return noErr;
}
pascal OSErr nsMacTSMMessagePump::OffsetToPositionHandler(const AppleEvent *theAppleEvent, AppleEvent *reply, UInt32 handlerRefcon)
{
	OSErr 				err;
	DescType			returnedType;
	nsMacEventHandler*	eventHandler;	
	Size				actualSize;
	Point				thePoint;
	long				offset;
	nsresult			res;

	//
	// Extract the nsMacEvenbtHandler for this TSMDocument.  It's stored as the refcon.
	//
	err = AEGetParamPtr(theAppleEvent,keyAETSMDocumentRefcon,typeLongInteger,&returnedType,
						&eventHandler,sizeof(eventHandler),&actualSize);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::OffsetToPositionHandler: AEGetParamPtr[TSMRefcon] failed.");
	if (err!=noErr) 
		return err;
	
	//
	// Extract the Offset parameter
	//
	err = AEGetParamPtr(theAppleEvent,keyAEOffset,typeLongInteger,&returnedType,
						&offset,sizeof(offset),&actualSize);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::PositionToOffsetHandler: AEGetParamPtr[Offset] failed.");					
	if (err!=noErr) 
		return err;
	
	//
	// Pass the OffsetToPosition request to the widgets to handle
	//
	res = eventHandler->HandleOffsetToPosition(offset,&thePoint);
	NS_ASSERTION(NS_SUCCEEDED(res),"nsMacMessagePup::PositionToOffsetHandler: OffsetToPosition handler failed.");
	if (NS_FAILED(res)) 
		return paramErr;
	
	//
	// build up the reply (point)
	//
	err = AEPutParamPtr(reply,keyAEPoint,typeQDPoint,&thePoint,sizeof(Point));
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::PositionToOffsetHandler: AEPutParamPtr[Point][ failed.");
	if (err!=noErr) 
		return err;
	
	return noErr;
}
pascal OSErr nsMacTSMMessagePump::UpdateHandler(const AppleEvent *theAppleEvent, AppleEvent *reply, UInt32 handlerRefcon)
{
	OSErr 					err;
	DescType				returnedType;
	nsMacEventHandler*		eventHandler;	
	Size					actualSize;
	AEDesc					text, hiliteRangeArray;
	ScriptCode				textScript;
	long					fixLength;
	nsresult				res;
	TextRangeArray*			hiliteRangePtr;
	
	//
	// refcon stores the nsMacEventHandler
	//
	err = AEGetParamPtr(theAppleEvent,keyAETSMDocumentRefcon,typeLongInteger,&returnedType,
						&eventHandler,sizeof(eventHandler),&actualSize);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::UpdateHandler: AEGetParamPtr[TSMRefcon] failed.");
	if (err!=noErr) 
		return err;
	
	//
	// IME update text
	//
	err = AEGetParamDesc(theAppleEvent,keyAETheData,typeChar,&text);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::UpdateHandler: AEGetParamDesc[Text] failed.");
	if (err!=noErr) 
		return err;
	
	//
	// get the script of text for Unicode conversion
	//
	textScript=smUninterp;
	AEDesc slr;
	err = AEGetParamDesc(theAppleEvent,keyAETSMScriptTag,typeIntlWritingCode,&slr);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::UpdateHandler: AEGetParamDesc[keyAETSMScriptTag] failed.");
	if (err!=noErr) 
		return err;
	
	textScript = ((ScriptLanguageRecord *)(*(slr.dataHandle)))->fScript;
	NS_ASSERTION( (textScript < smUninterp), "Illegal script code");
	
	NS_ASSERTION(textScript == (ScriptCode)::GetScriptManagerVariable(smKeyScript) , "wrong script code");
	//
	// length of converted text
	//
	err = AEGetParamPtr(theAppleEvent,keyAEFixLength,typeLongInteger,&returnedType,
						&fixLength,sizeof(fixLength),&actualSize);
	NS_ASSERTION(err==noErr,"nsMacTSMMessagePump::UpdateHandler: AEGetParamPtr[fixlen] failed.");
  	if (err!=noErr) 
  		return err;

  	//
  	// extract the hilite ranges (optional param)
  	//
  	err = AEGetParamDesc(theAppleEvent,keyAEHiliteRange,typeTextRangeArray,&hiliteRangeArray);
	NS_ASSERTION(err==noErr||err==errAEDescNotFound,"nsMacTSMMessagePump::UpdateHandler: AEGetParamPtr[fixlen] failed.");
  	if (err==errAEDescNotFound) {
  		hiliteRangePtr=NULL;
  	} else if (err==noErr) { 
  		::HLock(hiliteRangeArray.dataHandle); 
  		hiliteRangePtr=(TextRangeArray*)*(hiliteRangeArray.dataHandle);
  	} else { 
  		return err;
  	}
  	
#if TARGET_CARBON
	// ��� Fix Me !!!!!
 	res = eventHandler->HandleUpdateInputArea((char*)textPtr,textScript,fixLength,hiliteRangePtr);
#else
	nsCAutoString mbcsText;
	Size text_size = ::GetHandleSize(text.dataHandle);
	mbcsText.SetCapacity(text_size+1);
	char* mbcsTextPtr = (char*)mbcsText.GetBuffer();
	strncpy(mbcsTextPtr,*(text.dataHandle),text_size);
	mbcsTextPtr[text_size]=0;
	
	//
	// must pass HandleUpdateInputArea a null-terminated multibyte string, the text size must include the terminator
	//
	res = eventHandler->HandleUpdateInputArea(mbcsTextPtr,text_size,textScript,fixLength,hiliteRangePtr);

#endif
	NS_ASSERTION(NS_SUCCEEDED(res),"nsMacMessagePump::UpdateHandler: HandleUpdated failed.");
	if (NS_FAILED(res)) 
		return paramErr;
	
	//
	// clean up
	//
#if !TARGET_CARBON
	if(hiliteRangePtr)
		::HUnlock(hiliteRangeArray.dataHandle);
#endif

	(void)AEDisposeDesc(&text);
	(void)AEDisposeDesc(&hiliteRangeArray);
						
	return noErr;
}
