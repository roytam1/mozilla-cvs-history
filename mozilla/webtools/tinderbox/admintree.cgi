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
require 'tbglobals.pl';
require 'header.pl';

# Process the form arguments
my %form = &split_cgi_args();

$|=1;

my $pass_prompt = "<B>Password:</B> <INPUT NAME=password TYPE=password>\n";
if (defined($ENV{REMOTE_USER}) && $ENV{REMOTE_USER} ne "") {
    $pass_prompt = "";
}

print "Content-Type: text/html; charset=utf-8\n\n";

$form{noignore} = 1;            # Force us to load all build info, not
                                # paying any attention to ignore_builds stuff.
$form{hours} = 24;              # Force us to check the past 24 hrs of builds
$form{tree} = &validate_tree($form{tree});
my $treedata = &tb_load_data(\%form);
my $showbuilds_url = "showbuilds.cgi";
$showbuilds_url = $::tinderbox_url . "showbuilds.cgi" if ($::force_admin_ssl);

my (@names, $i, $checked);

if (defined($treedata)) {
    my $tree = $treedata->{name};
    my $safe_tree = value_encode($tree);

    EmitHtmlHeader("administer tinderbox", "tree: $safe_tree");

    # Sheriff
    my $current_sheriff = &tb_load_sheriff($tree);
    $current_sheriff =~ s/\s*$//;  # Trim trailing whitespace;

    # Status message.
    my $status_message = &tb_load_status($tree);
    $status_message =~ s/\s*$//;  # Trim trailing whitespace;

    # Tree rules.
    my $rules_message = &tb_load_rules($tree);
    $rules_message =~ s/\s*$//;  # Trim trailing whitespace;

#
# Change sheriff
#
    print "<HR>
<FORM method=post action=doadmin.cgi>
    <INPUT TYPE=HIDDEN NAME=tree VALUE='$safe_tree'>
    <INPUT TYPE=HIDDEN NAME=command VALUE=set_sheriff>
    <br><b>Change sheriff info.</b>  (mailto: url, phone number, etc.)<br>
    <TEXTAREA NAME=sheriff ROWS=8 COLS=75>
" . value_encode($current_sheriff) . "
    </TEXTAREA>
    <br>
    $pass_prompt
    <INPUT TYPE=SUBMIT VALUE='Change Sheriff'>
</FORM>
<hr>
";

#
#  Status message
#

    print "
<FORM method=post action=doadmin.cgi>
    <INPUT TYPE=HIDDEN NAME=tree VALUE='$safe_tree'>
    <INPUT TYPE=HIDDEN NAME=command VALUE=set_status_message>
    <br><b>Status message.</b>  (Use this for stay-out-of-the-tree warnings, etc.)<br>
    <TEXTAREA NAME=status ROWS=8 COLS=75>
" . value_encode($status_message) . "
    </TEXTAREA>
    <br>
    $pass_prompt
    <INPUT TYPE=SUBMIT VALUE='Change status message'>
</FORM>
<hr>
";

#
#  Rules message.
#

    print "
<FORM method=post action=doadmin.cgi>
    <INPUT TYPE=HIDDEN NAME=tree VALUE='$safe_tree'>
    <INPUT TYPE=HIDDEN NAME=command VALUE=set_rules_message>
    <br><b>The tree rules.</b><br>
    <TEXTAREA NAME=rules ROWS=18 COLS=75>
" . value_encode($rules_message) . "
    </TEXTAREA>
    <br>
    $pass_prompt
    <INPUT TYPE=SUBMIT VALUE='Change rules message'>
</FORM>
<hr>
";

#
#  Trim logs.
#

    # Determine the collective size & age of the build logs
    opendir(TRIM_DIR, &shell_escape("$::tree_dir/$tree")) || die "opendir($safe_tree): $!";
    my @trim_files = grep { /\.(?:gz|brief\.html)$/ && -f "$::tree_dir/$tree/$_" } readdir(TRIM_DIR);
    close(TRIM_DIR);
    my $trim_bytes = 0;
    my $trim_size;
    my $now = time();
    my $trim_oldest = $now;
    my $size_K = 1024;
    my $size_M = 1048576;
    my $size_G = 1073741824;
    for my $file (@trim_files) {
        my @file_stat = stat("$::tree_dir/$tree/$file");
        $trim_bytes += $file_stat[7];
        $trim_oldest = $file_stat[9] if ($trim_oldest > $file_stat[9]);
    }
    my $trim_days = int (($now - $trim_oldest) / 86400);
    if ($trim_bytes < $size_K) {
        $trim_size = "$trim_bytes b";
    } elsif ($trim_bytes < $size_M) {
        $trim_size = int($trim_bytes / $size_K) . " Kb";
    } elsif ($trim_bytes < $size_G){
        $trim_size = int($trim_bytes / $size_M) . " Mb";
    } else {
        $trim_size = (int($trim_bytes / $size_G * 1000)/1000) . " Gb";
    }

    print "
<FORM method=post action=doadmin.cgi>
    <INPUT TYPE=HIDDEN NAME=tree VALUE='$safe_tree'>
    <INPUT TYPE=HIDDEN NAME=command VALUE=trim_logs>
    <b>Trim Logs</b><br>
    Trim Logs to <INPUT NAME=days size=5 VALUE='$trim_days'> days<br>
    Tinderbox is configured to show up to $::global_treedata->{$tree}->{who_days} days of log history. Currently, there are $trim_days days of logging taking up $trim_size of space.<br>
    $pass_prompt
    <INPUT TYPE=SUBMIT VALUE='Trim Logs'>
</FORM>
<hr>
"   ;

#
# Individual tree administration
#
    print "<B><font size=\"+1\">Individual tree administration</font></b><br>";
    print "
<table border=1>
    <tr><td><b>Current</b></td><td>Indicates which builds are currently active relative to the current time window.</td></tr>
    <tr><td><b>Active</b></td><td>Checked builds are shown on builds page. Add <b><tt>&amp;noignore=1</tt></b> to the tinderbox URL to override.</td></tr>
    <tr><td><b>Scrape</b></td><td>Checked builds will have the logs scanned for a token of the form <b>TinderboxPrint:aaa,bbb,ccc</b>.<br>These values will show up as-is in the showbuilds.cgi output.</td></tr>
    <tr><td><b>Warnings</b></td><td>Checked builds will have the logs scanned for compiler warning messages.</td></tr>
</table>
";

    print "
<br>
<FORM method=post action=doadmin.cgi>
    <INPUT TYPE=HIDDEN NAME=tree VALUE='$safe_tree'>
    <INPUT TYPE=HIDDEN NAME=command VALUE=admin_builds>
    <TABLE BORDER=1>
    <TR><TH>Build</TH><TH>Current</TH><TH>Active</TH><TH>Scrape</TH><TH>Warnings</TH></TR>
";
    
    foreach my $aname (@{$treedata->{build_names}},
                       keys %{$treedata->{ignore_builds}},
                       keys %{$treedata->{scrape_builds}},
                       keys %{$treedata->{warning_builds}},
                       ) {
        push @names, $aname if (!grep(/^$aname$/, @names));
    }

    for $i (sort @names){
        if ($i ne "") {
            my $buildname = &value_encode($i);
            my $current_status = (grep(/^$i$/, @{$treedata->{build_names}}) ? "CHECKED" : "");
            my $active_check = ($treedata->{ignore_builds}->{$i} != 0 ? "": "CHECKED" );
            my $scrape_check = ($treedata->{scrape_builds}->{$i} != 0 ? "CHECKED" : "" );
            my $warning_check = ($treedata->{warning_builds}->{$i} != 0 ? "CHECKED": "" );
            print "    <TR>\n";
            print "\t<TD>$buildname</TD>\n";
            print "\t<TD><INPUT TYPE=checkbox NAME='current_$buildname' $current_status DISABLED></TD>\n";
            print "\t<TD><INPUT TYPE=checkbox NAME='active_$buildname' $active_check ></TD>\n";
            print "\t<TD><INPUT TYPE=checkbox NAME='scrape_$buildname' $scrape_check ></TD>\n";
            print "\t<TD><INPUT TYPE=checkbox NAME='warning_$buildname' $warning_check ></TD>\n";
            print "\t<INPUT TYPE=HIDDEN NAME='all_$buildname' CHECKED>\n";
            print "    </TR>\n";
        }
    }
    print "    </TABLE>\n";
 
    print "
    $pass_prompt
    <INPUT TYPE=SUBMIT VALUE='Change build configuration'>
</FORM>
<hr>
";
    print "<B>\n";
    print "<A HREF=\"${showbuilds_url}?tree=$tree\">Return to tree: $tree</A><BR>\n";
    print "<A HREF=\"admintree.cgi\">Create new tree</A><BR>\n";
    print "<A HREF=\"${showbuilds_url}\">Tinderbox Tree Overview</A><BR>\n";
    print "</B>\n";

} else {
#
# Create a new tinderbox page.
#
    # Array used to generate admin page
    # * Form name
    # * Description
    # * Extra input attributes
    # * SeaMonkey example value
    my @admin_form = 
        (
         [ undef, "Generic options:", undef, undef ],
         [ "treename", "Tinderbox tree name:", undef, "SeaMonkey" ],
         [ "who_days", "Days of commit history to display:", "VALUE=\"14\"", "14" ],
         [ undef, "Bonsai query options:", undef, undef ],
         [ "repository", "CVS Repository", undef, "/cvsroot" ],
         [ "modulename", "CVS Module:", undef, "MozillaTinderboxAll" ],
         [ "branchname", "CVS Branch:", undef, "HEAD" ],
         [ "bonsaitreename", "Bonsai tree:", undef, "SeaMonkey" ],
         [ "bonsaidir", "Bonsai dir:", undef, "/var/www/html/bonsai" ],
         [ "bonsaiurl", "Bonsai url:", undef, "http://bonsai.mozilla.org" ],
         [ "bonsai_dbdriver", "Bonsai database driver:", undef, "mysql" ],
         [ "bonsai_dbhost", "Bonsai database host:", undef, "localhost" ],
         [ "bonsai_dbport", "Bonsai database port:", undef, "3306" ],
         [ "bonsai_dbname", "Bonsai database name:", undef, "bonsai" ],
         [ "bonsai_dbuser", "Bonsai database username:", undef, "bonsai" ],
         [ "bonsai_dbpasswd", "Bonsai database password:", "TYPE=\"password\"", "bonsai" ],
         [ "registryurl", "Registry URL:", undef, "http://bonsai.mozilla.org/registry/" ],
         [ undef, "ViewVC query options:", undef, undef ],
         [ "viewvc_url", "ViewVC URL:", undef, "http://viewvc/cgi-bin/viewvc.cgi/svn" ],
         [ "viewvc_repository", "ViewVC Repository:", undef, "/svnroot" ],
         [ "viewvc_dbdriver", "ViewVC database driver:", undef, "mysql" ],
         [ "viewvc_dbhost", "ViewVC database host:", undef, "localhost" ],
         [ "viewvc_dbport", "ViewVC database port:", undef, "3306" ],
         [ "viewvc_dbname", "ViewVC database name:", undef, "viewvc" ],
         [ "viewvc_dbuser", "ViewVC database user:", undef, "viewvc" ],
         [ "viewvc_dbpasswd", "ViewVC database password:", "TYPE=\"password\"", "viewvc" ],
         );

    EmitHtmlHeader("administer tinderbox", "create a tinderbox page");

    print "
<HR>
<FORM method=post action=doadmin.cgi>
    <INPUT TYPE=HIDDEN NAME=tree VALUE=''>
    <INPUT TYPE=HIDDEN NAME=command VALUE=create_tree>
    <H3>Create a new tinderbox page, examples for SeaMonkey shown in parens.</H3>
    <HR>
    <TABLE>
";

    for my $row_ref (@admin_form) {
        if (!defined(@$row_ref[0])) {
            print "\t<TR><TD><B>" . @$row_ref[1] . "</B></TD></TR>\n";
        } else {
            print "\t<TR>\n";
            print "\t    <TD>" . @$row_ref[1] . "</TD>\n";
            print "\t    <TD><INPUT NAME=\"" . 
                @$row_ref[0] . "\"" .
                (defined(@$row_ref[2]) ? " " . @$row_ref[2] : "")  . 
                "></TD>\n";
            print "\t    <TD>(" . @$row_ref[3] . ")</TD>\n";
            print "\t</TR>\n";
        }
    }
                

print "
    </TABLE>
    $pass_prompt
    <INPUT TYPE=SUBMIT VALUE='Create a new Tinderbox page'>
</FORM>
<HR>
<B><A HREF=\"${showbuilds_url}\">Tinderbox Tree Overview</A></B>
";

}

print "</BODY>\n</HTML>\n";
