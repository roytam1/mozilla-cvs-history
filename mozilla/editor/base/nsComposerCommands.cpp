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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *    Ryan Cassin (rcassin@supernova.org)
 *   
 */


#include "nsIEditor.h"
#include "nsIHTMLEditor.h"

#include "nsIEditorShell.h"

#include "nsIDOMSelection.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMDocument.h"

#include "nsIClipboard.h"

#include "nsXPIDLString.h"

#include "nsComposerCommands.h"


nsBaseComposerCommand::nsBaseComposerCommand()
{
  NS_INIT_REFCNT();
}

NS_IMPL_ISUPPORTS(nsBaseComposerCommand, NS_GET_IID(nsIControllerCommand));


//--------------------------------------------------------------------------------------------------------------------
nsresult
nsBaseComposerCommand::GetInterfaceNode(const PRUnichar* nodeID, nsIEditorShell* editorShell, nsIDOMElement **outNode)
//--------------------------------------------------------------------------------------------------------------------
{
  *outNode = nsnull;
  NS_ASSERTION(editorShell, "Should have an editorShell here");

  nsCOMPtr<nsIDOMWindowInternal> webshellWindow;
  editorShell->GetWebShellWindow(getter_AddRefs(webshellWindow));  
  if (!webshellWindow) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDOMDocument> chromeDoc;
  webshellWindow->GetDocument(getter_AddRefs(chromeDoc));
  if (!chromeDoc) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDOMElement> elem;
  return chromeDoc->GetElementById(nsAutoString(nodeID), outNode);
}

//--------------------------------------------------------------------------------------------------------------------
nsresult
nsBaseComposerCommand::GetCommandNodeState(const PRUnichar *aCommand, nsIEditorShell* editorShell, nsString& outNodeState)
//--------------------------------------------------------------------------------------------------------------------
{
  nsCOMPtr<nsIDOMElement> uiNode;
  GetInterfaceNode(aCommand, editorShell, getter_AddRefs(uiNode));
  if (!uiNode) return NS_ERROR_FAILURE;

  return uiNode->GetAttribute(NS_ConvertASCIItoUCS2("state"), outNodeState);
}


//--------------------------------------------------------------------------------------------------------------------
nsresult
nsBaseComposerCommand::SetCommandNodeState(const PRUnichar *aCommand, nsIEditorShell* editorShell, const nsString& inNodeState)
//--------------------------------------------------------------------------------------------------------------------
{
  nsCOMPtr<nsIDOMElement> uiNode;
  GetInterfaceNode(aCommand, editorShell, getter_AddRefs(uiNode));
  if (!uiNode) return NS_ERROR_FAILURE;
  
  return uiNode->SetAttribute(NS_ConvertASCIItoUCS2("state"), inNodeState);
}


#ifdef XP_MAC
#pragma mark -
#endif


nsBaseStateUpdatingCommand::nsBaseStateUpdatingCommand(const char* aTagName)
: nsBaseComposerCommand()
, mTagName(aTagName)
{
}

nsBaseStateUpdatingCommand::~nsBaseStateUpdatingCommand()
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsBaseStateUpdatingCommand, nsBaseComposerCommand, nsIStateUpdatingControllerCommand);

NS_IMETHODIMP
nsBaseStateUpdatingCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;  
  if (!editorShell) return NS_OK;
  // check for plain text?
  nsCOMPtr<nsIEditor> editor;
  editorShell->GetEditor(getter_AddRefs(editor));
  if (!editor) return NS_OK;
 
  // Enable commands only if not in HTML source edit mode
  PRBool sourceMode = PR_FALSE;
  editorShell->IsHTMLSourceMode(&sourceMode);
  *outCmdEnabled = !sourceMode;
    
  // also udpate the command state  
  nsresult rv = UpdateCommandState(aCommand, refCon);
  if (NS_FAILED(rv))
  {
    *outCmdEnabled = PR_FALSE;
    return NS_OK;
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsBaseStateUpdatingCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  if (!editorShell) return NS_ERROR_NOT_INITIALIZED;

  nsresult rv = ToggleState(editorShell, mTagName);
  if (NS_FAILED(rv)) return rv;
  
  return UpdateCommandState(aCommand, refCon);
}

