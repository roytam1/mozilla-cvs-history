var rdfcontainer =
  Components
    .classes["@mozilla.org/rdf/container-utils;1"]
      .getService(Components.interfaces.nsIRDFContainerUtils);

var rdfparser =
  Components
    .classes["@mozilla.org/rdf/xml-parser;1"]
      .createInstance(Components.interfaces.nsIRDFXMLParser);

// For use when serializing content in Atom feeds.
var serializer =   
  Components
    .classes["@mozilla.org/xmlextras/xmlserializer;1"]
      .createInstance(Components.interfaces.nsIDOMSerializer);

// error codes used to inform the consumer about attempts to download a feed

const kNewsBlogSuccess = 0;
const kNewsBlogInvalidFeed = 1; // usually means there was an error trying to parse the feed...
const kNewsBlogRequestFailure = 2; // generic networking failure when trying to download the feed.


// Hash of feeds being downloaded, indexed by URL, so the load event listener
// can access the Feed objects after it finishes downloading the feed files.
function FeedCache(){
    this.nsURI = Components.classes["@mozilla.org/network/standard-url;1"].
                            createInstance(Components.interfaces.nsIURI);
    return this; 
}

FeedCache.prototype.putFeed =function(feed) {
    this[this.normalizeHost(feed.url)] = feed;
}

FeedCache.prototype.getFeedWithUrl =function(url) {
    return this[this.normalizeHost(url)];
}

FeedCache.prototype.removeFeedWithUrl = function(url) {
    delete this[this.normalizeHost(url)];
}

FeedCache.prototype.normalizeHost = function(url){
    this.nsURI.spec = url;    
    this.nsURI.host = this.nsURI.host.toLowerCase();
    return this.nsURI.spec;
}

var gFzFeedCache = new FeedCache();



function Feed(resource) {
    this.resource = resource.QueryInterface(Components.interfaces.nsIRDFResource);

    this.description = null;
    this.author = null;
  
    this.request = null;
    this.folder = null;
    this.server = null;

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
  this.downloadCallback = aCallback; // may be null 

  // Whether or not to parse items when downloading and parsing the feed.
  // Defaults to true, but setting to false is useful for obtaining
  // just the title of the feed when the user subscribes to it.
  this.parseItems = parseItems == null ? true : parseItems ? true : false;

  // Before we do anything...make sure the url is an http url. This is just a sanity check
  // so we don't try opening mailto urls, imap urls, etc. that the user may have tried to subscribe to 
  // as an rss feed..
  var uri = Components.classes["@mozilla.org/network/standard-url;1"].
                      createInstance(Components.interfaces.nsIURI);
  uri.spec = this.url;
  if (!(uri.schemeIs("http") || uri.schemeIs("https")))
    return this.onParseError(this); // simulate an invalid feed error

  // Before we try to download the feed, make sure we aren't already processing the feed
  // by looking up the url in our feed cache
  if (gFzFeedCache.getFeedWithUrl(this.url))
    return; // don't do anything, the feed is already in use

  this.request = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                 .createInstance(Components.interfaces.nsIXMLHttpRequest);
  this.request.onprogress = Feed.onProgress; // must be set before calling .open
  this.request.open("GET", this.url, true);

  this.request.overrideMimeType("text/xml");
  this.request.onload = Feed.onDownloaded;
  this.request.onerror = Feed.onDownloadError;
  gFzFeedCache.putFeed(this);
  this.request.send(null);
}

Feed.onDownloaded = function(event) {
  var request = event.target;
  var url = request.channel.originalURI.spec;
  debug(url + " downloaded");
  var feed = gFzFeedCache.getFeedWithUrl(url);
  if (!feed)
    throw("error after downloading " + url + ": couldn't retrieve feed from request");
  
  feed.parse();

  // parse will asynchronously call the download callback when it is done
}

