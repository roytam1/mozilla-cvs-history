#!/usr/bonsaitools/bin/perl --
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Tinderbox build tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 

use lib '../bonsai';
require 'tbglobals.pl';
require 'lloydcgi.pl';
require 'imagelog.pl';
require 'header.pl';

# Hack this until I can figure out how to do get default root. -slamm
$default_root = '/cvsroot';

# Show 12 hours by default
#
$nowdate = time; 
if (not defined($maxdate = $form{maxdate})) {
  $maxdate = $nowdate;
}
if ($form{showall}) {
  $mindate = 0;
}
else {
  $default_hours = 12;
  $hours = $default_hours;
  $hours = $form{hours} if $form{hours};
  $mindate = $maxdate - ($hours*60*60);
}

%colormap = (
  success    => '11DD11',
  busted     => 'EE0000',
  building   => 'EEFF00',
  testfailed => 'FFAA00'
);

%images = (
  flames    => '1afi003r.gif',
  star      => 'star.gif'
);

$::tree = $form{tree};

# $rel_path is the relative path to webtools/tinderbox used for links.
# It changes to "../" if the page is generated statically, because then
# it is placed in tinderbox/$::tree.
$rel_path = ''; 

&show_tree_selector,  exit if $::tree eq '';
&do_quickparse,       exit if $form{quickparse};
&do_express,          exit if $form{express};
&do_rdf,              exit if $form{rdf};
&do_static,           exit if $form{static};
&do_flash,            exit if $form{flash};
&do_panel,            exit if $form{panel};
&do_hdml,             exit if $form{hdml};
&do_tinderbox,        exit;

# end of main
#=====================================================================

sub make_tree_list {
  my @result;
  while(<*>) {
    if( -d $_ && $_ ne 'data' && $_ ne 'CVS' && -f "$_/treedata.pl") {
      push @result, $_;
    }
  }
  return @result;
}

sub show_tree_selector {

  print "Content-type: text/html\n\n";

  EmitHtmlHeader("tinderbox");

  print "<P><TABLE WIDTH=\"100%\">";
  print "<TR><TD ALIGN=CENTER>Select one of the following trees:</TD></TR>";
  print "<TR><TD ALIGN=CENTER>\n";
  print " <TABLE><TR><TD><UL>\n";
  
  my @list = make_tree_list();

  foreach (@list) {
    print "<LI><a href=showbuilds.cgi?tree=$_>$_</a>\n";
  }
  print "<//UL></TD></TR></TABLE></TD></TR></TABLE>";
  
  print "<P><TABLE WIDTH=\"100%\">";
  print "<TR><TD ALIGN=CENTER><a href=admintree.cgi>";
  print "Administer</a> one of the following trees:</TD></TR>";
  print "<TR><TD ALIGN=CENTER>\n";
  print " <TABLE><TR><TD><UL>\n";
  
  foreach (@list) {
    print "<LI><a href=admintree.cgi?tree=$_>$_</a>\n";
  }
  print "<//UL></TD></TR></TABLE></TD></TR></TABLE>";
}

sub do_static {
  local *OUT;

  $form{legend}=0;

  my @pages = ( ['index.html', 'do_tinderbox'],
                ['flash.rdf',  'do_flash'],
                ['panel.html', 'do_panel'],
                ['stats.hdml', 'do_hdml'] );
  
  $rel_path = '../';
  while (($key, $value) = each %images) {
    $images{$key} = "$rel_path$value";
  }

  my $oldfh = select;

  foreach $pair (@pages) {
    my ($page, $call) = @{$pair};
    my $outfile = "$::tree/$page";

    open(OUT,">$outfile.$$");
    select OUT;

    eval "$call";

    close(OUT);
    system "mv $outfile.$$ $outfile";
  }
  select $oldfh;
}

sub do_tinderbox {
  my $tinderbox_data = &tb_load_data;
  &print_page_head;
  &print_table_header;
  &print_table_body($tinderbox_data);
  &print_table_footer;
}

