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
# $Id$
#
# Contributor(s): Harrison Page <harrison@netscape.com>,
#                 Terry Weissman <terry@mozilla.org>,
#                 Andrew Anderson <andrew@redhat.com>

use diagnostics;
use strict;
use Chart::Lines;
use CGI;

$::cgi = new CGI;

require "CGI.pl";

use vars @::legal_product;

my $dir = "data/mining";
my $week = 60 * 60 * 24 * 7;
my @status = qw (NEW ASSIGNED REOPENED);

# while this looks odd/redundant, it allows us to name
# functions differently than the value passed in

my %reports = 
	( 
	"most_doomed" => \&most_doomed,
	"show_chart" => \&show_chart,
	);

# patch from Sam Ziegler <ziegler@mediaguaranty.com>:
#
# "reports.cgi currently has it's own idea of what 
# the header should be. This patch sets it to the 
# system wide header." 

print $::cgi->header('text/html');

if ($::cgi->param('nobanner') ne "") {
	print $::cgi->start_html(-title=>"Bug Reports", -BGCOLOR=>"#FFFFFF");
} else {
	PutHeader ("Bug Reports");
}

ConnectToDatabase();
GetVersionTable();

$::cgi->param(-name=>'output', 
              # a reasonable default
              -value=>$::cgi->param('output') || "most_doomed",
              -override=>"1");

if ($::cgi->param('product') eq "") {
	&choose_product;
} else {
	# we want to be careful about what subroutines 
	# can be called from outside. modify %reports
	# accordingly when a new report type is added

	if (! exists $reports{$::cgi->param('output')}) {
		# a reasonable default
		$::cgi->param(-name=>'output', 
                              -value=>"most_doomed", 
                              -override=>"1");
	}
	
	my $f = $reports{$::cgi->param('output')};

	if (! defined $f) {
		print "start over, your form data was all messed up.<p>\n";
		foreach (%::cgi->param()) {
			print $::cgi->font({-color=>"BLUE"}, "$_") .
			      " : " . 
			      ($::cgi->param($_) ? $::cgi->($_) : "undef") . 
                              $::cgi->br;
		}
		print $::cgi->end_html;
		exit;
	}

	&{$f};
}

print $::cgi->p, $::cgi->end_html;

##################################
# user came in with no form data #
##################################

sub choose_product {
	my $charts;
	my @chart_values;
        my %chart_labels;
        $chart_labels{'most_doomed'} = "Bug Counts";
        $chart_labels{'show_chart'} = "Bug Charts";
	push(@chart_values, 'most_doomed');
	push(@chart_values, 'show_chart') if (-d $dir);
	$charts = $::cgi->popup_menu(-name=>'output',
                                     '-values'=>\@chart_values,
                                     -labels=>\%chart_labels); 

	print $::cgi->center(
                 $::cgi->h1("Welcome to the Bugzilla Query Kitchen"),
                 $::cgi->startform,
                 $::cgi->table({-border=>"1", -cellpadding=>"5"},
                    $::cgi->TR(
                       $::cgi->td({-align=>"CENTER"}, $::cgi->b("Product:")),
		       $::cgi->td({-align=>"CENTER"}, 
		          $::cgi->popup_menu(-name=>"product",
			                   '-values'=>\@::legal_product,
					   -default=>$::legal_product[0])
                       ),
                    ),
		    $::cgi->TR(
                       $::cgi->td({-align=>"CENTER"}, $::cgi->b("Output:")),
		       $::cgi->td({-align=>"CENTER"}, "$charts")
                    ),
		    $::cgi->TR(
                       $::cgi->td({-align=>"CENTER"}, $::cgi->b("Switches:")),
                       $::cgi->td({-align=>"LEFT"}, 
		           $::cgi->checkbox(-name=>"links",
			                  -checked=>'checked',
					  -label=>"Links to Bugs",
			                  -value=>"1"),
			   $::cgi->br,
		           $::cgi->checkbox(-name=>"nobanner",
					  -label=>"No Banner",
					  -value=>"1"),
			   $::cgi->br,
		           $::cgi->checkbox(-name=>"quip",
			                  -checked=>'checked',
					  -label=>"Include Quip",
					  -value=>")1"),
			   $::cgi->br
		        )
                    ),
		    $::cgi->TR(
                       $::cgi->td({-align=>"CENTER", -colspan=>"2"}, 
		           $::cgi->submit(-name=>"submit", -value=>"Continue"))
		    )
		 ),
                 $::cgi->endform,
                 $::cgi->p,
	       );
}

