/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */
#include "nsCodebasePrincipal.h"
#include "nsJSPrincipals.h"
#include "plstr.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "jsapi.h"
#include "jsxdrapi.h"
#include "nsIJSRuntimeService.h"
#include "nsIServiceManager.h"
#include "nsMemory.h"

JS_STATIC_DLL_CALLBACK(void *)
nsGetPrincipalArray(JSContext *cx, struct JSPrincipals *prin)
{
    return nsnull;
}

JS_STATIC_DLL_CALLBACK(JSBool)
nsGlobalPrivilegesEnabled(JSContext *cx , struct JSPrincipals *jsprin)
{
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(void)
nsDestroyJSPrincipals(JSContext *cx, struct JSPrincipals *jsprin)
{
    nsJSPrincipals *nsjsprin = NS_STATIC_CAST(nsJSPrincipals *, jsprin);

    // We need to destroy the nsIPrincipal. We'll do this by adding
    // to the refcount and calling release

    // Note that we don't want to use NS_IF_RELEASE because it will try
    // to set nsjsprin->nsIPrincipalPtr to nsnull *after* nsjsprin has
    // already been destroyed.
#ifdef NS_BUILD_REFCNT_LOGGING
    // The refcount logging considers AddRef-to-1 to indicate creation,
    // so trick it into thinking it's otherwise, but balance the
    // Release() we do below.
    nsjsprin->refcount++;
    nsjsprin->nsIPrincipalPtr->AddRef();
    nsjsprin->refcount--;
#else
    nsjsprin->refcount++;
#endif
    if (nsjsprin->nsIPrincipalPtr)
        nsjsprin->nsIPrincipalPtr->Release();
    // The nsIPrincipal that we release owns the JSPrincipal struct,
    // so we don't need to worry about "codebase"
}

JS_STATIC_DLL_CALLBACK(JSBool)
nsDecodeJSPrincipals(JSXDRState *xdr, JSPrincipals **jsprinp)
{
    nsresult rv;
    nsCOMPtr<nsIPrincipal> prin;

    nsIObjectInputStream *stream = NS_REINTERPRET_CAST(nsIObjectInputStream*,
                                                       xdr->userdata);

    rv = stream->ReadObject(PR_TRUE, getter_AddRefs(prin));
    if (NS_SUCCEEDED(rv)) {
        PRUint32 size;
        rv = stream->Read32(&size);
        if (NS_SUCCEEDED(rv)) {
            char *data;
            rv = stream->ReadBytes(&data, size);
            if (NS_SUCCEEDED(rv)) {
                char *olddata;
                PRUint32 oldsize;

                // Any decode-mode JSXDRState whose userdata points to an
                // nsIObjectInputStream instance must use nsMemory to allocate
                // and free its data buffer.  So swap the new buffer we just
                // read for the old, exhausted data.

                olddata = (char*) ::JS_XDRMemGetData(xdr, &oldsize);
                nsMemory::Free(olddata);
                ::JS_XDRMemSetData(xdr, data, size);
            }
        }
    }

    if (NS_FAILED(rv)) {
        ::JS_ReportError(xdr->cx, "can't decode principals (failure code %x)",
                         (unsigned int) rv);
        return JS_FALSE;
    }

    prin->GetJSPrincipals(jsprinp);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
nsEncodeJSPrincipals(JSXDRState *xdr, struct JSPrincipals *jsprin)
{
    nsresult rv;

    // Flush xdr'ed data to the underlying object output stream.
    nsIObjectOutputStream *stream = NS_REINTERPRET_CAST(nsIObjectOutputStream*,
                                                        xdr->userdata);
    PRUint32 size;
    char *data = (char*) ::JS_XDRMemGetData(xdr, &size);

    rv = stream->Write32(size);
    if (NS_SUCCEEDED(rv))
        rv = stream->WriteBytes(data, size);
    if (NS_FAILED(rv)) return rv;

    ::JS_XDRMemResetData(xdr);

    // Require that GetJSPrincipals has been called already by the code that
    // compiled the script that owns this principals instance.
    nsJSPrincipals *nsjsprin = NS_STATIC_CAST(nsJSPrincipals *, jsprin);
    nsCOMPtr<nsIPrincipal> prin = nsjsprin->nsIPrincipalPtr;

    rv = stream->WriteObject(prin, PR_TRUE);
    return NS_SUCCEEDED(rv);                    // NB: guaranteed to be 0 or 1
}

nsresult
nsJSPrincipals::Startup()
{
    static const char rtsvc_id[] = "@mozilla.org/js/xpc/RuntimeService;1";
    nsCOMPtr<nsIJSRuntimeService> rtsvc(do_GetService(rtsvc_id));
    if (!rtsvc)
        return NS_ERROR_FAILURE;

    JSRuntime *rt;
    rtsvc->GetRuntime(&rt);
    NS_ASSERTION(rt != nsnull, "no JSRuntime?!");

    JSPrincipalsDecoder oldpd;
    oldpd = ::JS_SetPrincipalsDecoder(rt, nsDecodeJSPrincipals);
    NS_ASSERTION(oldpd == nsnull, "oops, JS_SetPrincipalsDecoder wars!");

    return NS_OK;
}

nsJSPrincipals::nsJSPrincipals()
{
    codebase = nsnull;
    getPrincipalArray = nsGetPrincipalArray;
    globalPrivilegesEnabled = nsGlobalPrivilegesEnabled;
    refcount = 0;
    destroy = nsDestroyJSPrincipals;
    encode = nsEncodeJSPrincipals;
    nsIPrincipalPtr = nsnull;
}

nsresult
nsJSPrincipals::Init(char *aCodebase)
{
    codebase = aCodebase;
    return NS_OK;
}

nsJSPrincipals::~nsJSPrincipals()
{
    if (codebase)
        PL_strfree(codebase);
}
