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

use vars @::legal_platform,
    @::versions,
    @::legal_product,
    %::MFORM,
    @::components,
    @::legal_severity,
    @::legal_priority,
    @::default_column_list,
    @::legal_resolution_no_dup,
    @::legal_target_milestone;



ConnectToDatabase();


if (!defined $::FORM{'cmdtype'}) {
    # This can happen if there's an old bookmark to a query...
    $::FORM{'cmdtype'} = 'doit';
}

CMD: for ($::FORM{'cmdtype'}) {
    /^runnamed$/ && do {
        $::buffer = $::COOKIE{"QUERY_" . $::FORM{"namedcmd"}};
        ProcessFormFields($::buffer);
        last CMD;
    };
    /^editnamed$/ && do {
	my $url = "query.cgi?" . $::COOKIE{"QUERY_" . $::FORM{"namedcmd"}};
        print "Content-type: text/html
Refresh: 0; URL=$url

<TITLE>What a hack.</TITLE>
Loading your query named <B>$::FORM{'namedcmd'}</B>...
";
        exit;
    };
    /^forgetnamed$/ && do {
        print "Set-Cookie: QUERY_" . $::FORM{'namedcmd'} . "= ; path=/ ; expires=Sun, 30-Jun-2029 00:00:00 GMT
Content-type: text/html

<HTML>
<TITLE>Forget what?</TITLE>
OK, the <B>$::FORM{'namedcmd'}</B> query is gone.
<P>
<A HREF=query.cgi>Go back to the query page.</A>
";
        exit;
    };
    /^asnamed$/ && do {
        if ($::FORM{'newqueryname'} =~ /^[a-zA-Z0-9_ ]+$/) {
    print "Set-Cookie: QUERY_" . $::FORM{'newqueryname'} . "=$::buffer ; path=/ ; expires=Sun, 30-Jun-2029 00:00:00 GMT
Content-type: text/html

<HTML>
<TITLE>OK, done.</TITLE>
OK, you now have a new query named <B>$::FORM{'newqueryname'}</B>.

<P>

<A HREF=query.cgi>Go back to the query page.</A>
";
        } else {
            print "Content-type: text/html

<HTML>
<TITLE>Picky, picky.</TITLE>
Query names can only have letters, digits, spaces, or underbars.  You entered 
\"<B>$::FORM{'newqueryname'}</B>\", which doesn't cut it.
<P>
Click the <B>Back</B> button and type in a valid name for this query.
";
        }
        exit;
    };
    /^asdefault$/ && do {
        print "Set-Cookie: DEFAULTQUERY=$::buffer ; path=/ ; expires=Sun, 30-Jun-2029 00:00:00 GMT
Content-type: text/html

<HTML>
<TITLE>OK, default is set.</TITLE>
OK, you now have a new default query.  You may also bookmark the result of any
individual query.

<P><A HREF=query.cgi>Go back to the query page, using the new default.</A>
";
        exit;
    };
}


