/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Andreas Otte.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef nsURLHelper_h__
#define nsURLHelper_h__

#include "nsString.h"

class nsIFile;
class nsIURLParser;

//----------------------------------------------------------------------------
// This module contains some private helper functions related to URL parsing.
//----------------------------------------------------------------------------

/* shutdown frees URL parser */
void net_ShutdownURLHelper();

/* access URL parsers */
nsIURLParser *net_GetAuthURLParser();
nsIURLParser *net_GetNoAuthURLParser();
nsIURLParser *net_GetStdURLParser();

/* convert between nsIFile and file:// URL spec */
nsresult net_GetURLSpecFromFile(nsIFile *, nsACString &);
nsresult net_GetFileFromURLSpec(const nsACString &, nsIFile **);

/* extract file path components from file:// URL */
nsresult net_ParseFileURL(const nsACString &inURL,
                          nsACString &outDirectory,
                          nsACString &outFileBaseName,
                          nsACString &outFileExtension);

/* handle .. in dirs while resolving relative URLs */
void net_CoalesceDirsRel(char* io_Path);

/* handle .. in dirs while resolving absolute URLs */
void net_CoalesceDirsAbs(char* io_Path);

/**
 * Resolves a relative path string containing "." and ".."
 * with respect to a base path (assumed to already be resolved). 
 * For example, resolving "../../foo/./bar/../baz.html" w.r.t.
 * "/a/b/c/d/e/" yields "/a/b/c/foo/baz.html". Attempting to 
 * ascend above the base results in the NS_ERROR_MALFORMED_URI
 * exception. If basePath is null, it treats it as "/".
 *
 * @param relativePath  a relative URI
 * @param basePath      a base URI
 *
 * @return a new string, representing canonical uri
 */
nsresult net_ResolveRelativePath(const nsACString &relativePath,
                                 const nsACString &basePath,
                                 nsACString &result);

/**
 * Extract URI-Scheme if possible
 *
 * @param inURI     URI spec
 * @param startPos  start of scheme (may be null)
 * @param endPos    end of scheme; index of colon (may be null)
 * @param scheme    scheme copied to this buffer on return (may be null)
 */
nsresult net_ExtractURLScheme(const nsACString &inURI,
                              PRUint32 *startPos, 
                              PRUint32 *endPos,
                              nsACString *scheme = nsnull);

/* check that the given scheme conforms to RFC 2396 */
PRBool net_IsValidScheme(const char *scheme, PRUint32 schemeLen);

inline PRBool net_IsValidScheme(const nsAFlatCString &scheme)
{
    return net_IsValidScheme(scheme.get(), scheme.Length());
}

/* convert to lower case (XXX this needs to be factored out) */
void net_ToLowerCase(char* str, PRUint32 length);
void net_ToLowerCase(char* str);

#endif // !nsURLHelper_h__
