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

//
// Mike Pinkerton, Netscape Communications
//
// <<Third time's the charm, I guess. I had this huge description of the complicated
// interactions of the tree and the selector widget and HT and it all _made sense_, and
// then we decided to go and rethink the UI from scratch. Sigh. So anyway, here's take 3.>>
//
// The RDFCoordiator suite of classes handles interactions between HT (the HyperTree interface
// to RDF) and the front end. The Coordinator's big job is funneling HT events to the right UI
// element (tree, title bar, etc). Right now, there are 4 basic types of Coordiators:
//   - CDockedRDFCoordinator
//       Tree is docked in the left of the window (handles opening/closing/etc of shelf)
//   - CShackRDFCoordinator
//       Tree is embedded in HTML content
//   - CWindowRDFCoordinator
//       Tree is in a standalone window
//   - CPopupRDFCoordinator
//       Tree is popped up from a container on the toolbar
//

#include "CRDFCoordinator.h"

#include "CHyperTreeFlexTable.h"
#include "CNavCenterTitle.h"
#include "UGraphicGizmos.h"


// XP headers
#include "xp_str.h"
#include "htrdf.h"
#include "xp_ncent.h"

// MacFE specific headers
#include "URDFUtilities.h"
#include "Netscape_Constants.h"
#include "URobustCreateWindow.h"
#include "BookmarksDialogs.h"
#include "divview.h"
#include "LGAIconSuiteControl.h"


#if 0
ViewFEData :: ViewFEData ( )
	: mSelector(NULL)
{
}


ViewFEData :: ViewFEData ( SelectorData* inSelector )
	: mSelector(inSelector)
{
}


ViewFEData :: ~ViewFEData ( )
{
	delete mSelector;
}
#endif

#pragma mark -

CRDFCoordinator::CRDFCoordinator(LStream* inStream)
:	LView(inStream),
	mTreePane(NULL),
	mHTPane(NULL),
	mColumnHeaders(NULL),
	mTitleStrip(NULL),
	mTitleCommandArea(NULL)
{
	*inStream >> mTreePaneID;
	*inStream >> mColumnHeaderID;
	*inStream >> mTitleStripID;
	*inStream >> mTitleCommandID;
	
} // constructor


CRDFCoordinator::~CRDFCoordinator()
{
	// if we don't do this, the LCommander destructor will try to clear the selection and
	// of course, the HTPane won't be around anymore to update the selection....boom....
	SwitchTarget(nil);
	
	if ( HTPane() ) {
		UnregisterNavCenter();
		HT_DeletePane ( mHTPane );
	}
	
} // destructor


void
CRDFCoordinator::FinishCreateSelf()
{
	// receive notifications from the tree view
	mTreePane = dynamic_cast<CHyperTreeFlexTable*>(FindPaneByID(mTreePaneID));
	Assert_( mTreePane != NULL );
	if (mTreePane)
		mTreePane->AddListener(this);

	mColumnHeaders = dynamic_cast<LPane*>(FindPaneByID(mColumnHeaderID));
	Assert_( mColumnHeaders != NULL );
	
	// Register the title strip as a listener so we can update it when the view
	// changes.
	mTitleStrip = dynamic_cast<CNavCenterTitle*>(FindPaneByID(mTitleStripID));
	Assert_( mTitleStrip != NULL );
	if ( mTitleStrip )
		AddListener(mTitleStrip);

	// Register the title command area as a listener so we can update it when the view
	// changes
	mTitleCommandArea = dynamic_cast<CNavCenterCommandStrip*>(FindPaneByID(mTitleCommandID));
	Assert_ ( mTitleCommandArea != NULL );
	if ( mTitleCommandArea ) {
		AddListener(mTitleCommandArea);

		// If the "show details" caption is there, register this class as a listener so we get the
		// mode toggle message
		LBroadcaster* showDetails = 
				dynamic_cast<LBroadcaster*>(FindPaneByID(CNavCenterCommandStrip::kDetailsPaneID));
		if ( showDetails )
			showDetails->AddListener(this);
	}

} // FinishCreateSelf


