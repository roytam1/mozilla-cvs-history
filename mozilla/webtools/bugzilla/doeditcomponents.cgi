#!/usr/bin/perl -w
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
# $Id$
#
# Contributor(s): Sam Ziegler <sam@ziegler.org>
#                 Andrew Anderson <andrew@redhat.com>

use diagnostics;
use strict;
use CGI;

$::cgi = new CGI;

require "CGI.pl";

confirm_login();

print "Content-type: text/html\n\n";

if (!UserInGroup("editcomponents")) {
    print "<H1>Sorry, you aren't a member of the 'editcomponents' group.</H1>\n";
    print "And so, you aren't allowed to edit the owners.\n";
    exit;
}

PutHeader("Saving new owners");

SendSQL("select program, value, initialowner from components 
	 where program = \'" . $::cgi->param('cur_program') . "\' order by program, value");

my @line;
my @updates;
my $curIndex = 0;
my $cgiItem;

while (@line = FetchSQLData()) {
    my $curItem = "$line[0]_$line[1]";
    my $cgiItem = $::cgi->param($curItem);
    if ($cgiItem ne "") {
        $cgiItem =~ s/\r\n/\n/;
        if ($cgiItem ne "$line[2]") {
            print "$line[0]: $line[1] is now owned by $cgiItem.<BR>\n";
            $updates[$curIndex++] = "update components set initialowner = '$cgiItem' where program = '$line[0]' and value = '$line[1]'";
        }
    }
}

foreach my $update (@updates) {
    #print $::cgi->pre($update);
    SendSQL($update);
}

print "OK, done.<P>\n",
      $::cgi->a({-href=>"editcomponents.cgi"}, "Edit the owners some more."),
      $::cgi->br,
      $::cgi->a({-href=>"query.cgi"}, "Go back to the query page.");
