/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#pragma once
// ===========================================================================
//	UApp.h
// main application module
// ===========================================================================

#include <LDocApplication.h>
#include <LPeriodical.h>
#include <LWindow.h>
#include <LCaption.h>
#include "CTaskBarListener.h"
#include "CURLDispatcher.h"

#include <list.h>

#ifndef _NetscapeTypes_
#include "ntypes.h"
#endif

#include "cstring.h"

#include "structs.h"

class CStr255;
class EarlManager;
class LDialogBox;
class CMimeMapper;
class StDialogHandler;
class CBrowseWin;
class CCheckMailContext;
class CSplashScreen;
class CStr255;
class CMailNewsWindow;
class CBrowserWindow;
class CLICommander;

/*****************************************************************************
 * CFrontApp
 * the application class. It takes care of managing the documents. Only one 
 * preference document can be open at any time, and no documents can be opened
 * before we read in the preferences.
 * The application uses windows in place of documents. We could have discarded
 * documents completely, but this would affect AppleEvent recording (I think).
 * 
 * Window is created blank, without the home page. After the window is created,
 * window's function DoOpenURL should be called to open its first document.
 *****************************************************************************/


// ---------------------------------------------------------------------------
//	For the moment CAppleEventHandler requires a small bit of knowledge
//	about uapp.cp.  All shared info goes here til the separation can
//	be completed.

#define FILE_TYPE_PREFS		1
#define FILE_TYPE_ODOC		2
#define FILE_TYPE_GETURL	3
#define FILE_TYPE_NONE		4
#define FILE_TYPE_PROFILES	5
#define FILE_TYPE_ASW		6
#define FILE_TYPE_LDIF		7
#define STARTUP_TYPE_NETPROFILE 8

char* GetBookmarksPath( FSSpec& spec, Boolean useDefault );

// ----- End stuff required by CAppleEvent.cp