NS_IMETHODIMP
nsBaseStateUpdatingCommand::UpdateCommandState(const PRUnichar *aCommandName, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  nsresult rv = NS_OK;
  if (editorShell)
  {
    PRBool stateIsSet;
    rv = GetCurrentState(editorShell, mTagName, stateIsSet);
    if (NS_FAILED(rv)) return rv;

    if (!mGotState || (stateIsSet != mState))
    {
      // poke the UI
      SetCommandNodeState(aCommandName, editorShell, NS_ConvertASCIItoUCS2(stateIsSet ? "true" : "false"));

      mGotState = PR_TRUE;
      mState = stateIsSet;
    }
  }
  return rv;
}


/*
#ifdef XP_MAC
#pragma mark -
#endif

NS_IMETHODIMP
nsCloseCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (editorShell)
  {
    *outCmdEnabled = PR_TRUE;   // check if loading etc?
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsCloseCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);

  nsresult rv = NS_OK;
  
  if (editorShell)
  {
    PRBool wasConfirmed;
    rv = editorShell->CloseWindow(&wasConfirmed);
  }
  
  return rv;  
}
*/


#ifdef XP_MAC
#pragma mark -
#endif

NS_IMETHODIMP
nsPrintingCommands::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (!editorShell) return NS_OK;

  nsAutoString cmdString(aCommand);
  if (cmdString.EqualsWithConversion("cmd_print"))
    *outCmdEnabled = PR_TRUE;
  else if (cmdString.EqualsWithConversion("cmd_printSetup"))
    *outCmdEnabled = PR_FALSE;        // not implemented yet
  else if (cmdString.EqualsWithConversion("cmd_printPreview"))
    *outCmdEnabled = PR_FALSE;        // not implemented yet
  
  return NS_OK;
}


NS_IMETHODIMP
nsPrintingCommands::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  if (!editorShell) return NS_ERROR_NULL_POINTER;
  nsresult rv = NS_OK;
  
  editorShell->FinishHTMLSource();

  nsAutoString cmdString(aCommand);
  if (cmdString.EqualsWithConversion("cmd_print"))
    rv = editorShell->Print();
  else if (cmdString.EqualsWithConversion("cmd_printSetup"))
    rv = NS_ERROR_NOT_IMPLEMENTED;
  else if (cmdString.EqualsWithConversion("cmd_printPreview"))
    rv = NS_ERROR_NOT_IMPLEMENTED;

  return rv;  
}

#ifdef XP_MAC
#pragma mark -
#endif

NS_IMETHODIMP
nsPasteQuotationCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (editorShell)
  {
    nsCOMPtr<nsIEditor> editor;
    editorShell->GetEditor(getter_AddRefs(editor));
    if (editor)
      editor->CanPaste(nsIClipboard::kGlobalClipboard, *outCmdEnabled);
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsPasteQuotationCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);

  nsresult rv = NS_OK;
  if (editorShell)
  {
    rv = editorShell->PasteAsQuotation(nsIClipboard::kGlobalClipboard);
  }
  
  return rv;  
}


#ifdef XP_MAC
#pragma mark -
#endif


nsStyleUpdatingCommand::nsStyleUpdatingCommand(const char* aTagName)
: nsBaseStateUpdatingCommand(aTagName)
{
}

nsresult
nsStyleUpdatingCommand::GetCurrentState(nsIEditorShell *aEditorShell, const char* aTagName, PRBool& outStyleSet)
{
  NS_ASSERTION(aEditorShell, "Need editor shell here");
  nsresult rv = NS_OK;

  PRBool firstOfSelectionHasProp = PR_FALSE;
  PRBool anyOfSelectionHasProp = PR_FALSE;
  PRBool allOfSelectionHasProp = PR_FALSE;

  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_NOT_INITIALIZED;
  
  nsCOMPtr<nsIAtom> styleAtom = getter_AddRefs(NS_NewAtom(aTagName));
  rv = htmlEditor->GetInlineProperty(styleAtom, nsnull, nsnull, firstOfSelectionHasProp, anyOfSelectionHasProp, allOfSelectionHasProp);
  outStyleSet = allOfSelectionHasProp;			// change this to alter the behaviour

  return rv;
}
 
