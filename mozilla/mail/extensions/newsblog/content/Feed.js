var rdfcontainer =
  Components
    .classes["@mozilla.org/rdf/container-utils;1"]
      .getService(Components.interfaces.nsIRDFContainerUtils);

var rdfparser =
  Components
    .classes["@mozilla.org/rdf/xml-parser;1"]
      .createInstance(Components.interfaces.nsIRDFXMLParser);

// For use when serializing content in Atom feeds.
var serializer = new XMLSerializer;

// Hash of feeds being downloaded, indexed by URL, so the load event listener
// can access the Feed objects after it finishes downloading the feed files.
var gFzFeedCache = new Object();

function Feed(resource) {
    this.resource = resource.QueryInterface(Components.interfaces.nsIRDFResource);

  this.description = null;
  this.author = null;

  this.request = null;

    this.downloadCallback = null;

    this.items = new Array();
  
  return this;
}

// The name of the message folder corresponding to the feed.
// XXX This should be called something more descriptive like "folderName".
// XXX Or maybe, when we support nested folders and downloading into any folder,
// there could just be a reference to the folder itself called "folder".
Feed.prototype.name getter = function() {
  var name = this.title || this.description || this.url;
  if (!name)
    throw("couldn't compute feed name, as feed has no title, description, or URL.");

  // Make sure the feed name doesn't have any line breaks, since we're going
  // to use it as the name of the folder in the filesystem.  This may not
  // be necessary, since Mozilla's mail code seems to handle other forbidden
  // characters in filenames and can probably handle these as well.
  name = name.replace(/[\n\r\t]+/g, " ");

  // Make sure the feed doesn't end in a period to work around bug 117840.
  name = name.replace(/\.+$/, "");

  return name;
}

Feed.prototype.download = function(parseItems, aCallback) {

  // Whether or not to parse items when downloading and parsing the feed.
  // Defaults to true, but setting to false is useful for obtaining
  // just the title of the feed when the user subscribes to it.
  this.parseItems = parseItems == null ? true : parseItems ? true : false;

  this.request = new XMLHttpRequest();
  this.request.onprogress = Feed.onProgress; // must be set before calling .open
  this.request.open("GET", this.url, true);

  this.downloadCallback = aCallback; // may be null

  // var loadgroup = this.request.channel.loadgroup;
  this.request.overrideMimeType("text/xml");
    this.request.onload = Feed.onDownloaded;
    this.request.onerror = Feed.onDownloadError;
    gFzFeedCache[this.url] = this;
  this.request.send(null);
}

Feed.onDownloaded = function(event) {
  var request = event.target;
  var url = request.channel.originalURI.spec;
  debug(url + " downloaded");
  var feed = gFzFeedCache[url];
  if (!feed)
    throw("error after downloading " + url + ": couldn't retrieve feed from request");
  feed.parse();

  if (feed.downloadCallback)
    feed.downloadCallback.downloaded(feed);
}

Feed.onProgress = function(event) {
  var request = event.target;
  var url = request.channel.originalURI.spec;
  var feed = gFzFeedCache[url];

  if (feed.downloadCallback)
    feed.downloadCallback.onProgress(event.position, event.totalSize);
}

Feed.onDownloadError = function(event) {
  // XXX add error message if available and notify the user?
  var request = event.target;
  var url = request.channel.originalURI.spec;
  var feed = gFzFeedCache[url];
  if (feed)
  {
    debug(feed.title + " download failed");
    if (feed.downloadCallback)
      feed.downloaded(nsnull);
  }
  throw("error downloading feed " + url);
}

Feed.prototype.url getter = function() {
    var ds = getSubscriptionsDS();
    var url = ds.GetTarget(this.resource, DC_IDENTIFIER, true);
    if (url)
      url = url.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
    else
      url = this.resource.Value;
    return url;
}

Feed.prototype.title getter = function() {
    var ds = getSubscriptionsDS();
    var title = ds.GetTarget(this.resource, DC_TITLE, true);
    if (title)
      title = title.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
    return title;
}

