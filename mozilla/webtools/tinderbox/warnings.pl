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

#
# Usage:
#
# Run this script on lounge.mozilla.org.  To run it there,
# cd to /opt/webtools/tinderbox and run it without
# any arguments:  
#
#   ./warnings.pl [--debug]
#

use FileHandle;

# Terrible globals:
#   %warnings
#   %warnings_by_who
#   %who_count
#   $ignore_pat
#   @ignore_match_pat
#

# This is for gunzip (should add a configure script to handle this).
$ENV{PATH} .= ":/usr/local/bin";

$debug = 1 if $ARGV[0] eq '--debug';

if ($debug) {
  foreach my $key (sort keys %ENV) {
    warn "debug> $key=$ENV{$key}\n";
  }
}

$tree = 'SeaMonkey';
# tinderbox/tbglobals.pl uses many shameful globals
$form{tree} = $tree;
require 'tbglobals.pl';

$cvsroot = '/cvsroot';
$source_root_pat = '^.*/mozilla/';

@ignore = ( 
  '__cmsg_data',
  'location of the previous definition',
  '\' was hidden',
  'declaration of \`index\' shadows global',
  'aggregate has a partly bracketed initializer', # mailnews guys say this has to stay
  'declaration of \`ws\' shadows global', # from istream
  'declaration of \`y0\' shadows global', # from mathcalls.h
  'declaration of \`y1\' shadows global', # from mathcalls.h
  'by \`nsHTML(?:Anchor|[^:]*Element)::(?:Set|Get)Attribute', # kipp says this is bogus
  'is not (any longer) pertinent', # cvs warning we can safely ignore
);
@ignore_match = (
  { warning=>'statement with no effect', source=>'(?:JS_|PR_)?ASSERT'},
);
$ignore_pat       = "(?:".join('|',@ignore).")";

print STDERR "Building hash of file names...";
($file_bases, $file_fullpaths) = build_file_hash($cvsroot, $tree);
print STDERR "done.\n";

for $br (last_successful_builds($tree)) {
  next unless $br->{buildname} =~ /shrike.*\b(Clobber|Clbr)\b/;

  my $log_file = "$br->{logfile}";

  warn "Parsing build log, $log_file\n";

  $fh = new FileHandle "gunzip -c $tree/$log_file |" 
    or die "Unable to open $tree/$log_file\n";
  &gcc_parser($fh, $cvsroot, $tree, $log_file, $file_bases, $file_fullpaths);
  $fh->close;

  &build_blame;

  my $warn_file = "$tree/warn$log_file";
  $warn_file =~ s/\.gz$/.html/;
  my $warn_file_by_file = $warn_file;
  $warn_file_by_file =~ s/\.html$/-by-file.html/;

  $fh->open(">$warn_file") or die "Unable to open $warn_file: $!\n";
  my $time_str = print_html_by_who($fh, $br);
  $fh->close;

#  $fh->open(">$warn_file_by_file")
#    or die "Unable to open $warn_file_by_file: $!\n";
#  print_html_by_file($fh, $br);
#  $fh->close;
  
  my $total_unignored_warnings = $total_warnings_count - $total_ignored_count;
  next unless $total_unignored_warnings > 0;

  # Make it live
  use File::Copy 'move';
  move($warn_file, "$tree/warnings.html");

  my $warn_summary = "$tree/warn$log_file";
  $warn_summary =~ s/.gz$/.pl/;

  $fh->open(">$warn_summary") or die "Unable to open $warn_summary: $!\n";
  $total_unignored_warnings = commify($total_unignored_warnings);
  print $fh '$warning_summary=\'<p>Check out the '
      ."<a href=\"http://tinderbox.mozilla.org/$tree/warnings.html\">"
      ."$total_unignored_warnings Build Warnings</a> (updated $time_str). "
      .'-<a href="mailto:slamm@netscape.com?subject=About the Build Warnings">'
      .'slamm</a><p>\';'."\n";
  $fh->close;

  move($warn_summary, "$tree/warn.pl");

  warn "$total_unignored_warnings warnings ($total_ignored_count ignored),"
      ." updated $time_str\n";

  last;
}

