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

package Litmus::Auth;

use strict;

# IMPORTANT! 
## You _must_ call Litmus::Auth methods before sending a Content-type
## header so that any required cookies can be sent.

require Exporter;
use Litmus::Error;
use Litmus::DB::User;
use Litmus::DB::PasswordResets;
use Litmus::Memoize;
use Litmus::Mailer;

use CGI;
use Date::Manip;

our @ISA = qw(Exporter);
our @EXPORT = qw();

my $logincookiename = $Litmus::Config::user_cookiename;
my $cookie_expire_days = 7;

# Given a username and password, validate the login. Returns the 
# Litmus::DB::User object associated with the username if the login 
# is sucuessful. Returns false otherwise.
sub validate_login($$) {
  my $username = shift;
  my $password = shift;

  return 0 if (!$username or $username eq '' or 
               !$password or $password eq '');

  my ($userobj) = Litmus::DB::User->search_userLowercaseEmail($username);

  if (!$userobj) { 
    return 0; 
  } 

  if (!$userobj->enabled() || $userobj->enabled() == 0) {    
    my $c = Litmus->cgi();
    my $errmsg = "Account ".$userobj->username()." has been disabled by the administrator.";
    loginError($c, $errmsg);
  }
	
  # for security reasons, we use the real (correct) crypt'd passwd
  # as the salt:
  if (checkPassword($userobj,$password)) {
    return $userobj;
  } else {
    return 0;
  }
}

# Used by a CGI when a login is required to proceed beyond a certain point.
# requireLogin() will return a Litmus::User object to indicate that the user 
# is logged in, or it will redirect to a login page to allow the login to be 
# completed. Once the login is complete, the user will be redirected back to 
# $return_to and any parameters in the current CGI.pm object will be passed 
# to the new script. 
#
sub requireLogin {
  my $return_to = shift;
  my $admin_login_required = shift;
  my $cgi = Litmus->cgi();

  # see if we are already logged in:
  my $user = getCurrentUser();
  return $user if $user; # well, that was easy...

  my $vars = {
              title => "Login",
              return_to => $return_to,
              adminrequired => $admin_login_required,
              params => $cgi,
             };

  print $cgi->header();
  Litmus->template()->process("auth/login.html.tmpl", $vars) ||
    internalError(Litmus->template()->error());
  
  exit;	
}

# Used by a CGI in much the same way as requireLogin() when the user must
# be a superuser to proceed.
sub requireAdmin {
  my $return_to = shift;
  my $cgi = Litmus->cgi();
  
  my $user = requireLogin($return_to, 1);
  if (!$user || !$user->is_admin()) { 
  	print $cgi->header();
    basicError("You must be a Litmus administrator to perform this function.");
  }
  return $user;
}

# similar to the above, but the user may be a run/day admin
sub requireRunDayAdmin {
  my $return_to = shift;
  my $cgi = Litmus->cgi();
  
  my $user = requireLogin($return_to, 1);
  if (!$user || !$user->isRunDayAdmin()) { 
  	print $cgi->header();
    basicError("You must be a Test Run/Test Day administrator to perform this function.");
  }
  return $user;
}

# similar to requireAdmin, but the user may be a product admin as well
sub requireProductAdmin {
	my $return_to = shift;
	my $product = shift; # optional
	
	my $cgi = Litmus->cgi();
  
    my $user = requireLogin($return_to, 1);
    if (!$user || !$user->isInAdminGroup()) { 
    	print $cgi->header();
        basicError("You must be a Litmus administrator to perform this function.");
    }
    # superusers can do anything:
    if ($user->isSuperUser()) {
    	return $user;
    }
    # otherwise, they must be an admin of $product if $product is specified:
    if ($product) {
    	if ($user->isProductAdmin($product)) {
    		return $user;
    	} else {
    		print $cgi->header();
       	    basicError("You must be an administrator of product ".
       	      $product->name()." to perform this function.");
    	}
    }
    return $user;
}

# Returns the current Litmus::DB::Session object corresponding to the current 
# logged-in user, or 0 if no valid session exists
sub getCurrentSession() {
	my $c = Litmus->cgi();
	
	# we're actually processing the login form right now, so the cookie hasn't
	# been sent yet...
	if (Litmus->request_cache->{'curSession'}) { 
		return Litmus->request_cache->{'curSession'}; 
	} 
	
	my $sessionCookie = $c->cookie($logincookiename);
  	if (! $sessionCookie) {
    	return 0
    }
  
	my @sessions = Litmus::DB::Session->search(sessioncookie => $sessionCookie);
	my $session = $sessions[0];
	if (! $session) { return 0 }
  
	# see if it's still valid and that the user hasn't been disabled
	if (! $session->isValid()) { return 0 }
  
	return $session;
}

