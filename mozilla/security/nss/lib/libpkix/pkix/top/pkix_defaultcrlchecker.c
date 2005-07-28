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
 * pkix_defaultcrlchecker.c
 *
 * Functions for default CRL Checkers
 *
 */

#include "pkix_defaultcrlchecker.h"

static char *reasonCodeMsgString[] = {
        "Certificate is revoked by CRL for unspecified reason",
        "Certificate is revoked by CRL for key compromise",
        "Certificate is revoked by CRL for CA compromise",
        "Certificate is revoked by CRL for affiliation changed",
        "Certificate is revoked by CRL for being superseded",
        "Certificate is revoked by CRL for cessation of operation",
        "Certificate is revoked by CRL for certificate hold",
        "Certificate is revoked by CRL for undefined reason",
        "Certificate is revoked by CRL for being removed from CRL",
        "Certificate is revoked by CRL for privilege withdrawn",
        "Certificate is revoked by CRL for aACompromise"
};

/* --Private-DefaultCRLCheckerState-Functions------------------------------- */

/*
 * FUNCTION: pkix_DefaultCRLCheckerstate_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_DefaultCRLCheckerState_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        pkix_DefaultCRLCheckerState *state = NULL;

        PKIX_ENTER(DEFAULTCRLCHECKERSTATE,
                    "pkix_DefaultCRLCheckerState_Destroy");
        PKIX_NULLCHECK_ONE(object);

        /* Check that this object is a default CRL checker state */
        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_DEFAULTCRLCHECKERSTATE_TYPE, plContext),
                    "Object is not a default CRL checker state");

        state = (pkix_DefaultCRLCheckerState *)object;

        state->certHasValidCrl = PKIX_FALSE;
        state->prevCertCrlSign = PKIX_FALSE;
        state->reasonCodeMask = 0;

        PKIX_DECREF(state->certStores);
        PKIX_DECREF(state->testDate);
        PKIX_DECREF(state->prevPublicKey);
        PKIX_DECREF(state->prevPublicKeyList);
        PKIX_DECREF(state->crlReasonCodeOID);

cleanup:

        PKIX_RETURN(DEFAULTCRLCHECKERSTATE);
}

/*
 * FUNCTION: pkix_DefaultCRLCheckerState_RegisterSelf
 *
 * DESCRIPTION:
 *  Registers PKIX_DEFAULTCRLCHECKERSTATE_TYPE and its related functions
 *  with systemClasses[]
 *
 * THREAD SAFETY:
 *  Not Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_DefaultCRLCheckerState_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(DEFAULTCRLCHECKERSTATE,
                    "pkix_DefaultCRLCheckerState_RegisterSelf");

        entry.description = "DefaultCRLCheckerState";
        entry.destructor = pkix_DefaultCRLCheckerState_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_DEFAULTCRLCHECKERSTATE_TYPE] = entry;

cleanup:
        PKIX_RETURN(DEFAULTCRLCHECKERSTATE);
}

/*
 * FUNCTION: pkix_DefaultCRLCheckerState_Create
 *
 * DESCRIPTION:
 *  Allocate and initialize DefaultCRLChecker state data.
 *
 * PARAMETERS
 *  "certStores"
 *      Address of CertStore List to be stored in state. Must be non-NULL.
 *  "testDate"
 *      Address of PKIX_PL_Date to be checked. May be NULL.
 *  "trustedPubKey"
 *      Trusted Anchor Public Key for verifying first Cert in the chain.
 *      Must be non-NULL.
 *  "certsRemaining"
 *      Number of certificates remaining in the chain.
 *  "pCheckerState"
 *      Address of DefaultCRLCheckerState that is returned. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a DefaultCrlCheckerState Error if the function fails in a
 *  non-fatal way.
 *  Returns a Fatal Error
 */
