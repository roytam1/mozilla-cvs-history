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
use CGI;

$::cgi = new CGI;

require "CGI.pl";
require "security.pl";

# Shut up misguided -w warnings about "used only once":
use vars 
    @::legal_severity,
    @::legal_sources,
    @::legal_opsys,
    @::legal_platforms,
    @::legal_priority;

my $product = $::cgi->param('product');
confirm_login();

if ($::cgi->param('product') eq "") {
    GetVersionTable();
    my @prodlist = keys %::versions;
    if ($#prodlist != 0) {
        PutHeader("Enter Bug");
        
	my @tabledata;
	my $tablerow;
        foreach my $p (sort (@prodlist)) {
            if (defined $::proddesc{$p}) {
	        $tablerow = $::cgi->TR(
                            $::cgi->td({-align=>"RIGHT", -valign=>"TOP"},
                               $::cgi->startform(-name=>"$p",
                                                 -action=>"enter_bug.cgi"),
                               $::cgi->hidden(-name=>'product', 
                                              -value=>"$p",
                                              -override=>'1'),
                               $::cgi->submit(-value=>"$p"),
                               ":"
                            ),
                            $::cgi->td({-valign=>"CENTER"}, 
                               $::proddesc{$p}),
                               $::cgi->endform
                            );
            } else {
	        $tablerow = $::cgi->TR(
                            $::cgi->td({-align=>"RIGHT", -valign=>"TOP"},
                               $::cgi->startform(-name=>"$p",
                                                 -action=>"enter_bug.cgi"),
                               $::cgi->hidden(-name=>'product', 
                                              -value=>"$p",
                                              -override=>'1'),
                               $::cgi->submit(-value=>"$p"),
                               ":"
                            ),
                            $::cgi->td({-valign=>"TOP"}, 
                               "&nbsp;"),
                               $::cgi->endform
                            );
            }
	    push(@tabledata, $tablerow);
	    $tablerow = '';
        }
	print 
          $::cgi->h2("First, you must pick a product on which to enter a bug"),
	  $::cgi->table(@tabledata),
	  $::cgi->end_html;
        exit;
    }
    $::cgi->param(-name=>'product', -value=>"$prodlist[0]", -override=>"1");
}

sub formvalue {
    my ($name, $default) = (@_);
    if ($::cgi->param($name) ne "") {
        return $::cgi->param($name);
    }
    if (defined $default) {
        return $default;
    }
    return "";
}

