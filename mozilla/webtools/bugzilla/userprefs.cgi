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
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Alan Raetz <al_raetz@yahoo.com>
#                 David Miller <dave@intrec.com>
#

use diagnostics;
use strict;

require "CGI.pl";

use RelationSet;

# Shut up misguided -w warnings about "used only once".  "use vars" just
# doesn't work for me.
sub sillyness {
    my $zz;
    $zz = $::defaultqueryname;
    $zz = $::usergroupset;
}

my $userid;

my $showNewEmailTech;

# Note the use of arrays instead of hashes: we want the items
# displayed in the same order as they appear in the array.

my @emailGroups = (
        'Owner',        'the Bug Owner',
        'Reporter',     'the Reporter',
        'QAcontact',    'the QA contact',
        'CClist',       'on the CC list'
        );

my @emailFlags = (
        'Removeme',     'When I\'m added to or removed from this capacity',
        'Comments',     'New Comments',
        'Attachments',  'New Attachments',
        'Status',       'Priority, status, severity, and milestone changes',
        'Resolved',     'When the bug is resolved or verified',
        'Keywords',     'Keywords field changes',
        'CC',           'CC field changes',
        'Other',        'Any field not mentioned above changes'
        );

my $defaultEmailFlagString =

        'emailOwnerRemoveme~'        . 'on~' .
        'emailOwnerComments~'        . 'on~' .
        'emailOwnerAttachments~'     . 'on~' .
        'emailOwnerStatus~'          . 'on~' .
        'emailOwnerResolved~'        . 'on~' .
        'emailOwnerKeywords~'        . 'on~' .
        'emailOwnerCC~'              . 'on~' .
        'emailOwnerOther~'           . 'on~' .

        'emailReporterRemoveme~'     . 'on~' .
        'emailReporterComments~'     . 'on~' .
        'emailReporterAttachments~'  . 'on~' .
        'emailReporterStatus~'       . 'on~' .
        'emailReporterResolved~'     . 'on~' .
        'emailReporterKeywords~'     . 'on~' .
        'emailReporterCC~'           . 'on~' .
        'emailReporterOther~'        . 'on~' .

        'emailQAcontactRemoveme~'    . 'on~' .
        'emailQAcontactComments~'    . 'on~' .
        'emailQAcontactAttachments~' . 'on~' .
        'emailQAcontactStatus~'      . 'on~' .
        'emailQAcontactResolved~'    . 'on~' .
        'emailQAcontactKeywords~'    . 'on~' .
        'emailQAcontactCC~'          . 'on~' .
        'emailQAcontactOther~'       . 'on~' .

        'emailCClistRemoveme~'       . 'on~' .
        'emailCClistComments~'       . 'on~' .
        'emailCClistAttachments~'    . 'on~' .
        'emailCClistStatus~'         . 'on~' .
        'emailCClistResolved~'       . 'on~' .
        'emailCClistKeywords~'       . 'on~' .
        'emailCClistCC~'             . 'on~' .
        'emailCClistOther~'          . 'on' ;


sub EmitEntry {
    my ($description, $entry) = (@_);
    print qq{<TR><TH ALIGN="right">$description:</TH><TD>$entry</TD></TR>\n};
}

sub Error {
    my ($msg) = (@_);
    print qq{
$msg
<P>
Please hit <B>back</B> and try again.
};
    PutFooter();
    exit();
}


sub ShowAccount {
    SendSQL("SELECT realname FROM profiles WHERE userid = $userid");
    my ($realname) = (FetchSQLData());

    $realname = value_quote($realname);
    
    EmitEntry("Old password",
              qq{<input type=password name="oldpwd">});
    EmitEntry("New password",
              qq{<input type=password name="pwd1">});
    EmitEntry("Re-enter new password", 
              qq{<input type=password name="pwd2">});
    EmitEntry("Your real name (optional)",
              qq{<INPUT SIZE=35 NAME="realname" VALUE="$realname">});
}

