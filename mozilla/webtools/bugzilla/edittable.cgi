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
# The Original Code is mozilla.org code 
#
# This code was based on editversions.cgi code by Holger Schurig 
# <holgerschurig@nikocity.de>
#
# The Initial Developer of the Original code is Alex 
# Schuilenburg. Portions created by Alex Schuilenburg are
# Copyright (C) 2002 Alex Schuilenburg. All
# Rights Reserved.
#
# Contributor(s): Alex Schuilenburg <alex@schuilenburg.org>
#
#
# Direct any questions on this source code to
#
# Alex Schuilenburg <alex@schuilenburg.org>


use diagnostics;
use strict;
use lib ".";

require "CGI.pl";
require "globals.pl";


# This could be moved to localparams or globals.pl
my %TableMap = (
    'bug_status'    => 'States in which a bug can be in during its lifecycle',
    'bug_severity'  => 'Impact the bug has on the application',
    'priority'      => 'Subjective priority placed on a bug',
    'op_sys'        => 'Host operating systems',
    'rep_platform'  => 'Target platforms',
    'resolution'    => 'State in which the bug was resolved'
);


# CheckTable:   same check, optionally  emit an error text
# TestValue:    just returns if the specified table/value combination exists
# CheckValue:   same check, optionally emit an error text


sub CheckTable ($)
{
    my $table = shift;
    my $ret;

    # do we have a table?
    unless ($table) {
        print "Sorry, you haven't specified a table.";
        PutTrailer();
        exit;
    }

    $ret = $TableMap{$table};

    unless ($ret) {
        print "Sorry, table '$table' does not exist or is not editable.";
        PutTrailer();
        exit;
    }

    return $ret;
}

