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
# The Original Code is the Bugzilla Bug Tracking System.
# 
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# Contributor(s): Terry Weissman <terry@mozilla.org>

use diagnostics;
use strict;

use vars @::versions,
	@::components,
	%::legal_priority,
	%::legal_severity,
	%::legal_resolution_no_dup,
	%::legal_classes;

my $canidoanything = 0;
my $pre = Param("prefix");
require "$pre-security.pl";


my $query = "
select
        bug_id,
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
        class,
        date_format(creation_ts,'Y-m-d')
from bugs
where bug_id = '" . $::FORM{'id'} . "'";

my $view_query = "SELECT type_id FROM type where name = 'public' ";
SendSQL($view_query);
my $type = FetchOneColumn();
$view_query = " and view = " . $type;
if (CanIView("view")){
        $view_query = "";
}
$query .= $view_query;

#print "<PRE>$query</PRE>\n";

SendSQL($query);

my %bug;
my @row = "";
if (@row = FetchSQLData()) {
    my $count = 0;
    foreach my $field ("bug_id", "product", "version", "rep_platform",
		       "op_sys", "bug_status", "resolution", "priority",
		       "bug_severity", "component", "assigned_to", "reporter",
		       "bug_file_loc", "short_desc", "class", "creation_ts") {
	$bug{$field} = shift @row;
	if (!defined $bug{$field}) {
	    $bug{$field} = "";
	}
	$count++;
    }
    PutHeader("Bugzilla bug $::FORM{'id'}", "Bugzilla Bug", $::FORM{'id'});
    navigation_header();

    # print "<PRE>$query</PRE>\n";
    print "<HR>\n";
} else {
    PutHeader("Query Error");
    print "Bug $::FORM{'id'} not found\n";
    exit 0;
}

my $source = "";
SendSQL("SELECT sources.source FROM sources WHERE sources.bug_id = '" . $::FORM{'id'} . "'");
if ($source = FetchOneColumn()) {
    $bug{'source'} = $source;
}

$bug{'assigned_to'} = DBID_to_name($bug{'assigned_to'});
$bug{'reporter'} = DBID_to_name($bug{'reporter'});
$bug{'long_desc'} = GetLongDescription($::FORM{'id'});

GetVersionTable();

#
# These should be read from the database ...
#

my $resolution_popup = make_options(\@::legal_resolution_no_dup, $bug{'resolution'}, 0);
my $version_popup = "<INPUT TYPE=\"HIDDEN\" NAME=\"version\" VALUE=\"$bug{'version'}\">$bug{'version'}";
my $platform_popup = "<INPUT TYPE=\"HIDDEN\" NAME=\"rep_platform\" VALUE=\"$bug{'rep_platform'}\">$bug{'rep_platform'}";
my $priority_popup = "<INPUT TYPE=\"HIDDEN\" NAME=\"priority\" VALUE=\"$bug{'priority'}\">$bug{'priority'}";
my $class_popup = "<INPUT TYPE=\"HIDDEN\" NAME=\"class\" VALUE=\"$bug{'class'}\">$bug{'class'}";
my $source_popup = "<INPUT TYPE=\"HIDDEN\" NAME=\"source\" VALUE=\"$bug{'source'}\">$bug{'source'}";
my $sev_popup = "<INPUT TYPE=\"HIDDEN\" NAME=\"bug_severity\" VALUE=\"$bug{'bug_severity'}\">$bug{'bug_severity'}";
my $component_popup = "<INPUT TYPE=\"HIDDEN\" NAME=\"component\" VALUE=\"$bug{'component'}\">$bug{'component'}";
my $cc_element = ShowCcList($::FORM{'id'});
my $URLBlock = $bug{'bug_file_loc'};
my $SummaryBlock = "<TD ALIGN=\"RIGHT\"><B>Summary:</B>";
my $AdditionalCommentsBlock = "<br>\n";
my $StatusBlock = "<br>\n";

