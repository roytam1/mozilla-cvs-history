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
 * The Original Code is the Netscape svrcore library.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef __SVRCORE_H_
#define __SVRCORE_H_

#include <prtypes.h>
#include <seccomon.h>
#include <pk11func.h>

PR_BEGIN_EXTERN_C
/* ------------------------------------------------------------ */
/*
 * SVRCOREError - error values generated by components in the
 *   SVRCORE module.
 */
enum SVRCOREError
{
  SVRCORE_Success                  = 0,
  SVRCORE_NoMemory_Error           = 1,
  SVRCORE_System_Error             = 2,
  SVRCORE_NoSuchToken_Error        = 3,
  SVRCORE_IncorrectPassword_Error  = 4,
  SVRCORE_MaximumErrorValue        = 4
};
typedef enum SVRCOREError SVRCOREError;


/* ------------------------------------------------------------ */
/*
 * PIN Object - provides an interface to get the PIN for
 *   a PKCS11 token.
 *
 * Methods:
 *   destroyObj - delete the object
 *   getPin - retrieve the PIN for the token with name "tokenName".  The
 *     retry flag is set if this request is a retry due to an incorrect
 *     PIN.  Implementations should not return a "cached" copy in this case,
 *     since it will result in multiple fails, and will disable some tokens.
 *     The memory allocated for the returned string must be on the system
 *     heap.  It will be released using "free"
 */
typedef struct SVRCOREPinObj SVRCOREPinObj;
typedef struct SVRCOREPinMethods SVRCOREPinMethods;
struct SVRCOREPinMethods
{
  void *(*reserved0)(SVRCOREPinObj *, void *);
  void  (*reserved1)(SVRCOREPinObj *);
  void  (*destroyObj)(SVRCOREPinObj* obj);
  char *(*getPin)(SVRCOREPinObj *obj, const char *tokenName, PRBool retryFlag);
};

struct SVRCOREPinObj
{
  const SVRCOREPinMethods *methods;
};

/*
 * Methods on SVRCOREPinObj
 */
/* char *SVRCORE_GetPin(SVRCOREPinObj *, char *tokenName, PRBool retry) */
#define SVRCORE_GetPin(obj, name, retry) \
  (obj)->methods->getPin(obj, name, retry)

/* void SVRCORE_DestroyPinObj(SVRCOREPinObj *) */
#define SVRCORE_DestroyPinObj(obj) \
  (obj)->methods->destroyObj(obj)


/* ------------------------------------------------------------ */
/*
 * SVRCORE_RegisterPinObj - registers the PIN handling object with the
 *   PK11 module.
 * 
 * The PIN object's getPin method will be called when the NSS layer
 * requires a password/PIN for a token.  The caller may provide NULL
 * as the pin object, in which case, no password callbacks will be invoked.
 * This may be used to unregister the object prior to deleting it.
 */
void
SVRCORE_RegisterPinObj(SVRCOREPinObj *obj);

/*
 * SVRCORE_GetRegisteredPinObj - get the currently registered Pin object
 *  (if any)
 *
 * Return a pointer to the currently register Pin object.  If none has been
 * registered, NULL is returned.
 */
SVRCOREPinObj *
SVRCORE_GetRegisteredPinObj(void);

/* ------------------------------------------------------------ */
/*
 * SVRCOREStdPinObj - implementation of SVRCOREPinObj that
 *   provides the standard handling for servers.  This includes
 *   optional file lookup, and optional caching
 *
 * SVRCORE_SetStdPinInteractive - allows the application to declare
 *   that input via the terminal is no longer possible (set interactive
 *   to PR_FALSE).  See the corresponding routine for UserPinObj
 *
 * SVRCORE_StdPinGetPin - get a (securely) cached PIN value.  Returns
 *   SVRCORE_NoSuchToken_Error if the object is not set up for caching.
 */
typedef struct SVRCOREStdPinObj SVRCOREStdPinObj;

SVRCOREError
SVRCORE_CreateStdPinObj(SVRCOREStdPinObj **out,
  const char *filename, PRBool cachePINs);

void
SVRCORE_SetStdPinInteractive(SVRCOREStdPinObj *obj, PRBool interactive);

SVRCOREError
SVRCORE_StdPinGetPin(char **pin, SVRCOREStdPinObj *obj,
  const char *tokenName);

