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

use strict;
use Data::Dumper;
require 'header.pl';

my %colormap = (
                null       => 'a5a5a5',
                success    => '11DD11',
                busted     => 'EE0000',
                building   => 'EEFF00',
                exception  => '770088',
                testfailed => 'FFAA00'
                );

my %titlemap = (
                success    => 'success',
                busted     => 'busted',
                building   => 'building',
                exception  => 'exception',
                testfailed => 'testfailed',
                flames     => 'burning',
                star       => ''
                );

my %textmap = (
               success    => 'L',
               busted     => 'L!',
               building   => 'L/',
               exception  => 'L?',
               testfailed => 'L-',
               flames     => '%',
               star       => '*'
               );

my %images = (
              flames    => '1afi003r.gif',
              star      => 'star.gif'
              );

my @who_check_list;


# $rel_path is the relative path to webtools/tinderbox used for links.
# It changes to "$::static_rel_path../" if the page is generated statically, 
# because then it is placed in tinderbox/$::tree_dir/$tree.
my $rel_path = '';


sub tb_build_static($) {
    my ($form_ref) = (@_);
    $form_ref->{static} = 1;
    &do_static($form_ref);
}

sub do_static($) {
    my ($form_ref) = (@_);
    my $tree = &require_only_one_tree($form_ref->{tree});

    local *OUT;

    $form_ref->{legend}=0;

    my @pages = ( ['index.html', 'do_tinderbox'],
                  ['panel.html', 'do_panel'],
                  ['quickparse.txt', 'do_quickparse'],
                  ['json.js', 'do_json'],
                  ['json2.js', 'do_json2'],
                  ['status.html', 'do_status'] );

    my ($key, $value);
    $rel_path = $::static_rel_path;
    foreach $key (keys %images) {
        $value = $images{$key};
        $images{$key} = "$rel_path$value";
    }

    my $oldfh = select;

    foreach my $pair (@pages) {
        my ($page, $call) = @{$pair};
        my $outfile = "$::tree_dir/$tree/$page";

        open(OUT, ">", "$outfile.$$");
        select OUT;

        eval "$call(\$form_ref)";

        close(OUT);
        unlink($outfile);
        rename("$outfile.$$", "$outfile");
    }
    select $oldfh;
}

sub do_tinderbox($) {
    my ($form_ref) = (@_);
    &require_only_one_tree($form_ref->{tree});
    my $tinderbox_data = &tb_load_data($form_ref);
    &print_page_head($form_ref, $tinderbox_data);
    &print_table_header($form_ref, $tinderbox_data);
    &print_table_body($tinderbox_data);
    &print_table_footer($form_ref, $tinderbox_data);
}

##
# Return all data that the waterfall normally would, but as JSON not HTML.
##
sub do_json($) {
    my ($form_ref) = (@_);
    my $tinderbox_data = tb_load_data($form_ref);
    if (!$form_ref->{static}) {
        print "Content-Type: text/javascript; charset=utf-8\n";
        print "Access-Control-Allow-Origin: *\n\n";
    }
    print "tinderbox_data";
    $Data::Dumper::Indent = 0;
    my $line = Dumper($tinderbox_data);
    $line =~ s/=>/:/g;
    $line =~ s/\$VAR1//g;
    $line =~ s/undef/'undef'/g;
    $line =~ s/\n/\\n/g;
    $line =~ s/\r//g;
    $line =~ s/: ,/: '',/g;
    print "$line\n";
}

##
# Return all data that the waterfall normally would, but as valid JSON not HTML.
##
sub do_json2($) {
    my ($form_ref) = (@_);
    if (!$form_ref->{static}) {
        print "Content-Type: text/javascript; charset=utf-8\n";
        print "Access-Control-Allow-Origin: *\n\n";
    }
    tb_print_json_data($form_ref);
}

##
# Return the tree status by itself for easier scraping.
##
sub do_status($) {
    my ($form_ref) = (@_);
    if (!$form_ref->{static}) {
        print "Content-Type: text/html; charset=utf-8\n";
        print "Access-Control-Allow-Origin: *\n\n";
    }
    my $tree = $form_ref->{tree};
    my $status_message = &tb_load_status($tree);
    $status_message =~ s:^\s*|\s*$::gs;
    if ($status_message and length($status_message) gt 0) {
        print "$status_message";
    }
}

