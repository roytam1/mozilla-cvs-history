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

/* --Private-LdapCertStoreContext-Message-Building-Functions---------------- */

/*
 * FUNCTION: pkix_pl_LdapCertStore_MakeBind
 * DESCRIPTION:
 *
 *  This function creates and encodes a Bind message, using the arena pointed
 *  to by "arena", the version number contained in "versionData", the
 *  LDAPBindAPI pointed to by "bindAPI", and the messageID contained in
 *  "msgNum", and stores a pointer to the encoded string at "pBindMsg".
 *
 *  See pkix_pl_ldaptemplates.c for the ASN.1 description of a Bind message.
 *
 *  This code is not used if the CertStore was created with a NULL pointer
 *  supplied for the LDAPBindAPI structure. (Bind and Unbind do not seem to be
 *  expected for anonymous Search requests.)
 *
 * PARAMETERS:
 *  "arena"
 *      The address of the PRArenaPool used in encoding the message. Must be
 *       non-NULL.
 *  "versionData"
 *      The Int32 containing the version number to be encoded in the Bind
 *      message.
 *  "bindAPI"
 *      The address of the LDAPBindAPI to be encoded in the Bind message. Must
 *      be non-NULL.
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
        LDAPBindAPI *bindAPI,
        PKIX_UInt32 msgNum,
        SECItem **pBindMsg,
        void *plContext)
{
        LDAPMessage msg;
        char version = '\0';
        SECItem *encoded = NULL;
        PKIX_UInt32 len = 0;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_MakeBind");
        PKIX_NULLCHECK_TWO(arena, pBindMsg);

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

        /*
         * XXX At present we only know how to handle anonymous requests (no
         * authentication), and we are guessing how to do simple authentication.
         * This section will need to be revised and extended when other
         * authentication is needed.
         */
        if (bindAPI->selector == SIMPLE_AUTH) {
                msg.protocolOp.op.bindMsg.bindName.type = siAsciiString;
                msg.protocolOp.op.bindMsg.bindName.data =
                        (void *)bindAPI->chooser.simple.bindName;
                len = PL_strlen(bindAPI->chooser.simple.bindName);
                msg.protocolOp.op.bindMsg.bindName.len = len;

                msg.protocolOp.op.bindMsg.authentication.type = siAsciiString;
                msg.protocolOp.op.bindMsg.authentication.data =
                        (void *)bindAPI->chooser.simple.authentication;
                len = PL_strlen(bindAPI->chooser.simple.authentication);
                msg.protocolOp.op.bindMsg.authentication.len = len;
        }

        PKIX_PL_NSSCALLRV(LDAPCERTSTORECONTEXT, encoded, SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, PKIX_PL_LDAPMessageTemplate));
        if (!encoded) {
                PKIX_ERROR("failed in encoding bindRequest");
        }

        *pBindMsg = encoded;
cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_MakeUnbind
 * DESCRIPTION:
 *
 *  This function creates and encodes a Unbind message, using the arena pointed
 *  to by "arena" and the messageID contained in "msgNum", and stores a pointer
 *  to the encoded string at "pUnbindMsg".
 *
 *  See pkix_pl_ldaptemplates.c for the ASN.1 description of an Unbind message.
 *
 *  This code is not used if the CertStore was created with a NULL pointer
 *  supplied for the LDAPBindAPI structure. (Bind and Unbind do not seem to be
 *  expected for anonymous Search requests.)
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

        PKIX_PL_NSSCALLRV(LDAPCERTSTORECONTEXT, encoded, SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, PKIX_PL_LDAPMessageTemplate));
        if (!encoded) {
                PKIX_ERROR("failed in encoding unbind");
        }

        *pUnbindMsg = encoded;
cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_MakeAbandon
 * DESCRIPTION:
 *
 *  This function creates and encodes a Abandon message, using the arena pointed
 *  to by "arena" and the messageID contained in "msgNum", and stores a pointer
 *  to the encoded string at "pAbandonMsg".
 *
 *  See pkix_pl_ldaptemplates.c for the ASN.1 description of an Abandon message.
 *
 * PARAMETERS:
 *  "arena"
 *      The address of the PRArenaPool used in encoding the message. Must be
 *       non-NULL.
 *  "msgNum"
 *      The Int32 containing the MessageID to be encoded in the Abandon message.
 *  "pAbandonMsg"
 *      The address at which the encoded Abandon message will be stored. Must
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
pkix_pl_LdapCertStore_MakeAbandon(
        PRArenaPool *arena,
        PKIX_UInt32 msgNum,
        SECItem **pAbandonMsg,
        void *plContext)
{
        LDAPMessage msg;
        SECItem *encoded = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_MakeAbandon");
        PKIX_NULLCHECK_TWO(arena, pAbandonMsg);

        PKIX_PL_NSSCALL(LDAPCERTSTORECONTEXT, PORT_Memset,
                (&msg, 0, sizeof (LDAPMessage)));

        msg.messageID.type = siUnsignedInteger;
        msg.messageID.data = (void*)&msgNum;
        msg.messageID.len = sizeof (msgNum);

        msg.protocolOp.selector = LDAP_ABANDONREQUEST_TYPE;

        msg.protocolOp.op.abandonRequestMsg.messageID.type = siBuffer;
        msg.protocolOp.op.abandonRequestMsg.messageID.data = (void*)&msgNum;
        msg.protocolOp.op.abandonRequestMsg.messageID.len = sizeof (msgNum);

        PKIX_PL_NSSCALLRV(LDAPCERTSTORECONTEXT, encoded, SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, PKIX_PL_LDAPMessageTemplate));
        if (!encoded) {
                PKIX_ERROR("failed in encoding Abandon");
        }

        *pAbandonMsg = encoded;
cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_DecodeBindResponse
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

        PKIX_PL_NSSCALLRV(LDAPCERTSTORECONTEXT, rv, SEC_ASN1DecodeItem,
            (arena, &response, PKIX_PL_LDAPMessageTemplate, src));

        if (rv == SECSuccess) {
                *pBindResponse = response;
        }

        *pStatus = rv;

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_VerifyBindResponse
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
 * FUNCTION: pkix_pl_LdapCertStoreContext_RecvCheckComplete
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
pkix_pl_LdapCertStoreContext_RecvCheckComplete(
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
                "pkix_pl_LdapCertStoreContext_RecvCheckComplete");
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

                        if ((lcs->entriesFound == NULL) &&
                            ((resultCode == SUCCESS) ||
                            (resultCode == NOSUCHOBJECT))) {
                                PKIX_CHECK(PKIX_List_Create
                                        (&(lcs->entriesFound),
                                        plContext),
                                        "PKIX_List_Create failed");
                        } else if (resultCode == SUCCESS) {
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
 *  by "socket" and the LDAPBindAPI pointed to by "bindAPI", and stores the
 *  result at "pContext".
 *
 * PARAMETERS:
 *  "socket"
 *      The address of the Socket to be used in communication with the LDAP
 *      server. Must be non-NULL.
 *  "bindAPI"
 *      The address of the LDAPBindAPI containing the Bind information to be
 *      encoded in the Bind message.
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
        LDAPBindAPI *bindAPI,
        PKIX_PL_LdapCertStoreContext **pContext,
        void *plContext)
{
        PKIX_PL_HashTable *ht;
        PKIX_PL_LdapCertStoreContext *ldapCertStoreContext = NULL;
        PKIX_PL_Socket_Callback *callbackList;
        PRArenaPool *arena = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT,
                    "pkix_pl_LdapCertStoreContext_Create");
        PKIX_NULLCHECK_TWO(socket, pContext);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LDAPCERTSTORECONTEXT_TYPE,
                    sizeof (PKIX_PL_LdapCertStoreContext),
                    (PKIX_PL_Object **)&ldapCertStoreContext,
                    plContext),
                    "Could not create LdapCertStoreContext object");

        /* ldapCertStoreContext->connectStatus = undefined; */

        PKIX_CHECK(PKIX_PL_HashTable_Create
                (LDAP_CACHEBUCKETS, 0, &ht, plContext),
                "PKIX_PL_HashTable_Create failed");

        ldapCertStoreContext->cachePtr = ht;

        PKIX_CHECK(pkix_pl_Socket_GetCallbackList
                (socket, &callbackList, plContext),
                "pkix_pl_Socket_GetCallbackList failed");
        ldapCertStoreContext->callbackList = callbackList;

        PKIX_INCREF(socket);
        ldapCertStoreContext->clientSocket = socket;

        ldapCertStoreContext->messageID = 0;

        ldapCertStoreContext->bindAPI = bindAPI;

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
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_PL_Socket_Callback *callbackList = NULL;
        SECItem *encoded = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT,
                    "pkix_pl_LdapCertStoreContext_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_LDAPCERTSTORECONTEXT_TYPE, plContext),
                    "Object is not an LdapCertStoreContext");

        lcs = (PKIX_PL_LdapCertStoreContext *)object;

        switch (lcs->connectStatus) {
        case LDAP_CONNECT_PENDING:
                break;
        case LDAP_BIND_PENDING:
        case LDAP_BIND_RESPONSE:
        case LDAP_BIND_RESPONSE_PENDING:
        case LDAP_SEND_PENDING:
        case LDAP_RECV:
        case LDAP_RECV_PENDING:
        case LDAP_BOUND:
        case LDAP_ABANDON_PENDING:
                if (lcs->bindAPI != NULL) {
                        PKIX_CHECK(pkix_pl_LdapCertStore_MakeUnbind
                                (lcs->arena,
                                ++(lcs->messageID),
                                &encoded,
                                plContext),
                                "pkix_pl_LdapCertStore_MakeUnbind failed");

                        callbackList =
                                (PKIX_PL_Socket_Callback *)(lcs->callbackList);
                        PKIX_CHECK(callbackList->sendCallback
                                (lcs->clientSocket,
                                encoded->data,
                                encoded->len,
                                &bytesWritten,
                                plContext),
                                "pkix_pl_Socket_Send failed");
                }
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
 * FUNCTION: pkix_pl_LdapCertStoreContext_Hashcode
 * (see comments for PKIX_PL_HashcodeCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_pl_LdapCertStoreContext_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_LdapCertStoreContext *ldapCertStoreContext = NULL;
        PKIX_UInt32 tempHash = 0;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStoreContext_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_LDAPCERTSTORECONTEXT_TYPE, plContext),
                "Object is not an LdapCertStoreContext");

        ldapCertStoreContext = (PKIX_PL_LdapCertStoreContext *)object;

        PKIX_CHECK(PKIX_PL_Object_Hashcode
                ((PKIX_PL_Object *)ldapCertStoreContext->clientSocket,
                &tempHash,
                plContext),
                "PKIX_PL_Socket_Hashcode failed");

        if (ldapCertStoreContext->bindAPI != NULL) {
                tempHash = (tempHash << 7) +
                        ldapCertStoreContext->bindAPI->selector;
        }

        *pHashcode = tempHash;

