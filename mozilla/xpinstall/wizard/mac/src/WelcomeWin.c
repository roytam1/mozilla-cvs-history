/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/ 
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License 
 * for the specific language governing rights and limitations under the 
 * License. 
 * 
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are Copyright (C) 1999
 * Netscape Communications Corporation. All Rights Reserved.  
 * 
 * Contributors:
 *     Samir Gehani <sgehani@netscape.com>
 */

#include "MacInstallWizard.h"


/*-----------------------------------------------------------*
 *   Welcome Window
 *-----------------------------------------------------------*/

void 
ShowWelcomeWin(void)
{
	Str255 next;
	Str255 back;
	Rect sbRect;
	int sbWidth;
	
	GrafPtr	oldPort;
	GetPort(&oldPort);
	
	if (gWPtr != NULL)
	{
		SetPort(gWPtr);

		gCurrWin = kWelcomeID; 
		/* gControls->ww = (WelcWin *) NewPtrClear(sizeof(WelcWin)); */
	
		GetIndString(next, rStringList, sNextBtn);
		GetIndString(back, rStringList, sBackBtn);
	
		gControls->ww->scrollBar = GetNewControl( rLicScrollBar, gWPtr);
		gControls->ww->welcBox = GetNewControl( rLicBox, gWPtr);

		if(gControls->ww->scrollBar && gControls->ww->welcBox)
		{
			HLock( (Handle) gControls->ww->scrollBar);
			sbRect = (*(gControls->ww->welcBox))->contrlRect;
				
			sbWidth = (*(gControls->ww->scrollBar))->contrlRect.right -
					  (*(gControls->ww->scrollBar))->contrlRect.left;
		
			(*(gControls->ww->scrollBar))->contrlRect.right = sbRect.right + kScrollBarPad;
			(*(gControls->ww->scrollBar))->contrlRect.left = sbRect.right + kScrollBarPad - 
														 sbWidth;
			(*(gControls->ww->scrollBar))->contrlRect.top = sbRect.top - kScrollBarPad;
			(*(gControls->ww->scrollBar))->contrlRect.bottom = sbRect.bottom + kScrollBarPad;
			HUnlock( (Handle) gControls->ww->scrollBar);
		}
		InitWelcTxt();

		ShowNavButtons( back, next);
		if (gControls->cfg->bReadme)
			ShowReadmeButton();
		ShowControl( gControls->ww->scrollBar);
		ShowControl( gControls->ww->welcBox);
		ShowTxt();
		InitScrollBar( gControls->ww->scrollBar);
	}
	
	SetPort(oldPort);
}

void
InitWelcTxt(void)
{
	Rect 		viewRect, destRect;
	long		welcStrLen;
	int 		i;
	
	/* TE specific init */
	HLock( (Handle) gControls->ww->welcBox);
	viewRect = (*(gControls->ww->welcBox))->contrlRect;	
	HUnlock( (Handle) gControls->ww->welcBox);

	destRect.left = viewRect.left;
		viewRect.right = (*(gControls->ww->scrollBar))->contrlRect.left; 
	destRect.right = viewRect.right - 1;
	destRect.top = viewRect.top;
	destRect.bottom = viewRect.bottom * kNumWelcScrns; /* XXX: hack */

	TextFont(applFont);
	TextFace(normal);
	TextSize(12);
	
	gControls->ww->welcTxt = TENew( &destRect, &viewRect);
	if (!gControls->ww->welcTxt)
	{
		ErrorHandler();
		return;
	}

	for (i=0; i<kNumWelcMsgs; i++)
	{
		HLock(gControls->cfg->welcMsg[i]);
		welcStrLen = strlen( *gControls->cfg->welcMsg[i]);
		TEInsert( *gControls->cfg->welcMsg[i], welcStrLen, gControls->ww->welcTxt);
		HUnlock(gControls->cfg->welcMsg[i]);
		TEInsert( "\r\r", 2, gControls->ww->welcTxt);
	}
	
	TextFont(systemFont);
	TextSize(12);
	
	TESetAlignment(teFlushDefault, gControls->ww->welcTxt);
}

