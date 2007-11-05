#!/usr/bin/perl -w
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
# The Original Code is the Bonsai CVS tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 

use strict;

require 'CGI.pl';

my $file= $::FORM{'file'};
my $mark= &SanitizeMark($::FORM{'mark'});
my $ln = (($mark =~ m/^\d+$/ && $mark > 10) ? $mark-10 : 1 );
my $rev = &SanitizeRevision($::FORM{'rev'});
my $debug = $::FORM{'debug'};

print "Content-Type: text/html; charset=UTF-8\n\n";

my $CVS_ROOT = $::FORM{'root'};
if( !defined($CVS_ROOT) || $CVS_ROOT eq '' ){ 
    $CVS_ROOT = pickDefaultRepository();
}
validateRepository($CVS_ROOT);

my $CVS_REPOS_SUFIX = $CVS_ROOT;
$CVS_REPOS_SUFIX =~ s/\//_/g;
    
&ConnectToDatabase();

my @bind_values = ( $CVS_ROOT, $file );
my $qstring = "SELECT DISTINCT dirs.dir FROM checkins,dirs,files," .
    "repositories WHERE dirs.id=dirid AND files.id=fileid AND " .
    "repositories.id=repositoryid AND repositories.repository=? AND " .
    "files.file=? ORDER BY dirs.dir";

if ($debug) {
    print "<pre wrap>\n";
    print &html_quote($qstring) . "\n";
    print "With values:\n";
    foreach my $v (@bind_values) {
        print "\t" . &html_quote($v) . "\n";
    }
    print "</pre>\n";
}

my (@row, $d, @fl, $s);

&SendSQL($qstring, @bind_values);
while(@row = &FetchSQLData()){
    $d = $row[0];
    push @fl, "$d/$file";
}
&DisconnectFromDatabase();

if( @fl == 0 ){
    print "<h3>No files matched this file name: " . &html_quote($file) . 
        ".  It may have been added recently.</h3>";
}
elsif( @fl == 1 ){
    $s = &url_quote($fl[0]);
    print "<head>
    <meta http-equiv=Refresh
      content=\"0; URL=cvsblame.cgi?file=$s&rev=$rev&root=$CVS_ROOT&mark=$mark#$ln\">
    </head>
    ";    
}
else {
    print "<h3>Pick the file that best matches the one you are looking for:</h3>\n";
    for $s (@fl) {
        print "<dt><a href=cvsblame.cgi?file=" . &url_quote($s) . 
            "&rev=$rev&mark=$mark#$ln>" . &html_quote($s) . "</a>";
    }
}