sub commify {
    my $text = reverse $_[0];
    $text =~ s/(\d\d\d)(?=\d)(?!\d*\.)/$1,/g;
    return scalar reverse $text;
}

# end of main
# ===================================================================

sub build_file_hash {
  my ($cvsroot, $tree) = @_;

  read_cvs_modules_file();
  @include_list = ();
  @exclude_list = ();
  expand_cvs_modules('SeaMonkeyAll', \@include_list, \@exclude_list);

  local $exclude_pat = join '|', @exclude_list;

  use File::Find;
  for my $include (@include_list) {
    $include .= ",v" unless -d "$cvsroot/$include";
    &find(\&find_cvs_files, "$cvsroot/$include"); 
  }
  return \%bases, \%fullpath;
}

sub read_cvs_modules_file
{
  local $_;
  open MODULES, "$cvsroot/CVSROOT/modules" 
    or die "Unable to open modules file: $cvsroot/CVSROOT/modules\n";
  while (<MODULES>) {
    if (/ -a /) {
      while (/\\$/) {
        chomp;
        chop;
        $_ .= <MODULES>;
      }
      chomp;
      my ($module_name, $list) = split /\s+-a\s+/, $_, 2;
      $modules{$module_name} = [ split /\s+/, $list ];
} } }

sub expand_cvs_modules {
  my ($module_name, $include_list, $exclude_list) = @_;
  warn "no module named $module_name\n" unless defined $modules{$module_name};
  for my $member (@{$modules{$module_name}}) {
    next if $member eq '';
    if (defined $modules{$member}) {
      expand_cvs_modules($member, $include_list, $exclude_list);
    } else {
      if ($member =~ /^!/) {
        push @$exclude_list, substr $member, 1;
      } else {
        push @$include_list, $member;
} } } }

