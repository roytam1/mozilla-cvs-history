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

#use vars '%::versions';
#use vars '@::legal_priority';
#use vars @::legal_severity;
#use vars @::legal_sources;
#use vars @::legal_components;
#use vars @::legal_platforms;
#use vars @::legal_resolution_no_dup;
#use vars @::legal_classes;

require "security.pl";

my $query = "
select
        bug_id,
        product,
        version,
        rep_platform,
        op_sys,
        bug_status,
        resolution,
        priority,
        bug_severity,
        component,
        assigned_to,
        reporter,
        bug_file_loc,
        short_desc,
        class,
        date_format(creation_ts,'Y-m-d')
from bugs
where bug_id = '" . $::bug_id . "'";

my $view_query = "SELECT type_id FROM type where name = 'public' ";
SendSQL($view_query);
my $type = FetchOneColumn();
$view_query = " and view = " . $type;
if (CanIView("view")){
        $view_query = "";
}
$query .= $view_query;

#print $::cgi->pre("$query");

SendSQL($query);

my %bug;
my @row = "";
if (@row = FetchSQLData()) {
    #print $::cgi->h1("@row");
    my $count = 0;
    foreach my $field ("bug_id", "product", "version", "rep_platform",
		       "op_sys", "bug_status", "resolution", "priority",
		       "bug_severity", "component", "assigned_to", "reporter",
		       "bug_file_loc", "short_desc", "class", "creation_ts") {
	$bug{"$field"} = shift @row;
        #print $::cgi->h2($field . " : " .  $bug{"$field"});
	if (!defined $bug{$field}) {
	    $bug{$field} = "";
	}
	$count++;
    }

    print $::cgi->hr;
} else {
    PutHeader("Query Error");
    print "Bug $::bug_id not found\n";
    exit 0;
}

my $source;
SendSQL("SELECT sources.source FROM sources WHERE sources.bug_id = '" . $::bug_id . "'");
if ($source = FetchOneColumn()) {
    $bug{'source'} = $source;
}

$bug{'assigned_to'} = DBID_to_name($bug{'assigned_to'});
$bug{'reporter'} = DBID_to_name($bug{'reporter'});
$bug{'long_desc'} = GetLongDescription($::bug_id);

GetVersionTable();

my $bug_status_html = Param('bugstatushtml');

#
# These should be read from the database ...
#

my $resolution_popup = $::cgi->hidden(-name=>'resolution', 
                                      -value=>"$bug{'resolution'}") . 
                          $bug{'resolution'};
my $version_popup    = $::cgi->hidden(-name=>"version", 
                                      -value=>"$bug{'version'}") . 
                          $bug{'version'};
my $platform_popup   = $::cgi->hidden(-name=>"rep_platform", 
                                      -value=>"$bug{'rep_platform'}") .
                          $bug{'rep_platform'};
my $priority_popup   = $::cgi->hidden(-name=>"priority", 
                                      -value=>"$bug{'priority'}") . 
                          $bug{'priority'};
my $class_row        = "&nbsp;";
my $source_popup     = $::cgi->hidden(-name=>"source", 
                                      -value=>"$bug{'source'}");
my $sev_popup        = $::cgi->hidden(-name=>"bug_severity", 
                                      -value=>"$bug{'bug_severity'}") .
                          $bug{'bug_severity'};
my $component_popup  = $::cgi->hidden(-name=>"component", 
                                      -value=>"$bug{'component'}") .
                          $bug{'component'};
my $cc_element       = $::cgi->td({-colspan=>"5"}, 
                          "&nbsp;" . ShowCcList($::bug_id));
my $URLBlock         = "";
my $SummaryBlock     = $::cgi->td({-align=>"RIGHT"}, "<B>Summary:</B>");
my $StatusBlock      = $::cgi->br;

if (CanIEdit("bug_status", $bug{'reporter'}, $bug{'bug_id'})) {
        my $resolution = lsearch(\@::legal_resolution_no_dup, "");
        if ($resolution >= 0) {
            splice(@::legal_resolution_no_dup, $resolution, 1);
        }

	$resolution_popup = $::cgi->popup_menu(-name=>'resolution',
                                         '-values'=>\@::legal_resolution_no_dup,
                                          -default=>$bug{'resolution'});
}

if (CanIEdit("version", $bug{'reporter'}, $bug{'bug_id'})) {
	$version_popup = $::cgi->popup_menu(-name=>'version',
                               '-values'=>$::versions{$bug{'product'}},
                               -default=>$bug{'version'});
}

if (CanIEdit("rep_platform", $bug{'reporter'}, $bug{'bug_id'})) {
	$platform_popup = $::cgi->popup_menu(-name=>'platform',
                               '-values'=>$::legal_platforms{$bug{'product'}},
                               -default=>$bug{'rep_platform'});
}

