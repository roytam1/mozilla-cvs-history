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
 * The Original Code is the Mozilla toolkit.
 *
 * The Initial Developer of the Original Code is
 * Benjamin Smedberg <benjamin@smedbergs.us>
 *
 * Portions created by the Initial Developer are Copyright (C) 2004
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
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsICommandLineRunner.h"

#include "nsICategoryManager.h"
#include "nsICommandLineHandler.h"
#include "nsIFile.h"
#include "nsISimpleEnumerator.h"
#include "nsIStringEnumerator.h"

#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsISupportsImpl.h"
#include "nsNativeCharsetUtils.h"
#include "nsNetUtil.h"
#include "nsUnicharUtils.h"
#include "nsVoidArray.h"
#include "nsXPCOMCID.h"
#include "plstr.h"

#ifdef XP_MACOSX
#include <CFURL.h>
#include "nsILocalFileMac.h"
#elif defined(XP_WIN)
#include <windows.h>
#include <shlobj.h>
#elif defined(XP_BEOS)
#include <Path.h>
#elif defined(XP_UNIX)
#include <unistd.h>
#endif

#ifdef DEBUG_bsmedberg
#define DEBUG_COMMANDLINE
#endif

class nsCommandLine : public nsICommandLineRunner
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOMMANDLINE
  NS_DECL_NSICOMMANDLINERUNNER

  nsCommandLine();

protected:
  ~nsCommandLine() { }

  void appendArg(const char* arg);
  nsresult getHandlers(nsCStringArray& handlers, nsICategoryManager* catman);

  nsStringArray     mArgs;
  PRUint32          mState;
  nsCOMPtr<nsIFile> mWorkingDir;
  PRBool            mPreventDefault;
};

nsCommandLine::nsCommandLine() :
  mState(STATE_INITIAL_LAUNCH),
  mPreventDefault(PR_FALSE)
{

}


NS_IMPL_ISUPPORTS2(nsCommandLine,
                   nsICommandLine,
                   nsICommandLineRunner)

