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
#				  David Lawrence <dkl@redhat.com>

use diagnostics;
use strict;

my $UserInEditGroupSet = -1;
my $UserInCanConfirmGroupSet = -1;

require "CGI.pl";

# Shut up misguided -w warnings about "used only once":

use vars %::versions,
    	 %::components,
    	 %::COOKIE,
	     %::keywordsbyname,
	     %::legal_keywords,
	     %::legal_opsys,
	     %::legal_platform,
	     %::legal_priority,
	     %::legal_severity;

my $whoid = confirm_login();

print "Content-type: text/html\n\n";
PutHeader ("Bug Processed", "Bug Processed", $::FORM{'id'});

GetVersionTable();

if ( Param("strictvaluechecks") ) {
    CheckFormFieldDefined(\%::FORM, 'product');
    CheckFormFieldDefined(\%::FORM, 'version');
    CheckFormFieldDefined(\%::FORM, 'component');
    # check if target milestone is defined - matthew@zeroknowledge.com
    if ( Param("usetargetmilestone") ) {
        CheckFormFieldDefined(\%::FORM, 'target_milestone');
    }
}


if ($::FORM{'product'} ne $::dontchange) {
    if ( Param("strictvaluechecks") ) {
        CheckFormField(\%::FORM, 'product', \@::legal_product);
    }
	my $prod = $::FORM{'product'};

    # note that when this script is called from buglist.cgi (rather
    # than show_bug.cgi), it's possible that the product will be changed
    # but that the version and/or component will be set to 
    # "--dont_change--" but still happen to be correct.  in this case,
    # the if statement will incorrectly trigger anyway.  this is a 
    # pretty weird case, and not terribly unreasonable behavior, but 
    # worthy of a comment, perhaps.
    #
    my $vok = lsearch($::versions{$prod}, $::FORM{'version'}) >= 0;
    my $cok = lsearch($::components{$prod}, $::FORM{'component'}) >= 0;

    my $mok = 1;   # so it won't affect the 'if' statement if milestones aren't used
    if ( Param("usetargetmilestone") ) {
       $mok = lsearch($::target_milestone{$prod}, $::FORM{'target_milestone'}) >= 0;
    }

    if (!$vok || !$cok || !$mok) {
        print "<H1>Changing product means changing version, target milestone and component.</H1>\n";
        print "You have chosen a new product, and now the version, target milestone and/or\n";
        print "component fields are not correct.  (Or, possibly, the bug did\n";
        print "not have a valid target milestone, component or version field in the first place.)\n";
        print "Anyway, please set the version, target milestone and component now.<p>\n";
        print "<FORM>\n";
        print "<TABLE>\n";
        print "<TR>\n";
        print "<TD ALIGN=right><B>Product:</B></TD>\n";
        print "<TD>$prod</TD>\n";
        print "</TR><TR>\n";
        print "<TD ALIGN=right><B>Version:</B></TD>\n";
        print "<TD>" . Version_element($::FORM{'version'}, $prod) . "</TD>\n";
        print "</TR><TR>\n";

        if ( Param("usetargetmilestone") ) {
            print "<td align=right><b>Target Milestone:</b></td>\n";
            print "<td>" . Milestone_element($::FORM{'target_milestone'}, $prod) . "</td>\n";
            print "</tr><tr>\n";
        }

        print "<TD ALIGN=right><B>Component:</B></TD>\n";
        print "<TD>" . Component_element($::FORM{'component'}, $prod) . "</TD>\n";
        print "</TR>\n";
        print "</TABLE>\n";
        foreach my $i (keys %::FORM) {
            if ($i ne 'version' && $i ne 'component' && $i ne 'target_milestone') {
                print "<INPUT TYPE=hidden NAME=$i VALUE=\"" .
                value_quote($::FORM{$i}) . "\">\n";
            }
        }
        print "<INPUT TYPE=submit VALUE=Commit>\n";
        print "</FORM>\n";
        print "</HR>\n";
        print "<A HREF=query.cgi>Cancel all this and go to the query page.</A>\n";
		PutFooter();
        exit;
    }
}


# Checks that the user is allowed to change the given field.  Actually, right
# now, the rules are pretty simple, and don't look at the field itself very
# much, but that could be enhanced.

my $lastbugid = 0;
my $ownerid;
my $reporterid;
my $qacontactid;

