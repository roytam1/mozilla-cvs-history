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
 *   Components Window
 *-----------------------------------------------------------*/

#define INVERT_HIGHLIGHT(_rectPtr)          \
			hiliteVal = LMGetHiliteMode();  \
			BitClr(&hiliteVal, pHiliteBit); \
			LMSetHiliteMode(hiliteVal);	    \
			InvertRect(_rectPtr);
			
static int rowToComp[kMaxComponents];
static int numRows = 0;			

void 
ShowComponentsWin(void)
{
	Str255		next, back;
	Str255		compDescTitle;
	StringPtr	selCompMsg;
	Handle		listBoxRect;
	Rect 		dataBounds, listBoxFrame, viewRect;
	short		reserr;
	int			totalRows = 0, i, instChoice;
	Point		cSize;
	Boolean		bCellSelected;
	GrafPtr		oldPort;
	GetPort(&oldPort);
		
	SetPort(gWPtr);
	
	gCurrWin = kComponentsID; 
	/* gControls->cw = (CompWin *) NewPtrClear(sizeof(CompWin)); */
	
	GetIndString(next, rStringList, sNextBtn);
	GetIndString(back, rStringList, sBackBtn);

	// get controls
	listBoxRect = Get1Resource('RECT', rCompListBox);
	reserr = ResError(); 
	if (reserr == noErr && listBoxRect != NULL)
	{
		HLock((Handle)listBoxRect);
		SetRect(&gControls->cw->compListBox, 	((Rect*)*listBoxRect)->left,
												((Rect*)*listBoxRect)->top,
												((Rect*)*listBoxRect)->right,
												((Rect*)*listBoxRect)->bottom);
		HUnlock((Handle)listBoxRect);
	}
	else
	{
		ErrorHandler();
		return;
	}
	gControls->cw->compDescBox = NULL;
	gControls->cw->compDescBox = GetNewControl(rCompDescBox, gWPtr);
	if (!gControls->cw->compDescBox)
	{
		ErrorHandler();
		return;
	}

	gControls->cw->compListBox.right -= kScrollBarWidth;
	instChoice = gControls->opt->instChoice-1;
	for (i=0; i<kMaxComponents; i++)
	{
		if (totalRows >= gControls->cfg->numComps)
			break;
		if (!gControls->cfg->comp[i].invisible && (gControls->cfg->st[instChoice].comp[i] == kInSetupType))
			totalRows++;
	}
		
	SetRect(&dataBounds, 0, 0, 1, totalRows);
	SetPt( &cSize, 0, 0);
	gControls->cw->compList = LNew((const Rect*)&gControls->cw->compListBox, (const Rect*)&dataBounds,
									cSize, rCheckboxLDEF, gWPtr, true, false, false, true);
	(*gControls->cw->compList)->selFlags = 68; /* NOTE: 64 (aka lExtendDrag) + 4 (aka lUseSense) = 68 */
	
	HLock((Handle)gControls->cw->compDescBox);
	SetRect(&viewRect, (*gControls->cw->compDescBox)->contrlRect.left,
					   (*gControls->cw->compDescBox)->contrlRect.top,
					   (*gControls->cw->compDescBox)->contrlRect.right,
					   (*gControls->cw->compDescBox)->contrlRect.bottom);
	HUnlock((Handle)gControls->cw->compDescBox);
	viewRect.top += kInterWidgetPad;
	SetRect(&viewRect, viewRect.left + kTxtRectPad,
						viewRect.top + kTxtRectPad,
						viewRect.right - kTxtRectPad,
						viewRect.bottom - kTxtRectPad);
	TextFont(applFont);
	TextSize(9);
	gControls->cw->compDescTxt = TENew(&viewRect, &viewRect);
	TextFont(systemFont);
	TextSize(12);
	
	// populate controls
	bCellSelected = PopulateCompInfo();
	
	// show controls
	GetIndString(compDescTitle, rStringList, sCompDescTitle);
	SetControlTitle(gControls->cw->compDescBox, compDescTitle);
	
	MoveTo( gControls->cw->compListBox.left, gControls->cw->compListBox.top - kInterWidgetPad);
	HLock(gControls->cfg->selCompMsg);
	selCompMsg = CToPascal(*gControls->cfg->selCompMsg);
	if (selCompMsg)
		DrawString( selCompMsg );
	HUnlock(gControls->cfg->selCompMsg);
	SetRect(&listBoxFrame, gControls->cw->compListBox.left, 
						 --gControls->cw->compListBox.top,
						   gControls->cw->compListBox.right += kScrollBarWidth,
						 ++gControls->cw->compListBox.bottom);
	FrameRect(&listBoxFrame);
	ShowNavButtons( back, next);
	if (bCellSelected)
		SetOptInfo(NULL);
	else
		DrawDiskSpaceMsgs( gControls->opt->vRefNum );

	// default highlight first row
	InitRowHighlight(0);
	
	//if (selCompMsg)
	//	DisposePtr((Ptr) selCompMsg);
	
	SetPort(oldPort);
}

