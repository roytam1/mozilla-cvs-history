/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
#include "nsIAppShellComponentImpl.h"

class nsITextServicesDocument;

// {4AA267A0-F81D-11d2-8067-00600811A9C3}
#define NS_FINDCOMPONENT_CID \
    { 0x4aa267a0, 0xf81d, 0x11d2, { 0x80, 0x67, 0x0, 0x60, 0x8, 0x11, 0xa9, 0xc3} }

class nsFindComponent : public nsIFindComponent, public nsAppShellComponentImpl
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR( NS_FINDCOMPONENT_CID );

    // ctor/dtor
    nsFindComponent();
    virtual ~nsFindComponent();

    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsIAppShellComponent interface functions.
    NS_DECL_IAPPSHELLCOMPONENT

    // This class implements the nsIFindComponent interface functions.
    NS_DECL_IFINDCOMPONENT

    // "Context" for this implementation.
    class Context : public nsISupports
    {
    	public:
        NS_DECL_ISUPPORTS
        
										Context();
				virtual 		~Context();
				NS_IMETHOD	Init( nsIWebShell *aWebShell,
			                 const nsString &lastSearchString,
			                 PRBool lastIgnoreCase,
			                 PRBool lastSearchBackwards,
			                 PRBool lastWrapSearch);

				NS_IMETHOD	Reset( nsIWebShell *aNewWebShell );
				NS_IMETHOD	DoFind();

                // Utility to construct new TS document from our webshell.
                nsCOMPtr<nsITextServicesDocument> MakeTSDocument();
      
        // Maybe add Find/FindNext functions here?

        nsCOMPtr<nsIWebShell> mWebShell;
        nsIDocument *mLastDocument; // Document last searched.
        nsString mSearchString;
        PRBool   mIgnoreCase;
        PRBool   mSearchBackwards;
        PRBool   mWrapSearch;
        PRUint32 mLastBlockOffset; // last offset within the cur block that we found something
        PRInt32  mLastBlockIndex;  // last block (negative indicates it's relative to last block)
    }; // nsFindComponent::Context

protected:
    nsString                     mLastSearchString;
    PRBool                       mLastIgnoreCase;
    PRBool                       mLastSearchBackwards;
    PRBool                       mLastWrapSearch;
    nsInstanceCounter            mInstanceCounter;
}; // nsFindComponent
