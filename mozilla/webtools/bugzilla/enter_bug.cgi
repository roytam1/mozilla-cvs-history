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

use diagnostics;
use strict;

require "CGI.pl";
require "globals.pl";

my $body = Param("prefix") . "-enterform.pl";
require "$body";
$body = Param("prefix") . "-security.pl";
require "$body";

# Shut up misguided -w warnings about "used only once":
use vars @::buffer,
    @::legal_severity,
    @::legal_opsys,
    @::legal_platforms,
    @::legal_priority;


if (!defined $::FORM{'product'}) {
    GetVersionTable();
    my @prodlist = keys %::versions;
    if ($#prodlist != 0) {
        print "Content-type: text/html\n\n";
        PutHeader("Enter Bug");
        
        print "<H2>First, you must pick a product on which to enter\n";
        print "a bug.</H2>\n";
        print "<table>";
        foreach my $p (sort (@prodlist)) {
            print "<tr><th align=\"right\" valign=\"top\"><a href=\"enter_bug.cgi?product=" . url_quote($p) . "\"&$::buffer>$p</a>:</th>\n";
            if (defined $::proddesc{$p}) {
                print "<td valign=\"top\">$::proddesc{$p}</td>\n";
            }
            print "</tr>";
        }
        print "</table>\n";
        exit;
    }
    $::FORM{'product'} = $prodlist[0];
}

my $product = url_decode($::FORM{'product'});

confirm_login();

print "Content-type: text/html\n\n";

sub formvalue {
    my ($name, $default) = (@_);
    if (exists $::FORM{$name}) {
        return $::FORM{$name};
    }
    if (defined $default) {
        return $default;
    }
    return "";
}

sub pickplatform {
    my $value = formvalue("rep_platform");
    if ($value ne "") {
        return $value;
    }
    for ($ENV{'HTTP_USER_AGENT'}) {
        /Mozilla.*\(X11/ && do {return "X-Windows";};
        /Mozilla.*\(Windows/ && do {return "PC";};
        /Mozilla.*\(Macintosh/ && do {return "Macintosh";};
        /Mozilla.*\(Win/ && do {return "PC";};
        # default
        return "PC";
    }
}



sub pickversion {
    my $version = formvalue('version');
    if ($version eq "") {
        if ($ENV{'HTTP_USER_AGENT'} =~ m@Mozilla[ /]([^ ]*)@) {
            $version = $1;
        }
    }
    
    if (lsearch($::versions{$product}, $version) >= 0) {
        return $version;
    } else {
        if (defined $::COOKIE{"VERSION-$product"}) {
            if (lsearch($::versions{$product},
                        $::COOKIE{"VERSION-$product"}) >= 0) {
                return $::COOKIE{"VERSION-$product"};
            }
        }
    }
    return $::versions{$product}->[0];
}

sub pickarch {
    my $arch = formvalue('arch');
    if ($arch eq "") {
        if ($ENV{'HTTP_USER_AGENT'} =~ m#Mozilla.*\(.*;.*;.* (.*)\)#) {
            $arch = $1;
        }
    }
    if( $arch =~ /i.86/ ) {
	$arch = "i386";
    }
    if (lsearch($::legal_platforms{$product}, $arch) < 0) {
	$arch = "";
    }
    return $arch;
}

sub pickcomponent {
    my $result =formvalue('component');
    if ($result ne "" && lsearch($::components{$product}, $result) < 0) {
        $result = "";
    }
    return $result;
}


sub pickos {
    if (formvalue('op_sys') ne "") {
        return formvalue('op_sys');
    }
    for ($ENV{'HTTP_USER_AGENT'}) {
        /Mozilla.*\(.*;.*; IRIX.*\)/    && do {return "IRIX";};
        /Mozilla.*\(.*;.*; 32bit.*\)/   && do {return "Windows 95";};
        /Mozilla.*\(.*;.*; 16bit.*\)/   && do {return "Windows 3.1";};
        /Mozilla.*\(.*;.*; 68K.*\)/     && do {return "System 7.5";};
        /Mozilla.*\(.*;.*; PPC.*\)/     && do {return "System 7.5";};
        /Mozilla.*\(.*;.*; OSF.*\)/     && do {return "OSF/1";};
        /Mozilla.*\(.*;.*; Linux.*\)/   && do {return "Linux";};
        /Mozilla.*\(.*;.*; SunOS 5.*\)/ && do {return "Solaris";};
        /Mozilla.*\(.*;.*; SunOS.*\)/   && do {return "SunOS";};
        /Mozilla.*\(.*;.*; SunOS.*\)/   && do {return "SunOS";};
        /Mozilla.*\(.*;.*; BSD\/OS.*\)/ && do {return "BSDI";};
        /Mozilla.*\(Win16.*\)/          && do {return "Windows 3.1";};
        /Mozilla.*\(Win95.*\)/          && do {return "Windows 95";};
        /Mozilla.*\(WinNT.*\)/          && do {return "Windows NT";};
        # default
        return "other";
    }
}


GetVersionTable();

my $assign_element = GeneratePersonInput('assigned_to', 1,
                                         formvalue('assigned_to'));
my $cc_element = GeneratePeopleInput('cc', 45, formvalue('cc'));


my $priority_popup = make_popup('priority', \@::legal_priority,
                                formvalue('priority', 'normal'), 0);
my $sev_popup = make_popup('bug_severity', \@::legal_severity,
                           formvalue('bug_severity', 'normal'), 0);
my $opsys_popup = make_popup('op_sys', \@::legal_opsys, pickos(), 0);

my $component_popup = make_popup('component', $::components{$product},
                                 formvalue('component'), 1);
my $platforms_popup = make_popup('rep_platform', $::legal_platforms{$product},
                                pickarch(), 0);

PutHeader ("Enter Bug");

PutBody($product, $component_popup, $platforms_popup, 
            $opsys_popup, $priority_popup, $sev_popup, 
            $assign_element, $cc_element);