nsresult
nsStyleUpdatingCommand::ToggleState(nsIEditorShell *aEditorShell, const char* aTagName)
{
  PRBool styleSet;
  nsresult rv = GetCurrentState(aEditorShell, aTagName, styleSet);
  if (NS_FAILED(rv)) return rv;
  
  nsAutoString tagName; tagName.AssignWithConversion(aTagName);
  if (styleSet)
    rv = aEditorShell->RemoveTextProperty(tagName.GetUnicode(), nsnull);
  else
    rv = aEditorShell->SetTextProperty(tagName.GetUnicode(), nsnull, nsnull);

  return rv;
}

#ifdef XP_MAC
#pragma mark -
#endif

nsListCommand::nsListCommand(const char* aTagName)
: nsBaseStateUpdatingCommand(aTagName)
{
}

nsresult
nsListCommand::GetCurrentState(nsIEditorShell *aEditorShell, const char* aTagName, PRBool& outInList)
{
  NS_ASSERTION(aEditorShell, "Need editorShell here");

  PRBool bMixed;
  PRUnichar *tagStr;
  nsresult rv = aEditorShell->GetListState(&bMixed, &tagStr);
  if (NS_FAILED(rv)) return rv;

  // Need to use mTagName????
  outInList = (0 == nsCRT::strcmp(tagStr, mTagName));

  if (tagStr) nsCRT::free(tagStr);

  return NS_OK;
}


nsresult
nsListCommand::ToggleState(nsIEditorShell *aEditorShell, const char* aTagName)
{
  PRBool inList;
  // Need to use mTagName????
  nsresult rv = GetCurrentState(aEditorShell, mTagName, inList);
  if (NS_FAILED(rv)) return rv;
  nsAutoString listType; listType.AssignWithConversion(mTagName);

  if (inList)
    rv = aEditorShell->RemoveList(listType.GetUnicode());    
  else
    rv = aEditorShell->MakeOrChangeList(listType.GetUnicode());
    
  return rv;
}


#ifdef XP_MAC
#pragma mark -
#endif

nsListItemCommand::nsListItemCommand(const char* aTagName)
: nsBaseStateUpdatingCommand(aTagName)
{
}

nsresult
nsListItemCommand::GetCurrentState(nsIEditorShell *aEditorShell, const char* aTagName, PRBool& outInList)
{
  NS_ASSERTION(aEditorShell, "Need editorShell here");

  PRBool bMixed;
  PRUnichar *tagStr;
  nsresult rv = aEditorShell->GetListItemState(&bMixed, &tagStr);
  if (NS_FAILED(rv)) return rv;

  outInList = (0 == nsCRT::strcmp(tagStr, mTagName));

  if (tagStr) nsCRT::free(tagStr);

  return NS_OK;
}

nsresult
nsListItemCommand::ToggleState(nsIEditorShell *aEditorShell, const char* aTagName)
{
  NS_ASSERTION(aEditorShell, "Need editorShell here");
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  if (!editor) return NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_NOT_INITIALIZED;

  PRBool inList;
  // Need to use mTagName????
  nsresult rv = GetCurrentState(aEditorShell, mTagName, inList);
  if (NS_FAILED(rv)) return rv;
  
  if (inList)
  {
    // To remove a list, first get what kind of list we're in
    PRBool bMixed;
    PRUnichar *tagStr;
    rv = aEditorShell->GetListState(&bMixed, &tagStr);
    if (NS_FAILED(rv)) return rv; 
    if (tagStr)
    {
      if (!bMixed)
      {
        nsAutoString listType(tagStr);
        rv = htmlEditor->RemoveList(listType);    
      }
      nsCRT::free(tagStr);
    }
  }
  else
  {
    nsAutoString itemType; itemType.AssignWithConversion(mTagName);
    // Set to the requested paragraph type
    //XXX Note: This actually doesn't work for "LI",
    //    but we currently don't use this for non DL lists anyway.
    // Problem: won't this replace any current block paragraph style?
    rv = htmlEditor->SetParagraphFormat(itemType);
  }
    
  return rv;
}


