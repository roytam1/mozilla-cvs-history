#!#perl# #perlflags# --
# -*- Mode: perl; indent-tabs-mode: nil -*-
#

# regenerate.cgi - the webform used by administrators to close the
#		 tree, set the message of the day and stop build 
#		 columns from being shown on the default pages.


# $Revision$ 
# $Date$ 
# $Author$ 
# $Source$ 
# $Name$ 

# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Tinderbox build tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#

# complete rewrite by Ken Estes:
#	 kestes@staff.mail.com Old work.
#	 kestes@reefedge.com New work.
#	 kestes@walrus.com Home.
# Contributor(s): 



# Standard perl libraries

# Tinderbox libraries

use lib '#tinder_libdir#';

use TreeData;
use HTMLPopUp;
use FileStructure;



#       Main        
{
    $tree = 'SeaMonkey';
    $url = FileStructure::get_filename($tree, 'tree_URL');

    HTMLPopUp::regenerate_HTML_pages();

    $out = <<EOF;
Content-type: text/html

<TITLE>tinderbox</TITLE>
<META HTTP-EQUIV="Refresh" CONTENT="0; URL=$url">
<BODY   BGCOLOR="#FFFFFF" TEXT="#000000"
        LINK="#0000EE" VLINK="#551A8B" ALINK="#FF0000">
<CENTER>
<TABLE BORDER=0 WIDTH="100%" HEIGHT="100%"><TR><TD ALIGN=CENTER VALIGN=CENTER>
<FONT SIZE="+2">
Regenerating HTML now.<br>
Sending you to the <A HREF="$url">default Tinderbox page</A>.
</FONT>
</TD></TR></TABLE>
</CENTER>

EOF

;

    print $out;

    exit 0;		
}
