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
# Contributor(s): Andrew Anderson <andrew@redhat.com>

use diagnostics;
use strict;
use English;

require "globals.pl";
my $basedir = '/home/httpd/html/bugzilla/';

my $bug_id = shift;
if(!$bug_id) {
	die "No Bug ID supplied";
}

my @message = ();
while(<>) {
	push(@message,$_);
}

my $i = 0;
my $header = "";
while ($message[$i] ne "\n") {
	$header .= $message[$i];
	$i++;
}

my @body = ();
while(defined($message[$i])) {
	push(@body, $message[$i++]);
}

$header =~ s/\n\s+/ /g;
my %head = ('FRONTSTUFF', split /^([-\w]+):/m, $header);

#print "HEADER:\n";
foreach my $item (keys %head) {
	chomp($head{$item});
	#print "\t$item: $head{$item}\n";
}
#print "BODY:\n";
#print "@body\n";

# Here we make the assumption that the last mail received 
# was indeed the message in question -- given the message
# load, this should be a valid assumption.
umask 0777;
opendir MAILDIR, "$basedir/data/maildir/$bug_id" or 
	die "Couldn't open mail directory: $ERRNO";
my @mail = readdir MAILDIR;
closedir MAILDIR;

#print "MAIL: $mail[$#mail]\n";
my $maillink = "data/maildir/$bug_id/$mail[$#mail]";

&ConnectToDatabase;

my $query = "SELECT long_desc FROM bugs WHERE bug_id = '$bug_id'";
#print "QUERY: $query\n";
SendSQL($query);

my $desc = FetchOneColumn();
my $now = time2str("%D %H:%M", time());
$desc .= "\n\n------- Email From $head{'From'} $now -------\n";
$desc .= "Attached to Bug # $bug_id.\n";

$query = "UPDATE bugs SET long_desc = " . SqlQuote($desc) .
         " WHERE bug_id = '$bug_id'";
#print "UPDATE: $query\n";
SendSQL($query);

chdir($basedir);
system("$basedir/processmail", "$bug_id");
