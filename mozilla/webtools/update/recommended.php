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
$page_title = 'We Recommend :: Mozilla Update';
require_once(HEADER);
installtrigger('extensions');
?>

<div id="mBody">
<h1>Getting Started with Extensions</h1>
<p>With over 300 extensions available for Firefox, there's something for everyone. To get you started, here's a list of some of our  recommended extensions that make use of popular online services.</p>

<div class="recommended">
<img class="recommended-img" alt="" src="./images/recommended/answers.com.png"/>
<h2><a href="./extensions/moreinfo.php?id=735">Answers.com</a></h2>
<div class="recommended-download">
<h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/answers/answers-1.24-fx+fl.xpi" onclick="return install(event,'answers 1.24', '<?=WEB_PATH?>/images/default.png');" title="Install answers 1.24 (Right-Click to Download)">Install Extension (60 KB)</a></h3>
</div>
<p>Hold down Alt (or Option on a Mac) and click on any word to get a quick definition, an up-to-the-minute reference and more. You don't even need to highlight the word!</p>
</div>

<div class="recommended">
<img class="recommended-img" alt="" src="./images/recommended/del.icio.us.png"/>
<h2><a href="./extensions/moreinfo.php?id=1532">del.icio.us</a></h2>
<div class="recommended-download">
<h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/del.icio.us/del.icio.us-1.0.2-fx.xpi" onclick="return install(event,'del.icio.us 1.0.2', '<?=WEB_PATH?>/images/default.png');" title="Install del.icio.us 1.0.2 (Right-Click to Download)">Install Extension (62 KB)</a> </h3>
</div>
<p>Everything you need to seamlessly integrate del.icio.us with your  browser! Easy posting
of new items, access to your saved items, and integration with the  Firefox search box.</p>
</div>


<div class="recommended">
<img class="recommended-img" alt="" src="./images/recommended/foxytunes.jpg"/>
<h2><a href="./extensions/moreinfo.php?id=219">FoxyTunes</a></h2>
<div class="recommended-download">
<h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/foxytunes/foxytunes-1.1.5.4-fx+mz+tb.xpi" onclick="return install(event,'FoxyTunes 1.1.5.4', '<?=WEB_PATH?>/images/default.png');" title="Install FoxyTunes 1.1.5.4 (Right-Click to Download)">Install Extension (200 KB)</a></h3>
</div>
<p>Do you listen to music while surfing the Web? Now you can control your favorite media player without ever leaving Firefox.</p>
</div>


<div class="recommended">
<img class="recommended-img" alt="" src="./images/recommended/kaboodle.jpg"/>
<h2><a href="./extensions/moreinfo.php?id=1577">Kaboodle</a></h2>
<div class="recommended-download">
<h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/kaboodle/kaboodle-0.1-fx.xpi" onclick="return install(event,'Kaboodle Toolbar 0.1', '<?=WEB_PATH?>/images/default.png');" title="Install Kaboodle Toolbar 0.1 (Right-Click to Download)">Install Extension (11 KB)</a></h3>
</div>
<p>Collect and share information you find on the web using Kaboodle. One click extracts the web page's location, title, description (and even prices of items featured on that page) and adds it to a list you can share with others.</p>
</div>


<div class="recommended">
<img class="recommended-img" alt="" src="./images/recommended/linkedin.png"/>
<h2><a href="./extensions/moreinfo.php?id=1512">LinkedIn</a></h2>
<div class="recommended-download">
<h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/linkedin_companion_for_firefox/linkedin_companion_for_firefox-1.1-fx+mz.xpi" onclick="return install(event,'LinkedIn 1.1', '<?=WEB_PATH?>/images/default.png');" title="Install LinkedIn 1.1 (Right-Click to Download)">Install Extension (82 KB)</a></h3>
</div>
<p>Take advantage of the LinkedIn Companion for Firefox to easily search  for professionals or jobs. LinkedIn is an online network of more than  4 million experienced professionals from around the world.</p>
</div>

<div class="recommended">
<img class="recommended-img" alt="" src="./images/recommended/paypal1.png"/>
<h2><a href="./extensions/moreinfo.php?id=1538">PayPal</a></h2>
<div class="recommended-download">
<h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/paypal_send_money/paypal_send_money-1.0.1-fx.xpi" onclick="return install(event,'PayPal 1.0.1', '<?=WEB_PATH?>/images/default.png');" title="Install PayPal 1.0.1 (Right-Click to Download)">Install Extension (52 KB)</a></h3>
</div>
<p>PayPal's Send Money extension provides a quick way to send money to  anyone with an email address. Enter the payment information and  you'll be brought to your PayPal login page to sign in and complete the payment.</p>
</div>

<div class="recommended">
<img class="recommended-img" alt="" src="./images/recommended/performancing-small.png"/>
<h2><a href="./extensions/moreinfo.php?id=1730">Performancing</a></h2>
<div class="recommended-download">
<h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/performancing/performancing-1.0.1-fx+fl+mz.xpi" onclick="return install(event,'Performancing 1.0.1', '/~morgamic/tip/images/default.png');" title="Install Performancing 1.0.1 (Right-Click to Download)">Install Extension (146KB)</a></h3>
</div>
<p>Performancing for Firefox is a full featured blog editor that sits right in your Firefox browse and lets you post to your blog easiy. You can drag and drop formatted text from the page you happen to be browsing, and take notes as well as post to your blog.</p>
</div>

<div class="recommended">
<img class="recommended-img" alt="" src="./images/recommended/sage.png"/>
<h2><a href="./extensions/moreinfo.php?id=77">Sage</a></h2>
<div class="recommended-download">
<h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/sage/sage-1.3.6-fx+mz.xpi" onclick="return install(event,'sage 1.3.6', '<?=WEB_PATH?>/images/default.png');" title="Install Sage 1.3.6 (Right-Click to Download)">Install Extension (132 KB)</a></h3>
</div>
<p>Sage is a lightweight Web feed reader for Mozilla Firefox. It's got a  lot of what you need and not much of what you don't. Supports RSS,  Atom, automatic feed discovery, and full integration with Firefox's Live Bookmarks.</p>
</div>

<div class="recommended">
<img class="recommended-img" alt="" src="./images/recommended/stumble3.png"/>
<h2><a href="./extensions/moreinfo.php?id=138">StumbleUpon</a></h2>
<div class="recommended-download">
<h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/stumbleupon/stumbleupon-2.4-fx+fl+mz+ns.xpi" onclick="return install(event,'stumbleupon 2.2', '<?=WEB_PATH?>/images/default.png');" title="Install stumbleupon 2.2 (Right-Click to Download)">Install Extension (103 KB)</a></h3>
</div>
<p>Break out of your web routine with this extension. StumbleUpon helps you find great new peer-reviewed sites through a collaborative site sharing and review system.</p>
</div>

<br class="clear-both"/>

</div>

</div>

<?php
require_once(FOOTER);
?>