//
// ShowOrHideColumnHeaders
//
// Wrapper that checks the HT property to determine if we need to show or hide
// the column headers.
//
void
CRDFCoordinator :: ShowOrHideColumnHeaders ( )
{
	if ( !mColumnHeaders || !HTPane() )
		return;
	
	HT_Resource topNode = HT_TopNode(HT_GetSelectedView(HTPane()));
	bool shouldShowHeaders = URDFUtilities::PropertyValueBool(topNode, gNavCenter->showColumnHeaders, true);
	if ( shouldShowHeaders && mColumnHeaders->GetVisibleState() == triState_Off )
		ShowColumnHeaders();
	else if ( !shouldShowHeaders && mColumnHeaders->GetVisibleState() != triState_Off )
		HideColumnHeaders();

} // ShowOrHideColumnHeaders


//
// ShowColumnHeaders
//
// Make the column headers visible. Assumes they are invisible.
//
void
CRDFCoordinator :: ShowColumnHeaders ( )
{
	SDimension16 columnHeaderFrame;
	mColumnHeaders->GetFrameSize ( columnHeaderFrame );
	mColumnHeaders->Show();
	
	mTreePane->MoveBy ( 0, columnHeaderFrame.height, false );
	mTreePane->ResizeFrameBy ( 0, -columnHeaderFrame.height, false );

} // ShowColumnHeaders


//
// HideColumnHeaders
//
// Make the column headers invisible. Assumes they are visible.
//
void
CRDFCoordinator :: HideColumnHeaders ( )
{
	SDimension16 columnHeaderFrame;
	mColumnHeaders->GetFrameSize ( columnHeaderFrame );
	mColumnHeaders->Hide();
	
	mTreePane->MoveBy ( 0, -columnHeaderFrame.height, false );
	mTreePane->ResizeFrameBy ( 0, columnHeaderFrame.height, false );

} // HideColumnHeaders


