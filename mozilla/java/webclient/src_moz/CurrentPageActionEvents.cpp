/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s): Kirk Baker <kbaker@eb.com>
 *               Ian Wilkinson <iw@ennoble.com>
 *               Ashutosh Kulkarni <ashuk@eng.sun.com>
 *               Mark Lin <mark.lin@eng.sun.com>
 *               Mark Goddard
 *               Ed Burns <edburns@acm.org>
 *      Jason Mawdsley <jason@macadamian.com>
 *      Louis-Philippe Gagnon <louisphilippe@macadamian.com>
 */

/*
 * CurrentPageActionEvents.cpp
 */

#include "CurrentPageActionEvents.h"
#include "nsIDOMWindowInternal.h"
#include "nsISearchContext.h"
#include "nsIDocShell.h"
#include "nsIContentViewer.h"
#include "nsIContentViewer.h"
#include "nsIContentViewerEdit.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIHistoryEntry.h"
#include "nsString.h"
#include "nsReadableUtils.h"

wsCopySelectionEvent::wsCopySelectionEvent(WebShellInitContext *yourInitContext) :
        nsActionEvent(),
        mInitContext(yourInitContext)
{
}

void *
wsCopySelectionEvent::handleEvent ()
{
    void *result = nsnull;
    
    if (mInitContext) {
        nsIContentViewer* contentViewer ;
        nsresult rv = nsnull;
        
        rv = mInitContext->docShell->GetContentViewer(&contentViewer);
        if (NS_FAILED(rv) || contentViewer==nsnull )  {
            return (void *) rv;
        }
    
        nsCOMPtr<nsIContentViewerEdit> contentViewerEdit(do_QueryInterface(contentViewer));
        
        rv = contentViewerEdit->CopySelection();
        result = (void *) rv;
    }
    return result;
}

wsFindEvent::wsFindEvent(WebShellInitContext *yourInitContext) :
    nsActionEvent(),
    mInitContext(yourInitContext),
    mSearchString(nsnull),
    mForward(JNI_FALSE),
    mMatchCase(JNI_FALSE)
{
}

wsFindEvent::wsFindEvent(WebShellInitContext *yourInitContext, jstring searchString,
                         jboolean forward, jboolean matchCase) :
    nsActionEvent(),
    mInitContext(yourInitContext),
    mSearchString(searchString),
    mForward(forward),
    mMatchCase(matchCase)
{
}

void *
wsFindEvent::handleEvent ()
{
    void *result = nsnull;
    nsresult rv = NS_ERROR_FAILURE;
    JNIEnv *env = (JNIEnv *) JNU_GetEnv(gVm, JNI_VERSION);
    
    if (mInitContext) {
#if 0
        //First get the FindComponent object
        nsCOMPtr<nsIFindComponent> findComponent;
        findComponent = do_GetService(NS_IFINDCOMPONENT_CONTRACTID, &rv);
        
        if (NS_FAILED(rv) || nsnull == findComponent)  {
            return (void *) rv;
        }

        nsCOMPtr<nsISupports> searchContext;
        // get the nsISearchContext
        // No seachString means this is Find, not FindNext.
        if (mSearchString) {
            
            nsCOMPtr<nsIDOMWindowInternal> domWindowInternal;
            if (mInitContext->docShell != nsnull) {
                nsCOMPtr<nsIInterfaceRequestor> interfaceRequestor(do_QueryInterface(mInitContext->docShell));
                nsCOMPtr<nsIURI> url = nsnull;
                
                rv = mInitContext->webNavigation->GetCurrentURI(getter_AddRefs(url));
                if (NS_FAILED(rv) || nsnull == url)  {
                    return (void *) rv;
                } 
                
                if (interfaceRequestor != nsnull) {
                    rv = interfaceRequestor->GetInterface(NS_GET_IID(nsIDOMWindowInternal), 
                                                          getter_AddRefs(domWindowInternal));
                    if (NS_FAILED(rv) || nsnull == domWindowInternal)  {
                        return (void *) rv;
                    }
                }
                else
                    {
                        mInitContext->initFailCode = kFindComponentError;
                        return (void *) rv;
                    }
            }
            else {
                mInitContext->initFailCode = kFindComponentError;
                return (void *) rv;
            }
            
            // if we get here, we have a domWindowInternal
            
            rv = findComponent->CreateContext(domWindowInternal, nsnull, getter_AddRefs(searchContext));
            if (NS_FAILED(rv))  {
                mInitContext->initFailCode = kSearchContextError;
                return (void *) rv;
            }

        }
        else {
            // this is findNext
            searchContext = mInitContext->searchContext;
        }
        if (!searchContext) {
            mInitContext->initFailCode = kSearchContextError;
            return (void *) NS_ERROR_FAILURE;
        }
        
        nsCOMPtr<nsISearchContext> srchcontext;
        rv = searchContext->QueryInterface(NS_GET_IID(nsISearchContext), getter_AddRefs(srchcontext));
        if (NS_FAILED(rv))  {
            mInitContext->initFailCode = kSearchContextError;
            return (void *) rv;
        }
        
        PRUnichar * aString;
        srchcontext->GetSearchString(& aString);
        
        PRUnichar * srchString = nsnull;
        if (mSearchString) {
            srchString = (PRUnichar *) ::util_GetStringChars(env, mSearchString);
            
            // Check if String is NULL
            if (nsnull == srchString) {
                return (void *) NS_ERROR_NULL_POINTER;
            }
            
            srchcontext->SetSearchString(srchString);
            srchcontext->SetSearchBackwards(!mForward);
            srchcontext->SetCaseSensitive(mMatchCase);
        }
        
        PRBool found = PR_TRUE;
        rv = findComponent->FindNext(srchcontext, &found);
        result = (void *) rv;
        if (mSearchString) {
            ::util_ReleaseStringChars(env, mSearchString, srchString);
            ::util_DeleteGlobalRef(env, mSearchString);
            mSearchString = nsnull;
        }
        // Save in initContext struct for future findNextInPage calls
        mInitContext->searchContext = srchcontext;

#else
        return (void *) rv;
#endif  
  
    }
    return result;
}

