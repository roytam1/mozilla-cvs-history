#!/usr/bonsaitools/bin/perl -w
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

use diagnostics;
use strict;

require "CGI.pl";

# Shut up misguided -w warnings about "used only once":

use vars %::FORM;


my $id = $::FORM{'id'};
my $linkedid = qq{<a href="show_bug.cgi?id=$id">$id</a>};

print "Content-type: text/html\n\n";
PutHeader("Dependency tree", "Dependency tree", "Bug $linkedid");

ConnectToDatabase();

quietly_check_login();

if ($::driver ne 'mysql') {
	my $userid = 0;
	if (defined($::COOKIE{'Bugzilla_login'})) {
		$userid = DBname_to_id($::COOKIE{'Bugzilla_login'});
	}
	if (!CanISee($id, $userid)) {
		PutHeader("Not Allowed");
		PutError("You do not have permission to access this bug.");
	}
}

$::usergroupset = $::usergroupset; # More warning suppression silliness.

my %seen;

sub DumpKids {
    my ($i, $me, $target) = (@_);
    if (exists $seen{$i}) {
        return;
    }
    $seen{$i} = 1;
    SendSQL("select $target from dependencies where $me = $i order by $target");
    my @list;
    while (MoreSQLData()) {
        push(@list, FetchOneColumn());
    }
    if (@list) {
        print "<ul>\n";
        foreach my $kid (@list) {
            my ($bugid, $stat, $milestone) = ("", "", "");
            my ($userid, $short_desc) = ("", "");
            if (Param('usetargetmilestone')) {
				if ($::driver eq 'mysql') {
                	SendSQL("select bug_id, bug_status, target_milestone, assigned_to, " .
							"short_desc from bugs where bug_id = $kid and " .
							"bugs.groupset & $::usergroupset = bugs.groupset");
				} else {
                   SendSQL("select bug_id, bug_status, target_milestone, assigned_to, " .
                            "short_desc from bugs where bug_id = $kid"); 
				}
                ($bugid, $stat, $milestone, $userid, $short_desc) = (FetchSQLData());
            } else {
				if ($::driver eq 'mysql') {
                	SendSQL("select bug_id, bug_status, assigned_to, short_desc " .
							"from bugs where bug_id = $kid and " .
							"bugs.groupset & $::usergroupset = bugs.groupset");
				} else {
                    SendSQL("select bug_id, bug_status, assigned_to, short_desc " .
                            "from bugs where bug_id = $kid");
				}
                ($bugid, $stat, $userid, $short_desc) = (FetchSQLData());

            }
            if (!defined $bugid) {
                next;
            }
            my $opened = ($stat eq "NEW" || $stat eq "ASSIGNED" ||
                          $stat eq "REOPENED");
            print "<LI>";
            if (!$opened) {
                print "<STRIKE>";
            }
            $short_desc = html_quote($short_desc);
            SendSQL("select login_name from profiles where userid = $userid");
            my ($owner) = (FetchSQLData());
            if ( (Param('usetargetmilestone')) && ($milestone) ) {
                print qq{<A HREF="show_bug.cgi?id=$kid">$kid [$milestone, $owner] - $short_desc</A>};
            } else {
                print qq{<A HREF="show_bug.cgi?id=$kid">$kid [$owner] - $short_desc</A>};
            }
            if (!$opened) {
                print "</STRIKE>";
            }
            DumpKids($kid, $me, $target);
        }
        print "</UL>\n";
    }
}

print "<TABLE WIDTH=700 ALIGN=center>\n<TR>\n<TD ALIGN=left>\n";
print "<H2>Bugs that bug $linkedid depends on</H2>";
DumpKids($id, "blocked", "dependson");
print "<H2>Bugs that depend on bug $linkedid</H2>";
undef %seen;
DumpKids($id, "dependson", "blocked");
print "</TD>\n</TR>\n</TABLE>\n";

PutFooter();
