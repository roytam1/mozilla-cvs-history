#!/usr/bonsaitools/bin/perl -wT
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Gervase Markham <gerv@gerv.net>

use diagnostics;
use strict;
use lib qw(.);

require "CGI.pl";

use vars qw($userid $usergroupset @legal_keywords %FORM);

# Use global template variables.
use vars qw($template $vars);

ConnectToDatabase();

quietly_check_login();

GetVersionTable();

my $generic_query = "
  SELECT 
    bugs.bug_id, 
    bugs.product, 
    bugs.version, 
    bugs.rep_platform,
    bugs.op_sys, 
    bugs.bug_status, 
    bugs.resolution, 
    bugs.priority,
    bugs.bug_severity, 
    bugs.component, 
    assign.login_name, 
    report.login_name,
    bugs.bug_file_loc, 
    bugs.short_desc, 
    bugs.target_milestone,
    bugs.qa_contact, 
    bugs.status_whiteboard, 
    bugs.keywords
  FROM bugs,profiles assign,profiles report
  WHERE assign.userid = bugs.assigned_to AND report.userid = bugs.reporter";

my $buglist = $::FORM{'buglist'} || 
              $::FORM{'bug_id'}  || 
              $::FORM{'id'}      || "";

my @bugs;

foreach my $bug_id (split(/[:,]/, $buglist)) {
    detaint_natural($bug_id) || next;
    SendSQL(SelectVisible("$generic_query AND bugs.bug_id = $bug_id",
                          $::userid, $::usergroupset));

    my %bug;
    my @row = FetchSQLData();

    foreach my $field ("bug_id", "product", "version", "rep_platform",
                       "op_sys", "bug_status", "resolution", "priority",
                       "bug_severity", "component", "assigned_to", "reporter",
                       "bug_file_loc", "short_desc", "target_milestone",
                       "qa_contact", "status_whiteboard", "keywords") 
    {
        $bug{$field} = shift @row;
    }
    
    if ($bug{'bug_id'}) {
        $bug{'comments'} = GetComments($bug{'bug_id'});
        $bug{'qa_contact'} = $bug{'qa_contact'} > 0 ? 
                                          DBID_to_name($bug{'qa_contact'}) : "";

        push (@bugs, \%bug);
    }
}

# Add the list of bug hashes to the variables
$vars->{'bugs'} = \@bugs;

$vars->{'use_keywords'} = 1 if (@::legal_keywords);

$vars->{'quoteUrls'} = \&quoteUrls;
$vars->{'time2str'} = \&time2str;
$vars->{'str2time'} = \&str2time;

# Work out a sensible filename for Content-Disposition.
# Sadly, I don't think we can tell if this was a named query.
my @time = localtime(time());
my $date = sprintf "%04d-%02d-%02d", 1900+$time[5],$time[4]+1,$time[3];
my $filename = "bugs-$date.html";

print "Content-Type: text/html\n";
print "Content-Disposition: inline; filename=$filename\n\n";

# Generate and return the UI (HTML page) from the appropriate template.
$template->process("bug/show-multiple.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
