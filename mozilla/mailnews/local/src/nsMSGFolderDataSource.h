/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsIRDFMSGFolderDataSource.h"
#include "nsIMsgFolder.h"
#include "nsISupports.h"    
#include "nsVoidArray.h"
#include "nsIRDFNode.h"
#include "nsIRDFCursor.h"
#include "nsFileSpec.h"

/**
 * The mail data source.
 */
class nsMSGFolderDataSource : public nsIRDFMSGFolderDataSource 
{
private:
  char*         mURI;
  nsVoidArray*  mObservers;
	PRBool				mInitialized;

public:
  
  NS_DECL_ISUPPORTS

  nsMSGFolderDataSource(void);
  virtual ~nsMSGFolderDataSource (void);


  // nsIRDFDataSource methods
  NS_IMETHOD Init(const char* uri);

  NS_IMETHOD GetURI(const char* *uri) const;

  NS_IMETHOD GetSource(nsIRDFResource* property,
                       nsIRDFNode* target,
                       PRBool tv,
                       nsIRDFResource** source /* out */);

  NS_IMETHOD GetTarget(nsIRDFResource* source,
                       nsIRDFResource* property,
                       PRBool tv,
                       nsIRDFNode** target);

  NS_IMETHOD GetSources(nsIRDFResource* property,
                        nsIRDFNode* target,
                        PRBool tv,
                        nsIRDFAssertionCursor** sources);

  NS_IMETHOD GetTargets(nsIRDFResource* source,
                        nsIRDFResource* property,    
                        PRBool tv,
                        nsIRDFAssertionCursor** targets);

  NS_IMETHOD Assert(nsIRDFResource* source,
                    nsIRDFResource* property, 
                    nsIRDFNode* target,
                    PRBool tv);

  NS_IMETHOD Unassert(nsIRDFResource* source,
                      nsIRDFResource* property,
                      nsIRDFNode* target);

  NS_IMETHOD HasAssertion(nsIRDFResource* source,
                          nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          PRBool* hasAssertion);

  NS_IMETHOD AddObserver(nsIRDFObserver* n);

  NS_IMETHOD RemoveObserver(nsIRDFObserver* n);

  NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
                         nsIRDFArcsInCursor** labels);

  NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
                          nsIRDFArcsOutCursor** labels); 

  NS_IMETHOD GetAllResources(nsIRDFResourceCursor** aCursor);

  NS_IMETHOD Flush();

  NS_IMETHOD IsCommandEnabled(const char* aCommand,
                              nsIRDFResource* aCommandTarget,
                              PRBool* aResult);

  NS_IMETHOD DoCommand(const char* aCommand,
                       nsIRDFResource* aCommandTarget);

  // caching frequently used resources
  static nsIRDFResource* kNC_Child;
  static nsIRDFResource* kNC_MessageChild;
  static nsIRDFResource* kNC_Folder;
  static nsIRDFResource* kNC_Name;
  static nsIRDFResource* kNC_Columns;
  static nsIRDFResource* kNC_MSGFolderRoot;

  static nsIRDFResource* kNC_Subject;
  static nsIRDFResource* kNC_Sender;
  static nsIRDFResource* kNC_Date;
};

