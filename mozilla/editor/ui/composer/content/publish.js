/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001-2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

var gPublishHandler = null;

function EditorPublish(destinationDirectoryLocation, fileName, login, password)
{
  post_current_editor_contents_to_server(destinationDirectoryLocation, fileName, login, password);
//  PublishCopyFile(srcDirectoryLocation, destinationDirectoryLocation, fileName);
}

var gPublishIOService;
function GetIOService()
{
  if (gPublishIOService)
    return gPublishIOService;

  gPublishIOService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
  if (!gPublishIOService)
    dump("failed to get io service\n");
  
  return gPublishIOService;
}

// this function takes a login and password and adds them to the destination url
function get_destination_channel(destinationDirectoryLocation, fileName, login, password)
{
  try
  {
    var ioService = GetIOService();
    if (!ioService)
      return;

    // create a channel for the destination location
    var fullurl = destinationDirectoryLocation + fileName;
    dump("location is "+fullurl+"\n");
    destChannel = create_channel_from_url(ioService, fullurl, login, password);
    if (!destChannel)
    {
      dump("can't create dest channel\n");
      return;
    }
    try {
    dump("about to set callbacks\n");
    destChannel.notificationCallbacks = window.docshell;  // docshell
    dump("notification callbacks set\n");
    }
    catch(e) {dump(e+"\n");}

    var ftpChannel = destChannel.QueryInterface(Components.interfaces.nsIFTPChannel);
    if (ftpChannel) dump("ftp channel found\n");
    if (ftpChannel)
      return ftpChannel;
    var httpChannel = destChannel.QueryInterface(Components.interfaces.nsIHTTPChannel);
    if (httpChannel) dump("http channel found\n");
    if (httpChannel)
      return httpChannel;
    var httpsChannel = destChannel.QueryInterface(Components.interfaces.nsIHTTPSChannel);
    if (httpsChannel) dump("https channel found\n");
    if (httpsChannel)
      return httpsChannel;
    else
      return null;
  }
  catch (e)
  {
    return null;
  }
}

function output_current_editor_to_channel(aChannel)
{
  var formatType = 'text/' + editorShell.editorType;
  var flags = 256; // nsIDocumentEncoder::OutputEncodeEntities
  var charset = editorShell.GetDocumentCharacterSet();
  editorShell.editor.outputToStream(aChannel, formatType, charset, flags); 
 //   protocolChannel.uploadStream = contentsStream;
}

function post_current_editor_contents_to_server(newLocation, fileName, login, password)
{
  try
  {
    var protocolChannel = get_destination_channel(newLocation, fileName, login, password);
    if (!protocolChannel)
    {
      dump("failed to get a destination channel\n");
      return;
    }

    output_current_editor_to_channel(protocolChannel);
    protocolChannel.asyncOpen(gPublishingListener, null);
    dump("done\n");
  }
  catch (e)
  {
    dump("an error occurred: " + e + "\n");
  }
}

// this function takes a full url, creates an nsIURI, and then creates a channel from that nsIURI
function create_channel_from_url(ioService, aURL, aLogin, aPassword)
{
  try
  {
    var nsiuri = Components.classes["@mozilla.org/network/standard-url;1"].createInstance(Components.interfaces.nsIURI);
    if (!nsiuri)
      return null;
    nsiuri.spec = aURL;
    if (aLogin)
    {
      nsiuri.username = aLogin;
      if (aPassword)
        nsiuri.password = aPassword;
    }
    return ioService.newChannelFromURI(nsiuri);
  }
  catch (e) 
  {
    dump(e+"\n");
    return null;
  }
}

function PublishCopyFile(srcDirectoryLocation, destinationDirectoryLocation, fileName, aLogin, aPassword)
{
  // append '/' if it's not there; inputs should be directories so should end in '/'
  if ( srcDirectoryLocation.charAt(srcDirectoryLocation.length-1) != '/' )
    srcDirectoryLocation = srcDirectoryLocation + '/';
  if ( destinationDirectoryLocation.charAt(destinationDirectoryLocation.length-1) != '/' )
    destinationDirectoryLocation = destinationDirectoryLocation + '/';

  try
  {
    // grab an io service
    var ioService = GetIOService();
    if (!ioService)
      return;

    // create a channel for the source location
    srcChannel = create_channel_from_url(ioService, srcDirectoryLocation + fileName, null, null);
    if (!srcChannel)
    {
      dump("can't create src channel\n");
      return;
    }

    // create a channel for the destination location
    var ftpChannel = get_destination_channel(destinationDirectoryLocation, fileName, aLogin, aPassword);
    if (!ftpChannel)
    {
      dump("failed to get ftp channel\n");
      return;
    }
    ftpChannel.uploadStream = srcChannel.open();
    ftpChannel.asyncOpen(null, null);
    dump("done\n");
  }
  catch (e)
  {
    dump("an error occurred: " + e + "\n");
  }
}

var gPublishingListener =
{
  QueryInterface: function(aIId, instance)
  {
    if (aIId.equals(Components.interfaces.nsIStreamListener))
      return this;
    if (aIId.equals(Components.interfaces.nsISupports))
      return this;
    
    dump("QueryInterface " + aIId + "\n");
    throw Components.results.NS_NOINTERFACE;
  },

  onStartRequest: function(request, ctxt)
  {
    dump("onStartRequest status = " + request.status + "\n");
  },

  onStopRequest: function(request, ctxt, status, errorMsg)
  {
    dump("onStopRequest status = " + request.status + " " + errorMsg + "\n");
  },

  onDataAvailable: function(request, ctxt, inStream, sourceOffset, count)
  {
    dump("onDataAvailable status = " + request.status + " " + count + "\n");
  }
}

