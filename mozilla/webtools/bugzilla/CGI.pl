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
#				  David Lawrence <dkl@redhat.com>

# Contains some global routines used throughout the CGI scripts of Bugzilla.

use diagnostics;
use strict;
use Text::Template;

# Shut up misguided -w warnings about "used only once".  For some reason,
# "use vars" chokes on me when I try it here.

sub CGI_pl_sillyness {
    my $zz;
    $zz = %::FILENAME;
	$zz = %::MFORM;
    $zz = %::dontchange;
}

use CGI::Carp qw(fatalsToBrowser);

require 'globals.pl';

# subroutine: 	GeneratePersonInput
# description:	Generates an html form text input box
# params:		$field 		= name of field associated with the text box
#				$required 	= not used
#				$def_value	= The default value for the text box
#				$extraJavaScript = action to be taken if box is changed
# returns:		returns string to create html text box (scalar)

sub GeneratePersonInput {
    my ($field, $required, $def_value, $extraJavaScript) = (@_);
    $extraJavaScript ||= "";
    if ($extraJavaScript ne "") {
        $extraJavaScript = "onChange=\"$extraJavaScript\"";
    }
    return "<INPUT NAME=\"$field\" SIZE=32 $extraJavaScript VALUE=\"$def_value\">";
}

# subroutine:	GeneratePeopleInput
# description:	Generates an html form text input box
# params:		$field      = name of field associated with the text box
#               $def_value  = The default value for the text box
# returns:		returns string to create html text box (scalar)

sub GeneratePeopleInput {
    my ($field, $def_value) = (@_);
    return "<INPUT NAME=\"$field\" SIZE=45 VALUE=\"$def_value\">";
}


# Implementations of several of the below were blatently stolen from CGI.pm,
# by Lincoln D. Stein.

# subroutine:	url_decode
# description:	Get rid of all the %xx encoding and the like from the given URL.
# params:		$todecode = string to remove url encoding from (scalar)
# returns:		$todecode = string with url encoding removed (scalar)

sub url_decode {
    my ($todecode) = (@_);
    $todecode =~ tr/+/ /;       # pluses become spaces
    $todecode =~ s/%([0-9a-fA-F]{2})/pack("c",hex($1))/ge;
    return $todecode;
}


# subroutine: 	url_quote
# description:	Quotify a string, suitable for putting into a URL.
# params:		$todecode = string to encode for use in url (scalar)
# returns:		$todecode = string with url encoded informatio (scalar)

sub url_quote {
    my($toencode) = (@_);
    $toencode=~s/([^a-zA-Z0-9_\-.])/uc sprintf("%%%02x",ord($1))/eg;
    return $toencode;
}


# subroutine:	ParseUrlString
# description:	parse url string to obtain form variables
# params:		$buffer = url query string (scalar)
#				$f = hash to contain single value form variables (hash ref)
#				$m = hash to contain multipart form variables (hash ref)
# returns: 		none

sub ParseUrlString {
    my ($buffer, $f, $m) = (@_);
    undef %$f;
    undef %$m;

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
        if (defined $f->{$name}) {
        $f->{$name} .= $value;
        my $ref = $m->{$name};
        push @$ref, $value;
        } else {
        $f->{$name} = $value;
        $m->{$name} = [$value];
        }
        } else {
            $isnull{$name} = 1;
        }
    }
    if (defined %isnull) {
        foreach my $name (keys(%isnull)) {
            if (!defined $f->{$name}) {
                $f->{$name} = "";
                $m->{$name} = [];
            }
        }
    }
}


# subroutine:	ProcessFormFields
# description:	process cgi form fields
# params:		$buffer = url query string (scalar)
# returns:		parsed url string (scalar)

#sub ProcessFormFields {
#    my ($buffer) = (@_);
#    return ParseUrlString($buffer, \%::FORM, \%::MFORM);
#}


# subroutine:	ProcessFormFields
# description:	process single value form fields
# params:		$buffer = url query string (scalar)
# returns:		none

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


# subroutine: 	ProcessMultipartFormFields
# description:  process multi value form fields 
# params:		$boundary = 
# returns:		none

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


# subroutine: 	CheckFormField
# description: 	check and see if a given field exists, is non-empty, and is set to a 
# 				legal value.  assume a browser bug and abort appropriately if not.
# 				if $legalsRef is not passed, just check to make sure the value exists and 
# 				is non-NULL
# params:		$formref = a reference to the form to check (hash ref)
#				$fieldname = the fieldname to check (scalar)
#				$legalsref = (optional) ref list of legal values (array ref)
# returns:		none

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
        PutFooter();
        exit 0;
    }
}


# subroutine: 	CheckFormFieldDefined
# description:	check and see if a given field is defined, and abort if not
# params:		$formref =  a reference to the form to check (hash ref)
# 				$fieldname = the fieldname to check (scalar)
# returns:		none

sub CheckFormFieldDefined (\%$) {
    my ($formRef,                # a reference to the form to check (a hash)
        $fieldname,              # the fieldname to check
       ) = @_;

    if ( !defined $formRef->{$fieldname} ) {
        print "$fieldname was not defined; ";
        print Param("browserbugmessage");
        PutFooter();
        exit 0;
      }
}