//
// HandleNotification
//
// Process notification events from HT. Things like nodes being added or deleted, etc come
// here and then are farmed out to the appropriate subview.
//
void CRDFCoordinator::HandleNotification(
	HT_Notification	/*notifyStruct*/,
	HT_Resource		node,
	HT_Event		event,
	void			*token,
	uint32			tokenType)
{
	PRBool isOpen;
	HT_Error err;

	HT_View view = HT_GetView(node);		// everyone needs this, more or less
	
	switch (event)
	{
		case HT_EVENT_VIEW_MODECHANGED:

			// tell the command bar about the mode switch
			BroadcastMessage ( CNavCenterCommandStrip::msg_ModeSwitch ) ;
			
			ShowOrHideColumnHeaders();
			
			// redraw the tree with new colors, etc
			if ( view == mTreePane->GetHTView() ) {
				// if the new mode doesn't want selection, make sure there isn't one
				if ( URDFUtilities::PropertyValueBool(node, gNavCenter->useSelection, true) == false )
					mTreePane->UnselectAllCells();

				mTitleStrip->Refresh();
				mTreePane->Refresh();
				
				// make sure the tree knows the right number of clicks to open up a row. It may
				// have changed with the mode switch
				Uint8 clicksToOpen = 2;
				if ( URDFUtilities::PropertyValueBool(node, gNavCenter->useSingleClick, false) == true )
					clicksToOpen = 1;
				mTreePane->ClickCountToOpen(clicksToOpen);
			}
			
			break;
			
		case HT_EVENT_NODE_ADDED:
		{
			if ( view == mTreePane->GetHTView() ) {
				// HT_GetNodeIndex() will return the row where this new item should go
				// in a zero-based world. This translates, in a 1-based world of PP, to be
				// the node after which this new row will go. Conveniently, that is what
				// we want for InsertRows() so we don't have to add 1 like we normally do
				// when coverting between HT and PP.
				TableIndexT index = HT_GetNodeIndex(view, node);
				mTreePane->InsertRows(1, index, NULL, 0, true);
				mTreePane->SyncSelectionWithHT();
			}
			break;
		}

		case HT_EVENT_VIEW_SELECTED:
		{
			// if the current view is changed by HT (not interactively by the user), this
			// is the only place in the call chain where we will get notification. Make sure
			// the shelf is open/closed accordingly and the titlebar is updated.
			SelectView(view);
			
			bool openShelf = (view != NULL);
			ListenToMessage ( CDockedRDFCoordinator::msg_ShelfStateShouldChange, &openShelf );
			BroadcastMessage ( CRDFCoordinator::msg_ActiveSelectorChanged, view );
			break;
		}
		
		//
		// we get this event before the node opens/closes. This is useful for closing so
		// we can compute the number of visible children of the node being closed before
		// all that info goes away.
		//
		case HT_EVENT_NODE_OPENCLOSE_CHANGING:
			err = HT_GetOpenState(node, &isOpen);
			if (isOpen)
				CollapseNode(node);
			break;
		
		//
		// we get this event after the node has finished opening/closing.
		//
		case HT_EVENT_NODE_OPENCLOSE_CHANGED:
			err = HT_GetOpenState(node, &isOpen);
			if (isOpen)
				ExpandNode(node);
			break;
			
		case HT_EVENT_NODE_DELETED_DATA:
		case HT_EVENT_NODE_DELETED_NODATA:
		{
			// delete the FE data for a node (will be a CTreeIcon*)
			void* data = HT_GetNodeFEData ( node );
			delete data;
			break;
		}

		//
		// we get this event when a new node is created and is to be put in "inline edit" mode.
		//
		case HT_EVENT_NODE_EDIT:
		{
			//��� There are currently some problems with redraw here because of the way that
			// the drawing code and the inline editing code interact. You can uncomment the
			// line below and see that the cell does not draw correctly because it never gets
			// a drawCellContents() called on it (since it is the cell being edited). This
			// needs some work.....
			if ( view == mTreePane->GetHTView() ) {
				TableIndexT rowToEdit = URDFUtilities::HTRowToPPRow( HT_GetNodeIndex(view, node) );
				mTreePane->DoInlineEditing( rowToEdit );			
			}
			break;
		}
					
		case HT_EVENT_NODE_VPROP_CHANGED:
		{
			if ( node == HT_TopNode(mTreePane->GetHTView()) )
				mTreePane->Refresh();
			else
				mTreePane->RedrawRow(node);
			break;
		}
		
		case HT_EVENT_NODE_SELECTION_CHANGED:
			mTreePane->SyncSelectionWithHT();
			break;

		// scroll the given node into view. Currently uses (more or less) the LTableView
		// implementation which tries to scroll as little as possible and isn't very smart
		// about centering the node. We should change that at some point (pinkerton).
		case HT_EVENT_NODE_SCROLLTO:
			TableIndexT index = HT_GetNodeIndex(view, node);
//			mTreePane->ScrollRowIntoFrame(index);
			break;
		
		case HT_EVENT_VIEW_REFRESH:
		{
			// only update if current view
			//
			// ��� There are some fairly noticable redraw problems here. For example, when
			// nodes are deleted or added, the entire view is redrawn (and flashes). We
			// need some help from HT to prevent this.
			if ( view == mTreePane->GetHTView() ) { 
				uint32 newRowsInView = HT_GetItemListCount ( view );
				TableIndexT numRows, numCols;
				mTreePane->GetTableSize ( numRows, numCols );
			
				int32 delta = newRowsInView - numRows;
				if ( delta > 0 )
					mTreePane->InsertRows ( delta, 1 );
				else
					mTreePane->RemoveRows ( abs(delta), 1, false );
				mTreePane->SyncSelectionWithHT();
				mTreePane->Refresh();
			} // if refresh for current view
			break;
		}
			
		case HT_EVENT_COLUMN_ADD:
			// just wipe everything out and start over...
			if ( view == mTreePane->GetHTView() )
				mTreePane->SetupColumns();
			break;

		case HT_EVENT_COLUMN_DELETE:
			// delete any FE data here, but since we don't have any...
			break;

		case HT_EVENT_COLUMN_SIZETO:
		case HT_EVENT_COLUMN_REORDER:
		case HT_EVENT_COLUMN_SHOW:
		case HT_EVENT_COLUMN_HIDE:
			//��� not implemented
			break;

		case HT_EVENT_VIEW_WORKSPACE_REFRESH:			
		case HT_EVENT_VIEW_ADDED:
		case HT_EVENT_VIEW_DELETED:
			break;
		
	} // case of which HT event

} // HandleNotification


