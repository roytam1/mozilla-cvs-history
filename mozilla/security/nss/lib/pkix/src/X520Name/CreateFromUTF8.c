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
static const char CVS_ID[] = "@(#) $Source$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

#ifndef PKIX_H
#include "pkix.h"
#endif /* PKIX_H */

/*
 * NSSPKIXX520Name_CreateFromUTF8
 *
 * { basically just enforces the length limit }
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_STRING
 *  NSS_ERROR_NO_MEMORY
 *  NSS_ERROR_INVALID_ARENA
 * 
 * Return value:
 *  A valid pointer to an NSSPKIXX520Name upon success
 *  NULL upon failure
 */

NSS_IMPLEMENT NSSPKIXX520Name *
NSSPKIXX520Name_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *utf8
)
{
  PRStatus status = PR_SUCCESS;
  PRUint32 length;
  nss_ClearErrorStack();
  

#ifdef DEBUG
  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSPKIXX520Name *)NULL;
    }
  }

  if( (NSSUTF8 *)NULL == utf8 ) {
    nss_SetError(NSS_ERROR_INVALID_STRING);
    return (NSSPKIXX520Name *)NULL;
  }
#endif /* DEBUG */

  length = nssUTF8_Length(utf8, &status); 
  if( PR_SUCCESS != status ) {
    if( NSS_ERROR_VALUE_TOO_LARGE == NSS_GetError() ) {
      nss_SetError(NSS_ERROR_STRING_TOO_LONG);
    }
    return (NSSPKIXX520Name *)NULL;
  }

  if( (length < 1 ) || (length > NSSPKIXX520Name_MAXIMUM_LENGTH) ) {
    nss_SetError(NSS_ERROR_STRING_TOO_LONG);
  }

  return nssPKIXX520Name_CreateFromUTF8(arenaOpt, utf8);
}

/*
 * NSSPKIXX520Name_MAXIMUM_LENGTH
 *
 * From RFC 2459:
 *
 *  ub-name INTEGER ::=     32768
 */

const PRUint32 NSSPKIXX520Name_MAXIMUM_LENGTH = 32768;