sub CheckCanChangeField {
    my ($f, $bugid, $oldvalue, $newvalue) = (@_);
    if ($f eq "assigned_to" || $f eq "reporter" || $f eq "qa_contact") {
        if ($oldvalue =~ /^\d+$/) {
            if ($oldvalue == 0) {
                $oldvalue = "";
            } else {
                $oldvalue = DBID_to_name($oldvalue);
            }
        }
    }
    if ($oldvalue eq $newvalue) {
        return 1;
    }
    if (trim($oldvalue) eq trim($newvalue)) {
        return 1;
    }
    if ($f =~ /^longdesc/) {
        return 1;
    }
    if ($UserInEditGroupSet < 0) {
        $UserInEditGroupSet = UserInGroup("editbugs");
    }
    if ($UserInEditGroupSet) {
        return 1;
    }
    if ($lastbugid != $bugid) {
        SendSQL("SELECT reporter, assigned_to, qa_contact FROM bugs " .
                "WHERE bug_id = $bugid");
        ($reporterid, $ownerid, $qacontactid) = (FetchSQLData());
    }
    if ($f eq "bug_status" && $newvalue ne $::unconfirmedstate &&
        IsOpenedState($newvalue)) {

        # Hmm.  They are trying to set this bug to some opened state
        # that isn't the UNCONFIRMED state.  Are they in the right
        # group?  Or, has it ever been confirmed?  If not, then this
        # isn't legal.

        if ($UserInCanConfirmGroupSet < 0) {
            $UserInCanConfirmGroupSet = UserInGroup("canconfirm");
        }
        if ($UserInCanConfirmGroupSet) {
            return 1;
        }
        SendSQL("SELECT everconfirmed FROM bugs WHERE bug_id = $bugid");
        my $everconfirmed = FetchOneColumn();
        if ($everconfirmed) {
            return 1;
       }
    } elsif ($reporterid eq $whoid || $ownerid eq $whoid ||
             $qacontactid eq $whoid) {
        return 1;
    }
    SendSQL("UNLOCK TABLES");
    $oldvalue = value_quote($oldvalue);
    $newvalue = value_quote($newvalue);
    print PuntTryAgain(qq{
Only the owner or submitter of the bug, or a sufficiently
empowered user, may make that change to the $f field.
<TABLE>
<TR><TH ALIGN="right">Old value:</TH><TD>$oldvalue</TD></TR>
<TR><TH ALIGN="right">New value:</TH><TD>$newvalue</TD></TR>
</TABLE>
});
    PutFooter();
    exit();
}


my @idlist;
if (defined $::FORM{'id'}) {
    # since this means that we were called from show_bug.cgi, now is a good
    # time to do a whole bunch of error checking that can't easily happen when
    # we've been called from buglist.cgi, because buglist.cgi only tweaks
    # values that have been changed instead of submitting all the new values.
    # (XXX those error checks need to happen too, but implementing them 
    # is more work in the current architecture of this script...)
    #
    if ( Param('strictvaluechecks') ) {
        CheckFormField(\%::FORM, 'rep_platform', \@::legal_platform);
        CheckFormField(\%::FORM, 'priority', \@::legal_priority);
        CheckFormField(\%::FORM, 'bug_severity', \@::legal_severity);
        CheckFormField(\%::FORM, 'component',
                       \@{$::components{$::FORM{'product'}}});
        CheckFormFieldDefined(\%::FORM, 'bug_file_loc');
        CheckFormFieldDefined(\%::FORM, 'short_desc');
        CheckFormField(\%::FORM, 'product', \@::legal_product);
        CheckFormField(\%::FORM, 'version',
                       \@{$::versions{$::FORM{'product'}}});
        CheckFormField(\%::FORM, 'op_sys', \@::legal_opsys);
        CheckFormFieldDefined(\%::FORM, 'longdesclength');
        CheckPosInt($::FORM{'id'});
    }
    push @idlist, $::FORM{'id'};
} else {
    foreach my $i (keys %::FORM) {
        if ($i =~ /^id_/) {
			if ( Param('strictvaluechecks') ) {
                CheckPosInt(substr($i, 3));
            }
            push @idlist, substr($i, 3);
        }
    }
}

if (!defined $::FORM{'who'}) {
    $::FORM{'who'} = $::COOKIE{'Bugzilla_login'};
}

# fix by {} - karin@root66.nl.eu.org
# we wouldn't like some hacker including shellchars, whould we?
$::FORM{'who'} =~ s/[^a-zA-Z0-9\@_\.\/\+\-]//g;

# the common updates to all bugs in @idlist start here
#
print "<TITLE>Update Bug " . join(" ", @idlist) . "</TITLE>\n";
#if (defined $::FORM{'id'}) {
#    navigation_header();
#}
#print "<HR>\n";

$::query = "update bugs\nset";
$::comma = "";
umask(0);

sub DoComma {
    $::query .= "$::comma\n    ";
    $::comma = ",";
}


sub DoConfirm {
    if ($UserInEditGroupSet < 0) {
        $UserInEditGroupSet = UserInGroup("editbugs");
    }
    if ($UserInCanConfirmGroupSet < 0) {
        $UserInCanConfirmGroupSet = UserInGroup("canconfirm");
    }
    if ($UserInEditGroupSet || $UserInCanConfirmGroupSet) {
        DoComma();
        $::query .= "everconfirmed = 1";
    }
}