if (CanIEdit("priority", $bug{'reporter'}, $bug{'bug_id'})) {
	$priority_popup = $::cgi->popup_menu(-name=>'priority',
                               '-values'=>\@::legal_priority,
                               -default=>$bug{'priority'});
}


my $class_popup = $::cgi->popup_menu(-name=>'class',
                                    '-values'=>\@::legal_classes,
                                     -default=>$bug{'class'});

if ($bug{'bug_status'} ne "NEW") {
    if (CanIEdit("class", $bug{'reporter'}, $bug{'bug_id'})) {
	$class_row = $::cgi->td({-align=>"RIGHT"},
                        $::cgi->a({-href=>"$bug_status_html#class"},
                           $::cgi->b('Class:'))
                     ) .
	             $::cgi->td({-align=>"RIGHT"}, $class_popup);
    } else {
        $class_popup = $::cgi->hidden(-name=>"class", 
                                      -value=>$bug{'class'}) . 
                                      $bug{'class'};
	$class_row = $::cgi->td({-align=>"RIGHT"}, "&nbsp;"),
                     $::cgi->td($class_popup);
    }
}

if (CanIEdit("source", $bug{'reporter'}, $bug{'bug_id'})) {
	$source_popup = $::cgi->popup_menu(-name=>'source',
                               '-values'=>\@::legal_sources,
                               -default=>$bug{'source'});
}

if (CanIEdit("bug_severity", $bug{'reporter'}, $bug{'bug_id'})) {
	$sev_popup = $::cgi->popup_menu(-name=>'bug_severity',
                               '-values'=>\@::legal_severity,
                               -default=>$bug{'bug_severity'});
}

if (CanIEdit("component", $bug{'reporter'}, $bug{'bug_id'})) {
	my @components = @{$::components{$bug{"product"}}};
	$component_popup = $::cgi->popup_menu(-name=>'component',
                               '-values'=>\@components,
                               -default=>$bug{'component'});
}

if (CanIEdit("cc", $bug{'reporter'}, $bug{'bug_id'})) {
	$cc_element = $::cgi->td({-colspan=>"5"}, 
                          $::cgi->textfield(-name=>"cc", 
                                            -size=>"60",
                                            -value=>ShowCcList($::bug_id)));
}

if (CanIEdit("bug_file_loc", $bug{'reporter'}, $bug{'bug_id'})) {
	$URLBlock = $bug{'bug_file_loc'};
	if (defined $URLBlock && $URLBlock ne "none" && 
            $URLBlock ne "NULL" && $URLBlock ne "") {
		$URLBlock = $::cgi->td({-align=>"RIGHT"}, 
                               $::cgi->a({-href=>"$URLBlock"}, 
                                  $::cgi->b("URL:"))) .
                            $::cgi->td({-colspan=>"6"}, 
                               $::cgi->textfield(-name=>"bug_file_loc",
                                               -value=>"$bug{'bug_file_loc'}",
                                               -size=>"60"));
	} else {
		$URLBlock = $::cgi->td({-align=>"RIGHT"}, $::cgi->b("URL:")) .
                            $::cgi->td({-colspan=>"6"}, 
                               $::cgi->textfield(-name=>"bug_file_loc",
                                               -value=>'',
                                               -size=>"60"));
	}
} else {
	if (defined $URLBlock && $URLBlock ne "none" && 
            $URLBlock ne "NULL" && $URLBlock ne "") {
		$URLBlock = $::cgi->td({-align=>"RIGHT"}, $::cgi->b("URL:"),
                               $::cgi->hidden(-name=>"bug_file_loc",
                                              -value=>"$bug{'bug_file_loc'}")).
                            $::cgi->td({-colspan=>"6"}, 
                               $::cgi->a({-href=>"$bug{'bug_file_loc'}"}, 
                                  "$bug{'bug_file_loc'}"));
	}
}

if (CanIEdit("short_desc", $bug{'reporter'}, $bug{'bug_id'})) {
	$SummaryBlock .= $::cgi->td({-colspan=>"6"}, 
                            $::cgi->textfield(-name=>"short_desc", 
                                              -value=>"$bug{'short_desc'}",
                                              -size=>"60"));
} else {
	$SummaryBlock .= $::cgi->td({-colspan=>"6"}, "&nbsp;$bug{'short_desc'}",
                         $::cgi->hidden(-name=>"short_desc", 
                                        -value=>"$bug{'short_desc'}"));
}

