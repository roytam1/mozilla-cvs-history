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

use vars %::FORM;

use diagnostics;
use strict;

require "CGI.pl";

ConnectToDatabase();
GetVersionTable();

print "Content-type: text/html\n\n";

my $product = $::FORM{'product'};
if (!defined $product || lsearch(\@::legal_product, $product) < 0) {

    PutHeader("Components");
    print "
<FORM>
<CENTER><B>Please specify the product whose components you want described.</B>
<P>
Product: <SELECT NAME=product>
";
    print make_options(\@::legal_product);
    print "
</SELECT>
<P>
<INPUT TYPE=\"submit\" VALUE=\"Submit\">
</CENTER>
</FORM>
";
	PutFooter();
    exit;
}


PutHeader("Components", "Components", $product);
print "
<TABLE ALIGN=center WIDTH=800>
<TR>
<TH ALIGN=left>Component</TH>
<TH ALIGN=left>Default owner</TH>
";

my $useqacontact = Param("useqacontact");

my $cols = 2;
if ($useqacontact) {
    print "<TH ALIGN=left>Default qa contact</TH>";
    $cols++;
}

my $colbut1 = $cols - 1;

print "</TR>";

SendSQL("select value, initialowner, initialqacontact, description from components where program = " . SqlQuote($product) . " order by value");

while (MoreSQLData()) {
    my @row = FetchSQLData();
    my ($component, $initialowner, $initialqacontact, $description) = (@row);

    print qq|
<tr><td colspan=$cols><hr></td></tr>
<tr><td rowspan=2>$component</td>
<td><a href="mailto:$initialowner">$initialowner</a></td>
|;
    if ($useqacontact) {
        print qq|
<td><a href="mailto:$initialqacontact">$initialqacontact</a></td>
|;
    }
    print "</TR><TR><TD COLSPAN=$colbut1>$description</TD></TR>\n";
}

print "<TR><TD COLSPAN=$cols><HR></TD></TR></TABLE>\n";

PutFooter();