sub TestValue ($$)
{
    my ($table,$value) = @_;

    # does the value exist?
    SendSQL("SELECT value
             FROM $table
             WHERE value=" . SqlQuote($value));
    return FetchOneColumn();
}

sub CheckValue ($$)
{
    my ($table,$value) = @_;
    # do we have the value?
    unless ($value) {
        print "Sorry, you haven't specified a value.";
        PutTrailer();
        exit;
    }

    my $desc = CheckTable($table);

    unless (TestValue($table,$value)) {
        print "Sorry, value '$value' for table '$table' does not exist.";
        PutTrailer();
        exit;
    }
}


#
# Displays the form to edit a value
#

sub EmitFormElements ($$)
{
    my ($table, $value) = @_;

    print "  <TH ALIGN=\"right\">Value:</TH>\n";
    print "  <TD><INPUT SIZE=64 MAXLENGTH=64 NAME=\"value\" VALUE=\"" .
        value_quote($value) . "\">\n";
    print "      <INPUT TYPE=HIDDEN NAME=\"table\" VALUE=\"" .
        value_quote($table) . "\"></TD>\n";
}


#
# Displays a text like "a.", "a or b.", "a, b or c.", "a, b, c or d."
#

sub PutTrailer (@)
{
    my (@links) = ("Back to the <A HREF=\"query.cgi\">query page</A>", @_);

    my $count = $#links;
    my $num = 0;
    print "<P>\n";
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
    PutFooter();
}







#
# Preliminary checks:
#

confirm_login();

print "Content-type: text/html\n\n";

unless (UserInGroup("editcomponents")) {
    PutHeader("Not allowed");
    print "Sorry, you aren't a member of the 'editcomponents' group.\n";
    print "And so, you aren't allowed to add, modify or delete table values.\n";
    PutTrailer();
    exit;
}


#
# often used variables
#
my $table = trim($::FORM{table} || '');
my $value = trim($::FORM{value} || '');
my $action  = trim($::FORM{action}  || '');
my $localtrailer;
if ($value) {
    $localtrailer = "<A HREF=\"edittable.cgi?table=" . url_quote($table) . "\">edit</A> more values";
} else {
    $localtrailer = "<A HREF=\"edittable.cgi\">edit</A> more values";
}



#
# table = '' -> Show nice list of values
#

unless ($table) {
    PutHeader("Select table");

    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0><TR BGCOLOR=\"#6666FF\">\n";
    print "  <TH ALIGN=\"left\">Edit values of ...</TH>\n";
    print "  <TH ALIGN=\"left\">Description</TH>\n";
    print "</TR>";
    foreach $table ( keys %TableMap ) {
        my $description = $TableMap{$table};
        $description ||= "<FONT COLOR=\"red\">missing</FONT>";
        print "<TR>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"edittable.cgi?table=", url_quote($table), "\"><B>$table</B></A></TD>\n";
        print "  <TD VALIGN=\"top\">$description</TD>\n";
        #print "  <TD VALIGN=\"top\"><A HREF=\"edittable.cgi?action=edit&table=", url_quote($table), "\">Edit</A></TD>\n";
    }
    print "</TR></TABLE>\n";

    PutTrailer();
    exit;
}



#
# action='' -> Show nice list of values
#

unless ($action) {
    PutHeader("Select value of $table ($TableMap{$table})");
    CheckTable($table);

    SendSQL("SELECT value
             FROM $table
             ORDER BY value");

    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0><TR BGCOLOR=\"#6666FF\">\n";
    print "  <TH ALIGN=\"left\">Edit value ...</TH>\n";
    print "  <TH ALIGN=\"left\">Action</TH>\n";
    print "</TR>";
    while ( MoreSQLData() ) {
        my ($value) = FetchSQLData();
        print "<TR>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"edittable.cgi?table=", url_quote($table), "&value=", url_quote($value), "&action=edit\"><B>$value</B></A></TD>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"edittable.cgi?table=", url_quote($table), "&value=", url_quote($value), "&action=del\"><B>Delete</B></A></TD>\n";
        print "</TR>";
    }
    print "<TR>\n";
    print "  <TD VALIGN=\"top\">Add a new value</TD>\n";
    print "  <TD VALIGN=\"top\" ALIGN=\"middle\"><A HREF=\"edittable.cgi?table=", url_quote($table) . "&action=add\">Add</A></TD>\n";
    print "</TR></TABLE>\n";

    PutTrailer();
    exit;
}




#
# action='add' -> present form for parameters for new value
#
# (next action will be 'new')
#

if ($action eq 'add') {
    PutHeader("Add value to $table ($TableMap{$table})");
    CheckTable($table);

    #print "This page lets you add a new value to a table in bugzilla.\n";

    print "<FORM METHOD=POST ACTION=edittable.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements($table, $value);

    print "</TABLE>\n<HR>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Add\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"new\">\n";
    print "</FORM>";

    my $other = $localtrailer;
    $other =~ s/more/other/;
    PutTrailer($other);
    exit;
}



#
# action='new' -> add value entered in the 'action=add' screen
#

if ($action eq 'new') {
    PutHeader("Adding new value to $table ($TableMap{$table})");
    CheckTable($table);

    # Cleanups and valididy checks

    unless ($value) {
        print "You must enter text for the new value. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }
    if (TestValue($table,$value)) {
        print "The value '$value' already exists. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }

    # Add the new value
    SendSQL("INSERT INTO $table ( " .
          "value" .
          " ) VALUES ( " .
          SqlQuote($value) . ")");

    # Make versioncache flush
    unlink "data/versioncache";

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
    PutHeader("Delete $value from $table ($TableMap{$table})");
    CheckValue($table, $value);

    SendSQL("SELECT count(bug_id),$table
             FROM bugs
             GROUP BY $table
             HAVING $table
             =" . SqlQuote($value));
    my $bugs = FetchOneColumn();

    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0>\n";
    print "<TR BGCOLOR=\"#6666FF\">\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Part</TH>\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Value</TH>\n";

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"left\" VALIGN=\"top\">Table:</TH>\n";
    print "  <TD VALIGN=\"top\">" . url_quote($table) . "</TD>\n";
    print "</TR><TR>\n";
    print "  <TH ALIGN=\"left\" VALIGN=\"top\">Value:</TH>\n";
    print "  <TD VALIGN=\"top\">" . url_quote($value) . "</TD>\n";
    print "</TR><TR>\n";
    print "  <TH ALIGN=\"left\" VALIGN=\"top\">Bugs:</TH>\n";
    print "  <TD VALIGN=\"top\">", $bugs || 'none' , "</TD>\n";
    print "</TR></TABLE>\n";

    print "<H2>Confirmation</H2>\n";

    if ($bugs) {
        if (!Param("allowbugdeletion")) {
            print "Sorry, there are $bugs bugs outstanding with this value.
You must reassign those bugs to another value before you can delete this
one.";
            PutTrailer($localtrailer);
            exit;
        }
        print "<TABLE BORDER=0 CELLPADDING=20 WIDTH=\"70%\" BGCOLOR=\"red\"><TR><TD>\n",
              "There are bugs entered for this value!  When you delete this ",
              "value, <B><BLINK>all</BLINK></B> stored bugs will be deleted, too. ",
              "You could not even see the bug history for this value anymore!\n",
              "</TD></TR></TABLE>\n";
    }

    print "<P>Do you really want to delete this value?<P>\n";
    print "<FORM METHOD=POST ACTION=edittable.cgi>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Yes, delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"table\" VALUE=\"" .
        value_quote($table) . "\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"value\" VALUE=\"" .
        value_quote($value) . "\">\n";
    print "</FORM>";

    PutTrailer($localtrailer);
    exit;
}



#
# action='delete' -> really delete the value
#

if ($action eq 'delete') {
    PutHeader("Deleting $value from $table ($TableMap{$table})");
    CheckValue($table,$value);

    # lock the tables before we start to change everything:
    
    SendSQL("LOCK TABLES attachments WRITE,
                         bugs WRITE,
                         bugs_activity WRITE, 
                         $table WRITE,
                         dependencies WRITE") if $::driver eq 'mysql';

    # According to MySQL doc I cannot do a DELETE x.* FROM x JOIN Y,
    # so I have to iterate over bugs and delete all the indivial entries
    # in bugs_activies and attachments.

    if (Param("allowbugdeletion")) {

        SendSQL("SELECT bug_id
             FROM bugs
             WHERE $table=" . SqlQuote($value));
        while (MoreSQLData()) {
            my $bugid = FetchOneColumn();

            PushGlobalSQLState();
            SendSQL("DELETE FROM attachments WHERE bug_id=$bugid");
            SendSQL("DELETE FROM bugs_activity WHERE bug_id=$bugid");
            SendSQL("DELETE FROM dependencies WHERE blocked=$bugid");
            PopGlobalSQLState();
        }
        print "Attachments, bug activity and dependencies deleted.<BR>\n";


        # Deleting the rest is easier:

        SendSQL("DELETE FROM bugs
             WHERE $table=" . SqlQuote($value));
        print "Bugs deleted.<BR>\n";
    }

    SendSQL("DELETE FROM $table
             WHERE value=" . SqlQuote($value));
    print "Value deleted.<P>\n";
    SendSQL("UNLOCK TABLES") if $::driver eq 'mysql';

    unlink "data/versioncache";
    PutTrailer($localtrailer);
    exit;
}



#
# action='edit' -> present the edit value form
#
# (next action would be 'update')
#

if ($action eq 'edit') {
    PutHeader("Edit value of $table ($TableMap{$table})");
    CheckValue($table,$value);

    print "<FORM METHOD=POST ACTION=edittable.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements($table, $value);

    print "</TR></TABLE>\n";

    print "<INPUT TYPE=HIDDEN NAME=\"valueold\" VALUE=\"" .
        value_quote($value) . "\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"update\">\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Update\">\n";

    print "</FORM>";

    my $other = $localtrailer;
    $other =~ s/more/other/;
    PutTrailer($other);
    exit;
}



#
# action='update' -> update the value
#

if ($action eq 'update') {
    PutHeader("Update value of $table ($TableMap{$table})");

    my $valueold = trim($::FORM{valueold} || '');

    CheckValue($table,$valueold);

    # Note that the order of this tests is important. If you change
    # them, be sure to test for WHERE='$value' or WHERE='$valueold'

    SendSQL("LOCK TABLES bugs WRITE, $table WRITE") if $::driver eq 'mysql';

    if ($value ne $valueold) {
        unless ($value) {
            print "Sorry, I can't delete the value text.";
            SendSQL("UNLOCK TABLES") if $::driver eq 'mysql';
            PutTrailer($localtrailer);
            exit;
        }
        if (TestValue($table,$value)) {
            print "Sorry, value '$value' is already in use.";
            SendSQL("UNLOCK TABLES") if $::driver eq 'mysql';
            PutTrailer($localtrailer);
            exit;
        }
        SendSQL("UPDATE bugs
                 SET $table=" . SqlQuote($value) . ",
                 delta_ts = delta_ts
                 WHERE $table=" . SqlQuote($valueold));
        SendSQL("UPDATE $table
                 SET value=" . SqlQuote($value) . "
                 WHERE value=" . SqlQuote($valueold));
        unlink "data/versioncache";
        print "Updated value.<BR>\n";
    }
    SendSQL("UNLOCK TABLES") if $::driver eq 'mysql';

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

