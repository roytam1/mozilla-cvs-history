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
#                 <david.gardiner@unisa.edu.au>

# Contains some global routines used throughout the CGI scripts of Bugzilla.

use diagnostics;
use strict;

use CGI::Carp qw(fatalsToBrowser);

require 'globals.pl';

# Implementations of several of the below were blatently stolen from CGI.pm,
# by Lincoln D. Stein.

# Get rid of all the %xx encoding and the like from the given URL.

sub url_decode {
    my ($todecode) = (@_);
    $todecode =~ tr/+/ /;       # pluses become spaces
    $todecode =~ s/%([0-9a-fA-F]{2})/pack("c",hex($1))/ge;
    return $todecode;
}


# Quotify a string, suitable for putting into a URL.

sub url_quote {
    my($toencode) = (@_);
    $toencode=~s/([^a-zA-Z0-9_\-.])/uc sprintf("%%%02x",ord($1))/eg;
    return $toencode;
}

sub html_quote {
    my ($var) = (@_);
    $var =~ s/\&/\&amp;/g;
    $var =~ s/</\&lt;/g;
    $var =~ s/>/\&gt;/g;
    return $var;
}

sub value_quote {
    my ($var) = (@_);
    $var =~ s/\&/\&amp;/g;
    $var =~ s/</\&lt;/g;
    $var =~ s/>/\&gt;/g;
    $var =~ s/"/\&quot;/g;
    return $var;
}

sub navigation_header {
    if ($::cgi->cookie('BUGLIST') ne "") {
	my @bugs = split(/:/, $::cgi->cookie('BUGLIST'));
	my $cur = lsearch(\@bugs, $::cgi->param('id'));
	print $::cgi->b('Bug List:') . " (@{[$cur + 1]} of @{[$#bugs + 1]})\n",
              $::cgi->a({-href=>"show_bug.cgi?id=$bugs[0]"}, 'First'),
              $::cgi->a({-href=>"show_bug.cgi?id=$bugs[$#bugs]"}, 'Last'),
	      "\n";
	if ($cur > 0) {
	    print $::cgi->a({-href=>"show_bug.cgi?id=$bugs[$cur - 1]"}, 'Prev');
	} else {
	    print $::cgi->i($::cgi->font({-color=>"#777777"}, 'Prev'));
	}
	if ($cur < $#bugs) {
	    $::next_bug = $bugs[$cur + 1];
	    print $::cgi->a({-href=>"show_bug.cgi?id=$::next_bug"}, 'Next');
	} else {
	    print $::cgi->i($::cgi->font({-color=>"#777777"}, 'Next'));
	}
    }
    print $::cgi->table({-cellspacing=>'0', -cellpadding=>'0', -border=>'0'},
             $::cgi->TR(
                $::cgi->td({-width=>'150', -valign=>'CENTER'},
                   $::cgi->a({-href=>"query.cgi"}, 'Query page')),
                $::cgi->td({-width=>'150', -valign=>'CENTER'},
                   $::cgi->a({-href=>"enter_bug.cgi"}, 'Enter new bug')),
                $::cgi->td({-width=>'250', -valign=>'CENTER', -align=>'LEFT'},
                   $::cgi->startform(-method=>'POST', -action=>"show_bug.cgi"),
                   $::cgi->submit(-name=>'submit', -value=>"Show"),
                   "&nbsp;bug #&nbsp;",
                   $::cgi->textfield(-name=>'id', -size=>'6', 
		                     -value=>'', -override=>"1"),
                   $::cgi->endform,
                )
             )
          );
}


sub make_options {
    my ($src,$default,$isregexp) = (@_);
    my $last = "";
    my $popup = "";
    my $found = 0;
    foreach my $item (@$src) {
        if ($item eq "-blank-" || $item ne $last) {
            if ($item eq "-blank-") {
		$item = "";
	    }
            $last = $item;
            if ($isregexp ? $item =~ $default : $default eq $item) {
                $popup .= "  <OPTION SELECTED VALUE=\"" . url_quote($item) . "\">" . url_decode($item) . "\n";
                $found = 1;
            } else {
		$popup .= "  <OPTION VALUE=\"" . url_quote($item) . "\">" . url_decode($item) . "\n";
            }
        }
    }
    if (!$found && $default ne "") {
	$popup .= "  <OPTION SELECTED>$default\n";
    }
    return $popup;
}