Feed.onProgress = function(event) {
  var request = event.target;
  var url = request.channel.originalURI.spec;
  var feed = gFzFeedCache.getFeedWithUrl(url);

  if (feed.downloadCallback)
    feed.downloadCallback.onProgress(feed, event.position, event.totalSize);
}

Feed.onDownloadError = function(event) {
  var request = event.target;
  var url = request.channel.originalURI.spec;
  var feed = gFzFeedCache.getFeedWithUrl(url);
  if (feed.downloadCallback)
    feed.downloadCallback.downloaded(feed, kNewsBlogRequestFailure);

  gFzFeedCache.removeFeedWithUrl(url);
}

Feed.prototype.onParseError = function(feed) {
  if (feed && feed.downloadCallback)
  {
    if (feed.downloadCallback)
      feed.downloadCallback.downloaded(feed, kNewsBlogInvalidFeed);
    gFzFeedCache.removeFeedWithUrl(url);
  }
}

Feed.prototype.url getter = function() {
    var ds = getSubscriptionsDS(this.server);
    var url = ds.GetTarget(this.resource, DC_IDENTIFIER, true);
    if (url)
      url = url.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
    else
      url = this.resource.Value;
    return url;
}

Feed.prototype.title getter = function() {
    var ds = getSubscriptionsDS(this.server);
    var title = ds.GetTarget(this.resource, DC_TITLE, true);
    if (title)
      title = title.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
    return title;
}

Feed.prototype.title setter = function(new_title) {
    var ds = getSubscriptionsDS(this.server);
    new_title = rdf.GetLiteral(new_title || "");
    var old_title = ds.GetTarget(this.resource, DC_TITLE, true);
    if (old_title)
        ds.Change(this.resource, DC_TITLE, old_title, new_title);
    else
        ds.Assert(this.resource, DC_TITLE, new_title, true);
}

Feed.prototype.quickMode getter = function() {
    var ds = getSubscriptionsDS(this.server);
    var quickMode = ds.GetTarget(this.resource, FZ_QUICKMODE, true);
    if (quickMode) {
        quickMode = quickMode.QueryInterface(Components.interfaces.nsIRDFLiteral);
        quickMode = quickMode.Value;
        quickMode = eval(quickMode);
    }    
    return quickMode;
}

Feed.prototype.quickMode setter = function(new_quickMode) {
    var ds = getSubscriptionsDS(this.server);
    new_quickMode = rdf.GetLiteral(new_quickMode || "");
    var old_quickMode = ds.GetTarget(this.resource, FZ_QUICKMODE, true);
    if (old_quickMode)
        ds.Change(this.resource, FZ_QUICKMODE, old_quickMode, new_quickMode);
    else
        ds.Assert(this.resource, FZ_QUICKMODE, new_quickMode, true);
}

