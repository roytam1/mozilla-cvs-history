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
#                 David Lawrence <dkl@redhat.com>
#    

use diagnostics;
use strict;

require "CGI.pl";
require "globals.pl";

# print "Content-type: text/html\n\n";
 
# subroutine: 	PutGroupForm
# description: 	prints form for multiple choices
# params: 		none
# returns 		none

sub PutGroupForm {
	my $query = "select groupid, description from groups order by groupid";
	SendSQL($query);
    my %grouplist;
    while (my @row = FetchSQLData()) {
        $grouplist{$row[0]} = $row[1];
    }

	print qq{
<P>
<B>1. Select members above that you want effected.<BR>	
2. Check which groups below you wish the members to be in.<BR>
3. Click Update to commit the changes.</B><BR>
<P>
<TABLE ALIGN=center BORDER=1 CELLSPACING=0>
<TR>
	<TD ALIGN=center><INPUT TYPE=radio NAME=group_type VALUE=add CHECKED>Add to checked groups</TD>
	<TD ALIGN=center><INPUT TYPE=radio NAME=group_type VALUE=remove>Remove from checked groups</TD>
</TR>
</TABLE>
<P>
};	
	my $count = 0;
	foreach my $i (keys %grouplist) {
    	print qq{<INPUT TYPE=checkbox NAME=group-$i>&nbsp;$grouplist{$i}<br>};
	}

#    print "
#<SCRIPT>
#numelements = document.changeform.elements.length;
#function SetCheckboxes(value) {
#    for (var i=0 ; i<numelements ; i++) {
#        item = document.changeform.elements\[i\];
#        item.checked = value;
#    }
#}

#document.write(\" <input type=button value=\\\"Uncheck All\\\" onclick=\\\"SetCheckboxes(false);\\\"> <input type=button value=\\\"Check All\\\" onclick=\\\"SetCheckboxes(true);\\\">\");
#</SCRIPT>\n";

	print qq{
<P>
<HR>
<INPUT TYPE=hidden NAME=multiple VALUE=1>
<CENTER><INPUT TYPE=submit NAME=update VALUE=" Update! "></CENTER>
</FORM>
};

}


# subroutine: 	UpdateGroups 
# description: 	updates member(s) group permissions
# params: 		none
# returns: 		none

sub UpdateGroups {
	if (defined($::FORM{'multiple'}) && $::FORM{'multiple'} eq '1') {
		foreach my $user (grep(/^user-.*$/, keys %::FORM)) {
			$user =~ s/^user-//g;
			foreach my $i (grep (/^group-.*$/, keys %::FORM)) {
				$i =~ s/^group-//g;
				SendSQL("select userid from user_group where userid = $user and groupid = $i");
				my $result = FetchOneColumn();
				if (!$result && $::FORM{'group_type'} eq 'add') {
            		SendSQL("insert into user_group values ($user, $i)");
				} elsif ($result && $::FORM{'group_type'} eq 'remove') {
					SendSQL("delete from user_group where userid = $user and groupid = $i");
				} 
        	}
		}
	} else {
		SendSQL("delete from user_group where userid = " . $::FORM{'userid'});
		foreach my $i (grep (/^group-.*$/, keys %::FORM)) {
			$i =~ s/^group-//g;
			SendSQL("insert into user_group values (" .
			$::FORM{'userid'} . ", $i)");
		}
	}
	return 1;
}


# subroutine: 	SortPopup
# description: 	generate popup for sorting results list by group
# params:		none
# returns:		$popup = string containing html to generate group sort popup (scalar)

sub SortPopup {
	my $popup = "";
	my $query = "select groupid, name from groups order by groupid";
	SendSQL($query);
	my %groups;
	while (my @row = FetchSQLData()) {
		$groups{$row[1]} = $row[0];
	}

	$popup .= qq{
<FORM METHOD=get ACTION=editmembers.cgi>
<B>Sort by group</B>
<SELECT NAME=sort>
<OPTION VALUE="all">All
};

	my @sorted_ids = sort keys %groups;
	foreach my $group (@sorted_ids) {
		my $selected = $::FORM{'sort'} eq $groups{$group} ? " SELECTED" : "";
		$popup .= "<OPTION VALUE=\"$groups{$group}\" $selected>$group";
	}

	$popup .= qq{
</SELECT>
<INPUT TYPE=submit NAME= do_sort VALUE="Sort">
<INPUT TYPE=hidden NAME=login_name VALUE="$::FORM{'login_name'}">
<INPUT TYPE=hidden NAME=real_name VALUE="$::FORM{'real_name'}">
<INPUT TYPE=hidden NAME=multiple VALUE="$::FORM{'multiple'}">
</FORM>
};
	return $popup;
}


confirm_login();

print "Content-type: text/html\n\n";

if (!UserInGroup("editgroupmembers")) {
	PutHeader("Not Allowed");
    PutError("Sorry, you aren't a member of the 'editgroupmembers' group.<BR>" .
    		 "And so, you aren't allowed to edit the members.");
}

PutHeader("Edit Members");

