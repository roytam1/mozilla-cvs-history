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
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Holger
# Schurig. Portions created by Holger Schurig are
# Copyright (C) 1999 Holger Schurig. All
# Rights Reserved.
#
# Contributor(s): Holger Schurig <holgerschurig@nikocity.de>
#                 Dave Miller <justdave@syndicomm.com>
#                 Joe Robins <jmrobins@tgix.com>
#                 Dan Mosedale <dmose@mozilla.org>
#
# Direct any questions on this source code to
#
# Holger Schurig <holgerschurig@nikocity.de>

use diagnostics;
use strict;

require "CGI.pl";
require "globals.pl";

# Shut up misguided -w warnings about "used only once".  "use vars" just
# doesn't work for me.

sub sillyness {
    my $zz;
    $zz = $::userid;
    $zz = $::superusergroupset;
}

my $editall;
my $opblessgroupset = '9223372036854775807'; # This is all 64 bits.



# TestUser:  just returns if the specified user does exists
# CheckUser: same check, optionally  emit an error text

sub TestUser ($)
{
    my $user = shift;

    # does the product exist?
    SendSQL("SELECT login_name
	     FROM profiles
	     WHERE login_name=" . SqlQuote($user));
    return FetchOneColumn();
}

sub CheckUser ($)
{
    my $user = shift;

    # do we have a product?
    unless ($user) {
	print "Sorry, you haven't specified a user.";
        PutTrailer();
	exit;
    }

    unless (TestUser $user) {
	print "Sorry, user '$user' does not exist.";
        PutTrailer();
	exit;
    }
}



sub EmitElement ($$)
{
    my ($name, $value) = (@_);
    $value = value_quote($value);
    if ($editall) {
        print qq{<TD><INPUT SIZE=64 MAXLENGTH=255 NAME="$name" VALUE="$value"></TD>\n};
    } else {
        print qq{<TD>$value</TD>\n};
    }
}


#
# Displays the form to edit a user parameters
#

sub EmitFormElements ($$$$$)
{
    my ($user, $realname, $groupset, $blessgroupset, $disabledtext) = @_;

    print "  <TH ALIGN=\"right\">Login name:</TH>\n";
    EmitElement("user", $user);

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Real name:</TH>\n";
    EmitElement("realname", $realname);

    if ($editall) {
        print "</TR><TR>\n";
        print "  <TH ALIGN=\"right\">Password:</TH>\n";
        if(Param('useLDAP')) {
          print "  <TD><FONT COLOR=RED>This site is using LDAP for authentication!</FONT></TD>\n";
        } else {
          print qq|
            <TD><INPUT TYPE="PASSWORD" SIZE="16" MAXLENGTH="16" NAME="password" VALUE=""><br>
                (enter new password to change)
            </TD>
          |;
        }
        print "</TR><TR>\n";

        print "  <TH ALIGN=\"right\">Disable text:</TH>\n";
        print "  <TD ROWSPAN=2><TEXTAREA NAME=\"disabledtext\" ROWS=10 COLS=60>" .
            value_quote($disabledtext) . "</TEXTAREA>\n";
        print "  </TD>\n";
        print "</TR><TR>\n";
        print "  <TD VALIGN=\"top\">If non-empty, then the account will\n";
        print "be disabled, and this text should explain why.</TD>\n";
    }
        
    
    if($user ne "") {
        print "</TR><TR><TH VALIGN=TOP ALIGN=RIGHT>Group Access:</TH><TD><TABLE><TR>";
        SendSQL("SELECT bit,name,description,bit & $groupset != 0, " .
                "       bit & $blessgroupset " .
                "FROM groups " .
                "WHERE bit & $opblessgroupset != 0 AND isbuggroup " .
                "ORDER BY name");
        if (MoreSQLData()) {
            if ($editall) {
                print "<TD COLSPAN=3 ALIGN=LEFT><B>Can turn this bit on for other users</B></TD>\n";
                print "</TR><TR>\n<TD ALIGN=CENTER><B>|</B></TD>\n";
            }
            print "<TD COLSPAN=2 ALIGN=LEFT><B>User is a member of these groups</B></TD>\n";
        }
        while (MoreSQLData()) {
            my ($bit,$name,$description,$checked,$blchecked) = FetchSQLData();
            print "</TR><TR>\n";
            if ($editall) {
                $blchecked = ($blchecked) ? "CHECKED" : "";
                print "<TD ALIGN=CENTER><INPUT TYPE=CHECKBOX NAME=\"blbit_$name\" $blchecked VALUE=\"$bit\"></TD>";
            }
            $checked = ($checked) ? "CHECKED" : "";
            print "<TD ALIGN=CENTER><INPUT TYPE=CHECKBOX NAME=\"bit_$name\" $checked VALUE=\"$bit\"></TD>";
            print "<TD><B>" . ucfirst($name) . "</B>: $description</TD>\n";
        }
        print "</TR></TABLE></TD>\n";
    
        print "</TR><TR><TH VALIGN=TOP ALIGN=RIGHT>Privileges:</TH><TD><TABLE><TR>";
        SendSQL("SELECT bit,name,description,bit & $groupset != 0, " .
                "       bit & $blessgroupset " .
                "FROM groups " .
                "WHERE bit & $opblessgroupset != 0 AND !isbuggroup " .
                "ORDER BY name");
        if (MoreSQLData()) {
            if ($editall) {
                print "<TD COLSPAN=3 ALIGN=LEFT><B>Can turn this bit on for other users</B></TD>\n";
                print "</TR><TR>\n<TD ALIGN=CENTER><B>|</B></TD>\n";
            }
            print "<TD COLSPAN=2 ALIGN=LEFT><B>User has these priveleges</B></TD>\n";
        }
        while (MoreSQLData()) {
            my ($bit,$name,$description,$checked,$blchecked) = FetchSQLData();
            print "</TR><TR>\n";
            if ($editall) {
                $blchecked = ($blchecked) ? "CHECKED" : "";
                print "<TD ALIGN=CENTER><INPUT TYPE=CHECKBOX NAME=\"blbit_$name\" $blchecked VALUE=\"$bit\"></TD>";
            }
            $checked = ($checked) ? "CHECKED" : "";
            print "<TD ALIGN=CENTER><INPUT TYPE=CHECKBOX NAME=\"bit_$name\" $checked VALUE=\"$bit\"></TD>";
            print "<TD><B>" . ucfirst($name) . "</B>: $description</TD>\n";
        }
    } else {
        print "</TR><TR><TH ALIGN=RIGHT>Groups and<br>Priveleges:</TH><TD><TABLE><TR>";        
        print "<TD COLSPAN=3>The new user will be inserted into groups " .
          "based on their userregexps.<BR>To change the group " .
          "permissions for this user, you must edit the account after ".
          "creating it.</TD>\n";
    }
    print "</TR></TABLE></TD>\n";

}



#
# Displays a text like "a.", "a or b.", "a, b or c.", "a, b, c or d."
#

sub PutTrailer (@)
{
    my (@links) = ("Back to the <A HREF=\"./\">index</A>", @_);

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

$editall = UserInGroup("editusers");

if (!$editall) {
    SendSQL("SELECT blessgroupset FROM profiles WHERE userid = $::userid");
    $opblessgroupset = FetchOneColumn();
    if (!$opblessgroupset) {
        PutHeader("Not allowed");
        print "Sorry, you aren't a member of the 'editusers' group, and you\n";
        print "don't have permissions to put people in or out of any group.\n";
        print "And so, you aren't allowed to add, modify or delete users.\n";
        PutTrailer();
        exit;
    }
}



#
# often used variables
#
my $user    = trim($::FORM{user}   || '');
my $action  = trim($::FORM{action} || '');
my $localtrailer = "<A HREF=\"editusers.cgi\">edit</A> more users";
my $candelete = Param('allowuserdeletion');



#
# action='' -> Ask for match string for users.
#

unless ($action) {
    PutHeader("Select match string");
    print qq{
<FORM METHOD=GET ACTION="editusers.cgi">
<INPUT TYPE=HIDDEN NAME="action" VALUE="list">
List users with login name matching: 
<INPUT SIZE=32 NAME="matchstr">
<SELECT NAME="matchtype">
<OPTION VALUE="substr" SELECTED>case-insensitive substring
<OPTION VALUE="regexp">case-sensitive regexp
<OPTION VALUE="notregexp">not (case-sensitive regexp)
</SELECT>
<BR>
<INPUT TYPE=SUBMIT VALUE="Submit">
</FORM>
};
    PutTrailer();
    exit;
}


#
# action='list' -> Show nice list of matching users
#

if ($action eq 'list') {
    PutHeader("Select user");
    my $query = "";
    if (exists $::FORM{'matchtype'}) {
      $query = "SELECT login_name,realname,disabledtext " .
          "FROM profiles WHERE login_name ";
      if ($::FORM{'matchtype'} eq 'substr') {
          $query .= "like";
          $::FORM{'matchstr'} = '%' . $::FORM{'matchstr'} . '%';
      } elsif ($::FORM{'matchtype'} eq 'regexp') {
          $query .= "regexp";
      } elsif ($::FORM{'matchtype'} eq 'notregexp') {
          $query .= "not regexp";
      } else {
          die "Unknown match type";
      }
      $query .= SqlQuote($::FORM{'matchstr'}) . " ORDER BY login_name";
    } elsif (exists $::FORM{'query'}) {
      $query = "SELECT login_name,realname,disabledtext " .
          "FROM profiles WHERE " . $::FORM{'query'} . " ORDER BY login_name";
    } else {
      die "Missing parameters";
    }

    SendSQL($query);
    my $count = 0;
    my $header = "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0><TR BGCOLOR=\"#6666FF\">
<TH ALIGN=\"left\">Edit user ...</TH>
<TH ALIGN=\"left\">Real name</TH>
";
    if ($candelete) {
        $header .= "<TH ALIGN=\"left\">Action</TH>\n";
    }
    $header .= "</TR>\n";
    print $header;
    while ( MoreSQLData() ) {
        $count++;
        if ($count % 100 == 0) {
            print "</table>$header";
        }
	my ($user, $realname, $disabledtext) = FetchSQLData();
        my $s = "";
        my $e = "";
        if ($disabledtext) {
            $s = "<STRIKE>";
            $e = "</STRIKE>";
        }
	$realname ||= "<FONT COLOR=\"red\">missing</FONT>";
	print "<TR>\n";
	print "  <TD VALIGN=\"top\"><A HREF=\"editusers.cgi?action=edit&user=", url_quote($user), "\"><B>$s$user$e</B></A></TD>\n";
	print "  <TD VALIGN=\"top\">$s$realname$e</TD>\n";
        if ($candelete) {
            print "  <TD VALIGN=\"top\"><A HREF=\"editusers.cgi?action=del&user=", url_quote($user), "\">Delete</A></TD>\n";
        }
	print "</TR>";
    }
    if ($editall && !Param('useLDAP')) {
        print "<TR>\n";
        my $span = $candelete ? 3 : 2;
        print qq{
<TD VALIGN="top" COLSPAN=$span ALIGN="right">
    <A HREF=\"editusers.cgi?action=add\">Add a new user</A>
</TD>
};
        print "</TR>";
    }
    print "</TABLE>\n";
    print "$count users found.\n";

    PutTrailer($localtrailer);
    exit;
}




#
# action='add' -> present form for parameters for new user
#
# (next action will be 'new')
#

if ($action eq 'add') {
    PutHeader("Add user");
    if (!$editall) {
        print "Sorry, you don't have permissions to add new users.";
        PutTrailer();
        exit;
    }

    if(Param('useLDAP')) {
      print "This site is using LDAP for authentication.  To add a new user, ";
      print "please contact the LDAP administrators.";
      PutTrailer();
      exit;
    }

    print "<FORM METHOD=POST ACTION=editusers.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements('', '', 0, 0, '');

    print "</TR></TABLE>\n<HR>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Add\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"new\">\n";
    print "</FORM>";

    my $other = $localtrailer;
    $other =~ s/more/other/;
    PutTrailer($other);
    exit;
}



#
# action='new' -> add user entered in the 'action=add' screen
#

if ($action eq 'new') {
    PutHeader("Adding new user");

    if (!$editall) {
        print "Sorry, you don't have permissions to add new users.";
        PutTrailer();
        exit;
    }

    if(Param('useLDAP')) {
      print "This site is using LDAP for authentication.  To add a new user, ";
      print "please contact the LDAP administrators.";
      PutTrailer();
      exit;
    }

    # Cleanups and valididy checks
    my $realname = trim($::FORM{realname} || '');
    # We don't trim the password since that could falsely lead the user
    # to believe a password with a space was accepted even though a space 
    # is an illegal character in a Bugzilla password.
    my $password = $::FORM{'password'};
    my $disabledtext = trim($::FORM{disabledtext} || '');
    my $emailregexp = Param("emailregexp");

    unless ($user) {
        print "You must enter a name for the new user. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }
    unless ($user =~ m/$emailregexp/) {
        print "The user name entered must be a valid e-mail address. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }
    if (TestUser($user)) {
	print "The user '$user' does already exist. Please press\n";
	print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
	exit;
    }
    my $passworderror = ValidatePassword($password);
    if ( $passworderror ) {
        print $passworderror;
        PutTrailer($localtrailer);
        exit;
    }

    # For new users, we use the regexps from the groups table to determine
    # their initial group membership.
    # We also keep a list of groups the user was added to for display on the
    # confirmation page.
    my $bits = "0";
    my @grouplist = ();
    SendSQL("select bit, name, userregexp from groups where userregexp != ''");
    while (MoreSQLData()) {
        my @row = FetchSQLData();
        if ($user =~ m/$row[2]/i) {
            $bits .= "+ $row[0]"; # Silly hack to let MySQL do the math,
                                  # not Perl, since we're dealing with 64
                                  # bit ints here, and I don't *think* Perl
                                  # does that.
            push(@grouplist, $row[1]);
        }
    }

    # Add the new user
    SendSQL("INSERT INTO profiles ( " .
            "login_name, cryptpassword, realname, groupset, " .
            "disabledtext" .
            " ) VALUES ( " .
            SqlQuote($user) . "," .
            SqlQuote(Crypt($password)) . "," .
            SqlQuote($realname) . "," .
            $bits . "," .
            SqlQuote($disabledtext) . ")" );

    #+++ send e-mail away

    print "OK, done.<br>\n";
    if($#grouplist > -1) {
        print "New user added to these groups based on group regexps:\n";
        print "<ul>\n";
        foreach (@grouplist) {
            print "<li>$_</li>\n";
        }
        print "</ul>\n";
    } else {
        print "New user not added to any groups.<br><br>\n";
    }
    print "To change ${user}'s permissions, go back and <a href=\"editusers.cgi?action=edit&user=" . url_quote($user)."\">edit this user</A>";
    print "<p>\n";
    PutTrailer($localtrailer,
	"<a href=\"editusers.cgi?action=add\">add</a> another user.");
    exit;

}



#
# action='del' -> ask if user really wants to delete
#
# (next action would be 'delete')
#

if ($action eq 'del') {
    PutHeader("Delete user");
    if (!$candelete) {
        print "Sorry, deleting users isn't allowed.";
        PutTrailer();
    }
    if (!$editall) {
        print "Sorry, you don't have permissions to delete users.";
        PutTrailer();
        exit;
    }
    CheckUser($user);

    # display some data about the user
    SendSQL("SELECT realname, groupset FROM profiles
	     WHERE login_name=" . SqlQuote($user));
    my ($realname, $groupset) = 
      FetchSQLData();
    $realname ||= "<FONT COLOR=\"red\">missing</FONT>";
    
    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0>\n";
    print "<TR BGCOLOR=\"#6666FF\">\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Part</TH>\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Value</TH>\n";

    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Login name:</TD>\n";
    print "  <TD VALIGN=\"top\">$user</TD>\n";

    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Real name:</TD>\n";
    print "  <TD VALIGN=\"top\">$realname</TD>\n";

    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Group set:</TD>\n";
    print "  <TD VALIGN=\"top\">";
    SendSQL("SELECT name
	     FROM groups
             WHERE bit & $groupset = bit
	     ORDER BY isbuggroup, name");
    my $found = 0;
    while ( MoreSQLData() ) {
	my ($name) = FetchSQLData();
        print "<br>\n" if $found;
        print ucfirst $name;
        $found = 1;
    }
    print "none" unless $found;
    print "</TD>\n</TR>";


    # Check if the user is an initialowner
    my $nodelete = '';

    SendSQL("SELECT program, value
	     FROM components
             WHERE initialowner=" . DBname_to_id($user));
    $found = 0;
    while (MoreSQLData()) {
	if ($found) {
	    print "<BR>\n";
	} else {
	    print "<TR>\n";
	    print "  <TD VALIGN=\"top\">Initial owner:</TD>\n";
	    print "  <TD VALIGN=\"top\">";
	}
	my ($product, $component) = FetchSQLData();
	print "<a href=\"editcomponents.cgi?product=", url_quote($product),
		"&component=", url_quote($component),
		"&action=edit\">$product: $component</a>";
	$found    = 1;
	$nodelete = 'initial bug owner';
    }
    print "</TD>\n</TR>" if $found;


    # Check if the user is an initialqacontact

    SendSQL("SELECT program, value
	     FROM components
             WHERE initialqacontact=" . DBname_to_id($user));
    $found = 0;
    while (MoreSQLData()) {
	if ($found) {
	    print "<BR>\n";
	} else {
	    print "<TR>\n";
	    print "  <TD VALIGN=\"top\">Initial QA contact:</TD>\n";
	    print "  <TD VALIGN=\"top\">";
	}
	my ($product, $component) = FetchSQLData();
	print "<a href=\"editcomponents.cgi?product=", url_quote($product),
		"&component=", url_quote($component),
		"&action=edit\">$product: $component</a>";
	$found    = 1;
	$nodelete = 'initial QA contact';
    }
    print "</TD>\n</TR>" if $found;

    print "</TABLE>\n";


    if ($nodelete) {
	print "<P>You can't delete this user because '$user' is an $nodelete ",
	      "for at least one product.";
	PutTrailer($localtrailer);
	exit;
    }


    print "<H2>Confirmation</H2>\n";
    print "<P>Do you really want to delete this user?<P>\n";

    print "<FORM METHOD=POST ACTION=editusers.cgi>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Yes, delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"user\" VALUE=\"$user\">\n";
    print "</FORM>";

    PutTrailer($localtrailer);
    exit;
}



#
# action='delete' -> really delete the user
#

if ($action eq 'delete') {
    PutHeader("Deleting user");
    if (!$candelete) {
        print "Sorry, deleting users isn't allowed.";
        PutTrailer();
    }
    if (!$editall) {
        print "Sorry, you don't have permissions to delete users.";
        PutTrailer();
        exit;
    }
    CheckUser($user);

    SendSQL("SELECT userid
	     FROM profiles
	     WHERE login_name=" . SqlQuote($user));
    my $userid = FetchOneColumn();

    SendSQL("DELETE FROM profiles
	     WHERE login_name=" . SqlQuote($user));
    SendSQL("DELETE FROM logincookies
	     WHERE userid=" . $userid);
    print "User deleted.<BR>\n";

    PutTrailer($localtrailer);
    exit;
}



#
# action='edit' -> present the user edit from
#
# (next action would be 'update')
#

if ($action eq 'edit') {
    PutHeader("Edit user");
    CheckUser($user);

    # get data of user
    SendSQL("SELECT realname, groupset, blessgroupset, disabledtext
	     FROM profiles
	     WHERE login_name=" . SqlQuote($user));
    my ($realname, $groupset, $blessgroupset,
        $disabledtext) = FetchSQLData();

    print "<FORM METHOD=POST ACTION=editusers.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements($user, $realname, $groupset, $blessgroupset, $disabledtext);
    
    print "</TR></TABLE>\n";

    print "<INPUT TYPE=HIDDEN NAME=\"userold\" VALUE=\"$user\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"realnameold\" VALUE=\"$realname\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"groupsetold\" VALUE=\"$groupset\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"blessgroupsetold\" VALUE=\"$blessgroupset\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"disabledtextold\" VALUE=\"" .
        value_quote($disabledtext) . "\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"update\">\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Update\">\n";

    print "</FORM>";

    my $x = $localtrailer;
    $x =~ s/more/other/;
    PutTrailer($x);
    exit;
}

#
# action='update' -> update the user
#

if ($action eq 'update') {
    PutHeader("Updated user");

    my $userold               = trim($::FORM{userold}              || '');
    my $realname              = trim($::FORM{realname}             || '');
    my $realnameold           = trim($::FORM{realnameold}          || '');
    my $password              = $::FORM{password}                  || '';
    my $disabledtext          = trim($::FORM{disabledtext}         || '');
    my $disabledtextold       = trim($::FORM{disabledtextold}      || '');
    my $groupsetold           = trim($::FORM{groupsetold}          || '0');
    my $blessgroupsetold      = trim($::FORM{blessgroupsetold}     || '0');

    my $groupset = "0";
    foreach (keys %::FORM) {
	next unless /^bit_/;
	#print "$_=$::FORM{$_}<br>\n";
	$groupset .= " + $::FORM{$_}";
    }
    my $blessgroupset = "0";
    foreach (keys %::FORM) {
	next unless /^blbit_/;
	#print "$_=$::FORM{$_}<br>\n";
	$blessgroupset .= " + $::FORM{$_}";
    }

    CheckUser($userold);

    # Note that the order of this tests is important. If you change
    # them, be sure to test for WHERE='$product' or WHERE='$productold'

    if ($groupset ne $groupsetold) {
        SendSQL("SELECT groupset FROM profiles WHERE login_name=" .
                SqlQuote($userold));
        $groupsetold = FetchOneColumn();
        # Updated, 5/7/00, Joe Robins
        # We don't want to change the groupset of a superuser.
        if($groupsetold eq $::superusergroupset) {
          print "Cannot change permissions of superuser.\n";
        } else {
           SendSQL("UPDATE profiles
                    SET groupset =
                         groupset - (groupset & $opblessgroupset) + $groupset
                    WHERE login_name=" . SqlQuote($userold));

           # I'm paranoid that someone who I give the ability to bless people
           # will start misusing it.  Let's log who blesses who (even though
           # nothing actually uses this log right now).
           my $fieldid = GetFieldID("groupset");
           SendSQL("SELECT userid, groupset FROM profiles WHERE login_name=" .
                   SqlQuote($userold));
           my $u;
           ($u, $groupset) = (FetchSQLData());
           if ($groupset ne $groupsetold) {
               SendSQL("INSERT INTO profiles_activity " .
                       "(userid,who,profiles_when,fieldid,oldvalue,newvalue) " .
                       "VALUES " .
                       "($u, $::userid, now(), $fieldid, " .
                       " $groupsetold, $groupset)");
           }
	   print "Updated permissions.\n";
       }
    }

    if ($editall && $blessgroupset ne $blessgroupsetold) {
        SendSQL("UPDATE profiles
		 SET blessgroupset=" . $blessgroupset . "
		 WHERE login_name=" . SqlQuote($userold));
	print "Updated ability to tweak permissions of other users.\n";
    }

    # Update the database with the user's new password if they changed it.
    if ( !Param('useLDAP') && $editall && $password ) {
        my $passworderror = ValidatePassword($password);
        if ( !$passworderror ) {
            my $cryptpassword = SqlQuote(Crypt($password));
            my $loginname = SqlQuote($userold);
            SendSQL("UPDATE  profiles
                     SET     cryptpassword = $cryptpassword
                     WHERE   login_name = $loginname");
            print "Updated password.<BR>\n";
        } else {
            print "Did not update password: $passworderror<br>\n";
        }
    }
    if ($editall && $realname ne $realnameold) {
        SendSQL("UPDATE profiles
		 SET realname=" . SqlQuote($realname) . "
		 WHERE login_name=" . SqlQuote($userold));
	print "Updated real name.<BR>\n";
    }
    if ($editall && $disabledtext ne $disabledtextold) {
        SendSQL("UPDATE profiles
		 SET disabledtext=" . SqlQuote($disabledtext) . "
		 WHERE login_name=" . SqlQuote($userold));
        SendSQL("SELECT userid
	         FROM profiles
	         WHERE login_name=" . SqlQuote($user));
        my $userid = FetchOneColumn();
        SendSQL("DELETE FROM logincookies
	         WHERE userid=" . $userid);
	print "Updated disabled text.<BR>\n";
    }
    if ($editall && $user ne $userold) {
	unless ($user) {
	    print "Sorry, I can't delete the user's name.";
            PutTrailer($localtrailer);
	    exit;
        }
	if (TestUser($user)) {
	    print "Sorry, user name '$user' is already in use.";
            PutTrailer($localtrailer);
	    exit;
        }

        SendSQL("UPDATE profiles
		 SET login_name=" . SqlQuote($user) . "
		 WHERE login_name=" . SqlQuote($userold));

	print "Updated user's name.<BR>\n";
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