my $AdditionalCommentsBlock = $::cgi->br .
                              $::cgi->b("Additional Comments:") .
                              $::cgi->br .
                              $::cgi->textarea(-wrap=>"HARD", 
                                               -name=>"comment", 
                                               -rows=>"5", 
                                               -cols=>"70",
					       -value=>'',
					       -override=>'1') .
                              $::cgi->br;
my $maildir;
if ( -d "data/maildir/$bug{'bug_id'}" ) {
    $maildir = $::cgi->td({-colspan=>"4"}, "&nbsp;$bug{'assigned_to'}") .
               $::cgi->td(
	          $::cgi->a({-href=>"data/maildir/$bug{'bug_id'}"}, 
	             "Bug #${bug{'bug_id'}} email"));
} else {
    $maildir = $::cgi->td({-colspan=>"5"}, "&nbsp;$bug{'assigned_to'}");
}

print $::cgi->start_html(-title=>"Bug $::bug_id -- $bug{'short_desc'}"),
      $::cgi->startform(-name=>"changeform",
                      -method=>"POST",
                      -action=>"process_bug.cgi"),
      $::cgi->hidden(-name=>"id", -value=>"$::bug_id"),
      $::cgi->hidden(-name=>"was_assigned_to", -value=>"$bug{'assigned_to'}"),
      $::cgi->hidden(-name=>"product", -value=>"$bug{'product'}"),
      $::cgi->table({-cellspacing=>"2", -cellpadding=>"2", -border=>"0"},
        $::cgi->TR(
           $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Bug#:")),
           $::cgi->td("&nbsp;$bug{'bug_id'}"),
           $::cgi->td({-align=>"RIGHT"}, 
              $::cgi->a({-href=>"$bug_status_html#rep_platform"}, 
                 $::cgi->b("Architecture:"))
           ),
           $::cgi->td("&nbsp;$platform_popup"),
           $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Version:")),
           $::cgi->td("&nbsp;$version_popup")
        ),
        $::cgi->TR(
           $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Product:")),
           $::cgi->td({-colspan=>"2"}, "&nbsp;$bug{'product'}"),
           $::cgi->td({-align=>"LEFT"}, $::cgi->b("Reporter:")),
           $::cgi->td({-colspan=>"2"}, "&nbsp;$bug{'reporter'}")
        ),
        $::cgi->TR(
           $::cgi->td({-align=>"RIGHT"}, 
	      $::cgi->a({-href=>"$bug_status_html"}, 
	         $::cgi->b("Status:"))
	   ),
	   $::cgi->td("&nbsp;$bug{'bug_status'}"),
	   $::cgi->td({-align=>"RIGHT"}, 
	      $::cgi->a({-href=>"$bug_status_html#priority"}, 
	         $::cgi->b("Priority:"))
	   ),
	   $::cgi->td("&nbsp;$priority_popup"),
           $class_row
        ),
        $::cgi->TR(
           $::cgi->td({-align=>"RIGHT"}, 
	      $::cgi->a({-href=>"$bug_status_html"},
	         $::cgi->b("Resolution:"))
	   ),
           $::cgi->td("&nbsp;$bug{'resolution'}"),
           $::cgi->td({-align=>"RIGHT"},
	      $::cgi->a({-href=>"$bug_status_html#severity"},
	         $::cgi->b("Severity:"))
	   ),
           $::cgi->td("&nbsp;$sev_popup"),
           $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Component:")),
           $::cgi->td("&nbsp;$component_popup")
        ),
        $::cgi->TR(
           $::cgi->td({-align=>"RIGHT"}, 
	      $::cgi->a({-href=>"$bug_status_html#assigned_to"},
	         $::cgi->b("Assigned&nbsp;To:"))),
           $maildir 
        ),
        $::cgi->TR(
           $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Cc:&nbsp")),
	   $cc_element
        ),
        $::cgi->TR($URLBlock),
        $::cgi->TR($SummaryBlock)
      ),
      $AdditionalCommentsBlock;

my $status = $bug{'bug_status'};
my @trans;
my $transition;

SendSQL("select trans from states where state = '$status'");

while (@row = FetchSQLData()) {
    push(@trans, @row);
}

my @radio_values;
my %radio_labels;
my $default = "none";
my $tablerow;
my @tabledata;

