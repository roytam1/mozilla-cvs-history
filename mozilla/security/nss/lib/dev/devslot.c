/* 
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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

#ifndef NSSCKEPV_H
#include "nssckepv.h"
#endif /* NSSCKEPV_H */

#ifndef DEVM_H
#include "devm.h"
#endif /* DEVM_H */

#ifndef CKHELPER_H
#include "ckhelper.h"
#endif /* CKHELPER_H */

/* measured in seconds */
#define NSSSLOT_TOKEN_DELAY_TIME 1

/* this should track global and per-transaction login information */

typedef enum {
  nssSlotAskPasswordTimes_FirstTime = 0,
  nssSlotAskPasswordTimes_EveryTime = 1,
  nssSlotAskPasswordTimes_Timeout = 2
} 
nssSlotAskPasswordTimes;

struct nssSlotAuthInfoStr
{
  PRTime lastLogin;
  nssSlotAskPasswordTimes askTimes;
  PRIntervalTime askPasswordTimeout;
};

/* XXX slot needs to maintain default r/o and/or rw session */
struct NSSSlotStr
{
  struct nssDeviceBaseStr base;
  NSSModule *module; /* Parent */
  NSSToken *token;  /* Peer */
  CK_SLOT_ID slotID;
  CK_FLAGS ckFlags; /* from CK_SLOT_INFO.flags */
  struct nssSlotAuthInfoStr authInfo;
  PRIntervalTime lastTokenPing;
  PRBool sessionLimit; /* can open limited # of sessions */
};

#define NSSSLOT_IS_FRIENDLY(slot) \
  (slot->base.flags & NSSSLOT_FLAGS_FRIENDLY)

/* measured as interval */
static PRIntervalTime s_token_delay_time = 0;

/* The flags needed to open a read-only session. */
static const CK_FLAGS s_ck_readonly_flags = CKF_SERIAL_SESSION;

/* In pk11slot.c, this was a no-op.  So it is here also. */
static CK_RV PR_CALLBACK
nss_ck_slot_notify (
  CK_SESSION_HANDLE session,
  CK_NOTIFICATION event,
  CK_VOID_PTR pData
)
{
    return CKR_OK;
}

NSS_IMPLEMENT NSSSlot *
nssSlot_Create (
  CK_SLOT_ID slotID,
  NSSModule *parent
)
{
    NSSArena *arena = NULL;
    NSSSlot *rvSlot;
    NSSToken *token = NULL;
    NSSUTF8 *slotName = NULL;
    PRUint32 length;
    CK_SLOT_INFO slotInfo;
    CK_RV ckrv;
    void *epv;
    arena = nssArena_Create();
    if(!arena) {
	return (NSSSlot *)NULL;
    }
    rvSlot = nss_ZNEW(arena, NSSSlot);
    if (!rvSlot) {
	goto loser;
    }
    /* Get slot information */
    epv = nssModule_GetCryptokiEPV(parent);
    ckrv = CKAPI(epv)->C_GetSlotInfo(slotID, &slotInfo);
    if (ckrv != CKR_OK) {
	/* set an error here, eh? */
	goto loser;
    }
    /* Grab the slot description from the PKCS#11 fixed-length buffer */
    length = nssPKCS11String_Length(slotInfo.slotDescription, 
                                    sizeof(slotInfo.slotDescription));
    if (length > 0) {
	slotName = nssUTF8_Create(arena, nssStringType_UTF8String, 
	                          (void *)slotInfo.slotDescription, length);
	if (!slotName) {
	    goto loser;
	}
    }
    rvSlot->base.arena = arena;
    rvSlot->base.refCount = 1;
    rvSlot->base.name = slotName;
    rvSlot->base.lock = PZ_NewLock(nssNSSILockOther); /* XXX */
    if (!rvSlot->base.lock) {
	goto loser;
    }
    rvSlot->module = parent; /* refs go from module to slots */
    rvSlot->slotID = slotID;
    rvSlot->ckFlags = slotInfo.flags;
    /* Initialize the token if present. */
    if (slotInfo.flags & CKF_TOKEN_PRESENT) {
	token = nssToken_Create(slotID, rvSlot);
	if (!token) {
	    goto loser;
	}
	rvSlot->sessionLimit = nssToken_HasSessionLimit(token);
    }
    rvSlot->token = token;
    return rvSlot;
loser:
    nssArena_Destroy(arena);
    /* everything was created in the arena, nothing to see here, move along */
    return (NSSSlot *)NULL;
}

