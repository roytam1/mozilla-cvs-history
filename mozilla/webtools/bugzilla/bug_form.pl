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
# 				  David Lawrence <dkl@redhat.com>

use diagnostics;
use strict;

# Shut up misguided -w warnings about "used only once".  For some reason,
# "use vars" chokes on me when I try it here.

sub bug_form_pl_sillyness {
    my $zz;
    $zz = %::FORM;
    $zz = %::components;
    $zz = %::prodmaxvotes;
    $zz = %::versions;
    $zz = @::legal_keywords;
    $zz = @::legal_opsys;
    $zz = @::legal_platform;
    $zz = @::legal_product;
    $zz = @::legal_priority;
    $zz = @::legal_resolution_no_dup;
    $zz = @::legal_severity;
	$zz = %::target_milestone;
}

my %bug_form;			# hash to hold variables passed on to the template
my $userid = 0;			# user id of current member
my $changeable = 0;		# whether the user can edit attributes of this bug

my $loginok = quietly_check_login();

if ($loginok) {
	$userid = DBname_to_id($::COOKIE{'Bugzilla_login'});
}

my $id = $::FORM{'id'}; # id number of current bug

my $query = "
select
        bugs.bug_id,
        product,
        version,
        rep_platform,
        op_sys,
        bug_status,
        resolution,
        priority,
        bug_severity,
        component,
        assigned_to,
        reporter,
        bug_file_loc,
        short_desc,
        target_milestone,
        qa_contact,
        status_whiteboard,
";

if ($::driver eq 'mysql') {
	$query .= "
        date_format(creation_ts,'%Y-%m-%d %H:%i'),
        groupset,
		delta_ts,
		sum(votes.count)
from 	bugs left join votes using(bug_id)
where 	bugs.bug_id = $id
and 	bugs.groupset & $::usergroupset = bugs.groupset
		group by bugs.bug_id";
} else {
    $query .= "
    TO_CHAR(creation_ts, 'YYYY-MM-DD HH:MI:SS'), 
    groupset,
    TO_CHAR(delta_ts, 'YYYYMMDDHH24MISS')
from
    bugs
where 
    bugs.bug_id = $id";
}

SendSQL($query);
my %bug;
my @row;
if (@row = FetchSQLData()) {
    my $count = 0;
    foreach my $field ("bug_id", "product", "version", "rep_platform",
		       "op_sys", "bug_status", "resolution", "priority",
		       "bug_severity", "component", "assigned_to", "reporter",
		       "bug_file_loc", "short_desc", "target_milestone",
               "qa_contact", "status_whiteboard", "creation_ts",
               "groupset", "delta_ts", "votes") {
		$bug{$field} = shift @row;
		if (!defined $bug{$field}) {
	    	$bug{$field} = "";
		}
		$count++;
    }
} else {
	if ($::driver eq 'mysql') {
	    SendSQL("select groupset from bugs where bug_id = $id");
	    if (@row = FetchSQLData()) {
    	    print "<H1>Permission denied.</H1>\n";
        	if ($loginok) {
            	print "Sorry; you do not have the permissions necessary to see\n";
            	print "bug $id.\n";
				PutFooter();
				exit;
        	} else {
            	print "Sorry; bug $id can only be viewed when logged\n";
            	print "into an account with the appropriate permissions.  To\n";
            	print "see this bug, you must first\n";
            	print "<a href=\"show_bug.cgi?id=$id&GoAheadAndLogIn=1\">";
            	print "log in</a>.";
				PutFooter();
				exit;
        	}
		}
	}
    PutError("<H1>Bug not found</H1>\n" .
    	 	 "There does not seem to be a bug numbered $id.\n");
}

if ($::driver ne 'mysql') {
	# If this bug has been set to private, lets see if we can view it.
	if (!CanISee($id, $userid)) {
    	my $error = "<H1>Permission denied.</H1>\n";
    	if ($loginok) {
        	$error .= "Sorry; you do not have the permissions necessary to see\n";
        	$error .= "bug $id.\n";
    	} else {
        	$error .= "Sorry; bug $id can only be viewed when logged\n";
        	$error .= "into an account with the appropriate permissions.  To\n";
        	$error .= "see this bug, you must first\n";
        	$error .= "<a href=\"show_bug.cgi?id=$id&GoAheadAndLogIn=1\">";
        	$error .= "log in</a>.";
    	}
		PutError($error);
	}
}



