#!/usr/bonsaitools/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>

use diagnostics;
use strict;

require "CGI.pl";
use Date::Parse;

# Shut up misguided -w warnings about "used only once".  "use vars" just
# doesn't work for me.

sub sillyness {
    my $zz;
    $zz = $::defaultqueryname;
    $zz = $::unconfirmedstate;
    $zz = @::components;
    $zz = @::default_column_list;
    $zz = @::keywordsbyname;
    $zz = @::legal_keywords;
    $zz = @::legal_platform;
    $zz = @::legal_priority;
    $zz = @::legal_product;
    $zz = @::legal_resolution_no_dup;
    $zz = @::legal_severity;
    $zz = @::versions;
    $zz = @::target_milestone;
};

my $serverpush = 0;

ConnectToDatabase();

#print "Content-type: text/plain\n\n";    # Handy for debugging.
#$::FORM{'debug'} = 1;


if (grep(/^cmd-/, keys(%::FORM))) {
    my $url = "query.cgi?$::buffer#chart";
    print qq{Refresh: 0; URL=$url
Content-type: text/html

Adding field to query page...
<P>
<A HREF="$url">Click here if page doesn't redisplay automatically.</A>
};
    exit();
}



if (!defined $::FORM{'cmdtype'}) {
    # This can happen if there's an old bookmark to a query...
    $::FORM{'cmdtype'} = 'doit';
}


sub SqlifyDate {
    my ($str) = (@_);
    if (!defined $str) {
        $str = "";
    }
    my $date = str2time($str);
    if (!defined $date) {
        PuntTryAgain("The string '<tt>$str</tt>' is not a legal date.");
    }
    return time2str("%Y/%m/%d %H:%M:%S", $date);
}


sub GetByWordList {
    my ($field, $strs) = (@_);
    my @list;

    foreach my $w (split(/[\s,]+/, $strs)) {
        my $word = $w;
        if ($word ne "") {
            $word =~ tr/A-Z/a-z/;
            $word = SqlQuote(quotemeta($word));
            $word =~ s/^'//;
            $word =~ s/'$//;
            $word = '(^|[^a-z0-9])' . $word . '($|[^a-z0-9])';
            push(@list, "lower($field) regexp '$word'");
        }
    }

    return \@list;
}



sub Error {
    my ($str) = (@_);
    if (!$serverpush) {
        print "Content-type: text/html\n\n";
    }
    PuntTryAgain($str);
}
    
        



