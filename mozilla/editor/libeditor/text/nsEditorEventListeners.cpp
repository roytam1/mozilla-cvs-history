/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include "nsEditorEventListeners.h"
#include "nsEditor.h"

#include "CreateElementTxn.h"

#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMCharacterData.h"
#include "nsIEditProperty.h"
#include "nsISupportsArray.h"
#include "nsString.h"

static NS_DEFINE_IID(kIDOMElementIID, NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIDOMCharacterDataIID, NS_IDOMCHARACTERDATA_IID);

 
/*
 * nsTextEditorKeyListener implementation
 */


NS_IMPL_ADDREF(nsTextEditorKeyListener)

NS_IMPL_RELEASE(nsTextEditorKeyListener)


nsTextEditorKeyListener::nsTextEditorKeyListener() 
{
  NS_INIT_REFCNT();
}



nsTextEditorKeyListener::~nsTextEditorKeyListener() 
{
}



nsresult
nsTextEditorKeyListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  static NS_DEFINE_IID(kIDOMKeyListenerIID, NS_IDOMKEYLISTENER_IID);
  static NS_DEFINE_IID(kIDOMEventListenerIID, NS_IDOMEVENTLISTENER_IID);
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMEventListenerIID)) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMKeyListenerIID)) {
    *aInstancePtr = (void*)(nsIDOMKeyListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}



nsresult
nsTextEditorKeyListener::ProcessEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}


//#define HAVE_EVENT_CHARCODE				// on when we have the charCode in the event

#ifndef HAVE_EVENT_CHARCODE

nsresult
nsTextEditorKeyListener::GetCharFromKeyCode(PRUint32 aKeyCode, PRBool aIsShift, char *aChar)
{
  /* This is completely temporary to get this working while I check out Unicode conversion code. */
#ifdef XP_MAC
  if (aChar) {
    *aChar = (char)aKeyCode;
    return NS_OK;
    }
#else
  if (aKeyCode >= 0x41 && aKeyCode <= 0x5A) {
    if (aIsShift) {
      *aChar = (char)aKeyCode;
    }
    else {
      *aChar = (char)(aKeyCode + 0x20);
    }
    return NS_OK;
  }
  else if ((aKeyCode >= 0x30 && aKeyCode <= 0x39) || aKeyCode == 0x20) {
      *aChar = (char)aKeyCode;
      return NS_OK;
  }
#endif
  return NS_ERROR_FAILURE;
}

#endif /* HAVE_EVENT_CHARCODE */

nsresult
nsTextEditorKeyListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  PRUint32 keyCode;
  PRBool   isShift;
  PRBool   ctrlKey;
  
  if (NS_SUCCEEDED(aKeyEvent->GetKeyCode(&keyCode)) && 
      NS_SUCCEEDED(aKeyEvent->GetShiftKey(&isShift)) &&
      NS_SUCCEEDED(aKeyEvent->GetCtrlKey(&ctrlKey))
      ) {
    PRBool keyProcessed;
    ProcessShortCutKeys(aKeyEvent, keyProcessed);
    if (PR_FALSE==keyProcessed)
    {
      switch(keyCode) {
      case nsIDOMEvent::VK_BACK:
        mEditor->DeleteSelection(nsIEditor::eRTL);
        break;

      case nsIDOMEvent::VK_DELETE:
        mEditor->DeleteSelection(nsIEditor::eLTR);
        break;

      case nsIDOMEvent::VK_RETURN:
        // Need to implement creation of either <P> or <BR> nodes.
        // Enter key?
        mEditor->InsertBreak();
        break;
        
      default:
        {
          nsAutoString  key;
#ifdef HAVE_EVENT_CHARCODE
          PRUint32     character;
          // do we convert to Unicode here, or has this already been done? (sfraser)
          if (NS_SUCCEEDED(aKeyEvent->GetCharCode(&character)))
          {
            key += character;
            mEditor->InsertText(key);
          }
#else
          char character;
          // XXX Replace with x-platform NS-virtkeycode transform.
          if (NS_OK == GetCharFromKeyCode(keyCode, isShift, & character)) {
            nsAutoString key;
            key += character;
            if (!isShift) {
              key.ToLowerCase();
            }
            mEditor->InsertText(key);
          }
#endif
        }
        break;
      }
    }
  }
  
  return NS_ERROR_BASE;
}



