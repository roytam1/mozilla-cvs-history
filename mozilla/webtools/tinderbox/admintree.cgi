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

use lib "../bonsai";

require 'lloydcgi.pl';
require 'tbglobals.pl';
require 'header.pl';

$|=1;

print "Content-type: text/html\n\n<HTML>\n";

EmitHtmlHeader("administer tinderbox", "tree: $tree");

$form{noignore} = 1;            # Force us to load all build info, not
                                # paying any attention to ignore_builds stuff.
$maxdate = time();
$mindate = $maxdate - 24*60*60;
tb_load_data();

if (defined($tree)) {
    if( -r "$tree/mod.pl" ){
        require "$tree/mod.pl";
        $message_of_day =~ s/\s*$//;  # Trim trailing whitespace;
    }
    else {
        $message_of_day = "";
    }

    print "
<FORM method=post action=doadmin.cgi>
<B>Password:</B> <INPUT NAME=password TYPE=password>
<INPUT TYPE=HIDDEN NAME=tree VALUE=$tree>
<INPUT TYPE=HIDDEN NAME=command VALUE=set_message>
<br><b>Message of the Day
<br><TEXTAREA NAME=message ROWS=30 COLS=75 WRAP=SOFT>$message_of_day
</TEXTAREA>
<br><INPUT TYPE=SUBMIT VALUE='Set Message of the Day'>
</FORM>
<hr>
";


    print "
<FORM method=post action=doadmin.cgi>
<B>Password:</B> <INPUT NAME=password TYPE=password>
<INPUT TYPE=HIDDEN NAME=tree VALUE=$tree>
<INPUT TYPE=HIDDEN NAME=command VALUE=trim_logs>
<br><b>Trim Logs to <INPUT NAME=days size=5 VALUE='7'> days.</b> (Tinderbox 
shows 2 days history by default.  You can see more by clicking show all).
<br><INPUT TYPE=SUBMIT VALUE='Trim Logs'>
</FORM>
<FORM method=post action=doadmin.cgi>
<hr>
"   ;
}

print "
<FORM method=post action=doadmin.cgi>
<B>Password:</B> <INPUT NAME=password TYPE=password> <BR>
<INPUT TYPE=HIDDEN NAME=tree VALUE=$tree>
<INPUT TYPE=HIDDEN NAME=command VALUE=create_tree>
<TABLE>
<TR>
<TD><B>tinderbox tree name:</B></TD>
<TD><INPUT NAME=treename VALUE=''></TD>
</TR><TR>
<TD><B>cvs repository:</B></TD>
<TD><INPUT NAME=repository VALUE=''></TD>
</TR><TR>
<TD><B>cvs module name:</B></TD>
<TD><INPUT NAME=modulename VALUE=''></TD>
</TR><TR>
<TD><B>cvs branch:</B></TD>
<TD><INPUT NAME=branchname VALUE='HEAD'></TD>
</TR>
</TABLE>
<INPUT TYPE=SUBMIT VALUE='Create a new Tinderbox page'>
</FORM>
<FORM method=post action=doadmin.cgi>
<hr>
";

if (defined($tree)) {
    print "
<B><font size=+1>If builds are behaving badly you can turn them off.</font></b><br>  Uncheck
the build that is misbehaving and click the button.  You can still see all the
builds even if some are disabled by adding the parameter <b><tt>&noignore=1</tt></b> to
the tinderbox URL.<br>
<B>Password:</B> <INPUT NAME=password TYPE=password> <BR>
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
<INPUT TYPE=SUBMIT VALUE='Show only checked builds'>
</FORM>
<hr>
";
}





