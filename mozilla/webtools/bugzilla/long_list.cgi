#!/usr/bonsaitools/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
# 
# The Original Code is the Bugzilla Bug Tracking System.
# 
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Andrew Anderson <andrew@redhat.com>

use diagnostics;
use strict;
use CGI;

$::cgi = new CGI;

require "CGI.pl";
require "security.pl";

print $::cgi->header(-type=>'text/html');

my $generic_query  = "
select
  bugs.bug_id,
  bugs.product,
  bugs.version,
  bugs.rep_platform,
  bugs.op_sys,
  bugs.bug_status,
  bugs.bug_severity,
  bugs.priority,
  bugs.resolution,
  assign.login_name,
  report.login_name,
  bugs.component,
  bugs.bug_file_loc,
  bugs.short_desc,
  bugs.class,
  bugs.view
from bugs,profiles assign,profiles report
where assign.userid = bugs.assigned_to 
and report.userid = bugs.reporter and
";

ConnectToDatabase();

my $view_html;
my $class_html;
my $view_query = "SELECT type_id, name FROM type WHERE name = 'public'";
SendSQL($view_query);
(my $type_id, my $type_name) = FetchSQLData();
$view_query = "and bugs.view = " . $type_id . " ";
if(CanIView("view")){
    $view_query = "";
    $view_html = $::cgi->td("<B>View:</B> $type_name");
} else {
    $view_html = $::cgi->td("&nbsp;");
}

foreach my $bug (split(/:/, $::cgi->param('buglist'))) {
    SendSQL("$generic_query bugs.bug_id = $bug $view_query");

    my @row;
    if (@row = FetchSQLData()) {
        my ($id, $product, $version, $platform, $opsys, $status, $severity,
            $priority, $resolution, $assigned, $reporter, $component, $url,
            $shortdesc, $class) = (@row);
	PutHeader("Full Text Bug Listing", 
                  html_quote($shortdesc), 
                  $::cgi->param('id'));

    if(CanIView("class")){
        $class_html = $::cgi->td("<B>Class:</B> $class");
    } else {
        $class_html = $::cgi->td("&nbsp;");
    }

    print $::cgi->img({-src=>"1x1.gif", 
                     -width=>"1", 
                     -height=>"80", 
                     -align=>"LEFT"}),
          $::cgi->table({-width=>"100%"},
             $::cgi->TR(
                $::cgi->td("<B>Bug#:</B>", 
                   $::cgi->a({-href=>"show_bug.cgi?id=$id"}, "$id")),
                $::cgi->td("<B>Product:</B> $product"),
                $::cgi->td("<B>Version:</B> $version"),
                $::cgi->td("<B>Platform:</B> $platform")
             ),
             $::cgi->TR(
                $::cgi->td("<B>OS/Version:</B> $opsys"),
                $::cgi->td("<B>Status:</B> $status"),
                $::cgi->td("<B>Severity:</B> $severity"),
                $::cgi->td("<B>Priority:</B> $priority")
             ),
             $::cgi->TR(
                $::cgi->td("<B>Resolution:</B> $resolution"),
                $::cgi->td("<B>Assigned To:</B> $assigned"),
                $::cgi->td({-colspan=>"2"}, "<B>Reported By:</B> $reporter")
             ),
             $::cgi->TR(
                $::cgi->td("<B>Component:</B>"),
		$::cgi->td("$component"),
                $view_html,
                $class_html,
             ),
             $::cgi->TR(
                $::cgi->td({-colspan=>"6"}, "<B>URL:</B> $url"),
             ),
             $::cgi->TR(
                $::cgi->td("<B>Summary:</B>"),
                $::cgi->td({-colspan=>"5"}, $shortdesc)
             ),
             $::cgi->TR(
                $::cgi->td({-colspan=>"5"}, "<B>Description:</B>")
             )
         ),
         $::cgi->pre(GetLongDescription($bug)),
         $::cgi->hr;
    }
}