NSS_IMPLEMENT PRStatus
nssSlot_Destroy (
  NSSSlot *slot
)
{
    void *epv;
    if (slot) {
	PR_AtomicDecrement(&slot->base.refCount);
	if (slot->base.refCount == 0) {
	    nssToken_Destroy(slot->token);
	    epv = nssModule_GetCryptokiEPV(slot->module);
	    if (epv) {
		(void)CKAPI(epv)->C_CloseAllSessions(slot->slotID);
	    }
	    nssModule_DestroyFromSlot(slot->module, slot);
	    PZ_DestroyLock(slot->base.lock);
	    return nssArena_Destroy(slot->base.arena);
	}
    }
    return PR_SUCCESS;
}

NSS_IMPLEMENT void
NSSSlot_Destroy (
  NSSSlot *slot
)
{
    (void)nssSlot_Destroy(slot);
}

NSS_IMPLEMENT NSSSlot *
nssSlot_AddRef (
  NSSSlot *slot
)
{
    PR_AtomicIncrement(&slot->base.refCount);
    return slot;
}

NSS_IMPLEMENT PRStatus
NSSSlot_GetInfo (
  NSSSlot *slot,
  NSSSlotInfo *slotInfo
)
{
    slotInfo->name = slot->base.name;
#if 0
    slotInfo->description = slot->description;
    slotInfo->manufacturerID = slot->manufacturerID;
    slotInfo->hardwareVersion.major = slot->hwVersion.major;
    slotInfo->hardwareVersion.minor = slot->hwVersion.minor;
    slotInfo->firmwareVersion.major = slot->fwVersion.major;
    slotInfo->firmwareVersion.minor = slot->fwVersion.minor;
#endif
    slotInfo->moduleName = nssModule_GetName(slot->module);
    if (slot->token) {
	slotInfo->tokenName = nssToken_GetName(slot->token);
	slotInfo->isTokenPresent = nssSlot_IsTokenPresent(slot);
    } else {
	slotInfo->isTokenPresent = PR_FALSE;
    }
    slotInfo->isTokenRemovable = slot->ckFlags & CKF_REMOVABLE_DEVICE;
    slotInfo->isHardware = slot->ckFlags & CKF_HW_SLOT;
    return PR_SUCCESS;
}

NSS_IMPLEMENT NSSUTF8 *
nssSlot_GetName (
  NSSSlot *slot
)
{
    return slot->base.name;
}

NSS_IMPLEMENT NSSUTF8 *
nssSlot_GetTokenName (
  NSSSlot *slot
)
{
    return nssToken_GetName(slot->token);
}

static PRBool
within_token_delay_period(NSSSlot *slot)
{
    PRIntervalTime time, lastTime;
    /* Set the delay time for checking the token presence */
    if (s_token_delay_time == 0) {
	s_token_delay_time = PR_SecondsToInterval(NSSSLOT_TOKEN_DELAY_TIME);
    }
    time = PR_IntervalNow();
    lastTime = slot->lastTokenPing;
    if ((lastTime) &&
	(time > lastTime) && ((time - lastTime) < s_token_delay_time)) {
	return PR_TRUE;
    }
    slot->lastTokenPing = time;
    return PR_FALSE;
}

