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

use strict;
use lib ".";

# This module implements a series - a set of data to be plotted on a chart.
#
# This Series is in the database if and only if self->{'series_id'} is defined. 
# Note that the series being in the database does not mean that the fields of 
# this object are the same as the DB entries, as the object may have been 
# altered.

package Bugzilla::Series;

use Bugzilla;
use Bugzilla::Util;
use Bugzilla::User;

use constant PUBLIC_USER_ID => 0;

sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
  
    # Create a ref to an empty hash and bless it
    my $self = {};
    bless($self, $class);

    my $arg_count = scalar(@_);
    
    if ($arg_count == 1) {
        if (ref($_[0])) {
            # We've been given a CGI object to create a new Series from.
            $self->initFromCGI($_[0]);
        }
        else {
            # We've been given a series_id, which should represent an existing
            # Series.
            $self->initFromDatabase($_[0]);
        }
    }
    elsif ($arg_count >= 6 && $arg_count <= 8) {
        # We've been given a load of parameters to create a new Series from.
        $self->initFromParameters(@_);
    }
    else {
        die("Bad parameters passed in - invalid number of args: $arg_count");
    }

    return $self;
}

sub initFromDatabase {
    my $self = shift;
    my $series_id = shift;
    
    &::detaint_natural($series_id) 
      || &::ThrowCodeError("invalid_series_id", { 'series_id' => $series_id });
    
    my $dbh = Bugzilla->dbh;
    my @series = $dbh->selectrow_array("SELECT series.series_id, cc1.name, " .
        "cc2.name, series.name, series.creator, series.frequency, " .
        "series.query " .
        "FROM series " .
        "LEFT JOIN series_categories AS cc1 " .
        "    ON series.category = cc1.category_id " .
        "LEFT JOIN series_categories AS cc2 " .
        "    ON series.subcategory = cc2.category_id " .
        "WHERE series.series_id = $series_id");
    
    if (@series) {
        # Note that we calculate $self->{'public'} ourselves instead of passing
        # it as the last parameter in @series; this is because isSubscribed()
        # requires the rest of the object to be set up correctly.
        $self->initFromParameters(@series);
        $self->{'public'} = $self->isSubscribed(PUBLIC_USER_ID);
    }
    else {
        &::ThrowCodeError("invalid_series_id", { 'series_id' => $series_id });
    }
}

sub initFromParameters {
    # Pass undef as the first parameter if you are creating a new series.
    my $self = shift;

    ($self->{'series_id'}, $self->{'category'},  $self->{'subcategory'},
     $self->{'name'}, $self->{'creator'}, $self->{'frequency'},
     $self->{'query'}, $self->{'public'}) = @_;
}

sub initFromCGI {
    my $self = shift;
    my $cgi = shift;

    $self->{'series_id'} = $cgi->param('series_id') || undef;
    if (defined($self->{'series_id'})) {
        detaint_natural($self->{'series_id'})
          || &::ThrowCodeError("invalid_series_id", 
                               { 'series_id' => $self->{'series_id'} });
    }
    
    $self->{'category'} = $cgi->param('category')
      || $cgi->param('newcategory')
      || &::ThrowUserError("missing_category");

    $self->{'subcategory'} = $cgi->param('subcategory')
      || $cgi->param('newsubcategory')
      || &::ThrowUserError("missing_subcategory");

    $self->{'name'} = $cgi->param('name')
      || &::ThrowUserError("missing_name");

    $self->{'creator'} = Bugzilla->user->id;

    $self->{'frequency'} = $cgi->param('frequency');
    detaint_natural($self->{'frequency'})
      || &::ThrowUserError("missing_frequency");

    $self->{'query'} = $cgi->canonicalise_query("format", "ctype", "action",
                                        "category", "subcategory", "name",
                                        "frequency", "public", "query_format");
                                        
    $self->{'public'} = $cgi->param('public') ? 1 : 0;    
}

