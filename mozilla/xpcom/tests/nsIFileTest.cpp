#include "nsILocalFile.h"
#include "nsFileUtils.h"

#include "stdio.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIAllocator.h"
#include "nsXPIDLString.h"

void Passed();
void Failed(const char* explanation = nsnull);
void Inspect();
void Banner(const char* bannerString);

void VerifyResult(nsresult rv)
{
    if (NS_FAILED(rv))
    {
        Failed("rv failed");
        printf("rv = %d\n", rv);
    }
}
//----------------------------------------------------------------------------
void Banner(const char* bannerString)
//----------------------------------------------------------------------------
{
    printf("---------------------------\n");
    printf("%s\n", bannerString);
    printf("---------------------------\n");
}

//----------------------------------------------------------------------------
void Passed()
//----------------------------------------------------------------------------
{
    printf("Test passed.");
}

//----------------------------------------------------------------------------
void Failed(const char* explanation)
//----------------------------------------------------------------------------
{
    printf("ERROR : Test failed.\n");
    printf("REASON: %s.\n", explanation);
}

//----------------------------------------------------------------------------
void Inspect()
//----------------------------------------------------------------------------
{
    printf("^^^^^^^^^^ PLEASE INSPECT OUTPUT FOR ERRORS\n");
}

void GetPaths(nsILocalFile* file)
{
    nsresult rv;
    nsXPIDLCString pathName;

    printf("Getting Path\n");

    rv = file->GetPath(getter_Copies(pathName));
    VerifyResult(rv);
    
    printf("filepath: %s\n", (const char *)pathName);
}

extern "C" void
NS_SetupRegistry()
{
  nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup, NULL);
}

void InitTest(char* creationPath, char* appendPath)
{
    nsILocalFile* file = nsnull;
    nsresult rv = nsComponentManager::CreateInstance(NS_LOCAL_FILE_PROGID, 
                                              nsnull, 
                                              nsCOMTypeInfo<nsILocalFile>::GetIID(), 
                                              (void**)&file);
    
    if (NS_FAILED(rv) || (!file)) 
    {
        printf("create nsILocalFile failed\n");
        return;
    }

    nsXPIDLCString leafName;

    Banner("InitWithPath");
    printf("creationPath == %s\nappendPath == %s\n", creationPath, appendPath);

    rv = file->InitWithPath(creationPath);
    VerifyResult(rv);
    
    printf("Getting Filename\n");
    rv = file->GetLeafName(getter_Copies(leafName));
    printf(" %s\n", (const char *)leafName);
    VerifyResult(rv);

    printf("Appending %s \n", appendPath);
    rv = file->AppendPath(appendPath);
    VerifyResult(rv);

    printf("Getting Filename\n");
    rv = file->GetLeafName(getter_Copies(leafName));
    printf(" %s\n", (const char *)leafName);
    VerifyResult(rv);

    GetPaths(file);

    
    printf("Check For Existence\n");

    PRBool exists;
    file->Exists(&exists);

    if (exists)
        printf("Yup!\n");
    else
        printf("no.\n");
}


void CreationTest(char* creationPath, char* appendPath,
		  PRInt32 whatToCreate, PRInt32 perm)
{
    nsCOMPtr<nsILocalFile> file;
    nsresult rv = 
    nsComponentManager::CreateInstance(NS_LOCAL_FILE_PROGID, 
                                              nsnull, 
                                              nsCOMTypeInfo<nsILocalFile>::GetIID(), 
                                              (void **)getter_AddRefs(file));

    if (NS_FAILED(rv) || (!file)) 
    {
        printf("create nsILocalFile failed\n");
        return;
    }

    Banner("Creation Test");
    printf("creationPath == %s\nappendPath == %s\n", creationPath, appendPath);

    rv = file->InitWithPath(creationPath);
    VerifyResult(rv);
 
    printf("Appending %s\n", appendPath);
    rv = file->AppendPath(appendPath);
    VerifyResult(rv);
    
    printf("Check For Existance\n");

    PRBool exists;
    file->Exists(&exists);

    if (exists)
        printf("Yup!\n");
    else
        printf("no.\n");


    rv = file->Create(whatToCreate, perm);  
    VerifyResult(rv);

    rv = file->Exists(&exists);
    VerifyResult(rv);

    
    if (!exists)
    {
        Failed("Did not create file system object!");
        return;
    }
    
}    
    
void
DeletionTest(char* creationPath, char* appendPath, PRBool recursive)
{
    nsCOMPtr<nsILocalFile> file;
    nsresult rv = 
      nsComponentManager::CreateInstance(NS_FILE_PROGID, NULL,
					 NS_GET_IID(nsILocalFile),
					 (void**)getter_AddRefs(file));
    
    if (NS_FAILED(rv) || (!file)) 
    {
        printf("create nsILocalFile failed\n");
        return;
    }

    Banner("Deletion Test");
    printf("creationPath == %s\nappendPath == %s\n", creationPath, appendPath);

    rv = file->InitWithPath(creationPath);
    VerifyResult(rv);
 
    printf("Appending %s\n", appendPath);
    rv = file->AppendPath(appendPath);
    VerifyResult(rv);
    
    printf("Check For Existance\n");

    PRBool exists;
    file->Exists(&exists);

    if (exists)
        printf("Yup!\n");
    else
        printf("no.\n");

    rv = file->Delete(recursive);  
    VerifyResult(rv);

    rv = file->Exists(&exists);
    VerifyResult(rv);
    
    if (exists)
    {
        Failed("Did not create delete system object!");
        return;
    }
    
}



int main(void)
{
    NS_SetupRegistry();


#ifdef XP_PC
    InitTest("c:\\temp\\", "sub1/sub2/");
    InitTest("d:\\temp\\", "sub1/sub2/");

    CreationTest("c:\\temp\\", "file.txt", nsIFile::NORMAL_FILE_TYPE, 0644);
    DeletionTest("c:\\temp\\", "file.txt", PR_FALSE);

    CreationTest("c:\\temp\\", "mumble/a/b/c/d/e/f/g/h/i/j/k/", nsIFile::DIRECTORY_TYPE, 0644);
    DeletionTest("c:\\temp\\", "mumble", PR_TRUE);

#else
#ifdef XP_UNIX
    InitTest("/tmp/", "sub1/sub2/");
    
    CreationTest("/tmp", "file.txt", nsIFile::NORMAL_FILE_TYPE, 0644);
    DeletionTest("/tmp/", "file.txt", PR_FALSE);
    
    CreationTest("/tmp", "mumble/a/b/c/d/e/f/g/h/i/j/k/", nsIFile::DIRECTORY_TYPE, 0644);
    DeletionTest("/tmp", "mumble", PR_TRUE);
#endif /* XP_UNIX */
#endif /* XP_PC */
}