void 
InWelcomeContent(EventRecord* evt, WindowPtr wCurrPtr)
{	
	Point 			localPt;
	Rect			r;
	short 			code, value;
	ControlPartCode	part;
	ControlHandle	scrollBar;
	ControlActionUPP	scrollActionFunctionUPP;
	GrafPtr			oldPort;
	
	GetPort(&oldPort);
	SetPort(wCurrPtr);
	localPt = evt->where;
	GlobalToLocal( &localPt);
	
	code = FindControl(localPt, wCurrPtr, &scrollBar);
	switch (code)
	{
		case kControlUpButtonPart:
		case kControlDownButtonPart:
		case kControlPageUpPart:
		case kControlPageDownPart:
			scrollActionFunctionUPP = NewControlActionProc((ProcPtr) DoScrollProc);
			value = TrackControl(scrollBar, localPt, scrollActionFunctionUPP);
			DisposeRoutineDescriptor(scrollActionFunctionUPP);
			return;
			
		case kControlIndicatorPart:
			value = GetControlValue(scrollBar);
			code = TrackControl(scrollBar, localPt, nil);
			if (code) 
			{
				value -= GetControlValue(scrollBar);
				if (value) 
				{
					TEScroll(0, value * kScrollAmount, gControls->ww->welcTxt);
				}
			}
			return;
	}	
			
#if 0
	HLock((Handle)gControls->backB);
	r = (**(gControls->backB)).contrlRect;
	HUnlock((Handle)gControls->backB);
	if (PtInRect( localPt, &r))
	{
		part = TrackControl(gControls->backB, evt->where, NULL);
		if (part)
		{
			KillControls(gWPtr);
			ShowLicenseWin();
			return;
		}
	}
#endif
	
	HLock((Handle)gControls->nextB);			
	r = (**(gControls->nextB)).contrlRect;
	HUnlock((Handle)gControls->nextB);
	if (PtInRect( localPt, &r))
	{
		part = TrackControl(gControls->nextB, evt->where, NULL);
		if (part)
		{
			KillControls(gWPtr);
			ShowSetupTypeWin();
			return;
		}
	}
	
	HLock((Handle)gControls->ww->readmeButton);
	r = (**(gControls->ww->readmeButton)).contrlRect;
	HUnlock((Handle)gControls->ww->readmeButton);
	if (PtInRect(localPt, &r))
	{
		part = TrackControl(gControls->ww->readmeButton, evt->where, NULL);
		if (part)
		{
			ShowReadme();
			return;
		}
	}
	
	SetPort(oldPort);
}

void 
ShowReadmeButton(void)
{
	Str255 readme;
	
	GetIndString(readme, rStringList, sReadme);
	gControls->ww->readmeButton = GetNewControl(rReadmeBtn, gWPtr);
	if (gControls->ww->readmeButton != NULL)
	{
		SetControlTitle(gControls->ww->readmeButton, readme);
		ShowControl(gControls->ww->readmeButton);
	}
}
	
#define UNIFY_CHAR_CODE(_targetUint32, _src1char, _src2char, _src3char, _src4char) 	\
	_targetUint32 = 																\
	( (unsigned long) 																\
	(((unsigned long)((_src1char & 0x000000FF) << 24) 								\
	|(unsigned long)((_src2char & 0x000000FF) << 16) 								\
	|(unsigned long)((_src3char & 0x000000FF) << 8)									\
	|(unsigned long)((_src4char & 0x000000FF)))))
	
void
ShowReadme(void)
{
	Ptr appSig;
	StringPtr file;
	OSErr err = noErr;
	FSSpec appSpec, docSpec;
	Boolean running = nil;
	ProcessSerialNumber psn;
	unsigned short launchFileFlags, launchControlFlags;
	unsigned long appSigULong;
	long currDirID;
	short currVRefNum;
	
	appSig = *gControls->cfg->readmeApp;
	appSigULong = 0x00000000;
	UNIFY_CHAR_CODE(appSigULong, *(appSig), *(appSig+1), *(appSig+2), *(appSig+3));
	err = FindAppUsingSig(appSigULong, &appSpec, &running, &psn);
	if (err != noErr)
	{
		SysBeep(10); // XXX  show error dialog
		goto au_revoir;
	}
	
	file = CToPascal(*gControls->cfg->readmeFile);
	GetCWD(&currDirID, &currVRefNum);
	err = FSMakeFSSpec(currVRefNum, currDirID, file, &docSpec);
	if (err != noErr)
	{
		SysBeep(10); // XXX  show error dialog
		goto au_revoir;
	}
		
	launchFileFlags = NULL;
	launchControlFlags = launchContinue + launchNoFileFlags + launchUseMinimum;
	err = LaunchAppOpeningDoc(running, &appSpec, &psn, &docSpec, 
							launchFileFlags, launchControlFlags);
	if (err != noErr)
	{
		SysBeep(10); // XXX  show error dialog
		goto au_revoir;
	}

au_revoir:	
	if (file)
		DisposePtr((Ptr) file);
		
	return;
}

void
EnableWelcomeWin(void)
{
	EnableNavButtons();
	
	if (gControls->cfg->bReadme)
		if (gControls->ww->readmeButton)
			HiliteControl(gControls->ww->readmeButton, kEnableControl);

	if(gControls->ww->scrollBar)
		HiliteControl(gControls->ww->scrollBar, kEnableControl);
}

