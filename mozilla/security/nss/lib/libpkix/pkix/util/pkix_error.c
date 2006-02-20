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
 * pkix_error.c
 *
 * Error Object Functions
 *
 */

#include "pkix_error.h"

/* --Private-Functions-------------------------------------------- */

/*
 * FUNCTION: pkix_Error_Equals
 * (see comments for PKIX_PL_EqualsCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_Error_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_Error *firstError = NULL;
        PKIX_Error *secondError = NULL;
        PKIX_Error *firstCause = NULL;
        PKIX_Error *secondCause = NULL;
        PKIX_PL_String *firstDesc = NULL;
        PKIX_PL_String *secondDesc = NULL;
        PKIX_PL_Object *firstInfo = NULL;
        PKIX_PL_Object *secondInfo = NULL;
        PKIX_UInt32 firstCode, secondCode, secondType;
        PKIX_Boolean boolResult, unequalFlag;

        PKIX_ENTER(ERROR, "pkix_Error_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        unequalFlag = PKIX_FALSE;

        /* First just compare pointer values to save time */
        if (firstObject == secondObject) {
                *pResult = PKIX_TRUE;
                goto cleanup;
        } else {
                /* Result will only be set to true if all tests pass */
                *pResult = PKIX_FALSE;
        }

        PKIX_CHECK(pkix_CheckType(firstObject, PKIX_ERROR_TYPE, plContext),
                    "First Object is not an Error object");

        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObject, &secondType, plContext),
                    "Error getting second object type");

        /* If types differ, then return false. Result is already set */
        if (secondType != PKIX_ERROR_TYPE) goto cleanup;

        /* It is safe to cast to PKIX_Error */
        firstError = (PKIX_Error *) firstObject;
        secondError = (PKIX_Error *) secondObject;

        /* Compare error codes */
        firstCode = firstError->code;
        secondCode = secondError->code;

        /* If codes differ, return false. Result is already set */
        if (firstCode != secondCode) goto cleanup;

        /* Compare causes */
        firstCause = firstError->cause;
        secondCause = secondError->cause;

        /* Ensure that either both or none of the causes are NULL */
        if (((firstCause != NULL) && (secondCause == NULL))||
            ((firstCause == NULL) && (secondCause != NULL)))
                unequalFlag = PKIX_TRUE;

        if ((firstCause != NULL) && (secondCause != NULL)) {
                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object*)firstCause,
                            (PKIX_PL_Object*)secondCause,
                            &boolResult,
                            plContext),
                            "Error in recursive equals call");

                /* Set the unequalFlag so that we return after dec refing */
                if (boolResult == 0) unequalFlag = PKIX_TRUE;
        }

        /* If the cause errors are not equal, return null */
        if (unequalFlag) goto cleanup;

        /* Compare info fields */
        firstInfo = firstError->info;
        secondInfo = secondError->info;

        if (firstInfo != secondInfo) goto cleanup;

        /* Ensure that either both or none of the infos are NULL */
        if (((firstInfo != NULL) && (secondInfo == NULL))||
            ((firstInfo == NULL) && (secondInfo != NULL)))
                unequalFlag = PKIX_TRUE;

        if ((firstInfo != NULL) && (secondInfo != NULL)) {

                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object*)firstInfo,
                            (PKIX_PL_Object*)secondInfo,
                            &boolResult,
                            plContext),
                            "Error in recursive equals call");

                /* Set the unequalFlag so that we return after dec refing */
                if (boolResult == 0) unequalFlag = PKIX_TRUE;
        }

        /* If the infos are not equal, return null */
        if (unequalFlag) goto cleanup;


        /* Compare descs */
        firstDesc = firstError->desc;
        secondDesc = secondError->desc;

        if (((firstDesc != NULL) && (secondDesc == NULL))||
            ((firstDesc == NULL) && (secondDesc != NULL))) {
                unequalFlag = PKIX_TRUE;
        } else if ((firstDesc != NULL) && (secondDesc != NULL)) {
                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object*) firstDesc,
                            (PKIX_PL_Object*) secondDesc,
                            &boolResult,
                            plContext),
                            "Error in recursive equals call");

                /* If the desc errors are not equal, return null */
                if (boolResult == 0) unequalFlag = PKIX_TRUE;
        }

        /* If the unequalFlag was set, return false */
        if (unequalFlag) goto cleanup;

        /* Errors are equal in all fields at this point */
        *pResult = PKIX_TRUE;

cleanup:

        PKIX_RETURN(ERROR);
}

/*
 * FUNCTION: pkix_Error_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_Error_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_Error *error = NULL;

        PKIX_ENTER(ERROR, "pkix_Error_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_ERROR_TYPE, plContext),
                    "Object is not an Error object");

        error = (PKIX_Error *)object;

        PKIX_DECREF(error->cause);

        PKIX_DECREF(error->desc);

        PKIX_DECREF(error->info);

cleanup:

        PKIX_RETURN(ERROR);
}


/* XXX This is not thread safe */
static PKIX_UInt32 pkix_error_cause_depth = 1;

