#!@PERL5@ -w
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
print "<TITLE>Full Text Bug Listing</TITLE>\n";

my $generic_query  = "

select
        a.id,
        b.name,
        c.name,
        d.name,
        e.name,
        f.name,
        h.name,
        m.name,
        g.name,
        k.login_name,
        l.login_name,
        i.name,
        a.bug_file_loc,
        a.short_desc,
        j.name,
        a.qa_id,
        a.status_whiteboard,
        date_format(a.creation_ts,'Y-m-d')
from bugs         a,
     products     b,
     versions     c,
     rep_platform d,
     op_sys       e,
     bug_status   f,
     resolution   g,
     bug_severity h,
     components   i,
     milestones   j,
     profiles     k,
     profiles     l,
     priority     m
where a.product_id = b.id and
      a.version_id = c.id and
      a.rep_platform_id = d.id and
      a.op_sys_id = e.id and
      a.bug_status_id = f.id and
      a.resolution_id = g.id and
      a.bug_severity_id = h.id and
      a.component_id = i.id and
      a.milestone_id = j.id and
      a.assigned_id = k.userid and
      a.reporter_id = l.userid and
      a.priority_id = m.id and
";

ConnectToDatabase();

foreach my $bug (split(/:/, $::FORM{'buglist'})) {
    SendSQL("$generic_query a.id = $bug");

    my @row;
    if (@row = FetchSQLData()) {
        my ($id, $product, $version, $platform, $opsys, $status, $severity,
            $priority, $resolution, $assigned, $reporter, $component, $url,
            $shortdesc, $target_milestone, $qa_contact,
            $status_whiteboard) = (@row);
        print "<IMG SRC=\"1x1.gif\" WIDTH=1 HEIGHT=80 ALIGN=LEFT>\n";
        print "<TABLE WIDTH=100%>\n";
        print "<TD COLSPAN=4><TR><DIV ALIGN=CENTER><B><FONT =\"+3\">" .
            html_quote($shortdesc) .
                "</B></FONT></DIV>\n";
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
        print "<TR><TD COLSPAN=6><B>URL:</B> " . html_quote($url) . "\n";
        print "<TR><TD COLSPAN=6><B>Summary:</B> " . html_quote($shortdesc) . "\n";
        if (Param("usestatuswhiteboard")) {
            print "<TR><TD COLSPAN=6><B>Status Whiteboard:" .
                html_quote($status_whiteboard) . "\n";
        }
        print "<TR><TD><B>Description:</B>\n</TABLE>\n";
        print "<PRE>" . html_quote(GetLongDescription($bug)) . "</PRE>\n";
        print "<HR>\n";
    }
}
