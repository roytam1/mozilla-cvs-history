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

use diagnostics;
use strict;

require "CGI.pl";

# Shut up misguided -w warnings about "used only once":
use vars %::COOKIE;


confirm_login();

print "Content-type: text/html\n\n";

if (Param("maintainer") ne $::COOKIE{'Bugzilla_login'}) {
    print "<H1>Sorry, you aren't the maintainer of this system.</H1>\n";
    print "And so, you aren't allowed to edit the parameters of it.\n";
    exit;
}


PutHeader("Saving groups");

SendSQL("select groupid, groupname, flags from groups order by groupid asc");

my @line;
my @groups;
my @flags;
my @fields;

while (@line = FetchSQLData()) {
	push(@groups, join(":", @line));
}

SendSQL("select field, flag from security");

while (@line = FetchSQLData()) {
        push(@flags, join(":", @line));
	push(@fields, $line[0]);
}

foreach my $key (sort keys(%::FORM)) {
    $::FORM{url_decode($key)} = $::FORM{$key};
    #print "$key = $::FORM{$key}<BR>\n";
}

my @updates;
my $curIndex = 0;
my $newflags = 0;
my $doupdate = 0;

foreach my $group (@groups) {
    $newflags = $doupdate = 0;
    (my $groupid, my $groupname, my $flags) = split(":", $group);
    my $curItem = "$groupid" . "-groupid";
    if (exists $::FORM{$curItem}) {
	foreach my $field (@fields) {
	    $curItem = $groupid . "-" . $field;
	    if (exists $::FORM{$curItem}) {
	        #print "$curItem: $::FORM{$curItem}<BR>";
		$newflags += $::FORM{$curItem};
	    }
	}
        $updates[$curIndex] = "update groups set ";
	my $curItem = $groupid . "-groupname";
	if (int($flags) != int($newflags)) {
	    $updates[$curIndex] .= "flags = '$newflags' ";
	    $doupdate = 1;
	} elsif ($::FORM{$curItem} ne $groupname) {
	    $updates[$curIndex] .= "groupname = '$::FORM{$curItem}' ";
	    $doupdate = 1;
	} elsif ((int($flags) != ($newflags)) && ($::FORM{$curItem} ne $groupname)) {
	    $updates[$curIndex] .= "flags = '$newflags', groupname = '$::FORM{$curItem}";
	    $doupdate = 1;
	}
	$updates[$curIndex] .= "where groupid = '$groupid'";

	if($doupdate) { 
	    #print "adding $updates[$curIndex]<BR>\n";
	    $curIndex++;
	} else {
	    #print "deleting $updates[$curIndex]<BR>\n";
	    undef $updates[$curIndex];
	}
    }
}
if($#updates) {
    foreach my $update (@updates) {
        #print "<PRE>$update</PRE>\n";
        SendSQL($update);
    }
    print "<P>OK, done.<p>\n";
} else {
    print "<P>Nothing to update.<p>\n";
}

print "<a href=\"editgroups.cgi\">Edit the groups some more.</a><p>\n";
print "<a href=\"query.cgi\">Go back to the query page.</a>\n";
