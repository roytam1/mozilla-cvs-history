/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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

#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include "nsFont.h"
#include "nsFontUtils.h"
#include "nsToolkit.h"

#include "nsMacControl.h"
#include "nsColor.h"
#include "nsFontMetricsMac.h"

#include "nsIServiceManager.h"
#include "nsIPlatformCharset.h"

#include <ControlDefinitions.h>

#include <Appearance.h>
#include <TextUtils.h>
#include <UnicodeConverter.h>
#include <Fonts.h>

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
// TODO: leaks, need to release when unloading the dll
nsIUnicodeEncoder * nsMacControl::mUnicodeEncoder = nsnull;
nsIUnicodeDecoder * nsMacControl::mUnicodeDecoder = nsnull;


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
nsMacControl::nsMacControl()
{
	mValue			= 0;
	mMin			= 0;
	mMax			= 1;
	mWidgetArmed	= PR_FALSE;
	mMouseInButton	= PR_FALSE;

	mControl		= nsnull;
	mControlType	= pushButProc;

	mLastBounds.SetRect(0,0,0,0);
	mLastValue = 0;
	mLastHilite = 0;

	AcceptFocusOnClick(PR_FALSE);
}

/**-------------------------------------------------------------------------
 * See documentation in nsMacControl.h
 * @update -- 12/10/98 dwc
 */
