/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Places Command Controller.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ben Goodger <beng@google.com>
 *   Myk Melez <myk@mozilla.org>
 *   Asaf Romano <mano@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

function LOG(str) {
  dump("*** " + str + "\n");
}

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

function QI_node(aNode, aIID) {
  var result = null;
  try {
    result = aNode.QueryInterface(aIID);
  }
  catch (e) {
  }
  NS_ASSERT(result, "Node QI Failed");
  return result;
}
function asFolder(aNode)   { return QI_node(aNode, Ci.nsINavHistoryFolderResultNode);   }
function asVisit(aNode)    { return QI_node(aNode, Ci.nsINavHistoryVisitResultNode);    }
function asFullVisit(aNode){ return QI_node(aNode, Ci.nsINavHistoryFullVisitResultNode);}
function asContainer(aNode){ return QI_node(aNode, Ci.nsINavHistoryContainerResultNode);}
function asQuery(aNode)    { return QI_node(aNode, Ci.nsINavHistoryQueryResultNode);    }

var PlacesUtils = {
  /**
   * The Bookmarks Service.
   */
  _bookmarks: null,
  get bookmarks() {
    if (!this._bookmarks) {
      this._bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                        getService(Ci.nsINavBookmarksService);
    }
    return this._bookmarks;
  },

  /**
   * The Nav History Service.
   */
  _history: null,
  get history() {
    if (!this._history) {
      this._history = Cc["@mozilla.org/browser/nav-history-service;1"].
                      getService(Ci.nsINavHistoryService);
    }
    return this._history;
  },

  /**
   * The Live Bookmark Service.
   */
  _livemarks: null,
  get livemarks() {
    if (!this._livemarks) {
      this._livemarks = Cc["@mozilla.org/browser/livemark-service;2"].
                        getService(Ci.nsILivemarkService);
    }
    return this._livemarks;
  },

  /**
   * The Annotations Service.
   */
  _annotations: null,
  get annotations() {
    if (!this._annotations) {
      this._annotations = Cc["@mozilla.org/browser/annotation-service;1"].
                          getService(Ci.nsIAnnotationService);
    }
    return this._annotations;
  },

  /**
   * The Transaction Manager for this window.
   */
  _tm: null,
  get tm() {
    if (!this._tm) {
      this._tm = Cc["@mozilla.org/transactionmanager;1"].
                 createInstance(Ci.nsITransactionManager);
    }
    return this._tm;
  },

  /**
   * Makes a URI from a spec.
   * @param   aSpec
   *          The string spec of the URI
   * @returns A URI object for the spec.
   */
  _uri: function PU__uri(aSpec) {
    NS_ASSERT(aSpec, "empty URL spec");
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    return ios.newURI(aSpec, null, null);
  },

  /**
   * Wraps a string in a nsISupportsString wrapper
   * @param   aString
   *          The string to wrap
   * @returns A nsISupportsString object containing a string.
   */
  _wrapString: function PU__wrapString(aString) {
    var s = Cc["@mozilla.org/supports-string;1"].
            createInstance(Ci.nsISupportsString);
    s.data = aString;
    return s;
  },

  /**
   * String bundle helpers
   */
  __bundle: null,
  get _bundle() {
    if (!this.__bundle) {
      const PLACES_STRING_BUNDLE_URI =
        "chrome://browser/locale/places/places.properties";
      this.__bundle = Cc["@mozilla.org/intl/stringbundle;1"].
                      getService(Ci.nsIStringBundleService).
                      createBundle(PLACES_STRING_BUNDLE_URI);
    }
    return this.__bundle;
  },

  getFormattedString: function PU_getFormattedString(key, params) {
    return this._bundle.formatStringFromName(key, params, params.length);
  },
  
  getString: function PU_getString(key) {
    return this._bundle.GetStringFromName(key);
  },

  /**
   * Determines whether or not a ResultNode is a Bookmark folder or not.
   * @param   aNode
   *          A NavHistoryResultNode
   * @returns true if the node is a Bookmark folder, false otherwise
   */
  nodeIsFolder: function PU_nodeIsFolder(aNode) {
    NS_ASSERT(aNode, "null node");
    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER);
  },

  /**
   * Determines whether or not a ResultNode represents a bookmarked URI.
   * @param   aNode
   *          A NavHistoryResultNode
   * @returns true if the node represents a bookmarked URI, false otherwise
   */
  nodeIsBookmark: function PU_nodeIsBookmark(aNode) {
    NS_ASSERT(aNode, "null node");

    if (!this.nodeIsURI(aNode))
      return false;
    var uri = this._uri(aNode.uri);
    return this.bookmarks.isBookmarked(uri);
  },

  /**
   * Determines whether or not a ResultNode is a Bookmark separator.
   * @param   aNode
   *          A NavHistoryResultNode
   * @returns true if the node is a Bookmark separator, false otherwise
   */
  nodeIsSeparator: function PU_nodeIsSeparator(aNode) {
    NS_ASSERT(aNode, "null node");

    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR);
  },

  /**
   * Determines whether or not a ResultNode is a URL item or not
   * @param   aNode
   *          A NavHistoryResultNode
   * @returns true if the node is a URL item, false otherwise
   */
  nodeIsURI: function PU_nodeIsURI(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    return aNode.type == NHRN.RESULT_TYPE_URI ||
           aNode.type == NHRN.RESULT_TYPE_VISIT ||
           aNode.type == NHRN.RESULT_TYPE_FULL_VISIT;
  },

  /**
   * Determines whether or not a ResultNode is a Query item or not
   * @param   aNode
   *          A NavHistoryResultNode
   * @returns true if the node is a Query item, false otherwise
   */
  nodeIsQuery: function PU_nodeIsQuery(aNode) {
    NS_ASSERT(aNode, "null node");

    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY;
  },

  /**
   * Determines if a node is read only (children cannot be inserted, sometimes
   * they cannot be removed depending on the circumstance)
   * @param   aNode
   *          A NavHistoryResultNode
   * @returns true if the node is readonly, false otherwise
   */
  nodeIsReadOnly: function PU_nodeIsReadOnly(aNode) {
    NS_ASSERT(aNode, "null node");

    if (this.nodeIsFolder(aNode))
      return this.bookmarks.getFolderReadonly(asFolder(aNode).folderId);
    if (this.nodeIsQuery(aNode))
      return asQuery(aNode).childrenReadOnly;
    return false;
  },

  /**
   * Determines whether or not a ResultNode is a host folder or not
   * @param   aNode
   *          A NavHistoryResultNode
   * @returns true if the node is a host item, false otherwise
   */
  nodeIsHost: function PU_nodeIsHost(aNode) {
    NS_ASSERT(aNode, "null node");

    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_HOST;
  },

  /**
   * Determines whether or not a ResultNode is a container item or not
   * @param   aNode
   *          A NavHistoryResultNode
   * @returns true if the node is a container item, false otherwise
   */
  nodeIsContainer: function PU_nodeIsContainer(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    return aNode.type == NHRN.RESULT_TYPE_HOST ||
           aNode.type == NHRN.RESULT_TYPE_QUERY ||
           aNode.type == NHRN.RESULT_TYPE_FOLDER ||
           aNode.type == NHRN.RESULT_TYPE_REMOTE_CONTAINER;
  },

  /**
   * Determines whether or not a ResultNode is a remotecontainer item.
   * ResultNote may be either a remote container result type or a bookmark folder
   * with a nonempty remoteContainerType.  The remote container result node
   * type is for dynamically created remote containers (i.e., for the file
   * browser service where you get your folders in bookmark menus).  Bookmark
   * folders are marked as remote containers when some other component is
   * registered as interested in them and providing some operations, in which
   * case their remoteContainerType indicates which component is thus registered.
   * For exmaple, the livemark service uses this mechanism.
   * @param   aNode
   *          A NavHistoryResultNode
   * @returns true if the node is a container item, false otherwise
   */
  nodeIsRemoteContainer: function PU_nodeIsRemoteContainer(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    if (aNode.type == NHRN.RESULT_TYPE_REMOTE_CONTAINER)
      return true;
    if (this.nodeIsFolder(aNode))
      return asContainer(aNode).remoteContainerType != "";
    return false;
  },

 /**
  * Determines whether a ResultNode is a remote container registered by the
  * livemark service.
  * @param aNode
  *        A NavHistory Result Node
  * @returns true if the node is a livemark container item
  */
  nodeIsLivemarkContainer: function PU_nodeIsLivemarkContainer(aNode) {
    return (this.nodeIsRemoteContainer(aNode) &&
            asContainer(aNode).remoteContainerType ==
               "@mozilla.org/browser/livemark-service;2");
  },

  /**
   * Determines whether or not a node is a readonly folder. 
   * @param   aNode
   *          The node to test.
   * @returns true if the node is a readonly folder.
  */
  folderIsReadonly: function(aNode) {
    NS_ASSERT(aNode, "null node");

    return this.nodeIsFolder(aNode) &&
           this.bookmarks.getFolderReadonly(asFolder(aNode).folderId);
  },

  /**
   * Gets the index of a node within its parent container
   * @param   aNode
   *          The node to look up
   * @returns The index of the node within its parent container, or -1 if the
   *          node was not found or the node specified has no parent.
   */
  getIndexOfNode: function PU_getIndexOfNode(aNode) {
    NS_ASSERT(aNode, "null node");

    var parent = aNode.parent;
    if (!parent || !PlacesUtils.nodeIsContainer(parent))
      return -1;
    var wasOpen = parent.containerOpen;
    parent.containerOpen = true;
    var cc = parent.childCount;
    for (var i = 0; i < cc && asContainer(parent).getChild(i) != aNode; ++i);
    parent.containerOpen = wasOpen;
    return i < cc ? i : -1;
  },

  /**
   * String-wraps a NavHistoryResultNode according to the rules of the specified
   * content type.
   * @param   aNode
   *          The Result node to wrap (serialize)
   * @param   aType
   *          The content type to serialize as
   * @param   [optional] aOverrideURI
   *          Used instead of the node's URI if provided.
   *          This is useful for wrapping a container as TYPE_X_MOZ_URL,
   *          TYPE_HTML or TYPE_UNICODE.
   * @returns A string serialization of the node
   */
  wrapNode: function PU_wrapNode(aNode, aType, aOverrideURI) {
    switch (aType) {
    case TYPE_X_MOZ_PLACE_CONTAINER:
    case TYPE_X_MOZ_PLACE:
    case TYPE_X_MOZ_PLACE_SEPARATOR:
      // Data is encoded like this:
      // bookmarks folder: <folderId>\n<>\n<parentId>\n<indexInParent>
      // uri:              0\n<uri>\n<parentId>\n<indexInParent>
      // separator:        0\n<>\n<parentId>\n<indexInParent>
      var wrapped = "";
      if (this.nodeIsFolder(aNode))
        wrapped += asFolder(aNode).folderId + NEWLINE;
      else
        wrapped += "0" + NEWLINE;

      if (this.nodeIsURI(aNode) || this.nodeIsQuery(aNode))
        wrapped += aNode.uri + NEWLINE;
      else
        wrapped += NEWLINE;

      if (this.nodeIsFolder(aNode.parent))
        wrapped += asFolder(aNode.parent).folderId + NEWLINE;
      else
        wrapped += "0" + NEWLINE;

      wrapped += this.getIndexOfNode(aNode);
      return wrapped;
    case TYPE_X_MOZ_URL:
      return (aOverrideURI || aNode.uri) + NEWLINE + aNode.title;
    case TYPE_HTML:
      return "<A HREF=\"" + (aOverrideURI || aNode.uri) + "\">" +
             aNode.title + "</A>";
    }
    // case TYPE_UNICODE:
    return (aOverrideURI || aNode.uri);
  },

  /**
   * Get a transaction for copying a leaf item from one container to another.
   * @param   aURI
   *          The URI of the item being copied
   * @param   aContainer
   *          The container being copied into
   * @param   aIndex
   *          The index within the container the item is copied to
   * @returns A nsITransaction object that performs the copy.
   */
  _getItemCopyTransaction: function (aURI, aContainer, aIndex) {
    var itemTitle = this.bookmarks.getItemTitle(aURI);
    var createTxn = new PlacesCreateItemTransaction(aURI, aContainer, aIndex);
    var editTxn = new PlacesEditItemTitleTransaction(aURI, itemTitle);
    return new PlacesAggregateTransaction("ItemCopy", [createTxn, editTxn]);
  },

  /**
   * Gets a transaction for copying (recursively nesting to include children)
   * a folder and its contents from one folder to another.
   * @param   aData
   *          Unwrapped dropped folder data
   * @param   aContainer
   *          The container we are copying into
   * @param   aIndex
   *          The index in the destination container to insert the new items
   * @returns A nsITransaction object that will perform the copy.
   */
  _getFolderCopyTransaction:
  function PU__getFolderCopyTransaction(aData, aContainer, aIndex) {
    var self = this;
    function getChildTransactions(folderId) {
      var childTransactions = [];
      var children = self.getFolderContents(folderId, false, false);
      var cc = children.childCount;
      var txn = null;
      for (var i = 0; i < cc; ++i) {
        var node = children.getChild(i);
        if (self.nodeIsFolder(node)) {
          var nodeFolderId = asFolder(node).folderId;
          var title = self.bookmarks.getFolderTitle(nodeFolderId);
          txn = new PlacesCreateFolderTransaction(title, -1, aIndex);
          txn.childTransactions = getChildTransactions(nodeFolderId);
        }
        else if (self.nodeIsURI(node) || self.nodeIsQuery(node)) {
          txn = self._getItemCopyTransaction(self._uri(node.uri), -1,
                                             aIndex);
        }
        else if (self.nodeIsSeparator(node)) {
          txn = new PlacesCreateSeparatorTransaction(-1, aIndex);
        }
        childTransactions.push(txn);
      }
      return childTransactions;
    }

    var title = this.bookmarks.getFolderTitle(aData.folderId);
    var createTxn =
      new PlacesCreateFolderTransaction(title, aContainer, aIndex);
    createTxn.childTransactions =
      getChildTransactions(aData.folderId, createTxn);
    return createTxn;
  },

  /**
   * Unwraps data from the Clipboard or the current Drag Session.
   * @param   blob
   *          A blob (string) of data, in some format we potentially know how
   *          to parse.
   * @param   type
   *          The content type of the blob.
   * @returns An array of objects representing each item contained by the source.
   */
  unwrapNodes: function PU_unwrapNodes(blob, type) {
    // We use \n here because the transferable system converts \r\n to \n
    var parts = blob.split("\n");
    var nodes = [];
    for (var i = 0; i < parts.length; ++i) {
      var data = { };
      switch (type) {
      case TYPE_X_MOZ_PLACE_CONTAINER:
      case TYPE_X_MOZ_PLACE:
      case TYPE_X_MOZ_PLACE_SEPARATOR:
        // Data in these types has 4 parts, so if there are less than 4 parts
        // remaining, the data blob is malformed and we should stop.
        if (i > (parts.length - 4))
          break;
        nodes.push({  folderId: parseInt(parts[i++]),
                      uri: parts[i] ? this._uri(parts[i]) : null,
                      parent: parseInt(parts[++i]),
                      index: parseInt(parts[++i]) });
        break;
      case TYPE_X_MOZ_URL:
        // See above.
        if (i > (parts.length - 2))
          break;
        nodes.push({  uri: this._uri(parts[i++]),
                      title: parts[i] });
        break;
      case TYPE_UNICODE:
        // See above.
        if (i > (parts.length - 1))
          break;
        nodes.push({  uri: this._uri(parts[i]) });
        break;
      default:
        LOG("Cannot unwrap data of type " + type);
        throw Cr.NS_ERROR_INVALID_ARG;
      }
    }
    return nodes;
  },

  /**
   * Constructs a Transaction for the drop or paste of a blob of data into
   * a container.
   * @param   data
   *          The unwrapped data blob of dropped or pasted data.
   * @param   type
   *          The content type of the data
   * @param   container
   *          The container the data was dropped or pasted into
   * @param   index
   *          The index within the container the item was dropped or pasted at
   * @param   copy
   *          The drag action was copy, so don't move folders or links.
   * @returns An object implementing nsITransaction that can perform
   *          the move/insert.
   */
  makeTransaction: function PU_makeTransaction(data, type, container,
                                               index, copy) {
    switch (type) {
    case TYPE_X_MOZ_PLACE_CONTAINER:
    case TYPE_X_MOZ_PLACE:
      if (data.folderId > 0) {
        // Place is a folder.
        if (copy)
          return this._getFolderCopyTransaction(data, container, index);
        return new PlacesMoveFolderTransaction(data.folderId, data.parent,
                                               data.index, container,
                                               index);
      }
      if (copy)
        return this._getItemCopyTransaction(data.uri, container, index);
      return new PlacesMoveItemTransaction(data.uri, data.parent,
                                           data.index, container,
                                           index);
    case TYPE_X_MOZ_PLACE_SEPARATOR:
      if (copy) {
        // There is no data in a separator, so copying it just amounts to
        // inserting a new separator.
        return new PlacesCreateSeparatorTransaction(container, index);
      }
      // Similarly, moving a separator is just removing the old one and
      // then creating a new one.
      var removeTxn =
        new PlacesRemoveSeparatorTransaction(data.parent, data.index);
      var createTxn =
        new PlacesCreateSeparatorTransaction(container, index);
      return new PlacesAggregateTransaction("SeparatorMove", [removeTxn, createTxn]);
    case TYPE_X_MOZ_URL:
      // Creating and Setting the title is a two step process, so create
      // a transaction for each then aggregate them.
      var createTxn =
        new PlacesCreateItemTransaction(data.uri, container, index);
      var editTxn =
        new PlacesEditItemTitleTransaction(data.uri, data.title);
      return new PlacesAggregateTransaction("DropMozURLItem", [createTxn, editTxn]);
    case TYPE_UNICODE:
      // Creating and Setting the title is a two step process, so create
      // a transaction for each then aggregate them.
      var createTxn =
        new PlacesCreateItemTransaction(data.uri, container, index);
      var editTxn =
        new PlacesEditItemTitleTransaction(data.uri, data.uri);
      return new PlacesAggregateTransaction("DropItem", [createTxn, editTxn]);
    }
    return null;
  },

  /**
   * Generates a HistoryResultNode for the contents of a folder.
   * @param   folderId
   *          The folder to open
   * @param   [optional] excludeItems
   *          True to hide all items (individual bookmarks). This is used on
   *          the left places pane so you just get a folder hierarchy.
   * @param   [optional] expandQueries
   *          True to make query items expand as new containers. For managing,
   *          you want this to be false, for menus and such, you want this to
   *          be true.
   * @returns A HistoryContainerResultNode containing the contents of the
   *          folder. This container is guaranteed to be open.
   */
  getFolderContents:
  function PU_getFolderContents(aFolderId, aExcludeItems, aExpandQueries) {
    var query = this.history.getNewQuery();
    query.setFolders([aFolderId], 1);
    var options = this.history.getNewQueryOptions();
    options.setGroupingMode([Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER], 1);
    options.excludeItems = aExcludeItems;
    options.expandQueries = aExpandQueries;

    var result = this.history.executeQuery(query, options);
    result.root.containerOpen = true;
    return asContainer(result.root);
  },

  /**
   * This method changes the URI of a bookmark.  Because of the URI-based
   * identity model, it accomplishes this by replacing instances of the old
   * URI with the new URI in each applicable folder, then copies the
   * metadata from the old URI to the new URI.
   */
  changeBookmarkURI: function PU_changeBookmarkProperties(oldURI, newURI) {
    this.bookmarks.changeBookmarkURI(oldURI, newURI);
  },

  /**
   * Show an "Add Bookmark" dialog for the specified URI.
   *
   * @param uri an nsIURI object for which the "add bookmark" dialog is
   *            to be shown.
   */
  showAddBookmarkUI: function PU_showAddBookmarkUI(uri, title) {
    this._showBookmarkDialog("add", uri, title);
  },

  /**
   * Show an "Add Bookmarks" dialog to allow the adding of a folder full
   * of bookmarks corresponding to the objects in the uriList.  This will
   * be called most often as the result of a "Bookmark All Tabs..." command.
   *
   * @param uriList  List of nsIURI objects representing the locations
   *                 to be bookmarked.
   */
  showAddMultiBookmarkUI: function PU_showAddMultiBookmarkUI(uriList) {
    NS_ASSERT(uriList.length, "showAddMultiBookmarkUI expects a list of nsIURI objects");
    this._showBookmarkDialog("addmulti", uriList);
  },

  /**
   * Opens the bookmark properties panel for a given URI.
   *
   * @param   uri an nsIURI object for which the properties are to be shown
   */
  showBookmarkProperties: function PU_showBookmarkProperties(uri) {
    this._showBookmarkDialog("edit", uri);
  },

  /**
   * Opens the folder properties panel for a given folder ID.
   *
   * @param folderid   an integer representing the ID of the folder to edit
   * @returns none
   */
  showFolderProperties: function PU_showFolderProperties(folderId) {
    NS_ASSERT(typeof(folderId)=="number",
              "showFolderProperties received a non-numerical value for its folderId parameter");
    this._showBookmarkDialog("edit", folderId);
  },

  /**
   * Shows the bookmark dialog corresponding to the specified user action.
   * This is an implementation function, and shouldn't be called directly;
   * rather, use the specific variant above that corresponds to your situation.
   *
   * @param identifier   the URI or folder ID or URI list to show
   *                     properties for
   * @param action "add" or "edit", see _determineVariant in
   *               bookmarkProperties.js
   */
  _showBookmarkDialog: function PU__showBookmarkDialog(action, identifier, title) {
    window.openDialog("chrome://browser/content/places/bookmarkProperties.xul",
                      "", "width=600,height=400,chrome,dependent,modal,resizable",
                      this.tm, action, identifier, title);
  },

  /**
   * Returns the closet ancestor places view for the given DOM node
   * @param aNode
   *        a DOM node
   * @return the closet ancestor places view if exists, null otherwsie.
   */
  getViewForNode: function(aNode) {
    var node = aNode;
    while (node) {
      // XXXmano: Use QueryInterface(nsIPlacesView) once we implement it...
      if (node.getAttribute("type") == "places")
        return node;

      node = node.parentNode;
    }

    return null;
  },

  /**
   * Allows opening of javascript/data URI only if the given node is
   * bookmarked (see bug 224521).
   * @param aURINode
   *        a URI node
   * @return true if it's safe to open the node in the browser, false otherwise.
   * 
   */
  checkURLSecurity: function PU_checkURLSecurity(aURINode) {
    if (!this.nodeIsBookmark(aURINode)) {
      var uri = this._uri(aURINode.uri);
      if (uri.schemeIs("javascript") || uri.schemeIs("data")) {
        const BRANDING_BUNDLE_URI = "chrome://branding/locale/brand.properties";
        var brandShortName = Cc["@mozilla.org/intl/stringbundle;1"].
                             getService(Ci.nsIStringBundleService).
                             createBundle(BRANDING_BUNDLE_URI).
                             GetStringFromName("brandShortName");
        var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                            getService(Ci.nsIPromptService);

        var errorStr = this.getString("load-js-data-url-error");
        promptService.alert(window, brandStr, errorStr);
        return false;
      }
    }
    return true;
  }
};
