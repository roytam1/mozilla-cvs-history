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
# Contributor(s): Gervase Markham <gerv@gerv.net>
#                 Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Stephan Niemz <st.n@gmx.net>
#                 Andreas Franke <afranke@mathweb.org>
#                 Myk Melez <myk@mozilla.org>
#                 Michael Schindler <michael@compressconsult.com>

use strict;

# The caller MUST require CGI.pl and globals.pl before using this

use vars qw($userid);

package Bugzilla::Search;

use Bugzilla::Config;
use Bugzilla::Util;

use Date::Format;
use Date::Parse;

# Create a new Search
sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
  
    my $self = { @_ };  
    bless($self, $class);
    
    $self->init();
    
    return $self;
}

sub init {
    my $self = shift;
    my $fieldsref = $self->{'fields'};
    my $urlstr = $self->{'url'};

    my $debug = 0;
        
    my @fields;
    my @supptables;
    my @wherepart;
    @fields = @$fieldsref if $fieldsref;
    my %F;
    my %M;
    &::ParseUrlString($urlstr, \%F, \%M);
    my @specialchart;
    my @andlist;

    # First, deal with all the old hard-coded non-chart-based poop.
    if (lsearch($fieldsref, 'map_assigned_to.login_name') >= 0) {
        push @supptables, "profiles AS map_assigned_to";
        push @wherepart, "bugs.assigned_to = map_assigned_to.userid";
    }

    if (lsearch($fieldsref, 'map_reporter.login_name') >= 0) {
        push @supptables, "profiles AS map_reporter";
        push @wherepart, "bugs.reporter = map_reporter.userid";
    }

    if (lsearch($fieldsref, 'map_qa_contact.login_name') >= 0) {
        push @supptables, "LEFT JOIN profiles map_qa_contact ON bugs.qa_contact = map_qa_contact.userid";
    }

    if (lsearch($fieldsref, 'map_products.name') >= 0) {
        push @supptables, "products AS map_products";
        push @wherepart, "bugs.product_id = map_products.id";
    }

    if (lsearch($fieldsref, 'map_components.name') >= 0) {
        push @supptables, "components AS map_components";
        push @wherepart, "bugs.component_id = map_components.id";
    }

    my $minvotes;
    if (defined $F{'votes'}) {
        my $c = trim($F{'votes'});
        if ($c ne "") {
            if ($c !~ /^[0-9]*$/) {
                $::vars->{'value'} = $c;
                &::ThrowUserError("illegal_at_least_x_votes");
            }
            push(@specialchart, ["votes", "greaterthan", $c - 1]);
        }
    }

    if ($M{'bug_id'}) {
        my $type = "anyexact";
        if ($F{'bugidtype'} && $F{'bugidtype'} eq 'exclude') {
            $type = "nowords";
        }
        push(@specialchart, ["bug_id", $type, join(',', @{$M{'bug_id'}})]);
    }

    my @legal_fields = ("product", "version", "rep_platform", "op_sys",
                        "bug_status", "resolution", "priority", "bug_severity",
                        "assigned_to", "reporter", "component",
                        "target_milestone", "bug_group");

    foreach my $field (keys %F) {
        if (lsearch(\@legal_fields, $field) != -1) {
            push(@specialchart, [$field, "anyexact",
                                 join(',', @{$M{$field}})]);
        }
    }

    if ($F{'product'}) {
        push(@supptables, "products products_");
        push(@wherepart, "products_.id = bugs.product_id");
        push(@specialchart, ["products_.name", "anyexact",
                             join(',',@{$M{'product'}})]);
    }

    if ($F{'component'}) {
        push(@supptables, "components components_");
        push(@wherepart, "components_.id = bugs.component_id");
        push(@specialchart, ["components_.name", "anyexact",
                             join(',',@{$M{'component'}})]);
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
                    &::DBNameToIdAndCheck($name);
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
            push(@supptables, "profiles $ptable");
            push(@wherepart, "$table.who = $ptable.userid");
            push(@clist, "$ptable.login_name", $type, $email);
        }
        if (@clist) {
            push(@specialchart, \@clist);
        } else {
            $::vars->{'email'} = $email;
            &::ThrowUserError("missing_email_type");
        }
    }


    if (defined $F{'changedin'}) {
        my $c = trim($F{'changedin'});
        if ($c ne "") {
            if ($c !~ /^[0-9]*$/) {
                $::vars->{'value'} = $c;
                &::ThrowUserError("illegal_changed_in_last_x_days");
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
            push(@list, "\nactcheck.fieldid = " . &::GetFieldID($f));
        }
        push(@wherepart, "actcheck.bug_id = bugs.bug_id");
        push(@wherepart, "(" . join(' OR ', @list) . ")");
        push(@wherepart, "actcheck.bug_when >= " .
             &::SqlQuote(SqlifyDate($F{'chfieldfrom'})));
        my $to = $F{'chfieldto'};
        if (defined $to) {
            $to = trim($to);
            if ($to ne "" && $to !~ /^now$/i) {
                push(@wherepart, "actcheck.bug_when <= " .
                     &::SqlQuote(SqlifyDate($to)));
            }
        }
        my $value = $F{'chfieldvalue'};
        if (defined $value) {
            $value = trim($value);
            if ($value ne "") {
                push(@wherepart, "actcheck.added = " .
                     &::SqlQuote($value))
            }
        }
    }

    foreach my $f ("short_desc", "long_desc", "bug_file_loc",
                   "status_whiteboard") {
        if (defined $F{$f}) {
            my $s = trim($F{$f});
            if ($s ne "") {
                my $n = $f;
                my $q = &::SqlQuote($s);
                my $type = $F{$f . "_type"};
                push(@specialchart, [$f, $type, $s]);
            }
        }
    }

    my $chartid;
    # $statusid is used by the code that queries for attachment statuses.
    my $statusid = 0;
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
             push(@supptables, "profiles AS map_$f");
             push(@wherepart, "bugs.$f = map_$f.userid");
             $f = "map_$f.login_name";
         },
         "^qa_contact," => sub {
             push(@supptables,
                  "LEFT JOIN profiles map_qa_contact ON bugs.qa_contact = map_qa_contact.userid");
             $f = "map_$f.login_name";
         },

         "^cc," => sub {
            push(@supptables, "LEFT JOIN cc cc_$chartid ON bugs.bug_id = cc_$chartid.bug_id");

            push(@supptables, "LEFT JOIN profiles map_cc_$chartid ON cc_$chartid.who = map_cc_$chartid.userid");
            $f = "map_cc_$chartid.login_name";
         },

         "^long_?desc,changedby" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "longdescs $table");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             my $id = &::DBNameToIdAndCheck($v);
             $term = "$table.who = $id";
         },
         "^long_?desc,changedbefore" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "longdescs $table");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             $term = "$table.bug_when < " . &::SqlQuote(SqlifyDate($v));
         },
         "^long_?desc,changedafter" => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "longdescs $table");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             $term = "$table.bug_when > " . &::SqlQuote(SqlifyDate($v));
         },
         "^long_?desc," => sub {
             my $table = "longdescs_$chartid";
             push(@supptables, "longdescs $table");
             if (Param("insidergroup") && !&::UserInGroup(Param("insidergroup"))) {
                 push(@wherepart, "$table.isprivate < 1") ;
             }
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             $f = "$table.thetext";
         },
         "^bug_group,(?!changed)" => sub {
            push(@supptables, "LEFT JOIN bug_group_map bug_group_map_$chartid ON bugs.bug_id = bug_group_map_$chartid.bug_id");

            push(@supptables, "LEFT JOIN groups groups_$chartid ON groups_$chartid.id = bug_group_map_$chartid.group_id");
            $f = "groups_$chartid.name";
         },
         "^attachments\..*," => sub {
             my $table = "attachments_$chartid";
             push(@supptables, "attachments $table");
             push(@wherepart, "bugs.bug_id = $table.bug_id");
             $f =~ m/^attachments\.(.*)$/;
             my $field = $1;
             if ($t eq "changedby") {
                 $v = &::DBNameToIdAndCheck($v);
                 $q = &::SqlQuote($v);
                 $field = "submitter_id";
                 $t = "equals";
             } elsif ($t eq "changedbefore") {
                 $v = SqlifyDate($v);
                 $q = &::SqlQuote($v);
                 $field = "creation_ts";
                 $t = "lessthan";
             } elsif ($t eq "changedafter") {
                 $v = SqlifyDate($v);
                 $q = &::SqlQuote($v);
                 $field = "creation_ts";
                 $t = "greaterthan";
             }
             if ($field eq "ispatch" && $v ne "0" && $v ne "1") {
                 &::ThrowUserError("illegal_attachment_is_patch");
             }
             if ($field eq "isobsolete" && $v ne "0" && $v ne "1") {
                 &::ThrowUserError("illegal_is_obsolete");
             }
             $f = "$table.$field";
         },
         "^attachstatusdefs.name," => sub {
             # The below has Fun with the names for attachment statuses. This
             # isn't needed for changed* queries, so exclude those - the
             # generic stuff will cope
             return if ($t =~ m/^changed/);

             # Searching for "status != 'bar'" wants us to look for an
             # attachment without the 'bar' status, not for an attachment with
             # a status not equal to 'bar' (Which would pick up an attachment
             # with more than one status). We do this by LEFT JOINS, after
             # grabbing the matching attachment status ids.
             # Note that this still won't find bugs with no attachments, since
             # that isn't really what people would expect.

             # First, get the attachment status ids, using the other funcs
             # to match the WHERE term.
             # Note that we need to reverse the negated bits for this to work
             # This somewhat abuses the definitions of the various terms -
             # eg, does 'contains all' mean that the status has to contain all
             # those words, or that all those words must be exact matches to
             # statuses, which must all be on a single attachment, or should
             # the match on the status descriptions be a contains match, too?

             my $inverted = 0;
             if ($t =~ m/not(.*)/) {
                 $t = $1;
                 $inverted = 1;
             }

             $ref = $funcsbykey{",$t"};
             &$ref;
             &::SendSQL("SELECT id FROM attachstatusdefs WHERE $term");

             my @as_ids;
             while (&::MoreSQLData()) {
                 push @as_ids, &::FetchOneColumn();
             }

             # When searching for multiple statuses within a single boolean chart,
             # we want to match each status record separately.  In other words,
             # "status = 'foo' AND status = 'bar'" should match attachments with
             # one status record equal to "foo" and another one equal to "bar",
             # not attachments where the same status record equals both "foo" and
             # "bar" (which is nonsensical).  In order to do this we must add an
             # additional counter to the end of the "attachstatuses" table
             # reference.
             ++$statusid;

             my $attachtable = "attachments_$chartid";
             my $statustable = "attachstatuses_${chartid}_$statusid";

             push(@supptables, "attachments $attachtable");
             my $join = "LEFT JOIN attachstatuses $statustable ON ".
               "($attachtable.attach_id = $statustable.attach_id AND " .
                "$statustable.statusid IN (" . join(",", @as_ids) . "))";
             push(@supptables, $join);
             push(@wherepart, "bugs.bug_id = $attachtable.bug_id");
             if ($inverted) {
                 $term = "$statustable.statusid IS NULL";
             } else {
                 $term = "$statustable.statusid IS NOT NULL";
             }
         },
         "^changedin," => sub {
             $f = "(to_days(now()) - to_days(bugs.delta_ts))";
         },

         "^component,(?!changed)" => sub {
             my $table = "components_$chartid";
             push(@supptables, "components $table");
             push(@wherepart, "bugs.component_id = $table.id");
             $f = $ff = "$table.name";
         },

         "^product,(?!changed)" => sub {
             my $table = "products_$chartid";
             push(@supptables, "products $table");
             push(@wherepart, "bugs.product_id = $table.id");
             $f = $ff = "$table.name";
         },

         "^keywords," => sub {
             &::GetVersionTable();
             my @list;
             my $table = "keywords_$chartid";
             foreach my $value (split(/[\s,]+/, $v)) {
                 if ($value eq '') {
                     next;
                 }
                 my $id = &::GetKeywordIdFromName($value);
                 if ($id) {
                     push(@list, "$table.keywordid = $id");
                 }
                 else {
                     $::vars->{'keyword'} = $v;
                     &::ThrowUserError("unknown_keyword");
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

         "^dependson," => sub {
                my $table = "dependson_" . $chartid;
                push(@supptables, "dependencies $table");
                $ff = "$table.$f";
                $ref = $funcsbykey{",$t"};
                &$ref;
                push(@wherepart, "$table.blocked = bugs.bug_id");
         },

         "^blocked," => sub {
                my $table = "blocked_" . $chartid;
                push(@supptables, "dependencies $table");
                $ff = "$table.$f";
                $ref = $funcsbykey{",$t"};
                &$ref;
                push(@wherepart, "$table.dependson = bugs.bug_id");
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
                 push(@list, "$ff = " . &::SqlQuote($w));
             }
             $term = join(" OR ", @list);
         },
         ",anywordssubstr" => sub {
             $term = join(" OR ", @{GetByWordListSubstr($ff, $v)});
         },
         ",allwordssubstr" => sub {
             $term = join(" AND ", @{GetByWordListSubstr($ff, $v)});
         },
         ",nowordssubstr" => sub {
             my @list = @{GetByWordListSubstr($ff, $v)};
             if (@list) {
                 $term = "NOT (" . join(" OR ", @list) . ")";
             }
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
         ",changedfrom" => sub {
             my $table = "act_$chartid";
             my $ftable = "fielddefs_$chartid";
             push(@supptables, "bugs_activity $table");
             push(@supptables, "fielddefs $ftable");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             push(@wherepart, "$table.fieldid = $ftable.fieldid");
             $term = "($ftable.name = '$f' AND $table.removed = $q)";
         },
         ",changedto" => sub {
             my $table = "act_$chartid";
             my $ftable = "fielddefs_$chartid";
             push(@supptables, "bugs_activity $table");
             push(@supptables, "fielddefs $ftable");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             push(@wherepart, "$table.fieldid = $ftable.fieldid");
             $term = "($ftable.name = '$f' AND $table.added = $q)";
         },
         ",changedby" => sub {
             my $table = "act_$chartid";
             my $ftable = "fielddefs_$chartid";
             push(@supptables, "bugs_activity $table");
             push(@supptables, "fielddefs $ftable");
             push(@wherepart, "$table.bug_id = bugs.bug_id");
             push(@wherepart, "$table.fieldid = $ftable.fieldid");
             my $id = &::DBNameToIdAndCheck($v);
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

    # first we delete any sign of "Chart #-1" from the HTML form hash
    # since we want to guarantee the user didn't hide something here
    my @badcharts = grep /^(field|type|value)-1-/, (keys %F);
    foreach my $field (@badcharts) {
        delete $F{$field};
    }

    # now we take our special chart and stuff it into the form hash
    my $chart = -1;
    my $row = 0;
    foreach my $ref (@specialchart) {
        my $col = 0;
        while (@$ref) {
            $F{"field$chart-$row-$col"} = shift(@$ref);
            $F{"type$chart-$row-$col"} = shift(@$ref);
            $F{"value$chart-$row-$col"} = shift(@$ref);
            if ($debug) {
                print qq{<p>$F{"field$chart-$row-$col"} | $F{"type$chart-$row-$col"} | $F{"value$chart-$row-$col"}*</p>\n};
            }
            $col++;

        }
        $row++;
    }


# A boolean chart is a way of representing the terms in a logical
# expression.  Bugzilla builds SQL queries depending on how you enter
# terms into the boolean chart. Boolean charts are represented in
# urls as tree-tuples of (chart id, row, column). The query form
# (query.cgi) may contain an arbitrary number of boolean charts where
# each chart represents a clause in a SQL query.
#
# The query form starts out with one boolean chart containing one
# row and one column.  Extra rows can be created by pressing the
# AND button at the bottom of the chart.  Extra columns are created
# by pressing the OR button at the right end of the chart. Extra
# charts are created by pressing "Add another boolean chart".
#
# Each chart consists of an artibrary number of rows and columns.
# The terms within a row are ORed together. The expressions represented
# by each row are ANDed together. The expressions represented by each
# chart are ANDed together.
#
#        ----------------------
#        | col2 | col2 | col3 |
# --------------|------|------|
# | row1 |  a1  |  a2  |      |
# |------|------|------|------|  => ((a1 OR a2) AND (b1 OR b2 OR b3) AND (c1))
# | row2 |  b1  |  b2  |  b3  |
# |------|------|------|------|
# | row3 |  c1  |      |      |
# -----------------------------
#
#        --------
#        | col2 |
# --------------|
# | row1 |  d1  | => (d1)
# ---------------
#
# Together, these two charts represent a SQL expression like this
# SELECT blah FROM blah WHERE ( (a1 OR a2)AND(b1 OR b2 OR b3)AND(c1)) AND (d1)
#
# The terms within a single row of a boolean chart are all constraints
# on a single piece of data.  If you're looking for a bug that has two
# different people cc'd on it, then you need to use two boolean charts.
# This will find bugs with one CC mathing 'foo@blah.org' and and another
# CC matching 'bar@blah.org'.
#
# --------------------------------------------------------------
# CC    | equal to
# foo@blah.org
# --------------------------------------------------------------
# CC    | equal to
# bar@blah.org
#
# If you try to do this query by pressing the AND button in the
# original boolean chart then what you'll get is an expression that
# looks for a single CC where the login name is both "foo@blah.org",
# and "bar@blah.org". This is impossible.
#
# --------------------------------------------------------------
# CC    | equal to
# foo@blah.org
# AND
# CC    | equal to
# bar@blah.org
# --------------------------------------------------------------

# $chartid is the number of the current chart whose SQL we're contructing
# $row is the current row of the current chart

# names for table aliases are constructed using $chartid and $row
#   SELECT blah  FROM $table "$table_$chartid_$row" WHERE ....

# $f  = field of table in bug db (e.g. bug_id, reporter, etc)
# $ff = qualified field name (field name prefixed by table)
#       e.g. bugs_activity.bug_id
# $t  = type of query. e.g. "equal to", "changed after", case sensitive substr"
# $v  = value - value the user typed in to the form
# $q  = sanitized version of user input (SqlQuote($v))
# @supptables = Tables and/or table aliases used in query
# %suppseen   = A hash used to store all the tables in supptables to weed
#               out duplicates.
# $suppstring = String which is pasted into query containing all table names

    # get a list of field names to verify the user-submitted chart fields against
    my %chartfields;
    &::SendSQL("SELECT name FROM fielddefs");
    while (&::MoreSQLData()) {
        my ($name) = &::FetchSQLData();
        $chartfields{$name} = 1;
    }

    $row = 0;
    for ($chart=-1 ;
         $chart < 0 || exists $F{"field$chart-0-0"} ;
         $chart++) {
        $chartid = $chart >= 0 ? $chart : "";
        for ($row = 0 ;
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
                # chart -1 is generated by other code above, not from the user-
                # submitted form, so we'll blindly accept any values in chart -1
                if ((!$chartfields{$f}) && ($chart != -1)) {
                    my $errstr = "Can't use $f as a field name.  " .
                        "If you think you're getting this in error, please copy the " .
                        "entire URL out of the address bar at the top of your browser " .
                        "window and email it to <109679\@bugzilla.org>";
                    die "Internal error: $errstr" if $chart < 0;
                    return &::DisplayError($errstr);
                }

                # This is either from the internal chart (in which case we
                # already know about it), or it was in %chartfields, so it is
                # a valid field name, which means that its ok.
                trick_taint($f);
                $q = &::SqlQuote($v);
                my $func;
                $term = undef;
                foreach my $key (@funcnames) {
                    if ("$f,$t" =~ m/$key/) {
                        my $ref = $funcsbykey{$key};
                        if ($debug) {
                            print "<p>$key ($f , $t ) => ";
                        }
                        $ff = $f;
                        if ($f !~ /\./) {
                            $ff = "bugs.$f";
                        }
                        &$ref;
                        if ($debug) {
                            print "$f , $t , $term</p>";
                        }
                        if ($term) {
                            last;
                        }
                    }
                }
                if ($term) {
                    push(@orlist, $term);
                }
                else {
                    # This field and this type don't work together.
                    $::vars->{'field'} = $F{"field$chart-$row-$col"};
                    $::vars->{'type'} = $F{"type$chart-$row-$col"};
                    &::ThrowCodeError("field_type_mismatch");
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
            if ($str !~ /^(LEFT|INNER) JOIN/i) {
                $suppstring .= ",";
            }
            $suppstring .= " $str";
            $suppseen{$str} = 1;
        }
    }
    my $query =  ("SELECT DISTINCT " . 
                    join(', ', @fields) .
                  ", COUNT(DISTINCT ugmap.group_id) AS cntuseringroups, " .
                  " COUNT(DISTINCT bgmap.group_id) AS cntbugingroups, " .
                  " ((COUNT(DISTINCT ccmap.who) AND cclist_accessible) " .
                  "  OR ((bugs.reporter = $::userid) AND bugs.reporter_accessible) " .
                  "  OR bugs.assigned_to = $::userid ) AS canseeanyway " .
                  " FROM $suppstring" .
                  " LEFT JOIN bug_group_map AS bgmap " .
                  " ON bgmap.bug_id = bugs.bug_id " .
                  " LEFT JOIN user_group_map AS ugmap " .
                  " ON bgmap.group_id = ugmap.group_id " .
                  " AND ugmap.user_id = $::userid " .
                  " AND ugmap.isbless = 0" .
                  " LEFT JOIN cc AS ccmap " .
                  " ON ccmap.who = $::userid AND ccmap.bug_id = bugs.bug_id " .
                  " WHERE " . join(' AND ', (@wherepart, @andlist)) .
                  " GROUP BY bugs.bug_id " .
                  " HAVING cntuseringroups = cntbugingroups" .
                  " OR canseeanyway" 
              );

    if ($debug) {
        print "<p><code>" . value_quote($query) . "</code></p>\n";
        exit;
    }
    
    $self->{'sql'} = $query;
}

###############################################################################
# Helper functions for the init() method.
###############################################################################
sub SqlifyDate {
    my ($str) = @_;
    $str = "" if !defined $str;
    if ($str =~ /^-?(\d+)([dDwWmMyY])$/) {   # relative date
        my ($amount, $unit, $date) = ($1, lc $2, time);
        my ($sec, $min, $hour, $mday, $month, $year, $wday)  = localtime($date);
        if ($unit eq 'w') {                  # convert weeks to days
            $amount = 7*$amount + $wday;
            $unit = 'd';
        }
        if ($unit eq 'd') {
            $date -= $sec + 60*$min + 3600*$hour + 24*3600*$amount;
            return time2str("%Y-%m-%d %H:%M:%S", $date);
        }
        elsif ($unit eq 'y') {
            return sprintf("%4d-01-01 00:00:00", $year+1900-$amount);
        }
        elsif ($unit eq 'm') {
            $month -= $amount;
            while ($month<0) { $year--; $month += 12; }
            return sprintf("%4d-%02d-01 00:00:00", $year+1900, $month+1);
        }
        return undef;                      # should not happen due to regexp at top
    }
    my $date = str2time($str);
    if (!defined($date)) {
        $::vars->{'date'} = $str;
        ThrowUserError("illegal_date");
    }
    return time2str("%Y-%m-%d %H:%M:%S", $date);
}

sub GetByWordList {
    my ($field, $strs) = (@_);
    my @list;

    foreach my $w (split(/[\s,]+/, $strs)) {
        my $word = $w;
        if ($word ne "") {
            $word =~ tr/A-Z/a-z/;
            $word = &::SqlQuote(quotemeta($word));
            $word =~ s/^'//;
            $word =~ s/'$//;
            $word = '(^|[^a-z0-9])' . $word . '($|[^a-z0-9])';
            push(@list, "lower($field) regexp '$word'");
        }
    }

    return \@list;
}

# Support for "any/all/nowordssubstr" comparison type ("words as substrings")
sub GetByWordListSubstr {
    my ($field, $strs) = (@_);
    my @list;

    foreach my $word (split(/[\s,]+/, $strs)) {
        if ($word ne "") {
            push(@list, "INSTR(LOWER($field), " . lc(&::SqlQuote($word)) . ")");
        }
    }

    return \@list;
}

sub getSQL {
    my $self = shift;
    return $self->{'sql'};
}

1;
