/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsPluginsDir_h___
#define nsPluginsDir_h___

#include "nsError.h"
#include "nsFileSpec.h"

/**
 * Provides a cross-platform way to obtain the location of the plugins
 * directory. Once constructed, can be used with nsDirectoryIterator to
 * scan the plugins directory. An nsPluginFileSpec can be constructed from the
 * nsFileSpec returned by the iterator.
 */
class nsPluginsDir : public nsFileSpec {
public:
	/**
	 * Locates the plugins directory in a platform-dependent manner.
	 */
	nsPluginsDir();
	virtual ~nsPluginsDir();

	/**
	 * Determines whether or not the given file is actually a plugin file.
	 */
	PRBool IsPluginFile(const nsFileSpec& fileSpec);
};

struct PRLibrary;

struct nsPluginInfo {
	PRUint32 fPluginInfoSize;	// indicates how large the structure is currently.
	char* fName;				// name of the plugin
	char* fDescription;			// etc.
	char* fMimeType;
	char* fMimeDescription;
	char* fExtensions;
	PRUint32 fVariantCount;
	char** fMimeTypeArray;
	char** fMimeDescriptionArray;
	char** fExtensionArray;
	char* fFileName;
};

/**
 * Provides cross-platform access to a plugin file. Deals with reading
 * properties from the plugin file, and loading the plugin's shared
 * library. Insulates core nsIPluginHost implementations from these
 * details.
 */
class nsPluginFile : public nsFileSpec {
  PRLibrary* pLibrary;
public:
	/**
	 * If spec corresponds to a valid plugin file, constructs a reference
	 * to a plugin file on disk. Plugins are typically located using the
	 * nsPluginsDir class.
	 */
	nsPluginFile(const nsFileSpec& spec);
	virtual ~nsPluginFile();

	/**
	 * Loads the plugin into memory using NSPR's shared-library loading
	 * mechanism. Handles platform differences in loading shared libraries.
	 */
	nsresult LoadPlugin(PRLibrary* &outLibrary);

	/**
	 * Obtains all of the information currently available for this plugin.
	 */
	nsresult GetPluginInfo(nsPluginInfo &outPluginInfo);
};

#endif /* nsPluginsDir_h___ */
