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

#include "nsIFindComponent.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIAppShellService.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsITextServicesDocument.h"
#include "nsTextServicesCID.h"
#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsIPresShell.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIContent.h"
#include "nsIURL.h"
#ifdef NECKO
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#endif // NECKO
#include "nsFileSpec.h"
#include "nsIFactory.h"
#include "pratom.h"
#include "nsIServiceManager.h"

#include "nsFindComponent.h"
#include "nsFindDialog.h"


#ifdef DEBUG
#define DEBUG_FIND
#endif


nsFindComponent::Context::Context()
{
  NS_INIT_REFCNT();
	// all our other members are self-initiating
}


nsFindComponent::Context::~Context()
{
}

NS_IMETHODIMP
nsFindComponent::Context::Init( nsIWebShell *aWebShell,
                 nsIEditor* aEditor,
                 const nsString& lastSearchString,
                 const nsString& lastReplaceString,
                 PRBool lastCaseSensitive,
                 PRBool lastSearchBackward,
                 PRBool lastWrapSearch)
{
	if (!aWebShell)
		return NS_ERROR_INVALID_ARG;
	
	mEditor          = aEditor;				// don't AddRef
	mTargetWebShell  = aWebShell;			// don't AddRef
	mSearchString    = lastSearchString;
	mReplaceString   = lastReplaceString;
	mCaseSensitive   = lastCaseSensitive;
	mSearchBackwards = lastSearchBackward;
	mWrapSearch      = lastWrapSearch;
	
	return NS_OK;
}


static NS_DEFINE_CID(kCTextServicesDocumentCID, NS_TEXTSERVICESDOCUMENT_CID);


NS_IMETHODIMP
nsFindComponent::Context::MakeTSDocument(nsIWebShell* aWebShell, nsITextServicesDocument** aDoc)
{
  if (!aWebShell)
    return NS_ERROR_INVALID_ARG;
    
  if (!aDoc)
    return NS_ERROR_NULL_POINTER;

  *aDoc = NULL;
  
  // Create the text services document.
  nsCOMPtr<nsITextServicesDocument>  tempDoc;
  nsresult rv = nsComponentManager::CreateInstance(kCTextServicesDocumentCID,
                                                      nsnull,
                                                      nsITextServicesDocument::GetIID(),
                                                      getter_AddRefs(tempDoc));
  if (NS_FAILED(rv) || !tempDoc)
    return rv;

  // Get content viewer from the web shell.
  nsCOMPtr<nsIContentViewer> contentViewer;
  rv = aWebShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (NS_FAILED(rv) || !contentViewer)
    return rv;

  // Up-cast to a document viewer.
  nsCOMPtr<nsIDocumentViewer> docViewer = do_QueryInterface(contentViewer, &rv);
  if (NS_FAILED(rv) || !docViewer)
    return rv;
    
  // Get the document and pres shell from the doc viewer.
  nsCOMPtr<nsIDocument>  document;
  nsCOMPtr<nsIPresShell> presShell;
  rv = docViewer->GetDocument(*getter_AddRefs(document));
  if (document)
      rv = docViewer->GetPresShell(*getter_AddRefs(presShell));

  if (NS_FAILED(rv) || !document || !presShell)
    return rv;

  // Upcast document to a DOM document.
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(document, &rv);
  if (NS_FAILED(rv) || !domDoc)
    return rv;

  // Initialize the text services document.
  rv = tempDoc->InitWithDocument(domDoc, presShell);
  if (NS_FAILED(rv))
    return rv;

  // Return the resulting text services document.
  *aDoc = tempDoc;
  NS_IF_ADDREF(*aDoc);
  
  return rv;
}

// ----------------------------------------------------------------
//	CharsMatch
//
//	Compare chars. Match if both are whitespace, or both are
//	non whitespace and same char.
// ----------------------------------------------------------------

inline static PRBool CharsMatch(PRUnichar c1, PRUnichar c2)
{
	return (nsString::IsSpace(c1) && nsString::IsSpace(c2)) ||
						(c1 == c2);
	
}


