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
 * pkix_pl_aiamgr.c
 *
 * AIAMgr Object Definitions
 *
 */

#include "pkix_pl_aiamgr.h"
extern PKIX_PL_HashTable *aiaConnectionCache;

/* --Virtual-LdapClient-Functions------------------------------------ */

PKIX_Error *
PKIX_PL_LdapClient_InitiateRequest(
        PKIX_PL_LdapClient *client,
        LDAPRequestParams *requestParams,
        void **pNBIO,
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_ENTER(LDAPCLIENT, "PKIX_PL_LdapClient_InitiateRequest");
        PKIX_NULLCHECK_TWO(client, client->initiateFcn);

        PKIX_CHECK(client->initiateFcn
                (client, requestParams, pNBIO, pResponse, plContext),
                "PKIX_PL_LdapClient_InitiateRequest failed");
cleanup:

        PKIX_RETURN(LDAPCLIENT);

}

PKIX_Error *
PKIX_PL_LdapClient_ResumeRequest(
        PKIX_PL_LdapClient *client,
        void **pNBIO,
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_ENTER(LDAPCLIENT, "PKIX_PL_LdapClient_ResumeRequest");
        PKIX_NULLCHECK_TWO(client, client->resumeFcn);

        PKIX_CHECK(client->resumeFcn
                (client, pNBIO, pResponse, plContext),
                "PKIX_PL_LdapClient_ResumeRequest failed");
cleanup:

        PKIX_RETURN(LDAPCLIENT);

}

/* --Private-AIAMgr-Functions----------------------------------*/

/*
 * FUNCTION: pkix_pl_AIAMgr_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_pki.h)
 */
static PKIX_Error *
pkix_pl_AIAMgr_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_AIAMgr *aiaMgr = NULL;

        PKIX_ENTER(AIAMGR, "pkix_pl_AIAMgr_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_AIAMGR_TYPE, plContext),
                "Object is not a AIAMgr");

        aiaMgr = (PKIX_PL_AIAMgr *)object;

        /* pointer to cert cache */
        /* pointer to crl cache */
        aiaMgr->method = 0;
        aiaMgr->aiaIndex = 0;
        aiaMgr->numAias = 0;
        PKIX_DECREF(aiaMgr->aia);
        PKIX_DECREF(aiaMgr->location);
        PKIX_DECREF(aiaMgr->results);
        PKIX_DECREF(aiaMgr->client);

cleanup:

        PKIX_RETURN(AIAMGR);
}

