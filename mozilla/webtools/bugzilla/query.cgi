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

# Shut up misguided -w warnings about "used only once":

use vars @::legal_resolution,
  @::legal_product,
  @::legal_bug_status,
  @::legal_priority,
  @::legal_components,
  @::legal_versions,
  @::legal_severity;

if ($::cgi->param('GoAheadAndLogIn') ne "") {
    # We got here from a login page, probably from relogin.cgi
    # We better make sure the password is legit.
    confirm_login();
}

my %default;
my %type;

foreach my $name ("bug_status", "resolution", "assigned_to", "rep_platforms",
                  "priority", "bug_severity", "product", "reporter", "op_sys",
                  "component", "version", "view", "order") {
    $default{$name} = [];
    $type{$name} = 'false';
}

GetVersionTable();

# Hack alert -- this handles the case when no product is selected, and
# we need to build a platform list to handle any possible query
if (scalar(@{$default{'product'}}) == 0) {
    my @platforms;
    foreach my $platformkey (sort(keys(%::legal_platforms))) {
        my $platformarr = $::legal_platforms{"$platformkey"};
        foreach my $platform (@{$platformarr}) {
            if (!grep { /$platform/ } @{$::legal_platforms{'empty'}}) {
                push(@{$::legal_platforms{'empty'}}, $platform);
            }
        }
    }
}

ConnectToDatabase();

my %namelist = {};
my @oldqueries;
my $dbquery = "";
my $id = "";
my $login = $::cgi->cookie('Bugzilla_login');

if ($login ne "") {
    $id = DBNameToIdAndCheck($login);
    # This is as good a place as any to clean up old query cookies
    my @cookies = $::cgi->cookie();
    foreach my $cookie (@cookies) {
        if ($cookie =~ /^QUERY_/ || $cookie =~ /^DEFAULTQUERY$/) {
	    my $oldquery = "";
	    my $value = $::cgi->cookie($cookie);
	    if ($cookie =~ /^DEFAULTQUERY$/) {
	        $oldquery = $::cgi->cookie(-name=>$cookie,
                                           -value=>'', -path=>"/",
                                           -expires=>"now");
	        push(@oldqueries, $oldquery);
	        $oldquery = $::cgi->cookie(-name=>$cookie,
                                           -value=>'', -path=>"/bugzilla/",
                                           -expires=>"now");
	        push(@oldqueries, $oldquery);
	        $cookie = "defaultquery";
	    } else {
	        $oldquery = $::cgi->cookie(-name=>$cookie,
                                           -value=>'', -path=>"/",
                                           -expires=>"now");
	        push(@oldqueries, $oldquery);
	        $oldquery = $::cgi->cookie(-name=>$cookie,
                                           -value=>'', -path=>"/bugzilla/",
                                           -expires=>"now");
	        push(@oldqueries, $oldquery);
	        $cookie = substr($cookie, 6);
	    }
	    $dbquery = "insert into queries values ($id, '" .
	               $cookie . "', '" . $value . "')";
	    SendSQL($dbquery);
        }
    }
    $dbquery = "select query_name, query from queries where userid = $id";
    SendSQL($dbquery);
    while (my @row = FetchSQLData()) {
        my ($query_name, $query) = (@row);
        $namelist{$query_name} = $query;
    }
}
my $nobuglist_cookie = $::cgi->cookie(-name=>'BUGLIST',
                                      -value=>'',
                                      -path=>"/bugzilla/",
                                      -expires=>"now");
push(@oldqueries, $nobuglist_cookie);
print $::cgi->header(-type=>'text/html', 
            -cookie=>\@oldqueries);

$namelist{'defaultquery'} = Param('defaultquery') 
	unless $namelist{'defaultquery'};

