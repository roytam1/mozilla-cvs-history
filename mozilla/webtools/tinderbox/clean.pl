#!/usr/bonsaitools/bin/perl --
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Netscape Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is the Tinderbox build tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.

# Figure out which directory tinderbox is in by looking at argv[0]

$tinderboxdir = $0;
$tinderboxdir =~ s:/[^/]*$::;      # Remove last word, and slash before it.
if ($tinderboxdir eq "") {
    $tinderboxdir = ".";
}

#print "tinderbox = $tinderboxdir\n"; 

chdir $tinderboxdir || die "Couldn't chdir to $tinderboxdir"; 

#print "cd ok\n";

open FL, "find . -name \"*.gz\" -mtime +7 -print |";

#print "find ok\n";

while( <FL> ){
    chop();
    #print "unlink $_\n";
    unlink $_;
}