NSS_IMPLEMENT PRBool
nssSlot_IsTokenPresent (
  NSSSlot *slot
)
{
    CK_RV ckrv;
    PRStatus nssrv;
    /* XXX */
    nssSession *session;
    CK_SLOT_INFO slotInfo;
    void *epv;
    /* permanent slots are always present */
    if (nssSlot_IsPermanent(slot)) {
	return PR_TRUE;
    }
    /* avoid repeated calls to check token status within set interval */
    if (within_token_delay_period(slot)) {
	return (PRBool)((slot->ckFlags & CKF_TOKEN_PRESENT) != 0);
    }
    /* First obtain the slot info */
    epv = nssModule_GetCryptokiEPV(slot->module);
    if (!epv) {
	return PR_FALSE;
    }
    ckrv = CKAPI(epv)->C_GetSlotInfo(slot->slotID, &slotInfo);
    if (ckrv != CKR_OK) {
	return PR_FALSE;
    }
    slot->ckFlags = slotInfo.flags;
    /* check for the presence of the token */
    if ((slot->ckFlags & CKF_TOKEN_PRESENT) == 0) {
	if (!slot->token) {
	    /* token was ne'er present */
	    return PR_FALSE;
	}
	session = nssToken_GetDefaultSession(slot->token);
	nssSession_EnterMonitor(session);
	/* token is not present */
	if (session->handle != CK_INVALID_SESSION) {
	    /* session is valid, close and invalidate it */
	    CKAPI(epv)->C_CloseSession(session->handle);
	    session->handle = CK_INVALID_SESSION;
	}
	nssSession_ExitMonitor(session);
	return PR_FALSE;
    } else if (!slot->token) {
	/* token was not present at boot time, is now */
	slot->token = nssToken_Create(slot->slotID, slot);
	return (slot->token != NULL);
    }
    /* token is present, use the session info to determine if the card
     * has been removed and reinserted.
     */
    session = nssToken_GetDefaultSession(slot->token);
    nssSession_EnterMonitor(session);
    if (session->handle != CK_INVALID_SESSION) {
	CK_SESSION_INFO sessionInfo;
	ckrv = CKAPI(epv)->C_GetSessionInfo(session->handle, &sessionInfo);
	if (ckrv != CKR_OK) {
	    /* session is screwy, close and invalidate it */
	    CKAPI(epv)->C_CloseSession(session->handle);
	    session->handle = CK_INVALID_SESSION;
	}
    }
    nssSession_ExitMonitor(session);
    /* token not removed, finished */
    if (session->handle != CK_INVALID_SESSION) {
	return PR_TRUE;
    } else {
	/* the token has been removed, and reinserted, invalidate all the old
	 * information we had on this token */
#ifdef NSS_3_4_CODE
	nssToken_NotifyCertsNotVisible(slot->token);
#endif /* NSS_3_4_CODE */
	nssToken_Remove(slot->token);
	/* token has been removed, need to refresh with new session */
	nssrv = nssSlot_Refresh(slot);
	if (nssrv != PR_SUCCESS) {
	    return PR_FALSE;
	}
	return PR_TRUE;
    }
}

NSS_IMPLEMENT NSSModule *
nssSlot_GetModule (
  NSSSlot *slot
)
{
    return nssModule_AddRef(slot->module);
}

NSS_IMPLEMENT void *
nssSlot_GetCryptokiEPV (
  NSSSlot *slot
)
{
    return nssModule_GetCryptokiEPV(slot->module);
}

NSS_IMPLEMENT NSSToken *
nssSlot_GetToken (
  NSSSlot *slot
)
{
    if (nssSlot_IsTokenPresent(slot)) {
	return nssToken_AddRef(slot->token);
    }
    return (NSSToken *)NULL;
}

NSS_IMPLEMENT NSSToken *
NSSSlot_GetToken (
  NSSSlot *slot
)
{
    return nssSlot_GetToken(slot);
}

NSS_IMPLEMENT PRBool
nssSlot_IsPermanent (
  NSSSlot *slot
)
{
    return (!(slot->ckFlags & CKF_REMOVABLE_DEVICE));
}

NSS_IMPLEMENT PRBool
nssSlot_IsFriendly (
  NSSSlot *slot
)
{
    return PR_TRUE /* XXX NSSSLOT_IS_FRIENDLY(slot)*/;
}

NSS_IMPLEMENT PRBool
nssSlot_IsHardware (
  NSSSlot *slot
)
{
    return (slot->ckFlags & CKF_HW_SLOT);
}

NSS_IMPLEMENT PRStatus
nssSlot_Refresh (
  NSSSlot *slot
)
{
    /* XXX */
#if 0
    nssToken_Destroy(slot->token);
    if (slotInfo.flags & CKF_TOKEN_PRESENT) {
	slot->token = nssToken_Create(NULL, slotID, slot);
    }
#endif
    return PR_SUCCESS;
}

