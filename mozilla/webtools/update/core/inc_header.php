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
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html lang="en">

<head>

    <?php
    $page_title = (empty($page_title)) ? 'Mozilla Update' : $page_title;
    echo '<title>'.$page_title.'</title>'."\n";
    ?>

    <meta http-equiv="content-type" content="text/html; charset=utf-8">
    
	<meta name="keywords" content="mozilla update, mozilla extensions, mozilla plugins, thunderbird themes, thunderbird extensions, firefox extensions, firefox themes">
	
	<link rel="stylesheet" type="text/css" href="<?php echo WEB_PATH; ?>/css/print.css" media="print">
	
	<link rel="stylesheet" type="text/css" href="<?php echo WEB_PATH; ?>/css/base/content.css" media="all">
	<link rel="stylesheet" type="text/css" href="<?php echo WEB_PATH; ?>/css/cavendish/content.css" title="Cavendish" media="all">
	
	<link rel="stylesheet" type="text/css" href="<?php echo WEB_PATH; ?>/css/base/template.css" media="screen">
	<link rel="stylesheet" type="text/css" href="<?php echo WEB_PATH; ?>/css/cavendish/template.css" title="Cavendish" media="screen">
	
	<link rel="icon" href="<?php echo WEB_PATH; ?>/favicon.ico" type="image/png">
	
	<link rel="home" title="Home" href="http://update.mozilla.org/">
<?php
if (!empty($page_headers)) {
    echo $page_headers;
}
?>
</head>

<body>
<div id="container">

<p class="skipLink"><a href="#firefox-feature" accesskey="2">Skip to main content</a></p>

<div id="mozilla-com"><a href="http://mozilla.com/">Visit mozilla.com</a></div>
<div id="header">


	<div id="key-title">

<?php
// Here we want to show different headers based on which application we are currently viewing.
// @TODO Consider a common 'branding' instead.
switch (strtolower($application)) {
    default:
    case 'firefox':
        echo '<h1><a href="'.WEB_PATH.'?application=firefox" title="Return to home page" accesskey="1"><img src="'.WEB_PATH.'/images/title-firefox.gif" width="276" height="54" alt="Firefox Add-ons Beta"></a></h1>';
    break;

    case 'thunderbird':
        echo '<h1><a href="'.WEB_PATH.'?application=thunderbird" title="Return to home page" accesskey="1"><img src="'.WEB_PATH.'/images/title-thunderbird.gif" width="355" height="54" alt="Thunderbird Add-ons Beta"></a></h1>';
    break;

    case 'mozilla':
        echo '<h1><a href="'.WEB_PATH.'?application=mozilla" title="Return to home page"
        accesskey="1"><img src="'.WEB_PATH.'/images/title-suite.gif" width="370" height="54" alt="Mozilla Suite Add-ons Beta"></a></h1>';
    break;
}
?>
	
		<form id="search" method="get" action="<?php echo WEB_PATH; ?>/quicksearch.php" title="Search Mozilla Update">
		<div>
		<label for="q" title="Search Mozilla Update">search:</label>
		<input type="text" id="q" name="q" accesskey="s" size="10">
		<select name="section" id="sectionsearch">
		  <option value="A">Entire Site</option>
		  <option value="E">Extensions</option>
		  <option value="T">Themes</option>
		</select>
		<input type="submit" id="submit" value="Go">
		</div>
		</form>
	</div>
	<div id="key-menu">	
        <ul id="menu-firefox">
            <li<?=(isset($currentTab)&&$currentTab=='home')?' class="current"':''?>><a href="<?=WEB_PATH?>/?application=<?=$application?>">Home</a></li>
            <li<?=(isset($currentTab)&&$currentTab=='extensions')?' class="current"':''?>><a href="<?=WEB_PATH?>/extensions/?application=<?=$application?>">Extensions</a></li>
            <li<?=(isset($currentTab)&&$currentTab=='pfs')?' class="current"':''?>><a href="https://pfs.mozilla.org/plugins/?application=<?=$application?>">Plugins</a></li>
            <li<?=(isset($currentTab)&&$currentTab=='search-engines')?' class="current"':''?>><a href="<?=WEB_PATH?>/search-engines.php?application=<?=$application?>">Search Engines</a></li>
            <li<?=(isset($currentTab)&&$currentTab=='themes')?' class="current"':''?>><a href="<?=WEB_PATH?>/themes/?application=<?=$application?>">Themes</a></li>
        </ul>
    </div>
</div>
<!-- closes #header-->

<hr class="hide">

