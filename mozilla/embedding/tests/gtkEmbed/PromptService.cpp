/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape Communications Corporation.
 * Portions created by Netscape Communications Corporation are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the LGPL or the GPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "PromptService.h"

#include "nsCOMPtr.h"
#include "nsXPComFactory.h"
#include "nsIPromptService.h"

class PromptServiceImpl : public nsIPromptService
{
public:
    PromptServiceImpl();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROMPTSERVICE
   
protected:
    virtual ~PromptServiceImpl();
};


NS_IMPL_ISUPPORTS1(PromptServiceImpl, nsIPromptService)

PromptServiceImpl::PromptServiceImpl()
{
    NS_INIT_REFCNT();
}

PromptServiceImpl::~PromptServiceImpl()
{
}

/* void alert (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text); */
NS_IMETHODIMP PromptServiceImpl::Alert(nsIDOMWindow *parent, const PRUnichar *dialogTitle, const PRUnichar *text)
{
    printf("PromptServce::Alert(%s)\n", text);
    return NS_OK;
}

/* void alertCheck (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP PromptServiceImpl::AlertCheck(nsIDOMWindow *parent, const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *checkMsg, PRBool *checkValue)
{
    printf("PromptServce::AlertCheck(%s)\n", text);
    return NS_OK;
}

/* boolean confirm (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text); */
NS_IMETHODIMP PromptServiceImpl::Confirm(nsIDOMWindow *parent, const PRUnichar *dialogTitle, const PRUnichar *text, PRBool *_retval)
{
    printf("PromptServce::Confirm(%s)\n", text);
    return NS_OK;
}

/* boolean confirmCheck (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP PromptServiceImpl::ConfirmCheck(nsIDOMWindow *parent, const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *checkMsg, PRBool *checkValue, PRBool *_retval)
{
    printf("PromptServce::ConfirmCheck(%s)\n", text);
    return NS_OK;
}

/* void confirmEx (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, in unsigned long buttonFlags, in wstring button0Title, in wstring button1Title, in wstring button2Title, in wstring checkMsg, inout boolean checkValue, out PRInt32 buttonPressed); */
NS_IMETHODIMP PromptServiceImpl::ConfirmEx(nsIDOMWindow *parent, const PRUnichar *dialogTitle, const PRUnichar *text, PRUint32 buttonFlags, const PRUnichar *button0Title, const PRUnichar *button1Title, const PRUnichar *button2Title, const PRUnichar *checkMsg, PRBool *checkValue, PRInt32 *buttonPressed)
{
    printf("PromptServce::ConfirmEx(%s)\n", text);
    return NS_OK;
}

/* boolean prompt (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, inout wstring value, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP PromptServiceImpl::Prompt(nsIDOMWindow *parent, const PRUnichar *dialogTitle, const PRUnichar *text, PRUnichar **value, const PRUnichar *checkMsg, PRBool *checkValue, PRBool *_retval)
{
    printf("PromptServce::ConfirmText(%s)\n", text);
    return NS_OK;
}

/* boolean promptUsernameAndPassword (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, inout wstring username, inout wstring password, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP PromptServiceImpl::PromptUsernameAndPassword(nsIDOMWindow *parent, const PRUnichar *dialogTitle, const PRUnichar *text, PRUnichar **username, PRUnichar **password, const PRUnichar *checkMsg, PRBool *checkValue, PRBool *_retval)
{
    return NS_OK;
}

/* boolean promptPassword (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, inout wstring password, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP PromptServiceImpl::PromptPassword(nsIDOMWindow *parent, const PRUnichar *dialogTitle, const PRUnichar *text, PRUnichar **password, const PRUnichar *checkMsg, PRBool *checkValue, PRBool *_retval)
{
    return NS_OK;
}

/* boolean select (in nsIDOMWindow parent, in wstring dialogTitle, in wstring text, in PRUint32 count, [array, size_is (count)] in wstring selectList, out long outSelection); */
NS_IMETHODIMP PromptServiceImpl::Select(nsIDOMWindow *parent, const PRUnichar *dialogTitle, const PRUnichar *text, PRUint32 count, const PRUnichar **selectList, PRInt32 *outSelection, PRBool *_retval)
{
    return NS_OK;
}

////////////////////////////////////////////

NS_DEF_FACTORY(PromptService, PromptServiceImpl);

nsresult NS_NewPromptServiceFactory(nsIFactory **aResult)
{
    nsresult rv = NS_OK;
    nsIFactory *inst = new nsPromptServiceFactory;
    if (inst == nsnull)
	rv = NS_ERROR_OUT_OF_MEMORY;
    else
	NS_ADDREF(inst);
    *aResult = inst;
    return rv;
}
