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
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dave Miller <dave@intrec.com>
#                 Joe Robins <jmrobins@tgix.com>
#				  David Lawrence <dkl@redhat.com>

########################################################################
#
# ::enter_bug.cgi
# -------------
# Displays bug entry form. Bug fields are specified through popup menus, 
# drop-down lists, or text fields. Default for these values can be passed
# in as parameters to the cgi.
#
########################################################################

use diagnostics;
use strict;

require "CGI.pl";

# Shut up misguided -w warnings about "used only once". "use vars" just
# doesn't work for me.

sub sillyness {
    my $zz;
    $zz = $::unconfirmedstate;
    $zz = @::legal_opsys;
    $zz = @::legal_platform;
    $zz = @::legal_priority;
    $zz = @::legal_severity;
}

# hash to hold values to be passed to the html fill in template
# my %::enter_bug;

confirm_login();

my $userid = 0;
if (defined ($::COOKIE{'Bugzilla_login'})) {
	$userid = DBname_to_id($::COOKIE{'Bugzilla_login'});
}

if (!defined $::FORM{'product'}) {
    GetVersionTable();

	my @prodlist = GenProductList($userid, keys %::versions);

    if (1 != @prodlist) {
        print "Content-type: text/html\n\n";
        PutHeader("Enter Bug");

        print "<CENTER><H3>First, did you <A HREF=\"query.cgi\">query</A> the current database of bugs";
		print "to see if your problem has already been reported?<P>";
		print "Also, have you checked to see if your problem has been fixed in a latest ";
		print "<A HREF=\"http://www.redhat.com/support/updates.html\">errata</A> updates?<P>"; 
		print "If it has not then pick a product or category on which to enter a bug.</H3></CENTER>";
		print "<TABLE ALIGN=center BORDER=1 CELLSPACING=0 CELLPADDING=3>\n";
		print "<TR BGCOLOR=\"#BFBFBF\">\n<TH ALIGN=left>Product</TH>\n";
		print "<TH ALIGN=left>Description</TH>\n</TR>\n";
        foreach my $p (sort (@prodlist)) {
            if (defined $::proddesc{$p} && $::proddesc{$p} eq '0') {
                # Special hack.  If we stuffed a "0" into proddesc, that means
                # that disallownew was set for this bug, and so we don't want
                # to allow people to specify that product here.
                next;
            }

            if(Param("usebuggroupsentry")
               && GroupExists($p)
               && !UserInGroup($p)) {
                # If we're using bug groups to restrict entry on products, and
                # this product has a bug group, and the user is not in that
                # group, we don't want to include that product in this list.
                next;
            }
			print "<TR BGCOLOR=\"#ECECEC\">";
			print "<TH ALIGN=left VALIGN=center><A HREF=\"enter_bug.cgi?product=" . url_quote($p);
			print "\">$p</A></TH>\n";
            if (defined $::proddesc{$p}) {
                print "<TD VALIGN=center ALIGN=left>$::proddesc{$p}</TD>\n";
            }
            print "</TR>";
        }
        print "</TABLE>\n";
        PutFooter();
		exit;
    }
    $::FORM{'product'} = $prodlist[0];
}

my $product = $::FORM{'product'};

print "Content-type: text/html\n\n";

sub formvalue {
    my ($name, $default) = (@_);
    if (exists $::FORM{$name}) {
        return $::FORM{$name};
    }
    if (defined $default) {
        return $default;
    }
    return "";
}


