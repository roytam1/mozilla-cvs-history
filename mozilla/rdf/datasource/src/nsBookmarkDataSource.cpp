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

/*
  A data source that reads the 4.x "bookmarks.html" file and
  constructs an in-memory data source.
 */

#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsFileSpec.h"
#include "nsFileStream.h"
#include "nsSpecialSystemDirectory.h"
#include "nsRDFCID.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsXPIDLString.h"
#include "prio.h"
#include "rdf.h"
#include "rdfutil.h"
#include "prlog.h"
#include "nsIComponentManager.h"
#include "xp_core.h"
#include "nsIBookmarkDataSource.h"

////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kIRDFDataSourceIID,        NS_IRDFDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFServiceIID,           NS_IRDFSERVICE_IID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,            NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,     NS_RDFCONTAINERUTILS_CID);
static NS_DEFINE_CID(kRDFBookmarkDataSourceCID, NS_RDFBOOKMARKDATASOURCE_CID);
static NS_DEFINE_IID(kIRDFBookmarkDataSourceIID,NS_IRDFBOOKMARKDATASOURCE_IID);

static const char kURINC_BookmarksRoot[]         = "NC:BookmarksRoot"; // XXX?
static const char kURINC_IEFavoritesRoot[]       = "NC:IEFavoritesRoot"; // XXX?
static const char kURINC_PersonalToolbarFolder[] = "NC:PersonalToolbarFolder"; // XXX?

DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, child);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Bookmark);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, BookmarkSeparator);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, BookmarkAddDate);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Description);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Folder);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, IEFavorite);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Name);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, PersonalToolbarFolderCategory);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, URL);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, ShortcutURL);

DEFINE_RDF_VOCAB(WEB_NAMESPACE_URI, WEB, LastVisitDate);
DEFINE_RDF_VOCAB(WEB_NAMESPACE_URI, WEB, LastModifiedDate);

DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, instanceOf);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, type);

static const char kPersonalToolbarFolder[]  = "Personal Toolbar Folder";


////////////////////////////////////////////////////////////////////////

PRInt32 gRefCnt;
nsIRDFService* gRDFService;
nsIRDFResource* kNC_Bookmark;
nsIRDFResource* kNC_BookmarkSeparator;
nsIRDFResource* kNC_BookmarkAddDate;
nsIRDFResource* kNC_BookmarksRoot;
nsIRDFResource* kNC_Description;
nsIRDFResource* kNC_Folder;
nsIRDFResource* kNC_IEFavorite;
nsIRDFResource* kNC_IEFavoritesRoot;
nsIRDFResource* kNC_Name;
nsIRDFResource* kNC_PersonalToolbarFolder;
nsIRDFResource* kNC_ShortcutURL;
nsIRDFResource* kNC_URL;
nsIRDFResource* kRDF_type;
nsIRDFResource* kWEB_LastModifiedDate;
nsIRDFResource* kWEB_LastVisitDate;

static void
bm_AddRefGlobals()
{
    if (gRefCnt++ == 0) {
        nsresult rv;
        if (NS_FAILED(rv = nsServiceManager::GetService(kRDFServiceCID,
                                                        kIRDFServiceIID,
                                                        (nsISupports**) &gRDFService))) {
            NS_ERROR("unable to get RDF service");
            return;
        }

        gRDFService->GetResource(kURINC_Bookmark,          &kNC_Bookmark);
        gRDFService->GetResource(kURINC_BookmarkSeparator, &kNC_BookmarkSeparator);
        gRDFService->GetResource(kURINC_BookmarkAddDate,   &kNC_BookmarkAddDate);
        gRDFService->GetResource(kURINC_BookmarksRoot,     &kNC_BookmarksRoot);
        gRDFService->GetResource(kURINC_Description,       &kNC_Description);
        gRDFService->GetResource(kURINC_Folder,            &kNC_Folder);
        gRDFService->GetResource(kURINC_IEFavorite,        &kNC_IEFavorite);
        gRDFService->GetResource(kURINC_IEFavoritesRoot,   &kNC_IEFavoritesRoot);
        gRDFService->GetResource(kURINC_Name,              &kNC_Name);
        gRDFService->GetResource(kURINC_PersonalToolbarFolder, &kNC_PersonalToolbarFolder);
        gRDFService->GetResource(kURINC_ShortcutURL,       &kNC_ShortcutURL);
        gRDFService->GetResource(kURINC_URL,               &kNC_URL);
        gRDFService->GetResource(kURIRDF_type,             &kRDF_type);
        gRDFService->GetResource(kURIWEB_LastModifiedDate, &kWEB_LastModifiedDate);
        gRDFService->GetResource(kURIWEB_LastVisitDate,    &kWEB_LastVisitDate);
    }    
}


static void
bm_ReleaseGlobals()
{
    if (--gRefCnt == 0) {
        if (gRDFService)
            nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);

        NS_IF_RELEASE(kNC_Bookmark);
        NS_IF_RELEASE(kNC_BookmarkSeparator);
        NS_IF_RELEASE(kNC_BookmarkAddDate);
        NS_IF_RELEASE(kNC_BookmarksRoot);
        NS_IF_RELEASE(kNC_Description);
        NS_IF_RELEASE(kNC_Folder);
        NS_IF_RELEASE(kNC_IEFavorite);
        NS_IF_RELEASE(kNC_IEFavoritesRoot);
        NS_IF_RELEASE(kNC_Name);
        NS_IF_RELEASE(kNC_PersonalToolbarFolder);
        NS_IF_RELEASE(kNC_ShortcutURL);
        NS_IF_RELEASE(kNC_URL);
        NS_IF_RELEASE(kRDF_type);
        NS_IF_RELEASE(kWEB_LastModifiedDate);
        NS_IF_RELEASE(kWEB_LastVisitDate);
    }
}


////////////////////////////////////////////////////////////////////////

/**
 * The bookmark parser knows how to read <tt>bookmarks.html</tt> and convert it
 * into an RDF graph.
 */
