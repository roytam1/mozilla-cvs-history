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

#
# Here's where we automagically determine a user's permission to 
# edit tickets 
#
# We try to be extra paranoid here, defaulting to denying access
# if we fail for any reason
#

sub CanIEdit {
	my $perms = 0; 
	my $flags = 0; 
	my $check = 0; 
	my $status;
	my $query;
	my @row;
	my ($field, $reporter, $bug_id) = @_;

	# 
	# check for privileged users first
	#
	$query =  "select groups.flags from groups, profiles "
		. "where profiles.login_name = '" 
		. $::COOKIE{"Bugzilla_login"} 
		. "' and profiles.groupid = groups.groupid";
	SendSQL($query);
	$perms = int(FetchOneColumn());
	$query =  "select flag from security where field = '"
		. $field . "'";
	SendSQL($query);
	$flags = int(FetchOneColumn());
	$check = $perms & $flags;
	#printf("<PRE>perms: %4x, flags: %4x, check: %4x</PRE>\n",
	#	$perms, $flags, $check);
	if ( $check ) {
		#print "<PRE>perms ok</PRE>\n";
		return 1;
	# 
	# The reporting user can edit their own ticket 
	# until it's not "NEW" anymore, but they cannot
	# change the state of the ticket.
	#
	} elsif (defined($reporter) 
		&& ($::COOKIE{"Bugzilla_login"} eq $reporter) 
		&& ($field ne "bug_status")) {
		$query =  "select bug_status from bugs "
			. "where bug_id = '" . $bug_id . "'";
		SendSQL($query);
		if (@row = FetchSQLData()) {
			$status = shift @row;
			if (!defined $status) {
	    			return 0;
			}
		}
		if ( $status eq "NEW" ) {
			#print "<PRE>status ok</PRE>\n";
			return 1;
		} else {
			#print "<PRE>status not ok</PRE>\n";
			return 0;
		}
	#
	# Everyone else can bugger off
	#
	} 
	#print "<PRE>bugger off</PRE>\n";
	return 0;
}

sub CanIView {
	my $perms = 0; 
	my $flags = 0; 
	my $check = 0; 
	my $query = "";
	my $field = shift;
	my @row;


	$query =  "select groups.flags,security.flag "
		. "from groups, profiles, security "
		. "where profiles.login_name = '" 
		. $::COOKIE{"Bugzilla_login"}
		. "' and profiles.groupid = groups.groupid "
		. "and security.field = '"
		. $field . "'";

	SendSQL($query);
	if (@row = FetchSQLData()) {
		($perms, $flags) = @row;
		$perms = int $perms;
		$flags = int $flags;
	}
	$check = $perms & $flags;
	if ( $check ) {
		return 1;
	} else {
		return 0;
	}
}
1;