Feed.prototype.title setter = function(new_title) {
    var ds = getSubscriptionsDS();
    new_title = rdf.GetLiteral(new_title || "");
    var old_title = ds.GetTarget(this.resource, DC_TITLE, true);
    if (old_title)
        ds.Change(this.resource, DC_TITLE, old_title, new_title);
    else
        ds.Assert(this.resource, DC_TITLE, new_title, true);
}

Feed.prototype.quickMode getter = function() {
    var ds = getSubscriptionsDS();
    var quickMode = ds.GetTarget(this.resource, FZ_QUICKMODE, true);
    if (quickMode) {
        quickMode = quickMode.QueryInterface(Components.interfaces.nsIRDFLiteral);
        quickMode = quickMode.Value;
        quickMode = eval(quickMode);
    }
    return quickMode;
}

Feed.prototype.parse = function() {
  // Figures out what description language (RSS, Atom) and version this feed
  // is using and calls a language/version-specific feed parser.

  debug("parsing feed " + this.url);

  if (!this.request.responseText) {
    throw("error parsing feed " + this.url + ": no data");
    return;
  }
  else if (this.request.responseText.search(/="http:\/\/purl\.org\/rss\/1\.0\/"/) != -1) {
    debug(this.url + " is an RSS 1.x (RDF-based) feed");
    this.parseAsRSS1();
  }
  else if (this.request.responseText.search(/="http:\/\/purl.org\/atom\/ns#"/) != -1) {
    debug(this.url + " is an Atom feed");
    this.parseAsAtom();
  }
  else if (this.request.responseText.search(/"http:\/\/my\.netscape\.com\/rdf\/simple\/0\.9\/"/) != -1)
  {
    // RSS 0.9x is forward compatible with RSS 2.0, so use the RSS2 parser to handle it.
    debug(this.url + " is an 0.9x feed");
    this.parseAsRSS2();
  }
  // XXX Explicitly check for RSS 2.0 instead of letting it be handled by the
  // default behavior (who knows, we may change the default at some point).
  else {
    // We don't know what kind of feed this is; let's pretend it's RSS 0.9x
    // and hope things work out for the best.  In theory even RSS 1.0 feeds
    // could be parsed by the 0.9x parser if the RSS namespace was the default.
    debug(this.url + " is of unknown format; assuming an RSS 0.9x feed");
    this.parseAsRSS2();
  }
  var ds = getItemsDS();
  ds = ds.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
  ds.Flush();
}

Feed.prototype.parseAsRSS2 = function() {
  if (!this.request.responseXML || !(this.request.responseXML instanceof XMLDocument))
    throw("error parsing RSS 2.0 feed " + this.url + ": data not parsed into XMLDocument object");

  // Get the first channel (assuming there is only one per RSS File).
  var channel = this.request.responseXML.getElementsByTagName("channel")[0];
  if (!channel)
    throw("error parsing RSS 2.0 feed " + this.url + ": channel element missing");

  this.title = this.title || getNodeValue(channel.getElementsByTagName("title")[0]);
  this.description = getNodeValue(channel.getElementsByTagName("description")[0]);

  if (!this.parseItems)
    return;

  this.invalidateItems();

  var itemNodes = this.request.responseXML.getElementsByTagName("item");
  for ( var i=0 ; i<itemNodes.length ; i++ ) {
    var itemNode = itemNodes[i];
    var item = new FeedItem();
    item.feed = this;

    var link = getNodeValue(itemNode.getElementsByTagName("link")[0]);

    var guidNode = itemNode.getElementsByTagName("guid")[0];
    if (guidNode) {
      var guid = getNodeValue(guidNode);
      var isPermaLink =
        guidNode.getAttribute('isPermaLink') == 'false' ? false : true;
    }

    item.url = (guid && isPermaLink) ? guid : link ? link : null;
    item.id = guid;
    item.description = getNodeValue(itemNode.getElementsByTagName("description")[0]);
    item.title = getNodeValue(itemNode.getElementsByTagName("title")[0])
                 || (item.description ? item.description.substr(0, 150) : null)
                 || item.title;
    item.author = getNodeValue(itemNode.getElementsByTagName("author")[0]
                               || itemNode.getElementsByTagName("creator")[0]
                               || channel.getElementsByTagName("creator")[0])
                  || this.title
                  || item.author;
    item.date = getNodeValue(itemNode.getElementsByTagName("pubDate")[0]
                             || itemNode.getElementsByTagName("date")[0])
                || item.date;

    item.store();
    item.markValid();

  }
  this.removeInvalidItems();
}

