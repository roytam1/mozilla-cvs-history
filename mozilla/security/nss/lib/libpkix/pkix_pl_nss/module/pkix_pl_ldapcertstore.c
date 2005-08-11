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

#include "pkix_pl_ldapcertstore.h"

static const char* connectedStatusStrings[] = {
        "NOT_CONNECTED",
        "CONNECT_PENDING",
        "CONNECTED",
        "BIND_PENDING",
        "BIND_RESPONSE",
        "BIND_RESPONSE_PENDING",
        "BOUND",
        "SEND_PENDING",
        "RECV",
        "RECV_PENDING",
        "RECV_INITIAL",
        "RECV_NONINITIAL"
};

/* --Private-LdapCertStoreContext-Message-Building-Functions---------------- */

/*
 * FUNCTION: pkix_pl_LdapCertStoreContext_MakeBind
 * DESCRIPTION:
 *
 *  This function creates and encodes a Bind message, using the arena pointed
 *  to by "arena", the version number contained in "versionData", the strings
 *  pointed to by "bindName" and "authentication", and the messageID contained
 *  in "msgNum", and stores a pointer to the encoded string at "pBindMsg".
 *
 *  See pkix_pl_ldaptemplates.c for the ASN.1 description of a Bind message.
 *
 *  Bind and Unbind do not seem to be expected for anonymous Search requests,
 *  and therefore this code is not used in the current configuration.
 *
 * PARAMETERS:
 *  "arena"
 *      The address of the PRArenaPool used in encoding the message. Must be
 *       non-NULL.
 *  "versionData"
 *      The Int32 containing the version number to be encoded in the Bind
 *      message.
 *  "bindName"
 *      The name to be encoded in the Bind message. Must be non-NULL.
 *  "authentication"
 *      The authentication string to be encoded in the Bind message. Must be
 *      non-NULL.
 *  "msgNum"
 *      The Int32 containing the MessageID to be encoded in the Bind message.
 *  "pBindMsg"
 *      The address at which the encoded Bind message will be stored. Must be
 *      non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_MakeBind(
        PRArenaPool *arena,
        PKIX_Int32 versionData,
        char *bindName,
        char *authentication,
        PKIX_UInt32 msgNum,
        SECItem **pBindMsg,
        void *plContext)
{
        LDAPMessage msg;
        char version = '\0';
        SECItem *encoded = NULL;
        PKIX_UInt32 len = 0;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_MakeBind");
        PKIX_NULLCHECK_FOUR(arena, bindName, authentication, pBindMsg);

        PKIX_PL_NSSCALL(LDAPCERTSTORECONTEXT, PORT_Memset,
                (&msg, 0, sizeof (LDAPMessage)));

        version = (char)versionData;

        msg.messageID.type = siUnsignedInteger;
        msg.messageID.data = (void*)&msgNum;
        msg.messageID.len = sizeof (msgNum);

        msg.protocolOp.selector = LDAP_BIND_TYPE;

        msg.protocolOp.op.bindMsg.version.type = siUnsignedInteger;
        msg.protocolOp.op.bindMsg.version.data = (void *)&version;
        msg.protocolOp.op.bindMsg.version.len = sizeof (char);

        msg.protocolOp.op.bindMsg.bindName.type = siAsciiString;
        msg.protocolOp.op.bindMsg.bindName.data = (void *)bindName;
        msg.protocolOp.op.bindMsg.bindName.len = strlen (bindName);

#if 0
        msg.protocolOp.op.bindMsg.authentication.selector = SIMPLE_AUTH;
        msg.protocolOp.op.bindMsg.authentication.ch.simple.type = siAsciiString;
        msg.protocolOp.op.bindMsg.authentication.ch.simple.data =
                (void *)authentication;
        len = strlen (authentication);
        if (len == 0) len = 1;
        msg.protocolOp.op.bindMsg.authentication.ch.simple.len = len;
#endif
        msg.protocolOp.op.bindMsg.authentication.type = siAsciiString;
        msg.protocolOp.op.bindMsg.authentication.data = (void *)authentication;
        len = strlen (authentication);
        msg.protocolOp.op.bindMsg.authentication.len = len;

        PKIX_PL_NSSCALLRV
                (LDAPCERTSTORECONTEXT,
                encoded,
                SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, LDAPMessageTemplate));
        if (!encoded) {
                PKIX_ERROR("failed in encoding bindRequest");
        }

        *pBindMsg = encoded;
cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStoreContext_MakeUnbind
 * DESCRIPTION:
 *
 *  This function creates and encodes a Unbind message, using the arena pointed
 *  to by "arena", the version number contained in "versionData", the strings
 *  pointed to by "bindName" and "authentication", and the messageID contained
 *  in "msgNum", and stores a pointer to the encoded string at "pBindMsg".
 *
 *  See pkix_pl_ldaptemplates.c for the ASN.1 description of an Unbind message.
 *
 *  Bind and Unbind do not seem to be expected for anonymous Search requests,
 *  and therefore this code is not used in the current configuration.
 *
 * PARAMETERS:
 *  "arena"
 *      The address of the PRArenaPool used in encoding the message. Must be
 *       non-NULL.
 *  "msgNum"
 *      The Int32 containing the MessageID to be encoded in the Unbind message.
 *  "pUnbindMsg"
 *      The address at which the encoded Unbind message will be stored. Must
 *      be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_MakeUnbind(
        PRArenaPool *arena,
        PKIX_UInt32 msgNum,
        SECItem **pUnbindMsg,
        void *plContext)
{
        LDAPMessage msg;
        SECItem *encoded = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_MakeUnbind");
        PKIX_NULLCHECK_TWO(arena, pUnbindMsg);

        PKIX_PL_NSSCALL(LDAPCERTSTORECONTEXT, PORT_Memset,
                (&msg, 0, sizeof (LDAPMessage)));

        msg.messageID.type = siUnsignedInteger;
        msg.messageID.data = (void*)&msgNum;
        msg.messageID.len = sizeof (msgNum);

        msg.protocolOp.selector = LDAP_UNBIND_TYPE;

        msg.protocolOp.op.unbindMsg.dummy.type = siBuffer;
        msg.protocolOp.op.unbindMsg.dummy.data = NULL;
        msg.protocolOp.op.unbindMsg.dummy.len = 0;

        PKIX_PL_NSSCALLRV
                (LDAPCERTSTORECONTEXT,
                encoded,
                SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, LDAPMessageTemplate));
        if (!encoded) {
                PKIX_ERROR("failed in encoding unbind");
        }

        *pUnbindMsg = encoded;
cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStoreContext_DecodeBindResponse
 * DESCRIPTION:
 *
 *  This function decodes the encoded data pointed to by "src", using the arena
 *  pointed to by "arena", storing the decoded LDAPMessage at "pBindResponse"
 *  and the decoding status at "pStatus".
 *
 * PARAMETERS:
 *  "arena"
 *      The address of the PRArenaPool to be used in decoding the message. Must
 *      be  non-NULL.
 *  "src"
 *      The address of the SECItem containing the DER- (or BER-)encoded string.
 *       Must be non-NULL.
 *  "pBindResponse"
 *      The address at which the LDAPMessage is stored, if the decoding is
 *      successful (the returned status is SECSuccess). Must be non-NULL.
 *  "pStatus"
 *      The address at which the decoding status is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_DecodeBindResponse(
        PRArenaPool *arena,
        SECItem *src,
        LDAPMessage *pBindResponse,
        SECStatus *pStatus,
        void *plContext)
{
        SECStatus rv = SECFailure;
        LDAPMessage response;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT,
                "pkix_pl_LdapCertStore_DecodeBindResponse");
        PKIX_NULLCHECK_FOUR(arena, src, pBindResponse, pStatus);

        PKIX_PL_NSSCALL
                (LDAPCERTSTORECONTEXT,
                PORT_Memset,
                (&response, 0, sizeof (LDAPMessage)));

        PKIX_PL_NSSCALLRV
                (LDAPCERTSTORECONTEXT,
                rv,
                SEC_ASN1DecodeItem,
                (arena, &response, LDAPMessageTemplate, src));

        if (rv == SECSuccess) {
                *pBindResponse = response;
        }

        *pStatus = rv;

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStoreContext_VerifyBindResponse
 * DESCRIPTION:
 *
 *  This function verifies that the contents of the message in the rcvbuf of
 *  the LdapCertStoreContext object pointed to by "lcs",  and whose length is
 *  provided by "buflen", is a response to a successful Bind.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "buflen"
 *      The value of the number of bytes in the receive buffer.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_VerifyBindResponse(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_UInt32 bufLen,
        void *plContext)
{
        SECItem decode = {siBuffer, NULL, 0};
        SECStatus rv = SECFailure;
        LDAPMessage msg;
        LDAPBindResponse *ldapBindResponse = NULL;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT,
                "pkix_pl_LdapCertStore_VerifyBindResponse");
        PKIX_NULLCHECK_TWO(lcs, lcs->rcvBuf);

        decode.data = (void *)(lcs->rcvBuf);
        decode.len = bufLen;

        PKIX_CHECK(pkix_pl_LdapCertStore_DecodeBindResponse
                (lcs->arena, &decode, &msg, &rv, plContext),
                "pkix_pl_LdapCertStore_DecodeBindResponse failed");

        if (rv == SECSuccess) {
                ldapBindResponse = &msg.protocolOp.op.bindResponseMsg;
                if (*(ldapBindResponse->resultCode.data) == SUCCESS) {
                        lcs->connectStatus = LDAP_BOUND;
                } else {
                        PKIX_ERROR("BIND rejected by server");
                }
        } else {
                PKIX_ERROR("Can't decode BIND response from server");
        }

cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_RecvCheckComplete
 * DESCRIPTION:
 *
 *  This function determines whether the current response in the
 *  LdapCertStoreContext pointed to by "lcs" is complete, in the sense that all
 *  bytes required to satisfy the message length field in the encoding have been
 *  received. If so, the pointer to input data is updated to reflect the number
 *  of bytes consumed, provided by "bytesProcessed". The state machine flag
 *  pointed to by "pKeepGoing" is updated to indicate whether processing can
 *  continue without further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "bytesProcessed"
 *      The UInt32 value of the number of bytes consumed from the current
 *      buffer.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_RecvCheckComplete(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_UInt32 bytesProcessed,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Boolean complete = PKIX_FALSE;
        SECStatus rv = SECFailure;
        LDAPMessageType messageType = 0;
        LDAPResultCode resultCode = 0;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT,
                "pkix_pl_LdapCertStore_RecvCheckComplete");
        PKIX_NULLCHECK_TWO(lcs, pKeepGoing);

        PKIX_CHECK(pkix_pl_LdapResponse_IsComplete
                (lcs->currentResponse, &complete, plContext),
                "pkix_pl_LdapResponse_IsComplete failed");

        if (complete) {
                PKIX_CHECK(pkix_pl_LdapResponse_Decode
                        (lcs->arena, lcs->currentResponse, &rv, plContext),
                        "pkix_pl_LDAPResponse_Decode failed");

                if (rv != SECSuccess) {
                        PKIX_ERROR("Can't decode SEARCH response from server");
                }

                PKIX_CHECK(pkix_pl_LdapResponse_GetMessageType
                        (lcs->currentResponse, &messageType, plContext),
                        "pkix_pl_LdapResponse_GetMessageType failed");

                if (messageType == LDAP_SEARCHRESPONSEENTRY_TYPE) {

                        if (lcs->entriesFound == NULL) {
                                PKIX_CHECK(PKIX_List_Create
                                        (&(lcs->entriesFound), plContext),
                                        "PKIX_List_Create failed");
                        }

                        PKIX_CHECK(PKIX_List_AppendItem
                                (lcs->entriesFound,
                                (PKIX_PL_Object *)lcs->currentResponse,
                                plContext),
                                "PKIX_List_AppendItem failed");

                        PKIX_DECREF(lcs->currentResponse);

                        /* current receive buffer empty? */
                        if (lcs->currentBytesAvailable == 0) {
                                lcs->connectStatus = LDAP_RECV;
                                *pKeepGoing = PKIX_TRUE;
                        } else {
                                lcs->connectStatus = LDAP_RECV_INITIAL;
                                lcs->currentInPtr = &((char *)
                                        (lcs->currentInPtr))[bytesProcessed];
                                *pKeepGoing = PKIX_TRUE;
                        }

                } else if (messageType == LDAP_SEARCHRESPONSERESULT_TYPE) {
                        PKIX_CHECK(pkix_pl_LdapResponse_GetResultCode
                                (lcs->currentResponse, &resultCode, plContext),
                                "pkix_pl_LdapResponse_GetResultCode failed");

                        if (resultCode == SUCCESS) {

                                PKIX_NULLCHECK_ONE(lcs->entriesFound);
                                PKIX_CHECK(PKIX_PL_HashTable_Add
                                        (lcs->cachePtr,
                                        (PKIX_PL_Object *)lcs->currentRequest,
                                        (PKIX_PL_Object *)lcs->entriesFound,
                                        plContext),
                                        "PKIX_PL_HashTable_Add failed");

                        }

                        lcs->connectStatus = LDAP_BOUND;
                        *pKeepGoing = PKIX_FALSE;
                        PKIX_DECREF(lcs->currentResponse);

                } else {
                        PKIX_ERROR("SearchResponse packet of unknown type");
                }
        } else {
                lcs->connectStatus = LDAP_RECV;
                *pKeepGoing = PKIX_TRUE;
        }

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/* --Private-LdapCertStoreContext-Object-Functions------------------------- */
/*
 * FUNCTION: PKIX_PL_LdapCertStoreContext_Create
 * DESCRIPTION:
 *
 *  This function creates a new LdapCertStoreContext using the Socket pointed to
 *  by "socket", the bindName pointed to by "bindName", and the authentication
 *  pointed to by "authentication", and stores the result at "pContext".
 *
 * PARAMETERS:
 *  "socket"
 *      The address of the Socket to be used in communication with the LDAP
 *      server. Must be non-NULL.
 *  "bindName"
 *      The name to be encoded in the Bind message. Must be non-NULL.
 *  "authentication"
 *      The authentication string to be encoded in the Bind message. Must be
 *      non-NULL.
 *  "pContext"
 *      The address at which the created LdapCertStoreContext is to be stored.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in
 *      a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStoreContext_Create(
        PKIX_PL_Socket *socket,
        char *bindName,
        char *authentication,
        PKIX_PL_LdapCertStoreContext **pContext,
        void *plContext)
{
        PKIX_PL_HashTable *ht;
        PKIX_PL_LdapCertStoreContext *ldapCertStoreContext = NULL;
        PKIX_PL_Socket_Callback *callbackList;
        PRArenaPool *arena = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT,
                    "pkix_pl_LdapCertStoreContext_Create");
        PKIX_NULLCHECK_FOUR(socket, bindName, authentication, pContext);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LDAPCERTSTORECONTEXT_TYPE,
                    sizeof (PKIX_PL_LdapCertStoreContext),
                    (PKIX_PL_Object **)&ldapCertStoreContext,
                    plContext),
                    "Could not create LdapCertStoreContext object");

        ldapCertStoreContext->connectStatus = LDAP_NOT_CONNECTED;

        PKIX_CHECK(PKIX_PL_HashTable_Create
                (LDAP_CACHEBUCKETS, &ht, plContext),
                "PKIX_PL_HashTable_Create failed");

        ldapCertStoreContext->cachePtr = ht;

        PKIX_CHECK(pkix_pl_Socket_GetCallbackList
                (socket, &callbackList, plContext),
                "pkix_pl_Socket_GetCallbackList failed");
        ldapCertStoreContext->callbackList = callbackList;

        PKIX_INCREF(socket);
        ldapCertStoreContext->clientSocket = socket;

        ldapCertStoreContext->messageID = 0;

        ldapCertStoreContext->bindName = bindName;
        ldapCertStoreContext->authentication = authentication;

        PKIX_PL_NSSCALLRV
                (LDAPCERTSTORECONTEXT,
                arena,
                PORT_NewArena,
                (DER_DEFAULT_CHUNKSIZE));
        if (!arena) {
                PKIX_ERROR_FATAL("Out of memory");
        }
        ldapCertStoreContext->arena = arena;

        ldapCertStoreContext->sendBuf = NULL;
        ldapCertStoreContext->bytesToWrite = 0;

        PKIX_CHECK(PKIX_PL_Malloc
                (RCVBUFSIZE, &ldapCertStoreContext->rcvBuf, plContext),
                "PKIX_PL_Malloc failed");
        ldapCertStoreContext->capacity = RCVBUFSIZE;

        ldapCertStoreContext->bindMsg = NULL;
        ldapCertStoreContext->bindMsgLen = 0;

        ldapCertStoreContext->entriesFound = NULL;
        ldapCertStoreContext->currentRequest = NULL;
        ldapCertStoreContext->currentResponse = NULL;

        *pContext = ldapCertStoreContext;

cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStoreContext_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_pl_LdapCertStoreContext_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_LdapCertStoreContext *lcs = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT,
                    "pkix_pl_LdapCertStoreContext_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_LDAPCERTSTORECONTEXT_TYPE, plContext),
                    "Object is not an LdapCertStoreContext");

        lcs = (PKIX_PL_LdapCertStoreContext *)object;

        switch (lcs->connectStatus) {
        case LDAP_NOT_CONNECTED:
        case LDAP_CONNECT_PENDING:
        case LDAP_CONNECTED:
                break;
        case LDAP_BIND_PENDING:
        case LDAP_BIND_RESPONSE:
        case LDAP_BIND_RESPONSE_PENDING:
        case LDAP_SEND_PENDING:
        case LDAP_RECV:
        case LDAP_RECV_PENDING:
        case LDAP_BOUND:
#if 0
                PKIX_CHECK(pkix_pl_LdapCertStore_MakeUnbind
                        (lcs->arena, ++(lcs->messageID), &encoded, plContext),
                        "pkix_pl_LdapCertStore_MakeUnbind failed");

                callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);
                PKIX_CHECK(callbackList->sendCallback
                        (lcs->clientSocket,
                        encoded->data,
                        encoded->len,
                        &bytesWritten,
                        plContext),
                        "pkix_pl_Socket_Send failed");
#endif
                break;
        default:
                PKIX_ERROR("LDAP CertStore in illegal state");
        }

        PKIX_DECREF(lcs->cachePtr);
        PKIX_DECREF(lcs->clientSocket);
        PKIX_DECREF(lcs->entriesFound);
        PKIX_DECREF(lcs->currentRequest);
        PKIX_DECREF(lcs->currentResponse);

        PKIX_CHECK(PKIX_PL_Free(lcs->rcvBuf, plContext), "PKIX_PL_Free failed");

        PKIX_PL_NSSCALL
                (LDAPCERTSTORECONTEXT,
                PORT_FreeArena,
                (lcs->arena, PR_FALSE));

cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStoreContext_ToString
 * (see comments for PKIX_PL_ToStringCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_pl_LdapCertStoreContext_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        char *asciiFormat =
                "hashTable at %0x, socket at %0x, session %s, messageID %d";
        PKIX_PL_LdapCertStoreContext *ldapCSC = NULL;
        PKIX_PL_String *format = NULL;
        PKIX_PL_String *statusString = NULL;
        PKIX_PL_String *outString = NULL;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStoreContext_ToString");

        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_LDAPCERTSTORECONTEXT_TYPE, plContext),
                "Object is not an LdapCertStoreContext");

        ldapCSC = (PKIX_PL_LdapCertStoreContext *)object;

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII,
                (void *)connectedStatusStrings[ldapCSC->connectStatus],
                0,
                &statusString,
                plContext),
                "Error in PKIX_PL_String_Create");

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, asciiFormat, 0, &format, plContext),
                "Error in PKIX_PL_String_Create");

        PKIX_CHECK(PKIX_PL_Sprintf
                (&outString,
                plContext,
                format,
                ldapCSC->cachePtr,
                ldapCSC->clientSocket,
                statusString,
                ldapCSC->messageID),
                "Error in PKIX_PL_Sprintf");

        *pString = outString;

cleanup:
        PKIX_DECREF(statusString);
        PKIX_DECREF(format);

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStoreContext_RegisterSelf
 *
 * DESCRIPTION:
 *  Registers PKIX_PL_LDAPCERTSTORECONTEXT_TYPE and its related
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
pkix_pl_LdapCertStoreContext_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT,
                "pkix_pl_LdapCertStoreContext_RegisterSelf");

        entry.description = "LdapCertStoreContext";
        entry.destructor = pkix_pl_LdapCertStoreContext_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = pkix_pl_LdapCertStoreContext_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_LDAPCERTSTORECONTEXT_TYPE] = entry;

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/* --Private-Ldap-CertStore-I/O-Functions---------------------------- */
/*
 * FUNCTION: pkix_pl_LdapCertStore_IsIOPending
 * DESCRIPTION:
 *
 *  This function determined whether the state of the connection of the
 *  LdapCertStoreContext pointed to by "lcs" indicates I/O is in progress, and
 *  stores the Boolean result at "pPending".
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pPending"
 *      The address at which the result is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error*
pkix_pl_LdapCertStore_IsIOPending(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pPending,
        void *plContext)
{
        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_IsIOPending");
        PKIX_NULLCHECK_TWO(lcs, pPending);

        if ((lcs->connectStatus == LDAP_CONNECT_PENDING) ||
            (lcs->connectStatus == LDAP_BIND_PENDING) ||
            (lcs->connectStatus == LDAP_BIND_RESPONSE_PENDING) ||
            (lcs->connectStatus == LDAP_SEND_PENDING) ||
            (lcs->connectStatus == LDAP_RECV_PENDING)) {
                *pPending = PKIX_TRUE;
        } else {
                *pPending = PKIX_FALSE;
        }

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

#if 0
/*
 * FUNCTION: pkix_pl_LdapCertStore_Connect
 * DESCRIPTION:
 *
 *  This function performs the socket Connect function for the CertStore
 *  embodied in the LdapCertStoreContext "lcs", and stores in "pKeepGoing" a
 *  flag indicating whether processing can continue without further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_Connect(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_PL_Socket_Callback *callbackList;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_Connect");
        PKIX_NULLCHECK_ONE(lcs);

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->connectCallback
                (lcs->clientSocket, plContext),
                "pkix_pl_Socket_Connect failed");

        lcs->connectStatus = LDAP_CONNECTED;
        lcs->lastIO = PR_Now();

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)lcs, plContext),
                "PKIX_PL_Object_InvalidateCache failed");

        *pKeepGoing = PKIX_TRUE;

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}
#endif

/*
 * FUNCTION: pkix_pl_LdapCertStore_ConnectContinue
 * DESCRIPTION:
 *
 *  This function determines whether a socket Connect initiated earlier for the
 *  CertStore embodied in the LdapCertStoreContext "lcs" has completed, and
 *  stores in "pKeepGoing" a flag indicating whether processing can continue
 *  without further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_ConnectContinue(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_PL_Socket_Callback *callbackList;
        PRErrorCode status;
        PKIX_Boolean keepGoing = PKIX_FALSE;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT,
                "pkix_pl_LdapCertStore_ConnectContinue");
        PKIX_NULLCHECK_ONE(lcs);

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->connectcontinueCallback
                (lcs->clientSocket, &status, plContext),
                "pkix_pl_Socket_ConnectContinue failed");

        if (status == 0) {
                lcs->connectStatus = LDAP_CONNECTED;
                keepGoing = PKIX_TRUE;
        } else if (status != PR_IN_PROGRESS_ERROR) {
                PKIX_ERROR("Unexpected error in establishing connection");
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)lcs, plContext),
                "PKIX_PL_Object_InvalidateCache failed");

        *pKeepGoing = keepGoing;

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_Bind
 * DESCRIPTION:
 *
 *  This function creates and sends the LDAP-protocol Bind message for the
 *  CertStore embodied in the LdapCertStoreContext "lcs", and stores in
 *  "pKeepGoing" a flag indicating whether processing can continue without
 *  further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_Bind(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        SECItem *encoded = NULL;
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_Bind");
        PKIX_NULLCHECK_ONE(lcs);

        /* if we have not yet constructed the BIND message, build it now */
        if (!(lcs->bindMsg)) {
                PKIX_CHECK(pkix_pl_LdapCertStore_MakeBind
                        (lcs->arena,
                        3,
                        lcs->bindName,
                        lcs->authentication,
                        lcs->messageID,
                        &encoded,
                        plContext),
                        "pkix_pl_LdapCertStore_MakeBind failed");
                lcs->bindMsg = encoded->data;
                lcs->bindMsgLen = encoded->len;
        }

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->sendCallback
                (lcs->clientSocket,
                lcs->bindMsg,
                lcs->bindMsgLen,
                &bytesWritten,
                plContext),
                "pkix_pl_Socket_Send failed");

        lcs->lastIO = PR_Now();

        if (bytesWritten < 0) {
                lcs->connectStatus = LDAP_BIND_PENDING;
                *pKeepGoing = PKIX_FALSE;
        } else {
                lcs->connectStatus = LDAP_BIND_RESPONSE;
                *pKeepGoing = PKIX_TRUE;
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)lcs, plContext),
                "PKIX_PL_Object_InvalidateCache failed");

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_BindContinue
 * DESCRIPTION:
 *
 *  This function determines whether the LDAP-protocol Bind message for the
 *  CertStore embodied in the LdapCertStoreContext "lcs" has completed, and
 *  stores in "pKeepGoing" a flag indicating whether processing can continue
 *  without further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *pkix_pl_LdapCertStore_BindContinue(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_BindContinue");
        PKIX_NULLCHECK_ONE(lcs);

        *pKeepGoing = PKIX_FALSE;

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (lcs->clientSocket, &bytesWritten, NULL, plContext),
                "pkix_pl_Socket_Poll failed");

        /*
         * If the send completed we can proceed to try for the
         * response. If the send did not complete we will have
         * continue to poll.
         */
        if (bytesWritten >= 0) {

                lcs->connectStatus = LDAP_BIND_RESPONSE;

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)lcs, plContext),
                        "PKIX_PL_Object_InvalidateCache failed");

                *pKeepGoing = PKIX_TRUE;
        }

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_BindResponse
 * DESCRIPTION:
 *
 *  This function attempts to read the LDAP-protocol BindResponse message for
 *  the CertStore embodied in the LdapCertStoreContext "lcs", and stores in
 *  "pKeepGoing" a flag indicating whether processing can continue without
 *  further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_BindResponse(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_BindResponse");
        PKIX_NULLCHECK_TWO(lcs, lcs->rcvBuf);

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->recvCallback
                (lcs->clientSocket,
                lcs->rcvBuf,
                lcs->capacity,
                &bytesRead,
                plContext),
                "pkix_pl_Socket_Recv failed");

        lcs->lastIO = PR_Now();

        if (bytesRead > 0) {
                PKIX_CHECK(pkix_pl_LdapCertStore_VerifyBindResponse
                        (lcs, bytesRead, plContext),
                        "pkix_pl_LdapCertStore_VerifyBindResponse failed");
                /* XXX Handle failure */
                lcs->connectStatus = LDAP_BOUND;
        } else {
                lcs->connectStatus = LDAP_BIND_RESPONSE_PENDING;
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)lcs, plContext),
                "PKIX_PL_Object_InvalidateCache failed");

        *pKeepGoing = PKIX_TRUE;

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_BindResponseContinue
 * DESCRIPTION:
 *
 *  This function determines whether the LDAP-protocol BindResponse message for
 *  the CertStore embodied in the LdapCertStoreContext "lcs" has completed, and
 *  stores in "pKeepGoing" a flag indicating whether processing can continue
 *  without further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_BindResponseContinue(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT,
                "pkix_pl_LdapCertStore_BindResponseContinue");
        PKIX_NULLCHECK_ONE(lcs);

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (lcs->clientSocket, NULL, &bytesRead, plContext),
                "pkix_pl_Socket_Poll failed");

        if (bytesRead > 0) {
                PKIX_CHECK(pkix_pl_LdapCertStore_VerifyBindResponse
                        (lcs, bytesRead, plContext),
                        "pkix_pl_LdapCertStore_VerifyBindResponse failed");
                lcs->connectStatus = LDAP_BOUND;

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)lcs, plContext),
                        "PKIX_PL_Object_InvalidateCache failed");

                *pKeepGoing = PKIX_TRUE;
        } else {
                *pKeepGoing = PKIX_FALSE;
        }

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_Send
 * DESCRIPTION:
 *
 *  This function creates and sends an LDAP-protocol message for the
 *  CertStore embodied in the LdapCertStoreContext "lcs", and stores in
 *  "pKeepGoing" a flag indicating whether processing can continue without
 *  further input, and at "pBytesTransferred" the number of bytes sent.
 *
 *  If "pBytesTransferred" is zero, it indicates that non-blocking I/O is in use
 *  and that transmission has not completed.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "pBytesTransferred"
 *      The address at which the number of bytes sent is stored. Must be
 *      non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_Send(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        PKIX_UInt32 *pBytesTransferred,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_Send");
        PKIX_NULLCHECK_THREE(lcs, pKeepGoing, pBytesTransferred);

        *pKeepGoing = PKIX_FALSE;

        /* Do we have anything waiting to go? */
        if (lcs->sendBuf) {
                callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

                PKIX_CHECK(callbackList->sendCallback
                        (lcs->clientSocket,
                        lcs->sendBuf,
                        lcs->bytesToWrite,
                        &bytesWritten,
                        plContext),
                        "pkix_pl_Socket_Send failed");

                lcs->lastIO = PR_Now();

                /*
                 * If the send completed we can proceed to try for the
                 * response. If the send did not complete we will have
                 * to poll for completion later.
                 */
                if (bytesWritten >= 0) {
                        lcs->sendBuf = NULL;
                        lcs->connectStatus = LDAP_RECV;
                        *pKeepGoing = PKIX_TRUE;

                } else {
                        *pKeepGoing = PKIX_FALSE;
                        lcs->connectStatus = LDAP_SEND_PENDING;
                }

        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)lcs, plContext),
                "PKIX_PL_Object_InvalidateCache failed");

        *pBytesTransferred = bytesWritten;

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_SendContinue
 * DESCRIPTION:
 *
 *  This function determines whether the sending of the LDAP-protocol message
 *  for the CertStore embodied in the LdapCertStoreContext "lcs" has completed,
 *  and stores in "pKeepGoing" a flag indicating whether processing can continue
 *  without further input, and at "pBytesTransferred" the number of bytes sent.
 *
 *  If "pBytesTransferred" is zero, it indicates that non-blocking I/O is in use
 *  and that transmission has not completed.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "pBytesTransferred"
 *      The address at which the number of bytes sent is stored. Must be
 *      non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_SendContinue(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        PKIX_UInt32 *pBytesTransferred,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_SendContinue");
        PKIX_NULLCHECK_THREE(lcs, pKeepGoing, pBytesTransferred);

        *pKeepGoing = PKIX_FALSE;

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (lcs->clientSocket, &bytesWritten, NULL, plContext),
                "pkix_pl_Socket_Poll failed");

        /*
         * If the send completed we can proceed to try for the
         * response. If the send did not complete we will have
         * continue to poll.
         */
        if (bytesWritten >= 0) {
                lcs->sendBuf = NULL;
                lcs->connectStatus = LDAP_RECV;

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)lcs, plContext),
                        "PKIX_PL_Object_InvalidateCache failed");

                *pKeepGoing = PKIX_TRUE;
        }

        *pBytesTransferred = bytesWritten;

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_Recv
 * DESCRIPTION:
 *
 *  This function receives an LDAP-protocol message for the  CertStore embodied
 *  in the LdapCertStoreContext "lcs", and stores in "pKeepGoing" a flag
 *  indicating whether processing can continue without further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_Recv(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_Recv");
        PKIX_NULLCHECK_THREE(lcs, pKeepGoing, lcs->rcvBuf);

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->recvCallback
                (lcs->clientSocket,
                (void *)lcs->rcvBuf,
                lcs->capacity,
                &bytesRead,
                plContext),
                "pkix_pl_Socket_Recv failed");

        lcs->currentInPtr = lcs->rcvBuf;
        lcs->lastIO = PR_Now();

        if (bytesRead > 0) {
                lcs->currentBytesAvailable = bytesRead;
                lcs->connectStatus = LDAP_RECV_INITIAL;
                *pKeepGoing = PKIX_TRUE;
        } else {
                lcs->connectStatus = LDAP_RECV_PENDING;
                *pKeepGoing = PKIX_FALSE;
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)lcs, plContext),
                "PKIX_PL_Object_InvalidateCache failed");

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_RecvContinue
 * DESCRIPTION:
 *
 *  This function determines whether the receiving of the LDAP-protocol message
 *  for the CertStore embodied in the LdapCertStoreContext "lcs" has completed,
 *  and stores in "pKeepGoing" a flag indicating whether processing can continue
 *  without further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_RecvContinue(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_RecvContinue");
        PKIX_NULLCHECK_TWO(lcs, pKeepGoing);

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (lcs->clientSocket, NULL, &bytesRead, plContext),
                "pkix_pl_Socket_Poll failed");

        if (bytesRead > 0) {
                lcs->currentBytesAvailable = bytesRead;
                lcs->connectStatus = LDAP_RECV_INITIAL;
                *pKeepGoing = PKIX_TRUE;

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)lcs, plContext),
                        "PKIX_PL_Object_InvalidateCache failed");
        } else {
                *pKeepGoing = PKIX_FALSE;
        }