# subroutine:	CheckPosInt
# description:	check and see if a given string actually represents a positive
# 				integer, and abort if not.
# params:		$number = the fieldname to check (scalar)
# returns:		none

sub CheckPosInt($) {
    my ($number) = @_;              # the fieldname to check

    if ( $number !~ /^[1-9][0-9]*$/ ) {
        print "Received string \"$number\" when postive integer expected; ";
        print Param("browserbugmessage");
        PutFooter();
        exit 0;
      }
}


# subroutine:	FormData
# description:	returns value of form field
# params:		$field = field to return value of (scalar)
# returns:		$::FORM{$field} = value of $field (scalar)

sub FormData {
    my ($field) = (@_);
    return $::FORM{$field};
}


# subroutine:	html_quote
# description:	replace certain html characters with encoded equivalents
# params:		$var = string to encode (scalar)
# returns:		$var = encoded string

sub html_quote {
    my ($var) = (@_);
    $var =~ s/\&/\&amp;/g;
    $var =~ s/</\&lt;/g;
    $var =~ s/>/\&gt;/g;
    return $var;
}


# subroutine:	value_quote
# description:	replace certain html characters with encoded equivalents 
# params:		$var = string to encode (scalar)
# returns:		$var = encoded string (scalar)

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


# subroutine:	navigation_header
# description:	prints control bar to move around a list of bugs, reads
#				list from BUGLIST cookie.
# params: 		none		
# returns:		$navigation = string containing html (scalar)

sub navigation_header {
    if (defined $::COOKIE{"BUGLIST"} && $::COOKIE{"BUGLIST"} ne "" &&
        defined $::FORM{'id'}) {
		my @bugs = split(/:/, $::COOKIE{"BUGLIST"});
		my $cur = lsearch(\@bugs, $::FORM{"id"});
		my $navigation = "<B>My Bug List:</B> (";
		$navigation .= "<A HREF=\"buglist.cgi?sql=bugs.bug_id%20in%20(";
		$navigation .= join(",", @bugs) . ")\"> @{[$cur + 1]} of @{[$#bugs + 1]}</A>) \n";
		$navigation .= "<A HREF=\"show_bug.cgi?id=$bugs[0]\">First</A>\n";
		$navigation .= "<A HREF=\"show_bug.cgi?id=$bugs[$#bugs]\">Last</A>\n";
		if ($cur > 0) {
	    	$navigation .= "<A HREF=\"show_bug.cgi?id=$bugs[$cur - 1]\">Prev</A>\n";
		} else {
	    	$navigation .= "<I><FONT COLOR=\"#ECECEC\">Prev</FONT></I>\n";
		}
		if ($cur < $#bugs) {
	    	$::next_bug = $bugs[$cur + 1];
	    	$navigation .= "<A HREF=\"show_bug.cgi?id=$::next_bug\">Next</A>\n";
		} else {
	    	$navigation .= "<I><FONT COLOR=\"#ECECEC\">Next</FONT></I>\n";
#			$navigation .= "&nbsp;&nbsp;<A HREF=\"buglist.cgi?regetlastlist=1\">Show list</A>\n";
		}
		return $navigation .= "</B>\n";
    }
#    print "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<A HREF=query.cgi>Query page</A>\n";
#    print "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<A HREF=enter_bug.cgi>Enter new bug</A>\n"
}


# subroutine:	make_checkboxes
# description:	create form checkboxes
# params:		$src = reference to array of check box items (array ref)
#				$default = item checked by default (scalar)
#				$isregexp = regular expression (scalar)
#				$name = form name for check boxes (scalar)
# returns:		$string = string containing html (scalar)

sub make_checkboxes {
    my ($src,$default,$isregexp,$name) = (@_);
    my $last = "";
    my $capitalized = "";
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
                $capitalized = $item;
                $capitalized =~ tr/A-Z/a-z/;
                $capitalized =~ s/^(.?)(.*)/\u$1$2/;
                if ($isregexp ? $item =~ $default : $default eq $item) {
 					$popup .= "<INPUT NAME=$name TYPE=CHECKBOX VALUE=\"$item\" CHECKED>$capitalized<br>";
                    $found = 1;
                } else {
                    $popup .= "<INPUT NAME=$name TYPE=CHECKBOX VALUE=\"$item\">$capitalized<br>";
                }
            }
        }
    }
    if (!$found && $default ne "") {
    $popup .= "<INPUT NAME=$name TYPE=CHECKBOX CHECKED>$default";
    }
    return $popup;
}


# subroutine: 	make_selection_widget
# description:	creates an HTML selection widget from a list of text strings.
# params:		$groupname = the name of the setting (form value) that this widget will control (scalar)
# 				$src = the list of options (array ref)
# 				$default = value which is either a string or a regex pattern to match to identify the value (scalar)
# 				$capitalize = optionally capitalize the option strings; the default is the value
#    			of Param("capitalizelists") (scalar)
# 				$multiple = 1 if several options are selectable (default), 0 otherwise. (scalar)
# 				$size  = used for lists to control how many items are shown. The default is 7. A list of
#    			size 1 becomes a popup menu. (scalar)
# 				$preferLists = 1 if selection lists should be used in favor of radio buttons and
#    			checkboxes, and 0 otherwise. The default is the value of Param("preferlists"). (scalar)
# returns:		$ popup = formatted html for display (scalar)
#
# The actual widget generated depends on the parameter settings:
# 
#        MULTIPLE     PREFERLISTS    SIZE     RESULT
#       0 (single)        0           =1      Popup Menu (normal for list of size 1)
#       0 (single)        0           >1      Radio buttons
#       0 (single)        1           =1      Popup Menu (normal for list of size 1)
#       0 (single)        1           n>1     List of size n, single selection
#       1 (multi)         0           n/a     Check boxes; size ignored
#       1 (multi)         1           n/a     List of size n, multiple selection, of size n

