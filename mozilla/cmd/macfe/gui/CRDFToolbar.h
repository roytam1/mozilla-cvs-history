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

#pragma once

#include "CDragBar.h"									// ...is a base class
#include "CRDFNotificationHandler.h"	// ...is a base class



class CRDFToolbar
		: public CDragBar,
			public CRDFNotificationHandler

		/*
			...

			A |CRDFToolbar| has a one-to-one relationship with a particular |HT_View|.  No more
			than one FE object should ever be instantiated for this |HT_View|.
		*/
	{
		public:
			CRDFToolbar( HT_View, LView* );
			virtual ~CRDFToolbar();

		private: // Pass-by-value is not allowed.  A single |CRDFToolbar| corresponds to a single on-screen object; copying doesn't make sense.
			CRDFToolbar( const CRDFToolbar& );						// DON'T IMPLEMENT
			CRDFToolbar& operator=( const CRDFToolbar& );	// DON'T IMPLEMENT


		public: // ...overriding the appropriate methods of |CRDFNotificationHandler|
			virtual void HandleNotification( HT_Notification, HT_Resource, HT_Event, void*, uint32 );

		private:
			HT_View _ht_view;
	};