static PKIX_Error *
pkix_DefaultCRLCheckerState_Create(
    PKIX_List *certStores,
    PKIX_PL_Date *testDate,
    PKIX_PL_PublicKey *trustedPubKey,
    PKIX_UInt32 certsRemaining,
    pkix_DefaultCRLCheckerState **pCheckerState,
    void *plContext)
{
        pkix_DefaultCRLCheckerState *state = NULL;

        PKIX_ENTER(DEFAULTCRLCHECKERSTATE,
                    "pkix_DefaultCRLCheckerState_Create");
        PKIX_NULLCHECK_TWO(certStores, pCheckerState);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_DEFAULTCRLCHECKERSTATE_TYPE,
                    sizeof (pkix_DefaultCRLCheckerState),
                    (PKIX_PL_Object **)&state,
                    plContext),
                    "Could not create DefaultCRLCheckerState object");

        /* Initialize fields */

        PKIX_INCREF(certStores);
        state->certStores = certStores;

        PKIX_INCREF(testDate);
        state->testDate = testDate;

        PKIX_INCREF(trustedPubKey);
        state->prevPublicKey = trustedPubKey;

        state->certHasValidCrl = PKIX_FALSE;
        state->prevCertCrlSign = PKIX_TRUE;
        state->prevPublicKeyList = NULL;
        state->reasonCodeMask = 0;
        state->certsRemaining = certsRemaining;

        PKIX_CHECK(PKIX_PL_OID_Create
                    (PKIX_CRLREASONCODE_OID,
                    &state->crlReasonCodeOID,
                    plContext),
                    "PKIX_PL_OID_Create failed");

        *pCheckerState = state;

cleanup:

        PKIX_RETURN(DEFAULTCRLCHECKERSTATE);
}

/* --Private-DefaultCRLChecker-Functions------------------------------------ */

/*
 * FUNCTION: pkix_DefaultCRLChecker_CheckCRLs
 *
 * DESCRIPTION:
 *  Check validity of "cert" based on CRLs at "crlList" that has correct
 *  signature verification with "publicKey".
 *
 * PARAMETERS
 *  "cert"
 *      Address of Cert which has the certificate data. Must be non-NULL.
 *  "publicKey"
 *      Address of Public Key that associates with the Cert Issuer.
 *      Must be non-NULL.
 *  "crlList"
 *      A List CRLs that the certificate is verified upon. Must be non-NULL.
 *  "state"
 *      Address of DefaultCRLCheckerState which keeps dynamic state data.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Conditionally Thread Safe
 *      (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertChainChecker Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error
 */
