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

#ifndef nsTextAreaWidget_h__
#define nsTextAreaWidget_h__

#include "nsWidget.h"
#include "nsTextHelper.h"

#include "nsITextAreaWidget.h"

/**
 * Native WIN32 multi-line edit control wrapper. 
 */

class nsTextAreaWidget :  public nsTextHelper
{

public:
  nsTextAreaWidget();
  virtual                 ~nsTextAreaWidget();
  
  // nsISupports
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);                           
  NS_IMETHOD_(nsrefcnt) AddRef(void);                                       
  NS_IMETHOD_(nsrefcnt) Release(void);          
  
  // nsIWidget Overrides
  virtual PRBool          OnMove(PRInt32 aX, PRInt32 aY);
  virtual PRBool          OnPaint();
  virtual PRBool          OnResize(nsRect &aWindowRect);
  NS_IMETHOD              GetBounds(nsRect &aRect);
  NS_IMETHOD              Paint(nsIRenderingContext& aRenderingContext,
				const nsRect& aDirtyRect);
  
  
 protected:
  
};

#endif // nsTextAreaWidget_h__
