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

require "CGI.pl";

# Shut up misguided -w warnings about "used only once".  For some reason,
# "use vars" chokes on me when I try it here.

# use vars qw($::buffer);
my $zz = $::buffer;
$zz = $zz . $zz;

confirm_login();

my $platform = url_quote($::FORM{'product'});
my $version = url_quote($::FORM{'version'});

print "Set-Cookie: PLATFORM=$platform ; path=/ ; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
print "Set-Cookie: VERSION-$platform=$version ; path=/ ; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
print "Content-type: text/html\n\n";

if (defined $::FORM{'maketemplate'}) {
    PutHeader("Bookmarks are your friend.", "Template constructed.");
    
    my $url = "enter_bug.cgi?$::buffer";

    print "If you put a bookmark <A HREF=\"$url\">to this link</A>, it will\n";
    print "bring up the submit-a-new-bug page with the fields initialized\n";
    print "as you've requested.\n";
    exit;
}

PutHeader("Posting Bug -- Please wait", "Posting Bug", "One moment please...");

umask 0;
ConnectToDatabase();

if (!defined $::FORM{'component'} || $::FORM{'component'} eq "") {
    print "You must choose a component that corresponds to this bug.  If\n";
    print "necessary, just guess.  But please hit the <B>Back</B> button\n";
    print "and choose a component.\n";
    exit 0
}
    

my $forceAssignedOK = 0;
if ($::FORM{'assigned_to'} eq "") {
    SendSQL("select initialowner from components where program=" .
            SqlQuote($::FORM{'product'}) .
            " and value=" . SqlQuote($::FORM{'component'}));
    $::FORM{'assigned_to'} = FetchOneColumn();
    $forceAssignedOK = 1;
}

$::FORM{'assigned_to'} = DBNameToIdAndCheck($::FORM{'assigned_to'}, $forceAssignedOK);
$::FORM{'reporter'} = DBNameToIdAndCheck($::FORM{'reporter'});


my @bug_fields = ("reporter", "product", "version", "rep_platform",
                  "bug_severity", "priority", "op_sys", "assigned_to",
                  "bug_status", "bug_file_loc", "short_desc", "component");
my $query = "insert into bugs (\n" . join(",\n", @bug_fields) . ",
creation_ts, long_desc )
values (
";


foreach my $field (@bug_fields) {
    $query .= SqlQuote($::FORM{$field}) . ",\n";
}

$query .= "now(), " . SqlQuote($::FORM{'comment'}) . " )\n";


my %ccids;


if (defined $::FORM{'cc'}) {
    foreach my $person (split(/[ ,]/, $::FORM{'cc'})) {
        if ($person ne "") {
            $ccids{DBNameToIdAndCheck($person)} = 1;
        }
    }
}


# print "<PRE>$query</PRE>\n";

SendSQL($query);

SendSQL("select LAST_INSERT_ID()");
my $id = FetchOneColumn();

foreach my $person (keys %ccids) {
    SendSQL("insert into cc (bug_id, who) values ($id, $person)");
}

print "<H2>Changes Submitted</H2>\n";
print "<A HREF=\"show_bug.cgi?id=$id\">Show BUG# $id</A>\n";
print "<BR><A HREF=\"query.cgi\">Back To Query Page</A>\n";
print "<BR><A HREF=\"enter_bug.cgi?product=" . url_quote($::FORM{'product'}). "\">Enter a new bug</A>\n";


system("./processmail $id < /dev/null > /dev/null 2> /dev/null &");
exit;