sub GenerateSQL {
    my $debug = 0;
    my ($fieldsref, $supptablesref, $wherepartref, $urlstr) = (@_);
    my @fields;
    my @supptables;
    my @wherepart;
    @fields = @$fieldsref if $fieldsref;
    @supptables = @$supptablesref if $supptablesref;
    @wherepart = @$wherepartref if $wherepartref;
    my %F;
    my %M;
    ParseUrlString($urlstr, \%F, \%M);
    my @specialchart;
    my @andlist;

    # First, deal with all the old hard-coded non-chart-based poop.

    unshift(@supptables,
            ("profiles map_assigned_to",
             "profiles map_reporter",
             "LEFT JOIN profiles map_qa_contact ON bugs.qa_contact = map_qa_contact.userid"));
    unshift(@wherepart,
            ("bugs.assigned_to = map_assigned_to.userid",
             "bugs.reporter = map_reporter.userid",
             "bugs.groupset & $::usergroupset = bugs.groupset"));
            

    my $minvotes;
    if (defined $F{'votes'}) {
        my $c = trim($F{'votes'});
        if ($c ne "") {
            if ($c !~ /^[0-9]*$/) {
                return Error("The 'At least ___ votes' field must be a\n" .
                             "simple number. You entered \"$c\", which\n" .
                             "doesn't cut it.");
            }
            push(@specialchart, ["votes", "greaterthan", $c - 1]);
        }
    }

    if ($M{'bug_id'}) {
        my $type = "anyexact";
        if ($F{'bugidtype'} && $F{'bugidtype'} eq 'exclude') {
            $type = "noexact";
        }
        push(@specialchart, ["bug_id", $type, join(',', @{$M{'bug_id'}})]);
    }

    if (defined $F{'sql'}) {
        die "Invalid sql: $F{'sql'}" if $F{'sql'} =~ /;/;
        push(@wherepart, "( $F{'sql'} )");
    }

    my @legal_fields = ("product", "version", "rep_platform", "op_sys",
                        "bug_status", "resolution", "priority", "bug_severity",
                        "assigned_to", "reporter", "component",
                        "target_milestone", "groupset");

    foreach my $field (keys %F) {
        if (lsearch(\@legal_fields, $field) != -1) {
            push(@specialchart, [$field, "anyexact",
                                 join(',', @{$M{$field}})]);
        }
    }

    if ($F{'keywords'}) {
        my $t = $F{'keywords_type'};
        if (!$t || $t eq "or") {
            $t = "anywords";
        }
        push(@specialchart, ["keywords", $t, $F{'keywords'}]);
    }

    foreach my $id ("1", "2") {
        if (!defined ($F{"email$id"})) {
            next;
        }
        my $email = trim($F{"email$id"});
        if ($email eq "") {
            next;
        }
        my $type = $F{"emailtype$id"};
        if ($type eq "exact") {
            $type = "anyexact";
            foreach my $name (split(',', $email)) {
                $name = trim($name);
                if ($name) {
                    DBNameToIdAndCheck($name);
                }
            }
        }

        my @clist;
        foreach my $field ("assigned_to", "reporter", "cc", "qa_contact") {
            if ($F{"email$field$id"}) {
                push(@clist, $field, $type, $email);
            }
        }
        if ($F{"emaillongdesc$id"}) {
            my $table = "longdescs_";
            push(@supptables, "longdescs $table");
            push(@wherepart, "$table.bug_id = bugs.bug_id");
            my $ptable = "longdescnames_";
            push(@supptables,
                 "LEFT JOIN profiles $ptable ON $table.who = $ptable.userid");
            push(@clist, "$ptable.login_name", $type, $email);
        }
        if (@clist) {
            push(@specialchart, \@clist);
        } else {
            return Error("You must specify one or more fields in which to\n" .
                  "search for <tt>$email</tt>.\n");
        }
    }

                
    if (defined $F{'changedin'}) {
        my $c = trim($F{'changedin'});
        if ($c ne "") {
            if ($c !~ /^[0-9]*$/) {
                return Error("The 'changed in last ___ days' field must be\n" .
                             "a simple number. You entered \"$c\", which\n" .
                             "doesn't cut it.");
            }
            push(@specialchart, ["changedin",
                                 "lessthan", $c + 1]);
        }
    }

    my $ref = $M{'chfield'};

    if (defined $ref) {
        my $which = lsearch($ref, "[Bug creation]");
        if ($which >= 0) {
            splice(@$ref, $which, 1);
            push(@specialchart, ["creation_ts", "greaterthan",
                                 SqlifyDate($F{'chfieldfrom'})]);
            my $to = $F{'chfieldto'};
            if (defined $to) {
                $to = trim($to);
                if ($to ne "" && $to !~ /^now$/i) {
                    push(@specialchart, ["creation_ts", "lessthan",
                                         SqlifyDate($to)]);
                }
            }
        }
    }



    if (defined $ref && 0 < @$ref) {
        push(@supptables, "bugs_activity actcheck");
    
        my @list;
        foreach my $f (@$ref) {
            push(@list, "\nactcheck.fieldid = " . GetFieldID($f));
        }
        push(@wherepart, "actcheck.bug_id = bugs.bug_id");
        push(@wherepart, "(" . join(' OR ', @list) . ")");
        push(@wherepart, "actcheck.bug_when >= " .
             SqlQuote(SqlifyDate($F{'chfieldfrom'})));
        my $to = $F{'chfieldto'};
        if (defined $to) {
            $to = trim($to);
            if ($to ne "" && $to !~ /^now$/i) {
                push(@wherepart, "actcheck.bug_when <= " .
                     SqlQuote(SqlifyDate($to)));
            }
        }
        my $value = $F{'chfieldvalue'};
        if (defined $value) {
            $value = trim($value);
            if ($value ne "") {
                push(@wherepart, "actcheck.newvalue = " .
                     SqlQuote($value))
            }
        }
    }


    foreach my $f ("short_desc", "long_desc", "bug_file_loc",
                   "status_whiteboard") {
        if (defined $F{$f}) {
            my $s = trim($F{$f});
            if ($s ne "") {
                my $n = $f;
                my $q = SqlQuote($s);
                my $type = $F{$f . "_type"};
                push(@specialchart, [$f, $type, $s]);
            }
        }
    }
    

    my $chartid;
    my $f;
    my $ff;
    my $t;
    my $q;
    my $v;
    my $term;
    my %funcsbykey;
    my @funcdefs =
        (
         "^(assigned_to|reporter)," => sub {
             push(@supptables, "profiles map_$f");
             push(@wherepart, "bugs.$f = map_$f.userid");
             $f = "map_$f.login_name";
         },
         "^qa_contact," => sub {
             push(@supptables,
                  "LEFT JOIN profiles map_qa_contact ON bugs.qa_contact = map_qa_contact.userid");
             $f = "map_$f.login_name";
         },
         "^cc," => sub {
             push(@supptables,
                  ("LEFT JOIN cc cc_$chartid ON bugs.bug_id = cc_$chartid.bug_id LEFT JOIN profiles map_cc_$chartid ON cc_$chartid.who = map_cc_$chartid.userid"));
             $f = "map_cc_$chartid.login_name";
         },
         "^long_?desc,changedby" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "longdescs $table");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             my $id = DBNameToIdAndCheck($v);
             $term = "$table.who = $id";
         },
         "^long_?desc,changedbefore" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "longdescs $table");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             $term = "$table.bug_when < " . SqlQuote(SqlifyDate($v));
         },
         "^long_?desc,changedafter" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "longdescs $table");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             $term = "$table.bug_when > " . SqlQuote(SqlifyDate($v));
         },
         "^long_?desc," => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "longdescs $table");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             $f = "$table.thetext";
         },
         "^attachments\..*," => sub {
             my $table = "attachments_$chartid";
             push(@supptables, "LEFT JOIN attachments $table ON bugs.bug_id = $table.bug_id");
             $f =~ m/^attachments\.(.*)$/;
             my $field = $1;
             if ($t eq "changedby") {
                 $v = DBNameToIdAndCheck($v);
                 $q = SqlQuote($v);
                 $field = "submitter_id";
                 $t = "equals";
             } elsif ($t eq "changedbefore") {
                 $v = SqlifyDate($v);
                 $q = SqlQuote($v);
                 $field = "creation_ts";
                 $t = "lessthan";
             } elsif ($t eq "changedafter") {
                 $v = SqlifyDate($v);
                 $q = SqlQuote($v);
                 $field = "creation_ts";
                 $t = "greaterthan";
             }
             if ($field eq "ispatch") {
                 if ($v ne "0" && $v ne "1") {
                     return Error("The only legal values for the 'Attachment is patch' field is 0 or 1.");
                 }
             }
             $f = "$table.$field";
         },
         "^changedin," => sub {
             $f = "(to_days(now()) - to_days(bugs.delta_ts))";
         },

         "^keywords," => sub {
             GetVersionTable();
             my @list;
             my $table = "keywords_$chartid";
             foreach my $value (split(/[\s,]+/, $v)) {
                 if ($value eq '') {
                     next;
                 }
                 my $id = $::keywordsbyname{$value};
                 if ($id) {
                     push(@list, "$table.keywordid = $id");
                 } else {
                     return Error("Unknown keyword named <code>$v</code>.\n" .
                                  "<P>The legal keyword names are\n" .
                                  "<A HREF=describekeywords.cgi>" . 
                                  "listed here</A>.\n");
                 }
             }
             my $haveawordterm;
             if (@list) {
                 $haveawordterm = "(" . join(' OR ', @list) . ")";
                 if ($t eq "anywords") {
                     $term = $haveawordterm;
                 } elsif ($t eq "allwords") {
                     $ref = $funcsbykey{",$t"};
                     &$ref;
                     if ($term && $haveawordterm) {
                         $term = "(($term) AND $haveawordterm)";
                     }
                 }
             }
             if ($term) {
                 push(@supptables, "keywords $table");
                 push(@wherepart, "$table.bug_id = bugs.bug_id");
             }
         },

	 "^(dependson|blocked)," => sub {
		push(@supptables, "dependencies");
		$ff = "dependencies.$f";
		$ref = $funcsbykey{",$t"};
		&$ref;
		push(@wherepart, "$term");
	 },


         ",equals" => sub {
             $term = "$ff = $q";
         },
         ",notequals" => sub {
             $term = "$ff != $q";
         },
         ",casesubstring" => sub {
             $term = "INSTR($ff, $q)";
         },
         ",(substring|substr)" => sub {
             $term = "INSTR(LOWER($ff), " . lc($q) . ")";
         },
         ",notsubstring" => sub {
             $term = "INSTR(LOWER($ff), " . lc($q) . ") = 0";
         },
         ",regexp" => sub {
             $term = "LOWER($ff) REGEXP $q";
         },
         ",notregexp" => sub {
             $term = "LOWER($ff) NOT REGEXP $q";
         },
         ",lessthan" => sub {
             $term = "$ff < $q";
         },
         ",greaterthan" => sub {
             $term = "$ff > $q";
         },
         ",anyexact" => sub {
             my @list;
             foreach my $w (split(/,/, $v)) {
                 if ($w eq "---" && $f !~ /milestone/) {
                     $w = "";
                 }
                 push(@list, "$ff = " . SqlQuote($w));
             }
             $term = join(" OR ", @list);
         },
         ",anywords" => sub {
             $term = join(" OR ", @{GetByWordList($ff, $v)});
         },
         ",allwords" => sub {
             $term = join(" AND ", @{GetByWordList($ff, $v)});
         },
         ",nowords" => sub {
             my @list = @{GetByWordList($ff, $v)};
             if (@list) {
                 $term = "NOT (" . join(" OR ", @list) . ")";
             }
         },
         ",changedbefore" => sub {
             my $table = "act_$chartid";
             my $ftable = "fielddefs_$chartid";
             push(@supptables, "bugs_activity $table");
             push(@supptables, "fielddefs $ftable");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             push(@wherepart, "$table.fieldid = $ftable.fieldid");
             $term = "($ftable.name = '$f' AND $table.bug_when < $q)";
         },
         ",changedafter" => sub {
             my $table = "act_$chartid";
             my $ftable = "fielddefs_$chartid";
             push(@supptables, "bugs_activity $table");
             push(@supptables, "fielddefs $ftable");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             push(@wherepart, "$table.fieldid = $ftable.fieldid");
             $term = "($ftable.name = '$f' AND $table.bug_when > $q)";
         },
         ",changedto" => sub {
             my $table = "act_$chartid";
             my $ftable = "fielddefs_$chartid";
             push(@supptables, "bugs_activity $table");
             push(@supptables, "fielddefs $ftable");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             push(@wherepart, "$table.fieldid = $ftable.fieldid");
             $term = "($ftable.name = '$f' AND $table.newvalue = $q)";
         },
         ",changedby" => sub {
             my $table = "act_$chartid";
             my $ftable = "fielddefs_$chartid";
             push(@supptables, "bugs_activity $table");
             push(@supptables, "fielddefs $ftable");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             push(@wherepart, "$table.fieldid = $ftable.fieldid");
             my $id = DBNameToIdAndCheck($v);
             $term = "($ftable.name = '$f' AND $table.who = $id)";
         },
         );
    my @funcnames;
    while (@funcdefs) {
        my $key = shift(@funcdefs);
        my $value = shift(@funcdefs);
        if ($key =~ /^[^,]*$/) {
            die "All defs in %funcs must have a comma in their name: $key";
        }
        if (exists $funcsbykey{$key}) {
            die "Duplicate key in %funcs: $key";
        }
        $funcsbykey{$key} = $value;
        push(@funcnames, $key);
    }


    my $chart = -1;
    my $row = 0;
    foreach my $ref (@specialchart) {
        my $col = 0;
        while (@$ref) {
            $F{"field$chart-$row-$col"} = shift(@$ref);
            $F{"type$chart-$row-$col"} = shift(@$ref);
            $F{"value$chart-$row-$col"} = shift(@$ref);
            if ($debug) {
                print qq{<P>$F{"field$chart-$row-$col"} | $F{"type$chart-$row-$col"} | $F{"value$chart-$row-$col"}*\n};
            }
            $col++;

        }
        $row++;
    }


    for ($chart=-1 ;
         $chart < 0 || exists $F{"field$chart-0-0"} ;
         $chart++) {
        $chartid = $chart >= 0 ? $chart : "";
        for (my $row = 0 ;
             exists $F{"field$chart-$row-0"} ;
             $row++) {
            my @orlist;
            for (my $col = 0 ;
                 exists $F{"field$chart-$row-$col"} ;
                 $col++) {
                $f = $F{"field$chart-$row-$col"} || "noop";
                $t = $F{"type$chart-$row-$col"} || "noop";
                $v = $F{"value$chart-$row-$col"};
                $v = "" if !defined $v;
                $v = trim($v);
                if ($f eq "noop" || $t eq "noop" || $v eq "") {
                    next;
                }
                $q = SqlQuote($v);
                my $func;
                $term = undef;
                foreach my $key (@funcnames) {
                    if ("$f,$t" =~ m/$key/) {
                        my $ref = $funcsbykey{$key};
                        if ($debug) {
                            print "<P>$key ($f , $t ) => ";
                        }
                        $ff = $f;
                        if ($f !~ /\./) {
                            $ff = "bugs.$f";
                        }
                        &$ref;
                        if ($debug) {
                            print "$f , $t , $term";
                        }
                        if ($term) {
                            last;
                        }
                    }
                }
                if ($term) {
                    push(@orlist, $term);
                } else {
                    my $errstr = "Can't seem to handle " .
                        qq{'<code>$F{"field$chart-$row-$col"}</code>' and } .
                            qq{'<code>$F{"type$chart-$row-$col"}</code>' } .
                                "together";
                    die "Internal error: $errstr" if $chart < 0;
                    return Error($errstr);
                }
            }
            if (@orlist) {
                push(@andlist, "(" . join(" OR ", @orlist) . ")");
            }
        }
    }
    my %suppseen = ("bugs" => 1);
    my $suppstring = "bugs";
    foreach my $str (@supptables) {
        if (!$suppseen{$str}) {
            if ($str !~ /^LEFT JOIN/i) {
                $suppstring .= ",";
            }
            $suppstring .= " $str";
            $suppseen{$str} = 1;
        }
    }
    my $query =  ("SELECT " . join(', ', @fields) .
                  " FROM $suppstring" .
                  " WHERE " . join(' AND ', (@wherepart, @andlist)) .
                  " GROUP BY bugs.bug_id");
    if ($debug) {
        print "<P><CODE>" . value_quote($query) . "</CODE><P>\n";
        exit();
    }
    return $query;
}


         
sub LookupNamedQuery {
    my ($name) = (@_);
    confirm_login();
    my $userid = DBNameToIdAndCheck($::COOKIE{"Bugzilla_login"});
    SendSQL("SELECT query FROM namedqueries " .
            "WHERE userid = $userid AND name = " . SqlQuote($name));
    my $result = FetchOneColumn();
    if (!defined $result) {
        print "Content-type: text/html\n\n";
        PutHeader("Something weird happened");
        print qq{The named query $name seems to no longer exist.};
        PutFooter();
        exit;
    }
    return $result;
}


        