sub SaveAccount {
    if ($::FORM{'oldpwd'} ne ""
        || $::FORM{'pwd1'} ne "" || $::FORM{'pwd2'} ne "") {
        my $old = SqlQuote($::FORM{'oldpwd'});
        my $pwd1 = SqlQuote($::FORM{'pwd1'});
        my $pwd2 = SqlQuote($::FORM{'pwd2'});
        SendSQL("SELECT cryptpassword = ENCRYPT($old, LEFT(cryptpassword, 2)) " .
                "FROM profiles WHERE userid = $userid");
        if (!FetchOneColumn()) {
            Error("You did not enter your old password correctly.");
        }
        if ($pwd1 ne $pwd2) {
            Error("The two passwords you entered did not match.");
        }
        if ($::FORM{'pwd1'} eq '') {
            Error("You must enter a new password.");
        }
        SendSQL("UPDATE profiles SET password = $pwd1, " .
                "cryptpassword = ENCRYPT($pwd1) " .
                "WHERE userid = $userid");
    }
    SendSQL("UPDATE profiles SET " .
            "realname = " . SqlQuote($::FORM{'realname'}) .
            " WHERE userid = $userid");
}

#
# Set email flags in database based on the parameter string.
#
sub setEmailFlags ($) {

    my $emailFlagString = $_[0];

    SendSQL("UPDATE profiles SET emailflags = " .
            SqlQuote($emailFlagString) . " WHERE userid = $userid");
}


sub ShowEmailOptions () {

    SendSQL("SELECT emailnotification, newemailtech FROM profiles " .
            "WHERE userid = $userid");
    my ($emailnotification, $dbNewEmailTech) = (FetchSQLData());

    # Override the database value with the current form value
    # if the disable/enable button has been pressed.
    if ( defined $::FORM{'newEmailTech'} ) {

        # If the user has clicked on 'Disable New Email Tech',
        # clear out their email flag preferences.
        if ($dbNewEmailTech == 1) {
            setEmailFlags('');
        }

        $showNewEmailTech = $::FORM{'newEmailTech'};
    } else {
        $showNewEmailTech = $dbNewEmailTech;
    }

    #print "<BR>database newemailtech = $showNewEmailTech<br>";

    my $qacontactpart = "";
    if (Param('useqacontact')) {
        $qacontactpart = ", the current QA Contact";
    }

    if ($showNewEmailTech==0) {
    print <<"--endquote--";
<TABLE>
<TR><TD COLSPAN="2">
Bugzilla will send out email notification of changed bugs to 
the current owner, the Reporter of the bug$qacontactpart, and anyone on the
CC list.  However, you can suppress some of those email notifications.
On which of these bugs would you like email notification of changes?
</TD></TR>
--endquote--
    my $entry =
        BuildPulldown("emailnotification", 
                      [["ExcludeSelfChanges",
                        "All qualifying bugs except those which I change"],
                       ["CConly",
                        "Only those bugs which I am listed on the CC line"],
                       ["All",
                        "All qualifying bugs"]],
                       $emailnotification);
        EmitEntry("Notify me of changes to", $entry);
    print "</TABLE>";
    }

    if (Param("newemailtech")) {
        print qq{
<TR><TD COLSPAN="2"><HR></TD></TR>
<TR><TD COLSPAN="2"><FONT COLOR="red">Updated!</FONT>
Bugzilla's new standard email notification scheme allows for the use of 
features such as watching other users and selecting which bug changes you get 
mail about.  Although it's still possible to use the old notification scheme, 
this isn't recommended, because the old scheme is no longer
supported and will be going away in an upcoming version of Bugzilla.
<FONT COLOR="red">Note that after clicking the link below, you must still click
on the <B>Submit Changes</B> button in order for your email tech change to be saved.</FONT>
</TD></TR>};

        if ($showNewEmailTech == 1) {
            print qq{
                <tr><td colspan=2><center>
                <A HREF="userprefs.cgi?bank=diffs&amp;newEmailTech=0">Disable New Email Tech</A>
                </center></td></tr>};
        } else {
            print qq{<tr><td colspan=2><center><A HREF="userprefs.cgi?bank=diffs&amp;newEmailTech=1">Enable New Email Tech</A></center></td></tr>};
    }

        if (Param("supportwatchers") && $showNewEmailTech == 1) {

      my $watcheduserSet = new RelationSet;
      $watcheduserSet->mergeFromDB("SELECT watched FROM watch WHERE" .
                                    " watcher=$userid");
      my $watchedusers = $watcheduserSet->toString();

      print qq{
<TR><TD COLSPAN="4"><HR></TD></TR>
<TR><TD COLSPAN="4">
<FONT COLOR="red">New Email Tech Feature: </FONT>If you want to help cover for someone when they're on vacation, or if
you need to do the QA related to all of their bugs, you can tell bugzilla
to send mail related to their bugs to you also.  List the email addresses
of any users you wish to watch here, separated by commas.
</TD></TR>};

      EmitEntry("Users to watch",
              qq{<INPUT SIZE=35 NAME="watchedusers" VALUE="$watchedusers">});
        }

        print qq{<TR><TD COLSPAN="2"><HR></TD></TR>};

        if ($showNewEmailTech == 1) {
            showAdvancedEmailFilterOptions();
        }
    }

print qq {
<TABLE CELLSPACING="0" CELLPADDING="10" BORDER=0 WIDTH="100%">
<TR><TD>};

}