Feed.prototype.parseAsRSS1 = function() {
  // RSS 1.0 is valid RDF, so use the RDF parser/service to extract data.

  // Create a new RDF data source and parse the feed into it.
  var ds = Components
             .classes["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"]
               .createInstance(Components.interfaces.nsIRDFDataSource);
  rdfparser.parseString(ds, this.request.channel.URI, this.request.responseText);

  // Get information about the feed as a whole.
  var channel = ds.GetSource(RDF_TYPE, RSS_CHANNEL, true);
  this.title = this.title || getRDFTargetValue(ds, channel, RSS_TITLE);
  this.description = getRDFTargetValue(ds, channel, RSS_DESCRIPTION);

  if (!this.parseItems)
    return;

  this.invalidateItems();

  var items = ds.GetTarget(channel, RSS_ITEMS, true);
  //items = items.QueryInterface(Components.interfaces.nsIRDFContainer);
  items = rdfcontainer.MakeSeq(ds, items);
  items = items.GetElements();
  // If the channel doesn't list any items, look for resources of type "item"
  // (a hacky workaround for some buggy feeds).
  if (!items.hasMoreElements())
    items = ds.GetSources(RDF_TYPE, RSS_ITEM, true);

  while (items.hasMoreElements()) {
    var itemResource = items.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
    var item = new FeedItem();
    item.feed = this;

    // Prefer the value of the link tag to the item URI since the URI could be
    // a relative URN.
    var uri = itemResource.Value;
    var link = getRDFTargetValue(ds, itemResource, RSS_LINK);

    item.url = link || uri;
    item.id = item.url;
    item.description = getRDFTargetValue(ds, itemResource, RSS_DESCRIPTION);
    item.title = getRDFTargetValue(ds, itemResource, RSS_TITLE)
                 || getRDFTargetValue(ds, itemResource, DC_SUBJECT)
                 || (item.description ? item.description.substr(0, 150) : null)
                 || item.title;
    item.author = getRDFTargetValue(ds, itemResource, DC_CREATOR)
                  || getRDFTargetValue(ds, channel, DC_CREATOR)
                  || this.title
                  || item.author;
    item.date = getRDFTargetValue(ds, itemResource, DC_DATE) || item.date;
    item.content = getRDFTargetValue(ds, itemResource, RSS_CONTENT_ENCODED);

    item.store();
    item.markValid();
  }
  this.removeInvalidItems();
}

