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
 * NSSPKIXRelativeDistinguishedName_SetAttributeTypeAndValues
 *
 * -- fgmr comments --
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_PKIX_RDN
 *  NSS_ERROR_INVALID_POINTER
 *  NSS_ERROR_INVALID_ATAV
 *
 * Return value:
 *  PR_SUCCESS upon success
 *  PR_FAILURE upon failure
 */

NSS_IMPLEMENT PRStatus
NSSPKIXRelativeDistinguishedName_SetAttributeTypeAndValues
(
  NSSPKIXRelativeDistinguishedName *rdn,
  NSSPKIXAttributeTypeAndValue *atavs[],
  PRInt32 countOpt
)
{
  nss_ClearErrorStack();

#ifdef DEBUG
  if( PR_SUCCESS != nssPKIXRelativeDistinguishedName_verifyPointer(rdn) ) {
    return PR_FAILURE;
  }

  if( (NSSPKIXAttributeTypeAndValue **)NULL == atavs ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return PR_FAILURE;
  }

  {
    PRUint32 i, count;

    if( 0 == countOpt ) {
      for( i = 0; i < 0x80000000; i++ ) {
        if( (NSSPKIXAttributeTypeAndValue *)NULL == atavs[i] ) {
          break;
        }
      }

#ifdef PEDANTIC
      if( 0x80000000 == i ) {
        nss_SetError(NSS_ERROR_VALUE_OUT_OF_RANGE);
        return PR_FAILURE;
      }
#endif /* PEDANTIC */

      count = (PRUint32)i;
    } else {
      if( countOpt < 0 ) {
        nss_SetError(NSS_ERROR_VALUE_OUT_OF_RANGE);
        return PR_FAILURE;
      }

      count = (PRUint32)countOpt;
    }

    for( i = 0; i < count; i++ ) {
      if( PR_SUCCESS != nssPKIXAttributeTypeAndValue_verifyPointer(atavs[i]) ) {
        return PR_FAILURE;
      }
    }
  }
#endif /* DEBUG */

  return nssPKIXRelativeDistinguishedName_SetAttributeTypeAndValues(rdn, atavs, countOpt);
}
