# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>

# Contains some global routines used throughout the CGI scripts of Bugzilla.

use diagnostics;
use strict;

# Shut up misguided -w warnings about "used only once".  For some reason,
# "use vars" chokes on me when I try it here.

sub CGI_pl_sillyness {
    my $zz;
    $zz = %::FILENAME;
}

use CGI::Carp qw(fatalsToBrowser);

require 'globals.pl';

sub GeneratePersonInput {
    my ($field, $required, $def_value, $extraJavaScript) = (@_);
    $extraJavaScript ||= "";
    if ($extraJavaScript ne "") {
        $extraJavaScript = "onChange=\"$extraJavaScript\"";
    }
    return "<INPUT NAME=\"$field\" SIZE=32 $extraJavaScript VALUE=\"$def_value\">";
}

sub GeneratePeopleInput {
    my ($field, $def_value) = (@_);
    return "<INPUT NAME=\"$field\" SIZE=45 VALUE=\"$def_value\">";
}




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


sub ProcessFormFields {
    my ($buffer) = (@_);
    undef %::FORM;
    undef %::MFORM;

    my %isnull;
    my $remaining = $buffer;
    while ($remaining ne "") {
	my $item;
	if ($remaining =~ /^([^&]*)&(.*)$/) {
	    $item = $1;
	    $remaining = $2;
	} else {
	    $item = $remaining;
	    $remaining = "";
	}

	my $name;
	my $value;
	if ($item =~ /^([^=]*)=(.*)$/) {
	    $name = $1;
	    $value = url_decode($2);
	} else {
	    $name = $item;
	    $value = "";
	}
	if ($value ne "") {
	    if (defined $::FORM{$name}) {
		$::FORM{$name} .= $value;
		my $ref = $::MFORM{$name};
		push @$ref, $value;
	    } else {
		$::FORM{$name} = $value;
		$::MFORM{$name} = [$value];
	    }
        } else {
            $isnull{$name} = 1;
        }
    }
    if (defined %isnull) {
        foreach my $name (keys(%isnull)) {
            if (!defined $::FORM{$name}) {
                $::FORM{$name} = "";
                $::MFORM{$name} = [];
            }
        }
    }
}


sub ProcessMultipartFormFields {
    my ($boundary) = (@_);
    $boundary =~ s/^-*//;
    my $remaining = $ENV{"CONTENT_LENGTH"};
    my $inheader = 1;
    my $itemname = "";
#    open(DEBUG, ">debug") || die "Can't open debugging thing";
#    print DEBUG "Boundary is '$boundary'\n";
    while ($remaining > 0 && ($_ = <STDIN>)) {
        $remaining -= length($_);
#        print DEBUG "< $_";
        if ($_ =~ m/^-*$boundary/) {
#            print DEBUG "Entered header\n";
            $inheader = 1;
            $itemname = "";
            next;
        }

        if ($inheader) {
            if (m/^\s*$/) {
                $inheader = 0;
#                print DEBUG "left header\n";
                $::FORM{$itemname} = "";
            }
            if (m/^Content-Disposition:\s*form-data\s*;\s*name\s*=\s*"([^\"]+)"/i) {
                $itemname = $1;
#                print DEBUG "Found itemname $itemname\n";
                if (m/;\s*filename\s*=\s*"([^\"]+)"/i) {
                    $::FILENAME{$itemname} = $1;
                }
            }
            
            next;
        }
        $::FORM{$itemname} .= $_;
    }
    delete $::FORM{""};
    # Get rid of trailing newlines.
    foreach my $i (keys %::FORM) {
        chomp($::FORM{$i});
        $::FORM{$i} =~ s/\r$//;
    }
}