Boolean
PopulateCompInfo(void)
{
	int 	i;
	char	*currDesc;
	Point	currCell;
	Boolean bCellSelected = false;
	int		nextRow = 0;
	
	for (i=0; i<gControls->cfg->numComps; i++)
	{
		if (gControls->cfg->comp[i].invisible == false)
		{
			HLock(gControls->cfg->comp[i].shortDesc);
			currDesc = *gControls->cfg->comp[i].shortDesc;
			SetPt(&currCell, 0, nextRow);
			rowToComp[nextRow++] = i;
			LSetCell( currDesc, strlen(currDesc), currCell, gControls->cw->compList);
			HUnlock(gControls->cfg->comp[i].shortDesc);
			if (gControls->cfg->comp[i].selected == true)
			{
				LSetSelect(true, currCell, gControls->cw->compList);
				bCellSelected = true;
			}
		}
	}

	numRows = nextRow;	
	return bCellSelected;
}

void		
UpdateCompWin(void)
{	
	Rect		r;
	Cell		c;
	int			i;
	GrafPtr		oldPort;
	GetPort(&oldPort);
	
	SetPort(gWPtr);
	
	MoveTo( gControls->cw->compListBox.left, gControls->cw->compListBox.top - kInterWidgetPad + 1);
	HLock(gControls->cfg->selCompMsg);
	DrawString( CToPascal(*gControls->cfg->selCompMsg));
	HUnlock(gControls->cfg->selCompMsg);
	LUpdate( (*gControls->cw->compList)->port->visRgn, gControls->cw->compList);
	FrameRect(&gControls->cw->compListBox);	
	
	SetPt(&c, 0, 0);
	if (LGetSelect(true, &c, gControls->cw->compList))
	{
		HLock((Handle)gControls->cw->compDescTxt);
		SetRect(&r, (*gControls->cw->compDescTxt)->viewRect.left,
					(*gControls->cw->compDescTxt)->viewRect.top,
					(*gControls->cw->compDescTxt)->viewRect.right,
					(*gControls->cw->compDescTxt)->viewRect.bottom);
		HUnlock((Handle)gControls->cw->compDescTxt);
		TEUpdate(&r, gControls->cw->compDescTxt);	
	}
	
	DrawDiskSpaceMsgs( gControls->opt->vRefNum );
	
	for (i = 0; i < numRows; i++)
	{
		if (gControls->cfg->comp[rowToComp[i]].highlighted)
		{
			InitRowHighlight(i);
			break;
		}
	}
	
	SetPort(oldPort);
}

