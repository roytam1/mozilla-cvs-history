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

#ifndef nsEventListenerManager_h__
#define nsEventListenerManager_h__

#include "nsIEventListenerManager.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsIPrincipal.h"
#include "nsIDOMEventReceiver.h"
#include "nsHashtable.h"

class nsIDOMEvent;
class nsIAtom;

typedef struct {
  nsIDOMEventListener* mListener;
  PRUint8 mFlags;
  PRUint8 mSubType;
  PRUint8 mHandlerIsString;
  PRUint8 mSubTypeCapture;
} nsListenerStruct;

//Flag must live higher than all event flags in nsGUIEvent.h
#define NS_PRIV_EVENT_FLAG_SCRIPT 0x80

//These define the internal type of the EventListenerManager
//No listener type defined, should happen only at creation
#define NS_ELM_NONE   0
//Simple indicates only a single event listener group type (i.e. mouse, key) 
#define NS_ELM_SINGLE 1
//Multi indicates any number of listener group types accessed as member vars
#define NS_ELM_MULTI  2
//Hash indicates any number of listener group types accessed out of a hash
#define NS_ELM_HASH   4

enum EventArrayType {
  eEventArrayType_Mouse = 0,
  eEventArrayType_MouseMotion = 1,
  eEventArrayType_ContextMenu = 2,
  eEventArrayType_Key = 3,
  eEventArrayType_Load = 4,
  eEventArrayType_Focus = 5,
  eEventArrayType_Form = 6,
  eEventArrayType_Drag = 7,
  eEventArrayType_Paint = 8,
  eEventArrayType_Text = 9,
  eEventArrayType_Composition = 10,
  eEventArrayType_Menu = 11,
  eEventArrayType_Scroll = 12,
  eEventArrayType_Mutation = 13,
  eEventArrayType_Hash,
  eEventArrayType_None
};

//Keep this in line with event array types, not counting
//types HASH and NONE
#define EVENT_ARRAY_TYPE_LENGTH 14

/*
 * Event listener manager
 */

class nsEventListenerManager : public nsIEventListenerManager,
                               public nsIDOMEventReceiver
{

public:
  nsEventListenerManager();
  virtual ~nsEventListenerManager();

  NS_DECL_ISUPPORTS

  /**
  * Sets events listeners of all types. 
  * @param an event listener
  */

  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener,
                                   const nsIID& aIID, PRInt32 aFlags);
  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                      const nsIID& aIID, PRInt32 aFlags);
  NS_IMETHOD AddEventListenerByType(nsIDOMEventListener *aListener,
                                    const nsAReadableString& type,
                                    PRInt32 aFlags);
  NS_IMETHOD RemoveEventListenerByType(nsIDOMEventListener *aListener,
                                       const nsAReadableString& type,
                                       PRInt32 aFlags) ;
  NS_IMETHOD AddScriptEventListener(nsIScriptContext*aContext,
                                    nsISupports *aObject,
                                    nsIAtom *aName,
                                    const nsAReadableString& aFunc,
                                    PRBool aDeferCompilation); 
  NS_IMETHOD RegisterScriptEventListener(nsIScriptContext *aContext,
                                         nsISupports *aObject,
                                         nsIAtom* aName);
  NS_IMETHOD CompileScriptEventListener(nsIScriptContext *aContext,
                                        nsISupports *aObject,
                                        nsIAtom* aName, PRBool *aDidCompile);

  NS_IMETHOD CaptureEvent(PRInt32 aEventTypes);
  NS_IMETHOD ReleaseEvent(PRInt32 aEventTypes);

  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext, 
                         nsEvent* aEvent, 
                         nsIDOMEvent** aDOMEvent,
                         nsIDOMEventTarget* aCurrentTarget,
                         PRUint32 aFlags,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD CreateEvent(nsIPresContext* aPresContext, 
                         nsEvent* aEvent,
                         const nsAReadableString& aEventType,
                         nsIDOMEvent** aDOMEvent);

  NS_IMETHOD RemoveAllListeners(PRBool aScriptOnly);

  NS_IMETHOD SetListenerTarget(nsISupports* aTarget);

  NS_IMETHOD HasMutationListeners(PRBool* aListener)
  {
    *aListener = (GetListenersByType(eEventArrayType_Mutation, nsnull,
                                     PR_FALSE) != nsnull);
    return NS_OK;
  }

  static nsresult GetIdentifiersForType(nsIAtom* aType,
                                        EventArrayType* aArrayType,
                                        PRInt32* aSubType);

  // nsIDOMEventTarget interface
  NS_IMETHOD AddEventListener(const nsAReadableString& aType, 
                              nsIDOMEventListener* aListener, 
                              PRBool aUseCapture);
  NS_IMETHOD RemoveEventListener(const nsAReadableString& aType, 
                                 nsIDOMEventListener* aListener, 
                                 PRBool aUseCapture);
  NS_IMETHOD DispatchEvent(nsIDOMEvent* aEvent);

  // nsIDOMEventReceiver interface
  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener,
                                   const nsIID& aIID);
  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                      const nsIID& aIID);
  NS_IMETHOD GetListenerManager(nsIEventListenerManager** aInstancePtrResult);
  NS_IMETHOD GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult);
  NS_IMETHOD HandleEvent(nsIDOMEvent *aEvent);