/*
 * FUNCTION: pkix_Error_ToString
 * (see comments for PKIX_PL_ToStringCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_Error_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        extern const char *PKIX_ERRORNAMES[PKIX_NUMERRORS];
        PKIX_Error *error = NULL;
        PKIX_Error *cause = NULL;
        PKIX_PL_String *desc = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *causeString = NULL;
        PKIX_PL_String *optCauseString = NULL;
        PKIX_PL_String *errorNameString = NULL;
        char *format = NULL;
        PKIX_UInt32 code;

        PKIX_ENTER(ERROR, "pkix_Error_ToString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_ERROR_TYPE, plContext),
                    "Object is not an Error object");

        error = (PKIX_Error *)object;

        /* Get this error's code, description and the string of its cause */
        code = error->code;

        /* Get the description string */
        desc = error->desc;

        /* Get the cause */
        cause = error->cause;

        /* Get the causes's description string */
        if (cause != NULL) {
                pkix_error_cause_depth++;

                /* Get the cause string */
                PKIX_CHECK(PKIX_PL_Object_ToString
                            ((PKIX_PL_Object*)cause, &causeString, plContext),
                            "Error getting cause string");

                format = "\n*** Cause (%d): %s";

                PKIX_CHECK(PKIX_PL_String_Create
                            (PKIX_ESCASCII,
                            format,
                            0,
                            &formatString,
                            plContext),
                            "PKIX_PL_String_Create failed");

                /* Create the optional Cause String */
                PKIX_CHECK(PKIX_PL_Sprintf
                            (&optCauseString,
                            plContext,
                            formatString,
                            pkix_error_cause_depth,
                            causeString),
                            "PKIX_PL_Sprintf failed");

                PKIX_DECREF(formatString);

                pkix_error_cause_depth--;
        }

        /* Create the Format String */
        if (optCauseString != NULL) {
                format = "*** %s - %s%s";
        } else {
                format = "*** %s - %s";
        }

        /* Ensure that error code is known, otherwise default to Object */
        if (code >= PKIX_NUMERRORS) {
                code = 0;
        }

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    (void *)PKIX_ERRORNAMES[code],
                    0,
                    &errorNameString,
                    plContext),
                    "PKIX_PL_String_Create failed");

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    format,
                    0,
                    &formatString,
                    plContext),
                    "PKIX_PL_String_Create failed");


        /* Create the output String */
        PKIX_CHECK(PKIX_PL_Sprintf
                    (pString,
                    plContext,
                    formatString,
                    errorNameString,
                    desc,
                    optCauseString),
                    "PKIX_PL_Sprintf failed");

cleanup:

        PKIX_DECREF(causeString);
        PKIX_DECREF(formatString);
        PKIX_DECREF(optCauseString);
        PKIX_DECREF(errorNameString);

        PKIX_RETURN(ERROR);
}

/*
 * FUNCTION: pkix_Error_Hashcode
 * (see comments for PKIX_PL_HashcodeCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_Error_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pResult,
        void *plContext)
{
        PKIX_ENTER(ERROR, "pkix_Error_Hashcode");
        PKIX_NULLCHECK_TWO(object, pResult);

        /* XXX Unimplemented */
        /* XXX Need to make hashcodes equal when two errors are equal */
        *pResult = (PKIX_UInt32)object;

        PKIX_RETURN(ERROR);
}

/* --Initializers------------------------------------------------- */

/*
 * PKIX_ERRORNAMES is an array of strings, with each string holding a
 * descriptive name for an error code. This is used by the default
 * PKIX_PL_Error_ToString function.
 *
 * Note: These default names are indexed by error type. Therefore, order of
 * these default names MUST match the order of the error types in pkixt.h.
 * For example, since PKIX_ERRORNAMES[2] = "Memory Error", then PKIX_MEM_ERROR
 * must have the value 2 in pkixt.h
 */
const char *
PKIX_ERRORNAMES[PKIX_NUMERRORS] =
{
        "Object Error",
        "Fatal Error",
        "Memory Error",
        "Error Error",
        "Mutex Error",
        "RWLock Error",
        "String Error",
        "OID Error",
        "List Error",
        "ByteArray Error",
        "BigInt Error",
        "HashTable Error",
        "Cert Error",
        "X500Name Error",
        "GeneralName Error",
        "PublicKey Error",
        "Date Error",
        "TrustAnchor Error",
        "ProcessingParams Error",
        "NoLongerUsed Error",
        "ValidateParams Error",
        "Validate Error",
        "ValidateResult Error",
        "CertChainChecker Error",
        "CertSelector Error",
        "ComCertSelParams Error",
        "TargetCertCheckerState Error",
        "CertBasicConstraints Error",
        "CertPolicyQualifier Error",
        "CertPolicyInfo Error",
        "CertPolicyNode Error",
        "CertPolicyCheckerState Error",
        "Lifecycle Error",
        "BasicConstraintsCheckerState Error",
        "ComCRLSelParams Error",
        "CertStore Error",
        "CollectionCertStoreContext Error",
        "DefaultCRLCheckerState Error",
        "CRL Error",
        "CRLEntry Error",
        "CRLSelector Error",
        "CertPolicyMap Error",
        "Build Error",
        "BuildResult Error",
        "BuildParams Error",
        "ForwardBuilderState Error",
        "SignatureCheckerState Error",
        "CertNameConstraints Error",
        "CertNameConstraintsCheckerState Error",
        "RevocationChecker Error",
        "User Defined Modules Error",
        "Context Error",
        "DefaultRevocationChecker Error",
        "LdapRequest Error",
        "LdapResponse Error",
        "LdapClient Error",
        "LdapDefaultClient Error",
        "Socket Error",
        "Resource Limits Error",
        "Logger Error",
        "MonitorLock Error",
        "InfoAccess Error",
        "AIAMgr Error",
        "OcspChecker Error",
        "OcspRequest Error",
        "OcspResponse Error",
        "HttpClient Error"
};

