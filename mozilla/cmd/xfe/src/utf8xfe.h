/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
/* 
   utf8xfe.h UTF8 to XmString conversion and XmFontList monitor stuff
   we need for RDF related UI
   Created: Frank Tang tftang@netscape.com>, 30-Sep-98
 */
#ifndef __utfxfe_h__
#define __utfxfe_h__
#include "mozilla.h"
#include "xfe.h"
#include "fonts.h"

class  FontListNotifier {
public:
	virtual void notifyFontListChanged(XmFontList inFontList) = 0;
};

class UTF8ToXmStringConverter
{
public:
	virtual XmString convertToXmString(const char* utf8text) = 0;
};

class UTF8ToXmStringConverterFactory {
public:	
	static UTF8ToXmStringConverter* make(
		FontListNotifier* inNotifier, fe_Font inUFont);
};

class UnicodeFontSingleton 
{
public:
	static	fe_Font Instance(Display *dpy, char* family, int pt);
private:
	static  fe_Font gUFont;
};


#endif /* __utfxfe_h__ */