my $assignedtoid = $bug{'assigned_to'};
my $reporterid = $bug{'reporter'};
my $qacontactid =  $bug{'qa_contact'};

# If changeable is true then changes to bug's attributes are allowed
if ($::driver ne 'mysql') {
	if (CanIChange($id, $userid, $reporterid, $assignedtoid)) {
    	$changeable = 1;
	} else {
    	$changeable = 0;
	}
}

$bug{'assigned_to'} = DBID_to_name($bug{'assigned_to'});
$bug{'reporter'} = DBID_to_name($bug{'reporter'});

print qq{<FORM NAME="changeform" METHOD="POST" ACTION="process_bug.cgi">\n};

#  foreach my $i (sort(keys(%bug))) {
#      my $q = value_quote($bug{$i});
#      print qq{<INPUT TYPE="HIDDEN" NAME="orig-$i" VALUE="$q">\n};
#  }

$bug_form{'long_desc'} = GetLongDescriptionAsHTML($id);
$bug_form{'longdesclength'} = length($bug_form{'long_desc'});

GetVersionTable();

#
# These should be read from the database ...
#
$bug_form{'platform_popup'} = "";
$bug_form{'priority_popup'} = "";
$bug_form{'sev_popup'} = "";
$bug_form{'component_popup'} = "";
$bug_form{'component_text'} = "";
$bug_form{'cc_element'} = "";
$bug_form{'version_popup'} = "";
$bug_form{'product_popup'} = "";
$bug_form{'opsys_popup'} = "";

if ($changeable) {
    $bug_form{'platform_popup'} = "<SELECT NAME=rep_platform>\n" .
                      make_options(\@::legal_platform, $bug{'rep_platform'}) . 
                      "\n</SELECT>\n";

    # Added by Red Hat for contract customer support 
    if (Param('contract')) {
        if (UserInContract($userid) && UserInGroup('setcontract')) {
            $bug_form{'priority_popup'} = "<SELECT NAME=priority>\n" . 
                          make_options(\@::legal_priority_contract, $bug{'priority'}) . 
                              "\n</SELECT>\n";
        } elsif (UserInContract($userid) && $bug{'priority'} eq 'contract') {
            $bug_form{'priority_popup'} = "contract<BR>" . 
                              "<SELECT NAME=priority>\n" . 
                              make_options(\@::legal_priority, $bug{'priority'}) . 
                              "\n</SELECT>\n";
        } else {
            $bug_form{'priority_popup'} = "<SELECT NAME=priority>\n" . 
                          make_options(\@::legal_priority, $bug{'priority'}) . 
                          "\n</SELECT>\n";
        }
    } else {
        $bug_form{'priority_popup'} = "<SELECT NAME=priority>\n" . 
                          make_options(\@::legal_priority, $bug{'priority'}) . 
                          "\n</SELECT>\n";
    }

    $bug_form{'severity_popup'} = "<SELECT NAME=bug_severity>\n" .
                               make_options(\@::legal_severity, $bug{'bug_severity'}) .
                               "\n</SELECT>\n";

    $bug_form{'component_popup'} = "<B><A HREF=\"describecomponents.cgi?product=$bug{'product'}\">Component:</A></B>" .
                                     "<BR><B>$bug{'component'}</B></TD>\n<TD ROWSPAN=3 VALIGN=top>" .
                                     make_popup('component', $::components{$bug{'product'}}, $bug{'component'}, 1, 0);

#   $bug_form{'component_text'} = "<INPUT NAME=component_text SIZE=20 VALUE=\"\">"; 
    $bug_form{'component_text'} = "(Coming Soon)";

    $bug_form{'cc_element'} = "Cc:</TH><TD ALIGN=left><INPUT NAME=cc SIZE=60 VALUE=\"" . ShowCcList($id) . "\">"; 

    $bug_form{'version_popup'} = "<SELECT NAME=version>\n" .
                                    make_options($::versions{$bug{'product'}}, $bug{'version'}) .
                                    "\n</SELECT>\n";

    $bug_form{'product_popup'} = "<SELECT NAME=product>\n" .
                                    make_options(\@::legal_product, $bug{'product'}) .
                                    "\n</SELECT>\n";
	$bug_form{'opsys_popup'} = "<SELECT NAME=op_sys>\n" .
            						make_options(\@::legal_opsys, $bug{'op_sys'}) .
            						"</SELECT>\n";

} else {
    $bug_form{'platform_popup'} = $bug{'rep_platform'};
    $bug_form{'priority_popup'} = $bug{'priority'};
    $bug_form{'sev_popup'} = $bug{'bug_severity'};
    $bug_form{'component_popup'} = "<B><A HREF=\"describecomponents.cgi?product=$bug{'product'}\">Component:</A></B>" .
                                     "</TD><TD ROWSPAN=3 VALIGN=top>" . $bug{'component'};
    $bug_form{'component_text'} = "";
    $bug_form{'cc_element'} = "Cc:</TH><TD ALIGN=left>" . ShowCcList($id);
    $bug_form{'version_popup'} = $bug{'version'};
    $bug_form{'product_popup'} = $bug{'product'};
	$bug_form{'opsys_popup'} = $bug{'op_sys'};
}


