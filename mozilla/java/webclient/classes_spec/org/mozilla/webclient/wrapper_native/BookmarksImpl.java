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
 *      Jason Mawdsley <jason@macadamian.com>
 *      Louis-Philippe Gagnon <louisphilippe@macadamian.com>
 */

package org.mozilla.webclient.wrapper_native;

import org.mozilla.util.Assert;
import org.mozilla.util.Log;
import org.mozilla.util.ParameterCheck;

import org.mozilla.webclient.BrowserControl;
import org.mozilla.webclient.Bookmarks;
import org.mozilla.webclient.BookmarkEntry;
import org.mozilla.webclient.WrapperFactory;

import javax.swing.tree.TreeModel;
import javax.swing.tree.TreeNode;
import javax.swing.tree.DefaultTreeModel;

import org.mozilla.webclient.UnimplementedException; 

public class BookmarksImpl extends ImplObjectNative implements Bookmarks
{
//
// Constants
//
private static final String NC_NS = "http://home.netscape.com/NC-rdf#";

//
// Class Variables
//

//
// Instance Variables
//

// Attribute Instance Variables

// Relationship Instance Variables

/**

 * The bookmarks

 */

private TreeModel bookmarksTree = null;


//
// Constructors and Initializers    
//

public BookmarksImpl(WrapperFactory yourFactory, 
                     BrowserControl yourBrowserControl)
{
    super(yourFactory, yourBrowserControl);
}

/**

 * Since this class is a singleton, we don't expect this method to be
 * called until the app is done with bookmarks for a considerable amount
 * of time.

// PENDING(): Write test case to see that a cycle of Bookmarks
// allocation/destruction/new instance allocation works correctly.
 
 */

public void delete()
{
    // we rely on the individual BookmarkEntryImpl instances' finalize
    // methods to be called to correctly de-allocate the native RDF
    // references.

    bookmarksTree = null;
}

//
// Class methods
//

//
// General Methods
//

//
// Methods from Bookmarks    
//

public void addBookmark(BookmarkEntry mayBeNullParent, 
                        BookmarkEntry bookmark)
{
    ParameterCheck.nonNull(bookmark);
    myFactory.throwExceptionIfNotInitialized();
    
    if  (!(bookmark instanceof BookmarkEntryImpl)) {
        throw new IllegalArgumentException("Can't add bookmark unless BookmarkEntry obtained from Bookmarks.newBookmarkEntry()");
    }

    BookmarkEntry parent = null;
    int lastChildIndex;
    getBookmarks();

    // if parent is null, insert as child of the root
    if (null == mayBeNullParent) {
        parent = (BookmarkEntry) bookmarksTree.getRoot();
    }
    else {
        if  (!(mayBeNullParent instanceof BookmarkEntryImpl)) {
            throw new IllegalArgumentException("Can't add bookmark unless BookmarkEntry obtained from Bookmarks.newBookmarkEntry()");
        }
        parent = mayBeNullParent;
    }
    if (!parent.isFolder()) {
        throw new IllegalArgumentException("Can't add bookmark unless parent BookmarkEntry is a folder");
    }
    lastChildIndex = parent.getChildCount();
    parent.insert(bookmark, lastChildIndex);
}
            
public TreeModel getBookmarks() throws IllegalStateException
{
    myFactory.throwExceptionIfNotInitialized();
    Assert.assert_it(-1 != nativeWebShell);

    if (null == bookmarksTree) {
        int nativeBookmarks;
        TreeNode root;
        if (-1 == (nativeBookmarks = nativeGetBookmarks(nativeWebShell))) {
            throw new IllegalStateException("BookmarksImpl.getBookmarks(): Can't get bookmarks from native browser.");
        }
        // if we can't create a root, or we can't create a tree
        if ((null == (root = new BookmarkEntryImpl(nativeWebShell, 
						   nativeBookmarks, null))) || 
            (null == (bookmarksTree = new DefaultTreeModel(root)))) {
            throw new IllegalStateException("BookmarksImpl.getBookmarks(): Can't create RDFTreeModel.");
        }
    }

    return bookmarksTree;
}
            
public void removeBookmark(BookmarkEntry bookmark)
{
    ParameterCheck.nonNull(bookmark);
    myFactory.throwExceptionIfNotInitialized();
    Assert.assert_it(-1 != nativeWebShell);
    
    throw new UnimplementedException("\nUnimplementedException -----\n API Function CurrentPage::getPageInfo has not yet been implemented.\n");
}

public BookmarkEntry newBookmarkEntry(String url)
{
    BookmarkEntry result = null;
    getBookmarks();
    int newNode;

    if (-1 != (newNode = nativeNewRDFNode(nativeWebShell, url, false))) {
        result = new BookmarkEntryImpl(nativeWebShell,
                                       newNode, null);
        // use put instead of setProperty for jdk1.1.x compatibility.
        result.getProperties().put(BookmarkEntry.NAME, url);
        result.getProperties().put(BookmarkEntry.URL, url);
    }
    
    return result;
}

/**

 * Due to the vagaries of the mozilla RDF implementation, folders and
 * bookmark entries are handled differently.  For Folders, we don't
 * create a nativeRDFNode at the outset.  Rather, we just create the
 * properties table and stock it with the known keys, then wait for the
 * nativeRDFNode to be created en addBookmark.

 */

public BookmarkEntry newBookmarkFolder(String name)
{
    ParameterCheck.nonNull(name);
    BookmarkEntry result = null;
    getBookmarks();
    
    if (null == (result = new BookmarkEntryImpl(nativeWebShell, -1, null))) {
        throw new NullPointerException("Can't create bookmark folder for: " + 
                                       name);
    }
    result.getProperties().put(BookmarkEntry.NAME, name);
    result.getProperties().put(BookmarkEntry.URL, NC_NS + "$" + name);
    result.getProperties().put(BookmarkEntry.IS_FOLDER, name);
    
    return result;
}

//
// native methods
//

private native int nativeGetBookmarks(int webShellPtr);

/**

 * the returned node has already been XPCOM AddRef'd

 */

private native int nativeNewRDFNode(int webShellPtr, String url, 
                                    boolean isFolder);

// ----VERTIGO_TEST_START

//
// Test methods
//

public static void main(String [] args)
{
    Assert.setEnabled(true);

    Log.setApplicationName("BookmarksImpl");
    Log.setApplicationVersion("0.0");
    Log.setApplicationVersionDate("$Id$");

    try {
        org.mozilla.webclient.BrowserControlFactory.setAppData(args[0]);
	org.mozilla.webclient.BrowserControl control = 
	    org.mozilla.webclient.BrowserControlFactory.newBrowserControl();
        Assert.assert_it(control != null);
	
	Bookmarks wc = (Bookmarks)
	    control.queryInterface(org.mozilla.webclient.BrowserControl.WINDOW_CONTROL_NAME);
	Assert.assert_it(wc != null);
    }
    catch (Exception e) {
	System.out.println("got exception: " + e.getMessage());
    }
}

// ----VERTIGO_TEST_END

} // end of class BookmarksImpl
