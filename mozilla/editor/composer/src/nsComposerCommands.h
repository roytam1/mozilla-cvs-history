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
 * Copyright (C) 1998-2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *    Ryan Cassin (rcassin@supernova.org)
 *   
 */

#ifndef nsComposerCommands_h_
#define nsComposerCommands_h_

#include "nsIControllerCommand.h"

// This is a virtual base class for commands registered with the composer controller.
// Note that such commands are instantiated once per composer, so can store state.
// Also note that IsCommandEnabled can be called with an editorShell that may not
// have an editor yet (because the document is loading). Most commands will want
// to return false in this case.
// Don't hold on to any references to the editorShell, editor, or document from
// your command. This will cause leaks. Also, be aware that the document the
// editor is editing can change under you (if the user Reverts the file, for
// instance).
class nsBaseComposerCommand : public nsIControllerCommand
{
public:

              nsBaseComposerCommand();
  virtual     ~nsBaseComposerCommand() {}
    
  NS_DECL_ISUPPORTS
    
  NS_IMETHOD  IsCommandEnabled(const PRUnichar *aCommand, nsISupports* refCon, PRBool *_retval) = 0;
  NS_IMETHOD  DoCommand(const PRUnichar *aCommand, nsISupports* refCon) = 0;

protected:

  // utility methods to get/set the "state" attribute on the command node in the XUL
  nsresult    GetInterfaceNode(const PRUnichar* nodeID, nsIEditorShell* editorShell, nsIDOMElement **outNode);
  
  nsresult    GetCommandNodeState(const PRUnichar *aCommand, nsIEditorShell* editorShell, nsString& outNodeState);
  nsresult    SetCommandNodeState(const PRUnichar *aCommand, nsIEditorShell* editorShell, const nsString& inNodeState);

};



#define NS_DECL_COMPOSER_COMMAND(_cmd)                  \
class _cmd : public nsBaseComposerCommand               \
{                                                       \
public:                                                 \
  NS_DECL_NSICONTROLLERCOMMAND                          \
};

// virtual base class for commands that need to save and update Boolean state (like styles etc)
class nsBaseStateUpdatingCommand : public nsBaseComposerCommand,
                                   public nsIStateUpdatingControllerCommand
{
public:

              nsBaseStateUpdatingCommand(const char* aTagName);
  virtual     ~nsBaseStateUpdatingCommand();
    
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSICONTROLLERCOMMAND
  NS_DECL_NSISTATEUPDATINGCONTROLLERCOMMAND

protected:

  // get the current state (on or off) for this style or block format
  virtual nsresult  GetCurrentState(nsIEditorShell *aEditorShell, const char* aTagName, PRBool& outStateSet) = 0;
  
  // add/remove the style
  virtual nsresult  ToggleState(nsIEditorShell *aEditorShell, const char* aTagName) = 0;

protected:

  const char* mTagName;
  const char* mAttributeName;
  
  PRPackedBool  mGotState;    // do we know the state yet?
  PRPackedBool  mState;       // is this style "on" ?
};


// Shared class for the various style updating commands like bold, italics etc.
// Suitable for commands whose state is either 'on' or 'off'.
class nsStyleUpdatingCommand : public nsBaseStateUpdatingCommand
{
public:

            nsStyleUpdatingCommand(const char* aTagName);
           
protected:

  // get the current state (on or off) for this style or block format
  virtual nsresult  GetCurrentState(nsIEditorShell *aEditorShell, const char* aTagName, PRBool& outStateSet);
  
  // add/remove the style
  virtual nsresult  ToggleState(nsIEditorShell *aEditorShell, const char* aTagName);
    
};


class nsListCommand : public nsBaseStateUpdatingCommand
{
public:

            nsListCommand(const char* aTagName);

protected:
  
  // get the current state (on or off) for this style or block format
  virtual nsresult  GetCurrentState(nsIEditorShell *aEditorShell, const char* aTagName, PRBool& outStateSet);
  
  // add/remove the style
  virtual nsresult  ToggleState(nsIEditorShell *aEditorShell, const char* aTagName);
};

class nsListItemCommand : public nsBaseStateUpdatingCommand
{
public:

            nsListItemCommand(const char* aTagName);

protected:
  
  // get the current state (on or off) for this style or block format
  virtual nsresult  GetCurrentState(nsIEditorShell *aEditorShell, const char* aTagName, PRBool& outStateSet);
  
  // add/remove the style
  virtual nsresult  ToggleState(nsIEditorShell *aEditorShell, const char* aTagName);
};

// Base class for commands whose state consists of a string (e.g. para format)
class nsMultiStateCommand : public nsBaseComposerCommand,
                            public nsIStateUpdatingControllerCommand
{
public:
  
                   nsMultiStateCommand();
  virtual          ~nsMultiStateCommand();
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICONTROLLERCOMMAND
  NS_DECL_NSISTATEUPDATINGCONTROLLERCOMMAND

protected:

  virtual nsresult GetCurrentState(nsIEditorShell *aEditorShell, nsString& outStateString, PRBool& outMixed) = 0;
  virtual nsresult SetState(nsIEditorShell *aEditorShell, nsString& newState) = 0;
  
protected:

  PRPackedBool  mGotState;
  nsString      mStateString;

};


class nsParagraphStateCommand : public nsMultiStateCommand
{
public:
                   nsParagraphStateCommand();

protected:

  virtual nsresult GetCurrentState(nsIEditorShell *aEditorShell, nsString& outStateString, PRBool& outMixed);
  virtual nsresult SetState(nsIEditorShell *aEditorShell, nsString& newState);
};

class nsFontFaceStateCommand : public nsMultiStateCommand
{
public:
                   nsFontFaceStateCommand();

protected:

  virtual nsresult GetCurrentState(nsIEditorShell *aEditorShell, nsString& outStateString, PRBool& outMixed);
  virtual nsresult SetState(nsIEditorShell *aEditorShell, nsString& newState);
};




// composer commands

NS_DECL_COMPOSER_COMMAND(nsCloseCommand)
NS_DECL_COMPOSER_COMMAND(nsPrintingCommands)

// Generic commands


// File menu
NS_DECL_COMPOSER_COMMAND(nsNewCommands)   // handles 'new' anything


// Edit menu
NS_DECL_COMPOSER_COMMAND(nsPasteQuotationCommand)

// Block transformations
NS_DECL_COMPOSER_COMMAND(nsIndentCommand)
NS_DECL_COMPOSER_COMMAND(nsOutdentCommand)

NS_DECL_COMPOSER_COMMAND(nsAlignCommand)
NS_DECL_COMPOSER_COMMAND(nsRemoveListCommand)
NS_DECL_COMPOSER_COMMAND(nsRemoveStylesCommand)
NS_DECL_COMPOSER_COMMAND(nsIncreaseFontSizeCommand)
NS_DECL_COMPOSER_COMMAND(nsDecreaseFontSizeCommand)


#endif // nsComposerCommands_h_
