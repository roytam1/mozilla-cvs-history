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


#include "editor.h"
#include "nsIDOMEventReceiver.h" 
#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"

//class implementations are in order they are declared in editor.h


nsEditor::nsEditor()
{
  //initialize member variables here
}


static NS_DEFINE_IID(kIDOMEventReceiverIID, NS_IDOMEVENTRECEIVER_IID);
static NS_DEFINE_IID(kIDOMMouseListenerIID, NS_IDOMMOUSELISTENER_IID);
static NS_DEFINE_IID(kIDOMKeyListenerIID, NS_IDOMKEYLISTENER_IID);
static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
static NS_DEFINE_IID(kIDOMElementIID, NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIDOMNodeIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDocumentIID, NS_IDOCUMENT_IID);


nsEditor::~nsEditor()
{
  //the autopointers will clear themselves up. 
  //but we need to also remove the listeners or we have a leak
  COM_auto_ptr<nsIDOMEventReceiver> erP;
  nsresult t_result = mDomInterfaceP->QueryInterface(kIDOMEventReceiverIID, func_AddRefs(erP));
  if (NS_SUCCEEDED( t_result )) 
  {
    erP->RemoveEventListener(mKeyListenerP, kIDOMKeyListenerIID);
    //erP->RemoveEventListener(mMouseListenerP, kIDOMMouseListenerIID);
  }
  else
    NS_NOTREACHED("!nsEditor");
}



//BEGIN nsIEditor interface implementations


NS_IMPL_ADDREF(nsEditor)

NS_IMPL_RELEASE(nsEditor)



nsresult
nsEditor::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  static NS_DEFINE_IID(kIEditorIID, NS_IEDITOR_IID);
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIEditorIID)) {
    *aInstancePtr = (void*)(nsIEditor*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}



nsresult 
nsEditor::GetDomInterface(nsIDOMDocument **aDomInterface)
{
  *aDomInterface = mDomInterfaceP; return NS_OK;
}



nsresult
nsEditor::Init(nsIDOMDocument *aDomInterface)
{
  if (!aDomInterface)
    return NS_ERROR_NULL_POINTER;

  mDomInterfaceP = aDomInterface;

  nsresult t_result = NS_NewEditorKeyListener(func_AddRefs(mKeyListenerP), this);
  if (NS_OK != t_result)
  {
    NS_NOTREACHED("Init Failed");
    return t_result;
  }
  t_result = NS_NewEditorMouseListener(func_AddRefs(mMouseListenerP), this);
  if (NS_OK != t_result)
  {
    mKeyListenerP = 0; //dont keep the key listener if the mouse listener fails.
    NS_NOTREACHED("Mouse Listener");
    return t_result;
  }
  COM_auto_ptr<nsIDOMEventReceiver> erP;
  t_result = mDomInterfaceP->QueryInterface(kIDOMEventReceiverIID, func_AddRefs(erP));
  if (NS_OK != t_result) 
  {
    mKeyListenerP = 0;
    mMouseListenerP = 0; //dont need these if we cant register them
    NS_NOTREACHED("query interface");
    return t_result;
  }
  erP->AddEventListener(mKeyListenerP, kIDOMKeyListenerIID);
  //erP->AddEventListener(mMouseListenerP, kIDOMMouseListenerIID);

  /*
  now to handle selection
  */
  COM_auto_ptr<nsIDocument> document;
  if (NS_SUCCEEDED(t_result = mDomInterfaceP->QueryInterface(kIDocumentIID, func_AddRefs(document))))
  {
    if (!NS_SUCCEEDED(t_result = document->GetSelection(*func_AddRefs(mSelectionP))))
    {
      NS_NOTREACHED("query interface");
      return t_result;
    }
  }
  else
  {
    NS_NOTREACHED("query interface");
    return t_result;
  }

  
  return NS_OK;
}



nsresult
nsEditor::InsertString(nsString *aString)
{
  return AppendText(aString);
}



nsresult
nsEditor::SetProperties(PROPERTIES aProperty)
{
  return NS_OK;
}



nsresult
nsEditor::GetProperties(PROPERTIES **)
{
  return NS_OK;
}



nsresult
nsEditor::Commit(PRBool aCtrlKey)
{
  if (aCtrlKey)
  {
    COM_auto_ptr<nsSelectionRange> 
    mSelectionP->
  }
  else
  {
  }
  return NS_OK;
}
 

 
//END nsIEditorInterfaces


//BEGIN nsEditor Calls from public
PRBool
nsEditor::KeyDown(int aKeycode)
{
  return PR_TRUE;
}



PRBool
nsEditor::MouseClick(int aX,int aY)
{
  return PR_FALSE;
}



//END nsEditor Calls from public



//BEGIN nsEditor Private methods



nsresult
nsEditor::AppendText(nsString *aStr)
{
  COM_auto_ptr<nsIDOMNode> currentNode;
  COM_auto_ptr<nsIDOMNode> textNode;
  COM_auto_ptr<nsIDOMText> text;
  if (!aStr)
    return NS_ERROR_NULL_POINTER;
  if (NS_SUCCEEDED(GetCurrentNode(func_AddRefs(currentNode))) && 
      NS_SUCCEEDED(GetFirstTextNode(currentNode,func_AddRefs(textNode))) && 
      NS_SUCCEEDED(textNode->QueryInterface(kIDOMTextIID, func_AddRefs(text)))) {
    text->AppendData(*aStr);
  }

  return NS_OK;
}



nsresult
nsEditor::GetCurrentNode(nsIDOMNode ** aNode)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;
  /* If no node set, get first text node */
  COM_auto_ptr<nsIDOMElement> docNode;

  if (NS_SUCCEEDED(mDomInterfaceP->GetDocumentElement(func_AddRefs(docNode))))
  {
    return docNode->QueryInterface(kIDOMNodeIID,(void **) aNode);
  }
  return NS_ERROR_FAILURE;
}