if ($::FORM{'submit'} || $::FORM{'sort'}) {
	my $sort_popup = SortPopup();
	print "<CENTER><H2>Results for search on ";
	if ($::FORM{'login_name'}) {
		print "$::FORM{'login_name'} ";
	}
	if ($::FORM{'login_name'} && $::FORM{'realname'}) {
		print " and ";
	}
	if ($::FORM{'realname'}) {
		print "$::FORM{'realname'} ";
	}
	print "</H2></CENTER>\n"; 

	# User didnt enter anything 
	if (!$::FORM{'login_name'} && !$::FORM{'realname'}) {
        PutError("<P>Illegal values - Back up and try again.");
    }

	# Form the Query
	my $query = "";
	if (defined($::FORM{'sort'}) && $::FORM{'sort'} ne 'all' && $::FORM{'sort'} ne '') {
		$query = "select profiles.userid, profiles.login_name, profiles.realname " .
					"from profiles, user_group " . 
					"where user_group.userid = profiles.userid and " .
					"user_group.groupid = $::FORM{'sort'} and ";
	} else {
		$query = "select profiles.userid, profiles.login_name, profiles.realname " .
                    "from profiles where ";
	}

	if ($::FORM{'login_name'}) {
		$query .= " profiles.login_name like \'%$::FORM{'login_name'}%\'";
	}
	if ($::FORM{'login_name'} && $::FORM{'realname'}) {
		$query .= " or ";
	}
	if ($::FORM{'realname'}) {
		$query .= " profiles.realname like \'%$::FORM{'realname'}%\'";
	}
	$query .= " order by profiles.userid";
	SendSQL($query);
	my @row = ();

	# Print the results
 	print qq{
<CENTER>$sort_popup</CENTER>
<FORM NAME=changeform METHOD=get ACTION=editmembers.cgi>
<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=4 ALIGN=center>
<TR BGCOLOR="#BFBFBF">
};

	if ($::FORM{'multiple'}) {
		print "<TH ALIGN=left>Select</TH>\n";
	}

	print qq{
	<TH ALIGN=left>User ID</TH>
	<TH ALIGN=left>Email Address</TH>
	<TH ALIGN=left>Real Name</TH>
</TR>
};
	my $count = 0;
	while (@row = FetchSQLData()) {
		print qq{
<TR BGCOLOR="#ECECEC">
};

		if ($::FORM{'multiple'}) {
			print "<TD ALIGN=center><INPUT TYPE=checkbox NAME=\"user-$row[0]\"></TD>\n";
		}

		print qq{
	<TD ALIGN=center><A HREF="editmembers.cgi?\&userid=$row[0]">$row[0]</A>&nbsp;</TD>
	<TD ALIGN=left>$row[1]</TD>
};
		# For some reason netscape was choking when this value was NULL
		if (!$row[2]) {
			$row[2] = "&nbsp;";
		}
		print qq{    
	<TD ALIGN=left>$row[2]</TD>
</TR>
};

	}

	print qq{</TABLE>\n};

	if ($::FORM{'multiple'}) {
		print "<P><TABLE ALIGN=center>\n<TR>\n<TD ALIGN=left>\n";
		PutGroupForm();
		print "</TD>\n</TR>\n</TABLE>\n";
	}
	
	print qq{
<P>
<CENTER>
<A HREF="editmembers.cgi?submit=1&login_name=$::FORM{'login_name'}&realname=$::FORM{'realname'}&multiple=1&sort=$::FORM{'sort'}">
[Edit multiple members at once]</A> &nbsp;
<A HREF="editmembers.cgi">[Find another member]</A>
</CENTER>
};
	
	PutFooter();
	exit;
}

