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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

#ifndef nsIScrollbarMediator_h___
#define nsIScrollbarMediator_h___

// {351C003A-E8F7-4d10-8BFA-635C9860D650}
#define NS_ISCROLLBARMEDIATOR_IID \
{ 0x351c003a, 0xe8f7, 0x4d10, { 0x8b, 0xfa, 0x63, 0x5c, 0x98, 0x60, 0xd6, 0x50 } }

static NS_DEFINE_IID(kIScrollbarMediatorIID,     NS_ISCROLLBARMEDIATOR_IID);

class nsIScrollbarMediator : public nsISupports {

public:
  static const nsIID& GetIID() { static nsIID iid = NS_ISCROLLBARMEDIATOR_IID; return iid; }
  
  NS_IMETHOD PositionChanged(PRInt32 aOldIndex, PRInt32& aNewIndex) = 0;
  NS_IMETHOD ScrollbarButtonPressed(PRInt32 aOldIndex, PRInt32 aNewIndex) = 0;

  NS_IMETHOD VisibilityChanged(PRBool aVisible) = 0;
};

#endif