sub make_selection_widget {
    my ($groupname,$src,$default,$isregexp,$multiple, $size, $capitalize, $preferLists) = (@_);
    my $last = "";
    my $popup = "";
    my $found = 0;
    my $displaytext = "";
    $groupname = "" if !defined $groupname;
    $default = "" if !defined $default;
    $capitalize = Param("capitalizelists") if !defined $capitalize;
    $multiple = 1 if !defined $multiple;
    $preferLists = Param("preferlists") if !defined $preferLists;
    $size = 7 if !defined $size;
    my $type = "LIST";
    if (!$preferLists) {
        if ($multiple) {
            $type = "CHECKBOX";
        } else {
            if ($size > 1) {
                $type = "RADIO";
            }
        }
    }

    if ($type eq "LIST") {
        $popup .= "<SELECT NAME=\"$groupname\"";
        if ($multiple) {
            $popup .= " MULTIPLE";
        }
        $popup .= " SIZE=$size>\n";
    }
    if ($src) {
        foreach my $item (@$src) {
            if ($item eq "-blank-" || $item ne $last) {
                if ($item eq "-blank-") {
                    $item = "";
                }
                $last = $item;
                $displaytext = $item;
                if ($capitalize) {
                    $displaytext =~ tr/A-Z/a-z/;
                    $displaytext =~ s/^(.?)(.*)/\u$1$2/;
                }

                if ($isregexp ? $item =~ $default : $default eq $item) {
                    if ($type eq "CHECKBOX") {
                        $popup .= "<INPUT NAME=$groupname type=checkbox VALUE=\"$item\" CHECKED>$displaytext<br>";
                    } elsif ($type eq "RADIO") {
                        $popup .= "<INPUT NAME=$groupname type=radio VALUE=\"$item\" check>$displaytext<br>";
                    } else {
                        $popup .= "<OPTION SELECTED VALUE=\"$item\">$displaytext";
                    }
                    $found = 1;
                } else {
                    if ($type eq "CHECKBOX") {
                        $popup .= "<INPUT NAME=$groupname type=checkbox VALUE=\"$item\">$displaytext<br>";
                    } elsif ($type eq "RADIO") {
                        $popup .= "<INPUT NAME=$groupname type=radio VALUE=\"$item\">$displaytext<br>";
                    } else {
                        $popup .= "<OPTION VALUE=\"$item\">$displaytext";
                    }
                }
            }
        }
    }
    if (!$found && $default ne "") {
        if ($type eq "CHECKBOX") {
            $popup .= "<INPUT NAME=$groupname type=checkbox CHECKED>$default";
        } elsif ($type eq "RADIO") {
            $popup .= "<INPUT NAME=$groupname type=radio checked>$default";
        } else {
            $popup .= "<OPTION SELECTED>$default";
        }
    }
    if ($type eq "LIST") {
        $popup .= "</SELECT>";
    }
    return $popup;
}


$::CheckOptionValues = 1;


# subroutine: 	make_options
# description:	create html popup menu options
# params:		$src = list of options (array ref)
#				$default = default item to be selected (scalar)
#				$isregexp = (scalar)
# returns:		$popup = formatted html options list (scalar)

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
	        if (!$src) {
    	        $src = ["???null???"];
        	}
        	print "<pre>src = " . value_quote(join(' ', @$src)) . "\n";
        	print "default = " . value_quote($default) . "</pre>";
  			PutFooter();
			confess ("Gulp");
    		exit 0;
		} else {
    		$popup .= "<OPTION SELECTED>$default";
      	}
    }
    return $popup;
}


# subroutine:	make_popup
# description:	
# params:
# returns:

