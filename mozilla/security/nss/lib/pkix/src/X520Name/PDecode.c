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
 * nssPKIXX520Name_Decode
 *
 * -- fgmr comments --
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_BER
 *  NSS_ERROR_NO_MEMORY
 *  NSS_ERROR_INVALID_ARENA
 * 
 * Return value:
 *  A valid pointer to an NSSPKIXX520Name upon success
 *  NULL upon failure
 */

NSS_IMPLEMENT NSSPKIXX520Name *
nssPKIXX520Name_Decode
(
  NSSArena *arenaOpt,
  NSSBER *ber
)
{
  NSSPKIXX520Name *rv = (NSSPKIXX520Name *)NULL;
  nssArenaMark *mark = (nssArenaMark *)NULL;
  PRStatus status;

#ifdef NSSDEBUG
  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSPKIXAttribute *)NULL;
    }
  }

  if( PR_SUCCESS != nssItem_verifyPointer(ber) ) {
    return (NSSPKIXAttribute *)NULL;
  }
#endif /* NSSDEBUG */

  if( (NSSArena *)NULL != arenaOpt ) {
    mark = nssArena_Mark(arenaOpt);
    if( (nssArenaMark *)NULL == mark ) {
      goto loser;
    }
  }

  rv = nss_ZNEW(arenaOpt, NSSPKIXX520Name);
  if( (NSSPKIXX520Name *)NULL == rv ) {
    goto loser;
  }

  status = nssASN1_DecodeBER(arenaOpt, rv, nssPKIXX520_template, ber);
  if( PR_SUCCESS != status ) {
    goto loser;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    rv->inArena = PR_TRUE;
  }

  if( (nssArenaMark *)NULL != mark ) {
    if( PR_SUCCESS != nssArena_Unmark(arenaOpt, mark) ) {
      goto loser;
    }
  }

#ifdef DEBUG
  if( PR_SUCCESS != nss_pkix_X520Name_add_pointer(rv) ) {
    goto loser;
  }

  if( PR_SUCCESS != nssArena_registerDestructor(arena, 
        nss_pkix_X520Name_remove_pointer, rv) ) {
    (void)nss_pkix_X520Name_remove_pointer(rv);
    goto loser;
  }
#endif /* DEBUG */

  return rv;

 loser:
  if( (nssArenaMark *)NULL != mark ) {
    (void)nssArena_Release(arenaOpt, mark);
  }

  if( (NSSArena *)NULL == arenaOpt ) {
    if( (NSSPKIXX520Name *)NULL != rv ) {
      nss_ZFreeIf(rv->string.data);
      nss_ZFreeIf(rv->utf8);
      nss_ZFreeIf(rv);
    }
  }

  return (NSSPKIXX520Name *)NULL;
}
