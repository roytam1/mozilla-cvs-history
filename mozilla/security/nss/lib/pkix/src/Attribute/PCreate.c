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
 * nssPKIXAttribute_Create
 *
 * This routine creates an NSSPKIXAttribute from specified components.
 * This routine may return NULL upon error, in which case it will have
 * created an error stack.  If the optional arena argument is non-NULL,
 * that arena will be used for the required memory.  There must be at
 * least one attribute value specified.  The final argument must be
 * NULL, to indicate the end of the set of attribute values.
 *
 * The error may be one of the following values:
 *  NSS_ERROR_NO_MEMORY
 *  NSS_ERROR_INVALID_ARENA
 *  NSS_ERROR_INVALID_OID
 *  NSS_ERROR_INVALID_ITEM
 *  NSS_ERROR_INSUFFICIENT_VALUES
 *  NSS_ERROR_VALUE_OUT_OF_RANGE
 *
 * Return value:
 *  A valid pointer to an NSSPKIXAttribute upon success
 *  NULL upon failure.
 */

NSS_EXTERN NSSPKIXAttribute *
nssPKIXAttribute_Create
(
  NSSArena *arenaOpt,
  NSSPKIXAttributeType *typeOid,
  NSSPKIXAttributeValue *value1,
  ...
)
{
  va_list ap;
  NSSPKIXAttribute *rv;
  PRUint32 count;

#ifdef NSSDEBUG
  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSPKIXAttribute *)NULL;
    }
  }

  if( PR_SUCCESS != nssOID_verifyPointer(typeOid) ) {
    return (NSSPKIXAttribute *)NULL;
  }

  if( (NSSPKIXAttributeValue *)NULL == value1 ) {
    nss_SetError(NSS_ERROR_INSUFFICIENT_VALUES);
    return (NSSPKIXAttribute *)NULL;
  }

  {
    va_start(ap, typeOid);

    while( 1 ) {
      NSSPKIXAttributeValue *v;
      v = (NSSPKIXAttributeValue *)va_arg(ap, NSSPKIXAttributeValue *);
      if( (NSSPKIXAttributeValue *)NULL == v ) {
        break;
      }
    
      if( PR_SUCCESS != nssItem_verifyPointer(v) ) {
        va_end(ap);
        return (NSSPKIXAttribute *)NULL;
      }
    }

    va_end(ap);
  }
#endif /* NSSDEBUG */

  va_start(ap, typeOid);

  for( count = 0; ; count++ ) {
    NSSPKIXAttributeValue *v;
    v = (NSSPKIXAttributeValue *)va_arg(ap, NSSPKIXAttributeValue *);
    if( (NSSPKIXAttributeValue *)NULL == v ) {
      break;
    }

#ifdef PEDANTIC
    if( count == 0xFFFFFFFF ) {
      nss_SetError(NSS_ERROR_VALUE_OUT_OF_RANGE);
      va_end(ap);
      return (NSSPKIXAttribute *)NULL;
    }
#endif /* PEDANTIC */
  }

  va_end(ap);

  va_start(ap, typeOid);
  rv = nss_pkix_Attribute_V_Create(arenaOpt, typeOid, count, ap);
  va_end(ap);

  return rv;
}
