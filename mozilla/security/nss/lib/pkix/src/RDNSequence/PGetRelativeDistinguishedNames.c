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
 * nssPKIXRDNSequence_GetRelativeDistinguishedNames
 *
 * This routine returns all of the relative distinguished names in the
 * specified RDN Sequence.  {...} If the array is allocated, or if the
 * specified one has extra space, the array will be null-terminated.
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_PKIX_RDN_SEQUENCE
 *  NSS_ERROR_INVALID_ARENA
 *  NSS_ERROR_NO_MEMORY
 *  NSS_ERROR_ARRAY_TOO_SMALL
 *
 * Return value:
 *  A valid pointer to an array of NSSPKIXRelativeDistinguishedName 
 *      pointers upon success
 *  NULL upon failure.
 */

NSS_IMPLEMENT NSSPKIXRelativeDistinguishedName **
nssPKIXRDNSequence_GetRelativeDistinguishedNames
(
  NSSPKIXRDNSequence *rdnseq,
  NSSPKIXRelativeDistinguishedName *rvOpt[],
  PRInt32 limit,
  NSSArena *arenaOpt
)
{
  NSSPKIXRelativeDistinguishedName **rv = (NSSPKIXRelativeDistinguishedName **)NULL;
  PRUint32 i;

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssPKIXRDNSequence_verifyPointer(rdnseq) ) {
    return (NSSPKIXRelativeDistinguishedName **)NULL;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyOpt(attribute) ) {
      return (NSSPKIXRelativeDistinguishedName **)NULL;
    }
  }
#endif /* NSSDEBUG */

  if( 0 == rdnseq->count ) {
    nss_pkix_RDNSequence_Count(rdnseq);
  }

#ifdef PEDANTIC
  if( 0 == rdnseq->count ) {
    nss_SetError(NSS_ERROR_VALUE_OUT_OF_RANGE);
    return (NSSPKIXRelativeDistinguishedName **)NULL;
  }
#endif /* PEDANTIC */

  if( (limit < rdnseq->count) &&
      !((0 == limit) && ((NSSPKIXRelativeDistinguishedName **)NULL == rvOpt)) ) {
    nss_SetError(NSS_ERROR_ARRAY_TOO_SMALL);
    return (NSSPKIXRelativeDistinguishedName **)NULL;
  }

  limit = rdnseq->count;
  if( (NSSPKIXRelativeDistinguishedName **)NULL == rvOpt ) {
    rv = nss_ZNEWARRAY(arenaOpt, NSSPKIXRelativeDistinguishedName *, limit);
    if( (NSSPKIXRelativeDistinguishedName **)NULL == rv ) {
      return (NSSPKIXRelativeDistinguishedName **)NULL;
    }
  } else {
    rv = rvOpt;
  }

  for( i = 0; i < limit; i++ ) {
    rv[i] = nssPKIXRelativedistinguishedName_Duplicate(rdnseq->rdns[i], arenaOpt);
    if( (NSSPKIXRelativeDistinguishedName *)NULL == rv[i] ) {
      goto loser;
    }
  }

  return rv;

 loser:
  for( i = 0; i < limit; i++ ) {
    NSSPKIXRelativeDistinguishedName *x = rv[i];
    if( (NSSPKIXRelativeDistinguishedName *)NULL == x ) {
      break;
    }
    (void)nssPKIXRelativeDistinguishedName_Destroy(x);
  }

  if( rv != rvOpt ) {
    nss_ZFreeIf(rv);
  }

  return (NSSPKIXRelativeDistinguishedName **)NULL;
}
