# -*- mode: cperl; c-basic-offset: 8; indent-tabs-mode: nil; -*-

=head1 COPYRIGHT

 # ***** BEGIN LICENSE BLOCK *****
 # Version: MPL 1.1
 #
 # The contents of this file are subject to the Mozilla Public License
 # Version 1.1 (the "License"); you may not use this file except in
 # compliance with the License. You may obtain a copy of the License
 # at http://www.mozilla.org/MPL/
 #
 # Software distributed under the License is distributed on an "AS IS"
 # basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 # the License for the specific language governing rights and
 # limitations under the License.
 #
 # The Original Code is Litmus.
 #
 # The Initial Developer of the Original Code is
 # the Mozilla Corporation.
 # Portions created by the Initial Developer are Copyright (C) 2006
 # the Initial Developer. All Rights Reserved.
 #
 # Contributor(s):
 #   Chris Cooper <ccooper@deadsquid.com>
 #   Zach Lipton <zach@zachlipton.com>
 #
 # ***** END LICENSE BLOCK *****

=cut

# Global object store and function library for Litmus

package Litmus;

use strict;

use Litmus::Template;
use Litmus::Config;
use Litmus::Auth;
use Litmus::CGI;

BEGIN {
	if ($Litmus::Config::disabled) {
  	  	my $c = new CGI();
    	print $c->header();
    	print "Litmus has been shutdown by the administrator. Please try again later.";
    	exit;
	}
}

# Global Template object
my $_template;
sub template() {
    my $class = shift;
    $_template ||= Litmus::Template->create();
    return $_template;
}

# Global CGI object
my $_cgi;
sub cgi() {
    my $class = shift;
    $_cgi ||= Litmus::CGI->new();
    return $_cgi;
}

# hook to handle a login in progress for any CGI script:
BEGIN {
	my $c = cgi();
	if ($c->param("login_type")) {
		Litmus::Auth::processLoginForm();
	}
}

1;