sub showAdvancedEmailFilterOptions () {

    my $flags;
    my $notify;
    my %userEmailFlags = ();

    print qq{
        <TR><TD COLSPAN="2"><center>
        <font size=+1>Advanced Email Filtering Options</font>
        </center>
        </TD></TR><tr><td colspan="2">
        <p>
        <center><FONT COLOR="red">New Email Tech Feature:</FONT>
        Filter email notifications for <b>modified</b>
        bugs (does not effect new bug email notices).
        </center></td></tr></table>
       <hr width=800 align=center>
    };

    SendSQL("SELECT emailflags, emailnotification FROM profiles WHERE " .
            "userid = $userid");

    ($flags, $notify) = FetchSQLData();

    # if the emailflags haven't been set before, that means that this user 
    # hasn't been to (the email pane of?) userprefs.cgi since the change to 
    # use emailflags.  create a default flagset for them, mostly based on
    # static defaults, but setting ExcludeSelf based on the old 
    # emailnotification column.
    #
    if ( !$flags ) {

        if ( !$notify ) { 
            confess("neither \$flags nor \$notify was set");
        }

        my $notifyString;
        if ( $notify eq 'ExcludeSelfChanges' ) {
            $notifyString = "ExcludeSelf~on~";
        } else {
            $notifyString = "ExcludeSelf~~";
        }

        $flags = $notifyString . $defaultEmailFlagString;
        setEmailFlags($flags);
    }

    # the 255 param is here, because without a third param, split will
    # trim any trailing null fields, which causes perl to eject lots of
    # warnings.  any suitably large number would do.
    #
    %userEmailFlags = split(/~/ , $flags, 255);

    showExcludeSelf(\%userEmailFlags);

    # print STDERR "$flags\n";

    print qq{
                <hr width=800 align=left>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
                  <b>Field/recipient specific options:</b><br><br>
              };


    my @tmpGroups = @emailGroups;
    while ((my $groupName,my $groupText) = splice(@tmpGroups,0,2) ) {
        printEmailPrefGroup($groupName,$groupText,\%userEmailFlags);
    }

}

