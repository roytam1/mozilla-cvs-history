#!/usr/bonsaitools/bin/perl -wT
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
#                 Stephan Niemz  <st.n@gmx.net>
#                 Andreas Franke <afranke@mathweb.org>
#                 Myk Melez <myk@mozilla.org>

################################################################################
# Script Initialization
################################################################################

# Make it harder for us to do dangerous things in Perl.
use strict;

use lib qw(.);

use vars qw($template $vars);

use Bugzilla::Search;

# Include the Bugzilla CGI and general utility library.
require "CGI.pl";

use vars qw($db_name
            @components
            @default_column_list
            $defaultqueryname
            @dontchange
            @legal_keywords
            @legal_platform
            @legal_priority
            @legal_product
            @legal_severity
            @settable_resolution
            @target_milestone
            $unconfirmedstate
            $userid
            @versions);

if (length($::buffer) == 0) {
    print "Refresh: 10; URL=query.cgi\n";
    print "Content-Type: text/html\n\n";
    $vars->{'header_done'} = 1;
    ThrowUserError("buglist_parameters_required");
}    

ConnectToDatabase();

################################################################################
# Data and Security Validation
################################################################################

# Whether or not the user wants to change multiple bugs.
my $dotweak = $::FORM{'tweak'} ? 1 : 0;