// ----------------------------------------------------------------
//	FindInString
//
//	Routine to search in an nsString which is smart about extra
//  whitespace, can search backwards, and do case insensitive search.
//
//	This uses a brute-force algorithm, which should be sufficient
//	for our purposes (text searching)
// 
//	searchStr contains the text from a content node, which can contain
//	extra white space between words, which we have to deal with.
//	The offsets passed in and back are offsets into searchStr,
//	and thus include extra white space.
//
//	If we are ignoring case, the strings have already been lowercased
// 	at this point.
//
//  startOffset is the offset in the search string to start seraching
//  at. If -1, it means search from the start (forwards) or end (backwards).
//
//	Returns -1 if the string is not found, or if the pattern is an
//	empty string, or if startOffset is off the end of the string.
// ----------------------------------------------------------------

static PRInt32 FindInString(const nsString &searchStr, const nsString &patternStr,
						PRInt32 startOffset, PRBool searchBackwards)
{
	PRInt32		foundOffset = -1;
	PRInt32		patternLen = patternStr.Length();
	PRInt32		searchStrLen = searchStr.Length();
		
	if (patternLen == 0)									// pattern is empty
		return -1;
	
	if (startOffset < 0)
		startOffset = (searchBackwards) ? searchStrLen : 0;
	
	if (startOffset > searchStrLen)			// bad start offset
		return -1;
	
	if (patternLen > searchStrLen)				// pattern is longer than string to search
		return -1;
	
	const PRUnichar	*searchBuf = searchStr.GetUnicode();
	const PRUnichar	*patternBuf = patternStr.GetUnicode();

	const PRUnichar	*searchEnd = searchBuf + searchStrLen;
	const PRUnichar	*patEnd = patternBuf + patternLen;
	
	if (searchBackwards)
	{
		// searching backwards
		const PRUnichar	*s = searchBuf + startOffset - patternLen - 1;
	
		while (s >= searchBuf)
		{
			if (CharsMatch(*patternBuf, *s))			// start potential match
			{
				const PRUnichar	*t = s;
				const PRUnichar	*p = patternBuf;
				PRInt32		curMatchOffset = t - searchBuf;
				PRBool		inWhitespace = nsString::IsSpace(*p);
				
				while (p < patEnd && CharsMatch(*p, *t))
				{
					if (inWhitespace && !nsString::IsSpace(*p))
					{
						// leaving p whitespace. Eat up addition whitespace in s
						while (t < searchEnd - 1 && nsString::IsSpace(*(t + 1)))
							t ++;
							
						inWhitespace = PR_FALSE;
					}
					else
						inWhitespace = nsString::IsSpace(*p);

					t ++;
					p ++;
				}
				
				if (p == patEnd)
				{
					foundOffset = curMatchOffset;
					goto done;
				}
				
				// could be smart about decrementing s here
			}
		
			s --;
		}
	
	}
	else
	{
		// searching forwards
		
		const PRUnichar	*s = &searchBuf[startOffset];
	
		while (s < searchEnd)
		{
			if (CharsMatch(*patternBuf, *s))			// start potential match
			{
				const PRUnichar	*t = s;
				const PRUnichar	*p = patternBuf;
				PRInt32		curMatchOffset = t - searchBuf;
				PRBool		inWhitespace = nsString::IsSpace(*p);
				
				while (p < patEnd && CharsMatch(*p, *t))
				{
					if (inWhitespace && !nsString::IsSpace(*p))
					{
						// leaving p whitespace. Eat up addition whitespace in s
						while (t < searchEnd - 1 && nsString::IsSpace(*(t + 1)))
							t ++;
							
						inWhitespace = PR_FALSE;
					}
					else
						inWhitespace = nsString::IsSpace(*p);

					t ++;
					p ++;
				}
				
				if (p == patEnd)
				{
					foundOffset = curMatchOffset;
					goto done;
				}
				
				// could be smart about incrementing s here
			}
			
			s ++;
		}
	
	}

done:
	return foundOffset;
}

// utility method to discover which block we're in. The TSDoc interface doesn't give
// us this, because it can't assume a read-only document.
NS_IMETHODIMP
nsFindComponent::Context::GetCurrentBlockIndex(nsITextServicesDocument *aDoc, PRInt32 *outBlockIndex)
{
  PRInt32  blockIndex = 0;
  PRBool   isDone = PR_FALSE;
  
  while (NS_SUCCEEDED(aDoc->IsDone(&isDone)) && !isDone)
  {
    aDoc->PrevBlock();
    blockIndex ++;
  }
  
  *outBlockIndex = blockIndex;
  return NS_OK;
}
    
