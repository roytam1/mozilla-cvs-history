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

#include "nsCalCanvas.h"
#include "nsCalUICIID.h"
#include "nsXPFCModelUpdateCommand.h"
#include "nsXPFCToolkit.h"
#include "nsIXPFCObserverManager.h"
#include "nsIServiceManager.h"
#include "nsIXPFCObserver.h"
#include "nsIXPFCSubject.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kCalCanvasCID, NS_CAL_CANVAS_CID);
static NS_DEFINE_IID(kCXPFCObserverManagerCID, NS_XPFC_OBSERVERMANAGER_CID);
static NS_DEFINE_IID(kIXPFCObserverManagerIID, NS_IXPFC_OBSERVERMANAGER_IID);
static NS_DEFINE_IID(kCXPFCObserverIID,         NS_IXPFC_OBSERVER_IID);
static NS_DEFINE_IID(kCXPFCSubjectIID,          NS_IXPFC_SUBJECT_IID);

nsCalCanvas :: nsCalCanvas(nsISupports* outer) : nsXPFCCanvas(outer)
{
  NS_INIT_REFCNT();
  mUser = nsnull;
}

nsCalCanvas :: ~nsCalCanvas()
{
  nsIXPFCObserverManager* om;
  nsServiceManager::GetService(kCXPFCObserverManagerCID, kIXPFCObserverManagerIID, (nsISupports**)&om);

  nsIXPFCObserver * observer = (nsIXPFCObserver *) this;

  om->UnregisterObserver(observer);

  nsServiceManager::ReleaseService(kCXPFCObserverManagerCID, om);

  NS_IF_RELEASE(mUser);
}

nsresult nsCalCanvas::QueryInterface(REFNSIID aIID, void** aInstancePtr)      
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 
  static NS_DEFINE_IID(kClassIID, kCalCanvasCID); 
  if (aIID.Equals(kClassIID)) {                                          
    *aInstancePtr = (void*) (nsCalCanvas *)this;                                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                      
  if (aIID.Equals(kISupportsIID)) {                                      
    *aInstancePtr = (void*) (this);                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                      
  return (nsXPFCCanvas::QueryInterface(aIID, aInstancePtr));
}

NS_IMPL_ADDREF(nsCalCanvas)
NS_IMPL_RELEASE(nsCalCanvas)

nsresult nsCalCanvas :: Init()
{
  return (nsXPFCCanvas::Init()); 
}

nsresult nsCalCanvas :: GetUser(nsICalendarUser *& aUser)
{
  aUser = mUser;
  return NS_OK;    
}

nsresult nsCalCanvas :: SetUser(nsICalendarUser * aUser)
{
  mUser = aUser;
  return NS_OK;    
}


nsIModel * nsCalCanvas :: GetModel()
{
  return (nsXPFCCanvas::GetModel());
}

nsresult nsCalCanvas :: GetModelInterface(const nsIID &aModelIID, nsISupports * aInterface)
{
  return (nsXPFCCanvas::GetModelInterface(aModelIID,aInterface));
}


nsresult nsCalCanvas :: SetModel(nsIModel * aModel)
{
  return (nsXPFCCanvas::SetModel(aModel));
}

nsEventStatus nsCalCanvas::Action(nsIXPFCCommand * aCommand)
{
  nsresult res;

  nsXPFCModelUpdateCommand * command  = nsnull;
  static NS_DEFINE_IID(kModelUpdateCommandCID, NS_XPFC_MODELUPDATE_COMMAND_CID);                 
  
  res = aCommand->QueryInterface(kModelUpdateCommandCID,(void**)&command);

  /*
   * On a ModelUpdate Command, just repaint ourselves....
   */

  if ((NS_OK == res) && (GetView() != nsnull))
  {
    nsRect bounds;

    GetView()->GetBounds(bounds);

    gXPFCToolkit->GetViewManager()->UpdateView(GetView(), 
                                               bounds, 
                                               NS_VMREFRESH_AUTO_DOUBLE_BUFFER | NS_VMREFRESH_NO_SYNC);

    NS_RELEASE(command);
  }

  return (nsXPFCCanvas::Action(aCommand));
}