sub ChangeStatus {
    my ($str) = (@_);
    if ($str ne $::dontchange) {
        DoComma();
        if (IsOpenedState($str) && $::driver eq 'mysql') {
            $::query .= "bug_status = IF(everconfirmed = 1, '$str', '$::unconfirmedstate')";
        } else {
	        $::query .= "bug_status = '$str'";
		}
        $::FORM{'bug_status'} = $str; # Used later for call to
                                      # CheckCanChangeField to make sure this
                                      # is really kosher.
    }
}


sub ChangeResolution {
    my ($str) = (@_);
    if ($str ne $::dontchange) {
        DoComma();
        $::query .= "resolution = '$str'";
    }
}


#
# This function checks if there is a comment required for a specific
# function and tests, if the comment was given.
# If comments are required for functions  is defined by params.
#
sub CheckonComment( $ ) {
    my ($function) = (@_);

    # Param is 1 if comment should be added !
    my $ret = Param( "commenton" . $function );

    # Allow without comment in case of undefined Params.
    $ret = 0 unless ( defined( $ret ));

    if( $ret ) {
        if (!defined $::FORM{'comment'} || $::FORM{'comment'} =~ /^\s*$/) {
            # No comment - sorry, action not allowed !
            PuntTryAgain("You have to specify a <b>comment</b> on this " .
                       	 "change.  Please give some words " .
                       	 "on the reason for your change.\n" );
        } else {
            $ret = 0;
        }
    }
    return( ! $ret ); # Return val has to be inverted
}


my $foundbit = 0;
foreach my $b (grep(/^bit-\d*$/, keys %::FORM)) {
    if (!$foundbit) {
        $foundbit = 1;
        DoComma();
        $::query .= "groupset = 0";
    }
    if ($::FORM{$b}) {
        my $v = substr($b, 4);
        $::query .= "+ $v";     # Carefully written so that the math is
                                # done by MySQL, which can handle 64-bit math,
                                # and not by Perl, which I *think* can not.
    }
}


# If certains groups were set insert values in bug_group
if ($::FORM{'id'}) {
	my $id = $::FORM{'id'};
	SendSQL("delete from bug_group where bugid = $id");
	foreach my $b (grep(/^group-.*$/, keys %::FORM)) {
		if ($::FORM{$b}) {
    		$b =~ s/^group-//;
			SendSQL("insert into bug_group values ($id, $b)");
		}
	}
}

# To fix in the near future
#if ($::FORM{'component_text'} ne "") {
#    SendSQL("Select value from components where program = " .
#            SqlQuote($::FORM{'product'}) .
#            " and value = " . SqlQuote($::FORM{'component_text'}));
#    my $comp = FetchOneColumn();
#    if ($comp) {
#        $::FORM{'component'} = $::FORM{'component_text'};
#    } else {
#        print "The component you typed in is invalid, please back up\n";
#        print "and correct the component text field or leave blank\n";
#        print "and choose a component from the scrolling list.\n";
#        exit;
#	}
#}

foreach my $field ("rep_platform", "priority", "bug_severity", 
                   "summary", "component", "bug_file_loc", "short_desc",
                   "product", "version", "op_sys", 
				   "target_milestone", "status_whiteboard") {
    if (defined $::FORM{$field}) {
        if ($::FORM{$field} ne $::dontchange) {
            DoComma();
            $::query .= "$field = " . SqlQuote(trim($::FORM{$field}));
        }
    }
}


if (defined $::FORM{'qa_contact'}) {
    my $name = trim($::FORM{'qa_contact'});
    if ($name ne $::dontchange) {
        my $id = 0;
        if ($name ne "") {
            $id = DBNameToIdAndCheck($name);
        }
        DoComma();
        $::query .= "qa_contact = $id";
    }
}


ConnectToDatabase();

my %ccids;
my $origcclist = "";

# We make sure to check out the CC list before we actually start touching any
# bugs.
if (defined $::FORM{'cc'} && defined $::FORM{'id'}) {
    $origcclist = ShowCcList($::FORM{'id'});
    if ($origcclist ne $::FORM{'cc'}) {
        foreach my $person (split(/[ ,]/, $::FORM{'cc'})) {
            if ($person ne "") {
                my $cid = DBNameToIdAndCheck($person);
                $ccids{$cid} = 1;
            }
        }
    }
}


if ( Param('strictvaluechecks') ) {
    CheckFormFieldDefined(\%::FORM, 'knob');
}


