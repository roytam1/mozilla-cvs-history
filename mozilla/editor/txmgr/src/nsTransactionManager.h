/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://wwwt.mozilla.org/NPL/
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

#ifndef nsTransactionManager_h__
#define nsTransactionManager_h__

#include "nsITransactionManager.h"

/** implementation of a transaction manager object.
 *
 */
class nsTransactionManager : public nsITransactionManager
{
public:

  /** The default constructor.
   */
  nsTransactionManager();

  /** The default destructor.
   */
  virtual ~nsTransactionManager();

  /* Macro for AddRef(), Release(), and QueryInterface() */
  NS_DECL_ISUPPORTS

  /* nsITransactionManager method implementations. */
  virtual nsresult Do(nsITransaction *aTransaction);
  virtual nsresult Undo(void);
  virtual nsresult Redo(void);
  virtual nsresult GetNumberOfUndoItems(PRInt32 *aNumItems);
  virtual nsresult GetNumberOfRedoItems(PRInt32 *aNumItems);
  virtual nsresult Write(nsIOutputStream *aOutputStream);
  virtual nsresult AddListener(nsITransactionListener *aListener);
  virtual nsresult RemoveListener(nsITransactionListener *aListener);
};

#endif // nsTransactionManager_h__
