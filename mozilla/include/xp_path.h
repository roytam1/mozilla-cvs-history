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

/*
 * The contents of this file used to be part of prosdep.h,
 * a private NSPR header file of the NSPR module.  The macros
 * defined in this file are the only things the Mozilla
 * client needs from prosdep.h, so I extracted them and put
 * them in this new file.
 *   -- Wan-Teh Chang <wtc@netscape.com>, 18 September 1998
 */

#ifndef _XP_PATH_H
#define _XP_PATH_H

#if defined(XP_UNIX) || defined(XP_MAC) || defined(XP_BEOS)
#define PR_DIRECTORY_SEPARATOR '/'
#define PR_DIRECTORY_SEPARATOR_STR "/"
#define PR_PATH_SEPARATOR ':'
#define PR_PATH_SEPARATOR_STR ":"
#elif defined(XP_PC) /* Win32, Win16, and OS/2 */
#define PR_DIRECTORY_SEPARATOR '\\'
#define PR_DIRECTORY_SEPARATOR_STR "\\"
#define PR_PATH_SEPARATOR ';'
#define PR_PATH_SEPARATOR_STR ";"
#else
#error "Unknown platform"
#endif

#ifdef XP_MAC
#define PATH_SEPARATOR_STR ":"
#define DIRECTORY_SEPARATOR_STR "/"
#define MAXPATHLEN 512
#endif

#endif /* _XP_PATH_H */