sub most_doomed {
	my $when = localtime (time);
	my $product = $::cgi->param('product');

	print $::cgi->center(
	         $::cgi->h1("Bug Report for $product"),
		 $when,
		 $::cgi->p
	      );

	my $query = <<FIN;
select 
	bugs.bug_id, bugs.assigned_to, bugs.bug_severity,
	bugs.bug_status, bugs.product, 
	assign.login_name,
	report.login_name,
	unix_timestamp(date_format(bugs.creation_ts, '%Y-%m-%d %h:%m:%s'))

from   bugs,
       profiles assign,
       profiles report,
       versions projector
where  bugs.assigned_to = assign.userid
and    bugs.reporter = report.userid
and    bugs.product='$product'
and 	 
	( 
	bugs.bug_status = 'NEW' or 
	bugs.bug_status = 'ASSIGNED' or 
	bugs.bug_status = 'REOPENED'
	)
FIN

	print $::cgi->font({-color=>"PURPLE"}, $::cgi->tt($query)), $::cgi->p
		unless ($::cgi->param('showsql') eq "");

	SendSQL ($query);
	
	my $c = 0;

	my $quip = "Summary";
	my $bugs_count = 0;
	my $bugs_new_this_week = 0;
	my $bugs_reopened = 0;
	my %bugs_owners;
	my %bugs_summary;
	my %bugs_status;
	my %bugs_totals;
	my %bugs_lookup;

	#############################
	# suck contents of database # 
	#############################

	while (my ($bid, $a, $sev, $st, $prod, $who, $rep, $ts) = FetchSQLData())
		{
		next if (exists $bugs_lookup{$bid});
		
		$bugs_lookup{$bid} ++;
		$bugs_owners{$who} ++;
		$bugs_new_this_week ++ if (time - $ts <= $week);
		$bugs_status{$st} ++;
		$bugs_count ++;
		
		push @{$bugs_summary{$who}{$st}}, $bid;
		
		$bugs_totals{$who}{$st} ++;
	}

	if ($::cgi->param('quip')) {
		if (open (COMMENTS, "<data/comments")) {
    	                my @cdata;
			while (<COMMENTS>) {
				push @cdata, $_;
			}
			close COMMENTS;
			$quip = $::cgi->i($cdata[int(rand($#cdata + 1))]);
		}
	} 

	#########################
	# start painting report #
	#########################

	print $::cgi->center(
                  $::cgi->h1("$quip"),
	          $::cgi->table({-border=>"1", -cellpadding=>"5"},
	             $::cgi->TR(
		        $::cgi->td({-align=>"RIGHT"}, 
		            $::cgi->b("New Bugs This Week")),
		        $::cgi->td({-align=>"CENTER"}, 
                            $bugs_new_this_week ? $bugs_new_this_week 
                                                : "&nbsp;")
		     ),
		     $::cgi->TR(
		        $::cgi->td({-align=>"RIGHT"}, 
                            $::cgi->b("Bugs Marked New")),
		        $::cgi->td({-align=>"CENTER"}, 
                            $bugs_status{'NEW'} ? $bugs_status{'NEW'}
                                                : "&nbsp;")
		     ),
		     $::cgi->TR(
		        $::cgi->td({-align=>"RIGHT"}, 
		            $::cgi->b("Bugs Marked Assigned")),
		        $::cgi->td({-align=>"CENTER"}, 
                            $bugs_status{'ASSIGNED'} ? $bugs_status{'ASSIGNED'}
                                                     : "&nbsp;")
		     ),
		     $::cgi->TR(
		        $::cgi->td({-align=>"RIGHT"}, 
		            $::cgi->b("Bugs Marked Reopened")),
		        $::cgi->td({-align=>"CENTER"}, 
                            $bugs_status{'REOPENED'} ? $bugs_status{'REOPENED'}
                                                     : "&nbsp;")
		     ),
		     $::cgi->TR(
		        $::cgi->td({-align=>"RIGHT"}, $::cgi->b("Total Bugs")),
		        $::cgi->td({-align=>"CENTER"}, 
                            $bugs_count ? $bugs_count : "&nbsp;")
		     )
	          )
	      ),
	      $::cgi->p;


	if ($bugs_count == 0) {
		print "No bugs found!\n";
		print $::cgi->end_html;
		exit;
	}

	my $tablerow;
	my @tabledata;

	foreach my $who (sort keys %bugs_summary) {
		my $bugz = 0;
		my @totals;
		foreach my $st (@status) {
			$bugs_totals{$who}{$st} += 0;
			push(@totals, $::cgi->td({-align=>"CENTER"}, 
			     $bugs_totals{$who}{$st}));
			$bugz += $#{$bugs_summary{$who}{$st}} + 1;
		}
		$tablerow = $::cgi->TR(
		               $::cgi->td({-align=>"LEFT"}, $::cgi->tt($who)),
			       @totals,
		               $::cgi->td({-align=>"LEFT"}, $bugz),
			    );
		push(@tabledata, $tablerow);
	}
	
	print $::cgi->center(
	          $::cgi->h1("Bug Count by Engineer"),
	          $::cgi->table({-border=>"3", -cellpadding=>"5"},
	              $::cgi->TR(
		          $::cgi->td({-align=>"CENTER", -BGCOLOR=>"#DDDDDD"},
		              $::cgi->b("Owner")),
		          $::cgi->td({-align=>"CENTER", -BGCOLOR=>"#DDDDDD"},
		              $::cgi->b("New")),
		          $::cgi->td({-align=>"CENTER", -BGCOLOR=>"#DDDDDD"},
		              $::cgi->b("Assigned")),
		          $::cgi->td({-align=>"CENTER", -BGCOLOR=>"#DDDDDD"},
		              $::cgi->b("Reopened")),
		          $::cgi->td({-align=>"CENTER", -BGCOLOR=>"#DDDDDD"},
		              $::cgi->b("Total")),
		      ),
		      @tabledata
	          )
	      ),
	      $::cgi->p;
	
	###############################
	# individual bugs by engineer #
	###############################

	@tabledata = undef;
	foreach my $who (sort keys %bugs_summary) {
		my @temprow;
		foreach my $st (@status) {
			my @l;
			foreach (sort {$a<=>$b} @{$bugs_summary{$who}{$st}}) {
				if ($::cgi->param('links')) {
					push @l, $::cgi->a({-href=>"show_bug.cgi?id=$_"}, "$_");
				} else {
					push @l, $_;
				}
			}
				
			my $bugz = join ' ', @l;
			$bugz = "&nbsp;" unless ($bugz);
			
			push(@temprow, $::cgi->td({-align=>"LEFT"}, $bugz));
		}
		$tablerow = $::cgi->TR(
		            $::cgi->td({-align=>"LEFT"}, $::cgi->tt("$who")),
			    @temprow
		);
		push(@tabledata, $tablerow);
	}
	print $::cgi->center(
	          $::cgi->h1("Individual Bugs by Engineer"),
	          $::cgi->table({-border=>"1", -cellpadding=>"5"},
	              $::cgi->TR(
		          $::cgi->td({-align=>"CENTER", -BGCOLOR=>"#DDDDDD"},
		              $::cgi->b("Owner")),
		          $::cgi->td({-align=>"CENTER", -BGCOLOR=>"#DDDDDD"},
		              $::cgi->b("New")),
		          $::cgi->td({-align=>"CENTER", -BGCOLOR=>"#DDDDDD"},
		              $::cgi->b("Assigned")),
		          $::cgi->td({-align=>"CENTER", -BGCOLOR=>"#DDDDDD"},
		              $::cgi->b("Reopened"))
		      ),
		      @tabledata
	          )
	      ),
	      $::cgi->p;

}

sub is_legal_product {
	my $product = shift;
	return grep { $_ eq $product} @::legal_product;
}

sub header {
	print $::cgi->table({-BGCOLOR=>"#000000", -width=>"100%", -border=>"0",
	                   -cellpadding=>"0", -cellspacing=>"0"},
	          $::cgi->TR(
		      $::cgi->td(
		          $::cgi->a({-href=>"http://www.mozilla.org/"}, 
			      $::cgi->img({-src=>"http://www.mozilla.org/images/mozilla-banner.gif", -alt=>"", -border=>"0", -width=>"600", -height=>"58"})
			  )
		      )
		  )
	      )
}

sub show_chart {
    my $when = localtime (time);
    my $product = $::cgi->param('product');

	if (! is_legal_product($product)) {
		&die_politely ("Unknown product: $product");
	}

    print "<CENTER>";
	
	my @dates;
	my @open;
	my @assigned;
	my @reopened;
        my $prodname = $product;

        $prodname =~ s/\//-/gs;

        my $file = join '/', $dir, $prodname;
	if (! open FILE, $file) {
		&die_politely ("The tool which gathers bug counts has not been run yet.");
	}
	
	my $image = "$file.gif";
	while (<FILE>) {
		chomp;
		next if ($_ =~ /^#/ or ! $_);
		my ($date, $open, $assigned, $reopened) = split /\|/, $_;
		my ($yy, $mm, $dd) = $date =~ /^\d{2}(\d{2})(\d{2})(\d{2})$/;

		push @dates, "$mm/$dd/$yy";
		push @open, $open;
		push @assigned, $assigned;
		push @reopened, $reopened;
	}
	
	close FILE;

	if ($#dates < 1) {
		&die_politely ("We don't have enough data points to make a graph (yet)");
	}
	
	my $img = Chart::Lines->new (800, 600);
	my @labels = qw (New Assigned Reopened);
	my @when;
	my $i = 0;
	my @data;

	push @data, \@dates;
	push @data, \@open;
	push @data, \@assigned;
	push @data, \@reopened;

	my %settings = (
		"title" => "Bug Charts for $product",
		"x_label" => "Dates",
		"y_label" => "Bug Count",
		"legend_labels" => \@labels,
	);
	
	$img->set (%settings);
	
	$img->gif("$image", \@data);

	# FIXME: get this hack fixed
	$image =~ s/ /%20/g;

	print $::cgi->img({-src=>$image}),
	      $::cgi->br({-clear=>"LEFT"}),
	      $::cgi->br;
}

sub die_politely {
	my $msg = shift;
	my $product = $::cgi->param('product');

	print $::cgi->p,
	      $::cgi->table({-border=>"1", -cellpadding=>"10"},
	          $::cgi->TR(
		      $::cgi->td({-align=>"CENTER"},
		          $::cgi->font({-color=>"BLUE"}, "Sorry, but ..."),
			  $::cgi->p,
			  "There is no graph available for ",
			  $::cgi->b("$product"),
			  $::cgi->p,
			  $::cgi->font({-size=>"-1"}, "$msg"),
			  $::cgi->p
		      )
		  )
	      ),
	      $::cgi->p,
	      $::cgi->end_html;
	exit;
}


