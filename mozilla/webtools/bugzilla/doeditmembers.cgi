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
# Contributor(s): Sam Ziegler <sam@ziegler.org>
#				  Terry Weissman <terry@mozilla.org>
#				  David Lawrence <dkl@redhat.com>

use diagnostics;
use strict;

require "CGI.pl";
require "globals.pl";

confirm_login();

print "Content-type: text/html\n\n";

if (!UserInGroup("editcomponents")) {
    print "<H1>Sorry, you aren't a member of the 'editcomponents' group.</H1>\n";
    print "And so, you aren't allowed to edit member data.\n";
    exit;
}

PutHeader("Saving new \nmember data");

foreach my $key (keys(%::FORM)) {
    $::FORM{url_decode($key)} = $::FORM{$key};
}

foreach my $i (keys %::FORM) {
	my $userid = "";
	my $query = "";
	if ($i =~ /^login_name/) {
		$userid = $i;
		$userid =~ s/^.*-//;
		$query = "update profiles set login_name = " . SqlQuote($::FORM{$i}) . 
				 " where userid = $userid";
		print "Changing login_name for userid $userid to $::FORM{$i}.<BR>\n";
		SendSQL($query);
	}
	if ($i =~ /^realname/) {
		$userid = $i;
		$userid  =~ s/^.*-//;
        $query = "update profiles set realname = " . SqlQuote($::FORM{$i}) . 
                 " where userid = $userid";
		print "Changing realname for userid $userid to $::FORM{$i}.<BR>\n";
        SendSQL($query);
	}
	if ($i =~ /^groupset/ ) {
		$userid = $i;
		$userid =~ s/^.*-//;
        $query = "update profiles set groupset = " . SqlQuote($::FORM{$i}) . 
                 " where userid = $userid";
		print "Changing groupset for userid $userid to $::FORM{$i}.<BR>\n";
        SendSQL($query);
	}
}

print "OK, done.<p>\n";
print "<a href=editmembers.cgi>Edit the members some more.</a><p>\n";

PutFooter();