SWITCH: for ($::FORM{'knob'}) {
    /^none$/ && do {
        last SWITCH;
    };
    /^confirm$/ && CheckonComment( "confirm" ) && do {
        DoConfirm();
        ChangeStatus('NEW');
        last SWITCH;
    };
    /^accept$/ && CheckonComment( "accept" ) && do {
		DoConfirm();
        ChangeStatus('ASSIGNED');
        last SWITCH;
    };
    /^clearresolution$/ && CheckonComment( "clearresolution" ) && do {
        ChangeResolution('');
        last SWITCH;
    };
    /^resolve$/ && CheckonComment( "resolve" ) && do {
        ChangeStatus('RESOLVED');
        ChangeResolution($::FORM{'resolution'});
        last SWITCH;
    };
	# Change status removed from original code because when we reassign something 
	# we want it to retain assigned status if already assigned.
    /^assign$/ && CheckonComment( "reassign" ) && do{
        if ($::FORM{'andconfirm'}) {
            DoConfirm();
        }
        # ChangeStatus('NEW');
		ChangeStatus('ASSIGNED');
        if ( Param("strictvaluechecks") ) {
          	if ( !defined$::FORM{'assigned_to'} ||
               	trim($::FORM{'assigned_to'}) eq "") {
            	PuntTryAgain("You cannot reassign to a bug to nobody.  Unless " .
                	         "you intentionally cleared out the " .
                    	     "\"Reassign bug to\" field, " .
                        	 Param("browserbugmessage"));
          	}
        }
		DoComma();
        my $newid = DBNameToIdAndCheck($::FORM{'assigned_to'});
        $::query .= "assigned_to = $newid";
        last SWITCH;
    };
    # Change status removed from original code because when we reassign something 
    # we want it to retain assigned status if already assigned.   
	/^assignbycomponent$/ && CheckonComment( "reassignbycomponent" ) && do {
		if ($::FORM{'product'} eq $::dontchange) {
            PuntTryAgain("You must specify a product to help determine the new\n" . 
            			 "owner of these bugs.\n");
        }
        if ($::FORM{'component'} eq $::dontchange) {
            PuntTryAgain("You must specify a component whose owner should get " .
            			 "assigned these bugs.\n");
        }
		# ChangeStatus('NEW');
        ChangeStatus('ASSIGNED');
        SendSQL("select initialowner from components where program=" .
                SqlQuote($::FORM{'product'}) . " and value=" .
                SqlQuote($::FORM{'component'}));
        my $newname = FetchOneColumn();
        my $newid = DBNameToIdAndCheck($newname, 1);
        DoComma();
        $::query .= "assigned_to = $newid";
        if (Param("useqacontact")) {
            SendSQL("select initialqacontact from components where program=" .
                    SqlQuote($::FORM{'product'}) .
                    " and value=" . SqlQuote($::FORM{'component'}));
            my $qacontact = FetchOneColumn();
            if (defined $qacontact && $qacontact ne "") {
                my $newqa = DBNameToIdAndCheck($qacontact, 1);
                DoComma();
                $::query .= "qa_contact = $newqa";
            }
        }
        last SWITCH;
    };   
    /^reopen$/ && CheckonComment( "reopen" ) && do {
        ChangeStatus('REOPENED');
		ChangeResolution('');
        last SWITCH;
    };
    /^verify$/ && CheckonComment( "verify" ) && do {
        ChangeStatus('VERIFIED');
        last SWITCH;
    };
    /^close$/ && CheckonComment( "close" ) && do {
        ChangeStatus('CLOSED');
        last SWITCH;
    };
    /^duplicate$/ &&  CheckonComment( "duplicate" ) && do {
        ChangeStatus('RESOLVED');
        ChangeResolution('DUPLICATE');
        if ( Param('strictvaluechecks') ) {
            CheckFormFieldDefined(\%::FORM,'dup_id');
        }
        my $num = trim($::FORM{'dup_id'});
		SendSQL("SELECT bug_id FROM bugs WHERE bug_id = " . SqlQuote($num));
        $num = FetchOneColumn();
        if (!$num) { 
            PuntTryAgain("You must specify a bug number of which this bug is a " .
            			 "duplicate.  The bug has not been changed.\n");
        }
        if (!defined($::FORM{'id'}) || $num == $::FORM{'id'}) {
            PuntTryAgain("Nice try, $::FORM{'who'}.  But it doesn't really make sense to mark a " .
            			 "bug as a duplicate of itself, does it?\n");
        }
        AppendComment($num, $::FORM{'who'}, "*** Bug $::FORM{'id'} has been marked as a duplicate of this bug. ***");
		if ( Param('strictvaluechecks') ) {
          CheckFormFieldDefined(\%::FORM,'comment');
        }
        $::FORM{'comment'} .= "\n\n*** This bug has been marked as a duplicate of $num ***";

        print "<TABLE BORDER=1><TD><H2>Notation added to bug $num</H2>\n";
        system("./processmail", $num, $::FORM{'who'});
        print "<TD><A HREF=\"show_bug.cgi?id=$num\">Go To BUG# $num</A></TABLE>\n";

        last SWITCH;
    };
	# non-privileged user wants to add theirself to the cc list
	/^add_cc$/ && do {
		if ($::COOKIE{'Bugzilla_login'} ne $::FORM{'reporter'}) {
            my $whoid = DBNameToIdAndCheck($::COOKIE{'Bugzilla_login'});
            my $cc_query = "SELECT bug_id FROM cc " .
                           "WHERE bug_id = $::FORM{'id'} and who = $whoid";
            SendSQL($cc_query);
            $cc_query = FetchOneColumn();
            if ($cc_query) {
                PuntTryAgain("Nice Try. You are already on the CC list of bug $::FORM{'id'}.");
            } else {
                SendSQL("INSERT INTO cc VALUES ($::FORM{'id'}, $whoid)");
				last SWITCH;
            }
        } else {
            PuntTryAgain("<CENTER><B>You are the reporter of this bug. ". 
						 "You will already receive mail for this report.</B>" .
            	  		 "<P>Click back to return to the previous bug report.</CENTER><P>\n");
        }
	};
	# non-privileged user wants to remove themselves from the cc list
	/^rem_cc$/ && do {
		my $whoid = DBNameToIdAndCheck($::COOKIE{'Bugzilla_login'});
        my $cc_query = "SELECT bug_id FROM cc " .
                       "WHERE bug_id = '$::FORM{'id'}' and who = '$whoid'";
        SendSQL($cc_query);
        $cc_query = FetchOneColumn();
        if ($cc_query) {
            $cc_query = "DELETE FROM cc " .
                        "WHERE bug_id = '$::FORM{'id'}' and who = '$whoid'";
            SendSQL($cc_query);
			last SWITCH;
        } else {
            PuntTryAgain("Nice Try. You are not on the CC list of bug " . $::FORM{'id'});
        }
	};
	# non-privileged user wants to add a comment 
    /^add_comment$/ && do {
        if (defined $::FORM{'comment'} && $::FORM{'comment'} ne "") {
            AppendComment($::FORM{'id'}, $::COOKIE{'Bugzilla_login'}, $::FORM{'comment'});
            last SWITCH;
        } else {
            PuntTryAgain("<B>You did not type a comment for submission."); 
        }
    };

    # default
    PuntTryAgain("Unknown action $::FORM{'knob'}!\n");
    exit;
}