sub print_page_head {
  print "Content-type: text/html\n\n<HTML>\n" unless $form{static};

  use POSIX qw(strftime);
  # Print time in format, "HH:MM timezone"
  my $now = strftime("%H:%M %Z", localtime);

  EmitHtmlTitleAndHeader("tinderbox: $::tree", "tinderbox",
                         "tree: $::tree ($now)");

  &print_javascript;

  # Print rules, sheriff, and status.  Only on the first pageful.
  if ($nowdate eq $maxdate) {
     do "$::tree/rules.pl";
     print "<a NAME=\"rules\"></a>$rules_message<br>";  # from $::tree/rules.pl
     
     do "$::tree/sheriff.pl";
     print "<a NAME=\"sheriff\"></a>$current_sheriff<br>";  # from $::tree/sheriff.pl

     do "$::tree/status.pl";
     print "<a NAME=\"status\"></a>$status_message<br>";  # from $::tree/status.pl
  }

  # Quote and Lengend
  #
  if ($form{legend}) {
    my ($imageurl,$imagewidth,$imageheight,$quote) = &get_image;
    print qq{
      <table width="100%" cellpadding=0 cellspacing=0>
        <tr>
          <td valign=bottom>
            <p><center><a href=addimage.cgi><img src="$rel_path$imageurl"
              width=$imagewidth height=$imageheight><br>
              $quote</a><br>
            </center>
            <p>
          <td align=right valign=bottom>
            <table cellspacing=0 cellpadding=1 border=0>
              <tr>
                <td align=center><TT>L</TT></td>
                <td>= Show Build Log</td>
              </tr>
              <tr>
                <td align=center><TT>C</TT></td>
                <td>= Show Checkins</td>
              </tr>
              <tr>
                <td align=center><TT>D</TT></td>
                <td>= Download Build</td>
              </tr>
              <tr>
              <td align=center>
              <img src="$images{star}"></td><td>= Show Log comments
            </td></tr><tr><td colspan=2>
              <table cellspacing=1 cellpadding=1 border=1>
                <tr bgcolor="$colormap{success}"><td>Successful Build, optional bloaty stats:<br>
                  <tt>Lk:XXX</tt> (bytes leaked)<br><tt>Bl:YYYY</tt> (bytes allocated, bloat)</td>
                <tr bgcolor="$colormap{building}"><td>Build in Progress</td>
                <tr bgcolor="$colormap{testfailed}"><td>Successful Build,
                                                          but Tests Failed</td>
                <tr bgcolor="$colormap{busted}"><td>Build Failed</td>
              </table>
            </td></tr></table>
          </td>
        </tr>
      </table>
    };
  }
  if (is_tree_state_available()) {
    print "<a NAME=\"open\"></a>";
    print "The tree is <font size=+2>";
    print (is_tree_open() ? 'open' : 'closed');
    print "</font>\n";
  }
}

sub print_table_body {
  my $tinderbox_data = $_[0];
  for (my $tt=0; $tt < $time_count; $tt++) {
    last if $build_time_times->[$tt] < $mindate;
    print_table_row($tinderbox_data, $tt);
  }
}

sub print_bloat_delta {
  my ($value, $compare) = @_;
  my $units = ' ';
  $value = $value || 0;
  $compare = $compare || 0;

  if ($value >= 1000000) {
    $value = int($value / 1000000);
    $min =   int($min / 1000000);
    $units = 'M';
  } elsif ($value >= 1000) {
    $value = int($value / 1000);
    $min =   int($min / 1000);
    $units = 'K';
  }

  # Took out colors because the numbers jump around too much. -slamm
  return "$value$units";
}

