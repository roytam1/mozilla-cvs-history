/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef nsImportMailboxDescriptor_h___
#define nsImportMailboxDescriptor_h___

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsIImportMailboxDescriptor.h"
#include "nsILocalFile.h"
#include "nsCOMPtr.h"

////////////////////////////////////////////////////////////////////////


class nsImportMailboxDescriptor : public nsIImportMailboxDescriptor
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD  GetIdentifier( PRUint32 *pIdentifier) { *pIdentifier = m_id; return( NS_OK);}
  NS_IMETHOD  SetIdentifier( PRUint32 ident) { m_id = ident; return( NS_OK);}

  /* attribute unsigned long depth; */
  NS_IMETHOD  GetDepth( PRUint32 *pDepth) { *pDepth = m_depth; return( NS_OK);}
  NS_IMETHOD  SetDepth( PRUint32 theDepth) { m_depth = theDepth; return( NS_OK);}

  /* attribute unsigned long size; */
  NS_IMETHOD  GetSize( PRUint32 *pSize) { *pSize = m_size; return( NS_OK);}
  NS_IMETHOD  SetSize( PRUint32 theSize) { m_size = theSize; return( NS_OK);}

  /* attribute wstring displayName; */
  NS_IMETHOD  GetDisplayName( PRUnichar **pName) { *pName = ToNewUnicode(m_displayName); return( NS_OK);}
  NS_IMETHOD  SetDisplayName( const PRUnichar * pName) { m_displayName = pName; return( NS_OK);}

  /* attribute boolean import; */
  NS_IMETHOD  GetImport( PRBool *pImport) { *pImport = m_import; return( NS_OK);}
  NS_IMETHOD  SetImport( PRBool doImport) { m_import = doImport; return( NS_OK);}

  /* readonly attribute nsILocalFile file; */
  NS_IMETHOD GetFile(nsILocalFile * *aFile) { if (m_pFile) { NS_ADDREF(*aFile = m_pFile); return( NS_OK);} else return( NS_ERROR_FAILURE); }



  nsImportMailboxDescriptor();
  virtual ~nsImportMailboxDescriptor() {}

   static NS_METHOD Create( nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
  PRUint32    m_id;      // used by creator of the structure
  PRUint32    m_depth;    // depth in the heirarchy
  nsString    m_displayName;// name of this mailbox
  nsCOMPtr <nsILocalFile> m_pFile;  // source file (if applicable)
  PRUint32    m_size;
  PRBool      m_import;    // import it or not?
};


#endif
