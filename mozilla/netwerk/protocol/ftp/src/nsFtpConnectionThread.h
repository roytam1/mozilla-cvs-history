/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef __nsftpconnectionthread__h_
#define __nsftpconnectionthread__h_

#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsIRequest.h"
#include "nsISocketTransportService.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIOutputStream.h"
#include "nsIURI.h"
#include "prtime.h"
#include "nsString2.h"
#include "nsIEventQueue.h"
#include "nsHashtable.h"
#include "nsIChannel.h"
#include "nsIFTPContext.h"
#include "nsIConnectionCache.h"
#include "nsConnectionCacheObj.h"
#include "nsIProtocolHandler.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

// ftp server types
#define FTP_GENERIC_TYPE     0
#define FTP_UNIX_TYPE        1
#define FTP_DCTS_TYPE        2
#define FTP_NCSA_TYPE        3
#define FTP_PETER_LEWIS_TYPE 4
#define FTP_MACHTEN_TYPE     5
#define FTP_CMS_TYPE         6
#define FTP_TCPC_TYPE        7
#define FTP_VMS_TYPE         8
#define FTP_NT_TYPE          9
#define FTP_WEBSTAR_TYPE     10

// ftp states
typedef enum _FTP_STATE {
///////////////////////
//// Internal states
///////////////////////
    FTP_READ_BUF,
    FTP_ERROR,
    FTP_COMPLETE,

///////////////////////
//// Command channel connection setup states
///////////////////////
    FTP_S_USER,        // send username
    FTP_R_USER,
    FTP_S_PASS,        // send password
    FTP_R_PASS,
    FTP_S_SYST,        // send system (interrogates server)
    FTP_R_SYST,
    FTP_S_ACCT,        // send account
    FTP_R_ACCT,
    FTP_S_MACB,
    FTP_R_MACB,
    FTP_S_PWD ,        // send parent working directory (pwd)
    FTP_R_PWD ,
    FTP_S_DEL_FILE, // send delete file
    FTP_R_DEL_FILE,
    FTP_S_DEL_DIR , // send delete directory
    FTP_R_DEL_DIR ,
    FTP_S_MKDIR,    // send mkdir
    FTP_R_MKDIR,
    FTP_S_MODE,     // send ASCII or BINARY
    FTP_R_MODE,
    FTP_S_CWD,      // send change working directory
    FTP_R_CWD,
    FTP_S_SIZE,     // send size
    FTP_R_SIZE,
    FTP_S_PUT,      // send STOR to upload the file
    FTP_R_PUT,
    FTP_S_RETR,     // send retrieve to download the file
    FTP_R_RETR,
    FTP_S_MDTM,     // send MDTM to get time information
    FTP_R_MDTM,
    FTP_S_LIST,     // send LIST or NLST (server dependent) to get a dir listing
    FTP_R_LIST,
    FTP_S_TYPE,      // send TYPE to indicate what type of file will be transfered
    FTP_R_TYPE,

///////////////////////
//// Data channel connection setup states
///////////////////////
    FTP_S_PASV,     // send passsive
    FTP_R_PASV
} FTP_STATE;

// higher level ftp actions
typedef enum _FTP_ACTION {
    GET,
    PUT,
    MKDIR,
    DEL
} FTP_ACTION;