CMD: for ($::FORM{'cmdtype'}) {
    /^runnamed$/ && do {
        $::buffer = LookupNamedQuery($::FORM{"namedcmd"});
        ProcessFormFields($::buffer);
        last CMD;
    };
    /^editnamed$/ && do {
	my $url = "query.cgi?" . LookupNamedQuery($::FORM{"namedcmd"});
        print qq{Content-type: text/html
Refresh: 0; URL=$url

<TITLE>What a hack.</TITLE>
<A HREF="$url">Loading your query named <B>$::FORM{'namedcmd'}</B>...</A>
};
        exit;
    };
    /^forgetnamed$/ && do {
        confirm_login();
        my $userid = DBNameToIdAndCheck($::COOKIE{"Bugzilla_login"});
        SendSQL("DELETE FROM namedqueries WHERE userid = $userid " .
                "AND name = " . SqlQuote($::FORM{'namedcmd'}));
        
        print "Content-type: text/html\n\n";
        PutHeader("Forget what?", "");

        print qq{
OK, the <B>$::FORM{'namedcmd'}</B> query is gone.
<P>
<A HREF="query.cgi">Go back to the query page.</A>
};
        PutFooter();
        exit;
    };
    /^asdefault$/ && do {
        confirm_login();
        my $userid = DBNameToIdAndCheck($::COOKIE{"Bugzilla_login"});
        print "Content-type: text/html\n\n";
        SendSQL("REPLACE INTO namedqueries (userid, name, query) VALUES " .
                "($userid, '$::defaultqueryname'," .
                SqlQuote($::buffer) . ")");
        PutHeader("OK, default is set");
        print qq{
OK, you now have a new default query.  You may also bookmark the result of any
individual query.

<P><A HREF="query.cgi">Go back to the query page, using the new default.</A>
};
        PutFooter();
        exit();
    };
    /^asnamed$/ && do {
        confirm_login();
        my $userid = DBNameToIdAndCheck($::COOKIE{"Bugzilla_login"});
        print "Content-type: text/html\n\n";
        my $name = trim($::FORM{'newqueryname'});
        if ($name eq "" || $name =~ /[<>&]/) {
            PutHeader("Please pick a valid name for your new query");
            print "Click the <B>Back</B> button and type in a valid name\n";
            print "for this query.  (Query names should not contain unusual\n";
            print "characters.)\n";
            PutFooter();
            exit();
        }
        $::buffer =~ s/[\&\?]cmdtype=[a-z]+//;
        my $qname = SqlQuote($name);
        SendSQL("SELECT query FROM namedqueries " .
                "WHERE userid = $userid AND name = $qname");
        if (!FetchOneColumn()) {
            SendSQL("REPLACE INTO namedqueries (userid, name, query) " .
                    "VALUES ($userid, $qname, " . SqlQuote($::buffer) . ")");
        } else {
            SendSQL("UPDATE namedqueries SET query = " . SqlQuote($::buffer) .
                    " WHERE userid = $userid AND name = $qname");
        }
        PutHeader("OK, query saved.");
        print qq{
OK, you have a new query named <code>$name</code>
<P>
<BR><A HREF="query.cgi">Go back to the query page</A>
};
        PutFooter();
        exit;
    };
}


