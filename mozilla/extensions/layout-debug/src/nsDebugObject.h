/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 */

#ifndef nsDebugObject_h__
#define nsDebugObject_h__

#include "nsCOMPtr.h"

#include "nsIFrameDebugObject.h"  
#include "nsILayoutDebugger.h"

class nsIDOMWindow;
class nsIPresShell;
class nsIDocShell;
class nsIDocShellTreeItem;

//*****************************************************************************
//***    nsDebugObject
//*****************************************************************************
class  nsDebugObject : public nsIFrameDebugObject
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFRAMEDEBUGOBJECT

  nsDebugObject();
  virtual ~nsDebugObject();

protected:
  
  nsresult    EnsureLayoutDebugger();
  nsresult    RefreshAllWindows();

  nsresult    GetDocShellFromWindow(nsIDOMWindow* inWindow, nsIDocShell** outShell);
  nsresult    GetPresShellFromWindow(nsIDOMWindow* inWindow, nsIPresShell** outShell);

  void        DumpAWebShell(nsIDocShellTreeItem* inShellItem, FILE* inDestFile, PRInt32 inIndent = 0);
  void        DumpMultipleWebShells(nsIDOMWindow* inWindow, FILE* inDestFile);
  void        DumpContentRecurse(nsIDocShell* inDocShell, FILE* inDestFile);
  void        DumpFramesRecurse(nsIDocShell* aDocShell, FILE* inDestFile);
  void        DumpViewsRecurse(nsIDocShell* aDocShell, FILE* inDestFile);
  
protected:

  nsCOMPtr<nsILayoutDebugger> mLayoutDebugger;
  
};



#endif /* nsDebugObject_h__ */
