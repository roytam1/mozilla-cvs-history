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
// Contributor(s):
//      Mike Morgan <morgamic@gmail.com>
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
?>
<?php

// Set this page to read from the SHADOW_DB.
define('USE_SHADOW_DB',true);

require_once('./core/init.php');
$page_title = 'Search Engines :: Mozilla Update';
$page_headers = <<<SEARCHJS
<script type="text/javascript">
function errorMsg(name,ext,cat)
{
  // alert("Netscape 6 or Mozilla is needed to install a sherlock plugin");
  f=document.createElement("form");
  f.setAttribute("name","installform");
  f.setAttribute("method","post");
  f.setAttribute("action","http://mycroft.mozdev.org/error.html");
  fe=document.createElement("input");
  fe.setAttribute("type","hidden");
  fe.setAttribute("name","name");
  fe.setAttribute("value",name);
  f.appendChild(fe);
  fe=document.createElement("input");
  fe.setAttribute("type","hidden");
  fe.setAttribute("name","ext");
  fe.setAttribute("value",ext);
  f.appendChild(fe);
  fe=document.createElement("input");
  fe.setAttribute("type","hidden");
  fe.setAttribute("name","cat");
  fe.setAttribute("value",cat);
  f.appendChild(fe);
  document.getElementsByTagName("body")[0].appendChild(f);
  if (document.installform) { 
    document.installform.submit();
  } else {
    location.href="http://mycroft.mozdev.org/error.html"; //hack for DOM-incompatible browsers
  }

}

function addEngine(name,ext,cat,type)
{
 if ((typeof window.sidebar == "object") && (typeof window.sidebar.addSearchEngine == "function")) { 
     window.sidebar.addSearchEngine(
       "http://addons.mozilla.org/search-engines-static/"+name+".src",
       "http://addons.mozilla.org/search-engines-static/"+name+"."+ext, name, cat );
 } else {
   errorMsg(name,ext,cat);
 } 
}

</script>
SEARCHJS;
$currentTab = 'search-engines';
require_once(HEADER);
?>

<div id="mBody">

<h1 class="first">Search Engines</h1>
<p>We've put a few popular search sites in the Search Bar in the upper-right corner of Firefox.  If you'd like to add more, there are hundreds to choose from.  Click on a Search Engine to add it to your Firefox Search Bar:</p>

<div class="front-section">
<dl>
<dt><img src="./images/search-engines/a9.png" alt=""/> <a href="javascript:addEngine('a9','png','General','0')">A9</a></dt><dd>Amazon's A9 search engine.</dd>
<dt><img src="./images/search-engines/aol.png" alt=""/> <a href="javascript:addEngine('aol','png','General','2759')">AOL</a></dt><dd>AOL search engine.</dd>
<dt><img src="./images/search-engines/jeeves.gif" alt=""/> <a href="javascript:addEngine('jeeves','gif','General','0')">Ask Jeeves</a></dt><dd>Better search results with keywords or questions.</dd>
<dt><img src="./images/search-engines/bbcnews.png" alt=""/> <a href="javascript:addEngine('bbcnews','png','News','0')">BBC News</a></dt><dd>Search for the latest news on BBC.</dd>
<dt><img src="./images/search-engines/bittorrent.png" alt=""/> <a href="javascript:addEngine('bittorrent','png','File Sharing','0')">BitTorrent</a></dt><dd>Find Torrent files</dd>
<dt><img src="./images/search-engines/del.icio.us.png" alt=""/> <a href="javascript:addEngine('del.icio.us','png','General','1091')">del.icio.us</a></dt><dd>Search through most bookmarked sites.</dd>
<dt><img src="./images/search-engines/espn.png" alt=""/> <a href="javascript:addEngine('espn','png','Recreation','0')">ESPN</a></dt><dd>Get the latest sports news, scores, and highlights.</dd>
<dt><img src="./images/search-engines/freedict.png" alt=""/> <a href="javascript:addEngine('freedict','png','Language dictionary','0')">The Free Dictionary</a></dt><dd>English, Medical, Legal, and Financial Dictionaries, Thesaurus, Encyclopedia and more.</dd>
</dl>
</div>

