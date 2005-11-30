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
 * pkix_pl_ldapcertstore.c
 *
 * LDAPCertStore Function Definitions
 *
 */

/* We can't decode the length of a message without at least this many bytes */
#define MINIMUM_MSG_LENGTH 5

#include "pkix_pl_ldapcertstore.h"

/* --Virtual-LdapClient-Functions------------------------------------ */

PKIX_Error *
PKIX_PL_LdapClient_InitiateRequest(
        PKIX_PL_LdapClient *client,
        LDAPRequestParams *requestParams,
        PRPollDesc **pPollDesc,
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_ENTER(LDAPCLIENT, "PKIX_PL_LdapClient_InitiateRequest");
        PKIX_NULLCHECK_TWO(client, client->initiateFcn);

        PKIX_CHECK(client->initiateFcn
                (client, requestParams, pPollDesc, pResponse, plContext),
                "PKIX_PL_LdapClient_InitiateRequest failed");
cleanup:

        PKIX_RETURN(LDAPCLIENT);

}

PKIX_Error *
PKIX_PL_LdapClient_ResumeRequest(
        PKIX_PL_LdapClient *client,
        PRPollDesc **pPollDesc,
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_ENTER(LDAPCLIENT, "PKIX_PL_LdapClient_ResumeRequest");
        PKIX_NULLCHECK_TWO(client, client->resumeFcn);

        PKIX_CHECK(client->resumeFcn
                (client, pPollDesc, pResponse, plContext),
                "PKIX_PL_LdapClient_ResumeRequest failed");
cleanup:

        PKIX_RETURN(LDAPCLIENT);

}

/* --Private-Ldap-CertStore-Database-Functions----------------------- */

/*
 * FUNCTION: pkix_pl_LdapCertStore_DecodeCert
 * DESCRIPTION:
 *
 *  This function decodes a DER-encoded Certificate pointed to by "derCertItem",
 *  adding the resulting PKIX_PL_Cert, if the decoding was successful, to the
 *  List (possibly empty) pointed to by "certList".
 *
 * PARAMETERS:
 *  "derCertItem"
 *      The address of the SECItem containing the DER-encoded Certificate. Must
 *      be non-NULL.
 *  "certList"
 *      The address of the List to which the decoded Certificate is added. Must
 *      be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapCertStore_DecodeCert(
        SECItem *derCertItem,
        PKIX_List *certList,
        void *plContext)
{
        CERTCertificate *nssCert = NULL;
        PKIX_PL_Cert *cert = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_DecodeCert");
        PKIX_NULLCHECK_TWO(derCertItem, certList);

        PKIX_PL_NSSCALLRV(CERTSTORE, nssCert, CERT_DecodeDERCertificate,
                (derCertItem, PR_FALSE, NULL));

        if (nssCert) {
                PKIX_CHECK_ONLY_FATAL(pkix_pl_Cert_CreateWithNSSCert
                        (nssCert, &cert, plContext),
                        "pkix_pl_Cert_CreateWithNSSCert failed");

                /* skip bad certs and append good ones */
                if (!PKIX_ERROR_RECEIVED) {
                        PKIX_CHECK(PKIX_List_AppendItem
                                (certList, (PKIX_PL_Object *) cert, plContext),
                                "PKIX_List_AppendItem failed");
                }

                PKIX_DECREF(cert);
        }
cleanup:

        PKIX_DECREF(cert);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_DecodeCrl
 * DESCRIPTION:
 *
 *  This function decodes a DER-encoded Certificate Revocation List pointed to
 *  by "derCrlItem", adding the resulting PKIX_PL_CRL, if the decoding was
 *  successful, to the List (possibly empty) pointed to by "crlList".
 *
 * PARAMETERS:
 *  "derCrlItem"
 *      The address of the SECItem containing the DER-encoded Certificate
 *      Revocation List. Must be non-NULL.
 *  "crlList"
 *      The address of the List to which the decoded CRL is added. Must be
 *      non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapCertStore_DecodeCrl(
        SECItem *derCrlItem,
        PKIX_List *crlList,
        void *plContext)
{
        CERTSignedCrl *nssCrl = NULL;
        PKIX_PL_CRL *crl = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_DecodeCrl");
        PKIX_NULLCHECK_TWO(derCrlItem, crlList);

        PKIX_PL_NSSCALLRV(CERTSTORE, nssCrl, CERT_DecodeDERCrl,
                (NULL, derCrlItem, SEC_CRL_TYPE));

        if (nssCrl) {
                PKIX_CHECK_ONLY_FATAL(pkix_pl_CRL_CreateWithSignedCRL
                        (nssCrl, &crl, plContext),
                        "pkix_pl_CRL_CreateWithSignedCRL failed");
    
                /* skip bad crls and append good ones */
                if (!PKIX_ERROR_RECEIVED) {
                        PKIX_CHECK(PKIX_List_AppendItem
                                (crlList, (PKIX_PL_Object *) crl, plContext),
                                "PKIX_List_AppendItem failed");
                }

                PKIX_DECREF(crl);

        }
cleanup:

        PKIX_DECREF(crl);

        PKIX_RETURN(CERTSTORE);
}