sub pickplatform {
    my $value = $::cgi->param("rep_platform");
    if ($value ne "") {
        return $value;
    }
    my $browser = $::cgi->user_agent();
    for ($browser) {
        /Mozilla.*\(X11/ && do {return "X-Windows";};
        /Mozilla.*\(Windows/ && do {return "PC";};
        /Mozilla.*\(Macintosh/ && do {return "Macintosh";};
        /Mozilla.*\(Win/ && do {return "PC";};
        # default
        return "PC";
    }
}

sub pickversion {
    my $version = $::cgi->param('version');
    my $browser = $::cgi->user_agent();
    if ($version eq "") {
        if ($browser =~ m@Mozilla[ /]([^ ]*)@) {
            $version = $1;
        }
    }
    
    if (lsearch($::versions{$product}, $version) >= 0) {
        return $version;
    } else {
        if ($::cgi->cookie("VERSION-$product") ne "") {
            if (lsearch($::versions{$product},
                        $::cgi->cookie("VERSION-$product")) >= 0) {
                return $::cgi->cookie("VERSION-$product");
            }
        }
    }
    return $::versions{$product}->[0];
}

sub pickarch {
    my $arch = $::cgi->param('arch');
    if ($arch eq "") {
        my $browser = $::cgi->user_agent();
        if ($browser =~ m#Mozilla.*\(.*;.*;.* (.*)\)#) {
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
    my $result = $::cgi->param('component');
    if ($result ne "" && lsearch($::components{$product}, $result) < 0) {
        $result = "";
    }
    return $result;
}


sub pickos {
    if ($::cgi->param('op_sys') ne "") {
        return $::cgi->param('op_sys');
    }
    my $browser = $::cgi->user_agent();
    for ($browser) {
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

my $assign_element = $::cgi->textfield(-name=>'assigned_to',
                                       -value=>$::cgi->param('assigned_to'));
my $cc_element = $::cgi->textfield(-name=>'cc', 
                                   -size=>"45", 
                                   -value=>$::cgi->param('cc'));
my $priority_popup = $::cgi->popup_menu(-name=>'priority',
                                       '-values'=>\@::legal_priority,
                                        -default=>'normal');
my $sev_popup = $::cgi->popup_menu(-name=>'bug_severity',
                                  '-values'=>\@::legal_severity,
                                   -default=>'normal');
my $opsys_popup = $::cgi->popup_menu(-name=>'op_sys', 
                                    '-values'=>\@::legal_opsys,
                                     -default=>pickos());

die "Product table empty" if ($::components{"$product"} eq "");

my @component_count = @{$::components{"$product"}};
my $component_popup = "";

if ($#component_count > 1) {
    $component_popup = $::cgi->scrolling_list(-name=>'component', 
                                          -size=>"5",
                                         '-values'=>$::components{$product},
                                          -default=>$::cgi->param('component'));
} else {
    $component_popup = $::cgi->scrolling_list(-name=>'component', 
                                          -size=>"5",
                                         '-values'=>$::components{$product},
                                          -default=>$component_count[0]);
}

my $platforms_popup = $::cgi->popup_menu(-name=>'rep_platform', 
                                '-values'=>$::legal_platforms{$product},
                                 -default=>pickarch());

PutHeader ("Enter Bug");

my $release_element = $::cgi->textfield(-name=>'release', 
                                        -size=>"5", 
                                        -value=>$::cgi->param('release'));

my $source_value = "bug reporting address and mailing lists";
my $source_cell;

if(CanIView("source")) {
    $source_cell = 
        $::cgi->TR(
           $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Source:")),
           $::cgi->td({-colspan=>"5"}, 
              $::cgi->popup_menu(-name=>'source',
                                '-values'=>\@::legal_sources,
                                 -default=>'support mail')
           )
        );
} else {
    $source_cell = 
        $::cgi->TR(
           $::cgi->td({-colspan=>"6"}, 
              $::cgi->hidden(-name=>"source", -value=>$source_value))
        );
}

my $view_cell;
if(CanIView("view")) {
    $view_cell = $::cgi->td({-align=>"RIGHT"}, $::cgi->b("View:")),
                 $::cgi->td($::cgi->popup_menu(-name=>'view',
                                              '-values'=>['Public', 'Private'],
                                               -default=>'Public'));
} else {
    $view_cell = $::cgi->td({-colspan=>"2"}, 
                    $::cgi->hidden(-name=>'view', -value=>"Public"));
}

my $assigned_cell;
my $bug_status_html = Param("bugstatushtml");
if(CanIView("assigned_to")) {
    $assigned_cell = $::cgi->TR(
                        $::cgi->td({-align=>"RIGHT"},
                           $::cgi->a({-href=>"$bug_status_html#assigned_to"},
                              $::cgi->b("Assigned To:"))),
                        $::cgi->td({-colspan=>"5"}, $assign_element, 
                           "(Leave blank to assign to default owner)")
                     );
}

print $::cgi->start_form(-name=>'enterForm', -action=>'post_bug.cgi'),
      $::cgi->hidden(-name=>'bug_status', -value=>'NEW'),
      $::cgi->hidden(-name=>'reporter', 
                     -value=>$::cgi->cookie('Bugzilla_login')),
      $::cgi->hidden(-name=>'product', -value=>$product),
      $::cgi->table({-cellspacing=>"2", -cellpadding=>"0", -border=>"0"},
         $::cgi->TR(
            $::cgi->td({-align=>"RIGHT", -valign=>"TOP"}, 
	       $::cgi->b("Product:")),
            $::cgi->td({-valign=>"TOP", -colspan=>"5"}, 
	       $product)
         ),
         $::cgi->TR(
            $::cgi->td({-align=>"RIGHT", -valign=>"TOP"}, 
	       $::cgi->b("Version:")),
	    $::cgi->td(Version_element(pickversion(), $product)),
            $::cgi->td({-align=>"RIGHT", -valign=>"TOP"}, 
	       $::cgi->b("Component:")),
	    $::cgi->td({-colspan=>"3"}, $component_popup)
         ),
         $::cgi->TR(
            $::cgi->td({-align=>"RIGHT"}, 
	       $::cgi->a({-href=>"$bug_status_html#priority"},
	          $::cgi->b("Priority:"))),
            $::cgi->td($priority_popup),
            $::cgi->td({-align=>"RIGHT"}, 
	       $::cgi->a({-href=>"$bug_status_html#severity"},
	          $::cgi->b("Severity:"))),
            $::cgi->td($sev_popup)
         ),
         $::cgi->TR(
            $::cgi->td({-align=>"RIGHT"}, 
	       $::cgi->a({-href=>"$bug_status_html#rep_platform"},
	          $::cgi->b("Architecture:"))),
            $::cgi->td($platforms_popup),
            $source_cell,
            $view_cell,
            $assigned_cell
         ),
         $::cgi->TR(
            $::cgi->td({-align=>"RIGHT"}, 
	       $::cgi->b("Cc:")),
            $::cgi->td({-colspan=>"5"}, $cc_element),
         ),
         $::cgi->TR(
            $::cgi->td({-align=>"RIGHT"}, 
	       $::cgi->b("URL:")),
            $::cgi->td({-colspan=>"5"}, 
	       $::cgi->textfield(-name=>'bug_file_loc', 
	                         -size=>'60', 
				 -value=>$::cgi->param('bug_file_loc')))
         ),
         $::cgi->TR(
            $::cgi->td({-align=>"RIGHT"}, 
	       $::cgi->b("Summary:")),
            $::cgi->td({-colspan=>"5"}, 
	       $::cgi->textfield(-name=>'short_desc', 
	                         -size=>'60', 
				 -value=>$::cgi->param('short_desc')))
         ),
         $::cgi->TR(
            $::cgi->td({-align=>"RIGHT"}, 
	       $::cgi->b("Description:")),
            $::cgi->td({-colspan=>"5"}, 
	       $::cgi->textarea(-wrap=>"HARD", 
	                        -name=>'comment',
				-rows=>'10',
				-cols=>'60',
				-default=>$::cgi->param('comment')))
         ),
         $::cgi->TR(
	    $::cgi->td("&nbsp;"),
            $::cgi->td({-colspan=>"5"}, 
	       $::cgi->submit(-name=>'submit', -value=>"    Commit    "),
	       "&nbsp;&nbsp;&nbsp;&nbsp;",
	       $::cgi->reset(-name=>'reset'),
	       "&nbsp;&nbsp;&nbsp;&nbsp;",
	       $::cgi->submit(-name=>'maketemplate', 
	          -value=>"Remember values as bookmarkable template"))
         ),
      ),
      $::cgi->hidden(-name=>"form_name", -value=>"enter_bug"),
      $::cgi->end_form,
      $::cgi->end_html;