static PRBool
slot_needs_login (
  NSSSlot *slot,
  nssSession *session
)
{
    PRBool needsLogin, logout;
    struct nssSlotAuthInfoStr *authInfo = &slot->authInfo;
    void *epv = nssModule_GetCryptokiEPV(slot->module);
    if (!nssToken_IsLoginRequired(slot->token)) {
	return PR_FALSE;
    }
    if (authInfo->askTimes == nssSlotAskPasswordTimes_EveryTime) {
	logout = PR_TRUE;
    } else if (authInfo->askTimes == nssSlotAskPasswordTimes_Timeout) {
	PRIntervalTime currentTime = PR_IntervalNow();
	if (authInfo->lastLogin - currentTime < authInfo->askPasswordTimeout) {
	    logout = PR_FALSE;
	} else {
	    logout = PR_TRUE;
	}
    } else { /* nssSlotAskPasswordTimes_FirstTime */
	logout = PR_FALSE;
    }
    if (logout) {
	/* The login has expired, timeout */
	nssSession_EnterMonitor(session);
	CKAPI(epv)->C_Logout(session->handle);
	nssSession_ExitMonitor(session);
	needsLogin = PR_TRUE;
    } else {
	CK_RV ckrv;
	CK_SESSION_INFO sessionInfo;
	nssSession_EnterMonitor(session);
	ckrv = CKAPI(epv)->C_GetSessionInfo(session->handle, &sessionInfo);
	nssSession_ExitMonitor(session);
	if (ckrv != CKR_OK) {
	    /* XXX error -- invalidate session */ 
	    return PR_FALSE;
	}
	switch (sessionInfo.state) {
	case CKS_RW_PUBLIC_SESSION:
	case CKS_RO_PUBLIC_SESSION:
	default:
	    needsLogin = PR_TRUE;
	    break;
	case CKS_RW_USER_FUNCTIONS:
	case CKS_RW_SO_FUNCTIONS:
	case CKS_RO_USER_FUNCTIONS:
	    needsLogin = PR_FALSE;
	    break;
	}
    }
    return needsLogin;
}

static PRStatus
slot_login (
  NSSSlot *slot, 
  nssSession *session, 
  CK_USER_TYPE userType, 
  NSSCallback *pwcb
)
{
    PRStatus nssrv;
    PRBool retry;
    PRBool keepTrying;
    NSSUTF8 *password = NULL;
    CK_ULONG pwLen;
    CK_RV ckrv;
    void *epv;
    if (!pwcb->getPW) {
	/* set error INVALID_ARG */
	return PR_FAILURE;
    }
    epv = nssModule_GetCryptokiEPV(slot->module);
    keepTrying = PR_TRUE;
    nssrv = PR_FAILURE;
    while (keepTrying) {
	/* use the token name, since it is present */
	NSSUTF8 *tokenName = nssToken_GetName(slot->token);
	nssrv = pwcb->getPW(tokenName, &retry, pwcb->arg, &password);
	if (nssrv != PR_SUCCESS) {
	    nss_SetError(NSS_ERROR_USER_CANCELED);
	    break;
	}
	pwLen = (CK_ULONG)nssUTF8_Length(password, &nssrv); 
	if (nssrv != PR_SUCCESS) {
	    break;
	}
	nssSession_EnterMonitor(session);
	ckrv = CKAPI(epv)->C_Login(session->handle, userType, 
                                   (CK_CHAR_PTR)password, pwLen);
	nssSession_ExitMonitor(session);
	switch (ckrv) {
	case CKR_OK:
	case CKR_USER_ALREADY_LOGGED_IN:
	    slot->authInfo.lastLogin = PR_Now();
	    nssrv = PR_SUCCESS;
	    keepTrying = PR_FALSE;
	    break;
	case CKR_PIN_INCORRECT:
	    nss_SetError(NSS_ERROR_INVALID_PASSWORD);
	    keepTrying = retry; /* received bad pw, keep going */
	    if (!retry) nssrv = PR_FAILURE;
	    break;
	default:
	    nssrv = PR_FAILURE;
	    keepTrying = PR_FALSE;
	    break;
	}
	nss_ZFreeIf(password);
	password = NULL;
    }
    return nssrv;
}

