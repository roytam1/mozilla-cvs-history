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
#                 Dan Mosedale <dmose@mozilla.org>
#                 Joe Robins <jmrobins@tgix.com>

use diagnostics;
use strict;

require "CGI.pl";

# Shut up misguided -w warnings about "used only once".  For some reason,
# "use vars" chokes on me when I try it here.

sub sillyness {
    my $zz;
    $zz = $::buffer;
    $zz = %::COOKIE;
    $zz = %::components;
    $zz = %::versions;
    $zz = @::legal_bug_status;
    $zz = @::legal_opsys;
    $zz = @::legal_platform;
    $zz = @::legal_priority;
    $zz = @::legal_product;
    $zz = @::legal_severity;
    $zz = %::target_milestone;
}

confirm_login();

my $cookiepath = Param('cookiepath');
print "Set-Cookie: PLATFORM=$::FORM{'product'} ; path=$cookiepath ; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
print "Set-Cookie: VERSION-$::FORM{'product'}=$::FORM{'version'} ; path=$cookiepath ; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
print "Content-type: text/html\n\n";

if (defined $::FORM{'maketemplate'}) {
    print "<TITLE>Bookmarks are your friend.</TITLE>\n";
    print "<H1>Template constructed.</H1>\n";
    
    my $url = "enter_bug.cgi?$::buffer";

    print "If you put a bookmark <a href=\"$url\">to this link</a>, it will\n";
    print "bring up the submit-a-new-bug page with the fields initialized\n";
    print "as you've requested.\n";
    PutFooter();
    exit;
}

PutHeader("Posting Bug -- Please wait", "Posting Bug", "One moment please...");

umask 0;
ConnectToDatabase();

my $product = $::FORM{'product'};

if(Param("usebuggroupsentry") && GroupExists($product)) {
  if(!UserInGroup($product)) {
    print "<H1>Permission denied.</H1>\n";
    print "Sorry; you do not have the permissions necessary to enter\n";
    print "a bug against this product.\n";
    print "<P>\n";
    PutFooter();
    exit;
  }
}

if (!defined $::FORM{'component'} || $::FORM{'component'} eq "") {
    PuntTryAgain("You must choose a component that corresponds to this bug. " .
                 "If necessary, just guess.");
}

if (!defined $::FORM{'short_desc'} || trim($::FORM{'short_desc'}) eq "") {
    PuntTryAgain("You must enter a summary for this bug.");
}

my $forceAssignedOK = 0;
if ($::FORM{'assigned_to'} eq "") {
    SendSQL("select initialowner from components where program = " .
            SqlQuote($::FORM{'product'}) .
            " and value = " . SqlQuote($::FORM{'component'}));
    $::FORM{'assigned_to'} = FetchOneColumn();
    $forceAssignedOK = 1;
}

$::FORM{'assigned_to'} = DBNameToIdAndCheck($::FORM{'assigned_to'}, $forceAssignedOK);
$::FORM{'reporter'} = DBNameToIdAndCheck($::FORM{'reporter'});


my @bug_fields = ("reporter", "product", "version", "rep_platform",
                  "bug_severity", "priority", "op_sys", "assigned_to",
                  "bug_status", "bug_file_loc", "short_desc", "component",
                  "target_milestone");

if (Param("useqacontact")) {
    SendSQL("select initialqacontact from components where program = " .
            SqlQuote($::FORM{'product'}) .
            " and value=" . SqlQuote($::FORM{'component'}));
    my $qacontact = FetchOneColumn();
    if (defined $qacontact && $qacontact ne "") {
        $::FORM{'qa_contact'} = DBNameToIdAndCheck($qacontact, 1);
        push(@bug_fields, "qa_contact");
    }
}

# If we're using bug groups, we need to include the groupset in the list of
# fields.  -JMR, 2/18/00
if(Param("usebuggroups")) {
  push(@bug_fields, "groupset");
}

if (exists $::FORM{'bug_status'}) {
    if (!UserInGroup("canedit") && !UserInGroup("canconfirm")) {
        #delete $::FORM{'bug_status'};
   		$::FORM{'bug_status'} = 'NEW';	
	}
}

if (!exists $::FORM{'bug_status'}) {
    $::FORM{'bug_status'} = $::unconfirmedstate;
    SendSQL("SELECT votestoconfirm FROM products WHERE product = " .
            SqlQuote($::FORM{'product'}));
    if (!FetchOneColumn()) {
        $::FORM{'bug_status'} = "NEW";
    }
}

#if (!exists $::FORM{'target_milestone'}) {
#   SendSQL("SELECT defaultmilestone FROM products " .
#            "WHERE product = " . SqlQuote($::FORM{'product'}));
#   $::FORM{'target_milestone'} = FetchOneColumn();
#}