BEGIN {
  # Make $lasthour persistent private variable for print_table_row().
  my $lasthour = ''; 

  sub print_table_row {
    my ($td, $tt) = @_;

    # Time column
    # 
    my $query_link = '';
    my $end_query  = '';
    my $pretty_time = print_time($build_time_times->[$tt]);
    
    ($hour) = $pretty_time =~ /(\d\d):/;

    if ($lasthour ne $hour or has_who_list($tt)) {
      $query_link = query_ref($td, $build_time_times->[$tt]);
      $end_query  = '</a>';
    }
    if ($lasthour eq $hour) {
      $pretty_time =~ s/^.*&nbsp;//;
    } else {
      $lasthour = $hour;
    }
    
    my $hour_color = '';
    $hour_color = ' bgcolor=#e7e7e7'
      if ($build_time_times->[$tt] + 1) % 7200 <= 3600;
    print "<tr align=center><td align=right$hour_color>",
      "$query_link\n$pretty_time$end_query</td>\n";
    
    # Guilty
    #
    print '<td>';
    for $who (sort keys %{$who_list->[$tt]} ){
      my $qr = &who_menu($td, $build_time_times->[$tt],
                         $build_time_times->[$tt-1],$who);
      $who =~ s/%.*$//;
      print "  $qr$who</a>\n";
    }
    print '</td>';
    
    # Build Status
    #
    for (my $build_index=0; $build_index < $name_count; $build_index++) {
      if (not defined($br = $build_table->[$tt][$build_index])) {
        # No build data for this time (e.g. no build after this time).
        print "<td></td>\n";
        next;
      }
      next if $br == -1; # Covered by rowspan

      my $rowspan = $br->{rowspan};
      $rowspan = $mindate_time_count - $tt + 1
        if $tt + $rowspan - 1 > $mindate_time_count;
      print "<td rowspan=$rowspan bgcolor=$colormap{$br->{buildstatus}}>\n";
      
      my $logfile = $br->{logfile};
      my $buildtree = $br->{td}->{name};
      
      print "<tt>\n";
        
      # Build Note
      # 
      my $logurl = "${rel_path}showlog.cgi?log=$buildtree/$logfile";
      
      if ($br->{hasnote}) {
        print "<a href='$logurl' onclick=\"return ",
          "note(event,$br->{noteid},'$logfile');\">",
          "<img src='$images{star}' border=0></a>\n";
      }
        
      # Build Log
      # 
      # Uncomment this line to print logfile names in build rectangle.
      # print "$logfile<br>";
      
      print "<A HREF='$logurl'"
           ." onclick=\"return log(event,$build_index,'$logfile');\">"
           ."L</a>";
 
      # What Changed
      #
      # Only add the "C" link if there have been changes since the last build.
      if ($br->{previousbuildtime}) {
        my $previous_br = $build_table->[$tt+$rowspan][$build_index];
        my $previous_rowspan = $previous_br->{rowspan};
        if (&has_who_list($tt+$rowspan,
                          $tt+$rowspan+$previous_rowspan-1)) {
          print "\n", &query_ref($br->{td}, 
                                 $br->{previousbuildtime},
                                 $br->{buildtime} - 1);
          print "C</a>";
        }
      }

      # Binary URL
      #
      # Only add the "D" link if there is a url to a downloadable binary
      if( $br->{binaryurl} ){
          $binaryurl = $br->{binaryurl};
          print" <A HREF=$binaryurl>D</A>";
      }
      
      # Leak/Bloat
      if (defined $td->{bloaty}{$logfile}) {
        my ($leaks, $bloat, $leaks_cmp, $bloat_cmp)
            = @{ $td->{bloaty}{$logfile} };
        print "<br>Lk:", print_bloat_delta($leaks, $leaks_cmp),
              "<br>Bl:", print_bloat_delta($bloat, $bloat_cmp);
      }

      # Warnings
      if (defined $td->{warnings}{$logfile}) {
        my ($warning_count) = $td->{warnings}{$logfile};
        my $warn_file = "$::tree/warn$logfile";
        $warn_file =~ s/\.gz$/.html/;
        print "<br><br><a href='${rel_path}$warn_file'>Warn:$warning_count</a>";
      }

      print "</tt>\n</td>";
    }
    print "</tr>\n";
  }
}

