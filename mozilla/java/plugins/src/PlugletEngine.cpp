/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Sun Microsystems,
 * Inc. Portions created by Sun are
 * Copyright (C) 1999 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsIGenericFactory.h"
#include "nsXPCOMCID.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsFileSpec.h"
#include "nsICategoryManager.h"
#include "nsObserverService.h"
#include "nsMemory.h"

#include "PlugletEngine.h"
#include "Pluglet.h"
#include "nsIServiceManager.h"
#include "nsServiceManagerUtils.h"
#include "prenv.h"
#include "plstr.h"
#include "prlink.h"
#include "PlugletManager.h"
#include "PlugletLog.h"

#include "XPCOMShutdownObserver.h"

#ifdef XP_PC
#define SEP "\\"
#define PATH_SEP ";"
#endif


#ifndef OJI_DISABLE
#include "ProxyJNI.h"
static NS_DEFINE_CID(kJVMManagerCID,NS_JVMMANAGER_CID);
PlugletSecurityContext *PlugletEngine::securityContext = NULL;
nsJVMManager * PlugletEngine::jvmManager = NULL;
#endif /* OJI_DISABLE */

static NS_DEFINE_CID(kIPluginIID,NS_IPLUGIN_IID);
static NS_DEFINE_CID(kPluginCID,NS_PLUGIN_CID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_CID(kPluginManagerCID, NS_PLUGINMANAGER_CID);

PRLogModuleInfo* PlugletLog::log = NULL;

typedef jint (*PJNI_CreateJavaVM)(JavaVM **pvm, void **penv, void *args);
typedef jint (*PJNI_GetCreatedJavaVMs)(JavaVM **, jsize, jsize *);

static PJNI_GetCreatedJavaVMs pJNI_GetCreatedJavaVMs = nsnull;
static PJNI_CreateJavaVM pJNI_CreateJavaVM = nsnull;

void PlugletEngine::LoadJNIFuntions()
{
    char *path = nsnull;
    char *java_home = nsnull;
    char *buf = nsnull;
    char *jvmDllPath = nsnull;
    char *relativePathToJvmDll;
    PRUint32 capVariant = 0;
    PRUint32 len = 0;
    // Get the path environment variable, accounting for capitilzation variants
    if (nsnull == (capVariant = 0, path = PR_GetEnv("PATH"))) {
        if (nsnull == (capVariant = 1, path = PR_GetEnv("path"))) {
            if (nsnull == (capVariant = 2, path = PR_GetEnv("Path"))) {
                return;
            }
        }
    }

    // If the path does not already have java in it
    if ((nsnull == PL_strstr(path, "Java")) &&
        (nsnull == PL_strstr(path, "java")) &&
        (nsnull == PL_strstr(path, "JAVA"))) {
        // Get the JAVA_HOME environment variable
        if (nsnull == (java_home = PR_GetEnv("JAVA_HOME"))) {
            return;
        }

        // get the path to jvm.dll, relative to JAVA_HOME
        relativePathToJvmDll = "jre\\bin\\client";

        // Build the absolute path to the jvm.dll
        len = PL_strlen(java_home) + PL_strlen(relativePathToJvmDll) + 
            2; // 1 for \\ 1 for \0
        buf = new char[len];
        memset(buf, 0, len);
        PL_strcat(buf, java_home);
        PL_strcat(buf, SEP);
        PL_strcat(buf, relativePathToJvmDll);

        jvmDllPath = PR_GetLibraryName(buf, "jvm.dll");
        delete [] buf;
    }
    else {
        jvmDllPath = "jvm.dll";
    }

    PRLibrary *jvmDll = PR_LoadLibrary((const char *) jvmDllPath);
    if (0 != PL_strstr("jvm.dll", jvmDllPath)) {
        PR_FreeLibraryName(jvmDllPath);
    }
    if (nsnull != jvmDllPath) {
        pJNI_GetCreatedJavaVMs = (PJNI_GetCreatedJavaVMs) 
            PR_FindFunctionSymbol(jvmDll, "JNI_GetCreatedJavaVMs");
        pJNI_CreateJavaVM = (PJNI_CreateJavaVM) 
            PR_FindFunctionSymbol(jvmDll, "JNI_CreateJavaVM");
    }
}

void PlugletEngine::PopulateClasspath()
{
    const char *classpath = nsnull;
    char *buf = nsnull;
    PRUint32 len = 0;
    
    if (nsnull == (classpath = PR_GetEnv("CLASSPATH"))) {
        if (nsnull == (classpath = PR_GetEnv("Classpath"))) {
            if (nsnull == (classpath = PR_GetEnv("classpath"))) {
            }
        }
    }

    // Get the profile directory
    nsresult rv;
    
    nsCOMPtr<nsIProperties> dirService
        (do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv));
    nsCOMPtr<nsIFile> profileDir;
    if (NS_SUCCEEDED(rv)) {
        rv = dirService->Get(NS_APP_USER_PROFILE_50_DIR,
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(profileDir));
        if (NS_SUCCEEDED(rv)) {
            rv = profileDir->AppendNative(NS_LITERAL_CSTRING("extensions"));
            if (NS_SUCCEEDED(rv)) {
                rv = profileDir->AppendNative(NS_LITERAL_CSTRING("pluglet@ridingthecrest.com"));
                if (NS_SUCCEEDED(rv)) {
                    rv = profileDir->AppendNative(NS_LITERAL_CSTRING("pluglet.jar"));
                    if (NS_SUCCEEDED(rv)) {
                        nsXPIDLCString plugletJar;
                        rv = profileDir->GetNativePath(plugletJar);
                        if (NS_SUCCEEDED(rv)) {
                            if (nsnull == classpath) {
                                // 10 for CLASSPATH= and 1 for \0
                                len = PL_strlen(plugletJar.get()) + 11;
                                buf = new char[len];
                                memset(buf, 0, len);
                                PL_strcat(buf, "CLASSPATH=");
                            }
                            else {
                                // 10 for CLASSPATH= and 1 for \0
                                len = PL_strlen(classpath) + 
                                      PL_strlen(plugletJar.get()) + 11;
                                buf = new char[len];
                                memset(buf, 0, len);
                                PL_strcat(buf, "CLASSPATH=");
                                PL_strcat(buf, classpath);
                                PL_strcat(buf, PATH_SEP);
                            }
                            PL_strcat(buf, plugletJar.get());
                            PR_SetEnv(buf);
                        }
                    }
                }
            }
        }
    }
}

