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

require 'tbglobals.pl';
require 'header.pl';

# Process the form arguments
%form = ();
&split_cgi_args();

$|=1;

print "Content-type: text/html\n\n";

$form{noignore} = 1;            # Force us to load all build info, not
                                # paying any attention to ignore_builds stuff.
$maxdate = time();
$mindate = $maxdate - 24*60*60;
tb_load_data();

EmitHtmlHeader("administer tinderbox", "tree: $tree");

if (defined($tree)) {

    # Sheriff
    if( -r "$tree/sheriff.pl" ){
        require "$tree/sheriff.pl";
        $current_sheriff =~ s/\s*$//;  # Trim trailing whitespace;
    } else {
        $current_sheriff = "";
    }

    # Status message.
    if( -r "$tree/status.pl" ){
        require "$tree/status.pl";
        $status_message =~ s/\s*$//;  # Trim trailing whitespace;
    } else {
        $status_message = "";
    }

    # Tree rules.
    if( -r "$tree/rules.pl" ){
        require "$tree/rules.pl";
        $rules_message =~ s/\s*$//;  # Trim trailing whitespace;
    } else {
        $rules_message = "";
    }


#
# Change sheriff
#
    print "
<FORM method=post action=doadmin.cgi>
<INPUT TYPE=HIDDEN NAME=tree VALUE=$tree>
<INPUT TYPE=HIDDEN NAME=command VALUE=set_sheriff>
<br><b>Change sheriff info.</b>  (mailto: url, phone number, etc.)<br>
<TEXTAREA NAME=sheriff ROWS=8 COLS=75 WRAP=SOFT>$current_sheriff
</TEXTAREA>
<br>
<B>Password:</B> <INPUT NAME=password TYPE=password>
<b><INPUT TYPE=SUBMIT VALUE='Change Sheriff'></b>
</FORM>
<hr>
";

#
#  Status message
#

    print "
<FORM method=post action=doadmin.cgi>
<INPUT TYPE=HIDDEN NAME=tree VALUE=$tree>
<INPUT TYPE=HIDDEN NAME=command VALUE=set_status_message>
<br><b>Status message.</b>  (Use this for stay-out-of-the-tree warnings, etc.)<br>
<TEXTAREA NAME=status ROWS=8 COLS=75 WRAP=SOFT>$status_message
</TEXTAREA>
<br>
<b>
<B>Password:</B> <INPUT NAME=password TYPE=password>
<INPUT TYPE=SUBMIT VALUE='Change status message'>
</b>
</FORM>
<hr>
";

#
#  Rules message.
#

    print "
<FORM method=post action=doadmin.cgi>
<INPUT TYPE=HIDDEN NAME=tree VALUE=$tree>
<INPUT TYPE=HIDDEN NAME=command VALUE=set_rules_message>
<br><b>The tree rules.</b>
<br><TEXTAREA NAME=rules ROWS=18 COLS=75 WRAP=SOFT>$rules_message
</TEXTAREA>
<br>
<B>Password:</B> <INPUT NAME=password TYPE=password>
<b><INPUT TYPE=SUBMIT VALUE='Change rules message'></b>
</FORM>
<hr>
";

#
#  Trim logs.
#

    # Determine the collective size & age of the build logs
    opendir(TRIM_DIR, "$tree") || die "opendir($tree): $!";
    my @trim_files = grep { /\.(?:gz|brief\.html)$/ && -f "$tree/$_" } readdir(TRIM_DIR);
    close(TRIM_DIR);
    my $trim_bytes = 0;
    my $now = time();
    my $trim_oldest = $now;
    my $size_K = 1024;
    my $size_M = 1048576;
    my $size_G = 1073741824;
    for my $file (@trim_files) {
        my @file_stat = stat("$tree/$file");
        $trim_bytes += $file_stat[7];
        $trim_oldest = $file_stat[9] if ($trim_oldest > $file_stat[9]);
    }
    $trim_days = int (($now - $trim_oldest) / 86400);
    if ($trim_bytes < $size_k) {
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
<INPUT TYPE=HIDDEN NAME=tree VALUE='$tree'>
<INPUT TYPE=HIDDEN NAME=command VALUE=trim_logs>
<b>Trim Logs</b><br>
Trim Logs to <INPUT NAME=days size=5 VALUE='$trim_days'> days<br>
Tinderbox is configured to show up to $who_days days of log history. Currently, there are $trim_days days of logging taking up $trim_size of space.<br>
<B>Password:</B> <INPUT NAME=password TYPE=password>
<INPUT TYPE=SUBMIT VALUE='Trim Logs'>
</FORM>
<hr>
"   ;
} else {

#
# Create a new tinderbox page.
#

print "
<FORM method=post action=doadmin.cgi>
<INPUT TYPE=HIDDEN NAME=tree VALUE=$tree>
<INPUT TYPE=HIDDEN NAME=command VALUE=create_tree>
<b>Create a new tinderbox page, examples for SeaMonkey shown in parens.</b>
<TABLE>
<TR>
<TD>tinderbox tree name:</TD>
<TD><INPUT NAME=treename VALUE=''></TD>
<TD>(SeaMonkey)</TD>
</TR>
<TR>
<TD>days of commit history to display:</TD>
<TD><INPUT NAME=who_days VALUE=14></TD>
<TD>(14)</TD>
</TR>
<TR>
<TD>
<b>Bonsai query options:</b><br>
</TD>
</TR>
<TR>
<TD>cvs repository:</TD>
<TD><INPUT NAME=repository VALUE=''></TD>
<TD>(/cvsroot)</TD>
</TR>
<TR>
<TD>cvs module name:</TD>
<TD><INPUT NAME=modulename VALUE=''></TD>
<TD>(MozillaTinderboxAll)</TD>
</TR>
<TR>
<TD>cvs branch:</TD>
<TD><INPUT NAME=branchname VALUE=''></TD>
<TD>(HEAD)</TD>
</TR>
<TR>
<TD>bonsai tree:</TD>
<TD><INPUT NAME=bonsaitreename></TD>
<TD>(SeaMonkey)</TD>
</TR>
<TR>
<TD>
<b>ViewVC query options:</b><br>
</TD>
</TR>
<TR>
<TD>ViewVC URL:</TD>
<TD><INPUT NAME=viewvc_url></TD>
<TD>(http://viewvc/cgi-bin/viewvc.cgi/svn)</TD>
</TR>
<TR>
<TD>ViewVC Repository:</TD>
<TD><INPUT NAME=viewvc_repository></TD>
<TD>(/svnroot)</TD>
</TR>
<TR>
<TD>ViewVC database driver:</TD>
<TD><INPUT NAME=viewvc_dbdriver></TD>
<TD>(mysql)</TD>
</TR>
<TR>
<TD>ViewVC database host:</TD>
<TD><INPUT NAME=viewvc_dbhost></TD>
<TD>(localhost)</TD>
</TR>
<TR>
<TD>ViewVC database port:</TD>
<TD><INPUT NAME=viewvc_dbport></TD>
<TD>(3306)</TD>
</TR>
<TR>
<TD>ViewVC database name:</TD>
<TD><INPUT NAME=viewvc_dbname></TD>
<TD>(viewvc)</TD>
</TR>
<TR>
<TD>ViewVC database username:</TD>
<TD><INPUT NAME=viewvc_dbuser></TD>
<TD>(viewvc)</TD>
</TR>
<TR>
<TD>ViewVC database password:</TD>
<TD><INPUT NAME=viewvc_dbpasswd TYPE=password></TD>
<TD>(viewvc)</TD>
</TR>

</TABLE>
<B>Password:</B> <INPUT NAME=password TYPE=password>
<INPUT TYPE=SUBMIT VALUE='Create a new Tinderbox page'>
</FORM>
<hr>
";
}

#
# Toggle scraping for builds.
#

if (defined($tree)) {
    print "
<B><font size=+1>Turn on log scraping.</font></b><br>  Checked builds will have the logs scanned fora token of the form <b>TinderboxPrint:aaa,bbb,ccc</b>.  These values will show up as-is in the showbuilds.cgi output.<br>
<FORM method=post action=doadmin.cgi>
<INPUT TYPE=HIDDEN NAME=tree VALUE=$tree>
<INPUT TYPE=HIDDEN NAME=command VALUE=scrape_builds>
";

    @names = sort (@$build_names) ;

    for $i (@names){
        if( $i ne "" ){
            $checked = ($scrape_builds->{$i} != 0 ? "CHECKED": "" );
            print "<INPUT TYPE=checkbox NAME='build_$i' $checked >";
            print "$i<br>\n";
        }
    }

    print "
<B>Password:</B> <INPUT NAME=password TYPE=password>
<INPUT TYPE=SUBMIT VALUE='Scrape only checked builds'>
</FORM>
<hr>
";
}



#
# Turn builds off.
#

if (defined($tree)) {
    print "
<B><font size=+1>If builds are behaving badly you can turn them off.</font></b><br>  Uncheck
the build that is misbehaving and click the button.  Add <b><tt>&noignore=1</tt></b> to
the tinderbox URL to override.<br>
<FORM method=post action=doadmin.cgi>
<INPUT TYPE=HIDDEN NAME=tree VALUE=$tree>
<INPUT TYPE=HIDDEN NAME=command VALUE=disable_builds>
";

    @names = sort (@$build_names) ;

    for $i (@names){
        if( $i ne "" ){
            $checked = ($ignore_builds->{$i} != 0 ? "": "CHECKED" );
            print "<INPUT TYPE=checkbox NAME='build_$i' $checked >";
            print "$i<br>\n";
        }
    }

    print "
<B>Password:</B> <INPUT NAME=password TYPE=password>
<INPUT TYPE=SUBMIT VALUE='Show only checked builds'>
</FORM>
<hr>
";
}
print "</BODY></HTML>\n";
