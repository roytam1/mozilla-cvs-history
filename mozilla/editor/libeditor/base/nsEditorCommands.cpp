/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#include "nsCRT.h"
#include "nsString.h"

#include "nsIEditor.h"
#include "nsIEditorMailSupport.h"
#include "nsIPlaintextEditor.h"
#include "nsISelectionController.h"
#include "nsIPresShell.h"
#include "nsIClipboard.h"

#include "nsEditorCommands.h"


#define COMMAND_NAME   NS_LITERAL_STRING("cmd_name")
#define STATE_ENABLED  NS_LITERAL_STRING("state_enabled")


nsBaseEditorCommand::nsBaseEditorCommand()
{
  NS_INIT_REFCNT();
}

NS_IMPL_ISUPPORTS1(nsBaseEditorCommand, nsIControllerCommand)

#ifdef XP_MAC
#pragma mark -
#endif


NS_IMETHODIMP
nsUndoCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = PR_FALSE;
  if (aEditor)
  {
    PRBool isEnabled;
    return aEditor->CanUndo(&isEnabled, outCmdEnabled);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsUndoCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  if (aEditor)
    return aEditor->Undo(1);
    
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsUndoCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsUndoCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}

NS_IMETHODIMP
nsRedoCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = PR_FALSE;
  if (aEditor)
  {
    PRBool isEnabled;
    return aEditor->CanRedo(&isEnabled, outCmdEnabled);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsRedoCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  if (aEditor)
    return aEditor->Redo(1);
    
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsRedoCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsRedoCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}

NS_IMETHODIMP
nsCutCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = PR_FALSE;
  if (aEditor)
    return aEditor->CanCut(outCmdEnabled);

  return NS_OK;
}


NS_IMETHODIMP
nsCutCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  if (aEditor)
    return aEditor->Cut();
    
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsCutCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsCutCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}


NS_IMETHODIMP
nsCutOrDeleteCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = (editor != nsnull);
  return NS_OK;
}


NS_IMETHODIMP
nsCutOrDeleteCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(aCommandRefCon);
  if (editor)
  {
    nsCOMPtr<nsISelection> selection;
    nsresult rv = editor->GetSelection(getter_AddRefs(selection));
    if (NS_SUCCEEDED(rv) && selection)
    {
      PRBool isCollapsed;
      rv = selection->GetIsCollapsed(&isCollapsed);
      if (NS_SUCCEEDED(rv) && isCollapsed)
        return editor->DeleteSelection(nsIEditor::eNext);
    }
    return editor->Cut();
  }
    
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsCutOrDeleteCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsCutOrDeleteCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}

NS_IMETHODIMP
nsCopyCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = PR_FALSE;
  if (aEditor)
    return aEditor->CanCopy(outCmdEnabled);

  return NS_OK;
}


NS_IMETHODIMP
nsCopyCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  if (aEditor)
    return aEditor->Copy();
    
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsCopyCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsCopyCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}

NS_IMETHODIMP
nsCopyOrDeleteCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = (editor != nsnull);
  return NS_OK;
}


NS_IMETHODIMP
nsCopyOrDeleteCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(aCommandRefCon);
  if (editor)
  {
    nsCOMPtr<nsISelection> selection;
    nsresult rv = editor->GetSelection(getter_AddRefs(selection));
    if (NS_SUCCEEDED(rv) && selection)
    {
      PRBool isCollapsed;
      rv = selection->GetIsCollapsed(&isCollapsed);
      if (NS_SUCCEEDED(rv) && isCollapsed)
        return editor->DeleteSelection(nsIEditor::eToEndOfLine);
    }
    return editor->Copy();
  }
    
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsCopyOrDeleteCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsCopyOrDeleteCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}

NS_IMETHODIMP
nsPasteCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = PR_FALSE;
  if (aEditor)
    return aEditor->CanPaste(nsIClipboard::kGlobalClipboard, outCmdEnabled);

  return NS_OK;
}