Feed.prototype.parseAsAtom = function() {
  if (!this.request.responseXML || !(this.request.responseXML instanceof XMLDocument))
    throw("error parsing Atom feed " + this.url + ": data not parsed into XMLDocument object");

  // Get the first channel (assuming there is only one per Atom File).
  var channel = this.request.responseXML.getElementsByTagName("feed")[0];
  if (!channel)
    throw("channel missing from Atom feed " + request.channel.name);

  this.title = this.title || getNodeValue(channel.getElementsByTagName("title")[0]);
  this.description = getNodeValue(channel.getElementsByTagName("tagline")[0]);

  if (!this.parseItems)
    return;

  this.invalidateItems();

  var items = this.request.responseXML.getElementsByTagName("entry");
  for ( var i=0 ; i<items.length ; i++ ) {
    var itemNode = items[i];
    var item = new FeedItem();
    item.feed = this;

    var url;
    var links = itemNode.getElementsByTagName("link");
    for ( var j=0 ; j<links.length ; j++ ) {
      var alink = links[j];
      if (alink && alink.getAttribute('rel') && alink.getAttribute('rel') == 'alternate' && alink.getAttribute('href')) {
        url = alink.getAttribute('href');
        break;
      }
    }

    item.url = url;
    item.id = getNodeValue(itemNode.getElementsByTagName("id")[0]);
    item.description = getNodeValue(itemNode.getElementsByTagName("summary")[0]);
    item.title = getNodeValue(itemNode.getElementsByTagName("title")[0])
                 || (item.description ? item.description.substr(0, 150) : null)
                 || item.title;

    var author = itemNode.getElementsByTagName("author")[0]
                 || itemNode.getElementsByTagName("contributor")[0]
                 || channel.getElementsByTagName("author")[0];
    if (author) {
      var name = getNodeValue(author.getElementsByTagName("name")[0]);
      var email = getNodeValue(author.getElementsByTagName("email")[0]);
      if (name)
        author = name + (email ? " <" + email + ">" : "");
      else if (email)
        author = email;
    }
    item.author = author || item.author || this.title;

    item.date = getNodeValue(itemNode.getElementsByTagName("modified")[0]
                             || itemNode.getElementsByTagName("issued")[0]
                             || itemNode.getElementsByTagName("created")[0])
                || item.date;

    // XXX We should get the xml:base attribute from the content tag as well
    // and use it as the base HREF of the message.
    // XXX Atom feeds can have multiple content elements; we should differentiate
    // between them and pick the best one.
    // Some Atom feeds wrap the content in a CTYPE declaration; others use
    // a namespace to identify the tags as HTML; and a few are buggy and put
    // HTML tags in without declaring their namespace so they look like Atom.
    // We deal with the first two but not the third.
    var content;
    var contentNode = itemNode.getElementsByTagName("content")[0];
    if (contentNode) {
      content = "";
      for ( var j=0 ; j<contentNode.childNodes.length ; j++ ) {
        var node = contentNode.childNodes.item(j);
        if (node.nodeType == node.CDATA_SECTION_NODE)
          content += node.data;
        else
          content += serializer.serializeToString(node);
          //content += getNodeValue(node);
      }
      if (contentNode.getAttribute('mode') == "escaped") {
        content = content.replace(/&lt;/g, "<");
        content = content.replace(/&gt;/g, ">");
        content = content.replace(/&amp;/g, "&");
      }
      if (content == "")
        content = null;
    }
    item.content = content;

    item.store();
    item.markValid();
  }
  this.removeInvalidItems();
}

Feed.prototype.invalidateItems = function invalidateItems() {
    var ds = getItemsDS();
    debug("invalidating items for " + this.url);
    var items = ds.GetSources(FZ_FEED, this.resource, true);
    var item;
    while (items.hasMoreElements()) {
        item = items.getNext();
        item = item.QueryInterface(Components.interfaces.nsIRDFResource);
        debug("invalidating " + item.Value);
        var valid = ds.GetTarget(item, FZ_VALID, true);
        if (valid)
            ds.Unassert(item, FZ_VALID, valid, true);
  }
}

Feed.prototype.removeInvalidItems = function() {
    var ds = getItemsDS();
    debug("removing invalid items for " + this.url);
    var items = ds.GetSources(FZ_FEED, this.resource, true);
    var item;
    while (items.hasMoreElements()) {
        item = items.getNext();
        item = item.QueryInterface(Components.interfaces.nsIRDFResource);
        if (ds.HasAssertion(item, FZ_VALID, RDF_LITERAL_TRUE, true))
            continue;
        debug("removing " + item.Value);
        ds.Unassert(item, FZ_FEED, this.resource, true);
        if (ds.hasArcOut(item, FZ_FEED))
            debug(item.Value + " is from more than one feed; only the reference to this feed removed");
        else
            removeAssertions(ds, item);
    }
}

