/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsLoadGroup_h__
#define nsLoadGroup_h__

#include "nsILoadGroup.h"
#include "nsAgg.h"

class nsISupportsArray;

class nsLoadGroup : public nsILoadGroup
{
public:
    NS_DECL_AGGREGATED
    
    ////////////////////////////////////////////////////////////////////////////
    // nsIRequest methods:
    
    /* void Cancel (); */
    NS_IMETHOD Cancel();

    /* void Suspend (); */
    NS_IMETHOD Suspend();

    /* void Resume (); */
    NS_IMETHOD Resume();

    ////////////////////////////////////////////////////////////////////////////
    // nsILoadGroup methods:
    
    /* attribute unsigned long DefaultLoadAttributes; */
    NS_IMETHOD GetDefaultLoadAttributes(PRUint32 *aDefaultLoadAttributes);
    NS_IMETHOD SetDefaultLoadAttributes(PRUint32 aDefaultLoadAttributes);

    /* void AddChannel (in nsIChannel channel); */
    NS_IMETHOD AddChannel(nsIChannel *channel);

    /* void RemoveChannel (in nsIChannel channel); */
    NS_IMETHOD RemoveChannel(nsIChannel *channel);

    /* readonly attribute nsISimpleEnumerator Channels; */
    NS_IMETHOD GetChannels(nsISimpleEnumerator * *aChannels);

    /* void AddSubGroup (in nsILoadGroup group); */
    NS_IMETHOD AddSubGroup(nsILoadGroup *group);

    /* void RemoveSubGroup (in nsILoadGroup group); */
    NS_IMETHOD RemoveSubGroup(nsILoadGroup *group);

    /* readonly attribute nsISimpleEnumerator SubGroups; */
    NS_IMETHOD GetSubGroups(nsISimpleEnumerator * *aSubGroups);

    ////////////////////////////////////////////////////////////////////////////
    // nsLoadGroup methods:

    nsLoadGroup(nsISupports* outer);
    virtual ~nsLoadGroup();
    
    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
    typedef nsresult (*IterateFun)(nsIRequest* request);
    nsresult Iterate(IterateFun fun);

protected:
    PRUint32                    mDefaultLoadAttributes;
    nsISupportsArray*           mChannels;
    nsISupportsArray*           mSubGroups;
};

#endif // nsLoadGroup_h__
