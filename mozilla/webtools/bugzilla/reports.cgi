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
# Contributor(s): Harrison Page <harrison@netscape.com>,
# Terry Weissman <terry@mozilla.org>

use diagnostics;
use strict;
use Chart::Lines;

require "CGI.pl";
require "globals.pl";

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

# patch from Sam Ziegler <sam@ziegler.org>:
#
# "reports.cgi currently has it's own idea of what 
# the header should be. This patch sets it to the 
# system wide header." 

print "Content-type: text/html\n\n";

if (defined $::FORM{'nobanner'})
	{
print <<FIN;
<html>
<head><title>Bug Reports</title></head>
<body bgcolor="#FFFFFF">
FIN
	}
else
	{
	PutHeader ("Bug Reports") unless (defined $::FORM{'nobanner'});
	}

ConnectToDatabase();
GetVersionTable();

$::FORM{'output'} = $::FORM{'output'} || "most_doomed"; # a reasonable default

if (! defined $::FORM{'product'})
	{
	&choose_product;
	}
else
	{
	# we want to be careful about what subroutines 
	# can be called from outside. modify %reports
	# accordingly when a new report type is added

	if (! exists $reports{$::FORM{'output'}})
		{
		$::FORM{'output'} = "most_doomed"; # a reasonable default
		}
	
	my $f = $reports{$::FORM{'output'}};

	if (! defined $f)
		{
		print "start over, your form data was all messed up.<p>\n";
		foreach (keys %::FORM)
			{
			print "<font color=blue>$_</font> : " . 
				($::FORM{$_} ? $::FORM{$_} : "undef") . "<br>\n";
			}
		exit;
		}

	&{$f};
	}

print <<FIN;
<p>
</body>
</html>
FIN

##################################
# user came in with no form data #
##################################

sub choose_product
	{
	my $product_popup = make_options (\@::legal_product, $::legal_product[0]);
	my $charts = (-d $dir) ? "<option value=\"show_chart\">Bug Charts" : "";

	print <<FIN;
<center>
<h1>Welcome to the Bugzilla Query Kitchen</h1>
<form method=get action=reports.cgi>
<table border=1 cellpadding=5>
<tr>
<td align=center><b>Product:</b></td>
<td align=center>
<select name="product">
$product_popup
</select>
</td>
</tr>
<tr>
<td align=center><b>Output:</b></td>
<td align=center>
<select name="output">
<option value="most_doomed">Bug Counts
$charts
</select>
<tr>
<td align=center><b>Switches:</b></td>
<td align=left>
<input type=checkbox name=links value=1>&nbsp;Links to Bugs<br>
<input type=checkbox name=nobanner value=1>&nbsp;No Banner<br>
<input type=checkbox name=quip value=1>&nbsp;Include Quip<br>
</td>
</tr>
<tr>
<td colspan=2 align=center>
<input type=submit value=Continue>
</td>
</tr>
</table>
</form>
<p>
FIN
	}