/*
 * FUNCTION: pkix_pl_AIAMgr_RegisterSelf
 * DESCRIPTION:
 *  Registers PKIX_AIAMGR_TYPE and its related functions with systemClasses[]
 * THREAD SAFETY:
 *  Not Thread Safe - for performance and complexity reasons
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_pl_AIAMgr_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(AIAMGR,
                "pkix_pl_AIAMgr_RegisterSelf");

        entry.description = "AIAMgr";
        entry.destructor = pkix_pl_AIAMgr_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_AIAMGR_TYPE] = entry;

        PKIX_RETURN(AIAMGR);
}

/*
 * FUNCTION: pkix_pl_AIAMgr_ConvertResponses
 * DESCRIPTION:
 *
 *  This function processes the List of LDAPResponses pointed to by "responses" into
 *  a List of resulting Certs, storing the result at "pCerts". If there are no responses
 *  converted successfully, a NULL may be stored.
 *
 * PARAMETERS:
 *  "responses"
 *      The LDAPResponses whose contents are to be converted. Must be non-NULL.
 *  "pCerts"
 *      Address at which the returned List is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns an AIAMgr Error if the function fails in a non-fatal way
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_AIAMgr_ConvertResponses(
        PKIX_List *responses,
        PKIX_List **pCerts,
        void *plContext)
{
        PKIX_List *certs = NULL;

        PKIX_ENTER(AIAMGR, "pkix_pl_AIAMgr_ConvertResponses");
        PKIX_NULLCHECK_TWO(responses, pCerts);

        *pCerts = NULL;

         if (responses != NULL) {
                PKIX_CHECK(pkix_pl_LdapCertStore_ConvertCertResponses
                        (responses,
                        PKIX_TRUE,
                        &certs,
                        plContext),
                        "pkix_pl_LdapCertStore_ConvertCertResponses failed");
                *pCerts = certs;
        }

cleanup:

        PKIX_RETURN(AIAMGR);

}

/*
 * FUNCTION: pkix_pl_AiaMgr_FindLDAPClient
 * DESCRIPTION:
 *
 *  This function checks the collection of LDAPClient connections held by the
 *  AIAMgr pointed to by "aiaMgr" for one matching the domain name given by
 *  "domainName". The string may include a port number: e.g., "betty.nist.gov"
 *  or "nss.red.iplanet.com:1389". If a match is found, that LDAPClient is
 *  stored at "pClient". Otherwise, an LDAPClient is created and added to the
 *  collection, and then stored at "pClient".
 *
 * PARAMETERS:
 *  "aiaMgr"
 *      The AIAMgr whose LDAPClient connected are to be managed. Must be
 *      non-NULL.
 *  "domainName"
 *      Address of a string pointing to a server name. Must be non-NULL.
 *  "pClient"
 *      Address at which the returned LDAPClient is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns an AIAMgr Error if the function fails in a non-fatal way
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_AiaMgr_FindLDAPClient(
        PKIX_PL_AIAMgr *aiaMgr,
        char *domainName,
        PKIX_PL_LdapClient **pClient,
        void *plContext)
{
        PKIX_PL_String *domainString = NULL;
        PKIX_PL_LdapDefaultClient *client = NULL;

        PKIX_ENTER(AIAMGR, "pkix_pl_AiaMgr_FindLDAPClient");
        PKIX_NULLCHECK_THREE(aiaMgr, domainName, pClient);

        /* create PKIX_PL_String from domain name */
        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, domainName, 0, &domainString, plContext),
                "PKIX_PL_String_Create failed");

        /* Is this domainName already in cache? */
        PKIX_CHECK(PKIX_PL_HashTable_Lookup
                (aiaConnectionCache,
                (PKIX_PL_Object *)domainString,
                (PKIX_PL_Object **)&client,
                plContext),
                "PKIX_PL_HashTable_Lookup failed");

        if (client == NULL) {

                /* No, create a connection (and cache it) */
                PKIX_CHECK(PKIX_PL_LdapDefaultClient_CreateByName
                        (domainName,
                        PR_INTERVAL_NO_TIMEOUT,
                        NULL,
                        &client,
                        plContext),
                        "PKIX_PL_LdapDefaultClient_CreateByName failed");

                PKIX_CHECK(PKIX_PL_HashTable_Add
                        (aiaConnectionCache,
                        (PKIX_PL_Object *)domainString,
                        (PKIX_PL_Object *)client,
                        plContext),
                        "PKIX_PL_HashTable_Add failed");

        }

        *pClient = (PKIX_PL_LdapClient *)client;

cleanup:

        PKIX_DECREF(domainString);

        PKIX_RETURN(AIAMGR);
}

