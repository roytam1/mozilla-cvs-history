#!/usr/bonsaitools/bin/perl -wT
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
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dave Miller <justdave@syndicomm.com>
#                 Joe Robins <jmrobins@tgix.com>
#                 Gervase Markham <gerv@gerv.net>

##############################################################################
#
# enter_bug.cgi
# -------------
# Displays bug entry form. Bug fields are specified through popup menus, 
# drop-down lists, or text fields. Default for these values can be 
# passed in as parameters to the cgi.
#
##############################################################################

use diagnostics;
use strict;


use lib qw(.);

require "CGI.pl";

use vars qw(
  $unconfirmedstate
  $template
  $vars
  %COOKIE
  @enterable_products
  @legal_opsys
  @legal_platform
  @legal_priority
  @legal_severity
  %MFORM
  %versions
);

# Suppress silly used once warnings
sub sillyness {
    my $zz;
    $zz = %::proddesc;
}

# If we're using bug groups to restrict bug entry, we need to know who the 
# user is right from the start.
my $userid = confirm_login();

if (!defined $::FORM{'product'}) {
    GetVersionTable();

    my %products;

    foreach my $p (@enterable_products) {
        # If we're using bug groups to restrict entry on products, and
        # this product is private to one or more bug groups and the user is not
        # in one of those groups group, we don't want to include that product in this list.
        next if !CanSeeProduct($userid, $p);
        $products{$p} = $::proddesc{$p};
    }
 
    my $prodsize = scalar(keys %products);
    if ($prodsize == 0) {
        DisplayError("Either no products have been defined to enter bugs ".
                     "against or you have not been given access to any.\n");
        exit;
    } 
    elsif ($prodsize > 1) {
        $vars->{'proddesc'} = \%products;

        $vars->{'target'} = "enter_bug.cgi";
        $vars->{'title'} = "Enter Bug";
        $vars->{'h2'} = 
                    "First, you must pick a product on which to enter a bug.";
        
        print "Content-type: text/html\n\n";
        $template->process("global/choose_product.tmpl", $vars)
          || DisplayError("Template process failed: " . $template->error());
        exit;        
    }

    $::FORM{'product'} = (keys %products)[0];
    $::MFORM{'product'} = [$::FORM{'product'}];

}

my $product = $::FORM{'product'};

##############################################################################
# Useful Subroutines
##############################################################################
sub formvalue {
    my ($name, $default) = (@_);
    return $::FORM{$name} || $default || "";
}

sub pickplatform {
    return formvalue("rep_platform") if formvalue("rep_platform");

    if ( Param('usebrowserinfo') ) {
        for ($ENV{'HTTP_USER_AGENT'}) {
        #PowerPC
            /\(.*PowerPC.*\)/i && do {return "Macintosh";};
            /\(.*PPC.*\)/ && do {return "Macintosh";};
            /\(.*AIX.*\)/ && do {return "Macintosh";};
        #Intel x86
            /\(.*[ix0-9]86.*\)/ && do {return "PC";};
        #Versions of Windows that only run on Intel x86
            /\(.*Windows 9.*\)/ && do {return "PC";};
            /\(.*Win9.*\)/ && do {return "PC";};
            /\(.*Windows 3.*\)/ && do {return "PC";};
            /\(.*Win16.*\)/ && do {return "PC";};
        #Sparc
            /\(.*sparc.*\)/ && do {return "Sun";};
            /\(.*sun4.*\)/ && do {return "Sun";};
        #Alpha
            /\(.*Alpha.*\)/i && do {return "DEC";};
        #MIPS
            /\(.*IRIX.*\)/i && do {return "SGI";};
            /\(.*MIPS.*\)/i && do {return "SGI";};
        #68k
            /\(.*68K.*\)/ && do {return "Macintosh";};
            /\(.*680[x0]0.*\)/ && do {return "Macintosh";};
        #ARM
#            /\(.*ARM.*\) && do {return "ARM";};
        #Stereotypical and broken
            /\(.*Macintosh.*\)/ && do {return "Macintosh";};
            /\(.*Mac OS [89].*\)/ && do {return "Macintosh";};
            /\(Win.*\)/ && do {return "PC";};
            /\(.*Windows NT.*\)/ && do {return "PC";};
            /\(.*OSF.*\)/ && do {return "DEC";};
            /\(.*HP-?UX.*\)/i && do {return "HP";};
            /\(.*IRIX.*\)/i && do {return "SGI";};
            /\(.*(SunOS|Solaris).*\)/ && do {return "Sun";};
        #Braindead old browsers who didn't follow convention:
            /Amiga/ && do {return "Macintosh";};
        }
    }
    # default
    return "Other";
}