cleanup:
        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_RecvInitial
 * DESCRIPTION:
 *
 *  This function processes the contents of the first buffer of a received
 *  LDAP-protocol message for the CertStore embodied in the LdapCertStoreContext
 *  "lcs", and stores in "pKeepGoing" a flag indicating whether processing can
 *  continue without further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_RecvInitial(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        unsigned char *msgBuf = NULL;
        PKIX_UInt32 dataIndex = 0;
        PKIX_UInt32 messageLength = 0;
        PKIX_UInt32 sizeofLength = 0;
        PKIX_UInt32 bytesProcessed = 0;
        unsigned char messageChar = 0;
        LDAPMessageType messageType = 0;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_RecvInitial");
        PKIX_NULLCHECK_TWO(lcs, pKeepGoing);

        /*
         * Is there an LDAPResponse in progress? I.e., have we
         * already processed the tag and length at the beginning of
         * the message?
         */
        if (lcs->currentResponse) {
                lcs->connectStatus = LDAP_RECV_NONINITIAL;
                *pKeepGoing = PKIX_TRUE;
                goto cleanup;
        }
        msgBuf = lcs->currentInPtr;

        /*
         * We have to determine whether the response is an entry, with
         * application-specific tag LDAP_SEARCHRESPONSEENTRY_TYPE, or a
         * resultCode, with application tag LDAP_SEARCHRESPONSERESULT_TYPE.
         * First, we have to figure out where to look for the tag.
         */

        /* XXX need to deal with currentBytesAvailable < sizeofLength */

        /* Is the message length short form (one octet) or long form? */
        if ((msgBuf[1] & 0x80) != 0) {
                sizeofLength = msgBuf[1] & 0x7F;
                for (dataIndex = 0; dataIndex < sizeofLength; dataIndex++) {
                        messageLength =
                                (messageLength << 8) + msgBuf[dataIndex + 2];
                }
        } else {
                messageLength = msgBuf[1];
        }

        /* How many bytes did the messageID require? */
        dataIndex += msgBuf[dataIndex + 3];

        messageChar = msgBuf[dataIndex + 4];

        /* Are we looking at an Entry message or a ResultCode message? */
        if ((SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION |
            LDAP_SEARCHRESPONSEENTRY_TYPE) == messageChar) {

                messageType = LDAP_SEARCHRESPONSEENTRY_TYPE;

        } else if ((SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION |
            LDAP_SEARCHRESPONSERESULT_TYPE) == messageChar) {

                messageType = LDAP_SEARCHRESPONSERESULT_TYPE;

        } else {

                PKIX_ERROR("SearchResponse packet of unknown type");

        }

        /*
         * messageLength is the length from (tag, length, value).
         * We have to allocate space for the tag and length bits too.
         */
        PKIX_CHECK(pkix_pl_LdapResponse_Create
                (messageType,
                messageLength + dataIndex + 1,
                lcs->currentBytesAvailable,
                msgBuf,
                &bytesProcessed,
                &(lcs->currentResponse),
                plContext),
                "pkix_pl_LdapResponseCreate failed");

        lcs->currentBytesAvailable -= bytesProcessed;

        PKIX_CHECK(pkix_pl_LdapCertStore_RecvCheckComplete
                (lcs, bytesProcessed, pKeepGoing, plContext),
                "pkix_pl_LdapCertStore_RecvCheckComplete failed");

cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_RecvNonInitial
 * DESCRIPTION:
 *
 *  This function processes the contents of buffers, after the first, of a
 *  received LDAP-protocol message for the CertStore embodied in the
 *  LdapCertStoreContext "lcs", and stores in "pKeepGoing" a flag indicating
 *  whether processing can continue without further input.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_RecvNonInitial(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{

        PKIX_UInt32 bytesProcessed = 0;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_RecvNonInitial");
        PKIX_NULLCHECK_TWO(lcs, pKeepGoing);

        PKIX_CHECK(pkix_pl_LdapResponse_Append
                (lcs->currentResponse,
                lcs->currentBytesAvailable,
                lcs->currentInPtr,
                &bytesProcessed,
                plContext),
                "pkix_pl_LdapResponse_Append failed");

        lcs->currentBytesAvailable -= bytesProcessed;

        PKIX_CHECK(pkix_pl_LdapCertStore_RecvCheckComplete
                (lcs, bytesProcessed, pKeepGoing, plContext),
                "pkix_pl_LdapCertStore_RecvCheckComplete failed");

cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertstore_Dispatch
 * DESCRIPTION:
 *
 *  This function is the state machine dispatcher for the CertStore embodied in
 *  the LdapCertStoreContext pointed to by "lcs". Results are returned by
 *  changes to various fields in the context.
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertstore_Dispatch(
        PKIX_PL_LdapCertStoreContext *lcs,
        void *plContext)
{
        PKIX_UInt32 bytesTransferred = 0;
        PKIX_Boolean keepGoing = PKIX_TRUE;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertstore_Dispatch");
        PKIX_NULLCHECK_ONE(lcs);

        while (keepGoing) {
                switch (lcs->connectStatus) {
#if 0
                case LDAP_NOT_CONNECTED:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_Connect
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_Connect failed");
                        break;
#endif
                case LDAP_CONNECT_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_ConnectContinue
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_ConnectContinue failed");
                        break;
                case LDAP_CONNECTED:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_Bind
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_Bind failed");
                        break;
                case LDAP_BIND_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_BindContinue
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_BindContinue failed");
                        break;
                case LDAP_BIND_RESPONSE:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_BindResponse
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_BindResponse failed");
                        break;
                case LDAP_BIND_RESPONSE_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_BindResponseContinue
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_BindResponseContinue"
                                " failed");
                        break;
                case LDAP_BOUND:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_Send
                                (lcs, &keepGoing, &bytesTransferred, plContext),
                                "pkix_pl_LdapCertStore_Send failed");
                        break;
                case LDAP_SEND_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_SendContinue
                                (lcs, &keepGoing, &bytesTransferred, plContext),
                                "pkix_pl_LdapCertStore_SendContinue failed");
                        break;
                case LDAP_RECV:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_Recv
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_Recv failed");
                        break;
                case LDAP_RECV_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_RecvContinue
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_RecvContinue failed");
                        break;
                case LDAP_RECV_INITIAL:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_RecvInitial
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_RecvInitial failed");
                        break;
                case LDAP_RECV_NONINITIAL:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_RecvNonInitial
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_RecvNonInitial failed");
                        break;
                default:
                        PKIX_ERROR("LDAP CertStore in illegal state");
                }
        }

cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_SendRequest
 * DESCRIPTION:
 *
 *  This function constructs an LDAPRequest object using the
 *  LdapCertStoreContext pointed to by "lcs", the subject name pointed to by
 *  "subjectName", and the attribute bits provided in "attrBits", and stores the
 *  response at "pResponse".
 *
 *  If non-blocking I/O is in use and the response is not yet available, NULL is
 *  stored at "pResponse". If the response has come back but contained no items
 *  satisfying the request, an empty List is stored at "pResponse".
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 *  "subjectName"
 *      The address of a SECItem containing the encoded subject name. Must be
 *      non-NULL.
 *  "attrBits"
 *      An LDAPAttrIncludeMask (defined in pkix_pl_ldapt.h) indicating
 *      attributes to be included in the LDAP-protocol Search request.
 *  "pResponse"
 *      The address where the List, or NULL for an unfinished request, is
 *      stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a LdapCertStoreContext Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_SendRequest(
        PKIX_PL_LdapCertStoreContext *lcs,
        SECItem *subjectName,
        LdapAttrIncludeMask attrBits,
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_PL_LdapRequest *searchRequest = NULL;
        PKIX_List *searchResponseList = NULL;
        SECItem *encoded = NULL;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT,
                "pkix_pl_LdapCertStore_SendRequest");
        PKIX_NULLCHECK_THREE(lcs, subjectName, pResponse);

        PKIX_CHECK(pkix_pl_LdapRequest_Create
                (lcs->arena,
                ++(lcs->messageID),
                "c=US",                 /* baseObject           */
                (char)WHOLE_SUBTREE,    /* scope                */
                (char)NEVER_DEREF,      /* derefAliases         */
                0,                      /* sizeLimit            */
                0,                      /* timeLimit            */
                PKIX_FALSE,             /* attrsOnly            */
                subjectName,            /* equalityMatch filter */
                attrBits,               /* attributes           */
                &searchRequest,
                plContext),
                "pkix_pl_LdapRequest_Create failed");

        /* check hashtable for matching request */
        PKIX_CHECK(PKIX_PL_HashTable_Lookup
                (lcs->cachePtr,
                (PKIX_PL_Object *)searchRequest,
                (PKIX_PL_Object **)&searchResponseList,
                plContext),
                "PKIX_PL_HashTable_Lookup failed");

        if (!searchResponseList) {
                /* It wasn't cached. We'll have to actually send it. */
                lcs->currentRequest = searchRequest;
                PKIX_INCREF(searchRequest);

                PKIX_CHECK(pkix_pl_LdapRequest_GetEncoded
                        (searchRequest, &encoded, plContext),
                        "pkix_pl_LdapRequest_GetEncoded failed");

                lcs->sendBuf = encoded->data;
                lcs->bytesToWrite = encoded->len;

                PKIX_CHECK(pkix_pl_LdapCertstore_Dispatch(lcs, plContext),
                        "pkix_pl_LdapCertstore_Dispatch failed");

                if (lcs->entriesFound) {
                        searchResponseList = lcs->entriesFound;
                        PKIX_INCREF(searchResponseList);

                        PKIX_DECREF(lcs->entriesFound);
                        PKIX_DECREF(lcs->currentRequest);
                }
        }

        *pResponse = searchResponseList;

cleanup:

        PKIX_DECREF(searchRequest);

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/* --Private-Ldap-CertStore-Database-Functions----------------------- */

/*
 * FUNCTION: pkix_pl_LdapCertStore_BuildCertList
 * DESCRIPTION:
 *
 *  This function takes a List of LdapResponse objects pointed to by
 *  "responseList" and extracts and decodes the Certificates in those responses,
 *  storing the List of those Certificates at "pCerts".
 *
 * PARAMETERS:
 * "responseList"
 *      The address of the List of LdapResponses. Must be non-NULL.
 * "pCerts"
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
        PKIX_PL_LdapResponse *response = NULL;
        PKIX_List *certList = NULL;
        LDAPMessage *message = NULL;
        LDAPSearchResponseEntry *sre = NULL;
        LDAPSearchResponseAttr **sreAttrArray = NULL;
        LDAPSearchResponseAttr *sreAttr = NULL;
        SECItem *attrType = NULL;
        SECItem **attrVal = NULL;

        PKIX_PL_Cert *cert = NULL;
        SECItem *derCertItem = NULL;
        CERTCertificate *nssCert = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_BuildCertList");
        PKIX_NULLCHECK_TWO(responseList, pCerts);

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
                    attrVal = sreAttr->val;
                    derCertItem = *attrVal++;
                    while (derCertItem != 0) {
                        /* create a PKIX_PL_Cert from derCert */
                        PKIX_PL_NSSCALLRV
                            (CERTSTORE, nssCert, CERT_DecodeDERCertificate,
                            (derCertItem, PR_FALSE, NULL));
                        if (nssCert) {
                            PKIX_CHECK_ONLY_FATAL
                                (pkix_pl_Cert_CreateWithNSSCert
                                (nssCert, &cert, plContext),
                                "pkix_pl_Cert_CreateWithNSSCert failed");

                            /* skip bad certs and append good ones */
                            if (!PKIX_ERROR_RECEIVED) {
                                if (certList == NULL) {
                                        PKIX_CHECK(PKIX_List_Create
                                        (&certList, plContext),
                                        "PKIX_List_Create failed");
                                }
                                PKIX_CHECK(PKIX_List_AppendItem
                                        (certList,
                                        (PKIX_PL_Object *) cert,
                                        plContext),
                                        "PKIX_List_AppendItem failed");
                            }
                        }
                        PKIX_DECREF(cert);
                        derCertItem = *attrVal++;
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

        PKIX_DECREF(cert);
        PKIX_DECREF(response);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_BuildCrlList
 * DESCRIPTION:
 *
 *  This function takes a List of LdapResponse objects pointed to by
 *  "responseList" and extracts and decodes the CRLs in those responses, storing
 *  the List of those CRLs at "pCrls".
 *
 * PARAMETERS:
 * "responseList"
 *      The address of the List of LdapResponses. Must be non-NULL.
 * "pCrls"
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
        PKIX_PL_LdapResponse *response = NULL;
        PKIX_List *crlList = NULL;
        LDAPMessage *message = NULL;
        LDAPSearchResponseEntry *sre = NULL;
        LDAPSearchResponseAttr **sreAttrArray = NULL;
        LDAPSearchResponseAttr *sreAttr = NULL;
        SECItem *attrType = NULL;
        SECItem **attrVal = NULL;

        PKIX_PL_CRL *crl = NULL;
        SECItem *derCrlItem = NULL;
        CERTSignedCrl *nssCrl = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_BuildCrlList");
        PKIX_NULLCHECK_TWO(responseList, pCrls);

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
                    attrVal = sreAttr->val;
                    derCrlItem = *attrVal++;
                    while (derCrlItem != 0) {
                        /* create a PKIX_PL_Crl from derCrl */
                        PKIX_PL_NSSCALLRV
                            (CERTSTORE, nssCrl, CERT_DecodeDERCrl,
                            (NULL, derCrlItem, SEC_CRL_TYPE));
                        if (nssCrl) {
                            PKIX_CHECK_ONLY_FATAL
                                (pkix_pl_CRL_CreateWithSignedCRL
                                (nssCrl, &crl, plContext),
                                "pkix_pl_CRL_CreateWithSignedCRL failed");

                            /* skip bad crls and append good ones */
                            if (!PKIX_ERROR_RECEIVED) {
                                if (crlList == NULL) {
                                        PKIX_CHECK(PKIX_List_Create
                                        (&crlList, plContext),
                                        "PKIX_List_Create failed");
                                }
                                PKIX_CHECK(PKIX_List_AppendItem
                                        (crlList,
                                        (PKIX_PL_Object *) crl,
                                        plContext),
                                        "PKIX_List_AppendItem failed");
                            }
                        }
                        PKIX_DECREF(crl);
                        derCrlItem = *attrVal++;
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

        PKIX_DECREF(crl);
        PKIX_DECREF(response);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_GetCertsBySubject
 * DESCRIPTION:
 *
 *  This function queries the CertStore embodied by LdapCertStoreContext "lcs"
 *  for Certificates whose subject matches the X500Name pointed to by
 *  "subjectName", using attributes depending on the minimum path length
 *  specified by "minPathLen", and stores the resulting List of Certificates at
 *  "pSelected".
 *
 *  If no matching Certificates are found an empty List is stored. If the query
 *  is still in progress (non-blocking I/O) a NULL pointer is stored.
 *
 * PARAMETERS:
 * "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 * "subjectName"
 *      The address of the X500Name that will be the subject of the desired
 *      Certificates. Must be non-NULL.
 * "minPathLen"
 *      The path length that will be used in determining whether to request
 *      user Certificates, CA Certificates, Cross-Pair Certificates,
 *      Certificate Revocation Lists, or Authority Revocation Lists.
 * "pSelected"
 *      The address at which the result is stored.
 * "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_GetCertsBySubject(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_PL_X500Name *subjectName,
        PKIX_Int32 minPathLen,
        PKIX_List **pSelected,
        void *plContext)
{
        SECItem nameItem = {siAsciiString, NULL, 0};
        LdapAttrIncludeMask attrBits = 0;
        PKIX_List *responseList = NULL;
        PKIX_List *certList = NULL;
        unsigned char *commonName = NULL;

        PKIX_ENTER
                (CERTSTORE, "pkix_pl_LdapCertStore_GetCertsBySubject");
        PKIX_NULLCHECK_THREE(lcs, subjectName, pSelected);

        PKIX_CHECK(pkix_pl_X500Name_GetCommonName
                (subjectName, &commonName, plContext),
                "pkix_pl_X500Name_GetCommonName failed");

        nameItem.data = commonName;
        nameItem.len = PORT_Strlen((const char *)commonName);

        if (minPathLen < 0) {

                attrBits |= LDAPATTR_INCLUDE_USERCERTS;

        }

        if (minPathLen > -2) {

                attrBits |= LDAPATTR_INCLUDE_CACERTS |
                        LDAPATTR_INCLUDE_CROSSPAIRCERTS |
                        LDAPATTR_INCLUDE_CERTREVLIST |
                        LDAPATTR_INCLUDE_AUTHREVLIST;

        }

#if 1
        attrBits = LDAPATTR_INCLUDE_CACERTS;
#endif
        PKIX_CHECK(pkix_pl_LdapCertStore_SendRequest
                (lcs, &nameItem, attrBits, &responseList, plContext),
                "pkix_pl_LdapCertStore_SendRequest failed");

        if (responseList) {
                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCertList
                        (responseList, &certList, plContext),
                        "pkix_pl_LdapCertStore_BuildCertList failed");
        }

        *pSelected = certList;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(certList);
        }

        if (commonName) {
                PKIX_PL_NSSCALL(CERTSTORE, PORT_Free, (commonName));
        }

        PKIX_DECREF(responseList);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_GetCRLsByIssuer
 * DESCRIPTION:
 *
 *  This function queries the CertStore embodied by LdapCertStoreContext "lcs"
 *  for Certificate Revocation Lists whose issuers match the X500Names pointed
 *  to by "issuerName", and stores the resulting List of CRLs at "pSelected".
 *
 * PARAMETERS:
 * "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 * "issuerNames"
 *      The address of a List of X500Names that are the issuers of the desired
 *      CRLs. Must be non-NULL.
 * "numNames"
 *      The number of elements in the "issuerNames" List. Must be non-zero.
 * "pSelected"
 *      The address at which the result is stored.
 * "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_GetCRLsByIssuer(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_List *issuerNames, /* List of X500Name */
        PKIX_UInt32 numNames,
        PKIX_List **pSelected,
        void *plContext)
{
        LdapAttrIncludeMask attrBits = 0;
        PKIX_UInt32 thisName = 0;
        SECItem nameItem = {siAsciiString, NULL, 0};
        PKIX_PL_X500Name *name = NULL;
        PKIX_List *responseList = NULL;
        PKIX_List *crlList = NULL;
        unsigned char *commonName = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCRLsByIssuer");
        PKIX_NULLCHECK_THREE(lcs, issuerNames, pSelected);

        if (numNames == 0) {
                PKIX_ERROR_FATAL("Zero argument");
        }

        for (thisName = 0; thisName < numNames; thisName++) {
                PKIX_CHECK(PKIX_List_GetItem
                        (issuerNames,
                        thisName,
                        (PKIX_PL_Object **)&name,
                        plContext),
                        "PKIX_List_GetItem failed");

                PKIX_CHECK(pkix_pl_X500Name_GetCommonName
                        (name, &commonName, plContext),
                        "pkix_pl_X500Name_GetCommonName failed");

                nameItem.data = commonName;
                nameItem.len = PORT_Strlen((const char *)commonName);

                /* add nameItem to request */

                PKIX_DECREF(name);
        }

        /* For now, we'll only request the last issuer name (from a list of 1) */

        attrBits = LDAPATTR_INCLUDE_CROSSPAIRCERTS |
                LDAPATTR_INCLUDE_CERTREVLIST | LDAPATTR_INCLUDE_AUTHREVLIST;

#if 1
        attrBits = LDAPATTR_INCLUDE_CERTREVLIST | LDAPATTR_INCLUDE_AUTHREVLIST;
#endif

        /* send request */
        PKIX_CHECK(pkix_pl_LdapCertStore_SendRequest
                (lcs, &nameItem, attrBits, &responseList, plContext),
                "pkix_pl_LdapCertStore_SendRequest failed");

        if (responseList) {
                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCrlList
                        (responseList, &crlList, plContext),
                        "pkix_pl_LdapCertStore_BuildCrlList failed");
        }

        *pSelected = crlList;