static PRStatus
init_slot_password (
  NSSSlot *slot, 
  nssSession *rwSession, 
  NSSUTF8 *password
)
{
    PRStatus status;
    NSSUTF8 *ssoPW = "";
    CK_ULONG userPWLen, ssoPWLen;
    CK_RV ckrv;
    void *epv = nssModule_GetCryptokiEPV(slot->module);
    /* Get the SO and user passwords */
    userPWLen = (CK_ULONG)nssUTF8_Length(password, &status); 
    if (status != PR_SUCCESS) {
	goto loser;
    }
    ssoPWLen = (CK_ULONG)nssUTF8_Length(ssoPW, &status); 
    if (status != PR_SUCCESS) {
	goto loser;
    }
    /* First log in as SO */
    ckrv = CKAPI(epv)->C_Login(rwSession->handle, CKU_SO, 
                               (CK_CHAR_PTR)ssoPW, ssoPWLen);
    if (ckrv != CKR_OK) {
	/* set error ...SO_LOGIN_FAILED */
	goto loser;
    }
	/* Now change the user PIN */
    ckrv = CKAPI(epv)->C_InitPIN(rwSession->handle, 
                                 (CK_CHAR_PTR)password, userPWLen);
    if (ckrv != CKR_OK) {
	/* set error */
	goto loser;
    }
    return PR_SUCCESS;
loser:
    return PR_FAILURE;
}

static PRStatus
password_return (
  NSSUTF8 *slotName,
  PRBool *retry,
  void *arg,
  NSSUTF8 **password
)
{
    *retry = PR_FALSE;
    *password = nssUTF8_Duplicate((NSSUTF8 *)arg, NULL);
    return PR_SUCCESS;
}

static PRStatus
change_slot_password (
  NSSSlot *slot, 
  nssSession *rwSession, 
  NSSUTF8 *oldPassword,
  NSSUTF8 *newPassword
)
{
    PRStatus status;
    CK_ULONG userPWLen, newPWLen;
    CK_RV ckrv;
    PRBool retried = PR_FALSE;
    NSSCallback have_pw_cb;
    void *epv = nssModule_GetCryptokiEPV(slot->module);

    userPWLen = (CK_ULONG)nssUTF8_Length(oldPassword, &status); 
    if (status != PR_SUCCESS) {
	return status;
    }

    newPWLen = (CK_ULONG)nssUTF8_Length(newPassword, &status); 
    if (status != PR_SUCCESS) {
	return status;
    }

retry:
    nssSession_EnterMonitor(rwSession);
    ckrv = CKAPI(epv)->C_SetPIN(rwSession->handle,
                                (CK_CHAR_PTR)oldPassword, userPWLen,
                                (CK_CHAR_PTR)newPassword, newPWLen);
    nssSession_ExitMonitor(rwSession);

    switch (ckrv) {
    case CKR_OK:
	slot->authInfo.lastLogin = PR_Now();
	status = PR_SUCCESS;
	break;
    case CKR_PIN_INCORRECT:
	nss_SetError(NSS_ERROR_INVALID_PASSWORD);
	status = PR_FAILURE;
	break;
    case CKR_USER_NOT_LOGGED_IN:
	/* Have to log in before trying the password.  Use the password
	 * provided by the caller, and retry.
	 */
	have_pw_cb.getPW = password_return;
	have_pw_cb.arg = oldPassword;
	status = slot_login(slot, rwSession, CKU_USER, &have_pw_cb);
	if (status == PR_SUCCESS && !retried) {
	    retried = PR_TRUE;
	    goto retry;
	}
	break;
    default:
	status = PR_FAILURE;
	break;
    }
    return status;
}

