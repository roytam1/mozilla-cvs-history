/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; c-file-style: "stroustrup" -*-
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

  The global bookmarks service.

 */

#include "nsCOMPtr.h"
#include "nsFileSpec.h"
#include "nsFileStream.h"
#include "nsIBookmarksService.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsSpecialSystemDirectory.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsXPIDLString.h"
#include "prio.h"
#include "prlog.h"
#include "rdf.h"
#include "xp_core.h"

////////////////////////////////////////////////////////////////////////

static NS_DEFINE_CID(kBookmarksServiceCID,      NS_BOOKMARKS_SERVICE_CID);
static NS_DEFINE_CID(kComponentManagerCID,      NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kGenericFactoryCID,        NS_GENERICFACTORY_CID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,            NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerCID,          NS_RDFCONTAINER_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,     NS_RDFCONTAINERUTILS_CID);
static NS_DEFINE_IID(kISupportsIID,             NS_ISUPPORTS_IID);

static const char kURINC_BookmarksRoot[]         = "NC:BookmarksRoot"; // XXX?
static const char kURINC_IEFavoritesRoot[]       = "NC:IEFavoritesRoot"; // XXX?
static const char kURINC_PersonalToolbarFolder[] = "NC:PersonalToolbarFolder"; // XXX?

static const char kPersonalToolbarFolder[]  = "Personal Toolbar Folder";


////////////////////////////////////////////////////////////////////////

PRInt32 gRefCnt;
nsIRDFService* gRDF;
nsIRDFContainerUtils* gRDFC;
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

static nsresult
bm_AddRefGlobals()
{
    if (gRefCnt++ == 0) {
        nsresult rv;
        rv = nsServiceManager::GetService(kRDFServiceCID,
                                          nsIRDFService::GetIID(),
                                          (nsISupports**) &gRDF);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
        if (NS_FAILED(rv)) return rv;


		rv = nsServiceManager::GetService(kRDFContainerUtilsCID,
										  nsIRDFContainerUtils::GetIID(),
										  (nsISupports**) &gRDFC);

		NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF container utils");
		if (NS_FAILED(rv)) return rv;

        gRDF->GetResource(kURINC_BookmarksRoot,         &kNC_BookmarksRoot);
        gRDF->GetResource(kURINC_IEFavoritesRoot,       &kNC_IEFavoritesRoot);
        gRDF->GetResource(kURINC_PersonalToolbarFolder, &kNC_PersonalToolbarFolder);

        gRDF->GetResource(NC_NAMESPACE_URI "Bookmark",          &kNC_Bookmark);
        gRDF->GetResource(NC_NAMESPACE_URI "BookmarkSeparator", &kNC_BookmarkSeparator);
        gRDF->GetResource(NC_NAMESPACE_URI "BookmarkAddDate",   &kNC_BookmarkAddDate);
        gRDF->GetResource(NC_NAMESPACE_URI "Description",       &kNC_Description);
        gRDF->GetResource(NC_NAMESPACE_URI "Folder",            &kNC_Folder);
        gRDF->GetResource(NC_NAMESPACE_URI "IEFavorite",        &kNC_IEFavorite);
        gRDF->GetResource(NC_NAMESPACE_URI "Name",              &kNC_Name);
        gRDF->GetResource(NC_NAMESPACE_URI "ShortcutURL",       &kNC_ShortcutURL);
        gRDF->GetResource(NC_NAMESPACE_URI "URL",               &kNC_URL);
        gRDF->GetResource(RDF_NAMESPACE_URI "type",             &kRDF_type);
        gRDF->GetResource(WEB_NAMESPACE_URI "LastModifiedDate", &kWEB_LastModifiedDate);
        gRDF->GetResource(WEB_NAMESPACE_URI "LastVisitDate",    &kWEB_LastVisitDate);
    }
    return NS_OK;
}


