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

#ifndef nsFindDataSource_h__
#define nsFindDataSource_h__


////////////////////////////////////////////////////////////////////////
// NS_DECL_IRDFRESOURCE, NS_IMPL_IRDFRESOURCE
//
//   Convenience macros for implementing the RDF resource interface.
//
//   XXX It might make sense to move these macros to nsIRDFResource.h?

#define NS_DECL_IRDFRESOURCE \
    NS_IMETHOD EqualsNode(nsIRDFNode* node, PRBool* result) const;\
    NS_IMETHOD GetValue(const char* *uri) const;\
    NS_IMETHOD EqualsResource(const nsIRDFResource* resource, PRBool* result) const;\
    NS_IMETHOD EqualsString(const char* uri, PRBool* result) const;


#define NS_IMPL_IRDFRESOURCE(__class) \
NS_IMETHODIMP \
__class::EqualsNode(nsIRDFNode* node, PRBool* result) const {\
    nsresult rv;\
    nsIRDFResource* resource;\
    if (NS_SUCCEEDED(node->QueryInterface(kIRDFResourceIID, (void**) &resource))) {\
        rv = EqualsResource(resource, result);\
        NS_RELEASE(resource);\
    }\
    else {\
        *result = PR_FALSE;\
        rv = NS_OK;\
    }\
    return rv;\
}\
NS_IMETHODIMP \
__class::GetValue(const char* *uri) const{\
    if (!uri)\
        return NS_ERROR_NULL_POINTER;\
    *uri = mURI;\
    return NS_OK;\
}\
NS_IMETHODIMP \
__class::EqualsResource(const nsIRDFResource* resource, PRBool* result) const {\
    if (!resource || !result)  return NS_ERROR_NULL_POINTER;\
    *result = (resource == (nsIRDFResource*) this);\
    return NS_OK;\
}\
NS_IMETHODIMP \
__class::EqualsString(const char* uri, PRBool* result) const {\
    if (!uri || !result)  return NS_ERROR_NULL_POINTER;\
    *result = (PL_strcmp(uri, mURI) == 0);\
    return NS_OK;\
}



typedef	struct	_findTokenStruct
{
	char			*token;
	char			*value;
} findTokenStruct, *findTokenPtr;



class FindDataSource : public nsIRDFFindDataSource
{
private:
	char			*mURI;
	nsVoidArray		*mObservers;

	static PRInt32		gRefCnt;

    // pseudo-constants
	static nsISupports	*kNC_Child;
	static nsISupports	*kNC_Name;
	static nsISupports	*kNC_URL;
	static nsISupports	*kNC_FindObject;
	static nsISupports	*kRDF_InstanceOf;
	static nsISupports	*kRDF_type;

	NS_METHOD	getFindResults(nsISupports *source,
				nsVoidArray **array /* out */);
	NS_METHOD	getFindName(nsISupports *source,
				nsVoidArray **array /* out */);
	NS_METHOD	parseResourceIntoFindTokens(nsISupports *u,
				findTokenPtr tokens);
	NS_METHOD	doMatch(nsIRDFLiteral *literal, char *matchMethod,
				char *matchText);
	NS_METHOD	parseFindURL(nsISupports *u,
				nsVoidArray *array);

public:

	NS_DECL_ISUPPORTS

			FindDataSource(void);
	virtual		~FindDataSource(void);

	// nsIRDFDataSource methods

	NS_IMETHOD	Init(const char *uri);
	NS_IMETHOD	GetURI(const char **uri) const;
	NS_IMETHOD	GetSource(nsISupports *property,
				nsISupports *target,
				PRBool tv,
				nsISupports **source /* out */);
	NS_IMETHOD	GetSources(nsISupports *property,
				nsISupports *target,
				PRBool tv,
				nsIRDFAssertionCursor **sources /* out */);
	NS_IMETHOD	GetTarget(nsISupports *source,
				nsISupports *property,
				PRBool tv,
				nsISupports **target /* out */);
	NS_IMETHOD	GetTargets(nsISupports *source,
				nsISupports *property,
				PRBool tv,
				nsIRDFAssertionCursor **targets /* out */);
	NS_IMETHOD	Assert(nsISupports *source,
				nsISupports *property,
				nsISupports *target,
				PRBool tv);
	NS_IMETHOD	Unassert(nsISupports *source,
				nsISupports *property,
				nsISupports *target);
	NS_IMETHOD	HasAssertion(nsISupports *source,
				nsISupports *property,
				nsISupports *target,
				PRBool tv,
				PRBool *hasAssertion /* out */);
	NS_IMETHOD	ArcLabelsIn(nsISupports *node,
				nsIRDFArcsInCursor **labels /* out */);
	NS_IMETHOD	ArcLabelsOut(nsISupports *source,
				nsIRDFArcsOutCursor **labels /* out */);
	NS_IMETHOD	GetAllResources(nsIRDFResourceCursor** aCursor);
	NS_IMETHOD	AddObserver(nsIRDFObserver *n);
	NS_IMETHOD	RemoveObserver(nsIRDFObserver *n);
	NS_IMETHOD	Flush();
	NS_IMETHOD	GetAllCommands(nsISupports* source,
				nsIEnumerator** commands);
	NS_IMETHOD	IsCommandEnabled(nsISupportsArray* aSources,
				nsISupports*   aCommand,
				nsISupportsArray* aArguments);
	NS_IMETHOD	DoCommand(nsISupportsArray* aSources,
				nsISupports*   aCommand,
				nsISupportsArray* aArguments);
};



class FindCursor : public nsIRDFAssertionCursor, public nsIRDFArcsOutCursor
{
private:
	nsISupports	*mValue;
	nsISupports	*mSource;
	nsISupports	*mProperty;
	nsISupports	*mTarget;
	int		mCount;
	nsVoidArray	*mArray;
	PRBool		mArcsOut;

public:
			FindCursor(nsISupports *source, nsISupports *property, PRBool isArcsOut, nsVoidArray *array);
	virtual		~FindCursor(void);

	NS_DECL_ISUPPORTS

	NS_IMETHOD	Advance(void);
	NS_IMETHOD	GetValue(nsISupports **aValue);
	NS_IMETHOD	GetDataSource(nsIRDFDataSource **aDataSource);
	NS_IMETHOD	GetSubject(nsISupports **aResource);
	NS_IMETHOD	GetPredicate(nsISupports **aPredicate);
	NS_IMETHOD	GetObject(nsISupports **aObject);
	NS_IMETHOD	GetTruthValue(PRBool *aTruthValue);
};



#endif // nsFindDataSource_h__