/*
 * FUNCTION: PKIX_PL_AIAMgr_Create
 * DESCRIPTION:
 *
 *  This function creates an AIAMgr, storing the result at "pAIAMgr".
 *
 * PARAMETERS:
 *  "pAIAMGR"
 *      Address at which the returned AIAMgr is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns an AIAMgr Error if the function fails in a non-fatal way
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
PKIX_PL_AIAMgr_Create(
        PKIX_PL_AIAMgr **pAIAMgr,
        void *plContext)
{
        PKIX_PL_AIAMgr *aiaMgr = NULL;

        PKIX_ENTER(AIAMGR, "PKIX_PL_AIAMgr_Create");
        PKIX_NULLCHECK_ONE(pAIAMgr);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_AIAMGR_TYPE,
                sizeof(PKIX_PL_AIAMgr),
                (PKIX_PL_Object **)&aiaMgr,
                plContext),
                "Could not create AiaMgr object");
        /* pointer to cert cache */
        /* pointer to crl cache */
        aiaMgr->method = 0;
        aiaMgr->aiaIndex = 0;
        aiaMgr->numAias = 0;
        aiaMgr->aia = NULL;
        aiaMgr->location = NULL;
        aiaMgr->results = NULL;
        aiaMgr->client = NULL;

        *pAIAMgr = aiaMgr;

cleanup:

        PKIX_RETURN(AIAMGR);
}

/* --Public-Functions------------------------------------------------------- */

/*
 * FUNCTION: PKIX_PL_AIAMgr_GetAIACerts (see description in pkix_pl_pki.h)
 */
