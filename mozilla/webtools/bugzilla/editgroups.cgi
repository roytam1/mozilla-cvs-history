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
use lib ".";

require "CGI.pl";

ConnectToDatabase();
confirm_login();

print "Content-type: text/html\n\n";

if (!UserInGroup("creategroups")) {
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
    print "<th>Id</th>";
    print "<th>Name</th>";
    print "<th>Description</th>";
    print "<th>User RegExp</th>";
    print "<th>Active</th>";
    print "<th>Buggroup</th>";
    print "<th>Action</th>";
    print "</tr>\n";

    SendSQL("SELECT group_id,name,description,userregexp,isactive,group_type " .
            "FROM groups " .
            "ORDER BY group_id");

    while (MoreSQLData()) {
        my ($groupid, $name, $desc, $regexp, $isactive, $group_type) = FetchSQLData();
        print "<tr>\n";
        print "<td valign=middle>$groupid</td>\n";
        print "<td><input size=20 name=\"name-$groupid\" value=\"$name\">\n";
        print "<input type=hidden name=\"oldname-$groupid\" value=\"$name\"></td>\n";
        print "<td><input size=40 name=\"desc-$groupid\" value=\"$desc\">\n";
        print "<input type=hidden name=\"olddesc-$groupid\" value=\"$desc\"></td>\n";
        print "<td><input size=30 name=\"regexp-$groupid\" value=\"$regexp\">\n";
        print "<input type=hidden name=\"oldregexp-$groupid\" value=\"$regexp\"></td>\n";
        print "<td><input type=\"checkbox\" name=\"isactive-$groupid\" value=\"1\"" . ($isactive ? " checked" : "") . ">\n";
        print "<input type=hidden name=\"oldisactive-$groupid\" value=\"$isactive\"></td>\n";
        print "<td>";
        if ($group_type == 0) {
            print "system";
        } else {
            print "<select name=\"group_type-$groupid\" value=\"$group_type\" >\n";
            print "<option value=1";
            print " selected" if ($group_type == 1);
            print ">buggoup</option><option value=2";
            print " selected" if ($group_type == 2);
            print ">user</option></select>\n";
        }
        print "</td>\n";
        print "<input type=hidden name=\"oldgroup_type-$groupid\" value=\"$group_type\"></td>\n";
        print "<td align=center valign=middle>
               <a href=\"editgroups.cgi?action=changeform&group=$groupid\">Edit</a>";
        print " | <a href=\"editgroups.cgi?action=del&group=$groupid\">Delete</a>" if ($group_type != 0);
        print "</td></tr>\n";
    }

    print "<tr>\n";
    print "<td colspan=6></td>\n";
    print "<td><a href=\"editgroups.cgi?action=add\">Add Group</a></td>\n";
    print "</tr>\n";
    print "</table>\n";
    print "<input type=hidden name=\"action\" value=\"update\">";
    print "<input type=submit value=\"Submit changes\">\n";

    print "<p>";
    print "<b>Name</b> is what is used with the UserInGroup() function in any
customized cgi files you write that use a given group.  It can also be used by
people submitting bugs by email to limit a bug to a certain set of groups. <p>";
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
    print "The <b>Buggroup</b> flag determines whether or not the group should
           generate a checkbox on bug entry/edit pages for group restrictions.
           If buggroup is set, the checkbox will be offerred.<p>";  
    print "In addition, the following groups that determine user privileges
exist.  You can only edit the User rexexp on these groups.  You should also take
care not to duplicate the Names of any of them in your user groups.<p>";
    print "</form>\n";

    PutFooter();
    exit;
}

#
#
# action='changeform' -> present form for altering an existing group
#
# (next action will be 'postchanges')
#

