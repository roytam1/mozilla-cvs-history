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

use diagnostics;
use strict;
use CGI;

$::cgi = new CGI;

require "CGI.pl";

# FIXME: make the path a config option
my $login_cookie = $::cgi->cookie(-name=>'Bugzilla_login',
                                -value=>'',
                                -expires=>"now",
                                -path=>"/bugzilla/");
my $logincookie_cookie = $::cgi->cookie(-name=>'Bugzilla_logincookie',
                                -value=>'',
                                -expires=>"now",
                                -path=>"/bugzilla/");
my $password_cookie = $::cgi->cookie(-name=>'Bugzilla_password',
                                -value=>'',
                                -expires=>"now",
                                -path=>"/bugzilla/");

print $::cgi->header(-type=>'text/html', 
          -cookie=>[$login_cookie, $logincookie_cookie, $password_cookie]);

PutHeader("Your login has been forgotten");

print "The cookie that was remembering your login is now gone.  " .
      "The next time you do an action that requires a login, you " .
      "will be prompted for it.<P>" .
      $::cgi->a({-href=>"query.cgi"}, "Back to the query page.");

exit;
