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
/*---------------------------------------*/
/*																		*/
/* Name:		BookmarkBase.h											*/
/* Description:	XFE_BookmarkBase class header.							*/
/*				Base class for components dealing with bookmarks.		*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/* Date:		Tue Mar  4 03:45:16 PST 1997							*/


/*																		*/
/*----------------------------------------------------------------------*/

#include "NotificationCenter.h"
#include "Frame.h"
#include "intl_csi.h"
#include "htrdf.h"

#ifndef _xfe_bookmark_base_h_
#define _xfe_bookmark_base_h_

// This class can be used with a DYNA_CASCADEBUTTON or
// DYNA_MENUITEMS.

class XFE_BookmarkBase : public XFE_NotificationCenter
{

public:

 	XFE_BookmarkBase			(XFE_Frame *	frame,
 								 XP_Bool		onlyHeaders,
								 XP_Bool		fancyItems);

	virtual ~XFE_BookmarkBase	();

    void newPane();
    void setPaneFromURL(char * url);
    void setPaneFromResource(HT_Resource node);

	static HT_Resource HT_FindFolderByName	(HT_Resource		root_entry,
											 char *			folder_name);

	static HT_Resource HT_FindEntryByAddress(HT_Resource		root_entry,
										 const char *	entry_name);

	static HT_Resource HT_FindNextEntry		(HT_Resource		entry);
	static HT_Resource HT_FindPreviousEntry	(HT_Resource		entry);

	static XP_Bool	HT_ResourceHasHeaderChildren	(HT_Resource		header);

	static void		guessTitle			(XFE_Frame *	frame,
										 const char *	address,
										 XP_Bool		sameShell,
										 char **		resolvedTitleOut,
										 time_t *		resolvedLastDateOut);
	

	void			addEntryAfter		(const char *	address,
										 const char *	title,
										 time_t		lastAccess,
										 HT_Resource		entryAfter);
	
	void			addEntryBefore		(const char *	address,
										 const char *	title,
										 time_t		lastAccess,
										 HT_Resource		entryBefore);
	
	void			addEntryToFolder	(const char *	address,
										 const char *	title,
										 time_t		lastAccess,
										 HT_Resource		header);

	void			setDropAddress		(const char *	address);
	void			setDropTitle		(const char *	title);
	void			setDropLastAccess	(time_t		lastAccess);

	const char *	getDropAddress		();
	const char *	getDropTitle		();
	time_t			getDropLastAccess	();

	// Access methods
	XFE_Frame *		getFrame			();

	virtual void	enableDropping			();
	virtual void	disableDropping			();
	virtual XP_Bool	isDroppingEnabled		();

protected:

	static Pixmap _bookmarkPixmap;
	static Pixmap _bookmarkMask;

	static Pixmap _bookmarkChangedPixmap;
	static Pixmap _bookmarkChangedMask;

	static Pixmap _mailBookmarkPixmap;
	static Pixmap _mailBookmarkMask;

	static Pixmap _newsBookmarkPixmap;
	static Pixmap _newsBookmarkMask;

	static Pixmap _folderArmedPixmap;
	static Pixmap _folderArmedMask;
	static Pixmap _folderPixmap;
	static Pixmap _folderMask;

	static Pixmap _personalFolderArmedMask;
	static Pixmap _personalFolderArmedPixmap;
	static Pixmap _personalFolderMask;
	static Pixmap _personalFolderPixmap;

	static Pixmap _newFolderArmedMask;
	static Pixmap _newFolderArmedPixmap;
	static Pixmap _newFolderMask;
	static Pixmap _newFolderPixmap;

	static Pixmap _menuFolderArmedMask;
	static Pixmap _menuFolderArmedPixmap;
	static Pixmap _menuFolderMask;
	static Pixmap _menuFolderPixmap;

	static Pixmap _newMenuFolderArmedMask;
	static Pixmap _newMenuFolderArmedPixmap;
	static Pixmap _newMenuFolderMask;
	static Pixmap _newMenuFolderPixmap;

	static Pixmap _newPersonalFolderArmedMask;
	static Pixmap _newPersonalFolderArmedPixmap;
	static Pixmap _newPersonalFolderMask;
	static Pixmap _newPersonalFolderPixmap;

	static Pixmap _newPersonalMenuFolderArmedMask;
	static Pixmap _newPersonalMenuFolderArmedPixmap;
	static Pixmap _newPersonalMenuFolderMask;
	static Pixmap _newPersonalMenuFolderPixmap;


	void getPixmapsForEntry(HT_Resource	entry,
							Pixmap *	pixmap,
							Pixmap *	mask,
							Pixmap *	armedPixmap,
							Pixmap *	armedMask);

#if 0
	/* 	static */ Pixmap bookmarkPixmapFromEntry(HT_Resourceentry);
 	/* static */ Pixmap bookmarkMaskFromEntry(HT_Resourceentry);
#endif