static void
bm_ReleaseGlobals()
{
    if (--gRefCnt == 0)
	{
        if (gRDF)
            nsServiceManager::ReleaseService(kRDFServiceCID, gRDF);

		if (gRDFC)
			nsServiceManager::ReleaseService(kRDFContainerUtilsCID, gRDFC);

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

    nsresult CreateAnonymousResource(nsCOMPtr<nsIRDFResource>* aResult);

    nsresult ParseBookmark(const nsString& aLine,
						   nsCOMPtr<nsIRDFContainer>& aContainer,
						   nsIRDFResource *nodeType);

    nsresult ParseBookmarkHeader(const nsString& aLine,
								 nsCOMPtr<nsIRDFContainer>& aContainer,
								 nsIRDFResource *nodeType);

    nsresult ParseBookmarkSeparator(const nsString& aLine,
									nsCOMPtr<nsIRDFContainer>& aContainer);

    nsresult ParseHeaderBegin(const nsString& aLine,
							  nsCOMPtr<nsIRDFContainer>& aContainer);

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

    nsresult AddBookmark(nsCOMPtr<nsIRDFContainer>& aContainer,
                         const char*      aURL,
                         const PRUnichar* aOptionalTitle,
                         PRInt32          aAddDate,
                         PRInt32          aLastVisitDate,
                         PRInt32          aLastModifiedDate,
                         const char*      aShortcutURL,
                         nsIRDFResource*  aNodeType);

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

BookmarkParser::~BookmarkParser()
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
static const char kIDEquals[]           = "ID=\"";


nsresult
BookmarkParser::Parse(nsIRDFResource* aContainer, nsIRDFResource *nodeType)
{
	// tokenize the input stream.
	// XXX this needs to handle quotes, etc. it'd be nice to use the real parser for this...
    nsRandomAccessInputStream in(*mStream);

	nsresult rv;

    nsCOMPtr<nsIRDFContainer> container;
	rv = nsComponentManager::CreateInstance(kRDFContainerCID,
											nsnull,
											nsIRDFContainer::GetIID(),
											getter_AddRefs(container));
    if (NS_FAILED(rv)) return rv;

	rv = container->Init(mDataSource, aContainer);
	if (NS_FAILED(rv)) return rv;

    nsAutoString line;
	while (NS_SUCCEEDED(rv) && !in.eof() && !in.failed()) {
        line.Truncate();

        while (1) {
            char buf[256];
            PRBool untruncated = in.readline(buf, sizeof(buf));

            // in.readline() return PR_FALSE if there was buffer overflow,
            // or there was a catastrophe. Check to see if we're here
            // because of the latter...
            NS_ASSERTION (! in.failed(), "error reading file");
            if (in.failed()) return NS_ERROR_FAILURE;

            // XXX Bug 5871. What charset conversion should we be
            // applying here?
            line.Append(buf);

            if (untruncated)
                break;
        }

        PRInt32 offset;

        if ((offset = line.Find(kHREFEquals)) >= 0) {
            rv = ParseBookmark(line, container, nodeType);
        }
        else if ((offset = line.Find(kOpenHeading)) >= 0 &&
                 nsString::IsDigit(line.CharAt(offset + 2))) {
            // XXX Ignore <H1> so that bookmarks root _is_ <H1>
            if (line.CharAt(offset + 2) != PRUnichar('1'))
                rv = ParseBookmarkHeader(line, container, nodeType);
        }
        else if ((offset = line.Find(kSeparator)) >= 0) {
            rv = ParseBookmarkSeparator(line, container);
        }
        else if ((offset = line.Find(kCloseUL)) >= 0 ||
                 (offset = line.Find(kCloseMenu)) >= 0 ||
                 (offset = line.Find(kCloseDL)) >= 0) {
            return ParseHeaderEnd(line);
        }
        else if ((offset = line.Find(kOpenUL)) >= 0 ||
                 (offset = line.Find(kOpenMenu)) >= 0 ||
                 (offset = line.Find(kOpenDL)) >= 0) {
            rv = ParseHeaderBegin(line, container);
        }
        else {
            // XXX Discard the line. We should be treating this as the
            // description.
        }
    }
    return rv;
}


nsresult
BookmarkParser::CreateAnonymousResource(nsCOMPtr<nsIRDFResource>* aResult)
{
	static PRInt32 gNext = 0;
	if (! gNext) {
		LL_L2I(gNext, PR_Now());
	}
	nsAutoString uri(kURINC_BookmarksRoot);
	uri.Append("#$");
	uri.Append(++gNext, 16);

	return gRDF->GetUnicodeResource(uri.GetUnicode(), getter_AddRefs(*aResult));
}

nsresult
BookmarkParser::ParseBookmark(const nsString& aLine,
							  nsCOMPtr<nsIRDFContainer>& aContainer,
							  nsIRDFResource *nodeType)
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

    nsAutoString	shortcut;
    ParseAttribute(aLine, kShortcutURLEquals, sizeof(kShortcutURLEquals) -1, shortcut);


    // Dunno. 4.5 did it, so will we.
    if (!lastModifiedDate)
        lastModifiedDate = lastVisitDate;

    // XXX There was some other cruft here to deal with aliases, but
    // since I have no clue what those are, I'll punt.

	nsresult rv = NS_ERROR_OUT_OF_MEMORY; // in case ToNewCString() fails

	char *cURL = url.ToNewCString();
	if (cURL) {
		char *cShortcutURL = shortcut.ToNewCString();
		if (cShortcutURL) {
			rv = AddBookmark(aContainer,
							 cURL,
							 name.GetUnicode(),
							 addDate,
							 lastVisitDate,
							 lastModifiedDate,
							 cShortcutURL,
							 nodeType);
		}
		delete [] cShortcutURL;
	}
	delete [] cURL;

	return rv;
}



    // Now create the bookmark
nsresult
BookmarkParser::AddBookmark(nsCOMPtr<nsIRDFContainer>&  aContainer,
                            const char*      aURL,
                            const PRUnichar* aOptionalTitle,
                            PRInt32          aAddDate,
                            PRInt32          aLastVisitDate,
                            PRInt32          aLastModifiedDate,
                            const char*      aShortcutURL,
                            nsIRDFResource*  aNodeType)
{
	nsresult rv;
	nsCOMPtr<nsIRDFResource> bookmark;

	if (NS_FAILED(rv = gRDF->GetResource(aURL, getter_AddRefs(bookmark) )))
	{
		NS_ERROR("unable to get bookmark resource");
		return rv;
	}

	if (nsnull != mIEFavoritesRoot)
	{
		if (!PL_strcmp(aURL, mIEFavoritesRoot))
		{
			mFoundIEFavoritesRoot = PR_TRUE;
		}
	}

    rv = aContainer->AppendElement(bookmark);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add bookmark to container");
    if (NS_FAILED(rv)) return rv;

	rv = mDataSource->Assert(bookmark, kRDF_type, aNodeType, PR_TRUE);
    if (rv != NS_RDF_ASSERTION_ACCEPTED)
    {
		NS_ERROR("unable to add bookmark to data source");
		return rv;
	}

	if ((nsnull != aOptionalTitle) && (*aOptionalTitle != PRUnichar('\0')))
	{
		nsCOMPtr<nsIRDFLiteral> literal;
		if (NS_FAILED(rv = gRDF->GetLiteral(aOptionalTitle, getter_AddRefs(literal))))
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

	AssertTime(bookmark, kNC_BookmarkAddDate, aAddDate);
	AssertTime(bookmark, kWEB_LastVisitDate, aLastVisitDate);
	AssertTime(bookmark, kWEB_LastModifiedDate, aLastModifiedDate);

	if ((nsnull != aShortcutURL) && (*aShortcutURL != '\0'))
	{
		nsCOMPtr<nsIRDFLiteral> shortcutLiteral;
		if (NS_FAILED(rv = gRDF->GetLiteral(nsAutoString(aShortcutURL).GetUnicode(),
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
BookmarkParser::ParseBookmarkHeader(const nsString& aLine,
									nsCOMPtr<nsIRDFContainer>& aContainer,
									nsIRDFResource *nodeType)
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

    nsAutoString id;
    ParseAttribute(aLine, kIDEquals, sizeof(kIDEquals) - 1, id);

    // Make the necessary assertions
    nsresult rv;
    nsCOMPtr<nsIRDFResource> folder;
    if (id.Length() > 0) {
        // Use the ID attribute, if one is set.
        rv = gRDF->GetUnicodeResource(id.GetUnicode(), getter_AddRefs(folder));
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create resource for folder");
        if (NS_FAILED(rv)) return rv;
    }
    else if (name.Equals(kPersonalToolbarFolder)) { // XXX I18n!!!
        folder = dont_QueryInterface( kNC_PersonalToolbarFolder );
    }
    else {
        // We've never seen this folder before. Assign it an anonymous ID
        rv = CreateAnonymousResource(&folder);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create anonymous resource for folder");
        if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsIRDFLiteral> literal;
    rv = gRDF->GetLiteral(name.GetUnicode(), getter_AddRefs(literal));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create literal for folder name");
    if (NS_FAILED(rv)) return rv;

    rv = mDataSource->Assert(folder, kNC_Name, literal, PR_TRUE);
    if (rv != NS_RDF_ASSERTION_ACCEPTED) {
        NS_ERROR("unable to set folder name");
        return rv;
    }

    rv = gRDFC->MakeSeq(mDataSource, folder, nsnull);
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

    rv = aContainer->AppendElement(folder);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add folder to container");
    if (NS_FAILED(rv)) return rv;

    // And now recursively parse the rest of the file...

    if (NS_FAILED(rv = Parse(folder, nodeType))) {
        NS_ERROR("recursive parse of bookmarks file failed");
        return rv;
    }

    return NS_OK;
}


nsresult
BookmarkParser::ParseBookmarkSeparator(const nsString& aLine,
									   nsCOMPtr<nsIRDFContainer>& aContainer)
{
	nsresult			rv;
	nsCOMPtr<nsIRDFResource>	separator;

	if (NS_SUCCEEDED(rv = CreateAnonymousResource(&separator)))
	{
		nsAutoString		defaultSeparatorName("-----");
		nsCOMPtr<nsIRDFLiteral> nameLiteral;
		if (NS_SUCCEEDED(rv = gRDF->GetLiteral(defaultSeparatorName.GetUnicode(), getter_AddRefs(nameLiteral))))
		{
			if (NS_SUCCEEDED(rv = mDataSource->Assert(separator, kNC_Name, nameLiteral, PR_TRUE)))
			{
			}
		}
		if (NS_SUCCEEDED(rv = mDataSource->Assert(separator, kRDF_type, kNC_BookmarkSeparator, PR_TRUE)))
		{
			rv = aContainer->AppendElement(separator);
			if (NS_FAILED(rv)) return rv;
		}
	}
	return(rv);
}



nsresult
BookmarkParser::ParseHeaderBegin(const nsString& aLine,
								 nsCOMPtr<nsIRDFContainer>& aContainer)
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

    nsAutoString timeStr;
    timeStr.Append(aTime, 10);

    nsresult rv;
    nsIRDFLiteral* literal;
    if (NS_FAILED(rv = gRDF->GetLiteral(timeStr.GetUnicode(), &literal))) {
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

class nsBookmarksService : public nsIBookmarksService,
						   public nsIRDFDataSource,
						   public nsIRDFRemoteDataSource
{
protected:
    nsIRDFDataSource* mInner;

    nsresult ReadBookmarks();
    nsresult WriteBookmarks(nsIRDFDataSource *ds, nsIRDFResource *root);
    nsresult WriteBookmarksContainer(nsIRDFDataSource *ds, nsOutputFileStream strm, nsIRDFResource *container, PRInt32 level);
    nsresult WriteBookmarkProperties(nsIRDFDataSource *ds, nsOutputFileStream strm, nsIRDFResource *node,
    					nsIRDFResource *property, const char *htmlAttrib, PRBool isFirst);
    PRBool CanAccept(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aTarget);

    nsBookmarksService();
    virtual ~nsBookmarksService();
	nsresult Init();

	friend NS_IMETHODIMP
	NS_NewBookmarksService(nsISupports* aOuter, REFNSIID aIID, void** aResult);

public:
    // nsISupports
    NS_DECL_ISUPPORTS

    // nsIRDFBookmarkDataSource
    NS_IMETHOD AddBookmark(const char *aURI, const PRUnichar *aOptionalTitle);
    NS_IMETHOD FindShortcut(const PRUnichar *aUserInput, char **aShortcutURL);

    NS_IMETHOD GetURI(char* *uri);

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

	// nsIRDFRemoteDataSource
	NS_IMETHOD Init(const char* aURI);
	NS_IMETHOD Refresh(PRBool aBlocking);
    NS_IMETHOD Flush();
};


////////////////////////////////////////////////////////////////////////

nsBookmarksService::nsBookmarksService()
{
	NS_INIT_REFCNT();
}

nsBookmarksService::~nsBookmarksService()
{
	Flush();
	NS_RELEASE(mInner);
	bm_ReleaseGlobals();
}


nsresult
nsBookmarksService::Init()
{
	nsresult rv;
	rv = bm_AddRefGlobals();
	if (NS_FAILED(rv)) return rv;

    rv = nsComponentManager::CreateInstance(kRDFInMemoryDataSourceCID,
                                            nsnull,
                                            nsIRDFDataSource::GetIID(),
                                            (void**) &mInner);
    if (NS_FAILED(rv)) return rv;

    rv = ReadBookmarks();
    if (NS_FAILED(rv)) return rv;

    // register this as a named data source with the RDF service
    rv = gRDF->RegisterDataSource(this, PR_FALSE);
    if (NS_FAILED(rv)) return rv;

	return NS_OK;
}


NS_IMETHODIMP
NS_NewBookmarksService(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
	NS_PRECONDITION(aResult != nsnull, "null ptr");
	if (! aResult)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(aOuter == nsnull, "no aggregation");
	if (aOuter)
		return NS_ERROR_NO_AGGREGATION;

	nsresult rv = NS_OK;

	nsBookmarksService* result = new nsBookmarksService();
	if (! result)
		return NS_ERROR_OUT_OF_MEMORY;

	rv = result->Init();
	if (NS_SUCCEEDED(rv))
		rv = result->QueryInterface(aIID, aResult);

	if (NS_FAILED(rv)) {
		delete result;
		*aResult = nsnull;
		return rv;
	}

	return rv;
}


////////////////////////////////////////////////////////////////////////

NS_IMPL_ADDREF(nsBookmarksService);
NS_IMPL_RELEASE(nsBookmarksService);

NS_IMETHODIMP
nsBookmarksService::QueryInterface(REFNSIID aIID, void **aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
	if (! aResult)
		return NS_ERROR_NULL_POINTER;

	if (aIID.Equals(nsIBookmarksService::GetIID()) ||
        aIID.Equals(kISupportsIID))
	{
		*aResult = NS_STATIC_CAST(nsIBookmarksService*, this);
	}
    else if (aIID.Equals(nsIRDFDataSource::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIRDFDataSource*, this);
    }
	else if (aIID.Equals(nsIRDFRemoteDataSource::GetIID())) {
		*aResult = NS_STATIC_CAST(nsIRDFRemoteDataSource*, this);
	}
    else {
        *aResult = nsnull;
        return NS_NOINTERFACE;
    }

    NS_ADDREF(this);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// nsIBookmarksService


NS_IMETHODIMP
nsBookmarksService::AddBookmark(const char *aURI, const PRUnichar *aOptionalTitle)
{
    // XXX For the moment, just add it as a child of
    // BookmarksRoot. Constructing a parser object to do this is so
    // gross. We need to factor AddBookmark() into its own little
    // routine or something.
	BookmarkParser parser;
	parser.Init(nsnull, mInner);

	nsresult rv;

    nsCOMPtr<nsIRDFContainer> container;
	rv = nsComponentManager::CreateInstance(kRDFContainerCID,
											nsnull,
											nsIRDFContainer::GetIID(),
											getter_AddRefs(container));
    if (NS_FAILED(rv)) return rv;

	rv = container->Init(mInner, kNC_BookmarksRoot);
	if (NS_FAILED(rv)) return rv;

	rv = parser.AddBookmark(container, aURI, aOptionalTitle,
							0L, 0L, 0L, nsnull, kNC_Bookmark);

	if (NS_FAILED(rv)) return rv;

	rv = Flush();
	if (NS_FAILED(rv)) return rv;

	return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::FindShortcut(const PRUnichar *aUserInput, char **aShortcutURL)
{
	NS_PRECONDITION(aUserInput != nsnull, "null ptr");
	if (! aUserInput)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(aShortcutURL != nsnull, "null ptr");
	if (! aShortcutURL)
		return NS_ERROR_NULL_POINTER;

	nsresult rv;

	nsCOMPtr<nsIRDFLiteral> literalTarget;
	rv = gRDF->GetLiteral(aUserInput, getter_AddRefs(literalTarget));
	if (NS_FAILED(rv)) return rv;

	if (rv != NS_RDF_NO_VALUE)
	{
		nsCOMPtr<nsIRDFResource> source;
		rv = GetSource(kNC_ShortcutURL, literalTarget,
					   PR_TRUE, getter_AddRefs(source));

		if (NS_FAILED(rv)) return rv;

		if (rv != NS_RDF_NO_VALUE)
		{
			rv = source->GetValue(aShortcutURL);
			if (NS_FAILED(rv)) return rv;

			return NS_OK;
		}
	}

	*aShortcutURL = nsnull;
    return NS_RDF_NO_VALUE;
}



////////////////////////////////////////////////////////////////////////
// nsIRDFDataSource

NS_IMETHODIMP
nsBookmarksService::GetURI(char* *aURI)
{
	*aURI = nsXPIDLCString::Copy("rdf:bookmarks");
	if (! *aURI)
		return NS_ERROR_OUT_OF_MEMORY;

	return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::GetTarget(nsIRDFResource* aSource,
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
            if (NS_FAILED(rv = gRDF->GetLiteral(nsAutoString(uri).GetUnicode(), &literal))) {
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
nsBookmarksService::Assert(nsIRDFResource* aSource,
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
nsBookmarksService::Unassert(nsIRDFResource* aSource,
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
nsBookmarksService::Change(nsIRDFResource* aSource,
						  nsIRDFResource* aProperty,
						  nsIRDFNode* aOldTarget,
						  nsIRDFNode* aNewTarget)
{
	if (CanAccept(aSource, aProperty, aNewTarget)) {
		return mInner->Change(aSource, aProperty, aOldTarget, aNewTarget);
	}
	else {
		return NS_RDF_ASSERTION_REJECTED;
	}
}


NS_IMETHODIMP
nsBookmarksService::Move(nsIRDFResource* aOldSource,
						 nsIRDFResource* aNewSource,
						 nsIRDFResource* aProperty,
						 nsIRDFNode* aTarget)
{
	if (CanAccept(aNewSource, aProperty, aTarget)) {
		return mInner->Move(aOldSource, aNewSource, aProperty, aTarget);
	}
	else {
		return NS_RDF_ASSERTION_REJECTED;
	}
}

NS_IMETHODIMP
nsBookmarksService::GetAllCommands(nsIRDFResource* source,
                                       nsIEnumerator/*<nsIRDFResource>*/** commands)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBookmarksService::GetAllCmds(nsIRDFResource* source,
                                       nsISimpleEnumerator/*<nsIRDFResource>*/** commands)
{
	return(NS_NewEmptyEnumerator(commands));
}

NS_IMETHODIMP
nsBookmarksService::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                         nsIRDFResource*   aCommand,
                                         nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                         PRBool* aResult)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBookmarksService::DoCommand(nsISupportsArray* aSources,
                                  nsIRDFResource*   aCommand,
                                  nsISupportsArray* aArguments)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////
// nsIRDFRemoteDataSource

NS_IMETHODIMP
nsBookmarksService::Init(const char* aURI)
{
	return NS_OK;
}


NS_IMETHODIMP
nsBookmarksService::Refresh(PRBool aBlocking)
{
	// XXX re-sync with the bookmarks file, if necessary.
	return NS_OK;
}


NS_IMETHODIMP
nsBookmarksService::Flush()
{
    return WriteBookmarks(mInner, kNC_BookmarksRoot);
}

////////////////////////////////////////////////////////////////////////
// Implementation methods


nsresult
nsBookmarksService::ReadBookmarks()
{
	nsresult rv;

	rv = gRDFC->MakeSeq(mInner, kNC_BookmarksRoot, nsnull);
	NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to make NC:BookmarksRoot a sequence");
	if (NS_FAILED(rv)) return rv;

	nsSpecialSystemDirectory bookmarksFile(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);

	// XXX we should get this from prefs.
	bookmarksFile += "res";
	bookmarksFile += "samples";
	bookmarksFile += "bookmarks.html";

	PRBool	foundIERoot = PR_FALSE;
#ifdef	XP_WIN
	nsCOMPtr<nsIRDFResource>	ieFolder;
	char				*ieFavoritesURL = nsnull;
#endif
	{ // <-- scope the stream to get the open/close automatically.
		nsInputFileStream strm(bookmarksFile);

		if (! strm.is_open())
		{
			NS_ERROR("unable to open file");
			return NS_ERROR_FAILURE;
		}

		BookmarkParser parser;
		parser.Init(&strm, mInner);

#ifdef	XP_MAC
		parser.SetIEFavoritesRoot(kURINC_IEFavoritesRoot);
#endif

#ifdef	XP_WIN
		nsSpecialSystemDirectory	ieFavoritesFile(nsSpecialSystemDirectory::Win_Favorites);
		nsFileURL			ieFavoritesURLSpec(ieFavoritesFile);
		const char			*favoritesURL = ieFavoritesURLSpec.GetAsString();
		if (favoritesURL)
		{
			ieFavoritesURL = strdup(favoritesURL);
		}
		parser.SetIEFavoritesRoot(ieFavoritesURL);
#endif

		parser.Parse(kNC_BookmarksRoot, kNC_Bookmark);

		parser.ParserFoundIEFavoritesRoot(&foundIERoot);
	} // <-- scope the stream to get the open/close automatically.
	
	// look for and import any IE Favorites
	nsAutoString	ieTitle("Imported IE Favorites");		// XXX localization?

#ifdef	XP_MAC
	nsSpecialSystemDirectory ieFavoritesFile(nsSpecialSystemDirectory::Mac_PreferencesDirectory);
	ieFavoritesFile += "Explorer";
	ieFavoritesFile += "Favorites.html";

	nsInputFileStream	ieStream(ieFavoritesFile);
	if (ieStream.is_open())
	{
		if (NS_SUCCEEDED(rv = gRDFC->MakeSeq(mInner, kNC_IEFavoritesRoot, nsnull)))
		{
			BookmarkParser parser;
			parser.Init(&ieStream, mInner);
			parser.Parse(kNC_IEFavoritesRoot, kNC_IEFavorite);
				
			nsCOMPtr<nsIRDFLiteral>	ieTitleLiteral;
			if (NS_SUCCEEDED(rv = gRDF->GetLiteral(ieTitle.GetUnicode(), getter_AddRefs(ieTitleLiteral))))
			{
				rv = mInner->Assert(kNC_IEFavoritesRoot, kNC_Name, ieTitleLiteral, PR_TRUE);
			}
				
			// if the IE Favorites root isn't somewhere in bookmarks.html, add it
			if (!foundIERoot)
			{
				nsCOMPtr<nsIRDFContainer> bookmarksRoot;
				rv = nsComponentManager::CreateInstance(kRDFContainerCID,
														nsnull,
														nsIRDFContainer::GetIID(),
														getter_AddRefs(bookmarksRoot));
				if (NS_FAILED(rv)) return rv;

				rv = bookmarksRoot->Init(mInner, kNC_BookmarksRoot);
				if (NS_FAILED(rv)) return rv;

				rv = bookmarksRoot->AppendElement(kNC_IEFavoritesRoot);
				if (NS_FAILED(rv)) return rv;
			}
		}
	}
#endif

#ifdef	XP_WIN
	if (NS_SUCCEEDED(rv = gRDF->GetResource(ieFavoritesURL, getter_AddRefs(ieFolder))))
	{
		nsCOMPtr<nsIRDFLiteral>	ieTitleLiteral;
		if (NS_SUCCEEDED(rv = gRDF->GetLiteral(ieTitle.GetUnicode(), getter_AddRefs(ieTitleLiteral))))
		{
			rv = mInner->Assert(ieFolder, kNC_Name, ieTitleLiteral, PR_TRUE);
		}

		// if the IE Favorites root isn't somewhere in bookmarks.html, add it
		if (!foundIERoot)
		{
			nsCOMPtr<nsIRDFContainer> container;
			rv = nsComponentManager::CreateInstance(kRDFContainerCID,
													nsnull,
													nsIRDFContainer::GetIID(),
													getter_AddRefs(container));
			if (NS_FAILED(rv)) return rv;

			rv = container->Init(mInner, kNC_BookmarksRoot);
			if (NS_FAILED(rv)) return rv;

			rv = container->AppendElement(ieFolder);
			if (NS_FAILED(rv)) return rv;
		}
	}
	if (ieFavoritesURL)
	{
		free(ieFavoritesURL);
		ieFavoritesURL = nsnull;
	}
#endif

	return NS_OK;	
}


nsresult
nsBookmarksService::WriteBookmarks(nsIRDFDataSource *ds, nsIRDFResource *root)
{
	nsSpecialSystemDirectory bookmarksFile(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);

	// XXX we should get this from prefs.
	bookmarksFile += "res";
	bookmarksFile += "samples";
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
nsBookmarksService::WriteBookmarksContainer(nsIRDFDataSource *ds, nsOutputFileStream strm, nsIRDFResource *parent, PRInt32 level)
{
	nsresult	rv = NS_OK;

	nsAutoString	indentationString("");
	for (PRInt32 loop=0; loop<level; loop++)	indentationString += "    ";
	char		*indentation = indentationString.ToNewCString();
	if (nsnull == indentation)	return(NS_ERROR_OUT_OF_MEMORY);

	nsCOMPtr<nsIRDFContainer> container;
	rv = nsComponentManager::CreateInstance(kRDFContainerCID,
											nsnull,
											nsIRDFContainer::GetIID(),
											getter_AddRefs(container));
	if (NS_FAILED(rv)) return rv;

	rv = container->Init(ds, parent);
	if (NS_SUCCEEDED(rv))
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
				if (child.get() == kNC_IEFavoritesRoot)
				{
					if (isIERoot == PR_FALSE)
					{
						if (NS_SUCCEEDED(rv = gRDFC->IsContainer(ds, child, &isContainer)))
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

					// output ID
					strm << " " << kIDEquals;
					nsXPIDLCString id;
					rv = child->GetValue(getter_Copies(id));
					if (NS_SUCCEEDED(rv) && (id)) {
						strm << (const char*) id;
					}
					strm << "\"";

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

			strm << indentation;
			strm << "</DL><p>\n";
		}
	}
	delete [] indentation;
	return(rv);
}


nsresult
nsBookmarksService::WriteBookmarkProperties(nsIRDFDataSource *ds, nsOutputFileStream strm,
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
nsBookmarksService::CanAccept(nsIRDFResource* aSource,
							  nsIRDFResource* aProperty,
							  nsIRDFNode* aTarget)
{
    // XXX This is really crippled, and needs to be stricter. We want
    // to exclude any property that isn't talking about a known
    // bookmark.
    nsresult rv;

    PRBool isOrdinal;
    rv = gRDFC->IsOrdinalProperty(aProperty, &isOrdinal);
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

////////////////////////////////////////////////////////////////////////
// Component Exports

extern "C" PR_IMPLEMENT(nsresult)
NSGetFactory(nsISupports* aServiceMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aProgID,
             nsIFactory **aFactory)
{
	NS_PRECONDITION(aFactory != nsnull, "null ptr");
	if (! aFactory)
		return NS_ERROR_NULL_POINTER;

	nsIGenericFactory::ConstructorProcPtr constructor;

	if (aClass.Equals(kBookmarksServiceCID)) {
		constructor = NS_NewBookmarksService;
	}
	else {
		*aFactory = nsnull;
		return NS_NOINTERFACE; // XXX
	}

	nsresult rv;
	NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServiceMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return rv;

	nsCOMPtr<nsIGenericFactory> factory;
	rv = compMgr->CreateInstance(kGenericFactoryCID,
								 nsnull,
								 nsIGenericFactory::GetIID(),
								 getter_AddRefs(factory));

	if (NS_FAILED(rv)) return rv;

	rv = factory->SetConstructor(constructor);
	if (NS_FAILED(rv)) return rv;

	*aFactory = factory;
	NS_ADDREF(*aFactory);
	return NS_OK;
}



extern "C" PR_IMPLEMENT(nsresult)
NSRegisterSelf(nsISupports* aServMgr , const char* aPath)
{
  nsresult rv;

  nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;

  NS_WITH_SERVICE1(nsIComponentManager, compMgr, servMgr, kComponentManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->RegisterComponent(kBookmarksServiceCID, "Bookmarks",
                                  NS_BOOKMARKS_SERVICE_PROGID,
                                  aPath, PR_TRUE, PR_TRUE);

  rv = compMgr->RegisterComponent(kBookmarksServiceCID, "Bookmarks",
                                  NS_BOOKMARKS_DATASOURCE_PROGID,
                                  aPath, PR_TRUE, PR_TRUE);

  return NS_OK;
}



extern "C" PR_IMPLEMENT(nsresult)
NSUnregisterSelf(nsISupports* aServMgr, const char* aPath)
{
  nsresult rv;

  nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;

  NS_WITH_SERVICE1(nsIComponentManager, compMgr, servMgr, kComponentManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->UnregisterComponent(kBookmarksServiceCID, aPath);

  return NS_OK;
}