sub print_table_header {
  print "<table border=1 bgcolor='#FFFFFF' cellspacing=1 cellpadding=1>\n";

  print "<tr align=center>\n";

  print "<TH>Build Time</TH>\n";
  print "<TH>Guilty</th>\n";

  for (my $ii=0; $ii < $name_count; $ii++) {

    my $bn = $build_names->[$ii];
    $bn =~ s/Clobber/Clbr/g;
    $bn =~ s/Depend/Dep/g;
    $bn = "<font face='Helvetica,Arial' size=-1>$bn</font>";

    my $last_status = tb_last_status($ii);
    if ($last_status eq 'busted') {
      unless ($form{legend}) {
        print "<td rowspan=2 bgcolor=$colormap{busted}>$bn</td>";
      } else {
        print "<td rowspan=2 bgcolor=000000 background='$images{flames}'>";
        print "<font color=white>$bn</font></td>";
      }
    }
    else {
      print "<td rowspan=2 bgcolor=$colormap{$last_status}>$bn</td>";
    }
  }
  print "</tr><tr>\n";

  print "<td rowspan=1><font size=-1>Click time to <br>see changes <br>",
        "since then</font></td>";
  print "<td><font size=-1>",
        "Click name to see what they did</font></td>";

  print "</tr>\n";
}

sub print_table_footer {
  print "</table>\n";

  my $nextdate = $maxdate - $hours*60*60;
  print &open_showbuilds_href(maxdate=>"$nextdate", legend=>'0')
       ."Show previous $hours hours</a><br>";
  if ($hours != 24) {
    my $save_hours = $hours;
    $hours = 24;
    print &open_showbuilds_href(maxdate=>"$nextdate", legend=>'0')
      ."Show previous 24 hours</a>";
    $hours = $save_hours;
  }
  print "<p><a href='${rel_path}admintree.cgi?tree=$::tree'>",
        "Administrate Tinderbox Trees</a><br>\n";
}

sub open_showbuilds_url {
  my %args = (
        legend => "$form{legend}",
        @_
  );

  my $url = "${rel_path}showbuilds.cgi?tree=$::tree";
  $url .= "&hours=$hours" if $hours ne $default_hours;
  while (my ($key, $value) = each %args) {
    $url .= "&$key=$value" if $value ne '';
  }
  return $url;
}

sub open_showbuilds_href {
  return "<a href=".open_showbuilds_url(@_).">";
}

sub query_ref {
  my ($td, $mindate, $maxdate, $who) = @_;
  my $output = '';

  $output = "<a href=${rel_path}../bonsai/cvsquery.cgi";
  $output .= "?module=$td->{cvs_module}";
  $output .= "&branch=$td->{cvs_branch}"   if $td->{cvs_branch} ne 'HEAD';
  $output .= "&cvsroot=$td->{cvs_root}"    if $td->{cvs_root} ne $default_root;
  $output .= "&date=explicit&mindate=$mindate";
  $output .= "&maxdate=$maxdate"           if $maxdate and $maxdate ne '';
  $output .= "&who=$who"                   if $who and $who ne '';
  $output .= ">";
}

sub who_menu {
  my ($td, $mindate, $maxdate, $who) = @_;
  my $treeflag;

  my $qr = "${rel_path}../registry/who.cgi?email=". url_encode($who)
      . "&d=$td->{cvs_module}|$td->{cvs_branch}|$td->{cvs_root}|$mindate|$maxdate";

  return "<a href='$qr' onclick=\"return who(event);\">";
}