# check and see if a given field exists, is non-empty, and is set to a 
# legal value.  assume a browser bug and abort appropriately if not.
# if $legalsRef is not passed, just check to make sure the value exists and 
# is non-NULL
# 
sub CheckFormField (\%$;\@) {
    my ($formRef,                # a reference to the form to check (a hash)
        $fieldname,              # the fieldname to check
        $legalsRef               # (optional) ref to a list of legal values 
       ) = @_;

    if ( !defined $formRef->{$fieldname} ||
         trim($formRef->{$fieldname}) eq "" ||
         (defined($legalsRef) && 
          lsearch($legalsRef, $formRef->{$fieldname})<0) ){

        print "A legal $fieldname was not set; ";
        print Param("browserbugmessage");
        exit 0;
      }
}

# check and see if a given field is defined, and abort if not
# 
sub CheckFormFieldDefined (\%$) {
    my ($formRef,                # a reference to the form to check (a hash)
        $fieldname,              # the fieldname to check
       ) = @_;

    if ( !defined $formRef->{$fieldname} ) {
        print "$fieldname was not defined; ";
        print Param("browserbugmessage");
        exit 0;
      }
}

# check and see if a given string actually represents a positive
# integer, and abort if not.
# 
sub CheckPosInt($) {
    my ($number) = @_;              # the fieldname to check

    if ( $number !~ /^[1-9][0-9]*$/ ) {
        print "Received string \"$number\" when postive integer expected; ";
        print Param("browserbugmessage");
        exit 0;
      }
}

sub FormData {
    my ($field) = (@_);
    return $::FORM{$field};
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
    $var =~ s/\n/\&#010;/g;
    $var =~ s/\r/\&#013;/g;
    return $var;
}

sub navigation_header {
    if (defined $::COOKIE{"BUGLIST"} && $::COOKIE{"BUGLIST"} ne "" &&
        defined $::FORM{'id'}) {
	my @bugs = split(/:/, $::COOKIE{"BUGLIST"});
	my $cur = lsearch(\@bugs, $::FORM{"id"});
	print "<B>Bug List:</B> (@{[$cur + 1]} of @{[$#bugs + 1]})\n";
	print "<A HREF=\"show_bug.cgi?id=$bugs[0]\">First</A>\n";
	print "<A HREF=\"show_bug.cgi?id=$bugs[$#bugs]\">Last</A>\n";
	if ($cur > 0) {
	    print "<A HREF=\"show_bug.cgi?id=$bugs[$cur - 1]\">Prev</A>\n";
	} else {
	    print "<I><FONT COLOR=\#777777>Prev</FONT></I>\n";
	}
	if ($cur < $#bugs) {
	    $::next_bug = $bugs[$cur + 1];
	    print "<A HREF=\"show_bug.cgi?id=$::next_bug\">Next</A>\n";
	} else {
	    print "<I><FONT COLOR=\#777777>Next</FONT></I>\n";
	}
    }
    print "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<A HREF=query.cgi>Query page</A>\n";
    print "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<A HREF=enter_bug.cgi>Enter new bug</A>\n"
}


$::CheckOptionValues = 1;

sub make_options {
    my ($src,$default,$isregexp) = (@_);
    my $last = "";
    my $popup = "";
    my $found = 0;
    $default = "" if !defined $default;

    if ($src) {
        foreach my $item (@$src) {
            if ($item eq "-blank-" || $item ne $last) {
                if ($item eq "-blank-") {
                    $item = "";
                }
                $last = $item;
                if ($isregexp ? $item =~ $default : $default eq $item) {
                    $popup .= "<OPTION SELECTED VALUE=\"$item\">$item";
                    $found = 1;
                } else {
                    $popup .= "<OPTION VALUE=\"$item\">$item";
                }
            }
        }
    }
    if (!$found && $default ne "") {
      if ( Param("strictvaluechecks") && $::CheckOptionValues &&
           ($default ne $::dontchange) && ($default ne "-All-") &&
           ($default ne "DUPLICATE") ) {
        print "Possible bug database corruption has been detected.  " .
              "Please send mail to " . Param("maintainer") . " with " .
              "details of what you were doing when this message " . 
              "appeared.  Thank you.\n";
        exit 0;
              
      } else {
	$popup .= "<OPTION SELECTED>$default";
      }
    }
    return $popup;
}


