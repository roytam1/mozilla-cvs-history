#ifndef nsHttpTransaction_h__
#define nsHttpTransaction_h__

#include "nsHttp.h"
#include "nsHttpHeaderArray.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"

class nsHttpRequestHead;
class nsHttpResponseHead;
class nsHttpConnection;
class nsHttpChunkedDecoder;

//-----------------------------------------------------------------------------
// nsHttpTransaction represents a single HTTP transaction.  It is thread-safe,
// intended to run on the socket thread.
//-----------------------------------------------------------------------------

class nsHttpTransaction : public nsIRequest
                        , public nsIInputStream
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSIINPUTSTREAM

    // A transaction is constructed from request headers.
    nsHttpTransaction(nsIStreamListener *);
    virtual ~nsHttpTransaction();

    // Called when assigned to a connection
    void SetConnection(nsHttpConnection *);

    // Called to initialize the transaction
    nsresult SetupRequest(nsHttpRequestHead *, nsIInputStream *);

    nsIStreamListener  *Listener()     { return mListener; }
    nsHttpConnection   *Connection()   { return mConnection; }
    nsHttpResponseHead *ResponseHead() { return mResponseHead; }

    // Called to take ownership of the response headers; the transaction
    // will drop any reference to the response headers after this call.
    nsHttpResponseHead *TakeResponseHead();

    // Called to write data to the socket until return NS_BASE_STREAM_CLOSED
    nsresult OnDataWritable(nsIOutputStream *);

    // Called to read data from the socket buffer
    nsresult OnDataReadable(nsIInputStream *);

    // Called when the transaction should stop, possibly prematurely with an error.
    nsresult OnStopTransaction(nsresult);

private:
    nsresult ParseLine(char *line);
    nsresult ParseHeaders(char *, PRUint32 count, PRUint32 *countRead);
    nsresult HandleContent(char *, PRUint32 count, PRUint32 *countRead);
    //nsresult InstallChunkedDecoder();

private:
    nsCOMPtr<nsIStreamListener> mListener;

    nsHttpConnection           *mConnection; // hard ref

    nsCString                   mReqHeaderBuf;    // flattened request headers
    nsCOMPtr<nsIInputStream>    mReqHeaderStream; // header data stream
    nsCOMPtr<nsIInputStream>    mReqUploadStream; // upload data stream

    nsCOMPtr<nsIInputStream>    mSource;
    nsHttpResponseHead         *mResponseHead;

    //char                       *mReadBuf;         // read ahead buffer
    nsCString                   mLineBuf;         // may contain a partial line

    PRInt32                     mContentLength;   // equals -1 if unknown
    PRUint32                    mContentRead;     // count of consumed content bytes

    nsHttpChunkedDecoder       *mChunkedDecoder;

    PRInt32                     mTransactionDone; // atomically {in,de}cremented

    PRPackedBool                mHaveStatusLine;
    PRPackedBool                mHaveAllHeaders;
    PRPackedBool                mFiredOnStart;
};

#if 0
//-----------------------------------------------------------------------------
// nsAHttpTransactionSink recieves notifications from the transaction.  This
// is, for example, implemented by nsHttpConnection.
//-----------------------------------------------------------------------------

class nsAHttpTransactionSink : public nsISupports
{
public:
    virtual nsresult OnHeadersAvailable(nsHttpTransaction *) = 0;
    virtual nsresult OnTransactionComplete(nsHttpTransaction *, nsresult) = 0;
};
#endif

#endif