$bug_form{'resolution_popup'} = make_options(\@::legal_resolution_no_dup,
				    $bug{'resolution'});

my $URL = $bug{'bug_file_loc'};
if (defined $URL && $URL ne "none" && $URL ne "NULL" && $URL ne "") {		
	$URL = "<B><A HREF=\"$URL\">URL:</A></B>";
} else {
	$URL = "<B>URL:</B>";
}

if (Param("usetargetmilestone")) {
	my $url = "";
	if (defined $::milestoneurl{$bug{'product'}}) {
		$url = $::milestoneurl{$bug{'product'}};
	}
	if ($url eq "") {
		$url = "notargetmilestone.html";
	}
	if ($bug{'target_milestone'} eq "") {
		$bug{'target_milestone'} = " ";
	}
	push(@::legal_target_milestone, " ");
	$bug_form{'milestone_popup'} = "
<A href=\"$url\"><B>Target Milestone:</B></A></TD>
<TD><SELECT NAME=target_milestone>\n" .
make_options(\@::legal_target_milestone,
$bug{'target_milestone'}) .
"</SELECT>\n";
}

if (Param("useqacontact")) {
	my $name = $bug{'qa_contact'} > 0 ? DBID_to_name($bug{'qa_contact'}) : "";
	if ($changeable) {
	    $bug_form{'qacontact_element'} = "QA Contact:</TH><TD ALIGN=left><INPUT NAME=qa_contact VALUE=\"" .
									   value_quote($name) . "\" SIZE=60>";
	} else {
		$bug_form{'qacontact_element'} = "QA Contact:</TH><TD ALIGN=left>" . value_quote($name);
	}
}

if ($changeable) {
	$bug_form{'url_element'} = "$URL</TH><TD ALIGN=left><INPUT NAME=bug_file_loc VALUE=\"$bug{'bug_file_loc'}\" SIZE=60>";
} else {
    $bug_form{'url_element'} = "$URL</TH><TD ALIGN=left>$bug{'bug_file_loc'}";
}

if ($changeable) {
    $bug_form{'summary_element'} = "Summary:</TH><TD COLSPAN=2 ALIGN=left><INPUT NAME=short_desc VALUE=\"" .
									 value_quote($bug{'short_desc'}) . "\" SIZE=60>";
} else {
    $bug_form{'summary_element'} = "Summary:</TH><TD ALIGN=left>$bug{'short_desc'}";
}

if (Param("usestatuswhiteboard")) {
    if ($changeable) {
    	$bug_form{'whiteboard_element'} = "Status Whiteboard:</TH>" . 
			"<TD ALIGN=left><INPUT NAME=status_whiteboard VALUE=\"" .
			value_quote($bug{'status_whiteboard'}) . "\" SIZE=60>";
    } else {
    	$bug_form{'whiteboard_element'} = "Status Whiteboard:</TH>" .
			"<TD ALIGN=left>$bug{'status_whiteboard'}";
    }
}