class BookmarkParser {
private:
    nsInputFileStream      *mStream;
    nsIRDFDataSource       *mDataSource;
    const char             *mIEFavoritesRoot;
    PRBool                 mFoundIEFavoritesRoot;

protected:
    nsresult AssertTime(nsIRDFResource* aSource,
                        nsIRDFResource* aLabel,
                        PRInt32 aTime);

    nsresult ParseBookmark(const nsString& aLine, nsIRDFResource* aContainer, nsIRDFResource *nodeType);
    nsresult ParseBookmarkHeader(const nsString& aLine, nsIRDFResource* aContainer, nsIRDFResource *nodeType);
    nsresult ParseBookmarkSeparator(const nsString& aLine, nsIRDFResource* aContainer);
    nsresult ParseHeaderBegin(const nsString& aLine, nsIRDFResource* aContainer);
    nsresult ParseHeaderEnd(const nsString& aLine);
    nsresult ParseAttribute(const nsString& aLine,
                            const char* aAttribute,
                            PRInt32 aAttributeLen,
                            nsString& aResult);

public:
    BookmarkParser();
    ~BookmarkParser();

    nsresult Init(nsInputFileStream *aStream, nsIRDFDataSource *aDataSource);
    nsresult Parse(nsIRDFResource* aContainer, nsIRDFResource *nodeType);
    nsresult AddBookmark(nsIRDFResource * aContainer, const char *url, const char *optionalTitle,
			PRInt32 addDate, PRInt32 lastVisitDate, PRInt32 lastModifiedDate,
			const char *shortcutURL, nsIRDFResource *nodeType);
    nsresult SetIEFavoritesRoot(const char *IEFavoritesRootURL)
    {
    	mIEFavoritesRoot = IEFavoritesRootURL;
    	return(NS_OK);
    }
    nsresult ParserFoundIEFavoritesRoot(PRBool *foundIEFavoritesRoot)
    {
    	*foundIEFavoritesRoot = mFoundIEFavoritesRoot;
    	return(NS_OK);
    }
};


BookmarkParser::BookmarkParser()
{
    bm_AddRefGlobals();
}

nsresult
BookmarkParser::Init(nsInputFileStream *aStream, nsIRDFDataSource *aDataSource)
{
	mStream = aStream;
	mDataSource = aDataSource;
	mIEFavoritesRoot = nsnull;
	mFoundIEFavoritesRoot = PR_FALSE;
	return(NS_OK);
}

BookmarkParser::~BookmarkParser(void)
{
    bm_ReleaseGlobals();
}

static const char kHREFEquals[]   = "HREF=\"";
static const char kCloseAnchor[] = "</A>";

static const char kOpenHeading[]  = "<H";
static const char kCloseHeading[] = "</H";

static const char kSeparator[]  = "<HR>";

static const char kOpenUL[]     = "<UL>";
static const char kCloseUL[]    = "</UL>";

static const char kOpenMenu[]   = "<MENU>";
static const char kCloseMenu[]  = "</MENU>";

static const char kOpenDL[]     = "<DL>";
static const char kCloseDL[]    = "</DL>";

static const char kTargetEquals[]       = "TARGET=\"";
static const char kAddDateEquals[]      = "ADD_DATE=\"";
static const char kLastVisitEquals[]    = "LAST_VISIT=\"";
static const char kLastModifiedEquals[] = "LAST_MODIFIED=\"";
static const char kShortcutURLEquals[]  = "SHORTCUTURL=\"";


nsresult
BookmarkParser::Parse(nsIRDFResource* aContainer, nsIRDFResource *nodeType)
{
	// tokenize the input stream.
	// XXX this needs to handle quotes, etc. it'd be nice to use the real parser for this...
    nsresult rv = NS_OK;
    nsAutoString line;
	while (NS_SUCCEEDED(rv) && !mStream->eof() && !mStream->failed()) {
        char c = mStream->get();
        line.Append(c);

        if (line.Last() != '\n' && line.Last() != '\r' && !mStream->eof())
            continue; // keep building the string

        PRInt32 offset;

        if ((offset = line.Find(kHREFEquals)) >= 0) {
            rv = ParseBookmark(line, aContainer, nodeType);
        }
        else if ((offset = line.Find(kOpenHeading)) >= 0 &&
                 nsString::IsDigit(line.CharAt(offset + 2))) {
            // XXX Ignore <H1> so that bookmarks root _is_ <H1>
            if (line.CharAt(offset + 2) != PRUnichar('1'))
                rv = ParseBookmarkHeader(line, aContainer, nodeType);
        }
        else if ((offset = line.Find(kSeparator)) >= 0) {
            rv = ParseBookmarkSeparator(line, aContainer);
        }
        else if ((offset = line.Find(kCloseUL)) >= 0 ||
                 (offset = line.Find(kCloseMenu)) >= 0 ||
                 (offset = line.Find(kCloseDL)) >= 0) {
            return ParseHeaderEnd(line);
        }
        else if ((offset = line.Find(kOpenUL)) >= 0 ||
                 (offset = line.Find(kOpenMenu)) >= 0 ||
                 (offset = line.Find(kOpenDL)) >= 0) {
            rv = ParseHeaderBegin(line, aContainer);
        }
        else {
            // XXX Discard the line. We should be treating this as the
            // description.
        }
        line.Truncate();
    }
    return rv;
}


