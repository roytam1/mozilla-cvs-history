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

#include "nsIThread.h"
#include "nsISocketTransportService.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIOutputStream.h"
#include "nsIUrl.h"

#include "nsString2.h"
#include "plevent.h"

#include "time.h" // XXX should probably be using PRTime stuff


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
    FTP_S_USER,		// send username
    FTP_R_USER,
    FTP_S_PASS,		// send password
    FTP_R_PASS,
	FTP_S_SYST,		// send system (interrogates server)
	FTP_R_SYST,
    FTP_S_ACCT,		// send account
    FTP_R_ACCT,
	FTP_S_MACB,
	FTP_R_MACB,
	FTP_S_PWD ,		// send parent working directory (pwd)
	FTP_R_PWD ,
    FTP_S_DEL_FILE, // send delete file
    FTP_R_DEL_FILE,
    FTP_S_DEL_DIR , // send delete directory
    FTP_R_DEL_DIR ,
    FTP_S_MKDIR,    // send mkdir
    FTP_R_MKDIR,
    FTP_S_MODE,     // send ASCII or BINARY
    FTP_R_MODE,
    FTP_S_CWD,
    FTP_R_CWD,
    FTP_S_SIZE,
    FTP_R_SIZE,
    FTP_S_PUT,
    FTP_R_PUT,
    FTP_S_RETR,
    FTP_R_RETR,
    FTP_S_MDTM,
    FTP_R_MDTM,
    FTP_S_LIST,
    FTP_R_LIST,

///////////////////////
//// Data channel connection setup states
///////////////////////
    FTP_S_PASV,     // send passsive
    FTP_R_PASV,
//    FTP_S_PORT,     // send port
//    FTP_R_PORT

} FTP_STATE;

// higher level ftp actions
typedef enum _FTP_ACTION {
	GET,
	PUT,
	MKDIR,
	DEL
} FTP_ACTION;

class nsFtpConnectionThread : public nsIRunnable {
public:
    NS_DECL_ISUPPORTS

	nsFtpConnectionThread(PLEventQueue* aEventQ, nsIStreamListener *aListener);
	virtual ~nsFtpConnectionThread();
	
	// nsIRunnable method
	NS_IMETHOD Run();

    nsresult Init(nsIThread* aThread);

	// user level setup
    nsresult SetAction(FTP_ACTION aAction);
    nsresult SetUsePasv(PRBool aUsePasv);

private:
    nsresult Read(void);
    void SetSystInternals(void);
    FTP_STATE FindActionState(void);
    FTP_STATE FindGetState(void);

    // Private members

	PLEventQueue*		mEventQueue;        // used to communicate outside this thread
    nsIUrl*             mUrl;

    FTP_STATE			mState;             // the current state
    FTP_STATE           mNextState;         // the next state
    FTP_ACTION          mAction;            // the higher level action

    nsIInputStream*     mCInStream;         // command channel input
    nsIOutputStream*    mCOutStream;        // command channel output

    //nsString2           mDataAddress;       // the host:port combo for the data connection
    nsIInputStream*     mDInStream;         // data channel input
    nsIOutputStream*    mDOutStream;        // data channel output

    PRInt32             mResponseCode;      // the last command response code.
	nsString2			mResponseMsg;       // the last command response text
    nsString2           mUsername;
    nsString2           mPassword;
    nsString2           mFilename;          // url filename (if any)
    PRInt32             mLength;            // length of the file
    time_t              mLastModified;      // last modified time for file

// these members should be hung off of a specific transport connection
    PRInt32             mServerType;
    PRBool              mPasv;
	PRBool				mList;              // use LIST instead of NLST
// end "these ...."

    PRBool              mConnected;
	PRBool			    mUseDefaultPath;    // use PWD to figure out path
    PRBool              mUsePasv;           // use a passive data connection.
    PRBool              mAscii;             // transfer mode (ascii or binary)
    PRBool              mDirectory;         // this url is a directory

    nsIStreamListener*  mListener;          // the listener we want to call
                                            // during our event firing.
};