/*
 * FUNCTION: pkix_Error_RegisterSelf
 * DESCRIPTION:
 *  Registers PKIX_ERROR_TYPE and its related functions with systemClasses[]
 * THREAD SAFETY:
 *  Not Thread Safe - for performance and complexity reasons
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_Error_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(ERROR, "pkix_Error_RegisterSelf");

        entry.description = "Error";
        entry.destructor = pkix_Error_Destroy;
        entry.equalsFunction = pkix_Error_Equals;
        entry.hashcodeFunction = pkix_Error_Hashcode;
        entry.toStringFunction = pkix_Error_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_ERROR_TYPE] = entry;

        PKIX_RETURN(ERROR);
}

/* --Public-Functions--------------------------------------------- */

/*
 * FUNCTION: PKIX_Error_Create (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Error_Create(
        PKIX_UInt32 errorCode,
        PKIX_Error *cause,
        PKIX_PL_Object *info,
        PKIX_PL_String *desc,
        PKIX_Error **pError,
        void *plContext)
{
        PKIX_Error *tempCause = NULL;
        PKIX_Error *error = NULL;

        PKIX_ENTER(ERROR, "PKIX_Error_Create");

        PKIX_NULLCHECK_ONE(pError);

        /*
         * when called here, if PKIX_PL_Object_Alloc returns an error,
         * it must be a PKIX_ALLOC_ERROR
         */
        pkixErrorResult = PKIX_PL_Object_Alloc
                (PKIX_ERROR_TYPE,
                ((PKIX_UInt32)(sizeof (PKIX_Error))),
                (PKIX_PL_Object **)&error,
                plContext);

        if (pkixErrorResult) return (pkixErrorResult);

        error->code = errorCode;

        /* Ensure we don't have a loop. Follow causes until NULL */
        for (tempCause = cause;
            tempCause != NULL;
            tempCause = tempCause->cause) {
                /* If we detect a loop, throw a new error */
                if (tempCause == error) {
                        PKIX_THROW(ERROR, "Loop of error causes detected");
                }
        }

        PKIX_INCREF(cause);
        error->cause = cause;

        PKIX_INCREF(info);
        error->info = info;

        PKIX_INCREF(desc);
        error->desc = desc;

        *pError = error;

        PKIX_RETURN(ERROR);
}

/*
 * FUNCTION: PKIX_Error_GetErrorCode (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Error_GetErrorCode(
        PKIX_Error *error,
        PKIX_UInt32 *pCode,
        void *plContext)
{
        PKIX_ENTER(ERROR, "PKIX_Error_GetErrorCode");
        PKIX_NULLCHECK_TWO(error, pCode);

        *pCode = error->code;

        PKIX_RETURN(ERROR);
}

/*
 * FUNCTION: PKIX_Error_GetCause (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Error_GetCause(
        PKIX_Error *error,
        PKIX_Error **pCause,
        void *plContext)
{
        PKIX_ENTER(ERROR, "PKIX_Error_GetCause");
        PKIX_NULLCHECK_TWO(error, pCause);

        if (error->cause != PKIX_ALLOC_ERROR()){
                PKIX_INCREF(error->cause);
        }

        *pCause = error->cause;

        PKIX_RETURN(ERROR);
}

/*
 * FUNCTION: PKIX_Error_GetSupplementaryInfo (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Error_GetSupplementaryInfo(
        PKIX_Error *error,
        PKIX_PL_Object **pInfo,
        void *plContext)
{
        PKIX_ENTER(ERROR, "PKIX_Error_GetSupplementaryInfo");
        PKIX_NULLCHECK_TWO(error, pInfo);

        PKIX_INCREF(error->info);

        *pInfo = error->info;

        PKIX_RETURN(ERROR);
}

/*
 * FUNCTION: PKIX_Error_GetDescription (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Error_GetDescription(
        PKIX_Error *error,
        PKIX_PL_String **pDesc,
        void *plContext)
{
        PKIX_ENTER(ERROR, "PKIX_Error_GetDescription");
        PKIX_NULLCHECK_TWO(error, pDesc);

        if (error != PKIX_ALLOC_ERROR()){
                PKIX_INCREF(error->desc);
        }

        *pDesc = error->desc;

        PKIX_RETURN(ERROR);
}