nsresult
BookmarkParser::ParseBookmark(const nsString& aLine, nsIRDFResource* aContainer, nsIRDFResource *nodeType)
{
    NS_PRECONDITION(aContainer != nsnull, "null ptr");
    if (! aContainer)
        return NS_ERROR_NULL_POINTER;

    PRInt32 start = aLine.Find(kHREFEquals);
    NS_ASSERTION(start >= 0, "no 'HREF=\"' string: how'd we get here?");
    if (start < 0)
        return NS_ERROR_UNEXPECTED;

    // 1. Crop out the URL

    // Skip past the first double-quote
    start += (sizeof(kHREFEquals) - 1);

    // ...and find the next so we can chop the URL.
    PRInt32 end = aLine.Find(PRUnichar('"'), start);
    NS_ASSERTION(end >= 0, "unterminated string");
    if (end < 0)
        return NS_ERROR_UNEXPECTED;

    nsAutoString url;
    aLine.Mid(url, start, end - start);

    {
        // Now do properly replace %22's (this is what 4.5 did, anyway...)
        static const char kEscape22[] = "%22";
        PRInt32 offset;
        while ((offset = url.Find(kEscape22)) >= 0) {
            url.SetCharAt(' ',offset);
            url.Cut(offset + 1, sizeof(kEscape22) - 2);
        }
    }

    // XXX At this point, the URL may be relative. 4.5 called into
    // netlib to make an absolute URL, and there was some magic
    // "relative_URL" parameter that got sent down as well. We punt on
    // that stuff.

    // 2. Parse the name

    start = aLine.Find(PRUnichar('>'), end + 1); // 'end' still points to the end of the URL
    NS_ASSERTION(start >= 0, "open anchor tag not terminated");
    if (start < 0)
        return NS_ERROR_UNEXPECTED;

    nsAutoString name;
    aLine.Right(name, aLine.Length() - (start + 1));

    end = name.Find(kCloseAnchor);
    NS_ASSERTION(end >= 0, "anchor tag not terminated");
    if (end < 0)
        return NS_ERROR_UNEXPECTED;

    name.Truncate(end);

    // 3. Parse the target
    nsAutoString target;

    start = aLine.Find(kTargetEquals);
    if (start >= 0) {
        start += (sizeof(kTargetEquals) - 1);
        end = aLine.Find(PRUnichar('"'), start);
        aLine.Mid(target, start, end - start);
    }


    // 4. Parse the addition date
    PRInt32 addDate = 0;

    {
        nsAutoString s;
        ParseAttribute(aLine, kAddDateEquals, sizeof(kAddDateEquals) - 1, s);
        if (s.Length() > 0) {
            PRInt32 err;
            addDate = s.ToInteger(&err); // ignored.
        }
    }

    // 5. Parse the last visit date

    PRInt32 lastVisitDate = 0;

    {
        nsAutoString s;
        ParseAttribute(aLine, kLastVisitEquals, sizeof(kLastVisitEquals) - 1, s);
        if (s.Length() > 0) {
            PRInt32 err;
            lastVisitDate = s.ToInteger(&err); // ignored.
        }
    }

    // 6. Parse the last modified date

    PRInt32 lastModifiedDate;

    {
        nsAutoString s;
        ParseAttribute(aLine, kLastModifiedEquals, sizeof(kLastModifiedEquals) - 1, s);
        if (s.Length() > 0) {
            PRInt32 err;
            lastModifiedDate = s.ToInteger(&err); // ignored.
        }
    }

    // 7. Parse the shortcut URL

    nsAutoString	shortcut("");
    ParseAttribute(aLine, kShortcutURLEquals, sizeof(kShortcutURLEquals) -1, shortcut);

    // Dunno. 4.5 did it, so will we.
    if (!lastModifiedDate)
        lastModifiedDate = lastVisitDate;

    // XXX There was some other cruft here to deal with aliases, but
    // since I have no clue what those are, I'll punt.

	char *cURL = url.ToNewCString();
	if (cURL)
	{
		char *cName = name.ToNewCString();
		char *cShortcutURL = shortcut.ToNewCString();
		nsresult rv = AddBookmark(aContainer, cURL, cName, addDate,
			lastVisitDate, lastModifiedDate, cShortcutURL, nodeType);
		delete [] cURL;
		if (cName)		delete [] cName;
		if (cShortcutURL)	delete [] cShortcutURL;
	}
	return(NS_OK);
}



    // Now create the bookmark
nsresult
BookmarkParser::AddBookmark(nsIRDFResource * aContainer, const char *url, const char *optionalTitle,
		PRInt32 addDate, PRInt32 lastVisitDate, PRInt32 lastModifiedDate, const char *shortcutURL,
		nsIRDFResource *nodeType)
{
	nsresult rv;
	nsCOMPtr<nsIRDFResource> bookmark;

	if (NS_FAILED(rv = gRDFService->GetResource(url, getter_AddRefs(bookmark) )))
	{
		NS_ERROR("unable to get bookmark resource");
		return rv;
	}

	PRBool		result = PR_FALSE;
	if (nsnull != mIEFavoritesRoot)
	{
		if (!PL_strcmp(url, mIEFavoritesRoot))
		{
			mFoundIEFavoritesRoot = PR_TRUE;
		}
	}

    nsCOMPtr<nsIRDFContainer> container;
    rv = NS_NewRDFContainer(mDataSource, aContainer, getter_AddRefs(container));
    if (NS_FAILED(rv)) return rv;

    rv = container->AppendElement(bookmark);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add bookmark to container");
    if (NS_FAILED(rv)) return rv;

	rv = mDataSource->Assert(bookmark, kRDF_type, nodeType, PR_TRUE);
    if (rv != NS_RDF_ASSERTION_ACCEPTED)
    {
		NS_ERROR("unable to add bookmark to data source");
		return rv;
	}

	if ((nsnull != optionalTitle) && (*optionalTitle != '\0'))
	{
		nsCOMPtr<nsIRDFLiteral> literal;
		if (NS_FAILED(rv = gRDFService->GetLiteral(nsAutoString(optionalTitle).GetUnicode(), getter_AddRefs(literal))))
		{
			NS_ERROR("unable to create literal for bookmark name");
			return rv;
		}

		rv = mDataSource->Assert(bookmark, kNC_Name, literal, PR_TRUE);
        if (rv != NS_RDF_ASSERTION_ACCEPTED)
		{
			NS_ERROR("unable to set bookmark name");
			return rv;
		}
	}

	AssertTime(bookmark, kNC_BookmarkAddDate, addDate);
	AssertTime(bookmark, kWEB_LastVisitDate, lastVisitDate);
	AssertTime(bookmark, kWEB_LastModifiedDate, lastModifiedDate);

	if ((nsnull != shortcutURL) && (*shortcutURL != '\0'))
	{
		nsCOMPtr<nsIRDFLiteral> shortcutLiteral;
		if (NS_FAILED(rv = gRDFService->GetLiteral(nsAutoString(shortcutURL).GetUnicode(),
			getter_AddRefs(shortcutLiteral))))
		{
			NS_ERROR("unable to get literal for bookmark shortcut URL");
			return(rv);
		}
		if (rv != NS_RDF_NO_VALUE)
		{
			rv = mDataSource->Assert(bookmark,
                                     kNC_ShortcutURL,
                                     shortcutLiteral,
                                     PR_TRUE);

            if (rv != NS_RDF_ASSERTION_ACCEPTED)
			{
				NS_ERROR("unable to set bookmark shortcut URL");
				return(rv);
			}
		}
	}

	return(NS_OK);
}