//----------------------------------------------------------------------------------------
class CFrontApp : public LDocApplication, public CTaskBarListener
//----------------------------------------------------------------------------------------
{
public:
	static CFrontApp*	sApplication;		// One and only instance of the application
	// �� Constructors/destructors
						CFrontApp();
	virtual				~CFrontApp();
	virtual void		Initialize();
	// for changing notification 
	virtual void		EventSuspend(const EventRecord &inMacEvent);
	virtual void		EventResume(const EventRecord &inMacEvent);

	// � patches in Mercutio
	virtual void		EventKeyDown( const EventRecord& inMacEvent );
	
	// so plug-ins can get key ups (which are ignored by default)
	virtual void		EventKeyUp(const EventRecord &inMacEvent);
	virtual void		ProcessNextEvent();

	Boolean				HasProperlyStartedUp() const { return fProperStartup; }
	
	// Delayed startup. Called with launched open/print file (if any).
	void 				ProperStartup( FSSpec* fileSpec, short fileType );
	
	// Startup w/o any documents
	virtual void		StartUp();		// Creates a home page document on startup
			void		CreateStartupEnvironment( Boolean openStartupWindows );	// Opens the proper window
	// �� documents/windows
	// Startup w/ a document or 
	virtual void		OpenDocument(FSSpec *inFileSpec);
	virtual void		PrintDocument(FSSpec *inFileSpec);
	virtual void		ChooseDocument();

	// Opens a hypertext document	
	virtual LModelObject* 	MakeNewDocument();
	
	// �� Global static routines. Could not think of better place to put them

	// Registers types with netlib
	static void			RegisterMimeType(CMimeMapper * mapper);

	// returns application object.

	static CFrontApp* 	GetApplication();	
	
	// �� Cached preference values (requires application restart to notice new values)
	
	Boolean				HasBookmarksMenu() const { return mHasBookmarksMenu; }
	Boolean				HasFrontierMenuSharing() const { return mHasFrontierMenuSharing; }
	Boolean				HasImportModule() const	{ return mImportModuleExists; }
	Boolean				HasAOLInstantMessenger() const	{ return mAOLMessengerExists; }
	// ���Menubar management

	static const list<CommandT>&
						GetCommandsToUpdateBeforeSelectingMenu() {
							return sCommandsToUpdateBeforeSelectingMenu;
						}

	virtual void		MakeMenuBar();
	virtual void		ClickMenuBar(const EventRecord& inMacEvent);
	virtual void		SetMenubar( ResIDT mbar, Boolean inUpdateNow = false );	// If appleMenu is 0, do not rebuild it
	virtual void		UpdateMenus();
	void				UpdateHierarchicalMenus();
	
	static void			InstallMenus();
	static int			BuildConfigurableMenu(MenuHandle, const char* xp_name, short stringsID = 0);
	Boolean				HandleSyntheticCommand( CommandT inCommand );
	static void			DoHelpMenuItem( short itemNum );

	// �� Command handling
	static void 		DoGetURL (const cstring& url, const char* inReferrer=NULL, const char* inTarget = NULL);
							// loads the given url into the frontmost window, or new one if there
							// is no frontmost
	static void			DoOpenDirectoryURL( CommandT menuCommand );
	static void			DoOpenLogoURL( CommandT menuCommand );
	void				OpenLocalURL( FSSpec* inFileSpec, 
									CBrowserWindow * win = NULL,
									char * mime_type = NULL,
									Boolean delayed = FALSE);	// use delayed argument if you are using drag'n'drop to avoid errors due to dialogs popping us
	void				OpenBookmarksFile( FSSpec* inFileSpec, CBrowserWindow * win, Boolean delayed);
	void				DoOpenDoc( FSSpec* initDoc, short fileType );

	// �� Taskbar support
	virtual void		ListenToMessage (MessageT inMessage, void *ioParam);
	
	virtual void		ShowAboutBox();

	//=== begin: add for TSMSupport

	virtual Boolean		AttemptQuitSelf(Int32 inSaveOption = kAEAsk);	
	virtual void		DoQuit(Int32 inSaveOption = kAEAsk);	
	virtual void		DispatchEvent(const EventRecord &inMacEvent);

	// splash screen
	static  void		SplashProgress(CStr255 inMessage);

	// ���Command and menus setup
	virtual Boolean		ObeyCommand(CommandT inCommand, void *ioParam = nil);
	virtual void		FindCommandStatus(CommandT inCommand,
											Boolean &outEnabled, Boolean &outUsesMark,
											Char16 &outMark, Str255 outName);
	virtual void 		ProcessCommandStatus(CommandT	inCommand,
											Boolean &outEnabled, Boolean &outUsesMark,
											Char16		&outMark, Str255		outName);


	
	// �� AE handling
	virtual void 		HandleAppleEvent(const AppleEvent	&inAppleEvent,
									AppleEvent			&outAEReply,
									AEDesc				&outResult,
									long				inAENumber);
	virtual void		GetAEProperty(DescType inProperty,
									const AEDesc	&inRequestedType,
									AEDesc			&outPropertyDesc) const;
	virtual void		SetAEProperty(DescType inProperty,
							const AEDesc	&inRequestedType,
							AEDesc			&outPropertyDesc);

	virtual void		SetupPage();

	// spy Apple Event suite

	static CMailNewsWindow*	GetMailNewsWindow();

#if 0
	// EA - Netcaster support
	void				LaunchNetcaster(void);
	MWContext*			GetNetcasterContext(void);	
	void				SetNetcasterContext(MWContext *);
#endif
	
	// 97-05-12 pkc -- dpi support for MWContext
	static double		sHRes;
	static double		sVRes;
	
	static CAutoPtr<CNSContext>	sRDFContext;

// DW FIX i made these non-static, and protected instead of public
protected:
	void				InitBookmarks();
#ifdef MOZ_LOC_INDEP
	void				InitializeLocationIndependence();
#endif	
	virtual void		UpdateMenusSelf();


	virtual void		AdjustCursor	(const EventRecord &inMacEvent);
	//=== end: add for TSMSupport

	void				ShowSplashScreen(void);
	void				DestroySplashScreen(void);

	CSplashScreen*		mSplashScreen;

	Boolean				AgreedToLicense( FSSpec* callbackWith, short fileType );
	
	void				MemoryIsLow();
	
	void				DoWindowsMenu(CommandT inCommand);	// Execute windows menu command


	void				DoOpenURLDialog(void);
#ifdef EDITOR
	void				DoOpenURLDialogInEditor(void);
#endif

	void				InsertItemIntoWindowsMenu(CStr255& title, 
							int currItem, LWindow * win, short iconID=0); // Utility function to add an item into windows menu

	void				LaunchExternalApp(OSType inAppSig, ResIDT inAppNameStringResourceID);

#ifdef IBM_HOST_ON_DEMAND
	Boolean				Find3270Applet(FSSpec& tn3270File);
	void				Launch3270Applet();
#endif

	Boolean				LaunchAccountSetup();
	

	ResIDT				fCurrentMbar;	// Currently active menu bar
	ResIDT				fWantedMbar;
	
	LArray				fWindowsMenu;	// LWindow * of windows in the window menu.
	static short		sHelpMenuOrigLength;
	static short		sHelpMenuItemCount;

	Boolean				fStartupAborted;
	Boolean				fSafeToQuit;
	Boolean				fUserWantsToQuit;
	Boolean				fProperStartup;
	
	LPeriodical*		mLibMsgPeriodical;
	
	Boolean				mConferenceApplicationExists;
	Boolean				mImportModuleExists;
	Boolean				mAOLMessengerExists;
	Boolean				mJavaEnabled;
	
	Boolean				mHasBookmarksMenu;
	Boolean				mHasFrontierMenuSharing;
	
	static list<CommandT>	sCommandsToUpdateBeforeSelectingMenu;
	
public:
	static	int			SetBooleanWithPref(const char *prefName, void *boolPtr);
							// boolPtr is really a (boolean *)

protected:
	FSSpec				CreateAccountSetupSpec();
	RgnHandle				mMouseRgnH;
};

#ifdef PROFILE
void		StartProfiling();
void		StopProfiling();
#endif

#ifdef MOZ_MAIL_NEWS
// TRUE if url *must* be loaded in this context type
Boolean URLRequiresContextType( const URL_Struct* url , MWContextType &type);
#endif