if (CanIEdit("bug_status", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$resolution_popup = make_options(\@::legal_resolution_no_dup, $bug{'resolution'}, 0);
}

if (CanIEdit("version", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$version_popup = "<SELECT NAME=\"version\">" . make_options($::versions{$bug{'product'}}, $bug{'version'}, 0) . "</SELECT>";
}

if (CanIEdit("rep_platform", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$platform_popup = "<SELECT NAME=\"rep_platform\">" . make_options($::legal_platforms{$bug{'product'}}, $bug{'rep_platform'}, 0) . "</SELECT>";
}

if (CanIEdit("priority", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$priority_popup = "<SELECT NAME=\"priority\">" . make_options(\@::legal_priority, $bug{'priority'}, 0) . "</SELECT>";
}

if (CanIEdit("class", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$class_popup = "<SELECT NAME=\"class\">" . make_options(\@::legal_classes, $bug{'class'}, 0) . "</SELECT>";
}

if (CanIEdit("source", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$source_popup = "<SELECT NAME=\"source\">" . make_options(\@::legal_sources, $bug{'source'}, 0) . "</SELECT>";
}

if (CanIEdit("bug_severity", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$sev_popup = "<SELECT NAME=\"bug_severity\">" . make_options(\@::legal_severity, $bug{'bug_severity'}, 0) . "</SELECT>";
}

if (CanIEdit("component", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$component_popup = "<SELECT NAME=\"component\">" . make_options($::components{$bug{'product'}}, $bug{'component'}, 0) . "</SELECT>";
}

if (CanIEdit("cc", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$cc_element = '<INPUT NAME="cc" SIZE="60" VALUE="' .
   		ShowCcList($::FORM{'id'}) . '">';
}

if (CanIEdit("bug_file_loc", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$URLBlock = $bug{'bug_file_loc'};
	if (defined $URLBlock && $URLBlock ne "none" && $URLBlock ne "NULL" && $URLBlock ne "") {
		$URLBlock =  "<TD ALIGN=\"RIGHT\"><B><A HREF=\"$URLBlock\">URL:</A></B>\n";
		$URLBlock .= "<TD COLSPAN=\"6\"><INPUT NAME=\"bug_file_loc\" VALUE=\"$bug{'bug_file_loc'}\" SIZE=\"60\"></TD>\n";
	} else {
		$URLBlock =  "<TD ALIGN=\"RIGHT\"><B>URL:</B>\n";
		$URLBlock .= "<TD COLSPAN=\"6\"><INPUT NAME=\"bug_file_loc\" SIZE=\"60\"></TD>\n";
	}
} else {
	if (defined $URLBlock && $URLBlock ne "none" && $URLBlock ne "NULL" && $URLBlock ne "") {
		$URLBlock =  "<TD ALIGN=\"RIGHT\"><B>URL:</B></TD>\n";
		$URLBlock .= "<INPUT TYPE=\"HIDDEN\" NAME=\"bug_file_loc\" VALUE=\"$bug{'bug_file_loc'}\">\n";
		$URLBlock .= "<TD COLSPAN=\"6\"><A HREF=\"$bug{'bug_file_loc'}\">$bug{'bug_file_loc'}</A></TD>\n";
	}
}

if (CanIEdit("short_desc", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$SummaryBlock .= "<TD COLSPAN=\"6\"><INPUT NAME=\"short_desc\" VALUE=\"" .  value_quote($bug{'short_desc'}) .  "\" SIZE=\"60\"></TD>";
} else {
	$SummaryBlock .= "<TD COLSPAN=\"6\">&nbsp;" . value_quote($bug{'short_desc'}) . "</TD>\n";
	$SummaryBlock .= "<INPUT TYPE=\"HIDDEN\" NAME=\"short_desc\" VALUE=\"$bug{'short_desc'}\">\n";
}

if (CanIEdit("long_desc", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	$AdditionalCommentsBlock .= "<B>Additional Comments:</B>\n<BR>\n<TEXTAREA WRAP=\"HARD\" NAME=\"comment\" ROWS=\"5\" COLS=\"80\"></TEXTAREA><BR>\n";
}

print "
<HEAD><TITLE>Bug $::FORM{'id'} -- " . html_quote($bug{'short_desc'}) .
    "</TITLE></HEAD><BODY>
<FORM NAME=\"changeform\" METHOD=\"POST\" ACTION=\"process_bug.cgi\">
<INPUT TYPE=\"HIDDEN\" NAME=\"id\" VALUE=\"$::FORM{'id'}\">
<INPUT TYPE=\"HIDDEN\" NAME=\"was_assigned_to\" VALUE=\"$bug{'assigned_to'}\">
<INPUT TYPE=\"HIDDEN\" NAME=\"product\" VALUE=\"$bug{'product'}\">
  <TABLE CELLSPACING=\"0\" CELLPADDING=\"0\" BORDER=\"0\"><TR>
    <TD ALIGN=\"RIGHT\"><B>Bug#:</B></TD><TD>&nbsp;$bug{'bug_id'}</TD>
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml#rep_platform\">Architecture:</A></B></TD>
    <TD>&nbsp;$platform_popup</TD>
    <TD ALIGN=\"RIGHT\"><B>Version:</B></TD>
    <TD>&nbsp;$version_popup</TD>
  </TR><TR>
    <TD ALIGN=\"RIGHT\"><B>Product:</B></TD>
    <TD>&nbsp;$bug{'product'}</TD>
    <TD ALIGN=\"RIGHT\"><B>Reporter:</B></TD>
    <TD COLSPAN=\"3\">&nbsp;$bug{'reporter'}</TD>
  </TR><TR>
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml\">Status:</A></B></TD>
      <TD>&nbsp;$bug{'bug_status'}</TD>
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml#priority\">Priority:</A></B></TD>
      <TD>&nbsp;$priority_popup</TD>";

if ($bug{'bug_status'} ne "NEW") {
    print "
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml#class\">Class:</A></B></TD>
      <TD>&nbsp;$class_popup</TD>\n";
}

print "
  </TR><TR>
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml\">Resolution:</A></B></TD>
      <TD>&nbsp;$bug{'resolution'}</TD>
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml#severity\">Severity:</A></B></TD>
      <TD>&nbsp;$sev_popup</TD>
    <TD ALIGN=\"RIGHT\"><B>Component:</B></TD>
      <TD>&nbsp;$component_popup</TD>
  </TR><TR>
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml#assigned_to\">Assigned&nbsp;To:</A></B></TD>
      <TD COLSPAN=\"5\">&nbsp;$bug{'assigned_to'}</TD>
  </TR><TR>
    <TD ALIGN=\"RIGHT\"><B>Cc:&nbsp</B></TD>
      <TD COLSPAN=\"5\">$cc_element </TD>
  </TR><TR>
    $URLBlock
  </TR><TR>
    $SummaryBlock
  </TR>
</TABLE>
$AdditionalCommentsBlock
<br>";

# knum is which knob number we're generating, in javascript terms.

my $knum = 1;

my $status = $bug{'bug_status'};

if ( $status eq "NEW" && CanIEdit("bug_status", $bug{'reporter'}, $bug{'bug_id'})) {
    print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"none\" CHECKED>";
    print "Leave as <B>$status</B><BR>\n";
    print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"verify\">";
    print "<B>VERIFY</B> bug as $class_popup<BR>\n";
} elsif ($status eq "VERIFIED"||$status eq "ASSIGNED"||$status eq "REOPENED") {
    print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"none\" CHECKED>";
    print "Leave as <B>$status</B><BR>\n";
    if (CanIEdit("assigned_to", $bug{'reporter'}, $bug{'bug_id'})) {
        $canidoanything = 1;
        my $assign_element = "<INPUT NAME=\"assigned_to\" SIZE=\"32\" "
	    . "ONCHANGE=\"document.changeform.knob\[$knum\].checked=true\" "
	    . "VALUE=\"$bug{'assigned_to'}\">";
        print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"reassign\"> 
              <A HREF=\"bug_status.phtml#assigned_to\">Assign</A> bug to
              $assign_element
            <br>\n";
        $knum++;
        print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"reassignbycomponent\">
              Assign bug to owner of selected component<br>\n";
        $knum++;
    }
    if (CanIEdit("source", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
	print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"newsource\">
	     Another report of this bug came from 
                $source_popup<BR>\n";
        $knum++;
    }
    if (CanIEdit("resolution", $bug{'reporter'}, $bug{'bug_id'})) {
	$canidoanything = 1;
        if ($bug{'resolution'} ne "") {
            print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"clearresolution\">\n";
            print "Clear the resolution (remove the current resolution of\n";
            print "<b>$bug{'resolution'}</b>)<br>\n";
            $knum++;
        }
        print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"resolve\">
            Resolve bug, changing <A HREF=\"bug_status.phtml\">resolution</A> to
            <SELECT NAME=\"resolution\"
              ONCHANGE=\"document.changeform.knob\[$knum\].checked=true\">
              $resolution_popup</SELECT><BR>\n";
        $knum++;
        print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"duplicate\">
            Resolve bug, mark it as duplicate of bug # 
            <INPUT NAME=\"dup_id\" SIZE=\"6\" ONCHANGE=\"document.changeform.knob\[$knum\].checked=true\"><br>\n";
        $knum++;
    }
} elsif(CanIEdit("bug_status", $bug{'reporter'}, $bug{'bug_id'})) {
    print "<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"reopen\"> Reopen bug<br>\n";
    $knum++;
} else {
    print "<INPUT TYPE=\"hidden\" NAME=\"knob\" VALUE=\"none\">";
}
 
if($canidoanything) {
print "
<INPUT TYPE=\"submit\" VALUE=\"Commit\">
<INPUT TYPE=\"reset\" VALUE=\"Reset\">
<INPUT TYPE=hidden name=form_name VALUE=process_bug>
<BR>";
}

print "
<FONT size=\"+1\"><B>
 <A HREF=\"show_activity.cgi?id=$::FORM{'id'}\">View Bug Activity</A>
 <A HREF=\"long_list.cgi?buglist=$::FORM{'id'}\">Format For Printing</A>
</B></FONT><BR>
</FORM>
<table><tr><td align=left><B>Description:</B></td><td width=100%>&nbsp;</td>
<td align=right>Opened:&nbsp;$bug{'creation_ts'}</td></tr></table>
<HR>
<PRE>
" . html_quote($bug{'long_desc'}) . "
</PRE>
<HR>\n";

# To add back option of editing the long description, insert after the above
# long_list.cgi line:
#  <A HREF=\"edit_desc.cgi?id=$::FORM{'id'}\">Edit Long Description</A>


navigation_header();

print "</BODY>\n";
