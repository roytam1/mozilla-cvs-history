/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef _nsMimeEmitter_h_
#define _nsMimeEmitter_h_

#include "prtypes.h"
#include "rebuffer.h"
#include "nsINetOStream.h"
#include "nsIMimeEmitter.h"

class nsMimeEmitter : public nsIMimeEmitter {
public: 
    nsMimeEmitter ();
    virtual       ~nsMimeEmitter (void);

    /* this macro defines QueryInterface, AddRef and Release for this class */
    NS_DECL_ISUPPORTS 

    // These will be called to start and stop the total operation
    NS_IMETHOD    Initialize(nsINetOStream *outStream);
    NS_IMETHOD    Complete();

    // Set the output stream for processed data.
    NS_IMETHOD    SetOutputStream(nsINetOStream *outStream);

    // Header handling routines.
    NS_IMETHOD    StartHeader(PRBool rootMailHeader);
    NS_IMETHOD    AddHeaderField(const char *field, const char *value);
    NS_IMETHOD    EndHeader();

    // Attachment handling routines
    NS_IMETHOD    StartAttachment();
    NS_IMETHOD    AddAttachmentField(const char *field, const char *value);
    NS_IMETHOD    EndAttachment();

    // Body handling routines
    NS_IMETHOD    StartBody();
    NS_IMETHOD    WriteBody(const char *buf, PRUint32 size, PRUint32 *amountWritten);
    NS_IMETHOD    EndBody();

    // Generic write routine. This is necessary for output that
    // libmime needs to pass through without any particular parsing
    // involved (i.e. decoded images, HTML Body Text, etc...
    NS_IMETHOD    Write(const char *buf, PRUint32 size, PRUint32 *amountWritten);
    NS_IMETHOD    UtilityWrite(const char *buf);

protected:
    // For buffer management on output
    MimeRebuffer  *mBufferMgr;

    // For the output stream
    nsINetOStream *mOutStream;
    PRUint32      mTotalWritten;
    PRUint32      mTotalRead;

    // For header determination...
    PRBool        mDocHeader;
};

/* this function will be used by the factory to generate an class access object....*/
extern nsresult NS_NewMimeEmitter(nsIMimeEmitter **aInstancePtrResult);

#endif /* _nsMimeEmitter_h_ */
