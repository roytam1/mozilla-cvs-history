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
#ifndef __nsftpdirlistingdconv__h__
#define __nsftpdirlistingdconv__h__

#include "nsIStreamConverter.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsString2.h"
#include "nsILocale.h"
#include "nsIDateTimeFormat.h"

#include "nsIFactory.h"

#define NS_FTPDIRLISTINGCONVERTER_CID                         \
{ /* 14C0E880-623E-11d3-A178-0050041CAF44 */         \
    0x14c0e880,                                      \
    0x623e,                                          \
    0x11d3,                                          \
    {0xa1, 0x78, 0x00, 0x50, 0x04, 0x1c, 0xaf, 0x44}       \
}
static NS_DEFINE_CID(kFTPDirListingConverterCID, NS_FTPDIRLISTINGCONVERTER_CID);

// The nsFTPDirListingConv stream converter converts a stream of type "text/ftp-dir-SERVER_TYPE"
// (where SERVER_TYPE is one of the following):
//
// SERVER TYPES:
// generic
// unix
// dcts
// ncsa
// peter_lewis
// machten
// cms
// tcpc
// vms
// nt
// 
// nsFTPDirListingConv converts the raw ascii text directory generated via a FTP
// LIST or NLST command, to the application/http-index-format MIME-type.
// For more info see: http://www.area.com/~roeber/file_format.html

typedef enum _FTP_Server_Type {
    GENERIC,
    UNIX,
    DCTS,
    NCSA,
    PETER_LEWIS,
    MACHTEN,
    CMS,
    TCPC,
    VMS,
    NT
} FTP_Server_Type;

typedef enum _FTPentryType {
    Dir,
    File,
    Link
} FTPentryType;


// indexEntry is the data structure used to maintain directory entry information.
class indexEntry {
public:
    indexEntry() { mContentLen = 0; mMDTM = PR_Now(); mType = File; mSupressSize = PR_FALSE; };

    nsCString       mName;              // the file or dir name
    FTPentryType    mType;              
    PRInt32         mContentLen;        // length of the file
    nsCString       mContentType;       // type of the file
    PRTime          mMDTM;              // modified time
    PRBool          mSupressSize;       // supress the size info from display
};

class nsFTPDirListingConv : public nsIStreamConverter {
public:
    // nsISupports methods
    NS_DECL_ISUPPORTS

    // nsIStreamConverter methods
    NS_DECL_NSISTREAMCONVERTER

    // nsIStreamListener methods
    NS_DECL_NSISTREAMLISTENER

    // nsIStreamObserver methods
    NS_DECL_NSISTREAMOBSERVER

    // nsFTPDirListingConv methods
    nsFTPDirListingConv();
    virtual ~nsFTPDirListingConv();
    nsresult Init();

    // For factory creation.
    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult) {
        nsresult rv;
        if (aOuter)
            return NS_ERROR_NO_AGGREGATION;

        nsFTPDirListingConv* _s = new nsFTPDirListingConv();
        if (_s == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(_s);
        rv = _s->Init();
        if (NS_FAILED(rv)) {
            delete _s;
            return rv;
        }
        rv = _s->QueryInterface(aIID, aResult);
        NS_RELEASE(_s);
        return rv;
    }

private:
    // util parsing methods
    PRInt8   MonthNumber(const char *aCStr);
    PRBool   IsLSDate(char *aCStr);

    // date conversion/parsing methods
    PRBool   ConvertUNIXDate(char *aCStr, PRTime& outDate);
    PRBool   ConvertVMSDate(char *aCStr, PRTime& outDate);
    PRBool   ConvertDOSDate(char *aCStr, PRTime& outDate);

    // line parsing methods
    nsresult ParseLSLine(char *aLine, indexEntry *aEntry);
    nsresult ParseVMSLine(char *aLine, indexEntry *aEntry);

    // member data
    FTP_Server_Type     mServerType;        // what kind of server is the data coming from?
    nsCAutoString       mBuffer;            // buffered data.
    PRBool              mSentHeading;       // have we sent 100, 101, 200, and 300 lines yet?


    nsIStreamListener   *mFinalListener; // this guy gets the converted data via his OnDataAvailable()
    nsIChannel          *mPartChannel;  // the channel for the given part we're processing.
                                        // one channel per part.
    nsILocale           *mLocale;            // the application locale for date formating
    nsIDateTimeFormat   *mDateTimeFormat;    // for the actual date time formatting.
};

//////////////////////////////////////////////////
// FACTORY
class FTPDirListingFactory : public nsIFactory
{
public:
    FTPDirListingFactory(const nsCID &aClass, const char* className, const char* progID);

    // nsISupports methods
    NS_DECL_ISUPPORTS

    // nsIFactory methods
    NS_IMETHOD CreateInstance(nsISupports *aOuter,
                              const nsIID &aIID,
                              void **aResult);

    NS_IMETHOD LockFactory(PRBool aLock);

protected:
    virtual ~FTPDirListingFactory();

protected:
    nsCID       mClassID;
    const char* mClassName;
    const char* mProgID;
};

#endif /* __nsftpdirlistingdconv__h__ */
