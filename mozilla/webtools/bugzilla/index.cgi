#!/usr/bonsaitools/bin/perl -w
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
# 				  David Lawrence <dkl@redhat.com>

use diagnostics;
use strict;
use Text::Template;

require 'CGI.pl';

print "Content-type:text/html\n\n";

PutHeader("Main Page");

print qq{
<TABLE ALIGN=center WIDTH=800 BORDER=1 CELLPADDING=0 CELLSPACING=0>
<TR BGCOLOR="#BFBFBF">
	<TD>
	<TABLE ALIGN=center WIDTH=100%>
	<TR>
		<TH ALIGN=left>Bugzilla News</TH>
		<TH ALIGN=right><A HREF=news.cgi>Old news...</A></TH>
	</TR>
	</TABLE>
	</TD>
</TR><TR>
	<TD>
};
print GetHeadlines();
print "</TD>\n</TR>\n</TABLE><P>\n";

print LoadTemplate('index_redhat.tmpl');

PutFooter();