if ($#idlist < 0) {
    PuntTryAgain("You apparently didn't choose any bugs to modify.\n" .
    			 "<p>Click <b>Back</b> and try again.</CENTER>\n");
}


my @keywordlist;
my %keywordseen;

if ($::FORM{'keywords'}) {
    foreach my $keyword (split(/[\s,]+/, $::FORM{'keywords'})) {
        if ($keyword eq '') {
            next;
        }
        my $i = $::keywordsbyname{$keyword};
        if (!$i) {
            PuntTryAgain("Unknown keyword named <code>$keyword</code>.\n" .
            			 "<P>The legal keyword names are <A HREF=describekeywords.cgi>" .
            			 "listed here</A>.\n" .
            			 "<P>Please click the <B>Back</B> button and try again.</CENTER>\n");
        }
        if (!$keywordseen{$i}) {
            push(@keywordlist, $i);
            $keywordseen{$i} = 1;
        }
    }
}

my $keywordaction = $::FORM{'keywordaction'} || "makeexact";

if ($::comma eq "" && 0 == @keywordlist && $keywordaction ne "makeexact") {
    if (!defined $::FORM{'comment'} || $::FORM{'comment'} =~ /^\s*$/) {
        PuntTryAgain("Um, you apparently did not change anything on the selected " .
        			 "bugs. <p>Click <b>Back</b> and try again.</CENTER>\n");
    }
}

my $basequery = $::query;
my $delta_ts;


sub SnapShotBug {
    my ($id) = (@_);
	if ($::driver eq "mysql") {
		SendSQL("select delta_ts, " . join(',', @::log_columns) .
                " from bugs where bug_id = $id");
	} else {
    	SendSQL("select TO_CHAR(delta_ts, 'YYYYMMDDHH24MISS'), " . join(',', @::log_columns) .
        	    " from bugs where bug_id = $id");
	}
    my @row = FetchSQLData();
    $delta_ts = shift @row;
    return @row;
}


sub SnapShotDeps {
    my ($i, $target, $me) = (@_);
    SendSQL("select $target from dependencies where $me = $i order by $target");
    my @list;
    while (MoreSQLData()) {
        push(@list, FetchOneColumn());
    }
    return join(',', @list);
}


my $whoid = DBNameToIdAndCheck($::FORM{'who'});
my $timestamp;

