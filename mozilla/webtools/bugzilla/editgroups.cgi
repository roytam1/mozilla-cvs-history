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

require "CGI.pl";

# Shut up misguided -w warnings about "used only once":
use vars %::COOKIE;

confirm_login();

print "Content-type: text/html\n\n";

if (Param("maintainer") ne $::COOKIE{Bugzilla_login}) {
    PutHeader("Sorry, you aren't the maintainer of this system.\n");
    print "And so, you aren't allowed to edit the parameters of it.\n";
    exit;
}

my @line;
my $group;
my $counter = 1;
my $rowbreak = "</TR><TR><TD COLSPAN=\"5\"><HR></TD></TR>";

PutHeader("Edit Group Permissions");

print "This lets you edit the group permissions of bugzilla.\n";

print "<FORM METHOD=\"POST\" ACTION=\"doeditgroups.cgi\">\n<TABLE>\n";

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
    print "
<TH>
    <TD ALIGN=\"LEFT\" VALIGN=\"TOP\" COLSPAN=\"2\">\n";
    my $fieldname = $groupid . "-groupid";
    print "
	<INPUT TYPE=\"HIDDEN\" NAME=\"$fieldname\" VALUE=\"$groupid\">$groupid:
    </TD>
    <TD VALIGN=\"TOP\" VALIGN\"TOP\" COLSPAN=\"4\">\n";
    $fieldname = $groupid . "-groupname";
    print "
	<INPUT SIZE=\"20\" NAME=\"$fieldname\" VALUE=\"$groupname\">\n";
    $fieldname = $groupid . "-groupflag";
    print "
	<INPUT TYPE=\"HIDDEN\" NAME=\"$fieldname\" VALUE=\"$groupflag\">
    </TD>
</TH>
";
    print "<TR>\n";
    foreach my $secline (@flags) {
	(my $field, my $secflag) = split(":", $secline);
	$fieldname = $groupid . "-" . $field;
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

print "
</TABLE>
<INPUT TYPE=\"submit\" VALUE=\"Submit changes\">
</FORM>
<P>
<A HREF=\"query.cgi\">Skip all this, and go back to the query page</A>
";