sub print_page_head($$) {
    my ($form_ref, $td) = (@_);
    my $tree = $form_ref->{tree};
    print "Content-Type: text/html; charset=utf-8\n\n<HTML>\n" unless $form_ref->{static};

    use POSIX qw(strftime);
    # Print time in format "YYYY-MM-DD HH:MM timezone"
    my $now = strftime("%Y-%m-%d %H:%M %Z", localtime);

    &EmitHtmlTitleAndHeader("tinderbox: $tree", "tinderbox",
                            "tree: $tree ($now)");

    &print_javascript($td);

    # Print rules, sheriff, and status.  Only on the first pageful.
    if ($::nowdate eq $td->{maxdate}) {
        unless ($form_ref->{norules}) {
            print "<a NAME=\"rules\"></a>" . &tb_load_rules($tree);
        }
        my $current_sheriff = &tb_load_sheriff($tree);
        $current_sheriff =~ s:^\s*|\s*$::gs;
        if ($current_sheriff and length($current_sheriff) gt 0) {
            print "<a NAME=\"sheriff\"></a>$current_sheriff";
        }

        my $status_message = &tb_load_status($tree);
        $status_message =~ s:^\s*|\s*$::gs;
        if ($status_message and length($status_message) gt 0) {
            print "<a NAME=\"status\"></a>$status_message";
        }

        # keeps the main table from clearing the IFRAME
        print "<br clear=\"all\">\n";
    }

    # Quote and Legend
    #
    if ($form_ref->{legend}) {
        print qq{
            <table width="100%" cellpadding=0 cellspacing=0>
                <tr>
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
                <img src="$images{star}" title="$titlemap{star}" alt="$textmap{star}"></td>
                <td>= Show Log comments</td>
                </tr>
                <tr>
                <td colspan=2>
                <table cellspacing=1 cellpadding=1 border=1>
                </tr>
                <tr bgcolor="$colormap{success}">
                <td>
                Successful Build, optional bloaty stats:<br>
                <tt>Lk:XXX</tt> (bytes leaked)<br>
                <tt>Bl:YYYY</tt> (bytes allocated, bloat)<br>
                <tt>Tp:TT.T</tt> (page-loader time, ms)<br>
                <tt>Txul:TT.T</tt> (XUL openwindow time, ms)<br>
                <tt>Ts:TT.T</tt>   (startup time, sec)<br>
                </td>
                </tr>
                <tr bgcolor="$colormap{null}">
                <td>No build in progress</td>
                </tr>
                <tr bgcolor="$colormap{building}">
                <td>Build in progress</td>
                </tr>
                <tr bgcolor="$colormap{testfailed}">
                <td>Successful build, but tests failed</td>
                </tr>
                <tr bgcolor="$colormap{busted}">
                <td>Build failed</td>
                </tr>
                <tr bgcolor="$colormap{exception}">
                <td>Non-build failure</td>
                </tr>
                </table>
                </td></tr></table>
                </td>
                </tr>
                </table>
            };
    }
    if (&is_tree_state_available($tree)) {
        print "<a NAME=\"open\"></a>";
        print 'The tree is <font size="+2" ';
        if (is_tree_open($tree)) {
            print 'color="green">OPEN';
        } else {
            print 'color="red">CLOSED';
        }
        print "</font>\n";
    }
}

sub print_table_body($) {
    my ($td) = (@_);
    # Reset globals
    undef @who_check_list;
    for (my $tt=0; $tt < $td->{time_count}; $tt++) {
        last if $td->{build_time_times}->[$tt] < $td->{mindate};
        &print_table_row($td, $tt);
    }
}