sub make_popup {
    my ($name,$src,$default,$listtype,$onchange) = (@_);
    my $popup = "<SELECT NAME=\"$name\"";
    if ($listtype > 0) {
        $popup .= " SIZE=\"5\"";
        if ($listtype == 2) {
            $popup .= " MULTIPLE";
        }
    }
    if (defined $onchange && $onchange ne "") {
        $popup .= " onchange=$onchange";
    }
    $popup .= ">" . make_options($src, $default,
                                 ($listtype == 2 && $default ne ""));
    $popup .= "</SELECT>";
    return $popup;
}


sub PasswordForLogin {
    my ($login) = (@_);
    SendSQL("select cryptpassword from profiles where login_name = " .
	    SqlQuote($login));
    my $result = FetchOneColumn();
    if (!defined $result) {
        $result = "";
    }
    return $result;
}

sub confirm_login {
    my ($nexturl) = (@_);
    my $printed_header = 0;

# Uncommenting the following lines can help debugging...
#    print $::cgi->header(-type=>'text/html'),
#          $::cgi->cookie('Bugzilla_login'),
#          $::cgi->cookie('Bugzilla_password'),
#          $::cgi->dump;

    ConnectToDatabase();

    my $authdomain = "";
    my $authorized = 0;
    my $rem_host = $::cgi->remote_host();

    # Apache 1.3.x users: this requires "HostNameLookups on" to work
    if (defined Param('authdomain') && $rem_host) {
        $authdomain = Param('authdomain');
        if ($::cgi->cookie("Bugzilla_login") =~ /$authdomain$/) {
            use Socket;
	    my @authnets = split (",", Param('authnet'));
	    for my $authnet (@authnets) {
		my $ip_addr = unpack("N4", inet_aton($rem_host));
		my ($network, $netmask) = split("/", $authnet);
		$network = unpack("N4", inet_aton($network));
		$netmask = unpack("N4", inet_aton($netmask));
		my $net = $network & $netmask;
		my $xor = $ip_addr ^ $net;
		my $and = $ip_addr | $net;
		if ($and == $ip_addr) {
			$authorized = 1;
		}
            }
        } else {
	    # We got here because we don't have a login cookie yet.
	    $authorized = 1;
	}
        if (!$authorized) {
            print $::cgi->header(-type=>'text/html');
	    PutHeader("Unauthorized host");
	    print "You have connedted as a user from ",
                  $::cgi->i($authdomain), ", but you are not in ",
                  $::cgi->i($authdomain), ", currently.", $::cgi->p,
	          "You must either create a new username to use ",
	          "from your current location, or you must connect ",
	          "from an authorized host.";
	    exit;
        }
    }

    my ($login_cookie, $logincookie_cookie, $password_cookie, $logincookie);
    my $loginok = 0;

    if ($::cgi->param('Bugzilla_login') ne "") {

	my $enteredlogin = $::cgi->param('Bugzilla_login');
        my $enteredpwd = $::cgi->param('Bugzilla_password');

	if ($enteredlogin !~ /^[^@, ]*@[^@, ]*\.[^@, ]*$/) {
            print $::cgi->header(-type=>'text/html');

	    PutHeader("Invalid e-mail address entered");

            print "The e-mail address you entered ($::cgi->b($enteredlogin)) ",
                  "didn't match our minimal syntax checking for a legal email ",
                  "address.  A legal address must contain exactly one '\@', ",
                  "and at least one '.' after the \@, and may not contain any ",
                  "commas or spaces.\n$::cgi->p\nPlease click ",
                  "$::cgi->b('back') and try again.\n",
            exit;
        }
        my $realcryptpwd  = PasswordForLogin($enteredlogin);
        
        if ($::cgi->param("PleaseMailAPassword") ne "") {
	    my $realpwd;
            if ($realcryptpwd eq "") {
		$realpwd = InsertNewUser($enteredlogin);
            } else {
                SendSQL("select password from profiles where login_name = " .
			SqlQuote($enteredlogin));
		$realpwd = FetchOneColumn();
            }
	    my $template = "
To use the wonders of bugzilla, you can use the following:

 E-mail address: %s
       Password: %s

 To change your password, go to:
 %schangepassword.cgi

 (Your bugzilla and CVS password, if any, are not currently synchronized.
 Top hackers are working around the clock to fix this, as you read this.)
";
            my $msg = sprintf($template, $enteredlogin, 
                              $realpwd, Param("urlbase"));
            
	    Mail($enteredlogin, "", "Your bugzilla password", $msg);

            print $::cgi->header(-type=>'text/html');
	    PutHeader("Password has been emailed.");
            print "The password for the e-mail address\n",
                  "$enteredlogin has been e-mailed to that address.\n",
                  $::cgi->p,
                  "When the e-mail arrives, you can click ",
                  $::cgi->b('back'),
                  "\nand enter your password in the form there.\n";
            exit;
        }

	my $enteredcryptpwd = crypt($enteredpwd, substr($realcryptpwd, 0, 2));
        if ($realcryptpwd eq "" || $enteredcryptpwd ne $realcryptpwd) {
            print $::cgi->header(-type=>'text/html');
	    PutHeader("Login failed.");
            print "The username or password you entered is not valid.\n";
            print "Please click ", $::cgi->b('back'), " and try again.\n";
            exit;
        }
	# FIXME: make path a config item
	$login_cookie = $::cgi->cookie(-name=>'Bugzilla_login',
                              -value=>$enteredlogin,
                              -path=>"/bugzilla/",
                              -expires=>"Sun, 30-Jun-2029 00:00:00 GMT");
	my $query = "insert into logincookies " .
                    "(userid,cryptpassword,hostname) " .
                    "values (@{[DBNameToIdAndCheck($enteredlogin)]}, " .
                    "@{[SqlQuote($realcryptpwd)]}, " .
                    "@{[SqlQuote($rem_host)]})";
	SendSQL($query);
        SendSQL("select LAST_INSERT_ID()");
        $logincookie = FetchOneColumn();

	# FIXME: make path a config item
	$logincookie_cookie = $::cgi->cookie(-name=>'Bugzilla_logincookie',
                                    -value=>$logincookie,
                                    -path=>"/bugzilla/",
                                    -expires=>"Sun, 30-Jun-2029 00:00:00 GMT");

        # This next one just cleans out any old bugzilla passwords that may
        # be sitting around in the cookie files, from the bad old days when
        # we actually stored the password there.
	$password_cookie = $::cgi->cookie(-name=>'Bugzilla_password',
                                          -value=>'',
                                          -path=>'/',
                                          -expires=>'now');
	my $oldlogin_cookie = $::cgi->cookie(-name=>'Bugzilla_login',
                                             -value=>'',
                                             -path=>'/',
                                             -expires=>'now');
	my $oldlogincookie_cookie=$::cgi->cookie(-name=>'Bugzilla_logincookie',
                                                 -value=>'',
                                                 -path=>'/',
                                                 -expires=>'now');
        # I sincerely hope that the order that the cookies gets processed is
        # deterministic in the future, or this may break
        print $::cgi->header(
               -cookie=>[$oldlogin_cookie, $oldlogincookie_cookie, 
                         $login_cookie, $logincookie_cookie, $password_cookie]);
	$printed_header = 1;
	$loginok = 1;
    }

    if (!$loginok &&
        ($::cgi->cookie('Bugzilla_login') ne "") &&
	($::cgi->cookie('Bugzilla_logincookie') ne "")) {

	my $login = $::cgi->cookie('Bugzilla_login');
	$logincookie = $::cgi->cookie('Bugzilla_logincookie');

        SendSQL("select profiles.login_name = " .
		SqlQuote($login) .
		" and profiles.cryptpassword = logincookies.cryptpassword " .
		"and logincookies.hostname = " .
		SqlQuote($rem_host) .
		" from profiles,logincookies where logincookies.cookie = '" .
		$logincookie .
		"' and profiles.userid = logincookies.userid");
        $loginok = FetchOneColumn();
        if (!defined $loginok) {
            $loginok = 0;
        }
    }

    if ($loginok ne "1") {
        print $::cgi->header(-type=>'text/html');
        PutHeader("Please log in.");
        #print $::cgi->cookie('Bugzilla_login') . $::cgi->br . "\n";
        #print $::cgi->cookie('Bugzilla_logincookie') . $::cgi->br . "\n";
        print "I need a legitimate e-mail address and password to continue.\n";
        if (!defined $nexturl || $nexturl eq "") {
	    # Sets nexturl to be argv0, stripping everything up to and
	    # including the last slash.
	    $0 =~ m:[^/]*$:;
	    $nexturl = $&;
        }
        print $::cgi->startform(-method=>'POST', -action=>"$nexturl"),
              $::cgi->table(
	         $::cgi->TR(
		    $::cgi->td({-align=>'RIGHT'},
		       $::cgi->b('E-mail address:')),
		    $::cgi->td(
		       $::cgi->textfield(-name=>'Bugzilla_login',
		                          -size=>"35"))
                 ),
	         $::cgi->TR(
		    $::cgi->td({-align=>'RIGHT'},
		       $::cgi->b('Password:')),
		    $::cgi->td(
		       $::cgi->password_field(-name=>'Bugzilla_password',
		                          -size=>"35"))
                 )
              );

	foreach my $param ($::cgi->param) {
		print $::cgi->hidden(-name=>$param, 
                                     -value=>$::cgi->param("$param"),
                                     -override=>"1");
	}
        print $::cgi->submit(-name=>'GoAheadAndLogIn', -value=>'Login'),
	      $::cgi->hr,
              "If you don't have a password, or have forgotten it, ",
	      "then please fill in the e-mail address above and click here: ",
	      $::cgi->submit(-name=>'PleaseMailAPassword', 
	                     -value=>'E-mail me a password'),
              $::cgi->endform,
              $::cgi->end_html;

        # This seems like as good as time as any to get rid of old
        # crufty junk in the logincookies table.  Get rid of any entry
        # that hasn't been used in a month.
        SendSQL("delete from logincookies where to_days(now()) - to_days(lastused) > 30");

        exit;
    } else {
	print $::cgi->header('text/html') unless $printed_header;
    }
# Update the timestamp on our logincookie, so it'll keep on working.
    SendSQL("update logincookies set lastused = null where cookie = $logincookie");
}