nsresult
nsTextEditorKeyListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorKeyListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}

/* these includes are for debug only.  this module should never instantiate it's own transactions */
#include "SplitElementTxn.h"
#include "TransactionFactory.h"
static NS_DEFINE_IID(kSplitElementTxnIID,   SPLIT_ELEMENT_TXN_IID);
nsresult
nsTextEditorKeyListener::ProcessShortCutKeys(nsIDOMEvent* aKeyEvent, PRBool& aProcessed)
{
  aProcessed=PR_FALSE;
  PRUint32 keyCode;
  PRBool isShift;
  PRBool ctrlKey;
  
  if (NS_SUCCEEDED(aKeyEvent->GetKeyCode(&keyCode)) && 
      NS_SUCCEEDED(aKeyEvent->GetShiftKey(&isShift)) &&
      NS_SUCCEEDED(aKeyEvent->GetCtrlKey(&ctrlKey))
      ) 
  {
    // XXX: please please please get these mappings from an external source!
    switch (keyCode)
    {
      // XXX: hard-coded select all
      case nsIDOMEvent::VK_A:
        if (PR_TRUE==ctrlKey)
        {
          aProcessed=PR_TRUE;
          if (mEditor)
            mEditor->SelectAll();
        }
        break;

      // XXX: hard-coded cut
      case nsIDOMEvent::VK_X:
        if (PR_TRUE==ctrlKey)
        {
          aProcessed=PR_TRUE;
          if (mEditor)
            mEditor->Cut();
        }
        break;

      // XXX: hard-coded copy
      case nsIDOMEvent::VK_C:
        if (PR_TRUE==ctrlKey)
        {
          aProcessed=PR_TRUE;
          if (mEditor)
            mEditor->Copy();
        }
        break;

      // XXX: hard-coded paste
      case nsIDOMEvent::VK_V:
        if (PR_TRUE==ctrlKey)
        {
          printf("control-v\n");
          aProcessed=PR_TRUE;
          if (mEditor)
            mEditor->Paste();
        }
        break;

      // XXX: hard-coded undo
      case nsIDOMEvent::VK_Z:
        if (PR_TRUE==ctrlKey)
        {
          aProcessed=PR_TRUE;
          if (mEditor)
            mEditor->Undo(1);
        }
        break;

      // XXX: hard-coded redo
      case nsIDOMEvent::VK_Y:
        if (PR_TRUE==ctrlKey)
        {
          aProcessed=PR_TRUE;
          if (mEditor)
            mEditor->Redo(1);
        }
        break;

      // hard-coded ChangeTextAttributes test -- italics
      case nsIDOMEvent::VK_I:
        if (PR_TRUE==ctrlKey)
        {
          aProcessed=PR_TRUE;
          if (mEditor)
          {
            mEditor->SetTextProperty(nsIEditProperty::italic);
          }
        }
        break;

      // hard-coded ChangeTextAttributes test -- bold
      case nsIDOMEvent::VK_B:
        if (PR_TRUE==ctrlKey)
        {
          aProcessed=PR_TRUE;
          if (mEditor)
          {
            mEditor->SetTextProperty(nsIEditProperty::bold);
          }
        }
        break;

      case nsIDOMEvent::VK_U:
        if (PR_TRUE==ctrlKey)
        {
          aProcessed=PR_TRUE;
          if (mEditor)
          {
            PRBool any, all;
            mEditor->GetTextProperty(nsIEditProperty::bold, any, all);
            printf("the selection has BOLD any=%d all=%d\n", any, all);
          }
        }
        break;


      //XXX: test for change and remove attribute, hard-coded to be width on first table in doc
      case nsIDOMEvent::VK_TAB:
        {
          //XXX: should be from a factory
          //XXX: should manage the refcount of txn
          /*
          nsAutoString attribute("width");
          nsAutoString value("400");

          nsAutoString tableTag("TABLE");
          nsCOMPtr<nsIDOMNode> currentNode;
          nsCOMPtr<nsIDOMElement> element;
          if (NS_SUCCEEDED(mEditor->GetFirstNodeOfType(nsnull, tableTag, getter_AddRefs(currentNode))))
          {
            if (NS_SUCCEEDED(currentNode->QueryInterface(kIDOMElementIID, getter_AddRefs(element)))) 
            {
              nsresult result;
              if (PR_TRUE==ctrlKey)   // remove the attribute
                result = mEditor->RemoveAttribute(element, attribute);
              else                    // change the attribute
                result = mEditor->SetAttribute(element, attribute, value);
            }
          }
          */
        }
        aProcessed=PR_TRUE;
        break;

      case nsIDOMEvent::VK_INSERT:
        {
          //XXX: should be from a factory
          //XXX: should manage the refcount of txn
          /*
          nsresult result;
          nsAutoString attribute("src");
          nsAutoString value("resource:/res/samples/raptor.jpg");

          nsAutoString imgTag("HR");
          nsAutoString bodyTag("BODY");
          nsCOMPtr<nsIDOMNode> currentNode;
          result = mEditor->GetFirstNodeOfType(nsnull, bodyTag, getter_AddRefs(currentNode));
          if (NS_SUCCEEDED(result))
          {
            PRInt32 position;
            if (PR_TRUE==ctrlKey)
              position=CreateElementTxn::eAppend;
            else
              position=0;
            result = mEditor->CreateNode(imgTag, currentNode, position);
          }
          mEditor->InsertNode(nsnull, nsnull, 0);
          */
        }
        aProcessed=PR_TRUE;
        break;

    }
  }
  return NS_OK;
}