static PKIX_Error *
pkix_DefaultCRLChecker_CheckCRLs(
        PKIX_PL_Cert *cert,
        PKIX_PL_PublicKey *publicKey,
        PKIX_List *crlList,
        pkix_DefaultCRLCheckerState *state,
        void *plContext)
{
        PKIX_PL_CRL *crl = NULL;
        PKIX_PL_CRLEntry *crlEntry = NULL;
        PKIX_PL_X500Name *certIssuer = NULL;
        PKIX_PL_BigInt *certSerialNumber = NULL;
        PKIX_PL_PublicKey *pKey = NULL;
        PKIX_List *unresCrlCritExtOIDs = NULL;
        PKIX_List *unresCrlEntryCritExtOIDs = NULL;
        PKIX_Error *verifyFail = NULL;
        PKIX_UInt32 numCrls = 0;
        PKIX_UInt32 numKeys = 0;
        PKIX_UInt32 numCritExtOIDs = 0;
        PKIX_Boolean cmpResult = PKIX_FALSE;
        PKIX_Boolean crlVerified = PKIX_FALSE;
        PKIX_Int32 reasonCode = 0;
        PKIX_UInt32 i;
        PKIX_Int32 j;

        PKIX_ENTER(CERTCHAINCHECKER,
                    "pkix_DefaultCRLChecker_CheckCRLs");
        PKIX_NULLCHECK_FOUR(cert, publicKey, crlList, state);

        PKIX_CHECK(PKIX_PL_Cert_GetIssuer(cert, &certIssuer, plContext),
                    "PKIX_PL_Cert_GetIssuer failed");

        PKIX_CHECK(PKIX_PL_Cert_GetSerialNumber
                    (cert, &certSerialNumber, plContext),
                    "PKIX_PL_Cert_GetSerialNumber failed");

        PKIX_CHECK(PKIX_List_GetLength(crlList, &numCrls, plContext),
                    "PKIX_List_GetLength failed");

        if (state->prevPublicKeyList != NULL) {

                PKIX_CHECK(PKIX_List_GetLength
                    (state->prevPublicKeyList, &numKeys, plContext),
                    "PKIX_List_GetLength failed");
        }

        /* Check if Cert is not revoked by any the the CRLs */

        for (i = 0; i < numCrls; i++){

                PKIX_CHECK(PKIX_List_GetItem
                            (crlList, i, (PKIX_PL_Object **)&crl, plContext),
                            "PKIX_List_GetItem failed");

                /*
                 * Checking serial number (issuer done in selector) then
                 * verify signature. If matches, get the CRL reason(s).
                 */

                if (state->prevCertCrlSign == PKIX_TRUE) {
                        verifyFail = PKIX_PL_CRL_VerifySignature
                                (crl, publicKey, plContext);
                        if (verifyFail == NULL) {
                                crlVerified = PKIX_TRUE;
                        } else {
                                crlVerified = PKIX_FALSE;
                                PKIX_DECREF(verifyFail);
                        }
                }

                if (crlVerified == PKIX_FALSE) {

                    /* Verify from old key(s) on the list */
                    for (j = numKeys - 1; j >= 0; j--) {

                            PKIX_CHECK(PKIX_List_GetItem
                                (state->prevPublicKeyList,
                                j,
                                (PKIX_PL_Object **) &pKey,
                                plContext),
                                "PKIX_List_GetItem failed");

                            verifyFail = PKIX_PL_CRL_VerifySignature
                                (crl, pKey, plContext);

                            if (verifyFail == NULL) {
                                crlVerified = PKIX_TRUE;
                                break;
                            } else {
                                crlVerified = PKIX_FALSE;
                                PKIX_DECREF(verifyFail);
                            }

                            PKIX_DECREF(pKey);
                    }
                }

                if (crlVerified == PKIX_FALSE) {
                    /* try next one ... */
                    goto cleanup_loop;
                }

                if (crlVerified == PKIX_FALSE) {
                    /* try next one ... */
                    goto cleanup_loop;
                }

                state->certHasValidCrl = PKIX_TRUE;

                PKIX_CHECK(PKIX_PL_CRL_GetCriticalExtensionOIDs
                            (crl, &unresCrlCritExtOIDs, plContext),
                            "PKIX_PL_CRL_GetCriticalExtensionOIDs failed");

                /*
                 * XXX Advanced CRL work - should put a
                 * Loop here to process and remove critical
                 * extension oids.
                 */

                if (unresCrlCritExtOIDs) {

                    PKIX_CHECK(PKIX_List_GetLength(unresCrlCritExtOIDs,
                        &numCritExtOIDs,
                        plContext),
                        "PKIX_List_GetLength failed");

                    if (numCritExtOIDs != 0) {
                        PKIX_DEFAULTCRLCHECKERSTATE_DEBUG
                                ("CRL Critical Extension OIDs not processed");
                        /*
                         * Uncomment this after we have implemented
                         * checkers for all the critical extensions.
                         *
                         * PKIX_ERROR
                         *      ("Unrecognized CRL Critical Extension");
                         */
                    }
                }

                PKIX_CHECK(PKIX_PL_CRL_GetCRLEntryForSerialNumber
                            (crl, certSerialNumber, &crlEntry, plContext),
                            "PKIX_PL_CRL_GetCRLEntryForSerialNumber failed");

                if (crlEntry == NULL) {
                    goto cleanup_loop;
                }

                PKIX_CHECK(PKIX_PL_CRLEntry_GetCRLEntryReasonCode
                            (crlEntry,
                            &reasonCode,
                            plContext),
                            "PKIX_PL_CRLEntry_GetCRLEntryReasonCode failed");

                /* Set reason code in state for advance CRL reviewing */

                if (reasonCode >= 0 &&
                    reasonCode < sizeof (reasonCodeMsgString)) {

                    state->reasonCodeMask |= 1 << reasonCode;
                    PKIX_DEFAULTCRLCHECKERSTATE_DEBUG_ARG
                        ("CRL revocation Reason: %s\n ",
                        reasonCodeMsgString[reasonCode]);

                } else {
                    PKIX_DEFAULTCRLCHECKERSTATE_DEBUG
                        ("Revoked by Unknown CRL ReasonCode");
                }

                PKIX_CHECK(PKIX_PL_CRLEntry_GetCriticalExtensionOIDs
                            (crlEntry, &unresCrlEntryCritExtOIDs, plContext),
                            "PKIX_PL_CRLEntry_GetCriticalExtensionOIDs failed");
                if (unresCrlEntryCritExtOIDs) {

                    PKIX_CHECK(pkix_List_Remove
                            (unresCrlEntryCritExtOIDs,
                            (PKIX_PL_Object *) state->crlReasonCodeOID,
                            plContext),
                            "PKIX_List_Remove failed");

                    PKIX_CHECK(PKIX_List_GetLength(unresCrlEntryCritExtOIDs,
                        &numCritExtOIDs,
                        plContext),
                        "PKIX_List_GetLength failed");

                    if (numCritExtOIDs != 0) {

                        PKIX_DEFAULTCRLCHECKERSTATE_DEBUG
                            ("CRLEntry Critical Extension was not processed");
                        PKIX_ERROR("Unrecognized CRLEntry Critical Extension");
                    }
                }

                PKIX_ERROR("Certificate is revoked by CRL");

                break;

        cleanup_loop:
                PKIX_DECREF(pKey);
                PKIX_DECREF(verifyFail);
                PKIX_DECREF(pKey);
                PKIX_DECREF(crlEntry);
                PKIX_DECREF(crl);
                PKIX_DECREF(unresCrlCritExtOIDs);
                PKIX_DECREF(unresCrlEntryCritExtOIDs);
        }

cleanup:

        PKIX_DECREF(pKey);
        PKIX_DECREF(verifyFail);
        PKIX_DECREF(certSerialNumber);
        PKIX_DECREF(certIssuer);
        PKIX_DECREF(crlEntry);
        PKIX_DECREF(crl);
        PKIX_DECREF(unresCrlCritExtOIDs);
        PKIX_DECREF(unresCrlEntryCritExtOIDs);

        PKIX_RETURN(CERTCHAINCHECKER);
}