//
// SelectView
//
// Make the given view the current view.
//
void CRDFCoordinator::SelectView(HT_View view)
{
	if ( view ) {
		mTreePane->OpenView(view);
		BroadcastMessage ( CRDFCoordinator::msg_ActiveSelectorChanged, view );
	}
	
} // SelectView


//
// SelectView
//
// Make the given type of view the current view and (by calling the above routine)
// ensure that the selector widget is up to date. HT_SetSelectedView() will send
// us a notification event to open the given view, so we don't have to do it here.
//
void CRDFCoordinator::SelectView ( HT_ViewType inPane )
{
	HT_View view = HT_GetViewType ( mHTPane, inPane );
	HT_SetSelectedView ( mHTPane, view );

} // SelectView



void CRDFCoordinator::ExpandNode(HT_Resource node)
{
	mTreePane->ExpandNode(node);
}

void CRDFCoordinator::CollapseNode(HT_Resource node)
{
	mTreePane->CollapseNode(node);
}


//
// ListenToMessage
//
// Process the various messages we get from the FE, such as requests to open/close the tree shelf
// or change which workspace is currently selected.
void
CRDFCoordinator::ListenToMessage ( MessageT inMessage, void *ioParam )
{
	switch (inMessage) {

		case CNavCenterCommandStrip::msg_ModeSwitch:
			// this will cause us to get a HT_EVENT_VIEW_MODECHANGED event above
			HT_ToggleTreeMode ( HT_GetSelectedView(HTPane()) );
			break;
			
#if 0
//��� This might be useful, depending on how the command pane works out...
		// the user clicked in the selector pane to change the selected workspace. Tell
		// the backend about it, but before we do that, turn off HT events so we don't actually
		// get the notification back -- we don't need it because the view change was caused
		// by the FE.
		case msg_ActiveSelectorChanged:
		{
			HT_View newView = reinterpret_cast<HT_View>(ioParam);
			URDFUtilities::StHTEventMasking saveMask(mHTPane, HT_EVENT_NO_NOTIFICATION_MASK);
			HT_SetSelectedView(mHTPane, newView);
			SelectView(newView);
			break;
		}
#endif
					
	} // case of which message
	
} // ListenToMessage


