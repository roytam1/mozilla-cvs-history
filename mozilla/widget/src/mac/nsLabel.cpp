/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsLabel.h"

NS_IMPL_ADDREF(nsLabel);
NS_IMPL_RELEASE(nsLabel);

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
nsLabel::nsLabel() : nsTextWidget(), nsILabel()
{
  NS_INIT_REFCNT();
  WIDGET_SET_CLASSNAME("nsLabel");
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
nsLabel::~nsLabel()
{
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
NS_INTERFACE_MAP_BEGIN(nsLabel)
  NS_INTERFACE_MAP_ENTRY(nsILabel)
NS_INTERFACE_MAP_END_INHERITING(nsTextWidget)

#pragma mark -
//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
NS_METHOD nsLabel::SetLabel(const nsString& aText)
{
	PRUint32 displayedSize;
	return SetText(aText, displayedSize);
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
NS_METHOD nsLabel::GetLabel(nsString& aBuffer)
{
	PRUint32 maxSize = -1;
	PRUint32 displayedSize;
	return GetText(aBuffer, maxSize, displayedSize);
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
NS_METHOD nsLabel::SetAlignment(nsLabelAlignment aAlignment)
{
	if (!mControl)
		return NS_ERROR_NOT_INITIALIZED;

	SInt16	just;
	switch (aAlignment)
	{
		case eAlign_Right:		just = teFlushRight;		break;
		case eAlign_Left:			just = teFlushLeft;			break;
		case eAlign_Center:		just = teCenter;				break;
	}
	ControlFontStyleRec fontStyleRec;
	fontStyleRec.flags = (kControlUseJustMask);
	fontStyleRec.just = just;
	::SetControlFontStyle(mControl, &fontStyleRec);
	
	return NS_OK;
}

