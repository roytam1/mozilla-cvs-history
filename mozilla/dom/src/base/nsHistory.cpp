/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Travis Bogard <travis@netscape.com> 
 */

#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsHistory.h"
#include "nsIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIWebShell.h"
#include "nsIDocShell.h"

//
//  History class implementation 
//
HistoryImpl::HistoryImpl(nsIDocShell* aDocShell) : mScriptObject(nsnull), 
   mDocShell(aDocShell) 
{
  NS_INIT_REFCNT();
}

HistoryImpl::~HistoryImpl()
{
}

NS_IMPL_ADDREF(HistoryImpl)
NS_IMPL_RELEASE(HistoryImpl)

NS_INTERFACE_MAP_BEGIN(HistoryImpl)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptObjectOwner)
   NS_INTERFACE_MAP_ENTRY(nsIScriptObjectOwner)
   NS_INTERFACE_MAP_ENTRY(nsIDOMHistory)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
HistoryImpl::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

NS_IMETHODIMP
HistoryImpl::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  NS_PRECONDITION(nsnull != aScriptObject, "null arg");
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    nsIScriptGlobalObject *global = aContext->GetGlobalObject();
    res = NS_NewScriptHistory(aContext, (nsIDOMHistory*)this, (nsIDOMWindow*)global, &mScriptObject);
    NS_IF_RELEASE(global);
  }
  
  *aScriptObject = mScriptObject;
  return res;
}

NS_IMETHODIMP_(void)       
HistoryImpl::SetDocShell(nsIDocShell *aDocShell)
{
  mDocShell = aDocShell; // Weak Reference
}

NS_IMETHODIMP
HistoryImpl::GetLength(PRInt32* aLength)
{
  if (mDocShell) {
    nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
    webShell->GetHistoryLength(*aLength);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
HistoryImpl::GetCurrent(nsString& aCurrent)
{
  PRInt32 curIndex;
  const PRUnichar* curURL = nsnull;

  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (webShell && NS_OK == webShell->GetHistoryIndex(curIndex)) {
    webShell->GetURL(curIndex, &curURL);
  }
  aCurrent.SetString(curURL);

  return NS_OK;
}

NS_IMETHODIMP
HistoryImpl::GetPrevious(nsString& aPrevious)
{
  PRInt32 curIndex;
  const PRUnichar* prevURL = nsnull;

  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (webShell && NS_OK == webShell->GetHistoryIndex(curIndex)) {
    webShell->GetURL(curIndex-1, &prevURL);
  }
  aPrevious.SetString(prevURL);

  return NS_OK;
}

NS_IMETHODIMP
HistoryImpl::GetNext(nsString& aNext)
{
  PRInt32 curIndex;
  const PRUnichar* nextURL = nsnull;

  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (webShell && NS_OK == webShell->GetHistoryIndex(curIndex)) {
    webShell->GetURL(curIndex+1, &nextURL);
  }
  aNext.SetString(nextURL);

  return NS_OK;
}

NS_IMETHODIMP
HistoryImpl::Back()
{
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (webShell && NS_OK == webShell->CanBack()) {
    webShell->Back();
  }
  
  return NS_OK;
}

NS_IMETHODIMP
HistoryImpl::Forward()
{
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (webShell && NS_OK == webShell->CanForward()) {
    webShell->Forward();
  }

  return NS_OK;
}

NS_IMETHODIMP    
HistoryImpl::Go(JSContext* cx, jsval* argv, PRUint32 argc)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (argc > 0) {
    if (JSVAL_IS_INT(argv[0])) {
      PRInt32 delta = JSVAL_TO_INT(argv[0]);
      PRInt32 curIndex;

      result = webShell->GetHistoryIndex(curIndex);
      if (NS_SUCCEEDED(result)) {
        result = webShell->GoTo(curIndex + delta);
      }
    }
    else {
      JSString* jsstr = JS_ValueToString(cx, argv[0]);
      PRInt32 i, count;

      if (nsnull != jsstr) {
        nsAutoString substr(JS_GetStringBytes(jsstr));

        result = webShell->GetHistoryLength(count);
        for (i = 0; (i < count) && NS_SUCCEEDED(result); i++) {
          const PRUnichar* urlstr;
          nsAutoString url;
          // XXX Ownership rules for the string passed back for this
          // method are not XPCOM compliant. If they were correct, 
          // we'd be deallocating the string passed back.
          result = webShell->GetURL(i, &urlstr);
          url.SetString(urlstr);

          if (-1 != url.Find(substr)) {
            result = webShell->GoTo(i);
            break;
          }
        }
      }
    }
  }
  
  return result;
}



