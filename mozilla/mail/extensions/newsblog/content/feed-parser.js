# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the RSS Parsing Engine
#
# The Initial Developer of the Original Code is
# The Mozilla Foundation.
# Portions created by the Initial Developer are Copyright (C) 2004
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK ***** */

// The feed parser depends on FeedItems.js, Feed.js.

var rdfcontainer =  Components.classes["@mozilla.org/rdf/container-utils;1"].getService(Components.interfaces.nsIRDFContainerUtils);
var rdfparser = Components.classes["@mozilla.org/rdf/xml-parser;1"].createInstance(Components.interfaces.nsIRDFXMLParser);
var serializer = Components.classes["@mozilla.org/xmlextras/xmlserializer;1"].createInstance(Components.interfaces.nsIDOMSerializer);

function FeedParser() 
{}

FeedParser.prototype = 
{
  // parseFeed returns an array of parsed items ready for processing
  // it is currently a synchronous operation. If there was an error parsing the feed, 
  // parseFeed returns an empty feed in addition to calling aFeed.onParseError
  parseFeed: function (aFeed, aSource, aDOM, aBaseURI)
  {
    if (!aSource || !(aDOM instanceof Components.interfaces.nsIDOMXMLDocument))
    {
      aFeed.onParseError(aFeed);   
      return new Array();
    }
    else if (aSource.search(/=(['"])http:\/\/purl\.org\/rss\/1\.0\/\1/) != -1) 
    {
      debug(aFeed.url + " is an RSS 1.x (RDF-based) feed");
      return this.parseAsRSS1(aFeed, aSource, aBaseURI);
    } 
    else if (aSource.search(/=(['"])http:\/\/purl.org\/atom\/ns#\1/) != -1) 
    {
      debug(aFeed.url + " is an Atom feed");
      return this.parseAsAtom(aFeed, aDOM);
    }
    else if (aSource.search(/"http:\/\/my\.netscape\.com\/rdf\/simple\/0\.9\/"/) != -1)
    {
      // RSS 0.9x is forward compatible with RSS 2.0, so use the RSS2 parser to handle it.
      debug(aFeed.url + " is an 0.9x feed");
      return this.parseAsRSS2(aFeed, aDOM);
    }
    // XXX Explicitly check for RSS 2.0 instead of letting it be handled by the
    // default behavior (who knows, we may change the default at some point).
    else 
    {
      // We don't know what kind of feed this is; let's pretend it's RSS 0.9x
      // and hope things work out for the best.  In theory even RSS 1.0 feeds
      // could be parsed by the 0.9x parser if the RSS namespace was the default.
      debug(aFeed.url + " is of unknown format; assuming an RSS 0.9x feed");
      return this.parseAsRSS2(aFeed, aDOM);
    }
  },

  parseAsRSS2: function (aFeed, aDOM) 
  {
    // Get the first channel (assuming there is only one per RSS File).
    var parsedItems = new Array();

    var channel = aDOM.getElementsByTagName("channel")[0];
    if (!channel)
      return aFeed.onParseError(aFeed);

    aFeed.title = aFeed.title || getNodeValue(channel.getElementsByTagName("title")[0]);
    aFeed.description = getNodeValue(channel.getElementsByTagName("description")[0]);

    if (!aFeed.parseItems)
      return parsedItems;

    aFeed.invalidateItems();
    var itemNodes = aDOM.getElementsByTagName("item");   
    var converter = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
    converter.charset = 'UTF-8';

    for (var i=0; i<itemNodes.length; i++) 
    {
      var itemNode = itemNodes[i];
      var item = new FeedItem();
      item.feed = aFeed;
      item.characterSet = "UTF-8";

      var link = getNodeValue(itemNode.getElementsByTagName("link")[0]);
      var guidNode = itemNode.getElementsByTagName("guid")[0];
      var guid;
      var isPermaLink;
      if (guidNode) 
      {
        guid = getNodeValue(guidNode);
        isPermaLink = guidNode.getAttribute('isPermaLink') == 'false' ? false : true;
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
                                 || aFeed.title
                                 || item.author;
      item.date = getNodeValue(itemNode.getElementsByTagName("pubDate")[0]
                               || itemNode.getElementsByTagName("date")[0])
                               || item.date;
    
      // If the date is invalid, users will see the beginning of the epoch
      // unless we reset it here, so they'll see the current time instead.
      // This is typical aggregator behavior.
      if(item.date)
      {
        item.date = trimString(item.date);
        if(!isValidRFC822Date(item.date))
        {
          // XXX Use this on the other formats as well
          item.date = dateRescue(item.date);
        }
      }

      var content = getNodeValue(itemNode.getElementsByTagNameNS(RSS_CONTENT_NS, "encoded")[0]);
      if (content)
        item.content = converter.ConvertFromUnicode(content);

      parsedItems[i] = item;
    }

    return parsedItems;
  },

  parseAsRSS1 : function(aFeed, aSource, aBaseURI) 
  {
    var parsedItems = new Array();

    // RSS 1.0 is valid RDF, so use the RDF parser/service to extract data.
    // Create a new RDF data source and parse the feed into it.
    var ds = Components.classes["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"]
             .createInstance(Components.interfaces.nsIRDFDataSource);

    rdfparser.parseString(ds, aBaseURI, aSource);
    
    // Get information about the feed as a whole.
    var channel = ds.GetSource(RDF_TYPE, RSS_CHANNEL, true);

    aFeed.title = aFeed.title || getRDFTargetValue(ds, channel, RSS_TITLE);
    aFeed.description = getRDFTargetValue(ds, channel, RSS_DESCRIPTION);

    if (!aFeed.parseItems)
      return parsedItems;

    aFeed.invalidateItems();

    var items = ds.GetTarget(channel, RSS_ITEMS, true);
    if (items)
      items = rdfcontainer.MakeSeq(ds, items).GetElements();
  
    // If the channel doesn't list any items, look for resources of type "item"
    // (a hacky workaround for some buggy feeds).
    if (!items || !items.hasMoreElements())
      items = ds.GetSources(RDF_TYPE, RSS_ITEM, true);

    var index = 0; 

    var converter = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"]
                   .createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    while (items.hasMoreElements()) 
    {
      var itemResource = items.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      var item = new FeedItem();
      item.feed = aFeed;
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
                                      || aFeed.title
                                      || item.author;
      
      item.date = getRDFTargetValue(ds, itemResource, DC_DATE) || item.date;
      item.content = getRDFTargetValue(ds, itemResource, RSS_CONTENT_ENCODED);

      parsedItems[index++] = item;
    }
  
    return parsedItems;
  },

  parseAsAtom: function(aFeed, aDOM) 
  {
    var parsedItems = new Array();

    // Get the first channel (assuming there is only one per Atom File).
    var channel = aDOM.getElementsByTagName("feed")[0];
    if (!channel)
    {
      aFeed.onParseError(aFeed);
      return parsedItems;
    }

    aFeed.title = aFeed.title || getNodeValue(channel.getElementsByTagName("title")[0]);
    aFeed.description = getNodeValue(channel.getElementsByTagName("tagline")[0]);

    if (!aFeed.parseItems)
      return parsedItems;

    aFeed.invalidateItems();
    var items = aDOM.getElementsByTagName("entry");
    debug("Items to parse: " + items.length);
  
    for (var i=0; i<items.length; i++) 
    {
      var itemNode = items[i];
      var item = new FeedItem();
      item.feed = aFeed;
      item.characterSet = "UTF-8";

      var url;
      var links = itemNode.getElementsByTagName("link");
      for (var j=0; j < links.length; j++) 
      {
        var alink = links[j];
        if (alink && alink.getAttribute('rel') && alink.getAttribute('rel') == 'alternate' && alink.getAttribute('href')) 
        {
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

      if (authorEl) 
      {
        var name = getNodeValue(authorEl.getElementsByTagName("name")[0]);
        var email = getNodeValue(authorEl.getElementsByTagName("email")[0]);
        if (name)
          author = name + (email ? " <" + email + ">" : "");
        else if (email)
          author = email;
      }
      
      item.author = author || item.author || aFeed.title;

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
      if (contentNode) 
      {
        content = "";
        for (var j=0; j < contentNode.childNodes.length; j++) 
        {
          var node = contentNode.childNodes.item(j);
          if (node.nodeType == node.CDATA_SECTION_NODE)
            content += node.data;
          else
            content += serializer.serializeToString(node);
        }
      
        if (contentNode.getAttribute('mode') == "escaped") 
        {
          content = content.replace(/&lt;/g, "<");
          content = content.replace(/&gt;/g, ">");
          content = content.replace(/&amp;/g, "&");
        }
      
        if (content == "")
          content = null;
      }
      
      item.content = content;
      parsedItems[i] = item;
    }
    return parsedItems;
  }
};