protected:
  nsresult HandleEventSubType(nsListenerStruct* aListenerStruct,
                              nsIDOMEvent* aDOMEvent,
                              nsIDOMEventTarget* aCurrentTarget,
                              PRUint32 aSubType,
                              PRUint32 aPhaseFlags);
  nsresult CompileEventHandlerInternal(nsIScriptContext *aContext,
                                       nsISupports *aObject,
                                       nsIAtom *aName,
                                       nsListenerStruct *aListenerStruct,
                                       PRUint32 aSubType);
  nsListenerStruct* FindJSEventListener(EventArrayType aType);
  nsresult SetJSEventListener(nsIScriptContext *aContext,
                              nsISupports *aObject, nsIAtom* aName,
                              PRBool aIsString);
  nsresult AddEventListener(nsIDOMEventListener *aListener, 
                            EventArrayType aType, 
                            PRInt32 aSubType,
                            nsHashKey* aKey,
                            PRInt32 aFlags);
  nsresult RemoveEventListener(nsIDOMEventListener *aListener,
                               EventArrayType aType,
                               PRInt32 aSubType,
                               nsHashKey* aKey,
                               PRInt32 aFlags);
  void ReleaseListeners(nsVoidArray** aListeners, PRBool aScriptOnly);
  nsresult FlipCaptureBit(PRInt32 aEventTypes, PRBool aInitCapture);
  nsVoidArray* GetListenersByType(EventArrayType aType, nsHashKey* aKey, PRBool aCreate);
  EventArrayType GetTypeForIID(const nsIID& aIID);

  PRUint8 mManagerType;
  EventArrayType mSingleListenerType;
  nsVoidArray* mSingleListener;
  nsVoidArray* mMultiListeners;
  nsHashtable* mGenericListeners;
  PRBool mListenersRemoved;

  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsISupports* mTarget;  //WEAK
};


//Set of defines for distinguishing event handlers within listener groupings
//XXX Current usage allows no more than 7 types per listener grouping

#define NS_EVENT_BITS_NONE    0x00

//nsIDOMMouseListener
#define NS_EVENT_BITS_MOUSE_NONE        0x00
#define NS_EVENT_BITS_MOUSE_MOUSEDOWN   0x01
#define NS_EVENT_BITS_MOUSE_MOUSEUP     0x02
#define NS_EVENT_BITS_MOUSE_CLICK       0x04
#define NS_EVENT_BITS_MOUSE_DBLCLICK    0x08
#define NS_EVENT_BITS_MOUSE_MOUSEOVER   0x10
#define NS_EVENT_BITS_MOUSE_MOUSEOUT    0x20

//nsIDOMMouseMotionListener
#define NS_EVENT_BITS_MOUSEMOTION_NONE        0x00
#define NS_EVENT_BITS_MOUSEMOTION_MOUSEMOVE   0x01
#define NS_EVENT_BITS_MOUSEMOTION_DRAGMOVE    0x02