<div class="front-section">
<dl>
<dt><img src="./images/search-engines/flickr-tags.png" alt=""/> <a href="javascript:addEngine('flickr-tags','png','Arts','0')">Flickr Tags</a></dt><dd>Search for photos on Flickr.</dd>
<dt><img src="./images/search-engines/foodnetwork.png" alt=""/> <a href="javascript:addEngine('foodtv','png','Undefined','4003')">Food Network Recipes</a></dt><dd>Find recipes from your favorite chefs.</dd>
<dt><img src="./images/search-engines/cddball.png" alt=""/> <a href="javascript:addEngine('cddball','png','CDDB Music Search','0')">gracenote</a></dt><dd>Find album, artist, and song information.</dd>
<dt><img src="./images/search-engines/IMDB.png" alt=""/> <a href="javascript:addEngine('IMDB','png','Arts','0')">IMDB</a></dt><dd>The Internet Movie Database.</dd>
<dt><img src="./images/search-engines/linkedin.png" alt=""/> <a href="javascript:addEngine('linkedin','png','Web Services','0')">LinkedIn</a></dt><dd>Search your LinkedIn network for the people you need when you are on any Web site.</dd>
<dt><img src="./images/search-engines/lonelyplanet.png" alt=""/> <a href="javascript:addEngine('lonelyplanet','png','Travel','0')">Lonely Planet Online</a></dt><dd>Search through Lonely Planet's travel guides.</dd>
<dt><img src="./images/search-engines/marketwatch.gif" alt=""/> <a href="javascript:addEngine('marketwatch','gif','Business and Economy','4897')">MarketWatch</a></dt><dd>Stock quote look-up and financial information.</dd>
</dl>
</div>

<div class="front-section">
<dl>
<dt><img src="./images/search-engines/webster.gif" alt=""/> <a href="javascript:addEngine('webster','gif','Language dictionary','0')">Merriam-Webster</a></dt><dd>English dictionary search.</dd>
<dt><img src="./images/search-engines/msn.png" alt=""/> <a href="javascript:addEngine('MSN','png','General','3796')">MSN</a></dt><dd>MSN search engine.</dd>
<dt><img src="./images/search-engines/odeo.png" alt=""/> <a href="javascript:addEngine('odeo','png','Music','2069')">Odeo</a></dt><dd>Explore the world of podcasting.</dd>
<dt><img src="./images/search-engines/technorati.gif" alt=""/> <a href="javascript:addEngine('technorati-new','gif','Weblogs','2662')">Technorati</a></dt><dd>A real-time search engine that keeps track of what is going on in the blogosphere</dd>
<dt><img src="./images/search-engines/weatherchannel.png" alt=""/> <a href="javascript:addEngine('weather','png','Weather','0')">Weather Channel</a></dt><dd>Enter city, state or zip code to find your weather information.</dd>
<dt><img src="./images/search-engines/WebMD.png" alt=""/> <a href="javascript:addEngine('WebMD','png','Health','0')">WebMD</a></dt><dd>WebMD provides health information and tools for managing your health.</dd>
<dt><img src="./images/search-engines/wikipedia.png" alt=""/> <a href="javascript:addEngine('wikipedia','png','Reference','2286')">Wikipedia</a></dt><dd>The incredible free encyclopedia.</dd>
<dt><img src="./images/search-engines/yahooligans.png" alt=""/> <a href="javascript:addEngine('yahooligans','png','Kids and Teens','0')">Yahooligans</a></dt><dd>Search engine for kids and teens.</dd>
</dl>
</div>

<h2 class="clear-both">Additional Resources</h2>
<ul>
<li><a href="http://mycroft.mozdev.org/">Browse through more search engines at mycroft.mozdev.org.</a></li>
<li><a href="./extensions/moreinfo.php?id=1563">Use the SearchPluginHacks extension to remove a Search Engine.</a></li>
</ul>

<p>Special thanks to the Mycroft Project for their work on Firefox Search Engines.</p>

</div>

<?php
require_once(FOOTER);
?>
