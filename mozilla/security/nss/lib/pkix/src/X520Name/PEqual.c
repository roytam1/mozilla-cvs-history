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
 * nssPKIXX520Name_Equal
 *
 *
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_X520_NAME
 *
 * Return value:
 *  PR_TRUE if the two objects have equal values
 *  PR_FALSE otherwise
 *  PR_FALSE upon error
 */

NSS_IMPLEMENT PRBool
nssPKIXX520Name_Equal
(
  NSSPKIXX520Name *name1,
  NSSPKIXX520Name *name2,
  PRStatus *statusOpt
)
{
  PRBool rv;

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssPKIXX520Name_verifyPointer(name1) ) {
    goto loser;
  }

  if( PR_SUCCESS != nssPKIXX520Name_verifyPointer(name2) ) {
    goto loser;
  }
#endif /* NSSDEBUG */

  if( (NSSUTF8 *)NULL == name1->utf8 ) {
    if( PR_SUCCESS != nss_pkix_X520Name_DoUTF8(name1) ) {
      goto loser;
    }
  }

  if( (NSSUTF8 *)NULL == name2->utf8 ) {
    if( PR_SUCCESS != nss_pkix_X520Name_DoUTF8(name2) ) {
      goto loser;
    }
  }

  if( (PR_TRUE == name1->wasPrintable) && (PR_TRUE == name2->wasPrintable) ) {
    return nssUTF8_PrintableMatch(name1->utf8, name2->utf8, statusOpt);
  }

  return nssUTF8_Equal(name1->utf8, name2->utf8, statusOpt);

 loser:
  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_FAILURE;
  }

  return PR_FALSE;
}