if (exists $ENV{'HTTP_USER_AGENT'} && $ENV{'HTTP_USER_AGENT'} =~ /Mozilla.[3-9]/ && $ENV{'HTTP_USER_AGENT'} !~ /[Cc]ompatible/ ) {
    # Search for real Netscape 3 and up.  http://www.browsercaps.org used as source of
    # browsers compatbile with server-push.  It's a Netscape hack, incompatbile
    # with MSIE and Lynx (at least).  Even Communicator 4.51 has bugs with it,
    # especially during page reload.
    $serverpush = 1;

    print "Content-type: multipart/x-mixed-replace;boundary=thisrandomstring\n\n";
    print "--thisrandomstring\n";
    print "Content-type: text/html\n\n";
    print "<p>Please stand by ... <p>\n";
    # Note! HTML header is complete!
} else {
    print "Content-type: text/html\n";
    # Note! Don't finish HTML header yet!  Only one newline so far!
}

sub DefCol {
    my ($name, $k, $t, $s, $q) = (@_);
    
    $::key{$name} = $k;
    $::title{$name} = $t;
    if (defined $s && $s ne "") {
        $::sortkey{$name} = $s;
    }
    if (!defined $q || $q eq "") {
        $q = 0;
    }
    $::needquote{$name} = $q;
}