void 
InComponentsContent(EventRecord* evt, WindowPtr wCurrPtr)
{	
	Point 			localPt;
	Rect			r, currCellRect, checkbox;
	ControlPartCode	part;
	int				i;
	Cell			currCell;
	UInt8			hiliteVal;
	PixPatHandle	ppH;
	GrafPtr			oldPort;
	GetPort(&oldPort);
	
	SetPort(wCurrPtr);
	localPt = evt->where;
	GlobalToLocal( &localPt);
	
	/* Mouse Up */
	if ((evt->what == mouseUp) && (PtInRect( localPt, &gControls->cw->compListBox)))
	{
		LClick(localPt, evt->modifiers, gControls->cw->compList);
		
		/* invert the checkbox rect */
		for (i=0; i<numRows; i++)
		{
			SetPt(&currCell, 0, i);
			LRect(&currCellRect, currCell, gControls->cw->compList);
			if (PtInRect(localPt, &currCellRect))
			{
				SetRect(&checkbox, currCellRect.left+4, currCellRect.top+2, 
							currCellRect.left+16, currCellRect.top+14);		
				INVERT_HIGHLIGHT(&checkbox);
				break;
			}
		}
		
		SetOptInfo(evt);
	}
	
	/* Mouse Down */
	if ((evt->what == mouseDown) && (PtInRect( localPt, &gControls->cw->compListBox)))
	{
		/* show depressed button state */
	    for (i=0; i<numRows; i++)
		{
			SetPt(&currCell, 0, i);
			LRect(&currCellRect, currCell, gControls->cw->compList);
			if (PtInRect(localPt, &currCellRect))
			{
				SetRect(&checkbox, currCellRect.left+4, currCellRect.top+2, 
							currCellRect.left+16, currCellRect.top+14);	
				ppH = GetPixPat(rGrayPixPattern);
				FillCRect(&checkbox, ppH);	
				FrameRect(&checkbox);	
				if (gControls->cfg->comp[rowToComp[i]].selected)
				{
					/* draw check mark */
					MoveTo(checkbox.left+1, checkbox.top+1);
					LineTo(checkbox.right-2, checkbox.bottom-2);
					MoveTo(checkbox.right-2, checkbox.top+1);
					LineTo(checkbox.left+1, checkbox.bottom-2); 
				}
				/* create 3D depression */
				
				MoveTo(checkbox.left+1, checkbox.top+1);
				LineTo(checkbox.left+1, checkbox.bottom-1);
				MoveTo(checkbox.left+1, checkbox.top+1);
				LineTo(checkbox.right-1, checkbox.top+1);
				
				ForeColor(whiteColor);
				
				MoveTo(checkbox.right-1, checkbox.top+1);
				LineTo(checkbox.right-1, checkbox.bottom-1);
				MoveTo(checkbox.left+1, checkbox.bottom-1);
				LineTo(checkbox.right-1, checkbox.bottom-1);
				
				ForeColor(blackColor);
			
				if (ppH)
					DisposePixPat(ppH);
				break;
			}
		}
	}
			
	HLock((Handle)gControls->backB);
	r = (**(gControls->backB)).contrlRect;
	HUnlock((Handle)gControls->backB);
	if (PtInRect( localPt, &r))
	{
		/* reset all rows to be not highlighted */
		for (i=1; i<numRows; i++)
			gControls->cfg->comp[rowToComp[i]].highlighted = false;
		
		part = TrackControl(gControls->backB, evt->where, NULL);
		if (part)
		{ 
			/* extra handling since we used DrawString for static text msg 
			 * and framed our own listbox etc. 
			 */
			gControls->cw->compListBox.top = 0;
			EraseRect(&gControls->cw->compListBox);
			ClearDiskSpaceMsgs();
			
			KillControls(gWPtr);
			ShowSetupTypeWin();
			return;
		}
	}
			
	HLock((Handle)gControls->nextB);			
	r = (**(gControls->nextB)).contrlRect;
	HUnlock((Handle)gControls->nextB);
	if (PtInRect( localPt, &r))
	{
		/* reset all rows to be not highlighted */
		for (i=0; i<numRows; i++)
			gControls->cfg->comp[rowToComp[i]].highlighted = false;
			
		part = TrackControl(gControls->nextB, evt->where, NULL);
		if (part)
		{	
			gControls->cw->compListBox.top = 0;
			EraseRect(&gControls->cw->compListBox);
			ClearDiskSpaceMsgs();
						
			KillControls(gWPtr);
			ShowTerminalWin();
			return;
		}
	}
	SetPort(oldPort);
}

void		
MouseMovedInComponentsWin(EventRecord *evt)
{
	Point 			localPt;
	GrafPtr			oldPort;
	GetPort(&oldPort);
	
	if (gWPtr)
		SetPort(gWPtr);
	
	localPt = evt->where;
	GlobalToLocal( &localPt );
	
	/* if within list box rect */
	if (PtInRect( localPt, &((*gControls->cw->compList)->rView) ))
	{		
		UpdateRowHighlight(localPt);
	}
	
	SetPort(oldPort);
}

void 
InitRowHighlight(int row)
{
	Cell hlCell;
	Rect hlCellRect;
	UInt8 hiliteVal;
	int i = 0;
	
	/* reset all highlighted markers */
	for (i=0; i<numRows; i++)
	{
		gControls->cfg->comp[rowToComp[i]].highlighted = false;
	}
	
	/* highlight and set marker for row to init */
	SetPt(&hlCell, 0, row);
	LRect(&hlCellRect, hlCell, gControls->cw->compList);
	INVERT_HIGHLIGHT(&hlCellRect);	
	UpdateLongDesc(row);
	gControls->cfg->comp[rowToComp[row]].highlighted = true; 
}

void
UpdateRowHighlight(Point localPt)
{
	int i, j;
	Rect currCellRect, oldCellRect;
	Cell currCell, oldCell;
	UInt8			hiliteVal;
	
	for (i=0; i<numRows; i++) 
	{
		/* note: numComps above includes invisible components */
		SetPt(&currCell, 0, i);
		LRect(&currCellRect, currCell, gControls->cw->compList);
			
		/* mouse move landed over this cell */
		if (PtInRect( localPt, &currCellRect ))
		{
			if (!gControls->cfg->comp[rowToComp[i]].highlighted)
			{	
				/* highlight this cell */
				INVERT_HIGHLIGHT(&currCellRect);	
				UpdateLongDesc(i);
					
				/* unhighlight old one */
				for (j=0; j<numRows; j++)
				{
					if (gControls->cfg->comp[rowToComp[j]].highlighted)
					{
						SetPt(&oldCell, 0, j);
						LRect(&oldCellRect, oldCell, gControls->cw->compList);
							
						INVERT_HIGHLIGHT(&oldCellRect);
						gControls->cfg->comp[rowToComp[j]].highlighted = false;
					}
				}
					
				/* mark this row highlighted to prevent incorrect inversion */
				gControls->cfg->comp[rowToComp[i]].highlighted = true; 
			}
		}
	}
}

