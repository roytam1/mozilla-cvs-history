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
 *   Simon Fraser <sfraser@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Ryan Cassin <rcassin@supernova.org>
 */

#include "nsIComponentManager.h"

#include "nsAString.h"
#include "nsIEditor.h"
#include "nsComposerCommands.h"
#include "nsComposerController.h"


nsComposerController::nsComposerController()
{
  NS_INIT_REFCNT();  
}

nsComposerController::~nsComposerController()
{
}

NS_IMPL_ISUPPORTS3(nsComposerController, nsIController, nsIEditorController, nsIInterfaceRequestor);

NS_IMETHODIMP
nsComposerController::Init(nsISupports *aCommandRefCon)
{
  nsresult  rv;
 
  // get our ref to the singleton command manager
  rv = GetComposerCommandManager(getter_AddRefs(mCommandManager));  
  if (NS_FAILED(rv)) return rv;  

  mCommandRefCon = aCommandRefCon;     // no addref  
  
  mCommandManager = do_CreateInstance("@mozilla.org/content/controller-command-manager;1", &rv);
  if (NS_FAILED(rv)) return rv;

  // register the commands.
  rv = nsComposerController::RegisterComposerCommands(mCommandManager);
  if (NS_FAILED(rv)) return rv;

  return NS_OK;
}

NS_IMETHODIMP
nsComposerController::SetCommandRefCon(nsISupports *aCommandRefCon)
{
  mCommandRefCon = aCommandRefCon;     // no addref  
  return NS_OK;
}

NS_IMETHODIMP
nsComposerController::GetInterface(const nsIID & aIID, void * *result)
{
  NS_ENSURE_ARG_POINTER(result);

  if (NS_SUCCEEDED(QueryInterface(aIID, result)))
    return NS_OK;
  
  if (mCommandManager && aIID.Equals(NS_GET_IID(nsIControllerCommandManager)))
    return mCommandManager->QueryInterface(aIID, result);
    
  return NS_NOINTERFACE;
}

#define NS_REGISTER_ONE_COMMAND(_cmdClass, _cmdName)                    \
  {                                                                     \
    _cmdClass* theCmd;                                                  \
    NS_NEWXPCOM(theCmd, _cmdClass);                                     \
    if (!theCmd) return NS_ERROR_OUT_OF_MEMORY;                         \
    rv = inCommandManager->RegisterCommand(NS_LITERAL_STRING(_cmdName), \
                       NS_STATIC_CAST(nsIControllerCommand *, theCmd)); \
  }

#define NS_REGISTER_FIRST_COMMAND(_cmdClass, _cmdName)                  \
  {                                                                     \
    _cmdClass* theCmd;                                                  \
    NS_NEWXPCOM(theCmd, _cmdClass);                                     \
    if (!theCmd) return NS_ERROR_OUT_OF_MEMORY;                         \
    rv = inCommandManager->RegisterCommand(NS_LITERAL_STRING(_cmdName), \
                       NS_STATIC_CAST(nsIControllerCommand *, theCmd));

#define NS_REGISTER_NEXT_COMMAND(_cmdClass, _cmdName)                   \
    rv = inCommandManager->RegisterCommand(NS_LITERAL_STRING(_cmdName), \
                        NS_STATIC_CAST(nsIControllerCommand *, theCmd));

#define NS_REGISTER_LAST_COMMAND(_cmdClass, _cmdName)                   \
    rv = inCommandManager->RegisterCommand(NS_LITERAL_STRING(_cmdName), \
                       NS_STATIC_CAST(nsIControllerCommand *, theCmd)); \
  }

#define NS_REGISTER_STYLE_COMMAND(_cmdClass, _cmdName, _styleTag)       \
  {                                                                     \
    _cmdClass* theCmd = new _cmdClass(_styleTag);                       \
    if (!theCmd) return NS_ERROR_OUT_OF_MEMORY;                         \
    rv = inCommandManager->RegisterCommand(NS_LITERAL_STRING(_cmdName), \
                       NS_STATIC_CAST(nsIControllerCommand *, theCmd)); \
  }
  