sub make_popup {
    my ($name,$src,$default,$listtype,$onchange) = (@_);
    my $popup = "<SELECT NAME=$name";
    if ($listtype > 0) {
        $popup .= " SIZE=7";
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


# subroutine:	BuildPullDown
# description:	build html pull down menu 
# params:		$name = name for selection pull down (scalar)
#				$valuelist = array reference to list of pull down items (array ref)
#				$default = default item selected (scalar)
# returns:

sub BuildPulldown {
    my ($name, $valuelist, $default) = (@_);

    my $entry = qq{<SELECT NAME="$name">};
    foreach my $i (@$valuelist) {
        my ($tag, $desc) = (@$i);
        my $selectpart = "";
        if ($tag eq $default) {
            $selectpart = " SELECTED";
        }
        $entry .= qq{<OPTION$selectpart VALUE="$tag">$desc\n};
    }
    $entry .= qq{</SELECT>};
    return $entry;
}


# subroutine:	PasswordForLogin
# description:  Get crypted password from database for member verification
# params:		$login = login name of member (scalar)
# returns:		$result = crypted password (scalar)

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


# subroutine:	quietly_check_login
# description:	compare stored cookie to database to confirm member's login
# params:		none
# returns:		1 = member has been verified
#				0 = member has not been verified

sub quietly_check_login() {
    $::usergroupset = '0';
    $::disabledreason = '';
	$::userid = 0;
    my $loginok = 0;
    if (defined $::COOKIE{"Bugzilla_login"} &&
        defined $::COOKIE{"Bugzilla_logincookie"}) {
        ConnectToDatabase();
        if (!defined $ENV{'REMOTE_HOST'}) {
            $ENV{'REMOTE_HOST'} = $ENV{'REMOTE_ADDR'};
        }
        my $query = "select profiles.userid, profiles.login_name, profiles.groupset, " .
					"profiles.disabledtext from profiles, logincookies " .
                    "where profiles.userid = logincookies.userid " .
                    "and profiles.cryptpassword = logincookies.cryptpassword " .
                    " and logincookies.hostname = " . SqlQuote($ENV{'REMOTE_HOST'}) .
                    " and logincookies.cookie = " . SqlQuote($::COOKIE{'Bugzilla_logincookie'}) .
                    " and profiles.login_name = " . SqlQuote($::COOKIE{'Bugzilla_login'});
		SendSQL($query);
		my @row;
        if (@row = FetchSQLData()) {
            my ($userid, $loginname, $groupset, $disabledtext) = (@row);
            if ($loginname) {
                if ($disabledtext eq '') {
                    $loginok = 1;
					$::userid = $userid;
                    $::usergroupset = $groupset;
                    $::COOKIE{"Bugzilla_login"} = $loginname; # Makes sure case
                                                              # is in
                                                              # canonical form.
                } else {
                    $::disabledreason = $disabledtext;
                }
            }
        }
    }
	# if 'who' is passed in, verify that it's a good value
	if ($::FORM{'who'}) {
		my $whoid = DBname_to_id($::FORM{'who'});
		delete $::FORM{'who'} unless $whoid;
	}
    if (!$loginok) {
        delete $::COOKIE{"Bugzilla_login"};
    }
	return $loginok;
}


# subroutine:	CheckEmailSyntax
# description:	check members email address for correctness
# params:		$addr = entered email address (scalar)
# returns:		none

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
		PutFooter();
        exit;
    }
}


# subroutine:	MailPassword
# description:  Mail a password to the newly added member or current member
# params:		$login = email address of the newly added member or current member (scalar)
#				$password = password to be mailed (scalar)
#				$quiet = do not print a confirmation message (scalar) (optional)
# returns:		none

sub MailPassword {
    my ($login, $password, $quiet) = (@_);
    my $urlbase = Param("urlbase");
    my $template = Param("passwordmail");
    my $msg = PerformSubsts($template,
                            {"mailaddress" => $login . Param('emailsuffix'),
                             "login" => $login,
                             "password" => $password});

    open SENDMAIL, "|/usr/lib/sendmail -t";
    print SENDMAIL $msg;
    close SENDMAIL;

	if (!$quiet) {
	    print "<CENTER>The password for the e-mail address\n";
	    print "$login has been e-mailed to that address.\n";
	    print "<p>When the e-mail arrives, you can click <b>Back</b>\n";
	    print "and enter your password in the form there.</CENTER>\n";
	}
}


# subroutine: 	confirm_login
# description:	confirms a users identity by checking the user information in the database
# params:		$nexturl = add a next url (optional) (scalar)
# returns: 		none