cleanup:

        /* XXX We'll have to do this for each nameItem in list */
        if (commonName) {
                PKIX_PL_NSSCALL(CERTSTORE, PORT_Free, (commonName));
        }

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(crlList);
        }

        PKIX_DECREF(name);
        PKIX_DECREF(responseList);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_CertQuery
 * DESCRIPTION:
 *
 *  This function iuses the CertStore embodied by the LdapCertStoreContext "lcs"
 *  to obtain from the database the Certs specified by the ComCertSelParams
 *  pointed to by "params" and stores the resulting List at "pSelected". If no
 *  matching Certs are found an empty List is stored. If the query is still in
 *  progress (non-blocking I/O) a NULL pointer is stored.
 *
 *  This function uses a "smart" database query if suitable criteria have been
 *  set in ComCertSelParams. Otherwise, this function returns a CertStore Error
 *  if the selector has not been provided with parameters that allow for a
 *  "smart" query. (Currently, only the Subject Name meets this requirement.)
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 * "params"
 *      Address of the ComCertSelParams. Must be non-NULL.
 * "pSelected"
 *      Address at which List will be stored. Must be non-NULL.
 * "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapCertStore_CertQuery(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_ComCertSelParams *params,
        PKIX_List **pSelected,
        void *plContext)
{
        PKIX_PL_X500Name *subjectName = NULL;
        PKIX_Int32 minPathLen = 0;
        PKIX_List *certList = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_CertQuery");
        PKIX_NULLCHECK_THREE(lcs, params, pSelected);

        /*
         * Any of the ComCertSelParams may be obtained and used to constrain
         * the database query, to allow the use of a "smart" query. See
         * pkix_certsel.h for a list of the PKIX_ComCertSelParams_Get*
         * calls available. No corresponding "smart" queries exist at present,
         * that we assume we can make an LDAP request based on Subject. When
         * others are added, corresponding code should be added to
         * pkix_pl_LdapCertStore_CertQuery to use them when appropriate
         * selector parameters have been set.
         */

        /*
         * If we have the subject name for the desired subject,
         * ask the database for Certs with that subject.
         */
        PKIX_CHECK(PKIX_ComCertSelParams_GetSubject
                (params, &subjectName, plContext),
                "PKIX_ComCertSelParams_GetSubject failed");

        PKIX_CHECK(PKIX_ComCertSelParams_GetBasicConstraints
                (params, &minPathLen, plContext),
                "PKIX_ComCertSelParams_GetBasicConstraints failed");

        if (subjectName) {

                PKIX_CHECK(pkix_pl_LdapCertStore_GetCertsBySubject
                        (lcs, subjectName, minPathLen, &certList, plContext),
                        "pkix_pl_LdapCertStore_GetCertsBySubject failed");

        /*
         * } else {
         *        Insert other "smart" queries here if available
         */

        } else {

                PKIX_ERROR("Insufficient criteria for Cert query");
        }

        *pSelected = certList;

cleanup:

        PKIX_DECREF(subjectName);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_CrlQuery
 * DESCRIPTION:
 *
 *  This function iuses the CertStore embodied by the LdapCertStoreContext "lcs"
 *  to obtain from the database the CRLs specified by the ComCRLSelParams
 *  pointed to by "params" and stores the List at "pSelected". If no Crls are
 *  found matching the criteria a NULL pointer is stored. If the query is still
 *  in progress (non-blocking I/O) a NULL pointer is stored.
 *
 *  This function uses a "smart" database query if suitable criteria have been
 *  set in ComCertSelParams. Otherwise, this function returns a CertStore Error
 *  if the selector has not been provided with parameters that allow for a
 *  "smart" query. (Currently, only the Issuer Name meets this requirement.)
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
 * "params"
 *      Address of the ComCRLSelParams. Must be non-NULL.
 * "pSelected"
 *      Address at which List will be stored. Must be non-NULL.
 * "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapCertStore_CrlQuery(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_ComCRLSelParams *params,
        PKIX_List **pSelected,
        void *plContext)
{
        PKIX_List *issuerNames = NULL;
        PKIX_List *crlList = NULL;
        PKIX_UInt32 numNames = 0;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_CrlQuery");
        PKIX_NULLCHECK_THREE(lcs, params, pSelected);

        /*
         * Any of the ComCRLSelParams may be obtained and used to constrain the
         * database query, to allow the use of a "smart" query. See
         * pkix_crlsel.h for a list of the PKIX_ComCRLSelParams_Get* calls
         * available. No corresponding "smart" queries exist at present, except
         * that we assume we can make an LDAP request based on the Issuer. When
         * others are added, corresponding code should be added to
         * pkix_pl_LdapCertStore_CrlQuery to use them when appropriate
         * selector parameters have been set.
         */

        /*
         * If we have constraints for a smart query, ask the server for CRLs
         * meeting those constraints.
         */
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

                        PKIX_CHECK(pkix_pl_LdapCertStore_GetCRLsByIssuer
                            (lcs, issuerNames, numNames, &crlList, plContext),
                            "pkix_pl_LdapCertStore_GetCRLsByIssuer failed");

                } else {
                        PKIX_ERROR("Impossible criterion for Crl Query");
                }

        /*
         * } else {
         *        Insert other "smart" queries here if available
         */

        } else {
                PKIX_ERROR("Insufficient criteria for Crl Query");
        }

        *pSelected = crlList;

