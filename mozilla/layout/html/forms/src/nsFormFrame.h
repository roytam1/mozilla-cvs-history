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
#ifndef nsFormFrame_h___
#define nsFormFrame_h___

#include "nsIFormManager.h"
#include "nsBlockFrame.h"
#include "nsVoidArray.h"
#include "nsIFileSpec.h"

class  nsString;
class  nsIContent;
class  nsIFrame;
class  nsIPresContext;
struct nsHTMLReflowState;
class  nsFormControlFrame;
class  nsRadioControlFrame;
class  nsIFormControlFrame;
class  nsIDOMHTMLFormElement;
class nsIDocument;
class nsIPresContext;
class nsFormFrame;
class nsIUnicodeEncoder;

class nsFormFrame : public nsBlockFrame, 
                    public nsIFormManager
{
public:
  nsFormFrame();
  virtual ~nsFormFrame();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  // nsIFormManager

  NS_IMETHOD OnReset();

  NS_IMETHOD OnSubmit(nsIPresContext* aPresContext, nsIFrame* aFrame);

  // other methods 

  void OnRadioChecked(nsRadioControlFrame& aRadio, PRBool aChecked = PR_TRUE); 
    
  void AddFormControlFrame(nsIFormControlFrame& aFrame);
  static void AddFormControlFrame(nsIPresContext& aPresContext, nsIFrame& aFrame);

  PRBool CanSubmit(nsFormControlFrame& aFrame);

  NS_IMETHOD GetMethod(PRInt32* aMethod);
  NS_IMETHOD GetEnctype(PRInt32* aEnctype);
  NS_IMETHOD GetTarget(nsString* aTarget);
  NS_IMETHOD GetAction(nsString* aAction);

  // static helper functions for nsIFormControls
  
  static PRBool GetDisabled(nsIFrame* aChildFrame, nsIContent* aContent = 0);
  static PRBool GetReadonly(nsIFrame* aChildFrame, nsIContent* aContent = 0);
  static nsresult GetName(nsIFrame* aChildFrame, nsString& aName, nsIContent* aContent = 0);
  static nsresult GetValue(nsIFrame* aChildFrame, nsString& aValue, nsIContent* aContent = 0);
  static void StyleChangeReflow(nsIPresContext* aPresContext,
                                nsIFrame* aFrame);

protected:
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);
  void RemoveRadioGroups();
  nsresult ProcessAsURLEncoded(PRBool aIsPost, nsString& aData, nsIFormControlFrame* aFrame);
  nsresult ProcessAsMultipart(nsIFileSpec*& aMultipartDataFile, nsIFormControlFrame* aFrame);
  static const char* GetFileNameWithinPath(char* aPathName);
  nsresult GetContentType(char* aPathName, char** aContentType);
  
  // i18n helper routines
  nsString* URLEncode(nsString& aString, nsIUnicodeEncoder* encoder);
  char* UnicodeToNewBytes(const PRUnichar* aSrc, PRUint32 aLen, nsIUnicodeEncoder* encoder);

  NS_IMETHOD GetEncoder(nsIUnicodeEncoder** encoder);
  void GetSubmitCharset(nsString& oCharset);

  nsVoidArray          mFormControls;
  nsVoidArray          mRadioGroups;
  nsIFormControlFrame* mTextSubmitter;
};

#endif // nsFormFrame_h___