DefCol("opendate", "date_format(bugs.creation_ts,'%Y-%m-%d')", "Opened",
       "bugs.creation_ts");
DefCol("changeddate", "date_format(bugs.delta_ts,'%Y-%m-%d')", "Changed",
       "bugs.delta_ts");
DefCol("severity", "substring(bugs.bug_severity, 1, 3)", "Sev",
       "bugs.bug_severity");
DefCol("priority", "substring(bugs.priority, 1, 3)", "Pri", "bugs.priority");
DefCol("platform", "substring(bugs.rep_platform, 1, 3)", "Plt",
       "bugs.rep_platform");
DefCol("owner", "map_assigned_to.login_name", "Owner",
       "map_assigned_to.login_name");
DefCol("reporter", "map_reporter.login_name", "Reporter",
       "map_reporter.login_name");
DefCol("qa_contact", "map_qa_contact.login_name", "QAContact", "map_qa_contact.login_name");
DefCol("status", "substring(bugs.bug_status,1,4)", "State", "bugs.bug_status");
DefCol("resolution", "substring(bugs.resolution,1,4)", "Result",
       "bugs.resolution");
DefCol("summary", "substring(bugs.short_desc, 1, 60)", "Summary", "", 1);
DefCol("summaryfull", "bugs.short_desc", "Summary", "", 1);
DefCol("status_whiteboard", "bugs.status_whiteboard", "StatusSummary", "bugs.status_whiteboard", 1);
DefCol("component", "substring(bugs.component, 1, 8)", "Comp",
       "bugs.component");
DefCol("product", "substring(bugs.product, 1, 8)", "Product", "bugs.product");
DefCol("version", "substring(bugs.version, 1, 5)", "Vers", "bugs.version");
DefCol("os", "substring(bugs.op_sys, 1, 4)", "OS", "bugs.op_sys");
DefCol("target_milestone", "bugs.target_milestone", "TargetM",
       "bugs.target_milestone");
DefCol("votes", "bugs.votes", "Votes", "bugs.votes desc");
DefCol("keywords", "bugs.keywords", "Keywords", "bugs.keywords");

my @collist;
if (defined $::COOKIE{'COLUMNLIST'}) {
    @collist = split(/ /, $::COOKIE{'COLUMNLIST'});
} else {
    @collist = @::default_column_list;
}

my $minvotes;
if (defined $::FORM{'votes'}) {
    if (trim($::FORM{'votes'}) ne "") {
        if (! (grep {/^votes$/} @collist)) {
            push(@collist, 'votes');
        }
    }
}


my $dotweak = defined $::FORM{'tweak'};

if ($dotweak) {
    confirm_login();
    if (!UserInGroup("editbugs")) {
        print qq{
Sorry; you do not have sufficient priviledges to edit a bunch of bugs
at once.
};
        PutFooter();
        exit();
    }
} else {
    quietly_check_login();
}


my @fields = ("bugs.bug_id", "bugs.groupset");

foreach my $c (@collist) {
    if (exists $::needquote{$c}) {
        push(@fields, "$::key{$c}");
    }
}


if ($dotweak) {
    push(@fields, "bugs.product", "bugs.bug_status");
}



if ($::FORM{'regetlastlist'}) {
    if (!$::COOKIE{'BUGLIST'}) {
        print qq{
Sorry, I seem to have lost the cookie that recorded the results of your last
query.  You will have to start over at the <A HREF="query.cgi">query page</A>.
};
        PutFooter();
        exit;
    }
    my @list = split(/:/, $::COOKIE{'BUGLIST'});
    $::FORM{'bug_id'} = join(',', @list);
    if (!$::FORM{'order'}) {
        $::FORM{'order'} = 'reuse last sort';
    }
    $::buffer = "bug_id=" . $::FORM{'bug_id'} . "&order=" .
        url_quote($::FORM{'order'});
}



ReconnectToShadowDatabase();

my $query = GenerateSQL(\@fields, undef, undef, $::buffer);



if ($::COOKIE{'LASTORDER'}) {
    if ((!$::FORM{'order'}) || $::FORM{'order'} =~ /^reuse/i) {
        $::FORM{'order'} = url_decode($::COOKIE{'LASTORDER'});
    }
}


if (defined $::FORM{'order'} && $::FORM{'order'} ne "") {
    $query .= " ORDER BY ";
    $::FORM{'order'} =~ s/votesum/bugs.votes/; # Silly backwards compatability
                                               # hack.
    $::FORM{'order'} =~ s/assign\.login_name/map_assigned_to.login_name/g;
                                # Another backwards compatability hack.
    
    ORDER: for ($::FORM{'order'}) {
        /\./ && do {
            # This (hopefully) already has fieldnames in it, so we're done.
            last ORDER;
        };
        /Number/ && do {
            $::FORM{'order'} = "bugs.bug_id";
            last ORDER;
        };
        /Import/ && do {
            $::FORM{'order'} = "bugs.priority, bugs.bug_severity";
            last ORDER;
        };
        /Assign/ && do {
            $::FORM{'order'} = "map_assigned_to.login_name, bugs.bug_status, priority, bugs.bug_id";
            last ORDER;
        };
        # DEFAULT
        $::FORM{'order'} = "bugs.bug_status, bugs.priority, map_assigned_to.login_name, bugs.bug_id";
    }
    die "Invalid order: $::FORM{'order'}" unless
        $::FORM{'order'} =~ /^([a-zA-Z0-9_., ]+)$/;
    
    # Extra special disgusting hack: if we are ordering by target_milestone,
    # change it to order by the sortkey of the target_milestone first.
    my $order = $::FORM{'order'};
    if ($order =~ /bugs.target_milestone/) {
        $query =~ s/ WHERE / LEFT JOIN milestones ms_order ON ms_order.value = bugs.target_milestone AND ms_order.product = bugs.product WHERE /;
        $order =~ s/bugs.target_milestone/ms_order.sortkey,ms_order.value/;
    }

    $query .= $order;
}


