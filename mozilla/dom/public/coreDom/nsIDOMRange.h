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
/* AUTO-GENERATED. DO NOT EDIT!!! */

#ifndef nsIDOMRange_h__
#define nsIDOMRange_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"

class nsIDOMNode;
class nsIDOMDocumentFragment;
class nsIDOMRange;

#define NS_IDOMRANGE_IID \
 { 0xa6cf90ce, 0x15b3, 0x11d2, \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class nsIDOMRange : public nsISupports {
public:

  NS_IMETHOD    GetIsPositioned(PRBool* aIsPositioned)=0;
  NS_IMETHOD    SetIsPositioned(PRBool aIsPositioned)=0;

  NS_IMETHOD    GetStartParent(nsIDOMNode** aStartParent)=0;
  NS_IMETHOD    SetStartParent(nsIDOMNode* aStartParent)=0;

  NS_IMETHOD    GetStartOffset(PRInt32* aStartOffset)=0;
  NS_IMETHOD    SetStartOffset(PRInt32 aStartOffset)=0;

  NS_IMETHOD    GetEndParent(nsIDOMNode** aEndParent)=0;
  NS_IMETHOD    SetEndParent(nsIDOMNode* aEndParent)=0;

  NS_IMETHOD    GetEndOffset(PRInt32* aEndOffset)=0;
  NS_IMETHOD    SetEndOffset(PRInt32 aEndOffset)=0;

  NS_IMETHOD    GetIsCollapsed(PRBool* aIsCollapsed)=0;
  NS_IMETHOD    SetIsCollapsed(PRBool aIsCollapsed)=0;

  NS_IMETHOD    GetCommonParent(nsIDOMNode** aCommonParent)=0;
  NS_IMETHOD    SetCommonParent(nsIDOMNode* aCommonParent)=0;

  NS_IMETHOD    SetStart(nsIDOMNode* aParent, PRInt32 aOffset)=0;

  NS_IMETHOD    SetEnd(nsIDOMNode* aParent, PRInt32 aOffset)=0;

  NS_IMETHOD    Collapse(PRBool aToStart)=0;

  NS_IMETHOD    Unposition()=0;

  NS_IMETHOD    SelectNode(nsIDOMNode* aN)=0;

  NS_IMETHOD    SelectNodeContents(nsIDOMNode* aN)=0;

  NS_IMETHOD    DeleteContents()=0;

  NS_IMETHOD    ExtractContents(nsIDOMDocumentFragment** aReturn)=0;

  NS_IMETHOD    CopyContents(nsIDOMDocumentFragment** aReturn)=0;

  NS_IMETHOD    InsertNode(nsIDOMNode* aN)=0;

  NS_IMETHOD    SurroundContents(nsIDOMNode* aN)=0;

  NS_IMETHOD    Clone(nsIDOMRange** aReturn)=0;

  NS_IMETHOD    ToString(nsString& aReturn)=0;
};


#define NS_DECL_IDOMRANGE   \
  NS_IMETHOD    GetIsPositioned(PRBool* aIsPositioned);  \
  NS_IMETHOD    SetIsPositioned(PRBool aIsPositioned);  \
  NS_IMETHOD    GetStartParent(nsIDOMNode** aStartParent);  \
  NS_IMETHOD    SetStartParent(nsIDOMNode* aStartParent);  \
  NS_IMETHOD    GetStartOffset(PRInt32* aStartOffset);  \
  NS_IMETHOD    SetStartOffset(PRInt32 aStartOffset);  \
  NS_IMETHOD    GetEndParent(nsIDOMNode** aEndParent);  \
  NS_IMETHOD    SetEndParent(nsIDOMNode* aEndParent);  \
  NS_IMETHOD    GetEndOffset(PRInt32* aEndOffset);  \
  NS_IMETHOD    SetEndOffset(PRInt32 aEndOffset);  \
  NS_IMETHOD    GetIsCollapsed(PRBool* aIsCollapsed);  \
  NS_IMETHOD    SetIsCollapsed(PRBool aIsCollapsed);  \
  NS_IMETHOD    GetCommonParent(nsIDOMNode** aCommonParent);  \
  NS_IMETHOD    SetCommonParent(nsIDOMNode* aCommonParent);  \
  NS_IMETHOD    SetStart(nsIDOMNode* aParent, PRInt32 aOffset);  \
  NS_IMETHOD    SetEnd(nsIDOMNode* aParent, PRInt32 aOffset);  \
  NS_IMETHOD    Collapse(PRBool aToStart);  \
  NS_IMETHOD    Unposition();  \
  NS_IMETHOD    SelectNode(nsIDOMNode* aN);  \
  NS_IMETHOD    SelectNodeContents(nsIDOMNode* aN);  \
  NS_IMETHOD    DeleteContents();  \
  NS_IMETHOD    ExtractContents(nsIDOMDocumentFragment** aReturn);  \
  NS_IMETHOD    CopyContents(nsIDOMDocumentFragment** aReturn);  \
  NS_IMETHOD    InsertNode(nsIDOMNode* aN);  \
  NS_IMETHOD    SurroundContents(nsIDOMNode* aN);  \
  NS_IMETHOD    Clone(nsIDOMRange** aReturn);  \
  NS_IMETHOD    ToString(nsString& aReturn);  \



#define NS_FORWARD_IDOMRANGE(_to)  \
  NS_IMETHOD    GetIsPositioned(PRBool* aIsPositioned) { return _to##GetIsPositioned(aIsPositioned); } \
  NS_IMETHOD    SetIsPositioned(PRBool aIsPositioned) { return _to##SetIsPositioned(aIsPositioned); } \
  NS_IMETHOD    GetStartParent(nsIDOMNode** aStartParent) { return _to##GetStartParent(aStartParent); } \
  NS_IMETHOD    SetStartParent(nsIDOMNode* aStartParent) { return _to##SetStartParent(aStartParent); } \
  NS_IMETHOD    GetStartOffset(PRInt32* aStartOffset) { return _to##GetStartOffset(aStartOffset); } \
  NS_IMETHOD    SetStartOffset(PRInt32 aStartOffset) { return _to##SetStartOffset(aStartOffset); } \
  NS_IMETHOD    GetEndParent(nsIDOMNode** aEndParent) { return _to##GetEndParent(aEndParent); } \
  NS_IMETHOD    SetEndParent(nsIDOMNode* aEndParent) { return _to##SetEndParent(aEndParent); } \
  NS_IMETHOD    GetEndOffset(PRInt32* aEndOffset) { return _to##GetEndOffset(aEndOffset); } \
  NS_IMETHOD    SetEndOffset(PRInt32 aEndOffset) { return _to##SetEndOffset(aEndOffset); } \
  NS_IMETHOD    GetIsCollapsed(PRBool* aIsCollapsed) { return _to##GetIsCollapsed(aIsCollapsed); } \
  NS_IMETHOD    SetIsCollapsed(PRBool aIsCollapsed) { return _to##SetIsCollapsed(aIsCollapsed); } \
  NS_IMETHOD    GetCommonParent(nsIDOMNode** aCommonParent) { return _to##GetCommonParent(aCommonParent); } \
  NS_IMETHOD    SetCommonParent(nsIDOMNode* aCommonParent) { return _to##SetCommonParent(aCommonParent); } \
  NS_IMETHOD    SetStart(nsIDOMNode* aParent, PRInt32 aOffset) { return _to##SetStart(aParent, aOffset); }  \
  NS_IMETHOD    SetEnd(nsIDOMNode* aParent, PRInt32 aOffset) { return _to##SetEnd(aParent, aOffset); }  \
  NS_IMETHOD    Collapse(PRBool aToStart) { return _to##Collapse(aToStart); }  \
  NS_IMETHOD    Unposition() { return _to##Unposition(); }  \
  NS_IMETHOD    SelectNode(nsIDOMNode* aN) { return _to##SelectNode(aN); }  \
  NS_IMETHOD    SelectNodeContents(nsIDOMNode* aN) { return _to##SelectNodeContents(aN); }  \
  NS_IMETHOD    DeleteContents() { return _to##DeleteContents(); }  \
  NS_IMETHOD    ExtractContents(nsIDOMDocumentFragment** aReturn) { return _to##ExtractContents(aReturn); }  \
  NS_IMETHOD    CopyContents(nsIDOMDocumentFragment** aReturn) { return _to##CopyContents(aReturn); }  \
  NS_IMETHOD    InsertNode(nsIDOMNode* aN) { return _to##InsertNode(aN); }  \
  NS_IMETHOD    SurroundContents(nsIDOMNode* aN) { return _to##SurroundContents(aN); }  \
  NS_IMETHOD    Clone(nsIDOMRange** aReturn) { return _to##Clone(aReturn); }  \
  NS_IMETHOD    ToString(nsString& aReturn) { return _to##ToString(aReturn); }  \


extern nsresult NS_InitRangeClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptRange(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMRange_h__