sub writeToDatabase {
    my $self = shift;

    # Lock some tables
    my $dbh = Bugzilla->dbh;
    $dbh->do("LOCK TABLES series_categories WRITE, series WRITE, " .
             "user_series_map WRITE");

    my $category_id = getCategoryID($self->{'category'});
    my $subcategory_id = getCategoryID($self->{'subcategory'});

    # Is this already in the database?                              
    if ($self->existsInDatabase()) {
        # Update existing series
        my $dbh = Bugzilla->dbh;
        $dbh->do("UPDATE series SET " .
                 "category = ?, subcategory = ?," .
                 "name = ?, frequency = ? " .
                 "WHERE series_id = ?", undef,
                 $category_id, $subcategory_id, $self->{'name'},
                 $self->{'frequency'}, $self->{'series_id'});
    }
    else {
        # Insert the new series into the series table
        trick_taint($self->{'query'});
        $dbh->do("INSERT INTO series (creator, category, subcategory, " .
                 "name, frequency, query) VALUES ($self->{'creator'}, " .
                 "$category_id, $subcategory_id, " .
                 $dbh->quote($self->{'name'}) . ", $self->{'frequency'}," .
                 $dbh->quote($self->{'query'}) . ")");

        # Retrieve series_id
        $self->{'series_id'} = $dbh->selectrow_array("SELECT MAX(series_id) " .
                                                     "FROM series");
        $self->{'series_id'}
          || &::ThrowCodeError("missing_series_id", { 'series' => $self });

        # Subscribe creator to the newly-created series.
        $self->subscribe($self->{'creator'});
    }
    
    # Update publicness by changing subscription
    if ($self->{'public'}) {
        $self->subscribe(PUBLIC_USER_ID);
    }
    else {
        $self->unsubscribe(PUBLIC_USER_ID);
    }             

    $dbh->do("UNLOCK TABLES");
}

# Check whether a series with this name, category and subcategory exists in
# the DB and, if so, sets series_id to its series_id.
sub existsInDatabase {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    my $category_id = getCategoryID($self->{'category'});
    my $subcategory_id = getCategoryID($self->{'subcategory'});
    
    trick_taint($self->{'name'});
    $self->{'series_id'} = $dbh->selectrow_array("SELECT series_id " .
                              "FROM series WHERE category = $category_id " .
                              "AND subcategory = $subcategory_id AND name = " .
                              $dbh->quote($self->{'name'}));
                              
    return(defined($self->{'series_id'}));
}

# Get a category or subcategory IDs, creating the category if it doesn't exist.
sub getCategoryID {
    my ($category) = @_;
    my $category_id;
    my $dbh = Bugzilla->dbh;

    # This seems for the best idiom for "Do A. Then maybe do B and A again."
    while (1) {
        # We are quoting this to put it in the DB, so we can remove taint
        trick_taint($category);

        $category_id = $dbh->selectrow_array("SELECT category_id " .
                                      "from series_categories " .
                                      "WHERE name =" . $dbh->quote($category));
        last if $category_id;

        $dbh->do("INSERT INTO series_categories (name) " .
                 "VALUES (" . $dbh->quote($category) . ")");
    }

    return $category_id;
}        

sub subscribe {
    my $self = shift;
    my $userid = shift;
    
    if (!$self->isSubscribed($userid)) {
        # Subscribe current user to series_id
        my $dbh = Bugzilla->dbh;
        $dbh->do("INSERT INTO user_series_map " .
                 "VALUES($userid, $self->{'series_id'})");
    }    
}

sub unsubscribe {
    my $self = shift;
    my $userid = shift;
    
    if ($self->isSubscribed($userid)) {
        # Remove current user's subscription to series_id
        my $dbh = Bugzilla->dbh;
        $dbh->do("DELETE FROM user_series_map " .
                "WHERE user_id = $userid AND series_id = $self->{'series_id'}");
    }        
}

sub isSubscribed {
    my $self = shift;
    my $userid = shift;
    
    my $dbh = Bugzilla->dbh;
    my $issubscribed = $dbh->selectrow_array("SELECT 1 FROM user_series_map " .
                                       "WHERE user_id = $userid " .
                                       "AND series_id = $self->{'series_id'}");
    return $issubscribed;
}

1;