sub most_doomed
	{
	my $when = localtime (time);

	print <<FIN;
<center>
<h1>
Bug Report for $::FORM{'product'}
</h1>
$when<p>
FIN

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
and    bugs.product='$::FORM{'product'}'
and 	 
	( 
	bugs.bug_status = 'NEW' or 
	bugs.bug_status = 'ASSIGNED' or 
	bugs.bug_status = 'REOPENED'
	)
FIN

	print "<font color=purple><tt>$query</tt></font><p>\n" 
		unless (! exists $::FORM{'showsql'});

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

	if ($::FORM{'quip'})
		{
		if (open (COMMENTS, "<data/comments")) 
			{
    	my @cdata;
			while (<COMMENTS>) 
				{
				push @cdata, $_;
				}
			close COMMENTS;
			$quip = "<i>" . $cdata[int(rand($#cdata + 1))] . "</i>";
			}
		} 

	#########################
	# start painting report #
	#########################

	print <<FIN;
<h1>$quip</h1>
<table border=1 cellpadding=5>
<tr>
<td align=right><b>New Bugs This Week</b></td>
<td align=center>$bugs_new_this_week</td>
</tr>

<tr>
<td align=right><b>Bugs Marked New</b></td>
<td align=center>$bugs_status{'NEW'}</td>
</tr>

<tr>
<td align=right><b>Bugs Marked Assigned</b></td>
<td align=center>$bugs_status{'ASSIGNED'}</td>
</tr>

<tr>
<td align=right><b>Bugs Marked Reopened</b></td>
<td align=center>$bugs_status{'REOPENED'}</td>
</tr>

<tr>
<td align=right><b>Total Bugs</b></td>
<td align=center>$bugs_count</td>
</tr>

</table>
<p>
FIN

	if ($bugs_count == 0)
		{
		print "No bugs found!\n";
		exit;
		}
	
	print <<FIN;
<h1>Bug Count by Engineer</h1>
<table border=3 cellpadding=5>
<tr>
<td align=center bgcolor="#DDDDDD"><b>Owner</b></td>
<td align=center bgcolor="#DDDDDD"><b>New</b></td>
<td align=center bgcolor="#DDDDDD"><b>Assigned</b></td>
<td align=center bgcolor="#DDDDDD"><b>Reopened</b></td>
<td align=center bgcolor="#DDDDDD"><b>Total</b></td>
</tr>
FIN

	foreach my $who (sort keys %bugs_summary)
		{
		my $bugz = 0;
	 	print <<FIN;
<tr>
<td align=left><tt>$who</tt></td>
FIN
		
		foreach my $st (@status)
			{
			$bugs_totals{$who}{$st} += 0;
			print <<FIN;
<td align=center>$bugs_totals{$who}{$st}
FIN
			$bugz += $#{$bugs_summary{$who}{$st}} + 1;
			}
		
		print <<FIN;
<td align=center>$bugz</td>
</tr>
FIN
		}
	
	print <<FIN;
</table>
<p>
FIN

	###############################
	# individual bugs by engineer #
	###############################

	print <<FIN;
<h1>Individual Bugs by Engineer</h1>
<table border=1 cellpadding=5>
<tr>
<td align=center bgcolor="#DDDDDD"><b>Owner</b></td>
<td align=center bgcolor="#DDDDDD"><b>New</b></td>
<td align=center bgcolor="#DDDDDD"><b>Assigned</b></td>
<td align=center bgcolor="#DDDDDD"><b>Reopened</b></td>
</tr>
FIN

	foreach my $who (sort keys %bugs_summary)
		{
		print <<FIN;
<tr>
<td align=left><tt>$who</tt></td>
FIN

		foreach my $st (@status)
			{
			my @l;

			foreach (sort { $a <=> $b } @{$bugs_summary{$who}{$st}})
				{
				if ($::FORM{'links'})
					{
					push @l, "<a href=\"show_bug.cgi?id=$_\">$_</a>\n"; 
					}
				else
					{
					push @l, $_;
					}
				}
				
			my $bugz = join ' ', @l;
			$bugz = "&nbsp;" unless ($bugz);
			
			print <<FIN
<td align=left>$bugz</td>
FIN
			}

		print <<FIN;
</tr>
FIN
		}

	print <<FIN;
</table>
<p>
FIN
	}

sub is_legal_product
	{
	my $product = shift;
	return grep { $_ eq $product} @::legal_product;
	}

sub header
	{
	print <<FIN;
<TABLE BGCOLOR="#000000" WIDTH="100%" BORDER=0 CELLPADDING=0 CELLSPACING=0>
<TR><TD><A HREF="http://www.mozilla.org/"><IMG
 SRC="http://www.mozilla.org/images/mozilla-banner.gif" ALT=""
 BORDER=0 WIDTH=600 HEIGHT=58></A></TD></TR></TABLE>
FIN
	}

sub show_chart
	{
  my $when = localtime (time);

	if (! is_legal_product ($::FORM{'product'}))
		{
		&die_politely ("Unknown product: $::FORM{'product'}");
		}

  print <<FIN;
<center>
FIN
	
	my @dates;
	my @open; my @assigned; my @reopened;

        my $prodname = $::FORM{'product'};

        $prodname =~ s/\//-/gs;

        my $file = join '/', $dir, $prodname;
	my $image = "$file.gif";

	if (! open FILE, $file)
		{
		&die_politely ("The tool which gathers bug counts has not been run yet.");
		}
	
	while (<FILE>)
		{
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

	if ($#dates < 1)
		{
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

    my $MAXTICKS = 20;      # Try not to show any more x ticks than this.
    my $skip = 1;
    if (@dates > $MAXTICKS) {
        $skip = int((@dates + $MAXTICKS - 1) / $MAXTICKS);
    }

	my %settings =
		(
		"title" => "Bug Charts for $::FORM{'product'}",
		"x_label" => "Dates",
		"y_label" => "Bug Count",
		"legend_labels" => \@labels,
        "skip_x_ticks" => $skip,
		);
	
	$img->set (%settings);
	
	open IMAGE, ">$image" or die "$image: $!";
	$img->gif (*IMAGE, \@data);
	close IMAGE;

	print <<FIN;
<img src="$image">
<br clear=left>
<br>
FIN
	}

sub die_politely
	{
	my $msg = shift;

	print <<FIN;
<p>
<table border=1 cellpadding=10>
<tr>
<td align=center>
<font color=blue>Sorry, but ...</font>
<p>
There is no graph available for <b>$::FORM{'product'}</b><p>

<font size=-1>
$msg
<p>
</font>
</td>
</tr>
</table>
<p>
FIN
	
	exit;
	}


