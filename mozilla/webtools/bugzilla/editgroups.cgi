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
# Contributor(s): Andrew Anderson <andrew@redhat.com>

# Code derived from editparams.cgi

use diagnostics;
use strict;

use CGI;
$::cgi = new CGI;

require "CGI.pl";

confirm_login();

if (Param("maintainer") ne $::cgi->cookie(-name=>'Bugzilla_login')) {
    PutHeader("Sorry, you aren't the maintainer of this system.\n");
    print "And so, you aren't allowed to edit the parameters of it.\n";
    exit;
}

my @line;
my $group;
my $counter = 1;
my $rowbreak = "</TR>" . $::cgi->TR($::cgi->td({-colspan=>"5"}, "<HR>"));

PutHeader("Edit Group Permissions");

print "This lets you edit the group permissions of bugzilla.\n";

print $::cgi->startform(-method=>"POST", -action=>"doeditgroups.cgi");

print "<TABLE BORDER=\"0\">\n";

SendSQL("select groupid, groupname, flags from groups order by groupid asc");

my @groups;

while (@line = FetchSQLData()) {
	push(@groups, join(":", @line));
}

SendSQL("select field, flag from security");

my @flags;

while (@line = FetchSQLData()) {
	push(@flags, join(":", @line));
}

foreach $group (@groups) {
    (my $groupid, my $groupname, my $groupflag) = split(":", $group);

    print $::cgi->TR(
             $::cgi->td({-align=>"LEFT", -valign=>"TOP", -colspan=>"2"},
                $::cgi->hidden(-name=>"${groupid}-groupid", 
                             -value=>"$groupid"),
                "$groupid:"
             ),
             $::cgi->td({-valign=>"TOP", -colspan=>"4"},
                $::cgi->textfield(-name=>"${groupid}-groupname",
                                -size=>"20",
                                -value=>"$groupname"),
                $::cgi->hidden(-name=>"${groupid}-groupflag",
                                -value=>"$groupflag"),
             )
          );

    print "<TR>\n";
    foreach my $secline (@flags) {
	(my $field, my $secflag) = split(":", $secline);
	my $fieldname = $groupid . "-" . $field;

	# Alas, CGI.pm's checkbox support can't duplicate this
	print "<TD ALIGN=\"RIGHT\">$field:<INPUT TYPE=\"CHECKBOX\" " .
              "NAME=\"$fieldname\" VALUE=\"$secflag\"";
        print "CHECKED" if (int($secflag) & int($groupflag));
        print "></TD>";

	if ($counter > 4) {
	    print "</TR>\n<TR>\n";
	    $counter = 0;
	}
        $counter++;
    }
    $counter = 1;
    print $rowbreak;
}

print "</TABLE>\n" .
      $::cgi->submit(-name=>"submit", -value=>"Submit changes") .
      $::cgi->endform .
      $::cgi->p;
      $::cgi->a({-href=>"query.cgi"}, 
              "Skip all this, and go back to the query page.");
