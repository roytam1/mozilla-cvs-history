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

#include "msg.h"

#include "subline.h"

const uint16 F_ISSUBSCRIBED	= 0x0001;


msg_SubscribeLine::msg_SubscribeLine(msg_GroupRecord* group, int16 depth,
									 XP_Bool issubscribed,
									 int32 numsubgroups)
{
	m_group = group;
	m_nummessages = -1;
	m_depth = depth;
	m_flags = 0;
	m_numsubgroups = numsubgroups;
	if (issubscribed) m_flags |= F_ISSUBSCRIBED;
}


XP_Bool
msg_SubscribeLine::CanExpand()
{
	return m_numsubgroups > 0;
}


XP_Bool
msg_SubscribeLine::GetSubscribed()
{
	return (m_flags & F_ISSUBSCRIBED) != 0;
}


void
msg_SubscribeLine::SetSubscribed(XP_Bool value)
{
	if (value) m_flags |= F_ISSUBSCRIBED;
	else m_flags &= ~F_ISSUBSCRIBED;
}




msg_SubscribeLineArray::msg_SubscribeLineArray()
{
}