sub pickos {
    if (formvalue('op_sys') ne "") {
        return formvalue('op_sys');
    }
    if ( Param('usebrowserinfo') ) {
        for ($ENV{'HTTP_USER_AGENT'}) {
            /\(.*IRIX.*\)/ && do {return "IRIX";};
            /\(.*OSF.*\)/ && do {return "OSF/1";};
            /\(.*Linux.*\)/ && do {return "Linux";};
            /\(.*SunOS 5.*\)/ && do {return "Solaris";};
            /\(.*SunOS.*\)/ && do {return "SunOS";};
            /\(.*HP-?UX.*\)/ && do {return "HP-UX";};
            /\(.*BSD\/OS.*\)/ && do {return "BSDI";};
            /\(.*FreeBSD.*\)/ && do {return "FreeBSD";};
            /\(.*OpenBSD.*\)/ && do {return "OpenBSD";};
            /\(.*NetBSD.*\)/ && do {return "NetBSD";};
            /\(.*BeOS.*\)/ && do {return "BeOS";};
            /\(.*AIX.*\)/ && do {return "AIX";};
            /\(.*IBM.*\)/ && do {return "OS/2";};
            /\(.*QNX.*\)/ && do {return "Neutrino";};
            /\(.*VMS.*\)/ && do {return "OpenVMS";};
#            /\(.*Windows XP.*\)/ && do {return "Windows XP";};
#            /\(.*Windows NT 5\.1.*\)/ && do {return "Windows XP";};
            /\(.*Windows 2000.*\)/ && do {return "Windows 2000";};
            /Windows NT 5.*\)/ && do {return "Windows 2000";};
            /\(Windows.*NT/ && do {return "Windows NT";};
            /\(.*Win.*98.*4\.9.*\)/ && do {return "Windows ME";};
            /\(.*Win98.*\)/ && do {return "Windows 98";};
            /\(.*Win95.*\)/ && do {return "Windows 95";};
            /\(.*Win16.*\)/ && do {return "Windows 3.1";};
            /\(.*WinNT.*\)/ && do {return "Windows NT";};
            /\(.*32bit.*\)/ && do {return "Windows 95";};
            /\(.*16bit.*\)/ && do {return "Windows 3.1";};
            /\(.*Mac OS 9.*\)/ && do {return "Mac System 9.x";};
            /\(.*Mac OS 8\.6.*\)/ && do {return "Mac System 8.6";};
            /\(.*Mac OS 8\.5.*\)/ && do {return "Mac System 8.5";};
        # Bugzilla doesn't have an entry for 8.1
            /\(.*Mac OS 8\.1.*\)/ && do {return "Mac System 8.0";};
            /\(.*Mac OS 8\.0.*\)/ && do {return "Mac System 8.0";};
            /\(.*Mac OS 8[^.].*\)/ && do {return "Mac System 8.0";};
            /\(.*Mac OS 8.*\)/ && do {return "Mac System 8.6";};
            /\(.*Mac OS X.*\)/ && do {return "MacOS X";};
            /\(.*Darwin.*\)/ && do {return "MacOS X";};
        # Silly
            /\(.*Mac.*PowerPC.*\)/ && do {return "Mac System 9.x";};
            /\(.*Mac.*PPC.*\)/ && do {return "Mac System 9.x";};
            /\(.*Mac.*68k.*\)/ && do {return "Mac System 8.0";};
        # Evil
            /Amiga/i && do {return "other";};
            /\(.*PowerPC.*\)/ && do {return "Mac System 9.x";};
            /\(.*PPC.*\)/ && do {return "Mac System 9.x";};
            /\(.*68K.*\)/ && do {return "Mac System 8.0";};
        }
    }
    # default
    return "other";
}

##############################################################################
# End of subroutines
##############################################################################

if (!CanSeeProduct($userid, $product)) {
    print "Content-type: text/html\n\n";
    PutHeader("Permission denied.", "Enter Bug", "This page lets you enter a new bug into Bugzilla.");
    print "<H1>Permission denied.</H1>\n";
    print "Sorry; you do not have the permissions necessary to enter\n";
    print "a bug against this product.\n";
    print "<P>\n";
    PutFooter();
    exit;
}

