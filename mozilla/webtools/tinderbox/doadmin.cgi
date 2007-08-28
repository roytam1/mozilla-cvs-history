#!/usr/bin/perl --
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
use Tie::IxHash;

require 'tbglobals.pl';

umask 002;
my $perm = "0660"; # Permission of created files
my $dir_perm = "0770"; # Permission of created dirs

# Process the form arguments
my %form = &split_cgi_args();
my %cookie_jar = &split_cookie_args();

$|=1;

&tb_check_password(\%form, \%cookie_jar);

print "Content-type: text/html\n\n<HTML>\n";

my $command = $form{'command'};
my $tree= $form{'tree'};

if ($command eq 'create_tree') {
    ($tree = $form{'treename'}) =~ s/^.*?([\w-\.]+).*$/\1/;
} else {
    $tree = &require_only_one_tree($tree);
}

if( $command eq 'create_tree' ){
    &create_tree;
}
elsif( $command eq 'trim_logs' ){
    &trim_logs;
}
elsif( $command eq 'set_status_message' ){
    &set_status_message;
}
elsif( $command eq 'set_rules_message' ){
    &set_rules_message;
}
elsif( $command eq 'set_sheriff' ){
    &set_sheriff;
}
elsif ($command eq 'admin_builds') {
    &admin_builds;
} else {
    print "Unknown command: \"$command\".";
}

sub trim_logs {
    print "<h2>Trimming Log files for $form{'tree'}...</h2><p>\n";
    my $builds_removed = tb_trim_logs($form{'tree'},  $form{'days'}, 1, 1);
    print "<h2>$builds_removed builds removed from build.dat</h2>\n";
    print "<h2><a href=\"showbuilds.cgi?tree=$tree\">Back to tree</a></h2>\n";
}

sub create_tree {
    tie my %treedata => 'Tie::IxHash';
    # make a copy of default_treedata to preserve order
    %treedata = %::default_treedata;
    $treedata{who_days} = $form{'who_days'};
    $treedata{cvs_root} = $form{'repository'};
    $treedata{cvs_module} = $form{'modulename'};
    $treedata{cvs_branch}= $form{'branchname'};
    $treedata{bonsai_tree} = $form{'bonsaitreename'};
    $treedata{bonsai_dir} = $form{'bonsaidir'};
    $treedata{bonsai_url} = $form{'bonsaiurl'};
    $treedata{bonsai_dbdriver} = $form{'bonsai_dbdriver'};
    $treedata{bonsai_dbhost} = $form{'bonsai_dbhost'};
    $treedata{bonsai_dbport} = $form{'bonsai_dbport'};
    $treedata{bonsai_dbname} = $form{'bonsai_dbname'};
    $treedata{bonsai_dbuser} = $form{'bonsai_dbuser'};
    $treedata{bonsai_dbpasswd} = $form{'bonsai_dbpasswd'};
    $treedata{registry_url} = $form{'registryurl'};
    $treedata{viewvc_url} = $form{'viewvc_url'};
    $treedata{viewvc_repository} = $form{'viewvc_repository'};
    $treedata{viewvc_dbdriver} = $form{'viewvc_dbdriver'};
    $treedata{viewvc_dbhost} = $form{'viewvc_dbhost'};
    $treedata{viewvc_dbport} = $form{'viewvc_dbport'};
    $treedata{viewvc_dbname} = $form{'viewvc_dbname'};
    $treedata{viewvc_dbuser} = $form{'viewvc_dbuser'};
    $treedata{viewvc_dbpasswd} = $form{'viewvc_dbpasswd'};

    $treedata{use_bonsai} = $treedata{use_viewvc} = 0;

    my $treename = $tree;

    for my $var ( 'cvs_module', 'cvs_branch', 'bonsai_tree', 'bonsai_dir',
                  'bonsai_url', 'bonsai_dbdriver', 'bonsai_dbhost',
                  'bonsai_dbport', 'bonsai_dbname', 'bonsai_dbuser',
                  'bonsai_dbpasswd', 'registry_url') {
        $treedata{use_bonsai} = 1 if (defined($treedata{$var}) && 
                                      "$treedata{$var}" ne "");
    }
    for my $var ('viewvc_url','viewvc_repository',
                 '{viewvc_dbdriver', 'viewvc_dbhost', 'viewvc_dbport',
                 'viewvc_dbname', 'viewvc_dbuser', 'viewvc_dbpasswd') {
        $treedata{use_viewvc} = 1 if (defined($treedata{$var}) && 
                                      "$treedata{$var}" ne "");
    }
    if ($treedata{use_bonsai} && $treedata{use_viewvc}) {
        my $errmsg = "Cannot configure tinderbox to use bonsai & viewvc at the same time.";
        print "<h1>$errmsg</h1>\n";
        die "$errmsg";
    }

    if( -r $treename ){
        chmod(oct($dir_perm), $treename);
    }
    else {
        mkdir( $treename, oct($dir_perm)) || die "<h1> Cannot mkdir $treename</h1>"; 
    }
    &write_treedata("$treename/treedata.pl", \%treedata);

    open( F, ">", "$treename/build.dat" );
    close( F );
    
    open( F, ">", "$treename/who.dat" );
    close( F );

    open( F, ">", "$treename/notes.txt" );
    close( F );

    open( F, ">", "$treename/index.html");
    print F "<HTML>\n";
    print F "<HEAD><META HTTP-EQUIV=\"refresh\" content=\"0,url=../showbuilds.cgi?tree=$treename\"></HEAD>\n";
    print F "<BODY></BODY>\n";
    print F "</HTML>\n";
    close( F );
    
    chmod oct($perm), "$treename/build.dat", "$treename/who.dat", "$treename/notes.txt",
    "$treename/treedata.pl", "$treename/index.html";

    print "<h2><a href=\"showbuilds.cgi?tree=$treename\">Tree created or modified</a></h2>\n";
}