//nsIDOMContextMenuListener
#define NS_EVENT_BITS_CONTEXTMENU_NONE   0x00
#define NS_EVENT_BITS_CONTEXTMENU        0x01

//nsIDOMKeyListener
#define NS_EVENT_BITS_KEY_NONE      0x00
#define NS_EVENT_BITS_KEY_KEYDOWN   0x01
#define NS_EVENT_BITS_KEY_KEYUP     0x02
#define NS_EVENT_BITS_KEY_KEYPRESS  0x04

//nsIDOMTextListener
#define NS_EVENT_BITS_TEXT_NONE      0x00
#define NS_EVENT_BITS_TEXT_TEXT      0x01

//nsIDOMCompositionListener
#define NS_EVENT_BITS_COMPOSITION_NONE      0x00
#define NS_EVENT_BITS_COMPOSITION_START     0x01
#define NS_EVENT_BITS_COMPOSITION_END		0x02
#define NS_EVENT_BITS_COMPOSITION_QUERY		0x04

//nsIDOMFocusListener
#define NS_EVENT_BITS_FOCUS_NONE    0x00
#define NS_EVENT_BITS_FOCUS_FOCUS   0x01
#define NS_EVENT_BITS_FOCUS_BLUR    0x02

//nsIDOMFormListener
#define NS_EVENT_BITS_FORM_NONE     0x00
#define NS_EVENT_BITS_FORM_SUBMIT   0x01
#define NS_EVENT_BITS_FORM_RESET    0x02
#define NS_EVENT_BITS_FORM_CHANGE   0x04
#define NS_EVENT_BITS_FORM_SELECT   0x08
#define NS_EVENT_BITS_FORM_INPUT    0x10

//nsIDOMLoadListener
#define NS_EVENT_BITS_LOAD_NONE     0x00
#define NS_EVENT_BITS_LOAD_LOAD     0x01
#define NS_EVENT_BITS_LOAD_UNLOAD   0x02
#define NS_EVENT_BITS_LOAD_ABORT    0x04
#define NS_EVENT_BITS_LOAD_ERROR    0x08

//nsIDOMMenuListener
#define NS_EVENT_BITS_MENU_NONE            0x00
#define NS_EVENT_BITS_MENU_CREATE          0x01
#define NS_EVENT_BITS_XUL_CLOSE            0x02
#define NS_EVENT_BITS_MENU_DESTROY         0x04
#define NS_EVENT_BITS_MENU_ACTION          0x08
#define NS_EVENT_BITS_XUL_BROADCAST        0x10
#define NS_EVENT_BITS_XUL_COMMAND_UPDATE   0x20

//nsIScrollListener
#define NS_EVENT_BITS_SCROLLPORT_NONE             0x00
#define NS_EVENT_BITS_SCROLLPORT_OVERFLOW         0x01
#define NS_EVENT_BITS_SCROLLPORT_UNDERFLOW        0x02
#define NS_EVENT_BITS_SCROLLPORT_OVERFLOWCHANGED  0x04

//nsIDOMDragListener
#define NS_EVENT_BITS_DRAG_NONE     0x00
#define NS_EVENT_BITS_DRAG_ENTER    0x01
#define NS_EVENT_BITS_DRAG_OVER     0x02
#define NS_EVENT_BITS_DRAG_EXIT     0x04
#define NS_EVENT_BITS_DRAG_DROP     0x08
#define NS_EVENT_BITS_DRAG_GESTURE  0x10

//nsIDOMPaintListener
#define NS_EVENT_BITS_PAINT_NONE    0x00
#define NS_EVENT_BITS_PAINT_PAINT   0x01
#define NS_EVENT_BITS_PAINT_RESIZE  0x02
#define NS_EVENT_BITS_PAINT_SCROLL  0x04

//nsIDOMMutationListener
// These bits are found in nsMutationEvent.h.

//nsIDOMContextMenuListener
#define NS_EVENT_BITS_CONTEXT_NONE  0x00
#define NS_EVENT_BITS_CONTEXT_MENU  0x01

#endif // nsEventListenerManager_h__
