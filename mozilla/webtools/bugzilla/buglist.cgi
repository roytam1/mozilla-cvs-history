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

my $serverpush = 0;

# Put browsers that can do serverpush here --  MSIE and Lynx cannot, 
# since Lynx will try to download this mime type we default to off
if ($::cgi->user_agent("Mozilla")) { $serverpush = 1; }

if ($serverpush) {
    #print $::cgi->multipart_init(-boundary=>"thisrandomstring");
    #print $::cgi->multipart_start(-type=>'text/html');
}

# Shut up misguided -w warnings about "used only once":

use vars @::legal_platforms,
    @::versions,
    @::legal_versions,
    @::legal_product,
    @::components,
    @::legal_severity,
    @::legal_priority,
    @::default_column_list,
    @::legal_resolution_no_dup;

ConnectToDatabase();

if ($::cgi->param('cmdtype') eq "") {
    # This can happen if there's an old bookmark to a query...
    $::cgi->param(-name=>'cmdtype', -value=>'doit', -override=>"1");
}

my $namedcmd = $::cgi->param('namedcmd');
my $newqueryname = $::cgi->param('newqueryname');
my $cookie = "";
my $query = "";
my $id = DBNameToIdAndCheck($::cgi->cookie('Bugzilla_login'));
my $needheader = 0;

if (!$id) { 
    confirm_login(); 
} else { 
    $needheader = 1;
}

my $command = $::cgi->param('cmdtype');
CMD: for ($command) {
    /^runnamed$/ && do {
        my $query = "select query from queries where query_name = '" . 
                    $namedcmd . "' and userid = $id";
        SendSQL($query);
        my $buffer = FetchOneColumn();
        my $newcgi = new CGI($buffer);
        $::cgi = $newcgi;
        last CMD;
    };
    /^editnamed$/ && do {
        my $query = "select query from queries where query_name = '" . 
                    $namedcmd . "' and userid = $id";
        SendSQL($query);
        my $buffer = FetchOneColumn();
        my $url = "query.cgi?" . $buffer;
        print $::cgi->redirect(-uri=>"$url");
        exit;
    };
    /^forgetnamed$/ && do {
        $query = "delete from queries where query_name = '" .
            $namedcmd . "' and userid = $id";
        SendSQL($query);
        print $::cgi->header(-type=>'text/html') if $needheader; 
        PutHeader("Forget what?");
        print "OK, the <B>$namedcmd</B> query is gone.<P>";
        print $::cgi->a({-href=>"query.cgi"}, "Go back to the query page.");
        print $::cgi->end_html;
        exit;
    };
    /^asnamed$/ && do {
        print $::cgi->header(-type=>'text/html') if $needheader; 
        if ($::cgi->param('newqueryname') =~ /^[a-zA-Z0-9_ ]+$/) {
           # make sure these don't get saved into the database
	   $::cgi->delete('Bugzilla_login');
	   $::cgi->delete('Bugzilla_password');
	   my $cmd = $::cgi->query_string();
	   $query = "select query from queries where userid = $id " .
                    "and query_name = '" . $newqueryname . "'";
	   SendSQL($query);
	   my $temp = FetchOneColumn();
           if ($temp ne "") {
	       $query = "update queries set query = '" .  $cmd . 
                        "' where userid = $id and query_name = '" .
                        $newqueryname . "'";
           } else {
	       $query = "insert into queries values ($id, '" . 
                        $newqueryname . "', '" .  $cmd . "')";
           }
	   SendSQL($query);
	   PutHeader("OK, done.");
	   print "OK, you now have a new query named <B>$newqueryname</B>.\n";
           print "<P>\n";
	   print $::cgi->a({-href=>"query.cgi"}, "Go back to the query page.");
           print $::cgi->end_html;
        } else {
	   PutHeader("Picky, picky.");
           print "Query names can only have letters, digits, spaces, or ";
	   print "underbars.  You entered \"<B>$newqueryname</B>\", which ";
	   print "doesn't cut it.\n<P>\n";
	   print "Click the <B>Back</B> button and type in a valid name ";
	   print "for this query.";
           print $::cgi->end_html;
        }
        exit;
    };
    /^asdefault$/ && do {
       print $::cgi->header(-type=>'text/html') if $needheader; 
       # make sure these don't get saved into the database
       $::cgi->delete('Bugzilla_login');
       $::cgi->delete('Bugzilla_password');
       my $cmd = $::cgi->query_string();
       $query = "select query from queries where userid = $id " .
                "and query_name = 'defaultquery'";
       SendSQL($query);
       my $temp = FetchOneColumn();
       if ($temp ne "") {
	   $query = "update queries set query = '" .  $cmd . 
                    "' where userid = $id and " . 
                    "query_name = 'defaultquery'";
       } else {
	   $query = "insert into queries values ($id, " .
                    "'defaultquery', '" .  $cmd . "')";
       }
       SendSQL($query);
       PutHeader("OK, default is set.");
       print "OK, you now have a new default query.<P>";
       print $::cgi->a({-href=>"query.cgi"}, 
              "Go back to the query page, using the new default.");
       exit;
    };
}