sub admin_builds {
    my ($i,%active_buildnames, %scrape_buildnames, %warning_buildnames);

    # Read build.dat
    open(BD, "<", "$tree/build.dat");
    while(<BD>){
        my ($endtime,$buildtime,$bname) = split( /\|/ );
        $active_buildnames{$bname} = 0;
        $scrape_buildnames{$bname} = 0;
        $warning_buildnames{$bname} = 0;
    }
    close(BD);

    for $i (keys %form) {
        if ($i =~ m/^active_/ ) {
            $i =~ s/^active_//;
            $active_buildnames{$i} = 1;
        } elsif ($i =~ m/^scrape_/ ) {
            $i =~ s/^scrape_//;
            $scrape_buildnames{$i} = 1;
        } elsif ($i =~ m/^warning_/ ) {
            $i =~ s/^warning_//;
            $warning_buildnames{$i} = 1;
        }
    }

    open(IGNORE, ">", "$tree/ignorebuilds.pl");
    print IGNORE '$ignore_builds = {' . "\n";
    for $i (sort keys %active_buildnames){
        if ($active_buildnames{$i} == 0){
            print IGNORE "\t\t'$i' => 1,\n";
        }
    }
    print IGNORE "\t};\n";
    close IGNORE;

    open(SCRAPE, ">", "$tree/scrapebuilds.pl");
    print SCRAPE '$scrape_builds = {' . "\n";
    for $i (sort keys %scrape_buildnames){
        if ($scrape_buildnames{$i} == 1){
            print SCRAPE "\t\t'$i' => 1,\n";
        }
    }
    print SCRAPE "\t};\n";
    close SCRAPE;

    open(WARNING, ">", "$tree/warningbuilds.pl");
    print WARNING '$warning_builds = {' . "\n";
    for $i (sort keys %warning_buildnames){
        if ($warning_buildnames{$i} == 1){
            print WARNING "\t\t'$i' => 1,\n";
        }
    }
    print WARNING "\t};\n";
    close WARNING;

    chmod( oct($perm), "$tree/ignorebuilds.pl", "$tree/scrapebuilds.pl",
           "$tree/warningbuilds.pl");
    print "<h2><a href=showbuilds.cgi?tree=$tree>Build state Changed</a></h2>\n";
}
sub set_sheriff {
    my $m = $form{'sheriff'};
    $m =~ s/\'/\\\'/g;
    open(SHERIFF, ">", "$tree/sheriff.pl");
    print SHERIFF "\$current_sheriff = '$m';\n1;";
    close(SHERIFF);
    chmod( oct($perm), "$tree/sheriff.pl");
    print "<h2><a href=showbuilds.cgi?tree=$tree>
            Sheriff Changed.</a><br></h2>\n";
}

sub set_status_message {
    my $m = $form{'status'};
    $m =~ s/\'/\\\'/g;
    open(TREESTATUS, ">", "$tree/status.pl");
    print TREESTATUS "\$status_message = \'$m\'\;\n1;";
    close(TREESTATUS);
    chmod( oct($perm), "$tree/status.pl");
    print "<h2><a href=showbuilds.cgi?tree=$tree>
            Status message changed.</a><br></h2>\n";
}

sub set_rules_message {
    my $m = $form{'rules'};
    $m =~ s/\'/\\\'/g;
    open(RULES, ">", "$tree/rules.pl");
    print RULES "\$rules_message = \'$m\';\n1;";
    close(RULES);
    chmod( oct($perm), "$tree/rules.pl");
    print "<h2><a href=showbuilds.cgi?tree=$tree>
            Rule message changed.</a><br></h2>\n";
}