sub LogDependencyActivity {
    my ($i, $oldstr, $target, $me) = (@_);
    my $newstr = SnapShotDeps($i, $target, $me);
    if ($oldstr ne $newstr) {
		my $fieldid = GetFieldID($target);
		if ($::driver eq "mysq") {
        	SendSQL("insert into bugs_activity " . 
					"(bug_id, who, bug_when, fieldid, oldvalue, newvalue) " .
					"values ($i, $whoid, $timestamp, '$fieldid', '$oldstr', '$newstr')");
		} else { 
			SendSQL("insert into bugs_activity " .
                    "(bug_id, who, bug_when, fieldid, oldvalue, newvalue) " .
                    "values ($i, $whoid, TO_DATE('$timestamp', 'YYYYMMDDHH24MISS'), '$fieldid', '$oldstr', '$newstr')");
		}
        return 1;
    }
    return 0;
}


delete $::FORM{'resolution'};   # Make sure we don't test the resolution
                                # against our permissions; we've already done
                                # that kind of testing, and this form field
                                # is actually usually not used.

my $userid = DBname_to_id($::COOKIE{'Bugzilla_login'});

# this loop iterates once for each bug to be processed (eg when this script
# is called with multiple bugs selected from buglist.cgi instead of
# show_bug.cgi).
#
foreach my $id (@idlist) {
	
	if (defined($id) && !CanIChange($id, $userid, DBname_to_id($::FORM{'reporter'}), 
		DBname_to_id($::FORM{'assigned_to'}))) {
		print "<H1>Permission denied.</H1><P>\n";
		print "<B>You do not have permission to modify bug # $id.</B> \n";	
		next;
	}

    my %dependencychanged;
    my $write = "WRITE";        # Might want to make a param to control
                                # whether we do LOW_PRIORITY ...
	if ($::driver eq 'mysql') {
    	SendSQL("LOCK TABLES bugs $write, bugs_activity $write, cc $write, " .
        	    "profiles $write, dependencies $write, votes $write, " .
            	"keywords $write, longdescs $write, fielddefs $write, " .
            	"keyworddefs READ, groups READ, attachments READ");
	}

    my @oldvalues = SnapShotBug($id);
#    my $i = 0;
#    foreach my $col (@::log_columns) {
#        if (exists $::FORM{$col}) {
#            CheckCanChangeField($col, $id, $oldvalues[$i], $::FORM{$col});
#        }
#        $i++;
#    }

    if (defined $::FORM{'delta_ts'} && $::FORM{'delta_ts'} ne $delta_ts) {
        print "
<H1>Mid-air collision detected!</H1>
Someone else has made changes to this bug at the same time you were trying to.
The changes made were:
<p>
";
        DumpBugActivity($id, $delta_ts);
        my $longdesc = GetLongDescriptionAsHTML($id);
        my $longchanged = 0;
        if (length($longdesc) > $::FORM{'longdesclength'}) {
            $longchanged = 1;
            print "<P>Added text to the long description:<blockquote>";
            print substr($longdesc, $::FORM{'longdesclength'});
            print "</blockquote>\n";
        }

		if ($::driver eq "mysql") {
        	SendSQL("unlock tables");
		}

        print "You have the following choices: <UL>\n";
        $::FORM{'delta_ts'} = $delta_ts;
        print "<LI><FORM METHOD=POST>";
        foreach my $i (keys %::FORM) {
            my $value = value_quote($::FORM{$i});
            print qq{<INPUT TYPE=hidden NAME="$i" VALUE="$value">\n};
        }
        print qq{<INPUT TYPE=submit VALUE="Submit my changes anyway">\n};
        print " This will cause all of the above changes to be overwritten";
        if ($longchanged) {
            print ", except for the changes to the description";
        }
        print qq{.</FORM>\n<LI><A HREF="show_bug.cgi?id=$id">Throw away my changes, and go revisit bug $id</A></UL>\n};
        PutFooter();
        exit;
    }
        
    my %deps;
    if (defined $::FORM{'dependson'}) {
        my $me = "blocked";
        my $target = "dependson";
        for (1..2) {
            $deps{$target} = [];
            my %seen;
            foreach my $i (split('[\s,]+', $::FORM{$target})) {
                if ($i eq "") {
                    next;

                }
                SendSQL("select bug_id from bugs where bug_id = " .
                        SqlQuote($i));
                my $comp = FetchOneColumn();
                if ($comp ne $i) {
					PuntTryAgain("$i is not a legal bug number");
                }
                if (!exists $seen{$i}) {
                    push(@{$deps{$target}}, $i);
                    $seen{$i} = 1;
                }
            }
            my @stack = @{$deps{$target}};
            while (@stack) {
                my $i = shift @stack;
                SendSQL("select $target from dependencies where $me = $i");
                while (MoreSQLData()) {
                    my $t = FetchOneColumn();
                    if ($t == $id) {
                        PuntTryAgain("Dependency loop detected!<P>" .
                                     "The change you are making to " .
                                     "dependencies has caused a circular " .
                                     "dependency chain.");
                    }
                    if (!exists $seen{$t}) {
                        push @stack, $t;
                        $seen{$t} = 1;
                    }
                }
            }
            my $tmp = $me;
            $me = $target;
            $target = $tmp;
        }
    }

	if (@::legal_keywords) {
        # There are three kinds of "keywordsaction": makeexact, add, delete.
        # For makeexact, we delete everything, and then add our things.
        # For add, we delete things we're adding (to make sure we don't
        # end up having them twice), and then we add them.
        # For delete, we just delete things on the list.
        my $changed = 0;
        if ($keywordaction eq "makeexact") {
            SendSQL("DELETE FROM keywords WHERE bug_id = $id");
            $changed = 1;
        }
        foreach my $keyword (@keywordlist) {
            if ($keywordaction ne "makeexact") {
                SendSQL("DELETE FROM keywords
                         WHERE bug_id = $id AND keywordid = $keyword");
                $changed = 1;
            }
            if ($keywordaction ne "delete") {
                SendSQL("INSERT INTO keywords 
                         (bug_id, keywordid) VALUES ($id, $keyword)");
                $changed = 1;
            }
        }
        if ($changed) {
            SendSQL("SELECT keyworddefs.name 
                     FROM keyworddefs, keywords
                     WHERE keywords.bug_id = $id
                         AND keyworddefs.id = keywords.keywordid
                     ORDER BY keyworddefs.name");
            my @list;
            while (MoreSQLData()) {
                push(@list, FetchOneColumn());
            }
            SendSQL("UPDATE bugs SET keywords = " .
                    SqlQuote(join(', ', @list)) .
                    " WHERE bug_id = $id");
        }
    }
    my $query = "$basequery\nwhere bug_id = $id";
    
#	print "<PRE>$query</PRE>\n";

    if ($::comma ne "") {
        SendSQL($query);
    	if ($::driver eq "mysql") {
        	SendSQL("select delta_ts from bugs where bug_id = $id");
    	} else {
        	SendSQL("select TO_CHAR(delta_ts, 'YYYYMMDDHH24MISS') from bugs where bug_id = $id");
    	}
	} else {
		if ($::driver eq 'mysql') {
			SendSQL("select now()");
		} else {
			SendSQL("select TO_CHAR(sysdate, 'YYYYMMDDHH24MISS') from dual");
		}
	}
    $timestamp = FetchOneColumn();
    
    if (defined $::FORM{'comment'}) {
        AppendComment($id, $::FORM{'who'}, $::FORM{'comment'});
	}

    if (defined $::FORM{'cc'} && ShowCcList($id) ne $::FORM{'cc'}) {
        my %ccids;
        foreach my $person (split(/[ ,]/, $::FORM{'cc'})) {
            if ($person ne "") {
				# Check to make sure it is in a valid email format
				my $match = Param('emailregexp');
				if ($person !~ /$match/) {
					PuntTryAgain("The Cc e-mail address you entered " . 
        						 "(<b>$person</b>) didn't match our minimal " .
        						 "syntax checking for a legal email address. " .
        						 Param('emailregexpdesc') . 
        						 "<p>Please click <b>back</b> and try again.");
				}
				my $cid = DBNameToIdAndCheck($person, 1);
                $ccids{$cid} = 1;
            }
        }
        SendSQL("delete from cc where bug_id = $id");
        foreach my $ccid (keys %ccids) {
            SendSQL("insert into cc (bug_id, who) values ($id, $ccid)");
        }
    }

    if (defined $::FORM{'cc'} && $origcclist ne $::FORM{'cc'}) {
        SendSQL("delete from cc where bug_id = $id");
        foreach my $ccid (keys %ccids) {
            SendSQL("insert into cc (bug_id, who) values ($id, $ccid)");
        }
        my $newcclist = ShowCcList($id);
        if ($newcclist ne $origcclist) {
            my $col = GetFieldID('cc');
            my $origq = SqlQuote($origcclist);
            my $newq = SqlQuote($newcclist);
			if ($::driver eq 'mysq') {
            	SendSQL("INSERT INTO bugs_activity " .
                	    "(bug_id, who, bug_when, fieldid, oldvalue, newvalue) VALUES " .
                    	"($id, $whoid, '$timestamp', $col, $origq, $newq)");
			} else {
            	SendSQL("INSERT INTO bugs_activity " .
                	    "(bug_id, who, bug_when, fieldid, oldvalue, newvalue) VALUES " .
                    	"($id, $whoid, TO_DATE('$timestamp', 'YYYYMMDDHH24MISS'), $col, $origq, $newq)");
			}	
        }
    }

    if (defined $::FORM{'dependson'}) {
        my $me = "blocked";
        my $target = "dependson";
        for (1..2) {
            SendSQL("select $target from dependencies where $me = $id order by $target");
            my %snapshot;
            my @oldlist;
            while (MoreSQLData()) {
                push(@oldlist, FetchOneColumn());
            }
            my @newlist = sort {$a <=> $b} @{$deps{$target}};
            @dependencychanged{@oldlist} = 1;
            @dependencychanged{@newlist} = 1;

            while (0 < @oldlist || 0 < @newlist) {
                if (@oldlist == 0 || (@newlist > 0 &&
                                      $oldlist[0] > $newlist[0])) {
                    $snapshot{$newlist[0]} = SnapShotDeps($newlist[0], $me,
                                                          $target);
                    shift @newlist;
                } elsif (@newlist == 0 || (@oldlist > 0 &&
                                           $newlist[0] > $oldlist[0])) {
                    $snapshot{$oldlist[0]} = SnapShotDeps($oldlist[0], $me,
                                                          $target);
                    shift @oldlist;
                } else {
                    if ($oldlist[0] != $newlist[0]) {
                        die "Error in list comparing code";
                    }
                    shift @oldlist;
                    shift @newlist;
                }
            }
            my @keys = keys(%snapshot);
            if (@keys) {
                my $oldsnap = SnapShotDeps($id, $target, $me);
                SendSQL("delete from dependencies where $me = $id");
                foreach my $i (@{$deps{$target}}) {
                    SendSQL("insert into dependencies ($me, $target) values ($id, $i)");
                }
                foreach my $k (@keys) {
                    LogDependencyActivity($k, $snapshot{$k}, $me, $target);
                }
                LogDependencyActivity($id, $oldsnap, $target, $me);
            }

            my $tmp = $me;
            $me = $target;
            $target = $tmp;
        }
    }

    # get a snapshot of the newly set values out of the database, 
    # and then generate any necessary bug activity entries by seeing 
    # what has changed since before we wrote out the new values.
    #
    my @newvalues = SnapShotBug($id);

    foreach my $c (@::log_columns) {
        my $col = $c;           # We modify it, don't want to modify array
                                # values in place.
        my $old = shift @oldvalues;
        my $new = shift @newvalues;
        if (!defined $old) {
            $old = "";
        }
        if (!defined $new) {
            $new = "";
        }
        if ($old ne $new) {
            if ($col eq 'assigned_to' || $col eq 'qa_contact') {
                $old = DBID_to_name($old) if $old != 0;
                $new = DBID_to_name($new) if $new != 0;
                $origcclist .= ",$old"; # make sure to send mail to people
                                        # if they are going to no longer get
                                        # updates about this bug.
            }
            if ($col eq 'product') {
                RemoveVotes($id, "This bug has been moved to a different product");
            }
			$col = GetFieldID($col);
            $old = SqlQuote($old);
            $new = SqlQuote($new);
			
			my $q = "";
			if ($::driver eq "mysql") {	
            	$q = "insert into bugs_activity (bug_id, who, bug_when, fieldid, oldvalue, newvalue) " .
					 "values ($id, $whoid, '$timestamp', $col, $old, $new)";
			} else {
				$q = "insert into bugs_activity (bug_id, who, bug_when, fieldid, oldvalue, newvalue) " .
                     "values ($id, $whoid, TO_DATE('$timestamp', 'YYYYMMDDHH24MISS'), $col, $old, $new)";
			}
#            print "<pre>$q</pre>";
            SendSQL($q);
        }
    }

    print "<TABLE BORDER=1 CELLSPACING=0 ALIGN=center>" ;
	print "	  <TD><H2>Changes to bug $id submitted</H2>\n<P>\n";

	if ($::driver eq "mysql") {
	    SendSQL("unlock tables");
	}

    system("./processmail", "-forcecc", $origcclist, $id, $::FORM{'who'});
	print "</TD>\n</TABLE>\n<P>\n";
	
	# Added because Oracle does not automatically update timestamps when using
    # UPDATE
    if ($::driver ne 'mysql') {
        SendSQL("update bugs set delta_ts = sysdate where bug_id = $id");
		SendSQL("select TO_CHAR(delta_ts, 'YYYYMMDDHH24MISS') from bugs " . 
				"where bug_id = $id");
		$::FORM{'delta_ts'} = FetchOneColumn();
    }

    foreach my $k (keys(%dependencychanged)) {
        print "<TABLE BORDER=1 CELLSPACING=0 ALIGN=center>";
		print "   <TD><H2>Checking for dependency changes on bug $k</H2>\n<P>\n";
        system("./processmail", $k, $::FORM{'who'});
        print "<TD><A HREF=\"show_bug.cgi?id=$k\">Go to BUG# $k</A></TABLE>\n<P>\n";
    }

}

#if (defined $::next_bug) {
#    print("<P>The next bug in your list is:\n");
#    $::FORM{'id'} = $::next_bug;
#    print "<HR>\n";
#
#    navigation_header();
#    do "bug_form.pl";
#} else {
#    navigation_header();
#    PutFooter();
#}
 
if ($#idlist == 0) {
    do "bug_form.pl";
}

PutFooter();