NS_IMETHODIMP
nsFindComponent::Context::SetupDocForSearch(nsITextServicesDocument *aDoc, PRInt32 *outBlockOffset)
{
  nsresult  rv;
  
  nsITextServicesDocument::TSDBlockSelectionStatus blockStatus;
  PRInt32 selOffset;
  PRInt32 selLength;
  
  if (!mSearchBackwards)	// searching forwards
  {
    rv = aDoc->LastSelectedBlock(&blockStatus, &selOffset, &selLength);
    if (NS_SUCCEEDED(rv) && (blockStatus != nsITextServicesDocument::eBlockNotFound))
    {
      switch (blockStatus)
      {
        case nsITextServicesDocument::eBlockOutside:		// No TB in S, but found one before/after S.
        case nsITextServicesDocument::eBlockPartial:		// S begins or ends in TB but extends outside of TB.
          // the TS doc points to the block we want.
          *outBlockOffset = selOffset + selLength;
          break;
                    
        case nsITextServicesDocument::eBlockInside:			// S extends beyond the start and end of TB.
          // we want the block after this one.
          rv = aDoc->NextBlock();
          *outBlockOffset = 0;
          break;
                
        case nsITextServicesDocument::eBlockContains:		// TB contains entire S.
          *outBlockOffset = selOffset + selLength;
          break;
        
        case nsITextServicesDocument::eBlockNotFound:		// There is no text block (TB) in or before the selection (S).
        default:
            NS_NOTREACHED("Shouldn't ever get this status");
      }
    
    }
    else		//failed to get last sel block. Just start at beginning
    {
      rv = aDoc->FirstBlock();
    }
  
  }
  else			// searching backwards
  {
    rv = aDoc->FirstSelectedBlock(&blockStatus, &selOffset, &selLength);
		if (NS_SUCCEEDED(rv) && (blockStatus != nsITextServicesDocument::eBlockNotFound))
		{
		  switch (blockStatus)
		  {
		    case nsITextServicesDocument::eBlockOutside:		// No TB in S, but found one before/after S.
		    case nsITextServicesDocument::eBlockPartial:		// S begins or ends in TB but extends outside of TB.
		      // the TS doc points to the block we want.
		      *outBlockOffset = selOffset;
		      break;
		                
		    case nsITextServicesDocument::eBlockInside:			// S extends beyond the start and end of TB.
		      // we want the block before this one.
		      rv = aDoc->PrevBlock();
		      *outBlockOffset = -1;
		      break;
		            
		    case nsITextServicesDocument::eBlockContains:		// TB contains entire S.
		      *outBlockOffset = selOffset;
		      break;
		    
		    case nsITextServicesDocument::eBlockNotFound:		// There is no text block (TB) in or before the selection (S).
		    default:
		        NS_NOTREACHED("Shouldn't ever get this status");
		  }
		}
		else
		{
		  rv = aDoc->LastBlock();
		}
  }

  return rv;
}