PKIX_Error *
pkix_pl_LdapCertStore_DecodeCrossCertPair(
        SECItem *derCCPItem,
        PKIX_List *certList,
        void *plContext)
{
        LDAPCertPair certPair = {{ siBuffer, NULL, 0 }, { siBuffer, NULL, 0 }};
        CERTCertificate *nssCert = NULL;
        PKIX_PL_Cert *cert = NULL;
        SECStatus rv = SECFailure;

        PRArenaPool *arena = NULL; /* xxx need to initialize this? */

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_DecodeCrossCertPair");
        PKIX_NULLCHECK_THREE(derCCPItem, arena, certList);

        PKIX_PL_NSSCALLRV(CERTSTORE, rv, SEC_ASN1DecodeItem,
                (arena,
                &certPair,
                PKIX_PL_LDAPCrossCertPairTemplate,
                derCCPItem));

        if (rv != SECSuccess) {
                goto cleanup;
        }

        if (certPair.forward.data != NULL) {

                PKIX_PL_NSSCALLRV
                        (CERTSTORE, nssCert, CERT_DecodeDERCertificate,
                        (&certPair.forward, PR_FALSE, NULL));

                if (nssCert) {
                        PKIX_CHECK_ONLY_FATAL(pkix_pl_Cert_CreateWithNSSCert
                                (nssCert, &cert, plContext),
                                "pkix_pl_Cert_CreateWithNSSCert failed");

                        /* skip bad certs and append good ones */
                        if (!PKIX_ERROR_RECEIVED) {
                                PKIX_CHECK(PKIX_List_AppendItem
                                        (certList,
                                        (PKIX_PL_Object *) cert,
                                        plContext),
                                        "PKIX_List_AppendItem failed");
                        }

                        PKIX_DECREF(cert);
                }
        }

        if (certPair.reverse.data != NULL) {

                PKIX_PL_NSSCALLRV
                        (CERTSTORE, nssCert, CERT_DecodeDERCertificate,
                        (&certPair.reverse, PR_FALSE, NULL));

                if (nssCert) {
                        PKIX_CHECK_ONLY_FATAL(pkix_pl_Cert_CreateWithNSSCert
                                (nssCert, &cert, plContext),
                                "pkix_pl_Cert_CreateWithNSSCert failed");

                        /* skip bad certs and append good ones */
                        if (!PKIX_ERROR_RECEIVED) {
                                PKIX_CHECK(PKIX_List_AppendItem
                                        (certList,
                                        (PKIX_PL_Object *) cert,
                                        plContext),
                                        "PKIX_List_AppendItem failed");
                        }

                        PKIX_DECREF(cert);
                }
        }

cleanup:

        PKIX_DECREF(cert);

        PKIX_RETURN(CERTSTORE);
}
/*
 * FUNCTION: pkix_pl_LdapCertStore_BuildCertList
 * DESCRIPTION:
 *
 *  This function takes a List of LdapResponse objects pointed to by
 *  "responseList" and extracts and decodes the Certificates in those responses,
 *  storing the List of those Certificates at "pCerts". If none of the objects
 *  can be decoded into a Cert, the returned List is empty.
 *
 * PARAMETERS:
 *  "responseList"
 *      The address of the List of LdapResponses. Must be non-NULL.
 *  "pCerts"
 *      The address at which the result is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapCertStore_BuildCertList(
        PKIX_List *responseList,
        PKIX_List **pCerts,
        void *plContext)
{
        PKIX_UInt32 numResponses = 0;
        PKIX_UInt32 respIx = 0;
        LdapAttrMask attrBits = 0;
        PKIX_PL_LdapResponse *response = NULL;
        PKIX_List *certList = NULL;
        LDAPMessage *message = NULL;
        LDAPSearchResponseEntry *sre = NULL;
        LDAPSearchResponseAttr **sreAttrArray = NULL;
        LDAPSearchResponseAttr *sreAttr = NULL;
        SECItem *attrType = NULL;
        SECItem **attrVal = NULL;
        SECItem *derCertItem = NULL;


        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_BuildCertList");
        PKIX_NULLCHECK_TWO(responseList, pCerts);

        PKIX_CHECK(PKIX_List_Create(&certList, plContext),
                "PKIX_List_Create failed");

        /* extract certs from response */
        PKIX_CHECK(PKIX_List_GetLength
                (responseList, &numResponses, plContext),
                "PKIX_List_GetLength failed");

        for (respIx = 0; respIx < numResponses; respIx++) {
                PKIX_CHECK(PKIX_List_GetItem
                        (responseList,
                        respIx,
                        (PKIX_PL_Object **)&response,
                        plContext),
                        "PKIX_List_GetItem failed");

                PKIX_CHECK(pkix_pl_LdapResponse_GetMessage
                        (response, &message, plContext),
                        "pkix_pl_LdapResponse_GetMessage failed");

                sre = &(message->protocolOp.op.searchResponseEntryMsg);
                sreAttrArray = sre->attributes;

                /* Get next element of null-terminated array */
                sreAttr = *sreAttrArray++;
                while (sreAttr != NULL) {
                    attrType = &(sreAttr->attrType);
                    PKIX_CHECK(pkix_pl_LdapRequest_AttrTypeToBit
                        (attrType, &attrBits, plContext),
                        "pkix_pl_LdapRequest_AttrTypeToBit failed");
                    /* Is this attrVal a Certificate? */
                    if (((LDAPATTR_CACERT | LDAPATTR_USERCERT) &
                            attrBits) == attrBits) {
                        attrVal = sreAttr->val;
                        derCertItem = *attrVal++;
                        while (derCertItem != 0) {
                            /* create a PKIX_PL_Cert from derCert */
                            PKIX_CHECK(pkix_pl_LdapCertStore_DecodeCert
                                (derCertItem, certList, plContext),
                                "pkix_pl_LdapCertStore_DecodeCert failed");
                            derCertItem = *attrVal++;
                        }
#if 0
                    } else if ((LDAPATTR_CROSSPAIRCERT & attrBits) == attrBits){
                        /* Is this attrVal a CrossPairCertificate? */
                        attrVal = sreAttr->val;
                        derCertItem = *attrVal++;
                        while (derCertItem != 0) {
                            /* create PKIX_PL_Certs from derCert */
                            PKIX_CHECK(pkix_pl_LdapCertStore_DecodeCrossCertPair
                                (derCertItem, certList, plContext),
                                "pkix_pl_LdapCertStore_DecodeCrossCertPair"
                                " failed");
                            derCertItem = *attrVal++;
                        }
#endif
                    }
                    sreAttr = *sreAttrArray++;
                }
                PKIX_DECREF(response);
        }

        *pCerts = certList;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(certList);
        }

        PKIX_DECREF(response);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_BuildCrlList
 * DESCRIPTION:
 *
 *  This function takes a List of LdapResponse objects pointed to by
 *  "responseList" and extracts and decodes the CRLs in those responses, storing
 *  the List of those CRLs at "pCrls". If none of the objects can be decoded
 *  into a CRL, the returned List is empty.
 *
 * PARAMETERS:
 *  "responseList"
 *      The address of the List of LdapResponses. Must be non-NULL.
 *  "pCrls"
 *      The address at which the result is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapCertStore_BuildCrlList(
        PKIX_List *responseList,
        PKIX_List **pCrls,
        void *plContext)
{
        PKIX_UInt32 numResponses = 0;
        PKIX_UInt32 respIx = 0;
        LdapAttrMask attrBits = 0;
        PKIX_PL_LdapResponse *response = NULL;
        PKIX_List *crlList = NULL;
        LDAPMessage *message = NULL;
        LDAPSearchResponseEntry *sre = NULL;
        LDAPSearchResponseAttr **sreAttrArray = NULL;
        LDAPSearchResponseAttr *sreAttr = NULL;
        SECItem *attrType = NULL;
        SECItem **attrVal = NULL;

        SECItem *derCrlItem = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_BuildCrlList");
        PKIX_NULLCHECK_TWO(responseList, pCrls);

        PKIX_CHECK(PKIX_List_Create(&crlList, plContext),
                "PKIX_List_Create failed");

        /* extract crls from response */
        PKIX_CHECK(PKIX_List_GetLength
                (responseList, &numResponses, plContext),
                "PKIX_List_GetLength failed");

        for (respIx = 0; respIx < numResponses; respIx++) {
                PKIX_CHECK(PKIX_List_GetItem
                        (responseList,
                        respIx,
                        (PKIX_PL_Object **)&response,
                        plContext),
                        "PKIX_List_GetItem failed");

                PKIX_CHECK(pkix_pl_LdapResponse_GetMessage
                        (response, &message, plContext),
                        "pkix_pl_LdapResponse_GetMessage failed");

                sre = &(message->protocolOp.op.searchResponseEntryMsg);
                sreAttrArray = sre->attributes;

                /* Get next element of null-terminated array */
                sreAttr = *sreAttrArray++;
                while (sreAttr != NULL) {
                    attrType = &(sreAttr->attrType);
                    PKIX_CHECK(pkix_pl_LdapRequest_AttrTypeToBit
                        (attrType, &attrBits, plContext),
                        "pkix_pl_LdapRequest_AttrTypeToBit failed");
                    /* Is this attrVal a Revocation List? */
                    if (((LDAPATTR_CERTREVLIST | LDAPATTR_AUTHREVLIST) &
                            attrBits) == attrBits) {
                        attrVal = sreAttr->val;
                        derCrlItem = *attrVal++;
                        while (derCrlItem != 0) {
                            /* create a PKIX_PL_Crl from derCrl */
                            PKIX_CHECK(pkix_pl_LdapCertStore_DecodeCrl
                                (derCrlItem, crlList, plContext),
                                "pkix_pl_LdapCertStore_DecodeCrl failed");
                            derCrlItem = *attrVal++;
                        }
                    }
                    sreAttr = *sreAttrArray++;
                }
                PKIX_DECREF(response);
        }

        *pCrls = crlList;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(crlList);
        }

        PKIX_DECREF(response);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_DestroyAVAList
 * DESCRIPTION:
 *
 *  This function frees the space allocated for the components of the
 *  equalFilters that make up the andFilter pointed to by "filter".
 *
 * PARAMETERS:
 *  "requestParams"
 *      The address of the andFilter whose components are to be freed. Must be
 *      non-NULL.
 *  "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_DestroyAVAList(
        LDAPNameComponent **nameComponents,
        void *plContext)
{
        LDAPNameComponent **currentNC = NULL;
        unsigned char *component = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_DestroyAVAList");
        PKIX_NULLCHECK_ONE(nameComponents);

        /* Set currentNC to point to first AVA pointer */
        currentNC = nameComponents;

        while ((*currentNC) != NULL) {
                component = (*currentNC)->attrValue;
                if (component != NULL) {
                        PORT_Free(component);
                }
                currentNC++;
        }

cleanup:

        PKIX_RETURN(CERTSTORE);

}

