<?php
// ***** BEGIN LICENSE BLOCK *****
// Version: MPL 1.1/GPL 2.0/LGPL 2.1
//
// The contents of this file are subject to the Mozilla Public License Version
// 1.1 (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
// http://www.mozilla.org/MPL/
//
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
// for the specific language governing rights and limitations under the
// License.
//
// The Original Code is Mozilla Update.
//
// The Initial Developer of the Original Code is
// Chris "Wolf" Crews.
// Portions created by the Initial Developer are Copyright (C) 2004
// the Initial Developer. All Rights Reserved.
//
// Contributor(s):
//   Chris "Wolf" Crews <psychoticwolf@carolina.rr.com>
//
// Alternatively, the contents of this file may be used under the terms of
// either the GNU General Public License Version 2 or later (the "GPL"), or
// the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
// in which case the provisions of the GPL or the LGPL are applicable instead
// of those above. If you wish to allow use of your version of this file only
// under the terms of either the GPL or the LGPL, and not to allow others to
// use your version of this file under the terms of the MPL, indicate your
// decision by deleting the provisions above and replace them with the notice
// and other provisions required by the GPL or the LGPL. If you do not delete
// the provisions above, a recipient may use your version of this file under
// the terms of any one of the MPL, the GPL or the LGPL.
//
// ***** END LICENSE BLOCK *****
	
// Set this page to read from the SHADOW_DB.
define('USE_SHADOW_DB',true);

require_once('./core/init.php');

$titleCaseApp=ucwords($application); // cache results!
$uriparams=uriparams(); // cache results!
$page_title = 'Web Developer Kit :: Mozilla Update';
require_once(HEADER);
?>

<div id="mBody">

<h1>Web Developer Kit</h1>
<p>There are several extensions specifically for Web Developers that make creating and debugging Web sites and applications easier in Firefox.  Here are a few of our favorites.</p>

<h2 class="clear-both"><a href="./extensions/moreinfo.php?id=60">Web Developer Extension</a></h2>
<img class="imgright" alt="" src="./images/webdevkit/webdeveloper.png"/>
<p>This extension has long been a mainstay for web developers.  When installed, the Web Developer extension adds a toolbar that gives you quick and easy access to a huge number of useful utilities, ranging from viewing and editing CSS through one-click CSS, HTML, and Section 508 validation. </p>

<h2 class="clear-both"><a href="./extensions/moreinfo.php?id=394">View Source With</a></h2>
<img class="imgright" alt="" src="./images/webdevkit/viewsourcewith.png"/>
<p>The ViewSourceWith extension allows you to specify any external application with which to view the source of a web page.  This extension gives you the freedom to use the application of your choice.</p>

<h2 class="clear-both"><a href="http://www.kevinfreitas.net/extensions/measureit/">Measure It</a></h2>
<img class="imgright" alt="" src="./images/webdevkit/measureit.png"/>
<p>If you've ever needed to know exactly how many pixels it is from point A to point B on a web page, Measure It is here to save the day. This little extension allows you to "pull" a ruler from any point to any other point within the browser window to get precise pixel measurements for height and width. It's turned on and off with a simple click in the bottom left-hand corner of the browser. Very handy for those pixel-perfect layouts.</p>

<h2 class="clear-both"><a href="./extensions/moreinfo.php?id=655">View Rendered Source Chart</a></h2>
<img class="imgright" alt="" src="./images/webdevkit/viewrenderedsourcechart.png"/>
<p>View Rendered Source Chart creates a beautifully formatted and shaded rendering of your page source (including dynamically-generated source, static source, and JavaScript output) that clearly and cleanly displays the nested elements.</p>

<h2 class="clear-both"><a href="./extensions/moreinfo.php?id=532">Link Checker</a></h2>
<img class="imgright" alt="" src="./images/webdevkit/linkchecker.png"/>
<p>Fast, simple, visual link checker. This is an absolute godsend of an extension for maintaining web pages and fending off the nefarious demons of link-rot.</p>

<h2 class="clear-both"><a href="./extensions/moreinfo.php?id=1290">UrlParams</a></h2>
<img class="imgright" alt="" src="./images/webdevkit/urlparams.png"/>
<p>Essential for testing and debugging form submissions or other URLs that contain a number of name-value pairs, this extension stays continually updated while you surf. In a nicely designed and compact sidebar, the extension displays the bare URL (without parameters), the referring URL, GET and POST values, the ability to add more name/value pairs to a set, and the option to submit or resubmit (in current window or new tab) the URL values.</p>

<h2 class="clear-both"><a href="./extensions/moreinfo.php?id=1146">Screen Grab</a></h2>
<img class="imgright" alt="" src="./images/webdevkit/screengrab.png"/>
<p>Requiring Java (JVM), you can take full “screen shots” of entire web pages with this extension. Unlike normal screen shots that only capture what’s displayed within the confines of the browser window, Screen Grab will capture an image of the whole page from top to bottom.</p>

<h2 class="clear-both"><a href="http://www.bitstorm.org/extensions/view-cookies/">View Cookies</a></h2>
<img class="imgright" alt="" src="./images/webdevkit/viewcookies.png"/>
<p>Adding a new tab in the “Page Info” dialog (accessible on any page via the right-click context menu), View Cookies displays all of the cookies and cookie values associated with a page. This is an obvious boon for tracking and debugging cookies during development.</p>

</div>

<?php
require_once(FOOTER);
?>
