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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef _nsRootAccessible_H_
#define _nsRootAccessible_H_

#include "nsAccessible.h"
#include "nsIAccessibleEventReceiver.h"
#include "nsIAccessibleEventListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMTextListener.h"
#include "nsIDOMMutationListener.h"
#include "nsIDocument.h"

class nsRootAccessible : public nsAccessible,
                         public nsIAccessibleEventReceiver,
                         public nsIDOMFocusListener,
             public nsIDOMFormListener,
             public nsIDOMTextListener,
             public nsIDOMMutationListener

{
  
  NS_DECL_ISUPPORTS_INHERITED

  public:
    nsRootAccessible(nsIWeakReference* aShell);
    virtual ~nsRootAccessible();

    /* attribute wstring accName; */
    NS_IMETHOD GetAccName(PRUnichar * *aAccName);
    NS_IMETHOD GetAccValue(PRUnichar * *aAccValue);
    NS_IMETHOD GetAccParent(nsIAccessible * *aAccParent);
    NS_IMETHOD GetAccRole(PRUint32 *aAccRole);

    // ----- nsIAccessibleEventReceiver -------------------

    NS_IMETHOD AddAccessibleEventListener(nsIAccessibleEventListener *aListener);
    NS_IMETHOD RemoveAccessibleEventListener(nsIAccessibleEventListener *aListener);

    // ----- nsIDOMEventListener --------------------------
    virtual nsresult HandleEvent(nsIDOMEvent* anEvent);
    virtual nsresult Focus(nsIDOMEvent* aEvent);
    virtual nsresult Blur(nsIDOMEvent* aEvent);

  // ----- nsIDOMFormListener ---------------------------
    virtual nsresult Submit(nsIDOMEvent* aEvent);
    virtual nsresult Reset(nsIDOMEvent* aEvent);
    virtual nsresult Change(nsIDOMEvent* aEvent);
    virtual nsresult Select(nsIDOMEvent* aEvent);
    virtual nsresult Input(nsIDOMEvent* aEvent);

  // ----- nsIDOMTextListener ---------------------------
  virtual nsresult HandleText(nsIDOMEvent* aTextEvent);

  // ----- nsIDOMMutationEventListener ------------------
    NS_IMETHOD SubtreeModified(nsIDOMEvent* aMutationEvent);
    NS_IMETHOD NodeInserted(nsIDOMEvent* aMutationEvent);
    NS_IMETHOD NodeRemoved(nsIDOMEvent* aMutationEvent);
    NS_IMETHOD NodeRemovedFromDocument(nsIDOMEvent* aMutationEvent);
    NS_IMETHOD NodeInsertedIntoDocument(nsIDOMEvent* aMutationEvent);
    NS_IMETHOD AttrModified(nsIDOMEvent* aMutationEvent);
    NS_IMETHOD CharacterDataModified(nsIDOMEvent* aMutationEvent);


protected:
  virtual void GetBounds(nsRect& aRect);
  virtual nsIFrame* GetFrame();
  virtual nsIAccessible* CreateNewAccessible(nsIAccessible* aAccessible, nsIDOMNode* aNode, nsIWeakReference* aShell);

  // not a com pointer. We don't own the listener
  // it is the callers responsibility to remove the listener
  // otherwise we will get into circular referencing problems
  nsIAccessibleEventListener* mListener;
  nsCOMPtr<nsIContent> mCurrentFocus;
  nsCOMPtr<nsIDocument> mDocument;
};


#endif  