nsresult
BookmarkParser::ParseBookmarkHeader(const nsString& aLine, nsIRDFResource* aContainer, nsIRDFResource *nodeType)
{
    // Snip out the header
    PRInt32 start = aLine.Find(kOpenHeading);
    NS_ASSERTION(start >= 0, "couldn't find '<H'; why am I here?");
    if (start < 0)
        return NS_ERROR_UNEXPECTED;

    start += (sizeof(kOpenHeading) - 1);
    start = aLine.Find(PRUnichar('>'), start); // skip to the end of the '<Hn>' tag

    if (start < 0) {
        NS_WARNING("couldn't find end of header tag");
        return NS_OK;
    }

    nsAutoString name;
    aLine.Right(name, aLine.Length() - (start + 1));

    PRInt32 end = name.Find(kCloseHeading);
    if (end < 0)
        NS_WARNING("No '</H' found to close the heading");

    if (end >= 0)
        name.Truncate(end);

    // Find the add date
    PRInt32 addDate = 0;

    nsAutoString s;
    ParseAttribute(aLine, kAddDateEquals, sizeof(kAddDateEquals) - 1, s);
    if (s.Length() > 0) {
        PRInt32 err;
        addDate = s.ToInteger(&err); // ignored
    }

    // Make the necessary assertions
    nsresult rv;
    nsCOMPtr<nsIRDFResource> folder;
    if (name.Equals(kPersonalToolbarFolder)) {
        folder = do_QueryInterface( kNC_PersonalToolbarFolder );
    }
    else {
        if (NS_FAILED(rv = rdf_CreateAnonymousResource(kURINC_BookmarksRoot, getter_AddRefs(folder)))) {
            NS_ERROR("unable to create anonymous resource for folder");
            return rv;
        }
    }

    nsCOMPtr<nsIRDFLiteral> literal;
    if (NS_FAILED(rv = gRDFService->GetLiteral(name.GetUnicode(), getter_AddRefs(literal)))) {
        NS_ERROR("unable to create literal for folder name");
        return rv;
    }

    rv = mDataSource->Assert(folder, kNC_Name, literal, PR_TRUE);
    if (rv != NS_RDF_ASSERTION_ACCEPTED) {
        NS_ERROR("unable to set folder name");
        return rv;
    }

    nsCOMPtr<nsIRDFContainer> container;
    rv = NS_NewRDFContainer(mDataSource, aContainer, getter_AddRefs(container));
    if (NS_FAILED(rv)) return rv;

    rv = container->AppendElement(folder);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add bookmark to container");
    if (NS_FAILED(rv)) return rv;

    NS_WITH_SERVICE(nsIRDFContainerUtils, rdfc, kRDFContainerUtilsCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = rdfc->MakeSeq(mDataSource, folder, nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to make new folder as sequence");
    if (NS_FAILED(rv)) return rv;

    rv = mDataSource->Assert(folder, kRDF_type, kNC_Folder, PR_TRUE);
    if (rv != NS_RDF_ASSERTION_ACCEPTED) {
        NS_ERROR("unable to mark new folder as folder");
        return rv;
    }

    if (NS_FAILED(rv = AssertTime(folder, kNC_BookmarkAddDate, addDate))) {
        NS_ERROR("unable to mark add date");
        return rv;
    }

    // And now recursively parse the rest of the file...
    
    if (NS_FAILED(rv = Parse(folder, nodeType))) {
        NS_ERROR("recursive parse of bookmarks file failed");
        return rv;
    }

    return NS_OK;
}


nsresult
BookmarkParser::ParseBookmarkSeparator(const nsString& aLine, nsIRDFResource* aContainer)
{
	nsresult			rv;
	nsCOMPtr<nsIRDFResource>	separator;

	if (NS_SUCCEEDED(rv = rdf_CreateAnonymousResource(kURINC_BookmarksRoot, getter_AddRefs(separator))))
	{
		nsAutoString		defaultSeparatorName("-----");
		nsCOMPtr<nsIRDFLiteral> nameLiteral;
		if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(defaultSeparatorName.GetUnicode(), getter_AddRefs(nameLiteral))))
		{
			if (NS_SUCCEEDED(rv = mDataSource->Assert(separator, kNC_Name, nameLiteral, PR_TRUE)))
			{
			}
		}
		if (NS_SUCCEEDED(rv = mDataSource->Assert(separator, kRDF_type, kNC_BookmarkSeparator, PR_TRUE)))
		{
			nsCOMPtr<nsIRDFContainer>	container;
			rv = NS_NewRDFContainer(mDataSource, aContainer, getter_AddRefs(container));
			if (NS_SUCCEEDED(rv))
			{
				rv = container->AppendElement(separator);
			}
		}
	}
	return(rv);
}



nsresult
BookmarkParser::ParseHeaderBegin(const nsString& aLine, nsIRDFResource* aContainer)
{
    return NS_OK;
}



nsresult
BookmarkParser::ParseHeaderEnd(const nsString& aLine)
{
    return NS_OK;
}


nsresult
BookmarkParser::ParseAttribute(const nsString& aLine,
                               const char* aAttributeName,
                               PRInt32 aAttributeLen,
                               nsString& aResult)
{
    aResult.Truncate();

    PRInt32 start = aLine.Find(aAttributeName);
    if (start < 0)
        return NS_OK;

    start += aAttributeLen;
    PRInt32 end = aLine.Find(PRUnichar('"'), start);
    aLine.Mid(aResult, start, end - start);

    return NS_OK;
}

