/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 *   Alec Flett <alecf@netscape.com>
 *   Darin Fisher <darin@netscape.com>
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

/* Windows-specific local file uri parsing */
#include "nsIOService.h"
#include "nsEscape.h"
#include "nsPrintfCString.h"
#include "nsILocalFile.h"
#include <windows.h>

NS_IMETHODIMP
nsIOService::GetURLSpecFromFile(nsIFile *aFile, nsACString &result)
{
    nsresult rv;
    nsXPIDLCString ePath;
  
    rv = aFile->GetPath(getter_Copies(ePath));
    if (NS_FAILED(rv)) return rv;
  
    // Replace \ with / to convert to an url
    for (char *s = (char *) ePath.get(); *s; ++s) {
        // We need to call IsDBCSLeadByte because
        // Japanese windows can have 0x5C in the second byte 
        // of a Japanese character, for example 0x8F 0x5C is
        // one Japanese character
        if(::IsDBCSLeadByte(*s) && *(s+1))
            ++s;
        else if (*s == '\\')
            *s = '/';
    }

    nsCAutoString escPath;
    NS_NAMED_LITERAL_CSTRING(prefix, "file:///");
  
    // Escape the path with the directory mask
    if (NS_EscapeURL(ePath.get(), ePath.Length(), esc_Directory+esc_Forced, escPath))
        escPath.Insert(prefix, 0);
    else
        escPath.Assign(prefix + ePath);

    // XXX this should be unnecessary
    if (escPath[escPath.Length() - 1] != '/') {
        PRBool dir;
        rv = aFile->IsDirectory(&dir);
        if (NS_FAILED(rv))
            NS_WARNING(PromiseFlatCString(
                NS_LITERAL_CSTRING("Cannot tell if ") +
                escPath + NS_LITERAL_CSTRING(" is a directory or file")
            ).get());
        else if (dir) {
            // make sure we have a trailing slash
            escPath += "/";
        }
    }
    
    result = escPath;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::InitFileFromURLSpec(nsIFile *aFile, const nsACString &aURL)
{
    nsresult rv;
    
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(aFile, &rv);
    if (NS_FAILED(rv)) {
        NS_ERROR("Only nsILocalFile supported right now");
        return rv;
    }
    
    nsCAutoString directory, fileBaseName, fileExtension;
    
    rv = ParseFileURL(aURL, directory, fileBaseName, fileExtension);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString path;

    if (!directory.IsEmpty()) {
        NS_EscapeURL(directory, esc_Directory|esc_AlwaysCopy, path);
		if (path.Length() > 2 && path.CharAt(2) == '|')
			path.SetCharAt(':', 2);
		path.ReplaceChar('/', '\\');
    }    
    if (!fileBaseName.IsEmpty())
        NS_EscapeURL(fileBaseName, esc_FileBaseName|esc_AlwaysCopy, path);
    if (!fileExtension.IsEmpty()) {
        path += '.';
        NS_EscapeURL(fileExtension, esc_FileExtension|esc_AlwaysCopy, path);
    }
    
    NS_UnescapeURL((char *) path.get());

    // remove leading '\'
    if (path.CharAt(0) == '\\')
        path.Cut(0, 1);

    return localFile->InitWithPath(path.get());
}
