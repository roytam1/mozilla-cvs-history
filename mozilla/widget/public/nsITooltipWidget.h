/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsITooltipWidget_h__
#define nsITooltipWidget_h__

#include "nsIWidget.h"
#include "nsString.h"

#define NS_ITOOLTIPWIDGET_IID \
{ 0x2910c191, 0xe38a, 0x11d1, { 0x9e, 0xc1, 0x0, 0xaa, 0x0, 0x2f, 0xb8, 0x21 } };

/**
 *
 * Tool tip display widget
 * 
 */

class nsITooltipWidget : public nsISupports
{
};

#endif // nsITooltipWidget_h__

