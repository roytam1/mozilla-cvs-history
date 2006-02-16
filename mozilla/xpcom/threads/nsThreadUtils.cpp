/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsThreadUtils.h"
#include "nsThreadManager.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsRunnable, nsIRunnable)
  
NS_IMETHODIMP
nsRunnable::Run()
{
  // Do nothing
  return NS_OK;
}

//-----------------------------------------------------------------------------

NS_COM NS_METHOD
NS_NewThread(const nsACString &name, nsIRunnable *runnable, nsIThread **result)
{
  nsresult rv = nsThreadManager::get()->nsThreadManager::NewThread(name, result);
  if (NS_SUCCEEDED(rv))
    rv = (*result)->Dispatch(runnable, NS_DISPATCH_NORMAL);
  return rv;
}

NS_COM NS_METHOD
NS_GetCurrentThread(nsIThread **result)
{
  return nsThreadManager::get()->nsThreadManager::GetCurrentThread(result);
}

NS_COM NS_METHOD
NS_GetMainThread(nsIThread **result)
{
  return nsThreadManager::get()->nsThreadManager::GetCurrentThread(result);
}

NS_COM NS_METHOD
NS_GetThread(const nsACString &name, nsIThread **result)
{
  return nsThreadManager::get()->nsThreadManager::GetThread(name, result);
}

NS_COM PRBool
NS_IsMainThread()
{
  PRBool result = PR_FALSE;
  nsThreadManager::get()->nsThreadManager::IsMainThread(&result);
  return result;
}
