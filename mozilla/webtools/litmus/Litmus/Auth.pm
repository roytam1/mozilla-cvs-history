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
 # Portions created by the Initial Developer are Copyright (C) 2005
 # the Initial Developer. All Rights Reserved.
 #
 # Contributor(s):
 #   Chris Cooper <ccooper@deadsquid.com>
 #   Zach Lipton <zach@zachlipton.com>
 #
 # ***** END LICENSE BLOCK *****

=cut

package Litmus::Auth;

use strict;

require Exporter;
use Litmus;
use Litmus::DB::User;
use Litmus::Error;

use CGI;

our @ISA = qw(Exporter);
our @EXPORT = qw();

my $logincookiename = $Litmus::Config::user_cookiename;

# (XXX) stub
# Given a username and password, validate the login. Returns the 
# Lutmus::DB::User object associated with the username if the login 
# is sucuessful. Returns false otherwise.
sub validate_login($$) {
	my $username = shift;
	my $password = shift;
	
	# we'll fill this in later once we actually _have_ passwords. 
	# for now, just return the user object or false:
	my @usrobjs = Litmus::DB::User->search(email => $username);
	if (@usrobjs) {
		return $usrobjs[0];
	} else {
		return 0;
	}
}

sub setCookie {
  my $user = shift;
  my $expires = shift;

  my $user_id = 0;
  if ($user) {
    $user_id = $user->userid();
  }
  
  if (!$expires or $expires eq '') {
    $expires = '+3d';
  }
    
  my $c = new CGI;
  
  my $cookie = $c->cookie( 
                          -name    => $logincookiename,
                          -value   => $user_id,
                          -domain  => $main::ENV{"HTTP_HOST"},
                          -expires => $expires,
                         );
  
  return $cookie;
}

sub getCookie() {
    my $c = new CGI;
    
    my $cookie = $c->cookie($logincookiename);
    
    my $user = Litmus::DB::User->retrieve($cookie);
    if (! $user) {
        return 0;
    } else {
        unless ($user->userid() == $cookie) {
            invalidInputError("Invalid login cookie");
        }
    }
    return $user;
}

sub istrusted($) {
    my $userobj = shift;

    return 0 if (!$userobj);
    
    if ($userobj->istrusted()) {
        return 1;
    } else {
        return 0;
    }
}

sub canEdit($) {
    my $userobj = shift;
    
    return $userobj->istrusted();
}

#########################################################################
# logout()
#
# Unset the user's cookie, and return them to the main index.
#########################################################################
sub logout() {
  my $c = new CGI;
  
  my $cookie = Litmus::Auth::setCookie(undef,'-1d');
  return $cookie;
}


1;