//
// ObeyCommand
//
// Do what PowerPlant tells us. =)
//
Boolean
CRDFCoordinator :: ObeyCommand ( CommandT inCommand, void* ioParam )
{
	Boolean cmdHandled = false;

	if ( inCommand >= cmd_NavCenterBase && inCommand <= cmd_NavCenterCap ) {
		// �make sure nav center is open???
		HT_Error err = HT_DoMenuCmd ( mHTPane, (HT_MenuCmd)(inCommand - cmd_NavCenterBase) );
		Assert_( err == HT_NoErr );
		return true;
	}

	switch ( inCommand ) {
	
		//
		// handle commands that we have to share with other parts of the UI
		//
		case cmd_Cut:
			HT_DoMenuCmd(mHTPane, HT_CMD_CUT );
			cmdHandled = true;
			break;
		case cmd_Copy:
			HT_DoMenuCmd(mHTPane, HT_CMD_COPY );
			cmdHandled = true;
			break;
		case cmd_Paste:
			HT_DoMenuCmd(mHTPane, HT_CMD_PASTE );
			cmdHandled = true;
			break;
		case cmd_Clear:
			HT_DoMenuCmd(mHTPane, HT_CMD_DELETE_FILE );
			cmdHandled = true;
			break;

		case cmd_NCFind:
			URobustCreateWindow::CreateWindow ( CBookmarksFindDialog::res_ID, nil );
			cmdHandled = true;
			break;
		
		case msg_TabSelect:
			// don't accept focus switching to this view
			cmdHandled = false;
			break;

		default:
			cmdHandled = LCommander::ObeyCommand ( inCommand, ioParam );
			break;
			
	} // case on command

	return cmdHandled;
	
} // ObeyCommand


//
// RegisterNavCenter
//
// Tell XP about this navCenter & context for site map stuff
//
void
CRDFCoordinator :: RegisterNavCenter ( MWContext* inContext )
{
	if ( HTPane() )
		XP_RegisterNavCenter ( mHTPane, inContext );

} // RegisterNavCenter


//
// UnregisterNavCenter
//
// Tell XP this window is going away.
//
void
CRDFCoordinator :: UnregisterNavCenter ( )
{
	if ( HTPane() )
		XP_UnregisterNavCenter ( mHTPane );	

} // UnregisterNavCenter


//
// SetTargetFrame
//
// Pass-through to the tree to tell it which pane to target when items (url's) are
// double-clicked.
//
void
CRDFCoordinator :: SetTargetFrame ( const char* inFrame )
{
	mTreePane->SetTargetFrame ( inFrame );

} // SetTargetFrame


#pragma mark -

CDockedRDFCoordinator :: CDockedRDFCoordinator(LStream* inStream)
	: CRDFCoordinator(inStream), mNavCenter(NULL)
{
	// don't create the HT_pane until we actually need it (someone opens a node
	// and the want it to be docked).
}


CDockedRDFCoordinator :: ~CDockedRDFCoordinator()
{
	delete mNavCenter;

}


//
// FinishCreateSelf
//
// Setup stuff related to when this thing is embedded in a browser window
//
void
CDockedRDFCoordinator :: FinishCreateSelf ( )
{
	CRDFCoordinator::FinishCreateSelf();

	// initialize the navCenter shelf. If we are a standalone window, we won't have
	// a LDividedView so no expando-collapso stuff happens.
	LDividedView* navCenter = dynamic_cast<LDividedView*>(FindPaneByID('ExCt'));
	if ( navCenter )
		mNavCenter = new CShelf ( navCenter, NULL );

	// If the close caption is there, register this class as a listener so we get the
	// close message
	LBroadcaster* closeCaption = 
			dynamic_cast<LBroadcaster*>(FindPaneByID(CNavCenterCommandStrip::kClosePaneID));
	if ( closeCaption )
		closeCaption->AddListener(this);
	
} // FinishCreateSelf


//
// SavePlace
//
// Pass through to the tree view so it can save the shelf width
//
void
CDockedRDFCoordinator :: SavePlace ( LStream* outStreamData )
{
	if ( !outStreamData )
		return;
		
	mNavCenter->GetShelf()->SavePlace(outStreamData);
	
} // SavePlace


//
// RestorePlace
//
// Pass through to the tree view so it can restore the shelf width
//
void
CDockedRDFCoordinator :: RestorePlace ( LStream* inStreamData )
{
	if ( !inStreamData )	// if reading from new/empty prefs, the stream will be null
		return;
		
	mNavCenter->GetShelf()->RestorePlace(inStreamData);
	
} // RestorePlace