sub PutHeader {
    my ($title, $h1, $h2) = (@_);

    if (!defined $h1) {
	$h1 = $title;
    }
    if (!defined $h2) {
	$h2 = "";
    }

    print $::cgi->start_html(-title=>$title,
                             -BGCOLOR=>"#FFFFFF",
                             -TEXT=>"#000000",
			     -LINK=>"#0000EE",
			     -VLINK=>#551A8B",
			     -ALINK=>"#FF0000");
    print PerformSubsts(Param("bannerhtml"), undef);

    print $::cgi->table({-border=>"0", 
                         -cellpadding=>"12", 
			 -cellspacing=>"0",
                         -width=>"100%"},
	     $::cgi->TR(
	        $::cgi->td(
		   $::cgi->table({-border=>"0", 
		                  -cellpadding=>"0", 
				  -cellspacing=>"2"},
		      $::cgi->TR(
		         $::cgi->td({-valign=>"TOP", 
			             -align=>"DENTER", 
				     -nowrap},
		            $::cgi->font({-size=>"+3"},
			       $::cgi->b($h1))
			 )
		      ),
		      $::cgi->TR(
		         $::cgi->td({-valign=>"TOP", -align=>"CENTER"},
			    $::cgi->b($h2))
		      )
		   )
		),
	        $::cgi->td(Param("blurbhtml"))
	     )
	  );
}

############# Live code below here (that is, not subroutine defs) #############


$| = 1;

# Uncommenting this next line can help debugging.
# print $::cgi->header('text/html'), "Hi mom";
# print $::cgi->dump;

1;
