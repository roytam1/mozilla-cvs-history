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
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems
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
/*
 * pkix_pl_httpcertstore.c
 *
 * HTTPCertStore Function Definitions
 *
 */

/* We can't decode the length of a message without at least this many bytes */

#include "pkix_pl_httpcertstore.h"
extern PKIX_PL_HashTable *httpSocketCache;

/* --Private-HttpCertStoreContext-Object Functions----------------------- */

/*
 * FUNCTION: pkix_pl_HttpCertStoreContext_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_pl_HttpCertStoreContext_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;

        PKIX_ENTER
                (HTTPCERTSTORECONTEXT, "pkix_pl_HttpCertStoreContext_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_HTTPCERTSTORECONTEXT_TYPE, plContext),
                    "Object is not an HttpCertStoreContext");

        context = (PKIX_PL_HttpCertStoreContext *)object;

        hcv1 = (const SEC_HttpClientFcnV1 *)(context->client);

        if (context->requestSession != NULL) {
                PKIX_PL_NSSCALL(HTTPCERTSTORECONTEXT, hcv1->freeFcn,
                        (context->requestSession));
                context->requestSession = NULL;
        }

        if (context->serverSession != NULL) {
                PKIX_PL_NSSCALL(HTTPCERTSTORECONTEXT, hcv1->freeSessionFcn,
                        (context->serverSession));
                context->serverSession = NULL;
        }

        if (context->path != NULL) {
                PKIX_PL_NSSCALL
                        (HTTPCERTSTORECONTEXT, PORT_Free, (context->path));
                context->path = NULL;
        }

cleanup:

        PKIX_RETURN(HTTPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_HttpCertStoreContext_RegisterSelf
 *
 * DESCRIPTION:
 *  Registers PKIX_PL_HTTPCERTSTORECONTEXT_TYPE and its related
 *  functions with systemClasses[]
 *
 * THREAD SAFETY:
 *  Not Thread Safe - for performance and complexity reasons
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_pl_HttpCertStoreContext_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER
                (HTTPCERTSTORECONTEXT,
                "pkix_pl_HttpCertStoreContext_RegisterSelf");

        entry.description = "HttpCertStoreContext";
        entry.destructor = pkix_pl_HttpCertStoreContext_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_HTTPCERTSTORECONTEXT_TYPE] = entry;

        PKIX_RETURN(HTTPCERTSTORECONTEXT);
}

/* --Private-Http-CertStore-Database-Functions----------------------- */

typedef struct callbackContextStruct  {
        PKIX_List *pkixCertList;
        void *plContext;
} callbackContext;

/*
 * FUNCTION: certCallback
 * DESCRIPTION:
 *
 *  This function processes the null-terminated array of SECItems produced by
 *  extracting the contents of a signedData message received in response to an
 *  HTTP cert query. Its address is supplied as a callback function to
 *  CERT_DecodeCertPackage; it is not expected to be called directly.
 *
 *  Note that it does not conform to the libpkix API standard of returning
 *  a PKIX_Error*. It returns a SECStatus.
 *
 * PARAMETERS:
 *  "arg"
 *      The address of the callbackContext provided as a void* argument to
 *      CERT_DecodeCertPackage. Must be non-NULL.
 *  "secitemCerts"
 *      The address of the null-terminated array of SECItems. Must be non-NULL.
 *  "numcerts"
 *      The number of SECItems found in the signedData. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns SECSuccess if the function succeeds.
 *  Returns SECFailure if the function fails.
 */
static SECStatus
certCallback(void *arg, SECItem **secitemCerts, int numcerts)
{
        SECStatus rv;
        callbackContext *cbContext;
        SECItem **certs = NULL;
        SECItem *cert = NULL;
        PKIX_List *pkixCertList = NULL;
        PKIX_Error *error = NULL;
        void *plContext = NULL;
        int certsFound = 0;

        if ((arg == NULL) || (secitemCerts == NULL)) {
                return (SECFailure);
        }

        cbContext = (callbackContext *)arg;
        plContext = cbContext->plContext;
        pkixCertList = cbContext->pkixCertList;
        certs = secitemCerts;

        while ( *certs ) {
                cert = *certs;
                error = pkix_pl_Cert_CreateToList
                        (cert, pkixCertList, plContext);
                certsFound++;
                certs++;
                if (error != NULL) {
                        PKIX_PL_Object_DecRef
                                ((PKIX_PL_Object *)error, plContext);
                }
        }

        rv = (certsFound == numcerts)?SECSuccess:SECFailure;

cleanup:

        return(rv);
}

