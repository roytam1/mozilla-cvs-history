#! /usr/bonsaitools/bin/perl
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.1 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Tinderbox
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1999
# Netscape Communications Corporation. All Rights Reserved.
#
# Contributor(s): Stephen Lamm <slamm@mozilla.org>

use FileHandle;

$tree = 'SeaMonkey';
# tinderbox/globals.pl uses many shameful globals
$form{tree} = $tree;
require 'globals.pl';

$cvsroot = '/cvsroot/mozilla';
$lxr_data_root = '/export2/lxr-data';
@ignore = ( 
  'long long',
  '__cmsg_data',
  'location of the previous definition',
  'by \`'
);
$ignore_pat = "(?:".join('|',@ignore).")";

print STDERR "Building hash of file names...";
($file_bases, $file_fullpaths) = build_file_hash($cvsroot, $tree);
print STDERR "done.\n";

for $br (last_successful_builds($tree)) {
  next unless $br->{buildname} =~ /shrike.*\b(Clobber|Clbr)\b/;

  my $log_file = "$br->{logfile}";

  warn "Parsing build log, $log_file\n";

  $fh = new FileHandle "gunzip -c $tree/$log_file |";
  &gcc_parser($fh, $cvsroot, $tree, $log_file, $file_bases, $file_fullpaths);
  $fh->close;

  &build_blame;

  my $warn_file = "$tree/warn$log_file";
  $warn_file =~ s/.gz$/.html/;

  $fh->open(">$warn_file") or die "Unable to open $warn_file: $!\n";
  &print_warnings_as_html($fh, $br);
  $fh->close;
  warn "Wrote output to $warn_file\n";

  last;
}

# end of main
# ===================================================================

sub build_file_hash {
  my ($cvsroot, $tree) = @_;

  $lxr_data_root = "/export2/lxr-data/\L$tree";

  $lxr_file_list = "\L$lxr_data_root/.glimpse_filenames";
  open(LXR_FILENAMES, "<$lxr_file_list")
    or die "Unable to open $lxr_file_list: $!\n";

  use File::Basename;
  
  while (<LXR_FILENAMES>) {
    my ($base, $dir, $ext) = fileparse($_,'\.[^/]*');

    next unless $ext =~ /^\.(cpp|h|C|s|c|mk|in)$/;

    $base = "$base$ext";
    $dir =~ s|$lxr_data_root/mozilla/||;
    $dir =~ s|/$||;

    $fullpath{"$dir/$base"}=1;

    unless (exists $bases{$base}) {
      $bases{$base} = $dir;
    } else {
      $bases{$base} = '[multiple]';
    }
  }
  return \%bases, \%fullpath;
}

sub last_successful_builds {
  my $tree = shift;
  my @build_records = ();
  my $br;


  $maxdate = time;
  $mindate = $maxdate - 5*60*60; # Go back 5 hours
  
  print STDERR "Loading build data...";
  &load_data;
  print STDERR "done\n";
  
  for (my $ii=1; $ii <= $name_count; $ii++) {
    for (my $tt=1; $tt <= $time_count; $tt++) {
      if (defined($br = $build_table->[$tt][$ii])
          and $br->{buildstatus} eq 'success') {
        push @build_records, $br;
        last;
  } } }
  return @build_records;
}

