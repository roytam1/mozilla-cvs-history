# -*- Mode: perl; indent-tabs-mode: nil -*-

# HTMLPopUp::None.pm - the implementation of the header and link
# command which will be used if no popup menus are desired.

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

# complete rewrite by Ken Estes, Mail.com (kestes@staff.mail.com).
# Contributor(s): 


# We need this empty package namespace for our dependency analysis, it
# gets confused if there is not a package name which matches the file
# name and in this case the file is one of several possible
# implementations.

package HTMLPopUp::None;

package HTMLPopUp;

$VERSION = '#tinder_version#';


# Each function here builds an $out string.  If there are bugs in the
# code you can put your breakpoint on the return statement and look at
# the completed string before it is returned.



# call like this
#    page_header(
#                'title'=>""
#                'refresh'=>""
#               );


sub page_header {
  my (%args) = @_;

  my ($html_time) = timeHTML($main::TIME);

  my ($header) = '';
  my ($refresh) = '';

  ($args{'refresh'}) &&
    ( $refresh =  "<META HTTP-EQUIV=\"Refresh: $args{'refresh'}\" CONTENT=\"300\">" );
  
$header .=<<EOF;
<HTML>
        <!-- This file was automatically created by $main::0  -->
        <!-- at $main::LOCALTIME -->
<HEAD>
	$refresh
        <TITLE>$args{'title'}</TITLE>
        </HEAD>
        <BODY TEXT="#000000" BGCOLOR="#ffffff">


<TABLE BORDER=0 CELLPADDING=12 CELLSPACING=0 WIDTH=\"100%\">
<TR><TD>
   <TABLE BORDER=0 CELLPADDING=0 CELLSPACING=2>
      <TR><TD VALIGN=TOP ALIGN=CENTER NOWRAP>
           <FONT SIZE=\"+3\"><B><NOBR>$args{'title'}</NOBR></B></FONT>
      </TD></TR>
      <TR><TD VALIGN=TOP ALIGN=CENTER>
           <B>Created at: $html_time</B>
      </TD></TR>
   </TABLE>
</TD></TR>
</TABLE>


EOF

  return $header;
}



# if we ever need to remove popups from the output just use this
# version of Link


# call the function like this 
#
# Link(
#	  "statuslinetxt"=>"", 
#	  "windowtxt"=>"", 
#	  "linktxt"=>"", 
#	  "name"=>"", 
#	  "href"=>"",
#
# (arguments with defaults)
#
#	  "windowtitle"=>"", 
#	  "windowheight"=>"", 
#	  "windowwidth"=>"",
#	 );


sub Link {
  my ($args) = @_;
  my ($out) = '';
  
  my ($name) ="";

  if ($args{'name'}) {
    $name = "name=\"$args{'name'}\"";
  }

  $out .= "<a $name href=\"$args{'href'}\">";
  $out .= "$args{'linktxt'}</a>\n";

  return $out;
}
  

# After all the links have been rendered we may need to dump some
# static data structures into the top of the HTML file.  Passing
# indexes to static structures should allow us to embed quotes and new
# lines in our strings.


sub define_structures {
  my (@out) = ();

  return @out;
}

1;

