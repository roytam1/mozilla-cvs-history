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
#                 Andrew Anderson <andrew@redhat.com>

use CGI;
$::cgi = new CGI;

require "CGI.pl";

confirm_login();

if ($::cgi->param('submit')) {
    if ($::cgi->param('pwd1') ne $::cgi->param('pwd2')) {
        PutHeader("Try Again.");
        print "The two passwords you entered did not match.\n";
        print "Please click <b>Back</b> and try again.\n";
        exit;
    }

    my $pwd = $::cgi->param('pwd1');

    if ($pwd !~ /^[a-zA-Z0-9-_]*$/ || length($pwd) < 3 || length($pwd) > 15) {
        PutHeader("Sorry; we're picky.");
        print "Please choose a password that is between 3 and 15 characters " . 
              "long, and that contains only numbers, letters, hyphens, or " .
              "underlines.\n<P>\nPlease click <b>Back</b> and try again.\n";
        exit;
    }

    # Generate a random salt.
    sub x {
      my $sc="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";
      return substr($sc, int (rand () * 100000) % (length ($sc) + 1), 1);
    }
    my $salt  = x() . x();

    my $encrypted = crypt($pwd, $salt);

    my $query = "update profiles set password='" . $pwd . 
                "',cryptpassword='" . $encrypted . 
                "' where login_name=" .
                SqlQuote($::cgi->cookie(-name=>'Bugzilla_login'));
    SendSQL($query);

    $query = "update logincookies set cryptpassword = '" .  $encrypted . 
             "' where cookie = '" . 
             $::cgi->cookie(-name=>'Bugzilla_logincookie') ."'";
    SendSQL($query);

    PutHeader("OK, done.");
    print "Your new password has been set.\n<P>\n" .
          $::cgi->a({href=>"query.cgi"}, "Back to query page.") . "\n";
} else {
    PutHeader("Change your password"); 
    print $::cgi->startform(-method=>"post") . "\n";
    print $::cgi->table(
             $::cgi->TR(
                $::cgi->td({ALIGN=>"RIGHT"}, 
                         "Please enter the new password for " .
                $::cgi->b($::cgi->cookie(-name=>"Bugzilla_login")). ":"),
                $::cgi->td($::cgi->password_field(-name=>"pwd1", 
                                              -value=>'',
                                              -override=>"1"))
             ),
             $::cgi->TR(
                $::cgi->td({ALIGN=>"RIGHT"}, 
                         "Re-enter your new password:"),
                $::cgi->td($::cgi->password_field(-name=>"pwd2", 
                                              -value=>'',
                                              -override=>"1"))
             )
          );
    print $::cgi->submit(-name=>"submit", -value=>"Submit") .
          $::cgi->endform . "\n";
}

print $::cgi->end_html . "\n";
