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
# Contributor(s): Dave Miller <justdave@syndicomm.com>
#                 Jake Steenhagen <jake@acutexx.net>

# Code derived from editowners.cgi and editusers.cgi

use diagnostics;
use strict;

require "CGI.pl";

my $userid = confirm_login();

print "Content-type: text/html\n\n";

if (!UserInGroup($userid, "creategroups")) {
    PutHeader("Not Authorized","Edit Groups","","Not Authorized for this function!");
    print "<H1>Sorry, you aren't a member of the 'creategroups' group.</H1>\n";
    print "And so, you aren't allowed to edit the groups.\n";
    print "<p>\n";
    PutFooter();
    exit;
}

my $action  = trim($::FORM{action} || '');

# TestGroup: check if the group name exists
sub TestGroup ($)
{
    my $group = shift;

    # does the group exist?
    SendSQL("SELECT name
             FROM groups
             WHERE name=" . SqlQuote($group));
    return FetchOneColumn();
}

sub ShowError ($)
{
    my $msgtext = shift;
    print "<TABLE BGCOLOR=\"#FF0000\" CELLPADDING=15><TR><TD>";
    print "<B>$msgtext</B>";
    print "</TD></TR></TABLE><P>";
    return 1;
}

#
# Displays a text like "a.", "a or b.", "a, b or c.", "a, b, c or d."
#

sub PutTrailer (@)
{
    my (@links) = ("<a href=\"./\">Back to the Main Bugs Page</a>", @_);

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
# action='' -> No action specified, get a list.
#

unless ($action) {
    PutHeader("Edit Groups","Edit Groups","This lets you edit the groups available to put users in.");

    print "<form method=post action=editgroups.cgi>\n";
    print "<table border=1>\n";
    print "<tr>";
    print "<th>ID</th>";
    print "<th>Name</th>";
    print "<th>Description</th>";
    print "<th>User RegExp</th>";
    print "<th>Active</th>";
    print "<th>Action</th>";
    print "</tr>\n";

    SendSQL("SELECT group_id,name,description,userregexp,isactive " .
            "FROM groups " .
            "WHERE isbuggroup != 0 " .
            "ORDER BY group_id");

    while (MoreSQLData()) {
        my ($id, $name, $desc, $regexp, $isactive) = FetchSQLData();
        print "<tr>\n";
        print "<td valign=middle>$id</td>\n";
        print "<td><input size=20 name=\"name-$id\" value=\"$name\">\n";
        print "<input type=hidden name=\"oldname-$id\" value=\"$name\"></td>\n";
        print "<td><input size=40 name=\"desc-$id\" value=\"$desc\">\n";
        print "<input type=hidden name=\"olddesc-$id\" value=\"$desc\"></td>\n";
        print "<td><input size=30 name=\"regexp-$id\" value=\"$regexp\">\n";
        print "<input type=hidden name=\"oldregexp-$id\" value=\"$regexp\"></td>\n";
        print "<td><input type=\"checkbox\" name=\"isactive-$id\" value=\"1\"" . ($isactive ? " checked" : "") . ">\n";
        print "<input type=hidden name=\"oldisactive-$id\" value=\"$isactive\"></td>\n";
        print "<td align=center valign=middle><a href=\"editgroups.cgi?action=del&group=$id\">Delete</a></td>\n";
        print "</tr>\n";
    }

    print "<tr>\n";
    print "<td colspan=5></td>\n";
    print "<td><a href=\"editgroups.cgi?action=add\">Add Group</a></td>\n";
    print "</tr>\n";
    print "</table>\n";
    print "<input type=hidden name=\"action\" value=\"update\">";
    print "<input type=submit value=\"Submit changes\">\n";

    print "<p>";
    print "<b>Name</b> is what is used with the UserInGroup() function in any
customized cgi files you write that use a given group.  It can also be used by
people submitting bugs by email to limit a bug to a certain groupset.  It
may not contain any spaces.<p>";
    print "<b>Description</b> is what will be shown in the bug reports to
members of the group where they can choose whether the bug will be restricted
to others in the same group.<p>";
    print "<b>User RegExp</b> is optional, and if filled in, will automatically
grant membership to this group to anyone creating a new account with an
email address that matches this regular expression.<p>";
    print "The <b>Active</b> flag determines whether or not the group is active.
If you deactivate a group it will no longer be possible for users to add bugs
to that group, although bugs already in the group will remain in the group.
Deactivating a group is a much less drastic way to stop a group from growing
than deleting the group would be.<p>";
    print "In addition, the following groups that determine user privileges
exist.  You can only edit the User rexexp on these groups.  You should also take
care not to duplicate the Names of any of them in your user groups.<p>";
    print "Also please note that both of the Submit Changes buttons on this page
will submit the changes in both tables.  There are two buttons simply for the
sake of convience.<p>";

    print "<table border=1>\n";
    print "<tr>";
    print "<th>ID</th>";
    print "<th>Name</th>";
    print "<th>Description</th>";
    print "<th>User RegExp</th>";
    print "</tr>\n";

    SendSQL("SELECT group_id,name,description,userregexp " .
            "FROM groups " .
            "WHERE isbuggroup = 0 " .
            "ORDER BY group_id");

    while (MoreSQLData()) {
        my ($id, $name, $desc, $regexp) = FetchSQLData();
        print "<tr>\n";
        print "<td>$id</td>\n";
        print "<td>$name</td>\n";
        print "<input type=hidden name=\"name-$id\" value=\"$name\">\n";
        print "<input type=hidden name=\"oldname-$id\" value=\"$name\">\n";
        print "<td>$desc</td>\n";
        print "<td><input type=text size=30 name=\"regexp-$id\" value=\"$regexp\"></td>\n";
        print "<input type=hidden name=\"oldregexp-$id\" value=\"$regexp\">\n";
        print "</tr>\n";
    }

    print "</table><p>\n";
    print "<input type=submit value=\"Submit changes\">\n";
    print "</form>\n";

    PutFooter();
    exit;
}

#
# action='add' -> present form for parameters for new group
#
# (next action will be 'new')
#

if ($action eq 'add') {
    PutHeader("Add group");

    print "<FORM METHOD=POST ACTION=editgroups.cgi>\n";
    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0><TR>\n";
    print "<th>New Name</th>";
    print "<th>New Description</th>";
    print "<th>New User RegExp</th>";
    print "<th>Active</th>";
    print "</tr><tr>";
    print "<td><input size=20 name=\"name\"></td>\n";
    print "<td><input size=40 name=\"desc\"></td>\n";
    print "<td><input size=30 name=\"regexp\"></td>\n";
    print "<td><input type=\"checkbox\" name=\"isactive\" value=\"1\" checked></td>\n";
    print "</TR></TABLE>\n<HR>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Add\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"new\">\n";
    print "</FORM>";

    print "<p>";
    print "<b>Name</b> is what is used with the UserInGroup() function in any
customized cgi files you write that use a given group.  It can also be used by
people submitting bugs by email to limit a bug to a certain groupset.  It
may not contain any spaces.<p>";
    print "<b>Description</b> is what will be shown in the bug reports to
members of the group where they can choose whether the bug will be restricted
to others in the same group.<p>";
    print "The <b>Active</b> flag determines whether or not the group is active.
If you deactivate a group it will no longer be possible for users to add bugs
to that group, although bugs already in the group will remain in the group.
Deactivating a group is a much less drastic way to stop a group from growing
than deleting the group would be.  <b>Note: If you are creating a group, you
probably want it to be active, in which case you should leave this checked.</b><p>";
    print "<b>User RegExp</b> is optional, and if filled in, will automatically
grant membership to this group to anyone creating a new account with an
email address that matches this regular expression.<p>";

    PutTrailer("<a href=editgroups.cgi>Back to the group list</a>");
    exit;
}



#
# action='new' -> add group entered in the 'action=add' screen
#

if ($action eq 'new') {
    PutHeader("Adding new group");

    # Cleanups and valididy checks
    my $name = trim($::FORM{name} || '');
    my $desc = trim($::FORM{desc} || '');
    my $regexp = trim($::FORM{regexp} || '');
    # convert an undefined value in the inactive field to zero
    # (this occurs when the inactive checkbox is not checked
    # and the browser does not send the field to the server)
    my $isactive = $::FORM{isactive} || 0;

    unless ($name) {
        ShowError("You must enter a name for the new group.<BR>" .
                  "Please click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }
    unless ($desc) {
        ShowError("You must enter a description for the new group.<BR>" .
                  "Please click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }
    if (TestGroup($name)) {
        ShowError("The group '" . $name . "' already exists.<BR>" .
                  "Please click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }

    if ($isactive != 0 && $isactive != 1) {
        ShowError("The active flag was improperly set.  There may be " . 
                  "a problem with Bugzilla or a bug in your browser.<br>" . 
                  "Please click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }

    # Add the new group
    SendSQL("INSERT INTO groups ( " .
            "name, description, isbuggroup, userregexp, isactive" .
            " ) VALUES ( " .
            SqlQuote($name) . "," .
            SqlQuote($desc) . "," .
            "1," .
            SqlQuote($regexp) . "," . 
            $isactive . ")" );

    print "OK, done.<p>\n";
	  SendSQL("SELECT group_id FROM groups where name = " . SqlQuote($name));
	  my $id = FetchOneColumn();
    print "Your new group was assigned id # $id.<p>";
    
    # Add each user designated as an Admin to the new group
    my @adminlist = ();
    SendSQL("SELECT userid FROM profiles WHERE admin = 1");
    while (my @row = FetchSQLData()) {
        push(@adminlist, $row[0]);
    }
    foreach my $userid (@adminlist) {
        SendSQL("INSERT INTO user_group_map VALUES ($userid, $id)");
    }
    PutTrailer("<a href=\"editgroups.cgi?action=add\">Add another group</a>",
               "<a href=\"editgroups.cgi\">Back to the group list</a>");
    exit;
}

#
# action='del' -> ask if user really wants to delete
#
# (next action would be 'delete')
#

if ($action eq 'del') {
    PutHeader("Delete group");
    my $id = trim($::FORM{group} || '');
    unless ($id) {
        ShowError("No group specified.<BR>" .
                  "Click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }
    SendSQL("SELECT group_id FROM groups WHERE group_id = " . SqlQuote($id));
    if (!FetchOneColumn()) {	
        ShowError("That group doesn't exist.<BR>" .
                  "Click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }
    SendSQL("SELECT name,description " .
            "FROM groups " .
            "WHERE group_id = " . SqlQuote($id));

    my ($name, $desc) = FetchSQLData();
    print "<table border=1>\n";
    print "<tr>";
    print "<th>ID</th>";
    print "<th>Name</th>";
    print "<th>Description</th>";
    print "</tr>\n";
    print "<tr>\n";
    print "<td>$id</td>\n";
    print "<td>$name</td>\n";
    print "<td>$desc</td>\n";
    print "</tr>\n";
    print "</table>\n";

    print "<FORM METHOD=POST ACTION=editgroups.cgi>\n";
    my $cantdelete = 0;
    SendSQL("SELECT COUNT(user_id) FROM user_group_map WHERE group_id = $id");
	my $usergroupcount = FetchOneColumn();
	SendSQL("SELECT COUNT(DISTINCT user_id) FROM bless_group_map WHERE group_id = $id");
    my $blessgroupcount = FetchOneColumn();
    if ($usergroupcount || $blessgroupcount) {
       $cantdelete = 1;
       print "
<B>One or more users belong to this group. You cannot delete this group while
there are users in it.</B><BR>
<A HREF=\"editusers.cgi?action=list&query=" .
url_quote("(groupset & $id) OR (blessgroupset & $id)") . "\">Show me which users.</A> - <INPUT TYPE=CHECKBOX NAME=\"removeusers\">Remove all users from
this group for me<P>
";
    }
    SendSQL("SELECT bug_id FROM bug_group_map WHERE group_id = $id");
    if (MoreSQLData()) {
       $cantdelete = 1;
       my $buglist = "0";
       while (MoreSQLData()) {
         my ($bug) = FetchSQLData();
         $buglist .= "," . $bug;
       }
       print "
<B>One or more bug reports are visible only to this group.
You cannot delete this group while any bugs are using it.</B><BR>
<A HREF=\"buglist.cgi?bug_id=$buglist\">Show me which bugs.</A> -
<INPUT TYPE=CHECKBOX NAME=\"removebugs\">Remove all bugs from this group
restriction for me<BR>
<B>NOTE:</B> It's quite possible to make confidential bugs public by checking
this box.  It is <B>strongly</B> suggested that you review the bugs in this
group before checking the box.<P>
";
    }
    SendSQL("SELECT product FROM products WHERE product=" . SqlQuote($name));
    if (MoreSQLData()) {
       $cantdelete = 1;
       print "
<B>This group is tied to the <U>$name</U> product.
You cannot delete this group while it is tied to a product.</B><BR>
<INPUT TYPE=CHECKBOX NAME=\"unbind\">Delete this group anyway, and make the
<U>$name</U> product publicly visible.<BR>
";
    }

    print "<H2>Confirmation</H2>\n";
    print "<P>Do you really want to delete this group?\n";
    if ($cantdelete) {
      print "<BR><B>You must check all of the above boxes or correct the " .
            "indicated problems first before you can proceed.</B>";
    }
    print "<P><INPUT TYPE=SUBMIT VALUE=\"Yes, delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"group\" VALUE=\"$id\">\n";
    print "</FORM>";

    PutTrailer("<a href=editgroups.cgi>No, go back to the group list</a>");
    exit;
}

#
# action='delete' -> really delete the group
#

if ($action eq 'delete') {
    PutHeader("Deleting group");
    my $bit = trim($::FORM{group} || '');
    unless ($bit) {
        ShowError("No group specified.<BR>" .
                  "Click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }
    SendSQL("SELECT name " .
            "FROM groups " .
            "WHERE bit = " . SqlQuote($bit));
    my ($name) = FetchSQLData();

    my $cantdelete = 0;
    my $opblessgroupset = '9223372036854775807'; # This is all 64 bits.

    SendSQL("SELECT userid FROM profiles " .
            "WHERE (groupset & $opblessgroupset)=$opblessgroupset");
    my @opusers = ();
    while (MoreSQLData()) {
      my ($userid) = FetchSQLData();
      push @opusers, $userid; # cache a list of the users with admin powers
    }
    SendSQL("SELECT login_name FROM profiles WHERE " .
            "(groupset & $bit)=$bit OR (blessgroupset & $bit)=$bit");
    if (FetchOneColumn()) {
      if (!defined $::FORM{'removeusers'}) {
        $cantdelete = 1;
      }
    }
    SendSQL("SELECT bug_id FROM bugs WHERE (groupset & $bit)=$bit");
    if (FetchOneColumn()) {
      if (!defined $::FORM{'removebugs'}) {
        $cantdelete = 1;
      }
    }
    SendSQL("SELECT product FROM products WHERE product=" . SqlQuote($name));
    if (FetchOneColumn()) {
      if (!defined $::FORM{'unbind'}) {
        $cantdelete = 1;
      }
    }

    if ($cantdelete == 1) {
      ShowError("This group cannot be deleted because there are child " .
          "records in the database which refer to it.  All child records " .
          "must be removed or altered to remove the reference to this " .
          "group before the group can be deleted.");
      print "<A HREF=\"editgroups.cgi?action=del&group=$bit\">" .
            "View the list of which records are affected</A><BR>";
      PutTrailer("<a href=editgroups.cgi>Back to group list</a>");
      exit;
    }

    SendSQL("SELECT login_name,groupset,blessgroupset FROM profiles WHERE " .
            "(groupset & $bit) OR (blessgroupset & $bit)");
    if (FetchOneColumn()) {
      SendSQL("UPDATE profiles SET groupset=(groupset-$bit) " .
              "WHERE (groupset & $bit)");
      print "All users have been removed from group $bit.<BR>";
      SendSQL("UPDATE profiles SET blessgroupset=(blessgroupset-$bit) " .
              "WHERE (blessgroupset & $bit)");
      print "All users with authority to add users to group $bit have " .
            "had that authority removed.<BR>";
    }
    SendSQL("SELECT bug_id FROM bugs WHERE (groupset & $bit)");
    if (FetchOneColumn()) {
      SendSQL("UPDATE bugs SET groupset=(groupset-$bit) " .
              "WHERE (groupset & $bit)");
      print "All bugs have had group bit $bit cleared.  Any of these " .
            "bugs that were not also in another group are now " .
            "publicly visible.<BR>";
    }
    SendSQL("DELETE FROM groups WHERE bit=$bit");
    print "<B>Group $bit has been deleted.</B><BR>";

    foreach my $userid (@opusers) {
      SendSQL("UPDATE profiles SET groupset=$opblessgroupset " .
              "WHERE userid=$userid");
      print "Group bits restored for " . DBID_to_name($userid) .
            " (maintainer)<BR>\n";
    }

    PutTrailer("<a href=editgroups.cgi>Back to group list</a>");
    exit;
}

#
# action='update' -> update the groups
#

if ($action eq 'update') {
    PutHeader("Updating groups");

    my $chgs = 0;

    foreach my $b (grep(/^name-\d*$/, keys %::FORM)) {
        if ($::FORM{$b}) {
            my $v = substr($b, 5);

# print "Old: '" . $::FORM{"oldname-$v"} . "', '" . $::FORM{"olddesc-$v"} .
#      "', '" . $::FORM{"oldregexp-$v"} . "'<br>";
# print "New: '" . $::FORM{"name-$v"} . "', '" . $::FORM{"desc-$v"} .
#      "', '" . $::FORM{"regexp-$v"} . "'<br>";

            if ($::FORM{"oldname-$v"} ne $::FORM{"name-$v"}) {
                $chgs = 1;
                SendSQL("SELECT name FROM groups WHERE name=" .
                         SqlQuote($::FORM{"name-$v"}));
                if (!FetchOneColumn()) {
                    SendSQL("SELECT name FROM groups WHERE name=" .
                             SqlQuote($::FORM{"oldname-$v"}) .
                             " && isbuggroup = 0");
                    if (FetchOneColumn()) {
                        ShowError("You cannot update the name of a " .
                                  "system group. Skipping $v");
                    } else {
                        SendSQL("UPDATE groups SET name=" .
                                SqlQuote($::FORM{"name-$v"}) .
                                " WHERE bit=" . SqlQuote($v));
                        print "Group $v name updated.<br>\n";
                    }
                } else {
                    ShowError("Duplicate name '" . $::FORM{"name-$v"} .
                              "' specified for group $v.<BR>" .
                              "Update of group $v name skipped.");
                }
            }
            if ($::FORM{"olddesc-$v"} ne $::FORM{"desc-$v"}) {
                $chgs = 1;
                SendSQL("SELECT description FROM groups WHERE description=" .
                         SqlQuote($::FORM{"desc-$v"}));
                if (!FetchOneColumn()) {
                    SendSQL("UPDATE groups SET description=" .
                            SqlQuote($::FORM{"desc-$v"}) .
                            " WHERE bit=" . SqlQuote($v));
                    print "Group $v description updated.<br>\n";
                } else {
                    ShowError("Duplicate description '" . $::FORM{"desc-$v"} .
                              "' specified for group $v.<BR>" .
                              "Update of group $v description skipped.");
                }
            }
            if ($::FORM{"oldregexp-$v"} ne $::FORM{"regexp-$v"}) {
                $chgs = 1;
                SendSQL("UPDATE groups SET userregexp=" .
                        SqlQuote($::FORM{"regexp-$v"}) .
                        " WHERE bit=" . SqlQuote($v));
                print "Group $v user regexp updated.<br>\n";
            }
            # convert an undefined value in the inactive field to zero
            # (this occurs when the inactive checkbox is not checked 
            # and the browser does not send the field to the server)
            my $isactive = $::FORM{"isactive-$v"} || 0;
            if ($::FORM{"oldisactive-$v"} != $isactive) {
                $chgs = 1;
                if ($isactive == 0 || $isactive == 1) {
                    SendSQL("UPDATE groups SET isactive=$isactive" .
                            " WHERE bit=" . SqlQuote($v));
                    print "Group $v active flag updated.<br>\n";
                } else {
                    ShowError("The value '" . $isactive .
                              "' is not a valid value for the active flag.<BR>" .
                              "There may be a problem with Bugzilla or a bug in your browser.<br>" . 
                              "Update of active flag for group $v skipped.");
                }
            }
        }
    }
    if (!$chgs) {
        print "You didn't change anything!<BR>\n";
        print "If you really meant it, hit the <B>Back</B> button and try again.<p>\n";
    } else {
        print "Done.<p>\n";
    }
    PutTrailer("<a href=editgroups.cgi>Back to the group list</a>");
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

PutTrailer("<a href=editgroups.cgi>Try the group list</a>");