BEGIN {
    # Make $lasthour persistent private variable for print_table_row().
    my $lasthour = ''; 

    sub print_table_row {
        my ($td, $tt) = @_;
        my $tree = $td->{name};

        # Time column
        # 
        my $query_link = '';
        my $end_query  = '';
        my $pretty_time = &print_time($td->{build_time_times}->[$tt]);
        my $hour;

        ($hour) = $pretty_time =~ /(\d\d):/;

        if ($lasthour ne $hour or &has_who_list($td, $tt)) {
            $query_link = &query_ref($td, $td->{build_time_times}->[$tt]);
            $end_query  = '</a>';
        }
        if ($lasthour eq $hour) {
            $pretty_time =~ s/^.*&nbsp;//;
        } else {
            $lasthour = $hour;
        }
        
        my $hour_color = '';
        $hour_color = ' bgcolor=#e7e7e7'
            if ($td->{build_time_times}->[$tt] + 1) % 7200 <= 3600;
        print "<tr align=center><td align=right$hour_color>",
        "$query_link\n$pretty_time$end_query</td>\n";
        
        # Guilty
        #
        print '<td>';
        for my $who (sort keys %{$td->{who_list}->[$tt]} ){
            my $qr;
            if ($tt eq 0) {
                $qr = &who_menu($td, $td->{build_time_times}->[$tt],
                                undef,$who);
            } else {
                $qr = &who_menu($td, $td->{build_time_times}->[$tt],
                                $td->{build_time_times}->[$tt-1],$who);
            }
            $who =~ s/%.*$//;
            print "  $qr$who</a>\n";
        }
        print '</td>';
        
        # Build Status
        #
        for (my $build_index=0; $build_index < $td->{name_count}; $build_index++) {
            my $br = $td->{build_table}->[$tt][$build_index];
            if (not defined($br)) {
                # No build data for this time (e.g. no build after this time).
                print "<td></td>\n";
                next;
            }
            next if $br == -1; # Covered by rowspan

            my $rowspan = $br->{rowspan};

            # This appears to be designed to keep the rowspan from running beyond
            # the length of the displayed table.  I'm not certain that can happen
            # in a table.  Besides, if rowspan is set to that sort of invalid value,
            # that's more of a sign that there's a bug in tbglobals.pl.
            #
            if ( $rowspan > $td->{mindate_time_count} - $tt + 1 ) {
                $rowspan = $td->{mindate_time_count} - $tt + 1
                }

            print "<td rowspan=\"$rowspan\" bgcolor=\"$colormap{$br->{buildstatus}}\">\n";

            if ( $br->{buildstatus} eq "null" ) {
                print "</td>\n";
                next;
            }
            
            my $logfile = $br->{logfile};
            my $buildtree = $br->{td}->{name};

            my $logfileexists = ( -f "$::tree_dir/$buildtree/$logfile" ? 1 : 0 );
            
            print "<tt>\n";
            
            # Build Note
            # 
            my $logurl = "${rel_path}showlog.cgi?log=$buildtree/$logfile";
            
            if ($br->{hasnote}) {
                print qq|
                    <a href="$logurl"
                    onclick="return note(event,$br->{noteid},'$logfile');">
                    <img src="$images{star}" title="$titlemap{star}" alt="$textmap{star}" border=0></a>
                    |;
            }
            
            # Build Log
            # 
            # Uncomment this line to print logfile names in build rectangle.
            # print "$logfile<br>";
            
            if ( 1 ) {
                # Add build start, end, and elapsed time where possible.
                my($start, $end, $elapsed);

                my $start_timet = $br->{buildtime};
                my $end_timet = $br->{endtime};

                # If either of the times aren't today, we need to qualify both with
                # the month and day-of-month.
                my $need_to_qualify;
                if ( both_are_today($start_timet, $end_timet) ) {
                    $need_to_qualify = 0;
                } else {
                    $need_to_qualify = 1;
                }

                # Grab the human-readable start time.
                $start = get_local_hms($start_timet, $need_to_qualify);

                # If we're still building, the endtime only reflects the opening
                # mail that the build has started, not the time at which the build
                # ended.  In that case, don't use it.  Use the current time, instead.
                my $time_info = "";
                if ($br->{buildstatus} eq 'building') {
                    $elapsed = get_time_difference(time(), $start_timet);

                    $time_info = "Started $start, still building..";
                } else {
                    $end = get_local_hms($end_timet, $need_to_qualify);
                    $elapsed = get_time_difference($end_timet, $start_timet);

                    $time_info = "Started $start, finished $end";
                }

                print qq|
                    <A HREF="$logurl"
                    onclick="return log(event,$build_index,$logfileexists,'$logfile','$time_info','$elapsed');"
                    title="$titlemap{$br->{buildstatus}}">
                    $textmap{$br->{buildstatus}}</a>
                    |;
            } else {
                print qq|
                    <A HREF="$logurl"
                    onclick="return log(event,$build_index,$logfileexists,'$logfile');"
                    title="$titlemap{$br->{buildstatus}}">
                    $textmap{$br->{buildstatus}}</a>
                    |;
            }
            
            # What Changed
            #
            # Only add the "C" link if there have been changes since the last build.
            if ($br->{previousbuildtime}) {
                my $previous_buildtime_index = $td->{build_time_index}->{$br->{previousbuildtime}};
                my $this_buildtime_index = $td->{build_time_index}->{$br->{buildtime}} + 1;

                if (&has_who_list($td,
                                  $this_buildtime_index,
                                  $previous_buildtime_index)) {
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
                my $binaryurl = $br->{binaryurl};
                print" <A HREF=$binaryurl>D</A>";
            }
            

            # Scrape data
            if (defined $td->{scrape}{$logfile}) {
                my (@scrape_data)
                    = @{ $td->{scrape}{$logfile} };
                # ex: Tp:5.45s
                my $i;
                foreach $i (@scrape_data) {
                    print "<br>$i";
                }
            }

            # Warnings
            if (defined $td->{warnings}{$logfile}) {
                my ($warning_count) = $td->{warnings}{$logfile};
                my $warn_file = "$tree/warn$logfile";
                $warn_file =~ s/\.gz$/.html/;
                print "<br><br><a href='${rel_path}$warn_file'>Warn:$warning_count</a>";
            }

            print "</tt>\n</td>";
        }
        print "</tr>\n";
    }
} # END

sub print_table_header($) {
    my ($form_ref, $td) = (@_);
    print "<table id='build_waterfall' border=1 bgcolor='#FFFFFF' cellspacing=1 cellpadding=1>\n";

    print "<tr align=center>\n";

    print "<TH>Build Time</TH>\n";
    print "<TH>Guilty</th>\n";

    for (my $ii=0; $ii < $td->{name_count}; $ii++) {

        my $bn = $td->{build_names}->[$ii];
        $bn =~ s/Clobber/Clbr/g;
        $bn =~ s/Depend/Dep/g;
        $bn = "<font face='Helvetica,Arial' size=-1>$bn</font>";

        my $last_status = &tb_last_status($td, $ii);
        if ($last_status eq 'busted') {
            if ($form_ref->{noflames}) {
                print "<td rowspan=2 bgcolor=$colormap{busted}><a title='$titlemap{flames}'>$bn $textmap{flames}</a></td>";
            } else {
                print "<td rowspan=2 bgcolor=000000 background='$images{flames}' style='background-position: bottom; background-repeat: repeat-x;'>";
                print "<font color=white><a title='$titlemap{flames}'>$bn $textmap{flames}</a></font></td>";
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

sub print_table_footer($$) {
    my ($form_ref, $td) = (@_);
    my $tree = $form_ref->{tree};
    print "</table>\n";

    # Copy form data into separate hash so that we can modify it
    # but retain the original url values
    my %footer_form = %{$form_ref};
    $footer_form{norules} = 1;
    $footer_form{legend} = 0;
    undef $footer_form{static};

    my $hours = $footer_form{hours} || $::default_hours;

    $footer_form{maxdate} = $td->{maxdate} - $hours*60*60;
    print open_showbuilds_href(%footer_form) .
        "Show previous $hours hours</a><br>";

    if ($hours != 24) {
        my $save_hours = $footer_form{hours};
        $footer_form{hours} = 24;
        print open_showbuilds_href(%footer_form) .
            "Show previous 24 hours</a><br>";
        $footer_form{hours} = $save_hours;
    }

    print "Show $hours hours from the previous ";
    $footer_form{maxdate} = $td->{maxdate} - 24*60*60*7;
    print open_showbuilds_href(%footer_form) . "1</a>, ";

    $footer_form{maxdate} = $td->{maxdate} - 24*60*60*7*4;
    print open_showbuilds_href(%footer_form) . "4</a>, ";

    $footer_form{maxdate} = $td->{maxdate} - 24*60*60*7*12;
    print open_showbuilds_href(%footer_form) . "12</a>, or ";

    $footer_form{maxdate} = $td->{maxdate} - 24*60*60*7*52;
    print open_showbuilds_href(%footer_form) . "52</a> weeks.<br>";

    print "<p><a href='${rel_path}admintree.cgi?tree=$tree'>" . 
        "Administrate Tinderbox Trees</a><br>\n";
}

sub open_showbuilds_url {
    my %args = (@_);
    my $url = "${rel_path}showbuilds.cgi?tree=$args{tree}";
    while (my ($key, $value) = each %args) {
        $url .= "&amp;" . url_encode($key) . "=" . url_encode($value) if ($value ne '' && $key ne 'tree');
    }
    return $url;
}
    
sub open_showbuilds_href {
    return "<a href=\"".open_showbuilds_url(@_)."\">";
}
    
# Same as open_showbuilds_href, but adding parent target
# so that URL's in iframes take over the parent window.
sub open_showbuilds_href_target {
    return "<a href=\"".open_showbuilds_url(@_)."\" target=\"_parent\">";
}

    sub query_ref {
        my ($td, $mindate, $maxdate, $who) = @_;
        my $output = '<a><!-- query system not configured -->';
        &tb_load_queryconfig();
        my $query_type = $::QueryInfo{$td->{query}}{type};

        if ($query_type eq "viewvc") {
            $output = "<a href=\"" .
                $::QueryInfo{$td->{query}}{url} .
                "?view=query&who_match=exact";
            $output .= "&date=explicit&mindate=" .
                strftime("%Y-%m-%d %T", gmtime($mindate));
            $output .= "&maxdate=" . 
                strftime("%Y-%m-%d %T", gmtime($maxdate))
                if (defined($maxdate) && $maxdate ne '');
            $output .= "&who=" . &url_encode($who) if (defined($who) && $who ne '');
            $output .= "\">";
        } elsif ($query_type eq "bonsai") {
            $output = "<a href=" .
                $::QueryInfo{$td->{query}}{url} .
                "cvsquery.cgi"; 
            $output .= "?module=$td->{cvs_module}";
            $output .= "&branch=$td->{cvs_branch}"   if $td->{cvs_branch} ne 'HEAD';
            $output .= "&branchtype=regexp"
                if $td->{cvs_branch} =~ /\+|\?|\*/;
            $output .= "&cvsroot=$td->{cvs_root}"    if $td->{cvs_root} ne $::default_cvsroot;
            $output .= "&date=explicit&mindate=$mindate";
            $output .= "&maxdate=$maxdate"           if $maxdate and $maxdate ne '';
            $output .= "&who=$who"                   if $who and $who ne '';
            $output .= ">";
        }
        return $output;
    }

    sub who_menu {
        my ($td, $mindate, $maxdate, $who) = @_;
        my $treeflag;
        # this variable isn't doing anything, so i'm going to use it shamelessly
        $treeflag = $td->{cvs_branch};
        # trick who.cgi into using regexps, escaping & and =
        $treeflag .= '%26branchtype%3Dregexp' if $treeflag =~ /\+|\?|\*/;

        &tb_load_treedata($td->{name});

        my $qr = '';
        my $ret = '<a><!-- no query system configured -->';
        &tb_load_queryconfig();
        my $query_type = $::QueryInfo{$td->{query}}{type};
        if ($query_type eq "viewvc") {
            $qr = $::QueryInfo{$td->{query}}{url} .
                "?view=query&who_match=exact&who=" . 
                &url_encode($who) . "&querysort=date&date=explicit" .
                "&mindate=" . strftime("%Y-%m-%d %T", gmtime($mindate));
            $qr .= "&maxdate=" . strftime("%Y-%m-%d %T", gmtime($maxdate)) if
                (defined($maxdate));
            $ret = "<a href='$qr'>";
        } elsif ($query_type eq "bonsai") {
            $qr = $::QueryInfo{$td->{query}}{registry_url} .
                "/who.cgi?email=". &url_encode($who)
                . "&d=$td->{cvs_module}|$treeflag|$td->{cvs_root}|$mindate";
            $qr = $qr . "|$maxdate" if defined($maxdate);
            $ret = "<a href=\"$qr\" onclick=\"return who(event);\">";
        }
        return $ret;

    }

# Check to see if anyone checked in during time slot.
#   ex.  has_who_list(1);    # Check for checkins in most recent time slot.
#   ex.  has_who_list(1,5);  # Check range of times.
    sub has_who_list($$$) {
        my ($td, $time1, $time2) = @_;

        if (not defined(@who_check_list)) {
            # Build a static array of true/false values for each time slot.
            $who_check_list[$td->{time_count} - 1] = 0;
            for (my $tt = 0; $tt < $td->{time_count}; $tt++) {
                $who_check_list[$tt] = 1 if each %{$td->{who_list}->[$tt]};
            }
        }
        if ($time2) {
            for (my $ii=$time1; $ii<=$time2; $ii++) {	 
                return 1 if $who_check_list[$ii];
            }
            return 0;
        } else {
            return 1 if $who_check_list[$time1]; 
        }
    }

    BEGIN {
        # Check bonsai tree for open/close state
        
        # Cache state in hash as multiple tinderboxes 
        # may have different bonsai trees
    
        my %treestate = {};
        my %checked_state = {};

        sub _check_tree_state($) {
            my ($tree) = (@_);

            $checked_state{$tree} = 1;
            &tb_load_treedata($tree);
            my $bonsai_tree = $::global_treedata->{$tree}->{bonsai_tree};
            &tb_load_queryconfig();
            my $query = $::global_treedata->{$tree}->{query};
            my $bonsai_dir = $::QueryInfo{$query}{directory};
            return if ($bonsai_tree =~ m/^$/ || $bonsai_dir =~ m/^$/);

            local $_;
            $::BatchID='';
            eval qq(do "$bonsai_dir/data/$bonsai_tree/batchid.pl");
            if ($::BatchID eq '') {
                warn "No BatchID in $bonsai_dir/data/$bonsai_tree/batchid.pl for $tree\n";
                return;
            }
            open(BATCH, "<", "$bonsai_dir/data/$bonsai_tree/batch-$::BatchID.pl")
                or warn "Cannot open $bonsai_dir/data/$bonsai_tree/batch-$::BatchID.pl for $tree";
            while (<BATCH>) { 
                if (/^\$::TreeOpen = '(\d+)';/) {
                    $treestate{$tree} = $1;
                    last;
                }
            }
            return;
        }

        sub is_tree_state_available($) {
            my ($tree) = (@_);
            return 1 if defined($treestate{$tree});
            return 0 if defined($checked_state{$tree});
            &_check_tree_state($tree);
            return &is_tree_state_available($tree);
        }

        sub is_tree_open($) {
            my ($tree) = (@_);
            &_check_tree_state($tree) unless $checked_state{$tree};
            return $treestate{$tree};
        }
    }

    sub print_javascript {
        my ($td) = (@_);
        my $tree = $td->{name};

        my $script;
        ($script = <<"__ENDJS") =~ s/^    //gm;
        <style type="text/css">
            #popup {
          position: absolute;
      margin: -5em 0 0 -5em;
      opacity: 0.9;
    }
    .who#popup{
  border: 0px;
  height: 8em;
  width: 16em;
}
.note#popup {
  width: 25em;
}
.log#popup {
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
if (typeof document.layers != 'undefined') {
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
        l = document.createElement("iframe");
    l.setAttribute("src", t.href);
    l.setAttribute("id", "popup");
    l.className = "who";
    t.appendChild(l);
}
return false;
}

function convert_timet_to_gmtdate(timet) {
    var timeconv = new Date();
timeconv.setTime( (timet * 1000) + \
                  (timeconv.getTimezoneOffset() * 60 * 1000) );
return timeconv.toLocaleString();
}

function convert_timet_to_localdate(timet) {
    var timeconv = new Date();
timeconv.setTime(timet * 1000);
return timeconv.toLocaleString();
}

function convert_timet_to_localhms(timet) {
    var timeconv = new Date();
timeconv.setTime(timet * 1000);

hours = timeconv.getHours();
if (hours < 10)
    hours = "0" + hours;

mins = timeconv.getMinutes();
if (mins < 10)
    mins = "0" + mins;

secs = timeconv.getSeconds();
if (secs < 10)
    secs = "0" + secs;

return hours + ":" + mins;
}

function log_url(logfile) {
    return "${rel_path}showlog.cgi?log=" + buildtree + "/" + logfile;
}
function note(d,noteid,logfile) {
    if (noDHTML) {
        document.location = log_url(logfile);
        return false;
    }
if (typeof document.layers != 'undefined') {
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
        l = document.createElement("div");
    l.innerHTML = notes[noteid];
    l.setAttribute("id", "popup");
    l.style.position = "absolute";
    l.className = "note";
    t.parentNode.parentNode.appendChild(l);
}
return false;
}
function log(e,buildindex,logfileexists,logfile,time_info,elapsed) {
    var logurl = log_url(logfile);
var commenturl = "${rel_path}addnote.cgi?log=" + buildtree + "/" + logfile;
if (noDHTML) {
    document.location = logurl;
    return false;
}

var blurb = "<B>" + builds[buildindex] + "</B><BR>";

// If time_info is set, it will contain either the start time of the
    // build or both the start and end time.
    if (time_info) {
        blurb = blurb + time_info + "<BR>"
        }

// elapsed tracks the time the build started to either its end time or
    // now.
    if (elapsed) {
        blurb = blurb + elapsed + " elapsed<BR>"
        }

if (logfileexists) {
    blurb = blurb + "<A HREF=" + logurl + ">View Brief Log</A><BR>"
        + "<A HREF=" + logurl + "&fulltext=1"+">View Full Log</A><BR>";
}

blurb = blurb + "<A HREF=" + commenturl + ">Add a Comment</A>";

if (typeof document.layers != 'undefined') {
    var q = document.layers["logpopup"];
    q.top = e.target.y - 6;

    var yy = e.target.x;
    if ( yy + q.clip.right > window.innerWidth) {
        yy = (window.innerWidth-30) - q.clip.right;
        if (yy < 0) { yy = 0; }
    }
    q.left = yy;
    q.visibility="show"; 
    q.document.write("<TABLE BORDER=1><TR><TD>" + blurb
                     + "</TD></TR></TABLE>");
    q.document.close();
} else {
    var t = e.target;
    while (t.nodeType != 1) {
        t = t.parentNode;
    }
    closepopup();
    var l = document.createElement("div");
    l.innerHTML = blurb + "<BR>";
    l.setAttribute("id", "popup");
    l.className = "log";
    t.parentNode.appendChild(l);
}
return false;
}

var notes = new Array();
var builds = new Array();

__ENDJS

    print $script;

my $ii = 0;
my $note_arrayref = $td->{note_array};
if (defined($note_arrayref)) {
    for ($ii=0; $ii < @$note_arrayref; $ii++) {
        my $ss = $note_arrayref->[$ii];
        print "notes[$ii] = ";
        $ss =~ s/\\/\\\\/g;
        $ss =~ s/\"/\\\"/g;
        $ss =~ s/\n/\\n/g;
        print "\"$ss\";\n";
    }
}
for ($ii=0; $ii < $td->{name_count}; $ii++) {
    my $bn = $td->{build_names}->[$ii];
print "builds[$ii]='$bn';\n";
}
print "var buildtree = '$tree';\n";

# Use JavaScript to refresh the page every 15 minutes
print "setTimeout('location.reload()',900000);\n" if $::nowdate eq $td->{maxdate};

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

sub do_express($) {
    my ($form_ref) = (@_);
    my $tree = &require_only_one_tree($form_ref->{tree});
    my %express_form = %{$form_ref};
    undef $express_form{express};

    print "Content-Type: text/html; charset=utf-8\nRefresh: 900\n\n<HTML>\n";

    my (%quickdata);
    tb_loadquickparseinfo($tree, $form_ref->{maxdate}, \%quickdata);

    my @keys = sort keys %quickdata;
    my $keycount = @keys;
    my $tm = &print_time(time);
    print "<table border=1 cellpadding=1 cellspacing=1><tr>";
    print "<th align=left colspan=$keycount>";
    print open_showbuilds_href_target(%express_form)."$tree";
    if (&is_tree_state_available($tree)) {
        print (&is_tree_open($tree) ? ' is open' : ' is closed');
    }
    print ", $tm</a></tr><tr>\n";
    foreach my $buildname (@keys) {
        print "<td bgcolor='$colormap{$quickdata{$buildname}->{buildstatus}}'>$buildname</td>";
    }
    print "</tr></table>\n";
}

# This is essentially do_express but it outputs a different format
sub do_panel($) {
    my ($form_ref) = (@_);
    my $tree = &require_only_one_tree($form_ref->{tree});
    my %panel_form = %{$form_ref};
    undef $panel_form{panel};

    print "Content-Type: text/html; charset=utf-8\n\n<HTML>\n" unless $form_ref->{static};

    my (%quickdata);
    tb_loadquickparseinfo($tree, $form_ref->{maxdate}, \%quickdata);

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
    # Make the static version of panel reference the static index file
    # Make sure the cgi panel output is reloaded in the same content window
    if ($form_ref->{static}) {
        print "<a target='_content' href='./'>";
    } else {
        print "<a href=\"" . open_showbuilds_url(%panel_form) .
            "\" target=\"_content\">";
    }
    print "$tree";
    
    if (&is_tree_state_available($tree)) {
        print " is ", &is_tree_open($tree) ? 'open' : 'closed';
    }
    # Add the current time
    my ($minute,$hour,$mday,$mon) = (localtime)[1..4];
    my $tm = sprintf("%d/%d&nbsp;%d:%02d",$mon+1,$mday,$hour,$minute);
    print ", $tm</a><br>";

    print "<table border=0 cellpadding=1 cellspacing=1>";
    foreach my $buildname (sort {$quickdata{$b}->{buildtime} cmp $quickdata{$a}->{buildtime}} keys %quickdata) {
        print "<tr><td bgcolor='$colormap{$quickdata{$buildname}->{buildstatus}}'>$buildname</td></tr>";
    }
    print "</table></body>";
}

sub do_quickparse($) {
    my ($form_ref) = (@_);

    print "Content-Type: text/plain; charset=utf-8\n\n" unless $form_ref->{static};
    my $tree = $form_ref->{tree};
    my @treelist = &make_tree_list();
    my @requestedtreelist = split /,/, $tree;
    foreach my $tt (@requestedtreelist) {
        next unless grep {$tt eq $_} @treelist;
        tb_load_treedata($tt);
        if (&is_tree_state_available($tt)) {
            my $state = &is_tree_open($tt) ? 'open' : 'closed';
            print "State|$tt|" . 
                $::global_treedata->{$tt}->{bonsai_tree} .
                "|$state\n";
        }
        my (%quickdata);
        tb_loadquickparseinfo($tt, $form_ref->{maxdate}, \%quickdata);
        
        # URL encode binaryurl so that urls with | will not
        # break the quickparse format
        foreach my $buildname (sort keys %quickdata) {
            print "Build|$tt|$buildname|" .
                "$quickdata{$buildname}->{buildstatus}|" .
                "$quickdata{$buildname}->{buildtime}|" . 
                url_encode($quickdata{$buildname}->{binaryurl}) . "\n";
        }
    }
}

sub do_rdf($) {
    my ($form_ref) = (@_);
    my $tree = &require_only_one_tree($form_ref->{tree});

    print "Content-Type: text/plain; charset=utf-8\n\n";

    my $mainurl = "http://$ENV{SERVER_NAME}$ENV{SCRIPT_NAME}?tree=$tree";
    my $dirurl = $mainurl;

    $dirurl =~ s@/[^/]*$@@;

    my (%quickdata);
    tb_loadquickparseinfo($tree, $form_ref->{maxdate}, \%quickdata);

    my $image = "channelok.gif";
    my $imagetitle = "OK";
    foreach my $buildname (sort keys %quickdata) {
        if ($quickdata{$buildname}->{buildstatus} eq 'busted') {
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
               <title>Tinderbox - $tree</title>
               <description>Build bustages for $tree</description>
               <link>$mainurl</link>
               </channel>
               <image>
               <title>$imagetitle</title>
               <url>$dirurl/$image</url>
               <link>$mainurl</link>
               </image>
           };    
    
    if (&is_tree_state_available($tree)) {
        my $state = &is_tree_open($tree) ? 'open' : 'closed';
        print "<item><title>The tree is currently $state</title>",
        "<link>$mainurl</link></item>\n";
    }

    foreach my $buildname (sort keys %quickdata) {
        if ($quickdata{$buildname}->{buildstatus} eq 'busted') {
            print "<item><title>$buildname is in flames</title>",
            "<link>$mainurl</link></item>\n";
        }
    }
    print "</rdf:RDF>\n";
}

1;