sub DefCol {
    my ($name, $k, $t, $s, $q) = (@_);
    
    $::key{$name} = $k;
    $::title{$name} = $t;
    if (defined $s && $s ne "") {
        $::sortkey{$name} = $s;
    }
    if (!defined $q || $q eq "") {
        $q = 0;
    }
    $::needquote{$name} = $q;
}

DefCol("opendate", "date_format(bugs.creation_ts,'Y-m-d')", "Opened",
       "bugs.creation_ts");
DefCol("changeddate", "date_format(bugs.delta_ts,'Y-m-d')", "Changed",
       "bugs.delta_ts");
DefCol("severity", "substring(bugs.bug_severity, 1, 3)", "Sev",
       "bugs.bug_severity");
DefCol("priority", "substring(bugs.priority, 1, 3)", "Pri", "bugs.priority");
DefCol("platform", "substring(bugs.rep_platform, 1, 3)", "Plt",
       "bugs.rep_platform");
DefCol("owner", "assign.login_name", "Owner", "assign.login_name");
DefCol("reporter", "report.login_name", "Reporter", "report.login_name");
DefCol("status", "substring(bugs.bug_status,1,4)", "State", "bugs.bug_status");
DefCol("resolution", "substring(bugs.resolution,1,4)", "Result",
       "bugs.resolution");
DefCol("summary", "substring(bugs.short_desc, 1, 60)", "Summary", "", 1);
DefCol("summaryfull", "bugs.short_desc", "Summary", "", 1);
DefCol("component", "substring(bugs.component, 1, 8)", "Comp",
       "bugs.component");
DefCol("product", "substring(bugs.product, 1, 8)", "Product", "bugs.product");
DefCol("version", "substring(bugs.version, 1, 5)", "Vers", "bugs.version");
DefCol("os", "substring(bugs.op_sys, 1, 4)", "OS", "bugs.op_sys");

my @collist;
if ($::cgi->cookie('COLUMNLIST') ne "") {
    @collist = split(/ /, $::cgi->cookie('COLUMNLIST'));
} else {
    @collist = @::default_column_list;
}

my $dotweak = $::cgi->param('tweak');

if ($dotweak) {
    confirm_login();
}

$query = "select bugs.bug_id";

foreach my $c (@collist) {
    if (exists $::needquote{$c}) {
        $query .= ",
\t$::key{$c}";
    }
}

if ($dotweak) {
    $query .= ",
bugs.product,
bugs.bug_status";
}

$query .= "
from   bugs,
       profiles assign,
       profiles report,
       versions projector
where  bugs.assigned_to = assign.userid 
and    bugs.reporter = report.userid
and    bugs.product = projector.program
and    bugs.version = projector.value
";

my $view_query = "SELECT type_id FROM type WHERE name = 'public'";
SendSQL($view_query);
my $type = FetchOneColumn();
$view_query = "and bugs.view = '" . $type . "' ";
if (CanIView("view")){
	$view_query = "";
}
$query .= $view_query;