if ($::FORM{'debug'} && $serverpush) {
    print "<P><CODE>" . value_quote($query) . "</CODE><P>\n";
}


if (Param('expectbigqueries')) {
    SendSQL("set option SQL_BIG_TABLES=1");
}
SendSQL($query);

my $count = 0;
$::bugl = "";
sub pnl {
    my ($str) = (@_);
    $::bugl  .= $str;
}

my $fields = $::buffer;
$fields =~ s/[&?]order=[^&]*//g;
$fields =~ s/[&?]cmdtype=[^&]*//g;


my $orderpart;
my $oldorder;

if (defined $::FORM{'order'} && trim($::FORM{'order'}) ne "") {
    $orderpart = "&order=" . url_quote("$::FORM{'order'}");
    $oldorder = url_quote(", $::FORM{'order'}");
} else {
    $orderpart = "";
    $oldorder = "";
}

if ($dotweak) {
    pnl "<FORM NAME=changeform METHOD=POST ACTION=\"process_bug.cgi\">";
}


my @th;
foreach my $c (@collist) {
    if (exists $::needquote{$c}) {
        my $h = "";
        if ($::needquote{$c}) {
            $h .= "<TH WIDTH=100%>";
        } else {
            $h .= "<TH>";
        }
        if (defined $::sortkey{$c}) {
            $h .= "<A HREF=\"buglist.cgi?$fields&order=" . url_quote($::sortkey{$c}) . "$oldorder\">$::title{$c}</A>";
        } else {
            $h .= $::title{$c};
        }
        $h .= "</TH>";
        push(@th, $h);
    }
}

my $tablestart = "<TABLE CELLSPACING=0 CELLPADDING=4 WIDTH=100%>
<TR ALIGN=LEFT><TH>
<A HREF=\"buglist.cgi?$fields&order=bugs.bug_id\">ID</A>";

my $splitheader = 0;
if ($::COOKIE{'SPLITHEADER'}) {
    $splitheader = 1;
}

if ($splitheader) {
    $tablestart =~ s/<TH/<TH COLSPAN="2"/;
    for (my $pass=0 ; $pass<2 ; $pass++) {
        if ($pass == 1) {
            $tablestart .= "</TR>\n<TR><TD></TD>";
        }
        for (my $i=1-$pass ; $i<@th ; $i += 2) {
            my $h = $th[$i];
            $h =~ s/TH/TH COLSPAN="2" ALIGN="left"/;
            $tablestart .= $h;
        }
    }
} else {
    $tablestart .= join("", @th);
}


$tablestart .= "\n";


my @row;
my %seen;
my @bugarray;
my %prodhash;
my %statushash;
my $buggroupset = "";
my %ownerhash;

my $pricol = -1;
my $sevcol = -1;
for (my $colcount = 0 ; $colcount < @collist ; $colcount++) {
    my $colname = $collist[$colcount];
    if ($colname eq "priority") {
        $pricol = $colcount;
    }
    if ($colname eq "severity") {
        $sevcol = $colcount;
    }
}

while (@row = FetchSQLData()) {
    my $bug_id = shift @row;
    my $g = shift @row;         # Bug's group set.
    if ($buggroupset eq "") {
        $buggroupset = $g;
    } elsif ($buggroupset ne $g) {
        $buggroupset = "x";     # We only play games with tweaking the
                                # buggroupset if all the bugs have exactly
                                # the same group.  If they don't, we leave
                                # it alone.
    }
    if (!defined $seen{$bug_id}) {
        $seen{$bug_id} = 1;
        $count++;
        if ($count % 200 == 0) {
            # Too big tables take too much browser memory...
            pnl "</TABLE>$tablestart";
        }
        push @bugarray, $bug_id;
        
        # retrieve this bug's priority and severity, if available,
        # by looping through all column names -- gross but functional
        my $priority = "unknown";
        my $severity;
        if ($pricol >= 0) {
            $priority = $row[$pricol];
        }
        if ($sevcol >= 0) {
            $severity = $row[$sevcol];
        }
        my $customstyle = "";
        if ($severity) {
            if ($severity eq "enhan") {
                $customstyle = "style='font-style:italic ! important'";
            }
            if ($severity eq "block") {
                $customstyle = "style='color:red ! important; font-weight:bold ! important'";
            }
            if ($severity eq "criti") {
                $customstyle = "style='color:red; ! important'";
            }
        }
        pnl "<TR VALIGN=TOP ALIGN=LEFT CLASS=$priority $customstyle><TD>";
        if ($dotweak) {
            pnl "<input type=checkbox name=id_$bug_id>";
        }
        pnl "<A HREF=\"show_bug.cgi?id=$bug_id\">";
        pnl "$bug_id</A> ";
        foreach my $c (@collist) {
            if (exists $::needquote{$c}) {
                my $value = shift @row;
                if (!defined $value) {
                    next;
                }
                if ($c eq "owner") {
                    $ownerhash{$value} = 1;
                }
                if ($::needquote{$c}) {
                    $value = html_quote($value);
                } else {
                    $value = "<nobr>$value</nobr>";
                }
                pnl "<td class=$c>$value";
            }
        }
        if ($dotweak) {
            my $value = shift @row;
            $prodhash{$value} = 1;
            $value = shift @row;
            $statushash{$value} = 1;
        }
        pnl "\n";
    }
}
my $buglist = join(":", @bugarray);