# Check to see if anyone checked in during time slot.
#   ex.  has_who_list(1);    # Check for checkins in most recent time slot.
#   ex.  has_who_list(1,5);  # Check range of times.
sub has_who_list {
  my ($time1, $time2) = @_;

  if (not defined(@who_check_list)) {
    # Build a static array of true/false values for each time slot.
    $who_check_list[$time_count - 1] = 0;
    for (my $tt = 0; $tt < $time_count; $tt++) {
      $who_check_list[$tt] = 1 if each %{$who_list->[$tt]};
    }
  }
  if ($time2) {
    for (my $ii=$time1; $ii<=$time2; $ii++) {	 
      return 1 if $who_check_list[$ii]
    }
    return 0
  } else {
    return $who_check_list[$time1]; 
  }
}

BEGIN {
  # Check bonsai tree for open/close state

  my $treestate = undef;
  my $checked_state = 0;

  sub _check_tree_state {
    my $tree = shift;

    $checked_state = 1;
    tb_load_treedata($tree); # Loading for the global, $bonsai_tree
    return unless defined $bonsai_tree and $bonsai_tree ne '';

    local $_;
    $::BatchID='';
    eval qq(require "../bonsai/data/$bonsai_tree/batchid.pl");
    if ($::BatchID eq '') {
      warn "No BatchID in ../bonsai/data/$bonsai_tree/batchid.pl\n";
      return;
    }
    open(BATCH, "<../bonsai/data/$bonsai_tree/batch-$::BatchID.pl")
      or print "can't open batch-$::BatchID.pl<br>";
    while (<BATCH>) { 
      if (/^\$::TreeOpen = '(\d+)';/) {
        $treestate = $1;
        last;
      }
    }
    return;
  }

  sub is_tree_state_available {
    my $tree = shift;
    $tree = $::tree unless defined $tree;
    return 1 if defined $treestate;
    return 0 if $checked_state;
    _check_tree_state($tree);
    return is_tree_state_available();
  }

  sub is_tree_open {
    my $tree = shift;
    $tree = $::tree unless defined $tree;
    _check_tree_state($tree) unless $checked_state;
    return $treestate;
  }
}