sub pickplatform {
    my $value = formvalue("rep_platform");
    if ($value ne "") {
        return $value;
    }
    if ( Param('usebrowserinfo') ) {
        for ($ENV{'HTTP_USER_AGENT'}) {
            /Mozilla.*\(Windows/ && do {return "PC";};
            /Mozilla.*\(Macintosh/ && do {return "Macintosh";};
            /Mozilla.*\(Win/ && do {return "PC";};
	        /Mozilla.*Windows NT/ && do {return "PC";};
            /Mozilla.*Linux.*86/ && do {return "i386";};
            /Mozilla.*Linux.*alpha/ && do {return "alpha";};
            /Mozilla.*OSF/ && do {return "DEC";};
            /Mozilla.*HP-UX/ && do {return "HP";};
            /Mozilla.*IRIX/ && do {return "SGI";};
            /Mozilla.*(SunOS|Solaris)/ && do {return "Sun";};
        }
    }
    # default
    return "Other";
}


sub pickversion {
    my $version = formvalue('version');

    if ( Param('usebrowserinfo') ) {
        if ($version eq "") {
            if ($ENV{'HTTP_USER_AGENT'} =~ m@Mozilla[ /]([^ ]*)@) {
                $version = $1;
            }
        }
    }
    
    if (lsearch($::versions{$product}, $version) >= 0) {
        return $version;
    } else {
        if (defined $::COOKIE{"VERSION-$product"}) {
            if (lsearch($::versions{$product},
                        $::COOKIE{"VERSION-$product"}) >= 0) {
                return $::COOKIE{"VERSION-$product"};
            }
        }
    }
    return $::versions{$product}->[0];
}


sub pickcomponent {
    my $result = formvalue('component');
    if ($result ne "" && lsearch($::components{$product}, $result) < 0) {
        $result = "";
    }
    return $result;
}


sub pickos {
    if (formvalue('op_sys') ne "") {
        return formvalue('op_sys');
    }
    if ( Param('usebrowserinfo') ) {
        for ($ENV{'HTTP_USER_AGENT'}) {
            /Mozilla.*\(.*;.*; IRIX.*\)/    && do {return "IRIX";};
            /Mozilla.*\(.*;.*; 32bit.*\)/   && do {return "Windows 95";};
            /Mozilla.*\(.*;.*; 16bit.*\)/   && do {return "Windows 3.1";};
            /Mozilla.*\(.*;.*; 68K.*\)/     && do {return "Mac System 8.5";};
            /Mozilla.*\(.*;.*; PPC.*\)/     && do {return "Mac System 8.5";};
            /Mozilla.*\(.*;.*; OSF.*\)/     && do {return "OSF/1";};
            /Mozilla.*\(.*;.*; Linux.*\)/   && do {return "Linux";};
            /Mozilla.*\(.*;.*; SunOS 5.*\)/ && do {return "Solaris";};
            /Mozilla.*\(.*;.*; SunOS.*\)/   && do {return "SunOS";};
            /Mozilla.*\(.*;.*; SunOS.*\)/   && do {return "SunOS";};
            /Mozilla.*\(.*;.*; BSD\/OS.*\)/ && do {return "BSDI";};
            /Mozilla.*\(Win16.*\)/          && do {return "Windows 3.1";};
            /Mozilla.*\(Win95.*\)/          && do {return "Windows 95";};
            /Mozilla.*\(Win98.*\)/          && do {return "Windows 98";};
            /Mozilla.*\(WinNT.*\)/          && do {return "Windows NT";};
        }
    }
    # default
    return "Linux";
#	return "other";
}

GetVersionTable();

$::enter_bug{'assign_element'} = "<A HREF=\"bug_status.cgi#assigned_to\"><B>Assigned To:</B></A></TD><TD>" . 
							   GeneratePeopleInput('assigned_to', formvalue('assigned_to'));

$::enter_bug{'cc_element'} = "<B>Cc:</B></TD><TD>" .
						   GeneratePeopleInput('cc', formvalue('cc'));

my $priority = Param('defaultpriority');

$::enter_bug{'severity_popup'} = "<A HREF=\"bug_status.cgi#bug_severity\"><B>Severity:</B></A></TD><TD>" . 
							   make_popup('bug_severity', \@::legal_severity,
                           	   formvalue('bug_severity', 'normal'), 0);

$::enter_bug{'platform_popup'} = "<A HREF=\"bug_status.cgi#rep_platform\"><B>Platform:<B></A></TD><TD>" . 
							   make_popup('rep_platform', \@::legal_platform,
                               pickplatform(), 0) ;

# $::enter_bug{'opsys_popup'} = "<A HREF=\"bug_status.cgi#op_sys\"><B>OpSys:</B></A></TD><TD>" . 
							make_popup('op_sys', \@::legal_opsys, pickos(), 0);
$::enter_bug{'opsys_popup'} = "<INPUT TYPE=hidden NAME=op_sys VALUE=\"" . value_quote(pickos()) ."\"></TD><TD>\n"; 

if (1 == @{$::components{$product}}) {
    # Only one component; just pick it.
    $::FORM{'component'} = $::components{$product}->[0];
}

$::enter_bug{'component_popup'} = "<A HREF=\"describecomponents.cgi?product=" . value_quote($product) . "\">" .
								"<B>Component:</B></A></TD><TD>" . 
								make_popup('component', $::components{$product},
                                formvalue('component'), 1);

$::enter_bug{'component_text'} = "<A HREF=\"bug_status.cgi#component\"><B>Component Text:</B></A></TD><TD>" .
							   GeneratePeopleInput('component_text', formvalue('component_text'));

PutHeader ("Enter Bug", "Enter Bug");
# PutHeader ("Enter Bug","Enter Bug","This page lets you enter a new bug into Bugzilla.");

# Modified, -JMR, 2/24,00
# If the usebuggroupsentry parameter is set, we need to check and make sure
# that the user has permission to enter a bug against this product.
if(Param("usebuggroupsentry")) {
  if(!UserInGroup($product)) {
    print "<H1>Permission denied.</H1>\n";
    print "Sorry; you do not have the permissions necessary to enter\n";
    print "a bug against this product.\n";
    print "<P>\n";
    PutFooter();
    exit;
  }
}

# Modified, -JMR, 2/18/00
# I'm putting in a select box in order to select whether to restrict this bug to
# the product's bug group or not, if the usebuggroups parameter is set, and if
# this product has a bug group.  This box will default to selected, but can be
# turned off if this bug should be world-viewable for some reason.
#
# To do this, I need to (1) get the bit and description for the bug group from
# the database, (2) insert the select box in the giant print statements below,
# and (3) update post_bug.cgi to process the additional input field.

# First we get the bit and description for the group.
my $group_bit=0;
my $group_desc;
if(Param("usebuggroups") && GroupExists($product)) {
    SendSQL("select bit, description from groups ".
            "where name = ".SqlQuote($product)." ".
            "and isbuggroup != 0");
    ($group_bit, $group_desc) = FetchSQLData();
}


if (Param("entryheaderhtml")){
	$::enter_bug{'entryheaderhtml'} = Param("entryheaderhtml"); 
}

$::enter_bug{'version_element'} = "<B>Version:</B></TD><TD>" . 
								Version_element(pickversion(), $product);
    
$::enter_bug{'component_describe'} = url_quote($product);

if (Param('letsubmitterchoosepriority')) {
    $::enter_bug{'priority_popup'} = "<B><A HREF=\"bug_status.cgi#priority\">Priority</A>:</B></TD><TD>" . 
								   make_popup('priority', \@::legal_priority,
    							   formvalue('priority', $priority), 0);
} else {
    $::enter_bug{'priority_popup'} = "<INPUT TYPE=HIDDEN NAME=priority VALUE=\"" .
        						   value_quote($priority) . "\"></TD><TD>\n";
}

$::enter_bug{'url_element'} = "<B>URL:</B></TD><TD>" . 
							GeneratePeopleInput('bug_file_loc', formvalue('bug_file_loc'));

$::enter_bug{'summary_element'} = "<B>Summary:</B></TD><TD>" .
								GeneratePeopleInput('short_desc', formvalue('short_desc'));

$::enter_bug{'comment'} = value_quote(formvalue('comment'));

if (UserInGroup("editbugs") || UserInGroup("canconfirm")) {
    SendSQL("SELECT votestoconfirm FROM products WHERE product = " .
            SqlQuote($product));
    if (FetchOneColumn()) {
        $::enter_bug{'initialstate_popup'} = qq{
    <B><A HREF="bug_status.html#status">Initial state:</B></A></TD>
    <TD>
};
        $::enter_bug{'initialstate_popup'} = BuildPulldown("bug_status",
                            			   [[$::unconfirmedstate], ["NEW"]],
                            			   "NEW");
    }
} else {
	$::enter_bug{'initialstate_popup'} = "<INPUT TYPE=hidden NAME=bug_status VALUE=\"NEW\">\n";
}

# Red Hat contract bug support
if (Param('contract') && UserInGroup('setcontract')) {
	$::enter_bug{'contract_checkbox'} = qq{
<INPUT TYPE="checkbox" NAME="iscontract" VALUE="1">&nbsp;<B>This is a contract bug</B></TD>
};

}

# In between the Description field and the Submit buttons, we'll put in the
# select box for the bug group, if necessary.
# Rather than waste time with another Param check and another database access,
# $group_bit will only have a non-zero value if we're using bug groups and have
# one for this product, so I'll check on that instead here.  -JMR, 2/18/00
if($group_bit && $::driver eq 'mysql') {
  # In addition, we need to handle the possibility that we're coming from
  # a bookmark template.  We'll simply check if we've got a parameter called
  # groupset passed with a value other than the current bit.  If so, then we're
  # coming from a template, and we don't have group_bit set, so turn it off.
  my $check0 = (formvalue("groupset",$group_bit) == $group_bit) ? "" : " SELECTED";
  my $check1 = ($check0 eq "") ? " SELECTED" : "";
  $::enter_bug{'group_select'} = "
  <table>
  <tr>
    <td align=right><B>Access:</td>
    <td colspan=5>
      <select name=\"groupset\">
        <option value=0$check0>
          People not in the \"$group_desc\" group can see this bug
        </option>
        <option value=$group_bit$check1>
          Only people in the \"$group_desc\" group can see this bug
        </option>
      </select>
    </td>
  </tr>
</table>";

} else {

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
		$::enter_bug{'group_select'} = qq{
<TABLE CELLSPACING=0 CELLPADDING=3 BORDER=1>
<TR BGCOLOR="#CFCFCF">
	<TD ALIGN=left><B>Groups that can see this bug.</B><BR> 
	(If all unchecked then same as everyone)</TD>
</TR><TR>
	<TD ALIGN=left>
};

		foreach my $group (keys %grouplist) {
			$::enter_bug{'group_select'} .= "<INPUT TYPE=checkbox NAME=group-$group VALUE=1>\n";
			$::enter_bug{'group_select'} .= "<B>$grouplist{$group}</B> can only see this bug.<BR>\n";
		}

		$::enter_bug{'group_select'} .= qq{
	</TD>
</TR>
</TABLE>
};

	}
}

if ( Param('usebrowserinfo') ) {
   $::enter_bug{'browserinfo'} = "
     Some fields initialized from your user-agent, 
     <B>$ENV{'HTTP_USER_AGENT'}</B>.  If you think it got it wrong, 
     please tell " . Param('maintainer') . " what it should have been.
";

}

foreach my $key (keys %::FORM) {
	$::enter_bug{$key} = $::FORM{$key};
}

# we should have enough now to fill in the template

print LoadTemplate('enterbug_redhat.tmpl', \%::enter_bug);

PutFooter();