NSS_IMPLEMENT PRStatus
nssSlot_Login (
  NSSSlot *slot,
  NSSCallback *pwcb
)
{
    PRStatus status;
    CK_USER_TYPE userType = CKU_USER;
    NSSToken *token = nssSlot_GetToken(slot);
    nssSession *session;
    if (!token) {
	return PR_FAILURE;
    }
    if (!nssToken_IsLoginRequired(token)) {
	nssToken_Destroy(token);
	return PR_SUCCESS;
    }
    session = nssToken_GetDefaultSession(slot->token);
    if (nssToken_NeedsPINInitialization(token)) {
	NSSUTF8 *password = NULL;
	if (!pwcb->getInitPW) {
	    nssToken_Destroy(token);
	    return PR_FAILURE; /* don't know how to get initial password */
	}
	status = (*pwcb->getInitPW)(slot->base.name, pwcb->arg, &password);
	if (status == PR_SUCCESS) {
	    session = nssSlot_CreateSession(slot, PR_TRUE);
	    status = init_slot_password(slot, session, password);
	    nssSession_Destroy(session);
	}
    } else if (slot_needs_login(slot, session)) {
	status = slot_login(slot, session, userType, pwcb);
    } else {
	status = PR_SUCCESS;
    }
    nssToken_Destroy(token);
    return status;
}

NSS_IMPLEMENT PRStatus
nssSlot_Logout (
  NSSSlot *slot,
  nssSession *session
)
{
    PRStatus nssrv = PR_SUCCESS;
    CK_RV ckrv;
    void *epv = nssModule_GetCryptokiEPV(slot->module);

    PR_ASSERT(session); /* XXX remove later */

    nssSession_EnterMonitor(session);
    ckrv = CKAPI(epv)->C_Logout(session->handle);
    nssSession_ExitMonitor(session);
    if (ckrv != CKR_OK) {
	/* translate the error */
	nssrv = PR_FAILURE;
    }
    return nssrv;
}

NSS_IMPLEMENT PRBool
nssSlot_IsLoggedIn (
  NSSSlot *slot
)
{
    nssSession *session = nssToken_GetDefaultSession(slot->token); /* XXX */
    return !slot_needs_login(slot, session);
}

NSS_IMPLEMENT PRStatus
nssSlot_CheckPassword (
  NSSSlot *slot,
  const NSSUTF8 *password
)
{
    nssSession *session = nssToken_GetDefaultSession(slot->token); /* XXX */
    NSSCallback have_pw_cb;

    have_pw_cb.getPW = password_return;
    have_pw_cb.arg = (char *)password;

    if (nssSlot_IsLoggedIn(slot)) {
	nssSlot_Logout(slot, session);
    }
    return slot_login(slot, session, CKU_USER, &have_pw_cb);
}

NSS_IMPLEMENT PRStatus
NSSSlot_CheckPassword (
  NSSSlot *slot,
  const NSSUTF8 *password
)
{
    return nssSlot_CheckPassword(slot, password);
}

NSS_IMPLEMENT void
nssSlot_SetPasswordDefaults (
  NSSSlot *slot,
  PRInt32 askPasswordTimeout
)
{
    slot->authInfo.askPasswordTimeout = askPasswordTimeout;
}


NSS_IMPLEMENT PRStatus
nssSlot_SetPassword (
  NSSSlot *slot,
  NSSUTF8 *oldPasswordOpt,
  NSSUTF8 *newPassword
)
{
    PRStatus status;
    nssSession *rwSession;
    NSSToken *token = nssSlot_GetToken(slot);
    if (!token) {
	return PR_FAILURE;
    }
    rwSession = nssSlot_CreateSession(slot, PR_TRUE);
    if (nssToken_NeedsPINInitialization(token)) {
	status = init_slot_password(slot, rwSession, newPassword);
    } else if (oldPasswordOpt) {
	status = change_slot_password(slot, rwSession, 
	                              oldPasswordOpt, newPassword);
    } else {
	/* old password must be given in order to change */
	status = PR_FAILURE;
    }
    nssSession_Destroy(rwSession);
    nssToken_Destroy(token);
    return status;
}

NSS_IMPLEMENT PRStatus
NSSSlot_SetPassword (
  NSSSlot *slot,
  NSSUTF8 *oldPasswordOpt,
  NSSUTF8 *newPassword
)
{
    return nssSlot_SetPassword(slot, oldPasswordOpt, newPassword);
}

