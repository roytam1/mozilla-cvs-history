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
#
# Direct any questions on this source code to
#
# Holger Schurig <holgerschurig@nikocity.de>

use diagnostics;
use strict;

require "CGI.pl";
require "globals.pl";

# TestGroup:  just returns if the specified group does exists
# CheckGroup: same check, optionally  emit an error text

sub TestGroup {
    my $group = shift;

    # does the group exist?
    SendSQL("SELECT name  
             FROM groups 
             WHERE name = " . SqlQuote($group));
    return FetchOneColumn();
}

sub CheckGroup {
    my $group = shift;

    # do we have a group?
    unless ($group) {
        print "Sorry, you haven't specified a group.";
        PutTrailer();
        exit;
    }

    unless (TestGroup($group)) {
        print "Sorry, group '$group' does not exist.";
        PutTrailer();
        exit;
    }
}


#
# Displays the form to edit a groups parameters
#
sub EmitFormElements {
    my ($group, $description, $isbuggroup, $iscontract) = @_;
	$isbuggroup = $isbuggroup ? " CHECKED " : "";
	$iscontract = $iscontract ? " CHECKED " : "";

	print qq{
	<TH ALIGN="right">Group</TH>
	<TD><INPUT SIZE=64 MAXLENGTH=64 NAME="group" VALUE="$group"></TD>
<TR><TR>
	<TH ALIGN="right">Description</TH>
	<TD><TEXTAREA ROWS=4 COLS=64 WRAP=VIRTUAL NAME="description">$description</TEXTAREA></TD>
<TR><TR>
	<TH ALIGN="right">Is Bug Group</TH>
	<TD><INPUT TYPE=CHECKBOX NAME="isbuggroup" VALUE="1" $isbuggroup></TD>
</TR><TR>
	<TH ALIGN=right>Is Contract Group</TH>
	<TD><INPUT TYPE=CHECKBOX NAME="iscontract" VALUE="1" $iscontract></TD>
};

}


#
# Displays a text like "a.", "a or b.", "a, b or c.", "a, b, c or d."
#
sub PutTrailer {
    my (@links) = ("Back to the <A HREF=\"query.cgi\">query page</A>", @_);

    my $count = $#links;
    my $num = 0;
    print "<P><CENTER>\n";
    foreach (@links) {
        print $_;
        if ($num == $count) {
            print ".\n";
        }
        elsif ($num == $count-1) {
            print " or ";
        }
        else {
            print ", ";
        }
        $num++;
    }
	print "</CENTER>\n";
}


#
# Preliminary checks:
#
confirm_login();

print "Content-type: text/html\n\n";

unless (UserInGroup("creategroups")) {
    PutHeader("Not allowed");
    print "Sorry, you aren't a member of the 'creategroups' group.\n";
    print "And so, you aren't allowed to add, modify or delete groups.\n";
    PutTrailer();
    exit;
}


#
# often used variables
#
my $group = trim($::FORM{'group'} || '');
my $action  = trim($::FORM{'action'}  || '');
my $localtrailer = "<A HREF=\"editgroups.cgi\">edit</A> more groups";


#
# action='' -> Show nice list of groups 
#
unless ($action) {
    PutHeader("Select Group");

    SendSQL("SELECT groupid, name, description, isbuggroup, contract
             FROM groups
             ORDER BY groupid");
    print "<P>\n<TABLE ALIGN=center BORDER=1 CELLPADDING=4 CELLSPACING=0><TR BGCOLOR=\"#BFBFBF\">\n";
    print "  <TH ALIGN=\"left\">Name</TH>\n";
    print "  <TH ALIGN=\"left\">Description</TH>\n";
    print "  <TH ALIGN=\"left\">Bug Group</TH>\n";
	print "  <TH ALIGN=\"left\">Contract Group</TH>\n";
    print "  <TH ALIGN=\"left\">Action</TH>\n";
    print "</TR>";
    while ( MoreSQLData() ) {
        my ($groupid, $name, $description, $isbuggroup, $iscontract) = FetchSQLData();
        $description ||= "<FONT COLOR=\"red\">missing</FONT>";
        $isbuggroup = $isbuggroup ? '<FONT COLOR=green>Yes</FONT>' : '<FONT COLOR=red>No</FONT>';
		$iscontract = $iscontract ? '<FONT COLOR=green>Yes</FONT>' : '<FONT COLOR=red>No</FONT>';

        print "<TR BGCOLOR=\"#ECECEC\">\n";
        print "  <TD ALIGN=\"left\"><A HREF=\"editgroups.cgi?action=edit&group=", url_quote($name), "\"><B>$name</B></A></TD>\n";
        print "  <TD ALIGN=\"left\">$description</TD>\n";
        print "  <TD ALIGN=\"left\">$isbuggroup</TD>\n";
		print "  <TD ALIGN=\"left\">$iscontract</TD>\n";
        print "  <TD ALIGN=\"left\"><A HREF=\"editgroups.cgi?action=del&group=", url_quote($name), "\">Delete</A></TD>\n";
        print "</TR>";
    }
    print "<TR BGCOLOR=\"#ECECEC\">\n";
    print "  <TH COLSPAN=5><A HREF=\"editgroups.cgi?action=add\">Add a new group</A></TH>\n";
    print "</TR></TABLE>\n";

    PutTrailer();
    PutFooter();
	exit;
}


#
# action='add' -> present form for parameters for new group 
#
# (next action will be 'new')
#
if ($action eq 'add') {
    PutHeader("Add Group");

    #print "This page lets you add a new group to bugzilla.\n";

    print "<FORM METHOD=POST ACTION=editgroups.cgi>\n";
    print "<TABLE ALIGN=center BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements('', '', '', 0);

    print "</TR>\n</TABLE>\n";
    print "<CENTER><INPUT TYPE=SUBMIT VALUE=\"Add\"></CENTER>\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"new\">\n";
    print "</FORM>";

    my $other = $localtrailer;
    $other =~ s/more/other/;
    PutTrailer($other);
	PutFooter();
    exit;
}


#
# action='new' -> add group entered in the 'action=add' screen
#
if ($action eq 'new') {
    PutHeader("Adding New Group");

    # Cleanups and valididy checks

    unless ($group) {
        print "You must enter a name for the new group. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }
    if (TestGroup($group)) {
        print "The group '$group' already exists. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }

    my $description  = trim($::FORM{'description'}  || '');
    my $isbuggroup = 0;
    $isbuggroup = 1 if $::FORM{'isbuggroup'};
	my $iscontract = 0;
    $iscontract = 1 if $::FORM{'iscontract'};

    # Add the new group.
	if ($::driver eq 'mysql') {
    	SendSQL("INSERT INTO groups ( " .
          		"name, description, isbuggroup, contract" .
          		" ) VALUES ( " .
          		SqlQuote($group) . "," .
          		SqlQuote($description) . "," .
          		SqlQuote($isbuggroup) . ", " . 
				SqlQuote($iscontract) . ")");
	} else {
		SendSQL("INSERT INTO groups ( " .
          		"groupid, name, description, isbuggroup, contract" .
          		" ) VALUES ( groups_seq.nextval, " .
          		SqlQuote($group) . "," .
          		SqlQuote($description) . "," .
          		SqlQuote($isbuggroup) . ", " .
				SqlQuote($iscontract) . ")");
	}

    print "OK, done.<p>\n";
    PutTrailer($localtrailer);
    exit;
}


#
# action='del' -> ask if user really wants to delete
#
# (next action would be 'delete')
#
if ($action eq 'del') {
    PutHeader("Delete Group");
    CheckGroup($group);

    # display some data about the group
    SendSQL("SELECT groupid, name, description, isbuggroup, contract
             FROM groups 
             WHERE name = " . SqlQuote($group));
    my ($groupid, $name, $description, $isbuggroup, $iscontract) = FetchSQLData();
    $description ||= "<FONT COLOR=\"red\">description missing</FONT>";
    $isbuggroup = $isbuggroup ? '<FONT COLOR=green>Yes</FONT>' : '<FONT COLOR=red>No</FONT>';
	$iscontract = $iscontract ? '<FONT COLOR=green>Yes</FONT>' : '<FONT COLOR=red>No</FONT>';
   
	# Find out how many people are in this group
	SendSQL("select count(userid) from user_group where groupid = $groupid");
    my $members = FetchOneColumn();
	$members = $members ? $members : 0;
	 
    print "<P>\n<TABLE ALIGN=center BORDER=1 CELLPADDING=4 CELLSPACING=0 BGCOLOR=\"#ECECEC\">\n";
    print "<TR BGCOLOR=\"#BFBFBF\">\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Part</TH>\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Value</TH>\n";
    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Group</TD>\n";
    print "  <TD VALIGN=\"top\">$name</TD>\n";
    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Description</TD>\n";
    print "  <TD VALIGN=\"top\">$description</TD>\n";
    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Is Bug Group</TD>\n";
    print "  <TD VALIGN=\"top\">$isbuggroup</TD>\n";
	print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Is Contract Group</TD>\n";
    print "  <TD VALIGN=\"top\">$iscontract</TD>\n";
    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Members</TD>\n";
    print "  <TD VALIGN=\"top\">$members</TD>\n";
    print "</TD>\n</TR></TABLE>";
	
	print "<P>\n<CENTER><B>Current members will be removed from the group.</B>\n";

    print "<H2>Confirmation</H2>\n";
    print "<P>Do you really want to delete this group?<P>\n";
    print "<FORM METHOD=POST ACTION=editgroups.cgi>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Yes, delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"group\" VALUE=\"$group\">\n";
    print "</FORM>\n</CENTER>\n";

    PutTrailer($localtrailer);
	PutFooter();
    exit;
}


#
# action='delete' -> really delete the group
#
if ($action eq 'delete') {
    PutHeader("Deleting Group");
    CheckGroup($group);

	SendSQL("select groupid from groups where name = " . SqlQuote($group));
	my $groupid = FetchOneColumn();

    SendSQL("DELETE FROM groups 
             WHERE name = " . SqlQuote($group));
    print "Group deleted.<BR>\n";

	SendSQL("delete from user_group where groupid = $groupid");
	print "Members removed.<BR>\n";

    PutTrailer($localtrailer);
    PutFooter();
	exit;
}



#
# action='edit' -> present the edit group 
#
# (next action would be 'update')
#
if ($action eq 'edit') {
    PutHeader("Edit Group");
    CheckGroup($group);

    # get data of group 
    SendSQL("SELECT groupid, name, description, isbuggroup, contract
             FROM groups 
             WHERE name = " . SqlQuote($group));
    my ($groupid, $name, $description, $isbuggroup, $iscontract) = FetchSQLData();

    print "<FORM METHOD=POST ACTION=editgroups.cgi>\n";
    print "<TABLE ALIGN=center BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements($group, $description, $isbuggroup, $iscontract);
    
    print "</TR>\n";
    print "</TABLE>\n";

    print "<INPUT TYPE=HIDDEN NAME=\"nameold\" VALUE=\"$group\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"descriptionold\" VALUE=\"$description\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"isbuggroupold\" VALUE=\"$isbuggroup\">\n";
	print "<INPUT TYPE=HIDDEN NAME=\"iscontractold\" VALUE=\"$iscontract\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"update\">\n";
    print "<CENTER><INPUT TYPE=SUBMIT VALUE=\"Update\"></CENTER>\n";

    print "</FORM>";

    my $x = $localtrailer;
    $x =~ s/more/other/;
    PutTrailer($x);
	PutFooter();
    exit;
}


#
# action='update' -> update the group 
#
if ($action eq 'update') {
    PutHeader("Update Group");

    my $nameold 		= trim($::FORM{'nameold'}      		|| '');
    my $description     = trim($::FORM{'description'}     	|| '');
    my $descriptionold  = trim($::FORM{'descriptionold'}  	|| '');
    my $isbuggroup 		= trim($::FORM{'isbuggroup'}     	|| '');
    my $isbuggroupold	= trim($::FORM{'isbuggroupold'}  	|| '');
	my $iscontract		= trim($::FORM{'iscontract'}        || '');
    my $iscontractold	= trim($::FORM{'iscontractold'}     || '');

    CheckGroup($nameold);

    if ($isbuggroup != $isbuggroupold) {
        $isbuggroup ||= 0;
        SendSQL("UPDATE groups 
                 SET isbuggroup = $isbuggroup
                 WHERE name = " . SqlQuote($nameold));
        print "Updated bug group status.<BR>\n";
    }

	if ($iscontract != $iscontractold) {
        $iscontract ||= 0;
        SendSQL("UPDATE groups 
                 SET contract = $iscontract
                 WHERE name = " . SqlQuote($nameold));
        print "Updated contract group status.<BR>\n";
    }

    if ($description ne $descriptionold) {
        unless ($description) {
            print "Sorry, I can't delete the description.";
            PutTrailer($localtrailer);
            exit;
        }
        SendSQL("UPDATE groups 
                 SET description=" . SqlQuote($description) . "
                 WHERE name =" . SqlQuote($nameold));
        print "Updated description.<BR>\n";
    }

    if ($group ne $nameold) {
        unless ($group) {
            print "Sorry, I can't delete the group name.";
            PutTrailer($localtrailer);
            exit;
        }
        if (TestGroup($group)) {
            print "Sorry, group name '$group' is already in use.";
            PutTrailer($localtrailer);
            exit;
        }

        SendSQL("UPDATE groups 
                 SET name = " . SqlQuote($group) . "
                 WHERE name = " . SqlQuote($nameold));
        print "Updated group name.<BR>\n";
    }

    PutTrailer($localtrailer);
    exit;
}



#
# No valid action found
#
PutHeader("Error");
print "I don't have a clue what you want.<BR>\n";

foreach ( sort keys %::FORM) {
    print "$_: $::FORM{$_}<BR>\n";
}