nsresult
BookmarkParser::AssertTime(nsIRDFResource* aSource,
                           nsIRDFResource* aLabel,
                           PRInt32 aTime)
{
    // XXX TO DO: Convert to a date literal

    nsAutoString time;
    time.Append(aTime, 10);

    nsresult rv;
    nsIRDFLiteral* literal;
    if (NS_FAILED(rv = gRDFService->GetLiteral(time.GetUnicode(), &literal))) {
        NS_ERROR("unable to get literal for time");
        return rv;
    }

    rv = mDataSource->Assert(aSource, aLabel, literal, PR_TRUE);
    NS_ASSERTION(rv == NS_RDF_ASSERTION_ACCEPTED, "unable to assert time");

    NS_RELEASE(literal);
    return rv;
}

////////////////////////////////////////////////////////////////////////
// BookmarkDataSourceImpl

/**
 * The bookmark data source uses a <tt>BookmarkParser</tt> to read the
 * <tt>bookmarks.html</tt> file from the local disk and present an
 * in-memory RDF graph based on it.
 */

class BookmarkDataSourceImpl : public nsIRDFBookmarkDataSource {
protected:
    nsIRDFDataSource* mInner;

    nsresult ReadBookmarks(void);
    nsresult WriteBookmarks(nsIRDFDataSource *ds, nsIRDFResource *root);
    nsresult WriteBookmarksContainer(nsIRDFDataSource *ds, nsOutputFileStream strm, nsIRDFResource *container, PRInt32 level);
    nsresult WriteBookmarkProperties(nsIRDFDataSource *ds, nsOutputFileStream strm, nsIRDFResource *node,
    					nsIRDFResource *property, const char *htmlAttrib, PRBool isFirst);
    PRBool CanAccept(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aTarget);

public:
    BookmarkDataSourceImpl(void);
    virtual ~BookmarkDataSourceImpl(void);

    // nsISupports
    NS_DECL_ISUPPORTS

    // nsIRDFBookmarkDataSource
    NS_IMETHOD AddBookmark(const char *uri, const char *optionalTitle)
    {
    	// XXX for the moment, just add it as a child of BookmarksRoot
	BookmarkParser parser;
	parser.Init(nsnull, NS_STATIC_CAST(nsIRDFDataSource *, this));
	nsresult rv = parser.AddBookmark(kNC_BookmarksRoot, uri, optionalTitle,
					0L, 0L, 0L, nsnull, kNC_Bookmark);
	if (NS_SUCCEEDED(rv))
	{
		Flush();
	}
    	return(rv);
    }

    NS_IMETHOD FindBookmarkShortcut(const char *userInput, char **shortcutURL)
    {
    	nsresult	rv = NS_RDF_NO_VALUE;
    	if (nsnull != shortcutURL)
    	{
    		*shortcutURL = nsnull;
    		nsCOMPtr<nsIRDFLiteral> literalTarget;
    		if (NS_FAILED(rv = gRDFService->GetLiteral(nsAutoString(userInput).GetUnicode(),
    			getter_AddRefs(literalTarget))))
    		{
    		}
    		else if (rv != NS_RDF_NO_VALUE)
    		{
    			nsCOMPtr<nsIRDFResource> source;
    			if (NS_FAILED(rv = GetSource(kNC_ShortcutURL, literalTarget,
    				PR_TRUE, getter_AddRefs(source))))
    			{
    			}
    			else if (rv != NS_RDF_NO_VALUE)
    			{
    				nsXPIDLCString	uri;
    				source->GetValue(getter_Copies(uri));
    				nsAutoString	url(uri);
    				*shortcutURL = url.ToNewCString();
    			}
    		}
    	}
    	return(rv);
    }

    // nsIRDFDataSource
    NS_IMETHOD Init(const char* uri);

    NS_IMETHOD GetURI(char* *uri)  {
        return mInner->GetURI(uri);
    }

    NS_IMETHOD GetSource(nsIRDFResource* property,
                         nsIRDFNode* target,
                         PRBool tv,
                         nsIRDFResource** source) {
        return mInner->GetSource(property, target, tv, source);
    }

    NS_IMETHOD GetSources(nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          nsISimpleEnumerator** sources) {
        return mInner->GetSources(property, target, tv, sources);
    }

    NS_IMETHOD GetTarget(nsIRDFResource* source,
                         nsIRDFResource* property,
                         PRBool tv,
                         nsIRDFNode** target);

    NS_IMETHOD GetTargets(nsIRDFResource* source,
                          nsIRDFResource* property,
                          PRBool tv,
                          nsISimpleEnumerator** targets) {
        return mInner->GetTargets(source, property, tv, targets);
    }

    NS_IMETHOD Assert(nsIRDFResource* aSource, 
                      nsIRDFResource* aProperty, 
                      nsIRDFNode* aTarget,
                      PRBool aTruthValue);

    NS_IMETHOD Unassert(nsIRDFResource* aSource,
                        nsIRDFResource* aProperty,
                        nsIRDFNode* aTarget);

    NS_IMETHOD HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target,
                            PRBool tv,
                            PRBool* hasAssertion) {
        return mInner->HasAssertion(source, property, target, tv, hasAssertion);
    }

    NS_IMETHOD AddObserver(nsIRDFObserver* n) {
        return mInner->AddObserver(n);
    }

    NS_IMETHOD RemoveObserver(nsIRDFObserver* n) {
        return mInner->RemoveObserver(n);
    }

    NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
                           nsISimpleEnumerator** labels) {
        return mInner->ArcLabelsIn(node, labels);
    }

    NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
                            nsISimpleEnumerator** labels) {
        return mInner->ArcLabelsOut(source, labels);
    }

    NS_IMETHOD GetAllResources(nsISimpleEnumerator** aResult) {
        return mInner->GetAllResources(aResult);
    }

    NS_IMETHOD Flush(void);

    NS_IMETHOD GetAllCommands(nsIRDFResource* source,
                              nsIEnumerator/*<nsIRDFResource>*/** commands);

    NS_IMETHOD IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                nsIRDFResource*   aCommand,
                                nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                PRBool* aResult);

    NS_IMETHOD DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                         nsIRDFResource*   aCommand,
                         nsISupportsArray/*<nsIRDFResource>*/* aArguments);

};