Feed.prototype.parse = function() {
  // Figures out what description language (RSS, Atom) and version this feed
  // is using and calls a language/version-specific feed parser.

  debug("parsing feed " + this.url);

  if (!this.request.responseText) {
    return this.onParseError(this);
  }
  else if (this.request.responseText.search(/=(['"])http:\/\/purl\.org\/rss\/1\.0\/\1/) != -1) {
    debug(this.url + " is an RSS 1.x (RDF-based) feed");
    this.parseAsRSS1();
  }
  else if (this.request.responseText.search(/=(['"])http:\/\/purl.org\/atom\/ns#\1/) != -1) {
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
  var ds = getItemsDS(this.server);
  ds = ds.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
  ds.Flush();
}

Feed.prototype.parseAsRSS2 = function() {
  if (!this.request.responseXML || !(this.request.responseXML instanceof Components.interfaces.nsIDOMXMLDocument))
    return this.onParseError(this);

  // Get the first channel (assuming there is only one per RSS File).
  var channel = this.request.responseXML.getElementsByTagName("channel")[0];
  if (!channel)
    return this.onParseError(this);

  this.title = this.title || getNodeValue(channel.getElementsByTagName("title")[0]);
  this.description = getNodeValue(channel.getElementsByTagName("description")[0]);

  if (!this.parseItems)
    return;

  this.invalidateItems();

  var itemNodes = this.request.responseXML.getElementsByTagName("item");

  this.itemsToStore = new Array();
  this.itemsToStoreIndex = 0; 

  var converter = Components
    .classes["@mozilla.org/intl/scriptableunicodeconverter"]
      .createInstance(Components.interfaces.nsIScriptableUnicodeConverter);

  converter.charset = 'UTF-8';

  for ( var i=0 ; i<itemNodes.length ; i++ ) {
    var itemNode = itemNodes[i];
    var item = new FeedItem();
    item.feed = this;

    item.characterSet = "UTF-8";

    var link = getNodeValue(itemNode.getElementsByTagName("link")[0]);

    var guidNode = itemNode.getElementsByTagName("guid")[0];
    if (guidNode) {
      var guid = getNodeValue(guidNode);
      var isPermaLink =
        guidNode.getAttribute('isPermaLink') == 'false' ? false : true;
    }

    // getNodeValue returns unicode strings...
    // we need to do the proper conversion on these before we call into
    // item.Store();

    item.url = link ? link : (guid && isPermaLink) ? guid : null;
    item.id = guid;
    item.description = getNodeValue(itemNode.getElementsByTagName("description")[0]);
    item.title = converter.ConvertFromUnicode(getNodeValue(itemNode.getElementsByTagName("title")[0])
                 || (item.description ? item.description.substr(0, 150) : null)
                 || item.title);
    // do this after we potentially assign item.description into item.title
    // because that potential assignment assumes the value is in unicode still
    item.description = converter.ConvertFromUnicode(item.description);

    item.author = getNodeValue(itemNode.getElementsByTagName("author")[0]
                               || itemNode.getElementsByTagName("creator")[0])
                  || this.title
                  || item.author;
    item.date = getNodeValue(itemNode.getElementsByTagName("pubDate")[0]
                             || itemNode.getElementsByTagName("date")[0])
                || item.date;
    
    // If the date is invalid, users will see the beginning of the epoch
    // unless we reset it here, so they'll see the current time instead.
    // This is typical aggregator behavior.
    if(item.date){
      item.date = trimString(item.date);
      if(!isValidRFC822Date(item.date) ){
        // XXX Use this on the other formats as well
        item.date = dateRescue(item.date);
      }
    }

    var content = getNodeValue(itemNode.getElementsByTagNameNS(RSS_CONTENT_NS, "encoded")[0]);
    if (content)
      item.content = converter.ConvertFromUnicode(content);

    this.itemsToStore[i] = item;
  }

  this.storeNextItem();
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
  if (items)
    items = rdfcontainer.MakeSeq(ds, items).GetElements();
  
  // If the channel doesn't list any items, look for resources of type "item"
  // (a hacky workaround for some buggy feeds).
  if (!items || !items.hasMoreElements())
    items = ds.GetSources(RDF_TYPE, RSS_ITEM, true);

  this.itemsToStore = new Array();
  this.itemsToStoreIndex = 0; 
  var index = 0; 

  var converter = Components
    .classes["@mozilla.org/intl/scriptableunicodeconverter"]
      .createInstance(Components.interfaces.nsIScriptableUnicodeConverter);

  converter.charset = "UTF-8";

  while (items.hasMoreElements()) {
    var itemResource = items.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
    var item = new FeedItem();
    item.feed = this;

    item.characterSet = "UTF-8";

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

    this.itemsToStore[index++] = item;
  }

  this.storeNextItem();
}

Feed.prototype.parseAsAtom = function() {
  if (!this.request.responseXML || !(this.request.responseXML instanceof Components.interfaces.nsIDOMXMLDocument))
    return this.onParseError(this);

  // Get the first channel (assuming there is only one per Atom File).
  var channel = this.request.responseXML.getElementsByTagName("feed")[0];
  if (!channel)
    return this.onParseError(this);

  this.title = this.title || getNodeValue(channel.getElementsByTagName("title")[0]);
  this.description = getNodeValue(channel.getElementsByTagName("tagline")[0]);

  if (!this.parseItems)
    return;

  this.invalidateItems();

  var items = this.request.responseXML.getElementsByTagName("entry");

  this.itemsToStore = new Array();
  this.itemsToStoreIndex = 0; 

  for ( var i=0 ; i<items.length ; i++ ) {
    var itemNode = items[i];
    var item = new FeedItem();
    item.feed = this;

    item.characterSet = "UTF-8";

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

    var authorEl = itemNode.getElementsByTagName("author")[0]
                 || itemNode.getElementsByTagName("contributor")[0]
                 || channel.getElementsByTagName("author")[0];
    var author = "";

    if (authorEl) {
      var name = getNodeValue(authorEl.getElementsByTagName("name")[0]);
      var email = getNodeValue(authorEl.getElementsByTagName("email")[0]);
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

    this.itemsToStore[i] = item;
  }
  
  this.storeNextItem();
}

Feed.prototype.invalidateItems = function invalidateItems() {
    var ds = getItemsDS(this.server);
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
    var ds = getItemsDS(this.server);
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

// gets the next item from gItemsToStore and forces that item to be stored
// to the folder. If more items are left to be stored, fires a timer for the next one.
// otherwise it triggers a download done notification to the UI
Feed.prototype.storeNextItem = function()
{
  if (!this.itemsToStore.length)
  {
    // create a folder for the feed if one does not exist already...
    var folder;

    try {
        folder = this.server.rootMsgFolder.getChildNamed(this.name);
     } catch(e) {}

    if (!folder) 
      this.server.rootMsgFolder.createSubfolder(this.name, null /* supposed to be a msg window */);

    return this.cleanupParsingState(this);
  }

  var item = this.itemsToStore[this.itemsToStoreIndex]; 

  item.store();
  item.markValid();

  // if the listener is tracking progress for storing each item, report it here...
  if (item.feed.downloadCallback && item.feed.downloadCallback.onFeedItemStored)
    item.feed.downloadCallback.onFeedItemStored(item.feed, this.itemsToStoreIndex, this.itemsToStore.length);
 
  this.itemsToStoreIndex++

  // eventually we'll report individual progress here....

  if (this.itemsToStoreIndex < this.itemsToStore.length)
  {
    if (!this.storeItemsTimer)
      this.storeItemsTimer = Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer);
    this.storeItemsTimer.initWithCallback(this, 50, Components.interfaces.nsITimer.TYPE_ONE_SHOT);
  }
  else
    this.cleanupParsingState(item.feed);   
}

Feed.prototype.cleanupParsingState = function(feed) {
    // now that we are done parsing the feed, remove the feed from our feed cache
  gFzFeedCache.removeFeedWithUrl(feed.url);

  feed.removeInvalidItems();

    // let's be sure to flush any feed item changes back to disk
  var ds = getItemsDS(feed.server);
    ds.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource).Flush(); // flush any changes

  if (feed.downloadCallback)
    feed.downloadCallback.downloaded(feed, kNewsBlogSuccess);

  feed.request = null; // force the xml http request to go away. This helps reduce some
                              // nasty assertions on shut down of all things.

    this.itemsToStore = "";
    this.itemsToStoreIndex = 0;
    this.storeItemsTimer = null;
  }   

Feed.prototype.notify = function(aTimer) {
  this.storeNextItem();
}

Feed.prototype.QueryInterface = function(aIID) {
  if (aIID.equals(Components.interfaces.nsITimerCallback) || aIID.equals(Components.interfaces.nsISupports))
    return this;

  Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
  return null;    
}