/*
 * FUNCTION: pkix_pl_LdapCertStore_MakeNameAVAList
 * DESCRIPTION:
 *
 *  This function allocates space from the arena pointed to by "arena" to
 *  construct a filter that will match components of the X500Name pointed to
 *  by "name", and stores the resulting filter at "pFilter".
 *
 *  "name" is checked for commonName and organizationName components (cn=,
 *  and o=). The component strings are extracted using the family of
 *  CERT_Get* functions, and each must be freed with PORT_Free.
 *
 *  It is not clear which components should be in a request, so, for now,
 *  we stop adding components after we have found one.
 *
 * PARAMETERS:
 *  "arena"
 *      The address of the PRArenaPool used in creating the filter. Must be
 *       non-NULL.
 *  "name"
 *      The address of the X500Name whose components define the desired
 *      matches. Must be non-NULL.
 *  "pList"
 *      The address at which the result is stored.
 *  "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_MakeNameAVAList(
        PRArenaPool *arena,
        PKIX_PL_X500Name *subjectName, 
        LDAPNameComponent ***pList,
        void *plContext)
{
        LDAPNameComponent **setOfNameComponents;
        LDAPNameComponent *currentNameComponent = NULL;
        PKIX_UInt32 componentsPresent = 0;
        void *v = NULL;
        unsigned char *component = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_MakeNameAVAList");
        PKIX_NULLCHECK_THREE(arena, subjectName, pList);

        /* Increase this if additional components may be extracted */