/*
 * FUNCTION: pkix_pl_HttpCertStore_ProcessCertResponse
 * DESCRIPTION:
 *
 *  This function verifies that the response code pointed to by "responseCode"
 *  and the content type pointed to by "responseContentType" are as expected,
 *  and then decodes the data pointed to by "responseData", of length
 *  "responseDataLen", into a List of Certs, possibly empty, which is returned
 *  at "pCertList".
 *
 * PARAMETERS:
 *  "responseCode"
 *      The value of the HTTP response code.
 *  "responseContentType"
 *      The address of the Content-type string. Must be non-NULL.
 *  "responseData"
 *      The address of the message data. Must be non-NULL.
 *  "responseDataLen"
 *      The length of the message data.
 *  "pCertList"
 *      The address of the List that is created. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpCertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_HttpCertStore_ProcessCertResponse(
        PRUint16 responseCode,
        const char *responseContentType,
        const char *responseData,
        PRUint32 responseDataLen,
        PKIX_List **pCertList,
        void *plContext)
{
        PRArenaPool *arena = NULL;
        char *encodedResponse = NULL;
        PRIntn compareVal = 0;
        PKIX_List *certs = NULL;
        SECStatus rv = SECFailure;
        callbackContext cbContext;

        PKIX_ENTER
                (HTTPCERTSTORECONTEXT,
                "pkix_pl_HttpCertStore_ProcessCertResponse");
        PKIX_NULLCHECK_ONE(pCertList);

        if (responseCode != 200) {
                PKIX_ERROR("Bad Http Response");
        }

        /* check that response type is application/pkcs7-mime */
        if (responseContentType == NULL) {
                PKIX_ERROR("No content type in Http Response");
        }

        PKIX_PL_NSSCALLRV(HTTPCERTSTORECONTEXT, compareVal, PORT_Strcasecmp,
                (responseContentType, "application/pkcs7-mime"));

        if (compareVal != 0) {
                PKIX_ERROR("Content type is not application/pkcs7-mime");
        }

        PKIX_PL_NSSCALLRV(HTTPCERTSTORECONTEXT, arena, PORT_NewArena,
                (DER_DEFAULT_CHUNKSIZE));

        if (arena == NULL) {
                PKIX_ERROR("Out of Memory");
        }

        if (responseData == NULL) {
                PKIX_ERROR("No responseData in Http Response");
        }

        PKIX_CHECK(PKIX_List_Create(&certs, plContext),
                "PKIX_List_Create failed");

        cbContext.pkixCertList = certs;
        cbContext.plContext = plContext;
        encodedResponse = (char *)responseData;

        PKIX_PL_NSSCALLRV(HTTPCERTSTORECONTEXT, rv, CERT_DecodeCertPackage,
                (encodedResponse, responseDataLen, certCallback, &cbContext));

        if (rv != SECSuccess) {
                PKIX_ERROR("Error decoding pkcs7-mime data");
        }

        *pCertList = cbContext.pkixCertList;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(cbContext.pkixCertList);
        }

        if (arena != NULL) {
                PKIX_PL_NSSCALL(CERTSTORE, PORT_FreeArena, (arena, PR_FALSE));
        }

        PKIX_RETURN(HTTPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_HttpCertStore_ProcessCrlResponse
 * DESCRIPTION:
 *
 *  This function verifies that the response code pointed to by "responseCode"
 *  and the content type pointed to by "responseContentType" are as expected,
 *  and then decodes the data pointed to by "responseData", of length
 *  "responseDataLen", into a List of Crls, possibly empty, which is returned
 *  at "pCrlList".
 *
 * PARAMETERS:
 *  "responseCode"
 *      The value of the HTTP response code.
 *  "responseContentType"
 *      The address of the Content-type string. Must be non-NULL.
 *  "responseData"
 *      The address of the message data. Must be non-NULL.
 *  "responseDataLen"
 *      The length of the message data.
 *  "pCrlList"
 *      The address of the List that is created. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpCertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_HttpCertStore_ProcessCrlResponse(
        PRUint16 responseCode,
        const char *responseContentType,
        const char *responseData,
        PRUint32 responseDataLen,
        PKIX_List **pCrlList,
        void *plContext)
{
        PRArenaPool *arena = NULL;
        SECItem *encodedResponse = NULL;
        PRInt16 compareVal = 0;
        PKIX_List *crls = NULL;

        PKIX_ENTER
                (HTTPCERTSTORECONTEXT,
                "pkix_pl_HttpCertStore_ProcessCrlResponse");
        PKIX_NULLCHECK_ONE(pCrlList);

        if (responseCode != 200) {
                PKIX_ERROR("Bad Http Response");
        }

        /* check that response type is application/pkix-crl */
        if (responseContentType == NULL) {
                PKIX_ERROR("No content type in Http Response");
        }

        PKIX_PL_NSSCALLRV
                (HTTPCERTSTORECONTEXT, compareVal, PORT_Strcasecmp,
                (responseContentType, "application/pkix-crl"));

        if (compareVal != 0) {
                PKIX_ERROR("Content type is not application/pkix-crl");
        }

        /* Make a SECItem of the response data */
        PKIX_PL_NSSCALLRV(HTTPCERTSTORECONTEXT, arena, PORT_NewArena,
                (DER_DEFAULT_CHUNKSIZE));

        if (arena == NULL) {
                PKIX_ERROR("Out of Memory");
        }

        if (responseData == NULL) {
                PKIX_ERROR("No responseData in Http Response");
        }

        PKIX_PL_NSSCALLRV
                (HTTPCERTSTORECONTEXT, encodedResponse, SECITEM_AllocItem,
                (arena, NULL, responseDataLen));

        if (encodedResponse == NULL) {
                PKIX_ERROR("Out of Memory");
        }

        PKIX_PL_NSSCALL(HTTPCERTSTORECONTEXT, PORT_Memcpy,
                (encodedResponse->data, responseData, responseDataLen));

        PKIX_CHECK(PKIX_List_Create(&crls, plContext),
                "PKIX_List_Create failed");

        PKIX_CHECK(pkix_pl_CRL_CreateToList
                (encodedResponse, crls, plContext),
                "pkix_pl_CRL_CreateToList failed");

        *pCrlList = crls;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(crls);
        }

        if (arena != NULL) {
                PKIX_PL_NSSCALL(CERTSTORE, PORT_FreeArena, (arena, PR_FALSE));
        }


        PKIX_RETURN(HTTPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_HttpCertStore_CreateRequestSession
 * DESCRIPTION:
 *
 *  This function takes elements from the HttpCertStoreContext pointed to by
 *  "context" (path, client, and serverSession) and creates a RequestSession.
 *  See the HTTPClient API described in ocspt.h for further details.
 *
 * PARAMETERS:
 *  "context"
 *      The address of the HttpCertStoreContext. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpCertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_HttpCertStore_CreateRequestSession(
        PKIX_PL_HttpCertStoreContext *context,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        SECStatus rv = SECFailure;
        char *pathString = NULL;

        PKIX_ENTER
                (HTTPCERTSTORECONTEXT,
                "pkix_pl_HttpCertStore_CreateRequestSession");
        PKIX_NULLCHECK_TWO(context, context->serverSession);

        pathString = PR_smprintf("%s", context->path);

        if (context->client->version == 1) {
                hcv1 = &(context->client->fcnTable.ftable1);

                if (context->requestSession != NULL) {
                        PKIX_PL_NSSCALL(HTTPCERTSTORECONTEXT, hcv1->freeFcn,
                                (context->requestSession));
                        context->requestSession = 0;
                }

                PKIX_PL_NSSCALLRV
                        (HTTPCERTSTORECONTEXT, rv, hcv1->createFcn,
                        (context->serverSession,
                        "http",
                        pathString,
                        "GET",
                        PR_TicksPerSecond() * 60,
                        &(context->requestSession)));

                if (rv != SECSuccess) {
                        if (pathString != NULL) {
                                PORT_Free(pathString);
                        }
                        PKIX_ERROR("HTTP Server Error");
                }
        } else {
                PKIX_ERROR("Unsupported version of Http Client");
        }

cleanup:

        PKIX_RETURN(HTTPCERTSTORECONTEXT);

}

/*
 * FUNCTION: pkix_pl_HttpCertStore_GetCert
 *  (see description of PKIX_CertStore_CertCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_HttpCertStore_GetCert(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCertList,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;
        void *nbioContext = NULL;
        SECStatus rv = SECFailure;
        PRUint16 responseCode = 0;
        const char *responseContentType = NULL;
        const char *responseData = NULL;
        PRUint32 responseDataLen = 0;
        PKIX_List *certList = NULL;

        PKIX_ENTER(HTTPCERTSTORECONTEXT, "pkix_pl_HttpCertStore_GetCert");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&context, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        if (context->client->version == 1) {
                hcv1 = &(context->client->fcnTable.ftable1);

                PKIX_CHECK(pkix_pl_HttpCertStore_CreateRequestSession
                        (context, plContext),
                        "pkix_pl_HttpCertStore_CreateRequestSession failed");

                PKIX_PL_NSSCALLRV
                        (HTTPCERTSTORECONTEXT, rv, hcv1->trySendAndReceiveFcn,
                        (context->requestSession,
                        (PRPollDesc **)&nbioContext,
                        &responseCode,
                        (const char **)&responseContentType,
                        NULL, /* &responseHeaders */
                        (const char **)&responseData,
                        &responseDataLen));

                if (rv != SECSuccess) {
                        PKIX_ERROR("HTTP Server Error");
                }

                if (nbioContext != 0) {
                        *pNBIOContext = nbioContext;
                        goto cleanup;
                }

        } else {
                PKIX_ERROR("Unsupported version of Http Client");
        }

        PKIX_CHECK(pkix_pl_HttpCertStore_ProcessCertResponse
                (responseCode,
                responseContentType,
                responseData,
                responseDataLen,
                &certList,
                plContext),
                "pkix_pl_HttpCertStore_ProcessCertResponse failed");

        *pCertList = certList;