NS_IMETHODIMP
nsPasteCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  if (!aEditor)
    return NS_ERROR_FAILURE;
  
  nsresult rv = NS_OK;
  nsAutoString cmdString(aCommandName);
  if (cmdString.Equals(NS_LITERAL_STRING("cmd_paste")))
    rv = aEditor->Paste(nsIClipboard::kGlobalClipboard);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_pasteQuote")))
  {
    nsCOMPtr<nsIEditorMailSupport> mailEditor = do_QueryInterface(aEditor, &rv);
    if (mailEditor)
      rv = mailEditor->PasteAsQuotation(nsIClipboard::kGlobalClipboard);
  }
    
  return rv;
}

NS_IMETHODIMP 
nsPasteCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsPasteCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}

#pragma mark -

NS_IMETHODIMP
nsInsertTextCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIPlaintextEditor> aEditor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = PR_FALSE;
  if (aEditor)
    return aEditor->CanInsertText(outCmdEnabled);

  return NS_OK;
}

NS_IMETHODIMP
nsInsertTextCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsInsertTextCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsAutoString tString;
  aParams->GetStringValue(COMMAND_NAME, tString);

  nsAutoString value;
  aParams->GetStringValue(NS_LITERAL_STRING("data"), value);

  nsCOMPtr<nsIPlaintextEditor> aEditor = do_QueryInterface(aCommandRefCon);
  if (!aEditor)
    return NS_ERROR_FAILURE;
  
  return aEditor->InsertText(value);
}

NS_IMETHODIMP 
nsInsertTextCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}

#pragma mark -
 
NS_IMETHODIMP
nsDeleteCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = PR_FALSE;
  // we can delete when we can cut
  if (!aEditor)
    return NS_OK;
    
  nsresult rv = NS_OK;
  
  nsAutoString cmdString(aCommandName);

  if (cmdString.Equals(NS_LITERAL_STRING("cmd_delete")))
    rv = aEditor->CanCut(outCmdEnabled);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteCharBackward")))
    *outCmdEnabled = PR_TRUE;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteCharForward")))
    *outCmdEnabled = PR_TRUE;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteWordBackward")))
    *outCmdEnabled = PR_TRUE;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteWordForward")))
    *outCmdEnabled = PR_TRUE;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteToBeginningOfLine")))
    *outCmdEnabled = PR_TRUE;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteToEndOfLine")))
    *outCmdEnabled = PR_TRUE;  

  return rv;
}


NS_IMETHODIMP
nsDeleteCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  if (!aEditor)
    return NS_ERROR_FAILURE;
    
  nsAutoString cmdString(aCommandName);

  nsIEditor::EDirection deleteDir = nsIEditor::eNone;
  
  if (cmdString.Equals(NS_LITERAL_STRING("cmd_delete")))
    deleteDir = nsIEditor::ePrevious;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteCharBackward")))
    deleteDir = nsIEditor::ePrevious;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteCharForward")))
    deleteDir = nsIEditor::eNext;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteWordBackward")))
    deleteDir = nsIEditor::ePreviousWord;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteWordForward")))
    deleteDir = nsIEditor::eNextWord;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteToBeginningOfLine")))
    deleteDir = nsIEditor::eToBeginningOfLine;
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_deleteToEndOfLine")))
    deleteDir = nsIEditor::eToEndOfLine;

  return aEditor->DeleteSelection(deleteDir);
}

NS_IMETHODIMP 
nsDeleteCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsDeleteCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}

NS_IMETHODIMP
nsSelectAllCommand::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = PR_FALSE;
  if (aEditor)
    *outCmdEnabled = PR_TRUE;     // you can always select all

  return NS_OK;
}


NS_IMETHODIMP
nsSelectAllCommand::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  if (aEditor)
    return aEditor->SelectAll();
    
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsSelectAllCommand::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsSelectAllCommand::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}


NS_IMETHODIMP
nsSelectionMoveCommands::IsCommandEnabled(const nsAString & aCommandName, nsISupports *aCommandRefCon, PRBool *outCmdEnabled)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  *outCmdEnabled = PR_FALSE;
  if (!aEditor)
    return NS_ERROR_FAILURE;

  *outCmdEnabled = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP
nsSelectionMoveCommands::DoCommand(const nsAString & aCommandName, nsISupports *aCommandRefCon)
{
  nsCOMPtr<nsIEditor> aEditor = do_QueryInterface(aCommandRefCon);
  if (!aEditor)
    return NS_ERROR_FAILURE;
 
  nsresult rv;
    
  nsCOMPtr<nsISelectionController> selCont;
  rv = aEditor->GetSelectionController(getter_AddRefs(selCont)); 
  if (NS_FAILED(rv))
    return rv;
  if (!selCont)
    return NS_ERROR_FAILURE;
  
  nsAutoString cmdString(aCommandName);
  
  // complete scroll commands
  if (cmdString.Equals(NS_LITERAL_STRING("cmd_scrollTop")))
    return selCont->CompleteScroll(PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_scrollBottom")))
    return selCont->CompleteScroll(PR_TRUE);

  // complete move commands
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_moveTop")))
    return selCont->CompleteMove(PR_FALSE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_moveBottom")))
    return selCont->CompleteMove(PR_TRUE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectTop")))
    return selCont->CompleteMove(PR_FALSE, PR_TRUE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectBottom")))
    return selCont->CompleteMove(PR_TRUE, PR_TRUE);

  // line move commands
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_lineNext")))
    return selCont->LineMove(PR_TRUE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_linePrevious")))
    return selCont->LineMove(PR_FALSE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectLineNext")))
    return selCont->LineMove(PR_TRUE, PR_TRUE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectLinePrevious")))
    return selCont->LineMove(PR_FALSE, PR_TRUE);

  // character move commands
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_charPrevious")))
    return selCont->CharacterMove(PR_FALSE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_charNext")))
    return selCont->CharacterMove(PR_TRUE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectCharPrevious")))
    return selCont->CharacterMove(PR_FALSE, PR_TRUE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectCharNext")))
    return selCont->CharacterMove(PR_TRUE, PR_TRUE);

  // intra line move commands
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_beginLine")))
    return selCont->IntraLineMove(PR_FALSE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_endLine")))
    return selCont->IntraLineMove(PR_TRUE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectBeginLine")))
    return selCont->IntraLineMove(PR_FALSE, PR_TRUE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectEndLine")))
    return selCont->IntraLineMove(PR_TRUE, PR_TRUE);
  
  // word move commands
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_wordPrevious")))
    return selCont->WordMove(PR_FALSE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_wordNext")))
    return selCont->WordMove(PR_TRUE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectWordPrevious")))
    return selCont->WordMove(PR_FALSE, PR_TRUE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectWordNext")))
    return selCont->WordMove(PR_TRUE, PR_TRUE);
  
  // scroll page commands
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_scrollPageUp")))
    return selCont->ScrollPage(PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_scrollPageDown")))
    return selCont->ScrollPage(PR_TRUE);
  
  // scroll line commands
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_scrollLineUp")))
    return selCont->ScrollLine(PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_scrollLineDown")))
    return selCont->ScrollLine(PR_TRUE);
  
  // page move commands
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_movePageUp")))
    return selCont->PageMove(PR_FALSE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_movePageDown")))
    return selCont->PageMove(PR_TRUE, PR_FALSE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectPageUp")))
    return selCont->PageMove(PR_FALSE, PR_TRUE);
  else if (cmdString.Equals(NS_LITERAL_STRING("cmd_selectPageDown")))
    return selCont->PageMove(PR_TRUE, PR_TRUE);
    
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsSelectionMoveCommands::DoCommandParams(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  return DoCommand(tString,aCommandRefCon);
}

NS_IMETHODIMP 
nsSelectionMoveCommands::GetCommandState(nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  nsString tString;
  aParams->GetStringValue(COMMAND_NAME,tString);
  PRBool canUndo;
  IsCommandEnabled(tString, aCommandRefCon, &canUndo);
  return aParams->SetBooleanValue(STATE_ENABLED,canUndo);
}