PKIX_Error *
PKIX_PL_AIAMgr_GetAIACerts(
        PKIX_PL_AIAMgr *aiaMgr,
        PKIX_PL_Cert *prevCert,
        void **pNBIOContext,
        PKIX_List **pCerts,
        void *plContext)
{
        PKIX_UInt32 numAias = 0;
        PKIX_UInt32 aiaIndex = 0;
        PKIX_UInt32 iaType = PKIX_INFOACCESS_LOCATION_UNKNOWN;
        PKIX_List *result = NULL;
        PKIX_List *certs = NULL;
        PKIX_PL_InfoAccess *ia = NULL;
        PKIX_PL_GeneralName *location = NULL;
        PKIX_PL_LdapDefaultClient *newClient = NULL;
        PKIX_PL_LdapClient *client = NULL;
        void *nbio = NULL;
        PRArenaPool *arena = NULL;
        char *domainName = NULL;
        LDAPRequestParams request;

        PKIX_ENTER(AIAMGR, "PKIX_PL_AIAMgr_GetAIACerts");
        PKIX_NULLCHECK_FOUR(aiaMgr, prevCert, pNBIOContext, pCerts);

        nbio = *pNBIOContext;
        *pCerts = NULL;
        *pNBIOContext = NULL;

        if (nbio == NULL) { /* a new request */

                /* Does this Cert have an AIA extension? */
                PKIX_CHECK(PKIX_PL_Cert_GetAuthorityInfoAccess
                        (prevCert, &aiaMgr->aia, plContext),
                        "PKIX_PL_Cert_GetAuthorityInfoAccess failed");

                if (aiaMgr->aia != NULL) {
                        PKIX_CHECK(PKIX_List_GetLength
                                (aiaMgr->aia, &numAias, plContext),
                                "PKIX_List_GetLength failed");
                }

                /* And if so, does it have any entries? */
                if ((aiaMgr->aia == NULL) || (numAias == 0)) {
                        *pCerts = NULL;
                        goto cleanup;
                }

                aiaMgr->aiaIndex = 0;
                aiaMgr->numAias = numAias;
                aiaMgr->results = NULL;

        }

        for (aiaIndex = aiaMgr->aiaIndex;
                aiaIndex < aiaMgr->numAias;
                aiaIndex ++) {

                PKIX_CHECK(PKIX_List_GetItem
                        (aiaMgr->aia,
                        aiaIndex,
                        (PKIX_PL_Object **)&ia,
                        plContext),
                        "PKIX_List_GetItem failed");

                PKIX_CHECK(PKIX_PL_InfoAccess_GetLocationType
                        (ia, &iaType, plContext),
                        "PKIX_PL_InfoAccess_GetLocationType failed");

                if (iaType == PKIX_INFOACCESS_LOCATION_LDAP) {

                        if (nbio == NULL) { /* a new request */

                                /* Initiate an LDAP request */

                                request.scope = WHOLE_SUBTREE;
                                request.derefAliases = NEVER_DEREF;
                                request.sizeLimit = 0;
                                request.timeLimit = 0;

                                PKIX_CHECK(PKIX_PL_InfoAccess_GetLocation
                                        (ia, &location, plContext),
                                       "PKIX_PL_InfoAccess_GetLocation failed");

                                /*
                                 * Get a short-lived arena. We'll be done with
                                 * this space once the request is encoded.
                                 */
                                PKIX_PL_NSSCALLRV(AIAMGR, arena, PORT_NewArena,
                                        (DER_DEFAULT_CHUNKSIZE));

                                if (!arena) {
                                        PKIX_ERROR_FATAL("Out of memory");
                                }

                                PKIX_CHECK(pkix_pl_InfoAccess_ParseLocation
                                        (location,
                                        arena,
                                        &request,
                                        &domainName,
                                        plContext),
                                        "pkix_pl_InfoAccess_ParseLocation"
                                        " failed");

                                PKIX_DECREF(location);

                                /* Find or create a connection to LDAP server */
                                PKIX_CHECK(pkix_pl_AiaMgr_FindLDAPClient
                                        (aiaMgr,
                                        domainName,
                                        &client,
                                        plContext),
                                        "pkix_pl_AiaMgr_FindLDAPClient failed");

                                aiaMgr->client = client;

                                PKIX_CHECK(PKIX_PL_LdapClient_InitiateRequest
                                        (aiaMgr->client,
                                        &request,
                                        &nbio,
                                        &result,
                                        plContext),
                                        "PKIX_PL_LdapClient_InitiateRequest"
                                        " failed");

                                PKIX_PL_NSSCALL(AIAMGR, PORT_FreeArena,
                                        (arena, PR_FALSE));

                        } else {

                                PKIX_CHECK(PKIX_PL_LdapClient_ResumeRequest
                                        (aiaMgr->client,
                                        &nbio,
                                        &result,
                                        plContext),
                                        "PKIX_PL_LdapClient_ResumeRequest failed");
                        }

                } else {
                        /* We only support ldap requests at this time. */
                }

                if (nbio != NULL) { /* WOULDBLOCK */
                        aiaMgr->aiaIndex = aiaIndex;
                        *pNBIOContext = nbio;
                        *pCerts = NULL;
                        goto cleanup;
                }

                /*
                 * We can't just use and modify the List we received.
                 * Because it's cached, it's set immutable.
                 */
                if (aiaMgr->results == NULL) {
                        PKIX_CHECK(PKIX_List_Create
                                (&(aiaMgr->results), plContext),
                                "PKIX_List_Create failed");
                }
                PKIX_CHECK(pkix_List_AppendList
                        (aiaMgr->results, result, plContext),
                        "pkix_pl_AppendList failed");
                PKIX_DECREF(result);

                PKIX_DECREF(ia);
                PKIX_DECREF(aiaMgr->client);
        }

        PKIX_DECREF(aiaMgr->aia);

        if (aiaMgr->results != NULL) {
                PKIX_CHECK(pkix_pl_AIAMgr_ConvertResponses
                        (aiaMgr->results, &certs, plContext),
                        "pkix_pl_AIAMgr_ConvertResponses failed");
        }

        *pNBIOContext = NULL;
        *pCerts = certs;
        PKIX_DECREF(aiaMgr->results);

cleanup:

        if (arena && (PKIX_ERROR_RECEIVED)) {
                PKIX_PL_NSSCALL(AIAMGR, PORT_FreeArena, (arena, PR_FALSE));
        }

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(aiaMgr->aia);
                PKIX_DECREF(aiaMgr->results);
                PKIX_DECREF(aiaMgr->client);
        }

        PKIX_DECREF(ia);
        PKIX_DECREF(location);

        PKIX_RETURN(AIAMGR);
}