my $serverpush = 0;
if ($ENV{'HTTP_USER_AGENT'} =~ /Mozilla.[3-9]/ && $ENV{'HTTP_USER_AGENT'} !~ /[Cc]ompatible/ ) {
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

DefCol("opendate", "date_format(bugs.creation_ts,'Y-m-d')", "Opened",
       "bugs.creation_ts");
DefCol("changeddate", "date_format(bugs.delta_ts,'Y-m-d')", "Changed",
       "bugs.delta_ts");
DefCol("severity", "substring(bugs.bug_severity, 1, 3)", "Sev",
       "bugs.bug_severity");
DefCol("priority", "substring(bugs.priority, 1, 3)", "Pri", "bugs.priority");
DefCol("platform", "substring(bugs.rep_platform, 1, 3)", "Plt",
       "bugs.rep_platform");
DefCol("owner", "assign.login_name", "Owner", "assign.login_name");
DefCol("reporter", "report.login_name", "Reporter", "report.login_name");
DefCol("qa_contact", "qacont.login_name", "QAContact", "qacont.login_name");
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

my @collist;
if (defined $::COOKIE{'COLUMNLIST'}) {
    @collist = split(/ /, $::COOKIE{'COLUMNLIST'});
} else {
    @collist = @::default_column_list;
}

my $minvotes;
my $votecolnum;
if (defined $::FORM{'votes'}) {
    my $c = trim($::FORM{'votes'});
    if ($c ne "") {
        if ($c !~ /^[0-9]*$/) {
            print "\n\n<P>The 'At least ___ votes' field must be a simple ";
            print "number. You entered \"$c\", which doesn't cut it.";
            print "<P>Please click the <B>Back</B> button and try again.\n";
            exit;
        }
        $minvotes = $c;
        if (! (grep {/^votes$/} @collist)) {
            push(@collist, 'votes');
        }
        $votecolnum = lsearch(\@collist, 'votes');
    }
}


my $dotweak = defined $::FORM{'tweak'};

if ($dotweak) {
    confirm_login();
} else {
    quietly_check_login();
}


my $query = "select bugs.bug_id, bugs.groupset";

foreach my $c (@collist) {
    if (exists $::needquote{$c}) {
        $query .= ",
\t$::key{$c}";
    }
}


if ($dotweak) {
    $query .= ",
bugs.product,
bugs.bug_status";
}


$query .= "
from   bugs,
       profiles assign,
       profiles report
       left join profiles qacont on bugs.qa_contact = qacont.userid,
       versions projector

where  bugs.assigned_to = assign.userid 
and    bugs.reporter = report.userid
and    bugs.product = projector.program
and    bugs.version = projector.value
and    bugs.groupset & $::usergroupset = bugs.groupset
";

if ((defined $::FORM{'emailcc1'} && $::FORM{'emailcc1'}) ||
    (defined $::FORM{'emailcc2'} && $::FORM{'emailcc2'})) {

    # We need to poke into the CC table.  Do weird SQL left join stuff so that
    # we can look in the CC table, but won't reject any bugs that don't have
    # any CC fields.
    $query =~ s/bugs,/bugs left join cc on bugs.bug_id = cc.bug_id left join profiles ccname on cc.who = ccname.userid,/;
}

if (defined $::FORM{'sql'}) {
  $query .= "and (\n$::FORM{'sql'}\n)"
} else {
  my @legal_fields = ("bug_id", "product", "version", "rep_platform", "op_sys",
                      "bug_status", "resolution", "priority", "bug_severity",
                      "assigned_to", "reporter", "component",
                      "target_milestone", "groupset");

  foreach my $field (keys %::FORM) {
      my $or = "";
      if (lsearch(\@legal_fields, $field) != -1 && $::FORM{$field} ne "") {
          $query .= "\tand (\n";
          if ($field eq "assigned_to" || $field eq "reporter") {
              foreach my $p (split(/,/, $::FORM{$field})) {
                  my $whoid = DBNameToIdAndCheck($p);
                  $query .= "\t\t${or}bugs.$field = $whoid\n";
                  $or = "or ";
              }
          } else {
              my $ref = $::MFORM{$field};
              foreach my $v (@$ref) {
                  if ($v eq "(empty)") {
                      $query .= "\t\t${or}bugs.$field is null\n";
                  } else {
                      if ($v eq "---") {
                          $query .= "\t\t${or}bugs.$field = ''\n";
                      } else {
                          $query .= "\t\t${or}bugs.$field = " . SqlQuote($v) .
                              "\n";
                      }
                  }
                  $or = "or ";
              }
          }
          $query .= "\t)\n";
      }
  }
}

if ($::FORM{'keywords'}) {
    GetVersionTable();
    my @list;
    foreach my $v (split(',', $::FORM{'keywords'})) {
        my $id = $::keywordsbyname{trim($v)};
        if ($id) {
            push(@list, "keywords.keywordid = $id");
        } else {
            print "Unknown keyword named <code>$v</code>.\n";
            print "<P>The legal keyword names are <A HREF=describekeywords.cgi>";
            print "listed here</A>.\n";
            print "<P>Please click the <B>Back</B> button and try again.\n";
            exit;
        }
    }
    if (@list) {
        $query =~ s/where/, keywords where/;
        $query .= "and keywords.bug_id = bugs.bug_id and (" .
            join(" $::FORM{'keywords_type'} ", @list) . ")\n";
    }
}


foreach my $id ("1", "2") {
    if (!defined ($::FORM{"email$id"})) {
        next;
    }
    my $email = trim($::FORM{"email$id"});
    if ($email eq "") {
        next;
    }
    my $qemail = SqlQuote($email); 
    my $type = $::FORM{"emailtype$id"};
    my $emailid;
    if ($type eq "exact") {
        $emailid = DBNameToIdAndCheck($email);
    }

    my $foundone = 0;
    my $lead= "and (\n";
    foreach my $field ("assigned_to", "reporter", "cc", "qa_contact") {
        my $doit = $::FORM{"email$field$id"};
        if (!$doit) {
            next;
        }
        $foundone = 1;
        my $table;
        if ($field eq "assigned_to") {
            $table = "assign";
        } elsif ($field eq "reporter") {
            $table = "report";
        } elsif ($field eq "qa_contact") {
            $table = "qacont";
        } else {
            $table = "ccname";
        }
        if ($type eq "exact") {
            if ($field eq "cc") {
                $query .= "\t$lead cc.who = $emailid\n";
            } else {
                $query .= "\t$lead $field = $emailid\n";
            }
        } elsif ($type eq "regexp") {
            $query .= "\t$lead $table.login_name regexp $qemail\n";
        } elsif ($type eq "notregexp") {
            $query .= "\t$lead $table.login_name not regexp $qemail\n";
        } else {
            $query .= "\t$lead instr($table.login_name, $qemail)\n";
        }
        $lead = " or ";
    }
    if (!$foundone) {
        print "\n\n<P>You must specify one or more fields in which to search for <tt>$email</tt>.\n";
        print "<P>Please click the <B>Back</B> button and try again.\n";
        exit;
    }
    if ($lead eq " or ") {
        $query .= ")\n";
    }
}
                




if (defined $::FORM{'changedin'}) {
    my $c = trim($::FORM{'changedin'});
    if ($c ne "") {
        if ($c !~ /^[0-9]*$/) {
            print "\n\n<P>The 'changed in last ___ days' field must be a simple ";
            print "number. You entered \"$c\", which doesn't cut it.";
            print "<P>Please click the <B>Back</B> button and try again.\n";
            exit;
        }
        $query .= "and to_days(now()) - to_days(bugs.delta_ts) <= $c ";
    }
}

if (defined $minvotes) {
    $query .= "and votes >= $minvotes ";
}


my $ref = $::MFORM{'chfield'};


sub SqlifyDate {
    my ($str) = (@_);
    if (!defined $str) {
        $str = "";
    }
    my $date = str2time($str);
    if (!defined $date) {
        print "\n\n<P>The string '<tt>$str</tt>' is not a legal date.\n";
        print "<P>Please click the <B>Back</B> button and try again.\n";
        exit;
    }
    return time2str("'%Y/%m/%d %H:%M:%S'", $date);
}


if (defined $ref) {
    my $which = lsearch($ref, "[Bug creation]");
    if ($which >= 0) {
        splice(@$ref, $which, 1);
        $query .= "and bugs.creation_ts >= " .
            SqlifyDate($::FORM{'chfieldfrom'}) . "\n";
        my $to = $::FORM{'chfieldto'};
        if (defined $to) {
            $to = trim($to);
            if ($to ne "" && $to !~ /^now$/i) {
                $query .= "and bugs.creation_ts <= " .
                    SqlifyDate($to) . "\n";
            }
        }
    }
}        



if (defined $ref && 0 < @$ref) {
    # Do surgery on the query to tell it to patch in the bugs_activity
    # table.
    $query =~ s/where/, bugs_activity where/;
    
    my @list;
    foreach my $f (@$ref) {
        push(@list, "\nbugs_activity.field = " . SqlQuote($f));
    }
    $query .= "and bugs_activity.bug_id = bugs.bug_id and (" .
        join(' or ', @list) . ") ";
    $query .= "and bugs_activity.bug_when >= " .
        SqlifyDate($::FORM{'chfieldfrom'}) . "\n";
    my $to = $::FORM{'chfieldto'};
    if (defined $to) {
        $to = trim($to);
        if ($to ne "" && $to !~ /^now$/i) {
            $query .= "and bugs_activity.bug_when <= " . SqlifyDate($to) . "\n";
        }
    }
    my $value = $::FORM{'chfieldvalue'};
    if (defined $value) {
        $value = trim($value);
        if ($value ne "") {
            $query .= "and bugs_activity.newvalue = " .
                SqlQuote($value) . "\n";
        }
    }
}

foreach my $f ("short_desc", "long_desc", "bug_file_loc",
               "status_whiteboard") {
    if (defined $::FORM{$f}) {
        my $s = trim($::FORM{$f});
        if ($s ne "") {
            $s = SqlQuote($s);
            if ($::FORM{$f . "_type"} eq "regexp") {
                $query .= "and $f regexp $s\n";
            } elsif ($::FORM{$f . "_type"} eq "notregexp") {
                $query .= "and $f not regexp $s\n";
            } elsif ($::FORM{$f . "_type"} eq "casesubstring") {
                $query .= "and instr($f, $s)\n";
            } else {
                $query .= "and instr(lower($f), lower($s))\n";
            }
        }
    }
}


$query .= "group by bugs.bug_id\n";

if (defined $::FORM{'order'} && $::FORM{'order'} ne "") {
    $query .= "order by ";
    $::FORM{'order'} =~ s/votesum/bugs.votes/; # Silly backwards compatability
                                               # hack.
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
            $::FORM{'order'} = "assign.login_name, bugs.bug_status, priority, bugs.bug_id";
            last ORDER;
        };
        # DEFAULT
        $::FORM{'order'} = "bugs.bug_status, bugs.priority, assign.login_name, bugs.bug_id";
    }
    $query .= $::FORM{'order'};
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

