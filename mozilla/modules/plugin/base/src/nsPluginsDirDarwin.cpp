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
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Patrick C. Beard <beard@netscape.com>
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
  nsPluginsDirDarwin.cpp
  
  Mac OS X implementation of the nsPluginsDir/nsPluginsFile classes.
  
  by Patrick C. Beard.
 */

#include "nsPluginsDir.h"
#include "prlink.h"

#include <Processes.h>
#include <Folders.h>
#include <Resources.h>
#include <TextUtils.h>
#include <Aliases.h>
#include <string.h>
#if TARGET_CARBON && (UNIVERSAL_INTERFACES_VERSION < 0x0340)
enum {
    kLocalDomain                  = -32765, /* All users of a single machine have access to these resources.*/
    kUserDomain                   = -32763, /* Read/write. Resources that are private to the user.*/
    kClassicDomain                = -32762 /* Domain referring to the currently configured Classic System Folder*/
};
#endif

#if TARGET_CARBON
#include <CFURL.h>
#include <CFBundle.h>
#include <CFString.h>
#include <CodeFragments.h>

/*
** Returns a CFBundleRef if the FSSpec refers to a Mac OS X bundle directory.
** The caller is responsible for calling CFRelease() to deallocate.
*/
static CFBundleRef getPluginBundle(const FSSpec& spec)
{
    CFBundleRef bundle = NULL;
    FSRef ref;
    OSErr err = FSpMakeFSRef(&spec, &ref);
    char path[512];
    if (err == noErr && (UInt32(FSRefMakePath) != kUnresolvedCFragSymbolAddress)) {
        err = FSRefMakePath(&ref, (UInt8*)path, sizeof(path) - 1);
        if (err == noErr) {
            CFStringRef pathRef = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
            if (pathRef) {
                CFURLRef bundleURL = CFURLCreateWithFileSystemPath(NULL, pathRef, kCFURLPOSIXPathStyle, true);
                if (bundleURL != NULL) {
                    bundle = CFBundleCreate(NULL, bundleURL);
                    CFRelease(bundleURL);
                }
                CFRelease(pathRef);
            }
        }
    }
    return bundle;
}

extern "C" {
    // Not yet in Universal Interfaces that I'm using.
    EXTERN_API_C( SInt16 )
        CFBundleOpenBundleResourceMap(CFBundleRef bundle);

    EXTERN_API_C( void )
        CFBundleGetPackageInfo(CFBundleRef bundle, UInt32 * packageType, UInt32 * packageCreator);
}

#endif

static nsresult getApplicationSpec(FSSpec& outAppSpec)
{
    // Use the process manager to get the application's FSSpec,
    // then construct an nsFileSpec that encapsulates it.
    ProcessInfoRec info;
    info.processInfoLength = sizeof(info);
    info.processName = NULL;
    info.processAppSpec = &outAppSpec;
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    OSErr result = GetProcessInformation(&psn, &info);
    return (result == noErr ? NS_OK : NS_ERROR_FAILURE);
}

static OSErr toFSSpec(const nsFileSpec& inFileSpec, FSSpec& outSpec)
{
    FSRef ref;
    OSErr err = FSPathMakeRef((const UInt8*)inFileSpec.GetCString(), &ref, NULL);
    if (err == noErr)
        err = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, &outSpec, NULL);
    return err;
}

#undef printf

PRBool nsPluginsDir::IsPluginFile(const nsFileSpec& fileSpec)
{
    printf("nsPluginsDir::IsPluginFile:  checking %s\n", fileSpec.GetCString());
    // look at file's creator/type and make sure it is a code fragment, etc.
    FSSpec spec;
    OSErr err = toFSSpec(fileSpec, spec);
    if (err != noErr)
        return PR_FALSE;

    FInfo info;
    err = FSpGetFInfo(&spec, &info);
    if (err == noErr && ((info.fdType == 'shlb' && info.fdCreator == 'MOSS') ||
                         info.fdType == 'NSPL')) {
        printf("found plugin '%s'.\n", fileSpec.GetCString());
        return PR_TRUE;
    }

#if TARGET_CARBON
    // Some additional plugin types for Carbon/Mac OS X
    if (err == noErr && (info.fdType == 'BRPL' || info.fdType == 'IEPL'))
        return PR_TRUE;

  // for Mac OS X bundles.
    CFBundleRef bundle = getPluginBundle(spec);
    if (bundle) {
        UInt32 packageType, packageCreator;
        CFBundleGetPackageInfo(bundle, &packageType, &packageCreator);
        CFRelease(bundle);
        switch (packageType) {
        case 'BRPL':
        case 'IEPL':
        case 'NSPL':
            return PR_TRUE;
        }
    }
#endif

    return PR_FALSE;
}

