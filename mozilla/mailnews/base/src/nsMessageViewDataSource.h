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

#ifndef NSMESSAGEVIEWDATASOURCE_H
#define NSMESSAGEVIEWDATASOURCE_H

#include "nsIRDFCompositeDataSource.h"
#include "nsIMessageView.h"
#include "nsIMsgFolder.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsISupportsArray.h"
#include "nsIEnumerator.h"
#include "nsIMessage.h"
#include "nsIMsgThread.h"
#include "nsCOMPtr.h"

/**
 * The mail data source.
 */
class nsMessageViewDataSource : public nsIRDFCompositeDataSource, public nsIMessageView,
								public nsIRDFObserver
{
private:
	nsCOMPtr<nsISupportsArray> mObservers;
	PRBool			mInitialized;
	nsIRDFService * mRDFService;

public:
  
	NS_DECL_ISUPPORTS

	nsMessageViewDataSource(void);
	virtual ~nsMessageViewDataSource (void);
  virtual nsresult Init();


	// nsIRDFDataSource methods
	NS_IMETHOD GetURI(char* *uri);

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
						nsISimpleEnumerator** sources);

	NS_IMETHOD GetTargets(nsIRDFResource* source,
						nsIRDFResource* property,    
						PRBool tv,
						nsISimpleEnumerator** targets);

	NS_IMETHOD Assert(nsIRDFResource* source,
					nsIRDFResource* property, 
					nsIRDFNode* target,
					PRBool tv);

	NS_IMETHOD Unassert(nsIRDFResource* source,
					  nsIRDFResource* property,
					  nsIRDFNode* target);

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
						  PRBool* hasAssertion);

	NS_IMETHOD AddObserver(nsIRDFObserver* n);

	NS_IMETHOD RemoveObserver(nsIRDFObserver* n);

	NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
						 nsISimpleEnumerator** labels);

	NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
						  nsISimpleEnumerator** labels); 

	NS_IMETHOD GetAllResources(nsISimpleEnumerator** aResult);

	NS_IMETHOD GetAllCommands(nsIRDFResource* source,
							nsIEnumerator/*<nsIRDFResource>*/** commands);
	NS_IMETHOD GetAllCmds(nsIRDFResource* source,
							nsISimpleEnumerator/*<nsIRDFResource>*/** commands);

	NS_IMETHOD IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
							  nsIRDFResource*   aCommand,
							  nsISupportsArray/*<nsIRDFResource>*/* aArguments,
							  PRBool* aResult);

	NS_IMETHOD DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
					   nsIRDFResource*   aCommand,
					   nsISupportsArray/*<nsIRDFResource>*/* aArguments);


	//nsIRDFCompositeDataSource
	NS_IMETHOD AddDataSource(nsIRDFDataSource* source);

	NS_IMETHOD RemoveDataSource(nsIRDFDataSource* source);

	//nsIRDFObserver
	NS_IMETHOD OnAssert(nsIRDFResource* subject,
						nsIRDFResource* predicate,
						nsIRDFNode* object);

	NS_IMETHOD OnUnassert(nsIRDFResource* subject,
                          nsIRDFResource* predicate,
                          nsIRDFNode* object);

    NS_IMETHOD OnChange(nsIRDFResource* aSource,
                        nsIRDFResource* aProperty,
                        nsIRDFNode* aOldTarget,
                        nsIRDFNode* aNewTarget);

    NS_IMETHOD OnMove(nsIRDFResource* aOldSource,
                      nsIRDFResource* aNewSource,
                      nsIRDFResource* aProperty,
                      nsIRDFNode* aTarget);
	//nsIMessageView
	NS_IMETHOD SetShowAll();
	NS_IMETHOD SetShowUnread();
	NS_IMETHOD SetShowRead();
	NS_IMETHOD SetShowWatched();
	NS_IMETHOD SetShowThreads(PRBool showThreads);


 
	// caching frequently used resources
protected:

	nsCOMPtr<nsIRDFDataSource> mDataSource;
	PRUint32 mShowStatus;
	PRBool mShowThreads;

	static nsIRDFResource* kNC_MessageChild;
	static nsIRDFResource* kNC_Subject;
	static nsIRDFResource* kNC_Date;
	static nsIRDFResource* kNC_Sender;
	static nsIRDFResource* kNC_Status;

};

class nsMessageViewMessageEnumerator: public nsIEnumerator
{

public:

	NS_DECL_ISUPPORTS

	nsMessageViewMessageEnumerator(nsIEnumerator *srcEnumerator, PRUint32 showStatus);
	virtual ~nsMessageViewMessageEnumerator();

	//nsIEnumerator interface	
	/** First will reset the list. will return NS_FAILED if no items
	*/
	NS_IMETHOD First(void);

	/** Next will advance the list. will return failed if already at end
	*/
	NS_IMETHOD Next(void);

	/** CurrentItem will return the CurrentItem item it will fail if the list is empty
	*  @param aItem return value
	*/
	NS_IMETHOD CurrentItem(nsISupports **aItem);

	/** return if the collection is at the end.  that is the beginning following a call to Prev
	*  and it is the end of the list following a call to next
	*  @param aItem return value
	*/
	NS_IMETHOD IsDone(void);

protected:
	nsresult SetAtNextItem();
	nsresult MeetsCriteria(nsIMessage *message, PRBool *meetsCriteria);

protected:

	nsCOMPtr<nsIEnumerator> mSrcEnumerator;
	PRUint32 mShowStatus;

};

class nsMessageViewThreadEnumerator: public nsIEnumerator
{

public:

	NS_DECL_ISUPPORTS

	nsMessageViewThreadEnumerator(nsIEnumerator *srcEnumerator, nsIMsgFolder *srcFolder);
	virtual ~nsMessageViewThreadEnumerator();

	//nsIEnumerator interface	
	/** First will reset the list. will return NS_FAILED if no items
	*/
	NS_IMETHOD First(void);

	/** Next will advance the list. will return failed if already at end
	*/
	NS_IMETHOD Next(void);

	/** CurrentItem will return the CurrentItem item it will fail if the list is empty
	*  @param aItem return value
	*/
	NS_IMETHOD CurrentItem(nsISupports **aItem);

	/** return if the collection is at the end.  that is the beginning following a call to Prev
	*  and it is the end of the list following a call to next
	*  @param aItem return value
	*/
	NS_IMETHOD IsDone(void);

protected:
	nsresult GetMessagesForCurrentThread();
protected:

	nsCOMPtr<nsIEnumerator> mThreads;
	nsCOMPtr<nsIEnumerator> mMessages;
	nsCOMPtr<nsIMsgFolder> mFolder;

};
#endif


PR_EXTERN(nsresult)
NS_NewMessageViewDataSource(const nsIID& iid, void **result);