/*
 * FUNCTION: pkix_DefaultCRLChecker_Check_Helper
 *
 * DESCRIPTION:
 *  Check if the Cert has been revoked based the CRLs data. It exhausts all
 *  CertStores and match CRLs at those stores for the certificate. It set the
 *  current date and issuer in CRLSelector to fetching CRL data from cache.
 *
 * PARAMETERS
 *  "checker"
 *      Address of CertChainChecker which has the state data.
 *      Must be non-NULL.
 *  "cert"
 *      Address of Certificate that is to be validated. Must be non-NULL.
 *  "prevPublicKey"
 *      Address of previous public key in the backward chain. May be NULL.
 *  "state"
 *      Address of DefaultCrlCheckerState. Must be non-NULL.
 *  "unresolvedCriticalExtensions"
 *      A List OIDs. Not **yet** used in this checker function.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Not Thread Safe
 *      (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertChainChecker Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error
 */
PKIX_Error *
pkix_DefaultCRLChecker_Check_Helper(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_PL_PublicKey *prevPublicKey,
        pkix_DefaultCRLCheckerState *state,
        PKIX_List *unresolvedCriticalExtensions,
        void *plContext)
{

        PKIX_CertStore_CRLCallback certStoreGetCrlCallback = NULL;
        PKIX_CertStore *certStore = NULL;
        PKIX_CRLSelector *crlSelector = NULL;
        PKIX_List *crlList = NULL;
        PKIX_PL_X500Name *certIssuer = NULL;
        PKIX_ComCRLSelParams *comCrlSelParams = NULL;
        PKIX_PL_Date *nowDate = NULL;

        PKIX_Error *checkCrlFail = NULL;
        PKIX_UInt32 type = 0;
        PKIX_UInt32 numCertStores = 0;
        PKIX_UInt32 i = 0;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_DefaultCRLChecker_Check_Helper");
        PKIX_NULLCHECK_THREE(checker, cert, state);

        /* Set up CRLSelector */
        PKIX_CHECK(PKIX_PL_Cert_GetIssuer(cert, &certIssuer, plContext),
                    "PKIX_PL_Cert_GetIssuer failed");

        if (state->testDate != NULL) {

                PKIX_INCREF(state->testDate);

                nowDate = state->testDate;

        } else {

                PKIX_CHECK(PKIX_PL_Date_Create_UTCTime
                            (NULL, &nowDate, plContext),
                            "PKIX_PL_Date_Create_UTCTime failed");
        }

        PKIX_CHECK(PKIX_ComCRLSelParams_Create
                            (&comCrlSelParams, plContext),
                            "PKIX_ComCRLSelParams_Create failed");

        PKIX_CHECK(PKIX_ComCRLSelParams_AddIssuerName
                            (comCrlSelParams, certIssuer, plContext),
                            "PKIX_ComCRLSelParams_AddIssuerName failed");

        PKIX_CHECK(PKIX_ComCRLSelParams_SetDateAndTime
                            (comCrlSelParams, nowDate, plContext),
                            "PKIX_ComCRLSelParams_SetDateAndTime failed");

        PKIX_CHECK(PKIX_CRLSelector_Create
                            (NULL,
                            (PKIX_PL_Object *)checker,
                            &crlSelector,
                            plContext),
                            "PKIX_CRLSelector_Create failed");

        PKIX_CHECK(PKIX_CRLSelector_SetCommonCRLSelectorParams
                        (crlSelector, comCrlSelParams, plContext),
                        "PKIX_CRLSelector_SetCommonCRLSelectorParams failed");

        /* Set up CertStores */

        state->certHasValidCrl = PKIX_FALSE;

        PKIX_NULLCHECK_ONE(state->certStores);

        PKIX_CHECK(PKIX_List_GetLength
                    (state->certStores, &numCertStores, plContext),
                    "PKIX_List_GetLength failed");

        for (i = 0; i < numCertStores; i++){

                /*
                 * For Basic CRL work, exit the loop when there is a valid
                 * CRL. For advance CRL, need to exhaust CRL until all
                 * reason mask are checked or a revoked is issued.
                 */

                if (state->certHasValidCrl == PKIX_TRUE) {
                        break;
                }

                PKIX_CHECK(PKIX_List_GetItem
                            (state->certStores,
                            i,
                            (PKIX_PL_Object **)&certStore,
                            plContext),
                            "PKIX_List_GetItem failed");

                /* From CertStore, get CRLCallback to retrieve selected CRLs */

                PKIX_CHECK(PKIX_CertStore_GetCRLCallback
                            (certStore, &certStoreGetCrlCallback, plContext),
                            "PKIX_CertStore_GetCRLCallback failed");

                PKIX_CHECK(certStoreGetCrlCallback
                            (certStore, crlSelector, &crlList, plContext),
                            "certStoreGetCrlCallback failed");

                /* Verify Certificate validity */

                if (crlList != NULL) {
                        checkCrlFail = pkix_DefaultCRLChecker_CheckCRLs
                                (cert,
                                prevPublicKey,
                                crlList,
                                state,
                                plContext);

                        if (checkCrlFail) {
                                PKIX_ERROR("Certificate is revoked by CRL");
                        }
                }

                PKIX_DECREF(crlList);
                PKIX_DECREF(certStore);
        }

        if (state->certHasValidCrl == PKIX_FALSE) {
                PKIX_ERROR("Certificate doesn't have a valid CRL");
        }

cleanup:

        PKIX_DECREF(nowDate);
        PKIX_DECREF(comCrlSelParams);
        PKIX_DECREF(crlSelector);
        PKIX_DECREF(certIssuer);
        PKIX_DECREF(certStore);
        PKIX_DECREF(crlList);
        PKIX_DECREF(checkCrlFail);

        PKIX_RETURN(CERTCHAINCHECKER);
}