#define MAX_NUM_COMPONENTS 3

        /* Space for (MAX_NUM_COMPONENTS + 1) pointers to LDAPNameComponents */
        PKIX_PL_NSSCALLRV(CERTSTORE, v, PORT_ArenaZAlloc,
                (arena, (MAX_NUM_COMPONENTS + 1)*sizeof(LDAPNameComponent *)));
        setOfNameComponents = (LDAPNameComponent **)v;

        /* Space for MAX_NUM_COMPONENTS LDAPNameComponents */
        PKIX_PL_NSSCALLRV(CERTSTORE, v, PORT_ArenaZNewArray,
                (arena, LDAPNameComponent, MAX_NUM_COMPONENTS));

        currentNameComponent = (LDAPNameComponent *)v;

        /* Try for commonName */
        PKIX_CHECK(pkix_pl_X500Name_GetCommonName
                (subjectName, &component, plContext),
                "pkix_pl_X500Name_GetCommonName failed");
        if (component) {
                setOfNameComponents[componentsPresent] = currentNameComponent;
                currentNameComponent->attrType = (unsigned char *)"cn";
                currentNameComponent->attrValue = component;
                componentsPresent++;
                currentNameComponent++;
        }

#if 0
        /* Try for orgName */
        PKIX_CHECK(pkix_pl_X500Name_GetOrgName
                (subjectName, &component, plContext),
                "pkix_pl_X500Name_GetOrgName failed");
        if (component) {
                setOfNameComponents[componentsPresent] = currentNameComponent;
                currentNameComponent->attrType = (unsigned char *)"o";
                currentNameComponent->attrValue = component;
                componentsPresent++;
                currentNameComponent++;
        }

        /* Try for countryName */
        PKIX_CHECK(pkix_pl_X500Name_GetCountryName
                (subjectName, &component, plContext),
                "pkix_pl_X500Name_GetCountryName failed");
        if (component) {
                setOfNameComponents[componentsPresent] = currentNameComponent;
                currentNameComponent->attrType = (unsigned char *)"c";
                currentNameComponent->attrValue = component;
                componentsPresent++;
                currentNameComponent++;
        }
#endif

        setOfNameComponents[componentsPresent] = NULL;

        *pList = setOfNameComponents;