if ( Param("strictvaluechecks") ) {
    GetVersionTable();  
    CheckFormField(\%::FORM, 'reporter');
    CheckFormField(\%::FORM, 'product', \@::legal_product);
    CheckFormField(\%::FORM, 'version', \@{$::versions{$::FORM{'product'}}});
#    CheckFormField(\%::FORM, 'target_milestone', \@{$::target_milestone{$::FORM{'product'}}});
    CheckFormField(\%::FORM, 'rep_platform', \@::legal_platform);
    CheckFormField(\%::FORM, 'bug_severity', \@::legal_severity);
    CheckFormField(\%::FORM, 'priority', \@::legal_priority);
    CheckFormField(\%::FORM, 'op_sys', \@::legal_opsys);
    CheckFormFieldDefined(\%::FORM, 'assigned_to');
    CheckFormField(\%::FORM, 'bug_status', \@::legal_bug_status);
    CheckFormFieldDefined(\%::FORM, 'bug_file_loc');
    CheckFormField(\%::FORM, 'component', \@{$::components{$::FORM{'product'}}}); 
    CheckFormFieldDefined(\%::FORM, 'comment');
}

my @used_fields;
foreach my $f (@bug_fields) {
    if (exists $::FORM{$f}) {
        push (@used_fields, $f);
    }
}

#if (exists $::FORM{'bug_status'} && $::FORM{'bug_status'} ne $::unconfirmedstate) {
#    push(@used_fields, "everconfirmed");
#    $::FORM{'everconfirmed'} = 1;
#}

my $query = "";
my $date = "";
if ($::driver eq 'mysql') {
	$date = "now()";
} else {
	$date = "sysdate";
}

if ($::driver eq 'mysql') {
	$query = "INSERT INTO bugs (\n" . join(",\n", @used_fields) . 
			 ", creation_ts, groupset) values (";
} else {
	# Oracle does not use automatically updating timestamps so we have to do
	# the updating by hand. Also we have to grab the next bug_id by hand.
	$query = "INSERT INTO bugs (\nbug_id,\n " . join(",\n", @used_fields) . 
             ", creation_ts, groupset, delta_ts) values (bugs_seq.nextval,\n";
}

foreach my $field (@used_fields) {
    $query .= SqlQuote($::FORM{$field}) . ",\n";
}

my $comment = $::FORM{'comment'};
$comment =~ s/\r\n/\n/g;     # Get rid of windows-style line endings.
$comment =~ s/\r/\n/g;       # Get rid of mac-style line endings.
$comment = trim($comment);

$query .= "$date, 0, $date";

if ($::driver eq 'mysql') {
	foreach my $b (grep(/^bit-\d*$/, keys %::FORM)) {
    	if ($::FORM{$b}) {
        	my $v = substr($b, 4);
        	$query .= " + $v";    	# Carefully written so that the math is
            	                    # done by MySQL, which can handle 64-bit math,
                	                # and not by Perl, which I *think* can not.
    	}
	}
}

$query .= ")\n";


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

if ($::driver eq 'mysql') {
	SendSQL("select LAST_INSERT_ID()");
} else {
	SendSQL("select bugs_seq.currval from dual"); 
}

my $id = FetchOneColumn();

# $date defined earlier depending on what database using
SendSQL("INSERT INTO longdescs (bug_id, who, bug_when, thetext) VALUES " .
    	"($id, $::FORM{'reporter'}, $date, " . SqlQuote($comment) . ")");

foreach my $person (keys %ccids) {
    SendSQL("insert into cc (bug_id, who) values ($id, $person)");
}

# If certains groups were set insert values in bug_group
if ($::driver ne 'mysql') {
	foreach my $b (grep(/^group-.*$/, keys %::FORM)) {
    	if ($::FORM{$b}) {
        	$b =~ s/^group-//;
        	SendSQL("insert into bug_group values ($id, $b)");
    	}
	}
}

# If this bug is set to contract then we set the priority differently
if (Param("contract") && UserInGroup('setcontract')) {
    if (defined ($::FORM{'iscontract'}) && $::FORM{'iscontract'} eq '1') {
        SendSQL("update bugs set priority = " . SqlQuote('contract') .
                " where bug_id = $id");
    }
}

print "<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=3 ALIGN=center><TD><H2>Bug $id posted</H2>\n";
system("./processmail", $id, $::COOKIE{'Bugzilla_login'});
print "<TD><A HREF=\"show_bug.cgi?id=$id\">Back To BUG# $id</A></TABLE>\n";

print "<P><CENTER><A HREF=\"createattachment.cgi?id=$id\">Attach a file to this bug</A></CENTER>\n";

PutFooter();
exit;