# FIXME: should this be protected by a config option?
if ($::cgi->param('sql') ne "") {
  $query .= "and (\n" . $::cgi->param('sql') . "\n)"
} else {
  my @legal_fields = ("bug_id", "product", "version", "rep_platform", "op_sys",
                      "bug_status", "resolution", "priority", "bug_severity",
                      "assigned_to", "reporter", "bug_file_loc", "component");
  foreach my $field ($::cgi->param) {
      my $or = "";
      if (lsearch(\@legal_fields, $field) != -1 && $::cgi->param($field) ne "") {
          $query .= "\tand (\n";
          if ($field eq "assigned_to" || $field eq "reporter") {
              foreach my $p (split(/,/, $::cgi->param($field))) {
                  my $whoid = DBNameToIdAndCheck($p);
                  $query .= "\t\t${or}bugs.$field = $whoid\n";
                  $or = "or ";
              }
          } else {
              my @arr = $::cgi->param($field);
	      foreach my $v (@arr) {
                  if ($v eq "(empty)") {
                      $query .= "\t\t${or}bugs.$field is null\n";
                  } else {
                      if ($v eq "---") {
                          $query .= "\t\t${or}bugs.$field = ''\n";
                      } else {
                          $query .= "\t\t${or}bugs.$field = " . 
                                    SqlQuote($v) . "\n";
                      }
                  }
                  $or = "or ";
              }
          }
          $query .= "\t)\n";
      }
  }
}

if ($::cgi->param('changedin') ne "") {
    my $c = trim($::cgi->param('changedin'));
    if ($c ne "") {
        if ($c !~ /^[0-9]*$/) {
            PutHeader("Try Again");
            print "The 'changed in last ___ days' field must be a simple ";
	    print "number.  You entered \"$c\", which doesn't cut it.<P>";
	    print "Click the <B>Back</B> button and try again.";
	    print $::cgi->end_html;
            exit;
        }
        $query .= "and to_days(now()) - to_days(bugs.delta_ts) <= $c ";
    }
}

foreach my $f ("short_desc", "long_desc") {
    if ($::cgi->param($f) ne "") {
        my $s = SqlQuote(trim($::cgi->param($f)));
        if ($s ne "") {
            if ($::cgi->param($f . "_type") eq "regexp") {
                $query .= "and $f regexp $s ";
            } else {
                $query .= "and instr($f, $s) ";
            }
        }
    }
}


if (($::cgi->param('order') ne "") && ($::cgi->param('order') ne "")) {
    $query .= "order by ";
    ORDER: for ($::cgi->param('order')) {
        /\./ && do {
            # This (hopefully) already has fieldnames in it, so we're done.
            last ORDER;
        };
        /Number/ && do {
            $::cgi->param(-name=>'order', 
                          -value=>"bugs.bug_id", 
                          -override=>"1");
            last ORDER;
        };
        /Import/ && do {
            $::cgi->param(-name=>'order', 
                          -value=>"bugs.priority", 
                          -override=>"1");
            last ORDER;
        };
        /Assign/ && do {
            $::cgi->param(-name=>'order', 
                          -value=>"assign.login_name, bugs.bug_status, priority, bugs.bug_id", 
                          -override=>"1");
            last ORDER;
        };
        # DEFAULT
        $::cgi->param(-name=>'order',
                      -value=>"bugs.bug_status, priorities.rank, assign.login_name, bugs.bug_id",
                      -override=>"1");
    }
    $query .= $::cgi->param('order');
}

if ($serverpush) {
    #print "Please stand by ... <p>\n";
    #if ($::cgi->param('debug') ne "") {
    #    print "<pre>$query</pre>\n";
    #}
}

SendSQL($query);

my $count = 0;
$::bugl = "";
sub pnl {
    my ($str) = (@_);
    $::bugl  .= $str;
}

my $fields = $::cgi->query_string();
$fields =~ s/[&?]order=[^&]*//g;
$fields =~ s/[&?]cmdtype=[^&]*//g;

my $oldorder;

if ($::cgi->param('order') ne "") {
    $oldorder = "," .$::cgi->param('order');
} else {
    $oldorder = "";
}

if ($dotweak) {
    pnl $::cgi->startform(-method=>'POST', 
                          -action=>'process_bug.cgi',
                          -name=>'changeform');
}

my @tabledata;

