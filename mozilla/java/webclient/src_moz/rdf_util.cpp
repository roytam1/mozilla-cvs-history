/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
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
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s):  Ed Burns <edburns@acm.org>
 */

#include "rdf_util.h"

#include "nsIServiceManager.h"

static PRBool rdf_inited = PR_FALSE;

nsCOMPtr<nsIRDFContainerUtils> gRDFCU = NULL;
nsCOMPtr<nsIRDFService> gRDF = NULL;
nsCOMPtr<nsIBookmarksService> gBookmarks = NULL;
nsCOMPtr<nsIRDFDataSource> gBookmarksDataSource = NULL;

nsCOMPtr<nsIRDFResource> kNC_BookmarksRoot = NULL;
nsCOMPtr<nsIRDFResource> kNC_Name = NULL;
nsCOMPtr<nsIRDFResource> kNC_URL = NULL;
nsCOMPtr<nsIRDFResource> kNC_Folder = NULL;
nsCOMPtr<nsIRDFResource> kRDF_type = NULL;


nsresult rdf_InitRDFUtils()
{
    nsresult rv = NS_ERROR_FAILURE;

    if (rdf_inited) {
        return NS_OK;
    }

    // Init the global instances
    
    if (NULL == gBookmarks) {
        // Get the bookmarks service
        rv = nsServiceManager::GetService(NS_IBOOKMARKSSERVICE_PROGID,
                                          NS_GET_IID(nsIBookmarksService),
                                          getter_AddRefs(gBookmarks));
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    if (NULL == gBookmarksDataSource) {
        // get the bookmarks service as an RDFDataSource
        rv = gBookmarks->QueryInterface(NS_GET_IID(nsIRDFDataSource),
                                        getter_AddRefs(gBookmarksDataSource));
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    if (NULL == gRDF) {
        // get the RDF service
        rv = nsServiceManager::GetService(NS_IRDFSERVICE_PROGID,
                                          NS_GET_IID(nsIRDFService),
                                          getter_AddRefs(gRDF));
        if (NS_FAILED(rv)) {
            return rv;
        }
    }
    
    if (NULL == gRDFCU) {
        // get the RDF service
        rv = nsServiceManager::GetService(NS_ICONTAINERUTILS_PROGID,
                                          NS_GET_IID(nsIRDFContainerUtils),
                                          getter_AddRefs(gRDFCU));
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    // init the properties
    // find the nsIRDFResource for the bookmarks
    if (NULL == kNC_BookmarksRoot) {
        rv = gRDF->GetResource(BOOKMARKS_URI, 
                               getter_AddRefs(kNC_BookmarksRoot));
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    if (NULL == kNC_Name) {
        rv = gRDF->GetResource("http://home.netscape.com/NC-rdf#Name", 
                               getter_AddRefs(kNC_Name));
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    if (NULL == kNC_URL) {
        rv = gRDF->GetResource("http://home.netscape.com/NC-rdf#URL", 
                               getter_AddRefs(kNC_URL));
        if (NS_FAILED(rv)) {
            return rv;
        }
    }
    if (NULL == kNC_Folder) {
      rv = gRDF->GetResource("http://home.netscape.com/NC-rdf#Folder", 
                            getter_AddRefs(kNC_Folder));
      if (NS_FAILED(rv)) {
	return rv;
      }
    }
  
    if (NULL == kRDF_type) {
      rv = gRDF->GetResource("http://home.netscape.com/NC-rdf#type", 
			     getter_AddRefs(kRDF_type));
      if (NS_FAILED(rv)) {
	return rv;
      }
    }     

    rdf_inited = PR_TRUE;
    return rv;
}

void rdf_recursiveResourceTraversal(nsCOMPtr<nsIRDFResource> currentResource)
{
    nsresult rv;
    PRBool result;
    const PRUnichar *textForNode = NULL;
    nsCOMPtr<nsISupports> supportsResult;
    nsCOMPtr<nsIRDFNode> node;
    nsCOMPtr<nsIRDFLiteral> literal;
    nsCOMPtr<nsIRDFResource> childResource;
    nsCOMPtr<nsIRDFContainer> container;
    nsCOMPtr<nsISimpleEnumerator> elements;
        
    // ASSERT(NULL != gRDFCU)

    rv = gRDFCU->IsContainer(gBookmarksDataSource, currentResource, 
                             &result);

    if (result) {
        // It's a folder
        
        printf("FOLDER:\n");
        rdf_printArcLabels(currentResource);

        // see if it has a name target
	// Ashu
	// used 1 instead of true
        rv = gBookmarksDataSource->GetTarget(currentResource,
                                                    kNC_Name, PR_TRUE, 
                                                    getter_AddRefs(node));
        
        if (NS_SUCCEEDED(rv)) {
            // if so, make sure it's an nsIRDFLiteral
            rv = node->QueryInterface(NS_GET_IID(nsIRDFLiteral), 
                                      getter_AddRefs(literal));
            if (NS_SUCCEEDED(rv)) {
                rv = literal->GetValueConst(&textForNode);
                printf("folder: %S\n", textForNode);
            }
            else {
                printf("recursiveResourceTraversal: node is not an nsIRDFLiteral.\n");
            }
            
        } 
        else {
            printf("recursiveResourceTraversal: can't get name from currentResource.\n");
        }
            
        // get a container in order to recurr
        rv = nsComponentManager::CreateInstance(NS_IRDFCONTAINER_PROGID,
                                                nsnull,
                                                NS_GET_IID(nsIRDFContainer),
                                                getter_AddRefs(container));
        if (NS_FAILED(rv)) {
            printf("recursiveResourceTraversal: can't get a new container\n");
            return;
        }
        
        rv = container->Init(gBookmarksDataSource, currentResource);
        if (NS_FAILED(rv)) {
            printf("recursiveResourceTraversal: can't init container\n");
            return;
        }
        
        // go over my children
        rv = container->GetElements(getter_AddRefs(elements));

        if (NS_FAILED(rv)) {
            printf("recursiveResourceTraversal: can't get elements from folder\n");
            return;
        }

        rv = elements->HasMoreElements(&result);
        if (NS_FAILED(rv)) {
            printf("recursiveResourceTraversal: folder %s has no children.\n",
                   textForNode); 
            return;
        }
        while (result) {
            rv = elements->GetNext(getter_AddRefs(supportsResult));
            if (NS_FAILED(rv)) {
                printf("Exception: Can't get next from enumerator.\n");
                return;
            }

            // make sure it's an RDFResource
            rv = supportsResult->QueryInterface(NS_GET_IID(nsIRDFResource), 
                                                getter_AddRefs(childResource));
            if (NS_FAILED(rv)) {
                printf("Exception: next from enumerator is not an nsIRDFResource.\n");
                return;
            }

            rdf_recursiveResourceTraversal(childResource);

            rv = elements->HasMoreElements(&result);
            if (NS_FAILED(rv)) {
                printf("Exception: can't tell if we have more elements.\n"); 
                return;
            }
        }
    }
    else {
        // It's a bookmark
        printf("BOOKMARK:\n");
        rdf_printArcLabels(currentResource);

        // see if it has a URL target
	// Ashu
	// used 1 instead of true
        rv = gBookmarksDataSource->GetTarget(currentResource,
                                                    kNC_URL, PR_TRUE, 
                                                    getter_AddRefs(node));
        if (NS_FAILED(rv)) {
            printf("recursiveResourceTraversal: can't get url from currentResource\n");
            return;
        }
        
        // if so, make sure it's an nsIRDFLiteral
        rv = node->QueryInterface(NS_GET_IID(nsIRDFLiteral), 
                                  getter_AddRefs(literal));
        if (NS_FAILED(rv)) {
            printf("recursiveResourceTraversal: node is not an nsIRDFLiteral.\n");
            return;
        }

        // get the value of the literal
        rv = literal->GetValueConst(&textForNode);
        if (NS_FAILED(rv)) {
            printf("recursiveResourceTraversal: node doesn't have a value.\n");
            return;
        }

        printf("\turl: %S\n", textForNode);
        
    }
    
}

void rdf_printArcLabels(nsCOMPtr<nsIRDFResource> currentResource)
{
    nsCOMPtr<nsISimpleEnumerator> labels;
    nsCOMPtr<nsISupports> supportsResult;
    nsCOMPtr<nsIRDFResource> resourceResult;
    nsresult rv;
    PRBool result;
    const char *arcLabel;
    const char *currentResourceValue;

    rv = currentResource->GetValueConst(&currentResourceValue);
    if (NS_SUCCEEDED(rv)) {
        printf("resource: %s\n", currentResourceValue);
    }
    else {
        printf("printArcLabels: can't get value from current resource.\n");
    }

    rv = gBookmarksDataSource->ArcLabelsOut(currentResource, 
                                            getter_AddRefs(labels));

    if (NS_FAILED(rv)) {
        printf("printArcLabels: currentResource has no outgoing arcs.\n");
        return;
    }
    
    rv = labels->HasMoreElements(&result);
    if (NS_FAILED(rv)) {
        printf("printArcLabels: can't get elements from currentResource's enum.\n");
        return;
    }

    while (result) {
        rv = labels->GetNext(getter_AddRefs(supportsResult));
        if (NS_FAILED(rv)) {
            printf("printArcLabels: Can't get next arc from enumerator.\n");
            return;
        }
        
        // make sure it's an RDFResource
        rv = supportsResult->QueryInterface(NS_GET_IID(nsIRDFResource), 
                                            getter_AddRefs(resourceResult));
        if (NS_SUCCEEDED(rv)) {
            rv = resourceResult->GetValueConst(&arcLabel);
            if (NS_SUCCEEDED(rv)) {
                printf("\tarc label: %s\n", arcLabel);
            }
            else {
                printf("printArcLabels: can't get value from current arc.\n");
            }
        }
        else {
            printf("printArcLabels: next arc from enumerator is not an nsIRDFResource.\n");
        }
        rv = labels->HasMoreElements(&result);
        if (NS_FAILED(rv)) {
            printf("printArcLabels: can't get elements from currentResource's enum.\n");
            return;
        }
    }
}

nsresult rdf_getChildAt(int index, nsIRDFResource *theParent,
                        nsIRDFResource **retval)
{
    // Since there's no getChildAt type thing in RDF, we just get an
    // enumeration and get the indexth child.
    nsCOMPtr<nsIRDFResource> parent = theParent;
    nsCOMPtr<nsIRDFContainer> container;
    nsCOMPtr<nsISimpleEnumerator> elements;
    nsCOMPtr<nsISupports> supportsResult;
    nsCOMPtr<nsIRDFResource> childResource;
    nsresult rv;
    PRBool result, hasChildAtIndex = PR_FALSE;
    *retval = NULL;
    PRInt32 i = 0;

    rv = gRDFCU->IsContainer(gBookmarksDataSource, parent, 
                             &result);
    if (NS_FAILED(rv)) {
        return rv;
    }

    if (PR_FALSE == result) {
        return NS_OK;
    }

    rv = nsComponentManager::CreateInstance(NS_IRDFCONTAINER_PROGID,
                                            nsnull,
                                            NS_GET_IID(nsIRDFContainer),
                                            getter_AddRefs(container));
    if (NS_FAILED(rv)) {
        return rv;
    }
    
    rv = container->Init(gBookmarksDataSource, parent);
    if (NS_FAILED(rv)) {
        return rv;
    }

    rv = container->GetElements(getter_AddRefs(elements));
    
    if (NS_FAILED(rv)) {
        return rv;
    }
    
    rv = elements->HasMoreElements(&result);
    if (NS_FAILED(rv)) {
        return rv;
    }
    while (result) {
        rv = elements->GetNext(getter_AddRefs(supportsResult));
        if (NS_FAILED(rv)) {
            return rv;
        }

        if (index == i) {
            hasChildAtIndex = PR_TRUE;
            break;
        }
        i++;
        rv = elements->HasMoreElements(&result);
        if (NS_FAILED(rv)) {
            return rv;
        }
    }
    if (hasChildAtIndex) {
        rv = supportsResult->QueryInterface(NS_GET_IID(nsIRDFResource), 
                                            getter_AddRefs(childResource));
        if (NS_FAILED(rv)) {
            return rv;
        }
        *retval = childResource;
    }
    return NS_OK;
}

nsresult rdf_getChildCount(nsIRDFResource *theParent, PRInt32 *count)
{
    nsresult rv;
    nsCOMPtr<nsIRDFResource> parent = theParent;
    nsCOMPtr<nsIRDFContainer> container;
    PRBool result;
    *count = 0;

    rv = gRDFCU->IsContainer(gBookmarksDataSource, parent, 
                             &result);
    if (NS_FAILED(rv)) {
        return rv;
    }
    
    if (PR_FALSE == result) {
        return NS_OK;
    }
    
    rv = nsComponentManager::CreateInstance(NS_IRDFCONTAINER_PROGID,
                                            nsnull,
                                            NS_GET_IID(nsIRDFContainer),
                                            getter_AddRefs(container));
    if (NS_FAILED(rv)) {
        return rv;
    }
    
    rv = container->Init(gBookmarksDataSource, parent);
    if (NS_FAILED(rv)) {
        return rv;
    }


    rv = container->GetCount(count);
    if (NS_FAILED(rv)) {
        return rv;
    }
    return NS_OK;
}

nsresult rdf_getIndexOfChild(nsIRDFResource *theParent, 
                             nsIRDFResource *theChild, 
                             PRInt32 *index)
{
    nsCOMPtr<nsIRDFResource> parent = theParent;
    nsCOMPtr<nsIRDFResource> child = theChild;
    nsCOMPtr<nsIRDFContainer> container;
    nsCOMPtr<nsISimpleEnumerator> elements;
    nsCOMPtr<nsISupports> supportsResult;
    nsresult rv;
    PRBool result;
    *index = -1;
    PRInt32 i = 0;

    rv = gRDFCU->IsContainer(gBookmarksDataSource, parent, 
                             &result);
    if (NS_FAILED(rv)) {
        return rv;
    }

    if (PR_FALSE == result) {
        return NS_OK;
    }

    rv = nsComponentManager::CreateInstance(NS_IRDFCONTAINER_PROGID,
                                            nsnull,
                                            NS_GET_IID(nsIRDFContainer),
                                            getter_AddRefs(container));
    if (NS_FAILED(rv)) {
        return rv;
    }
    
    rv = container->Init(gBookmarksDataSource, parent);
    if (NS_FAILED(rv)) {
        return rv;
    }

    rv = container->GetElements(getter_AddRefs(elements));
    
    if (NS_FAILED(rv)) {
        return rv;
    }
    
    rv = elements->HasMoreElements(&result);
    if (NS_FAILED(rv)) {
        return rv;
    }
    while (result) {
        rv = elements->GetNext(getter_AddRefs(supportsResult));
        if (NS_FAILED(rv)) {
            return rv;
        }

        if (supportsResult == child) {
            *index = i;
            rv = NS_OK;
            break;
        }
        i++;
        rv = elements->HasMoreElements(&result);
        if (NS_FAILED(rv)) {
            return rv;
        }
    }
    return rv;
}
