#!/usr/bonsaitools/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Netscape Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is the Bonsai CVS tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.

require 'CGI.pl';
print "Content-type: text/html\n\n";
LoadTreeConfig();

sub IsChecked {
     my ($value) = @_;
     my $rval = '';

     $rval = "CHECKED" if ($value eq $::TreeID);
     return $rval;
}

my $title = "George, George, George of the jungle...";
PutsHeader($title, "Switch-o-Matic");

print "
<b>Which tree would you like to see?</b>
<FORM method=get action=\"toplevel.cgi\">\n";

foreach my $i (@::TreeList) {
     next if (exists($::TreeInfo{$i}{nobosai}));

     print "<INPUT TYPE=radio NAME=treeid VALUE=$i " . IsChecked($i) . ">\n";
     print "$::TreeInfo{$i}{description}<BR>\n";
}

print "<INPUT TYPE=SUBMIT Value=\"Submit\"></FORM>\n";
PutsTrailer();
exit;