# Returns the Litmus::User object corresponding to the current logged-in
# user, or 0 if no valid login cookie exists
sub getCurrentUser() {
  my $session = getCurrentSession();

  if ($session) {
    return $session->user_id();
  } else {
    return 0;
  }
}

########################################
# Reset Password					   #
########################################

# Change the user's forgotten password. This is done in two phases, with an 
# email confirmation to validate the user's identity before changing the 
# password.
sub resetPassword {
	my $user = shift;
	
	# invalidate any pending password reset sessions:
	my @resets = Litmus::DB::PasswordResets->search(user => $user);
	foreach my $cur (@resets) {
		$cur->session()->makeExpire();
		$cur->delete();
	}
	
	my $session = makeSession($user, 1);
	Litmus::DB::PasswordResets->create({user => $user, session => $session});
	
	my $vars_mail = {
		user => $user,
		token => $session->sessioncookie(),
	};
	my $output;
	Litmus->template()->process("auth/passwordreset/message.mail.tmpl", 
	  $vars_mail, \$output, {POST_CHOMP => 0, PRE_CHOMP => 0}) ||
        internalError(Litmus->template()->error());
	
	# fix weird template nonsense
	$output =~ s/^.*(To: )/To: /s;
	
	Litmus::Mailer::sendMessage($output);
	
	my $vars_html = {
              title => "Reset Password",
              return_to => "login.cgi",
             };
	print Litmus->cgi()->header();
	Litmus->template()->process("auth/passwordreset/checkEmail.html.tmpl", $vars_html) ||
      internalError(Litmus->template()->error());
  
	exit;	
}

# after the user follows the link in the email, they end up here to actually
# reset their password:
sub resetPasswordForm {
	my $token = shift;
	
	my $reset = validateToken($token);
	
	my $vars = {
		title => "Reset Password",
		return_to => "login.cgi",
		user => $reset->user(),
		token => $token,
	};
	print Litmus->cgi()->header();
	Litmus->template()->process("auth/passwordreset/resetForm.html.tmpl", $vars) ||
      internalError(Litmus->template()->error());
}

# and after they complete the reset password from, they come here to 
# actually change the password:
sub doResetPassword {
	my $uid = shift;
	my $token = shift;
	my $password = shift;
	
	my $reset = validateToken($token);
	changePassword($reset->user(), $password);
	
	my $session = makeSession($reset->user());
    Litmus->cgi()->storeCookie(makeCookie($session));
    
    $reset->session()->makeExpire();
	$reset->delete();
}

