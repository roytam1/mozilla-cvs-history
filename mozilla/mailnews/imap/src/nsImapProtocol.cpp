/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "msgCore.h"  // for pre-compiled headers

#include "nsImapProtocol.h"
#include "nscore.h"
#include "nsImapProxyEvent.h"

// netlib required files
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsINetService.h"

#ifdef XP_PC
#include <windows.h>    // for InterlockedIncrement
#endif

#define ONE_SECOND ((PRUint32)1000)    // one second

static NS_DEFINE_CID(kNetServiceCID, NS_NETSERVICE_CID);

#define OUTPUT_BUFFER_SIZE (4096*2) // mscott - i should be able to remove this if I can use nsMsgLineBuffer???

/* the following macros actually implement addref, release and query interface for our component. */
NS_IMPL_THREADSAFE_ADDREF(nsImapProtocol)
NS_IMPL_THREADSAFE_RELEASE(nsImapProtocol)

NS_IMETHODIMP nsImapProtocol::QueryInterface(const nsIID &aIID, void** aInstancePtr)
{                                                                        
  if (NULL == aInstancePtr)
    return NS_ERROR_NULL_POINTER;
        
  *aInstancePtr = NULL;
                                                                         
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID); 
  static NS_DEFINE_IID(kIsThreadsafeIID, NS_ISTHREADSAFE_IID); 

  if (aIID.Equals(nsIStreamListener::GetIID())) 
  {
	  *aInstancePtr = (nsIStreamListener *) this;                                                   
	  NS_ADDREF_THIS();
	  return NS_OK;
  }
  if (aIID.Equals(nsIImapProtocol::GetIID()))
  {
	  *aInstancePtr = (nsIImapProtocol *) this;
	  NS_ADDREF_THIS();
	  return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) 
  {
	  *aInstancePtr = (void*) ((nsISupports*)this);
	  NS_ADDREF_THIS();
    return NS_OK;                                                        
  }                                                                      
  
  if (aIID.Equals(kIsThreadsafeIID)) 
  {
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsImapProtocol::nsImapProtocol()
{
	NS_INIT_REFCNT();
	m_runningUrl = nsnull; // initialize to NULL
	m_transport = nsnull;
	m_outputStream = nsnull;
	m_outputConsumer = nsnull;
	m_urlInProgress = PR_FALSE;
	m_socketIsOpen = PR_FALSE;
	m_dataBuf = nsnull;
    
    // ***** Thread support *****
    m_sinkEventQueue = nsnull;
    m_eventQueue = nsnull;
    m_thread = nsnull;
    m_dataMonitor = nsnull;
    m_imapThreadIsRunning = PR_FALSE;
    m_consumer = nsnull;

    m_imapState = nsImapProtocol::NOT_CONNECTED;
}

nsresult nsImapProtocol::Initialize(PLEventQueue * aSinkEventQueue)
{
	NS_PRECONDITION(aSinkEventQueue, "oops...trying to initalize with a null sink event queue!");
	if (aSinkEventQueue)
		m_sinkEventQueue = aSinkEventQueue;

	return NS_OK;
}

nsImapProtocol::~nsImapProtocol()
{
	// free handles on all networking objects...
	NS_IF_RELEASE(m_outputStream); 
	NS_IF_RELEASE(m_outputConsumer);
	NS_IF_RELEASE(m_transport);
	PR_FREEIF(m_dataBuf);

    // **** We must be out of the thread main loop function
    NS_ASSERTION(m_imapThreadIsRunning == PR_FALSE, 
                 "Oops, thread is still running.\n");
    if (m_eventQueue)
    {
        PL_DestroyEventQueue(m_eventQueue);
        m_eventQueue = nsnull;
    }

    if (m_dataMonitor)
    {
        PR_DestroyMonitor(m_dataMonitor);
        m_dataMonitor = nsnull;
    }
}

void nsImapProtocol::SetupWithUrl(nsIURL * aURL)
{
	NS_PRECONDITION(aURL, "null URL passed into Imap Protocol");

	m_flags = 0;

	// query the URL for a nsIImapUrl
	m_runningUrl = nsnull; // initialize to NULL
	m_transport = nsnull;

	if (aURL)
	{
		nsresult rv = aURL->QueryInterface(nsIImapUrl::GetIID(), (void **)&m_runningUrl);
		if (NS_SUCCEEDED(rv) && m_runningUrl)
		{
			// extract the file name and create a file transport...
			const char * hostName = nsnull;
			PRUint32 port = IMAP_PORT;

			m_runningUrl->GetHost(&hostName);
			m_runningUrl->GetHostPort(&port);

			/*JT - Should go away when netlib registers itself! */
			nsComponentManager::RegisterComponent(kNetServiceCID, NULL, NULL, 
												  "netlib.dll", PR_FALSE, PR_FALSE); 
            nsINetService* pNetService;
            rv = nsServiceManager::GetService(kNetServiceCID,
                                              nsINetService::GetIID(),
                                              (nsISupports**)&pNetService);
			if (NS_SUCCEEDED(rv) && pNetService)
			{
				rv = pNetService->CreateSocketTransport(&m_transport, port, hostName);
                (void)nsServiceManager::ReleaseService(kNetServiceCID, pNetService);
			}
		}
	}
	
	m_outputStream = NULL;
	m_outputConsumer = NULL;

	nsresult rv = m_transport->GetOutputStream(&m_outputStream);
	NS_ASSERTION(NS_SUCCEEDED(rv), "ooops, transport layer unable to create an output stream");
	rv = m_transport->GetOutputStreamConsumer(&m_outputConsumer);
	NS_ASSERTION(NS_SUCCEEDED(rv), "ooops, transport layer unable to provide us with an output consumer!");

	// register self as the consumer for the socket...
	rv = m_transport->SetInputStreamConsumer((nsIStreamListener *) this);
	NS_ASSERTION(NS_SUCCEEDED(rv), "unable to register Imap instance as a consumer on the socket");

	m_urlInProgress = PR_FALSE;
	m_socketIsOpen = PR_FALSE;

	// m_dataBuf is used by ReadLine and SendData
	m_dataBuf = (char *) PR_Malloc(sizeof(char) * OUTPUT_BUFFER_SIZE);
	m_dataBufSize = OUTPUT_BUFFER_SIZE;

    // ******* Thread support *******
    if (m_thread == nsnull)
    {
        m_dataMonitor = PR_NewMonitor();
        m_thread = PR_CreateThread(PR_USER_THREAD, ImapThreadMain, (void*)
                                   this, PR_PRIORITY_NORMAL, PR_LOCAL_THREAD,
                                   PR_UNJOINABLE_THREAD, 0);
        NS_ASSERTION(m_thread, "Unable to create imap thread.\n");
    }
}

void
nsImapProtocol::ImapThreadMain(void *aParm)
{
    nsImapProtocol *me = (nsImapProtocol *) aParm;
    NS_ASSERTION(me, "Yuk, me is null.\n");
    
    PR_CEnterMonitor(aParm);
    NS_ASSERTION(me->m_imapThreadIsRunning == PR_FALSE, 
                 "Oh. oh. thread is already running. What's wrong here?");
    if (me->m_imapThreadIsRunning)
    {
        PR_CExitMonitor(me);
        return;
    }
    me->m_eventQueue = PL_CreateEventQueue("ImapProtocolThread",
                                       PR_GetCurrentThread());
    NS_ASSERTION(me->m_eventQueue, 
                 "Unable to create imap thread event queue.\n");
    if (!me->m_eventQueue)
    {
        PR_CExitMonitor(me);
        return;
    }
    me->m_imapThreadIsRunning = PR_TRUE;
    PR_CExitMonitor(me);

    // call the platform specific main loop ....
    me->ImapThreadMainLoop();

    PR_DestroyEventQueue(me->m_eventQueue);
    me->m_eventQueue = nsnull;

    // ***** Important need to remove the connection out from the connection
    // pool - nsImapService **********
    delete me;
}

PRBool
nsImapProtocol::ImapThreadIsRunning()
{
    PRBool retValue = PR_FALSE;
    PR_CEnterMonitor(this);
    retValue = m_imapThreadIsRunning;
    PR_CExitMonitor(this);
    return retValue;
}

NS_IMETHODIMP
nsImapProtocol::GetThreadEventQueue(PLEventQueue **aEventQueue)
{
    // *** should subclassing PLEventQueue and ref count it ***
    // *** need to find a way to prevent dangling pointer ***
    // *** a callback mechanism or a semaphor control thingy ***
    if (aEventQueue)
        *aEventQueue = m_eventQueue;
    return NS_OK;
}

void
nsImapProtocol::ImapThreadMainLoop()
{
    // ****** please implement PR_LOG 'ing ******
    while (ImapThreadIsRunning())
    {
        PR_EnterMonitor(m_dataMonitor);

        PR_Wait(m_dataMonitor, PR_INTERVAL_NO_TIMEOUT);

        ProcessCurrentURL();

        PR_ExitMonitor(m_dataMonitor);
    }
}

void
nsImapProtocol::ProcessCurrentURL()
{
    nsresult res;

    if (!m_urlInProgress)
    {
        // **** we must be just successfully connected to the sever; we
        // haven't got a chance to run the url yet; let's call load the url
        // again
        res = LoadUrl((nsIURL*)m_runningUrl, m_consumer);
        return;
    }
    else 
    {
        // **** temporary for now
        nsIImapLog* aImapLog = nsnull;
        res = m_runningUrl->GetImapLog(&aImapLog);
        if (NS_SUCCEEDED(res) && aImapLog)
        {
            nsImapLogProxy *aProxy = 
                new nsImapLogProxy(aImapLog, m_sinkEventQueue, m_thread);
            NS_ADDREF(aProxy);
            aProxy->HandleImapLogData(m_dataBuf);
            NS_RELEASE(aImapLog);
            NS_RELEASE(aProxy);
            // we are done running the imap log url so mark the url as done...
            // set change in url state...
            m_runningUrl->SetUrlState(PR_FALSE, NS_OK); 
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////
// we suppport the nsIStreamListener interface 
////////////////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsImapProtocol::OnDataAvailable(nsIURL* aURL, nsIInputStream *aIStream, PRUint32 aLength)
{
    PR_CEnterMonitor(this);

	// we would read a line from the stream and then parse it.....I think this function can
	// effectively replace ReadLineFromSocket...
    nsIImapUrl *aImapUrl;
    nsresult res = aURL->QueryInterface(nsIImapUrl::GetIID(),
                                        (void**)&aImapUrl);
    PRUint32 len = aLength > OUTPUT_BUFFER_SIZE-1 ? OUTPUT_BUFFER_SIZE-1 : aLength;

    if(NS_SUCCEEDED(res))
    {
        NS_PRECONDITION( m_runningUrl->Equals(aImapUrl), 
                         "Oops... running a different imap url. Hmmm...");
            
        if (m_imapState == NOT_CONNECTED)
        {
            m_imapState = NON_AUTHENTICATED_STATE;
        }
        
        res = aIStream->Read(m_dataBuf, len, &len);
        if (NS_SUCCEEDED(res))
        {
            m_dataBuf[len] = 0;
			PR_EnterMonitor(m_dataMonitor);
            PR_Notify(m_dataMonitor);
			PR_ExitMonitor(m_dataMonitor);
        }
        NS_RELEASE(aImapUrl);
    }

    PR_CExitMonitor(this);

	return res;
}

NS_IMETHODIMP nsImapProtocol::OnStartBinding(nsIURL* aURL, const char *aContentType)
{
	m_runningUrl->SetUrlState(PR_TRUE, NS_OK);
	return NS_OK;
}

// stop binding is a "notification" informing us that the stream associated with aURL is going away. 
NS_IMETHODIMP nsImapProtocol::OnStopBinding(nsIURL* aURL, nsresult aStatus, const PRUnichar* aMsg)
{
	m_runningUrl->SetUrlState(PR_FALSE, aStatus); // set change in url state...
	return NS_OK;

}

/////////////////////////////////////////////////////////////////////////////////////////////
// End of nsIStreamListenerSupport
//////////////////////////////////////////////////////////////////////////////////////////////

PRInt32 nsImapProtocol::ReadLine(nsIInputStream * inputStream, PRUint32 length, char ** line)
{
	// I haven't looked into writing this yet. We have a couple of possibilities:
	// (1) insert ReadLine *yuck* into here or better yet into the nsIInputStream
	// then we can just turn around and call it here. 
	// OR
	// (2) we write "protocol" specific code for news which looks for a CRLF in the incoming
	// stream. If it finds it, that's our new line that we put into @param line. We'd
	// need a buffer (m_dataBuf) to store extra info read in from the stream.....

	// read out everything we've gotten back and return it in line...this won't work for much but it does
	// get us going...

	// XXX: please don't hold this quick "algorithm" against me. I just want to read just one
	// line for the stream. I promise this is ONLY temporary to test out NNTP. We need a generic
	// way to read one line from a stream. For now I'm going to read out one character at a time.
	// (I said it was only temporary =)) and test for newline...

	PRUint32 numBytesToRead = 0;  // MAX # bytes to read from the stream
	PRUint32 numBytesRead = 0;	  // total number bytes we have read from the stream during this call
	inputStream->GetLength(&length); // refresh the length in case it has changed...

	if (length > OUTPUT_BUFFER_SIZE)
		numBytesToRead = OUTPUT_BUFFER_SIZE;
	else
		numBytesToRead = length;

	m_dataBuf[0] = '\0';
	PRUint32 numBytesLastRead = 0;  // total number of bytes read in the last cycle...
	do
	{
		inputStream->Read(m_dataBuf + numBytesRead /* offset into m_dataBuf */, 1 /* read just one byte */, &numBytesLastRead);
		numBytesRead += numBytesLastRead;
	} while (numBytesRead <= numBytesToRead && numBytesLastRead > 0 && m_dataBuf[numBytesRead-1] != '\n');

	m_dataBuf[numBytesRead] = '\0'; // null terminate the string.

	// oops....we also want to eat up the '\n' and the \r'...
	if (numBytesRead > 1 && m_dataBuf[numBytesRead-2] == '\r')
		m_dataBuf[numBytesRead-2] = '\0'; // hit both cr and lf...
	else
		if (numBytesRead > 0 && (m_dataBuf[numBytesRead-1] == '\r' || m_dataBuf[numBytesRead-1] == '\n'))
			m_dataBuf[numBytesRead-1] = '\0';

	if (line)
		*line = m_dataBuf;
	return numBytesRead;
}

/*
 * Writes the data contained in dataBuffer into the current output stream. It also informs
 * the transport layer that this data is now available for transmission.
 * Returns a positive number for success, 0 for failure (not all the bytes were written to the
 * stream, etc). We need to make another pass through this file to install an error system (mscott)
 */

PRInt32 nsImapProtocol::SendData(const char * dataBuffer)
{
	PRUint32 writeCount = 0; 
	PRInt32 status = 0; 

	NS_PRECONDITION(m_outputStream && m_outputConsumer, "no registered consumer for our output");
	if (dataBuffer && m_outputStream)
	{
		nsresult rv = m_outputStream->Write(dataBuffer, PL_strlen(dataBuffer), &writeCount);
		if (NS_SUCCEEDED(rv) && writeCount == PL_strlen(dataBuffer))
		{
			// notify the consumer that data has arrived
			// HACK ALERT: this should really be m_runningUrl once we have NNTP url support...
			nsIInputStream *inputStream = NULL;
			m_outputStream->QueryInterface(nsIInputStream::GetIID() , (void **) &inputStream);
			if (inputStream)
			{
				m_outputConsumer->OnDataAvailable(m_runningUrl, inputStream, writeCount);
				NS_RELEASE(inputStream);
			}
			status = 1; // mscott: we need some type of MK_OK? MK_SUCCESS? Arrgghhh
		}
		else // the write failed for some reason, returning 0 trips an error by the caller
			status = 0; // mscott: again, I really want to add an error code here!!
	}

	return status;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Begin protocol state machine functions...
//////////////////////////////////////////////////////////////////////////////////////////////

nsresult nsImapProtocol::LoadUrl(nsIURL * aURL, nsISupports * aConsumer)
{
	nsresult rv = NS_OK;
	nsIImapUrl * imapUrl = nsnull;
	if (aURL)
	{
		// mscott: I used to call SetupWithUrl from the constructor but we really need the url
		// in order to properly intialize the protocol instance with a transport / host / port
		// etc. So I'm going to experiment by saying every time we load a Url, we look to see
		// if we need to initialize the protocol....I really need to check to make sure the host
		// and port haven't changed if they have then we need to re-initialize in order to pick up
		// a new connection to the imap server...

		if (m_transport == nsnull) // i.e. we haven't been initialized yet....
			SetupWithUrl(aURL);
		
		if (m_transport && m_runningUrl)
		{
			PRBool transportOpen = PR_FALSE;
			m_transport->IsTransportOpen(&transportOpen);
			if (transportOpen == PR_FALSE)
			{
                // m_urlInProgress = PR_TRUE;
				rv = m_transport->Open(m_runningUrl);  // opening the url will cause to get notified when the connection is established
			}
			else  // the connection is already open so we should begin processing our new url...
			{
				// mscott - I think Imap urls always come in fresh for each Imap protocol connection
				// so we should always be calling m_transport->open(our url)....
				// NS_ASSERTION(0, "I don't think we should get here for imap
                // urls");
                // ********** jefft ********* okay let's use ? search string
                // for passing the raw command now.
                m_urlInProgress = PR_TRUE;
                const char *search = nsnull;
                aURL->GetSearch(&search);
                char *tmpBuffer = nsnull;
                if (search && PL_strlen(search))
                {
                    tmpBuffer = PR_smprintf("%s\r\n", search);
                    if (tmpBuffer)
                    {
                        SendData(tmpBuffer);
                        PR_Free(tmpBuffer);
                    }
                }
			}
		} // if we have an imap url and a transport
        if (aConsumer)
            m_consumer = aConsumer;
	} // if we received a url!

	return rv;
}
