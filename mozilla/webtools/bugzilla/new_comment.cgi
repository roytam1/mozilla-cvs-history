#!/usr/bonsaitools/bin/perl
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
# $Id$
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Andrew Anderson <andrew@redhat.com>

use strict;
use diagnostics;
use CGI;

my $cgi = new CGI;

my $c=$cgi->param("comment");

open(COMMENTS, ">>data/comments");
print COMMENTS "$c\n";
close(COMMENTS);

print $cgi->header(-type=>'text/html'),
      $cgi->start_html(-title=>"The Word Of Confirmation"),
      $cgi->h1("Done"),
      $c,
      $cgi->end_html;