if(CanIEdit("bug_status", $bug{'reporter'}, $bug{'bug_id'})) {
    foreach $transition (@trans) {
        if ($transition eq $status) {
	    push(@radio_values, "none");
	    $radio_labels{"none"} = "Leave as " . 
                                    $::cgi->b($status) . $::cgi->br;
	}
	if ($transition eq "VERIFIED" && $status ne "VERIFIED") {
	    push(@radio_values, "verify");
	    $radio_labels{"verify"} = $::cgi->b('VERIFY') . 
                                      " bug as $class_popup" . $::cgi->br;
	}
	if ($transition eq "ASSIGNED") {
            if (CanIEdit("assigned_to", $bug{'reporter'}, $bug{'bug_id'})) {
		my $assign_element = $::cgi->textfield(-name=>"assigned_to",
		                               -size=>"32",
					       -value=>"$bug{'assigned_to'}");
	        push(@radio_values, "reassign");
	        $radio_labels{"reassign"} = 
		    "Assign bug to $assign_element<BR>";
	        push(@radio_values, "reassignbycomponent");
	        $radio_labels{"reassignbycomponent"} = 
		    "Assign bug to owner of selected component" . $::cgi->br;
            }
	}
	if ($transition eq "RESOLVED") {
            if (CanIEdit("resolution", $bug{'reporter'}, $bug{'bug_id'})) {
               if ($bug{'resolution'} ne "") {
	           push(@radio_values, "clearresolution");
		   $radio_labels{"clearresolution"} =
		       "Clear the resolution (remove the current " .
		       "resolution of " . $::cgi->b($bug{'resolution'}) . 
                       ")" . $::cgi->br;
               }
	       push (@radio_values, "resolve");
	       $radio_labels{"resolve"} = "<B>RESOLVE</B> bug, changing " .
	           $::cgi->a({-href=>"$bug_status_html"}, "resolution") .
		   " to " .  $resolution_popup . $::cgi->br;
	       push(@radio_values, "duplicate");
	       $radio_labels{"duplicate"} = 
	           "Resolve bug, mark it as duplicate of bug #" .
		   $::cgi->textfield(-name=>"dup_id", -size=>"6") . $::cgi->br;
            }
	}
	if ($transition eq "REOPENED" and $status ne "REOPENED") {
	    push(@radio_values, "reopen");
	    $radio_labels{"reopen"} = "Reopen bug" . $::cgi->br;
	}
	if ($transition eq "CLOSED" and $status ne "CLOSED") {
	    push(@radio_values, "close");
	    $radio_labels{"close"} = "Mark bug as " . 
                                     $::cgi->b('CLOSED') . $::cgi->br;
	}
    }
    if ($status ne "NEW" && 
        CanIEdit("source", $bug{'reporter'}, $bug{'bug_id'})) {
	push(@radio_values, "newsource");
	$radio_labels{"newsource"} = 
	    "Another report of this bug came from $source_popup" . $::cgi->br;
    }
    foreach my $radio (@radio_values) {
        $tablerow = $::cgi->TR(
                       $::cgi->td({-valign=>"CENTER", -align=>"CENTER"},
                          $::cgi->radio_group(-name=>"knob", 
                                             '-values'=>["$radio"],
                                              -default=>$default,
			                      -linebreak=>"true",
                                              -labels=>{"$radio" => " "}),
                          "&nbsp;"
                         ),
                         $::cgi->td({-valign=>"CENTER", -align=>"LEFT"},
                            $radio_labels{$radio}
                         )
                       );
        push(@tabledata, $tablerow);
    }
    print $::cgi->table({-border=>"0", 
                         -cellpadding=>"0", 
                         -cellspacing=>"0"}, 
                         @tabledata
                        );
} else {
    print $::cgi->hidden(-name=>"knob", -value=>"none");
}

print $::cgi->submit(-name=>"submit", -value=>"Commit"),
      "&nbsp;",
      $::cgi->reset,
      $::cgi->hidden(-name=>"form_name", -value=>"process_bug"),
      $::cgi->br,
      $::cgi->font({-size=>"+1"}, 
         $::cgi->a({-href=>"show_activity.cgi?id=$::bug_id"}, 
	     $::cgi->b("View Bug Activity")),
	 $::cgi->a({-href=>"long_list.cgi?buglist=$::bug_id"}, 
	     $::cgi->b("Format For Printing"))
      ),
      $::cgi->br,
      $::cgi->endform,
      $::cgi->table(
         $::cgi->TR(
	    $::cgi->td({-align=>"LEFT"}, $::cgi->b('Description:')),
	    $::cgi->td({-width=>"100%"}, "&nbsp;"),
	    $::cgi->td({-align=>"RIGHT"}, "Opened:&nbsp;$bug{'creation_ts'}")
	 )
      ),
      $::cgi->hr,
      $::cgi->pre("$bug{'long_desc'}"),
      $::cgi->hr;

# To add back option of editing the long description, insert after the above
# long_list.cgi line:
#  %::cgi->a({-href=>"edit_desc.cgi?id=$::bug_id"}, "Edit Long Description")

navigation_header();

print $::cgi->end_html;
