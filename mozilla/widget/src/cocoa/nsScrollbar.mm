/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsScrollbar.h"
#include "nsIDeviceContext.h"
#if TARGET_CARBON || (UNIVERSAL_INTERFACES_VERSION >= 0x0330)
#include <ControlDefinitions.h>
#endif

NS_IMPL_ISUPPORTS_INHERITED1(nsScrollbar, nsChildView, nsIScrollbar);


ControlActionUPP nsScrollbar::sControlActionProc = nsnull;


nsScrollbar::nsScrollbar()
	:	Inherited()
	, mValue(0)
	,	mFullImageSize(0)
	,	mVisibleImageSize(0)
	,	mLineIncrement(0)
	,	mMouseDownInScroll(PR_FALSE)
	,	mClickedPartCode(0)
	, mParentView(nsnull)
{
	WIDGET_SET_CLASSNAME("nsScrollbar");
#if 0
	SetControlType(kControlScrollBarLiveProc);
	if (!sControlActionProc)
		sControlActionProc = NewControlActionUPP(nsScrollbar::ScrollActionProc);
	// Unfortunately, not disposed when the app quits, but that's still a non-issue.
#endif
}


nsScrollbar::~nsScrollbar()
{
  printf("-- scrollbar going away!\n");

}


#if 0
/**-------------------------------------------------------------------------------
 * The create method for a scrollbar, using a nsIWidget as the parent
 * @update  dc 08/31/98
 * @param  aParent -- the widget which will be this widgets parent in the tree
 * @param  aRect -- The bounds in parental coordinates of this widget
 * @param  aHandleEventFunction -- Procedures to be executed for this widget
 * @param  aContext -- device context to be used by this widget
 * @param  aAppShell -- 
 * @param  aToolkit -- toolkit to be used by this widget
 * @param  aInitData -- Initialization data used by frames
 * @return -- NS_OK if everything was created correctly
 */ 
NS_IMETHODIMP nsScrollbar::Create(nsIWidget *aParent,
								const nsRect &aRect,
								EVENT_CALLBACK aHandleEventFunction,
								nsIDeviceContext *aContext,
								nsIAppShell *aAppShell,
								nsIToolkit *aToolkit,
								nsWidgetInitData *aInitData)
{
//FIXME - probably don't need this at all
	Inherited::Create(aParent, aRect, aHandleEventFunction,
						aContext, aAppShell, aToolkit, aInitData);

	return NS_OK;
}
#endif


NSView*
nsScrollbar::CreateCocoaView ( )
{
printf("++ creating native scrollbar\n");
  // Cocoa sets the orientation of a scrollbar at creation time by looking
  // at its frame and taking the longer side to be the orientation. Since
  // chances are good at this point gecko just wants us to be 1x1, use
  // the flag at creation to force the desired orientation.
  NSRect orientation;
  orientation.origin.x = orientation.origin.y = 0;
  if ( IsVertical() ) {
    orientation.size.width = 20;
    orientation.size.height = 100;
  }
  else {
    orientation.size.width = 100;
    orientation.size.height = 20;
  }
  return [[[NSScroller alloc] initWithFrame:orientation] autorelease];
}


GrafPtr
nsScrollbar::GetQuickDrawPort ( )
{
  // pray we're always a child of a NSQuickDrawView
  NSQuickDrawView* parent = (NSQuickDrawView*)[mView superview];
  return [parent qdPort];
}



/**-------------------------------------------------------------------------------
 * ScrollActionProc Callback for TrackControl
 * @update	jrm 99/01/11
 * @param ctrl - The Control being tracked
 * @param part - Part of the control (arrow, thumb, gutter) being hit
 */
pascal 
void nsScrollbar::ScrollActionProc(ControlHandle ctrl, ControlPartCode part)
{
	nsScrollbar* me = (nsScrollbar*)(::GetControlReference(ctrl));
	NS_ASSERTION(nsnull != me, "NULL nsScrollbar");
	if (nsnull != me)
		me->DoScrollAction(part);
}

