#include "nscore.h"
#include "nsIComponentManager.h"
#include "nsIFile.h"
#include "nsIDirectoryEnumerator.h"
#include "nsIDirEnumeratorImpl.h"
#include "nsFileUtils.h"


nsresult NS_COM 
NS_NewFile(nsIFile** file)
{
    return nsComponentManager::CreateInstance(NS_FILE_PROGID, 
                                              nsnull, 
                                              NS_GET_IID(nsIFile), 
                                              (void**)file);
}

nsresult NS_COM 
NS_NewDirectoryEnumerator(nsIFile* parent, PRBool resolveSymlinks, nsIDirectoryEnumerator** enumerator)
{
    nsresult rv = nsComponentManager::CreateInstance(NS_DIRECTORY_ENUMERATOR_PROGID, 
                                         nsnull, 
                                         NS_GET_IID(nsIDirectoryEnumerator), 
                                         (void**)enumerator); 
        
    if (NS_SUCCEEDED(rv) && *enumerator)
    {
        (*enumerator)->Init(parent, resolveSymlinks);
    }
    return rv;
}