my $buffer = "";
# this gets set in buglist.cgi
$::cgi->delete("QUERY");
if (($buffer = $::cgi->query_string()) eq "") {
	$buffer = $namelist{'defaultquery'};
}
# There's got to be a better way to do this
foreach my $item (split(/\&/, $buffer)) {
    my @el = split(/=/, $item);
    my $name = $el[0];
    my $value;
    if ($#el > 0) {
        $value = url_decode($el[1]);
    } else {
        $value = "";
    }
    #print $::cgi->h2("name: $name, value: $value");
    if (defined $default{$name}) {
        if (scalar($default{$name}) > 0) {
	    my @olddefault = @{$default{$name}};
            $default{$name} = [$value, @olddefault];
            $type{$name} = 'true';
        } else {
            $default{$name} = [$value];
        }
    }
    #print "default: @{$default{$name}}" if defined $default{$name};
}
                  
my $who = $::cgi->textfield(-name=>'assigned_to', 
                            -size=>'45', 
                            -override=>'1',
                            -value=>@{$default{'assigned_to'}});
my $reporter = $::cgi->textfield(-name=>'reporter', 
                                 -size=>'45', 
                                 -override=>'1',
                                 -value=>@{$default{'reporter'}});


# Muck the "legal product" list so that the default one is always first (and
# is therefore visibly selected.

# Commented out, until we actually have enough products for this to matter.

# set w [lsearch $legal_product @{$default{"product"}}]
# if {$w >= 0} {
#    set legal_product [concat @{$default{"product"}} [lreplace $legal_product $w $w]]
# }

PutHeader("Bugzilla Query Page", "Query Page");

# Oy, what a hack.
# This strips out '' (or no resolution)
my @local_legal_resolution;
foreach my $res (@::legal_resolution) {
	if ($res ne "") {
		push(@local_legal_resolution, $res);
	}
}

my $bug_status_html = Param("bugstatushtml");
my $help_html = Param("helphtml");

my @type_values = ['substr', 'regex'];
my %type_labels;
my $type_default = 'substr';
$type_labels{'substr'} = 'Substring';
$type_labels{'regex'} = 'Regex';

delete($namelist{'defaultquery'});
my $named_query = '';
my @queries = keys(%namelist);
if (scalar(@queries) > 0) {
    $named_query = $::cgi->TR(
                      $::cgi->td(
                         $::cgi->radio_group(-name=>'cmdtype',
                                     '-values'=>['editnamed'],
                                      -default=>'-',
                                      -linebreak=>'true',
                                      -labels=>{'editnamed' => 
                                                'Load the remembered query:'})
			 . "\n",
                         $::cgi->radio_group(-name=>'cmdtype',
                                     '-values'=>['runnamed'],
                                      -default=>'-',
                                      -linebreak=>'true',
                                      -labels=>{'runnamed' => 
                                                'Run the remembered query:'})
			 . "\n",
                         $::cgi->radio_group(-name=>'cmdtype',
                                     '-values'=>['forgetnamed'],
                                      -default=>'-',
                                      -linebreak=>'true',
                                      -labels=>{'forgetnamed' => 
                                                'Forget the remembered query:'})
			 . "\n"
                      ),
                      $::cgi->td({-valign=>"CENTER"},
                         $::cgi->popup_menu(-name=>'namedcmd',
			                   '-values'=>\@queries)
                      )
                   );
}

my @prod_array = [];
if (scalar(@{$default{'product'}}) > 1) {
    foreach my $prod (@{$default{'product'}}) {
	@prod_array = $::legal_platforms{$prod};
    }
} else { 
	@prod_array = $::legal_platforms{'empty'};
}