////////////////////////////////////////////////////////////////////////

BookmarkDataSourceImpl::BookmarkDataSourceImpl(void)
{
	// XXX rvg there should be only one instance of this class. 
	// this is actually true of all datasources.
	NS_INIT_REFCNT();
    bm_AddRefGlobals();
}

BookmarkDataSourceImpl::~BookmarkDataSourceImpl(void)
{
	Flush();
	NS_RELEASE(mInner);
	bm_ReleaseGlobals();
}

//NS_IMPL_ISUPPORTS(BookmarkDataSourceImpl, kIRDFDataSourceIID);
NS_IMPL_ADDREF(BookmarkDataSourceImpl);
NS_IMPL_RELEASE(BookmarkDataSourceImpl);

NS_IMETHODIMP
BookmarkDataSourceImpl::QueryInterface(REFNSIID iid, void **result)
{
	if (! result)
		return NS_ERROR_NULL_POINTER;

	*result = nsnull;
	if (iid.Equals(kIRDFDataSourceIID) ||
		iid.Equals(kIRDFBookmarkDataSourceIID))
	{
		*result = NS_STATIC_CAST(nsIRDFBookmarkDataSource *, this);
		AddRef();
		return NS_OK;
	}
	return(NS_NOINTERFACE);
}

NS_IMETHODIMP
BookmarkDataSourceImpl::Init(const char* uri)
{
    nsresult rv;

    if (NS_FAILED(rv = nsComponentManager::CreateInstance(kRDFInMemoryDataSourceCID,
                                                    nsnull,
                                                    kIRDFDataSourceIID,
                                                    (void**) &mInner)))
        return rv;

    if (NS_FAILED(rv = mInner->Init(uri)))
        return rv;

    if (NS_FAILED(rv = ReadBookmarks()))
        return rv;

    // register this as a named data source with the RDF service
    return gRDFService->RegisterDataSource(this, PR_FALSE);
}

NS_IMETHODIMP
BookmarkDataSourceImpl::GetTarget(nsIRDFResource* aSource,
                                  nsIRDFResource* aProperty,
                                  PRBool aTruthValue,
                                  nsIRDFNode** aTarget)
{
    nsresult rv;

    // If they want the URL...
    if (aTruthValue && aProperty == kNC_URL) {
        // ...and it is in fact a bookmark...
        PRBool hasAssertion;
        if (NS_SUCCEEDED(mInner->HasAssertion(aSource, kRDF_type, kNC_Bookmark, PR_TRUE, &hasAssertion))
            && hasAssertion) {

            nsXPIDLCString uri;
            if (NS_FAILED(rv = aSource->GetValue( getter_Copies(uri) ))) {
                NS_ERROR("unable to get source's URI");
                return rv;
            }

		nsAutoString	ncURI(uri);
		if (ncURI.Find("NC:") == 0)
		{
			return(NS_RDF_NO_VALUE);
		}

            nsIRDFLiteral* literal;
            if (NS_FAILED(rv = gRDFService->GetLiteral(nsAutoString(uri).GetUnicode(), &literal))) {
                NS_ERROR("unable to construct literal for URL");
                return rv;
            }

            *aTarget = (nsIRDFNode*)literal;
            return NS_OK;
        }
    }

    return mInner->GetTarget(aSource, aProperty, aTruthValue, aTarget);
}


NS_IMETHODIMP
BookmarkDataSourceImpl::Assert(nsIRDFResource* aSource,
                               nsIRDFResource* aProperty,
                               nsIRDFNode* aTarget,
                               PRBool aTruthValue)
{
    if (CanAccept(aSource, aProperty, aTarget)) {
        return mInner->Assert(aSource, aProperty, aTarget, aTruthValue);
    }
    else {
        return NS_RDF_ASSERTION_REJECTED;
    }
}



NS_IMETHODIMP
BookmarkDataSourceImpl::Unassert(nsIRDFResource* aSource,
                                 nsIRDFResource* aProperty,
                                 nsIRDFNode* aTarget)
{
    if (CanAccept(aSource, aProperty, aTarget)) {
        return mInner->Unassert(aSource, aProperty, aTarget);
    }
    else {
        return NS_RDF_ASSERTION_REJECTED;
    }
}



NS_IMETHODIMP
BookmarkDataSourceImpl::Flush(void)
{
    return WriteBookmarks(mInner, kNC_BookmarksRoot);
}


NS_IMETHODIMP
BookmarkDataSourceImpl::GetAllCommands(nsIRDFResource* source,
                                       nsIEnumerator/*<nsIRDFResource>*/** commands)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
BookmarkDataSourceImpl::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                         nsIRDFResource*   aCommand,
                                         nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                         PRBool* aResult)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
BookmarkDataSourceImpl::DoCommand(nsISupportsArray* aSources,
                                  nsIRDFResource*   aCommand,
                                  nsISupportsArray* aArguments)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