sub confirm_login {
    my ($nexturl) = (@_);

	# Uncommenting the next line can help debugging...
	# print "Content-type: text/plain\n\n";

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
	    	PutHeader("Password has been emailed");
            MailPassword($enteredlogin, $realpwd);
  			PutFooter();
            exit;
	    }

		my $enteredcryptpwd = crypt($enteredpwd, substr($realcryptpwd, 0, 2));
        if ($realcryptpwd eq "" || $enteredcryptpwd ne $realcryptpwd) {
            print "Content-type: text/html\n\n";
	    	PutHeader("Login failed");
            print "<CENTER>The username or password you entered is not valid.\n";
            print "Please click <b>Back</b> and try again.</CENTER>\n";
            exit;
        }
        $::COOKIE{"Bugzilla_login"} = $enteredlogin;
        if (!defined $ENV{'REMOTE_HOST'}) {
            $ENV{'REMOTE_HOST'} = $ENV{'REMOTE_ADDR'};
        }

		if ($::driver eq "mysql") {
			SendSQL("insert into logincookies (userid, cryptpassword, hostname) values " .
					"(@{[DBNameToIdAndCheck($enteredlogin)]}, @{[SqlQuote($realcryptpwd)]}, " .
					"@{[SqlQuote($ENV{'REMOTE_HOST'})]})");
        	SendSQL("select LAST_INSERT_ID()");
		} else {
            SendSQL("insert into logincookies (cookie, userid, cryptpassword, hostname) values " .
                    "(logincookies_seq.nextval, @{[DBNameToIdAndCheck($enteredlogin)]}, @{[SqlQuote($realcryptpwd)]}, " .
                    "@{[SqlQuote($ENV{'REMOTE_HOST'})]})");
			SendSQL("select logincookies_seq.currval from dual");
		}			
		my $logincookie = FetchOneColumn();

        $::COOKIE{"Bugzilla_logincookie"} = $logincookie;
        print "Set-Cookie: Bugzilla_login=$enteredlogin ; path=/bugzilla2/; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
        print "Set-Cookie: Bugzilla_logincookie=$logincookie ; path=/bugzilla2/; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";

        # This next one just cleans out any old bugzilla passwords that may
        # be sitting around in the cookie files, from the bad old days when
        # we actually stored the password there.
        print "Set-Cookie: Bugzilla_password= ; path=/bugzilla2/; expires=Sun, 30-Jun-80 00:00:00 GMT\n";

    }

    my $loginok = quietly_check_login();

    if ($loginok != 1) {
		if ($::disabledreason) {
            print "Set-Cookie: Bugzilla_login= ; path=/bugzilla2/; expires=Sun, 30-Jun-80 00:00:00 GMT
Set-Cookie: Bugzilla_logincookie= ; path=/bugzilla2/; expires=Sun, 30-Jun-80 00:00:00 GMT
Set-Cookie: Bugzilla_password= ; path=/bugzilla2/; expires=Sun, 30-Jun-80 00:00:00 GMT
Content-type: text/html

";
            PutHeader("Your account has been disabled");
            print $::disabledreason;
            print "<HR>\n";
            print "If you believe your account should be restored, please\n";
            print "send email to " . Param("maintainer") . " explaining\n";
            print "why.\n";
            PutFooter();
            exit();
        }
        print "Content-type: text/html\n\n";
        PutHeader("Login", undef, undef, undef, 1);
        print "<CENTER><FONT SIZE=+1><B>I need a legitimate e-mail address and password to continue.
			</B></FONT></CENTER>\n";
        if (!defined $nexturl || $nexturl eq "") {
	    	# Sets nexturl to be argv0, stripping everything up to and
	    	# including the last slash.
	    	$0 =~ m:[^/]*$:;
	    	$nexturl = $&;
        }
        my $method = "POST";
#        if (defined $ENV{"REQUEST_METHOD"} && length($::buffer) > 1) {
#            $method = $ENV{"REQUEST_METHOD"};
#        }
        print "
<FORM action=$nexturl method=$method>
<TABLE ALIGN=center>
<TR>
	<TD ALIGN=right><B>E-mail address:</B></TD>
	<TD><INPUT SIZE=35 NAME=Bugzilla_login></TD>
	<TD VALIGN=center ROWSPAN=2><INPUT TYPE=submit VALUE=Login NAME=GoAheadAndLogIn></TD>
</TR><TR>
	<TD ALIGN=right><B>Password:</B></TD>
	<TD><INPUT TYPE=password SIZE=35 NAME=Bugzilla_password></TD>
</TR>
</TABLE>
";
        foreach my $i (keys %::FORM) {
            if ($i =~ /^Bugzilla_/) {
                next;
            }
            print "<INPUT TYPE=hidden NAME=$i VALUE=\"@{[value_quote($::FORM{$i})]}\">\n";
        }
        print qq{
<HR WIDTH=800 ALIGN=center>
<CENTER>If you don't have a password, or have forgotten it, then please fill in the
e-mail address above and click here:
<INPUT TYPE=submit VALUE="E-mail me a password" NAME=PleaseMailAPassword>
</CENTER>
</FORM>
};

        # This seems like as good as time as any to get rid of old
        # crufty junk in the logincookies table.  Get rid of any entry
        # that hasn't been used in a month.
		if ($::dbwritesallowed) {
			if ($::driver eq "mysql") {
	        	SendSQL("delete from logincookies where to_days(now()) - to_days(lastused) > 30");
			} else {
				SendSQL("delete from logincookies where sysdate - lastused > 30");
	        }
		}
        exit;
    }

    # Update the timestamp on our logincookie, so it'll keep on working.
	if ($::driver eq "mysql") {
    	SendSQL("update logincookies set lastused = null " .
				"where cookie = $::COOKIE{'Bugzilla_logincookie'}");
	} else {
        SendSQL("update logincookies set lastused = sysdate " .
                "where cookie = $::COOKIE{'Bugzilla_logincookie'}");
	}
}


# subroutine:	PutHeader
# description:	Prints html for top of each page
# params:		$title = title for top of page (scalar)
#				$h1 = header of page (scalar)	
#				$h2 = additional header information (scalar)
#				$extra = additional page information (scalar)
#				$ignoreshutdown = ignore displaying shutdown information (scalar)
# returns:		none
 