/**-------------------------------------------------------------------------------
 * ScrollActionProc Callback for TrackControl
 * @update	jrm 99/01/11
 * @param part - Part of the control (arrow, thumb, gutter) being hit
 */
void nsScrollbar::DoScrollAction(ControlPartCode part)
{
	PRUint32 pos;
	PRUint32 incr;
	PRUint32 visibleImageSize;
	PRInt32 scrollBarMessage = 0;
	GetPosition(pos);
	GetLineIncrement(incr);
	GetThumbSize(visibleImageSize);
	switch(part)
	{
		case kControlUpButtonPart:
		{
			scrollBarMessage = NS_SCROLLBAR_LINE_PREV;
			SetPosition(pos - incr);
			break;
		}
		case kControlDownButtonPart:
			scrollBarMessage = NS_SCROLLBAR_LINE_NEXT;
			SetPosition(pos + incr);
			break;
		case kControlPageUpPart:
			scrollBarMessage = NS_SCROLLBAR_PAGE_PREV;
			SetPosition(pos - visibleImageSize);
			break;
		case kControlPageDownPart:
			scrollBarMessage = NS_SCROLLBAR_PAGE_NEXT;
			SetPosition(pos + visibleImageSize);
			break;
		case kControlIndicatorPart:
			scrollBarMessage = NS_SCROLLBAR_POS;
			SetPosition([mView intValue]);
			break;
	}
	EndDraw();
	
	// send event to scroll the parent
	nsScrollbarEvent scrollBarEvent;
	scrollBarEvent.eventStructType = NS_GUI_EVENT;
	scrollBarEvent.widget = this;
	scrollBarEvent.message = scrollBarMessage;
	GetPosition(pos);
	scrollBarEvent.position = pos;
	Inherited::DispatchWindowEvent(scrollBarEvent);

	// update the area of the parent uncovered by the scrolling
	nsIWidget* parent = GetParent();
	parent->Update();
	NS_RELEASE(parent);

	// update this scrollbar
	Invalidate(PR_FALSE);
	Update();

	StartDraw();
}

/**-------------------------------------------------------------------------------
 * DispatchMouseEvent handle an event for this scrollbar
 * @update  dc 08/31/98
 * @Param aEvent -- The mouse event to respond to for this button
 * @return -- True if the event was handled, PR_FALSE if we did not handle it.
 */ 
PRBool nsScrollbar::DispatchMouseEvent(nsMouseEvent &aEvent)
{
//FIXME!!!!
  return PR_TRUE;
  
#if 0
	PRBool eatEvent = PR_FALSE;
	switch (aEvent.message)
	{
		case NS_MOUSE_LEFT_DOUBLECLICK:
		case NS_MOUSE_LEFT_BUTTON_DOWN:
			NS_ASSERTION(this != 0, "NULL nsScrollbar2");
//			::SetControlReference(mControl, (UInt32) this);
			StartDraw();
			{
				Point thePoint;
				thePoint.h = aEvent.point.x;
				thePoint.v = aEvent.point.y;
				mClickedPartCode = ::TestControl(mControl, thePoint);
				if (mClickedPartCode > 0)
					::HiliteControl(mControl, mClickedPartCode);

				switch (mClickedPartCode)
				{
					case kControlUpButtonPart:
					case kControlDownButtonPart:
					case kControlPageUpPart:
					case kControlPageDownPart:
					case kControlIndicatorPart:
						// We are assuming Appearance 1.1 or later, so we
						// have the "live scroll" variant of the scrollbar,
						// which lets you pass the action proc to TrackControl
						// for the thumb (this was illegal in previous
						// versions of the defproc).
						::TrackControl(mControl, thePoint, sControlActionProc);
						// We don't dispatch the mouseDown event because mouseUp is eaten
						// by TrackControl anyway and the only messages the app really
						// cares about are the NS_SCROLLBAR_xxx messages.
						eatEvent = PR_TRUE;
						break;
#if 0
					case kControlIndicatorPart:
						// This is what you have to do for appearance 1.0 or
						// no appearance.
						::TrackControl(mControl, thePoint, nsnull);
						mValue = ::GetControl32BitValue(mControl);
						EndDraw();
						nsScrollbarEvent scrollBarEvent;
						scrollBarEvent.eventStructType = NS_GUI_EVENT;
						scrollBarEvent.widget = this;
						scrollBarEvent.message = NS_SCROLLBAR_POS;
						scrollBarEvent.position = mValue;
						DispatchWindowEvent(scrollBarEvent);
						nsIWidget* parent = GetParent();
						parent->Update();
						NS_RELEASE(parent);
						StartDraw();
						break;
#endif
				}
				SetPosition(mValue);
			}
			EndDraw();
			break;


		case NS_MOUSE_LEFT_BUTTON_UP:
			mClickedPartCode = 0;
			break;

		case NS_MOUSE_EXIT:
			if (mWidgetArmed)
			{
				StartDraw();
				::HiliteControl(mControl, 0);
				EndDraw();
			}
			break;

		case NS_MOUSE_ENTER:
			if (mWidgetArmed)
			{
				StartDraw();
				::HiliteControl(mControl, mClickedPartCode);
				EndDraw();
			}
			break;
	}

	if (eatEvent)
		return PR_TRUE;
	return (Inherited::DispatchMouseEvent(aEvent));
#endif
}

