/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*
	nsPluginsDirUNIX.cpp
	
	UNIX implementation of the nsPluginsDir/nsPluginsFile classes.
	
	by Alex Musil
 */

#include "nsPluginsDir.h"
#include "prlink.h"
#include "plstr.h"
#include "prmem.h"

#include "nsSpecialSystemDirectory.h"

/* Local helper functions */
 
static char* GetFileName(const char* pathname)
{
        const char* filename = nsnull;
                
        // this is most likely a path, so skip to the filename
        filename = PL_strrchr(pathname, '/');
        if(filename)
                ++filename;
        else
                filename = pathname;

        return PL_strdup(filename);
}

static PRUint32 CalculateVariantCount(char* mimeTypes)
{
        PRUint32 variants = 0;
        char* index = mimeTypes;
        while (*index)
        {
                if (*index == ';')
                        variants++;

                ++index; 
        }
        return variants;
}

static char** MakeStringArray(PRUint32 variants, char* data)
{
        char** buffer = NULL;
        char* index = data;
        PRUint32 count = 0;
 
        buffer = (char **)PR_Malloc(variants * sizeof(char *));
        buffer[count++] = index;
 
        while (*index && count<variants)
        {
                if (*index == '|')
                {
                  buffer[count++] = index + 1;
                  *index = 0;
                }
                ++index; 
        }
        return buffer;  
}

///////////////////////////////////////////////////////////////////////////

/* nsPluginsDir implementation */

nsPluginsDir::nsPluginsDir()
{
  // this is somewhat lacking, in that it doesn't fall back to any other directories.
  // then again, I'm not sure we should be falling back at all.  plugins have been (and probably
  // should continue to be) loaded from both <libdir>/plugins and ~/.mozilla/plugins.  There
  // doesn't seem to be any way to do this in the current nsPluginsDir code, which is disheartening.
  //

  // use MOZILLA_FIVE_HOME/plugins

  nsSpecialSystemDirectory sysdir(nsSpecialSystemDirectory::OS_CurrentProcessDirectory); 
  sysdir += "plugins"; 
  const char *pluginsDir = sysdir.GetCString(); // native path
  if (pluginsDir != NULL)
  {
      const char* allocPath;
      
      allocPath = PL_strdup(pluginsDir);
      *(nsFileSpec*)this = allocPath;
  }
}

nsPluginsDir::~nsPluginsDir()
{
	// do nothing
}

PRBool nsPluginsDir::IsPluginFile(const nsFileSpec& fileSpec)
{
    const char* pathname = fileSpec.GetCString();

#ifdef NS_DEBUG
	printf("IsPluginFile(%s)\n", pathname);
#endif

	return PR_TRUE;
}

///////////////////////////////////////////////////////////////////////////

/* nsPluginFile implementation */

nsPluginFile::nsPluginFile(const nsFileSpec& spec)
	:	nsFileSpec(spec)
{
	// nada
}

nsPluginFile::~nsPluginFile()
{
	// nada
}

/**
 * Loads the plugin into memory using NSPR's shared-library loading
 * mechanism. Handles platform differences in loading shared libraries.
 */
nsresult nsPluginFile::LoadPlugin(PRLibrary* &outLibrary)
{
        const char* path = this->GetCString();
        pLibrary = outLibrary = PR_LoadLibrary(path);

#ifdef NS_DEBUG
        printf("LoadPlugin() %s returned %lx\n",path,pLibrary);
#endif

        return NS_OK;
}

typedef char* (*UNIX_Plugin_GetMIMEDescription)();


/**
 * Obtains all of the information currently available for this plugin.
 */
nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info)
{
	const char *path = this->GetCString();
    char *mimedescr,*mdesc,*start,*nexttoc,*mtype,*exten,*descr;
    int i,num;

    UNIX_Plugin_GetMIMEDescription procedure = nsnull;
    mimedescr="";

    if(procedure = (UNIX_Plugin_GetMIMEDescription)PR_FindSymbol(pLibrary,"NP_GetMIMEDescription")) {
        mimedescr = procedure();
    } else {
#ifdef NS_DEBUG
        printf("Cannot get plugin info: no GetMIMEDescription procedure!\n");
#endif
        return NS_ERROR_FAILURE;
    }

	info.fName = GetFileName(path);

#ifdef NS_DEBUG
    printf("GetMIMEDescription() %lx returned \"%s\"\n",procedure,mimedescr);
#endif

    // copy string
    
    mdesc = (char *)PR_Malloc(strlen(mimedescr)+1);
    strcpy(mdesc,mimedescr);
    num=CalculateVariantCount(mimedescr)+1;
    info.fVariantCount = num;

    info.fMimeTypeArray =(char **)PR_Malloc(num * sizeof(char *));
    info.fMimeDescriptionArray =(char **)PR_Malloc(num * sizeof(char *));
    info.fExtensionArray =(char **)PR_Malloc(num * sizeof(char *));

    start=mdesc;
    for(i=0;i<num && *start;i++) {
        // search start of next token (separator is ';')

        if(i+1<num) {
            if(nexttoc=PL_strchr(start, ';'))
                *nexttoc++=0;
            else
                nexttoc=start+strlen(start);
        } else
            nexttoc=start+strlen(start);

        // split string into: mime type ':' extensions ':' description

        mtype=start;
        exten=PL_strchr(start, ':');
        if(exten) {
            *exten++=0;
            descr=PL_strchr(exten, ':');
        } else
            descr=NULL;

        if(descr)
            *descr++=0;

#ifdef NS_DEBUG
        printf("Registering plugin for: \"%s\",\"%s\",\"%s\"\n", mtype,descr ? descr : "null",exten ? exten : "null");
#endif

        if(i==0) {
            info.fMimeType = mtype ? mtype : "";
            info.fMimeDescription = descr ? descr : "";
            info.fExtensions = exten ? exten : "";
        }

        if(!*mtype && !descr && !exten) {
            i--;
            info.fVariantCount--;
        } else {
            info.fMimeTypeArray[i] = mtype ? mtype : "";
            info.fMimeDescriptionArray[i] = descr ? descr : "";
            info.fExtensionArray[i] = exten ? exten : "";
        }
        start=nexttoc;
    }
	return NS_OK;
}