print $::cgi->startform(-name=>"queryForm", -action=>"buglist.cgi"),
      $::cgi->table(
          $::cgi->TR(
	      $::cgi->th({-align=>"LEFT"}, 
	          $::cgi->a({-href=>"$bug_status_html"}, "Status"), ":"),
	      $::cgi->th({-align=>"LEFT"}, 
	          $::cgi->a({-href=>"$bug_status_html"}, "Resolution"), ":"),
	      $::cgi->th({-align=>"LEFT"}, 
	          $::cgi->a({-href=>"$bug_status_html#rep_platform"}, 
		      "Platform"), ":"),
	      $::cgi->th({-align=>"LEFT"}, 
	          $::cgi->a({-href=>"$bug_status_html#priority"}, 
		      "Priority"), ":"),
	      $::cgi->th({-align=>"LEFT"}, 
	          $::cgi->a({-href=>"$bug_status_html#severity"}, 
		      "Severity"), ":")
	  ),
          $::cgi->TR(
	      $::cgi->td({-align=>"LEFT", -valign=>"TOP"},
	          $::cgi->scrolling_list(-name=>"bug_status",
		        '-values'=>\@::legal_bug_status,
			-default=>\@{$default{'bug_status'}},
			-size=>"7",
			'-multiple'=>$type{'bug_status'})),
	      $::cgi->p,
	      $::cgi->td({-align=>"LEFT", -valign=>"TOP"},
	          $::cgi->scrolling_list(-name=>"resolution",
		        '-values'=>\@local_legal_resolution,
			-size=>"7",
			-default=>\@{$default{'resolution'}},
			'-multiple'=>$type{'resolution'})),
	      $::cgi->p,
	      $::cgi->td({-align=>"LEFT", -valign=>"TOP"},
	          $::cgi->scrolling_list(-name=>"rep_platform",
		        '-values'=>@prod_array,
			-size=>"7",
			-default=>\@{$default{'rep_platform'}},
			'-multiple'=>"$type{'rep_platform'}")),
	      $::cgi->td({-align=>"LEFT", -valign=>"TOP"},
	          $::cgi->scrolling_list(-name=>"priority",
		        '-values'=>\@::legal_priority,
			-size=>"7",
			-default=>\@{$default{'priority'}},
			'-multiple'=>"$type{'priority'}")),
	      $::cgi->p,
	      $::cgi->td({-align=>"LEFT", -valign=>"TOP"},
	          $::cgi->scrolling_list(-name=>"bug_severity",
		        '-values'=>\@::legal_severity,
			-default=>\@{$default{'bug_severity'}},
			-size=>"7",
			'-multiple'=>"$type{'bug_severity'}")),
	      $::cgi->p
	  ),
      ),
      $::cgi->p,
      $::cgi->table(
          $::cgi->TR(
	      $::cgi->td({-align=>"RIGHT"},
	          $::cgi->a({-href=>"$bug_status_html#assigned_to"}, 
		      $::cgi->b("Assigned To:"))
	      ),
	      $::cgi->td("$who")
	  ),
          $::cgi->TR(
	      $::cgi->td({-align=>"RIGHT"},
	          $::cgi->a({-href=>"$bug_status_html#reporter"},
		      $::cgi->b("Reporter:"))
	      ),
	      $::cgi->td("$reporter"),
	  )
      ),
      $::cgi->p,
      "Changed in the last ", 
      $::cgi->textfield(-name=>"changedin", -size=>"2"),
      " days.",
      $::cgi->p,
      $::cgi->table(
          $::cgi->TR(
	      $::cgi->th({-align=>"LEFT"}, "Program:"),
	      $::cgi->th({-align=>"LEFT"}, "Version:"),
	      $::cgi->th({-align=>"LEFT"}, "Component:"),
	  ),
          $::cgi->TR(
	      $::cgi->td({-align=>"LEFT", -valign=>"TOP"},
	          $::cgi->scrolling_list(-name=>"product",
		        '-values'=>\@::legal_product,
			-size=>"5",
			-default=>\@{$default{'product'}},
			'-multiple'=>$type{'product'})),
	      $::cgi->p,
	      $::cgi->td({-align=>"LEFT", -valign=>"TOP"},
	          $::cgi->scrolling_list(-name=>"version",
		        '-values'=>\@::legal_versions,
			-size=>"5",
			-default=>\@{$default{'version'}},
			'-multiple'=>$type{'version'})),
	      $::cgi->p,
	      $::cgi->td({-align=>"LEFT", -valign=>"TOP"},
	          $::cgi->scrolling_list(-name=>"component",
		        '-values'=>\@::legal_components,
			-size=>"5",
			-default=>\@{$default{'component'}},
			'-multiple'=>$type{'component'})),
	      $::cgi->p
	  )
      ),
      $::cgi->table(
          $::cgi->TR(
	      $::cgi->td({-align=>"RIGHT"}, "Summary:"),
	      $::cgi->td($::cgi->textfield(-name=>"short_desc", -size=>"30")),
	      $::cgi->td(
	          $::cgi->radio_group(-name=>"short_desc_type",
		                      '-values'=>@type_values,
		                      -default=>$type_default,
				      -labels=>\%type_labels))
	  ),
          $::cgi->TR(
	      $::cgi->td({-align=>"RIGHT"}, "Description:"),
	      $::cgi->td($::cgi->textfield(-name=>"long_desc", -size=>"30")),
	      $::cgi->td(
	          $::cgi->radio_group(-name=>"long_desc_type",
		                      '-values'=>@type_values,
		                      -default=>$type_default,
				      -labels=>\%type_labels))
	  )
      ),
      $::cgi->p,
      $::cgi->table({-border=>'0'},
         $::cgi->TR(
            $::cgi->td(
               $::cgi->radio_group(-name=>'cmdtype',
                                  '-values'=>['doit'],
                                   -default=>'doit',
                                   -linebreak=>'true',
                                   -labels=>{'doit' => 'Run this query'}) 
               . "\n"
            ),
            $::cgi->td("&nbsp;")
         ),
         $named_query,
         $::cgi->TR(
            $::cgi->td(
               $::cgi->radio_group(-name=>'cmdtype',
                                  '-values'=>['asdefault'],
                                   -default=>'-',
                                   -linebreak=>'true',
                                   -labels=>{'asdefault' => 
                                      'Remember this as the default query'})
               . "\n"
            ),
            $::cgi->td("&nbsp;")
         ),
         $::cgi->TR(
            $::cgi->td(
               $::cgi->radio_group(-name=>'cmdtype',
                                  '-values'=>['asnamed'],
                                   -default=>'-',
			           -linebreak=>'true',
			           -labels=>{'asnamed' =>
                                          'Remember this query, and name it:'})
               . "\n"
            ),
            $::cgi->td(
	      # FIXME: make this size limited to the size of query_name
               $::cgi->textfield(-name=>'newqueryname')
            )
         )
      ),
      $::cgi->p,
      $::cgi->b("Sort By:"), 
      "&nbsp;&nbsp;",
      $::cgi->popup_menu(-name=>'order',
            '-values'=>['Bug Number', 'Importance', 'Assignee'],
	     -default=>\@{$default{'order'}}),
      $::cgi->submit(-name=>'submit', -value=>'Submit'),
      "&nbsp;&nbsp;",
      $::cgi->reset(-value=>'Reset back to the default query'),
      $::cgi->hidden(-name=>"form_name", -value=>"query"),
      $::cgi->endform,
      $::cgi->br,
      "Give me a ",
      $::cgi->a({-href=>"$help_html"}, "clue"),
      " about how to use this form.",
      $::cgi->p;

if ($::cgi->cookie('Bugzilla_login') ne "") {
    if ($::cgi->cookie('Bugzilla_login') eq Param("maintainer")) {
        print $::cgi->a({-href=>"editparams.cgi"}, 
	          "Edit Bugzilla operating parameters"),
	      $::cgi->br,
              $::cgi->a({-href=>"editowners.cgi"}, 
	          "Edit Bugzilla component owners"),
	      $::cgi->br,
              $::cgi->a({-href=>"editgroups.cgi"}, 
	          "Edit Bugzilla group permissions"),
	      $::cgi->br
    }
    print $::cgi->a({-href=>"relogin.cgi"}, "Log in as someone besides",
              $::cgi->b($::cgi->cookie('Bugzilla_login'))),
	  $::cgi->br;
}

print $::cgi->a({-href=>"changepassword.cgi"}, "Change your password."),
      $::cgi->br,
      $::cgi->a({-href=>"enter_bug.cgi"}, "Enter a new bug."),
      $::cgi->br,
      $::cgi->a({-href=>"reports.cgi"}, "Bug reports."),
      $::cgi->br,
      $::cgi->end_html;