#ifdef XP_MAC
#pragma mark -
#endif

NS_IMETHODIMP
nsRemoveListCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (editorShell)
  {
    // It is enabled if we are in any list type
    PRBool bMixed;
    PRUnichar *tagStr;
    nsresult rv = editorShell->GetListState(&bMixed, &tagStr);
    if (NS_FAILED(rv)) return rv;

    if (bMixed)
      *outCmdEnabled = PR_TRUE;
    else
      *outCmdEnabled = (tagStr && *tagStr);
    
    if (tagStr) nsCRT::free(tagStr);
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsRemoveListCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);

  nsresult rv = NS_OK;
  if (editorShell)
    // This removes any list type
    rv = editorShell->RemoveList(nsnull);

  return rv;  
}


#ifdef XP_MAC
#pragma mark -
#endif

NS_IMETHODIMP
nsIndentCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (editorShell)
  {
    nsCOMPtr<nsIEditor> editor;
    editorShell->GetEditor(getter_AddRefs(editor));
    if (editor)
      *outCmdEnabled = PR_TRUE;     // can always indent (I guess)
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsIndentCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);

  nsresult rv = NS_OK;
  if (editorShell)
  {
    nsAutoString indentStr; indentStr.AssignWithConversion("indent");
    rv = editorShell->Indent(indentStr.GetUnicode());
  }
  
  return rv;  
}

NS_IMETHODIMP
nsOutdentCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (editorShell)
  {
    nsCOMPtr<nsIEditor> editor;
    editorShell->GetEditor(getter_AddRefs(editor));
    nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
    if (htmlEditor)
    {
      PRBool canIndent, canOutdent;
      htmlEditor->GetIndentState(canIndent, canOutdent);
      
      *outCmdEnabled = canOutdent;
    }
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsOutdentCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);

  nsresult rv = NS_OK;
  if (editorShell)
  {
    nsAutoString indentStr; indentStr.AssignWithConversion("outdent");
    rv = editorShell->Indent(indentStr.GetUnicode());
  }
  
  return rv;  
}

#ifdef XP_MAC
#pragma mark -
#endif


nsMultiStateCommand::nsMultiStateCommand()
: nsBaseComposerCommand()
, mGotState(PR_FALSE)
{
}

nsMultiStateCommand::~nsMultiStateCommand()
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsMultiStateCommand, nsBaseComposerCommand, nsIStateUpdatingControllerCommand);

NS_IMETHODIMP
nsMultiStateCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (editorShell)
  {
    nsCOMPtr<nsIEditor> editor;
    editorShell->GetEditor(getter_AddRefs(editor));
    if (editor)
    {
      // should be disabled sometimes, like if the current selection is an image
      *outCmdEnabled = PR_TRUE;
    }
  }
    
  nsresult rv = UpdateCommandState(aCommand, refCon);
  if (NS_FAILED(rv)) {
    *outCmdEnabled = PR_FALSE;
    return NS_OK;
  }
   
  return NS_OK; 
}


NS_IMETHODIMP
nsMultiStateCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);

  nsresult rv = NS_OK;
  if (editorShell)
  {
      // we have to grab the state attribute on our command node to find out
      // what format to set the paragraph to
      nsAutoString stateAttribute;    
      rv = GetCommandNodeState(aCommand, editorShell, stateAttribute);
      if (NS_FAILED(rv)) return rv;

      rv = SetState(editorShell, stateAttribute);
  }
  
  return rv;  
}


NS_IMETHODIMP
nsMultiStateCommand::UpdateCommandState(const PRUnichar *aCommandName, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  nsresult rv = NS_OK;
  if (editorShell)
  {
      nsString  curFormat;
      PRBool    isMixed;
     
      rv = GetCurrentState(editorShell, curFormat, isMixed);
      if (NS_FAILED(rv)) return rv;
      
      if (isMixed)
        curFormat.AssignWithConversion("mixed");
        
      if (!mGotState || (curFormat != mStateString))
      {
        // poke the UI
        SetCommandNodeState(aCommandName, editorShell, curFormat);

        mGotState = PR_TRUE;
        mStateString = curFormat;
      }
  }
  return rv;
}