# single member selected so display information for changing
if ($::FORM{'userid'}) {
	# we made changes to a single member
	if ($::FORM{'update'}) {
		if ($::FORM{'password'} ne "" && $::FORM{'retype'} ne "") {
			if ($::FORM{'password'} ne $::FORM{'retype'}) {
				print qq{
<FONT SIZE=+1 COLOR=red>Sorry.</FONT><B>The passwords you entered were not the same.
Please click BACK and correct the error.</B>
};				
				exit;
			}
			my $password = $::FORM{'password'};
			my $encrypted = crypt($password, substr($password, 0, 2));
			SendSQL("update profiles set password = " . SqlQuote($password) .
					", cryptpassword = " . SqlQuote($encrypted) . 
					" where login_name = " . SqlQuote($::FORM{'login_name'}));
			MailPassword($::FORM{'login_name'}, $password, 1);	
			print "Password changed and member notified.<BR>\n";
		}

		SendSQL("update profiles set login_name = " . SqlQuote($::FORM{'login_name'}) .
				" where userid = " . $::FORM{'userid'});
		print "<CENTER><B>Login name updated.</B></CENTER><BR>\n";

		SendSQL("update profiles set realname = " . SqlQuote($::FORM{'realname'}) .
                " where userid = " . $::FORM{'userid'});
		print "<CENTER><B>Real name updated.</B></CENTER><BR>\n";

		if (UpdateGroups()) {
			print "<CENTER><B>Group membership updated.</B></CENTER><P>\n";
		}
	}
	
	SendSQL("select login_name, realname from profiles where userid = " . $::FORM{'userid'});
	my @user_info = FetchSQLData();	

	SendSQL("select groupid, description from groups order by groupid");
	my %grouplist;
	my @row;
	while (@row = FetchSQLData()) {
		$grouplist{$row[0]} = $row[1];
	}

	SendSQL("select groupid from user_group where userid = " . $::FORM{'userid'});
	my @groupbelong;
	while (@row = FetchSQLData()) {
		push (@groupbelong, $row[0]);
	}
	
	print qq{
<CENTER><H1>Information for <FONT COLOR=blue>$user_info[0]</FONT></H1></CENTER>
<P>	
<FORM NAME=changeform METHOD=post ACTION=editmembers.cgi>
<TABLE ALIGN=center CELLSPACING=0 CELLPADDING=4>
<TR>
	<TD ALIGN=right><B>User ID#: </B></TD>
	<TD ALIGN=left><B><FONT COLOR=blue>$::FORM{'userid'}</FONT></B></TD>
</TR><TR>
	<TD ALIGN=right><B>Email Address: </B></TD>
	<TD ALIGN=left><INPUT TYPE=text SIZE=40 MAXLENGTH=60 NAME=login_name VALUE="$user_info[0]"></TD>
</TR><TR>
	<TD ALIGN=right><B>Real Name: </B></TD>
	<TD ALIGN=left><INPUT TYPE=text SIZE=40 MAXLENGTH=60 NAME=realname VALUE="$user_info[1]"></TD>
</TR><TR>
	<TD ALIGN=right><B>New Password (8 chars.): </B></TD>
	<TD ALIGN=left><INPUT TYPE=password NAME=password></TD>
</TR><TR>
	<TD ALIGN=right><B>Retype New Password (8 chars.): </TD>
	<TD ALIGN=left><INPUT TYPE=password NAME=retype></TD>
</TR>
</TABLE>
<CENTER><B><FONT SIZE=+1 COLOR=red>Note:</FONT> If the current password is changed, it will be automatically
mailed out to the member.</B></CENTER>
<P>
<HR WIDTH=800>	
<TABLE ALIGN=center CELLSPACING=0 CELLPADDING=4>
<TR>
	<TH>Check which groups you wish this member to be in.</TH>
</TR><TR>
	<TD ALIGN=left>
	<INPUT TYPE=HIDDEN NAME=userid VALUE="$::FORM{'userid'}">
};	
	my $count = 0;
	foreach my $i (keys %grouplist) {
    	my $c;
    	if (lsearch(\@groupbelong, $i) >= 0) {
        	$c = 'CHECKED';
    	} else {
        	$c = '';
    	}
    	print qq{
	<INPUT TYPE=checkbox NAME=group-$i $c> $grouplist{$i}<br>
};
	}

    print "
<P>
<SCRIPT>
numelements = document.changeform.elements.length;
function SetCheckboxes(value) {
    for (var i=0 ; i<numelements ; i++) {
        item = document.changeform.elements\[i\];
        item.checked = value;
    }
}
document.write(\" <input type=button value=\\\"Uncheck All\\\" onclick=\\\"SetCheckboxes(false);\\\"> <input type=button value=\\\"Check All\\\" onclick=\\\"SetCheckboxes(true);\\\">\");
</SCRIPT>\n";

	print qq{
	</TD>
</TR>
</TABLE>
<P>
<CENTER>
<HR WIDTH=800>
<INPUT TYPE=submit NAME=update VALUE=" Update! ">
</FORM>
<A HREF="editmembers.cgi">[Edit another member]</A>
</CENTER>

};

	PutFooter();
	exit;
}

# we made group changes to multiple members
if ($::FORM{'update'} && $::FORM{'multiple'} eq '1') {
	if (UpdateGroups()) {
		print "<CENTER><B>Member group permissions successfully updated.</B></CENTER>\n";
		exit;
	} else {
		print "<CENTER><B>Member group updated unsuccessful!</B></CENTER>\n";
		exit;
	}
}

print qq{
<FORM <METHOD=post ACTION=editmembers.cgi>
<TABLE ALIGN=center>
<TR>
	<TD COLSPAN=2><U><B>Enter email address or partial real name of member you want to edit</B></U></TD>
</TR><TR>
	<TD ALIGN=right><B>Email Address: </B></TD>
	<TD ALIGN=left><INPUT TYPE=text SIZE=30 MAXLENGTH=40 NAME=login_name></TD>
</TR><TR>
	<TD ALIGN=right><B>Full Name: </B></TD>
	<TD ALIGN=left><INPUT TYPE=text SIZE=30 MAXLENGTH=40 NAME=realname></TD>
</TR><TR>
	<TD COLSPAN=2 ALIGN=center><INPUT TYPE=submit NAME=submit VALUE=Find></TD>
</TR>
</TABLE>
</FORM>
};

PutFooter();
exit;