/*
 * FUNCTION: pkix_DefaultCRLChecker_Check
 *
 * DESCRIPTION:
 *  Check if the Cert has been revoked based the CRLs data.  This function
 *  maintains the checker state to be current.
 *
 * PARAMETERS
 *  "checker"
 *      Address of CertChainChecker which has the state data.
 *      Must be non-NULL.
 *  "cert"
 *      Address of Certificate that is to be validated. Must be non-NULL.
 *  "unresolvedCriticalExtensions"
 *      A List OIDs. Not **yet** used in this checker function.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Not Thread Safe
 *      (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertChainChecker Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error
 */
static PKIX_Error *
pkix_DefaultCRLChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,
        void *plContext)
{
        pkix_DefaultCRLCheckerState *state = NULL;
        PKIX_PL_PublicKey *publicKey = NULL;
        PKIX_PL_PublicKey *newPublicKey = NULL;
        PKIX_Error *checkKeyUsageFail = NULL;
        PKIX_Boolean selfIssued = PKIX_FALSE;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_DefaultCRLChecker_Check");
        PKIX_NULLCHECK_TWO(checker, cert);

        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                    (checker, (PKIX_PL_Object **)&state, plContext),
                    "PKIX_CertChainChecker_GetCertChainCheckerState failed");

        PKIX_CHECK(PKIX_PL_Cert_GetSubjectPublicKey
                    (cert, &publicKey, plContext),
                    "PKIX_PL_Cert_GetSubjectPublicKey failed");

        state->certsRemaining--;

        PKIX_NULLCHECK_ONE(state->prevPublicKey);

        if (state->prevCertCrlSign == PKIX_FALSE) {
                PKIX_ERROR("Validation failed: KeyUsage cRLSign bit is not on");
        }

        PKIX_CHECK(pkix_DefaultCRLChecker_Check_Helper
                    (checker,
                    cert,
                    state->prevPublicKey,
                    state,
                    unresolvedCriticalExtensions,
                    plContext),
                    "pkix_DefaultCRLChecker_Check_Helper failed");

        /*
         * Some NIST test case in 4.5.* use different publicKeys for
         * Cert and its CRL on the chain. Self-issued Certs are used
         * to speciy multiple keys for those cases. That is why we apply
         * the following algorithm:
         *
         * Check if Cert is self-issued. If so, the public key of the Cert
         * that issues this Cert (old key) can be used together with this
         * current key (new key) for key verification. If there are multiple
         * self-issued certs, keys of those Certs (old keys) can also be used
         * for key verification. Old key(s) is saved in a list (PrevPublickKey-
         * List) and cleared when a Cert is no longer self-issued.
         * PrevPublicKey keep key of the previous Cert.
         * PrevPublicKeyList keep key(s) of Cert before the previous one.
         */
        PKIX_CHECK(pkix_IsCertSelfIssued(cert, &selfIssued, plContext),
                    "pkix_IsCertSelfIssue failed");

        if (selfIssued == PKIX_TRUE) {

                if (state->prevPublicKeyList == NULL) {

                        PKIX_CHECK(PKIX_List_Create
                            (&state->prevPublicKeyList, plContext),
                            "PKIX_List_Create falied");

                }

                PKIX_CHECK(PKIX_List_AppendItem
                            (state->prevPublicKeyList,
                            (PKIX_PL_Object *) state->prevPublicKey,
                            plContext),
                            "PKIX_List_AppendItem failed");

        } else {
                /* Not self-issued Cert any more, clear old key(s) saved */
                PKIX_DECREF(state->prevPublicKeyList);
        }

        /* Make inheritance and save current Public Key */
        PKIX_CHECK(PKIX_PL_PublicKey_MakeInheritedDSAPublicKey
                    (publicKey, state->prevPublicKey, &newPublicKey, plContext),
                    "PKIX_PL_PublicKey_MakeInheritedDSAPublicKey failed");

        if (newPublicKey == NULL){
                PKIX_INCREF(publicKey);
                newPublicKey = publicKey;
        }

        PKIX_DECREF(state->prevPublicKey);
        PKIX_INCREF(newPublicKey);
        state->prevPublicKey = newPublicKey;

        /* Save current Cert's cRLSign bit for CRL checking later */
        if (state->certsRemaining != 0) {
                checkKeyUsageFail = PKIX_PL_Cert_VerifyKeyUsage
                        (cert, PKIX_CRL_SIGN, plContext);

                state->prevCertCrlSign = (checkKeyUsageFail == NULL)?
                        PKIX_TRUE : PKIX_FALSE;

                PKIX_DECREF(checkKeyUsageFail);
        }

        PKIX_CHECK(PKIX_CertChainChecker_SetCertChainCheckerState
                (checker, (PKIX_PL_Object *)state, plContext),
                "PKIX_CertChainChecker_SetCertChainCheckerState failed");