#ifdef XP_MAC
#pragma mark -
#endif

nsParagraphStateCommand::nsParagraphStateCommand()
: nsMultiStateCommand()
{
}

nsresult
nsParagraphStateCommand::GetCurrentState(nsIEditorShell *aEditorShell, nsString& outStateString, PRBool& outMixed)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;
  return htmlEditor->GetParagraphState(outMixed, outStateString);
}


nsresult
nsParagraphStateCommand::SetState(nsIEditorShell *aEditorShell, nsString& newState)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;

  return htmlEditor->SetParagraphFormat(newState);
}


#ifdef XP_MAC
#pragma mark -
#endif

nsFontFaceStateCommand::nsFontFaceStateCommand()
: nsMultiStateCommand()
{
}

nsresult
nsFontFaceStateCommand::GetCurrentState(nsIEditorShell *aEditorShell, nsString& outStateString, PRBool& outMixed)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;

  return htmlEditor->GetFontFaceState(outMixed, outStateString);
}


nsresult
nsFontFaceStateCommand::SetState(nsIEditorShell *aEditorShell, nsString& newState)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;
  
  nsresult rv;
  
  NS_ConvertASCIItoUCS2 emptyString("");
  NS_ConvertASCIItoUCS2 fontString("font");
  NS_ConvertASCIItoUCS2 faceString("face");
  
  nsCOMPtr<nsIAtom> ttAtom = getter_AddRefs(NS_NewAtom("tt"));
  nsCOMPtr<nsIAtom> fontAtom = getter_AddRefs(NS_NewAtom("font"));

  if (newState.EqualsWithConversion("tt"))
  {
    // The old "teletype" attribute  
    rv = htmlEditor->SetInlineProperty(ttAtom, &emptyString, &emptyString);  
    // Clear existing font face
    rv = htmlEditor->RemoveInlineProperty(fontAtom, &faceString);
  }
  else
  {
    // Remove any existing TT nodes
    rv = htmlEditor->RemoveInlineProperty(ttAtom, &emptyString);  

    if (newState == emptyString || newState.EqualsWithConversion("normal")) {
      rv = htmlEditor->RemoveInlineProperty(fontAtom, &faceString);
    } else {
      rv = htmlEditor->SetInlineProperty(fontAtom, &faceString, &newState);
    }
  }
  
  return rv;
}

#ifdef XP_MAC
#pragma mark -
#endif

nsFontColorStateCommand::nsFontColorStateCommand()
: nsMultiStateCommand()
{
}

nsresult
nsFontColorStateCommand::GetCurrentState(nsIEditorShell *aEditorShell, nsString& outStateString, PRBool& outMixed)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;

  return htmlEditor->GetFontColorState(outMixed, outStateString);
}


nsresult
nsFontColorStateCommand::SetState(nsIEditorShell *aEditorShell, nsString& newState)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;
  
  nsresult rv;
  
  NS_ConvertASCIItoUCS2 emptyString("");
  NS_ConvertASCIItoUCS2 fontString("font");
  NS_ConvertASCIItoUCS2 colorString("color");
  
  nsCOMPtr<nsIAtom> fontAtom = getter_AddRefs(NS_NewAtom("font"));

  if (newState == emptyString || newState.EqualsWithConversion("normal")) {
    rv = htmlEditor->RemoveInlineProperty(fontAtom, &colorString);
  } else {
    rv = htmlEditor->SetInlineProperty(fontAtom, &colorString, &newState);
  }
  
  return rv;
}

#ifdef XP_MAC
#pragma mark -
#endif

nsBackgroundColorStateCommand::nsBackgroundColorStateCommand()
: nsMultiStateCommand()
{
}

nsresult
nsBackgroundColorStateCommand::GetCurrentState(nsIEditorShell *aEditorShell, nsString& outStateString, PRBool& outMixed)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;


  return htmlEditor->GetBackgroundColorState(outMixed, outStateString);
}