//
// ListenToMessages
//
// Respond to "shelf-related" messages. If it's not one of those, pass it to the base class.
//
void
CDockedRDFCoordinator :: ListenToMessage ( MessageT inMessage, void *ioParam )
{
	switch (inMessage) {

		// expand/collapse the shelf to the state pointed to by |ioParam|. If we don't
		// switch the target, we run into the problem where we are still the active
		// commander and get called on to handle the menus. Since there will be no
		// view, HT will barf.
		case CDockedRDFCoordinator::msg_ShelfStateShouldChange:
			if ( mNavCenter ) {
				bool nowOpen = *(reinterpret_cast<bool*>(ioParam));
				mNavCenter->SetShelfState ( nowOpen );
				if ( nowOpen ) {
					mTreePane->SetRightmostVisibleColumn(1);	//���avoid annoying columns
					SwitchTarget(this);
				}
				else
					SwitchTarget(GetSuperCommander());
			}
			break;
		
		// similar to above, but can cut out the crap because we are closing things
		// down explicitly.
		case CNavCenterCommandStrip::msg_CloseShelfNow:
			if ( mNavCenter ) {
				mNavCenter->SetShelfState ( false );
				SwitchTarget(GetSuperCommander());
			}
			break;

		default:
			CRDFCoordinator::ListenToMessage ( inMessage, ioParam );

	} // case of which message

} // ListenToMessage


//
// BuildHTPane
//
// Create a new pane with |inNode| as the root. Delete the old pane if it exists
// 
void
CDockedRDFCoordinator :: BuildHTPane ( HT_Resource inNode )
{
	// are we already displaying this one? If so, bail.
	HT_View currView = mTreePane->GetHTView();
	if ( currView && inNode == HT_TopNode(currView) )
		return;
		
	// out with the old
	if ( HTPane() ) {
		UnregisterNavCenter();
		HT_DeletePane ( mHTPane );
	}
	
	// in with the new
	mHTPane = CreateHTPane ( inNode );
	if ( mHTPane ) {
		ShowOrHideColumnHeaders();
		
		// we don't get a view selected event like other trees, so setup the tree 
		// view to point to the already selected view in HT and broadcast to tell 
		// the title bar to update the title.
		SelectView ( HT_GetSelectedView(mHTPane) );
	}
	

} // BuildHTPane


#pragma mark -


CShackRDFCoordinator :: CShackRDFCoordinator ( LStream* inStream )
	: CRDFCoordinator ( inStream )
{
	// don't create a default HT_Pane, it has to be done later
}


CShackRDFCoordinator :: ~CShackRDFCoordinator ( )
{

}


void
CShackRDFCoordinator :: BuildHTPane ( const char* inURL, unsigned int inCount, 
										char** inParamNames, char** inParamValues )
{
	mHTPane = CreateHTPane ( inURL, inCount, inParamNames, inParamValues );
	
	if ( mHTPane ) {
		ShowOrHideColumnHeaders();
		
		// we don't get a view selected event like other trees, so setup the tree 
		// view to point to the already selected view in HT and broadcast to tell 
		// the title bar to update the title.
		HT_View view =  HT_GetSelectedView(mHTPane);
		SelectView ( view );		
	}

} // BuildHTPane


#pragma mark -


CWindowRDFCoordinator :: CWindowRDFCoordinator ( LStream* inStream )
	: CRDFCoordinator ( inStream )
{
}

CWindowRDFCoordinator :: ~CWindowRDFCoordinator ( )
{
}


void
CWindowRDFCoordinator :: FinishCreateSelf ( )
{
	CRDFCoordinator::FinishCreateSelf();
	
} // FinishCreateSelf


void
CWindowRDFCoordinator :: BuildHTPane ( HT_Resource inNode )
{
	mHTPane = CreateHTPane ( inNode );
	if ( mHTPane ) {
		ShowOrHideColumnHeaders();
		
		// we don't get a view selected event like other trees, so setup the tree 
		// view to point to the already selected view in HT and broadcast to tell 
		// the title bar to update the title.
		SelectView ( HT_GetSelectedView(mHTPane) );
	}

} // BuildHTPane