cleanup:

        PKIX_DECREF(issuerNames);

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
        PKIX_List **pCertList,
        void *plContext)
{
        PKIX_UInt32 i = 0;
        PKIX_UInt32 numFound = 0;
        PKIX_Boolean pending = PKIX_FALSE;
        PKIX_PL_Cert *candidate = NULL;
        PKIX_List *selected = NULL;
        PKIX_List *filtered = NULL;
        PKIX_CertSelector_MatchCallback callback = NULL;
        PKIX_ComCertSelParams *params = NULL;
        PKIX_Error *errReturn = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCert");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store,
                (PKIX_PL_Object **)&lcs,
                plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        PKIX_CHECK(pkix_pl_LdapCertStore_IsIOPending(lcs, &pending, plContext),
                "pkix_pl_LdapCertStore_IsIOPending failed");

        if (pending == PKIX_TRUE) {

                PKIX_CHECK(pkix_pl_LdapCertstore_Dispatch(lcs, plContext),
                        "pkix_pl_LdapCertstore_Dispatch failed");

        } else {

                PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
                        (selector, &callback, plContext),
                        "PKIX_CertSelector_GetMatchCallback failed");

                PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                        (selector, &params, plContext),
                        "PKIX_CertSelector_GetComCertSelParams failed");

                PKIX_CHECK(pkix_pl_LdapCertStore_CertQuery
                        (lcs, params, &selected, plContext),
                        "pkix_pl_LdapCertStore_CertQuery failed");

        }

        if (selected) {
                PKIX_CHECK(PKIX_List_GetLength(selected, &numFound, plContext),
                        "PKIX_List_GetLength failed");

                PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                        "PKIX_List_Create failed");

                for (i = 0; i < numFound; i++) {
                        PKIX_CHECK(PKIX_List_GetItem
                                (selected,
                                i,
                                (PKIX_PL_Object **)&candidate,
                                plContext),
                                "PKIX_List_GetItem failed");

#if 0
                        errReturn = callback(selector, candidate, plContext);
                        if (!errReturn) {
#endif
                                PKIX_CHECK(PKIX_List_AppendItem
                                        (filtered,
                                        (PKIX_PL_Object *)candidate,
                                        plContext),
                                        "PKIX_List_AppendItem failed");
#if 0
                        }
                        PKIX_DECREF(errReturn);
#endif
                        PKIX_DECREF(candidate);
                }
        }

        *pCertList = filtered;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filtered);
        }
        PKIX_DECREF(candidate);
        PKIX_DECREF(selected);
        PKIX_DECREF(params);
        PKIX_DECREF(errReturn);
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
        PKIX_List **pCrlList,
        void *plContext)
{
        PKIX_UInt32 i = 0;
        PKIX_UInt32 numFound = 0;
        PKIX_Boolean pending = PKIX_FALSE;
        PKIX_PL_CRL *candidate = NULL;
        PKIX_List *selected = NULL;
        PKIX_List *filtered = NULL;
        PKIX_CRLSelector_MatchCallback callback = NULL;
        PKIX_ComCRLSelParams *params = NULL;
        PKIX_Error *errReturn = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCRL");
        PKIX_NULLCHECK_THREE(store, selector, pCrlList);

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store,
                (PKIX_PL_Object **)&lcs,
                plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        PKIX_CHECK(pkix_pl_LdapCertStore_IsIOPending(lcs, &pending, plContext),
                "pkix_pl_LdapCertStore_IsIOPending failed");

        if (pending == PKIX_TRUE) {

                PKIX_CHECK(pkix_pl_LdapCertstore_Dispatch(lcs, plContext),
                        "pkix_pl_LdapCertstore_Dispatch failed");

        } else {

                PKIX_CHECK(PKIX_CRLSelector_GetMatchCallback
                        (selector, &callback, plContext),
                        "PKIX_CRLSelector_GetMatchCallback failed");

                PKIX_CHECK(PKIX_CRLSelector_GetCommonCRLSelectorParams
                        (selector, &params, plContext),
                        "PKIX_CRLSelector_GetComCertSelParams failed");

                PKIX_CHECK(pkix_pl_LdapCertStore_CrlQuery
                        (lcs, params, &selected, plContext),
                        "pkix_pl_LdapCertStore_CrlQuery failed");

        }

        if (selected) {
                PKIX_CHECK(PKIX_List_GetLength(selected, &numFound, plContext),
                        "PKIX_List_GetLength failed");
        }

        PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                "PKIX_List_Create failed");

        for (i = 0; i < numFound; i++) {
                PKIX_CHECK(PKIX_List_GetItem
                        (selected,
                        i,
                        (PKIX_PL_Object **)&candidate,
                        plContext),
                        "PKIX_List_GetItem failed");

                errReturn = callback(selector, candidate, plContext);
                if (!errReturn) {
                        PKIX_CHECK(PKIX_List_AppendItem
                                (filtered,
                                (PKIX_PL_Object *)candidate,
                                plContext),
                                "PKIX_List_AppendItem failed");
                }
                PKIX_DECREF(candidate);
                PKIX_DECREF(errReturn);
        }

        *pCrlList = filtered;