class nsFtpConnectionThread : public nsIRunnable,
                              public nsIRequest {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE
    NS_DECL_NSIREQUEST

    nsFtpConnectionThread();
    virtual ~nsFtpConnectionThread();

    nsresult Init(nsIEventQueue* aFTPEventQ,
                  nsIURI* aUrl,
                  nsIEventQueue* aEventQ,
                  nsIProtocolHandler* aHandler,
                  nsIChannel* channel,
                  nsISupports* ctxt);
    nsresult Process();

    // user level setup
    nsresult SetAction(FTP_ACTION aAction);

private:

    ///////////////////////////////////
    // STATE METHODS
    ///////////////////////////////////
    nsresult        S_user();
    FTP_STATE       R_user();
    nsresult        S_pass();
    FTP_STATE       R_pass();
    nsresult        S_syst();
    FTP_STATE       R_syst();
    nsresult        S_acct();
    FTP_STATE       R_acct();
    nsresult        S_macb();
    FTP_STATE       R_macb();
    nsresult        S_pwd();
    FTP_STATE       R_pwd();
    nsresult        S_mode();
    FTP_STATE       R_mode();
    nsresult        S_cwd();
    FTP_STATE       R_cwd();
    nsresult        S_size();
    FTP_STATE       R_size();
    nsresult        S_mdtm();
    FTP_STATE       R_mdtm();
    nsresult        S_list();
    FTP_STATE       R_list();
    nsresult        S_retr();
    FTP_STATE       R_retr();

    nsresult        S_pasv();
    FTP_STATE       R_pasv();
    nsresult        S_del_file();
    FTP_STATE       R_del_file();
    nsresult        S_del_dir();
    FTP_STATE       R_del_dir();
    nsresult        S_mkdir();
    FTP_STATE       R_mkdir();
    ///////////////////////////////////
    // END: STATE METHODS
    ///////////////////////////////////

    nsresult StopProcessing();
    void SetSystInternals(void);
    FTP_STATE FindActionState(void);
    FTP_STATE FindGetState(void);
    nsresult MapResultCodeToString(nsresult aResultCode, PRUnichar* *aOutMsg);
    void SetDirMIMEType(nsString& aString);

    // Private members

    nsCOMPtr<nsIEventQueue> mFTPEventQueue;     // the eventq for this thread.
    nsCOMPtr<nsIURI>    mURL;

    FTP_STATE           mState;             // the current state
    FTP_STATE           mNextState;         // the next state
    FTP_ACTION          mAction;            // the higher level action (GET/PUT)

    nsISocketTransportService *mSTS;        // the socket transport service;

    nsCOMPtr<nsIChannel> mCPipe;            // the command channel transport
    nsCOMPtr<nsIChannel> mDPipe;            // the data channel transport

    nsCOMPtr<nsIOutputStream>    mCOutStream;        // command channel output
    nsCOMPtr<nsIInputStream>     mCInStream;         // command channel input

    nsCOMPtr<nsIOutputStream>    mDOutStream;        // data channel output
    nsCOMPtr<nsIInputStream>     mDInStream;         // data channel input

    PRInt32             mResponseCode;      // the last command response code.
    nsCAutoString       mResponseMsg;       // the last command response text
    nsString2           mUsername;
    nsString2           mPassword;
    nsString2           mFilename;          // url filename (if any)
    PRUint32            mLength;            // length of the file
    PRTime              mLastModified;      // last modified time for file

// these members should be hung off of a specific transport connection
    PRInt32             mServerType;
    PRBool              mPasv;
    PRBool              mList;              // use LIST instead of NLST
    nsCAutoString       mCwd;               // Our current working dir.
    nsCAutoString       mCwdAttempt;        // the dir we're trying to get into.
// end "these ...."

    nsCAutoString       mCacheKey;         // the key into the cache hash.

    PRBool              mConnected;
    PRBool              mUseDefaultPath;    // use PWD to figure out path
    PRBool              mUsePasv;           // use a passive data connection.
    PRBool              mDirectory;         // this url is a directory
    PRBool              mBin;               // transfer mode (ascii or binary)
    PRBool              mContinueRead;      // continue digesting a multi-line reponse
    PRBool              mResetMode;         // have we reset the mode to ascii
    PRBool              mAnonymous;         // try connecting anonymous (default)
    PRBool              mRetryPass;         // retrying the password
    PRBool              mCachedConn;        // is this connection from the cache
    PRBool              mSentStart;         // have we sent an OnStartRequest() notification
    nsresult            mInternalError;     // represents internal state errors

    nsCOMPtr<nsIStreamListener>     mListener;          // the listener we want to call
                                                        // during our event firing.
    nsCOMPtr<nsIStreamListener>     mSyncListener;      // a syncronous version of our listener

    nsCOMPtr<nsIChannel>            mChannel;
    nsCOMPtr<nsISupports>           mContext;
    nsIFTPContext*                  mFTPContext;        // FTP channel specific context.
    nsCOMPtr<nsIConnectionCache>    mConnCache;         // the nsISupports proxy ptr to the FTP proto handler
    nsConnectionCacheObj* mConn;            // The cached connection.
    PRBool              mKeepRunning;       // thread event loop boolean

    nsString2           mContentType;       // the content type of the data we're dealing w/.
    nsXPIDLCString      mURLSpec;
};

#define NS_FTP_BUFFER_READ_SIZE             (8*1024)
#define NS_FTP_BUFFER_WRITE_SIZE            (8*1024)

#endif //__nsftpconnectionthread__h_