sub gcc_parser {
  my ($fh, $cvsroot, $tree, $log_file, $file_bases, $file_fullnames) = @_;
  my $dir = '';

  while (<$fh>) {
    # Directory
    #
    if (/^gmake\[\d\]: Entering directory \`(.*)\'$/) {
      ($build_dir = $1) =~ s|.*/mozilla/||;
      next;
    }

    # Now only match lines with "warning:"
    next unless /warning:/;
    next if /$ignore_pat/o;

    chomp; # Yum, yum

    my ($filename, $line, $warning_text);
    ($filename, $line, undef, $warning_text) = split /:\s*/, $_, 4;
    $filename =~ s/.*\///;
    
    # Special case for Makefiles
    $filename =~ s/Makefile$/Makefile.in/;

    my $dir = '';
    if ($file_fullnames->{"$build_dir/$filename"}) {
      $dir = $build_dir;
    } else {
      unless(defined($dir = $file_bases->{$filename})) {
        $dir = '[no_match]';
      }
    }
    my $file = "$dir/$filename";

    unless (defined($warnings{$file}{$line})) {
      # Remember where in the build log the warning occured

      $warnings{$file}{$line} = {
         first_seen_line => $.,
         log_file        => $log_file,
         warning_text    => $warning_text,
      };
    }
    $warnings{$file}{$line}->{count}++;
    $total_warnings_count++;
  }
}


sub dump_warning_data {
  while (my ($file, $lines_hash) = each %warnings) {
    while (my ($line, $record) = each %{$lines_hash}) {
      print join ':', 
      $file,$line,
      $record->{first_seen_line},
      $record->{count},
      $record->{warning_text};
      print "\n";
    }
  }
}

sub build_blame {
  use lib '../bonsai';
  require 'utils.pl';
  require 'cvsblame.pl';

  while (($file, $lines_hash) = each %warnings) {

    my $rcs_filename = "$cvsroot/$file,v";

    unless (-e $rcs_filename) {
      warn "Unable to find $rcs_filename\n";
      $unblamed{$file} = 1;
      next;
    }

    my $revision = &parse_cvs_file($rcs_filename);
    @text = &extract_revision($revision);
    while (my ($line, $warn_rec) = each %{$lines_hash}) {
      my $line_rev = $revision_map[$line-1];
      my $who = $revision_author{$line_rev};
      my $source_text = join '', @text[$line-4..$line+2];
      chomp $source_text;
      
      $warn_rec->{line_rev} = $line_rev;
      $warn_rec->{source}   = $source_text;

      $warnings_by_who{$who}{$file}{$line} = $warn_rec;

      $total_who_count++ unless exists $who_count{$who};
      $who_count{$who} += $warn_rec->{count};
    }
  }
}

sub print_warnings_as_html {
  my ($fh, $br) = @_;
  my ($buildname, $buildtime) = ($br->{buildname}, $br->{buildtime});

  my $time_str = print_time( $buildtime );

  # Change the default destination for print to $fh
  my $old_fh = select($fh);

  print <<"__END_HEADER";
  <html>
    <head>
      <title>Blamed Build Warnings</title>
    </head>
    <body>
      <font size="+2" face="Helvetica,Arial"><b>
        Blamed Build Warnings
      </b></font><br>
      <font size="+1" face="Helvetica,Arial">
        $buildname on $time_str<br>
        $total_warnings_count total warnings
      </font><p>
    
__END_HEADER

  for $who (sort { $who_count{$b} <=> $who_count{$a}
                   || $a cmp $b } keys %who_count) {
    push @who_list, $who;
  }
  # Summary Table (name, count)
  #
  print "<table border=0 cellpadding=1 cellspacing=0>";
  my $num_columns = 5;
  my $num_per_column = $#who_list / $num_columns;
  for (my $ii=0; $ii <= $num_per_column; $ii++) {
    print "<tr>";
    for (my $jj=0; $jj < $num_columns; $jj++) {
      my $who = $who_list[$ii+$jj*$num_per_column + 1];
      my $count = $who_count{$who};
      my $name = $who;
      $name =~ s/%.*//;
      print "<td><a href='#$name'>$name</a>";
      print "</td><td>";
      print "$count";
      print "</td><td>&nbsp;</td>";
    }
    print "</tr>";
  }
  print "</table><p>";

  # Print all the warnings
  #
  for $who (@who_list) {
    my $count = $who_count{$who};
    my ($name, $email);
    ($name = $who) =~ s/%.*//;
    ($email = $who) =~ s/%/@/;
    
    print "<font size='+1' face='Helvetica,Arial'><b>";
    print "<a name='$name' href='mailto:$email'>$name</a>";
    print " (1 warning)"       if $count == 1;
    print " ($count warnings)" if $count > 1;
    print "</b></font>";

    print "\n<ol>\n";
    for $file (sort keys %{$warnings_by_who{$who}}) {
      for $linenum (sort keys %{$warnings_by_who{$who}{$file}}) {
        my $warn_rec = $warnings_by_who{$who}{$file}{$linenum};

        print_warning($tree, $br, $file, $linenum, $warn_rec);
        print_source_code($linenum, $warn_rec);
      }
    }
    print "</ol>\n"
  }

  # Unblamed warnings
  #
  my $total_unblamed_warnigns=0;
  for my $file (keys %unblamed) {
    for my $linenum (keys %{$warnings{$file}}) {
      $total_unblamed_warnings++;
    }
  }
  print "<font size='+1' face='Helvetica,Arial'><b>";
  print "Unblamed ($total_unblamed_warnings warnings)";
  print "</b></font>";
  print "<ul>";
  for my $file (sort keys %unblamed) {
    for my $linenum (sort keys %{$warnings{$file}}) {
      my $warn_rec = $warnings{$file}{$linenum};
      print_warning($tree, $br, $file, $linenum, $warn_rec);
    }
  }
  print "</ul>";

  print <<"__END_FOOTER";
  <p>
    <hr align=left>
    Send questions or comments to 
    &lt;<a href="mailto:slamm\@netscape.com?subject=About the Blamed Build Warnings">slamm\@netcape.com</a>&gt;.
   </body></html>
__END_FOOTER

  # Change default destination back.
  select($old_fh);
}

sub print_warning {
  my ($tree, $br, $file, $linenum, $warn_rec) = @_;

  my $warning = $warn_rec->{warning_text};
  print "<li>";

  # File link
  if ($file =~ /\[multiple\]/) {
    $file =~ s/.\[multiple\]//;
    print   "<a target='_other' href='http://lxr.mozilla.org/seamonkey/find?string=$file'>";
    print   "$file:$linenum";
    print "</a> (multiple file matches)";
  } elsif ($file =~ /\[no_match\]/) {
    $file =~ s/.\[no_match\]//;
    print   "$file:$linenum (no file match)";
  } else {
    print "<a target='_other' href='"
      .file_url($file,$linenum)."'>";
    print   "$file:$linenum";
    print "</a> ";
  }
  print "<br>";
  # Warning text
  print "\u$warning";
  # Build log link
  my $log_line = $warn_rec->{first_seen_line};
  print " (<a href='"
    .build_url($tree, $br, $log_line)
      ."'target='_other'>";
  if ($warn_rec->{count} == 1) {
    print "See build log";
  } else {
    print "See 1st of $warn_rec->{count} occurrences in build log";
  }
  print "</a>)<br>";
}

sub print_source_code {
  my ($linenum, $warn_rec) = @_;
  my $warning = $warn_rec->{warning_text};

  # Source code fragment
  #
  my ($keyword) = $warning =~ /\`([^\']*)\'/;
  print "<table cellpadding=4><tr><td bgcolor='#ededed'>";
  print "<pre><font size='-1'>";
  
  my $source_text = $warn_rec->{source};
  my @source_lines = split /\n/, $source_text;
  my $line_index = $linenum - 3;
  for $line (@source_lines) {
    $line =~ s/&/&amp;/g;
    $line =~ s/</&lt;/g;
    $line =~ s/>/&gt;/g;
    $line =~ s|\Q$keyword\E|<b>$keyword</b>|g;
    print "<font color='red'>" if $line_index == $linenum;
    print "$line_index $line<BR>";
    print "</font>" if $line_index == $linenum;
    $line_index++;
  }
  print "</font>"; #</pre>";
  print "</td></tr></table>\n";
}

sub build_url {
  my ($tree, $br, $linenum) = @_;

  my $name = $br->{buildname};
  $name =~ s/ /%20/g;

  return "http://tinderbox.mozilla.org/showlog.cgi?tree=$tree"
        ."&logfile=$br->{logfile}"
        ."&errorparser=$br->{errorparser}"
        ."&buildname=$name"
        ."&buildtime=$br->{buildtime}"
        ."&line=$linenum"
        ."&numlines=50";
}

sub file_url {
  my ($file, $linenum) = @_;

  return "http://cvs-mirror.mozilla.org/webtools/bonsai/cvsblame.cgi"
        ."?file=mozilla/$file&mark=$linenum#".($linenum-10);

}