cleanup:
        PKIX_DECREF(candidate);
        PKIX_DECREF(selected);
        PKIX_DECREF(params);
        PKIX_DECREF(errReturn);
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
        PRNetAddr *sockaddr,
        PRIntervalTime timeout,
        char *bindName,
        char *authentication,
        PRPollDesc **pDesc,
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        PKIX_CertStore *certStore = NULL;
        PKIX_PL_Socket *socket = NULL;
        PKIX_PL_LdapCertStoreContext *ldapCertStoreContext = NULL;
        PRErrorCode status = 0;
        PRFileDesc *fileDesc = NULL;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_LdapCertStore_Create");
        PKIX_NULLCHECK_THREE(bindName, authentication, pCertStore);

        PKIX_CHECK(pkix_pl_Socket_Create
                (PKIX_FALSE, timeout, sockaddr, &status, &socket, plContext),
                "pkix_pl_Socket_Create failed");

        PKIX_CHECK(pkix_pl_LdapCertStoreContext_Create
                (socket,
                bindName,
                authentication,
                &ldapCertStoreContext,
                plContext),
                "pkix_pl_LdapCertStoreContext_Create failed");

        PKIX_CHECK(PKIX_CertStore_Create
                (pkix_pl_LdapCertStore_GetCert,
                pkix_pl_LdapCertStore_GetCRL,
                (PKIX_PL_Object *)ldapCertStoreContext,
                &certStore,
                plContext),
                "PKIX_CertStore_Create failed");

        PKIX_CHECK(pkix_pl_Socket_GetPRFileDesc
                (socket, &fileDesc, plContext),
                "pkix_pl_Socket_GetPRFileDesc failed");

        ldapCertStoreContext->pollDesc.fd = fileDesc;
        ldapCertStoreContext->pollDesc.in_flags = 0;
        ldapCertStoreContext->pollDesc.out_flags = 0;

        /* Did Socket_Create say the connection was made? */
        if (status == 0) {
                ldapCertStoreContext->connectStatus = LDAP_CONNECTED;

                /* Assume server does not require a BIND. */ 
                ldapCertStoreContext->connectStatus = LDAP_BOUND;
        } else {
                ldapCertStoreContext->connectStatus = LDAP_CONNECT_PENDING;
        }

        *pDesc = &(ldapCertStoreContext->pollDesc);
        *pCertStore = certStore;

cleanup:

        PKIX_DECREF(socket);
        PKIX_DECREF(ldapCertStoreContext);

        PKIX_RETURN(CERTSTORE);
}