void PlugletEngine::SatisfyJavaPreconditions()
{
    LoadJNIFuntions();
    PopulateClasspath();
}


int PlugletEngine::objectCount = 0;
PlugletsDir * PlugletEngine::dir = NULL;
PRInt32 PlugletEngine::lockCount = 0;
nsCOMPtr<nsIPluginManager> PlugletEngine::pluginManager;
nsCOMPtr<nsIObserver> PlugletEngine::shutdownObserver;
jobject PlugletEngine::plugletManager = NULL;


#define PLUGIN_MIME_DESCRIPTION "*:*:Pluglet Engine"

PlugletEngine::PlugletEngine() {
    NS_INIT_ISUPPORTS();

    PlugletLog::log = PR_NewLogModule("pluglets");
    dir = new PlugletsDir();
    objectCount++;
}

PlugletEngine::~PlugletEngine(void) {
    delete dir;
    objectCount--;
}

NS_IMPL_ISUPPORTS3(PlugletEngine, nsIObserver, iPlugletEngine, nsIPlugin);
NS_IMETHODIMP
PlugletEngine::Observe(nsISupports *aSubject, const char *aTopic, 
                       const PRUnichar *aData)
{
    return NS_OK;
}

NS_IMETHODIMP 
PlugletEngine::Initialize(void) 
{
    //nb ???
    return NS_OK;
}

NS_IMETHODIMP 
PlugletEngine::Shutdown(void) 
{
    //nb ???
    return NS_OK;
}

NS_IMETHODIMP 
PlugletEngine::CreateInstance(nsISupports *aOuter, REFNSIID aIID,
                              void **aResult) 
{
    return NS_ERROR_FAILURE; //nb to do
}