sub print_javascript {
  my $script;
  ($script = <<"__ENDJS") =~ s/^    //gm;
    <style type="text/css">
    #popup {
      position: absolute;
      height: 10em;
      margin: -5em 0 0 -5em;
      }
    .who#popup{
      width: 20em;
      }
    .note#popup {
      width: 25em;
      height: 20em;
      }
    .log#popup {
      width: 15em;
      }
    .note#popup, .log#popup {
      border: 2px solid black;
      background: white;
      color: black;
      padding: 0.5em;
      }
    </style>
    <script>
    var noDHTML = false;
    if (parseInt(navigator.appVersion) < 4) {
      window.event = 0;
      noDHTML = true;
    } else if (navigator.userAgent.indexOf("MSIE") > 0 ) {
      noDHTML = true;
    }
    if (document.body && document.body.addEventListener) {
      document.body.addEventListener("click",maybeclosepopup,false);
    }
    function nodewrite(n,t) {
      var r = document.createRange();
      r.setStart(n,0);
      n.appendChild(r.createContextualFragment(t));
    }
    function closepopup() {
      var p = document.getElementById("popup");
      if (p && p.parentNode) {
        p.parentNode.removeChild(p);
      }
    }
    function maybeclosepopup(e) {
      var n = e.target;
      var close = true;
      while(close && n && (n != document)) {
        close = (n.id != "popup") && !(n.tagName && (n.tagName.toLowerCase() == "a"));
        n = n.parentNode;
      }
      if (close) closepopup();
    }
    function who(d) {
      if (noDHTML) {
        return true;
      }
      if (document.layers) {
        var l  = document.layers['popup'];
        l.src  = d.target.href;
        l.top  = d.target.y - 6;
        l.left = d.target.x - 6;
        if (l.left + l.clipWidth > window.width) {
          l.left = window.width - l.clipWidth;
        }
        l.visibility="show";
      } else {
        var t = d.target;
        while (t.nodeType != 1) {
          t = t.parentNode;
        }
        closepopup()
        var l = document.createElement("iframe");
        l.setAttribute("src", t.href);
        l.setAttribute("id", "popup");
        l.className = "who";
        t.appendChild(l);
      }
      return false;
    }
    function log_url(logfile) {
      return "${rel_path}showlog.cgi?log=" + buildtree + "/" + logfile;
    }
    function note(d,noteid,logfile) {
      if (noDHTML) {
        document.location = log_url(logfile);
        return false;
      }
      if (document.layers) {
        var l = document.layers['popup'];
        l.document.write("<table border=1 cellspacing=1><tr><td>"
                         + notes[noteid] + "</tr></table>");
        l.document.close();
        l.top = d.y-10;
        var zz = d.x;
        if (zz + l.clip.right > window.innerWidth) {
          zz = (window.innerWidth-30) - l.clip.right;
          if (zz < 0) { zz = 0; }
        }
        l.left = zz;
        l.visibility="show";
      } else {
        var t = d.target;
        while (t.nodeType != 1) {
          t = t.parentNode;
        }
        closepopup()
        var l = document.createElement("div");
        nodewrite(l,notes[noteid]);
        l.setAttribute("id", "popup");
        l.style.position = "absolute";
        l.className = "note";
        t.parentNode.parentNode.appendChild(l);
      }
      return false;
    }
    function log(e,buildindex,logfile)
    {
      var logurl = log_url(logfile);
      var commenturl = "${rel_path}addnote.cgi?log=" + buildtree + "/" + logfile;

      if (noDHTML) {
        document.location = logurl;
        return false;
      }
      if (document.layers) {
        var q = document.layers["logpopup"];
        q.top = e.target.y - 6;

        var yy = e.target.x;
        if ( yy + q.clip.right > window.innerWidth) {
          yy = (window.innerWidth-30) - q.clip.right;
          if (yy < 0) { yy = 0; }
        }
        q.left = yy;
        q.visibility="show"; 
        q.document.write("<TABLE BORDER=1><TR><TD><B>"
          + builds[buildindex] + "</B><BR>"
          + "<A HREF=" + logurl + ">View Brief Log</A><BR>"
          + "<A HREF=" + logurl + "&fulltext=1"+">View Full Log</A><BR>"
          + "<A HREF=" + commenturl + ">Add a Comment</A>"
          + "</TD></TR></TABLE>");
        q.document.close();
      } else {
        var t = e.target;
        while (t.nodeType != 1) {
          t = t.parentNode;
        }
        closepopup();
        var l = document.createElement("div");
        nodewrite(l,"<B>" + builds[buildindex] + "</B><BR>"
          + "<A HREF=" + logurl + ">View Brief Log</A><BR>"
          + "<A HREF=" + logurl + "&fulltext=1"+">View Full Log</A><BR>"
          + "<A HREF=" + commenturl + ">Add a Comment</A><BR>");
        l.setAttribute("id", "popup");
        l.className = "log";
        t.parentNode.appendChild(l);
      }
      return false;
    }

    notes = new Array();
    builds = new Array();

__ENDJS
  print $script;
  
  $ii = 0;
  while ($ii < @note_array) {
    $ss = $note_array[$ii];
    while ($ii < @note_array && $note_array[$ii] eq $ss) {
      print "notes[$ii] = ";
      $ii++;
    }
    $ss =~ s/\\/\\\\/g;
    $ss =~ s/\"/\\\"/g;
    $ss =~ s/\n/\\n/g;
    print "\"$ss\";\n";
  }
  for ($ii=0; $ii < $name_count; $ii++) {
    if (defined($br = $build_table->[0][$ii])) {
      my $bn = $build_names->[$ii];
      print "builds[$ii]='$bn';\n";
    }
  }
  print "buildtree = '$::tree';\n";

  # Use JavaScript to refresh the page every 15 minutes
  print "setTimeout('location.reload()',900000);\n" if $nowdate eq $maxdate;

  ($script = <<'__ENDJS') =~ s/^    //gm;
    </script>

    <layer name="popup" onMouseOut="this.visibility='hide';" 
           left=0 top=0 bgcolor="#ffffff" visibility="hide">
    </layer>

    <layer name="logpopup" onMouseOut="this.visibility='hide';"
           left=0 top=0 bgcolor="#ffffff" visibility="hide">
    </layer>