nsPluginFile::nsPluginFile(const nsFileSpec& spec)
    : nsFileSpec(spec)
{
}

nsPluginFile::~nsPluginFile() {}

/**
 * Loads the plugin into memory using NSPR's shared-library loading
 * mechanism. Handles platform differences in loading shared libraries.
 */
nsresult nsPluginFile::LoadPlugin(PRLibrary* &outLibrary)
{
    const char* path = this->GetCString();
    outLibrary = PR_LoadLibrary(path);
    return NS_OK;
}

static char* p2cstrdup(StringPtr pstr)
{
    int len = pstr[0];
    char* cstr = new char[len + 1];
    if (cstr != NULL) {
        ::BlockMoveData(pstr + 1, cstr, len);
        cstr[len] = '\0';
    }
    return cstr;
}

static char* GetPluginString(short id, short index)
{
    Str255 str;
    ::GetIndString(str, id, index);
    return p2cstrdup(str);
}

// Opens the resource fork for the plugin
// Also checks if the plugin is a CFBundle and opens gets the correct resource
short nsPluginFile::OpenPluginResource()
{
    FSSpec spec;
    OSErr err = toFSSpec(*this, spec);
    Boolean targetIsFolder, wasAliased;
    err = ::ResolveAliasFile(&spec, true, &targetIsFolder, &wasAliased);
    short refNum = ::FSpOpenResFile(&spec, fsRdPerm);
  
#if TARGET_CARBON
    if (refNum == -1) {
        CFBundleRef bundle = getPluginBundle(spec);
        if (bundle) {
            refNum = CFBundleOpenBundleResourceMap(bundle);
            CFRelease(bundle);
        }
    }
#endif
  
    return refNum;
}

/**
 * Obtains all of the information currently available for this plugin.
 */
nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info)
{
    // clear out the info, except for the first field.
    memset(&info.fName, 0, sizeof(info) - sizeof(PRUint32));

    // need to open the plugin's resource file and read some resources.
    short refNum = OpenPluginResource();

    if (refNum != -1) {
        if (info.fPluginInfoSize >= sizeof(nsPluginInfo)) {
            // 'STR#', 126, 2 => plugin name.
            info.fName = GetPluginString(126, 2);
      
            // 'STR#', 126, 1 => plugin description.
            info.fDescription = GetPluginString(126, 1);
      
            // Determine how many  'STR#' resource for all MIME types/extensions.
            Handle typeList = ::Get1Resource('STR#', 128);
            if (typeList != NULL) {
                short stringCount = **(short**)typeList;
                info.fVariantCount = stringCount / 2;
                ::ReleaseResource(typeList);
            }

            FSSpec spec;
            OSErr err = toFSSpec(*this, spec);
            int variantCount = info.fVariantCount;
            info.fMimeTypeArray = new char*[variantCount];
            info.fMimeDescriptionArray = new char*[variantCount];
            info.fExtensionArray = new char*[variantCount];
            info.fFileName = p2cstrdup(spec.name);
            info.fFullPath = PL_strdup(this->GetCString());
      
#if TARGET_CARBON
            CFBundleRef bundle = getPluginBundle(spec);
            if (bundle) {
                info.fBundle = PR_TRUE;
                CFRelease(bundle);
            } else
                info.fBundle = PR_FALSE;
#endif

            short mimeIndex = 1, descriptionIndex = 1;
            for (int i = 0; i < variantCount; i++) {
                info.fMimeTypeArray[i] = GetPluginString(128, mimeIndex++);
                info.fExtensionArray[i] = GetPluginString(128, mimeIndex++);
                info.fMimeDescriptionArray[i] = GetPluginString(127, descriptionIndex++);
            }
        }
    
        ::CloseResFile(refNum);
    }
    return NS_OK;
}

nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
    if (info.fPluginInfoSize <= sizeof(nsPluginInfo)) 
        {
            delete[] info.fName;
            delete[] info.fDescription;
            int variantCount = info.fVariantCount;
            for (int i = 0; i < variantCount; i++) 
                {
                    delete[] info.fMimeTypeArray[i];
                    delete[] info.fExtensionArray[i];
                    delete[] info.fMimeDescriptionArray[i];
                }
            delete[] info.fMimeTypeArray;
            delete[] info.fMimeDescriptionArray;
            delete[] info.fExtensionArray;
            delete[] info.fFileName;
            delete[] info.fFullPath;
        }
    return NS_OK;
}