BookmarkDataSourceImpl::ReadBookmarks(void)
{
	nsresult rv;

	NS_WITH_SERVICE(nsIRDFContainerUtils, rdfc, kRDFContainerUtilsCID, &rv);
	if (NS_FAILED(rv)) return rv;

	rv = rdfc->MakeSeq(mInner, kNC_BookmarksRoot, nsnull);
	NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to make NC:BookmarksRoot a sequence");
	if (NS_FAILED(rv)) return rv;

	nsSpecialSystemDirectory bookmarksFile(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);

	// XXX we should get this from prefs.
	bookmarksFile += "res";
	bookmarksFile += "rdf";
	bookmarksFile += "bookmarks.html";

	nsInputFileStream strm(bookmarksFile);

	if (! strm.is_open())
	{
		NS_ERROR("unable to open file");
		return NS_ERROR_FAILURE;
	}

	BookmarkParser parser;
	parser.Init(&strm, NS_STATIC_CAST(nsIRDFDataSource *, this));

#ifdef	XP_MAC
	parse.SetIEFavoritesRoot(kURINC_IEFavoritesRoot);
#endif

#ifdef	XP_WIN
	nsCOMPtr<nsIRDFResource>	ieFolder;
	nsSpecialSystemDirectory	ieFavoritesFile(nsSpecialSystemDirectory::Win_Favorites);
	nsFileURL			ieFavoritesURLSpec(ieFavoritesFile);
	const char			*ieFavoritesURL = ieFavoritesURLSpec.GetAsString();
	parser.SetIEFavoritesRoot(ieFavoritesURL);
#endif

	parser.Parse(kNC_BookmarksRoot, kNC_Bookmark);

	PRBool	foundIERoot = PR_FALSE;
	parser.ParserFoundIEFavoritesRoot(&foundIERoot);

	// look for and import any IE Favorites
	nsAutoString	ieTitle("Imported IE Favorites");		// XXX localization?

#ifdef	XP_MAC
	nsSpecialSystemDirectory ieFavoritesFile(nsSpecialSystemDirectory::Mac_PreferencesDirectory);
	ieFavoritesFile += "Explorer";
	ieFavoritesFile += "Favorites.html";

	nsInputFileStream	ieStream(ieFavoritesFile);
	if (strm.is_open())
	{
		NS_WITH_SERVICE(nsIRDFContainerUtils, rdfc, kRDFContainerUtilsCID, &rv);
		if (NS_SUCCEEDED(rv))
		{
			if (NS_SUCCEEDED(rv = rdfc->MakeSeq(mInner, kNC_IEFavoritesRoot, nsnull)))
			{
				BookmarkParser parser;
				parser.Init(&ieStream, this);
				parser.Parse(kNC_IEFavoritesRoot, kNC_IEFavorite);
				
				nsCOMPtr<nsIRDFLiteral>	ieTitleLiteral;
				if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(ieTitle, getter_AddRefs(ieTitleLiteral))))
				{
					rv = mInner->Assert(kNC_IEFavoritesRoot, kNC_Name, ieTitleLiteral, PR_TRUE);
				}
				
				// if the IE Favorites root isn't somewhere in bookmarks.html, add it
				if (foundIERoot == PR_FALSE)
				{
					nsCOMPtr<nsIRDFContainer> bookmarksRoot;
					rv = NS_NewRDFContainer(mInner, kNC_BookmarksRoot, getter_AddRefs(bookmarksRoot));
					if (NS_SUCCEEDED(rv))
					{
						rv = bookmarksRoot->AppendElement(kNC_IEFavoritesRoot);
					}
				}
			}
		}
	}
#endif

#ifdef	XP_WIN
	if (NS_SUCCEEDED(rv = gRDFService->GetResource(ieFavoritesURL, getter_AddRefs(ieFolder))))
	{
		nsCOMPtr<nsIRDFLiteral>	ieTitleLiteral;
		if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(ieTitle.GetUnicode(), getter_AddRefs(ieTitleLiteral))))
		{
			rv = mInner->Assert(ieFolder, kNC_Name, ieTitleLiteral, PR_TRUE);
		}

		// if the IE Favorites root isn't somewhere in bookmarks.html, add it
		if (foundIERoot == PR_FALSE)
		{
			nsCOMPtr<nsIRDFContainer> container;
			rv = NS_NewRDFContainer(mInner, kNC_BookmarksRoot, getter_AddRefs(container));
			if (NS_SUCCEEDED(rv))
			{
				rv = container->AppendElement(ieFolder);
			}
		}
	}
#endif

	return NS_OK;	
}


nsresult
BookmarkDataSourceImpl::WriteBookmarks(nsIRDFDataSource *ds, nsIRDFResource *root)
{
	nsSpecialSystemDirectory bookmarksFile(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);

	// XXX we should get this from prefs.
	bookmarksFile += "res";
	bookmarksFile += "rdf";
	bookmarksFile += "bookmarks.html";

	nsresult		rv = NS_ERROR_FAILURE;
	nsOutputFileStream	strm(bookmarksFile);
	if (strm.is_open())
	{
		strm << "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n";
		strm << "<!-- This is an automatically generated file.\n";
		strm << "It will be read and overwritten.\n";
		strm << "Do Not Edit! -->\n";
		strm << "<TITLE>Bookmarks</TITLE>\n";
		strm << "<H1>Bookmarks</H1>\n\n";
		
		rv = WriteBookmarksContainer(ds, strm, root, 0);
	}
	return(rv);
}


