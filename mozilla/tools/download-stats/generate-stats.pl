#!/usr/bin/perl

use DBI;
use LWP::Simple;
use Template;
use strict;

# Establish a database connection.
my $dsn = "DBI:mysql:host=mecha.mozilla.org;database=downloadstats;port=3306";
my $dbh = DBI->connect($dsn,
                       "logprocessord",
                       "1ssw?w?",
                       { RaiseError => 1,
                         PrintError => 0,
                         ShowErrorStatement => 1 }
                      );

################################################################################
# Stats Configuration

# All these variables can be redefined in the stats definition file.
# These are just the default values.

# List of stats to generate.
our @stats;

# Date range to which we limit the query.
our ($start_date, $end_date);

# Whether or not to try to add up partial downloads from the same client
# to see if they count as a complete download.  Doesn't work well without
# DNS lookups, which we aren't doing for performance reasons.
our $do_segment_count = 0;

# Parse the definition file and make sure it defines some stats.
my $stats_defs = $ARGV[0];
defined($stats_defs) or die "You didn't reference a stats definition file.\n";
do $stats_defs || die "Couldn't parse stats definition file: $!\n";
defined(@stats) or die "The stats definition file didn't define any stats.\n";

################################################################################
# Data Validation

if ($start_date) {
    $start_date =~ /^\d\d\d\d-\d\d-\d\d( \d\d:\d\d(:\d\d)?)?$/
      or die "Invalid start date $start_date (must be in format yyyy-mm-dd (hh:mm(:ss)?)?).";
}

if ($end_date) {
    $end_date =~ /^\d\d\d\d-\d\d-\d\d( \d\d:\d\d(:\d\d)?)?$/
      or die "Invalid end date $end_date (must be in format yyyy-mm-dd (hh:mm(:ss)?)?).";
}

################################################################################
# Queries

my @date_criteria = ("1=1");
if ($start_date) { push(@date_criteria, "date_time >= '$start_date'") }
if ($end_date)   { push(@date_criteria, "date_time <= '$end_date'") }
my $date_clause = join(" AND ", @date_criteria);

# Completed downloads.
my $done = 
    $dbh->prepare("SELECT COUNT(*) FROM entries JOIN files ON entries.file_id = files.id " .
                  "WHERE $date_clause AND files.path = ? AND files.name = ? AND bytes = ?");

# Not completed downloads.
my $not_done = 
    $dbh->prepare("SELECT COUNT(*) FROM entries JOIN files ON entries.file_id = files.id " .
                  "WHERE $date_clause AND files.path = ? AND files.name = ? AND bytes != ? " .
                  "AND status = 200");

# Partial content requests; may or may not be completed.
my $may_be_done = 
    $dbh->prepare("SELECT COUNT(*) FROM entries JOIN files ON entries.file_id = files.id " .
                  "WHERE $date_clause AND files.path = ? AND files.name = ? AND status = 206");

# A way to get the count of people who altogether completed a download.
# Only run if $do_segment_count is true.  Note that this query is expensive
# and only ever returns a fraction of the total, so it's not that useful.
# Also, it probably doesn't work unless we reverse DNS every address
# in the logs, which we aren't doing at the moment for performance.
my $done_in_segments = 
    $dbh->prepare("SELECT 1 FROM entries JOIN files ON entries.file_id = files.id " .
                  "WHERE $date_clause AND files.path = ? AND files.name = ? " .
                  "GROUP BY client HAVING SUM(bytes) = ? AND COUNT(bytes) > 1");


################################################################################
# Stats Retrieval

foreach my $stat (@stats) {
    next if !$stat->{isactive};

    print STDERR "$stat->{name} $stat->{version}...\n";

    my $platforms = $stat->{platforms};

    foreach my $platform (keys %$platforms) {
	print STDERR "  $platform\n";

        my $files = $platforms->{$platform};

	foreach my $type (keys %$files) {
	    print STDERR "    $type: ";

	    my $file = $files->{$type};

	    my (undef, $file_size) = head("http://ftp.mozilla.org$stat->{path}/$file->{name}");
            $file_size ||= $file->{size}
	      or die "Can't figure out the size of $stat->{path}/$file->{name}.";

            $done->execute($stat->{path}, $file->{name}, $file_size);
	    my ($done_count) = $done->fetchrow_array();

	    my $done_in_segments_count = "N/A";
	    my $total_done = $done_count;
	    if ($do_segment_count) {
		$done_in_segments->execute($stat->{path}, $file->{name}, $file_size);
		$done_in_segments_count = $done_in_segments->fetchall_arrayref();
		$done_in_segments_count = scalar(@$done_in_segments_count);
		$total_done += $done_in_segments_count;
	    }

            $not_done->execute($stat->{path}, $file->{name}, $file_size);
	    my ($not_done_count) = $not_done->fetchrow_array();

            $may_be_done->execute($stat->{path}, $file->{name});
	    my ($may_be_done_count) = $may_be_done->fetchrow_array();

            $file->{counts} = {
              complete_uni   => $done_count,
              #complete_multi => $done_in_segments_count, 
              incomplete     => $not_done_count, 
              partial        => $may_be_done_count, 
            };

	    print STDERR "$done_count / $not_done_count / $may_be_done_count / $done_in_segments_count / $total_done\n";
	}
    }
}

################################################################################
# Output

my $template = <<'EOF';
<html>
<head>
  <title></title>
  <style type="text/css">
    th { text-align: left; }
    th, td { border: solid 1px black; }
    table { border-collapse: collapse;
            border: solid 1px black; }
  </style>
</head>
<body>
[% FOREACH stat = stats %]
  [% NEXT IF !stat.isactive %]
  [% app_total = 0 %]
  <h2>[% stat.name %] [%+ stat.version %] Download Stats</h2>

  <p>[% start_date || "the beginning of time" %] to [% end_date || "the end of time" %]</p>

  <table summary="[% stat.name %] [%+ stat.version %] Downloads">
    <tr>
      <th>Build</th>
      <th>Downloads</th>
    </tr>
    [% FOREACH platform = stat.platforms %]
      [% platform_total = 0 %]

      <tr>
        <td colspan="2"><h3>[% platform.key %]</h3></td>
      </tr>

      [% files = platform.value %]
      [% FOREACH file = files %]
        [% file_total = file.value.counts.complete_uni + file.value.counts.complete_multi %]
        <tr>
          <td>[% file.key %]:[% file.value.name %]</td>
          <td>[% file_total %]</td>
        </tr>
        [% platform_total = platform_total + file_total %]
      [% END %]

      <tr>
        <td>total for [% platform.key %]</td>
        <td>[% platform_total %]</td>
      </tr>

      [% app_total = app_total + platform_total %]
    [% END %]
    <tr>
      <td>grand total</td>
      <td>[% app_total %]</td>
    </tr>
  </table>
[% END %]
</body>
</html>
EOF

my $tt = new Template({ PRE_CHOMP => 1, POST_CHOMP => 1});

$tt->process(\$template, {stats => \@stats, 
			  start_date => $start_date, 
			  end_date => $end_date})
    || die "Template process failed: ", $template->error(), "\n";
