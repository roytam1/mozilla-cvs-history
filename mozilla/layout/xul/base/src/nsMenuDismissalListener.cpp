/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsMenuDismissalListener.h"
#include "nsIMenuParent.h"
#include "nsMenuFrame.h"

/*
 * nsMenuDismissalListener implementation
 */

NS_IMPL_ADDREF(nsMenuDismissalListener)
NS_IMPL_RELEASE(nsMenuDismissalListener)


////////////////////////////////////////////////////////////////////////
nsMenuDismissalListener::nsMenuDismissalListener() :
  mWidget(0), mEnabled(PR_TRUE)
{
  NS_INIT_REFCNT();
  mMenuParent = nsnull;
}

////////////////////////////////////////////////////////////////////////
nsMenuDismissalListener::~nsMenuDismissalListener() 
{
}

////////////////////////////////////////////////////////////////////////
nsresult
nsMenuDismissalListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(nsCOMTypeInfo<nsIDOMEventReceiver>::GetIID())) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)(nsIDOMKeyListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsCOMTypeInfo<nsIDOMMouseListener>::GetIID())) {
    *aInstancePtr = (void*)(nsIDOMMouseListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsCOMTypeInfo<nsIRollupListener>::GetIID())) {
    *aInstancePtr = (void*)(nsIRollupListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID())) {                                      
    *aInstancePtr = (void*)(nsISupports*)(nsIDOMMouseListener*)this;                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }
  return NS_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////
nsresult
nsMenuDismissalListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

void
nsMenuDismissalListener::SetCurrentMenuParent(nsIMenuParent* aMenuParent)
{
  if (aMenuParent == mMenuParent)
    return;

  nsCOMPtr<nsIRollupListener> kungFuDeathGrip = this;
  Unregister();
  
  mMenuParent = aMenuParent;
  if (!aMenuParent)
    return;

  nsCOMPtr<nsIWidget> widget;
  aMenuParent->GetWidget(getter_AddRefs(widget));
  if (!widget)
    return;

  widget->CaptureRollupEvents(this, PR_TRUE, PR_FALSE);
  mWidget = widget;

  NS_ADDREF(nsMenuFrame::mDismissalListener = this);
}

NS_IMETHODIMP
nsMenuDismissalListener::Rollup()
{
  if (mEnabled) {
    if (mMenuParent) {
      mMenuParent->HideChain();
      mMenuParent->DismissChain();
    }
    else
      Unregister();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuDismissalListener::Unregister()
{
  if (mWidget)
    mWidget->CaptureRollupEvents(this, PR_FALSE, PR_FALSE);    
  
  NS_RELEASE(nsMenuFrame::mDismissalListener);
  return NS_OK;
}

NS_IMETHODIMP
nsMenuDismissalListener::EnableListener(PRBool aEnabled)
{
  mEnabled = aEnabled;
  return NS_OK;
}