/*
 * wsGetURLEvent
 */

wsGetURLEvent::wsGetURLEvent(WebShellInitContext *yourInitContext) :
        nsActionEvent(),
	mInitContext(yourInitContext)
{
}


void *
wsGetURLEvent::handleEvent ()
{
    void *result = nsnull;
    if (mInitContext) {
        nsISHistory* mHistory;
        nsresult rv;
        PRInt32 currentIndex;
        char *currentURL = nsnull;

        
        rv = mInitContext->webNavigation->GetSessionHistory(&mHistory);
        if (NS_FAILED(rv)) {
            return (void *) rv;
        }
        
        rv = mHistory->GetIndex(&currentIndex);
        
        if (NS_FAILED(rv)) {
            return result;
        }
        
        nsIHistoryEntry * Entry;
        rv = mHistory->GetEntryAtIndex(currentIndex, PR_FALSE, &Entry);

        if (NS_FAILED(rv)) {
            return result;
        }

	nsIURI * URI;
	rv = Entry->GetURI(&URI);

	if (NS_FAILED(rv)) {
            return result;
        }

    nsCString urlSpecString;
	
	rv = URI->GetSpec(urlSpecString);
	if (NS_FAILED(rv)) {
        return result;
    }
    currentURL = ToNewCString(urlSpecString);
    
    result = (void *) currentURL;
    }
    return result;
} // handleEvent()


wsSelectAllEvent::wsSelectAllEvent(WebShellInitContext *yourInitContext) :
        nsActionEvent(),
        mInitContext(yourInitContext)
{
}

void *
wsSelectAllEvent::handleEvent ()
{
    void *result = nsnull;
    
    if (mInitContext) {
        nsIContentViewer* contentViewer;
        nsresult rv = nsnull;
        rv = mInitContext->docShell->GetContentViewer(&contentViewer);
        if (NS_FAILED(rv) || contentViewer==nsnull)  {
            mInitContext->initFailCode = kGetContentViewerError;
            return (void *) rv;
        }

        nsCOMPtr<nsIContentViewerEdit> contentViewerEdit(do_QueryInterface(contentViewer));
        
        rv = contentViewerEdit->SelectAll();
        result = (void *) rv;
    }
    return result;
}

/* PENDING(ashuk): remove this from here and in the motif directory
wsViewSourceEvent::wsViewSourceEvent(nsIDocShell* docShell, PRBool viewMode) :
    nsActionEvent(),
    mDocShell(docShell),
    mViewMode(viewMode)
{
}

void *
wsViewSourceEvent::handleEvent ()
{
    if(mDocShell) {
        if(mViewMode) {
            nsresult rv = mDocShell->SetViewMode(nsIDocShell::viewSource);
            return (void *) rv;
        }
        else
            {
                nsresult rv = mDocShell->SetViewMode(nsIDocShell::viewNormal);
                return (void *) rv;
            }
    }
    return nsnull;
}
*/


wsGetDOMEvent::wsGetDOMEvent(JNIEnv *yourEnv, jclass clz, 
                             jmethodID yourID, jlong yourDoc) :
    nsActionEvent(),
    mEnv(yourEnv),
    mClazz(clz),
    mID(yourID),
    mDoc(yourDoc)
{
}

void *
wsGetDOMEvent::handleEvent ()
{

    void * result = nsnull;
    if (mEnv != nsnull && mClazz != nsnull && 
        mID != nsnull && mDoc != nsnull)
        result = (void *) util_CallStaticObjectMethodlongArg(mEnv, mClazz, mID, mDoc);
    
    return result;
}




 
