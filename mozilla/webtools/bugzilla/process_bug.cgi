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

use diagnostics;
use strict;
use CGI;

$::cgi = new CGI;

require "CGI.pl";
require "security.pl";

# Shut up misguided -w warnings about "used only once":

use vars %::versions,
         %::components;

confirm_login();

GetVersionTable();

if ($::cgi->param('product') ne $::dontchange) {
    my $prod = $::cgi->param('product');
    my $vok = lsearch($::versions{$prod}, $::cgi->param('version')) >= 0;
    my $cok = lsearch($::components{$prod}, $::cgi->param('component')) >= 0;
    if (!$vok || !$cok) {
	print 
           $::cgi->h1("Changing product means changing version and component."),
           "You have chosen a new product, and now the version and/or\n",
           "component fields are not correct.  (Or, possibly, the bug did\n",
           "not have a valid component or version field in the first place.)\n",
           "Anyway, please set the version and component now.<p>\n",
           $::cgi->start_form,
           $::cgi->table(
	      $::cgi->TR(
	         $::cgi->td({-align=>"RIGHT"}, $::cgi->b('Product:')),
	         $::cgi->td($prod)
	      ),
	      $::cgi->TR(
	         $::cgi->td({-align=>"RIGHT"}, $::cgi->b('Version:')),
	         $::cgi->td(Version_element($::cgi->param('version'), $prod))
	      ),
	      $::cgi->TR(
	         $::cgi->td({-align=>"RIGHT"}, $::cgi->b('Component:')),
	         $::cgi->td(
		    Component_element($::cgi->param('component'), $prod))
	      )
           );

        foreach my $i ($::cgi->param) {
            if ($i ne 'version' && $i ne 'component') {
	        print $::cgi->hidden(-name=>"$i", 
		                     -value=>$::cgi->param($i),
				     -override=>"1") . "\n";
            }
        }
        print
	   $::cgi->submit(-name=>"submit", -value=>"Commit"),
	   $::cgi->end_form,
	   $::cgi->hr,
	   $::cgi->a({-href=>"query.cgi"},
	      "Cancel all this and go back to the query page.");
        exit;
    }
}
my @idlist;
if ($::cgi->param('id')) {
    push @idlist, $::cgi->param('id');
} else {
    foreach my $i ($::cgi->param) {
        if ($i =~ /^id_/) {
            push @idlist, substr($i, 3);
        }
    }
}

my $who = $::cgi->cookie('Bugzilla_login');
$::bug_id = $::cgi->param('id');

$::query = "update bugs\nset";
$::comma = "";
umask(0);

sub DoComma {
    $::query .= "$::comma\n    ";
    $::comma = ",";
}

sub ChangeStatus {
    my ($str) = (@_);
    if ($str ne $::dontchange && 
	CanIEdit("bug_status", $who, $::cgi->param('id'))) {
        DoComma();
        $::query .= "bug_status = '$str'";
    }
}

sub ChangeResolution {
    my ($str) = (@_);
    if ($str ne $::dontchange &&
	CanIEdit("resolution", $who, $::cgi->param('id'))) {
        DoComma();
        $::query .= "resolution = '$str'";
    }
}

ConnectToDatabase();

foreach my $field ("rep_platform", "priority", "bug_severity", "url",
                   "summary", "component", "bug_file_loc", "short_desc",
                   "product", "version", "component", "class") {
    if ($::cgi->param($field) && $::cgi->param($field) ne $::dontchange
	&& CanIEdit($field, $who, $::cgi->param('id'))) {
            DoComma();
            $::query .= "$field = " . SqlQuote($::cgi->param($field));
    }
}

SWITCH: for ($::cgi->param('knob')) {
    /^none$/ && do {
        last SWITCH;
    };
    /^accept$/ && do {
        ChangeStatus('ASSIGNED');
        last SWITCH;
    };
    /^clearresolution$/ && do {
        ChangeResolution('');
        last SWITCH;
    };
    /^resolve$/ && do {
        ChangeStatus('RESOLVED');
        ChangeResolution($::cgi->param('resolution'));
        last SWITCH;
    };
    /^reassign$/ && do {
        ChangeStatus('ASSIGNED');
        DoComma();
        my $newid = DBNameToIdAndCheck($::cgi->param('assigned_to'));
        $::query .= "assigned_to = $newid";
        last SWITCH;
    };
    /^reassignbycomponent$/ && do {
        if ($::cgi->param('component') eq $::dontchange) {
            print "You must specify a component whose owner should get\n";
            print "assigned these bugs.\n";
            exit 0;
        }
        ChangeStatus('ASSIGNED');
        SendSQL("select initialowner from components where program=" .
                SqlQuote($::cgi->param('product')) . " and value=" .
                SqlQuote($::cgi->param('component')));
        my $newname = FetchOneColumn();
        my $newid = DBNameToIdAndCheck($newname, 1);
        DoComma();
        $::query .= "assigned_to = $newid";
        last SWITCH;
    };   
    /^reopen$/ && do {
        ChangeStatus('REOPENED');
        ChangeResolution('');
        last SWITCH;
    };
    /^verify$/ && do {
        ChangeStatus('VERIFIED');
        last SWITCH;
    };
    /^close$/ && do {
        ChangeStatus('CLOSED');
        last SWITCH;
    };
    /^duplicate$/ && do {
        ChangeStatus('RESOLVED');
        ChangeResolution('DUPLICATE');
        my $num = trim($::cgi->param('dup_id'));
        if ($num !~ /^[0-9]*$/) {
            print "You must specify a bug number of which this bug is a\n";
            print "duplicate.  The bug has not been changed.\n";
            exit;
        }
        if ($num == $::cgi->param('id')) {
	    PutHeader("Nice try.");
            print "But it doesn't really make sense to mark a\n";
            print "bug as a duplicate of itself, does it?\n";
            exit;
        }
        AppendComment($num, $who, 
             "*** Bug $num has been marked as a duplicate of this bug. ***\n" .
             GetLongDescription($::bug_id));
        $::cgi->param(-name=>'comment', 
              -value=>$::cgi->param('comment') . 
              "\n\n*** This bug has been marked as a duplicate of $num ***",
              -override=>"1");
	# FIXME: copy cc list from dup to orig
        system("./processmail $num < /dev/null > /dev/null 2> /dev/null &");
        last SWITCH;
    };
    /^newsource$/ && do {
	my $source_query;
	if (($::cgi->param('source')) && 
            ($::cgi->param('source') ne $::dontchange) && 
            (CanIEdit('source', $who, $::cgi->param('id')))) {
		$source_query = "insert into sources (bug_id, source) ".
			" values ('" . $::cgi->param('id') . "', '" .
			($::cgi->param('source')) ."')";
                SendSQL($source_query);
	}
	last SWITCH;
    };
    # default
    print "Unknown action $::cgi->param('knob')!\n";
    exit;
}

