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
 * NSSPKIXAttribute_GetValues
 *
 * This routine returns all of the attribute values in the specified
 * NSSPKIXAttribute.  If the optional pointer to an array of NSSItems
 * is non-null, then that array will be used and returned; otherwise,
 * an array will be allocated and returned.  If the limit is nonzero
 * (which is must be if the specified array is nonnull), then an
 * error is indicated if it is smaller than the value count.
 * {arenaOpt}
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_PKIX_ATTRIBUTE
 *  NSS_ERROR_INVALID_ARENA
 *  NSS_ERROR_NO_MEMORY
 *  NSS_ERROR_ARRAY_TOO_SMALL
 *
 * Return value:
 *  A valid pointer to an array of NSSItem's upon success
 *  NULL upon failure.
 */

NSS_IMPLEMENT NSSPKIXAttributeValue *
NSSPKIXAttribute_GetValues
(
  NSSPKIXAttribute *attribute,
  NSSPKIXAttributeValue rvOpt[],
  PRInt32 limit,
  NSSArena *arenaOpt
)
{
  nss_ClearErrorStack();

#ifdef DEBUG
  if( PR_SUCCESS != nssPKIXAttribute_verifyPointer(attribute) ) {
    return (NSSPKIXAttributeValue *)NULL;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyOpt(attribute) ) {
      return (NSSPKIXAttributeValue *)NULL;
    }
  }
#endif /* DEBUG */

  return nssPKIXAttribute_GetValues(attribute, rvOpt, limit, arenaOpt);
}