sub find_cvs_files {
  $File::Find::prune = 1 if /.OBJ$/ or /^CVS$/ or /^Attic$/;
  if (-d $_) {
    $File::Find::prune = 1 if /$exclude_pat/o or $seen{$File::Find::name};
    $seen{$File::Find::name} = 1;
    return;
  }
  my $dir = $File::Find::dir;
  $dir =~ s|^$cvsroot/||o;
  $dir =~ s|/$||;
  my $file = substr $_, 0, -2;

  if (defined $module_files{$file}) {
    $bases{$file} = '[multiple]';
  } else {
    $bases{$file} = $dir;
  }
  $fullpath{"$dir/$file"} = 1;
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
  my $build_dir = '';

 PARSE_TOP: while (<$fh>) {
    # Directory
    #
    if (/^gmake\[\d\]: Entering directory \`(.*)\'$/) {
      $build_dir = $1;
      $build_dir =~ s|$source_root_pat||o;
    }
    
    # Now only match lines with "warning:"
    next unless /warning:/;

    chomp; # Yum, yum

    warn "debug> $_\n" if $debug;

    my ($filename, $line, $warning_text);
    ($filename, $line, undef, $warning_text) = split /:\s*/, $_, 4;
    $filename =~ s/.*\///;
    
    # Special case for Makefiles
    $filename =~ s/Makefile$/Makefile.in/;

    # Look up the file name to determine the directory
    my $dir = '';
    if ($file_fullnames->{"$build_dir/$filename"}) {
      $dir = $build_dir;
    } else {
      unless(defined($dir = $file_bases->{$filename})) {
        $dir = '[no_match]';
      }
    }
    my $file = "$dir/$filename";

    # Special case for "`foo' was hidden\nby `foo2'"
    $warning_text = "...was hidden $warning_text" if $warning_text =~ /^by \`/;

    # Remember line of first occurrence in the build log
    $warnings{$file}{$line}->{first_seen_line} = $.
      unless defined $warnings{$file}{$line};

    my $ignore_it = /$ignore_pat/o;
    if ($ignore_it) {
      $warnings{$file}{$line}->{ignorecount}++;
      $total_ignored_count++;
    }

    $warnings{$file}{$line}->{count}++;
    $total_warnings_count++;

    # Do not re-add this warning if it has been seen before
    for my $rec (@{ $warnings{$file}{$line}->{list} }) {
      next PARSE_TOP if $rec->{warning_text} eq $warning_text;
    }

    # Remember where in the build log the warning occured
    push @{ $warnings{$file}{$line}->{list} }, {
         log_file        => $log_file,
         warning_text    => $warning_text,
         ignore          => $ignore_it,
    };
  }
  warn "debug> $. lines read\n" if $debug;
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
} } }

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
    LINE: while (my ($line, $line_rec) = each %{$lines_hash}) {
      my $line_rev = $revision_map[$line-1];
      my $who = $revision_author{$line_rev};
      my $source_text = join '', @text[$line-3..$line+1];
      $source_text =~ s/\t/    /g;
      
      $who = "$who%netscape.com" unless $who =~ /[%]/;

      $line_rec->{line_rev} = $line_rev;
      $line_rec->{source}   = $source_text;

      for $ignore_rec (@ignore_match) {
        for my $warn_rec (@{ $line_rec->{list}}) {
          if ($warn_rec->{warning_text} =~ /$ignore_rec->{warning}/
              and $source_text =~ /$ignore_rec->{source}/
             and not $warn_rec->{ignore}) {
            $warn_rec->{ignore} = 1;
            $line_rec->{ignorecount}++;
            next LINE;
          }
        }
      }

      $warnings_by_who{$who}{$file}{$line} = $line_rec;

      $who_count{$who} += $line_rec->{count} - $line_rec->{ignorecount};
    }
  }
}

sub print_summary_table
{
  my ($who_list_ref, $who_count_hash_ref) = @_;
  my $num_whos = $#{$who_list_ref};

  # Summary Table (name, count)
  #
  use POSIX;
  print "<table border=0 cellpadding=1 cellspacing=0 bgcolor=white>\n";
  my $num_columns = 3;
  my $num_rows = ceil($num_whos / $num_columns);
  for (my $ii=0; $ii < $num_rows; $ii++) {
    print "<tr>";
    for (my $jj=0; $jj < $num_columns; $jj++) {
      my $index = $ii + $jj * $num_rows;
      next if $index > $num_whos;
      my $name = $who_list_ref->[$index];
      my $count = $who_count_hash_ref->{$name};
      next if $count == 0;
      #warn "$ii\t$jj\t$index\t$name\t$count\n";
      $name =~ s/%.*//;
      print "  " x $jj;
      print "<td><a href='#$name'>$name</a>";
      print "</td><td>";
      print "$count";
      print "</td><td>&nbsp;&nbsp;&nbsp;</td>\n";
    }
    print "</tr>\n";
  }
  print "</table><p>\n";
}

sub print_html_by_who {
  my ($fh, $br) = @_;
  my ($buildname, $buildtime) = ($br->{buildname}, $br->{buildtime});

  my $time_str = print_time( $buildtime );

  # Change the default destination for print to $fh
  my $old_fh = select($fh);

  my $total_unignored_count = $total_warnings_count - $total_ignored_count;
  for $who (sort { $who_count{$b} <=> $who_count{$a}
                   || $a cmp $b } keys %who_count) {
    next if $who_count{$who} == 0;
    push @who_list, $who;
  }
  print <<"__END_HEADER";
  <html>
    <head>
      <title>Blamed Build Warnings</title>
    </head>
    <body BGCOLOR="#FFFFFF" TEXT="#000000" 
          LINK="#0000EE" VLINK="#551A8B" ALINK="#FF0000">
      <font size="+2" face="Helvetica,Arial"><b>
        Blamed Build Warnings
      </b></font><br>
      <font size="+1" face="Helvetica,Arial">
        $buildname on $time_str<br>
        $total_unignored_count total warnings
      </font><p>
    
      <table cellspacing=0 cellpadding=0 border=0><tr bgcolor="#F0A000"><td>
      <table cellpadding=2 cellspacing=2 border=0><tr bgcolor="#FFFFFF">
        <th colspan=2><font size="+1" face="Helvetica,Arial">
          Summary</font></th></tr>
      <tr bgcolor="#FFFFFF"><td><font face="Helvetica,Arial"><b>
      by count</b></font>
__END_HEADER
  print_summary_table(\@who_list, \%who_count);
  print "</td><td><font face='Helvetica,Arial'><b>";
  print "by name</b></font>";
  print_summary_table([sort @who_list], \%who_count);
  print "</td></tr></table>";
  print "</td></tr></table>";

  # Count Unblamed warnings
  #
  for my $file (keys %unblamed) {
    for my $linenum (keys %{$warnings{$file}}) {
      $who_count{Unblamed} += $warnings{$file}{$linenum}{count};
      $who_count{Unblamed} -= $warnings{$file}{$linenum}{ignorecount};
      $warnings_by_who{Unblamed}{$file}{$linenum} = $warnings{$file}{$linenum};
    }
  }

  # Print all the warnings
  #
  for $who (@who_list, "Unblamed") {
    my $total_count = $who_count{$who};
    
    next if $total_count == 0;

    my ($name, $email);
    ($name = $who) =~ s/%.*//;
    ($email = $who) =~ s/%/@/;
    
    print "<h2>";
    print "<a name='$name' href='mailto:$email'>" unless $name eq 'Unblamed';
    print "$name";
    print "</a>" unless $name eq 'Unblamed';
    print " (1 warning)"       if $total_count == 1;
    print " ($total_count warnings)" if $total_count > 1;
    print "</h2>";

    print "\n<table>\n";
    my $count = 1;
    for $file (sort keys %{$warnings_by_who{$who}}) {
      for $linenum (sort keys %{$warnings_by_who{$who}{$file}}) {
        my $line_rec = $warnings_by_who{$who}{$file}{$linenum};
        my $count_for_line = $line_rec->{count} - $line_rec->{ignorecount};
        next if $count_for_line == 0;
        print_count($count, $count_for_line);
        print_warning($tree, $br, $file, $linenum, $line_rec);
        print_source_code($linenum, $line_rec) unless $unblamed{$file};
        $count += $count_for_line;
      }
    }
    print "</table>\n";
  }

  print <<"__END_FOOTER";
  <p>
    <hr align=left>
    Send questions or comments to 
    &lt;<a href="mailto:slamm\@netscape.com?subject=About the Blamed Build Warnings">slamm\@netcape.com</a>&gt;.
   </body></html>
__END_FOOTER

  # Change default destination back.
  select($old_fh);

  return $time_str;
}

sub print_html_by_file {
  my ($fh, $br) = @_;
  my ($buildname, $buildtime) = ($br->{buildname}, $br->{buildtime});

  my $time_str = print_time( $buildtime );

  # Change the default destination for print to $fh
  my $old_fh = select($fh);

  my $total_unignored_count = $total_warnings_count - $total_ignored_count;
  print <<"__END_HEADER";
  <html>
    <head>
      <title>Build Warnings By File</title>
    </head>
    <body>
      <font size="+2" face="Helvetica,Arial"><b>
        Build Warnings By File
      </b></font><br>
      <font size="+1" face="Helvetica,Arial">
        $buildname on $time_str<br>
        $total_unignored_count total warnings
      </font><p>
    
__END_HEADER

  # Print all the warnings
  #
  for $who (@who_list, "Unblamed") {
    my $total_count = $who_count{$who};
    my ($name, $email);
    ($name = $who) =~ s/%.*//;
    ($email = $who) =~ s/%/@/;
    
    print "<h2>";
    print "<a name='$name' href='mailto:$email'>" unless $name eq 'Unblamed';
    print "$name";
    print "</a>" unless $name eq 'Unblamed';
    print " (1 warning)"       if $total_count == 1;
    print " ($total_count warnings)" if $total_count > 1;
    print "</h2>";

    print "\n<table>\n";
    my $count = 1;
    for $file (sort keys %{$warnings_by_who{$who}}) {
      for $linenum (sort keys %{$warnings_by_who{$who}{$file}}) {
        my $line_rec = $warnings_by_who{$who}{$file}{$linenum};
        my $count_for_line = $line_rec->{count} - $line_rec->{ignorecount};
        next if $count_for_line == 0;
        print_count($count, $count_for_line);
        print_warning($tree, $br, $file, $linenum, $line_rec);
        print_source_code($linenum, $line_rec) unless $unblamed{$file};
        $count += $count_for_line;
      }
    }
    print "</table>\n";
  }

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

sub print_count {
  my ($start, $count) = @_;
  print "<tr><td align=right>$start";
  print "-".($start+$count-1) if $count > 1;
  print ".</td>";
}

sub print_warning {
  my ($tree, $br, $file, $linenum, $line_rec) = @_;

  print "<td>";

  # File link
  if ($file =~ /\[multiple\]/) {
    $file =~ s/\[multiple\]\///;
    print   "<a href='http://lxr.mozilla.org/seamonkey/find?string=$file'>";
    print   "$file:$linenum";
    print "</a> (multiple file matches)";
  } elsif ($file =~ /\[no_match\]/) {
    $file =~ s/\[no_match\]\///;
    print   "<b>$file:$linenum</b> (no file match)";
  } else {
    print "<a href='"
      .file_url($file,$linenum)."'>";
    print   "$file:$linenum";
    print "</a> ";
  }

  # Build log link
  my $log_line = $line_rec->{first_seen_line};
  print " (<a href='"
        .build_url($tree, $br, $log_line)
        ."'>";
  my $count = $line_rec->{count} - $line_rec->{ignorecount};
  if ($count == 1) {
    print "See build log excerpt";
  } else {
    print "See 1st of $count warnings in build log";
  }
  print "</a>)";

  print "</td></tr><tr><td></td><td>";

  for my $warn_rec (@{ $line_rec->{list}}) {
    next if $warn_rec->{ignore};
    my $warning  = $warn_rec->{warning_text};

    # Warning text
    print "\u$warning<br>";
  }
  print "</td></tr>";
}

sub print_source_code {
  my ($linenum, $line_rec) = @_;

  # Source code fragment
  #
  print "<tr><td></td><td bgcolor=#ededed>";
  print "<pre>";
  
  my $source_text = trim_common_leading_whitespace($line_rec->{source});
  $source_text =~ s/&/&amp;/gm;
  $source_text =~ s/</&lt;/gm;
  $source_text =~ s/>/&gt;/gm;
  # Highlight the warning's keyword
  for my $warn_rec (@{ $line_rec->{list}}) {
    my $warning = $warn_rec->{warning_text};
    my ($keyword) = $warning =~ /\`([^\']*)\'/;
    next if $keyword eq '';
    $source_text =~ s|\b\Q$keyword\E\b|<b>$keyword</b>|gm;
    last;
  }
  my $line_index = $linenum - 2;
  $source_text =~ s/^(.*)$/$line_index++." $1"/gme;
  $source_text =~ s|^($linenum.*)$|<font color='red'>\1</font>|gm;
  chomp $source_text;
  print $source_text;

  #print "</pre>";
  print "</td></tr>\n";
}

sub build_url {
  my ($tree, $br, $linenum) = @_;

  my $name = $br->{buildname};
  $name =~ s/ /%20/g;

  return "../showlog.cgi?log=$tree/$br->{logfile}:$linenum";
}

sub file_url {
  my ($file, $linenum) = @_;

  return "http://cvs-mirror.mozilla.org/webtools/bonsai/cvsblame.cgi"
        ."?file=$file&mark=$linenum#".($linenum-10);

}

sub trim_common_leading_whitespace {
  # Adapted from dequote() in Perl Cookbook by Christiansen and Torkington
  local $_ = shift;
  my $white;  # common whitespace
  if (/(?:(\s*).*\n)(?:(?:\1.*\n)|(?:\s*\n))+$/) {
    $white = $1;
  } else {
    $white = /^(\s+)/;
  }
  s/^(?:$white)?//gm;
  s/^\s+$//gm;
  return $_;
}

