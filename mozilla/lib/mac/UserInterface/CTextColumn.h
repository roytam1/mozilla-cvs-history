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

// This class is a lightweight override of PowerPlant's LTextColumn
// to provide instrumentation through QA-Partner.
//
// It should be used everywhere in place of LTextColumn.


#ifndef CTextColumn_H
#define CTextColumn_H
#pragma once

#include <LTextColumn.h>
//#include <QAP_Assist.h>

class	CTextColumn : public LTextColumn
//					, public CQAPartnerTableMixin
{
private:

	typedef	LTextColumn super;

public:
	enum 				{ class_ID = 'TxCl' };
	
	
						CTextColumn(LStream* inStream);
	virtual				~CTextColumn();

#if defined(QAP_BUILD)
		virtual void	QapGetListInfo (PQAPLISTINFO pInfo);
		virtual Ptr		QapAddCellToBuf(Ptr pBuf, Ptr pLimit, const STableCell& sTblCell);
#endif
};

#endif