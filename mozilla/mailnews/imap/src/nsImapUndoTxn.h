/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1998, 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsImapUndoTxn_h__
#define nsImapUndoTxn_h__

#include "nsIMsgFolder.h"
#include "nsImapCore.h"
#include "nsIImapService.h"
#include "nsIUrlListener.h"
#include "nsIEventQueue.h"
#include "nsMsgTxn.h"
#include "nsMsgKeyArray.h"
#include "nsCOMPtr.h"

#define NS_IMAPMOVECOPYMSGTXN_IID \
{ /* 51c925b0-208e-11d3-abea-00805f8ac968 */ \
	0x51c925b0, 0x208e, 0x11d3, \
    { 0xab, 0xea, 0x00, 0x80, 0x5f, 0x8a, 0xc9, 0x68 } }

class nsImapMoveCopyMsgTxn : public nsMsgTxn
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IMAPMOVECOPYMSGTXN_IID)

    nsImapMoveCopyMsgTxn();
    nsImapMoveCopyMsgTxn(nsIMsgFolder* srcFolder, nsMsgKeyArray* srcKeyArray,
                         const char* srcMsgIdString, nsIMsgFolder* dstFolder,
                         PRBool idsAreUids, PRBool isMove, 
                         nsIEventQueue *eventQueue, 
                         nsIUrlListener *urlListener);
    virtual ~nsImapMoveCopyMsgTxn();

    NS_DECL_ISUPPORTS_INHERITED 

    NS_IMETHOD Undo(void);
    NS_IMETHOD Redo(void);

    // helper
    nsresult SetCopyResponseUid(nsMsgKeyArray* keyArray,
                                const char *msgIdString);
    nsresult GetSrcKeyArray(nsMsgKeyArray& srcKeyArray);
    nsresult GetDstKeyArray(nsMsgKeyArray& dstKeyArray);
    nsresult AddDstKey(nsMsgKey aKey);
    nsresult UndoMailboxDelete();
    nsresult RedoMailboxDelete();
    nsresult Init(nsIMsgFolder* srcFolder, nsMsgKeyArray* srcKeyArray,
                  const char* srcMsgIdString, nsIMsgFolder* dstFolder,
                  PRBool idsAreUids, PRBool isMove, 
                  nsIEventQueue *eventQueue, 
                  nsIUrlListener *urlListener);

private:

    nsCOMPtr<nsIMsgFolder> m_srcFolder;
    nsMsgKeyArray m_srcKeyArray;
    nsCString m_srcMsgIdString;
    nsCOMPtr<nsIMsgFolder> m_dstFolder;
    nsMsgKeyArray m_dstKeyArray;
    nsCString m_dstMsgIdString;
    nsCOMPtr<nsIEventQueue> m_eventQueue;
    nsCOMPtr<nsIUrlListener> m_urlListener;
    nsString m_undoString;
    nsString m_redoString;
    PRBool m_idsAreUids;
    PRBool m_isMove;
    PRBool m_srcIsPop3;
    nsUInt32Array m_srcSizeArray;
};

#endif
