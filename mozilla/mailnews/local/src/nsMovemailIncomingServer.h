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
 * Adam D. Moss <adam@gimp.org>
 */

#ifndef __nsMovemailIncomingServer_h
#define __nsMovemailIncomingServer_h

#include "msgCore.h"
#include "nsIMovemailIncomingServer.h"
#include "nsILocalMailIncomingServer.h"
#include "nsMsgIncomingServer.h"

/* get some implementation from nsMsgIncomingServer */
class nsMovemailIncomingServer : public nsMsgIncomingServer,
                                 public nsIMovemailIncomingServer,
                                 public nsILocalMailIncomingServer

{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIMOVEMAILINCOMINGSERVER
    NS_DECL_NSILOCALMAILINCOMINGSERVER

    nsMovemailIncomingServer();
    virtual ~nsMovemailIncomingServer();
    
    NS_IMETHOD GetLocalStoreType(char * *type);
    NS_IMETHOD GetCanSearchMessages(PRBool *canSearchMessages);
};


#endif