sub make_popup {
    my ($name,$src,$default,$listtype,$onchange) = (@_);
    my $popup = "<SELECT NAME=$name";
    if ($listtype > 0) {
        $popup .= " SIZE=5";
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


sub quietly_check_login() {
    $::usergroupset = '0';
    my $loginok = 0;
    if (defined $::COOKIE{"Bugzilla_login"} &&
	defined $::COOKIE{"Bugzilla_logincookie"}) {
        ConnectToDatabase();
        if (!defined $ENV{'REMOTE_HOST'}) {
            $ENV{'REMOTE_HOST'} = $ENV{'REMOTE_ADDR'};
        }
        SendSQL("select profiles.groupset, profiles.login_name, " .
                "profiles.login_name = " .
		SqlQuote($::COOKIE{"Bugzilla_login"}) .
		" and profiles.cryptpassword = logincookies.cryptpassword " .
		"and logincookies.hostname = " .
		SqlQuote($ENV{"REMOTE_HOST"}) .
		" from profiles,logincookies where logincookies.cookie = " .
		SqlQuote($::COOKIE{"Bugzilla_logincookie"}) .
		" and profiles.userid = logincookies.userid");
        my @row;
        if (@row = FetchSQLData()) {
            $loginok = $row[2];
            if ($loginok) {
                $::usergroupset = $row[0];
                $::COOKIE{"Bugzilla_login"} = $row[1]; # Makes sure case is in
                                                       # canonical form.
            }
        }
    }
    if (!$loginok) {
        delete $::COOKIE{"Bugzilla_login"};
    }
    return $loginok;
}




sub CheckEmailSyntax {
    my ($addr) = (@_);
    my $match = Param('emailregexp');
    if ($addr !~ /$match/) {
        print "Content-type: text/html\n\n";

        PutHeader("Check e-mail syntax");
        print "The e-mail address you entered\n";
        print "(<b>$addr</b>) didn't match our minimal\n";
        print "syntax checking for a legal email address.\n";
        print Param('emailregexpdesc');
        print "<p>Please click <b>back</b> and try again.\n";
        exit;
    }
}



sub MailPassword {
    my ($login, $password) = (@_);
    my $urlbase = Param("urlbase");
    my $template = "From: bugzilla-daemon
To: %s
Subject: Your bugzilla password.

To use the wonders of bugzilla, you can use the following:

 E-mail address: %s
       Password: %s

 To change your password, go to:
 ${urlbase}changepassword.cgi

 (Your bugzilla and CVS password, if any, are not currently synchronized.
 Top hackers are working around the clock to fix this, as you read this.)
";
    my $msg = sprintf($template, $login . Param('emailsuffix'),
                      $login, $password);

    open SENDMAIL, "|/usr/lib/sendmail -t";
    print SENDMAIL $msg;
    close SENDMAIL;

    print "The password for the e-mail address\n";
    print "$login has been e-mailed to that address.\n";
    print "<p>When the e-mail arrives, you can click <b>Back</b>\n";
    print "and enter your password in the form there.\n";
}


sub confirm_login {
    my ($nexturl) = (@_);

# Uncommenting the next line can help debugging...
#    print "Content-type: text/plain\n\n";

    ConnectToDatabase();
    if (defined $::FORM{"Bugzilla_login"} &&
	defined $::FORM{"Bugzilla_password"}) {

	my $enteredlogin = $::FORM{"Bugzilla_login"};
        my $enteredpwd = $::FORM{"Bugzilla_password"};
        CheckEmailSyntax($enteredlogin);

        my $realcryptpwd  = PasswordForLogin($::FORM{"Bugzilla_login"});
        
        if (defined $::FORM{"PleaseMailAPassword"}) {
	    my $realpwd;
            if ($realcryptpwd eq "") {
		$realpwd = InsertNewUser($enteredlogin, "");
            } else {
                SendSQL("select password from profiles where login_name = " .
			SqlQuote($enteredlogin));
		$realpwd = FetchOneColumn();
            }
	    print "Content-type: text/html\n\n";
	    PutHeader("<H1>Password has been emailed");
            MailPassword($enteredlogin, $realpwd);
            exit;
        }

	my $enteredcryptpwd = crypt($enteredpwd, substr($realcryptpwd, 0, 2));
        if ($realcryptpwd eq "" || $enteredcryptpwd ne $realcryptpwd) {
            print "Content-type: text/html\n\n";
	    PutHeader("Login failed");
            print "The username or password you entered is not valid.\n";
            print "Please click <b>Back</b> and try again.\n";
            exit;
        }
        $::COOKIE{"Bugzilla_login"} = $enteredlogin;
        if (!defined $ENV{'REMOTE_HOST'}) {
            $ENV{'REMOTE_HOST'} = $ENV{'REMOTE_ADDR'};
        }
	SendSQL("insert into logincookies (userid,cryptpassword,hostname) values (@{[DBNameToIdAndCheck($enteredlogin)]}, @{[SqlQuote($realcryptpwd)]}, @{[SqlQuote($ENV{'REMOTE_HOST'})]})");
        SendSQL("select LAST_INSERT_ID()");
        my $logincookie = FetchOneColumn();

        $::COOKIE{"Bugzilla_logincookie"} = $logincookie;
        print "Set-Cookie: Bugzilla_login=$enteredlogin ; path=/; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
        print "Set-Cookie: Bugzilla_logincookie=$logincookie ; path=/; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";

        # This next one just cleans out any old bugzilla passwords that may
        # be sitting around in the cookie files, from the bad old days when
        # we actually stored the password there.
        print "Set-Cookie: Bugzilla_password= ; path=/; expires=Sun, 30-Jun-80 00:00:00 GMT\n";

    }


    my $loginok = quietly_check_login();

    if ($loginok != 1) {
        print "Content-type: text/html\n\n";
        PutHeader("Login");
        print "I need a legitimate e-mail address and password to continue.\n";
        if (!defined $nexturl || $nexturl eq "") {
	    # Sets nexturl to be argv0, stripping everything up to and
	    # including the last slash.
	    $0 =~ m:[^/]*$:;
	    $nexturl = $&;
        }
        my $method = "POST";
        if (defined $ENV{"REQUEST_METHOD"} && length($::buffer) > 1) {
            $method = $ENV{"REQUEST_METHOD"};
        }
        print "
<FORM action=$nexturl method=$method>
<table>
<tr>
<td align=right><b>E-mail address:</b></td>
<td><input size=35 name=Bugzilla_login></td>
</tr>
<tr>
<td align=right><b>Password:</b></td>
<td><input type=password size=35 name=Bugzilla_password></td>
</tr>
</table>
";
        foreach my $i (keys %::FORM) {
            if ($i =~ /^Bugzilla_/) {
                next;
            }
            print "<input type=hidden name=$i value=\"@{[value_quote($::FORM{$i})]}\">\n";
        }
        print "
<input type=submit value=Login name=GoAheadAndLogIn><hr>
If you don't have a password, or have forgotten it, then please fill in the
e-mail address above and click
 here:<input type=submit value=\"E-mail me a password\"
name=PleaseMailAPassword>
</form>\n";

        # This seems like as good as time as any to get rid of old
        # crufty junk in the logincookies table.  Get rid of any entry
        # that hasn't been used in a month.
        SendSQL("delete from logincookies where to_days(now()) - to_days(lastused) > 30");

        
        exit;
    }

    # Update the timestamp on our logincookie, so it'll keep on working.
    SendSQL("update logincookies set lastused = null where cookie = $::COOKIE{'Bugzilla_logincookie'}");
}


sub PutHeader {
    my ($title, $h1, $h2, $extra) = (@_);

    if (!defined $h1) {
	$h1 = $title;
    }
    if (!defined $h2) {
	$h2 = "";
    }
    if (!defined $extra) {
	$extra = "";
    }

    print "<HTML><HEAD>\n<TITLE>$title</TITLE>\n";
    print Param("headerhtml") . "\n</HEAD>\n";
    print "<BODY   BGCOLOR=\"#FFFFFF\" TEXT=\"#000000\"\n";
    print "LINK=\"#0000EE\" VLINK=\"#551A8B\" ALINK=\"#FF0000\" $extra>\n";

    print PerformSubsts(Param("bannerhtml"), undef);

    print "<TABLE BORDER=0 CELLPADDING=12 CELLSPACING=0 WIDTH=\"100%\">\n";
    print " <TR>\n";
    print "  <TD>\n";
    print "   <TABLE BORDER=0 CELLPADDING=0 CELLSPACING=2>\n";
    print "    <TR><TD VALIGN=TOP ALIGN=CENTER NOWRAP>\n";
    print "     <FONT SIZE=\"+3\"><B><NOBR>$h1</NOBR></B></FONT>\n";
    print "    </TD></TR><TR><TD VALIGN=TOP ALIGN=CENTER>\n";
    print "     <B>$h2</B>\n";
    print "    </TD></TR>\n";
    print "   </TABLE>\n";
    print "  </TD>\n";
    print "  <TD>\n";

    print Param("blurbhtml");

    print "</TD></TR></TABLE>\n";
}


sub DumpBugActivity {
    my ($id, $starttime) = (@_);
    my $datepart = "";
    if (defined $starttime) {
        $datepart = "and bugs_activity.bug_when >= $starttime";
    }
    my $query = "
        select bugs_activity.field, bugs_activity.bug_when,
                bugs_activity.oldvalue, bugs_activity.newvalue,
                profiles.login_name
        from bugs_activity,profiles
        where bugs_activity.bug_id = $id $datepart
        and profiles.userid = bugs_activity.who
        order by bugs_activity.bug_when";

    SendSQL($query);
    
    print "<table border cellpadding=4>\n";
    print "<tr>\n";
    print "    <th>Who</th><th>What</th><th>Old value</th><th>New value</th><th>When</th>\n";
    print "</tr>\n";
    
    my @row;
    while (@row = FetchSQLData()) {
        my ($field,$when,$old,$new,$who) = (@row);
        $old = value_quote($old);
        $new = value_quote($new);
        if ($old eq "") {
            $old = "&nbsp;";
        }
        if ($new eq "") {
            $new = "&nbsp;";
        }
        print "<tr>\n";
        print "<td>$who</td>\n";
        print "<td>$field</td>\n";
        print "<td>$old</td>\n";
        print "<td>$new</td>\n";
        print "<td>$when</td>\n";
        print "</tr>\n";
    }
    print "</table>\n";
}


#
# Prints a warnbanner incl. image with given message
# 
sub warnBanner( $ )
{
  my ($msg) = (@_);
  print Param("warnbannerhtml");
  print $msg;
  print Param("warnfooterhtml");
}


############# Live code below here (that is, not subroutine defs) #############


$| = 1;

# Uncommenting this next line can help debugging.
# print "Content-type: text/html\n\nHello mom\n";

# foreach my $k (sort(keys %ENV)) {
#     print "$k $ENV{$k}<br>\n";
# }

if (defined $ENV{"REQUEST_METHOD"}) {
    if ($ENV{"REQUEST_METHOD"} eq "GET") {
        if (defined $ENV{"QUERY_STRING"}) {
            $::buffer = $ENV{"QUERY_STRING"};
        } else {
            $::buffer = "";
        }
        ProcessFormFields $::buffer;
    } else {
        if ($ENV{"CONTENT_TYPE"} =~
            m@multipart/form-data; boundary=\s*([^; ]+)@) {
            ProcessMultipartFormFields($1);
            $::buffer = "";
        } else {
            read STDIN, $::buffer, $ENV{"CONTENT_LENGTH"} ||
                die "Couldn't get form data";
            ProcessFormFields $::buffer;
        }
    }
}


if (defined $ENV{"HTTP_COOKIE"}) {
    foreach my $pair (split(/;/, $ENV{"HTTP_COOKIE"})) {
	$pair = trim($pair);
	if ($pair =~ /^([^=]*)=(.*)$/) {
	    $::COOKIE{$1} = $2;
	} else {
	    $::COOKIE{$pair} = "";
	}
    }
}

1;