sub showExcludeSelf (\%) {

     my %CurrentFlags = %{$_[0]};
     
     my $excludeSelf = " ";

     while ( my ($key,$value) = each (%CurrentFlags) ) {

     # print qq{flag name: $key    value: $value<br>};

        if ( $key eq 'ExcludeSelf' ) {

                if ( $value eq 'on' ) {

                        $excludeSelf = "CHECKED";
                        }
                }
        }

        print qq {
                <table><tr><td colspan=2>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
                <b>Global options:</b></tr>
                <tr><td width=150></td><td>
                Do not email me bugs that I change
             <input type="checkbox" name="ExcludeSelf" VALUE="on" $excludeSelf>
                <br>
                </td>
                </tr>
                </table>
                };

}

sub printEmailPrefGroup ($$\%) {

    my ($groupName,$textName,$refCurrentFlags) = @_[0,1,2];
    my @tmpFlags = @emailFlags;

    print qq {<table cellspacing=0 cellpadding=6> };
    print qq {<tr><td colspan=2> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
                When I\'m $textName, email me:</td></tr> };

    while ((my $flagName,my $flagText) = splice(@tmpFlags,0,2)) {

        printEmailOption($groupName . $flagName, $flagText, $refCurrentFlags);
    }
    print qq { </table> };
    print qq { <hr WIDTH=320 ALIGN=left> };
}

sub printEmailOption ($$\%) {

    my $value= '';

    my ($optionName,$description,$refCurrentFlags) = @_[0,1,2];

    #print qq{ email$optionName: $$refCurrentFlags{"email$optionName"} <br>};

    # if the db value is 'on', then mark that checkbox
    if ($$refCurrentFlags{"email$optionName"} eq 'on'){
        $value = 'CHECKED';
    }

    print qq{
        <tr><td width=320></td>
        <td><input type="checkbox" name="email$optionName" VALUE="on" $value>
        $description</td>
        </tr>
    };
}

sub SaveEmailOptions () {

    # I don't understand: global variables and %FORM variables are
    # not preserved between ShowEmailOptions() and SaveEmailOptions()
    # The form value here is from a hidden variable just before the SUBMIT.

    my $useNewEmailTech = $::FORM{'savedEmailTech'};
    my $updateString;

    if ($useNewEmailTech == 0) {

        # we force the NEW email filter entry to allow all email
        # (empty string defaults to allowing all email).
        $updateString = '';

    } else {
        if ( defined $::FORM{'ExcludeSelf'}) {
            $updateString .= 'ExcludeSelf~on';
        } else {
            $updateString .= 'ExcludeSelf~';
        }
        my @tmpGroups = @emailGroups;

        while ((my $groupName,my $groupText) = splice(@tmpGroups,0,2) ) {

            my @tmpFlags = @emailFlags;

            while ((my $flagName,my $flagText) = splice(@tmpFlags,0,2) ) {

                my $entry = 'email' . $groupName . $flagName;
                my $entryValue;

                if (!defined $::FORM{$entry} ) {
                    $entryValue = "";
                } else {
                    $entryValue = $::FORM{$entry};
                }

                $updateString .= '~' . $entry . '~' . $entryValue;
            }
        }

        # we force the OLD email tech flag to allow all email
        $::FORM{'emailnotification'} = "All";
    }
        
    #open(FID,">updateString");
    #print qq{UPDATE STRING: $updateString <br>};
    #close(FID);

    SendSQL("UPDATE profiles SET emailnotification = "
            . SqlQuote($::FORM{'emailnotification'})
            . ", newemailtech = $useNewEmailTech "
            . "WHERE userid = $userid" );

    SendSQL("UPDATE profiles SET emailflags = " .
            SqlQuote($updateString) . " WHERE userid = $userid");

    if (Param("supportwatchers") ) {

        if (exists $::FORM{'watchedusers'}) {

            Error ('You must have "New email tech" set to watch someone')
                if ( $::FORM{'watchedusers'} ne "" && $useNewEmailTech == 0);

            # Just in case.  Note that this much locking is actually overkill:
            # we don't really care if anyone reads the watch table.  So 
            # some small amount of contention could be gotten rid of by
            # using user-defined locks rather than table locking.
            #
            SendSQL("LOCK TABLES watch WRITE, profiles READ");

            # what the db looks like now
            #
            my $origWatchedUsers = new RelationSet;
            $origWatchedUsers->mergeFromDB("SELECT watched FROM watch WHERE" .
                                           " watcher=$userid");

            # update the database to look like the form
            #
            my $newWatchedUsers = new RelationSet($::FORM{'watchedusers'});
            my @CCDELTAS = $origWatchedUsers->generateSqlDeltas(
                                                             $newWatchedUsers, 
                                                             "watch", 
                                                             "watcher", 
                                                             $userid,
                                                             "watched");
            $CCDELTAS[0] eq "" || SendSQL($CCDELTAS[0]);
            $CCDELTAS[1] eq "" || SendSQL($CCDELTAS[1]);

            # all done
            #
            SendSQL("UNLOCK TABLES");
        
        }
    }
}



sub ShowFooter {
    SendSQL("SELECT mybugslink FROM profiles " .
            "WHERE userid = $userid");
    my ($mybugslink) = (FetchSQLData());
    my $entry =
        BuildPulldown("mybugslink",
                      [["1", "should appear"],
                       ["0", "should not be displayed"]],
                      $mybugslink);
    EmitEntry("The 'My bugs' link at the footer of each page", $entry);
    SendSQL("SELECT name, linkinfooter FROM namedqueries " .
            "WHERE userid = $userid");
    my $count = 0;
    while (MoreSQLData()) {
        my ($name, $linkinfooter) = (FetchSQLData());
        if ($name eq $::defaultqueryname) {
            next;
        }
        my $entry =
            BuildPulldown("query-$count",
                          [["0", "should only appear in the query page"],
                           ["1", "should appear on the footer of every page"]],
                          $linkinfooter);
        EmitEntry("Your query named '$name'", $entry);
        my $q = value_quote($name);
        print qq{<INPUT TYPE=HIDDEN NAME="name-$count" VALUE="$q">\n};
        $count++;
    }
    print qq{<INPUT TYPE=HIDDEN NAME="numqueries" VALUE="$count">\n};
    if (!$count) {
        print qq{
<TR><TD COLSPAN="4">                            
If you go create remembered queries in the <A HREF="query.cgi">query page</A>,
you can then come to this page and choose to have some of them appear in the 
footer of each Bugzilla page.
</TD></TR>};
    }
}
              
    
sub SaveFooter {
    my %old;
    SendSQL("SELECT name, linkinfooter FROM namedqueries " .
            "WHERE userid = $userid");
    while (MoreSQLData()) {
        my ($name, $linkinfooter) = (FetchSQLData());
        $old{$name} = $linkinfooter;
    }
    
    for (my $c=0 ; $c<$::FORM{'numqueries'} ; $c++) {
        my $name = $::FORM{"name-$c"};
        if (exists $old{$name}) {
            my $new = $::FORM{"query-$c"};
            if ($new ne $old{$name}) {
                SendSQL("UPDATE namedqueries SET linkinfooter = $new " .
                        "WHERE userid = $userid " .
                        "AND name = " . SqlQuote($name));
            }
        } else {
            Error("Hmm, the $name query seems to have gone away.");
        }
    }
    SendSQL("UPDATE profiles SET mybugslink = '" . $::FORM{'mybugslink'} .
            "' WHERE userid = $userid");
}
    


sub ShowPermissions {
    print "You have the following permission bits set on your account:\n";
    print "<P><UL>\n";
    my $found = 0;
    SendSQL("SELECT description FROM groups " .
            "WHERE bit & $::usergroupset != 0 " .
            "ORDER BY bit");
    while (MoreSQLData()) {
        my ($description) = (FetchSQLData());
        print "<LI>$description\n";
        $found = 1;
    }
    if ($found == 0) {
        print "<LI>(No extra permission bits have been set).\n";
    }
    print "</UL>\n";
    SendSQL("SELECT blessgroupset FROM profiles WHERE userid = $userid");
    my $blessgroupset = FetchOneColumn();
    if ($blessgroupset) {
        print "And you can turn on or off the following bits for\n";
        print qq{<A HREF="editusers.cgi">other users</A>:\n};
        print "<P><UL>\n";
        SendSQL("SELECT description FROM groups " .
                "WHERE bit & $blessgroupset != 0 " .
                "ORDER BY bit");
        while (MoreSQLData()) {
            my ($description) = (FetchSQLData());
            print "<LI>$description\n";
        }
        print "</UL>\n";
    }
}
        



######################################################################
################# Live code (not sub defs) starts here ###############


confirm_login();

print "Content-type: text/html\n\n";

GetVersionTable();

PutHeader("User Preferences", "User Preferences", $::COOKIE{'Bugzilla_login'});

#  foreach my $k (sort(keys(%::FORM))) {
#      print "<pre>" . value_quote($k) . ": " . value_quote($::FORM{$k}) . "\n</pre>";
#  }

my $bank = $::FORM{'bank'} || "account";

my @banklist = (
                ["account", "Account settings",
                 \&ShowAccount, \&SaveAccount],
                ["diffs", "Email settings",
                                 \&ShowEmailOptions, \&SaveEmailOptions],
                ["footer", "Page footer",
                 \&ShowFooter, \&SaveFooter],
                ["permissions", "Permissions",
                 \&ShowPermissions, undef]
                );


my $numbanks = @banklist;
my $numcols = $numbanks + 2;

my $headcol = '"lightblue"';

print qq{
<CENTER>
<TABLE CELLSPACING="0" CELLPADDING="10" BORDER=0 WIDTH="100%">
<TR>
<TH COLSPAN="$numcols" BGCOLOR="lightblue">User preferences</TH>
</TR>
<TR><TD BGCOLOR=$headcol>&nbsp;</TD>
};


my $bankdescription;
my $showfunc;
my $savefunc;

foreach my $i (@banklist) {
    my ($name, $description) = (@$i);
    my $color = "";
    if ($name eq $bank) {
        print qq{<TD ALIGN="center">$description</TD>};
        my $zz;
        ($zz, $bankdescription, $showfunc, $savefunc) = (@$i);
    } else {
        print qq{<TD ALIGN="center" BGCOLOR="lightblue"><A HREF="userprefs.cgi?bank=$name">$description</A></TD>};
    }
}
print qq{
<TD BGCOLOR=$headcol>&nbsp;</TD></TR>
</TABLE>
</CENTER>
<P>
};




if (defined $bankdescription) {
    $userid = DBNameToIdAndCheck($::COOKIE{'Bugzilla_login'});

    if ($::FORM{'dosave'}) {
        &$savefunc;
        print "Your changes have been saved.";
    }
    print qq{<H3>$bankdescription</H3><FORM METHOD="POST"><TABLE>};

    # execute subroutine from @banklist based on bank selected.
    &$showfunc;

    print qq{</TABLE><INPUT TYPE="hidden" NAME="dosave" VALUE="1">};
    print qq{<INPUT TYPE="hidden" NAME="savedEmailTech" VALUE="};

    # default this to 0 if it's not already set
    #
    if (defined $showNewEmailTech) {
        print qq{$showNewEmailTech">};
    } else {
        print qq{0">};
    }
    print qq{<INPUT TYPE="hidden" NAME="bank" VALUE="$bank"> };

    if ($savefunc) {
              print qq{<table><tr><td width=150></td><td>
                        <INPUT TYPE="submit" VALUE="Submit Changes">
                        </td></tr></table> };
    }
    print qq{</FORM>\n};
} else {
    print "<P>Please choose from the above links which settings you wish to change.</P>";
}


print "<P>";


PutFooter();