if (@::legal_keywords) {
    SendSQL("SELECT keyworddefs.name 
             FROM keyworddefs, keywords
             WHERE keywords.bug_id = $id AND keyworddefs.id = keywords.keywordid
             ORDER BY keyworddefs.name");
    my @list;
    while (MoreSQLData()) {
        push(@list, FetchOneColumn());
    }
    my $value = value_quote(join(', ', @list));
    $bug_form{'keywords_element'} = "
	<A HREF=\"describekeywords.cgi\">Keywords</A>:</TH>
	<TD ALIGN=left><INPUT NAME=\"keywords\" VALUE=\"$value\" SIZE=60>\n";
}

$bug_form{'attachment_element'} = "<TABLE WIDTH=\"100%\">\n";

if ($::driver eq 'mysql') {
	SendSQL("select attach_id, filename, date_format(creation_ts, '%Y-%m-%d %h:%i:%s'), " . 
			"description from attachments where bug_id = $id");
} else {
	SendSQL("select attach_id, filename, TO_CHAR(creation_ts, 'YYYY-MM-DD HH:MI:SS'), " . 
			"description from attachments where bug_id = $id");
}

while (MoreSQLData()) {
    my ($attachid, $filename, $date, $desc) = (FetchSQLData());
#   if ($date =~ /^(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)$/) {
#   	$date = "$3/$4/$2 $5:$6";
#   }	
    my $link = "showattachment.cgi?attach_id=$attachid";
    $desc = value_quote($desc);
    $bug_form{'attachment_element'} .= qq{
	<TR>
		<TD>$date</TD><TD><A HREF="$link">$filename</A></TD><TD>$desc</TD>
	</TR>
};
		
}

if ($changeable) {
	$bug_form{'attachment_element'} .= qq{
	<TR>
		<TD COLSPAN=6><A HREF="createattachment.cgi?id=$id"> 
		Create a new attachment</A> (proposed patch, testcase, etc.)</TD>
	</TR>
};
}

$bug_form{'attachment_element'} .= "</TABLE>\n";


sub EmitDependList {
    my ($desc, $myfield, $targetfield) = (@_);
	my $depends;
    $depends = "<TD ALIGN=right>$desc:</TD><TD>";
    my @list;
    SendSQL("select dependencies.$targetfield, bugs.bug_status " .
 			"from dependencies, bugs " . 
 			"where dependencies.$myfield = $id " .
   			"and bugs.bug_id = dependencies.$targetfield " .
			"order by dependencies.$targetfield");
    while (MoreSQLData()) {
        my ($i, $stat) = (FetchSQLData());
        push(@list, $i);
        my $opened = ($stat eq "NEW" || $stat eq "ASSIGNED" ||
                      $stat eq "REOPENED");
        if (!$opened) {
            $depends .= "<STRIKE>";
        }
        $depends .= qq{<A HREF="show_bug.cgi?id=$i">$i</A>};
        if (!$opened) {
            $depends .= "</STRIKE>";
        }
        $depends .= " ";
    }
	if ($changeable) {
	    $depends .= "</TD><TD><INPUT NAME=$targetfield VALUE=\"" .
    			    join(',', @list) . "\"></TD>\n";
	} else {
		$depends .= "</TD>\n";
	}	
	return $depends;
}

if (Param("usedependencies")) {
    $bug_form{'depends_element'} = "<TABLE><TR>\n<TH ALIGN=right>Dependencies:</TH>\n";
    $bug_form{'depends_element'} .= EmitDependList("Bug $id depends on", "blocked", "dependson");
    $bug_form{'depends_element'} .= qq{
<TD ROWSPAN=2><A HREF="showdependencytree.cgi?id=$id">Show dependency tree</A>
};
    if (Param("webdotbase") ne "") {
        $bug_form{'depends_element'} .= qq{
<BR><A HREF="showdependencygraph.cgi?id=$id">Show dependency graph</A>
};
    }
    $bug_form{'depends_element'} .= "</TD></TR><TR><TD></TD>";
    $bug_form{'depends_element'} .= EmitDependList("Bug $id blocks", "dependson", "blocked");
    $bug_form{'depends_element'} .= "</TR></TABLE>\n";
}

if ($::prodmaxvotes{$bug{'product'}}) {
    $bug_form{'votes_element'} = qq{
<TABLE><TR>
<TH><A HREF="votehelp.html">Votes</A> for bug $id:</TH><TD>
<A HREF="showvotes.cgi?bug_id=$id">$bug{'votes'}</A>
&nbsp;&nbsp;&nbsp;<A HREF="showvotes.cgi?voteon=$id">Vote for this bug</A>
</TD></TR></TABLE>
};
}

if ($::driver eq 'mysql') {
	if ($::usergroupset ne '0') {
    	SendSQL("select bit, description, (bit & $bug{'groupset'} != 0) from groups where bit & $::usergroupset != 0 and isbuggroup != 0 order by bit");
    	while (MoreSQLData()) {
        	my ($bit, $description, $ison) = (FetchSQLData());
        	my $check0 = !$ison ? " SELECTED" : "";
        	my $check1 = $ison ? " SELECTED" : "";
        	print "<SELECT NAME=bit-$bit><OPTION VALUE=0$check0>\n";
        	print "People not in the \"$description\" group can see this bug\n";
        	print "<OPTION VALUE=1$check1>\n";
        	print "Only people in the \"$description\" group can see this bug\n";
        	print "</SELECT><BR>\n";
    	}
	}
}


if ($changeable) {
    # Find the default owner so user will know who assigning to
    # if they choose the 'Assign to owner of selected component'
    my $query = "select initialowner from components " .
            "where program = " . SqlQuote($bug{'product'}) .
            " and value = " . SqlQuote($bug{'component'});
    SendSQL($query);
    my $initial_owner = "(" . FetchOneColumn() . ")";

    $bug_form{'resolution_change'} = qq{
    <TABLE CELLSPACING=0 CELLPADDING=3 BORDER=1 WIDTH=100%>
    <TR BGCOLOR="#CFCFCF">
        <TD ALIGN=left><B>Change State or Resolution</B>
        <BR>(Allowed only by reporter and privileged members)</TD>
    </TR><TR BGCOLOR="#ECECEC">
        <TD ALIGN=left>
        <INPUT TYPE=radio NAME=knob VALUE=none CHECKED>
            Leave as <b>$bug{'bug_status'} $bug{'resolution'}</b><br>
};

	# knum is which knob number we're generating, in javascript terms.
    my $knum = 1;

    my $status = $bug{'bug_status'};

    if ($status eq "NEW" || $status eq "ASSIGNED" || $status eq "REOPENED") {
        if ($status ne "ASSIGNED") {
            $bug_form{'resolution_change'} .= "<INPUT TYPE=radio NAME=knob VALUE=accept>" .
                  "Accept bug (change status to <b>ASSIGNED</b>)<br>";
            $knum++;
        }
        if ($bug{'resolution'} ne "") {
            $bug_form{'resolution_change'} .= "<INPUT TYPE=radio NAME=knob VALUE=clearresolution>\n" .
                "Clear the resolution (remove the current resolution of\n" .
                "<b>$bug{'resolution'}</b>)<br>\n";
            $knum++;
        }
        $bug_form{'resolution_change'} .= "<INPUT TYPE=radio NAME=knob VALUE=resolve>" .
               "Resolve bug, changing <A HREF=\"bug_status.cgi\">resolution</A> to " .
               "<SELECT NAME=resolution " .
               "ONCHANGE=\"document.changeform.knob\[$knum\].checked=true\"> " .
               $bug_form{'resolution_popup'} . "</SELECT><br>\n";
        $knum++;
        $bug_form{'resolution_change'} .= "<INPUT TYPE=radio NAME=knob VALUE=duplicate> " .
            "Resolve bug, mark it as duplicate of bug # " .
            "<INPUT NAME=dup_id SIZE=6 ONCHANGE=\"document.changeform.knob\[$knum\].checked=true\">\n" .
            "(<A HREF=\"buglist.cgi?component=$bug{'component'}\" target=\"new_window\">$bug{'component'} bugs</A>)<BR>\n";
        $knum++;
        my $assign_element = "<INPUT NAME=\"assigned_to\" SIZE=25 MAXSIZE=50 ONCHANGE=\"document.changeform.knob\[$knum\].checked=true\" VALUE=\"$bug{'assigned_to'}\">";

        $bug_form{'resolution_change'} .= "<INPUT TYPE=radio NAME=knob VALUE=assign> " .
            "<A HREF=\"bug_status.cgi#assigned_to\">Assign</A> bug to $assign_element <BR>\n";
        $knum++;
        $bug_form{'resolution_change'} .= "<INPUT TYPE=radio NAME=knob VALUE=assignbycomponent> " .
            "Assign bug to owner of selected component &nbsp;$initial_owner<br>\n";
        $knum++;
    } else {
		$bug_form{'resolution_change'} .= "<INPUT TYPE=hidden NAME=assigned_to VALUE=\"$bug{'assigned_to'}\">\n";
        $bug_form{'resolution_change'} .= "<INPUT TYPE=radio NAME=knob VALUE=reopen> Reopen bug<br>\n";
        $knum++;
        if ($status eq "RESOLVED") {
            $bug_form{'resolution_change'} .= "<INPUT TYPE=radio NAME=knob VALUE=verify> " .
                "Mark bug as <b>VERIFIED</b><br>\n";
            $knum++;
        }
        if ($status ne "CLOSED") {
            $bug_form{'resolution_change'} .= "<INPUT TYPE=radio NAME=knob VALUE=close> " .
                "Mark bug as <b>CLOSED</b><br>\n";
            $knum++;
        }
    }

    $bug_form{'resolution_change'} .= "
        </TD>
    </TR>
    </TABLE>
";

    # Find out which groups we are a member of and form radio buttons
    SendSQL("select user_group.groupid, groups.description, groups.isbuggroup " .
            "from user_group, groups " .
            "where user_group.groupid = groups.groupid " .
            "and user_group.userid = $userid order by groups.groupid");
    my %grouplist;
    my @buggrouplist;
    my @row;
    my $flag = 0;
    while (@row = FetchSQLData()) {
        if ($row[2] == 0) {
            next;
        }
        $grouplist{$row[0]} = $row[1];
        $flag = 1;
    }

    if ($flag) {
        $bug_form{'group_change'} = "
    <TABLE CELLSPACING=0 CELLPADDING=3 BORDER=1>
    <TR BGCOLOR=\"#CFCFCF\">
        <TD ALIGN=left><B>Groups that can see this bug.</B><BR> 
            (If all unchecked then same as everyone)</TD>
    </TR><TR BGCOLOR=\"ECECEC\">
        <TD ALIGN=left>
";

        SendSQL("select groupid from bug_group where bugid = $id");
        while (@row = FetchSQLData()) {
            push (@buggrouplist, $row[0]);
        }

        foreach my $group (keys %grouplist) {
            my $checked = lsearch(\@buggrouplist, $group) >= 0 ? "CHECKED" : "";
            $bug_form{'group_change'} .= "<INPUT TYPE=checkbox NAME=group-$group $checked VALUE=1>\n" .
                "Only <B>$grouplist{$group}</B> can see this bug.<BR>\n";
        }

        $bug_form{'group_change'} .= "
        </TD>
    </TR>
    </TABLE>
";
    }
}

# added <INPUT TYPE=hidden NAME=reporter VALUE=$bug{'reporter'}> to get the necessary
# redhat changes in process_bug.cgi to work which disallows all users to make changes 

if ($changeable) {
    $bug_form{'commit_change'} = "
    <TABLE CELLSPACING=0 CELLPADDING=3 BORDER=1>
    <TR BGCOLOR=\"#CFCFCF\">
        <TD ALIGN=left><B>Private Changes</B><BR>
        (Reporter, assigned, or privileged members only)</TD>
    </TR><TR BGCOLOR=\"#ECECEC\">
        <TD ALIGN=center>
        <TABLE> 
        <TR>
            <TD ALIGN=center VALIGN=top>    
            <INPUT TYPE=\"submit\" VALUE=\"Save Changes\">
            </TD><TD ALIGN=center VALIGN=top>
            <INPUT TYPE=\"reset\" VALUE=\"Reset\">
            <INPUT TYPE=hidden name=form_name VALUE=process_bug>
            <INPUT TYPE=hidden NAME=reporter VALUE=\"$bug{'reporter'}\">
            </FORM>
            </TD>
";

    if (UserInGroup("errata")) {
        $bug_form{'commit_change'} .= "
        <TD ALIGN=center VALIGN=top>
        <FORM ACTION=\"http://porkchop.redhat.com/bugzilla/newerrata.cgi\">
        <INPUT TYPE=hidden NAME=product VALUE=\"$bug{'product'}\">
        <INPUT TYPE=hidden NAME=synopsis VALUE=\"$bug{'component'}\">
        <INPUT TYPE=hidden NAME=id_fixed VALUE=\"$::FORM{'id'}\">
        <INPUT TYPE=hidden NAME=\"rep_platform-$bug{'rep_platform'}\" VALUE=\"$bug{'rep_platform'}\">
        <INPUT TYPE=hidden NAME=\"release-$bug{'version'}\" VALUE=\"$bug{'version'}\">
        <INPUT TYPE=submit NAME=action VALUE=\"New Errata\">
        </FORM>
        </TD>
";
    }
    $bug_form{'commit_change'} .= "
    </TR>
    </TABLE>
";

} else {
    $bug_form{'commit_change'} = "
    <TABLE CELLSPACING=0 CELLPADDING=3 BORDER=1>
    <TR BGCOLOR=\"CFCFCF\">
        <TD ALIGN=left>
        <B>Public Changes</B><BR>(Non-members will need to create account)</TD>
    </TR><TR BGCOLOR=\"#ECECEC\"> 
        <TD>
        <TABLE CELLSPACING=0 CELLPADDING=0 WIDTH=\"100%\">
        <TR>
            <TD ALIGN=center VALIGN=center>
			<INPUT TYPE=hidden NAME=id VALUE=$id>
            <INPUT TYPE=hidden NAME=product VALUE=\"$bug{'product'}\">
            <INPUT TYPE=hidden NAME=version VALUE=\"$bug{'version'}\">
            <INPUT TYPE=hidden NAME=component VALUE=\"$bug{'component'}\"> 
			<INPUT TYPE=hidden name=knob VALUE=add_comment>	
            <INPUT TYPE=submit NAME=add_comment VALUE=\"Add Comment\">
            </FORM></TD>
            <TD ALIGN=center VALIGN=center> 
            <FORM METHOD=post ACTION=process_bug.cgi>
            <INPUT TYPE=hidden NAME=id VALUE=$id>
            <INPUT TYPE=hidden NAME=product VALUE=\"$bug{'product'}\">
            <INPUT TYPE=hidden NAME=version VALUE=\"$bug{'version'}\">
            <INPUT TYPE=hidden NAME=component VALUE=\"$bug{'component'}\">
            <INPUT TYPE=hidden name=knob VALUE=add_cc>
            <INPUT TYPE=submit NAME=add_cc VALUE=\"Add Me to CC List\">
            </FORM></TD>
            <TD ALIGN=center VALIGN=center>
            <FORM METHOD=post ACTION=process_bug.cgi>
            <INPUT TYPE=hidden NAME=id VALUE=$id>
            <INPUT TYPE=hidden NAME=product VALUE=\"$bug{'product'}\">
            <INPUT TYPE=hidden NAME=version VALUE=\"$bug{'version'}\">
            <INPUT TYPE=hidden NAME=component VALUE=\"$bug{'component'}\">
            <INPUT TYPE=hidden name=knob VALUE=rem_cc>
            <INPUT TYPE=submit NAME=add_cc VALUE=\"Remove Me from CC List\">
            </FORM>
            </TD>
        </TR>
        </TABLE>
    </TR>
    </TABLE>
";
}


# We need to throw all remaining %bug variables into %bug_form for display if needed
foreach my $key (keys %bug) {
    $bug_form{$key} = $bug{$key};
}
foreach my $key (keys %::FORM) {
    $bug_form{$key} = $::FORM{$key};
}

# we can now fill in the bug form template
print LoadTemplate('bugform_redhat.tmpl', \%bug_form);
 
# To add back option of editing the long description, insert after the above
# long_list.cgi line:
#  <A HREF=\"edit_desc.cgi?id=$id\">Edit Long Description</A>

1;