NS_IMETHODIMP
nsCommandLine::GetLength(PRInt32 *aResult)
{
  *aResult = mArgs.Count();
  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::GetArgument(PRInt32 aIndex, nsAString& aResult)
{
  NS_ENSURE_ARG_MIN(aIndex, 0);
  NS_ENSURE_ARG_MAX(aIndex, mArgs.Count());

  mArgs.StringAt(aIndex, aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::FindFlag(const nsAString& aFlag, PRBool aCaseSensitive, PRInt32 *aResult)
{
  NS_ENSURE_ARG(!aFlag.IsEmpty());

  PRInt32 f;

  nsDefaultStringComparator caseCmp;
  nsCaseInsensitiveStringComparator caseICmp;
  nsStringComparator& c = aCaseSensitive ?
    NS_STATIC_CAST(nsStringComparator&, caseCmp) :
    NS_STATIC_CAST(nsStringComparator&, caseICmp);

  for (f = 0; f < mArgs.Count(); ++f) {
    const nsString &arg = *mArgs[f];

    if (arg.Length() >= 2 && arg.First() == PRUnichar('-')) {
      if (aFlag.Equals(Substring(arg, 1), c)) {
        *aResult = f;
        return NS_OK;
      }
    }
  }

  *aResult = -1;
  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::RemoveArguments(PRInt32 aStart, PRInt32 aEnd)
{
  NS_ENSURE_ARG_MIN(aStart, 0);
  NS_ENSURE_ARG_MAX(aEnd, mArgs.Count() - 1);

  for (PRInt32 i = aEnd; i >= aStart; --i) {
    mArgs.RemoveStringAt(i);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::HandleFlag(const nsAString& aFlag, PRBool aCaseSensitive,
                          PRBool *aResult)
{
  nsresult rv;

  PRInt32 found;
  rv = FindFlag(aFlag, aCaseSensitive, &found);
  NS_ENSURE_SUCCESS(rv, rv);

  if (found == -1) {
    *aResult = PR_FALSE;
    return NS_OK;
  }

  *aResult = PR_TRUE;
  RemoveArguments(found, found);

  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::HandleFlagWithParam(const nsAString& aFlag, PRBool aCaseSensitive,
                                   nsAString& aResult)
{
  nsresult rv;

  PRInt32 found;
  rv = FindFlag(aFlag, aCaseSensitive, &found);
  NS_ENSURE_SUCCESS(rv, rv);

  if (found == -1) {
    aResult.SetIsVoid(PR_TRUE);
    return NS_OK;
  }

  if (found == mArgs.Count() - 1) {
    return NS_ERROR_INVALID_ARG;
  }

  ++found;

  if (mArgs[found]->First() == '-') {
    return NS_ERROR_INVALID_ARG;
  }

  mArgs.StringAt(found, aResult);
  RemoveArguments(found - 1, found);

  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::GetState(PRUint32 *aResult)
{
  *aResult = mState;
  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::GetPreventDefault(PRBool *aResult)
{
  *aResult = mPreventDefault;
  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::SetPreventDefault(PRBool aValue)
{
  mPreventDefault = aValue;
  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::GetWorkingDirectory(nsIFile* *aResult)
{
  NS_ENSURE_TRUE(mWorkingDir, NS_ERROR_NOT_INITIALIZED);

  NS_ADDREF(*aResult = mWorkingDir);
  return NS_OK;
}

NS_IMETHODIMP
nsCommandLine::ResolveFile(const nsAString& aArgument, nsIFile* *aResult)
{
  NS_ENSURE_TRUE(mWorkingDir, NS_ERROR_NOT_INITIALIZED);

  // This is some seriously screwed-up code. nsILocalFile.appendRelativeNativePath
  // explicitly does not accept .. or . path parts, but that is exactly what we
  // need here. So we hack around it.

  nsresult rv;

#if defined(XP_MACOSX)
  nsCOMPtr<nsILocalFileMac> lfm (do_QueryInterface(mWorkingDir));
  NS_ENSURE_TRUE(lfm, NS_ERROR_NO_INTERFACE);

  nsCOMPtr<nsILocalFileMac> newfile (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
  NS_ENSURE_TRUE(newfile, NS_ERROR_OUT_OF_MEMORY);

  CFURLRef baseurl;
  rv = lfm->GetCFURL(&baseurl);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString path;
  NS_CopyUnicodeToNative(aArgument, path);

  CFURLRef newurl =
    CFURLCreateFromFileSystemRepresentationRelativeToBase(NULL, (const UInt8*) path.get(),
                                                          path.Length(),
                                                          true, baseurl);

  CFRelease(baseurl);

  rv = newfile->InitWithCFURL(newurl);
  CFRelease(newurl);
  if (NS_FAILED(rv)) return rv;

  NS_ADDREF(*aResult = newfile);
  return NS_OK;

#elif defined(XP_BEOS)
  nsCOMPtr<nsILocalFile> lf (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
  NS_ENSURE_TRUE(lf, NS_ERROR_OUT_OF_MEMORY);

  if (aArgument.First() == '/') {
    // absolute path
    rv = lf->InitWithPath(aArgument);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = lf);
    return NS_OK;
  }

  nsCAutoString carg;
  NS_CopyUnicodeToNative(aArgument, carg);

  nsCAutoString wd;
  rv = mWorkingDir->GetNativePath(wd);
  NS_ENSURE_SUCCESS(rv, rv);

  BDirectory bwd(wd.get());

  BPath resolved(bwd, carg.get(), true);
  if (resolved.InitCheck() != B_OK)
    return NS_ERROR_FAILURE;

  rv = lf->InitWithNativePath(BPath.Path());
  if (NS_FAILED(rv)) return rv;

  NS_ADDREF(*aResult = lf);
  return NS_OK;

#elif defined(XP_UNIX)
  nsCOMPtr<nsILocalFile> lf (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
  NS_ENSURE_TRUE(lf, NS_ERROR_OUT_OF_MEMORY);

  if (aArgument.First() == '/') {
    // absolute path
    rv = lf->InitWithPath(aArgument);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = lf);
    return NS_OK;
  }

  nsCAutoString nativeArg;
  NS_CopyUnicodeToNative(aArgument, nativeArg);

  nsCAutoString newpath;
  mWorkingDir->GetNativePath(newpath);

  newpath.Append('/');
  newpath.Append(nativeArg);

  rv = lf->InitWithNativePath(newpath);
  if (NS_FAILED(rv)) return rv;

  rv = lf->Normalize();
  if (NS_FAILED(rv)) return rv;

  NS_ADDREF(*aResult = lf);
  return NS_OK;

#elif defined(XP_WIN32) || defined(XP_OS2)
  nsCOMPtr<nsILocalFile> lf (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
  NS_ENSURE_TRUE(lf, NS_ERROR_OUT_OF_MEMORY);

  rv = lf->InitWithPath(aArgument);
  if (NS_FAILED(rv)) {
    // If it's a relative path, the Init is *going* to fail. We use string magic and
    // win32 _fullpath. Note that paths of the form "\Relative\To\CurDrive" are
    // going to fail, and I haven't figured out a way to work around this without
    // the PathCombine() function, which is not available in plain win95/nt4

    nsCAutoString fullPath;
    mWorkingDir->GetNativePath(fullPath);

    nsCAutoString carg;
    NS_CopyUnicodeToNative(aArgument, carg);

    fullPath.Append('\\');
    fullPath.Append(carg);

    char pathBuf[MAX_PATH];
    if (!_fullpath(pathBuf, fullPath.get(), MAX_PATH))
      return NS_ERROR_FAILURE;

    rv = lf->InitWithNativePath(nsDependentCString(pathBuf));
    if (NS_FAILED(rv)) return rv;
  }
  NS_ADDREF(*aResult = lf);
  return NS_OK;

#else
#error Need platform-specific logic here.
#endif
}

NS_IMETHODIMP
nsCommandLine::ResolveURI(const nsAString& aArgument, nsIURI* *aResult)
{
  nsresult rv;

  NS_ENSURE_TRUE(mWorkingDir, NS_ERROR_NOT_INITIALIZED);

  // First, we try to init the argument as an absolute file path. If this doesn't
  // work, it is an absolute or relative URI.

  nsCOMPtr<nsIIOService> io = do_GetIOService();
  NS_ENSURE_TRUE(io, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsILocalFile> lf (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
  rv = lf->InitWithPath(aArgument);
  if (NS_SUCCEEDED(rv)) {
    lf->Normalize();
    return io->NewFileURI(lf, aResult);
  }

  nsCOMPtr<nsIURI> workingDirURI;
  rv = io->NewFileURI(mWorkingDir, getter_AddRefs(workingDirURI));
  NS_ENSURE_SUCCESS(rv, rv);

  return io->NewURI(NS_ConvertUTF16toUTF8(aArgument),
                    nsnull,
                    workingDirURI,
                    aResult);
}

void
nsCommandLine::appendArg(const char* arg)
{
#ifdef DEBUG_COMMANDLINE
  printf("Adding XP arg: %s\n", arg);
#endif

  nsAutoString warg;
  NS_CopyNativeToUnicode(nsDependentCString(arg), warg);

  mArgs.AppendString(warg);
}

NS_IMETHODIMP
nsCommandLine::Init(PRInt32 argc, char** argv, nsIFile* aWorkingDir,
                    PRUint32 aState)
{
  NS_ENSURE_ARG_MIN(aState, 0);
  NS_ENSURE_ARG_MAX(aState, 2);

  PRInt32 i;

  mWorkingDir = aWorkingDir;

  // skip argv[0], we don't want it
  for (i = 1; i < argc; ++i) {
    const char* curarg = argv[i];

#ifdef DEBUG_COMMANDLINE
    printf("Testing native arg %i: '%s'\n", i, curarg);
#endif
#if defined(XP_WIN) || defined(XP_OS2)
    if (*curarg == '/') {
      char* dup = PL_strdup(curarg);
      if (!dup) return NS_ERROR_OUT_OF_MEMORY;

      *dup = '-';
      char* colon = PL_strchr(dup, ':');
      if (colon) {
        *colon = '\0';
        appendArg(dup);
        appendArg(colon+1);
      } else {
        appendArg(dup);
      }
      PL_strfree(dup);
      continue;
    }
#endif
#ifdef XP_UNIX
    if (*curarg == '-' &&
        *(curarg+1) == '-') {
      ++curarg;

      char* dup = PL_strdup(curarg);
      if (!dup) return NS_ERROR_OUT_OF_MEMORY;

      char* eq = PL_strchr(dup, '=');
      if (eq) {
        *eq = '\0';
        appendArg(dup);
        appendArg(eq + 1);
      } else {
        appendArg(dup);
      }
      PL_strfree(dup);
      continue;
    }
#endif

    appendArg(curarg);
  }

  mState = aState;

  return NS_OK;
}

nsresult
nsCommandLine::getHandlers(nsCStringArray &handlers, nsICategoryManager* catman)
{
  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> entenum;
  rv = catman->EnumerateCategory("command-line-handler",
                                 getter_AddRefs(entenum));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUTF8StringEnumerator> strenum (do_QueryInterface(entenum));
  NS_ENSURE_TRUE(strenum, NS_ERROR_UNEXPECTED);

  nsCAutoString entry;
  PRBool hasMore;
  while (NS_SUCCEEDED(strenum->HasMore(&hasMore)) && hasMore) {
    strenum->GetNext(entry);
    handlers.AppendCString(entry);
  }

  handlers.Sort();
  return NS_OK;
}

struct RunClosure
{
  nsICommandLine* cl;
  nsICategoryManager* catman;
};

static PRBool
EnumRun(nsCString& aEntry, void* aData)
{
  nsresult rv;

  RunClosure* closure = NS_STATIC_CAST(RunClosure*, aData);

  nsXPIDLCString value;
  rv = closure->catman->GetCategoryEntry("command-line-handler",
                                         aEntry.get(),
                                         getter_Copies(value));
  NS_ENSURE_SUCCESS(rv, PR_TRUE);

  nsCOMPtr<nsICommandLineHandler> clh (do_GetService(value));
  NS_ENSURE_TRUE(clh, PR_TRUE);

  rv = clh->Handle(closure->cl);
  return rv != NS_ERROR_ABORT;
}  

NS_IMETHODIMP
nsCommandLine::Run()
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catman
    (do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  NS_ENSURE_TRUE(catman, NS_ERROR_UNEXPECTED);

  nsCStringArray handlers;
  rv = getHandlers(handlers, catman);
  NS_ENSURE_SUCCESS(rv, rv);

  RunClosure closure = { this, catman };

  if (!handlers.EnumerateForwards(&EnumRun, &closure))
    return NS_ERROR_ABORT;

  return NS_OK;
}

struct HelpClosure
{
  HelpClosure(nsACString& aText, nsICategoryManager* aCatman) :
    text(aText), catman(aCatman) { }

  nsACString& text;
  nsICategoryManager* catman;
};
  
static PRBool
EnumHelp(nsCString& aEntry, void* aData)
{
  nsresult rv;

  HelpClosure* closure = NS_STATIC_CAST(HelpClosure*, aData);

  nsXPIDLCString value;
  rv = closure->catman->GetCategoryEntry("command-line-handler",
                                         aEntry.get(),
                                         getter_Copies(value));
  NS_ENSURE_SUCCESS(rv, PR_TRUE);

  nsCOMPtr<nsICommandLineHandler> clh (do_GetService(value));
  NS_ENSURE_TRUE(clh, PR_TRUE);

  nsCString text;
  rv = clh->GetHelpInfo(text);
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(text.Length() == 0 || text.Last() == '\n',
                 "Help text from command line handlers should end in a newline.");
    closure->text.Append(text);
  }

  return PR_TRUE;
}  

NS_IMETHODIMP
nsCommandLine::GetHelpText(nsACString& aResult)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catman
    (do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  NS_ENSURE_TRUE(catman, NS_ERROR_UNEXPECTED);

  nsCStringArray handlers;
  rv = getHandlers(handlers, catman);
  NS_ENSURE_SUCCESS(rv, rv);

  HelpClosure closure (aResult, catman);

  handlers.EnumerateForwards(EnumHelp, &closure);
  return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsCommandLine)

static const nsModuleComponentInfo components[] =
{
  { "nsCommandLine",
    { 0x23bcc750, 0xdc20, 0x460b, { 0xb2, 0xd4, 0x74, 0xd8, 0xf5, 0x8d, 0x36, 0x15 } },
    "@mozilla.org/toolkit/command-line;1",
    nsCommandLineConstructor
  }
};

NS_IMPL_NSGETMODULE(CommandLineModule, components)
