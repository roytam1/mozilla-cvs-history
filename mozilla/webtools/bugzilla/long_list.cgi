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
my $pre = Param("prefix");
require "$pre-security.pl";

# Shut up misguided -w warnings about "used only once":
use vars %::FORM;

print "Content-type: text/html\n\n";

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

my $view_query = "SELECT type_id, name FROM type WHERE name = 'public'";
SendSQL($view_query);
(my $type_id, my $type_name) = FetchSQLData();
$view_query = "and bugs.view = " . $type_id . " ";
if(CanIView("view")){
	$view_query = "";
}

foreach my $bug (split(/:/, $::FORM{'buglist'})) {
    SendSQL("$generic_query bugs.bug_id = $bug $view_query");

    my @row;
    if (@row = FetchSQLData()) {
        my ($id, $product, $version, $platform, $opsys, $status, $severity,
            $priority, $resolution, $assigned, $reporter, $component, $url,
            $shortdesc, $class) = (@row);
	PutHeader("Full Text Bug Listing", html_quote($shortdesc), $::FORM{'id'});

        print "
<IMG SRC=\"1x1.gif\" WIDTH=\"1\" HEIGHT=\"80\" ALIGN=\"LEFT\">
<TABLE WIDTH=\"100%\">
  <TR>
    <TD><B>Bug#:</B> <A HREF=\"show_bug.cgi?id=$id\">$id</A>
    <TD><B>Product:</B> $product
    <TD><B>Version:</B> $version
    <TD><B>Platform:</B> $platform
  <TR>
    <TD><B>OS/Version:</B> $opsys
    <TD><B>Status:</B> $status
    <TD><B>Severity:</B> $severity
    <TD><B>Priority:</B> $priority
  <TR>
    <TD><B>Resolution:</B> $resolution</TD>
    <TD><B>Assigned To:</B> $assigned
    <TD COLSPAN=\"2\"><B>Reported By:</B> $reporter
  <TR>
    <TD COLSPAN=\"2\"><B>Component:</B> $component";

if(CanIView("class")){
    print "    <TD><B>Class:</B> $class\n";
} else {
    print "    <TD>&nbsp; </TD>\n";
}

if(CanIView("view")){
    print "    <TD><B>View:</B> $type_name\n";
} else {
    print "    <TD>&nbsp; </TD>\n";
}

print "
  <TR>
    <TD COLSPAN=\"6\"><B>URL:</B> " . html_quote($url) . "
  <TR>
    <TD><B>Summary:</B>
    <TD COLSPAN=\"5\"> " . html_quote($shortdesc) . "
  <TR>
    <TD COLSPAN=\"5\"><B>Description:</B>
</TABLE>
<PRE>" . html_quote(GetLongDescription($bug)) . "</PRE>
<HR>
";
    }
}