sub validateToken {
	my $token = shift;
	my @sessions = Litmus::DB::Session->search(sessioncookie => $token);
	my $session = $sessions[0];
	if (!$session) {
		invalidInputError("That authentication token is invalid. It may have 
		  been copied incorrectly from the email, or have already been used. 
		  Please check that the entire URL was copied correctly.");
	}
	
	if (! $session->isValid()) { 
		invalidInputError("That authentication token has expired. You'll need to
		request a new password reset and try again.");
	}
	
	my @resets = Litmus::DB::PasswordResets->search(session => $session);
	my $reset = $resets[0];
    if (!$reset) {
		invalidInputError("That authentication token is invalid. It does not 
		  belong to a user that has requested a password reset.");
	}
	return $reset;
}

#
# ONLY NON-PUBLIC API BEYOND THIS POINT
#

# Processes data from a login form (auth/login.html.tmpl) using data 
# from the current Litmus::CGI object in use. When complete, returns the 
# flow of control to the script the user wanted to reach before the login,
# setting a login cookie for the user.
sub processLoginForm {
  my $c = Litmus->cgi();
  my $thepassword;
  
  my $type = $c->param("login_type");
  
  if ($c->param("accountCreated")  && 
      $c->param("accountCreated") eq "true") {
    # make sure they really are logged in and didn't just set
    # the accountCreated flag:
    requireLogin("index.cgi");
    return; # we're done here
  }
  
  # the user just reset their password, so no need to do anything
  if ($type eq 'doResetPassword') { 
      return;
  }
  
  # check to see if they have forgotten their password:
  if ($type eq "forgot_password") {
    my $username = $c->param("email");
    my @users = Litmus::DB::User->search_userLowercaseEmail($username);
    if (! $users[0]) {
    	loginError($c, "Invalid email address entered. Please try again");
    }
  	resetPassword($users[0]);
  	return;
  }
  
  if ($type eq "litmus") {
    my $username = $c->param("email");
    my $password = $c->param("password");
    
    my $user = validate_login($username, $password);
    
    if (!$user) {
      loginError($c, "Username/Password incorrect. Please try again.");
    }

    expireOldSessions($user);
    my $session = makeSession($user);
    $c->storeCookie(makeCookie($session));
    
  } elsif ($type eq "newaccount") {
    my $email = $c->param("email");
    my $name = $c->param("realname");
    my $password = $c->param("password"); 
    my $nickname = $c->param("irc_nickname");
    
    # some basic form-field validation:
    my $emailregexp = q:^[\\w\\.\\+\\-=]+@[\\w\\.\\-]+\\.[\\w\\-]+$:;
    if (! $email || ! $email =~ /$emailregexp/) {
      loginError($c, "You must enter a valid email address");
    }
    if ($password ne $c->param("password_confirm")) {
      loginError($c, "Passwords do not match. Please try again.");
    }

    my @users = Litmus::DB::User->search_userLowercaseEmail($email);
    if ($users[0]) {
      loginError($c, "User ".$users[0]->email() ." already exists.");
    }
    if ($nickname and $nickname ne '') {
      @users = Litmus::DB::User->search(irc_nickname => $nickname);
      if ($users[0]) {
        loginError($c, "An account with that IRC nickname already exists.");
      }
    } else {
      $nickname = undef;
    }
    
    my $now = &Date::Manip::UnixDate("now","%q");
    my $userobj = 
      Litmus::DB::User->create({email => $email, 
                                password => bz_crypt($password),
                                bugzilla_uid => 0,
                                realname => $name,
                                enabled => 1, 
                                is_admin => 0,
                                irc_nickname => $nickname,
                                creation_date => $now
                               });
    
    my $session = makeSession($userobj);
    $c->storeCookie(makeCookie($session));
    
    my $vars = {
                title => "Account Created",
                email => $email,
                password => $password,
                realname => $name,
                return_to => $c->param("login_loc"),
                params => $c,
                login_extension => $c->param("login_extension"),
               };
    
    print $c->header();
    Litmus->template()->process("auth/accountcreated.html.tmpl", $vars) ||
      internalError(Litmus->template()->error());
    
    exit;	
    
  } elsif ($type eq "bugzilla") {
    my $username = $c->param("username");
    my $password = $c->param("password");    
  } else {
    internalError("Unknown login scheme attempted");
  }
}

sub checkPassword {
  my $userobj = shift;
  my $password = shift;

  my $realPasswordCrypted = $userobj->getRealPasswd();
  return 0 if (!$realPasswordCrypted or $realPasswordCrypted eq '');
  my $enteredPasswordCrypted = crypt($password, $realPasswordCrypted);
  if ($enteredPasswordCrypted eq $realPasswordCrypted) {
    return 1;
  } else {
    return 0;
  }
}

sub changePassword {
  my $userobj = shift;
  my $password = shift;

  $userobj->password(bz_crypt($password));
  $userobj->update();

  expireSessions($userobj);
}

sub expireSessions {
  my $userobj = shift;
  my @sessions = $userobj->sessions();
  foreach my $session (@sessions) {
    $session->makeExpire();
  }
}

sub expireOldSessions {
  my $userobj = shift;
  my @sessions = $userobj->sessions();
  foreach my $session (@sessions) {
    $session->isValid();
  }
}

# Given a userobj, process the login and return a session object
sub makeSession {
  my $userobj = shift;
  my $dontsetsession = shift;
  my $c = Litmus->cgi();

  my $err;
  my $expires = &Date::Manip::UnixDate(&Date::Manip::DateCalc("now","+ $cookie_expire_days days",\$err),"%q");

  my $sessioncookie = randomToken(64);

  my $session = Litmus::DB::Session->create({
                                             user_id => $userobj, 
                                             sessioncookie => $sessioncookie,
                                             expires => $expires});

  unless ($dontsetsession) {
  	Litmus->request_cache->{'curSession'} = $session;
  }
  return $session;
}

# Given a session, create a login cookie to go with it
sub makeCookie {
  my $session = shift;
  
  my $cookie = Litmus->cgi()->cookie( 
                                     -name    => $logincookiename,
                                     -value   => $session->sessioncookie(),
                                     -domain  => $main::ENV{"HTTP_HOST"},
                                     -expires => &Date::Manip::UnixDate($session->expires(),"%c"),
                                    );
  return $cookie;
}

sub loginError {
  my $c = shift;
  my $message = shift;
  my $template = shift;
  my $title = shift;

  if (!$template) {
    $template = "auth/login.html.tmpl";
  }
  if (!$title) {
    $title = "Login";
  }

  print $c->header();
  
  my $return_to = $c->param("login_loc") || "";
  my $email = $c->param("email") || "";
	
  unsetFields();
  
  my $vars = {
              title => $title,
              return_to => $return_to,
              email => $email,
              params => $c,
              onload => "toggleMessage('failure','$message')", 
            };

  Litmus->template()->process($template, $vars) ||
    internalError(Litmus->template()->error());
  
  exit;
}

sub unsetFields() {
  my $c = Litmus->cgi();
  
  # We need to unset some params in $c since otherwise we end up with 
  # a hidden form field set for "email" and friends and madness results:
  $c->param('email', '');
  $c->param('username', '');
  $c->param('irc_nickname', '');
  $c->param('login_type', '');
  $c->param('login_loc', '');
  $c->param('realname', '');
  $c->param('password', '');
  $c->param('password_confirm', '');
  $c->param('Submit', '');
}

# Like crypt(), but with a random salt. Thanks to Bugzilla for this.
sub bz_crypt {
  my ($password) = @_;
  
  # The list of characters that can appear in a salt.  Salts and hashes
  # are both encoded as a sequence of characters from a set containing
  # 64 characters, each one of which represents 6 bits of the salt/hash.
  # The encoding is similar to BASE64, the difference being that the
  # BASE64 plus sign (+) is replaced with a forward slash (/).
  my @saltchars = (0..9, 'A'..'Z', 'a'..'z', '.', '/');
  
  # Generate the salt.  We use an 8 character (48 bit) salt for maximum
  # security on systems whose crypt uses MD5.  Systems with older
  # versions of crypt will just use the first two characters of the salt.
  my $salt = '';
  for ( my $i=0 ; $i < 8 ; ++$i ) {
    $salt .= $saltchars[rand(64)];
  }
  
  # Crypt the password.
  my $cryptedpassword = crypt($password, $salt);
  
  # Return the crypted password.
  return $cryptedpassword;
}

sub randomToken {
  my $size = shift || 10; # default to 10 chars if nothing specified
  return join("", map{ ('0'..'9','a'..'z','A'..'Z')[rand 62] } (1..$size));
}

# Deprecated:
# DO NOT USE
sub setCookie {
  my $user = shift;
  my $expires = shift;

  my $user_id = 0;
  if ($user) {
    $user_id = $user->user_id();
  }
  
  if (!$expires or $expires eq '') {
    $expires = '+3d';
  }
    
  my $c = Litmus->cgi();
  
  my $cookie = $c->cookie( 
                          -name    => $logincookiename,
                          -value   => $user_id,
                          -domain  => $main::ENV{"HTTP_HOST"},
                          -expires => $expires,
                         );
  
  return $cookie;
}

# Deprecated:
sub getCookie() {
  return getCurrentUser();
}

# trusted means "is a super user"
sub istrusted($) {
  my $userobj = shift;

  return 0 if (!$userobj);
  return $userobj->isSuperUser();
}

sub canEdit($) {
  my $userobj = shift;
  
  return $userobj->istrusted();
}

#########################################################################
# logout()
#
# Unset the user's cookie
#########################################################################
sub logout() {
  my $c = Litmus->cgi();

  my $cookie = $c->cookie(
                          -name => $logincookiename,
                          -value => '',
                          -domain  => $main::ENV{"HTTP_HOST"},
                          -expires => '-1d'
                         );
  $c->storeCookie($cookie);
  
  # invalidate the session behind the cookie as well:
  my $session = getCurrentSession();
  if ($session) { $session->makeExpire() } 
}


1;


