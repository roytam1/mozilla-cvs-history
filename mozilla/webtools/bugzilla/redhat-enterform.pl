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
#                 Andrew Anderson <andrew@redhat.com>

# Shut up misguided -w warnings about "used only once".
use vars @::COOKIE,
	@::legal_source;

sub PutBody() {
	my ($product, $component_popup, $platforms_popup, 
	    $opsys_popup, $priority_popup, $sev_popup, 
	    $assign_element, $cc_element ) = @_;

	my $release_element = GeneratePeopleInput('release', 5, formvalue('release'));
	my $source_value = "bug reporting address and mailing lists";
	my $source_popup = "<SELECT TYPE=\"HIDDEN\" NAME=\"source\" VALUE=\"" . 
		url_quote($source_value) . "\"></SELECT>";

	print "
<FORM NAME=\"enterForm\" METHOD=\"POST\" ACTION=\"post_bug.cgi\">
<INPUT TYPE=\"HIDDEN\" NAME=\"bug_status\" VALUE=\"NEW\">
<INPUT TYPE=\"HIDDEN\" NAME=\"reporter\" VALUE=\"$::COOKIE{'Bugzilla_login'}\">
<INPUT TYPE=\"HIDDEN\" NAME=\"product\" VALUE=\"$product\">
  <TABLE CELLSPACING=\"2\" CELLPADDING=\"0\" BORDER=\"0\">
  <TR>
    <TD ALIGN=\"right\" valign=\"top\"><B>Product:</B></td>
    <TD VALIGN=\"top\" COLSPAN=\"5\">$product</td>
  </TR>
  <TR>
    <TD ALIGN=\"right\" VALIGN=\"top\"><B>Version:</B></Td>
    <TD>" . Version_element(pickversion(), $product) . "</TD>
    <TD ALIGN=\"right\" VALIGN=\"top\"><B>Component:</B></TD>
    <TD COLSPAN=\"3\">$component_popup</td>
  </TR>
  <TR>
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml#priority\">Priority</A>:</B></TD>
    <TD>$priority_popup</TD>
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml#severity\">Severity</A>:</B></TD>
    <TD>$sev_popup</TD>
  </TR>
  <TR>
    <TD ALIGN=\"RIGHT\"><b><B><A HREF=\"bug_status.phtml#rep_platform\">Architecture:</A></B></td>
    <TD>$platforms_popup</TD>
";

if(CanIView("view")) {
    print "
    <TD ALIGN=\"RIGHT\"><B>View:</B></TD>
    <TD>
      <SELECT NAME=\"view\">
        <OPTION VALUE=\"public\">Public
        <OPTION SELECTED VALUE=\"private\">Private
      </SELECT>
    </TD>
  </TD>\n";
} else {
    print "<TD><INPUT TYPE=\"HIDDEN\" NAME=\"view\" VALUE=\"Public\"></TD><TD></TD>\n";
}

print "
  </TR>
";

if(CanIView("source")) {
    $source_popup = make_popup('source', \@::legal_sources, formvalue('source', 'support mail'), 0);
    print "
  <TR>
    <TD ALIGN=\"RIGHT\"><B>Source:</B></TD>
    <TD COLSPAN=\"5\">$source_popup</TD>
  </TR>
";
} 

if(CanIView("assigned_to")) {
    print "
  </TR>
  <TR>
    <TD ALIGN=\"RIGHT\"><B><A HREF=\"bug_status.phtml#assigned_to\">Assigned To:
        </A></B></TD>
    <TD colspan=\"5\">$assign_element
    (Leave blank to assign to default owner)</td>
  </TR>\n";
}

print "
  <tr>
    <TD ALIGN=\"RIGHT\"><B>Cc:</B></TD>
    <TD colspan=\"5\">$cc_element</TD>
  </tr>
  <TR>
    <TD ALIGN=\"RIGHT\"><B>Summary:</B>
    <TD COLSPAN=\"5\">
      <INPUT NAME=\"short_desc\" SIZE=\"60\" value=\"" .
    value_quote(formvalue('short_desc')) .
    "\"></TD>
  </TR>
  <tr>
    <td aligh=\"right\" valign=\"top\"><B>Description:</b>
    <td colspan=\"5\"><TEXTAREA WRAP=\"HARD\" NAME=\"comment\" ROWS=\"10\" COLS=\"60\">" .
    value_quote(formvalue('comment')) .
    "</TEXTAREA><BR></td>
  </tr>
  <tr>
    <td></td>
    <td colspan=\"5\">
       <INPUT TYPE=\"submit\" VALUE=\"    Commit    \">
       &nbsp;&nbsp;&nbsp;&nbsp;
       <INPUT TYPE=\"reset\" VALUE=\"Reset\">
       &nbsp;&nbsp;&nbsp;&nbsp;
       <INPUT TYPE=\"submit\" NAME=\"maketemplate\" VALUE=\"Remember values as bookmarkable template\">
    </td>
  </tr>
  </TABLE>
  <INPUT TYPE=\"hidden\" name=\"form_name\" VALUE=\"enter_bug\">
</FORM>
</BODY>
</HTML>";
}
1;