/*
 * Attempts to open a session on the slot.  If the slot cannot open another
 * session, the session is copied from the token's default.  These sessions
 * are 'virtual' sessions, from a higher level they appear no different, but
 * they share the same CK_SESSION_HANDLE and lock.
 * 
 *
 * Description of session multiplexing
 *
 * When session starvation occurs (CKR_SESSION_COUNT), sessions are created
 * as children of a parent session.  The parent is the session that
 * originally obtained the CK_SESSION_HANDLE.  The parent and all of its
 * children share a lock.  When the session monitor is requested for
 * the parent or any of its children, the shared lock is acquired.  At
 * that point, multiplexing is performed.  If the session is the "owner"
 * of the CK_SESSION_HANDLE, nothing more is done.  Otherwise, the owner
 * is obtained from the parent.  The state of the owner is saved.  If
 * the current session has saved state, that state is restored.  The
 * current session is set as the owner.
 */
NSS_IMPLEMENT nssSession *
nssSlot_CreateSession (
  NSSSlot *slot,
  PRBool readWrite
)
{
    CK_RV ckrv;
    CK_FLAGS ckflags;
    CK_SESSION_HANDLE handle;
    void *epv = nssModule_GetCryptokiEPV(slot->module);
    nssSession *rvSession;

    ckflags = s_ck_readonly_flags;
    if (readWrite) {
	ckflags |= CKF_RW_SESSION;
    }

    /* Even virtual sessions get their own memory */
    rvSession = nss_ZNEW(NULL, nssSession);
    if (!rvSession) {
	return (nssSession *)NULL;
    }

    /* Open a new session if there are any available */
    ckrv = CKAPI(epv)->C_OpenSession(slot->slotID, ckflags,
                                     slot, nss_ck_slot_notify, &handle);
    if (ckrv == CKR_SESSION_COUNT) {
	/* there weren't any, create a virtual session */
	nssSession *defaultSession = NULL;
	if (slot->token) {
	    defaultSession = nssToken_GetDefaultSession(slot->token);
	}
	if (!defaultSession) {
	    return (nssSession *)NULL;
	}
	if (readWrite && !defaultSession->isRW) {
	    /* XXX need to handle this case */
	    nss_ZFreeIf(rvSession);
	    return (nssSession *)NULL;
	}
	*rvSession = *defaultSession; /* copy it */
	rvSession->refCount = 1;
	rvSession->owner = defaultSession;
	rvSession->isParent = PR_FALSE;
	/* keep a reference to the parent so it doesn't go away */
	PR_AtomicIncrement(&defaultSession->refCount);
	return rvSession;
    } else if (ckrv != CKR_OK) {
	/* set an error here, eh? */
	nss_ZFreeIf(rvSession);
	return (nssSession *)NULL;
    }

    if (!nssModule_IsThreadSafe(slot->module) || slot->sessionLimit) {
	/* If the parent module is not threadsafe, create lock to manage 
	 * session within threads.
	 * If the number of sessions is limited, create lock to handle
	 * session multiplexing.
	 */
	rvSession->lock = PZ_NewLock(nssILockOther);
	if (!rvSession->lock) {
	    nss_ZFreeIf(rvSession);
	    return (nssSession *)NULL;
	}
    }

    rvSession->handle = handle;
    rvSession->slot = slot;
    rvSession->isRW = readWrite;
    PR_AtomicIncrement(&rvSession->refCount);
    rvSession->isParent = PR_TRUE;
    /* parent starts off owning the session */
    rvSession->owner = rvSession;
    return rvSession;
}

NSS_IMPLEMENT nssSession *
nssSession_AddRef (
  nssSession *s
)
{
    PR_AtomicIncrement(&s->refCount);
    return s;
}

NSS_IMPLEMENT PRStatus
nssSession_Destroy (
  nssSession *s
)
{
    CK_RV ckrv = CKR_OK;
    if (s) {
	void *epv = nssModule_GetCryptokiEPV(s->slot->module);
	if (PR_AtomicDecrement(&s->refCount) == 0) {
	    if (!s->isParent) {
		/* virtual session (child), just notify the parent */
		nssSession_Destroy(s->owner);
	    } else {
		/* own session -- all children must be gone */
		ckrv = CKAPI(epv)->C_CloseSession(s->handle);
		if (s->lock) {
		    PZ_DestroyLock(s->lock);
		}
	    }
	    nss_ZFreeIf(s);
	}
    }
    return (ckrv == CKR_OK) ? PR_SUCCESS : PR_FAILURE;
}

