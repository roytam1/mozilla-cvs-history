#include "nsNetUtil.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIProgressEventSink.h"

#define RETURN_IF_FAILED(rv, step) \
    PR_BEGIN_MACRO \
    if (NS_FAILED(rv)) { \
        printf(">>> %s failed: rv=%x\n", step, rv); \
        return rv;\
    } \
    PR_END_MACRO

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static nsIEventQueue* gEventQ = nsnull;
static PRBool gKeepRunning = PR_TRUE;

//-----------------------------------------------------------------------------
// nsIStreamListener implementation
//-----------------------------------------------------------------------------

class MyListener : public nsIStreamListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    MyListener() { NS_INIT_ISUPPORTS(); }
    virtual ~MyListener() {}
};

NS_IMPL_ISUPPORTS2(MyListener,
                   nsIRequestObserver,
                   nsIStreamListener)

NS_IMETHODIMP
MyListener::OnStartRequest(nsIRequest *req, nsISupports *ctxt)
{
    printf(">>> OnStartRequest\n");
    return NS_OK;
}

NS_IMETHODIMP
MyListener::OnStopRequest(nsIRequest *req, nsISupports *ctxt, nsresult status)
{
    printf(">>> OnStopRequest\n");
    gKeepRunning = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
MyListener::OnDataAvailable(nsIRequest *req, nsISupports *ctxt,
                            nsIInputStream *stream,
                            PRUint32 offset, PRUint32 count)
{
    printf(">>> OnDataAvailable [count=%u]\n", count);

    char buf[256];
    nsresult rv;
    PRUint32 bytesRead=0;

    while (count) {
        PRUint32 amount = PR_MIN(count, sizeof(buf));

        rv = stream->Read(buf, amount, &bytesRead);
        if (NS_FAILED(rv)) {
            printf(">>> stream->Read failed with rv=%x\n", rv);
            return rv;
        }

        fwrite(buf, 1, bytesRead, stdout);

        count -= bytesRead;
    }
    return NS_OK;
}

//-----------------------------------------------------------------------------
// NotificationCallbacks implementation
//-----------------------------------------------------------------------------

class MyNotifications : public nsIInterfaceRequestor
                      , public nsIProgressEventSink
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIPROGRESSEVENTSINK

    MyNotifications() { NS_INIT_ISUPPORTS(); }
    virtual ~MyNotifications() {}
};

NS_IMPL_ISUPPORTS2(MyNotifications,
                   nsIInterfaceRequestor,
                   nsIProgressEventSink)

NS_IMETHODIMP
MyNotifications::GetInterface(const nsIID &iid, void **result)
{
    return QueryInterface(iid, result);
}

NS_IMETHODIMP
MyNotifications::OnStatus(nsIRequest *req, nsISupports *ctx,
                          nsresult status, const PRUnichar *statusText)
{
    printf("status: %x\n", status);
    return NS_OK;
}

NS_IMETHODIMP
MyNotifications::OnProgress(nsIRequest *req, nsISupports *ctx,
                            PRUint32 progress, PRUint32 progressMax)
{
    printf("progress: %u/%u\n", progress, progressMax);
    return NS_OK;
}

//-----------------------------------------------------------------------------
// main, etc..
//-----------------------------------------------------------------------------

nsresult NS_AutoregisterComponents()
{
  nsresult rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup, NULL /* default */);
  return rv;
}

int main(int argc, char **argv)
{
    nsresult rv;

    if (argc == 1) {
        printf("usage: TestHttp <url>\n");
        return -1;
    }

    rv = NS_AutoregisterComponents();
    RETURN_IF_FAILED(rv, "NS_AutoregisterComponents");

    // Create the Event Queue for this thread...
    NS_WITH_SERVICE(nsIEventQueueService, eqs, kEventQueueServiceCID, &rv);
    RETURN_IF_FAILED(rv, "do_GetService(EventQueueService)");

    rv = eqs->CreateMonitoredThreadEventQueue();
    RETURN_IF_FAILED(rv, "CreateMonitoredThreadEventQueue");

    rv = eqs->GetThreadEventQueue(NS_CURRENT_THREAD, &gEventQ);
    RETURN_IF_FAILED(rv, "GetThreadEventQueue");

    nsCOMPtr<nsIURI> uri;
    MyListener *listener = new MyListener();
    MyNotifications *callbacks = new MyNotifications();

    rv = NS_NewURI(getter_AddRefs(uri), argv[1]);
    RETURN_IF_FAILED(rv, "NS_NewURI");

    rv = NS_OpenURI(listener, nsnull, uri, nsnull, nsnull, callbacks);
    RETURN_IF_FAILED(rv, "NS_OpenURI");

    while (gKeepRunning)
        gEventQ->ProcessPendingEvents();

    printf(">>> done\n");
    return 0;
}