NS_IMETHODIMP 
PlugletEngine::LockFactory(PRBool aLock) 
{
    if(aLock) {
        PR_AtomicIncrement(&lockCount); 
    } else {
        PR_AtomicDecrement(&lockCount);
    }
    return NS_OK;
}

NS_IMETHODIMP 
PlugletEngine::CreatePluginInstance(nsISupports *aOuter, REFNSIID aIID, 
                                    const char* aPluginMIMEType, 
                                    void **aResult) 
{
    PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
           ("PlugletEngine::CreatePluginInstance\n"));
    if (!aResult) {
	return NS_ERROR_FAILURE; 
    }
    PlugletFactory * plugletFactory = NULL;
    nsresult res = NS_ERROR_NULL_POINTER; 
    if ((res = dir->GetPlugletFactory(aPluginMIMEType,&plugletFactory)) != NS_OK
	|| !plugletFactory) {
        return res; 
    }
    //we do not delete pluglet because we do not allocate new memory in dir->GetPluglet()
    return plugletFactory->CreatePluginInstance(aPluginMIMEType,aResult);
}

NS_IMETHODIMP PlugletEngine::GetMIMEDescription(const char* *result) {
    PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
	    ("PlugletEngine::GetMimeDescription\n"));
    if (!result) {
	return NS_ERROR_FAILURE;
    }
    *result = strdup(PLUGIN_MIME_DESCRIPTION);
    return NS_OK;
}

NS_IMETHODIMP PlugletEngine::GetValue(nsPluginVariable variable, void *value) {
    PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
	    ("PlugletEngine::GetValue; stub\n"));
    //nb ????
    return NS_OK;
}

char *ToString(jobject obj,JNIEnv *env) {
	static jmethodID toStringID = NULL;
	if (!toStringID) {
	    jclass clazz = env->FindClass("java/lang/Object");
	    toStringID = env->GetMethodID(clazz,"toString","()Ljava/lang/String;");
	}
	jstring jstr = (jstring) env->CallObjectMethod(obj,toStringID);
	const char * str = NULL;
	str = env->GetStringUTFChars(jstr,NULL);
	char * res = new char[strlen(str)];
	strcpy(res,str);
	env->ReleaseStringUTFChars(jstr,str);
	return res;
}


#ifdef OJI_DISABLE

#ifdef XP_PC
#define PATH_SEPARATOR ';'
#else /* XP_PC */
#define PATH_SEPARATOR ':'
#endif /* XP_PC */

JavaVM *PlugletEngine::jvm = NULL;	