# Log the user in
if ($dotweak) {
    confirm_login();
    if (!UserInGroup("editbugs")) {
        DisplayError("Sorry, you do not have sufficient privileges to edit
                      multiple bugs.");
        exit;
    }
    GetVersionTable();
}
else {
    quietly_check_login();
}

# Determine the format in which the user would like to receive the output.
# Uses the default format if the user did not specify an output format;
# otherwise validates the user's choice against the list of available formats.
my $format = GetFormat("list/list", $::FORM{'format'}, $::FORM{'ctype'});

# Use server push to display a "Please wait..." message for the user while
# executing their query if their browser supports it and they are viewing
# the bug list as HTML and they have not disabled it by adding &serverpush=0
# to the URL.
#
# Server push is a Netscape 3+ hack incompatible with MSIE, Lynx, and others. 
# Even Communicator 4.51 has bugs with it, especially during page reload.
# http://www.browsercaps.org used as source of compatible browsers.
#
my $serverpush =
  exists $ENV{'HTTP_USER_AGENT'} 
    && $ENV{'HTTP_USER_AGENT'} =~ /Mozilla.[3-9]/ 
      && $ENV{'HTTP_USER_AGENT'} !~ /[Cc]ompatible/
        && $format->{'extension'} eq "html"
          && !defined($::FORM{'serverpush'})
            || $::FORM{'serverpush'};

my $order = $::FORM{'order'} || "";
my $order_from_cookie = 0;  # True if $order set using $::COOKIE{'LASTORDER'}

# If the user is retrieving the last bug list they looked at, hack the buffer
# storing the query string so that it looks like a query retrieving those bugs.
if ($::FORM{'regetlastlist'}) {
    if (!$::COOKIE{'BUGLIST'}) {
        DisplayError(qq|Sorry, I seem to have lost the cookie that recorded
                        the results of your last query.  You will have to start
                        over at the <a href="query.cgi">query page</a>.|);
        exit;
    }
    $::FORM{'bug_id'} = join(",", split(/:/, $::COOKIE{'BUGLIST'}));
    $order = "reuse last sort" unless $order;
    $::buffer = "bug_id=$::FORM{'bug_id'}&order=" . url_quote($order);
}

if ($::buffer =~ /&cmd-/) {
    my $url = "query.cgi?$::buffer#chart";
    print "Refresh: 0; URL=$url\n";
    print "Content-Type: text/html\n\n";
    # Generate and return the UI (HTML page) from the appropriate template.
    $vars->{'message'} = "buglist_adding_field";
    $vars->{'url'} = $url;
    $template->process("global/message.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

# Generate a reasonable filename for the user agent to suggest to the user
# when the user saves the bug list.  Uses the name of the remembered query
# if available.  We have to do this now, even though we return HTTP headers 
# at the end, because the fact that there is a remembered query gets 
# forgotten in the process of retrieving it.
my @time = localtime(time());
my $date = sprintf "%04d-%02d-%02d", 1900+$time[5],$time[4]+1,$time[3];
my $filename = "bugs-$date.$format->{extension}";
$::FORM{'cmdtype'} ||= "";
if ($::FORM{'cmdtype'} eq 'runnamed') {
    $filename = "$::FORM{'namedcmd'}-$date.$format->{extension}";
    # Remove white-space from the filename so the user cannot tamper
    # with the HTTP headers.
    $filename =~ s/\s//;
}

################################################################################
# Utilities
################################################################################

my @weekday= qw( Sun Mon Tue Wed Thu Fri Sat );
sub DiffDate {
    my ($datestr) = @_;
    my $date = str2time($datestr);
    my $age = time() - $date;
    my ($s,$m,$h,$d,$mo,$y,$wd)= localtime $date;
    if( $age < 18*60*60 ) {
        $date = sprintf "%02d:%02d:%02d", $h,$m,$s;
    } elsif( $age < 6*24*60*60 ) {
        $date = sprintf "%s %02d:%02d", $weekday[$wd],$h,$m;
    } else {
        $date = sprintf "%04d-%02d-%02d", 1900+$y,$mo+1,$d;
    }
    return $date;
}

sub LookupNamedQuery {
    my ($name) = @_;
    confirm_login();
    my $userid = DBNameToIdAndCheck($::COOKIE{"Bugzilla_login"});
    my $qname = SqlQuote($name);
    SendSQL("SELECT query FROM namedqueries WHERE userid = $userid AND name = $qname");
    my $result = FetchOneColumn();
    if (!$result) {
        my $qname = html_quote($name);
        DisplayError("The query named <em>$qname</em> seems to no longer exist.");
        exit;
    }
    return $result;
}

sub GetQuip {

    my $quip;

    SendSQL("SELECT quip FROM quips ORDER BY RAND() LIMIT 1");

    if (MoreSQLData()) {
        ($quip) = FetchSQLData();
    }

    return $quip;
}

sub GetGroupsByGroupSet {
    my ($groupset) = @_;

    return if !$groupset;

    SendSQL("
        SELECT  bit, name, description, isactive
          FROM  groups
         WHERE  (bit & $groupset) != 0
           AND  isbuggroup != 0
      ORDER BY  description ");

    my @groups;

    while (MoreSQLData()) {
        my $group = {};
        ($group->{'bit'}, $group->{'name'},
         $group->{'description'}, $group->{'isactive'}) = FetchSQLData();
        push(@groups, $group);
    }

    return \@groups;
}


################################################################################
# Command Execution
################################################################################

# Backwards-compatibility - the old interface had cmdtype="runnamed" to run
# a named command, and we can't break this because it's in bookmarks.
if ($::FORM{'cmdtype'} eq "runnamed") {  
    $::FORM{'cmdtype'} = "dorem"; 
    $::FORM{'remaction'} = "run";
}

# Take appropriate action based on user's request.
if ($::FORM{'cmdtype'} eq "dorem") {  
    if ($::FORM{'remaction'} eq "run") {
        $::buffer = LookupNamedQuery($::FORM{"namedcmd"});
        $vars->{'title'} = "Bug List: $::FORM{'namedcmd'}";
        ProcessFormFields($::buffer);
        $order = $::FORM{'order'} || $order;
    }
    elsif ($::FORM{'remaction'} eq "load") {
        my $url = "query.cgi?" . LookupNamedQuery($::FORM{"namedcmd"});
        print "Refresh: 0; URL=$url\n";
        print "Content-Type: text/html\n\n";
        # Generate and return the UI (HTML page) from the appropriate template.
        $vars->{'message'} = "buglist_load_named_query";
        $vars->{'namedcmd'} = $::FORM{'namedcmd'};
        $vars->{'url'} = $url;
        $template->process("global/message.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }
    elsif ($::FORM{'remaction'} eq "forget") {
        confirm_login();
        my $userid = DBNameToIdAndCheck($::COOKIE{"Bugzilla_login"});
        my $qname = SqlQuote($::FORM{'namedcmd'});
        SendSQL("DELETE FROM namedqueries WHERE userid = $userid AND name = $qname");
        # Now remove this query from the footer
        my $count = 0;
        foreach my $q (@{$::vars->{'user'}{'queries'}}) {
            if ($q->{'name'} eq $::FORM{'namedcmd'}) {
                splice(@{$::vars->{'user'}{'queries'}}, $count, 1);
                last;
            }
            $count++;
        }

        print "Content-Type: text/html\n\n";
        # Generate and return the UI (HTML page) from the appropriate template.
        $vars->{'message'} = "buglist_query_gone";
        $vars->{'namedcmd'} = $::FORM{'namedcmd'};
        $vars->{'url'} = "query.cgi";
        $template->process("global/message.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }
}
elsif ($::FORM{'cmdtype'} eq "doit" && $::FORM{'remember'}) {
    if ($::FORM{'remember'} == 1 && $::FORM{'remtype'} eq "asdefault") {
        confirm_login();
        my $userid = DBNameToIdAndCheck($::COOKIE{"Bugzilla_login"});
        my $qname = SqlQuote($::defaultqueryname);
        my $qbuffer = SqlQuote($::buffer);
        SendSQL("REPLACE INTO namedqueries (userid, name, query)
                 VALUES ($userid, $qname, $qbuffer)");
        # Generate and return the UI (HTML page) from the appropriate template.
        $vars->{'message'} = "buglist_new_default_query";
    }
    elsif ($::FORM{'remember'} == 1 && $::FORM{'remtype'} eq "asnamed") {
        confirm_login();
        my $userid = DBNameToIdAndCheck($::COOKIE{"Bugzilla_login"});

        my $name = trim($::FORM{'newqueryname'});
        $name
          || DisplayError("You must enter a name for your query.")
            && exit;
        $name =~ /[<>&]/
          && DisplayError("The name of your query cannot contain any
                           of the following characters: &lt;, &gt;, &amp;.")
            && exit;
        my $qname = SqlQuote($name);

        $::buffer =~ s/[\&\?]cmdtype=[a-z]+//;
        my $qbuffer = SqlQuote($::buffer);

        my $tofooter = $::FORM{'tofooter'} ? 1 : 0;

        SendSQL("SELECT query FROM namedqueries WHERE userid = $userid AND name = $qname");
        if (FetchOneColumn()) {
            SendSQL("UPDATE  namedqueries
                        SET  query = $qbuffer , linkinfooter = $tofooter
                      WHERE  userid = $userid AND name = $qname");
        }
        else {
            SendSQL("REPLACE INTO namedqueries (userid, name, query, linkinfooter)
                     VALUES ($userid, $qname, $qbuffer, $tofooter)");
        }
        
        my $new_in_footer = $tofooter;
        
        # Don't add it to the list if they are reusing an existing query name.
        foreach my $query (@{$vars->{'user'}{'queries'}}) {
            if ($query->{'name'} eq $name && $query->{'linkinfooter'} == 1) {
                $new_in_footer = 0;
            }
        }        
        
        if ($new_in_footer) {
            my %query = (name => $name,
                         query => $::buffer, 
                         linkinfooter => $tofooter);
            push(@{$vars->{'user'}{'queries'}}, \%query);
        }
        
        $vars->{'message'}   = "buglist_new_named_query";
        $vars->{'queryname'} = $name;
    }
}


################################################################################
# Column Definition
################################################################################

# Define the columns that can be selected in a query and/or displayed in a bug
# list.  Column records include the following fields:
#
# 1. ID: a unique identifier by which the column is referred in code;
#
# 2. Name: The name of the column in the database (may also be an expression
#          that returns the value of the column);
#
# 3. Title: The title of the column as displayed to users.
# 
# Note: There are a few hacks in the code that deviate from these definitions.
#       In particular, when the list is sorted by the "votes" field the word 
#       "DESC" is added to the end of the field to sort in descending order, 
#       and the redundant summaryfull column is removed when the client
#       requests "all" columns.

my $columns = {};
sub DefineColumn {
    my ($id, $name, $title) = @_;
    $columns->{$id} = { 'name' => $name , 'title' => $title };
}

# Column:     ID                    Name                           Title
DefineColumn("id"                , "bugs.bug_id"                , "ID"               );
DefineColumn("groupset"          , "bugs.groupset"              , "Groupset"         );
DefineColumn("opendate"          , "bugs.creation_ts"           , "Opened"           );
DefineColumn("changeddate"       , "bugs.delta_ts"              , "Changed"          );
DefineColumn("severity"          , "bugs.bug_severity"          , "Severity"         );
DefineColumn("priority"          , "bugs.priority"              , "Priority"         );
DefineColumn("platform"          , "bugs.rep_platform"          , "Platform"         );
DefineColumn("owner"             , "map_assigned_to.login_name" , "Owner"            );
DefineColumn("reporter"          , "map_reporter.login_name"    , "Reporter"         );
DefineColumn("qa_contact"        , "map_qa_contact.login_name"  , "QA Contact"       );
DefineColumn("status"            , "bugs.bug_status"            , "State"            );
DefineColumn("resolution"        , "bugs.resolution"            , "Result"           );
DefineColumn("summary"           , "bugs.short_desc"            , "Summary"          );
DefineColumn("summaryfull"       , "bugs.short_desc"            , "Summary"          );
DefineColumn("status_whiteboard" , "bugs.status_whiteboard"     , "Status Summary"   );
DefineColumn("component"         , "map_components.name"        , "Component"        );
DefineColumn("product"           , "map_products.name"          , "Product"          );
DefineColumn("version"           , "bugs.version"               , "Version"          );
DefineColumn("os"                , "bugs.op_sys"                , "OS"               );
DefineColumn("target_milestone"  , "bugs.target_milestone"      , "Target Milestone" );
DefineColumn("votes"             , "bugs.votes"                 , "Votes"            );
DefineColumn("keywords"          , "bugs.keywords"              , "Keywords"         );


################################################################################
# Display Column Determination
################################################################################

# Determine the columns that will be displayed in the bug list via the 
# columnlist CGI parameter, the user's preferences, or the default.
my @displaycolumns = ();
if (defined $::FORM{'columnlist'}) {
    if ($::FORM{'columnlist'} eq "all") {
        # If the value of the CGI parameter is "all", display all columns,
        # but remove the redundant "summaryfull" column.
        @displaycolumns = grep($_ ne 'summaryfull', keys(%$columns));
    }
    else {
        @displaycolumns = split(/[ ,]+/, $::FORM{'columnlist'});
    }
}
elsif (defined $::COOKIE{'COLUMNLIST'}) {
    # Use the columns listed in the user's preferences.
    @displaycolumns = split(/ /, $::COOKIE{'COLUMNLIST'});
}
else {
    # Use the default list of columns.
    @displaycolumns = @::default_column_list;
}

# Weed out columns that don't actually exist to prevent the user 
# from hacking their column list cookie to grab data to which they 
# should not have access.  Detaint the data along the way.
@displaycolumns = grep($columns->{$_} && trick_taint($_), @displaycolumns);

# Remove the "ID" column from the list because bug IDs are always displayed
# and are hard-coded into the display templates.
@displaycolumns = grep($_ ne 'id', @displaycolumns);

# IMPORTANT! Never allow the groupset column to be displayed!
@displaycolumns = grep($_ ne 'groupset', @displaycolumns);

# Add the votes column to the list of columns to be displayed
# in the bug list if the user is searching for bugs with a certain
# number of votes and the votes column is not already on the list.

# Some versions of perl will taint 'votes' if this is done as a single
# statement, because $::FORM{'votes'} is tainted at this point
$::FORM{'votes'} ||= "";
if (trim($::FORM{'votes'}) && !grep($_ eq 'votes', @displaycolumns)) {
    push(@displaycolumns, 'votes');
}


################################################################################
# Select Column Determination
################################################################################

# Generate the list of columns that will be selected in the SQL query.

# The bug ID and groupset are always selected because bug IDs are always
# displayed and we need the groupset to determine whether or not the bug
# is visible to the user.
my @selectcolumns = ("id", "groupset");

# Display columns are selected because otherwise we could not display them.
push (@selectcolumns, @displaycolumns);

# If the user is editing multiple bugs, we also make sure to select the product
# and status because the values of those fields determine what options the user
# has for modifying the bugs.
if ($dotweak) {
    push(@selectcolumns, "product") if !grep($_ eq 'product', @selectcolumns);
    push(@selectcolumns, "status") if !grep($_ eq 'status', @selectcolumns);
}


################################################################################
# Query Generation
################################################################################

# Convert the list of columns being selected into a list of column names.
my @selectnames = map($columns->{$_}->{'name'}, @selectcolumns);

# Generate the basic SQL query that will be used to generate the bug list.
my $search = new Bugzilla::Search('fields' => \@selectnames, 
                                  'url' => $::buffer);
my $query = $search->getSQL();


################################################################################
# Sort Order Determination
################################################################################

# Add to the query some instructions for sorting the bug list.
if ($::COOKIE{'LASTORDER'} && (!$order || $order =~ /^reuse/i)) {
    $order = url_decode($::COOKIE{'LASTORDER'});
    $order_from_cookie = 1;
}

if ($order) {
    my $db_order;  # Modified version of $order for use with SQL query

    # Convert the value of the "order" form field into a list of columns
    # by which to sort the results.
    ORDER: for ($order) {
        /\./ && do {
            my @columnnames = map($columns->{lc($_)}->{'name'}, keys(%$columns));
            # A custom list of columns.  Make sure each column is valid.
            foreach my $fragment (split(/,/, $order)) {
                $fragment = trim($fragment);
                # Accept an order fragment matching a column name, with
                # asc|desc optionally following (to specify the direction)
                if (!grep($fragment =~ /^\Q$_\E(\s+(asc|desc))?$/, @columnnames)) {
                    my $qfragment = html_quote($fragment);
                    my $error = "The custom sort order you specified in your "
                              . "form submission contains an invalid column "
                              . "name <em>$qfragment</em>.";
                    if ($order_from_cookie) {
                        my $cookiepath = Param("cookiepath");
                        print "Set-Cookie: LASTORDER= ; path=$cookiepath; expires=Sun, 30-Jun-80 00:00:00 GMT\n";
                        $error =~ s/form submission/cookie/;
                        $error .= "  The cookie has been cleared.";
                    }
                    DisplayError($error);
                    exit;
                }
            }
            # Now that we have checked that all columns in the order are valid,
            # detaint the order string.
            trick_taint($order);
            last ORDER;
        };
        /Number/ && do {
            $order = "bugs.bug_id";
            last ORDER;
        };
        /Import/ && do {
            $order = "bugs.priority, bugs.bug_severity";
            last ORDER;
        };
        /Assign/ && do {
            $order = "map_assigned_to.login_name, bugs.bug_status, bugs.priority, bugs.bug_id";
            last ORDER;
        };
        /Changed/ && do {
            $order = "bugs.delta_ts, bugs.bug_status, bugs.priority, map_assigned_to.login_name, bugs.bug_id";
            last ORDER;
        };
        # DEFAULT
        $order = "bugs.bug_status, bugs.priority, map_assigned_to.login_name, bugs.bug_id";
    }

    $db_order = $order;  # Copy $order into $db_order for use with SQL query

    # Extra special disgusting hack: if we are ordering by target_milestone,
    # change it to order by the sortkey of the target_milestone first.
    if ($db_order =~ /bugs.target_milestone/) {
        $db_order =~ s/bugs.target_milestone/ms_order.sortkey,ms_order.value/;
        $query =~ s/\sWHERE\s/ LEFT JOIN milestones ms_order ON ms_order.value = bugs.target_milestone AND ms_order.product_id = bugs.product_id WHERE /;
    }

    # If we are sorting by votes, sort in descending order if no explicit
    # sort order was given
    $db_order =~ s/bugs.votes\s*(,|$)/bugs.votes desc$1/i;

    $query .= " ORDER BY $db_order ";
}


################################################################################
# Query Execution
################################################################################

# Time to use server push to display an interim message to the user until
# the query completes and we can display the bug list.
if ($serverpush) {
    # Generate HTTP headers.
    print "Content-Disposition: inline; filename=$filename\n";
    print "Content-Type: multipart/x-mixed-replace;boundary=thisrandomstring\n\n";
    print "--thisrandomstring\n";
    print "Content-Type: text/html\n\n";

    # Generate and return the UI (HTML page) from the appropriate template.
    $template->process("list/server-push.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

# Connect to the shadow database if this installation is using one to improve
# query performance.
ReconnectToShadowDatabase();

# Normally, we ignore SIGTERM and SIGPIPE (see globals.pl) but we need to
# respond to them here to prevent someone DOSing us by reloading a query
# a large number of times.
$::SIG{TERM} = 'DEFAULT';
$::SIG{PIPE} = 'DEFAULT';

# Execute the query.
SendSQL($query);


################################################################################
# Results Retrieval
################################################################################

# Retrieve the query results one row at a time and write the data into a list
# of Perl records.

my $bugowners = {};
my $bugproducts = {};
my $bugstatuses = {};

my @bugs; # the list of records

while (my @row = FetchSQLData()) {
    my $bug = {}; # a record

    # Slurp the row of data into the record.
    foreach my $column (@selectcolumns) {
        $bug->{$column} = shift @row;
    }

    # Process certain values further (i.e. date format conversion).
    if ($bug->{'changeddate'}) {
        $bug->{'changeddate'} =~ 
          s/^(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})$/$1-$2-$3 $4:$5:$6/;
        $bug->{'changeddate'} = DiffDate($bug->{'changeddate'});
    }
    ($bug->{'opendate'} = DiffDate($bug->{'opendate'})) if $bug->{'opendate'};

    # Record the owner, product, and status in the big hashes of those things.
    $bugowners->{$bug->{'owner'}} = 1 if $bug->{'owner'};
    $bugproducts->{$bug->{'product'}} = 1 if $bug->{'product'};
    $bugstatuses->{$bug->{'status'}} = 1 if $bug->{'status'};

    # Add the record to the list.
    push(@bugs, $bug);
}

# Switch back from the shadow database to the regular database so PutFooter()
# can determine the current user even if the "logincookies" table is corrupted
# in the shadow database.
SendSQL("USE $::db_name");


################################################################################
# Template Variable Definition
################################################################################

# Define the variables and functions that will be passed to the UI template.

$vars->{'bugs'} = \@bugs;
$vars->{'buglist'} = join(',', map($_->{id}, @bugs));
$vars->{'columns'} = $columns;
$vars->{'displaycolumns'} = \@displaycolumns;

my @openstates = OpenStates();
$vars->{'openstates'} = \@openstates;
$vars->{'closedstates'} = ['CLOSED', 'VERIFIED', 'RESOLVED'];

# The list of query fields in URL query string format, used when creating
# URLs to the same query results page with different parameters (such as
# a different sort order or when taking some action on the set of query
# results).  To get this string, we start with the raw URL query string
# buffer that was created when we initially parsed the URL on script startup,
# then we remove all non-query fields from it, f.e. the sort order (order)
# and command type (cmdtype) fields.
$vars->{'urlquerypart'} = $::buffer;
$vars->{'urlquerypart'} =~ s/(order|cmdtype)=[^&]*&?//g;
$vars->{'order'} = $order;

# The user's login account name (i.e. email address).
my $login = $::COOKIE{'Bugzilla_login'};

$vars->{'caneditbugs'} = UserInGroup('editbugs');
$vars->{'usebuggroups'} = Param('usebuggroups');

# Whether or not this user is authorized to move bugs to another installation.
$vars->{'ismover'} = 1
  if Param('move-enabled')
    && defined($login)
      && Param('movers') =~ /^(\Q$login\E[,\s])|([,\s]\Q$login\E[,\s]+)/;

my @bugowners = keys %$bugowners;
if (scalar(@bugowners) > 1 && UserInGroup('editbugs')) {
    my $suffix = Param('emailsuffix');
    map(s/$/$suffix/, @bugowners) if $suffix;
    my $bugowners = join(",", @bugowners);
    $vars->{'bugowners'} = $bugowners;
}

if ($::FORM{'debug'}) {
    $vars->{'debug'} = 1;
    $vars->{'query'} = $query;
}

# Whether or not to split the column titles across two rows to make
# the list more compact.
$vars->{'splitheader'} = $::COOKIE{'SPLITHEADER'} ? 1 : 0;

$vars->{'quip'} = GetQuip();
$vars->{'currenttime'} = time();

# The following variables are used when the user is making changes to multiple bugs.
if ($dotweak) {
    $vars->{'dotweak'} = 1;
    $vars->{'use_keywords'} = 1 if @::legal_keywords;

    $vars->{'products'} = \@::legal_product;
    $vars->{'platforms'} = \@::legal_platform;
    $vars->{'priorities'} = \@::legal_priority;
    $vars->{'severities'} = \@::legal_severity;
    $vars->{'resolutions'} = \@::settable_resolution;

    # The value that represents "don't change the value of this field".
    $vars->{'dontchange'} = $::dontchange;

    $vars->{'unconfirmedstate'} = $::unconfirmedstate;

    $vars->{'bugstatuses'} = [ keys %$bugstatuses ];

    # The groups to which the user belongs.
    $vars->{'groups'} = GetGroupsByGroupSet($::usergroupset) if $::usergroupset ne '0';

    # If all bugs being changed are in the same product, the user can change
    # their version and component, so generate a list of products, a list of
    # versions for the product (if there is only one product on the list of
    # products), and a list of components for the product.
    $vars->{'bugproducts'} = [ keys %$bugproducts ];
    if (scalar(@{$vars->{'bugproducts'}}) == 1) {
        my $product = $vars->{'bugproducts'}->[0];
        $vars->{'versions'} = $::versions{$product};
        $vars->{'components'} = $::components{$product};
        $vars->{'targetmilestones'} = $::target_milestone{$product} if Param('usetargetmilestone');
    }
}


################################################################################
# HTTP Header Generation
################################################################################

# If we are doing server push, output a separator string.
print "\n--thisrandomstring\n" if $serverpush;
    
# Generate HTTP headers

# Suggest a name for the bug list if the user wants to save it as a file.
# If we are doing server push, then we did this already in the HTTP headers
# that started the server push, so we don't have to do it again here.
print "Content-Disposition: inline; filename=$filename\n" unless $serverpush;

if ($format->{'extension'} eq "html") {
    my $cookiepath = Param("cookiepath");
    print "Content-Type: text/html\n";

    if ($order) {
        my $qorder = url_quote($order);
        print "Set-Cookie: LASTORDER=$qorder ; path=$cookiepath; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
    }
    my $bugids = join(":", map( $_->{'id'}, @bugs));
    # See also Bug 111999
    if (length($bugids) < 4000) {
        print "Set-Cookie: BUGLIST=$bugids ; path=$cookiepath; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
    }
    else {
        print "Set-Cookie: BUGLIST= ; path=$cookiepath; expires=Sun, 30-Jun-2029 00:00:00 GMT\n";
        $vars->{'toolong'} = 1;
    }
}
else {
    print "Content-Type: $format->{'ctype'}\n";
}

print "\n"; # end HTTP headers


################################################################################
# Content Generation
################################################################################

# Generate and return the UI (HTML page) from the appropriate template.
$template->process($format->{'template'}, $vars)
  || ThrowTemplateError($template->error());


################################################################################
# Script Conclusion
################################################################################

print "\n--thisrandomstring--\n" if $serverpush;