# This is stupid.  We really really need to move the quip list into the DB!
my $quip;
if (Param('usequip')){
  if (open (COMMENTS, "<data/comments")) {
    my @cdata;
    while (<COMMENTS>) {
      push @cdata, $_;
    }
    close COMMENTS;
    $quip = $cdata[int(rand($#cdata + 1))];
  }
  $quip ||= "Bugzilla would like to put a random quip here, but nobody has entered any.";
}

 
# We've done all we can without any output.  If we can server push it is time
# take down the waiting page and put up the real one.       
if ($serverpush) {
    print "\n";
    print "--thisrandomstring\n";
    print "Content-type: text/html\n";
    # Note! HTML header not yet closed
}
my $toolong = 0;
if ($::FORM{'order'}) {
    my $q = url_quote($::FORM{'order'});
    print "Set-Cookie: LASTORDER=$q ; path=/; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
}
if (length($buglist) < 4000) {
    print "Set-Cookie: BUGLIST=$buglist\n\n";
} else {
    print "Set-Cookie: BUGLIST=\n\n";
    $toolong = 1;
}
PutHeader("Bug List");


print "
<CENTER>
<B>" .  time2str("%a %b %e %T %Z %Y", time()) . "</B>";

if (defined $::FORM{'debug'}) {
    print "<P><CODE>" . value_quote($query) . "</CODE><P>\n";
}

if ($toolong) {
    print "<h2>This list is too long for bugzilla's little mind; the\n";
    print "Next/Prev/First/Last buttons won't appear.</h2>\n";
}

if (Param('usequip')){
  print "<HR><A HREF=newquip.html><I>$quip</I></A></CENTER>\n";
}
print "<HR SIZE=10>$tablestart\n";
print $::bugl;
print "</TABLE>\n";

if ($count == 0) {
    print "Zarro Boogs found.\n";
    # I've been asked to explain this ... way back when, when Netscape released
    # version 4.0 of its browser, we had a release party.  Naturally, there
    # had been a big push to try and fix every known bug before the release.
    # Naturally, that hadn't actually happened.  (This is not unique to
    # Netscape or to 4.0; the same thing has happened with every software
    # project I've ever seen.)  Anyway, at the release party, T-shirts were
    # handed out that said something like "Netscape 4.0: Zarro Boogs".
    # Just like the software, the T-shirt had no known bugs.  Uh-huh.
    #
    # So, when you query for a list of bugs, and it gets no results, you
    # can think of this as a friendly reminder.  Of *course* there are bugs
    # matching your query, they just aren't in the bugsystem yet...

    print qq{<p><A HREF="query.cgi">Query Page</A>\n};
    print qq{&nbsp;&nbsp;<A HREF="enter_bug.cgi">Enter New Bug</A>\n};
    print qq{<NOBR><A HREF="query.cgi?$::buffer">Edit this query</A></NOBR>\n};
} elsif ($count == 1) {
    print "One bug found.\n";
} else {
    print "$count bugs found.\n";
}

if ($dotweak) {
    GetVersionTable();
    print "
<SCRIPT>
numelements = document.changeform.elements.length;
function SetCheckboxes(value) {
    var item;
    for (var i=0 ; i<numelements ; i++) {
        item = document.changeform.elements\[i\];
        item.checked = value;
    }
}
document.write(\" <input type=button value=\\\"Uncheck All\\\" onclick=\\\"SetCheckboxes(false);\\\"> <input type=button value=\\\"Check All\\\" onclick=\\\"SetCheckboxes(true);\\\">\");
</SCRIPT>";
    my $resolution_popup = make_options(\@::legal_resolution_no_dup, "FIXED");
    my @prod_list = keys %prodhash;
    my @list = @prod_list;
    my @legal_versions;
    my @legal_component;
    if (1 == @prod_list) {
        @legal_versions = @{$::versions{$prod_list[0]}};
        @legal_component = @{$::components{$prod_list[0]}};
    }
    
    my $version_popup = make_options(\@legal_versions, $::dontchange);
    my $platform_popup = make_options(\@::legal_platform, $::dontchange);
    my $priority_popup = make_options(\@::legal_priority, $::dontchange);
    my $sev_popup = make_options(\@::legal_severity, $::dontchange);
    my $component_popup = make_options(\@legal_component, $::dontchange);
    my $product_popup = make_options(\@::legal_product, $::dontchange);


    print "
<hr>
<TABLE>
<TR>
    <TD ALIGN=RIGHT><B>Product:</B></TD>
    <TD><SELECT NAME=product>$product_popup</SELECT></TD>
    <TD ALIGN=RIGHT><B>Version:</B></TD>
    <TD><SELECT NAME=version>$version_popup</SELECT></TD>
<TR>
    <TD ALIGN=RIGHT><B><A HREF=\"bug_status.html#rep_platform\">Platform:</A></B></TD>
    <TD><SELECT NAME=rep_platform>$platform_popup</SELECT></TD>
    <TD ALIGN=RIGHT><B><A HREF=\"bug_status.html#priority\">Priority:</A></B></TD>
    <TD><SELECT NAME=priority>$priority_popup</SELECT></TD>
</TR>
<TR>
    <TD ALIGN=RIGHT><B>Component:</B></TD>
    <TD><SELECT NAME=component>$component_popup</SELECT></TD>
    <TD ALIGN=RIGHT><B><A HREF=\"bug_status.html#severity\">Severity:</A></B></TD>
    <TD><SELECT NAME=bug_severity>$sev_popup</SELECT></TD>
</TR>";

    if (Param("usetargetmilestone")) {
        my @legal_milestone;
        if(1 == @prod_list) {
            @legal_milestone = @{$::target_milestone{$prod_list[0]}};
        }
        my $tfm_popup = make_options(\@legal_milestone, $::dontchange);
        print "
    <TR>
    <TD ALIGN=RIGHT><B>Target milestone:</B></TD>
    <TD><SELECT NAME=target_milestone>$tfm_popup</SELECT></TD>
    </TR>";
    }

    if (Param("useqacontact")) {
        print "
<TR>
<TD><B>QA Contact:</B></TD>
<TD COLSPAN=3><INPUT NAME=qa_contact SIZE=32 VALUE=\"" .
            value_quote($::dontchange) . "\"></TD>
</TR>";
    }
        

    if (@::legal_keywords) {
        print qq{
<TR><TD><B><A HREF="describekeywords.cgi">Keywords</A>:</TD>
<TD COLSPAN=3><INPUT NAME=keywords SIZE=32 VALUE="">
<SELECT NAME="keywordaction">
<OPTION VALUE="add">Add these keywords
<OPTION VALUE="delete">Delete these keywords
<OPTION VALUE="makeexact">Make the keywords be exactly this list
</SELECT>
</TD>
</TR>
};
    }


    print "</TABLE>

<INPUT NAME=multiupdate value=Y TYPE=hidden>

<B>Additional Comments:</B>
<BR>
<TEXTAREA WRAP=HARD NAME=comment ROWS=5 COLS=80></TEXTAREA><BR>";

if ($::usergroupset ne '0' && $buggroupset =~ /^\d+$/) {
    SendSQL("select bit, description, (bit & $buggroupset != 0) from groups where bit & $::usergroupset != 0 and isbuggroup != 0 order by bit");
    while (MoreSQLData()) {
        my ($bit, $description, $ison) = (FetchSQLData());
        my $check0 = !$ison ? " SELECTED" : "";
        my $check1 = $ison ? " SELECTED" : "";
        print "<select name=bit-$bit><option value=0$check0>\n";
        print "People not in the \"$description\" group can see these bugs\n";
        print "<option value=1$check1>\n";
        print "Only people in the \"$description\" group can see these bugs\n";
        print "</select><br>\n";
    }
}




    # knum is which knob number we're generating, in javascript terms.

    my $knum = 0;
    print "
<INPUT TYPE=radio NAME=knob VALUE=none CHECKED>
        Do nothing else<br>";
    $knum++;
    if ($statushash{$::unconfirmedstate} && 1 == scalar(keys(%statushash))) {
        print "
<INPUT TYPE=radio NAME=knob VALUE=confirm>
        Confirm bugs (change status to <b>NEW</b>)<br>";
    }
    $knum++;
    print "
<INPUT TYPE=radio NAME=knob VALUE=accept>
        Accept bugs (change status to <b>ASSIGNED</b>)<br>";
    $knum++;
    if (!defined $statushash{'CLOSED'} &&
        !defined $statushash{'VERIFIED'} &&
        !defined $statushash{'RESOLVED'}) {
        print "
<INPUT TYPE=radio NAME=knob VALUE=clearresolution>
        Clear the resolution<br>";
        $knum++;
        print "
<INPUT TYPE=radio NAME=knob VALUE=resolve>
        Resolve bugs, changing <A HREF=\"bug_status.html\">resolution</A> to
        <SELECT NAME=resolution
          ONCHANGE=\"document.changeform.knob\[$knum\].checked=true\">
          $resolution_popup</SELECT><br>";
        $knum++;
    }
    if (!defined $statushash{'NEW'} &&
        !defined $statushash{'ASSIGNED'} &&
        !defined $statushash{'REOPENED'}) {
        print "
<INPUT TYPE=radio NAME=knob VALUE=reopen> Reopen bugs<br>";
        $knum++;
    }
    my @statuskeys = keys %statushash;
    if (1 == @statuskeys) {
        if (defined $statushash{'RESOLVED'}) {
            print "
<INPUT TYPE=radio NAME=knob VALUE=verify>
        Mark bugs as <b>VERIFIED</b><br>";
            $knum++;
        }
        if (defined $statushash{'VERIFIED'}) {
            print "
<INPUT TYPE=radio NAME=knob VALUE=close>
        Mark bugs as <b>CLOSED</b><br>";
            $knum++;
        }
    }
    print "
<INPUT TYPE=radio NAME=knob VALUE=reassign> 
        <A HREF=\"bug_status.html#assigned_to\">Reassign</A> bugs to
        <INPUT NAME=assigned_to SIZE=32
          ONCHANGE=\"document.changeform.knob\[$knum\].checked=true\"
          VALUE=\"$::COOKIE{'Bugzilla_login'}\"><br>";
    $knum++;
    print "<INPUT TYPE=radio NAME=knob VALUE=reassignbycomponent>
          Reassign bugs to owner of selected component<br>";
    $knum++;

    print "
<p>
<font size=-1>
To make changes to a bunch of bugs at once:
<ol>
<li> Put check boxes next to the bugs you want to change.
<li> Adjust above form elements.  (It's <b>always</b> a good idea to add some
     comment explaining what you're doing.)
<li> Click the below \"Commit\" button.
</ol></font>
<INPUT TYPE=SUBMIT VALUE=Commit>
</FORM><hr>\n";
}


if ($count > 0) {
    print "<FORM METHOD=POST ACTION=\"long_list.cgi\">
<INPUT TYPE=HIDDEN NAME=buglist VALUE=$buglist>
<INPUT TYPE=SUBMIT VALUE=\"Long Format\">
<NOBR><A HREF=\"query.cgi\">Query Page</A></NOBR>
&nbsp;&nbsp;
<NOBR><A HREF=\"enter_bug.cgi\">Enter New Bug</A></NOBR>
&nbsp;&nbsp;
<NOBR><A HREF=\"colchange.cgi?$::buffer\">Change columns</A></NOBR>";
    if (!$dotweak && $count > 1 && UserInGroup("editbugs")) {
        print "&nbsp;&nbsp;\n";
        print "<NOBR><A HREF=\"buglist.cgi?$fields$orderpart&tweak=1\">";
        print "Change several bugs at once</A></NOBR>\n";
    }
    my @owners = sort(keys(%ownerhash));
    if (@owners > 1 && UserInGroup("editbugs")) {
        my $suffix = Param('emailsuffix');
        if ($suffix ne "") {
            map(s/$/$suffix/, @owners);
        }
        my $list = join(',', @owners);
        print qq{&nbsp;&nbsp;\n};
        print qq{<NOBR><A HREF="mailto:$list">Send mail to bug owners</A></NOBR>\n};
    }
    print qq{&nbsp;&nbsp;\n};
    print qq{<NOBR><A HREF="query.cgi?$::buffer">Edit this query</A></NOBR>\n};
    print "</FORM>\n";
}
PutFooter();

if ($serverpush) {
    print "\n--thisrandomstring--\n";
}