__ENDJS
  print $script;
}

sub do_express {
  print "Content-type: text/html\nRefresh: 900\n\n<HTML>\n";

  my (%build, %times);
  tb_loadquickparseinfo($::tree, \%build, \%times);

  my @keys = sort keys %build;
  my $keycount = @keys;
  my $tm = &print_time(time);
  print "<table border=1 cellpadding=1 cellspacing=1><tr>";
  print "<th align=left colspan=$keycount>";
  print &open_showbuilds_href."$::tree";
  if (is_tree_state_available()) {
    print (is_tree_open() ? ' is open' : ' is closed');
  }
  print ", $tm</a></tr><tr>\n";
  foreach my $buildname (@keys) {
    print "<td bgcolor='$colormap{$build{$buildname}}'>$buildname</td>";
  }
  print "</tr></table>\n";
}

# This is essentially do_express but it outputs a different format
sub do_panel {
  print "Content-type: text/html\n\n<HTML>\n" unless $form{static};

  my (%build, %times);
  tb_loadquickparseinfo($::tree, \%build, \%times);
  
  print q(
    <head>
      <META HTTP-EQUIV="Refresh" CONTENT="300">
      <style>
        body, td { 
          font-family: Verdana, Sans-Serif;
          font-size: 8pt;
        }
      </style>
    </head>
    <body BGCOLOR="#FFFFFF" TEXT="#000000" 
          LINK="#0000EE" VLINK="#551A8B" ALINK="#FF0000">
  );
  # Hack the panel link for now.
  print "<a target='_content' href='http://tinderbox.mozilla.org/$::tree/'>$::tree";
  
  if (is_tree_state_available()) {
    print " is ", is_tree_open() ? 'open' : 'closed';
  }
  # Add the current time
  my ($minute,$hour,$mday,$mon) = (localtime)[1..4];
  my $tm = sprintf("%d/%d&nbsp;%d:%02d",$mon+1,$mday,$hour,$minute);
  print ", $tm</a><br>";
  
  print "<table border=0 cellpadding=1 cellspacing=1>";
  while (my ($name, $status) = each %build) {
    print "<tr><td bgcolor='$colormap{$status}'>$name</td></tr>";
  }
  print "</table></body>";
}

sub do_flash {
  print "Content-type: text/rdf\n\n" unless $form{static};

  my (%build, %times);
  tb_loadquickparseinfo($::tree, \%build, \%times);

  my ($mac,$unix,$win) = (0,0,0);

  while (my ($name, $status) = each %build) {
    next if $status eq 'success';
    $mac = 1, next if $name =~ /Mac/;
    $win = 1, next if $name =~ /Win/;
    $unix = 1;
  }

  print q{
    <RDF:RDF xmlns:RDF='http://www.w3.org/1999/02/22-rdf-syntax-ns#' 
             xmlns:NC='http://home.netscape.com/NC-rdf#'>
    <RDF:Description about='NC:FlashRoot'>
  };

  my $busted = $mac + $unix + $win;
  if ($busted) {

    # Construct a legible sentence; e.g., "Mac, Unix, and Windows
    # are busted", "Windows is busted", etc. This is hideous. If
    # you can think of something better, please fix it.

    my $text;
    if ($mac) {
      $text .= 'Mac' . ($busted > 2 ? ', ' : ($busted > 1 ? ' and ' : ''));
    }
    if ($unix) {
      $text .= 'Unix' . ($busted > 2 ? ', and ' : ($win ? ' and ' : ''));
    }
    if ($win) {
      $text .= 'Windows';
    }
    $text .= ($busted > 1 ? ' are ' : ' is ') . 'busted';
    
    # The Flash spec says we need to give ctime.
    use POSIX;
    my $tm = POSIX::ctime(time());
    $tm =~ s/^...\s//;   # Strip day of week
    $tm =~ s/:\d\d\s/ /; # Strip seconds
    chop $tm;
  
    print qq{
      <NC:child>
        <RDF:Description ID='flash'>
          <NC:type resource='http://www.mozilla.org/RDF#TinderboxFlash' />
          <NC:source>$::tree</NC:source>
          <NC:description>$text</NC:description>
          <NC:timestamp>$tm</NC:timestamp>
        </RDF:Description>
      </NC:child>
    };
  }
  print q{
    </RDF:Description>
    </RDF:RDF>
  };
}