GetVersionTable();

if (lsearch(\@::enterable_products, $product) == -1) {
    DisplayError("'" . html_quote($product) . "' is not a valid product.");
    exit;
}
    
if (0 == @{$::components{$product}}) {
    my $error = "Sorry; there needs to be at least one component for this " .
                "product in order to create a new bug. ";
    if (UserInGroup($userid, 'editcomponents')) {
        $error .= "<a href=\"editcomponents.cgi\">" . 
                  "Create a new component</a>\n";
    } else {              
        $error .= "Please contact " . Param("maintainer") . ", detailing " .
                  "the product in which you tried to create a new bug.\n";
    }    
    DisplayError($error);   
    exit;
} elsif (1 == @{$::components{$product}}) {
    # Only one component; just pick it.
    $::FORM{'component'} = $::components{$product}->[0];
}

my %default;

$vars->{'component_'} = $::components{$product};
$default{'component_'} = formvalue('component');

$vars->{'assigned_to'} = formvalue('assigned_to');
$vars->{'cc'} = formvalue('cc');
$vars->{'reporter'} = $::COOKIE{'Bugzilla_login'};
$vars->{'user_agent'} = $ENV{'HTTP_USER_AGENT'};
$vars->{'product'} = $product;
$vars->{'bug_file_loc'} = formvalue('bug_file_loc', "http://");
$vars->{'short_desc'} = formvalue('short_desc');
$vars->{'comment'} = formvalue('comment');

$vars->{'priority'} = \@legal_priority;
$default{'priority'} = formvalue('priority', Param('defaultpriority'));

$vars->{'bug_severity'} = \@legal_severity;
$default{'bug_severity'} = formvalue('bug_severity', 'normal');

$vars->{'rep_platform'} = \@legal_platform;
$default{'rep_platform'} = pickplatform();

$vars->{'op_sys'} = \@legal_opsys; 
$default{'op_sys'} = pickos();

# Default version is the last one in the list (hopefully the latest one).
# Eventually maybe each product should have a "current version" parameter.
$vars->{'version'} = $::versions{$product} || [];
$default{'version'} = $vars->{'version'}->[$#{$vars->{'version'}}];

# There must be at least one status in @status.
my @status = "NEW";

if (UserInGroup($userid, "editbugs") || UserInGroup($userid, "canconfirm")) {
    SendSQL("SELECT votestoconfirm FROM products WHERE product = " . SqlQuote($product));
    push(@status, $unconfirmedstate) if (FetchOneColumn());
}

$vars->{'bug_status'} = \@status; 
$default{'bug_status'} = $status[0];

my %productgroups = ();
my @groups;

SendSQL("SELECT product_group_map.group_id FROM products, product_group_map " .
        "WHERE products.product_id = product_group_map.product_id " . 
        "AND products.product = " . SqlQuote($product) . 
        " ORDER BY products.product_id");
while (MoreSQLData()) {
    my ($prodid) = FetchSQLData();
    $productgroups{$prodid} = 1;
}

SendSQL("SELECT groups.group_id, groups.description " .
        "FROM groups, user_group_map " .
        "WHERE groups.group_id = user_group_map.group_id " . 
        "AND user_group_map.user_id = $userid " . 
        "AND isbuggroup != 0 AND isactive = 1 ORDER BY description");

while (MoreSQLData()) {
    my ($group_id, $description) = (FetchSQLData());
        
    my $check;

    # If this is the group for this product, make it checked.
    if (formvalue("maketemplate") eq "Remember values as bookmarkable template") {
        # If this is a bookmarked template, then we only want to set the
        # bit for those bits set in the template.        
        $check = formvalue("bit-$group_id", 0);
    } elsif ($productgroups{$group_id}) {
        $check = 1;
    }

    my $group = 
    {
        'bit' => $group_id, 
        'checked' => $check , 
        'description' => $description 
    };

    push @groups, $group;        
} 

$vars->{'group'} = \@groups;
$vars->{'default'} = \%default;

print "Content-type: text/html\n\n";
$template->process("entry/enter_bug.tmpl", $vars)
  || DisplayError("Template process failed: " . $template->error());          
exit;