NS_IMETHODIMP nsMacControl::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData) 
{
	Inherited::Create(aParent, aRect, aHandleEventFunction,
						aContext, aAppShell, aToolkit, aInitData);
  
	// create native control. mBounds has been set up at this point
	nsresult		theResult = CreateOrReplaceMacControl(mControlType);

	mLastBounds = mBounds;

	return theResult;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
nsMacControl::~nsMacControl()
{
	if (mControl)
	{
		Show(PR_FALSE);
		::DisposeControl(mControl);
		mControl = nsnull;
	}
}

#pragma mark -
//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
PRBool nsMacControl::OnPaint(nsPaintEvent &aEvent)
{
	if (mControl && mVisible)
	{
		// turn off drawing for setup to avoid ugliness
		Boolean		isVisible = IsControlVisible(mControl);
		::SetControlVisibility(mControl, false, false);

		// update title
		if (mLabel != mLastLabel)
		{
			NSStringSetControlTitle(mControl,mLabel);
		}

		// update bounds
		if (mBounds != mLastBounds)
		{
			nsRect ctlRect;
			GetRectForMacControl(ctlRect);
			Rect macRect;
			nsRectToMacRect(ctlRect, macRect);

			if ((mBounds.x != mLastBounds.x) || (mBounds.y != mLastBounds.y))
				::MoveControl(mControl, macRect.left, macRect.top);
			if ((mBounds.width != mLastBounds.width) || (mBounds.height != mLastBounds.height))
				::SizeControl(mControl, ctlRect.width, ctlRect.height);

			mLastBounds = mBounds;

			// Erase the widget rect (which can be larger than the control rect).
			// Note: this should paint the backgrount with the right color but
			// it doesn't work right now, see bug #5685 for more info.
			nsRect bounds = mBounds;
			bounds.x = bounds. y = 0;
			nsRectToMacRect(bounds, macRect);
			::EraseRect(&macRect);
		}

		// update value
		if (mValue != mLastValue)
		{
			mLastValue = mValue;
			if (nsToolkit::HasAppearanceManager())
				::SetControl32BitValue(mControl, mValue);
			else
				::SetControlValue(mControl, mValue);
		}

		// update hilite
		PRInt16 hilite;
		if (mEnabled)
			hilite = (mWidgetArmed && mMouseInButton ? 1 : 0);
		else
			hilite = kControlInactivePart;
		if (hilite != mLastHilite)
		{
			mLastHilite = hilite;
			::HiliteControl(mControl, hilite);
		}

		::SetControlVisibility(mControl, isVisible, false);

		// Draw the control
		::DrawOneControl(mControl);

		Rect macRect;
		nsRect bounds = mBounds;
		bounds.x = bounds. y = 0;
		nsRectToMacRect(bounds, macRect);
		::ValidWindowRect(mWindowPtr, &macRect);
	}
	return PR_FALSE;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
PRBool  nsMacControl::DispatchMouseEvent(nsMouseEvent &aEvent)
{
	PRBool eatEvent = PR_FALSE;
	switch (aEvent.message)
	{
		case NS_MOUSE_LEFT_DOUBLECLICK:
		case NS_MOUSE_LEFT_BUTTON_DOWN:
			if (mEnabled)
			{
				mMouseInButton = PR_TRUE;
				mWidgetArmed = PR_TRUE;
				Invalidate(PR_TRUE);
			}
			break;

		case NS_MOUSE_LEFT_BUTTON_UP:
			// if the widget was not armed, eat the event
			if (!mWidgetArmed)
				eatEvent = PR_TRUE;
			// if the mouseUp happened on another widget, eat the event too
			// (the widget which got the mouseDown is always notified of the mouseUp)
			if (! mMouseInButton)
				eatEvent = PR_TRUE;
			mWidgetArmed = PR_FALSE;
			if (mMouseInButton)
				Invalidate(PR_TRUE);
			break;

		case NS_MOUSE_EXIT:
			mMouseInButton = PR_FALSE;
			if (mWidgetArmed)
				Invalidate(PR_TRUE);
			break;

		case NS_MOUSE_ENTER:
			mMouseInButton = PR_TRUE;
			if (mWidgetArmed)
				Invalidate(PR_TRUE);
			break;
	}
	if (eatEvent)
		return PR_TRUE;
	return (Inherited::DispatchMouseEvent(aEvent));
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void  nsMacControl::ControlChanged(PRInt32 aNewValue)
{
	if (aNewValue != mValue)
	{
		mValue = aNewValue;
		mLastValue = mValue;	// safely assume that the control has been repainted already

		nsGUIEvent guiEvent(NS_CONTROL_CHANGE, this);
 		guiEvent.time	 	= PR_IntervalNow();
		Inherited::DispatchWindowEvent(guiEvent);
	}
}


#pragma mark -
//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsMacControl::Enable(PRBool bState)
{
  PRBool priorState = mEnabled;
  Inherited::Enable(bState);
  if ( priorState != bState )
    Invalidate(PR_FALSE);
  return NS_OK;
}
//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsMacControl::Show(PRBool bState)
{
  Inherited::Show(bState);
  if (mControl)
  {
  	::SetControlVisibility(mControl, bState, false);		// don't redraw
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set this control font
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsMacControl::SetFont(const nsFont &aFont)
{
	Inherited::SetFont(aFont);	

	SetupMacControlFont();
	
 	return NS_OK;
}

#pragma mark -

//-------------------------------------------------------------------------
//
// Get the rect which the Mac control uses. This may be different for
// different controls, so this method allows overriding
//
//-------------------------------------------------------------------------
void nsMacControl::GetRectForMacControl(nsRect &outRect)
{
		outRect = mBounds;
		outRect.x = outRect.y = 0;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------

NS_METHOD nsMacControl::CreateOrReplaceMacControl(short inControlType)
{
	nsRect		controlRect;
	GetRectForMacControl(controlRect);
	Rect macRect;
	nsRectToMacRect(controlRect, macRect);

	if(nsnull != mWindowPtr)
	{
		if (mControl)
			::DisposeControl(mControl);

		StartDraw();
		mControl = ::NewControl(mWindowPtr, &macRect, "\p", mVisible, mValue, mMin, mMax, inControlType, nil);
  		EndDraw();
		
		// need to reset the font now
		// XXX to do: transfer the text in the old control over too
		if (mControl && mFontMetrics)
		{
			SetupMacControlFont();
		}
	}

	return (mControl) ? NS_OK : NS_ERROR_NULL_POINTER;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void nsMacControl::SetupMacControlFont()
{
	NS_PRECONDITION(mFontMetrics != nsnull, "No font metrics in SetupMacControlFont");
	NS_PRECONDITION(mContext != nsnull, "No context metrics in SetupMacControlFont");
	
	TextStyle		theStyle;
	// if needed, impose a min size of 9pt on the control font
	if (theStyle.tsSize < 9)
		theStyle.tsSize = 9;
	
	ControlFontStyleRec fontStyleRec;
	fontStyleRec.flags = (kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask);
	fontStyleRec.font = theStyle.tsFont;
	fontStyleRec.size = theStyle.tsSize;
	fontStyleRec.style = theStyle.tsFace;
	::SetControlFontStyle(mControl, &fontStyleRec);
}

#pragma mark -

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------

void nsMacControl::StringToStr255(const nsAString& aText, Str255& aStr255)
{
	nsresult rv = NS_OK;
	nsAString::const_iterator begin;
	const PRUnichar *text = aText.BeginReading(begin).get();

	// get file system charset and create a unicode encoder
	if (nsnull == mUnicodeEncoder) {
		nsCAutoString fileSystemCharset;
		GetFileSystemCharset(fileSystemCharset);

		nsCOMPtr<nsICharsetConverterManager> ccm = 
		         do_GetService(kCharsetConverterManagerCID, &rv); 
		if (NS_SUCCEEDED(rv)) {
			rv = ccm->GetUnicodeEncoderRaw(fileSystemCharset.get(), &mUnicodeEncoder);
            if (NS_SUCCEEDED(rv)) {
              rv = mUnicodeEncoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, (PRUnichar)'?');
            }
		}
	}

	// converts from unicode to the file system charset
	if (NS_SUCCEEDED(rv)) {
		PRInt32 inLength = aText.Length();
		PRInt32 outLength = 255;
		rv = mUnicodeEncoder->Convert(text, &inLength, (char *) &aStr255[1], &outLength);
		if (NS_SUCCEEDED(rv))
			aStr255[0] = outLength;
	}

	if (NS_FAILED(rv)) {
//		NS_ASSERTION(0, "error: charset covnersion");
		NS_LossyConvertUCS2toASCII buffer(Substring(aText,0,254));
		PRInt32 len = buffer.Length();
		memcpy(&aStr255[1], buffer.get(), len);
		aStr255[0] = len;
	}
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------

void nsMacControl::Str255ToString(const Str255& aStr255, nsString& aText)
{
	nsresult rv = NS_OK;
	
	// get file system charset and create a unicode encoder
	if (nsnull == mUnicodeDecoder) {
		nsCAutoString fileSystemCharset;
		GetFileSystemCharset(fileSystemCharset);

		nsCOMPtr<nsICharsetConverterManager> ccm = 
		         do_GetService(kCharsetConverterManagerCID, &rv); 
		if (NS_SUCCEEDED(rv)) {
			rv = ccm->GetUnicodeDecoderRaw(fileSystemCharset.get(), &mUnicodeDecoder);
		}
	}
  
	// converts from the file system charset to unicode
	if (NS_SUCCEEDED(rv)) {
		PRUnichar buffer[512];
		PRInt32 inLength = aStr255[0];
		PRInt32 outLength = 512;
		rv = mUnicodeDecoder->Convert((char *) &aStr255[1], &inLength, buffer, &outLength);
		if (NS_SUCCEEDED(rv)) {
			aText.Assign(buffer, outLength);
		}
	}
	
	if (NS_FAILED(rv)) {
//		NS_ASSERTION(0, "error: charset covnersion");
		aText.AssignWithConversion((char *) &aStr255[1], aStr255[0]);
	}
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------

void nsMacControl::NSStringSetControlTitle(ControlHandle theControl, nsString title)
{	
  // wow, it sure is nice being able to use core foundation ;)
  CFStringRef str = CFStringCreateWithCharacters(NULL, (const UniChar*)title.get(), title.Length());
  SetControlTitleWithCFString(theControl, str);
  CFRelease(str);
}
//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void nsMacControl::SetupMacControlFontForScript(short theScript)
{
	short					themeFontID;
	Str255					themeFontName;
	SInt16					themeFontSize;
	Style					themeFontStyle;
	TextStyle				theStyle;
	OSErr					err;

	NS_PRECONDITION(mFontMetrics != nsnull, "No font metrics in SetupMacControlFont");
	NS_PRECONDITION(mContext != nsnull, "No context metrics in SetupMacControlFont");

	//
	// take the script and select and override font
	//
	err = ::GetThemeFont(kThemeSystemFont,theScript,themeFontName,&themeFontSize,&themeFontStyle);
	NS_ASSERTION(err==noErr,"nsMenu::NSStringNewMenu: GetThemeFont failed.");
	::GetFNum(themeFontName,&themeFontID);
	
	// if needed, impose a min size of 9pt on the control font
	if (theStyle.tsSize < 9)
		theStyle.tsSize = 9;
	
	ControlFontStyleRec fontStyleRec;
	fontStyleRec.flags = (kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask);
	fontStyleRec.font = themeFontID;
	fontStyleRec.size = theStyle.tsSize;
	fontStyleRec.style = theStyle.tsFace;
	::SetControlFontStyle(mControl, &fontStyleRec);
}
//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void nsMacControl::GetFileSystemCharset(nsCString & fileSystemCharset)
{
  static nsCAutoString aCharset;
  nsresult rv;

  if (aCharset.IsEmpty()) {
    nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
	  if (NS_SUCCEEDED(rv)) 
		  rv = platformCharset->GetCharset(kPlatformCharsetSel_FileName, aCharset);

    NS_ASSERTION(NS_SUCCEEDED(rv), "error getting platform charset");
	  if (NS_FAILED(rv)) 
		  aCharset.AssignLiteral("x-mac-roman");
  }
  fileSystemCharset = aCharset;
}
	