nsresult
nsEditor::GetFirstTextNode(nsIDOMNode *aNode, nsIDOMNode **aRetNode)
{
  if (!aNode || !aRetNode)
  {
    NS_NOTREACHED("GetFirstTextNode Failed");
    return NS_ERROR_NULL_POINTER;
  }

  PRUint16 mType;
  PRBool mCNodes;

  COM_auto_ptr<nsIDOMNode> answer;
  
  aNode->GetNodeType(&mType);

  if (nsIDOMNode::ELEMENT_NODE == mType) {
    if (NS_SUCCEEDED(aNode->HasChildNodes(&mCNodes)) && PR_TRUE == mCNodes) 
    {
      COM_auto_ptr<nsIDOMNode> node1;
      COM_auto_ptr<nsIDOMNode> node2;

      if (!NS_SUCCEEDED(aNode->GetFirstChild(func_AddRefs(node1))))
      {
        NS_NOTREACHED("GetFirstTextNode Failed");
      }
      while(!answer && node1) 
      {
        GetFirstTextNode(node1, func_AddRefs(answer));
        node1->GetNextSibling(func_AddRefs(node2));
        node1 = node2;
      }
    }
  }
  else if (nsIDOMNode::TEXT_NODE == mType) {
    answer = aNode;
  }

    // OK, now return the answer, if any
  *aRetNode = answer;
  if (*aRetNode)
    NS_IF_ADDREF(*aRetNode);
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}

//END nsEditor Private methods



//BEGIN FACTORY METHODS



nsresult 
NS_InitEditor(nsIEditor ** aInstancePtrResult, nsIDOMDocument *aDomDoc)
{
  if (!aInstancePtrResult || !aDomDoc)
    return NS_ERROR_NULL_POINTER;
  nsEditor* editor = new nsEditor();
  if (NULL == editor) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult result = editor->Init(aDomDoc);
  if (NS_SUCCEEDED(result))
  {
    static NS_DEFINE_IID(kIEditorIID, NS_IEDITOR_IID);

    return editor->QueryInterface(kIEditorIID, (void **) aInstancePtrResult);   
  }
  else
    return result;
}
 


//END FACTORY METHODS
