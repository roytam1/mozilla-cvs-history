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


use diagnostics;
use strict;

require "CGI.pl";
require "defparams.pl";

# Shut up misguided -w warnings about "used only once":
use vars @::param_desc,
    @::param_list;

confirm_login();

print "Content-type: text/html\n\n";

if (!UserInGroup("tweakparams")) {
	PutHeader("Not Allowed");
    PutError("Sorry, you aren't a member of the 'tweakparams' group.<BR>\n" .
    		 "And so, you aren't allowed to edit the parameters.\n");
    exit;
}


PutHeader("Edit parameters", undef, undef, undef, 1);

print "<CENTER>This lets you edit the basic operating parameters of bugzilla.\n";
print "Be careful!\n";
print "<p>\n";
print "Any item you check Reset on will get reset to its default value.</CENTER>\n";

print "<FORM METHOD=post ACTION=doeditparams.cgi>\n<TABLE ALIGN=center WIDTH=800>\n";

my $rowbreak = "<TR><TD COLSPAN=2><HR></TD></TR>";
print $rowbreak;

foreach my $i (@::param_list) {
    print "<TR><TH ALIGN=right VALIGN=top>$i:</TH><TD>$::param_desc{$i}</TD></TR>\n";
    print "<TR><TD VALIGN=top><INPUT TYPE=checkbox NAME=reset-$i>Reset</TD><TD>\n";
    my $value = Param($i);
    SWITCH: for ($::param_type{$i}) {
	/^t$/ && do {
            print "<INPUT SIZE=60 MAXSIZE=80 NAME=$i VALUE=\"" .
                value_quote($value) . "\">\n";
            last SWITCH;
	};
	/^l$/ && do {
            print "<TEXTAREA WRAP=hard NAME=$i ROWS=10 COLS=60>" .
                value_quote($value) . "</TEXTAREA>\n";
            last SWITCH;
	};
        /^b$/ && do {
            my $on;
            my $off;
            if ($value) {
                $on = "checked";
                $off = "";
            } else {
                $on = "";
                $off = "checked";
            }
            print "<INPUT TYPE=radio NAME=$i VALUE=1 $on>On\n";
            print "<INPUT TYPE=radio NAME=$i VALUE=0 $off>Off\n";
            last SWITCH;
        };
        # DEFAULT
        print "<FONT COLOR=red><BLINK>Unknown param type $::param_type{$i}!!!</BLINK></FONT>\n";
    }
    print "</TD></TR>\n";
    print $rowbreak;
}

print "<TR><TH ALIGN=right VALIGN=top>version:</TH><TD>
What version of Bugzilla this is.  This can't be modified here, but
<TT>%version%</TT> can be used as a parameter in places that understand
such parameters</TD></TR>
<TR><TD></TD><TD>" . Param('version') . "</TD></TR>";

print "</TABLE>\n";

print "<CENTER><INPUT TYPE=reset VALUE=\"Reset form\">\n";
print "<INPUT TYPE=submit VALUE=\"Submit changes\"></CENTER>\n";

print "</FORM>\n";

print "<P><CENTER><A HREF=query.cgi>Skip all this, and go back to the query page</A></CENTER>\n";
PutFooter();

exit;