sub PutHeader {
    my ($title, $h1, $h2, $extra, $ignoreshutdown, $jscript) = (@_);
	my %header;

    if (!defined $h1) {
        $h1 = $title;
    }
    if (!defined $h2) {
        $h2 = "";
    }
    if (!defined $extra) {
        $extra = "";
    }
    if ($h1 ne "" && $h2 ne "") {
        $h1 .= "&nbsp;-&nbsp;";
    }

	$header{'title'} = $title;
	$header{'header'} = "$h1 $h2";	
	$header{'navigation'} = navigation_header();
	$header{'jscript'} = $jscript;
	$header{'login'} = "";
	$header{'bannerhtml'} = PerformSubsts(Param("bannerhtml"), undef);

	if (defined($::COOKIE{'Bugzilla_login'})) {
		$header{'login'} = "<B>Login: </B>$::COOKIE{'Bugzilla_login'}";
	}

	print LoadTemplate('header_redhat.tmpl', \%header);

	if (Param("shutdownhtml")) {
        if (!$ignoreshutdown) {
            print Param("shutdownhtml");
            exit;
        }
    }
}


# subroutine:	PutFooter
# description:	Prints html for display at the bottom of a page
# params:		none
# returns:		none

sub PutFooter {
	print LoadTemplate('footer_redhat.tmpl');
#    print PerformSubsts(Param("footerhtml"));
    SyncAnyPendingShadowChanges();
}


# subroutine: 	PuntTryAgain
# description:	outputs a formatted error message
# params:		$str = error message to ouput (scalar)
# returns:		none

sub PuntTryAgain ($) {
    my ($str) = (@_);
    print PerformSubsts(Param("errorhtml"),
                        {errormsg => $str});
    PutFooter();
    exit;
}


# subroutine:	CheckIfVotedConfirmed
# description:	
# params:		$id = bug number (scalar)
# returns:		none

sub CheckIfVotedConfirmed {
    my ($id, $who) = (@_);
    SendSQL("SELECT bugs.votes, bugs.bug_status, products.votestoconfirm, " .
            "       bugs.everconfirmed " .
            "FROM bugs, products " .
            "WHERE bugs.bug_id = $id AND products.product = bugs.product");
    my ($votes, $status, $votestoconfirm, $everconfirmed) = (FetchSQLData());
    if ($votes >= $votestoconfirm && $status eq $::unconfirmedstate) {
        SendSQL("UPDATE bugs SET bug_status = 'NEW', everconfirmed = 1 " .
                "WHERE bug_id = $id");
        my $fieldid = GetFieldID("bug_status");
		if ($::driver eq 'mysql') {
	        SendSQL("INSERT INTO bugs_activity " .
	                "(bug_id, who, bug_when, fieldid, oldvalue, newvalue) VALUES " .
	                "($id, $who, now(), $fieldid, '$::unconfirmedstate', 'NEW')");
		} else {
			SendSQL("INSERT INTO bugs_activity " .
                    "(bug_id, who, bug_when, fieldid, oldvalue, newvalue) VALUES " .
                    "($id, $who, sysdate, fieldid, '$::unconfirmedstate', 'NEW')");
		}
        if (!$everconfirmed) {
            $fieldid = GetFieldID("everconfirmed");
			if ($::driver eq 'mysql') {
	            SendSQL("INSERT INTO bugs_activity " .
  	                    "(bug_id, who, bug_when, fieldid, oldvalue, newvalue) VALUES " .
	                    "($id, $who, now(), $fieldid, '0', '1')");
			} else {
				SendSQL("INSERT INTO bugs_activity " .
                        "(bug_id, who, bug_when, fieldid, oldvalue, newvalue) VALUES " .
                        "($id, $who, sysdate, $fieldid, '0', '1')");
			}
        }
        AppendComment($id, DBID_to_name($who),
                      "*** This bug has been confirmed by popular vote. ***");
        print "<TABLE BORDER=1><TD><H2>Bug $id has been confirmed by votes.</H2>\n";
        system("./processmail", $id);
        print "<TD><A HREF=\"show_bug.cgi?id=$id\">Go To BUG# $id</A></TABLE>\n";
    }
}


# subroutine: 	PutNotify
# description: 	puts popup to notify important things such as contract bugs
# params: 		$userid = Userid for the current user (scalar)
# returns: 		none

sub PutNotify {
	my $userid = shift;
	my %contract_form;

	my $query = "select bug_id from bugs where assigned_to = $userid and priority = " .
				SqlQuote('contract') . " and bug_status != " . SqlQuote('RESOLVED') .
				" and bug_status != " . SqlQuote('CLOSED');
	my @bug_list;
	$contract_form{'count'} = 0;
	SendSQL($query);
	while (my @row = FetchSQLData()) {
		push (@bug_list, $row[0]);
		$contract_form{'count'}++;
	}

	if ($contract_form{'count'} > 0) {
		$contract_form{'link'} = "<A HREF=\"buglist.cgi?sql=bugs.bug_id%20in%20(" .
			join(",", @bug_list) . ")\">";

		print LoadTemplate('contract_redhat.tmpl', \%contract_form);
	}
}


# subroutine: 	DumpBugActivity
# description:  Prints table containing  changes made to the particular report
# params:		$id = id of the bug report (scalar)
#				$starttime = date to start displaying from
# returns:		none