cleanup:

        PKIX_RETURN(CERTSTORE);

}

/*
 * FUNCTION: pkix_pl_LdapCertStore_GetCert
 *  (see description of PKIX_CertStore_CertCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_LdapCertStore_GetCert(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCertList,
        void *plContext)
{
        PRArenaPool *requestArena = NULL;
        LDAPRequestParams requestParams;
        PRPollDesc *pollDesc = NULL;
        PKIX_UInt32 i = 0;
        PKIX_UInt32 numFound = 0;
        PKIX_Int32 minPathLen = 0;
        PKIX_Boolean match = PKIX_FALSE;
        PKIX_Boolean cacheFlag = PKIX_FALSE;
        PKIX_PL_Cert *candidate = NULL;
        PKIX_ComCertSelParams *params = NULL;
        PKIX_CertSelector_MatchCallback callback = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_List *responses = NULL;
        PKIX_List *certList = NULL;
        PKIX_List *filtered = NULL;
        PKIX_PL_X500Name *subjectName = 0;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCert");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        requestParams.baseObject = "c=US";
        requestParams.scope = WHOLE_SUBTREE;
        requestParams.derefAliases = NEVER_DEREF;
        requestParams.sizeLimit = 0;
        requestParams.timeLimit = 0;

        /* Prepare elements for request filter */

        /*
         * Get a short-lived arena. We'll be done with this space once
         * the request is encoded.
         */
        PKIX_PL_NSSCALLRV
            (CERTSTORE, requestArena, PORT_NewArena, (DER_DEFAULT_CHUNKSIZE));

        if (!requestArena) {
                PKIX_ERROR_FATAL("Out of memory");
        }

        PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                (selector, &params, plContext),
                "PKIX_CertSelector_GetComCertSelParams failed");

        /*
         * If we have the subject name for the desired subject,
         * ask the server for Certs with that subject.
         */
        PKIX_CHECK(PKIX_ComCertSelParams_GetSubject
                (params, &subjectName, plContext),
                "PKIX_ComCertSelParams_GetSubject failed");

        PKIX_CHECK(PKIX_ComCertSelParams_GetBasicConstraints
                (params, &minPathLen, plContext),
                "PKIX_ComCertSelParams_GetBasicConstraints failed");

        if (subjectName) {
                PKIX_CHECK(pkix_pl_LdapCertStore_MakeNameAVAList
                        (requestArena,
                        subjectName,
                        &(requestParams.nc),
                        plContext),
                        "pkix_pl_LdapCertStore_MakeNameAVAList failed");
        } else {
                PKIX_ERROR("Insufficient criteria for Cert query");
        }

        /* Prepare attribute field of request */

        requestParams.attributes = 0;

        if (minPathLen < 0) {
                requestParams.attributes |= LDAPATTR_USERCERT;
        }

        if (minPathLen > -2) {
                requestParams.attributes |=
                        LDAPATTR_CACERT | LDAPATTR_CROSSPAIRCERT;
        }

        /* All request fields are done */

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        PKIX_CHECK(PKIX_PL_LdapClient_InitiateRequest
                ((PKIX_PL_LdapClient *)lcs,
                &requestParams,
                &pollDesc,
                &responses,
                plContext),
                "PKIX_PL_LdapClient_InitiateRequest failed");

        PKIX_CHECK(pkix_pl_LdapCertStore_DestroyAVAList
                (requestParams.nc, plContext),
                "pkix_pl_LdapCertStore_DestroyAVAList failed");

        if (requestArena) {
                PKIX_PL_NSSCALL(CERTSTORE, PORT_FreeArena,
                        (requestArena, PR_FALSE));
        }

        if (pollDesc != NULL) {
                /* client is waiting for non-blocking I/O to complete */
                *pNBIOContext = (void *)pollDesc;
                *pCertList = NULL;
                goto cleanup;
        }
        /* LdapClient has given us a response! */

        if (responses) {
                PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
                        (selector, &callback, plContext),
                        "PKIX_CertSelector_GetMatchCallback failed");

                PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                        "PKIX_List_Create failed");

                /*
                 * We have a List of LdapResponse objects that still have to be
                 * turned into Certs.
                 */
                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCertList
                        (responses, &certList, plContext),
                        "pkix_pl_LdapCertStore_BuildCertList failed");

                PKIX_CHECK(PKIX_List_GetLength(certList, &numFound, plContext),
                        "PKIX_List_GetLength failed");

                PKIX_CHECK(PKIX_CertStore_GetCertStoreCacheFlag
                        (store, &cacheFlag, plContext),
                        "PKIX_CertStore_GetCertStoreCacheFlag failed");

                for (i = 0; i < numFound; i++) {
                        PKIX_CHECK(PKIX_List_GetItem
                                (certList,
                                i,
                                (PKIX_PL_Object **)&candidate,
                                plContext),
                                "PKIX_List_GetItem failed");

                        PKIX_CHECK_ONLY_FATAL(callback
                                (selector, candidate, &match, plContext),
                                "PKIX_CertSelector_MatchCallback failed");

                        if ((!(PKIX_ERROR_RECEIVED)) && (match == PKIX_TRUE)) {

                                PKIX_CHECK(PKIX_PL_Cert_SetCacheFlag
                                        (candidate, cacheFlag, plContext),
                                        "PKIX_PL_Cert_SetCacheFlag failed");

                                PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                        (filtered,
                                        (PKIX_PL_Object *)candidate,
                                        plContext),
                                        "PKIX_List_AppendItem failed");
                        }
                        PKIX_DECREF(candidate);
                }

                PKIX_CHECK(PKIX_List_SetImmutable(filtered, plContext),
                        "PKIX_List_SetImmutable failed");

        }

        /* Don't throw away the list if one Cert was bad! */
        pkixTempErrorReceived = PKIX_FALSE;

        *pNBIOContext = NULL;
        *pCertList = filtered;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filtered);
        }
        PKIX_DECREF(params);
        PKIX_DECREF(subjectName);
        PKIX_DECREF(candidate);
        PKIX_DECREF(certList);
        PKIX_DECREF(responses);
        PKIX_DECREF(lcs);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_GetCertContinue
 *  (see description of PKIX_CertStore_CertCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_LdapCertStore_GetCertContinue(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCertList,
        void *plContext)
{
        PKIX_UInt32 i = 0;
        PKIX_UInt32 numFound = 0;
        PKIX_Boolean cacheFlag = PKIX_FALSE;
        PKIX_Boolean match = PKIX_FALSE;
        PKIX_PL_Cert *candidate = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PRPollDesc *pollDesc = NULL;
        PKIX_List *responses = NULL;
        PKIX_CertSelector_MatchCallback callback = NULL;
        PKIX_List *certList = NULL;
        PKIX_List *filtered = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCertContinue");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        PKIX_CHECK(PKIX_PL_LdapClient_ResumeRequest
                ((PKIX_PL_LdapClient *)lcs, &pollDesc, &responses, plContext),
                "PKIX_PL_LdapClient_ResumeRequest failed");

        if (pollDesc != NULL) {
                /* client is waiting for non-blocking I/O to complete */
                *pNBIOContext = (void *)pollDesc;
                *pCertList = NULL;
                goto cleanup;
        }
        /* LdapClient has given us a response! */

        if (responses) {
                PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
                        (selector, &callback, plContext),
                        "PKIX_CertSelector_GetMatchCallback failed");

                PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                        "PKIX_List_Create failed");

                /*
                 * We have a List of LdapResponse objects that still have to be
                 * turned into Certs.
                 */
                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCertList
                        (responses, &certList, plContext),
                        "pkix_pl_LdapCertStore_BuildCertList failed");

                PKIX_CHECK(PKIX_List_GetLength(certList, &numFound, plContext),
                        "PKIX_List_GetLength failed");

                PKIX_CHECK(PKIX_CertStore_GetCertStoreCacheFlag
                        (store, &cacheFlag, plContext),
                        "PKIX_CertStore_GetCertStoreCacheFlag failed");

                for (i = 0; i < numFound; i++) {
                        PKIX_CHECK(PKIX_List_GetItem
                                (certList,
                                i,
                                (PKIX_PL_Object **)&candidate,
                                plContext),
                                "PKIX_List_GetItem failed");

                        PKIX_CHECK_ONLY_FATAL(callback
                                (selector, candidate, &match, plContext),
                                "PKIX_CertSelector_MatchCallback failed");

                        if ((!(PKIX_ERROR_RECEIVED)) && (match == PKIX_TRUE)) {

                                PKIX_CHECK(PKIX_PL_Cert_SetCacheFlag
                                        (candidate, cacheFlag, plContext),
                                        "PKIX_PL_Cert_SetCacheFlag failed");

                                PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                        (filtered,
                                        (PKIX_PL_Object *)candidate,
                                        plContext),
                                        "PKIX_List_AppendItem failed");
                        }
                        PKIX_DECREF(candidate);
                }

                PKIX_CHECK(PKIX_List_SetImmutable(filtered, plContext),
                        "PKIX_List_SetImmutable failed");

        }

        /* Don't throw away the list if one Cert was bad! */
        pkixTempErrorReceived = PKIX_FALSE;

        *pNBIOContext = NULL;
        *pCertList = filtered;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filtered);
        }
        PKIX_DECREF(candidate);
        PKIX_DECREF(certList);
        PKIX_DECREF(responses);
        PKIX_DECREF(lcs);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_GetCRL
 *  (see description of PKIX_CertStore_CRLCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_LdapCertStore_GetCRL(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrlList,
        void *plContext)
{
        LDAPRequestParams requestParams;
        PRPollDesc *pollDesc = NULL;
        PRArenaPool *requestArena = NULL;
        PKIX_UInt32 i = 0;
        PKIX_UInt32 numNames = 0;
        PKIX_UInt32 thisName = 0;
        PKIX_UInt32 numFound = 0;
        PKIX_PL_CRL *candidate = NULL;
        PKIX_List *responses = NULL;
        PKIX_List *crlList = NULL;
        PKIX_List *filtered = NULL;
        PKIX_List *issuerNames = NULL;
        PKIX_PL_X500Name *issuer = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_ComCRLSelParams *params = NULL;
        PKIX_CRLSelector_MatchCallback callback = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCRL");
        PKIX_NULLCHECK_THREE(store, selector, pCrlList);

        requestParams.baseObject = "c=US";
        requestParams.scope = WHOLE_SUBTREE;
        requestParams.derefAliases = NEVER_DEREF;
        requestParams.sizeLimit = 0;
        requestParams.timeLimit = 0;
        requestParams.attributes = LDAPATTR_CERTREVLIST | LDAPATTR_AUTHREVLIST;
        /* Prepare elements for request filter */

        /*
         * Get a short-lived arena. We'll be done with this space once
         * the request is encoded.
         */
        PKIX_PL_NSSCALLRV
            (CERTSTORE, requestArena, PORT_NewArena, (DER_DEFAULT_CHUNKSIZE));

        if (!requestArena) {
                PKIX_ERROR_FATAL("Out of memory");
        }

        PKIX_CHECK(PKIX_CRLSelector_GetCommonCRLSelectorParams
                (selector, &params, plContext),
                "PKIX_CRLSelector_GetComCertSelParams failed");

        PKIX_CHECK(PKIX_ComCRLSelParams_GetIssuerNames
                (params, &issuerNames, plContext),
                "PKIX_ComCRLSelParams_GetIssuerNames failed");

        /*
         * The specification for PKIX_ComCRLSelParams_GetIssuerNames in
         * pkix_crlsel.h says that if the criterion is not set we get a null
         * pointer. If we get an empty List the criterion is impossible to
         * meet ("must match at least one of the names in the List").
         */
        if (issuerNames) {

                PKIX_CHECK(PKIX_List_GetLength
                        (issuerNames, &numNames, plContext),
                        "PKIX_List_GetLength failed");

                if (numNames > 0) {
                        for (thisName = 0; thisName < numNames; thisName++) {
                                PKIX_CHECK(PKIX_List_GetItem
                                (issuerNames,
                                thisName,
                                (PKIX_PL_Object **)&issuer,
                                plContext),
                                "PKIX_List_GetItem failed");

                                PKIX_CHECK
                                        (pkix_pl_LdapCertStore_MakeNameAVAList
                                        (requestArena,
                                        issuer,
                                        &(requestParams.nc),
                                        plContext),
                                        "pkix_pl_LdapCertStore_MakeNameAVAList failed");

                                PKIX_DECREF(issuer);
                                /* XXX haven't figured out yet how to handle more than one */
                                break;
                        }
                } else {
                        PKIX_ERROR("Impossible criterion for Crl Query");
                }
        } else {
                PKIX_ERROR("Impossible criterion for Crl Query");
        }

        /* All request fields are done */

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        PKIX_CHECK(PKIX_PL_LdapClient_InitiateRequest
                ((PKIX_PL_LdapClient *)lcs,
                &requestParams,
                &pollDesc,
                &responses,
                plContext),
                "PKIX_PL_LdapClient_InitiateRequest failed");

        PKIX_CHECK(pkix_pl_LdapCertStore_DestroyAVAList
                (requestParams.nc, plContext),
                "pkix_pl_LdapCertStore_DestroyAVAList failed");

        if (requestArena) {
                PKIX_PL_NSSCALL(CERTSTORE, PORT_FreeArena,
                        (requestArena, PR_FALSE));
        }

        if (pollDesc != NULL) {
                /* client is waiting for non-blocking I/O to complete */
                *pNBIOContext = (void *)pollDesc;
                *pCrlList = NULL;
                goto cleanup;
        }
        /* client has finished! */

        if (responses) {
                PKIX_CHECK(PKIX_CRLSelector_GetMatchCallback
                        (selector, &callback, plContext),
                        "PKIX_CRLSelector_GetMatchCallback failed");

                PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                        "PKIX_List_Create failed");

                /*
                 * We have a List of LdapResponse objects that still have to be
                 * turned into Crls.
                 */
                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCrlList
                        (responses, &crlList, plContext),
                        "pkix_pl_LdapCertStore_BuildCrlList failed");

                PKIX_CHECK(PKIX_List_GetLength(crlList, &numFound, plContext),
                        "PKIX_List_GetLength failed");

                for (i = 0; i < numFound; i++) {
                        PKIX_CHECK(PKIX_List_GetItem
                                (crlList,
                                i,
                                (PKIX_PL_Object **)&candidate,
                                plContext),
                                "PKIX_List_GetItem failed");

                        PKIX_CHECK_ONLY_FATAL(callback(selector, candidate, plContext),
                                "PKIX_CRLSelector_MatchCallback failed");
                        if (!PKIX_ERROR_RECEIVED) {
                                PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                        (filtered,
                                        (PKIX_PL_Object *)candidate,
                                        plContext),
                                        "PKIX_List_AppendItem failed");
                        }
                        PKIX_DECREF(candidate);
                }

                PKIX_CHECK(PKIX_List_SetImmutable(filtered, plContext),
                        "PKIX_List_SetImmutable failed");

        }

        /* Don't throw away the list if one CRL was bad! */
        pkixTempErrorReceived = PKIX_FALSE;

        *pNBIOContext = NULL;
        *pCrlList = filtered;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filtered);
        }

        PKIX_DECREF(params);
        PKIX_DECREF(issuerNames);
        PKIX_DECREF(issuer);
        PKIX_DECREF(candidate);
        PKIX_DECREF(responses);
        PKIX_DECREF(crlList);
        PKIX_DECREF(lcs);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_GetCRLContinue
 *  (see description of PKIX_CertStore_CRLCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_LdapCertStore_GetCRLContinue(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrlList,
        void *plContext)
{
        PRPollDesc *pollDesc = NULL;
        PKIX_UInt32 i = 0;
        PKIX_UInt32 numFound = 0;
        PKIX_PL_CRL *candidate = NULL;
        PKIX_List *responses = NULL;
        PKIX_List *crlList = NULL;
        PKIX_List *filtered = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_CRLSelector_MatchCallback callback = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCRLContinue");
        PKIX_NULLCHECK_FOUR(store, selector, pNBIOContext, pCrlList);

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        PKIX_CHECK(PKIX_PL_LdapClient_ResumeRequest
                ((PKIX_PL_LdapClient *)lcs, &pollDesc, &responses, plContext),
                "PKIX_PL_LdapClient_ResumeRequest failed");

        if (pollDesc != NULL) {
                /* client is waiting for non-blocking I/O to complete */
                *pNBIOContext = (void *)pollDesc;
                *pCrlList = NULL;
                goto cleanup;
        }
        /* client has finished! */

        if (responses) {
                PKIX_CHECK(PKIX_CRLSelector_GetMatchCallback
                        (selector, &callback, plContext),
                        "PKIX_CRLSelector_GetMatchCallback failed");

                PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                        "PKIX_List_Create failed");

                /*
                 * We have a List of LdapResponse objects that still have to be
                 * turned into Crls.
                 */
                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCrlList
                        (responses, &crlList, plContext),
                        "pkix_pl_LdapCertStore_BuildCrlList failed");

                PKIX_CHECK(PKIX_List_GetLength(crlList, &numFound, plContext),
                        "PKIX_List_GetLength failed");

                for (i = 0; i < numFound; i++) {
                        PKIX_CHECK(PKIX_List_GetItem
                                (crlList,
                                i,
                                (PKIX_PL_Object **)&candidate,
                                plContext),
                                "PKIX_List_GetItem failed");

                        PKIX_CHECK_ONLY_FATAL(callback(selector, candidate, plContext),
                                "PKIX_CRLSelector_MatchCallback failed");
                        if (!PKIX_ERROR_RECEIVED) {
                                PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                        (filtered,
                                        (PKIX_PL_Object *)candidate,
                                        plContext),
                                        "PKIX_List_AppendItem failed");
                        }
                        PKIX_DECREF(candidate);
                }

                PKIX_CHECK(PKIX_List_SetImmutable(filtered, plContext),
                        "PKIX_List_SetImmutable failed");

        }

        /* Don't throw away the list if one CRL was bad! */
        pkixTempErrorReceived = PKIX_FALSE;

        *pCrlList = filtered;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filtered);
        }

        PKIX_DECREF(candidate);
        PKIX_DECREF(responses);
        PKIX_DECREF(crlList);
        PKIX_DECREF(lcs);

        PKIX_RETURN(CERTSTORE);
}

/* --Public-LdapCertStore-Functions----------------------------------- */

/*
 * FUNCTION: PKIX_PL_LdapCertStore_Create
 * (see comments in pkix_samples_modules.h)
 */
PKIX_Error *
PKIX_PL_LdapCertStore_Create(
        PKIX_PL_LdapClient *client,
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        PKIX_CertStore *certStore = NULL;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_LdapCertStore_Create");
        PKIX_NULLCHECK_TWO(client, pCertStore);

        PKIX_CHECK(PKIX_CertStore_Create
                (pkix_pl_LdapCertStore_GetCert,
                pkix_pl_LdapCertStore_GetCRL,
                pkix_pl_LdapCertStore_GetCertContinue,
                pkix_pl_LdapCertStore_GetCRLContinue,
                NULL,       /* don't support trust */
                (PKIX_PL_Object *)client,
                PKIX_TRUE,  /* cache flag */
                PKIX_FALSE, /* not local */
                &certStore,
                plContext),
                "PKIX_CertStore_Create failed");

        *pCertStore = certStore;

cleanup:

        PKIX_RETURN(CERTSTORE);
}