nsresult
nsBackgroundColorStateCommand::SetState(nsIEditorShell *aEditorShell, nsString& newState)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;

  return htmlEditor->SetBackgroundColor(newState);
}

#ifdef XP_MAC
#pragma mark -
#endif

nsAlignCommand::nsAlignCommand()
: nsMultiStateCommand()
{
}

nsresult
nsAlignCommand::GetCurrentState(nsIEditorShell *aEditorShell, nsString& outStateString, PRBool& outMixed)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;
 
  nsIHTMLEditor::EAlignment firstAlign;
  nsresult rv = htmlEditor->GetAlignment(outMixed, firstAlign);
  if (NS_FAILED(rv)) return rv;
  switch (firstAlign)
  {
    default:
    case nsIHTMLEditor::eLeft:
      outStateString.AssignWithConversion("left");
      break;
      
    case nsIHTMLEditor::eCenter:
      outStateString.AssignWithConversion("center");
      break;
      
    case nsIHTMLEditor::eRight:
      outStateString.AssignWithConversion("right");
      break;

    case nsIHTMLEditor::eJustify:
      outStateString.AssignWithConversion("justify");
      break;
  }
  
  return NS_OK;
}


nsresult
nsAlignCommand::SetState(nsIEditorShell *aEditorShell, nsString& newState)
{
  NS_ASSERTION(aEditorShell, "Need an editor shell here");
  
  nsCOMPtr<nsIEditor> editor;
  aEditorShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) return NS_ERROR_FAILURE;

  return htmlEditor->Align(newState);
}


#ifdef XP_MAC
#pragma mark -
#endif

NS_IMETHODIMP
nsRemoveStylesCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (editorShell)
  {
    nsCOMPtr<nsIEditor> editor;
    editorShell->GetEditor(getter_AddRefs(editor));
    if (editor)
    {
      // test if we have any styles?
      *outCmdEnabled = PR_TRUE;
    }
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsRemoveStylesCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);

  nsresult rv = NS_OK;
  if (editorShell)
  {
    rv = editorShell->RemoveTextProperty(NS_ConvertASCIItoUCS2("all").GetUnicode(), NS_ConvertASCIItoUCS2("").GetUnicode());
    if (NS_FAILED(rv)) return rv;
    
    // now get all the style buttons to update
    nsCOMPtr<nsIDOMWindowInternal> contentWindow;
    editorShell->GetContentWindow(getter_AddRefs(contentWindow));
    if (contentWindow)
      contentWindow->UpdateCommands(NS_ConvertASCIItoUCS2("style"));
  }
  
  return rv;  
}

#ifdef XP_MAC
#pragma mark -
#endif

NS_IMETHODIMP
nsIncreaseFontSizeCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (editorShell)
  {
    nsCOMPtr<nsIEditor> editor;
    editorShell->GetEditor(getter_AddRefs(editor));
    if (editor)
    {
      // test if we are at the max size?
      *outCmdEnabled = PR_TRUE;
    }
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsIncreaseFontSizeCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);

  nsresult rv = NS_OK;
  if (editorShell)
  {
    rv = editorShell->IncreaseFontSize();
  }
  
  return rv;  
}

#ifdef XP_MAC
#pragma mark -
#endif

NS_IMETHODIMP
nsDecreaseFontSizeCommand::IsCommandEnabled(const PRUnichar *aCommand, nsISupports * refCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);
  *outCmdEnabled = PR_FALSE;
  if (editorShell)
  {
    nsCOMPtr<nsIEditor> editor;
    editorShell->GetEditor(getter_AddRefs(editor));
    if (editor)
    {
      // test if we are at the min size?
      *outCmdEnabled = PR_TRUE;
    }
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsDecreaseFontSizeCommand::DoCommand(const PRUnichar *aCommand, nsISupports * refCon)
{
  nsCOMPtr<nsIEditorShell> editorShell = do_QueryInterface(refCon);

  nsresult rv = NS_OK;
  if (editorShell)
  {
    rv = editorShell->DecreaseFontSize();
  }
  
  return rv;  
}