void
UpdateDependencies(int row, EventRecord* evt)
{
	int 	i;
	short	currRow;
	Cell	currCell;
	Rect	currCellRect;
	Point	currCellPt;
	GrafPtr	oldPort;
	
	GetPort(&oldPort);
	if (gWPtr)
		SetPort(gWPtr);
		
	// if row is selected
	if (gControls->cfg->comp[rowToComp[row]].selected)
	{
		// loop through all components numComps
		for (i=0; i<gControls->cfg->numComps; i++)
		{
			// if kDependencyOn for curr dep component --> comp[rowToComp[row]].dep[i]
			if (gControls->cfg->comp[rowToComp[row]].dep[i] == kDependencyOn)
			{
				// if curr dep component isn't on
				if (gControls->cfg->comp[i].selected == kNotSelected)
				{
					// set curr dep comp to kSelected
					gControls->cfg->comp[i].selected = kSelected;
					
					// if curr dep comp is in currently displayed comps
					currRow = GetCompRow(i);
					if (currRow != kInvalidCompIdx)
					{
						// LClick the row to check curr dep comp's checkbox
						SetPt(&currCell, 0, currRow);
						LRect(&currCellRect, currCell, gControls->cw->compList);
						SetPt(&currCellPt, currCellRect.left+1, currCellRect.top+1);
						LClick(currCellPt, evt->modifiers, gControls->cw->compList);
					}
					
					// resolve its dependencies
					UpdateDependencies(currRow, evt);
				}
			}
		}
	}
	
	SetPort(oldPort);
}

short
GetCompRow(int compIdx)
{
	short i = kInvalidCompIdx;
	
	for (i=0; i<numRows; i++)
	{
		if (rowToComp[i] == compIdx)
			break;
	}
	
	return i;
}

void
SetOptInfo(EventRecord* evt)
{
	Boolean		isCellSelected;
	Cell		currCell;
	int			i;
	
	for(i=0; i<numRows; i++)
	{
		if (gControls->cfg->comp[rowToComp[i]].invisible)
			continue;
		SetPt(&currCell, 0, i);
		if ( (isCellSelected = LGetSelect( false, &currCell, gControls->cw->compList)) == true)
		{
			if (gControls->cfg->comp[rowToComp[i]].selected == false)
			{
				gControls->cfg->comp[rowToComp[i]].selected = true;
				gControls->opt->numCompSelected++;
				
				if (evt)
					UpdateDependencies(i, evt);
			}
			
			if (!gControls->cw->compDescBox)
			{
				ErrorHandler();
				return;
			}
		}
		else if (gControls->cfg->comp[rowToComp[i]].selected == true)
		{
			gControls->cfg->comp[rowToComp[i]].selected = false;
			gControls->opt->numCompSelected--;
		}
	}
	
	ClearDiskSpaceMsgs();
	DrawDiskSpaceMsgs( gControls->opt->vRefNum );
}

void
UpdateLongDesc(int row)
{
	Rect		viewRect;
	
	HLock((Handle)gControls->cw->compDescBox);
	SetRect(&viewRect, (*gControls->cw->compDescBox)->contrlRect.left,
					   (*gControls->cw->compDescBox)->contrlRect.top,
					   (*gControls->cw->compDescBox)->contrlRect.right,
					   (*gControls->cw->compDescBox)->contrlRect.bottom);
	HUnlock((Handle)gControls->cw->compDescBox);
	viewRect.top += kInterWidgetPad;
	SetRect(&viewRect, viewRect.left + kTxtRectPad,
						viewRect.top + kTxtRectPad,
						viewRect.right - kTxtRectPad,
						viewRect.bottom - kTxtRectPad);
	EraseRect(&viewRect);
	
	HLock(gControls->cfg->comp[rowToComp[row]].longDesc);
	TESetText( *gControls->cfg->comp[rowToComp[row]].longDesc, 
				strlen(*gControls->cfg->comp[rowToComp[row]].longDesc), gControls->cw->compDescTxt);
	TEUpdate( &viewRect, gControls->cw->compDescTxt);
	HUnlock(gControls->cfg->comp[rowToComp[row]].longDesc);
}

void
EnableComponentsWin(void)
{	
	EnableNavButtons();
	
	// TO DO
}


void
DisableComponentsWin(void)
{
	DisableNavButtons();
	
	// TO DO
}