// static
nsresult nsComposerController::RegisterComposerCommands(nsIControllerCommandManager *inCommandManager)
{
  nsresult rv;
  
  // File menu
/*
  NS_REGISTER_FIRST_COMMAND(nsPrintingCommands, "cmd_print");
  NS_REGISTER_NEXT_COMMAND(nsPrintingCommands, "cmd_printSetup");
  NS_REGISTER_NEXT_COMMAND(nsPrintingCommands,"cmd_print_button");
  NS_REGISTER_LAST_COMMAND(nsPrintingCommands, "cmd_printPreview");
*/  
  // Edit menu
  NS_REGISTER_ONE_COMMAND(nsPasteQuotationCommand, "cmd_pasteQuote");

  // indent/outdent
  NS_REGISTER_ONE_COMMAND(nsIndentCommand, "cmd_indent");
  NS_REGISTER_ONE_COMMAND(nsOutdentCommand, "cmd_outdent");

  // Styles
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_bold", "b");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_italic", "i");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_underline", "u");

  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_strikethrough", "strike");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_superscript", "sup");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_subscript", "sub");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_nobreak", "nobr");

  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_em", "em");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_strong", "strong");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_cite", "cite");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_abbr", "abbr");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_acronym", "acronym");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_code", "code");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_samp", "samp");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_var", "var");
  
  // lists
  NS_REGISTER_STYLE_COMMAND(nsListCommand,     "cmd_ol", "ol");
  NS_REGISTER_STYLE_COMMAND(nsListCommand,     "cmd_ul", "ul");
  NS_REGISTER_STYLE_COMMAND(nsListItemCommand, "cmd_dt", "dt");
  NS_REGISTER_STYLE_COMMAND(nsListItemCommand, "cmd_dd", "dd");
  NS_REGISTER_ONE_COMMAND(nsRemoveListCommand, "cmd_removeList");

  // format stuff
  NS_REGISTER_ONE_COMMAND(nsParagraphStateCommand,       "cmd_paragraphState");
  NS_REGISTER_ONE_COMMAND(nsFontFaceStateCommand,        "cmd_fontFace");
  NS_REGISTER_ONE_COMMAND(nsFontColorStateCommand,       "cmd_fontColor");
  NS_REGISTER_ONE_COMMAND(nsBackgroundColorStateCommand, "cmd_backgroundColor");

  NS_REGISTER_ONE_COMMAND(nsAlignCommand, "cmd_align");
  NS_REGISTER_ONE_COMMAND(nsRemoveStylesCommand, "cmd_removeStyles");

  NS_REGISTER_ONE_COMMAND(nsIncreaseFontSizeCommand, "cmd_increaseFont");
  NS_REGISTER_ONE_COMMAND(nsDecreaseFontSizeCommand, "cmd_decreaseFont");

  return NS_OK;
}

/* =======================================================================
 * nsIController
 * ======================================================================= */

NS_IMETHODIMP
nsComposerController::IsCommandEnabled(const nsAReadableString & aCommand,
                                       PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  return mCommandManager->IsCommandEnabled(aCommand, mCommandRefCon, aResult);
}

NS_IMETHODIMP
nsComposerController::SupportsCommand(const nsAReadableString & aCommand,
                                      PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  return mCommandManager->SupportsCommand(aCommand, mCommandRefCon, aResult);
}

NS_IMETHODIMP
nsComposerController::DoCommand(const nsAReadableString & aCommand)
{
  return mCommandManager->DoCommand(aCommand, mCommandRefCon);
}

NS_IMETHODIMP
nsComposerController::OnEvent(const nsAReadableString & aEventName)
{
  return NS_OK;
}

nsWeakPtr nsComposerController::sComposerCommandManager = NULL;

// static
nsresult nsComposerController::GetComposerCommandManager(nsIControllerCommandManager* *outCommandManager)
{
  NS_ENSURE_ARG_POINTER(outCommandManager);

  nsCOMPtr<nsIControllerCommandManager> cmdManager = do_QueryReferent(sComposerCommandManager);
  if (!cmdManager)
  {
    nsresult rv;
    cmdManager = do_CreateInstance("@mozilla.org/content/controller-command-manager;1", &rv);
    if (NS_FAILED(rv)) return rv;

    // register the commands. This just happens once per instance
    rv = nsComposerController::RegisterComposerCommands(cmdManager);
    if (NS_FAILED(rv)) return rv;

    // save the singleton in our static weak reference
    sComposerCommandManager = getter_AddRefs(NS_GetWeakReference(cmdManager, &rv));
    if (NS_FAILED(rv))  return rv;
  }

  NS_ADDREF(*outCommandManager = cmdManager);
  return NS_OK;
}

