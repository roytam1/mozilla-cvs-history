# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bonsai CVS tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 

use strict;

sub EmitHtmlTitleAndHeader {
    my($doctitle,$heading,$subheading, $header_extras) = @_;
    
    print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n";
    print "<HTML>\n<HEAD>\n  <TITLE>$doctitle</TITLE>\n";
    print "  $::site_icon_link\n";
    print "  $header_extras\n" if (defined($header_extras));
    print "</HEAD>\n";

    print "<BODY BGCOLOR=\"#FFFFFF\" TEXT=\"#000000\"";
    print "LINK=\"#0000EE\" VLINK=\"#551A8B\" ALINK=\"#FF0000\">\n";

    if (open(BANNER, "<", "$::data_dir/banner.html")) {
        while (<BANNER>) { print; }
        close BANNER;
    }

    print "<TABLE BORDER=0 CELLPADDING=12 CELLSPACING=0 WIDTH=\"100%\">\n";
    print " <TR>\n";
    print "  <TD>\n";
    print "   <TABLE BORDER=0 CELLPADDING=0 CELLSPACING=2>\n";
    print "    <TR><TD VALIGN=TOP ALIGN=CENTER NOWRAP>";
    print "<FONT SIZE=\"+3\"><B>$heading</B></FONT></TD></TR>\n";
    print "    <TR><TD VALIGN=TOP ALIGN=CENTER><B>$subheading</B></TD></TR>\n";
    print "   </TABLE>\n";
    print "  </TD>\n";
    print "  <TD>\n";

    print "  <!-- Insert blurb here -->\n";
    if (open(BLURB, "<", "$::data_dir/blurb")) {
        while (<BLURB>) { print; }
        close BLURB;
    }

    print "  </TD>\n </TR>\n</TABLE>\n";
}

sub EmitHtmlHeader {
    my($heading,$subheading,$heading_extras) = @_;
    EmitHtmlTitleAndHeader($heading,$heading,$subheading,$heading_extras);
}

1;
