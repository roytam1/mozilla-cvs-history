/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef __nsWindowMediator_h
#define __nsWindowMediator_h

#include "nsIWindowMediator.h"
#include "nsXPIDLString.h"

class nsWindowEnumerator;
struct nsWindowInfo;

class nsWindowMediator : public nsIWindowMediator
{
friend class nsWindowEnumerator;

public:
	nsWindowMediator();
	virtual ~nsWindowMediator();
  nsresult Init();

  NS_DECL_NSIWINDOWMEDIATOR
	
	// COM and RDF 
	NS_DECL_ISUPPORTS	

	// RDF
  // nsIRDFDataSource
  NS_IMETHOD GetURI(char* *uri)
  {
    NS_PRECONDITION(uri != nsnull, "null ptr");
    if (! uri)
      return NS_ERROR_NULL_POINTER;

    *uri = nsXPIDLCString::Copy("rdf:window-mediator");
    if (! *uri)
      return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
  }

  NS_IMETHOD GetSource(nsIRDFResource* property,
                       nsIRDFNode* target,
                       PRBool tv,
                       nsIRDFResource** source)
  {
      return mInner->GetSource(property, target, tv, source);
  }

  NS_IMETHOD GetSources(nsIRDFResource* property,
                        nsIRDFNode* target,
                        PRBool tv,
                        nsISimpleEnumerator** sources)
  {
      return mInner->GetSources(property, target, tv, sources);
  }

  NS_IMETHOD GetTarget(nsIRDFResource* source,
                       nsIRDFResource* property,
                       PRBool tv,
                       nsIRDFNode** target)
 {
   return mInner->GetTarget(source, property, tv, target);
 }

 	NS_IMETHOD GetTargets(nsIRDFResource* source,
                        nsIRDFResource* property,
                        PRBool tv,
                        nsISimpleEnumerator** targets)
  {
      return mInner->GetTargets(source, property, tv, targets);
  }

  NS_IMETHOD Assert(nsIRDFResource* aSource, 
                    nsIRDFResource* aProperty, 
                    nsIRDFNode* aTarget,
                    PRBool aTruthValue);

  NS_IMETHOD Unassert(nsIRDFResource* aSource,
                      nsIRDFResource* aProperty,
                      nsIRDFNode* aTarget);

  NS_IMETHOD Change(nsIRDFResource* aSource,
                    nsIRDFResource* aProperty,
                    nsIRDFNode* aOldTarget,
                    nsIRDFNode* aNewTarget);

  NS_IMETHOD Move(nsIRDFResource* aOldSource,
                  nsIRDFResource* aNewSource,
                  nsIRDFResource* aProperty,
                  nsIRDFNode* aTarget);

  NS_IMETHOD HasAssertion(nsIRDFResource* source,
                          nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          PRBool* hasAssertion)
  {
      return mInner->HasAssertion(source, property, target, tv, hasAssertion);
  }

  NS_IMETHOD AddObserver(nsIRDFObserver* n)
  {
      return mInner->AddObserver(n);
  }

  NS_IMETHOD RemoveObserver(nsIRDFObserver* n)
  {
      return mInner->RemoveObserver(n);
  }

  NS_IMETHOD ArcLabelsIn( nsIRDFNode* node, nsISimpleEnumerator** labels)
  {
      return mInner->ArcLabelsIn(node, labels);
  }

  NS_IMETHOD ArcLabelsOut(nsIRDFResource* source, nsISimpleEnumerator** labels)
  {
      return mInner->ArcLabelsOut(source, labels);
  }

  NS_IMETHOD GetAllResources(nsISimpleEnumerator** aCursor)
  {
      return mInner->GetAllResources(aCursor);
  }

  NS_IMETHOD GetAllCommands(nsIRDFResource* source,
                            nsIEnumerator** commands);
  NS_IMETHOD GetAllCmds(nsIRDFResource* source,
                            nsISimpleEnumerator** commands);

  NS_IMETHOD IsCommandEnabled(nsISupportsArray* aSources,
                              nsIRDFResource*   aCommand,
                              nsISupportsArray* aArguments,
                              PRBool* aResult);

  NS_IMETHOD DoCommand(nsISupportsArray* aSources,
                       nsIRDFResource*   aCommand,
                       nsISupportsArray* aArguments);
private:
	// Helper functions
	nsresult AddWindowToRDF( nsWindowInfo* ioWindowInfo );
	PRInt32 AddEnumerator( nsWindowEnumerator* inEnumerator );
	PRInt32 RemoveEnumerator( nsWindowEnumerator* inEnumerator);
	PRInt32		mTimeStamp;
	nsVoidArray mEnumeratorList;
	nsVoidArray	mWindowList;
	

  // pseudo-constants for RDF
  static nsIRDFResource* kNC_WindowMediatorRoot;
  static nsIRDFResource* kNC_Name;
  static nsIRDFResource* kNC_URL;
  static PRInt32 gRefCnt;
  static nsIRDFDataSource* mInner;
};

#endif