/**-------------------------------------------------------------------------------
 *	set the maximum range of a scroll bar
 *	@update	dc 09/16/98
 *	@param	aMaxRange -- the maximum to set this to
 *	@return -- If a good size was returned
 */
NS_METHOD nsScrollbar::SetMaxRange(PRUint32 aEndRange) // really means set full image size.
{
	mFullImageSize = ((int)aEndRange) > 0 ? aEndRange : 10;

//printf("** max range is %d\n", aEndRange);
  //StartDraw();
  [mView setFloatValue:[mView floatValue] knobProportion:(mVisibleImageSize / (float)mFullImageSize)];
  //EndDraw();

	return NS_OK;
}

/**-------------------------------------------------------------------------------
 *	get the maximum range of a scroll bar
 *	@update	dc 09/16/98
 *	@param	aMaxRange -- The current maximum this slider can be
 *	@return -- If a good size was returned
 */
NS_METHOD nsScrollbar::GetMaxRange(PRUint32& aMaxRange) // really means get full image size
{
	aMaxRange = mFullImageSize;
	return NS_OK;
}

/**-------------------------------------------------------------------------------
 *	Set the current position of the slider
 *	@update	dc 09/16/98
 *	@param	aMaxRange -- The current value to set the slider position to.
 *	@return -- NS_OK if the position is valid
 */
NS_METHOD nsScrollbar::SetPosition(PRUint32 aPos)
{
	if ((PRInt32)aPos < 0)
		aPos = 0;
	
	PRUint32 oldValue = mValue;
	mValue = ((PRInt32)aPos) > mFullImageSize ? mFullImageSize : ((int)aPos);

//printf("** set value to %d\n", aPos);
  [mView setFloatValue:(aPos / (float)mFullImageSize)];
  	
	return NS_OK;
}


/**-------------------------------------------------------------------------------
 *	Get the current position of the slider
 *	@update	dc 09/16/98
 *	@param	aMaxRange -- The current slider position.
 *	@return -- NS_OK if the position is valid
 */
NS_METHOD nsScrollbar::GetPosition(PRUint32& aPos)
{
	aPos = mValue;
	return NS_OK;
}

/**-------------------------------------------------------------------------------
 *	Set the height of a vertical, or width of a horizontal scroll bar thumb control
 *	@update	dc 09/16/98
 *	@param	aSize -- the size to set the thumb control to
 *	@return -- NS_OK if the position is valid
 */
NS_METHOD nsScrollbar::SetThumbSize(PRUint32 aSize)
{
	mVisibleImageSize = aSize;

	if (mVisibleImageSize > mFullImageSize)
		mVisibleImageSize = mFullImageSize;

//printf("** visible image size is %d\n", mVisibleImageSize);	
//printf("** new knob proportion %f\n", (mVisibleImageSize / (float)mFullImageSize));
  //StartDraw();
  [mView setFloatValue:[mView floatValue] knobProportion:(mVisibleImageSize / (float)mFullImageSize)];
  //EndDraw();

	return NS_OK;
}