NS_IMETHODIMP
nsFindComponent::Context::DoFind(PRBool *aDidFind)
{
#ifdef DEBUG_FIND
  printf("Doing find\n");
#endif

  if (!aDidFind)
    return NS_ERROR_NULL_POINTER;
  
	if (!mTargetWebShell)
		return NS_ERROR_NOT_INITIALIZED;

	*aDidFind = PR_FALSE;
	
	nsAutoString		matchString = mSearchString;
	if (!mCaseSensitive)
		matchString.ToLowerCase();
	
  nsresult	rv = NS_OK;

  // Construct a text services document to use. This is freed when we
  // return from this function.
  nsCOMPtr<nsITextServicesDocument> txtDoc;
  rv = MakeTSDocument(mTargetWebShell, getter_AddRefs(txtDoc));
  if (NS_FAILED(rv) || !txtDoc)
    return rv;
  
  // Set up the TSDoc. We are going to start searching thus:
  // 
  // Searching forwards:
  //        Look forward from the end of the selection
  // Searching backwards:
  //        Look backwards from the start of the selection
  //
  PRInt32  selOffset = 0;
  rv = SetupDocForSearch(txtDoc, &selOffset);
  if (NS_FAILED(rv))
    return rv;  
  
  // find out where we started
  PRInt32  blockIndex;
  rv = GetCurrentBlockIndex(txtDoc, &blockIndex);
  if (NS_FAILED(rv))
    return rv;

  // remember where we started
  PRInt32  startingBlockIndex = blockIndex;
  
  // and set the starting position again (hopefully, in future we won't have to do this)
  rv = SetupDocForSearch(txtDoc, &selOffset);
  if (NS_FAILED(rv))
    return rv;  
    
  PRBool wrappedOnce = PR_FALSE;	// Remember whether we've already wrapped
	PRBool done = PR_FALSE;
	
  // Loop till we find a match or fail.
  while ( !done )
  {
      PRBool atExtremum = PR_FALSE;		// are we at the end (or start)

      while ( NS_SUCCEEDED(txtDoc->IsDone(&atExtremum)) && !atExtremum )
      {
          nsString str;
          rv = txtDoc->GetCurrentTextBlock(&str);
  
          if (NS_FAILED(rv))
              return rv;
  
          if (!mCaseSensitive)
              str.ToLowerCase();
          
          PRInt32 foundOffset = FindInString(str, matchString, selOffset, mSearchBackwards);
          selOffset = -1;			// reset for next block
  
          if (foundOffset != -1)
          {
              // Match found.  Select it, remember where it was, and quit.
              txtDoc->SetSelection(foundOffset, mSearchString.Length());
              done = PR_TRUE;
             	*aDidFind = PR_TRUE;
              break;
          }
          else
          {
              // have we already been around once?
              if (wrappedOnce && (blockIndex == startingBlockIndex))
              {
                done = PR_TRUE;
                break;
              }

              // No match found in this block, try the next (or previous) one.
              if (mSearchBackwards) {
                  txtDoc->PrevBlock();
                  blockIndex--;
              } else {
                  txtDoc->NextBlock();
                  blockIndex++;
              }
          }
          
      } // while !atExtremum
      
      // At end (or matched).  Decide which it was...
      if (!done)
      {
          // Hit end without a match.  If we haven't passed this way already,
          // then reset to the first/last block (depending on search direction).
          if (!wrappedOnce)
          {
              // Reset now.
              wrappedOnce = PR_TRUE;
              // If not wrapping, give up.
              if ( !mWrapSearch ) {
                  done = PR_TRUE;
              }
              else
              {
                  if ( mSearchBackwards ) {
                      // Reset to last block.
                      rv = txtDoc->LastBlock();
                      // ugh
                      rv = GetCurrentBlockIndex(txtDoc, &blockIndex);
                      rv = txtDoc->LastBlock();
                  } else {
                      // Reset to first block.
                      rv = txtDoc->FirstBlock();
                      blockIndex = 0;
                  }
              }
          } else
          {
              // already wrapped.  This means no matches were found.
              done = PR_TRUE;
          }
      }
  }

	return NS_OK;
}