cleanup:

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStoreContext_Equals
 * (see comments for PKIX_PL_EqualsCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_pl_LdapCertStoreContext_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Int32 *pResult,
        void *plContext)
{
        PKIX_PL_LdapCertStoreContext *firstLCSContext = NULL;
        PKIX_PL_LdapCertStoreContext *secondLCSContext = NULL;
        PKIX_Int32 compare = 0;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStoreContext_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        *pResult = PKIX_FALSE;

        PKIX_CHECK(pkix_CheckTypes
                (firstObject,
                secondObject,
                PKIX_LDAPCERTSTORECONTEXT_TYPE,
                plContext),
                "Object is not an LdapCertStoreContext");

        firstLCSContext = (PKIX_PL_LdapCertStoreContext *)firstObject;
        secondLCSContext = (PKIX_PL_LdapCertStoreContext *)secondObject;

        if (firstLCSContext == secondLCSContext) {
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        PKIX_CHECK(PKIX_PL_Object_Equals
                ((PKIX_PL_Object *)firstLCSContext->clientSocket,
                (PKIX_PL_Object *)secondLCSContext->clientSocket,
                &compare,
                plContext),
                "PKIX_PL_Socket_Equals failed");

        if (!compare) {
                goto cleanup;
        }

        if (PKIX_EXACTLY_ONE_NULL
                (firstLCSContext->bindAPI, secondLCSContext->bindAPI)) {
                goto cleanup;
        }

        if (firstLCSContext->bindAPI) {
                if (firstLCSContext->bindAPI->selector !=
                    secondLCSContext->bindAPI->selector) {
                        goto cleanup;
                }
        }

        *pResult = PKIX_TRUE;

cleanup:

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
        entry.equalsFunction = pkix_pl_LdapCertStoreContext_Equals;
        entry.hashcodeFunction = pkix_pl_LdapCertStoreContext_Hashcode;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_LDAPCERTSTORECONTEXT_TYPE] = entry;

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/*
 * FUNCTION: pkix_pl_LdapCertStoreContext_GetPollDesc
 * DESCRIPTION:
 *
 *  This function retrieves the PRPollDesc from the LdapCertStoreContext
 *  pointed to by "context" and stores the address at "pPollDesc".
 *
 * PARAMETERS:
 *  "context"
 *      The LdapCertStoreContext whose PRPollDesc is desired. Must be non-NULL.
 *  "pPollDesc"
 *      Address where PRPollDesc will be stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapCertStoreContext_GetPollDesc(
        PKIX_PL_LdapCertStoreContext *context,
        PRPollDesc **pPollDesc,
        void *plContext)
{
        PKIX_ENTER
                (LDAPCERTSTORECONTEXT,
                "pkix_pl_LdapCertStoreContext_GetPollDesc");
        PKIX_NULLCHECK_TWO(context, pPollDesc);

        *pPollDesc = &(context->pollDesc);

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

/* --Private-Ldap-CertStore-I/O-Functions---------------------------- */
/*
 * FUNCTION: pkix_pl_LdapCertStore_IsIOPending
 * DESCRIPTION:
 *
 *  This function determines whether the state of the connection of the
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

        if ((lcs->connectStatus == LDAP_SEND_PENDING) ||
            (lcs->connectStatus == LDAP_BIND_PENDING) ||
            (lcs->connectStatus == LDAP_BIND_RESPONSE_PENDING) ||
            (lcs->connectStatus == LDAP_SEND_PENDING) ||
            (lcs->connectStatus == LDAP_RECV_PENDING) ||
            (lcs->connectStatus == LDAP_ABANDON_PENDING)) {
                *pPending = PKIX_TRUE;
        } else {
                *pPending = PKIX_FALSE;
        }

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
}

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
                if (lcs->bindAPI != NULL) {
                        lcs->connectStatus = LDAP_CONNECTED;
                } else {
                        lcs->connectStatus = LDAP_BOUND;
                }
                keepGoing = PKIX_FALSE;
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
                        lcs->bindAPI,
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
 *  If a BindResponse is received with a Result code of 0 (success), we
 *  continue with the connection. If a non-zero Result code is received,
 *  we throw an Error. Some more sophisticated handling of that condition
 *  might be in order in the future.
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
                /*
                 * XXX What should we do if failure? At present if
                 * VerifyBindResponse throws an Error, we do too.
                 */
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
 *  This function receives an LDAP-protocol message for the CertStore embodied
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
        PKIX_UInt32 bytesToRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_Recv");
        PKIX_NULLCHECK_THREE(lcs, pKeepGoing, lcs->rcvBuf);

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        /*
         * If we attempt to fill our buffer with every read, we increase
         * the risk of an ugly situation: one or two bytes of a new message
         * left over at the end of processing one message. With such a
         * fragment, we can't decode a byte count and so won't know how much
         * space to allocate for the next LdapResponse. We try to avoid that
         * case by reading just enough to complete the current message, unless
         * there will be at least MINIMUM_MSG_LENGTH bytes left over.
         */
        if (lcs->currentResponse) {
                PKIX_CHECK(pkix_pl_LdapResponse_GetCapacity
                        (lcs->currentResponse, &bytesToRead, plContext),
                        "pkix_pl_LdapResponse_GetCapacity failed");
                if ((bytesToRead > lcs->capacity) ||
                    ((bytesToRead + MINIMUM_MSG_LENGTH) < lcs->capacity)) {
                        bytesToRead = lcs->capacity;
                }
        } else {
                bytesToRead = lcs->capacity;
        }

        lcs->currentBytesAvailable = 0;

        PKIX_CHECK(callbackList->recvCallback
                (lcs->clientSocket,
                (void *)lcs->rcvBuf,
                bytesToRead,
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
                lcs->currentBytesAvailable += bytesRead;
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
 * FUNCTION: pkix_pl_LdapCertStore_AbandonContinue
 * DESCRIPTION:
 *
 *  This function determines whether the abandon-message request of the
 *  LDAP-protocol message for the CertStore embodied in the LdapCertStoreContext
 *  "lcs" has completed, and stores in "pKeepGoing" a flag indicating whether
 *  processing can continue without further input.
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
pkix_pl_LdapCertStore_AbandonContinue(
        PKIX_PL_LdapCertStoreContext *lcs,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT, "pkix_pl_LdapCertStore_AbandonContinue");
        PKIX_NULLCHECK_TWO(lcs, pKeepGoing);

        callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (lcs->clientSocket, &bytesWritten, NULL, plContext),
                "pkix_pl_Socket_Poll failed");

        if (bytesWritten > 0) {
                lcs->connectStatus = LDAP_BOUND;
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
        unsigned char *to = NULL;
        unsigned char *from = NULL;
        PKIX_UInt32 dataIndex = 0;
        PKIX_UInt32 messageLength = 0;
        PKIX_UInt32 sizeofLength = 0;
        PKIX_UInt32 bytesProcessed = 0;
        unsigned char messageChar = 0;
        LDAPMessageType messageType = 0;
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

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

        /* Do we have enough of the message to decode the message length? */
        if (lcs->currentBytesAvailable < MINIMUM_MSG_LENGTH) {
                /*
                 * No! Move these few bytes to the beginning of rcvBuf
                 * and hang another read.
                 */
                to = (unsigned char *)lcs->rcvBuf;
                from = lcs->currentInPtr;
                for (dataIndex = 0;
                    dataIndex < lcs->currentBytesAvailable;
                    dataIndex++) {
                        *to++ = *from++;
                }
                callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);
                PKIX_CHECK(callbackList->recvCallback
                        (lcs->clientSocket,
                        (void *)to,
                        lcs->capacity - lcs->currentBytesAvailable,
                        &bytesRead,
                        plContext),
                        "pkix_pl_Socket_Recv failed");

                lcs->currentInPtr = lcs->rcvBuf;
                lcs->lastIO = PR_Now();

                if (bytesRead <= 0) {
                        lcs->connectStatus = LDAP_RECV_PENDING;
                        *pKeepGoing = PKIX_FALSE;
                        goto cleanup;
                } else {
                        lcs->currentBytesAvailable += bytesRead;
                }
        }

        /*
         * We have to determine whether the response is an entry, with
         * application-specific tag LDAP_SEARCHRESPONSEENTRY_TYPE, or a
         * resultCode, with application tag LDAP_SEARCHRESPONSERESULT_TYPE.
         * First, we have to figure out where to look for the tag.
         */

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

        PKIX_CHECK(pkix_pl_LdapCertStoreContext_RecvCheckComplete
                (lcs, bytesProcessed, pKeepGoing, plContext),
                "pkix_pl_LdapCertStoreContext_RecvCheckComplete failed");

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

        PKIX_CHECK(pkix_pl_LdapCertStoreContext_RecvCheckComplete
                (lcs, bytesProcessed, pKeepGoing, plContext),
                "pkix_pl_LdapCertStoreContext_RecvCheckComplete failed");

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
                case LDAP_ABANDON_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapCertStore_AbandonContinue
                                (lcs, &keepGoing, plContext),
                                "pkix_pl_LdapCertStore_AbandonContinue failed");
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
 *  This function tries to obtain a response for the LdapRequest contained in
 *  the LdapCertStoreContext pointed to by "lcs", and stores the response at
 *  "pResponse". If the LdapRequest was previously stored with a response in
 *  the certstore's cache, that response is retrieved immediately. Otherwise,
 *  the request is transmitted.
 *
 *  If non-blocking I/O is in use and the response is not yet available, NULL is
 *  stored at "pResponse". If the response has come back but contained no items
 *  satisfying the request, an empty List is stored at "pResponse".
 *
 * PARAMETERS:
 *  "lcs"
 *      The address of the LdapCertStoreContext object. Must be non-NULL.
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
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_List *searchResponseList = NULL;
        SECItem *encoded = NULL;

        PKIX_ENTER
                (LDAPCERTSTORECONTEXT,
                "pkix_pl_LdapCertStore_SendRequest");
        PKIX_NULLCHECK_TWO(lcs, pResponse);

        /* check hashtable for matching request */
        PKIX_CHECK(PKIX_PL_HashTable_Lookup
                (lcs->cachePtr,
                (PKIX_PL_Object *)(lcs->currentRequest),
                (PKIX_PL_Object **)&searchResponseList,
                plContext),
                "PKIX_PL_HashTable_Lookup failed");

        if (searchResponseList == NULL) {
                /* It wasn't cached. We'll have to actually send it. */

                PKIX_CHECK(pkix_pl_LdapRequest_GetEncoded
                        (lcs->currentRequest, &encoded, plContext),
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

        PKIX_RETURN(LDAPCERTSTORECONTEXT);
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
        PRArenaPool *arena,
        PKIX_List *certList,
        void *plContext)
{
        LDAPCertPair certPair = {{ siBuffer, NULL, 0 }, { siBuffer, NULL, 0 }};
        CERTCertificate *nssCert = NULL;
        PKIX_PL_Cert *cert = NULL;
        SECStatus rv = SECFailure;

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
 *  "arena"
 *      The address of a PRArenaPool that may be used in decoding the message. Must
 *      be  non-NULL.
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
        PRArenaPool *arena,
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
        PKIX_NULLCHECK_THREE(responseList, arena, pCerts);

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
                    } else if ((LDAPATTR_CROSSPAIRCERT & attrBits) == attrBits){
                        /* Is this attrVal a CrossPairCertificate? */
                        attrVal = sreAttr->val;
                        derCertItem = *attrVal++;
                        while (derCertItem != 0) {
                            /* create PKIX_PL_Certs from derCert */
                            PKIX_CHECK(pkix_pl_LdapCertStore_DecodeCrossCertPair
                                (derCertItem, arena, certList, plContext),
                                "pkix_pl_LdapCertStore_DecodeCrossCertPair"
                                " failed");
                            derCertItem = *attrVal++;
                        }
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
 * FUNCTION: pkix_pl_LdapCertStore_DestroyAndFilter
 * DESCRIPTION:
 *
 *  This function frees the space allocated for the components of the
 *  equalFilters that make up the andFilter pointed to by "filter".
 *
 * PARAMETERS:
 *  "andFilter"
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
pkix_pl_LdapCertStore_DestroyAndFilter(
        LDAPFilter *andFilter,
        void *plContext)
{
        LDAPFilter *currentFilter = NULL;
        LDAPFilter **setOfFilter = NULL;
        unsigned char* component = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_DestroyAndFilter");
        PKIX_NULLCHECK_ONE(andFilter);

        if (andFilter->selector != LDAP_ANDFILTER_TYPE) {
            PKIX_ERROR
                ("Invalid argument to pkix_pl_LdapCertStore_DestroyAndFilter");
        }

        /* Set currentFilter to point to first EqualFilter pointer */
        setOfFilter = andFilter->filter.andFilter.filters;

        currentFilter = *setOfFilter++;

        while (currentFilter != NULL) {
                component = 
                    currentFilter->filter.equalFilter.attrValue.data;
                if (component != NULL) {
                    PORT_Free(component);
                }
                currentFilter = *setOfFilter++;
        }

cleanup:

        PKIX_RETURN(CERTSTORE);

}

/*
 * FUNCTION: pkix_pl_LdapCertStore_DestroyOrFilter
 * DESCRIPTION:
 *
 *  This function frees the space allocated for the components of the
 *  andFilters that make up the orFilter pointed to by "filter".
 *
 * PARAMETERS:
 *  "orFilter"
 *      The address of the orFilter whose components are to be freed. Must be
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
pkix_pl_LdapCertStore_DestroyOrFilter(
        LDAPFilter *orFilter,
        void *plContext)
{
        LDAPFilter *currentFilter = NULL;
        LDAPFilter **setOfFilter = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_DestroyOrFilter");
        PKIX_NULLCHECK_ONE(orFilter);

        if (orFilter->selector != LDAP_ORFILTER_TYPE) {
            PKIX_ERROR
                ("Invalid argument to pkix_pl_LdapCertStore_DestroyOrFilter");
        }

        /* Set currentFilter to point to first AndFilter pointer */
        setOfFilter = orFilter->filter.orFilter.filters;

        currentFilter = *setOfFilter++;

        while (currentFilter != NULL) {
                PKIX_CHECK(pkix_pl_LdapCertStore_DestroyAndFilter
                        (currentFilter, plContext),
                        "pkix_pl_LdapCertStore_DestroyAndFilter failed");
                currentFilter = *setOfFilter++;
        }

cleanup:

        PKIX_RETURN(CERTSTORE);

}

/*
 * FUNCTION: pkix_pl_LdapCertStore_MakeAndFilter
 * DESCRIPTION:
 *
 *  This function allocates space from the arena pointed to by "arena" to
 *  construct a filter that will match components of the X500Name pointed to by
 *  "subjectName", and stores the resulting filter at "pFilter".
 *
 *  "subjectName" is checked for commonName and organizationName
 *  components (cn=, and o=) and the filter is the "and" of an
 *  equality match on each component found.
 *
 *  The component strings are extracted using the family of CERT_Get* functions,
 *  and each must be freed with PORT_Free.
 *
 *  It is not clear which components should be in a request, so, for now,
 *  we stop adding components after we have found one.
 *
 * PARAMETERS:
 *  "arena"
 *      The address of the PRArenaPool used in creating the filter. Must be
 *       non-NULL.
 *  "subjectName"
 *      The address of the X500Name whose components are the subject of the
 *      desired matches. Must be non-NULL.
 *  "pFilter"
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
pkix_pl_LdapCertStore_MakeAndFilter(
        PRArenaPool *arena,
        PKIX_PL_X500Name *subjectName, 
        LDAPFilter **pFilter,
        void *plContext)
{
        LDAPFilter **setOfFilter;
        LDAPFilter *andFilter = NULL;
        LDAPFilter *currentFilter = NULL;
        PKIX_UInt32 componentsPresent = 0;
        void *v = NULL;
        unsigned char *component = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_MakeAndFilter");
        PKIX_NULLCHECK_THREE(arena, subjectName, pFilter);

        /* Increase this if additional components may be extracted */
#define MAX_NUM_COMPONENTS 8

        /* Space for (MAX_NUM_COMPONENTS + 1) pointers to LDAPFilter */
        PKIX_PL_NSSCALLRV(CERTSTORE, v, PORT_ArenaZAlloc,
                (arena, (MAX_NUM_COMPONENTS + 1)*sizeof(LDAPFilter *)));
        setOfFilter = (LDAPFilter **)v;

        /* Space for AndFilter and MAX_NUM_COMPONENTS EqualFilters */
        PKIX_PL_NSSCALLRV(CERTSTORE, v, PORT_ArenaZNewArray,
                (arena, LDAPFilter, MAX_NUM_COMPONENTS + 1));
        setOfFilter[0] = (LDAPFilter *)v;

        /* Claim the first array element for the ANDFilter */
        andFilter = setOfFilter[0];

        /* Set ANDFilter to point to the first EqualFilter pointer */
        andFilter->selector = LDAP_ANDFILTER_TYPE;
        andFilter->filter.andFilter.filters = setOfFilter;

        currentFilter = andFilter + 1;

        /* Try for commonName */
        PKIX_CHECK(pkix_pl_X500Name_GetCommonName
                (subjectName, &component, plContext),
                "pkix_pl_X500Name_GetCommonName failed");
        if (component) {
                setOfFilter[componentsPresent] = currentFilter;
                currentFilter->selector = LDAP_EQUALFILTER_TYPE;
                currentFilter->filter.equalFilter.attrType.data =
                        (unsigned char *)"cn";
                currentFilter->filter.equalFilter.attrType.len = 2;
                currentFilter->filter.equalFilter.attrValue.data =
                        component;
                currentFilter->filter.equalFilter.attrValue.len =
                        PL_strlen((const char *)component);
                componentsPresent++;
                currentFilter++;

                goto enough;
        }

        /* Try for orgName */
        PKIX_CHECK(pkix_pl_X500Name_GetOrgName
                (subjectName, &component, plContext),
                "pkix_pl_X500Name_GetOrgName failed");
        if (component) {
                setOfFilter[componentsPresent] = currentFilter;
                currentFilter->selector = LDAP_EQUALFILTER_TYPE;
                currentFilter->filter.equalFilter.attrType.data =
                        (unsigned char *)"o";
                currentFilter->filter.equalFilter.attrType.len = 1;
                currentFilter->filter.equalFilter.attrValue.data =
                        component;
                currentFilter->filter.equalFilter.attrValue.len =
                        PL_strlen((const char *)component);
                componentsPresent++;
                currentFilter++;

                goto enough;
        }

        /* Try for countryName */
        PKIX_CHECK(pkix_pl_X500Name_GetCountryName
                (subjectName, &component, plContext),
                "pkix_pl_X500Name_GetCountryName failed");
        if (component) {
                setOfFilter[componentsPresent] = currentFilter;
                currentFilter->selector = LDAP_EQUALFILTER_TYPE;
                currentFilter->filter.equalFilter.attrType.data =
                        (unsigned char *)"c";
                currentFilter->filter.equalFilter.attrType.len = 1;
                currentFilter->filter.equalFilter.attrValue.data =
                        component;
                currentFilter->filter.equalFilter.attrValue.len =
                        PL_strlen((const char *)component);
                componentsPresent++;
                currentFilter++;

                goto enough;
        }

enough:

        setOfFilter[componentsPresent] = NULL;

        *pFilter = andFilter;

cleanup:

        PKIX_RETURN(CERTSTORE);

}

/*
 * FUNCTION: pkix_pl_LdapCertStore_MakeSubjectCertRequest
 * DESCRIPTION:
 *
 *  This function creates an LdapRequest for Certificates whose subject matches
 *  the X500Name pointed to by "subjectName", using the arena pointed to by
 *  "msgArena", the messageID in "msgnum", and attributes depending on the
 *  minimum path length specified by "minPathLen", and stores the resulting
 *  LdapRequest at "pRequest".
 *
 * PARAMETERS:
 *  "msgArena"
 *      The address of the arena to be used for encoding of the LdapRequest.
 *      Must be non-NULL.
 *  "msgnum"
 *      The UInt32 value of the messageID.
 *  "subjectName"
 *      The address of the X500Name that will be the subject of the desired
 *      Certificates. Must be non-NULL.
 *  "minPathLen"
 *      The path length that will be used in determining whether to request
 *      user Certificates, CA Certificates, Cross-Pair Certificates,
 *      Certificate Revocation Lists, or Authority Revocation Lists.
 *  "pRequest"
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
pkix_pl_LdapCertStore_MakeSubjectCertRequest(
        PRArenaPool *msgArena,
        PKIX_UInt32 msgnum,
        PKIX_PL_X500Name *subjectName,
        PKIX_Int32 minPathLen,
        PKIX_PL_LdapRequest **pRequest,
        void *plContext)
{
        LDAPFilter *andFilter = NULL;
        LdapAttrMask attrBits = 0;
        PKIX_PL_LdapRequest *request;
        PRArenaPool *filterArena = NULL;

        PKIX_ENTER
                (CERTSTORE, "pkix_pl_LdapCertStore_MakeSubjectCertRequest");
        PKIX_NULLCHECK_THREE(msgArena, subjectName, pRequest);

        /*
         * Get a short-lived arena. We'll be done with this space once
         * the request is encoded.
         */
        PKIX_PL_NSSCALLRV
                (LDAPCERTSTORECONTEXT,
                filterArena,
                PORT_NewArena,
                (DER_DEFAULT_CHUNKSIZE));

        if (!filterArena) {
                PKIX_ERROR_FATAL("Out of memory");
        }

        PKIX_CHECK(pkix_pl_LdapCertStore_MakeAndFilter
                (filterArena, subjectName, &andFilter, plContext),
                "pkix_pl_LdapCertStore_MakeAndFilter failed");

        if (minPathLen < 0) {

                attrBits |= LDAPATTR_USERCERT;

        }

        if (minPathLen > -2) {

                attrBits |= LDAPATTR_CACERT | LDAPATTR_CROSSPAIRCERT;

        }

        PKIX_CHECK(pkix_pl_LdapRequest_Create
                (msgArena,
                msgnum,
                "o=Test Certificates,c=US",                     /* baseObject           */
                (char)WHOLE_SUBTREE,    /* scope                */
                (char)NEVER_DEREF,      /* derefAliases         */
                0,                      /* sizeLimit            */
                0,                      /* timeLimit            */
                PKIX_FALSE,             /* attrsOnly            */
                andFilter,
                attrBits,               /* attributes           */
                &request,
                plContext),

                "pkix_pl_LdapRequest_Create failed");

        PKIX_CHECK(pkix_pl_LdapCertStore_DestroyAndFilter(andFilter, plContext),
                "pkix_pl_LdapCertStore_DestroyAndFilter failed");

        *pRequest = request;

cleanup:
        if (filterArena) {
                PKIX_PL_NSSCALL(CERTSTORE, PORT_FreeArena, (filterArena, PR_FALSE));
        }

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_MakeIssuerCRLRequest
 * DESCRIPTION:
 *
 *  This function creates an LdapRequest to request Certificate Revocation Lists
 *  whose issuers match the X500Names pointed to by "issuerName", using the
 *  arena pointed to by "msgArena" and the messageID in "msgnum", and stores the
 *  resulting LdapRequest at "pRequest".
 *
 * PARAMETERS:
 *  "msgArena"
 *      The address of the arena to be used for encoding of the LdapRequest.
 *      Must be non-NULL.
 *  "msgnum"
 *      The UInt32 value of the messageID.
 *  "issuerNames"
 *      The address of a List of X500Names that are the issuers of the desired
 *      CRLs. Must be non-NULL.
 *  "numNames"
 *      The number of elements in the "issuerNames" List. Must be non-zero.
 *  "pRequest"
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
pkix_pl_LdapCertStore_MakeIssuerCRLRequest(
        PRArenaPool *msgArena,
        PKIX_UInt32 msgnum,
        PKIX_List *issuerNames, /* List of X500Name */
        PKIX_UInt32 numNames,
        PKIX_PL_LdapRequest **pRequest,
        void *plContext)
{
        LdapAttrMask attrBits = 0;
        PKIX_UInt32 thisName = 0;
        PKIX_PL_X500Name *name = NULL;
        PKIX_PL_LdapRequest *request;
        PRArenaPool *filterArena = NULL;
        LDAPFilter **setOfFilter = NULL;
        LDAPFilter orFilter;
        void *v = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_MakeIssuerCRLRequest");
        PKIX_NULLCHECK_THREE(msgArena, issuerNames, pRequest);

        if (numNames == 0) {
                PKIX_ERROR_FATAL("Zero argument");
        }

        /*
         * Get a short-lived arena. We'll be done with this space once
         * the request is encoded.
         */
        PKIX_PL_NSSCALLRV
                (LDAPCERTSTORECONTEXT,
                filterArena,
                PORT_NewArena,
                (DER_DEFAULT_CHUNKSIZE));

        if (!filterArena) {
                PKIX_ERROR_FATAL("Out of memory");
        }

        /* Space for (numNames + 1) pointers to LDAPFilter */
        PKIX_PL_NSSCALLRV(CERTSTORE, v, PORT_ArenaZAlloc,
                (filterArena, (numNames + 1)*sizeof(LDAPFilter *)));
        setOfFilter = (LDAPFilter **)v;
        setOfFilter[0] = (LDAPFilter *)v;

        orFilter.selector = LDAP_ORFILTER_TYPE;
        orFilter.filter.orFilter.filters = setOfFilter;

        for (thisName = 0; thisName < numNames; thisName++) {
                PKIX_CHECK(PKIX_List_GetItem
                        (issuerNames,
                        thisName,
                        (PKIX_PL_Object **)&name,
                        plContext),
                        "PKIX_List_GetItem failed");

                PKIX_CHECK(pkix_pl_LdapCertStore_MakeAndFilter
                        (filterArena, name, &setOfFilter[thisName], plContext),
                        "pkix_pl_LdapCertStore_MakeAndFilter failed");

                PKIX_DECREF(name);
        }

        setOfFilter[numNames] = NULL;

        attrBits = LDAPATTR_CERTREVLIST | LDAPATTR_AUTHREVLIST;

        PKIX_CHECK(pkix_pl_LdapRequest_Create
                (msgArena,
                msgnum,
                "c=US",                 /* baseObject           */
                (char)WHOLE_SUBTREE,    /* scope                */
                (char)NEVER_DEREF,      /* derefAliases         */
                0,                      /* sizeLimit            */
                0,                      /* timeLimit            */
                PKIX_FALSE,             /* attrsOnly            */
                &orFilter,
                attrBits,               /* attributes           */
                &request,
                plContext),
                "pkix_pl_LdapRequest_Create failed");

        PKIX_CHECK(pkix_pl_LdapCertStore_DestroyOrFilter(&orFilter, plContext),
                "pkix_pl_LdapCertStore_DestroyOrFilter failed");

        *pRequest = request;

cleanup:

        if (filterArena) {
                PKIX_PL_NSSCALL
                        (CERTSTORE, PORT_FreeArena, (filterArena, PR_FALSE));
        }

        PKIX_DECREF(name);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_MakeCertRequest
 * DESCRIPTION:
 *
 *  This function creates an LdapRequest to obtain from the server the Certs
 *  specified by the ComCertSelParams pointed to by "params", using the arena
 *  pointed to by "msgArena" and the UInt32 messageID in "msgnum", and stores
 *  the resulting request at "pRequest".
 *
 *  This function creates a "smart" database query if suitable criteria have
 *  been set in ComCertSelParams. If the selector has not been provided with
 *  parameters that allow for a "smart" query, this function returns a CertStore
 *  Error. (Currently, only the Subject Name meets this requirement.)
 *
 * PARAMETERS:
 *  "msgArena"
 *      The address of the arena to be used for encoding of the LdapRequest.
 *      Must be non-NULL.
 *  "msgnum"
 *      The UInt32 value of the messageID.
 *  "params"
 *      Address of the ComCertSelParams. Must be non-NULL.
 *  "pRequest"
 *      Address at which LdapRequest will be stored. Must be non-NULL.
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
pkix_pl_LdapCertStore_MakeCertRequest(
        PRArenaPool *msgArena,
        PKIX_UInt32 msgnum,
        PKIX_ComCertSelParams *params,
        PKIX_PL_LdapRequest **pRequest,
        void *plContext)
{
        PKIX_PL_X500Name *subjectName = NULL;
        PKIX_Int32 minPathLen = 0;
        PKIX_PL_LdapRequest *request;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_MakeCertRequest");
        PKIX_NULLCHECK_THREE(msgArena, params, pRequest);

        /*
         * Any of the ComCertSelParams may be obtained and used to constrain
         * the database query, to allow the use of a "smart" query. See
         * pkix_certsel.h for a list of the PKIX_ComCertSelParams_Get*
         * calls available. No corresponding "smart" queries exist at present,
         * except that we assume we can make an LDAP request based on Subject.
         * When others are added, corresponding code should be added to
         * pkix_pl_LdapCertStore_MakeCertRequest to use them when appropriate
         * selector parameters have been set.
         */

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

                PKIX_CHECK(pkix_pl_LdapCertStore_MakeSubjectCertRequest
                        (msgArena,
                        msgnum,
                        subjectName,
                        minPathLen,
                        &request,
                        plContext),
                        "pkix_pl_LdapCertStore_MakeSubjectCertRequest failed");

        /*
         * } else {
         *        Insert other "smart" queries here if available
         */

        } else {

                PKIX_ERROR("Insufficient criteria for Cert query");
        }

        *pRequest = request;

cleanup:

        PKIX_DECREF(subjectName);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_LdapCertStore_MakeCrlRequest
 * DESCRIPTION:
 *
 *  This function creates an LdapRequest to obtain from the server the CRLs
 *  specified by the ComCRLSelParams pointed to by "params", using the arena
 *  pointed to by msgArena and the messageID in "msgnum", and stores the
 *  LdapRequest at "pRequest".
 *
 *  This function uses a "smart" database query if suitable criteria have been
 *  set in ComCertSelParams. If the selector has not been provided with
 *  parameters that allow for a "smart" query, this function returns a CertStore
 *  Error. (Currently, only the Issuer Name meets this requirement.)
 *
 * PARAMETERS:
 *  "msgArena"
 *      The address of the arena to be used for encoding of the LdapRequest.
 *      Must be non-NULL.
 *  "msgnum"
 *      The UInt32 value of the messageID.
 *  "params"
 *      Address of the ComCRLSelParams. Must be non-NULL.
 *  "pSelected"
 *      Address at which List will be stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapCertStore_MakeCrlRequest(
        PRArenaPool *msgArena,
        PKIX_UInt32 msgnum,
        PKIX_ComCRLSelParams *params,
        PKIX_PL_LdapRequest **pRequest,
        void *plContext)
{
        PKIX_List *issuerNames = NULL;
        PKIX_UInt32 numNames = 0;
        PKIX_PL_LdapRequest *request = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_MakeCrlRequest");
        PKIX_NULLCHECK_THREE(msgArena, params, pRequest);

        /*
         * Any of the ComCRLSelParams may be obtained and used to constrain the
         * database query, to allow the use of a "smart" query. See
         * pkix_crlsel.h for a list of the PKIX_ComCRLSelParams_Get* calls
         * available. No corresponding "smart" queries exist at present, except
         * that we assume we can make an LDAP request based on the Issuer. When
         * others are added, corresponding code should be added to
         * pkix_pl_LdapCertStore_MakeCrlRequest to use them when appropriate
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
                    PKIX_CHECK(pkix_pl_LdapCertStore_MakeIssuerCRLRequest
                        (msgArena,
                        msgnum,
                        issuerNames,
                        numNames,
                        &request,
                        plContext),
                        "pkix_pl_LdapCertStore_MakeIssuerCRLRequest failed");

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

        *pRequest = request;

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
        PKIX_Boolean match = PKIX_FALSE;
        PKIX_PL_Cert *candidate = NULL;
        PKIX_List *responses = NULL;
        PKIX_List *certList = NULL;
        PKIX_List *filtered = NULL;
        PKIX_CertSelector_MatchCallback callback = NULL;
        PKIX_ComCertSelParams *params = NULL;
        PKIX_PL_LdapRequest *request = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_Boolean cacheFlag = PKIX_FALSE;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCert");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        /*
         * If we're still waiting for a connection, we can do
         * nothing until the connection completes.
         */
        if (lcs->connectStatus == LDAP_CONNECT_PENDING) {

                PKIX_CHECK(pkix_pl_LdapCertstore_Dispatch(lcs, plContext),
                        "pkix_pl_LdapCertstore_Dispatch failed");

                if (lcs->connectStatus == LDAP_CONNECT_PENDING) {
                        *pCertList = NULL;
                        goto cleanup;
                }
        }

        /*
         * If we're waiting for an I/O completion, check whether we have
         * a complete message received.
         */
        PKIX_CHECK(pkix_pl_LdapCertStore_IsIOPending(lcs, &pending, plContext),
                "pkix_pl_LdapCertStore_IsIOPending failed");

        if (pending == PKIX_TRUE) {

                PKIX_CHECK(pkix_pl_LdapCertstore_Dispatch(lcs, plContext),
                        "pkix_pl_LdapCertstore_Dispatch failed");

                /*
                 * Another buffer completed does not mean we're done.
                 * We're done only when I/O is no longer pending.
                 */
                PKIX_CHECK(pkix_pl_LdapCertStore_IsIOPending
                        (lcs, &pending, plContext),
                        "pkix_pl_LdapCertStore_IsIOPending failed");

                if ((pending == PKIX_FALSE) && (lcs->entriesFound)) {
                        responses = lcs->entriesFound;
                        PKIX_INCREF(responses);
                        PKIX_DECREF(lcs->entriesFound);
                        PKIX_DECREF(lcs->currentRequest);

                        PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
                                (selector, &callback, plContext),
                                "PKIX_CertSelector_GetMatchCallback failed");
                }

        } else {

                PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
                        (selector, &callback, plContext),
                        "PKIX_CertSelector_GetMatchCallback failed");

                PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                        (selector, &params, plContext),
                        "PKIX_CertSelector_GetComCertSelParams failed");

                PKIX_CHECK(pkix_pl_LdapCertStore_MakeCertRequest
                        (lcs->arena,
                        ++(lcs->messageID),
                        params,
                        &request,
                        plContext),
                        "pkix_pl_LdapCertStore_MakeCertRequest failed");

                lcs->currentRequest = request;

                PKIX_CHECK(pkix_pl_LdapCertStore_SendRequest
                        (lcs, &responses, plContext),
                        "pkix_pl_LdapCertStore_SendRequest failed");
        }

        if (responses) {
                PKIX_DECREF(lcs->currentRequest);

                PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                        "PKIX_List_Create failed");

                /*
                 * We have a List of LdapResponse objects that still have to be
                 * turned into Certs.
                 */
                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCertList
                        (responses, lcs->arena, &certList, plContext),
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

        *pCertList = filtered;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filtered);
        }
        PKIX_DECREF(candidate);
        PKIX_DECREF(certList);
        PKIX_DECREF(responses);
        PKIX_DECREF(params);
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
        PKIX_List *responses = NULL;
        PKIX_List *crlList = NULL;
        PKIX_List *filtered = NULL;
        PKIX_CRLSelector_MatchCallback callback = NULL;
        PKIX_ComCRLSelParams *params = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_PL_LdapRequest *request = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCRL");
        PKIX_NULLCHECK_THREE(store, selector, pCrlList);

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        if (lcs->connectStatus == LDAP_CONNECT_PENDING) {

                PKIX_CHECK(pkix_pl_LdapCertstore_Dispatch(lcs, plContext),
                        "pkix_pl_LdapCertstore_Dispatch failed");

                if (lcs->connectStatus == LDAP_CONNECT_PENDING) {
                        *pCrlList = NULL;
                        goto cleanup;
                }
        }

        PKIX_CHECK(pkix_pl_LdapCertStore_IsIOPending(lcs, &pending, plContext),
                "pkix_pl_LdapCertStore_IsIOPending failed");

        if (pending == PKIX_TRUE) {

                PKIX_CHECK(pkix_pl_LdapCertstore_Dispatch(lcs, plContext),
                        "pkix_pl_LdapCertstore_Dispatch failed");

                /*
                 * Another buffer completed does not mean we're done.
                 * We're done only when I/O is no longer pending.
                 */
                PKIX_CHECK(pkix_pl_LdapCertStore_IsIOPending
                        (lcs, &pending, plContext),
                        "pkix_pl_LdapCertStore_IsIOPending failed");

                if ((pending == PKIX_FALSE) && (lcs->entriesFound)) {
                        responses = lcs->entriesFound;
                        PKIX_INCREF(responses);
                        PKIX_DECREF(lcs->entriesFound);
                        PKIX_DECREF(lcs->currentRequest);

                        PKIX_CHECK(PKIX_CRLSelector_GetMatchCallback
                                (selector, &callback, plContext),
                                "PKIX_CRLSelector_GetMatchCallback failed");
                }
        } else {

                PKIX_CHECK(PKIX_CRLSelector_GetMatchCallback
                        (selector, &callback, plContext),
                        "PKIX_CRLSelector_GetMatchCallback failed");

                PKIX_CHECK(PKIX_CRLSelector_GetCommonCRLSelectorParams
                        (selector, &params, plContext),
                        "PKIX_CRLSelector_GetComCertSelParams failed");

                PKIX_CHECK(pkix_pl_LdapCertStore_MakeCrlRequest
                        (lcs->arena,
                        ++(lcs->messageID),
                        params,
                        &request,
                        plContext),
                        "pkix_pl_LdapCertStore_MakeCrlRequest failed");

                lcs->currentRequest = request;

                PKIX_CHECK(pkix_pl_LdapCertStore_SendRequest
                        (lcs, &responses, plContext),
                        "pkix_pl_LdapCertStore_SendRequest failed");
        }

        if (responses) {
                PKIX_DECREF(lcs->currentRequest);

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
        PKIX_DECREF(params);
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
        LDAPBindAPI *bindAPI,
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        PKIX_CertStore *certStore = NULL;
        PKIX_PL_Socket *socket = NULL;
        PKIX_PL_LdapCertStoreContext *ldapCertStoreContext = NULL;
        PRErrorCode status = 0;
        PRFileDesc *fileDesc = NULL;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_LdapCertStore_Create");
        PKIX_NULLCHECK_ONE(pCertStore);

        PKIX_CHECK(pkix_pl_Socket_Create
                (PKIX_FALSE, timeout, sockaddr, &status, &socket, plContext),
                "pkix_pl_Socket_Create failed");

        PKIX_CHECK(pkix_pl_LdapCertStoreContext_Create
                (socket,
                bindAPI,
                &ldapCertStoreContext,
                plContext),
                "pkix_pl_LdapCertStoreContext_Create failed");

        PKIX_CHECK(pkix_pl_Socket_GetPRFileDesc
                (socket, &fileDesc, plContext),
                "pkix_pl_Socket_GetPRFileDesc failed");

        ldapCertStoreContext->pollDesc.fd = fileDesc;
        ldapCertStoreContext->pollDesc.in_flags = 0;
        ldapCertStoreContext->pollDesc.out_flags = 0;

        PKIX_CHECK(PKIX_CertStore_Create
                (pkix_pl_LdapCertStore_GetCert,
                pkix_pl_LdapCertStore_GetCRL,
                (PKIX_PL_Object *)ldapCertStoreContext,
                PKIX_TRUE, /* cache flag */
                NULL, /* don't support trust */
                (timeout == 0)?PKIX_TRUE:PKIX_FALSE, /* supports NBIO */
                PKIX_FALSE, /* not local */
                &certStore,
                plContext),
                "PKIX_CertStore_Create failed");

        /* Did Socket_Create say the connection was made? */
        if (status == 0) {
                if (ldapCertStoreContext->bindAPI != NULL) {
                        ldapCertStoreContext->connectStatus = LDAP_CONNECTED;
                } else {
                        ldapCertStoreContext->connectStatus = LDAP_BOUND;
                }
        } else {
                ldapCertStoreContext->connectStatus = LDAP_CONNECT_PENDING;
        }

        *pCertStore = certStore;

cleanup:

        PKIX_DECREF(socket);
        PKIX_DECREF(ldapCertStoreContext);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: PKIX_PL_LdapCertStore_AbandonRequest
 */
PKIX_Error *
PKIX_PL_LdapCertStore_AbandonRequest(
        PKIX_CertStore *store,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_PL_Socket_Callback *callbackList = NULL;
        SECItem *encoded = NULL;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_LdapCertStore_AbandonRequest");
        PKIX_NULLCHECK_ONE(store);

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                "PKIX_CertStore_GetCertStoreContext failed");

        if (lcs->connectStatus == LDAP_RECV_PENDING) {
                PKIX_CHECK(pkix_pl_LdapCertStore_MakeAbandon
                        (lcs->arena, (lcs->messageID) - 1, &encoded, plContext),
                        "pkix_pl_LdapCertStore_MakeAbandon failed");

                callbackList = (PKIX_PL_Socket_Callback *)(lcs->callbackList);
                PKIX_CHECK(callbackList->sendCallback
                        (lcs->clientSocket,
                        encoded->data,
                        encoded->len,
                        &bytesWritten,
                        plContext),
                        "pkix_pl_Socket_Send failed");

                if (bytesWritten < 0) {
                        lcs->connectStatus = LDAP_ABANDON_PENDING;
                } else {
                        lcs->connectStatus = LDAP_BOUND;
                }
        }

        PKIX_DECREF(lcs->entriesFound);
        PKIX_DECREF(lcs->currentRequest);
        PKIX_DECREF(lcs->currentResponse);

cleanup:

        PKIX_DECREF(lcs);

        PKIX_RETURN(CERTSTORE);
}
