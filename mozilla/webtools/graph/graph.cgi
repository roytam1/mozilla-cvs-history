#!/usr/bin/perl
# -*- Mode: perl; indent-tabs-mode: nil -*-
use CGI::Carp qw(fatalsToBrowser);
use CGI::Request;
use Date::Calc qw(Add_Delta_Days);  # http://www.engelschall.com/u/sb/download/Date-Calc/

my $req = new CGI::Request;

my $TESTNAME  = lc($req->param('testname'));
my $UNITS     = lc($req->param('units'));
my $TBOX      = lc($req->param('tbox'));
my $AUTOSCALE = lc($req->param('autoscale'));
my $DAYS      = lc($req->param('days'));
my $LTYPE     = lc($req->param('ltype'));
my $POINTS    = lc($req->param('points'));
my $DATAFILE  = "db/$TESTNAME/$TBOX";

sub make_filenames_list {
  my ($dir) = @_;

  my @result;

  if (-d "$dir") {
	chdir "$dir";
	while(<*>) {
	  if( $_ ne 'config.txt' ) {
		push @result, $_;
	  }
	}
	chdir "../..";
  }
  return @result;
}

# Print out a list of testnames in db directory
sub print_testnames {
  my ($testname) = @_;

  # HTTP header
  print "Content-type: text/html\n\n<HTML>\n";
  print "<title>testnames</title>";
  print "<center><h2><b>testnames</b></h2></center>";
  print "<p><table width=\"100%\">";
  print "<tr><td align=center>Select one of the following tests:</td></tr>";
  print "<tr><td align=center>\n";
  print " <table><tr><td><ul>\n";

  my @machines = make_filenames_list("db");
  my $machines_string = join(" ", @machines);

  foreach (@machines) {
	print "<li><a href=graph.cgi?&testname=$_$testname>$_</a>\n";
  }
  print "</ul></td></tr></table></td></tr></table>";

}


# Print out a list of machines in db/<testname> directory, with links.
sub print_machines {
  my ($testname) = @_;

  # HTTP header
  print "Content-type: text/html\n\n<HTML>\n";
  print "<title>$TESTNAME machines</title>";
  print "<center><h2><b>$TESTNAME machines:</b></h2></center>";
  print "<p><table width=\"100%\">";
  print "<tr><td align=center>Select one of the following machines:</td></tr>";
  print "<tr><td align=center>\n";
  print " <table><tr><td><ul>\n";

  my @machines = make_filenames_list("db/$testname");
  my $machines_string = join(" ", @machines);

  foreach (@machines) {
	print "<li><a href=graph.cgi?tbox=$_&testname=$testname&autoscale=0&days=0>$_</a>\n";
  }
  print "</ul></td></tr></table></td></tr></table>";

}

sub show_graph {
  die "$TBOX: no data file found." 
	unless -e $DATAFILE; 

  my $PNGFILE  = "/tmp/gnuplot.$$";

  # Find gnuplot, sorry this is for solaris.
  my $gnuplot;
  if(-f "/usr/bin/gnuplot") {
	$gnuplot = "/usr/bin/gnuplot";
  } elsif(-f "/usr/local/bin/gnuplot") {
	$gnuplot = "/usr/local/bin/gnuplot";
	$ENV{LD_LIBRARY_PATH} .= "/usr/local/lib";
  } else {
	die "Can't find gnuplot.";
  }
  

  # Set scaling for x-axis (time)
  my $xscale;
  my $today;
  my $n_days_ago;

  # Get current time, $today.
  my ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdat) = localtime();  
  $year += 1900;
  $mon += 1;
  $today = sprintf "%04d:%02d:%02d:%02d:%02d:%02d", $year, $mon, $mday, $hour, $min, $sec;

  # Calculate date $DAYS before $today.
  my ($year2, $mon2, $mday2) = Add_Delta_Days($year, $mon, $mday, -$DAYS);
  $n_days_ago= sprintf "%04d:%02d:%02d:%02d:%02d:%02d", $year2, $mon2, $mday2, $hour, $min, $sec;

  if($DAYS) {
	# Assume we want to see 7 days on the graph, so we set the max value also.
	$xscale = "set xrange [\"$n_days_ago\" : \"$today\"]";
  } else {
	$xscale = "";	
  }


  # Set scaling for y-axis.
  my $yscale;
  if($AUTOSCALE) {
	$yscale = "";
  } else {
	$yscale = "set yrange [ 0 : ]";
  }


  # Set units.  Assume ms unless otherwise specified.
  unless($UNITS) {
	$UNITS = "ms";
  }

  my $ltype = "lines";
  if($LTYPE eq "steps") {
	$ltype = "steps";
  }

  my $plot_cmd;
  if($POINTS) {
	# lt <point-color>
	$plot_cmd = "plot \"$DATAFILE\" using 1:2 with $ltype, \"$DATAFILE\" using 1:2 with points ls 1";
  } else {
	$plot_cmd = "plot \"$DATAFILE\" using 1:2 with $ltype";
  }

  # interpolate params into gnuplot command
  # Removing set format x, let gnuplot figure this out.
  #
  # ls 1 = small blue points
  # ls 2 = larger blue points
  my $cmds = qq{
				reset
				set term png color
				set output "$PNGFILE"
				set title  "$TBOX $TESTNAME"
				set key graph 0.1,0.95 reverse spacing .75 width -18
				set linestyle 1 lt 3 lw 1 pt 7 ps .5
				set linestyle 2 lt 3 lw 1 pt 7 ps 1
				set data style points
				set timefmt "%Y:%m:%d:%H:%M:%S"
				set xdata time
				$xscale
				$yscale
				set ylabel "$TESTNAME ($UNITS)"
				set timestamp "Generated: %d/%b/%y %H:%M" 0,-1
				set nokey
				set grid
				$plot_cmd
			   };

# plot "$DATAFILE" using 1:2 with points ps 1, "$DATAFILE" using 1:2 with lines ls 2

  # Set up command string for gnuplot
  open  (GNUPLOT, "| $gnuplot") || die "can't fork: $!";
  print  GNUPLOT $cmds;
  close (GNUPLOT) || die "Empty data set?  Gnuplot failed to set up the plot command string : $!";

  # Actually do the gnuplot command.
  open  (GNUPLOT, "< $PNGFILE") || die "can't read: $!";
  { local $/; $blob = <GNUPLOT>; }
  close (GNUPLOT) || die "can't close: $!";
  unlink $PNGFILE || die "can't unlink $PNGFILE: $!";
  
  print "Content-type: image/png\n\n";
  print $blob;
}

# main
{
  if(!$TESTNAME) {
	print_testnames();
  } elsif(!$TBOX) {
	print_machines($TESTNAME);
  } else {
	show_graph();
  }
}

exit 0;

