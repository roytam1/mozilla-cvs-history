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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef __nsMsgSearchScopeTerm_h
#define __nsMsgSearchScopeTerm_h

#include "nsMsgSearchCore.h"
#include "nsMsgSearchArray.h"
#include "nsMsgSearchScopeTerm.h"
#include "nsIMsgSearchAdapter.h"
#include "nsIMsgFolder.h"
#include "nsIMsgSearchAdapter.h"
#include "nsFileStream.h"
#include "nsIMsgSearchSession.h"
#include "nsCOMPtr.h"
#include "nsIWeakReference.h"

class nsMsgSearchScopeTerm : public nsIMsgSearchScopeTerm
{
public:
  nsMsgSearchScopeTerm (nsIMsgSearchSession *, nsMsgSearchScopeValue, nsIMsgFolder *);
  nsMsgSearchScopeTerm ();
  virtual ~nsMsgSearchScopeTerm ();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMSGSEARCHSCOPETERM
    
  nsresult TimeSlice (PRBool *aDone);
  
  nsresult InitializeAdapter (nsISupportsArray *termList);
  
  char *GetStatusBarName ();
  
  nsMsgSearchScopeValue m_attribute;
  char *m_name;
  nsCOMPtr <nsIMsgFolder> m_folder;
  nsCOMPtr <nsIMsgSearchAdapter> m_adapter;
  nsWeakPtr m_searchSession;
  PRBool m_searchServer;
  
};

#endif