nsresult
BookmarkDataSourceImpl::WriteBookmarksContainer(nsIRDFDataSource *ds, nsOutputFileStream strm, nsIRDFResource *parent, PRInt32 level)
{
	nsresult	rv = NS_OK;

	nsAutoString	indentationString("");
	for (PRInt32 loop=0; loop<level; loop++)	indentationString += "    ";
	char		*indentation = indentationString.ToNewCString();
	if (nsnull == indentation)	return(NS_ERROR_OUT_OF_MEMORY);

	nsCOMPtr<nsIRDFContainer> container;
	if (NS_SUCCEEDED(rv = NS_NewRDFContainer(ds, parent, getter_AddRefs(container))))
	{
		nsCOMPtr<nsIRDFContainerUtils> containerUtils;
		if (NS_SUCCEEDED(rv = NS_NewRDFContainerUtils(getter_AddRefs(containerUtils))))
		{
			strm << indentation;
			strm << "<DL><p>\n";

			nsCOMPtr<nsISimpleEnumerator>	children;
			if (NS_SUCCEEDED(rv = container->GetElements(getter_AddRefs(children))))
			{
				PRBool	more = PR_TRUE;
				while (more == PR_TRUE)
				{
					if (NS_FAILED(rv = children->HasMoreElements(&more)))	break;
					if (more != PR_TRUE)	break;

					nsCOMPtr<nsISupports>	iSupports;					
					if (NS_FAILED(rv = children->GetNext(getter_AddRefs(iSupports))))	break;

					nsCOMPtr<nsIRDFResource>	child = do_QueryInterface(iSupports);
					if (!child)	break;

					PRBool	isIERoot = PR_FALSE, isContainer = PR_FALSE;
					if (NS_SUCCEEDED(child->EqualsResource(kNC_IEFavoritesRoot, &isIERoot)))
					{
						if (isIERoot == PR_FALSE)
						{
							if (NS_SUCCEEDED(rv = containerUtils->IsContainer(ds, child, &isContainer)))
							{
							}
						}
					}

					nsCOMPtr<nsIRDFNode>	nameNode;
					nsAutoString		nameString("");
					char			*name = nsnull;
					if (NS_SUCCEEDED(rv = ds->GetTarget(child, kNC_Name, PR_TRUE, getter_AddRefs(nameNode))))
					{
						nsCOMPtr<nsIRDFLiteral>	nameLiteral = do_QueryInterface(nameNode);
						PRUnichar	*title = nsnull;
						if (NS_SUCCEEDED(rv = nameLiteral->GetValue(&title)))
						{
							nameString = title;
							name = nameString.ToNewCString();
						}
						
					}

					strm << indentation;
					strm << "    ";
					if (isContainer == PR_TRUE)
					{
						strm << "<DT><H3";
						// output ADD_DATE
						WriteBookmarkProperties(ds, strm, child, kNC_BookmarkAddDate, kAddDateEquals, PR_FALSE);
						strm << ">";

						// output title
						if (name)	strm << name;
						strm << "</H3>\n";
						rv = WriteBookmarksContainer(ds, strm, child, level+1);
					}
					else
					{
						char	*url = nsnull;
						if (NS_SUCCEEDED(rv = child->GetValue(&url)))
						{
							if (url)
							{
								nsAutoString	uri(url);
								// XXX What's the best way to determine if its a separator?
								if (uri.Find(kURINC_BookmarksRoot) == 0)
								{
									// its a separator
									strm << "<HR>\n";
								}
								else
								{
									strm << "<DT><A HREF=\"";
									// output URL
									strm << url;
									strm << "\"";
									
									// output ADD_DATE
									WriteBookmarkProperties(ds, strm, child, kNC_BookmarkAddDate, kAddDateEquals, PR_FALSE);

									// output LAST_VISIT
									WriteBookmarkProperties(ds, strm, child, kWEB_LastVisitDate, kLastVisitEquals, PR_FALSE);
									
									// output LAST_MODIFIED
									WriteBookmarkProperties(ds, strm, child, kWEB_LastModifiedDate, kLastModifiedEquals, PR_FALSE);
									
									// output SHORTCUTURL
									WriteBookmarkProperties(ds, strm, child, kNC_ShortcutURL, kShortcutURLEquals, PR_FALSE);
									
									strm << ">";
									// output title
									if (name)	strm << name;
									strm << "</A>\n";
								}
							}
						}
					}

					if (nsnull != name)
					{
						delete []name;
						name = nsnull;
					}
					
					if (NS_FAILED(rv))	break;
				}
			}
			strm << indentation;
			strm << "</DL><p>\n";
		}
	}
	delete [] indentation;
	return(rv);
}


nsresult
BookmarkDataSourceImpl::WriteBookmarkProperties(nsIRDFDataSource *ds, nsOutputFileStream strm,
	nsIRDFResource *child, nsIRDFResource *property, const char *htmlAttrib, PRBool isFirst)
{
	nsresult		rv;
	nsCOMPtr<nsIRDFNode>	node;
	if (NS_SUCCEEDED(rv = ds->GetTarget(child, property, PR_TRUE, getter_AddRefs(node))))
	{
		nsCOMPtr<nsIRDFLiteral>	literal = do_QueryInterface(node);
		if (literal)
		{
			PRUnichar	*literalUni = nsnull;
			if (NS_SUCCEEDED(rv = literal->GetValue(&literalUni)))
			{
				nsAutoString	literalString = literalUni;
				char		*attribute = literalString.ToNewCString();
				if (nsnull != attribute)
				{
					if (isFirst == PR_FALSE)
					{
						strm << " ";
					}
					strm << htmlAttrib;
					strm << attribute;
					strm << "\"";
					delete [] attribute;
					attribute = nsnull;
				}
			}
		}
	}
	return(rv);
}


PRBool
BookmarkDataSourceImpl::CanAccept(nsIRDFResource* aSource,
                                  nsIRDFResource* aProperty,
                                  nsIRDFNode* aTarget)
{
    // XXX This is really crippled, and needs to be stricter. We want
    // to exclude any property that isn't talking about a known
    // bookmark.
    nsresult rv;
    NS_WITH_SERVICE(nsIRDFContainerUtils, rdfc, kRDFContainerUtilsCID, &rv);
    if (NS_FAILED(rv))
        return PR_FALSE;

    PRBool isOrdinal;
    rv = rdfc->IsOrdinalProperty(aProperty, &isOrdinal);
    if (NS_FAILED(rv))
        return PR_FALSE;

    if (isOrdinal) {
        return PR_TRUE;
    }
    else if ((aProperty != kRDF_type) ||
        (aProperty != kWEB_LastModifiedDate) ||
        (aProperty != kWEB_LastVisitDate) ||
        (aProperty != kNC_Name) ||
        (aProperty != kNC_BookmarkAddDate)) {
        return PR_TRUE;
    }
    else {
        return PR_FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////

#include "nsXPComFactory.h"

NS_DEF_FACTORY(BookmarkDataSource,BookmarkDataSourceImpl)

nsresult
NS_NewRDFBookmarkDataSourceFactory(nsIFactory** aResult)
{
    nsresult rv = NS_OK;
    nsIFactory* inst = new nsBookmarkDataSourceFactory;
    if (NULL == inst) {
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        NS_ADDREF(inst);
    }
    *aResult = inst;
    return rv;
}

nsresult
NS_NewRDFBookmarkDataSource(nsIRDFDataSource** result)
{
    BookmarkDataSourceImpl* ds = new BookmarkDataSourceImpl();
    if (! ds)
        return NS_ERROR_NULL_POINTER;

    *result = ds;
    NS_ADDREF(*result);
    return NS_OK;
}