cleanup:

        PKIX_DECREF(state);
        PKIX_DECREF(publicKey);
        PKIX_DECREF(newPublicKey);
        PKIX_DECREF(checkKeyUsageFail);

        PKIX_RETURN(CERTCHAINCHECKER);
}

/*
 * FUNCTION: pkix_DefaultCRLChecker_Initialize
 *
 * DESCRIPTION:
 *  Create a CertChainChecker with DefaultCRLCheckerState.
 *
 * PARAMETERS
 *  "certStores"
 *      Address of CertStore List to be stored in state. Must be non-NULL.
 *  "testDate"
 *      Address of PKIX_PL_Date to be checked. May be NULL.
 *  "trustedPubKey"
 *      Address of Public Key of Trust Anchor. Must be non-NULL.
 *  "certsRemaining"
 *      Number of certificates remaining in the chain.
 *  "pChecker"
 *      Address where object pointer will be stored. Must be non-NULL.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertChainChecker Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error
 */
PKIX_Error *
pkix_DefaultCRLChecker_Initialize(
        PKIX_List *certStores,
        PKIX_PL_Date *testDate,
        PKIX_PL_PublicKey *trustedPubKey,
        PKIX_UInt32 certsRemaining,
        PKIX_CertChainChecker **pChecker,
        void *plContext)
{
        pkix_DefaultCRLCheckerState *state = NULL;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_DefaultCRLChecker_Initialize");
        PKIX_NULLCHECK_TWO(certStores, pChecker);

        PKIX_CHECK(pkix_DefaultCRLCheckerState_Create
                    (certStores,
                    testDate,
                    trustedPubKey,
                    certsRemaining,
                    &state,
                    plContext),
                    "pkix_DefaultCRLCheckerState_Create failed");

        PKIX_CHECK(PKIX_CertChainChecker_Create
                    (pkix_DefaultCRLChecker_Check,
                    PKIX_FALSE,
                    PKIX_FALSE,
                    NULL,
                    (PKIX_PL_Object *) state,
                    pChecker,
                    plContext),
                    "PKIX_CertChainChecker_Create failed");

cleanup:

        PKIX_DECREF(state);

        PKIX_RETURN(CERTCHAINCHECKER);
}