my $tablestart = "<TABLE CELLSPACING=0 CELLPADDING=2>
<TR ALIGN=LEFT><TH>
<A HREF=\"buglist.cgi?$fields&order=bugs.bug_id\">ID</A>";


foreach my $c (@collist) {
    if (exists $::needquote{$c}) {
        if ($::needquote{$c}) {
            $tablestart .= "<TH WIDTH=100% valign=left>";
        } else {
            $tablestart .= "<TH valign=left>";
        }
        if (defined $::sortkey{$c}) {
            $tablestart .= "<A HREF=\"buglist.cgi?$fields&order=" . url_quote($::sortkey{$c}) . "$oldorder\">$::title{$c}</A>";
        } else {
            $tablestart .= $::title{$c};
        }
    }
}

$tablestart .= "\n";


my @row;
my %seen;
my @bugarray;
my %prodhash;
my %statushash;
my $buggroupset = "";

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
        pnl "<TR VALIGN=TOP ALIGN=LEFT><TD>";
        if ($dotweak) {
            pnl "<input type=checkbox name=id_$bug_id>";
        }
        pnl "<A HREF=\"show_bug.cgi?id=$bug_id\">";
        pnl "$bug_id</A> ";
        foreach my $c (@collist) {
            if (!exists $::needquote{$c}) {
                next;
            }
            my $value = shift @row;
            if (!defined $value) {
                next;
            }
            if ($::needquote{$c}) {
                $value = html_quote($value);
            } else {
                $value = "<nobr>$value</nobr>";
            }
            pnl "<td>$value";
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
    print "<PRE>$query</PRE>\n";
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
        push(@::legal_target_milestone, " ");
        my $tfm_popup = make_options(\@::legal_target_milestone,
                                     $::dontchange);
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
<A HREF=\"query.cgi\">Query Page</A>
&nbsp;&nbsp;<A HREF=\"enter_bug.cgi\">Enter New Bug</A>
&nbsp;&nbsp;<A HREF=\"colchange.cgi?$::buffer\">Change columns</A>";
    if (!$dotweak && $count > 1) {
        print "&nbsp;&nbsp;<A HREF=\"buglist.cgi?$fields$orderpart&tweak=1\">";
        print "Change several bugs at once</A>\n";
    }
    print "</FORM>\n";
}
if ($serverpush) {
    print "\n--thisrandomstring--\n";
}