sub do_quickparse {
  print "Content-type: text/plain\n\n";

  my @treelist = split /,/, $::tree;
  foreach my $tt (@treelist) {
    if (is_tree_state_available($tt)) {
      my $state = is_tree_open($tt) ? 'open' : 'closed';
      print "State|$tt|$bonsai_tree|$state\n";
    }
    my (%build, %times);
    tb_loadquickparseinfo($tt, \%build, \%times);
    
    foreach my $buildname (sort keys %build) {
      print "Build|$tt|$buildname|$build{$buildname}\n";
    }
  }
}

sub do_rdf {
  print "Content-type: text/plain\n\n";

  my $mainurl = "http://$ENV{SERVER_NAME}$ENV{SCRIPT_NAME}?tree=$::tree";
  my $dirurl = $mainurl;

  $dirurl =~ s@/[^/]*$@@;

  my (%build, %times);
  tb_loadquickparseinfo($::tree, \%build, \%times);

  my $image = "channelok.gif";
  my $imagetitle = "OK";
  foreach my $buildname (sort keys %build) {
    if ($build{$buildname} eq 'busted') {
      $image = "channelflames.gif";
      $imagetitle = "Bad";
      last;
    }
  }
  print qq{<?xml version="1.0"?>
    <rdf:RDF 
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns="http://my.netscape.com/rdf/simple/0.9/">
    <channel>
      <title>Tinderbox - $::tree</title>
      <description>Build bustages for $::tree</description>
      <link>$mainurl</link>
    </channel>
    <image>
      <title>$imagetitle</title>
      <url>$dirurl/$image</url>
      <link>$mainurl</link>
    </image>
  };    
    
  if (is_tree_state_available()) {
    my $state = is_tree_open() ? 'open' : 'closed';
    print "<item><title>The tree is currently $state</title>",
          "<link>$mainurl</link></item>\n";
  }

  foreach my $buildname (sort keys %build) {
    if ($build{$buildname} eq 'busted') {
      print "<item><title>$buildname is in flames</title>",
            "<link>$mainurl</link></item>\n";
    }
  }
  print "</rdf:RDF>\n";
}

# This is for Sprint phones
sub do_hdml {
  print "Content-type: text/hdml\n\n" unless $form{static};

  print q{<hdml public=true version=2.0 ttl=0>
    <display title=Tinderbox>
      <action type=help task=go dest=#help>
  };
  %state_symbols = (success=>'+',busted=>'!',testfailed=>'~');

  if (is_tree_state_available()) {
    print "<LINE>$::tree is " . (is_tree_open() ? 'open' : 'closed');
  }
  my (%build, %times);
  tb_loadquickparseinfo($::tree, \%build, \%times);
  
  foreach my $buildname (sort keys %build) {
    print "<LINE>$state_symbols{$build{$buildname}} $buildname\n";
  }

  print q{
    </display>
    <DISPLAY NAME=help>
      Legend:<BR>
      + : Good Build<BR>
      ! : Broken Build<BR>
      ~ : Tests Failed
    </DISPLAY>
   </HDML>
  };

}