void PlugletEngine::StartJVM() {
    if (nsnull == pJNI_CreateJavaVM) {
        SatisfyJavaPreconditions();
    }
    if (nsnull == pJNI_CreateJavaVM) {
        PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
               ("PlugletEngine::StartJVM SEVERE ERROR: Cannot start JVM.  Is Java in your path?\n"));
        return;
    }


    JNIEnv *env = NULL;
    jint res;
    jsize jvmCount;
    pJNI_GetCreatedJavaVMs(&jvm, 1, &jvmCount);
    if (jvmCount) {
        PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
               ("PlugletEngine::StartJVM we got already started JVM\n"));
        return;
    }
    char classpath[4096]="";
    char debug[256]="";
    char runjdwp[256]="";
    char jniLibPath[4096] ="";

    sprintf(debug, "-Xdebug");
    sprintf(runjdwp, 
            "-Xrunjdwp:transport=dt_shmem,address=jdbconn,server=y,suspend=y");
    JavaVMInitArgs vm_args;
    JavaVMOption options[5];
    char * classpathEnv = PR_GetEnv("CLASSPATH");
    if (classpathEnv != NULL) {
        sprintf(classpath, "-Djava.class.path=%s",classpathEnv);
        PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
               ("PlugletEngine::StartJVM about to create JVM classpath=%s\n",classpath));
    }

    nsresult rv;
    
    nsCOMPtr<nsIProperties> dirService
        (do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv));
    nsCOMPtr<nsIFile> file;
    if (NS_SUCCEEDED(rv)) {
        rv = dirService->Get(NS_APP_USER_PROFILE_50_DIR,
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(file));
        if (NS_SUCCEEDED(rv)) {
            rv = file->AppendNative(NS_LITERAL_CSTRING("extensions"));
            if (NS_SUCCEEDED(rv)) {
                rv = file->AppendNative(NS_LITERAL_CSTRING("pluglet@ridingthecrest.com"));
                if (NS_SUCCEEDED(rv)) {
                    rv = file->AppendNative(NS_LITERAL_CSTRING("platform"));
                    if (NS_SUCCEEDED(rv)) {
#ifdef XP_PC
                        rv = file->AppendNative(NS_LITERAL_CSTRING("WINNT_x86-msvc"));
#endif
                        if (NS_SUCCEEDED(rv)) {
                            rv = file->AppendNative(NS_LITERAL_CSTRING("plugletjni.dll"));
                            if (NS_SUCCEEDED(rv)) {
                                nsXPIDLCString plugletJni;
                                rv = file->GetNativePath(plugletJni);
                                if (NS_SUCCEEDED(rv)) {
                                    // 20 for -Djava.library.path=, 1 for \0
                                    PRUint32 len = PL_strlen(plugletJni.get())+
                                        21;
                                    PL_strcat(jniLibPath, 
                                              "-Djava.library.path=");
                                    PL_strcat(jniLibPath, plugletJni.get());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
                
    options[0].optionString = classpath;
    options[1].optionString = jniLibPath;
    options[2].optionString = debug;
    options[3].optionString = runjdwp;
    options[4].optionString=""; //-Djava.compiler=NONE";
    vm_args.version = JNI_VERSION_1_4;
    vm_args.options = options;
    vm_args.nOptions = 2; // EDBURNS: Change to 4 for debugging, 2 for non-debuging
    vm_args.ignoreUnrecognized = JNI_FALSE;
    /* Create the Java VM */
    PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
           ("PlugletEngine::StartJVM about to start JVM with options %s %s %s\n",
            options[0].optionString, options[1].optionString, 
            options[2].optionString));
    res = pJNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);

    PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
           ("PlugletEngine::StartJVM after CreateJavaVM res = %d\n",res));
}

#endif /* OJI_DISABLE */

NS_IMETHODIMP PlugletEngine::GetJNIEnv(JNIEnv * *jniEnv) 
{
   JNIEnv * res = NULL;
   if (nsnull == jniEnv) {
       return NS_ERROR_NULL_POINTER;
   }
#ifndef OJI_DISABLE
   nsresult result;
   if (!jvmManager) {
       NS_WITH_SERVICE(nsIJVMManager, _jvmManager, kJVMManagerCID, &result);
       if (NS_SUCCEEDED(result)) {
           jvmManager = (nsJVMManager*)((nsIJVMManager*)_jvmManager);
       }
   }
   if (!jvmManager) {
       return result;
   }
   if (NS_FAILED(result = jvmManager->CreateProxyJNI(NULL,&res))) {
       return result;
   }
   if (!securityContext) {
       securityContext = new PlugletSecurityContext();
   }
   SetSecurityContext(res,securityContext);
#else  /* OJI_DISABLE */
   if (!jvm) {
       PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
              ("PlugletEngine::GetJNIEnv going to start our own jvm \n"));
       StartJVM();
   }
   if (jvm) {
       jvm->AttachCurrentThread((void**)&res,NULL);
   }
#endif /* OJI_DISABLE */
   *jniEnv = res;
   return NS_OK;
}

NS_IMETHODIMP PlugletEngine::GetPlugletManager(void * *jobj)
{
    if (nsnull == jobj) {
        return NS_ERROR_NULL_POINTER;
    }

    PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
           ("PlugletEngine::GetPlugletManager\n"));
    //Changed by John Sublet NS_WITH_SERVICE deprecated currently is
    //problematic: lxr.mozilla.org indicates version of
    //do_GetService that allows the res to be included in the
    //do_GetService call but that wouldn't work: FIXME
    
    //NS_WITH_SERVICE(nsIPluginManager,_pluginManager,kPluginManagerCID,&res);
    nsCOMPtr<nsIPluginManager> _pluginManager (do_GetService(kPluginManagerCID));
    
    // Changed by John Sublet : FIXME this assumes _pluginManager will be properly set to NULL
    if (_pluginManager) {
        pluginManager = _pluginManager;
    }
    if (!pluginManager) {
        return NS_ERROR_NULL_POINTER;
    }
    if (!plugletManager) {
        plugletManager = PlugletManager::GetJObject(pluginManager.get());
    }
    *jobj = pluginManager;
    return NS_OK;
}

NS_IMETHODIMP PlugletEngine::IncObjectCount()
{
    objectCount++;
    return NS_OK;
}

NS_IMETHODIMP PlugletEngine::DecObjectCount()
{
    objectCount--;
    return NS_OK;
}

NS_IMETHODIMP PlugletEngine::GetUnloadable(PRBool *aUnloadable) 
{
    if (nsnull == aUnloadable) {
        return NS_ERROR_NULL_POINTER;
    }
    *aUnloadable = (PRBool) (lockCount == 0 
                             && objectCount == 0);
    return NS_OK;
}


static NS_METHOD PlugletEngineRegistration(nsIComponentManager *aCompMgr,
				     nsIFile *aPath,
				     const char *registryLocation,
				     const char *componentType,
				     const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsIServiceManager> servman =
	do_QueryInterface((nsISupports*)aCompMgr, &rv);
    if (NS_FAILED(rv)) {
        return rv;
    }
    nsCOMPtr<nsICategoryManager> catman;
    servman->GetServiceByContractID(NS_CATEGORYMANAGER_CONTRACTID,
				    NS_GET_IID(nsICategoryManager),
				    getter_AddRefs(catman));
    if (NS_FAILED(rv)) {
        return rv;
    }
    char* previous = nsnull;
    rv = catman->AddCategoryEntry("xpcom-startup",
				  "PlugletEngine",
				  PLUGLETENGINE_ContractID,
				  PR_TRUE,
				  PR_TRUE,
				  &previous);
    if (previous) {
        nsMemory::Free(previous);
    }

    PlugletEngine::shutdownObserver = new XPCOMShutdownObserver();
    nsCOMPtr<nsIObserverService> obs
        (do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv)) {
        rv = obs->AddObserver(PlugletEngine::shutdownObserver, 
                              NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    }


    return rv;
}
static NS_METHOD PlugletEngineUnregistration(nsIComponentManager *aCompMgr,
				       nsIFile *aPath,
				       const char *registryLocation,
				       const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsIServiceManager> servman =
	do_QueryInterface((nsISupports*)aCompMgr, &rv);
    if (NS_FAILED(rv))
	return rv;
    nsCOMPtr<nsICategoryManager> catman;
    servman->GetServiceByContractID(NS_CATEGORYMANAGER_CONTRACTID,
				    NS_GET_IID(nsICategoryManager),
				    getter_AddRefs(catman));
    if (NS_FAILED(rv))
	return rv;
    rv = catman->DeleteCategoryEntry("xpcom-startup",
				     "PlugletEngine",
				     PR_TRUE);
    return rv;
}

NS_EXPORT
nsresult
iPlugletEngine::GetInstance(void ** result)
{
    nsIServiceManager *servman = nsnull;
    NS_GetServiceManager(&servman);
    nsresult rv;
    rv = servman->GetServiceByContractID(PLUGLETENGINE_ContractID,
                                         NS_GET_IID(iPlugletEngine),
                                         (void **) &result);

    if (NS_FAILED(rv)) {
        PR_LOG(PlugletLog::log, PR_LOG_DEBUG,
               ("Pluglet::PlugletFactory: Cannot access iPlugletEngine service\n"));
        return rv;
    }


    return NS_OK;
}

PlugletEngine *PlugletEngine::_NewInstance()
{
    PlugletEngine *result = new PlugletEngine();
    return result;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(PlugletEngine)

static const nsModuleComponentInfo components[] =
{
    { "PlugletEngine",
      PLUGLETENGINE_CID,
      PLUGLETENGINE_ContractID,
      PlugletEngineConstructor,
      PlugletEngineRegistration,
      PlugletEngineUnregistration
    }
};
NS_IMPL_NSGETMODULE(PlugletEngineModule, components)
