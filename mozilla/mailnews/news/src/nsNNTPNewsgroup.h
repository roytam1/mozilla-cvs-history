/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef __nsNNTPNewsgroup_h
#define __nsNNTPNewsgroup_h

#include "nsMsgKeySet.h"
#include "nsINNTPHost.h"
#include "nsINNTPNewsgroup.h"
#include "nsINNTPNewsgroupList.h"
#include "nsCOMPtr.h"

class nsNNTPNewsgroup : public nsINNTPNewsgroup 
{
public: 
	 nsNNTPNewsgroup();
	 virtual ~nsNNTPNewsgroup();
	 
	 NS_DECL_ISUPPORTS
     NS_DECL_NSINNTPNEWSGROUP
protected:
	 char * m_groupName;
	 char * m_prettyName;
	 
	 PRBool	m_isSubscribed;
	 PRBool m_wantsNewTotals;
	 PRBool m_needsExtraInfo;
	 PRBool m_category;

	 nsCOMPtr<nsINNTPNewsgroupList> m_newsgroupList;
};

#endif /* __nsNNTPNewsgroup_h */