push(@tabledata, $::cgi->TR({align=>"LEFT"}), 
                 $::cgi->th(
		     $::cgi->a({href=>"buglist.cgi?$fields&order=bugs.bug_id"},
		     "ID")
		 )
    );

my $tablestart = 
    $::cgi->table({cellspacing=>"0", cellpadding=>"2"}) .
    $::cgi->TR({align=>"LEFT"}) .
    $::cgi->th .
    $::cgi->a({href=>"buglist.cgi?$fields&order=bugs.bug_id"}, "ID");

foreach my $c (@collist) {
    if (exists $::needquote{$c}) {
        if ($::needquote{$c}) {
            $tablestart .= $::cgi->th({width=>"100%", valign=>"LEFT"});
        } else {
            $tablestart .= $::cgi->th({valign=>"LEFT"});
        }
        if (defined $::sortkey{$c}) {
            $tablestart .= $::cgi->a(
                  {href=>"buglist.cgi?$fields&order=$::sortkey{$c}$oldorder"}, 
                  "$::title{$c}"
            );
        } else {
            $tablestart .= $::title{$c};
        }
    }
}

my @row;
my %seen;
my @bugarray;
my %prodhash;
my %statushash;

while (@row = FetchSQLData()) {
    my $bug_id = shift @row;
    if (!defined $seen{$bug_id}) {
        $seen{$bug_id} = 1;
        $count++;
        if ($count % 200 == 0) {
            # Too big tables take too much browser memory...
            pnl "</TABLE>$tablestart";
        }
        push @bugarray, $bug_id;
        pnl "<TR VALIGN=\"TOP\" ALIGN=\"LEFT\"><TD>";
        if ($dotweak) {
            pnl "<input type=\"checkbox\" name=\"id_$bug_id\">";
        }
        pnl "<A HREF=\"show_bug.cgi?id=$bug_id\">";
        pnl "$bug_id</A> ";
        foreach my $c (@collist) {
            my $value = shift @row;
            my $nowrap = "";

            if (exists $::needquote{$c} && $::needquote{$c}) {
                $value = html_quote($value);
            } else {
                $value = "<nobr>$value</nobr>";
            }
            pnl "<td $nowrap>$value";
        }
        if ($dotweak) {
            my $value = shift @row;
            $prodhash{$value} = 1;
            $value = shift @row;
            $statushash{$value} = 1;
        }
        pnl "\n";
    }
}


my $buglist = join(":", @bugarray);


if ($serverpush) {
    #print $::cgi->multipart_end;
    #print $::cgi->multipart_start(-type=>'text/plain');
}

my $toolong = 0;

if (length($buglist) < 4000) {
    # FIXME: make path configurable
    $cookie = $::cgi->cookie(-name=>'BUGLIST',
                       -path=>"/bugzilla/",
                       -value=>$buglist);
} else {
    # FIXME: make path configurable
    $cookie = $::cgi->cookie(-name=>'BUGLIST',
                       -path=>"/bugzilla/",
                       -value=>'');
    $toolong = 1;
}

print $::cgi->header(-cookie=>"$cookie");
PutHeader("Bug List");

print $::cgi->b(time2str("%a %b %e %T %Z %Y", time()));

if (defined $::cgi->param('debug')) {
    print "<PRE>$query</PRE>\n";
}

if ($toolong) {
    print "<h2>This list is too long for bugzilla's little mind; the\n";
    print "Next/Prev/First/Last buttons won't appear.</h2>\n";
}

# This is stupid.  We really really need to move the quip list into the DB!