/*
 * nsTextEditorMouseListener implementation
 */



NS_IMPL_ADDREF(nsTextEditorMouseListener)

NS_IMPL_RELEASE(nsTextEditorMouseListener)


nsTextEditorMouseListener::nsTextEditorMouseListener() 
{
  NS_INIT_REFCNT();
}



nsTextEditorMouseListener::~nsTextEditorMouseListener() 
{
}



nsresult
nsTextEditorMouseListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  static NS_DEFINE_IID(kIDOMMouseListenerIID, NS_IDOMMOUSELISTENER_IID);
  static NS_DEFINE_IID(kIDOMEventListenerIID, NS_IDOMEVENTLISTENER_IID);
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMEventListenerIID)) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMMouseListenerIID)) {
    *aInstancePtr = (void*)(nsIDOMMouseListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}



nsresult
nsTextEditorMouseListener::ProcessEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIDOMNode> target;
  if (NS_OK == aMouseEvent->GetTarget(getter_AddRefs(target))) {
//    nsSetCurrentNode(aTarget);
  }
  //Should not be error.  Need a new way to do return values
  return NS_ERROR_BASE;
}



nsresult
nsTextEditorMouseListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



/*
 * Factory functions
 */



nsresult 
NS_NewEditorKeyListener(nsIDOMEventListener ** aInstancePtrResult, 
                        nsITextEditor *aEditor)
{
  nsTextEditorKeyListener* it = new nsTextEditorKeyListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);

  static NS_DEFINE_IID(kIDOMEventListenerIID, NS_IDOMEVENTLISTENER_IID);

  return it->QueryInterface(kIDOMEventListenerIID, (void **) aInstancePtrResult);   
}



nsresult
NS_NewEditorMouseListener(nsIDOMEventListener ** aInstancePtrResult, 
                          nsITextEditor *aEditor)
{
  nsTextEditorMouseListener* it = new nsTextEditorMouseListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);

  static NS_DEFINE_IID(kIDOMEventListenerIID, NS_IDOMEVENTLISTENER_IID);

  return it->QueryInterface(kIDOMEventListenerIID, (void **) aInstancePtrResult);   
}




