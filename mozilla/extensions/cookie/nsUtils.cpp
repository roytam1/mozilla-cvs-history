/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsUtils.h"
#include "nsXPIDLString.h"
#include "nsIStringBundle.h"
#include "nsIPref.h"
#include "nsIFileSpec.h"
#include "nsILocalFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "prmem.h"
#include "nsComObsolete.h"

static NS_DEFINE_IID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

#define LOCALIZATION "chrome://cookie/locale/cookie.properties"

nsresult
ckutil_getChar(nsInputFileStream& strm, 
               char *buffer, PRInt32 bufsize, 
               PRInt32& next, PRInt32& count, 
               char& c) {

  if (next == count) {
    if (bufsize > count) { // never say "count < ..." vc6.0 thinks this is a template beginning and crashes
      next = bufsize;
      count = bufsize;
      return NS_ERROR_FAILURE;
    }
    count = strm.read(buffer, bufsize);
    next = 0;
    if (count == 0) {
      next = bufsize;
      count = bufsize;
      return NS_ERROR_FAILURE;
    }
  }
  c = buffer[next++];
  return NS_OK;
}

/*
 * get a line from a file
 * return -1 if end of file reached
 * strip carriage returns and line feeds from end of line
 */
PUBLIC PRInt32
CKutil_GetLine(nsInputFileStream& strm, char *buffer, PRInt32 bufsize,
               PRInt32& next, PRInt32& count, nsACString& aLine) {

  /* read the line */
  aLine.Truncate();
  char c;
  for (;;) {
    if NS_FAILED(ckutil_getChar(strm, buffer, bufsize, next, count, c)) {
      return -1;
    }
    if (c == '\n') {
      break;
    }

    if (c != '\r') {
      aLine.Append(c);
    }
  }
  return 0;
}

PRUnichar *
CKutil_Localize(const PRUnichar *genericString) {
  nsresult ret;
  PRUnichar *ptrv = nsnull;
  nsCOMPtr<nsIStringBundleService> pStringService = 
           do_GetService(kStringBundleServiceCID, &ret); 
  if (NS_SUCCEEDED(ret) && (nsnull != pStringService)) {
    nsCOMPtr<nsIStringBundle> bundle;
    ret = pStringService->CreateBundle(LOCALIZATION, getter_AddRefs(bundle));
    if (NS_SUCCEEDED(ret) && bundle) {
      ret = bundle->GetStringFromName(genericString, &ptrv);
      if ( NS_SUCCEEDED(ret) && (ptrv) ) {
        return ptrv;
      }
    }
  }
  return nsCRT::strdup(genericString);
}

PUBLIC nsresult
CKutil_ProfileDirectory(nsFileSpec& dirSpec) {
  nsresult res;
  nsCOMPtr<nsIFile> aFile;
  nsCOMPtr<nsIFileSpec> tempSpec;
  
  res = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(aFile));
  if (NS_FAILED(res)) return res;
  
  // TODO: When the calling code can take an nsIFile,
  // this conversion to nsFileSpec can be avoided. 
  res = NS_NewFileSpecFromIFile(aFile, getter_AddRefs(tempSpec));
  if (NS_FAILED(res)) return res;
  res = tempSpec->GetFileSpec(&dirSpec);
  
  return res;
}

PUBLIC char *
CKutil_StrAllocCopy(char *&destination, const char *source) {
  if(destination) {
    PL_strfree(destination);
    destination = 0;
  }
  destination = PL_strdup(source);
  return destination;
}

PUBLIC char *
CKutil_StrAllocCat(char *&destination, const char *source) {
  if (source && *source) {
    if (destination) {
      int length = PL_strlen (destination);
      destination = (char *) PR_Realloc(destination, length + PL_strlen(source) + 1);
      if (destination == NULL) {
        return(NULL);
      }
      PL_strcpy (destination + length, source);
    } else {
      destination = PL_strdup(source);
    }
  }
  return destination;
}