sub DumpBugActivity {
    my ($id, $starttime) = (@_);
    my $datepart = "";

    die "Invalid id: $id" unless $id=~/^\s*\d+\s*$/;

    if (defined $starttime) {
		if ($::driver eq "mysql") {
        	$datepart = "and bugs_activity.bug_when >= $starttime";
		} else {
			$datepart = "and bugs_activity.bug_when >= TO_DATE('$starttime', 'YYYYMMDDHH24MISS') ";
		}
    }
	
	my $query = "";
	if ($::driver eq "mysql") {
    	$query ="
         SELECT IFNULL(fielddefs.name, bugs_activity.fieldid),
                  bugs_activity.bug_when,
                  bugs_activity.oldvalue, bugs_activity.newvalue,
                  profiles.login_name
         FROM bugs_activity LEFT JOIN fielddefs ON 
                  bugs_activity.fieldid = fielddefs.fieldid,
              	  profiles
         WHERE bugs_activity.bug_id = $id $datepart
         AND profiles.userid = bugs_activity.who
         ORDER BY bugs_activity.bug_when";

	} else {
       $query = 
        	"select fielddefs.name, " . 
			"TO_CHAR(bugs_activity.bug_when, 'YYYY-mm-dd HH:MI:SS'), " .
        	"bugs_activity.oldvalue, bugs_activity.newvalue, " .
        	"profiles.login_name " . 
        	"from bugs_activity, profiles, fielddefs " .
        	"where bugs_activity.bug_id = $id $datepart " .
			"and bugs_activity.fieldid = fielddefs.fieldid (+) " .
       		"and profiles.userid = bugs_activity.who " .
       		"order by bugs_activity.bug_when";

	}
    SendSQL($query);
    
    print "<P>\n<TABLE BORDER=1 CELLPADDING=3 CELLSPACING=0 ALIGN=center>\n";
    print "<TR BGCOLOR=\"BFBFBF\">\n";
    print "    <TH ALIGN=left>Who</TH>\n";
	print "	   <TH ALIGN=left>What</TH>\n";
	print "    <TH ALIGN=left>Old value</TH>\n";
	print "    <TH ALIGN=left>New value</TH>\n";
	print "    <TH ALIGN=left>When</TH>\n";
    print "</TR>\n";
    
    my @row;
    while (@row = FetchSQLData()) {
        my ($field, $when, $old, $new, $who) = (@row);
        $old = value_quote($old);
        $new = value_quote($new);
        if ($old eq "") {
            $old = "&nbsp;";
        }
        if ($new eq "") {
            $new = "&nbsp;";
        }
#		$field = GetFieldID($field);
        print "<TR BGCOLOR=\"#ECECEC\">\n";
        print "<TD ALIGN=left>$who</TD>\n";
        print "<TD ALIGN=left>$field</TD>\n";
        print "<TD ALIGN=left>$old</TD>\n";
        print "<TD ALIGN=left>$new</TD>\n";
        print "<TD ALIGN=left>$when</TD>\n";
        print "</TR>\n";
    }
    print "</TABLE>\n<P>\n";
}


# subroutine:	warnBanner
# description:	Prints a warnbanner incl. image with given message
# params:		$msg = message to print (scalar)
# returns: 		none

sub warnBanner($) {
  my ($msg) = (@_);
  print Param("warnbannerhtml");
  print $msg;
  print Param("warnfooterhtml");
}


# subroutine:	GetCommandMenu
# description:	command menu for adding to footer bar
# params:		none
# returns:		$html = formatted string containing html (scalar)