void
CWindowRDFCoordinator :: BuildHTPane ( RDF_Resource inNode )
{
	mHTPane = CreateHTPane ( inNode );
	if ( mHTPane ) {
		ShowOrHideColumnHeaders();
		
		// we don't get a view selected event like other trees, so setup the tree 
		// view to point to the already selected view in HT and broadcast to tell 
		// the title bar to update the title.
		SelectView ( HT_GetSelectedView(mHTPane) );
	}

} // BuildHTPane


#pragma mark -


CPopdownRDFCoordinator :: CPopdownRDFCoordinator ( LStream* inStream )
	: CRDFCoordinator ( inStream )
{
}

CPopdownRDFCoordinator :: ~CPopdownRDFCoordinator ( )
{
}


//
// BuildHTPane
//
// Creates a new HT Pane rooted at the given resource. This can (and should) be called
// to switch the HT_Pane of a popdown that has already been created. It will correctly
// clean up after itself before making the new pane.
//
void
CPopdownRDFCoordinator :: BuildHTPane ( HT_Resource inNode )
{
	// out with the old, but only if it is not the pane originating d&d. 
	if ( mHTPane != CPopdownFlexTable::HTPaneOriginatingDragAndDrop() )
		HT_DeletePane ( mHTPane );
	
	// in with the new
	mHTPane = CreateHTPane ( inNode );
	if ( mHTPane ) {
		ShowOrHideColumnHeaders();
		
		// we don't get a view selected event like other trees, so setup the tree 
		// view to point to the already selected view in HT and broadcast to tell 
		// the title bar to update the title.
		SelectView ( HT_GetSelectedView(mHTPane) );
	}

} // BuildHTPane


//
// FinishCreateSelf
//
// Setup ourselves as a listener for the "close" button so we can pass along the
// message to the browser window.
//
void
CPopdownRDFCoordinator :: FinishCreateSelf ( )
{
	CRDFCoordinator::FinishCreateSelf();
	
	// If the close caption is there, register this class as a listener so we get the
	// close message
	LBroadcaster* closeCaption = 
			dynamic_cast<LBroadcaster*>(FindPaneByID(CNavCenterCommandStrip::kClosePaneID));
	if ( closeCaption )
		closeCaption->AddListener(this);

} // FinishCreateSelf


//
// FindCommandStatus
//
// All menu commands, etc should be disabled.
//
void
CPopdownRDFCoordinator :: FindCommandStatus ( 
									CommandT inCommand,
									Boolean	&outEnabled,
									Boolean	&outUsesMark,
									Char16	&outMark,
									Str255	outName)
{
	outEnabled = false;
	outUsesMark = false;

} // FindCommandStatus


//
// ListenToMessage
//
// Handle the message to close up the tree.
//
void
CPopdownRDFCoordinator :: ListenToMessage ( MessageT inMessage, void *ioParam )
{
	// pass through the message up to the browser window that can actually do
	// something about closing this popdown
	if ( inMessage == CPopdownFlexTable::msg_ClosePopdownTree ||
		inMessage == CNavCenterCommandStrip::msg_CloseShelfNow ) {
		BroadcastMessage ( msg_ClosePopdownTree );
	}
	else
		CRDFCoordinator::ListenToMessage ( inMessage, ioParam );

} // ListenToMessage


//
// DrawSelf
//
// Draw a sunken bevel frame so that we look like we are a drop target
//
void
CPopdownRDFCoordinator :: DrawSelf ( )
{
	Rect localRect;
	CalcLocalFrameRect(localRect);
	
	UGraphicGizmos::BevelTintRect ( localRect, -2, 0x6000, 0x6000 );

} // DrawSelf

