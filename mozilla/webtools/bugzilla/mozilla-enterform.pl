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

sub PutBody() {
	my ($product, $component_popup, $platforms_popup, 
	    $opsys_popup, $priority_popup, $sev_popup, 
	    $assign_element, $cc_element ) = @_;
print "
<FORM NAME=enterForm METHOD=POST ACTION=\"post_bug.cgi\">
<INPUT TYPE=HIDDEN NAME=bug_status VALUE=NEW>
<INPUT TYPE=HIDDEN NAME=reporter VALUE=\"$::COOKIE{'Bugzilla_login'}\">
<INPUT TYPE=HIDDEN NAME=product VALUE=\"$product\">
  <TABLE CELLSPACING=2 CELLPADDING=0 BORDER=0>
  <TR>
    <td ALIGN=right valign=top><B>Product:</B></td>
    <td valign=top>$product</td>
  </TR>
  <TR>
    <td ALIGN=right valign=top><B>Version:</B></td>
    <td>" . Version_element(pickversion(), $product) . "</td>
    <td align=right valign=top><b>Component:</b></td>
    <td>$component_popup</td>
  </TR>
  <tr><td>&nbsp<td> <td> <td> <td> <td> </tr>
  <TR>
    <td align=right><b><B><A HREF=\"bug_status.html#rep_platform\">Platform:</A></B></td>
    <TD>$platforms_popup</TD>
    <TD ALIGN=RIGHT><B>OS:</B></TD>
    <TD>$opsys_popup</TD>
    <td align=right valign=top></td>
    <td rowspan=3></td>
    <td></td>
  </TR>
  <TR>
    <TD ALIGN=RIGHT><B><A HREF=\"bug_status.html#priority\">Priority</A>:</B></TD>
    <TD>$priority_popup</TD>
    <TD ALIGN=RIGHT><B><A HREF=\"bug_status.html#severity\">Severity</A>:</B></TD>
    <TD>$sev_popup</TD>
    <td></td>
    <td></td>
  </TR>
  <tr><td>&nbsp<td> <td> <td> <td> <td> </tr>
  <tr>
    <TD ALIGN=RIGHT><B><A HREF=\"bug_status.html#assigned_to\">Assigned To:
        </A></B></TD>
    <TD colspan=5>$assign_element
    (Leave blank to assign to default owner for component)</td>
  </tr>
  <tr>
    <TD ALIGN=RIGHT ><B>Cc:</B></TD>
    <TD colspan=5>$cc_element</TD>
  </tr>
  <tr><td>&nbsp<td> <td> <td> <td> <td> </tr>
  <TR>
    <TD ALIGN=RIGHT><B>URL:</B>
    <TD COLSPAN=5>
      <INPUT NAME=bug_file_loc SIZE=60 value=\"" .
    value_quote(formvalue('bug_file_loc')) .
    "\"></TD>
  </TR>
  <TR>
    <TD ALIGN=RIGHT><B>Summary:</B>
    <TD COLSPAN=5>
      <INPUT NAME=short_desc SIZE=60 value=\"" .
    value_quote(formvalue('short_desc')) .
    "\"></TD>
  </TR>
  <tr><td>&nbsp<td> <td> <td> <td> <td> </tr>
  <tr>
    <td aligh=right valign=top><B>Description:</b>
    <td colspan=5><TEXTAREA WRAP=HARD NAME=comment ROWS=10 COLS=80>" .
    value_quote(formvalue('comment')) .
    "</TEXTAREA><BR></td>
  </tr>
  <tr>
    <td></td>
    <td colspan=5>
       <INPUT TYPE=\"submit\" VALUE=\"    Commit    \">
       &nbsp;&nbsp;&nbsp;&nbsp;
       <INPUT TYPE=\"reset\" VALUE=\"Reset\">
       &nbsp;&nbsp;&nbsp;&nbsp;
       <INPUT TYPE=\"submit\" NAME=maketemplate VALUE=\"Remember values as bookmarkable template\">
    </td>
  </tr>
  </TABLE>
  <INPUT TYPE=hidden name=form_name VALUE=enter_bug>
</FORM>

Some fields initialized from your user-agent, <b>$ENV{'HTTP_USER_AGENT'}</b>.
If you think it got it wrong, please tell " . Param('maintainer') . " what it should have been.

</BODY></HTML>";
}
1;