sub GetCommandMenu {
    my $loggedin = quietly_check_login();
    my $html = qq{<FORM METHOD=GET ACTION="show_bug.cgi">};
    $html .= "<a href='enter_bug.cgi'>New</a> | <a href='query.cgi'>Query</a>";
    if (-e "query2.cgi") {
        $html .= "[<a href='query2.cgi'>beta</a>]";
    }

    $html .=
        qq{ | <INPUT TYPE=SUBMIT VALUE="Find"> bug \# <INPUT NAME=id SIZE=6>};

    $html .= " | <a href='reports.cgi'>Reports</a>";
    if ($loggedin) {
        my $mybugstemplate = Param("mybugstemplate");
        my %substs;
        $substs{'userid'} = url_quote($::COOKIE{"Bugzilla_login"});
        if (!defined $::anyvotesallowed) {
            GetVersionTable();
        }
        if ($::anyvotesallowed) {
            $html .= qq{ | <A HREF="showvotes.cgi"><NOBR>My votes</NOBR></A>};
        }
        SendSQL("SELECT mybugslink, userid, blessgroupset FROM profiles " .
				"WHERE login_name = " . SqlQuote($::COOKIE{'Bugzilla_login'}));
        my ($mybugslink, $userid, $blessgroupset) = (FetchSQLData());
        if ($mybugslink) {
            my $mybugsurl = PerformSubsts($mybugstemplate, \%substs);
            $html = $html . " | <A HREF='$mybugsurl'><NOBR>My bugs</NOBR></A>";
        }
        SendSQL("SELECT name,query FROM namedqueries " .
                "WHERE userid = $userid AND linkinfooter");
        while (MoreSQLData()) {
            my ($name, $query) = (FetchSQLData());
            $html .= qq{ | <A HREF="buglist.cgi?$query"><NOBR>$name</A>};
        }
        $html .= " | <NOBR>Edit <a href='userprefs.cgi'>prefs</a></NOBR>";
        if (UserInGroup("tweakparams")) {
            $html .= ", <a href=editparams.cgi>parameters</a>";
            $html .= ", <a href=sanitycheck.cgi><NOBR>sanity check</NOBR></a>";
        }
        if (UserInGroup("editusers" || $blessgroupset)) {
			$html .= ", <a href=editmembers.cgi>users</a>";
#            $html .= ", <a href=editusers.cgi>users</a>";
        }
        if (UserInGroup("editcomponents")) {
            $html .= ", <a href=editproducts.cgi>components</a>";
        }
        if (UserInGroup("editkeywords")) {
            $html .= ", <a href=editkeywords.cgi>keywords</a>";
        }
        $html .= " | <NOBR><a href=relogin.cgi>Log out</a> $::COOKIE{'Bugzilla_login'}</NOBR>";
    } else {
        $html .=
            " | <a href=\"createaccount.cgi\"><NOBR>New account</NOBR></a>\n";
        $html .=
            " | <NOBR><a href=query.cgi?GoAheadAndLogIn=1>Log in</a></NOBR>";
    }
    $html .= "</FORM>";
    return $html;
}


# subroutine:	GetAdminMenu
# description:  returns html for creating menu for administrative tasks
# params:		none
# returns:		$html = string containing html for admin menu (scalar)

sub GetAdminMenu {
	my $admin_menu = "";
	if (UserInGroup("tweakparams")) {
    	$admin_menu .= "<a href=\"editparams.cgi\">Edit Bugzilla parameters</a><br>\n";
	}
	if (UserInGroup("editcomponents")) {
    	$admin_menu .= "<a href=\"editcomponents.cgi\">Edit Bugzilla components</a><br>\n";
	}
	if (UserInGroup("editgroupmembers")) {
    	$admin_menu .= "<a href=\"editmembers.cgi\">Edit Bugzilla members</a><br>\n";
	}
	if (UserInGroup("editcomponents")) {
    	$admin_menu .= "<a href=\"editversions.cgi\">Edit Bugzilla versions</a><br>\n";
	}
	if (UserInGroup("editcomponents")) {
    	$admin_menu .= "<a href=\"editproducts.cgi\">Edit Bugzilla products</a><br>\n";
	}
	if (UserInGroup("creategroups")) {
    	$admin_menu .= "<a href=\"editgroups.cgi\">Edit Bugzilla groups</a><br>\n";
	}
	if (UserInGroup("addnews")) {
    	$admin_menu .= "<a href=\"editnews.cgi\">Edit Bugzilla news items</a><br>\n";
	}
	if (UserInGroup("errata")) {
    	$admin_menu .= "<a href=\"http://porkchop.redhat.com/bugzilla/listerrata.cgi?type=update\">Edit past errata</a><br>\n";
	}
	return $admin_menu;
}


# subroutine: 	LoadTemplate
# description: 	loads in specified template file and fills in variables
# params: 		$filename = filename of template to load (scalar)
#				$hasref = hash containing fill in vars (optional, if none supplied vars must be global) (hash ref)
# returns: 		$html = string var containing formatted text (scalar)

sub LoadTemplate {
	my ($filename, $hashref) = (@_);
	my $html = "";
	my $template_dir = Param('templatepath');

	my $template = new Text::Template(TYPE => 'file',
									  SOURCE => "$template_dir/$filename",
									  DELIMITERS => ['[+', '+]']);

	if (defined($hashref)) {
		$html = $template->fill_in(HASH => $hashref);
	} else {
		$html = $template->fill_in();
	}

	if (defined($html)) {
		return $html;
	} else {
		die "Could not fill in template: $Text::Template::ERROR";
	}
}


# subroutine:   GetHeadlines
# description:  gets a list of current headlines with date
# params:       none
# returns:      $html = string containing current headlines or 'No News Yet' if no headlines

sub GetHeadlines {
    my $html = "";
    my $query = "";
	my $success = 0;

    if ($::driver eq 'mysql') {
        $query = "select id, DATE_FORMAT(add_date, '\%b \%d, \%Y \%l:\%i'), headline " .
                 "from news where to_days(now()) - to_days(add_date) < 30";
    } else {
        $query = "select id, TO_CHAR(add_date, 'MON DD, YYYY HH:MI'), headline " .
                 "from news where sysdate - add_date < 30";
    }

    SendSQL($query);
    while (my @row = FetchSQLData()) {
		$success = 1;
        my ($id, $add_date, $headline) = (@row);
        $html .= "<P><TABLE WIDTH=100% CELLSPACING=0 CELLPADDING=3 BORDER=0>\n";
        $html .= "<TR>\n<TD ALIGN=left BGCOLOR=\"#ECECEC\">\n";
        $html .= "<FONT SIZE=+2><A HREF=\"news.cgi?id=$id\">$headline</A></FONT><BR>\n";
        $html .= "<FONT SIZE=-1>Added on <I>$add_date</I></FONT></TD>\n";
        $html .= "</TR>\n</TABLE><P>\n";
    }
	if ($success) {
    	return $html;
	} else {
		return "<B>No news is good news.</B>\n";
	}
}


# subroutine:	PutError
# description:	prints error message to the browser and stops
# params:		$msg = error message to print (scalar)
# returns:		none

sub PutError {
	my ($msg) = (@_);
	print "<P><CENTER><FONT SIZE=+1>$msg</FONT></CENTER><P>\n";
	exit;
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