my $quip;
if (open (COMMENTS, "<data/comments")) {
    my @cdata;
    while (<COMMENTS>) {
        push @cdata, $_;
    }
    close COMMENTS;
    $quip = $cdata[int(rand($#cdata + 1))];
} else {
    $quip = "Bugzilla would like to put a random quip here, but nobody has entered any.";
}
        

print $::cgi->hr . $::cgi->i($::cgi->a({href=>"newquip.html"}, "$quip"));
print $::cgi->hr({size=>"10"}) . "$tablestart\n";
print $::bugl;
print "</TABLE>\n";

if ($count == 0) {
    print "Zarro Boogs found.\n";
} elsif ($count == 1) {
    print "One bug found.\n";
} else {
    print "$count bugs found.\n";
}

if ($dotweak) {
    GetVersionTable();
my $JSCRIPT=<<END;
numelements = document.changeform.elements.length;
function SetCheckboxes(value) {
    for (var i=0 ; i<numelements ; i++) {
        item = document.changeform.elements[i];
        item.checked = value;
    }
}
document.write("<input type="button" value="Uncheck All" onclick="SetCheckboxes(false);"><input type="button" value="Check All" onclick="SetCheckboxes(true);">");
END
    my $resolution_popup = $::cgi->popup_menu(-name=>"resolution",
                              '-values'=>@::legal_resolution_no_dup,
			      -defaults=>"FIXED");
    my @prod_list = keys %prodhash;
    my @list = @prod_list;
    my @legal_versions;
    my @legal_component;
    if ($#prod_list == 1) {
        @legal_versions = @{$::versions{$prod_list[0]}};
        @legal_component = @{$::components{$prod_list[0]}};
    }
    
    my $version_popup = make_options(\@legal_versions, $::dontchange);
    my $platform_popup = make_options($::legal_platforms{$prod_list[0]}, $::dontchange);
    my $priority_popup = make_options(\@::legal_priority, $::dontchange);
    my $sev_popup = make_options(\@::legal_severity, $::dontchange);
    my $component_popup = make_options($::components{$::cgi->param('product')}, $::cgi->param('component'));
    my $product_popup = make_options(\@::legal_product, $::dontchange);

    my $bug_status_html = Param("bugstatushtml");

    print $::cgi->hr,
	  $::cgi->table(
             $::cgi->TR(
	        $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Product:")),
		$::cgi->td($::cgi->popup_menu(-name=>"product",
		                             '-values'=>@::legal_product)),
	        $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Version:")),
		$::cgi->td($::cgi->popup_menu(-name=>"version",
		                             '-values'=>@::legal_versions))
	     ),
             $::cgi->TR(
	        $::cgi->td({-align=>"RIGHT"}, 
		   $::cgi->a({-href=>"$bug_status_html#rep_platform"}, 
		   $::cgi->b("Platform:"))),
		$::cgi->td($::cgi->popup_menu(-name=>"rep_platform",
		             '-values'=>@{$::legal_platforms{$prod_list[0]}})),
	        $::cgi->td({-align=>"RIGHT"}, 
		   $::cgi->a({-href=>"$bug_status_html#priority"}, 
		   $::cgi->b("Priority:"))),
		$::cgi->td($::cgi->popup_menu(-name=>"priority",
		             '-values'=>$::legal_priority))
	     ),
             $::cgi->TR(
	        $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Component:")),
		$::cgi->td($::cgi->popup_menu(-name=>"component",
		              -default=>$::cgi->param('component'),
		             '-values'=>@{$::components{$::cgi->param('product')}})),
	        $::cgi->td({-align=>"RIGHT"}, 
		   $::cgi->a({-href=>"$bug_status_html#severity"}, 
		   $::cgi->b("Severity:"))),
		$::cgi->td($::cgi->popup_menu(-name=>"severity",
		             '-values'=>$::legal_severity))
	     ),
	  ),
	  $::cgi->hidden(-name=>"multiupdate", -value=>"Y"),
	  $::cgi->b("Additional Comments:"),
	  $::cgi->br,
	  $::cgi->textarea({-wrap=>"HARD"}, 
	                    -name=>"comment",
			    -rows=>"5",
			    -cols=>"80"),
	  $::cgi->br;

    # knum is which knob number we're generating, in javascript terms.

    my $knum = 0;
    print "
<INPUT TYPE=\"radio\" NAME=\"knob\" VALUE=\"none\" CHECKED>
        Do nothing else<br>";
    $knum++;
    print $::cgi->radio_group(-name=>"knob", 
             -value=>"verify", 
	     -default=>"-",
	     -linebreak=>'true',
	     -labels=>{"Verify bugs (change status to $::cgi->b('VERIFIED')"});
    $knum++;
    if (!defined $statushash{'CLOSED'} &&
        !defined $statushash{'VERIFIED'} &&
        !defined $statushash{'RESOLVED'}) {
        print $::cgi->radio_group(-name=>"knob",
	         -value=>"clearresolution",
		 -default=>"-",
		 -linebreak=>'true',
		 -labels=>{"Clear the resolution"});
        $knum++;
        print $::cgi->radio_group(-name=>"knob",
	         -value=>"resolve",
		 -default=>"-",
		 -linebreak=>'true',
		 -labels=>{"Resolve bugs, changing $::cgi->a{-href=>$bug_status_html#resolution}, 'resolution' to $resolution_popup"});
        $knum++;
    }
    if (!defined $statushash{'NEW'} &&
        !defined $statushash{'ASSIGNED'} &&
        !defined $statushash{'REOPENED'}) {
        print $::cgi->radio_group(-name=>"knob",
	         -value=>"reopen",
		 -default=>"-",
		 -linebreak=>'true',
		 -labels=>{"Reopen bugs"});
        $knum++;
    }
    my @statuskeys = keys %statushash;
    if ($#statuskeys == 1) {
        if (defined $statushash{'RESOLVED'}) {
            print $::cgi->radio_group(-name=>"knob",
	             -value=>"verify",
		     -default=>"-",
		     -linebreak=>'true',
		     -labels=>{"Mark bugs as $::cgi->b('VERIFIED')"});
            $knum++;
        }
        if (defined $statushash{'VERIFIED'}) {
            print $::cgi->radio_group(-name=>"knob",
	             -value=>"close",
		     -default=>"-",
		     -linebreak=>'true',
		     -labels=>{"Mark bugs as $::cgi->b('CLOSED')"});
            $knum++;
        }
    }
    print $::cgi->radio_group(-name=>"knob",
	     -value=>"reassign",
	     -default=>"-",
	     -linebreak=>'true',
	     -labels=>{"$::cgi->a({-href=>$bug_status_html#assigned_to},
	                'Reassign') bugs to 
			$::cgi->textfield(-name=>'assigned_to',
			   -size=>'32',
			   -default=>$::cgi->cookie('Bugzilla_login'))"});

    $knum++;
    print $::cgi->radio_group(-name=>"knob",
	     -value=>"reassignbycomponent",
	     -default=>"-",
	     -linebreak=>'true',
	     -labels=>{"Reassign bugs to owner of selected component"});
    $knum++;

    print $::cgi->p,
          $::cgi->font({-size=>"-1"},
	     "To make changes to a bunch of bugs at once:",
	     $::cgi->ol(
	        $::cgi->li("Put check boxes next to the " .
		           "bugs you want to change."),
		$::cgi->li("Adjust above form elements.  (It's " .
		           $::cgi->b("always") . "a good idea to " .
			   "add some comment explaining what you're doing.)"),
		$::cgi->li("Click the below \"Commit\" button.")
	     )
	  ),
          $::cgi->submit(-name=>"submit", -value=>"Commit"),
          $::cgi->endform;
}


if ($count > 0) {
    print $::cgi->table(
             $::cgi->TR(
                $::cgi->td(
                   $::cgi->startform(-method=>"POST", 
                                     -action=>"long_list.cgi"),
                   $::cgi->hidden(-name=>'buglist', -value=>$buglist),
                   $::cgi->submit(-name=>"SUBMIT", -value=>"Long Format"),
                   $::cgi->endform
                ),
                $::cgi->td(
                   $::cgi->startform(-method=>"POST", -action=>"query.cgi"),
	           $::cgi->submit(-name=>"QUERY", -value=>"Query Page"),
                   $::cgi->endform
                ),
                $::cgi->td(
                   $::cgi->startform(-method=>"POST", 
                                     -action=>"colchange.cgi"),
	           $::cgi->submit(-name=>"COLCHANGE", -value=>"Change Columns"),
                   $::cgi->endform
                )
             )
          );

    if (!$dotweak && $count > 1) {
	print $::cgi->a({-href=>"buglist.cgi?$fields&tweak=1"}, 
              "Make changes to several of these bugs at once.") . "\n";
    }
}
if ($serverpush) {
    #print $::cgi->multipart_end;
}
