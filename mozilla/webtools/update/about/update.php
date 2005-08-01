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

require_once('../core/config.php');
$page_title = 'Mozilla Update :: News and Updates';
require_once(HEADER);
?>

<hr class="hide">
<div id="mBody">

<h1>Progress! :: 2005/08/01</h1>

<dl>
<dt>Reviewers</dt>
<dd>Thanks to our <a href="http://wiki.mozilla.org/Update:Reviewers_Guide">many reviewers</a> we have been able to keep the approval queue moving along for new Addons.  However, there is still a lot of work - you can help get new Addons out quicker by <a href="http://wiki.mozilla.org/Update:Home_Page#Volunteering">becoming a reviewer today!</a></dd>

<dt>v2.0 Progress</dt>
<dd><a href="http://wiki.mozilla.org/Update:Development:v2.0"><acronym title="addons.mozilla.org">AMO</acronym> v2.0</a> is <a href="http://bonsai.mozilla.org/cvsquery.cgi?treeid=default&amp;module=Webtools&amp;branch=HEAD&amp;branchtype=match&amp;dir=mozilla%2Fwebtools%2Faddons&amp;file=&amp;filetype=match&amp;who=&amp;whotype=match&amp;sortby=Date&amp;hours=2&amp;date=week&amp;mindate=&amp;maxdate=&amp;cvsroot=%2Fcvsroot">well underway</a>.  Our development team is working hard on a new framework that will bring you new features and improvements.  Stay tuned!</dd>

<dt>CSS Issues</dt>
<dd>You may have noticed some CSS problems with the site.  A solution has been found and the problem should be fixed.  Thanks to all of you who submitted comments regarding this matter.</dd>
</dl>

<h1>Going Live :: 2005/04/15</h1>

<p>It's time to get the Addons site back on track.  We have spent the last month working
hard making the Addons site more secure so that users can trust the software they are
using, downloading and installing.</p>

<p>The enabling of the developer section of the Addons site is absolutely critical
to the success of Firefox.  We believe that the differentiating factor behind Firefox is
more than just a great application; it's an enabled community of users and developers
that have exactly what they want, when they want it.</p>

<p>We are now going to be focusing our attention on Addons 
<a href="http://wiki.mozilla.org/Update:Development:v2.0">v2.0</a> which is going to
be a complete re-write of the current codebase focusing on ease of use, scalability,
security and most importantly maintainability over time.  We want to build a platform
that will make it easy for extension and theme developers to do what they do best;
innovate.</p>


<h1>Status Update :: 2005/03/23</h1>

        <p>Over the past month, the UMO developers have been working on doing a security audit of the codebase that supports addons.mozilla.org and pfs.mozilla.org.  That audit is complete as well as <a href="https://bugzilla.mozilla.org/show_bug.cgi?id=287159">some</a> <a href="https://bugzilla.mozilla.org/show_bug.cgi?id=278016">patches</a> to ease the development of the application by single developers and solve some performance issues.  These are in the process of being reviewed.</p>
    <p>The <a href="http://wiki.mozilla.org/Update:Development:v1.0">security audit</a> revealed <a href="http://wiki.mozilla.org/Update:Development:v1.0#Audit_Log">several problems</a> with the existing codebase.  Most of these can be remedied but there is a lot of work to be done.  The final word on the security audit is that <a href="http://wiki.mozilla.org/Update:Development:v2.0">v2.0</a> of UMO will most likely need to be re-written from the ground up.</p>

    <p>We have brought up a new <a href="http://wiki.mozilla.org/Update:Home_Page#Developer.27s_Guide">development environment</a> with development, staging and sandbox space for developers.  This is going to allow us to do much faster development and give the release engineering folks a solid foundation for testing of new features, etc.</p>
    <p>Now that we have identified <a href="http://wiki.mozilla.org/Update:Development:v1.0#Audit_Log">things</a> we need to get cleaned up, we&#8217;re looking for volunteers to help make it happen.  If you, or someone you know, is interested in working on the UMO project and doing some PHP development we can definitely use the help.  In addition, we are going to be looking for people in the near future to act as moderators, QA and reviewers of user extensions that land on UMO.  Application development is not required there but would be helpful.  If you are interested in being a developer or moderator please contact <a href="http://wiki.mozilla.org/Update:Home_Page#Contact_Information">me</a> or join the development team in #umo on irc.mozilla.org.
</p>

<h1>Status Update :: 2005/02/03</h1>

    <p>The Mozilla Update service (a.k.a UMO) has been frozen for close to a month now for various reasons.  We would like to take this opportunity to bring folks up to speed on where it is going and when it will get there.</p>
    <p>Just a few weeks ago, the <a href="http://www.psychoticwolf.net">lead developer</a> stepped down and so the Mozilla Foundation has taken the chance to re-evaluate the project and look at how to best move it forward.  Looking over the existing v1.0 codebase, the remaining developers felt there were security concerns and scalability issues with the site that needed to be addressed.  At this time, the site and CVS for updates to the UMO codebase are frozen.</p>
    <p>However, the community awaits and we really want to get the the site back on-line so we can continue the momentum that the release of Firefox 1.0 has created.  To that end, we are moving quickly to do the following:</p>
    <ul>

    <li>Next week we will be launching a <a href="http://wiki.mozilla.org/wiki/Update:Development:v1.0">security audit</a> of the existing UMO code.  The goal is to get the UMO site back to a known state so that we can feel comfortable with pushing out updates to end-users.</li>

    <li>Starting next week, we are going to start processing the pending requests in the UMO queue.  We want to do updates for existing extension/theme developers via the UMO site admins.</li>
    <li>Upon completion of the <a href="http://wiki.mozilla.org/wiki/Update:Development:v1.0">security audit</a> we will be starting to work up requirements for <a href="http://wiki.mozilla.org/wiki/Update:Development:v2.0">UMO v2.0</a> which will be a complete rewrite and will allow us to usethings like database abstraction and templating.</li>

    <li>To help scale the site, we have decoupled the services from one URL (http://update.mozilla.org) to several: <a href="http://wiki.mozilla.org/wiki/Update:Architecture_and_General_Design#Application_Update_Service">Application Update Service - AUS</a> , <a href="http://wiki.mozilla.org/wiki/Update:Architecture_and_General_Design#Plugin_Finder_Service">Plugin Finder Service - PFS</a> and <a href="http://wiki.mozilla.org/wiki/Update:Architecture_and_General_Design#Addons.Mozilla.Org_End-User_Website">Addons section with extensions/themes</a>.  This will allow us to upgrade the busiest parts of the site in the most cost effective and scalable manners.</li>

    </ul>
    <p>The biggest concern right now is that we don&#8217;t know what we&#8217;re dealing with.  Once we know what we have and have completed the security audit, we will turn the site back on in its existing state (with several bugs fixed).  In the mean time, please bear with us as we hand audit each pending request and get the site back rolling (as slow as that may be).</p>

    <p>We&#8217;re working hard to get UMO going again and we want to be sure to include everyone in our <a href="http://wiki.mozilla.org/wiki/Update:Home_Page">discussions</a>.</p>

</div>

<?php
require_once(FOOTER);
?>