/**-------------------------------------------------------------------------------
 *	get the height of a vertical, or width of a horizontal scroll bar thumb control
 *	@update	dc 09/16/98
 *	@param	aSize -- the size to set the thumb control to
 *	@return -- NS_OK if the position is valid
 */
NS_METHOD nsScrollbar::GetThumbSize(PRUint32& aSize)
{
	aSize = mVisibleImageSize;
	return NS_OK;
}

/**-------------------------------------------------------------------------------
 *	Set the increment of the scroll bar
 *	@update	dc 09/16/98
 *	@param	aLineIncrement -- the control increment
 *	@return -- NS_OK if the position is valid
 */
NS_METHOD nsScrollbar::SetLineIncrement(PRUint32 aLineIncrement)
{
	mLineIncrement	= (((int)aLineIncrement) > 0 ? aLineIncrement : 1);
	return NS_OK;
}


/**-------------------------------------------------------------------------------
 *	Get the increment of the scroll bar
 *	@update	dc 09/16/98
 *	@param aLineIncrement -- the control increment
 *	@return NS_OK if the position is valid
 */
NS_METHOD nsScrollbar::GetLineIncrement(PRUint32& aLineIncrement)
{
	aLineIncrement = mLineIncrement;
	return NS_OK;
}

/**-------------------------------------------------------------------------------
 *	See documentation in nsScrollbar.h
 *	@update	dc 012/10/98
 */
NS_METHOD nsScrollbar::SetParameters(PRUint32 aMaxRange, PRUint32 aThumbSize,
								PRUint32 aPosition, PRUint32 aLineIncrement)
{
	SetLineIncrement(aLineIncrement);
	SetPosition(aPosition);
	mVisibleImageSize = aThumbSize; // needed by SetMaxRange
	SetMaxRange(aMaxRange);
	SetThumbSize(aThumbSize); // Needs to know the maximum value when calling Mac toolbox.

	return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the rect which the Mac control uses. This may be different for
// different controls, so this method allows overriding
//
//-------------------------------------------------------------------------
void nsScrollbar::GetRectForMacControl(nsRect &outRect)
{
		outRect = mBounds;
		outRect.x = outRect.y = 0;
		
		if (mBounds.height > mBounds.width)
		{
			// vertical scroll bar
			outRect.Inflate(0, 1);
		}
		else
		{
			// horizontal scroll bar
			outRect.Inflate(1, 0);
		}
}


//
// Show
//
// Hide or show the scrollbar
//
NS_IMETHODIMP
nsScrollbar::Show(PRBool bState)
{
  // the only way to get the scrollbar view to not draw is to remove it
  // from the view hierarchy. cache the parent view so that we can
  // hook it up later if we're told to show.
  if ( mVisible && !bState ) {
    mParentView = [mView superview];
    [mView removeFromSuperview];
  }
  else if ( !mVisible && bState ) {
    if ( mParentView )
      [mParentView addSubview:mView];
  }
    
  return Inherited::Show(bState);
}


//-------------------------------------------------------------------------
//
// Enable/disable this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP
nsScrollbar::Enable(PRBool bState)
{
  [mView setEnabled:(bState ? YES : NO)];
  return NS_OK;
}


#pragma mark -

@implementation ScrollbarView


- (void)trackKnob:(NSEvent *)theEvent
{
  printf("tracking knob\n");
  [super trackKnob:theEvent];
  printf("done tracking knob\n");
}

- (void)trackScrollButtons:(NSEvent *)theEvent
{
  printf("tracking buttons");
  [super trackScrollButtons:theEvent];
  printf("done tracking buttons");
}


- (BOOL)isFlipped
{
  return YES;
}

- (void)mouseDown:(NSEvent *)theEvent
{
  printf("mouse down in scrollbar\n");
  [super mouseDown:theEvent];
}


@end
