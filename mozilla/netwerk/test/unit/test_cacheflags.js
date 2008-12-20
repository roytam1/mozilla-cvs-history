do_import_script("netwerk/test/httpserver/httpd.js");

var httpserver = null;

// Need to randomize, because apparently no one clears our cache
var suffix = Math.random();
var shortexpPath = "/shortexp" + suffix;
var longexpPath = "/longexp" + suffix;
var nocachePath = "/nocache" + suffix;
var nostorePath = "/nostore" + suffix;

function make_channel(url, flags) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
    getService(Ci.nsIIOService);
  var req = ios.newChannel(url, null, null);
  req.loadFlags = flags;
  return req;
}

function Test(path, flags, expectSuccess, readFromCache, hitServer) {
  this.path = path;
  this.flags = flags;
  this.expectSuccess = expectSuccess;
  this.readFromCache = readFromCache;
  this.hitServer = hitServer;
}

Test.prototype = {
  flags: 0,
  expectSuccess: true,
  readFromCache: false,
  hitServer: true,
  _buffer: "",
  _isFromCache: false,

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIStreamListener) ||
        iid.equals(Components.interfaces.nsIRequestObserver) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onStartRequest: function(request, context) {
    var cachingChannel = request.QueryInterface(Ci.nsICachingChannel);
    this._isFromCache = request.isPending() && cachingChannel.isFromCache();
  },

  onDataAvailable: function(request, context, stream, offset, count) {
    this._buffer = this._buffer.concat(read_stream(stream, count));
  },

  onStopRequest: function(request, context, status) {
    dump("status: " + status + ", from cache: " + this._isFromCache + ", hit server: " + gHitServer + "\n");
    do_check_eq(Components.isSuccessCode(status), this.expectSuccess);
    do_check_eq(this._isFromCache, this.readFromCache);
    do_check_eq(gHitServer, this.hitServer);

    do_timeout(0, "run_next_test();");
  },

  run: function() {
    dump("Running:" +
         "\n  " + this.path +
         "\n  " + this.flags +
         "\n  " + this.expectSuccess +
         "\n  " + this.readFromCache +
         "\n  " + this.hitServer + "\n");
    gHitServer = false;
    var channel = make_channel("http://localhost:4444" + this.path, this.flags);
    channel.asyncOpen(this, null);
  }
};

var gHitServer = false;

var gTests = [
  new Test(shortexpPath, 0,
           true,   // expect success
           false,  // read from cache
           true),  // hit server
  new Test(shortexpPath, 0,
           true,   // expect success
           true,   // read from cache
           true),  // hit server
  new Test(shortexpPath, Ci.nsIRequest.LOAD_BYPASS_CACHE,
           true,   // expect success
           false,  // read from cache
           true),  // hit server
  new Test(shortexpPath, Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE,
           false,  // expect success
           false,  // read from cache
           false), // hit server
  new Test(shortexpPath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_NEVER,
           true,   // expect success
           true,   // read from cache
           false), // hit server
  new Test(shortexpPath, Ci.nsIRequest.LOAD_FROM_CACHE,
           true,   // expect success
           true,   // read from cache
           false), // hit server

  new Test(longexpPath, 0,
           true,   // expect success
           false,  // read from cache
           true),  // hit server
  new Test(longexpPath, 0,
           true,   // expect success
           true,   // read from cache
           false), // hit server
  new Test(longexpPath, Ci.nsIRequest.LOAD_BYPASS_CACHE,
           true,   // expect success
           false,  // read from cache
           true),  // hit server
  new Test(longexpPath,
           Ci.nsIRequest.VALIDATE_ALWAYS,
           true,   // expect success
           true,   // read from cache
           true),  // hit server
  new Test(longexpPath, Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE,
           true,   // expect success
           true,   // read from cache
           false), // hit server
  new Test(longexpPath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_NEVER,
           true,   // expect success
           true,   // read from cache
           false), // hit server
  new Test(longexpPath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_ALWAYS,
           false,  // expect success
           false,  // read from cache
           false), // hit server
  new Test(longexpPath, Ci.nsIRequest.LOAD_FROM_CACHE,
           true,   // expect success
           true,   // read from cache
           false), // hit server

  new Test(nocachePath, 0,
           true,   // expect success
           false,  // read from cache
           true),  // hit server
  new Test(nocachePath, 0,
           true,   // expect success
           true,   // read from cache
           true),  // hit server
  new Test(nocachePath, Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE,
           false,  // expect success
           false,  // read from cache
           false), // hit server
  new Test(nocachePath, Ci.nsIRequest.LOAD_FROM_CACHE,
           true,   // expect success
           true,   // read from cache
           false), // hit server

/**
 * XXX bug 454878: The memory cache is currently broken in 1.9.0, and these
 * tests rely on the memory cache.

  // LOAD_ONLY_FROM_CACHE would normally fail (because no-cache forces
  // a validation), but VALIDATE_NEVER should override that.
  new Test(nocachePath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_NEVER,
           true,   // expect success
           true,   // read from cache
           false), // hit server

  new Test(nostorePath, 0,
           true,   // expect success
           false,  // read from cache
           true),  // hit server
  new Test(nostorePath, 0,
           true,   // expect success
           false,  // read from cache
           true),  // hit server
  new Test(nostorePath, Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE,
           false,  // expect success
           false,  // read from cache
           false), // hit server
  new Test(nostorePath, Ci.nsIRequest.LOAD_FROM_CACHE,
           true,   // expect success
           true,   // read from cache
           false), // hit server
  // no-store should force the validation (and therefore failure, with
  // LOAD_ONLY_FROM_CACHE) even if VALIDATE_NEVER is set.
  new Test(nostorePath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_NEVER,
           false,  // expect success
           false,  // read from cache
           false)  // hit server
*/
  ];

function run_next_test()
{
  if (gTests.length == 0) {
    httpserver.stop();
    do_test_finished();
    return;
  }

  var test = gTests.shift();
  test.run();
}

function handler(metadata, response) {
  gHitServer = true;
  try {
    var etag = metadata.getHeader("If-None-Match");
  } catch(ex) {
    var etag = "";
  }
  if (etag == "testtag") {
    // Allow using the cached data
    response.setStatusLine(metadata.httpVersion, 304, "Not Modified");
  } else {
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "text/plain", false);
    response.setHeader("ETag", "testtag", false);
    const body = "data";
    response.bodyOutputStream.write(body, body.length);
  }
}

function nocache_handler(metadata, response) {
  response.setHeader("Cache-Control", "no-cache", false);
  handler(metadata, response);
}

function nostore_handler(metadata, response) {
  response.setHeader("Cache-Control", "no-store", false);
  handler(metadata, response);
}

function shortexp_handler(metadata, response) {
  response.setHeader("Cache-Control", "max-age=0", false);
  handler(metadata, response);
}

function longexp_handler(metadata, response) {
  response.setHeader("Cache-Control", "max-age=10000", false);
  handler(metadata, response);
}

function run_test() {
  httpserver = new nsHttpServer();
  httpserver.registerPathHandler(shortexpPath, shortexp_handler);
  httpserver.registerPathHandler(longexpPath, longexp_handler);
  httpserver.registerPathHandler(nocachePath, nocache_handler);
  httpserver.registerPathHandler(nostorePath, nostore_handler);
  httpserver.start(4444);

  run_next_test();
  do_test_pending();
}
