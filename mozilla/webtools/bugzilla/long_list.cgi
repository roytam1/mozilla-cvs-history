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


use diagnostics;
use strict;

require "CGI.pl";

# Shut up misguided -w warnings about "used only once":
use vars %::FORM;

print "Content-type: text/html\n\n";

ConnectToDatabase();
quietly_check_login();

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
  bugs.target_milestone,
  bugs.qa_contact,
  bugs.status_whiteboard
from bugs,profiles assign,profiles report
where assign.userid = bugs.assigned_to and report.userid = bugs.reporter and ";

foreach my $bug (split(/:/, $::FORM{'buglist'})) {
 	if (!CanISee($bug, DBname_to_id($::COOKIE{'Bugzilla_login'}))) {
		next;
	} 

	SendSQL("$generic_query bugs.bug_id = $bug");

    my @row;
    if (@row = FetchSQLData()) {
        my ($id, $product, $version, $platform, $opsys, $status, $severity,
            $priority, $resolution, $assigned, $reporter, $component, $url,
            $shortdesc, $target_milestone, $qa_contact,
            $status_whiteboard) = (@row);
		print "<HTML><HEAD><TITLE>Bug # $id</TITLE></HEAD><BODY BGCOLOR=white>\n";
        print "<TABLE CELLSPACING=4 CELLPADDING=4>\n";
        print "<TR><TD COLSPAN=4 ALIGN=center><B><FONT =\"+3\">" .
            html_quote($shortdesc) .
                "</B></FONT></TD></TR>\n";
        print "<TR><TD><B>Bug#:</B> <A HREF=\"show_bug.cgi?id=$id\">$id</A>\n";
        print "<TD><B>Product:</B> $product\n";
        print "<TD><B>Version:</B> $version\n";
        print "<TD><B>Platform:</B> $platform\n";
        print "<TR><TD><B>OS/Version:</B> $opsys\n";
        print "<TD><B>Status:</B> $status\n";
        print "<TD><B>Severity:</B> $severity\n";
        print "<TD><B>Priority:</B> $priority\n";
        print "<TR><TD><B>Resolution:</B> $resolution</TD>\n";
        print "<TD><B>Assigned To:</B> $assigned\n";
        print "<TD><B>Reported By:</B> $reporter\n";
        if (Param("useqacontact")) {
            my $name = "";
            if ($qa_contact > 0) {
                $name = DBID_to_name($qa_contact);
            }
            print "<TD><B>QA Contact:</B> $name\n";
        }
        print "<TR><TD><B>Component:</B> $component\n";
        if (Param("usetargetmilestone")) {
            print "<TD><B>Target milestone:</B>$target_milestone\n";
        }
        print "<TR><TD COLSPAN=6><B>URL:</B>&nbsp;";
	print "<A HREF=\"" . $url . "\">" .  html_quote($url) . "</A>\n"; 
        print "<TR><TD COLSPAN=6><B>Summary:</B> " . html_quote($shortdesc) . "\n";
        if (Param("usestatuswhiteboard")) {
            print "<TR><TD COLSPAN=6><B>Status Whiteboard:" .
                html_quote($status_whiteboard) . "\n";
        }
        print "<TR><TD><B>Description:</B></TD></TR>\n";
        print "<TR><TD ALIGN=left COLSPAN=4><PRE>" . html_quote(GetLongDescriptionAsText($bug)) . "</PRE></TD>\n";
		print "</TR></TABLE></BODY></HTML>\n";
    }
}