void
SVRCORE_DestroyStdPinObj(SVRCOREStdPinObj *obj);

/* ------------------------------------------------------------ */
/*
 * SVRCOREUserPinObj - implementation of SVRCOREPinObj that
 *    requests the PIN on the terminal.
 *
 * SVRCORE_SetUserPinInteractive - allows the application to declare
 *   that input via the terminal is no longer possible (set interactive
 *   to PR_FALSE).  When this is the case, the object returns NULL (no
 *   PIN available)
 */
typedef struct SVRCOREUserPinObj SVRCOREUserPinObj;

SVRCOREError
SVRCORE_CreateUserPinObj(SVRCOREUserPinObj **out);

void
SVRCORE_SetUserPinInteractive(SVRCOREUserPinObj *obj, PRBool interactive);

void
SVRCORE_DestroyUserPinObj(SVRCOREUserPinObj *obj);

/* ------------------------------------------------------------ */
/*
 * SVRCOREAltPinObj - allows cascading of PinObj.  For example, an
 *   application can first check a file (FilePinObj) and then the terminal
 *   (UserPinObj).  The primary object is called first then, if no PIN is
 *   available, the alternate object is called.
 *
 *   This object does not claim ownership of the PinObjs.  The application
 *   must delete them (after deleting the AltPinObj).
 */
typedef struct SVRCOREAltPinObj SVRCOREAltPinObj;

SVRCOREError
SVRCORE_CreateAltPinObj(
  SVRCOREAltPinObj **out,
  SVRCOREPinObj *primary, SVRCOREPinObj *alt);

void SVRCORE_DestroyAltPinObj(SVRCOREAltPinObj *obj);

/* ------------------------------------------------------------ */
/*
 * SVRCOREFilePinObj - implements reading PINs from a file.  The
 *   name of the file is provided in the constructor.
 */
typedef struct SVRCOREFilePinObj SVRCOREFilePinObj;

SVRCOREError
SVRCORE_CreateFilePinObj(
  SVRCOREFilePinObj **out,
  const char *filename);

void
SVRCORE_DestroyFilePinObj(SVRCOREFilePinObj *obj);

/* ------------------------------------------------------------ */
/*
 * SVRCORECachedPinObj - implementation of SVRCOREPinObj that
 *    caches the PIN in a secure way.
 *
 * SVRCORE_CachedPinGetPin - allows the application to retrieve
 *   the stored pin.  The application should free the value useing free()
 *   after clearing the memory.
 */
typedef struct SVRCORECachedPinObj SVRCORECachedPinObj;

SVRCOREError
SVRCORE_CreateCachedPinObj(SVRCORECachedPinObj **out, SVRCOREPinObj *alt);

SVRCOREError
SVRCORE_CachedPinGetPin(char **pin, SVRCORECachedPinObj *obj,
  const char *tokenName);

void
SVRCORE_DestroyCachedPinObj(SVRCORECachedPinObj *obj);


/* ------------------------------------------------------------ */
/*
 * Implements SVRCORESecurePinStore interface
 */
typedef struct SVRCOREPk11PinStore SVRCOREPk11PinStore;

/*
 * SVRCORE_GetPk11PinStoreError
 */
SECStatus SVRCORE_Pk11StoreGetError(const SVRCOREPk11PinStore *store);

/* Experimental */
const char *SVRCORE_Pk11StoreGetMechName(const SVRCOREPk11PinStore *store);

/*
 * SVRCORE_CreatePk11PinStore
 * Args:
 *   None
 * Errors:
 *   SVRCORE_Success
 *   SVRCORE_NoMemory_Error
 *   SVRCORE_NoSuchToken_Error
 *   SVRCORE_System_Error
 *   SVRCORE_IncorrectPassword_Error
 */
SVRCOREError
SVRCORE_CreatePk11PinStore(
  SVRCOREPk11PinStore **out,  /* Output */
  const char *tokenName,
  const char *pin);

SVRCOREError
SVRCORE_Pk11StoreGetPin(
  char **out,                /* Output */
  SVRCOREPk11PinStore *store);

void
SVRCORE_DestroyPk11PinStore(
  SVRCOREPk11PinStore *store);


PR_END_EXTERN_C

#endif