/* XXX these need to handle multiplexing */
NSS_IMPLEMENT PRStatus
nssSession_Save (
  nssSession *s,
  NSSItem *state,
  NSSArena *arenaOpt
)
{
    CK_RV ckrv;
    CK_ULONG stateLen;
    void *epv = nssSlot_GetCryptokiEPV(s->slot);
    nssSession_EnterMonitor(s);
    ckrv = CKAPI(epv)->C_GetOperationState(s->handle, NULL, &stateLen);
    if (ckrv == CKR_OK && stateLen > 0) {
	state->data = nss_ZAlloc(arenaOpt, stateLen);
	if (!state->data) {
	    return PR_FAILURE;
	}
	state->size = stateLen;
	ckrv = CKAPI(epv)->C_GetOperationState(s->handle,
	                                       (CK_BYTE_PTR)state->data,
	                                       (CK_ULONG_PTR)&state->size);
	if (ckrv != CKR_OK) {
	    nss_ZFreeIf(state->data);
	    state->data = NULL;
	    state->size = 0;
	}
    }
    nssSession_ExitMonitor(s);
    return (ckrv == CKR_OK) ? PR_SUCCESS : PR_FAILURE;
}

NSS_IMPLEMENT PRStatus
nssSession_Restore (
  nssSession *s,
  NSSItem *state
)
{
    CK_RV ckrv;
    void *epv = nssSlot_GetCryptokiEPV(s->slot);
    nssSession_EnterMonitor(s);
    if (state->size > 0) {
	ckrv = CKAPI(epv)->C_SetOperationState(s->handle, 
	                                       (CK_BYTE_PTR)state->data,
	                                       (CK_ULONG)state->size,
	                                       CK_INVALID_HANDLE,
	                                       CK_INVALID_HANDLE);
	if (ckrv == CKR_OK) {
	    nss_ZFreeIf(state->data);
	    state->data = NULL;
	    state->size = 0;
	}
    }
    nssSession_ExitMonitor(s);
    return (ckrv == CKR_OK) ? PR_SUCCESS : PR_FAILURE;
}

NSS_IMPLEMENT nssSession *
nssSession_Clone (
  nssSession *s
)
{
    PRStatus status;
    NSSItem state;
    nssSession *rvSession = NULL;

    /* Get a new session with the same properties as the original one */
    rvSession = nssSlot_CreateSession(s->slot, s->isRW);
    if (rvSession) {
	/* save the original session's state */
	status = nssSession_Save(s, &state, NULL);
	if (status == PR_SUCCESS) {
	    /* and restore it into the new session */
	    status = nssSession_Restore(rvSession, &state);
	    /* free the saved state */
	    nss_ZFreeIf(state.data);
	    if (status == PR_SUCCESS) {
		return rvSession;
	    }
	}
	nssSession_Destroy(rvSession);
    }
    return (nssSession *)NULL;
}

static PRStatus
save_session_state(nssSession *s)
{
    return nssSession_Save(s, &s->state, NULL);
}

static PRStatus
restore_session_state(nssSession *s)
{
    return nssSession_Restore(s, &s->state);
}

NSS_IMPLEMENT PRStatus
nssSession_EnterMonitor (
  nssSession *s
)
{
    nssSession *parent;
    if (s->lock) {
	/* acquire the shared lock */
	PZ_Lock(s->lock);
	/* get the parent session (this may be the parent) */
	parent = s->isParent ? s : s->owner;
	if (parent->owner != s) {
	    /* multiplex - save owner's state */
	    save_session_state(parent->owner);
	    /* restore my state */
	    restore_session_state(s);
	    /* I'm now the owner */
	    parent->owner = s;
	}
    }
    return PR_SUCCESS;
}

NSS_IMPLEMENT PRStatus
nssSession_ExitMonitor (
  nssSession *s
)
{
    if (s->lock) {
	PZ_Unlock(s->lock);
    }
    return PR_SUCCESS;
}

NSS_EXTERN PRBool
nssSession_IsReadWrite (
  nssSession *s
)
{
    return s->isRW;
}