void
DisableWelcomeWin(void)
{
	DisableNavButtons();
	
	if (gControls->cfg->bReadme)
		if (gControls->ww->readmeButton)
			HiliteControl(gControls->ww->readmeButton, kDisableControl);
			
	if(gControls->ww->scrollBar)
		HiliteControl(gControls->ww->scrollBar, kDisableControl);
}

OSErr LaunchAppOpeningDoc (Boolean running, FSSpec *appSpec, ProcessSerialNumber *psn,
	FSSpec *docSpec, unsigned short launchFileFlags, unsigned short launchControlFlags)
{
	ProcessSerialNumber thePSN;
	AEDesc 		target = {0, nil};
	AEDesc 		docDesc = {0, nil};
	AEDesc 		launchDesc = {0, nil};
	AEDescList 	docList = {0, nil};
	AppleEvent 	theEvent = {0, nil};
	AppleEvent 	theReply = {0, nil};
	OSErr 		err = noErr;
	Boolean 		autoParamValue = false;

	if (running) thePSN = *psn;
	err = AECreateDesc(typeProcessSerialNumber, &thePSN, sizeof(thePSN), &target); 
	if (err != noErr) goto exit;
	
	err = AECreateAppleEvent(kCoreEventClass, kAEOpenDocuments, &target,
		kAutoGenerateReturnID, kAnyTransactionID, &theEvent);
	if (err != noErr) goto exit;
	
	if (docSpec)
	{
		err = AECreateList(nil, 0, false, &docList);
		if (err != noErr) goto exit;
		
		err = AECreateDesc(typeFSS, docSpec, sizeof(FSSpec), &docDesc);
		if (err != noErr) goto exit;
		
		err = AEPutDesc(&docList, 0, &docDesc);
		if (err != noErr) goto exit;
		
		err = AEPutParamDesc(&theEvent, keyDirectObject, &docList);
		if (err != noErr) goto exit;
	}
	
	if (running)
	{
		err = AESend(&theEvent, &theReply, kAENoReply, kAENormalPriority, kNoTimeOut, nil, nil);
		if (err != noErr) goto exit;
		if ((launchControlFlags & launchDontSwitch) == 0) {
			err = SetFrontProcess(psn);
			if (err != noErr) goto exit;
		}
	}
	else
	{
		LaunchParamBlockRec	launchThis = {0};
		
		err = AECoerceDesc(&theEvent, typeAppParameters, &launchDesc);
		if (err != noErr) goto exit;
		HLock(theEvent.dataHandle);
		
		launchThis.launchAppSpec = appSpec;
		launchThis.launchAppParameters = (AppParametersPtr)*launchDesc.dataHandle;
		launchThis.launchBlockID = extendedBlock;
		launchThis.launchEPBLength = extendedBlockLen;
		launchThis.launchFileFlags = launchFileFlags;
		launchThis.launchControlFlags = launchControlFlags;
		err = LaunchApplication(&launchThis);
	}
	
exit:

	if (target.dataHandle != nil) AEDisposeDesc(&target);
	if (docDesc.dataHandle != nil) AEDisposeDesc(&docDesc);
	if (launchDesc.dataHandle != nil) AEDisposeDesc(&launchDesc);
	if (docList.dataHandle != nil) AEDisposeDesc(&docList);
	if (theEvent.dataHandle != nil) AEDisposeDesc(&theEvent);
	if (theReply.dataHandle != nil) AEDisposeDesc(&theReply);
	return err;
}

OSErr FindAppUsingSig (OSType sig, FSSpec *fSpec, Boolean *running, ProcessSerialNumber *psn)
{
	OSErr 	err = noErr;
	short 	sysVRefNum, vRefNum, index;
	Boolean 	hasDesktopDB;

	if (running != nil) {
		err = FindRunningAppBySignature(sig, fSpec, psn);
		*running = true;
		if (err == noErr) return noErr;
		*running = false;
		if (err != procNotFound) return err;
	}
	err = GetSysVolume(&sysVRefNum);
	if (err != noErr) return err;
	vRefNum = sysVRefNum;
	index = 0;
	while (true) {
		if (index == 0 || vRefNum != sysVRefNum) {
			err = VolHasDesktopDB(vRefNum, &hasDesktopDB);
			if (err != noErr) return err;
			if (hasDesktopDB) {
				err = FindAppOnVolume(sig, vRefNum, fSpec);
				if (err != afpItemNotFound) return err;
			}
		}
		index++;
		err = GetIndVolume(index, &vRefNum);
		if (err == nsvErr) return fnfErr;
		if (err != noErr) return err;
	}
}