if ($action eq 'changeform') {
    PutHeader("Change Group");

    my $gid = trim($::FORM{group} || '');
    unless ($gid) {
        ShowError("No group specified.<BR>" .
                  "Click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }

    SendSQL("SELECT group_id, name, description, userregexp
             FROM groups WHERE group_id=" . SqlQuote($gid));
    my ($group_id, $name, $description, $rexp) = FetchSQLData();

    print "<P> 
           <B>Group:</B> $name <P>
           <B>Description:</B> $description <P>
           <B>User Regexp:</B> \"$rexp\" <P>
           <BR>
           In addition to the users explicitly included in this group 
           and users matching the regular expression listed 
           above, users who are members members of other groups can be 
           included by including the other groups in this group. <P>\n";

    print "<FORM METHOD=POST ACTION=editgroups.cgi>\n";
    print "<TABLE>";
    print "<TR><TD COLSPAN=4>Members of these groups can grant membership to this group</TD></TR>";
    print "<TR><TD ALIGN=CENTER>|</TD><TD COLSPAN=3>Members of these groups are included in this group</TD></TR>";
    print "<TR><TD ALIGN=CENTER>|</TD><TD ALIGN=CENTER>|</TD><TD COLSPAN=2></TD><TR>";
    SendSQL("SELECT groups.group_id, groups.name, groups.description,
             ISNULL(member_group_map.member_id) = 0, 
             ISNULL(B.member_id) = 0
             FROM groups
             LEFT JOIN member_group_map 
             ON member_group_map.member_id = groups.group_id
             AND member_group_map.group_id = $group_id
             AND member_group_map.maptype = 2
             LEFT JOIN member_group_map as B
             ON B.member_id = groups.group_id
             AND B.group_id = $group_id
             AND B.maptype = 3
             WHERE groups.group_id != $group_id ORDER by name");

    while (MoreSQLData()) {
        my ($grpid, $grpnam, $grpdesc, $grpmember, $blessmember) = FetchSQLData();
        my $grpchecked = $grpmember ? "CHECKED" : "";
        my $blesschecked = $blessmember ? "CHECKED" : "";
        print "<TR>";
        print "<TD><INPUT TYPE=checkbox NAME=\"bless-$grpid\" $blesschecked VALUE=1>";
        print "<INPUT TYPE=HIDDEN NAME=\"oldbless-$grpid\" VALUE=$blessmember></TD>";
        print "<TD><INPUT TYPE=checkbox NAME=\"grp-$grpid\" $grpchecked VALUE=1>";
        print "<INPUT TYPE=HIDDEN NAME=\"oldgrp-$grpid\" VALUE=$grpmember></TD>";
        print "<TD><B>$grpnam</B></TD>";
        print "<TD>$grpdesc</TD>";
        print "</TR>\n";
    }

    print "</TABLE><BR>";
    print "<INPUT TYPE=SUBMIT VALUE=\"Submit\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"postchanges\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"group\" VALUE=$gid>\n";
    print "</FORM>";



    PutTrailer("<a href=editgroups.cgi>Back to group list</a>");
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
    print "<th>Type</th>";
    print "</tr><tr>";
    print "<td><input size=20 name=\"name\"></td>\n";
    print "<td><input size=40 name=\"desc\"></td>\n";
    print "<td><input size=30 name=\"regexp\"></td>\n";
    print "<td><input type=\"checkbox\" name=\"isactive\" value=\"1\" checked></td>\n";
    print "<td><select name=\"group_type\" value=\"1\" >\n";
    print "<option value=1>buggoup</option>\n";
    print "<option value=2>user</option>\n";
    print "</select></td>\n";
    print "</TR></TABLE>\n<HR>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Add\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"new\">\n";
    print "</FORM>";

    print "<p>";
    print "<b>Name</b> is what is used with the UserInGroup() function in any
customized cgi files you write that use a given group.  It can also be used by
people submitting bugs by email to limit a bug to a certain set of groups.  It
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
    my $group_type = $::FORM{group_type} || 1;

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
            "name, description, group_type, userregexp, isactive, group_when " .
            " ) VALUES ( " .
            SqlQuote($name) . "," .
            SqlQuote($desc) . "," .
            "$group_type," .
            SqlQuote($regexp) . "," . 
            $isactive . ", NOW())" );
    SendSQL("SELECT last_insert_id()");
    my $gid = FetchOneColumn();
    my $admin = GroupNameToId('admin');
    SendSQL("INSERT INTO member_group_map (member_id, group_id, maptype)
             VALUES ($admin, $gid, 3)");
    print "OK, done.<p>\n";
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
    my $gid = trim($::FORM{group} || '');
    unless ($gid) {
        ShowError("No group specified.<BR>" .
                  "Click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }
    SendSQL("SELECT group_id FROM groups WHERE group_id=" . SqlQuote($gid));
    if (!FetchOneColumn()) {
        ShowError("That group doesn't exist.<BR>" .
                  "Click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }
    SendSQL("SELECT name,description " .
            "FROM groups " .
            "WHERE group_id = " . SqlQuote($gid));

    my ($name, $desc) = FetchSQLData();
    print "<table border=1>\n";
    print "<tr>";
    print "<th>Id</th>";
    print "<th>Name</th>";
    print "<th>Description</th>";
    print "</tr>\n";
    print "<tr>\n";
    print "<td>$gid</td>\n";
    print "<td>$name</td>\n";
    print "<td>$desc</td>\n";
    print "</tr>\n";
    print "</table>\n";

    print "<FORM METHOD=POST ACTION=editgroups.cgi>\n";
    my $cantdelete = 0;
    SendSQL("SELECT member_id FROM member_group_map 
             WHERE group_id = $gid");
    if (!FetchOneColumn()) {} else {
       $cantdelete = 1;
       print "
<B>One or more users belong to this group. You cannot delete this group while
there are users in it.</B><BR>
<A HREF=\"editusers.cgi?action=list&group=$gid\">Show me which users.</A> - <INPUT TYPE=CHECKBOX NAME=\"removeusers\">Remove all users from
this group for me<P>
";
    }
    SendSQL("SELECT bug_id FROM bug_group_map WHERE group_id = $gid");
    my $buglist="";
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
    print "<INPUT TYPE=HIDDEN NAME=\"group\" VALUE=\"$gid\">\n";
    print "</FORM>";

    PutTrailer("<a href=editgroups.cgi>No, go back to the group list</a>");
    exit;
}

#
# action='delete' -> really delete the group
#

if ($action eq 'delete') {
    PutHeader("Deleting group");
    my $gid = trim($::FORM{group} || '');
    unless ($gid) {
        ShowError("No group specified.<BR>" .
                  "Click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }
    SendSQL("SELECT name " .
            "FROM groups " .
            "WHERE group_id = " . SqlQuote($gid));
    my ($name) = FetchSQLData();

    my $cantdelete = 0;

    SendSQL("SELECT member_id FROM member_group_map 
             WHERE group_id = $gid");
    if (FetchOneColumn()) {
      if (!defined $::FORM{'removeusers'}) {
        $cantdelete = 1;
      }
    }
    SendSQL("SELECT bug_id FROM bug_group_map WHERE group_id = $gid");
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
      print "<A HREF=\"editgroups.cgi?action=del&group=$gid\">" .
            "View the list of which records are affected</A><BR>";
      PutTrailer("<a href=editgroups.cgi>Back to group list</a>");
      exit;
    }

    SendSQL("DELETE FROM member_group_map WHERE group_id = $gid");
    SendSQL("DELETE FROM bug_group_map WHERE group_id = $gid");
    SendSQL("DELETE FROM groups WHERE group_id = $gid");
    print "<B>Group $gid has been deleted.</B><BR>";


    PutTrailer("<a href=editgroups.cgi>Back to group list</a>");
    exit;
}

#
# action='postchanges' -> update the groups
#

if ($action eq 'postchanges') {
    PutHeader("Updating group hierarchy");
    my $gid = trim($::FORM{group} || '');
    unless ($gid) {
        ShowError("No group specified.<BR>" .
                  "Click the <b>Back</b> button and try again.");
        PutFooter();
        exit;
    }

    my $chgs = 0;
    print "Checking....";
    foreach my $b (grep(/^oldgrp-\d*$/, keys %::FORM)) {
        if (defined($::FORM{$b})) {
            my $v = substr($b, 7);
            print "checking $v<P>\n";
            my $grp = $::FORM{"grp-$v"} || 0;
            if ($::FORM{"oldgrp-$v"} != $grp) {
                $chgs = 1;
                print "changed";
                if ($grp != 0) {
                    print " set ";
                    SendSQL("INSERT INTO member_group_map 
                             (member_id, group_id, maptype, isderived)
                             VALUES ($v, $gid, 2, 0)");
                } else {
                    print " cleared ";
                    SendSQL("DELETE FROM member_group_map
                             WHERE member_id = $v AND group_id = $gid
                             AND maptype = 2");
                }
            }

            my $bless = $::FORM{"bless-$v"} || 0;
            if ($::FORM{"oldbless-$v"} != $bless) {
                $chgs = 1;
                print "changed";
                if ($bless != 0) {
                    print " set ";
                    SendSQL("INSERT INTO member_group_map 
                             (member_id, group_id, maptype, isderived)
                             VALUES ($v, $gid, 3, 0)");
                } else {
                    print " cleared ";
                    SendSQL("DELETE FROM member_group_map
                             WHERE member_id = $v AND group_id = $gid
                             AND maptype = 3");
                }
            }

        }
    }
    if (!$chgs) {
        print "You didn't change anything!<BR>\n";
        print "If you really meant it, hit the <B>Back</B> button and try again.<p>\n";
    } else {
        SendSQL("UPDATE groups SET group_when = NOW() WHERE group_id = $gid");
        print "Done.<p>\n";
    }
    PutTrailer("<a href=editgroups.cgi>Back to the group list</a>");
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
                             " && group_type = 0");
                    if (FetchOneColumn()) {
                        ShowError("You cannot update the name of a " .
                                  "system group. Skipping $v");
                    } else {
                        SendSQL("UPDATE groups SET name=" .
                                SqlQuote($::FORM{"name-$v"}) .
                                " WHERE group_id=" . SqlQuote($v));
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
                            " WHERE group_id=" . SqlQuote($v));
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
                        " , group_when = NOW() " .
                        " WHERE group_id=" . SqlQuote($v));
                print "Group $v user regexp updated.<br>\n";
            }
            # convert an undefined value in the inactive field to zero
            # (this occurs when the inactive checkbox is not checked 
            # and the browser does not send the field to the server)
            my $isactive = $::FORM{"isactive-$v"} || 0;
            if ($::FORM{"oldisactive-$v"} != $isactive) {
                $chgs = 1;
                if ($isactive == 0 || $isactive == 1) {
                    SendSQL("UPDATE groups SET isactive=$isactive, " .
                            " group_when = NOW() " .
                            " WHERE group_id=" . SqlQuote($v));
                    print "Group $v active flag updated.<br>\n";
                } else {
                    ShowError("The value '" . $isactive .
                              "' is not a valid value for the active flag.<BR>" .
                              "There may be a problem with Bugzilla or a bug in your browser.<br>" . 
                              "Update of active flag for group $v skipped.");
                }
            }
            # convert an undefined value in the group_type field to zero
            # (this occurs when the inactive checkbox is not checked 
            # and the browser does not send the field to the server)
            my $group_type = $::FORM{"group_type-$v"} || 0;
            if ($::FORM{"oldgroup_type-$v"} != $group_type) {
                $chgs = 1;
                if ($group_type == 0 || $group_type == 1 || $group_type == 2) {
                    SendSQL("UPDATE groups SET group_type=$group_type, " .
                            " group_when = NOW() " .
                            " WHERE group_id=" . SqlQuote($v));
                    print "Group $v group_type updated.<br>\n";
                } else {
                    ShowError("The value '" . $group_type .
                              "' is not a valid value for the group_type.<BR>" .
                              "There may be a problem with Bugzilla or a bug in your browser.<br>" . 
                              "Update of group_type for group $v skipped.");
                }
            }
        SendSQL("UPDATE groups SET group_when = NOW()
                 WHERE group_id=" . SqlQuote($v));
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