if ($#idlist < 0) {
    PutHeader("Nothing to modify");
    print "You apparently didn't choose any bugs to modify.\n",
          $::cgi->p,
          "Click $::cgi->b(Back) and try again\n";
    exit;
}

if ($::comma eq "" && 
    ($::cgi->param('comment') eq "" || 
     $::cgi->param('comment') =~ /^\s*$/)) {
        print "You apparently did not change anything on the selected bugs.\n",
              "$::cgi->p Click $::cgi->b(Back) and try again.\n";
        exit;
}

my $basequery = $::query;

sub SnapShotBug {
    my ($id) = (@_);
    SendSQL("select " . join(',', @::log_columns) .
            " from bugs where bug_id = '" . $id . "'");
    return FetchSQLData();
}


foreach my $id (@idlist) {
    SendSQL("lock tables bugs write, bugs_activity write, cc write, profiles write, groups write");
    my @oldvalues = SnapShotBug($id);

    my $query = "$basequery\nwhere bug_id = '" . $id . "'";
    
#print $::cgi->pre($query) . "\n";

    if ($::comma ne "") {
        SendSQL($query);
    }

    if ($::cgi->param('comment') ne "") {
	my $pass_comment = $::cgi->param('comment');
        AppendComment($id, $who, $pass_comment);
    }
    
    if ($::cgi->param('cc') && ShowCcList($id) ne $::cgi->param('cc')) {
        my %ccids;
        foreach my $person (split(/[ ,]/, $::cgi->param('cc'))) {
            if ($person ne "") {
                my $cid = DBNameToIdAndCheck($person, 1);
                $ccids{$cid} = 1;
            }
        }
        
        SendSQL("delete from cc where bug_id = '" . $id . "'");
        foreach my $ccid (keys %ccids) {
            SendSQL("insert into cc (bug_id, who) values ($id, $ccid)");
        }
    }

    my @newvalues = SnapShotBug($id);
    my $whoid;
    my $timestamp;
    foreach my $col (@::log_columns) {
        my $old = shift @oldvalues;
        my $new = shift @newvalues;
        if ($old ne $new) {
            if (!defined $whoid) {
                $whoid = DBNameToIdAndCheck($who);
                $query = "select delta_ts from bugs where bug_id = '" .
                          $id . "'";
                SendSQL($query);
                $timestamp = FetchOneColumn();
            }
            if ($col eq 'assigned_to') {
                $old = DBID_to_name($old);
                $new = DBID_to_name($new);
            }
            $col = SqlQuote($col);
            $old = SqlQuote($old);
            $new = SqlQuote($new);
            my $q = "insert into bugs_activity (bug_id,who,when,field,oldvalue,newvalue) values ($id,$whoid,$timestamp,$col,$old,$new)";
            # print "$::cgi->pre($q)";
            SendSQL($q);
        }
    }
    

    PutHeader("Changes submitted for bug " . $::cgi->param('id'), 
	"Changes Submitted", $::cgi->param('id'));
    #if ($::cgi->param('id')) {
    #    navigation_header();
    #}
    #print "$::cgi->hr\n$cgi->p\n",
    #      $::cgi->a({-href=>"show_bug.cgi?id=$id"}, "Back To BUG# $id"),
    #      $::cgi->br,
    #      $::cgi->a({-href=>"enter_bug.cgi"}, "Enter a new bug") . "\n";

    SendSQL("unlock tables");

    system("./processmail $id < /dev/null > /dev/null 2> /dev/null &");
}

#if (defined $::next_bug) {
#    $::cgi->hidden(-name=>'id', -value=>$::next_bug, -override=>"1");
#    print "$::cgi->hr\n";
#
    navigation_header();
    do "bug_form.pl";
#} else {
#    print "<BR><A HREF=\"query.cgi\">Back To Query Page</A>\n";
#    print $::cgi->br . 
#          $::cgi->a({-href->"query.cgi"}, "Back to query page) . "\n";
#}