OSErr FindRunningAppBySignature (OSType sig, FSSpec *fSpec, ProcessSerialNumber *psn)
{
	OSErr 			err = noErr;
	ProcessInfoRec 	info;
	FSSpec			tempFSSpec;
	
	psn->highLongOfPSN = 0;
	psn->lowLongOfPSN  = kNoProcess;
	while (true)
	{
		err = GetNextProcess(psn);
		if (err != noErr) return err;
		info.processInfoLength = sizeof(ProcessInfoRec);
		info.processName = nil;
		info.processAppSpec = &tempFSSpec;
		err = GetProcessInformation(psn, &info);
		if (err != noErr) return err;
		
		if (info.processSignature == sig)
		{
			if (fSpec != nil)
				*fSpec = tempFSSpec;
			return noErr;
		}
	}
	
	return procNotFound;
}

static OSErr VolHasDesktopDB (short vRefNum, Boolean *hasDesktop)
{
	HParamBlockRec 		pb;
	GetVolParmsInfoBuffer	info;
	OSErr 				err = noErr;
	
	pb.ioParam.ioCompletion = nil;
	pb.ioParam.ioNamePtr = nil;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.ioParam.ioBuffer = (Ptr)&info;
	pb.ioParam.ioReqCount = sizeof(info);
	err = PBHGetVolParmsSync(&pb);
	*hasDesktop = err == noErr && (info.vMAttrib & (1L << bHasDesktopMgr)) != 0;
	return err;
}

static OSErr FindAppOnVolume (OSType sig, short vRefNum, FSSpec *file)
{
	DTPBRec pb;
	OSErr err = noErr;
	short ioDTRefNum, i;
	FInfo fInfo;
	FSSpec candidate;
	unsigned long lastModDateTime, maxLastModDateTime;

	memset(&pb, 0, sizeof(DTPBRec));
	pb.ioCompletion = nil;
	pb.ioVRefNum = vRefNum;
	pb.ioNamePtr = nil;
	err = PBDTGetPath(&pb);
	if (err != noErr) return err;
	ioDTRefNum = pb.ioDTRefNum;

	memset(&pb, 0, sizeof(DTPBRec));
	pb.ioCompletion = nil;
	pb.ioIndex = 0;
	pb.ioFileCreator = sig;
	pb.ioNamePtr = file->name;
	pb.ioDTRefNum = ioDTRefNum;
	err = PBDTGetAPPL(&pb, false);
	
	if (err == fnfErr || err == paramErr) return afpItemNotFound;
	if (err != noErr) return err;

	file->vRefNum = vRefNum;
	file->parID = pb.ioAPPLParID;
	
	err = FSpGetFInfo(file, &fInfo);
	if (err == noErr) return noErr;
	
	i = 1;
	maxLastModDateTime = 0;
	while (true) {
		memset(&pb, 0, sizeof(DTPBRec)); 
		pb.ioCompletion = nil;
		pb.ioIndex = i;
		pb.ioFileCreator = sig;
		pb.ioNamePtr = candidate.name;
		pb.ioDTRefNum = ioDTRefNum;
		err = PBDTGetAPPLSync(&pb);
		if (err != noErr) break;
		candidate.vRefNum = vRefNum;
		candidate.parID = pb.ioAPPLParID;
		err = GetLastModDateTime(&candidate, &lastModDateTime);
		if (err == noErr) {
			if (lastModDateTime > maxLastModDateTime) {
				maxLastModDateTime = lastModDateTime;
				*file = candidate;
			}
		}
		i++;
	}
	
	return maxLastModDateTime > 0 ? noErr : afpItemNotFound;
}

OSErr GetSysVolume (short *vRefNum)
{
	long dir;
	
	return FindFolder(kOnSystemDisk, kSystemFolderType, false, vRefNum, &dir);
}

OSErr GetIndVolume (short index, short *vRefNum)
{
	ParamBlockRec pb;
	OSErr err = noErr;
	
	pb.volumeParam.ioCompletion = nil;
	pb.volumeParam.ioNamePtr = nil;
	pb.volumeParam.ioVolIndex = index;
	
	err = PBGetVInfoSync(&pb);
	
	*vRefNum = pb.volumeParam.ioVRefNum;
	return err;
}

OSErr GetLastModDateTime(const FSSpec *fSpec, unsigned long *lastModDateTime)
{
	CInfoPBRec	pBlock;
	OSErr 		err = noErr;
	
	pBlock.hFileInfo.ioNamePtr = (StringPtr)fSpec->name;
	pBlock.hFileInfo.ioVRefNum = fSpec->vRefNum;
	pBlock.hFileInfo.ioFDirIndex = 0;
	pBlock.hFileInfo.ioDirID = fSpec->parID;
	err = PBGetCatInfoSync(&pBlock);
	if (err != noErr) return err;
	*lastModDateTime = pBlock.hFileInfo.ioFlMdDat;
	return noErr;
}