	static void getBookmarkPixmaps(Pixmap & pixmap_out,Pixmap & mask_out);

#if 0
	// Access bookmark entries
	HT_Resource		getFirstEntry			();
	HT_Resource		getTopLevelFolder		(const char * name);
#endif /*0*/

	// Create a bookmark item menu tree
	void			createItemTree			(Widget menu,HT_Resource entry);

	// Install submenu pane that controls dropping disability
	void			trackSubmenuMapping		(Widget submenu);

	// Override in derived class to configure
	virtual void	entryArmed				(Widget,HT_Resource);
	virtual void	entryDisarmed			(Widget,HT_Resource);
	virtual void	entryActivated			(Widget,HT_Resource);
	virtual void	entryCascading			(Widget,HT_Resource);
	virtual void	entryEnter				(Widget,HT_Resource);
	virtual void	entryLeave				(Widget,HT_Resource);

	// Gets called when the whole thing needs updating
	virtual void	prepareToUpdateRoot		();

	// Gets called to actually update the whole thing
	virtual void	reallyUpdateRoot		();

	// Gets called to update icon appearance
	virtual void	updateAppearance		();

	// Gets called when the personal toolbar folder's name changes
	virtual void	updateToolbarFolderName	();

	// Configure the items
	virtual void	configureXfeCascade		(Widget,HT_Resource);
	virtual void	configureXfeButton		(Widget,HT_Resource);
	virtual void	configureXfeBmButton	(Widget,HT_Resource);
	virtual void	configureXfeBmCascade	(Widget,HT_Resource);
	virtual void	configureButton			(Widget,HT_Resource);
	virtual void	configureCascade		(Widget,HT_Resource);
	virtual void	configureSeparator		(Widget,HT_Resource);

	// Menu component creation methods
	Widget	createCascadeButton		(Widget menu,HT_Resource entry,XP_Bool ignore_children);
	Widget	createPushButton		(Widget menu,HT_Resource entry);
	Widget	createSeparator			(Widget menu);
	Widget	createMoreButton		(Widget menu);

	// Toolbar component creation methods
	Widget	createXfeCascade		(Widget parent,HT_Resource entry);
	Widget	createXfeButton			(Widget parent,HT_Resource entry);

	HT_Resource getRootFolder		();
	HT_Resource getAddFolder			();
	HT_Resource getMenuFolder		();


    HT_Pane _ht_pane;
    HT_View _ht_view;
    HT_NotificationStruct _ht_ns;

private:

	XFE_Frame *			_frame;					// The ancestor frame
	XP_Bool				_onlyHeaders;			// Only show headers
	XP_Bool				_fancyItems;			// Fancy items (pixmap & label)
	INTL_CharSetInfo	_charSetInfo;			// Char set info

	char *				_dropAddressBuffer;		// 
	char *				_dropTitleBuffer;		// 
	time_t				_dropLastAccess;		// 

	// Item callbacks
	static void		item_armed_cb		(Widget,XtPointer,XtPointer);
	static void		item_disarmed_cb	(Widget,XtPointer,XtPointer);
 	static void		item_activated_cb	(Widget,XtPointer,XtPointer);
 	static void		item_cascading_cb	(Widget,XtPointer,XtPointer);
 	static void		item_enter_cb		(Widget,XtPointer,XtPointer);
 	static void		item_leave_cb		(Widget,XtPointer,XtPointer);
 	static void		item_free_data_cb	(Widget,XtPointer,XtPointer);
	static void		pane_mapping_eh		(Widget,XtPointer,XEvent *,Boolean *);

	// Format item blah blah blah
	static XmString	formatItem			(HT_Resource		entry, 
										 Boolean		no_indent,
										 int16			charset);

	// Obtain an internationallized XmString from an entry
	static XmString entryToXmString	(HT_Resource			entry,
									 INTL_CharSetInfo	char_set_info);


	void setItemLabelString(Widget menu, HT_Resource entry);

	void createPixmaps();

	Widget	getLastMoreMenu		(Widget menu);

	// Bookmarks changed notice invoked by MozillaApp
	XFE_CALLBACK_DECL(bookmarksChanged)

    // update the icon appearance
    XFE_CALLBACK_DECL(updateIconAppearance)

    void notify(HT_Notification ns, HT_Resource n, HT_Event whatHappened);

    static void notify_cb(HT_Notification ns, HT_Resource n, 
                          HT_Event whatHappened, 
                          void *token, uint32 tokenType);
};


#endif /* _xfe_bookmark_base_h_ */
