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

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Name:        ToolbarSeparator.h                                      //
//                                                                      //
// Description:	XFE_ToolbarSeparator class header.                      //
//              A toolbar item separator.                               //
//                                                                      //
// Author:		Ramiro Estrugo <ramiro@netscape.com>                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef _xfe_toolbar_separator_h_
#define _xfe_toolbar_separator_h_

#include "ToolbarItem.h"

class XFE_ToolbarSeparator : public XFE_ToolbarItem
{
public:

    XFE_ToolbarSeparator(XFE_Frame *	frame,
						 Widget			parent,
						 HT_Resource	htResource,
						 const String	name);
	
    virtual ~XFE_ToolbarSeparator();

	//////////////////////////////////////////////////////////////////////
	//                                                                  //
	// Initialize                                                       //
	//                                                                  //
	//////////////////////////////////////////////////////////////////////
 	virtual void	initialize			();

protected:

	//////////////////////////////////////////////////////////////////////
	//                                                                  //
	// Widget creation interface                                        //
	//                                                                  //
	//////////////////////////////////////////////////////////////////////
	virtual Widget	createBaseWidget	(Widget			parent,
										 const String	name);

	//////////////////////////////////////////////////////////////////////
	//                                                                  //
	// Configure                                                        //
	//                                                                  //
	//////////////////////////////////////////////////////////////////////
	virtual void	configure			();

	//////////////////////////////////////////////////////////////////////
	//                                                                  //
	// ToolTip interface                                                //
	//                                                                  //
	//////////////////////////////////////////////////////////////////////
 	virtual void	tipStringObtain		(XmString *		stringReturn,
										 Boolean *		needToFreeString);
	
 	virtual void	docStringObtain		(XmString *		stringReturn,
										 Boolean *		needToFreeString);
	
 	virtual void	docStringSet		(XmString		string);

 	virtual void	docStringClear		(XmString		string);

private:

};

#endif // _xfe_toolbar_separator_h_