NS_IMETHODIMP
nsFindComponent::Context::DoReplace()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFindComponent::Context::Reset( nsIWebShell *aNewWebShell )
{
	if (!aNewWebShell)
		return NS_ERROR_INVALID_ARG;

  mTargetWebShell = aNewWebShell;		// don't AddRef
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif

// ctor
nsFindComponent::nsFindComponent()
    : mLastSearchString(),
      mLastCaseSensitive( PR_TRUE ),
      mLastSearchBackwards( PR_FALSE ),
      mLastWrapSearch( PR_FALSE )
{
    NS_INIT_REFCNT();

    // Initialize "last" stuff from prefs, if we wanted to be really clever...
}

// dtor
nsFindComponent::~nsFindComponent()
{
}

NS_IMETHODIMP
nsFindComponent::CreateContext( nsIWebShell *aWebShell, nsIEditor* aEditor,
                                nsISupports **aResult )
{

    if (!aResult)
			return NS_ERROR_NULL_POINTER;
      
    // Construct a new Context with this document.
    Context		*newContext = new Context();
   	if (!newContext)
   		return NS_ERROR_OUT_OF_MEMORY;
   
     // Do the expected AddRef on behalf of caller.
    newContext->AddRef();

    nsresult	rv = newContext->Init( aWebShell,
                        aEditor,
                        mLastSearchString,
                        mLastReplaceString,
                        mLastCaseSensitive,
                        mLastSearchBackwards,
                        mLastWrapSearch);
 		if (NS_FAILED(rv))
 		{
 			NS_RELEASE(newContext);
 			return rv;
 		}
 
		*aResult = newContext;
    return NS_OK;
}

NS_IMETHODIMP
nsFindComponent::Find(nsISupports *aContext, PRBool *aDidFind)
{
    nsresult rv = NS_OK;

    if ( aContext && GetAppShell() )
    {
        Context *context = (Context*)aContext;

        // Open Find dialog and prompt for search parameters.

        // Make url for dialog xul.
        nsIURI *url;
        char * urlStr = "resource:/res/samples/finddialog.xul";

        // this should be a chrome URI
        // chrome://navigator/dialogs/content/default/finddialog.xul or something.
#ifndef NECKO
        rv = NS_NewURL( &url, urlStr );
#else
        NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        nsIURI *uri = nsnull;
        rv = service->NewURI(urlStr, nsnull, &uri);
        if (NS_FAILED(rv)) return rv;

        rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&url);
        NS_RELEASE(uri);
        if (NS_FAILED(rv)) return rv;
#endif // NECKO

        // Create callbacks object for the find dialog.
        nsFindDialog *dialog = new nsFindDialog( this, context );

        nsCOMPtr<nsIWebShellWindow> newWindow;
        rv = GetAppShell()->CreateTopLevelWindow( nsnull,
                                                  url,
                                                  PR_TRUE,
                                                  getter_AddRefs(newWindow),
                                                  nsnull,
                                                  dialog,
                                                  0,
                                                  0 );

        if ( NS_SUCCEEDED( rv ) ) {
            // Tell the dialog its nsIWebShellWindow.
            dialog->SetWindow( newWindow );
        }

        // Release the url for the xul file.
        NS_RELEASE( url );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }

    return rv;
}

NS_IMETHODIMP
nsFindComponent::Replace( nsISupports *aContext )
{
    nsresult rv = NS_OK;

		if (!aContext)
			return NS_ERROR_NULL_POINTER;
			
		// For now, just record request to console.
		Context *context = (Context*)aContext;

	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFindComponent::FindNext(nsISupports *aContext, PRBool *aDidFind)
{
    nsresult rv = NS_OK;

		if (!aContext)
			return NS_ERROR_NULL_POINTER;
			
		// For now, just record request to console.
		Context *context = (Context*)aContext;
#ifdef DEBUG_FIND
		printf( "nsFindComponent::FindNext\n\tkey=%s\n\tcaseSensitive=%d\tsearchBackward=%d\n",
			      (const char *)nsAutoCString( context->mSearchString ),
			      context->mCaseSensitive, context->mSearchBackwards);
#endif
		context->DoFind(aDidFind);


		// Record this for out-of-the-blue FindNext calls.
		mLastSearchString    = context->mSearchString;
		mLastCaseSensitive   = context->mCaseSensitive;
		mLastSearchBackwards = context->mSearchBackwards;
		mLastWrapSearch      = context->mWrapSearch;

    return rv;
}

NS_IMETHODIMP
nsFindComponent::ResetContext( nsISupports *aContext,
                               nsIWebShell *aNewWebShell,
                               nsIEditor* aEditor )
{
    nsresult rv = NS_OK;
    if ( aContext && aNewWebShell ) {
        // Pass on the new document to the context.
        Context *context = (Context*)aContext;
        context->Reset( aNewWebShell );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

// nsFindComponent::Context implementation...
NS_IMPL_ISUPPORTS( nsFindComponent::Context, nsCOMTypeInfo<nsISupports>::GetIID() )

NS_IMPL_IAPPSHELLCOMPONENT( nsFindComponent, nsIFindComponent, NS_IFINDCOMPONENT_PROGID, 0 )