cleanup:

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_HttpCertStore_GetCertContinue
 *  (see description of PKIX_CertStore_CertCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_HttpCertStore_GetCertContinue(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCertList,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;
        void *nbioContext = NULL;
        SECStatus rv = SECFailure;
        PRUint16 responseCode = 0;
        const char *responseContentType = NULL;
        const char *responseData = NULL;
        PRUint32 responseDataLen = 0;
        PKIX_List *certList = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_HttpCertStore_GetCertContinue");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&context, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        if (context->client->version == 1) {
                hcv1 = &(context->client->fcnTable.ftable1);

                PKIX_NULLCHECK_ONE(context->requestSession);

                PKIX_PL_NSSCALLRV
                        (HTTPCERTSTORECONTEXT, rv, hcv1->trySendAndReceiveFcn,
                        (context->requestSession,
                        (PRPollDesc **)&nbioContext,
                        &responseCode,
                        (const char **)&responseContentType,
                        NULL, /* &responseHeaders */
                        (const char **)&responseData,
                        &responseDataLen));

                if (rv != SECSuccess) {
                        PKIX_ERROR("HTTP Server Error");
                }

                if (nbioContext != 0) {
                        *pNBIOContext = nbioContext;
                        goto cleanup;
                }

        } else {
                PKIX_ERROR("Unsupported version of Http Client");
        }

        PKIX_CHECK(pkix_pl_HttpCertStore_ProcessCertResponse
                (responseCode,
                responseContentType,
                responseData,
                responseDataLen,
                &certList,
                plContext),
                "pkix_pl_HttpCertStore_ProcessCertResponse failed");

        *pCertList = certList;

cleanup:

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_HttpCertStore_GetCRL
 *  (see description of PKIX_CertStore_CRLCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_HttpCertStore_GetCRL(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrlList,
        void *plContext)
{

        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;
        void *nbioContext = NULL;
        SECStatus rv = SECFailure;
        PRUint16 responseCode = 0;
        const char *responseContentType = NULL;
        const char *responseData = NULL;
        PRUint32 responseDataLen = 0;
        PKIX_List *crlList = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_HttpCertStore_GetCRL");
        PKIX_NULLCHECK_THREE(store, selector, pCrlList);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&context, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        if (context->client->version == 1) {
                hcv1 = &(context->client->fcnTable.ftable1);

                PKIX_CHECK(pkix_pl_HttpCertStore_CreateRequestSession
                        (context, plContext),
                        "pkix_pl_HttpCertStore_CreateRequestSession failed");

                PKIX_PL_NSSCALLRV
                        (HTTPCERTSTORECONTEXT, rv, hcv1->trySendAndReceiveFcn,
                        (context->requestSession,
                        (PRPollDesc **)&nbioContext,
                        &responseCode,
                        (const char **)&responseContentType,
                        NULL, /* &responseHeaders */
                        (const char **)&responseData,
                        &responseDataLen));

                if (rv != SECSuccess) {
                        PKIX_ERROR("HTTP Server Error");
                }

                if (nbioContext != 0) {
                        *pNBIOContext = nbioContext;
                        goto cleanup;
                }

        } else {
                PKIX_ERROR("Unsupported version of Http Client");
        }

        PKIX_CHECK(pkix_pl_HttpCertStore_ProcessCrlResponse
                (responseCode,
                responseContentType,
                responseData,
                responseDataLen,
                &crlList,
                plContext),
                "pkix_pl_HttpCertStore_ProcessCrlResponse failed");

        *pCrlList = crlList;

cleanup:

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_HttpCertStore_GetCRLContinue
 *  (see description of PKIX_CertStore_CRLCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_HttpCertStore_GetCRLContinue(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrlList,
        void *plContext)
{
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *context = NULL;
        void *nbioContext = NULL;
        SECStatus rv = SECFailure;
        PRUint16 responseCode = 0;
        const char *responseContentType = NULL;
        const char *responseData = NULL;
        PRUint32 responseDataLen = 0;
        PKIX_List *crlList = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_HttpCertStore_GetCRLContinue");
        PKIX_NULLCHECK_FOUR(store, selector, pNBIOContext, pCrlList);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&context, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        if (context->client->version == 1) {
                hcv1 = &(context->client->fcnTable.ftable1);

                PKIX_CHECK(pkix_pl_HttpCertStore_CreateRequestSession
                        (context, plContext),
                        "pkix_pl_HttpCertStore_CreateRequestSession failed");

                PKIX_PL_NSSCALLRV
                        (HTTPCERTSTORECONTEXT, rv, hcv1->trySendAndReceiveFcn,
                        (context->requestSession,
                        (PRPollDesc **)&nbioContext,
                        &responseCode,
                        (const char **)&responseContentType,
                        NULL, /* &responseHeaders */
                        (const char **)&responseData,
                        &responseDataLen));

                if (rv != SECSuccess) {
                        PKIX_ERROR("HTTP Server Error");
                }

                if (nbioContext != 0) {
                        *pNBIOContext = nbioContext;
                        goto cleanup;
                }

        } else {
                PKIX_ERROR("Unsupported version of Http Client");
        }

        PKIX_CHECK(pkix_pl_HttpCertStore_ProcessCrlResponse
                (responseCode,
                responseContentType,
                responseData,
                responseDataLen,
                &crlList,
                plContext),
                "pkix_pl_HttpCertStore_ProcessCrlResponse failed");

        *pCrlList = crlList;

cleanup:

        PKIX_RETURN(CERTSTORE);
}

/* --Public-HttpCertStore-Functions----------------------------------- */

/*
 * FUNCTION: pkix_pl_HttpCertStore_CreateWithAsciiName
 * DESCRIPTION:
 *
 *  This function uses the HttpClient pointed to by "client" and the string
 *  (hostname:portnum/path, with portnum optional) pointed to by "locationAscii"
 *  to create an HttpCertStore connected to the desired location, storing the
 *  created CertStore at "pCertStore".
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpClient. Must be non-NULL.
 *  "locationAscii"
 *      The address of the character string indicating the hostname, port, and
 *      path to be queried for Certs or Crls. Must be non-NULL.
 *  "pCertStore"
 *      The address in which the object is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpCertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_HttpCertStore_CreateWithAsciiName(
        PKIX_PL_HttpClient *client,
        char *locationAscii,
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        const SEC_HttpClientFcn *clientFcn = NULL;
        const SEC_HttpClientFcnV1 *hcv1 = NULL;
        PKIX_PL_HttpCertStoreContext *httpCertStore = NULL;
        PKIX_CertStore *certStore = NULL;
        PKIX_PL_String *locationString = NULL;
        char *hostname = NULL;
        char *path = NULL;
        PKIX_UInt32 len = 0;
        PRUint16 port = 0;
        SECStatus rv = SECFailure;

        PKIX_ENTER(CERTSTORE, "pkix_pl_HttpCertStore_CreateWithAsciiName");
        PKIX_NULLCHECK_TWO(locationAscii, pCertStore);

        if (client == NULL) {
                clientFcn = GetRegisteredHttpClient();
                if (clientFcn == NULL) {
                        PKIX_ERROR("No registered Http Client");
                }
        } else {
                clientFcn = (const SEC_HttpClientFcn *)client;
        }

        /* create a PKIX_PL_HttpCertStore object */
        PKIX_CHECK(PKIX_PL_Object_Alloc
                  (PKIX_HTTPCERTSTORECONTEXT_TYPE,
                  sizeof (PKIX_PL_HttpCertStoreContext),
                  (PKIX_PL_Object **)&httpCertStore,
                  plContext),
                  "Could not create object");

        /* Initialize fields */
        httpCertStore->client = clientFcn; /* not a PKIX object! */

        /* parse location -> hostname, port, path */
        PKIX_PL_NSSCALLRV(CERTSTORE, rv, CERT_ParseURL,
                (locationAscii, &hostname, &port, &path));

        if ((hostname == NULL) || (path == NULL)) {
                PKIX_ERROR("URL Parsing failed");
        }

        httpCertStore->path = path;

        if (clientFcn->version == 1) {

                hcv1 = &(clientFcn->fcnTable.ftable1);

                PKIX_PL_NSSCALLRV
                        (HTTPCERTSTORECONTEXT,
                        rv,
                        hcv1->createSessionFcn,
                        (hostname, port, &(httpCertStore->serverSession)));

                if (rv != SECSuccess) {
                        PKIX_ERROR("HttpClient->CreateSession failed");
                }
        } else {
                PKIX_ERROR("Unsupported version of Http Client");
        }

        httpCertStore->requestSession = NULL;

        PKIX_CHECK(PKIX_CertStore_Create
                (pkix_pl_HttpCertStore_GetCert,
                pkix_pl_HttpCertStore_GetCRL,
                pkix_pl_HttpCertStore_GetCertContinue,
                pkix_pl_HttpCertStore_GetCRLContinue,
                NULL,       /* don't support trust */
                (PKIX_PL_Object *)httpCertStore,
                PKIX_TRUE,  /* cache flag */
                PKIX_FALSE, /* not local */
                &certStore,
                plContext),
                "PKIX_CertStore_Create failed");

        *pCertStore = certStore;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(httpCertStore);
        }

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: PKIX_PL_HttpCertStore_Create
 * (see comments in pkix_samples_modules.h)
 */
PKIX_Error *
PKIX_PL_HttpCertStore_Create(
        PKIX_PL_HttpClient *client,
        PKIX_PL_GeneralName *location,
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        PKIX_PL_String *locationString = NULL;
        char *locationAscii = NULL;
        PKIX_UInt32 len = 0;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_HttpCertStore_Create");
        PKIX_NULLCHECK_TWO(location, pCertStore);

        PKIX_TOSTRING(location, &locationString, plContext,
                "PKIX_PL_GeneralName_ToString failed");

        PKIX_CHECK(PKIX_PL_String_GetEncoded
                (locationString,
                PKIX_ESCASCII,
                (void **)&locationAscii,
                &len,
                plContext),
                "PKIX_PL_String_GetEncoded failed");

        PKIX_CHECK(pkix_pl_HttpCertStore_CreateWithAsciiName
                (client, locationAscii, pCertStore, plContext),
                "PKIX_PL_HttpCertStore_CreateWithAsciiName failed");

cleanup:

        PKIX_DECREF(locationString);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_HttpCertStore_FindSocketConnection
 * DESCRIPTION:
 *
        PRIntervalTime timeout,
        char *hostname,
        PRUint16 portnum,
        PRErrorCode *pStatus,
        PKIX_PL_Socket **pSocket,

 *  This function checks for an existing socket, creating a new one if unable
 *  to find an existing one, for the host pointed to by "hostname" and the port
 *  pointed to by "portnum". If a new socket is created the PRIntervalTime in
 *  "timeout" will be used for the timeout value and a creation status is
 *  returned at "pStatus". The address of the socket is stored at "pSocket".
 *
 * PARAMETERS:
 *  "timeout"
 *      The PRIntervalTime of the timeout value.
 *  "hostname"
 *      The address of the string containing the hostname. Must be non-NULL.
 *  "portnum"
 *      The port number for the desired socket.
 *  "pStatus"
 *      The address at which the status is stored. Must be non-NULL.
 *  "pSocket"
 *      The address at which the socket is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpCertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_HttpCertStore_FindSocketConnection(
        PRIntervalTime timeout,
        char *hostname,
        PRUint16 portnum,
        PRErrorCode *pStatus,
        PKIX_PL_Socket **pSocket,
        void *plContext)
{
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *hostString = NULL;
        PKIX_PL_String *domainString = NULL;
        PKIX_PL_Socket *socket = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_HttpCertStore_FindSocketConnection");
        PKIX_NULLCHECK_THREE(hostname, pStatus, pSocket);

        *pStatus = 0;

        /* create PKIX_PL_String from hostname and port */
        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, "%s:%d", 0, &formatString, plContext),
                "PKIX_PL_String_Create failed");

hostname = "variation.red.iplanet.com";
portnum = 2001;

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, hostname, 0, &hostString, plContext),
                "PKIX_PL_String_Create failed");

        PKIX_CHECK(PKIX_PL_Sprintf
                (&domainString, plContext, formatString, hostString, portnum),
                "PKIX_PL_String_Create failed");

        /* Is this domainName already in cache? */
        PKIX_CHECK(PKIX_PL_HashTable_Lookup
                (httpSocketCache,
                (PKIX_PL_Object *)domainString,
                (PKIX_PL_Object **)&socket,
                plContext),
                "PKIX_PL_HashTable_Lookup failed");

        if (socket == NULL) {

                /* No, create a connection (and cache it) */
                PKIX_CHECK(pkix_pl_Socket_CreateByHostAndPort
                        (PKIX_FALSE,       /* create a client, not a server */
                        timeout,
                        hostname,
                        portnum,
                        pStatus,
                        &socket,
                        plContext),
                        "pkix_pl_Socket_CreateByHostAndPort failed");

                PKIX_CHECK(PKIX_PL_HashTable_Add
                        (httpSocketCache,
                        (PKIX_PL_Object *)domainString,
                        (PKIX_PL_Object *)socket,
                        plContext),
                        "PKIX_PL_HashTable_Add failed");

        }

        *pSocket = socket;

cleanup:

        PKIX_DECREF(formatString);
        PKIX_DECREF(hostString);
        PKIX_DECREF(domainString);

        PKIX_RETURN(CERTSTORE);
}
