#!/usr/bin/perl -w
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
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Holger
# Schurig. Portions created by Holger Schurig are
# Copyright (C) 1999 Holger Schurig. All
# Rights Reserved.
#
# Contributor(s): Holger Schurig <holgerschurig@nikocity.de>
#                 Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Dave Miller <justdave@syndicomm.com>
#                 Zach Lipton  <zach@zachlipton.com>
#                 Jacob Steenhagen <jake@bugzilla.org>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Tobias Burnus <burnus@net-b.de>
#                 Shane H. W. Travis <travis@sedsystems.ca>
#                 Gervase Markham <gerv@gerv.net>
#                 Erik Stambaugh <erik@dasbistro.com>
#                 Dave Lawrence <dkl@redhat.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Joel Peshkin <bugreport@peshkin.net>
#                 Lance Larsh <lance.larsh@oracle.com>
#                 A. Karl Kornel <karl@kornel.name>
#                 Marc Schumann <wurblzap@gmail.com>

=head1 NAME

checksetup.pl - A do-it-all upgrade and installation script for Bugzilla.

=head1 SYNOPSIS

 ./checksetup.pl [--help|--check-modules]
 ./checksetup.pl [SCRIPT [--verbose]] [--no-templates|-t]

=head1 OPTIONS

=over

=item F<SCRIPT>

Name of script to drive non-interactive mode. This script should
define an C<%answer> hash whose keys are variable names and the
values answers to all the questions checksetup.pl asks. For details
on the format of this script, do C<perldoc checksetup.pl> and look for
the L</"RUNNING CHECKSETUP NON-INTERACTIVELY"> section.

=item B<--help>

Display this help text

=item B<--check-modules>

Only check for correct module dependencies and quit afterward.

=item B<--no-templates> (B<-t>) 

Don't compile the templates at all. Existing compiled templates will 
remain; missing compiled templates will not be created. (Used primarily
by developers to speed up checksetup.) Use this switch at your own risk.

=item B<--verbose>

Output results of SCRIPT being processed.

=back

=head1 DESCRIPTION

Hey, what's this?

F<checksetup.pl> is a script that is supposed to run during 
installation time and also after every upgrade.

The goal of this script is to make the installation even easier.
It does this by doing things for you as well as testing for problems
in advance.

You can run the script whenever you like. You SHOULD run it after
you update Bugzilla, because it may then update your SQL table
definitions to resync them with the code.

Currently, this script does the following:

=over

=item * 

Check for required perl modules

=item * 

Set defaults for local configuration variables

=item * 

Create and populate the F<data> directory after installation

=item * 

Set the proper rights for the F<*.cgi>, F<*.html>, etc. files

=item * 

Verify that the code can access the database server

=item * 

Creates the database C<bugs> if it does not exist

=item * 

Creates the tables inside the database if they don't exist

=item * 

Automatically changes the table definitions if they are from
an older version of Bugzilla

=item * 

Populates the groups

=item * 

Puts the first user into all groups so that the system can
be administered

=item * 

...And a whole lot more.

=back

=head1 MODIFYING CHECKSETUP

There should be no need for Bugzilla Administrators to modify
this script; all user-configurable stuff has been moved 
into a local configuration file called F<localconfig>. When that file
in changed and F<checksetup.pl> is run, then the user's changes
will be reflected back into the database.

Developers, however, have to modify this file at various places. To
make this easier, there are some special tags for which one
can search.

 To                                               Search for

 add/delete local configuration variables         --LOCAL--
 check for more required modules                  --MODULES--
 add more database-related checks                 --DATABASE--
 change the defaults for local configuration vars --DATA--
 update the assigned file permissions             --CHMOD--
 change table definitions                         --TABLE--
 add more groups                                  --GROUPS--
 add user-adjustable settings                     --SETTINGS--
 create initial administrator account             --ADMIN--

Note: sometimes those special comments occur more than once. For
example, C<--LOCAL--> is used at least 3 times in this code!  C<--TABLE-->
is also used more than once, so search for each and every occurrence!

=head1 RUNNING CHECKSETUP NON-INTERACTIVELY

To operate checksetup non-interactively, run it with a single argument
specifying a filename that contains the information usually obtained by
prompting the user or by editing localconfig.

The format of that file is as follows:

 $answer{'db_host'}   = 'localhost';
 $answer{'db_driver'} = 'mydbdriver';
 $answer{'db_port'}   = 0;
 $answer{'db_name'}   = 'mydbname';
 $answer{'db_user'}   = 'mydbuser';
 $answer{'db_pass'}   = 'mydbpass';

 $answer{'urlbase'} = 'http://bugzilla.mydomain.com/';

 (Any localconfig variable or parameter can be specified as above.)

 $answer{'ADMIN_OK'} = 'Y';
 $answer{'ADMIN_EMAIL'} = 'myadmin@mydomain.net';
 $answer{'ADMIN_PASSWORD'} = 'fooey';
 $answer{'ADMIN_REALNAME'} = 'Joel Peshkin';

 $answer{'SMTP_SERVER'} = 'mail.mydomain.net';

Note: Only information that supersedes defaults from C<LocalVar()>
function calls needs to be specified in this file.

=head1 SEE ALSO

L<Bugzilla::Install::Requirements>

L<Bugzilla::Install::Localconfig>

L<Bugzilla::Install::Filesystem>

L<Bugzilla::Config/update_params>

L<Bugzilla::DB/CONNECTION>

=cut

######################################################################
# Initialization
######################################################################

use strict;
use 5.008;
use File::Basename;
use Getopt::Long qw(:config bundling);
use Pod::Usage;
use Safe;

BEGIN { chdir dirname($0); }
use lib ".";
use Bugzilla::Constants;
use Bugzilla::Install::Requirements;

require 5.008001 if ON_WINDOWS; # for CGI 2.93 or higher

######################################################################
# Subroutines
######################################################################

sub read_answers_file {
    my %hash;
    if ($ARGV[0]) {
        my $s = new Safe;
        $s->rdo($ARGV[0]);

        die "Error reading $ARGV[0]: $!" if $!;
        die "Error evaluating $ARGV[0]: $@" if $@;

        # Now read the param back out from the sandbox
        %hash = %{$s->varglob('answer')};
    }
    return \%hash;
}

######################################################################
# Live Code
######################################################################

my %switch;
GetOptions(\%switch, 'help|h|?', 'check-modules', 'no-templates|t',
                     'verbose|v|no-silent');

# Print the help message if that switch was selected.
pod2usage({-verbose => 1, -exitval => 1}) if $switch{'help'};

# Read in the "answers" file if it exists, for running in 
# non-interactive mode.
our %answer = %{read_answers_file()};
my $silent = scalar(keys %answer) && !$switch{'verbose'};

# Display version information
printf "\n*** This is Bugzilla " . BUGZILLA_VERSION . " on perl %vd ***\n", 
    $^V unless $silent;

# Check required --MODULES--
my $module_results = check_requirements(!$silent);
exit if !$module_results->{pass};
# Break out if checking the modules is all we have been asked to do.
exit if $switch{'check-modules'};

###########################################################################
# Load Bugzilla Modules
###########################################################################

# It's never safe to "use" a Bugzilla module in checksetup. If a module
# prerequisite is missing, and you "use" a module that requires it,
# then instead of our nice normal checksetup message, the user would
# get a cryptic perl error about the missing module.

# We need $::ENV{'PATH'} to remain defined.
my $env = $::ENV{'PATH'};
require Bugzilla;
$::ENV{'PATH'} = $env;

require Bugzilla::Config;
import Bugzilla::Config qw(:admin);

require Bugzilla::User::Setting;
import Bugzilla::User::Setting qw(add_setting);

require Bugzilla::Util;
import Bugzilla::Util qw(bz_crypt trim html_quote is_7bit_clean
                         clean_text url_quote);

require Bugzilla::User;
import Bugzilla::User qw(insert_new_user);

require Bugzilla::Bug;
import Bugzilla::Bug qw(is_open_state);

require Bugzilla::Install::Localconfig;
import Bugzilla::Install::Localconfig qw(read_localconfig update_localconfig);

require Bugzilla::Install::Filesystem;
import Bugzilla::Install::Filesystem qw(update_filesystem create_htaccess);

require Bugzilla::DB;
require Bugzilla::Template;

###########################################################################
# Check and update --LOCAL-- configuration
###########################################################################

print "Reading " .  bz_locations()->{'localconfig'} . "...\n" unless $silent;
update_localconfig({ output => !$silent, answer => \%answer });
my $lc_hash = read_localconfig();

# XXX Eventually this variable can be eliminated, but it is
# used more than once throughout checksetup right now.
my $my_webservergroup = $lc_hash->{'webservergroup'};

###########################################################################
# Check --DATABASE-- setup
###########################################################################

# At this point, localconfig is defined and is readable. So we know
# everything we need to create the DB. We have to create it early,
# because some data required to populate data/params is stored in the DB.

Bugzilla::DB::bz_check_requirements(!$silent);
Bugzilla::DB::bz_create_database() if $lc_hash->{'db_check'};

# now get a handle to the database:
my $dbh = Bugzilla->dbh;

###########################################################################
# Create tables
###########################################################################

# Note: --TABLE-- definitions are now in Bugzilla::DB::Schema.
$dbh->bz_setup_database();

# Populate the tables that hold the values for the <select> fields.
$dbh->bz_populate_enum_tables();

###########################################################################
# Check --DATA-- directory
###########################################################################

update_filesystem({ index_html => $lc_hash->{'index_html'} });
create_htaccess() if $lc_hash->{'create_htaccess'};

# XXX Some parts of checksetup still need these, right now.
my $datadir   = bz_locations()->{'datadir'};
my $webdotdir = bz_locations()->{'webdotdir'};

# Remove parameters from the params file that no longer exist in Bugzilla,
# and set the defaults for new ones
update_params({ answer => \%answer});

###########################################################################
# Pre-compile --TEMPLATE-- code
###########################################################################

Bugzilla::Template::precompile_templates(!$silent)
    unless $switch{'no-templates'};

###########################################################################
# Set proper rights
###########################################################################

#
# Here we use --CHMOD-- and friends to set the file permissions
#
# The rationale is that the web server generally runs as apache, so the cgi
# scripts should not be writable for apache, otherwise someone may be possible
# to change the cgi's when exploiting some security flaw somewhere (not
# necessarily in Bugzilla!)
#
# Also, some *.pl files are executable, some are not.
#
# +++ Can anybody tell me what a Windows Perl would do with this code?
#
# Changes 03/14/00 by SML
#
# This abstracts out what files are executable and what ones are not.  It makes
# for slightly neater code and lets us do things like determine exactly which
# files are executable and which ones are not.
#
# Not all directories have permissions changed on them.  i.e., changing ./CVS
# to be 0640 is bad.
#
# Fixed bug in chmod invocation.  chmod (at least on my linux box running perl
# 5.005 needs a valid first argument, not 0.
#
# (end changes, 03/14/00 by SML)
#
# Changes 15/06/01 kiko@async.com.br
# 
# Fix file permissions for non-webservergroup installations (see
# http://bugzilla.mozilla.org/show_bug.cgi?id=71555). I'm setting things
# by default to world readable/executable for all files, and
# world-writable (with sticky on) to data and graphs.
#

# These are the files which need to be marked executable
my @executable_files = ('whineatnews.pl', 'collectstats.pl',
   'checksetup.pl', 'importxml.pl', 'runtests.pl', 'testserver.pl',
   'whine.pl', 'customfield.pl');

# tell me if a file is executable.  All CGI files and those in @executable_files
# are executable
sub isExecutableFile {
    my ($file) = @_;
    if ($file =~ /\.cgi/) {
        return 1;
    }

    my $exec_file;
    foreach $exec_file (@executable_files) {
        if ($file eq $exec_file) {
            return 1;
        }
    }
    return undef;
}

# fix file (or files - wildcards ok) permissions 
sub fixPerms {
    my ($file_pattern, $owner, $group, $umask, $do_dirs) = @_;
    my @files = glob($file_pattern);
    my $execperm = 0777 & ~ $umask;
    my $normperm = 0666 & ~ $umask;
    foreach my $file (@files) {
        next if (!-e $file);
        # do not change permissions on directories here unless $do_dirs is set
        if (!(-d $file)) {
            chown $owner, $group, $file;
            # check if the file is executable.
            if (isExecutableFile($file)) {
                #printf ("Changing $file to %o\n", $execperm);
                chmod $execperm, $file;
            } else {
                #printf ("Changing $file to %o\n", $normperm);
                chmod $normperm, $file;
            }
        }
        elsif ($do_dirs) {
            chown $owner, $group, $file;
            if ($file =~ /CVS$/) {
                chmod 0700, $file;
            }
            else {
                #printf ("Changing $file to %o\n", $execperm);
                chmod $execperm, $file;
                fixPerms("$file/.htaccess", $owner, $group, $umask, $do_dirs);
                # do the contents of the directory
                fixPerms("$file/*", $owner, $group, $umask, $do_dirs); 
            }
        }
    }
}

if ($^O !~ /MSWin32/i) {
    my $templatedir = bz_locations()->{'templatedir'};
    if ($my_webservergroup) {
        # Funny! getgrname returns the GID if fed with NAME ...
        my $webservergid = getgrnam($my_webservergroup) 
        or die("no such group: $my_webservergroup");
        # chown needs to be called with a valid uid, not 0.  $< returns the
        # caller's uid.  Maybe there should be a $bugzillauid, and call 
        # with that userid.
        fixPerms('.htaccess', $<, $webservergid, 027); # glob('*') doesn't catch dotfiles
        fixPerms("$datadir/.htaccess", $<, $webservergid, 027);
        fixPerms("$datadir/duplicates", $<, $webservergid, 027, 1);
        fixPerms("$datadir/mining", $<, $webservergid, 027, 1);
        fixPerms("$datadir/template", $<, $webservergid, 007, 1); # webserver will write to these
        # webserver will write to attachdir.
        fixPerms(bz_locations()->{'attachdir'}, $<, $webservergid, 007, 1);
        fixPerms($webdotdir, $<, $webservergid, 007, 1);
        fixPerms("$webdotdir/.htaccess", $<, $webservergid, 027);
        fixPerms("$datadir/params", $<, $webservergid, 017);
        # The web server must be the owner of bugzilla-update.xml.
        fixPerms("$datadir/bugzilla-update.xml", $webservergid, $webservergid, 017);
        fixPerms('*', $<, $webservergid, 027);
        fixPerms('Bugzilla', $<, $webservergid, 027, 1);
        fixPerms($templatedir, $<, $webservergid, 027, 1);
        fixPerms('images', $<, $webservergid, 027, 1);
        fixPerms('css', $<, $webservergid, 027, 1);
        fixPerms('skins', $<, $webservergid, 027, 1);
        fixPerms('js', $<, $webservergid, 027, 1);

        # Don't use fixPerms here, because it won't change perms 
        # on the directory unless it's using recursion
        chown $<, $webservergid, $datadir;
        chmod 0771, $datadir;
        chown $<, $webservergid, 'graphs';
        chmod 0770, 'graphs';
    } else {
        # get current gid from $( list
        my $gid = (split " ", $()[0];
        fixPerms('.htaccess', $<, $gid, 022); # glob('*') doesn't catch dotfiles
        fixPerms("$datadir/.htaccess", $<, $gid, 022);
        fixPerms("$datadir/duplicates", $<, $gid, 022, 1);
        fixPerms("$datadir/mining", $<, $gid, 022, 1);
        fixPerms("$datadir/template", $<, $gid, 000, 1); # webserver will write to these
        fixPerms($webdotdir, $<, $gid, 000, 1);
        chmod 01777, $webdotdir;
        fixPerms("$webdotdir/.htaccess", $<, $gid, 022);
        fixPerms("$datadir/params", $<, $gid, 011);
        fixPerms("$datadir/bugzilla-update.xml", $gid, $gid, 011);
        fixPerms('*', $<, $gid, 022);
        fixPerms('Bugzilla', $<, $gid, 022, 1);
        fixPerms($templatedir, $<, $gid, 022, 1);
        fixPerms('images', $<, $gid, 022, 1);
        fixPerms('css', $<, $gid, 022, 1);
        fixPerms('skins', $<, $gid, 022, 1);
        fixPerms('js', $<, $gid, 022, 1);
        
        # Don't use fixPerms here, because it won't change perms
        # on the directory unless it's using recursion
        chown $<, $gid, $datadir;
        chmod 0777, $datadir;
        chown $<, $gid, 'graphs';
        chmod 01777, 'graphs';
    }
}

###########################################################################
# Check GraphViz setup
###########################################################################

#
# If we are using a local 'dot' binary, verify the specified binary exists
# and that the generated images are accessible.
#

if( Bugzilla->params->{'webdotbase'} && Bugzilla->params->{'webdotbase'} !~ /^https?:/ ) {
    printf("Checking for %15s %-9s ", "GraphViz", "(any)") unless $silent;
    if(-x Bugzilla->params->{'webdotbase'}) {
        print "ok: found\n" unless $silent;
    } else {
        print "not a valid executable: " . Bugzilla->params->{'webdotbase'} . "\n";
    }

    # Check .htaccess allows access to generated images
    if(-e "$webdotdir/.htaccess") {
        open HTACCESS, "$webdotdir/.htaccess";
        if(! grep(/png/,<HTACCESS>)) {
            print "Dependency graph images are not accessible.\n";
            print "delete $webdotdir/.htaccess and re-run checksetup.pl to fix.\n";
        }
        close HTACCESS;
    }
}

print "\n" unless $silent;

###########################################################################
# Populate groups table
###########################################################################

sub GroupDoesExist
{
    my ($name) = @_;
    my $sth = $dbh->prepare("SELECT name FROM groups WHERE name='$name'");
    $sth->execute;
    if ($sth->rows) {
        return 1;
    }
    return 0;
}


#
# This subroutine ensures that a group exists. If not, it will be created 
# automatically, and given the next available groupid
#

sub AddGroup {
    my ($name, $desc, $userregexp) = @_;
    $userregexp ||= "";

    return if GroupDoesExist($name);
    
    print "Adding group $name ...\n";
    my $sth = $dbh->prepare('INSERT INTO groups
                          (name, description, userregexp, isbuggroup,
                           last_changed)
                          VALUES (?, ?, ?, ?, NOW())');
    $sth->execute($name, $desc, $userregexp, 0);

    my $last = $dbh->bz_last_key('groups', 'id');
    return $last;
}


###########################################################################
# Populate the list of fields.
###########################################################################

my $headernum = 1;

sub AddFDef {
    my ($name, $description, $mailhead) = (@_);

    my $sth = $dbh->prepare("SELECT id FROM fielddefs " .
                            "WHERE name = ?");
    $sth->execute($name);
    my ($fieldid) = ($sth->fetchrow_array());
    if (!$fieldid) {
        $dbh->do(q{INSERT INTO fielddefs
                               (name, description, mailhead, sortkey)
                   VALUES (?, ?, ?, ?)},
                 undef, ($name, $description, $mailhead, $headernum));
    } else {
        $dbh->do(q{UPDATE fielddefs
                      SET name = ?, description = ?,
                          mailhead = ?, sortkey = ?
                    WHERE id = ?}, undef,
                 $name, $description, $mailhead, $headernum, $fieldid);
    }
    $headernum++;
}

# Change the name of the fieldid column to id, so that fielddefs
# can use Bugzilla::Object easily. We have to do this up here, because
# otherwise adding these field definitions will fail.
$dbh->bz_rename_column('fielddefs', 'fieldid', 'id');

# Note that all of these entries are unconditional, from when get_field_id
# used to create an entry if it wasn't found. New fielddef columns should
# be created with their associated schema change.
AddFDef("bug_id", "Bug \#", 1);
AddFDef("short_desc", "Summary", 1);
AddFDef("classification", "Classification", 1);
AddFDef("product", "Product", 1);
AddFDef("version", "Version", 1);
AddFDef("rep_platform", "Platform", 1);
AddFDef("bug_file_loc", "URL", 1);
AddFDef("op_sys", "OS/Version", 1);
AddFDef("bug_status", "Status", 1);
AddFDef("status_whiteboard", "Status Whiteboard", 0);
AddFDef("keywords", "Keywords", 1);
AddFDef("resolution", "Resolution", 0);
AddFDef("bug_severity", "Severity", 1);
AddFDef("priority", "Priority", 1);
AddFDef("component", "Component", 1);
AddFDef("assigned_to", "AssignedTo", 1);
AddFDef("reporter", "ReportedBy", 1);
AddFDef("votes", "Votes", 0);
AddFDef("qa_contact", "QAContact", 1);
AddFDef("cc", "CC", 1);
AddFDef("dependson", "BugsThisDependsOn", 1);
AddFDef("blocked", "OtherBugsDependingOnThis", 1);
AddFDef("attachments.description", "Attachment description", 0);
AddFDef("attachments.filename", "Attachment filename", 0);
AddFDef("attachments.mimetype", "Attachment mime type", 0);
AddFDef("attachments.ispatch", "Attachment is patch", 0);
AddFDef("attachments.isobsolete", "Attachment is obsolete", 0);
AddFDef("attachments.isprivate", "Attachment is private", 0);

AddFDef("target_milestone", "Target Milestone", 0);
AddFDef("creation_ts", "Creation date", 0);
AddFDef("delta_ts", "Last changed date", 0);
AddFDef("longdesc", "Comment", 0);
AddFDef("alias", "Alias", 0);
AddFDef("everconfirmed", "Ever Confirmed", 0);
AddFDef("reporter_accessible", "Reporter Accessible", 0);
AddFDef("cclist_accessible", "CC Accessible", 0);
AddFDef("bug_group", "Group", 0);
AddFDef("estimated_time", "Estimated Hours", 1);
AddFDef("remaining_time", "Remaining Hours", 0);
AddFDef("deadline", "Deadline", 1);
AddFDef("commenter", "Commenter", 0);

# Oops. Bug 163299
$dbh->do("DELETE FROM fielddefs WHERE name='cc_accessible'");

# Oops. Bug 215319
$dbh->do("DELETE FROM fielddefs WHERE name='requesters.login_name'");

AddFDef("flagtypes.name", "Flag", 0);
AddFDef("requestees.login_name", "Flag Requestee", 0);
AddFDef("setters.login_name", "Flag Setter", 0);
AddFDef("work_time", "Hours Worked", 0);
AddFDef("percentage_complete", "Percentage Complete", 0);

AddFDef("content", "Content", 0);

$dbh->do("DELETE FROM fielddefs WHERE name='attachments.thedata'");
AddFDef("attach_data.thedata", "Attachment data", 0);
AddFDef("attachments.isurl", "Attachment is a URL", 0);

# 2005-11-13 LpSolit@gmail.com - Bug 302599
# One of the field names was a fragment of SQL code, which is DB dependent.
# We have to rename it to a real name, which is DB independent.
my $new_field_name = 'days_elapsed';
my $field_description = 'Days since bug changed';

my ($old_field_id, $old_field_name) =
    $dbh->selectrow_array('SELECT id, name
                           FROM fielddefs
                           WHERE description = ?',
                           undef, $field_description);

if ($old_field_id && ($old_field_name ne $new_field_name)) {
    print "SQL fragment found in the 'fielddefs' table...\n";
    print "Old field name: " . $old_field_name . "\n";
    # We have to fix saved searches first. Queries have been escaped
    # before being saved. We have to do the same here to find them.
    $old_field_name = url_quote($old_field_name);
    my $broken_named_queries =
        $dbh->selectall_arrayref('SELECT userid, name, query
                                  FROM namedqueries WHERE ' .
                                  $dbh->sql_istrcmp('query', '?', 'LIKE'),
                                  undef, "%=$old_field_name%");

    my $sth_UpdateQueries = $dbh->prepare('UPDATE namedqueries SET query = ?
                                           WHERE userid = ? AND name = ?');

    print "Fixing saved searches...\n" if scalar(@$broken_named_queries);
    foreach my $named_query (@$broken_named_queries) {
        my ($userid, $name, $query) = @$named_query;
        $query =~ s/=\Q$old_field_name\E(&|$)/=$new_field_name$1/gi;
        $sth_UpdateQueries->execute($query, $userid, $name);
    }

    # We now do the same with saved chart series.
    my $broken_series =
        $dbh->selectall_arrayref('SELECT series_id, query
                                  FROM series WHERE ' .
                                  $dbh->sql_istrcmp('query', '?', 'LIKE'),
                                  undef, "%=$old_field_name%");

    my $sth_UpdateSeries = $dbh->prepare('UPDATE series SET query = ?
                                          WHERE series_id = ?');

    print "Fixing saved chart series...\n" if scalar(@$broken_series);
    foreach my $series (@$broken_series) {
        my ($series_id, $query) = @$series;
        $query =~ s/=\Q$old_field_name\E(&|$)/=$new_field_name$1/gi;
        $sth_UpdateSeries->execute($query, $series_id);
    }

    # Now that saved searches have been fixed, we can fix the field name.
    print "Fixing the 'fielddefs' table...\n";
    print "New field name: " . $new_field_name . "\n";
    $dbh->do('UPDATE fielddefs SET name = ? WHERE id = ?',
              undef, ($new_field_name, $old_field_id));
}
AddFDef($new_field_name, $field_description, 0);

###########################################################################
# Create initial test product if there are no products present.
###########################################################################
my $sth = $dbh->prepare("SELECT description FROM products");
$sth->execute;
unless ($sth->rows) {
    print "Creating initial dummy product 'TestProduct' ...\n";
    my $test_product_name = 'TestProduct';
    my $test_product_desc = 
        'This is a test product. This ought to be blown away and'
        . ' replaced with real stuff in a finished installation of bugzilla.';
    my $test_product_version = 'other';

    $dbh->do(q{INSERT INTO products(name, description, milestoneurl, 
                           disallownew, votesperuser, votestoconfirm)
               VALUES (?, ?, '', ?, ?, ?)},
               undef, $test_product_name, $test_product_desc, 0, 0, 0);

    # We could probably just assume that this is "1", but better
    # safe than sorry...
    my $product_id = $dbh->bz_last_key('products', 'id');
    
    $dbh->do(q{INSERT INTO versions (value, product_id) 
                VALUES (?, ?)}, 
             undef, $test_product_version, $product_id);
    # note: since admin user is not yet known, components gets a 0 for 
    # initialowner and this is fixed during final checks.
    $dbh->do("INSERT INTO components (name, product_id, description, " .
                                     "initialowner) " .
             "VALUES (" .
             "'TestComponent', $product_id, " .
             "'This is a test component in the test product database.  " .
             "This ought to be blown away and replaced with real stuff in " .
             "a finished installation of Bugzilla.', 0)");
    $dbh->do(q{INSERT INTO milestones (product_id, value, sortkey) 
               VALUES (?,?,?)},
             undef, $product_id, '---', 0);
}

# Create a default classification if one does not exist
my $class_count =
    $dbh->selectrow_array("SELECT COUNT(*) FROM classifications");
if (!$class_count) {
    $dbh->do("INSERT INTO classifications (name,description) " .
             "VALUES('Unclassified','Unassigned to any classifications')");
}

###########################################################################
# Update the tables to the current definition  --TABLE--
###########################################################################

# Both legacy code and modern code need this variable.
my @admins = ();

# really old fields that were added before checksetup.pl existed
# but aren't in very old bugzilla's (like 2.1)
# Steve Stock (sstock@iconnect-inc.com)

$dbh->bz_add_column('bugs', 'target_milestone', 
                    {TYPE => 'varchar(20)', NOTNULL => 1, DEFAULT => "'---'"});
$dbh->bz_add_column('bugs', 'qa_contact', {TYPE => 'INT3'});
$dbh->bz_add_column('bugs', 'status_whiteboard', 
                   {TYPE => 'MEDIUMTEXT', NOTNULL => 1, DEFAULT => "''"});
$dbh->bz_add_column('products', 'disallownew', 
                    {TYPE => 'BOOLEAN', NOTNULL => 1}, 0);
$dbh->bz_add_column('products', 'milestoneurl', 
                    {TYPE => 'TINYTEXT', NOTNULL => 1}, '');
$dbh->bz_add_column('components', 'initialqacontact', 
                    {TYPE => 'TINYTEXT'});
$dbh->bz_add_column('components', 'description',
                    {TYPE => 'MEDIUMTEXT', NOTNULL => 1}, '');

# 1999-06-22 Added an entry to the attachments table to record who the
# submitter was.  Nothing uses this yet, but it still should be recorded.
$dbh->bz_add_column('attachments', 'submitter_id', 
                    {TYPE => 'INT3', NOTNULL => 1}, 0);

#
# One could even populate this field automatically, e.g. with
#
# unless (GetField('attachments', 'submitter_id') {
#    $dbh->bz_add_column ...
#    populate
# }
#
# For now I was too lazy, so you should read the documentation :-)



# 1999-9-15 Apparently, newer alphas of MySQL won't allow you to have "when"
# as a column name.  So, I have had to rename a column in the bugs_activity
# table.

$dbh->bz_rename_column('bugs_activity', 'when', 'bug_when');



# 1999-10-11 Restructured voting database to add a cached value in each bug
# recording how many total votes that bug has.  While I'm at it, I removed
# the unused "area" field from the bugs database.  It is distressing to
# realize that the bugs table has reached the maximum number of indices
# allowed by MySQL (16), which may make future enhancements awkward.
# (P.S. All is not lost; it appears that the latest betas of MySQL support
# a new table format which will allow 32 indices.)

$dbh->bz_drop_column('bugs', 'area');
if (!$dbh->bz_column_info('bugs', 'votes')) {
    $dbh->bz_add_column('bugs', 'votes', {TYPE => 'INT3', NOTNULL => 1,
                                          DEFAULT => 0});
    $dbh->bz_add_index('bugs', 'bugs_votes_idx', [qw(votes)]);
}
$dbh->bz_add_column('products', 'votesperuser', 
                    {TYPE => 'INT2', NOTNULL => 1}, 0);


# The product name used to be very different in various tables.
#
# It was   varchar(16)   in bugs
#          tinytext      in components
#          tinytext      in products
#          tinytext      in versions
#
# tinytext is equivalent to varchar(255), which is quite huge, so I change
# them all to varchar(64).

# Only do this if these fields still exist - they're removed below, in
# a later change
if ($dbh->bz_column_info('products', 'product')) {
    $dbh->bz_alter_column('bugs',       'product', 
                         {TYPE => 'varchar(64)', NOTNULL => 1});
    $dbh->bz_alter_column('components', 'program', {TYPE => 'varchar(64)'});
    $dbh->bz_alter_column('products',   'product', {TYPE => 'varchar(64)'});
    $dbh->bz_alter_column('versions',   'program', 
                          {TYPE => 'varchar(64)', NOTNULL => 1});
}

# 2000-01-16 Added a "keywords" field to the bugs table, which
# contains a string copy of the entries of the keywords table for this
# bug.  This is so that I can easily sort and display a keywords
# column in bug lists.

if (!$dbh->bz_column_info('bugs', 'keywords')) {
    $dbh->bz_add_column('bugs', 'keywords',
                        {TYPE => 'MEDIUMTEXT', NOTNULL => 1, DEFAULT => "''"});

    my @kwords;
    print "Making sure 'keywords' field of table 'bugs' is empty ...\n";
    $dbh->do("UPDATE bugs SET keywords = '' " .
              "WHERE keywords != ''");
    print "Repopulating 'keywords' field of table 'bugs' ...\n";
    my $sth = $dbh->prepare("SELECT keywords.bug_id, keyworddefs.name " .
                              "FROM keywords, keyworddefs " .
                             "WHERE keyworddefs.id = keywords.keywordid " .
                          "ORDER BY keywords.bug_id, keyworddefs.name");
    $sth->execute;
    my @list;
    my $bugid = 0;
    my @row;
    while (1) {
        my ($b, $k) = ($sth->fetchrow_array());
        if (!defined $b || $b ne $bugid) {
            if (@list) {
                $dbh->do("UPDATE bugs SET keywords = " .
                         $dbh->quote(join(', ', @list)) .
                         " WHERE bug_id = $bugid");
            }
            if (!$b) {
                last;
            }
            $bugid = $b;
            @list = ();
        }
        push(@list, $k);
    }
}


# 2000-01-18 Added a "disabledtext" field to the profiles table.  If not
# empty, then this account has been disabled, and this field is to contain
# text describing why.
$dbh->bz_add_column('profiles', 'disabledtext',
                    {TYPE => 'MEDIUMTEXT', NOTNULL => 1}, '');


# 2000-01-20 Added a new "longdescs" table, which is supposed to have all the
# long descriptions in it, replacing the old long_desc field in the bugs 
# table.  The below hideous code populates this new table with things from
# the old field, with ugly parsing and heuristics.

sub WriteOneDesc {
    my ($id, $who, $when, $buffer) = (@_);
    $buffer = trim($buffer);
    if ($buffer eq '') {
        return;
    }
    $dbh->do("INSERT INTO longdescs (bug_id, who, bug_when, thetext) VALUES " .
             "($id, $who, " .  time2str("'%Y/%m/%d %H:%M:%S'", $when) .
             ", " . $dbh->quote($buffer) . ")");
}


if ($dbh->bz_column_info('bugs', 'long_desc')) {
    eval("use Date::Parse");
    eval("use Date::Format");
    my $sth = $dbh->prepare("SELECT count(*) FROM bugs");
    $sth->execute();
    my ($total) = ($sth->fetchrow_array);

    print "Populating new long_desc table.  This is slow.  There are $total\n";
    print "bugs to process; a line of dots will be printed for each 50.\n\n";
    $| = 1;

    $dbh->bz_lock_tables('bugs write', 'longdescs write', 'profiles write',
                         'bz_schema WRITE');

    $dbh->do('DELETE FROM longdescs');

    $sth = $dbh->prepare("SELECT bug_id, creation_ts, reporter, long_desc " .
                           "FROM bugs ORDER BY bug_id");
    $sth->execute();
    my $count = 0;
    while (1) {
        my ($id, $createtime, $reporterid, $desc) = ($sth->fetchrow_array());
        if (!$id) {
            last;
        }
        print ".";
        $count++;
        if ($count % 10 == 0) {
            print " ";
            if ($count % 50 == 0) {
                print "$count/$total (" . int($count * 100 / $total) . "%)\n";
            }
        }
        $desc =~ s/\r//g;
        my $who = $reporterid;
        my $when = str2time($createtime);
        my $buffer = "";
        foreach my $line (split(/\n/, $desc)) {
            $line =~ s/\s+$//g;       # Trim trailing whitespace.
            if ($line =~ /^------- Additional Comments From ([^\s]+)\s+(\d.+\d)\s+-------$/) {
                my $name = $1;
                my $date = str2time($2);
                $date += 59;    # Oy, what a hack.  The creation time is
                                # accurate to the second.  But we the long
                                # text only contains things accurate to the
                                # minute.  And so, if someone makes a comment
                                # within a minute of the original bug creation,
                                # then the comment can come *before* the
                                # bug creation.  So, we add 59 seconds to
                                # the time of all comments, so that they
                                # are always considered to have happened at
                                # the *end* of the given minute, not the
                                # beginning.
                if ($date >= $when) {
                    WriteOneDesc($id, $who, $when, $buffer);
                    $buffer = "";
                    $when = $date;
                    my $s2 = $dbh->prepare("SELECT userid FROM profiles " .
                                            "WHERE login_name = " .
                                           $dbh->quote($name));
                    $s2->execute();
                    ($who) = ($s2->fetchrow_array());
                    if (!$who) {
                        # This username doesn't exist.  Try a special
                        # netscape-only hack (sorry about that, but I don't
                        # think it will hurt any other installations).  We
                        # have many entries in the bugsystem from an ancient
                        # world where the "@netscape.com" part of the loginname
                        # was omitted.  So, look up the user again with that
                        # appended, and use it if it's there.
                        if ($name !~ /\@/) {
                            my $nsname = $name . "\@netscape.com";
                            $s2 =
                                $dbh->prepare("SELECT userid FROM profiles " .
                                               "WHERE login_name = " .
                                              $dbh->quote($nsname));
                            $s2->execute();
                            ($who) = ($s2->fetchrow_array());
                        }
                    }
                            
                    if (!$who) {
                        # This username doesn't exist.  Maybe someone renamed
                        # him or something.  Invent a new profile entry,
                        # disabled, just to represent him.
                        $dbh->do("INSERT INTO profiles " .
                                 "(login_name, cryptpassword," .
                                 " disabledtext) VALUES (" .
                                 $dbh->quote($name) .
                                 ", " . $dbh->quote(bz_crypt('okthen')) . 
                                 ", " . 
                                 "'Account created only to maintain database integrity')");
                        $who = $dbh->bz_last_key('profiles', 'userid');
                    }
                    next;
                } else {
#                    print "\nDecided this line of bug $id has a date of " .
#                        time2str("'%Y/%m/%d %H:%M:%S'", $date) .
#                            "\nwhich is less than previous line:\n$line\n\n";
                }

            }
            $buffer .= $line . "\n";
        }
        WriteOneDesc($id, $who, $when, $buffer);
    }
                

    print "\n\n";

    $dbh->bz_drop_column('bugs', 'long_desc');

    $dbh->bz_unlock_tables();
}


# 2000-01-18 Added a new table fielddefs that records information about the
# different fields we keep an activity log on.  The bugs_activity table
# now has a pointer into that table instead of recording the name directly.

if ($dbh->bz_column_info('bugs_activity', 'field')) {
    $dbh->bz_add_column('bugs_activity', 'fieldid',
                        {TYPE => 'INT3', NOTNULL => 1}, 0);

    $dbh->bz_add_index('bugs_activity', 'bugs_activity_fieldid_idx',
                       [qw(fieldid)]);
    print "Populating new fieldid field ...\n";

    $dbh->bz_lock_tables('bugs_activity WRITE', 'fielddefs WRITE');

    my $sth = $dbh->prepare('SELECT DISTINCT field FROM bugs_activity');
    $sth->execute();
    my %ids;
    while (my ($f) = ($sth->fetchrow_array())) {
        my $q = $dbh->quote($f);
        my $s2 =
            $dbh->prepare("SELECT id FROM fielddefs WHERE name = $q");
        $s2->execute();
        my ($id) = ($s2->fetchrow_array());
        if (!$id) {
            $dbh->do("INSERT INTO fielddefs (name, description) VALUES " .
                     "($q, $q)");
            $id = $dbh->bz_last_key('fielddefs', 'id');
        }
        $dbh->do("UPDATE bugs_activity SET fieldid = $id WHERE field = $q");
    }
    $dbh->bz_unlock_tables();

    $dbh->bz_drop_column('bugs_activity', 'field');
}

        

# 2000-01-18 New email-notification scheme uses a new field in the bug to 
# record when email notifications were last sent about this bug.  Also,
# added 'newemailtech' field to record if user wants to use the experimental
# stuff.
# 2001-04-29 jake@bugzilla.org - The newemailtech field is no longer needed
#   http://bugzilla.mozilla.org/show_bugs.cgi?id=71552

if (!$dbh->bz_column_info('bugs', 'lastdiffed')) {
    $dbh->bz_add_column('bugs', 'lastdiffed', {TYPE =>'DATETIME'});
    $dbh->do('UPDATE bugs SET lastdiffed = now()');
}


# 2000-01-22 The "login_name" field in the "profiles" table was not
# declared to be unique.  Sure enough, somehow, I got 22 duplicated entries
# in my database.  This code detects that, cleans up the duplicates, and
# then tweaks the table to declare the field to be unique.  What a pain.
if (!$dbh->bz_index_info('profiles', 'profiles_login_name_idx') ||
    !$dbh->bz_index_info('profiles', 'profiles_login_name_idx')->{TYPE}) {
    print "Searching for duplicate entries in the profiles table ...\n";
    while (1) {
        # This code is weird in that it loops around and keeps doing this
        # select again.  That's because I'm paranoid about deleting entries
        # out from under us in the profiles table.  Things get weird if
        # there are *three* or more entries for the same user...
        $sth = $dbh->prepare("SELECT p1.userid, p2.userid, p1.login_name " .
                               "FROM profiles AS p1, profiles AS p2 " .
                              "WHERE p1.userid < p2.userid " .
                                "AND p1.login_name = p2.login_name " .
                           "ORDER BY p1.login_name");
        $sth->execute();
        my ($u1, $u2, $n) = ($sth->fetchrow_array);
        if (!$u1) {
            last;
        }
        print "Both $u1 & $u2 are ids for $n!  Merging $u2 into $u1 ...\n";
        foreach my $i (["bugs", "reporter"],
                       ["bugs", "assigned_to"],
                       ["bugs", "qa_contact"],
                       ["attachments", "submitter_id"],
                       ["bugs_activity", "who"],
                       ["cc", "who"],
                       ["votes", "who"],
                       ["longdescs", "who"]) {
            my ($table, $field) = (@$i);
            print "   Updating $table.$field ...\n";
            $dbh->do("UPDATE $table SET $field = $u1 " .
                      "WHERE $field = $u2");
        }
        $dbh->do("DELETE FROM profiles WHERE userid = $u2");
    }
    print "OK, changing index type to prevent duplicates in the future ...\n";
    
    $dbh->bz_drop_index('profiles', 'profiles_login_name_idx');
    $dbh->bz_add_index('profiles', 'profiles_login_name_idx',
                       {TYPE => 'UNIQUE', FIELDS => [qw(login_name)]});
}    


# 2000-01-24 Added a new field to let people control whether the "My
# bugs" link appears at the bottom of each page.  Also can control
# whether each named query should show up there.

$dbh->bz_add_column('profiles', 'mybugslink', {TYPE => 'BOOLEAN', NOTNULL => 1,
                                               DEFAULT => 'TRUE'});

my $comp_init_owner = $dbh->bz_column_info('components', 'initialowner');
if ($comp_init_owner && $comp_init_owner->{TYPE} eq 'TINYTEXT') {
    $sth = $dbh->prepare(
         "SELECT program, value, initialowner, initialqacontact " .
         "FROM components");
    $sth->execute();
    while (my ($program, $value, $initialowner) = $sth->fetchrow_array()) {
        $initialowner =~ s/([\\\'])/\\$1/g; $initialowner =~ s/\0/\\0/g;
        $program =~ s/([\\\'])/\\$1/g; $program =~ s/\0/\\0/g;
        $value =~ s/([\\\'])/\\$1/g; $value =~ s/\0/\\0/g;

        my $s2 = $dbh->prepare("SELECT userid " .
                                 "FROM profiles " .
                                "WHERE login_name = '$initialowner'");
        $s2->execute();

        my $initialownerid = $s2->fetchrow_array();

        unless (defined $initialownerid) {
            print "Warning: You have an invalid default assignee '$initialowner'\n" .
              "in component '$value' of program '$program'. !\n";
            $initialownerid = 0;
        }

        my $update = 
          "UPDATE components " .
             "SET initialowner = $initialownerid " .
           "WHERE program = '$program' " .
             "AND value = '$value'";
        my $s3 = $dbh->prepare("UPDATE components " .
                                  "SET initialowner = $initialownerid " .
                                "WHERE program = '$program' " .
                                  "AND value = '$value';");
        $s3->execute();
    }

    $dbh->bz_alter_column('components','initialowner',{TYPE => 'INT3'});
}

my $comp_init_qa = $dbh->bz_column_info('components', 'initialqacontact');
if ($comp_init_qa && $comp_init_qa->{TYPE} eq 'TINYTEXT') {
    $sth = $dbh->prepare(
           "SELECT program, value, initialqacontact, initialqacontact " .
           "FROM components");
    $sth->execute();
    while (my ($program, $value, $initialqacontact) = $sth->fetchrow_array()) {
        $initialqacontact =~ s/([\\\'])/\\$1/g; $initialqacontact =~ s/\0/\\0/g;
        $program =~ s/([\\\'])/\\$1/g; $program =~ s/\0/\\0/g;
        $value =~ s/([\\\'])/\\$1/g; $value =~ s/\0/\\0/g;

        my $s2 = $dbh->prepare("SELECT userid " .
                               "FROM profiles " .
                               "WHERE login_name = '$initialqacontact'");
        $s2->execute();

        my $initialqacontactid = $s2->fetchrow_array();

        unless (defined $initialqacontactid) {
            if ($initialqacontact ne '') {
                print "Warning: You have an invalid default QA contact $initialqacontact' in program '$program', component '$value'!\n";
            }
            $initialqacontactid = 0;
        }

        my $update = "UPDATE components " .
            "SET initialqacontact = $initialqacontactid " .
            "WHERE program = '$program' AND value = '$value'";
        my $s3 = $dbh->prepare("UPDATE components " .
                               "SET initialqacontact = $initialqacontactid " .
                               "WHERE program = '$program' " .
                               "AND value = '$value';");
        $s3->execute();
    }

    $dbh->bz_alter_column('components','initialqacontact',{TYPE => 'INT3'});
}


if (!$dbh->bz_column_info('bugs', 'everconfirmed')) {
    $dbh->bz_add_column('bugs', 'everconfirmed',
        {TYPE => 'BOOLEAN', NOTNULL => 1}, 1);
}
$dbh->bz_add_column('products', 'maxvotesperbug',
                    {TYPE => 'INT2', NOTNULL => 1, DEFAULT => '10000'});
$dbh->bz_add_column('products', 'votestoconfirm',
                    {TYPE => 'INT2', NOTNULL => 1}, 0);

# 2000-03-21 Adding a table for target milestones to 
# database - matthew@zeroknowledge.com
# If the milestones table is empty, and we're still back in a Bugzilla
# that has a bugs.product field, that means that we just created
# the milestones table and it needs to be populated.
my $milestones_exist = $dbh->selectrow_array("SELECT 1 FROM milestones");
if (!$milestones_exist && $dbh->bz_column_info('bugs', 'product')) {
    print "Replacing blank milestones...\n";

    $dbh->do("UPDATE bugs " .
             "SET target_milestone = '---' " .
             "WHERE target_milestone = ' '");

    # If we are upgrading from 2.8 or earlier, we will have *created*
    # the milestones table with a product_id field, but Bugzilla expects
    # it to have a "product" field. So we change the field backward so
    # other code can run. The change will be reversed later in checksetup.
    if ($dbh->bz_column_info('milestones', 'product_id')) {
        # Dropping the column leaves us with a milestones_product_id_idx
        # index that is only on the "value" column. We need to drop the
        # whole index so that it can be correctly re-created later.
        $dbh->bz_drop_index('milestones', 'milestones_product_id_idx');
        $dbh->bz_drop_column('milestones', 'product_id');
        $dbh->bz_add_column('milestones', 'product', 
            {TYPE => 'varchar(64)', NOTNULL => 1}, '');
    }

    # Populate the milestone table with all existing values in the database
    $sth = $dbh->prepare("SELECT DISTINCT target_milestone, product FROM bugs");
    $sth->execute();
    
    print "Populating milestones table...\n";
    
    my $value;
    my $product;
    while(($value, $product) = $sth->fetchrow_array())
    {
        # check if the value already exists
        my $sortkey = substr($value, 1);
        if ($sortkey !~ /^\d+$/) {
            $sortkey = 0;
        } else {
            $sortkey *= 10;
        }
        $value = $dbh->quote($value);
        $product = $dbh->quote($product);
        my $s2 = $dbh->prepare("SELECT value " .
                               "FROM milestones " .
                               "WHERE value = $value " .
                               "AND product = $product");
        $s2->execute();
        
        if(!$s2->fetchrow_array())
        {
            $dbh->do("INSERT INTO milestones(value, product, sortkey) VALUES($value, $product, $sortkey)");
        }
    }
}

# 2000-03-22 Changed the default value for target_milestone to be "---"
# (which is still not quite correct, but much better than what it was 
# doing), and made the size of the value field in the milestones table match
# the size of the target_milestone field in the bugs table.

$dbh->bz_alter_column('bugs', 'target_milestone',
    {TYPE => 'varchar(20)', NOTNULL => 1, DEFAULT => "'---'"});
$dbh->bz_alter_column('milestones', 'value', 
    {TYPE => 'varchar(20)', NOTNULL => 1});


# 2000-03-23 Added a defaultmilestone field to the products table, so that
# we know which milestone to initially assign bugs to.

if (!$dbh->bz_column_info('products', 'defaultmilestone')) {
    $dbh->bz_add_column('products', 'defaultmilestone',
             {TYPE => 'varchar(20)', NOTNULL => 1, DEFAULT => "'---'"});
    $sth = $dbh->prepare("SELECT product, defaultmilestone FROM products");
    $sth->execute();
    while (my ($product, $defaultmilestone) = $sth->fetchrow_array()) {
        $product = $dbh->quote($product);
        $defaultmilestone = $dbh->quote($defaultmilestone);
        my $s2 = $dbh->prepare("SELECT value FROM milestones " .
                               "WHERE value = $defaultmilestone " .
                               "AND product = $product");
        $s2->execute();
        if (!$s2->fetchrow_array()) {
            $dbh->do("INSERT INTO milestones(value, product) " .
                     "VALUES ($defaultmilestone, $product)");
        }
    }
}

# 2000-03-24 Added unique indexes into the cc and keyword tables.  This
# prevents certain database inconsistencies, and, moreover, is required for
# new generalized list code to work.

if (!$dbh->bz_index_info('cc', 'cc_bug_id_idx')->{TYPE}) {

    # XXX should eliminate duplicate entries before altering
    #
    $dbh->bz_drop_index('cc', 'cc_bug_id_idx');
    $dbh->bz_add_index('cc', 'cc_bug_id_idx', 
                      {TYPE => 'UNIQUE', FIELDS => [qw(bug_id who)]});
}    

if (!$dbh->bz_index_info('keywords', 'keywords_bug_id_idx')->{TYPE}) {

    # XXX should eliminate duplicate entries before altering
    #
    $dbh->bz_drop_index('keywords', 'keywords_bug_id_idx');
    $dbh->bz_add_index('keywords', 'keywords_bug_id_idx', 
                      {TYPE => 'UNIQUE', FIELDS => [qw(bug_id keywordid)]});
}    

# 2000-07-15 Added duplicates table so Bugzilla tracks duplicates in a better 
# way than it used to. This code searches the comments to populate the table
# initially. It's executed if the table is empty; if it's empty because there
# are no dupes (as opposed to having just created the table) it won't have
# any effect anyway, so it doesn't matter.
$sth = $dbh->prepare("SELECT count(*) from duplicates");
$sth->execute();
if (!($sth->fetchrow_arrayref()->[0])) {
    # populate table
    print("Populating duplicates table...\n") unless $silent;
    
    $sth = $dbh->prepare(
        "SELECT longdescs.bug_id, thetext " .
          "FROM longdescs " .
     "LEFT JOIN bugs ON longdescs.bug_id = bugs.bug_id " .
         "WHERE (" . $dbh->sql_regexp("thetext",
                 "'[.*.]{3} This bug has been marked as a duplicate of [[:digit:]]+ [.*.]{3}'") . ") " .
           "AND (resolution = 'DUPLICATE') " .
      "ORDER BY longdescs.bug_when");
    $sth->execute();

    my %dupes;
    my $key;

    # Because of the way hashes work, this loop removes all but the last dupe
    # resolution found for a given bug.
    while (my ($dupe, $dupe_of) = $sth->fetchrow_array()) {
        $dupes{$dupe} = $dupe_of;
    }

    foreach $key (keys(%dupes)){
        $dupes{$key} =~ /^.*\*\*\* This bug has been marked as a duplicate of (\d+) \*\*\*$/ms;
        $dupes{$key} = $1;
        $dbh->do("INSERT INTO duplicates VALUES('$dupes{$key}', '$key')");
        #                                        BugItsADupeOf   Dupe
    }
}

# 2000-12-18.  Added an 'emailflags' field for storing preferences about
# when email gets sent on a per-user basis.
if (!$dbh->bz_column_info('profiles', 'emailflags') && 
    !$dbh->bz_column_info('email_setting', 'user_id')) {
    $dbh->bz_add_column('profiles', 'emailflags', {TYPE => 'MEDIUMTEXT'});
}

# 2000-11-27 For Bugzilla 2.5 and later. Copy data from 'comments' to
# 'longdescs' - the new name of the comments table.
if ($dbh->bz_table_info('comments')) {
    my $quoted_when = $dbh->quote_identifier('when');
    # This is MySQL-specific syntax, but that's OK because it will only
    # ever run on MySQL.
    $dbh->do("INSERT INTO longdescs (bug_when, bug_id, who, thetext)
              SELECT $quoted_when, bug_id, who, comment
                FROM comments");
    $dbh->bz_drop_table("comments");
}

#
# 2001-04-10 myk@mozilla.org:
# isactive determines whether or not a group is active.  An inactive group
# cannot have bugs added to it.  Deactivation is a much milder form of
# deleting a group that allows users to continue to work on bugs in the group
# without enabling them to extend the life of the group by adding bugs to it.
# http://bugzilla.mozilla.org/show_bug.cgi?id=75482
#
$dbh->bz_add_column('groups', 'isactive', 
                    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'TRUE'});

#
# 2001-06-15 myk@mozilla.org:
# isobsolete determines whether or not an attachment is pertinent/relevant/valid.
#
$dbh->bz_add_column('attachments', 'isobsolete', 
                    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'FALSE'});

# 2001-04-29 jake@bugzilla.org - Remove oldemailtech
#   http://bugzilla.mozilla.org/show_bugs.cgi?id=71552
if (-d 'shadow') {
    print "Removing shadow directory...\n";
    unlink glob("shadow/*");
    unlink glob("shadow/.*");
    rmdir "shadow";
}
$dbh->bz_drop_column("profiles", "emailnotification");
$dbh->bz_drop_column("profiles", "newemailtech");


# 2003-11-19; chicks@chicks.net; bug 225973: fix field size to accommodate
# wider algorithms such as Blowfish. Note that this needs to be run
# before recrypting passwords in the following block.
$dbh->bz_alter_column('profiles', 'cryptpassword', {TYPE => 'varchar(128)'});

# 2001-06-12; myk@mozilla.org; bugs 74032, 77473:
# Recrypt passwords using Perl &crypt instead of the mysql equivalent
# and delete plaintext passwords from the database.
if ($dbh->bz_column_info('profiles', 'password')) {
    
    print <<ENDTEXT;
Your current installation of Bugzilla stores passwords in plaintext 
in the database and uses mysql's encrypt function instead of Perl's 
crypt function to crypt passwords.  Passwords are now going to be 
re-crypted with the Perl function, and plaintext passwords will be 
deleted from the database.  This could take a while if your  
installation has many users. 
ENDTEXT

    # Re-crypt everyone's password.
    my $sth = $dbh->prepare("SELECT userid, password FROM profiles");
    $sth->execute();

    my $i = 1;

    print "Fixing password #1... ";
    while (my ($userid, $password) = $sth->fetchrow_array()) {
        my $cryptpassword = $dbh->quote(bz_crypt($password));
        $dbh->do("UPDATE profiles " .
                    "SET cryptpassword = $cryptpassword " .
                  "WHERE userid = $userid");
        ++$i;
        # Let the user know where we are at every 500 records.
        print "$i... " if !($i%500);
    }
    print "$i... Done.\n";

    # Drop the plaintext password field.
    $dbh->bz_drop_column('profiles', 'password');
}

#
# 2001-06-06 justdave@syndicomm.com:
# There was no index on the 'who' column in the long descriptions table.
# This caused queries by who posted comments to take a LONG time.
#   http://bugzilla.mozilla.org/show_bug.cgi?id=57350
$dbh->bz_add_index('longdescs', 'longdescs_who_idx', [qw(who)]);

# 2001-06-15 kiko@async.com.br - Change bug:version size to avoid
# truncates re http://bugzilla.mozilla.org/show_bug.cgi?id=9352
$dbh->bz_alter_column('bugs', 'version', 
                      {TYPE => 'varchar(64)', NOTNULL => 1});

# 2001-07-20 jake@bugzilla.org - Change bugs_activity to only record changes
#  http://bugzilla.mozilla.org/show_bug.cgi?id=55161
if ($dbh->bz_column_info('bugs_activity', 'oldvalue')) {
    $dbh->bz_add_column("bugs_activity", "removed", {TYPE => "TINYTEXT"});
    $dbh->bz_add_column("bugs_activity", "added", {TYPE => "TINYTEXT"});

    # Need to get field id's for the fields that have multiple values
    my @multi = ();
    foreach my $f ("cc", "dependson", "blocked", "keywords") {
        my $sth = $dbh->prepare("SELECT id " .
                                "FROM fielddefs " .
                                "WHERE name = '$f'");
        $sth->execute();
        my ($fid) = $sth->fetchrow_array();
        push (@multi, $fid);
    } 

    # Now we need to process the bugs_activity table and reformat the data
    my $i = 0;
    print "Fixing activity log ";
    my $sth = $dbh->prepare("SELECT bug_id, who, bug_when, fieldid,
                            oldvalue, newvalue FROM bugs_activity");
    $sth->execute;
    while (my ($bug_id, $who, $bug_when, $fieldid, $oldvalue, $newvalue) = $sth->fetchrow_array()) {
        # print the iteration count every 500 records 
        # so the user knows we didn't die
        print "$i..." if !($i++ % 500); 
        # Make sure (old|new)value isn't null (to suppress warnings)
        $oldvalue ||= "";
        $newvalue ||= "";
        my ($added, $removed) = "";
        if (grep ($_ eq $fieldid, @multi)) {
            $oldvalue =~ s/[\s,]+/ /g;
            $newvalue =~ s/[\s,]+/ /g;
            my @old = split(" ", $oldvalue);
            my @new = split(" ", $newvalue);
            my (@add, @remove) = ();
            # Find values that were "added"
            foreach my $value(@new) {
                if (! grep ($_ eq $value, @old)) {
                    push (@add, $value);
                }
            }
            # Find values that were removed
            foreach my $value(@old) {
                if (! grep ($_ eq $value, @new)) {
                    push (@remove, $value);
                }
            }
            $added = join (", ", @add);
            $removed = join (", ", @remove);
            # If we can't determine what changed, put a ? in both fields
            unless ($added || $removed) {
                $added = "?";
                $removed = "?";
            }
            # If the original field (old|new)value was full, then this
            # could be incomplete data.
            if (length($oldvalue) == 255 || length($newvalue) == 255) {
                $added = "? $added";
                $removed = "? $removed";
            }
        } else {
            $removed = $oldvalue;
            $added = $newvalue;
        }
        $added = $dbh->quote($added);
        $removed = $dbh->quote($removed);
        $dbh->do("UPDATE bugs_activity SET removed = $removed, added = $added
                  WHERE bug_id = $bug_id AND who = $who
                   AND bug_when = '$bug_when' AND fieldid = $fieldid");
    }
    print ". Done.\n";
    $dbh->bz_drop_column("bugs_activity", "oldvalue");
    $dbh->bz_drop_column("bugs_activity", "newvalue");
} 

$dbh->bz_alter_column("profiles", "disabledtext", 
                      {TYPE => 'MEDIUMTEXT', NOTNULL => 1}, '');

# 2001-07-26 myk@mozilla.org            bug 39816 (original)
# 2002-02-06 bbaetz@student.usyd.edu.au bug 97471 (revision)
# Add fields to the bugs table that record whether or not the reporter
# and users on the cc: list can see bugs even when
# they are not members of groups to which the bugs are restricted.
$dbh->bz_add_column("bugs", "reporter_accessible", 
                    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'TRUE'});
$dbh->bz_add_column("bugs", "cclist_accessible", 
                    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'TRUE'});

# 2001-08-21 myk@mozilla.org bug84338:
# Add a field to the bugs_activity table for the attachment ID, so installations
# using the attachment manager can record changes to attachments.
$dbh->bz_add_column("bugs_activity", "attach_id", {TYPE => 'INT3'});

# 2002-02-04 bbaetz@student.usyd.edu.au bug 95732
# Remove logincookies.cryptpassword, and delete entries which become
# invalid
if ($dbh->bz_column_info("logincookies", "cryptpassword")) {
    # We need to delete any cookies which are invalid before dropping the
    # column

    print "Removing invalid login cookies...\n";

    # mysql doesn't support DELETE with multi-table queries, so we have
    # to iterate
    my $sth = $dbh->prepare("SELECT cookie FROM logincookies, profiles " .
                            "WHERE logincookies.cryptpassword != " .
                            "profiles.cryptpassword AND " .
                            "logincookies.userid = profiles.userid");
    $sth->execute();
    while (my ($cookie) = $sth->fetchrow_array()) {
        $dbh->do("DELETE FROM logincookies WHERE cookie = $cookie");
    }

    $dbh->bz_drop_column("logincookies", "cryptpassword");
}

# 2002-02-13 bbaetz@student.usyd.edu.au - bug 97471
# qacontact/assignee should always be able to see bugs,
# so remove their restriction column
if ($dbh->bz_column_info("bugs", "qacontact_accessible")) {
    print "Removing restrictions on bugs for assignee and qacontact...\n";

    $dbh->bz_drop_column("bugs", "qacontact_accessible");
    $dbh->bz_drop_column("bugs", "assignee_accessible");
}

# 2002-02-20 jeff.hedlund@matrixsi.com - bug 24789 time tracking
$dbh->bz_add_column("longdescs", "work_time", 
                    {TYPE => 'decimal(5,2)', NOTNULL => 1, DEFAULT => '0'});
$dbh->bz_add_column("bugs", "estimated_time", 
                    {TYPE => 'decimal(5,2)', NOTNULL => 1, DEFAULT => '0'});
$dbh->bz_add_column("bugs", "remaining_time",
                    {TYPE => 'decimal(5,2)', NOTNULL => 1, DEFAULT => '0'});
$dbh->bz_add_column("bugs", "deadline", {TYPE => 'DATETIME'});

# 2002-03-15 bbaetz@student.usyd.edu.au - bug 129466
# 2002-05-13 preed@sigkill.com - bug 129446 patch backported to the 
#  BUGZILLA-2_14_1-BRANCH as a security blocker for the 2.14.2 release
# 
# Use the ip, not the hostname, in the logincookies table
if ($dbh->bz_column_info("logincookies", "hostname")) {
    # We've changed what we match against, so all entries are now invalid
    $dbh->do("DELETE FROM logincookies");

    # Now update the logincookies schema
    $dbh->bz_drop_column("logincookies", "hostname");
    $dbh->bz_add_column("logincookies", "ipaddr", 
                        {TYPE => 'varchar(40)', NOTNULL => 1}, '');
}

# 2002-08-19 - bugreport@peshkin.net bug 143826
# Add private comments and private attachments on less-private bugs
$dbh->bz_add_column('longdescs', 'isprivate', 
                    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'FALSE'});
$dbh->bz_add_column('attachments', 'isprivate',
                    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'FALSE'});


# 2002-07-03 myk@mozilla.org bug99203:
# Add a bug alias field to the bugs table so bugs can be referenced by alias
# in addition to ID.
if (!$dbh->bz_column_info("bugs", "alias")) {
    $dbh->bz_add_column("bugs", "alias", {TYPE => "varchar(20)"});
    $dbh->bz_add_index('bugs', 'bugs_alias_idx', 
                       {TYPE => 'UNIQUE', FIELDS => [qw(alias)]});
}

# 2002-07-15 davef@tetsubo.com - bug 67950
# Move quips to the db.
if (-r "$datadir/comments" && -s "$datadir/comments"
    && open (COMMENTS, "<$datadir/comments")) {
    print "Populating quips table from $datadir/comments...\n\n";
    while (<COMMENTS>) {
        chomp;
        $dbh->do("INSERT INTO quips (quip) VALUES ("
                 . $dbh->quote($_) . ")");
    }
    print "Quips are now stored in the database, rather than in an external file.\n" .
          "The quips previously stored in $datadir/comments have been copied into\n" .
          "the database, and that file has been renamed to $datadir/comments.bak\n"  .
          "You may delete the renamed file once you have confirmed that all your \n" .
          "quips were moved successfully.\n\n";
    close COMMENTS;
    rename("$datadir/comments", "$datadir/comments.bak");
}

# 2002-07-31 bbaetz@student.usyd.edu.au bug 158236
# Remove unused column
if ($dbh->bz_column_info("namedqueries", "watchfordiffs")) {
    $dbh->bz_drop_column("namedqueries", "watchfordiffs");
}

# 2002-08-12 jake@bugzilla.org/bbaetz@student.usyd.edu.au - bug 43600
# Use integer IDs for products and components.
if ($dbh->bz_column_info("products", "product")) {
    print "Updating database to use product IDs.\n";

    # First, we need to remove possible NULL entries
    # NULLs may exist, but won't have been used, since all the uses of them
    # are in NOT NULL fields in other tables
    $dbh->do("DELETE FROM products WHERE product IS NULL");
    $dbh->do("DELETE FROM components WHERE value IS NULL");

    $dbh->bz_add_column("products", "id", 
        {TYPE => 'SMALLSERIAL', NOTNULL => 1, PRIMARYKEY => 1});
    $dbh->bz_add_column("components", "product_id", 
        {TYPE => 'INT2', NOTNULL => 1}, 0);
    $dbh->bz_add_column("versions", "product_id", 
        {TYPE => 'INT2', NOTNULL => 1}, 0);
    $dbh->bz_add_column("milestones", "product_id", 
        {TYPE => 'INT2', NOTNULL => 1}, 0);
    $dbh->bz_add_column("bugs", "product_id",
        {TYPE => 'INT2', NOTNULL => 1}, 0);

    # The attachstatusdefs table was added in version 2.15, but removed again
    # in early 2.17.  If it exists now, we still need to perform this change
    # with product_id because the code further down which converts the
    # attachment statuses to flags depends on it.  But we need to avoid this
    # if the user is upgrading from 2.14 or earlier (because it won't be
    # there to convert).
    if ($dbh->bz_table_info("attachstatusdefs")) {
        $dbh->bz_add_column("attachstatusdefs", "product_id", 
            {TYPE => 'INT2', NOTNULL => 1}, 0);
    }

    my %products;
    my $sth = $dbh->prepare("SELECT id, product FROM products");
    $sth->execute;
    while (my ($product_id, $product) = $sth->fetchrow_array()) {
        if (exists $products{$product}) {
            print "Ignoring duplicate product $product\n";
            $dbh->do("DELETE FROM products WHERE id = $product_id");
            next;
        }
        $products{$product} = 1;
        $dbh->do("UPDATE components SET product_id = $product_id " .
                 "WHERE program = " . $dbh->quote($product));
        $dbh->do("UPDATE versions SET product_id = $product_id " .
                 "WHERE program = " . $dbh->quote($product));
        $dbh->do("UPDATE milestones SET product_id = $product_id " .
                 "WHERE product = " . $dbh->quote($product));
        $dbh->do("UPDATE bugs SET product_id = $product_id " .
                 "WHERE product = " . $dbh->quote($product));
        $dbh->do("UPDATE attachstatusdefs SET product_id = $product_id " .
                 "WHERE product = " . $dbh->quote($product)) 
            if $dbh->bz_table_info("attachstatusdefs");
    }

    print "Updating the database to use component IDs.\n";
    $dbh->bz_add_column("components", "id", 
        {TYPE => 'SMALLSERIAL', NOTNULL => 1, PRIMARYKEY => 1});
    $dbh->bz_add_column("bugs", "component_id",
                        {TYPE => 'INT2', NOTNULL => 1}, 0);

    my %components;
    $sth = $dbh->prepare("SELECT id, value, product_id FROM components");
    $sth->execute;
    while (my ($component_id, $component, $product_id) = $sth->fetchrow_array()) {
        if (exists $components{$component}) {
            if (exists $components{$component}{$product_id}) {
                print "Ignoring duplicate component $component for product $product_id\n";
                $dbh->do("DELETE FROM components WHERE id = $component_id");
                next;
            }
        } else {
            $components{$component} = {};
        }
        $components{$component}{$product_id} = 1;
        $dbh->do("UPDATE bugs SET component_id = $component_id " .
                  "WHERE component = " . $dbh->quote($component) .
                   " AND product_id = $product_id");
    }
    print "Fixing Indexes and Uniqueness.\n";
    $dbh->bz_drop_index('milestones', 'milestones_product_idx');

    $dbh->bz_add_index('milestones', 'milestones_product_id_idx',
        {TYPE => 'UNIQUE', FIELDS => [qw(product_id value)]});

    $dbh->bz_drop_index('bugs', 'bugs_product_idx');
    $dbh->bz_add_index('bugs', 'bugs_product_id_idx', [qw(product_id)]);
    $dbh->bz_drop_index('bugs', 'bugs_component_idx');
    $dbh->bz_add_index('bugs', 'bugs_component_id_idx', [qw(component_id)]);

    print "Removing, renaming, and retyping old product and component fields.\n";
    $dbh->bz_drop_column("components", "program");
    $dbh->bz_drop_column("versions", "program");
    $dbh->bz_drop_column("milestones", "product");
    $dbh->bz_drop_column("bugs", "product");
    $dbh->bz_drop_column("bugs", "component");
    $dbh->bz_drop_column("attachstatusdefs", "product")
        if $dbh->bz_table_info("attachstatusdefs");
    $dbh->bz_rename_column("products", "product", "name");
    $dbh->bz_alter_column("products", "name",
                          {TYPE => 'varchar(64)', NOTNULL => 1});
    $dbh->bz_rename_column("components", "value", "name");
    $dbh->bz_alter_column("components", "name",
                          {TYPE => 'varchar(64)', NOTNULL => 1});

    print "Adding indexes for products and components tables.\n";
    $dbh->bz_add_index('products', 'products_name_idx',
                       {TYPE => 'UNIQUE', FIELDS => [qw(name)]});
    $dbh->bz_add_index('components', 'components_product_id_idx',
        {TYPE => 'UNIQUE', FIELDS => [qw(product_id name)]});
    $dbh->bz_add_index('components', 'components_name_idx', [qw(name)]);
}

# 2002-09-22 - bugreport@peshkin.net - bug 157756
#
# If the whole groups system is new, but the installation isn't, 
# convert all the old groupset groups, etc...
#
# This requires:
# 1) define groups ids in group table
# 2) populate user_group_map with grants from old groupsets and blessgroupsets
# 3) populate bug_group_map with data converted from old bug groupsets
# 4) convert activity logs to use group names instead of numbers
# 5) identify the admin from the old all-ones groupset
#
# ListBits(arg) returns a list of UNKNOWN<n> if the group
# has been deleted for all bits set in arg. When the activity
# records are converted from groupset numbers to lists of
# group names, ListBits is used to fill in a list of references
# to groupset bits for groups that no longer exist.
# 
sub ListBits {
    my ($num) = @_;
    my @res = ();
    my $curr = 1;
    while (1) {
        # Convert a big integer to a list of bits 
        my $sth = $dbh->prepare("SELECT ($num & ~$curr) > 0, 
                                        ($num & $curr), 
                                        ($num & ~$curr), 
                                        $curr << 1");
        $sth->execute;
        my ($more, $thisbit, $remain, $nval) = $sth->fetchrow_array;
        push @res,"UNKNOWN<$curr>" if ($thisbit);
        $curr = $nval;
        $num = $remain;
        last if (!$more);
    }
    return @res;
}

# The groups system needs to be converted if groupset exists
if ($dbh->bz_column_info("profiles", "groupset")) {
    $dbh->bz_add_column('groups', 'last_changed', 
        {TYPE => 'DATETIME', NOTNULL => 1}, '0000-00-00 00:00:00');

    # Some mysql versions will promote any unique key to primary key
    # so all unique keys are removed first and then added back in
    $dbh->bz_drop_index('groups', 'groups_bit_idx');
    $dbh->bz_drop_index('groups', 'groups_name_idx');
    if ($dbh->primary_key(undef, undef, 'groups')) {
        $dbh->do("ALTER TABLE groups DROP PRIMARY KEY");
    }

    $dbh->bz_add_column('groups', 'id',
        {TYPE => 'MEDIUMSERIAL', NOTNULL => 1, PRIMARYKEY => 1});

    $dbh->bz_add_index('groups', 'groups_name_idx', 
                       {TYPE => 'UNIQUE', FIELDS => [qw(name)]});
    $dbh->bz_add_column('profiles', 'refreshed_when',
        {TYPE => 'DATETIME', NOTNULL => 1}, '0000-00-00 00:00:00');

    # Convert all existing groupset records to map entries before removing
    # groupset fields or removing "bit" from groups.
    $sth = $dbh->prepare("SELECT bit, id FROM groups
                WHERE bit > 0");
    $sth->execute();
    while (my ($bit, $gid) = $sth->fetchrow_array) {
        # Create user_group_map membership grants for old groupsets.
        # Get each user with the old groupset bit set
        my $sth2 = $dbh->prepare("SELECT userid FROM profiles
                   WHERE (groupset & $bit) != 0");
        $sth2->execute();
        while (my ($uid) = $sth2->fetchrow_array) {
            # Check to see if the user is already a member of the group
            # and, if not, insert a new record.
            my $query = "SELECT user_id FROM user_group_map 
                WHERE group_id = $gid AND user_id = $uid 
                AND isbless = 0"; 
            my $sth3 = $dbh->prepare($query);
            $sth3->execute();
            if ( !$sth3->fetchrow_array() ) {
                $dbh->do("INSERT INTO user_group_map
                       (user_id, group_id, isbless, grant_type)
                       VALUES($uid, $gid, 0, " . GRANT_DIRECT . ")");
            }
        }
        # Create user can bless group grants for old groupsets, but only
        # if we're upgrading from a Bugzilla that had blessing.
        if($dbh->bz_column_info('profiles', 'blessgroupset')) {
            # Get each user with the old blessgroupset bit set
            $sth2 = $dbh->prepare("SELECT userid FROM profiles
                       WHERE (blessgroupset & $bit) != 0");
            $sth2->execute();
            while (my ($uid) = $sth2->fetchrow_array) {
                $dbh->do("INSERT INTO user_group_map
                       (user_id, group_id, isbless, grant_type)
                       VALUES($uid, $gid, 1, " . GRANT_DIRECT . ")");
            }
        }
        # Create bug_group_map records for old groupsets.
        # Get each bug with the old group bit set.
        $sth2 = $dbh->prepare("SELECT bug_id FROM bugs
                   WHERE (groupset & $bit) != 0");
        $sth2->execute();
        while (my ($bug_id) = $sth2->fetchrow_array) {
            # Insert the bug, group pair into the bug_group_map.
            $dbh->do("INSERT INTO bug_group_map
                   (bug_id, group_id)
                   VALUES($bug_id, $gid)");
        }
    }
    # Replace old activity log groupset records with lists of names of groups.
    # Start by defining the bug_group field and getting its id.
    AddFDef("bug_group", "Group", 0);
    $sth = $dbh->prepare("SELECT id " .
                           "FROM fielddefs " .
                          "WHERE name = " . $dbh->quote('bug_group'));
    $sth->execute();
    my ($bgfid) = $sth->fetchrow_array;
    # Get the field id for the old groupset field
    $sth = $dbh->prepare("SELECT id " .
                           "FROM fielddefs " .
                          "WHERE name = " . $dbh->quote('groupset'));
    $sth->execute();
    my ($gsid) = $sth->fetchrow_array;
    # Get all bugs_activity records from groupset changes
    if ($gsid) {
        $sth = $dbh->prepare("SELECT bug_id, bug_when, who, added, removed
                              FROM bugs_activity WHERE fieldid = $gsid");
        $sth->execute();
        while (my ($bug_id, $bug_when, $who, $added, $removed) = $sth->fetchrow_array) {
            $added ||= 0;
            $removed ||= 0;
            # Get names of groups added.
            my $sth2 = $dbh->prepare("SELECT name " .
                                       "FROM groups " .
                                      "WHERE (bit & $added) != 0 " .
                                        "AND (bit & $removed) = 0");
            $sth2->execute();
            my @logadd = ();
            while (my ($n) = $sth2->fetchrow_array) {
                push @logadd, $n;
            }
            # Get names of groups removed.
            $sth2 = $dbh->prepare("SELECT name " .
                                    "FROM groups " .
                                   "WHERE (bit & $removed) != 0 " .
                                     "AND (bit & $added) = 0");
            $sth2->execute();
            my @logrem = ();
            while (my ($n) = $sth2->fetchrow_array) {
                push @logrem, $n;
            }
            # Get list of group bits added that correspond to missing groups.
            $sth2 = $dbh->prepare("SELECT ($added & ~BIT_OR(bit)) FROM groups");
            $sth2->execute();
            my ($miss) = $sth2->fetchrow_array;
            if ($miss) {
                push @logadd, ListBits($miss);
                print "\nWARNING - GROUPSET ACTIVITY ON BUG $bug_id CONTAINS DELETED GROUPS\n";
            }
            # Get list of group bits deleted that correspond to missing groups.
            $sth2 = $dbh->prepare("SELECT ($removed & ~BIT_OR(bit)) FROM groups");
            $sth2->execute();
            ($miss) = $sth2->fetchrow_array;
            if ($miss) {
                push @logrem, ListBits($miss);
                print "\nWARNING - GROUPSET ACTIVITY ON BUG $bug_id CONTAINS DELETED GROUPS\n";
            }
            my $logr = "";
            my $loga = "";
            $logr = join(", ", @logrem) . '?' if @logrem;
            $loga = join(", ", @logadd) . '?' if @logadd;
            # Replace to old activity record with the converted data.
            $dbh->do("UPDATE bugs_activity SET fieldid = $bgfid, added = " .
                      $dbh->quote($loga) . ", removed = " . 
                      $dbh->quote($logr) .
                     " WHERE bug_id = $bug_id AND bug_when = " . 
                      $dbh->quote($bug_when) .
                     " AND who = $who AND fieldid = $gsid");
    
        }
        # Replace groupset changes with group name changes in profiles_activity.
        # Get profiles_activity records for groupset.
        $sth = $dbh->prepare(
             "SELECT userid, profiles_when, who, newvalue, oldvalue " .
               "FROM profiles_activity " .
              "WHERE fieldid = $gsid");
        $sth->execute();
        while (my ($uid, $uwhen, $uwho, $added, $removed) = $sth->fetchrow_array) {
            $added ||= 0;
            $removed ||= 0;
            # Get names of groups added.
            my $sth2 = $dbh->prepare("SELECT name " .
                                       "FROM groups " .
                                      "WHERE (bit & $added) != 0 " .
                                        "AND (bit & $removed) = 0");
            $sth2->execute();
            my @logadd = ();
            while (my ($n) = $sth2->fetchrow_array) {
                push @logadd, $n;
            }
            # Get names of groups removed.
            $sth2 = $dbh->prepare("SELECT name " .
                                    "FROM groups " .
                                   "WHERE (bit & $removed) != 0 " .
                                     "AND (bit & $added) = 0");
            $sth2->execute();
            my @logrem = ();
            while (my ($n) = $sth2->fetchrow_array) {
                push @logrem, $n;
            }
            my $ladd = "";
            my $lrem = "";
            $ladd = join(", ", @logadd) . '?' if @logadd;
            $lrem = join(", ", @logrem) . '?' if @logrem;
            # Replace profiles_activity record for groupset change 
            # with group list.
            $dbh->do("UPDATE profiles_activity " .
                     "SET fieldid = $bgfid, newvalue = " .
                      $dbh->quote($ladd) . ", oldvalue = " . 
                      $dbh->quote($lrem) .
                      " WHERE userid = $uid AND profiles_when = " . 
                      $dbh->quote($uwhen) .
                      " AND who = $uwho AND fieldid = $gsid");
    
        }
    }
    # Identify admin group.
    my $sth = $dbh->prepare("SELECT id FROM groups 
                WHERE name = 'admin'");
    $sth->execute();
    my ($adminid) = $sth->fetchrow_array();
    # find existing admins
    # Don't lose admins from DBs where Bug 157704 applies
    $sth = $dbh->prepare(
           "SELECT userid, (groupset & 65536), login_name " .
             "FROM profiles " .
            "WHERE (groupset | 65536) = 9223372036854775807");
    $sth->execute();
    while ( my ($userid, $iscomplete, $login_name) = $sth->fetchrow_array() ) {
        # existing administrators are made members of group "admin"
        print "\nWARNING - $login_name IS AN ADMIN IN SPITE OF BUG 157704\n\n"
            if (!$iscomplete);
        push @admins, $userid;
    }
    $dbh->bz_drop_column('profiles','groupset');
    $dbh->bz_drop_column('profiles','blessgroupset');
    $dbh->bz_drop_column('bugs','groupset');
    $dbh->bz_drop_column('groups','bit');
    $dbh->do("DELETE FROM fielddefs WHERE name = " . $dbh->quote('groupset'));
}

# September 2002 myk@mozilla.org bug 98801
# Convert the attachment statuses tables into flags tables.
if ($dbh->bz_table_info("attachstatuses") 
    && $dbh->bz_table_info("attachstatusdefs")) 
{
    print "Converting attachment statuses to flags...\n";
    
    # Get IDs for the old attachment status and new flag fields.
    my ($old_field_id) = $dbh->selectrow_array(
        "SELECT id FROM fielddefs WHERE name='attachstatusdefs.name'")
        || 0;
    
    $sth = $dbh->prepare("SELECT id FROM fielddefs " . 
                         "WHERE name='flagtypes.name'");
    $sth->execute();
    my $new_field_id = $sth->fetchrow_arrayref()->[0];

    # Convert attachment status definitions to flag types.  If more than one
    # status has the same name and description, it is merged into a single 
    # status with multiple inclusion records.
    $sth = $dbh->prepare("SELECT id, name, description, sortkey, product_id " . 
                         "FROM attachstatusdefs");
    
    # status definition IDs indexed by name/description
    my $def_ids = {};
    
    # merged IDs and the IDs they were merged into.  The key is the old ID,
    # the value is the new one.  This allows us to give statuses the right
    # ID when we convert them over to flags.  This map includes IDs that
    # weren't merged (in this case the old and new IDs are the same), since 
    # it makes the code simpler.
    my $def_id_map = {};
    
    $sth->execute();
    while (my ($id, $name, $desc, $sortkey, $prod_id) = $sth->fetchrow_array()) {
        my $key = $name . $desc;
        if (!$def_ids->{$key}) {
            $def_ids->{$key} = $id;
            my $quoted_name = $dbh->quote($name);
            my $quoted_desc = $dbh->quote($desc);
            $dbh->do("INSERT INTO flagtypes (id, name, description, sortkey, " .
                     "target_type) VALUES ($id, $quoted_name, $quoted_desc, " .
                     "$sortkey, 'a')");
        }
        $def_id_map->{$id} = $def_ids->{$key};
        $dbh->do("INSERT INTO flaginclusions (type_id, product_id) " . 
                "VALUES ($def_id_map->{$id}, $prod_id)");
    }
    
    # Note: even though we've converted status definitions, we still can't drop
    # the table because we need it to convert the statuses themselves.
    
    # Convert attachment statuses to flags.  To do this we select the statuses
    # from the status table and then, for each one, figure out who set it
    # and when they set it from the bugs activity table.
    my $id = 0;
    $sth = $dbh->prepare(
        "SELECT attachstatuses.attach_id, attachstatusdefs.id, " . 
               "attachstatusdefs.name, attachments.bug_id " . 
          "FROM attachstatuses, attachstatusdefs, attachments " . 
         "WHERE attachstatuses.statusid = attachstatusdefs.id " .
           "AND attachstatuses.attach_id = attachments.attach_id");
    
    # a query to determine when the attachment status was set and who set it
    my $sth2 = $dbh->prepare("SELECT added, who, bug_when " . 
                             "FROM bugs_activity " . 
                             "WHERE bug_id = ? AND attach_id = ? " . 
                             "AND fieldid = $old_field_id " . 
                             "ORDER BY bug_when DESC");
    
    $sth->execute();
    while (my ($attach_id, $def_id, $status, $bug_id) = $sth->fetchrow_array()) {
        ++$id;
        
        # Determine when the attachment status was set and who set it.
        # We should always be able to find out this info from the bug activity,
        # but we fall back to default values just in case.
        $sth2->execute($bug_id, $attach_id);
        my ($added, $who, $when);
        while (($added, $who, $when) = $sth2->fetchrow_array()) {
            last if $added =~ /(^|[, ]+)\Q$status\E([, ]+|$)/;
        }
        $who = $dbh->quote($who); # "NULL" by default if $who is undefined
        $when = $when ? $dbh->quote($when) : "NOW()";
            
        
        $dbh->do("INSERT INTO flags (id, type_id, status, bug_id, attach_id, " .
                 "creation_date, modification_date, requestee_id, setter_id) " . 
                 "VALUES ($id, $def_id_map->{$def_id}, '+', $bug_id, " . 
                 "$attach_id, $when, $when, NULL, $who)");
    }
    
    # Now that we've converted both tables we can drop them.
    $dbh->bz_drop_table("attachstatuses");
    $dbh->bz_drop_table("attachstatusdefs");
    
    # Convert activity records for attachment statuses into records for flags.
    my $sth = $dbh->prepare("SELECT attach_id, who, bug_when, added, removed " .
                            "FROM bugs_activity WHERE fieldid = $old_field_id");
    $sth->execute();
    while (my ($attach_id, $who, $when, $old_added, $old_removed) = 
      $sth->fetchrow_array())
    {
        my @additions = split(/[, ]+/, $old_added);
        @additions = map("$_+", @additions);
        my $new_added = $dbh->quote(join(", ", @additions));
        
        my @removals = split(/[, ]+/, $old_removed);
        @removals = map("$_+", @removals);
        my $new_removed = $dbh->quote(join(", ", @removals));
        
        $old_added = $dbh->quote($old_added);
        $old_removed = $dbh->quote($old_removed);
        $who = $dbh->quote($who);
        $when = $dbh->quote($when);
        
        $dbh->do("UPDATE bugs_activity SET fieldid = $new_field_id, " . 
                 "added = $new_added, removed = $new_removed " . 
                 "WHERE attach_id = $attach_id AND who = $who " . 
                 "AND bug_when = $when AND fieldid = $old_field_id " . 
                 "AND added = $old_added AND removed = $old_removed");
    }
    
    # Remove the attachment status field from the field definitions.
    $dbh->do("DELETE FROM fielddefs WHERE name='attachstatusdefs.name'");

    print "done.\n";
}

# 2004-12-13 Nick.Barnes@pobox.com bug 262268
# Check for spaces and commas in flag type names; if found, rename them.
if ($dbh->bz_table_info("flagtypes")) {
    # Get all names and IDs, to find broken ones and to
    # check for collisions when renaming.
    $sth = $dbh->prepare("SELECT name, id FROM flagtypes");
    $sth->execute();

    my %flagtypes;
    my @badflagnames;
    
    # find broken flagtype names, and populate a hash table
    # to check for collisions.
    while (my ($name, $id) = $sth->fetchrow_array()) {
        $flagtypes{$name} = $id;
        if ($name =~ /[ ,]/) {
            push(@badflagnames, $name);
        }
    }
    if (@badflagnames) {
        print "Removing spaces and commas from flag names...\n";
        my ($flagname, $tryflagname);
        my $sth = $dbh->prepare("UPDATE flagtypes SET name = ? WHERE id = ?");
        foreach $flagname (@badflagnames) {
            print "  Bad flag type name \"$flagname\" ...\n";
            # find a new name for this flagtype.
            ($tryflagname = $flagname) =~ tr/ ,/__/;
            # avoid collisions with existing flagtype names.
            while (defined($flagtypes{$tryflagname})) {
                print "  ... can't rename as \"$tryflagname\" ...\n";
                $tryflagname .= "'";
                if (length($tryflagname) > 50) {
                    my $lastchanceflagname = (substr $tryflagname, 0, 47) . '...';
                    if (defined($flagtypes{$lastchanceflagname})) {
                        print "  ... last attempt as \"$lastchanceflagname\" still failed.'\n",
                              "Rename the flag by hand and run checksetup.pl again.\n";
                        die("Bad flag type name $flagname");
                    }
                    $tryflagname = $lastchanceflagname;
                }
            }
            $sth->execute($tryflagname, $flagtypes{$flagname});
            print "  renamed flag type \"$flagname\" as \"$tryflagname\"\n";
            $flagtypes{$tryflagname} = $flagtypes{$flagname};
            delete $flagtypes{$flagname};
        }
        print "... done.\n";
    }
}

# 2002-11-24 - bugreport@peshkin.net - bug 147275 
#
# If group_control_map is empty, backward-compatibility 
# usebuggroups-equivalent records should be created.
my $entry = Bugzilla->params->{'useentrygroupdefault'};
$sth = $dbh->prepare("SELECT COUNT(*) FROM group_control_map");
$sth->execute();
my ($mapcnt) = $sth->fetchrow_array();
if ($mapcnt == 0) {
    # Initially populate group_control_map.
    # First, get all the existing products and their groups.
    $sth = $dbh->prepare("SELECT groups.id, products.id, groups.name, " .
                         "products.name FROM groups, products " .
                         "WHERE isbuggroup != 0");
    $sth->execute();
    while (my ($groupid, $productid, $groupname, $productname) 
            = $sth->fetchrow_array()) {
        if ($groupname eq $productname) {
            # Product and group have same name.
            $dbh->do("INSERT INTO group_control_map " .
                     "(group_id, product_id, entry, membercontrol, " .
                     "othercontrol, canedit) " .
                     "VALUES ($groupid, $productid, $entry, " .
                     CONTROLMAPDEFAULT . ", " .
                     CONTROLMAPNA . ", 0)");
        } else {
            # See if this group is a product group at all.
            my $sth2 = $dbh->prepare("SELECT id FROM products WHERE name = " .
                                 $dbh->quote($groupname));
            $sth2->execute();
            my ($id) = $sth2->fetchrow_array();
            if (!$id) {
                # If there is no product with the same name as this
                # group, then it is permitted for all products.
                $dbh->do("INSERT INTO group_control_map " .
                         "(group_id, product_id, entry, membercontrol, " .
                         "othercontrol, canedit) " .
                         "VALUES ($groupid, $productid, 0, " .
                         CONTROLMAPSHOWN . ", " .
                         CONTROLMAPNA . ", 0)");
            }
        }
    }
}

# 2004-07-17 GRM - Remove "subscriptions" concept from charting, and add
# group-based security instead. 
if ($dbh->bz_table_info("user_series_map")) {
    # Oracle doesn't like "date" as a column name, and apparently some DBs
    # don't like 'value' either. We use the changes to subscriptions as 
    # something to hang these renamings off.
    $dbh->bz_rename_column('series_data', 'date', 'series_date');
    $dbh->bz_rename_column('series_data', 'value', 'series_value');
    
    # series_categories.category_id produces a too-long column name for the
    # auto-incrementing sequence (Oracle again).
    $dbh->bz_rename_column('series_categories', 'category_id', 'id');
    
    $dbh->bz_add_column("series", "public", 
        {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'FALSE'});

    # Migrate public-ness across from user_series_map to new field
    $sth = $dbh->prepare("SELECT series_id from user_series_map " .
                         "WHERE user_id = 0");
    $sth->execute();
    while (my ($public_series_id) = $sth->fetchrow_array()) {
        $dbh->do("UPDATE series SET public = 1 " .
                 "WHERE series_id = $public_series_id");
    }

    $dbh->bz_drop_table("user_series_map");
}    

# 2003-06-26 Copy the old charting data into the database, and create the
# queries that will keep it all running. When the old charting system goes
# away, if this code ever runs, it'll just find no files and do nothing.
my $series_exists = $dbh->selectrow_array("SELECT 1 FROM series " .
                                          $dbh->sql_limit(1));
if (!$series_exists) {
    print "Migrating old chart data into database ...\n" unless $silent;
    
    require Bugzilla::Series;
      
    # We prepare the handle to insert the series data    
    my $seriesdatasth = $dbh->prepare("INSERT INTO series_data " . 
                                     "(series_id, series_date, series_value) " .
                                     "VALUES (?, ?, ?)");

    my $deletesth = $dbh->prepare("DELETE FROM series_data 
                                   WHERE series_id = ? AND series_date = ?");
    
    my $groupmapsth = $dbh->prepare("INSERT INTO category_group_map " . 
                                    "(category_id, group_id) " . 
                                    "VALUES (?, ?)");
                                     
    # Fields in the data file (matches the current collectstats.pl)
    my @statuses = 
                qw(NEW ASSIGNED REOPENED UNCONFIRMED RESOLVED VERIFIED CLOSED);
    my @resolutions = 
             qw(FIXED INVALID WONTFIX LATER REMIND DUPLICATE WORKSFORME MOVED);
    my @fields = (@statuses, @resolutions);

    # We have a localisation problem here. Where do we get these values?
    my $all_name = "-All-";
    my $open_name = "All Open";
        
    my $products = $dbh->selectall_arrayref("SELECT name FROM products");
     
    foreach my $product ((map { $_->[0] } @$products), "-All-") {
        # First, create the series
        my %queries;
        my %seriesids;
        
        my $query_prod = "";
        if ($product ne "-All-") {
            $query_prod = "product=" . html_quote($product) . "&";
        }
        
        # The query for statuses is different to that for resolutions.
        $queries{$_} = ($query_prod . "bug_status=$_") foreach (@statuses);
        $queries{$_} = ($query_prod . "resolution=$_") foreach (@resolutions);
        
        foreach my $field (@fields) {            
            # Create a Series for each field in this product.
            # user ID = 0 is used.
            my $series = new Bugzilla::Series(undef, $product, $all_name,
                                              $field, 0, 1,
                                              $queries{$field}, 1);
            $series->writeToDatabase();
            $seriesids{$field} = $series->{'series_id'};
        }
        
        # We also add a new query for "Open", so that migrated products get
        # the same set as new products (see editproducts.cgi.)
        my @openedstatuses = ("UNCONFIRMED", "NEW", "ASSIGNED", "REOPENED");
        my $query = join("&", map { "bug_status=$_" } @openedstatuses);
        my $series = new Bugzilla::Series(undef, $product, $all_name,
                                          $open_name, 0, 1, 
                                          $query_prod . $query, 1);
        $series->writeToDatabase();
        $seriesids{$open_name} = $series->{'series_id'};
        
        # Now, we attempt to read in historical data, if any
        # Convert the name in the same way that collectstats.pl does
        my $product_file = $product;
        $product_file =~ s/\//-/gs;
        $product_file = "$datadir/mining/$product_file";

        # There are many reasons that this might fail (e.g. no stats for this
        # product), so we don't worry if it does.        
        open(IN, $product_file) or next;

        # The data files should be in a standard format, even for old 
        # Bugzillas, because of the conversion code further up this file.
        my %data;
        my $last_date = "";
        
        while (<IN>) {
            if (/^(\d+\|.*)/) {
                my @numbers = split(/\||\r/, $1);
                
                # Only take the first line for each date; it was possible to
                # run collectstats.pl more than once in a day.
                next if $numbers[0] eq $last_date;
                
                for my $i (0 .. $#fields) {
                    # $numbers[0] is the date
                    $data{$fields[$i]}{$numbers[0]} = $numbers[$i + 1];
                    
                    # Keep a total of the number of open bugs for this day
                    if (is_open_state($fields[$i])) {
                        $data{$open_name}{$numbers[0]} += $numbers[$i + 1];
                    }
                }
                
                $last_date = $numbers[0];
            }
        }

        close(IN);

        foreach my $field (@fields, $open_name) {            
            # Insert values into series_data: series_id, date, value
            my %fielddata = %{$data{$field}};
            foreach my $date (keys %fielddata) {
                # We need to delete in case the text file had duplicate entries
                # in it.
                $deletesth->execute($seriesids{$field},
                                    $date);
                         
                # We prepared this above
                $seriesdatasth->execute($seriesids{$field},
                                        $date, 
                                        $fielddata{$date} || 0);
            }
        }
        
        # Create the groupsets for the category
        my $category_id = 
            $dbh->selectrow_array("SELECT id " . 
                                    "FROM series_categories " . 
                                   "WHERE name = " . $dbh->quote($product));
        my $product_id =
            $dbh->selectrow_array("SELECT id " .
                                    "FROM products " . 
                                   "WHERE name = " . $dbh->quote($product));
                                  
        if (defined($category_id) && defined($product_id)) {
          
            # Get all the mandatory groups for this product
            my $group_ids = 
                $dbh->selectcol_arrayref("SELECT group_id " . 
                     "FROM group_control_map " . 
                     "WHERE product_id = $product_id " . 
                     "AND (membercontrol = " . CONTROLMAPMANDATORY . 
                       " OR othercontrol = " . CONTROLMAPMANDATORY . ")");
                                            
            foreach my $group_id (@$group_ids) {
                $groupmapsth->execute($category_id, $group_id);
            }
        }
    }
}

AddFDef("owner_idle_time", "Time Since Assignee Touched", 0);

# 2004-04-12 - Keep regexp-based group permissions up-to-date - Bug 240325
if ($dbh->bz_column_info("user_group_map", "isderived")) {
    $dbh->bz_add_column('user_group_map', 'grant_type', 
        {TYPE => 'INT1', NOTNULL => 1, DEFAULT => '0'});
    $dbh->do("DELETE FROM user_group_map WHERE isderived != 0");
    $dbh->do("UPDATE user_group_map SET grant_type = " . GRANT_DIRECT);
    $dbh->bz_drop_column("user_group_map", "isderived");

    $dbh->bz_drop_index('user_group_map', 'user_group_map_user_id_idx');
    $dbh->bz_add_index('user_group_map', 'user_group_map_user_id_idx',
        {TYPE => 'UNIQUE', 
         FIELDS => [qw(user_id group_id grant_type isbless)]});

    # Make sure groups get rederived
    $dbh->do("UPDATE groups SET last_changed = NOW() WHERE name = 'admin'");
}

# 2004-07-16 - Make it possible to have group-group relationships other than
# membership and bless.
if ($dbh->bz_column_info("group_group_map", "isbless")) {
    $dbh->bz_add_column('group_group_map', 'grant_type', 
        {TYPE => 'INT1', NOTNULL => 1, DEFAULT => '0'});
    $dbh->do("UPDATE group_group_map SET grant_type = " .
                             "IF(isbless, " . GROUP_BLESS . ", " .
                             GROUP_MEMBERSHIP . ")");
    $dbh->bz_drop_index('group_group_map', 'group_group_map_member_id_idx');
    $dbh->bz_drop_column("group_group_map", "isbless");
    $dbh->bz_add_index('group_group_map', 'group_group_map_member_id_idx',
        {TYPE => 'UNIQUE', FIELDS => [qw(member_id grantor_id grant_type)]});
}    

# Allow profiles to optionally be linked to a unique identifier in an outside
# login data source
$dbh->bz_add_column("profiles", "extern_id", {TYPE => 'varchar(64)'});

# 2004-11-20 - LpSolit@netscape.net - Bug 180879
# Add grant and request groups for flags
$dbh->bz_add_column('flagtypes', 'grant_group_id', {TYPE => 'INT3'});
$dbh->bz_add_column('flagtypes', 'request_group_id', {TYPE => 'INT3'});

# 2004-01-03 - bug 253721 erik@dasbistro.com
# mailto is no longer just userids
$dbh->bz_rename_column('whine_schedules', 'mailto_userid', 'mailto');
$dbh->bz_add_column('whine_schedules', 'mailto_type', 
    {TYPE => 'INT2', NOTNULL => 1, DEFAULT => '0'});

# 2005-01-29 - mkanat@bugzilla.org
if (!$dbh->bz_column_info('longdescs', 'already_wrapped')) {
    # Old, pre-wrapped comments should not be auto-wrapped
    $dbh->bz_add_column('longdescs', 'already_wrapped',
        {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'FALSE'}, 1);
    # If an old comment doesn't have a newline in the first 80 characters,
    # (or doesn't contain a newline at all) and it contains a space,
    # then it's probably a mis-wrapped comment and we should wrap it
    # at display-time.
    print "Fixing old, mis-wrapped comments...\n";
    $dbh->do(q{UPDATE longdescs SET already_wrapped = 0
                WHERE (} . $dbh->sql_position(q{'\n'}, 'thetext') . q{ > 80
                   OR } . $dbh->sql_position(q{'\n'}, 'thetext') . q{ = 0)
                  AND SUBSTRING(thetext FROM 1 FOR 80) LIKE '% %'});
}

# 2001-09-03 (landed 2005-02-24)  dkl@redhat.com bug 17453
# Moved enum types to separate tables so we need change the old enum types to 
# standard varchars in the bugs table.
$dbh->bz_alter_column('bugs', 'bug_status', 
                      {TYPE => 'varchar(64)', NOTNULL => 1});
# 2005-03-23 Tomas.Kopal@altap.cz - add default value to resolution, bug 286695
$dbh->bz_alter_column('bugs', 'resolution',
                      {TYPE => 'varchar(64)', NOTNULL => 1, DEFAULT => "''"});
$dbh->bz_alter_column('bugs', 'priority',
                      {TYPE => 'varchar(64)', NOTNULL => 1});
$dbh->bz_alter_column('bugs', 'bug_severity',
                      {TYPE => 'varchar(64)', NOTNULL => 1});
$dbh->bz_alter_column('bugs', 'rep_platform',
                      {TYPE => 'varchar(64)', NOTNULL => 1}, '');
$dbh->bz_alter_column('bugs', 'op_sys',
                      {TYPE => 'varchar(64)', NOTNULL => 1});


# 2005-02-20 - LpSolit@gmail.com - Bug 277504
# When migrating quips from the '$datadir/comments' file to the DB,
# the user ID should be NULL instead of 0 (which is an invalid user ID).
if ($dbh->bz_column_info('quips', 'userid')->{NOTNULL}) {
    $dbh->bz_alter_column('quips', 'userid', {TYPE => 'INT3'});
    print "Changing owner to NULL for quips where the owner is unknown...\n";
    $dbh->do('UPDATE quips SET userid = NULL WHERE userid = 0');
}

# 2005-02-21 - LpSolit@gmail.com - Bug 279910
# qacontact_accessible and assignee_accessible field names no longer exist
# in the 'bugs' table. Their corresponding entries in the 'bugs_activity'
# table should therefore be marked as obsolete, meaning that they cannot
# be used anymore when querying the database - they are not deleted in
# order to keep track of these fields in the activity table.
if (!$dbh->bz_column_info('fielddefs', 'obsolete')) {
    $dbh->bz_add_column('fielddefs', 'obsolete', 
        {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'FALSE'});
    print "Marking qacontact_accessible and assignee_accessible as obsolete fields...\n";
    $dbh->do("UPDATE fielddefs SET obsolete = 1
              WHERE name = 'qacontact_accessible'
                 OR name = 'assignee_accessible'");
}

# Add fulltext indexes for bug summaries and descriptions/comments.
if (!$dbh->bz_index_info('bugs', 'bugs_short_desc_idx')) {
    print "Adding full-text index for short_desc column in bugs table...\n";
    $dbh->bz_add_index('bugs', 'bugs_short_desc_idx', 
                       {TYPE => 'FULLTEXT', FIELDS => [qw(short_desc)]});
}
# Right now, we only create the "thetext" index on MySQL.
if ($dbh->isa('Bugzilla::DB::Mysql') 
    && !$dbh->bz_index_info('longdescs', 'longdescs_thetext_idx')) 
{
    print "Adding full-text index for thetext column in longdescs table...\n";
    $dbh->bz_add_index('longdescs', 'longdescs_thetext_idx',
        {TYPE => 'FULLTEXT', FIELDS => [qw(thetext)]});
}

# 2002 November, myk@mozilla.org, bug 178841:
#
# Convert the "attachments.filename" column from a ridiculously large
# "mediumtext" to a much more sensible "varchar(100)".  Also takes
# the opportunity to remove paths from existing filenames, since they 
# shouldn't be there for security.  Buggy browsers include them, 
# and attachment.cgi now takes them out, but old ones need converting.
#
{
    my $ref = $dbh->bz_column_info("attachments", "filename");
    if ($ref->{TYPE} ne 'varchar(100)') {
        print "Removing paths from filenames in attachments table...\n";
        
        $sth = $dbh->prepare("SELECT attach_id, filename FROM attachments " . 
                "WHERE " . $dbh->sql_position(q{'/'}, 'filename') . " > 0 OR " .
                           $dbh->sql_position(q{'\\\\'}, 'filename') . " > 0");
        $sth->execute;
        
        while (my ($attach_id, $filename) = $sth->fetchrow_array) {
            $filename =~ s/^.*[\/\\]//;
            my $quoted_filename = $dbh->quote($filename);
            $dbh->do("UPDATE attachments SET filename = $quoted_filename " . 
                     "WHERE attach_id = $attach_id");
        }
        
        print "Done.\n";
        
        print "Resizing attachments.filename from mediumtext to varchar(100).\n";
        $dbh->bz_alter_column("attachments", "filename", 
                              {TYPE => 'varchar(100)', NOTNULL => 1});
    }
}

# 2003-01-11, burnus@net-b.de, bug 184309
# Support for quips approval
$dbh->bz_add_column('quips', 'approved', 
    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'TRUE'});
 
# 2002-12-20 Bug 180870 - remove manual shadowdb replication code
$dbh->bz_drop_table("shadowlog");

# 2003-04-27 - bugzilla@chimpychompy.org (GavinS)
#
# Bug 180086 (http://bugzilla.mozilla.org/show_bug.cgi?id=180086)
#
# Renaming the 'count' column in the votes table because Sybase doesn't
# like it
if ($dbh->bz_column_info('votes', 'count')) {
    # 2003-04-24 - myk@mozilla.org/bbaetz@acm.org, bug 201018
    # Force all cached groups to be updated at login, due to security bug
    # Do this here, inside the next schema change block, so that it doesn't
    # get invalidated on every checksetup run.
    $dbh->do("UPDATE profiles SET refreshed_when='1900-01-01 00:00:00'");

    $dbh->bz_rename_column('votes', 'count', 'vote_count');
}

# 2004/02/15 - Summaries shouldn't be null - see bug 220232
if (!exists $dbh->bz_column_info('bugs', 'short_desc')->{NOTNULL}) {
    $dbh->bz_alter_column('bugs', 'short_desc',
                          {TYPE => 'MEDIUMTEXT', NOTNULL => 1}, '');
}

# 2003-10-24 - alt@sonic.net, bug 224208
# Support classification level
$dbh->bz_add_column('products', 'classification_id',
                    {TYPE => 'INT2', NOTNULL => 1, DEFAULT => '1'});

# 2005-01-12 Nick Barnes <nb@ravenbrook.com> bug 278010
# Rename any group which has an empty name.
# Note that there can be at most one such group (because of
# the SQL index on the name column).
$sth = $dbh->prepare("SELECT id FROM groups where name = ''");
$sth->execute();
my ($emptygroupid) = $sth->fetchrow_array();
if ($emptygroupid) {
    # There is a group with an empty name; find a name to rename it
    # as.  Must avoid collisions with existing names.  Start with
    # group_$gid and add _<n> if necessary.
    my $trycount = 0;
    my $trygroupname;
    my $trygroupsth = $dbh->prepare("SELECT id FROM groups where name = ?");
    do {
        $trygroupname = "group_$emptygroupid";
        if ($trycount > 0) {
            $trygroupname .= "_$trycount";
        }
        $trygroupsth->execute($trygroupname);
        if ($trygroupsth->rows > 0) {
            $trycount ++;
        }
    } while ($trygroupsth->rows > 0);
    $sth = $dbh->prepare("UPDATE groups SET name = ? " .
                         "WHERE id = $emptygroupid");
    $sth->execute($trygroupname);
    print "Group $emptygroupid had an empty name; renamed as '$trygroupname'.\n";
}

# 2005-02-12 bugreport@peshkin.net, bug 281787
$dbh->bz_add_index('bugs_activity', 'bugs_activity_who_idx', [qw(who)]);

# This lastdiffed change and these default changes are unrelated,
# but in order for MySQL to successfully run these default changes only once,
# they have to be inside this block.
# If bugs.lastdiffed is NOT NULL...
if($dbh->bz_column_info('bugs', 'lastdiffed')->{NOTNULL}) {
    # Add defaults for some fields that should have them but didn't.
    $dbh->bz_alter_column('bugs', 'status_whiteboard',
        {TYPE => 'MEDIUMTEXT', NOTNULL => 1, DEFAULT => "''"});
    $dbh->bz_alter_column('bugs', 'keywords',
        {TYPE => 'MEDIUMTEXT', NOTNULL => 1, DEFAULT => "''"});
    $dbh->bz_alter_column('bugs', 'votes',
                          {TYPE => 'INT3', NOTNULL => 1, DEFAULT => '0'});
    # And change lastdiffed to NULL
    $dbh->bz_alter_column('bugs', 'lastdiffed', {TYPE => 'DATETIME'});
}

# 2005-03-09 qa_contact should be NULL instead of 0, bug 285534
if ($dbh->bz_column_info('bugs', 'qa_contact')->{NOTNULL}) {
    $dbh->bz_alter_column('bugs', 'qa_contact', {TYPE => 'INT3'});
    $dbh->do("UPDATE bugs SET qa_contact = NULL WHERE qa_contact = 0");
}

# 2005-03-27 initialqacontact should be NULL instead of 0, bug 287483
if ($dbh->bz_column_info('components', 'initialqacontact')->{NOTNULL}) {
    $dbh->bz_alter_column('components', 'initialqacontact', {TYPE => 'INT3'});
    $dbh->do("UPDATE components SET initialqacontact = NULL " .
             "WHERE initialqacontact = 0");
}

# 2005-03-29 - gerv@gerv.net - bug 73665.
# Migrate email preferences to new email prefs table.
if ($dbh->bz_column_info("profiles", "emailflags")) {
    print "Migrating email preferences to new table ...\n" unless $silent;
    
    # These are the "roles" and "reasons" from the original code, mapped to
    # the new terminology of relationships and events.
    my %relationships = ("Owner" => REL_ASSIGNEE, 
                         "Reporter" => REL_REPORTER,
                         "QAcontact" => REL_QA,
                         "CClist" => REL_CC,
                         "Voter" => REL_VOTER);
                         
    my %events = ("Removeme" => EVT_ADDED_REMOVED, 
                  "Comments" => EVT_COMMENT, 
                  "Attachments" => EVT_ATTACHMENT, 
                  "Status" => EVT_PROJ_MANAGEMENT, 
                  "Resolved" => EVT_OPENED_CLOSED,
                  "Keywords" => EVT_KEYWORD, 
                  "CC" => EVT_CC, 
                  "Other" => EVT_OTHER,
                  "Unconfirmed" => EVT_UNCONFIRMED);
                                    
    # Request preferences
    my %requestprefs = ("FlagRequestee" => EVT_FLAG_REQUESTED,
                        "FlagRequester" => EVT_REQUESTED_FLAG);

    # Select all emailflags flag strings
    my $sth = $dbh->prepare("SELECT userid, emailflags FROM profiles");
    $sth->execute();

    while (my ($userid, $flagstring) = $sth->fetchrow_array()) {
        # If the user has never logged in since emailprefs arrived, and the
        # temporary code to give them a default string never ran, then 
        # $flagstring will be null. In this case, they just get all mail.
        $flagstring ||= "";
        
        # The 255 param is here, because without a third param, split will
        # trim any trailing null fields, which causes Perl to eject lots of
        # warnings. Any suitably large number would do.
        my %emailflags = split(/~/, $flagstring, 255); # Appease my editor: /
     
        my $sth2 = $dbh->prepare("INSERT into email_setting " .
                                 "(user_id, relationship, event) VALUES (" . 
                                 "$userid, ?, ?)");
        foreach my $relationship (keys %relationships) {
            foreach my $event (keys %events) {
                my $key = "email$relationship$event";
                if (!exists($emailflags{$key}) || $emailflags{$key} eq 'on') {
                    $sth2->execute($relationships{$relationship},
                                   $events{$event});
                }
            }
        }

        # Note that in the old system, the value of "excludeself" is assumed to
        # be off if the preference does not exist in the user's list, unlike 
        # other preferences whose value is assumed to be on if they do not 
        # exist.
        #
        # This preference has changed from global to per-relationship.
        if (!exists($emailflags{'ExcludeSelf'}) 
            || $emailflags{'ExcludeSelf'} ne 'on')
        {
            foreach my $relationship (keys %relationships) {
                $dbh->do("INSERT into email_setting " .
                         "(user_id, relationship, event) VALUES (" . 
                         $userid . ", " .
                         $relationships{$relationship}. ", " .
                         EVT_CHANGED_BY_ME . ")");
            }
        }

        foreach my $key (keys %requestprefs) {
            if (!exists($emailflags{$key}) || $emailflags{$key} eq 'on') {
              $dbh->do("INSERT into email_setting " . 
                       "(user_id, relationship, event) VALUES (" . 
                       $userid . ", " . REL_ANY . ", " . 
                       $requestprefs{$key} . ")");            
            }
        }
    }
   
    # EVT_ATTACHMENT_DATA should initially have identical settings to 
    # EVT_ATTACHMENT. 
    CloneEmailEvent(EVT_ATTACHMENT, EVT_ATTACHMENT_DATA); 
       
    $dbh->bz_drop_column("profiles", "emailflags");    
}

# Check for any "new" email settings that wouldn't have been ported over
# during the block above.  Since these settings would have otherwise
# fallen under EVT_OTHER, we'll just clone those settings.  That way if
# folks have already disabled all of that mail, there won't be any change.
{
    my %events = ("Dependency Tree Changes" => EVT_DEPEND_BLOCK); 

    foreach my $desc (keys %events) {
        my $event = $events{$desc};
        $sth = $dbh->prepare("SELECT count(*) FROM email_setting WHERE event = $event");
        $sth->execute();
        if (!($sth->fetchrow_arrayref()->[0])) {
            # No settings in the table yet, so we assume that this is the
            # first time it's being set.
            print "Initializing \"$desc\" email_setting ...\n" unless $silent;
            CloneEmailEvent(EVT_OTHER, $event);
        }
    }
}

sub CloneEmailEvent {
    my ($source, $target) = @_;

    my $sth1 = $dbh->prepare("SELECT user_id, relationship FROM email_setting
                              WHERE event = $source");
    my $sth2 = $dbh->prepare("INSERT into email_setting " . 
                             "(user_id, relationship, event) VALUES (" . 
                             "?, ?, $target)");
    
    $sth1->execute();

    while (my ($userid, $relationship) = $sth1->fetchrow_array()) {
        $sth2->execute($userid, $relationship);            
    }    
} 

# 2005-03-27: Standardize all boolean fields to plain "tinyint"
if ( $dbh->isa('Bugzilla::DB::Mysql') ) {
    # This is a change to make things consistent with Schema, so we use
    # direct-database access methods.
    my $quip_info_sth = $dbh->column_info(undef, undef, 'quips', '%');
    my $quips_cols    = $quip_info_sth->fetchall_hashref("COLUMN_NAME");
    my $approved_col  = $quips_cols->{'approved'};
    if ( $approved_col->{TYPE_NAME} eq 'TINYINT'
         and $approved_col->{COLUMN_SIZE} == 1 )
    {
        # series.public could have been renamed to series.is_public,
        # and so wouldn't need to be fixed manually.
        if ($dbh->bz_column_info('series', 'public')) {
            $dbh->bz_alter_column_raw('series', 'public',
                {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => '0'});
        }
        $dbh->bz_alter_column_raw('bug_status', 'isactive',
            {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => '1'});
        $dbh->bz_alter_column_raw('rep_platform', 'isactive',
            {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => '1'});
        $dbh->bz_alter_column_raw('resolution', 'isactive',
            {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => '1'});
        $dbh->bz_alter_column_raw('op_sys', 'isactive',
            {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => '1'});
        $dbh->bz_alter_column_raw('bug_severity', 'isactive',
            {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => '1'});
        $dbh->bz_alter_column_raw('priority', 'isactive',
            {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => '1'});
        $dbh->bz_alter_column_raw('quips', 'approved',
            {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => '1'});
    }
}

# 2005-04-07 - alt@sonic.net, bug 289455
# make classification_id field type be consistent with DB:Schema
$dbh->bz_alter_column('products', 'classification_id',
                      {TYPE => 'INT2', NOTNULL => 1, DEFAULT => '1'});

# initialowner was accidentally NULL when we checked-in Schema,
# when it really should be NOT NULL.
$dbh->bz_alter_column('components', 'initialowner',
                      {TYPE => 'INT3', NOTNULL => 1}, 0);

# 2005-03-28 - bug 238800 - index flags.type_id to make editflagtypes.cgi speedy
$dbh->bz_add_index('flags', 'flags_type_id_idx', [qw(type_id)]);

# For a short time, the flags_type_id_idx was misnamed in upgraded installs.
$dbh->bz_drop_index('flags', 'type_id');

# 2005-04-28 - LpSolit@gmail.com - Bug 7233: add an index to versions
$dbh->bz_alter_column('versions', 'value',
                      {TYPE => 'varchar(64)', NOTNULL => 1});
$dbh->bz_add_index('versions', 'versions_product_id_idx',
                   {TYPE => 'UNIQUE', FIELDS => [qw(product_id value)]});

# Milestone sortkeys get a default just like all other sortkeys.
if (!exists $dbh->bz_column_info('milestones', 'sortkey')->{DEFAULT}) {
    $dbh->bz_alter_column('milestones', 'sortkey', 
                          {TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0});
}

# 2005-06-14 - LpSolit@gmail.com - Bug 292544: only set creation_ts
# when all bug fields have been correctly set.
$dbh->bz_alter_column('bugs', 'creation_ts', {TYPE => 'DATETIME'});

if (!exists $dbh->bz_column_info('whine_queries', 'title')->{DEFAULT}) {

    # The below change actually has nothing to do with the whine_queries
    # change, it just has to be contained within a schema change so that
    # it doesn't run every time we run checksetup.

    # Old Bugzillas have "other" as an OS choice, new ones have "Other"
    # (capital O).
    print "Setting any 'other' op_sys to 'Other'...\n";
    $dbh->do('UPDATE op_sys SET value = ? WHERE value = ?',
             undef, "Other", "other");
    $dbh->do('UPDATE bugs SET op_sys = ? WHERE op_sys = ?',
             undef, "Other", "other");
    if (Bugzilla->params->{'defaultopsys'} eq 'other') {
        # We can't actually fix the param here, because WriteParams() will
        # make $datadir/params unwriteable to the webservergroup.
        # It's too much of an ugly hack to copy the permission-fixing code
        # down to here. (It would create more potential future bugs than
        # it would solve problems.)
        print "WARNING: Your 'defaultopsys' param is set to 'other', but"
            . " Bugzilla now\n"
            . "         uses 'Other' (capital O).\n";
    }

    # Add a DEFAULT to whine_queries stuff so that editwhines.cgi
    # works on PostgreSQL.
    $dbh->bz_alter_column('whine_queries', 'title', {TYPE => 'varchar(128)', 
                          NOTNULL => 1, DEFAULT => "''"});
}

# 2005-06-29 bugreport@peshkin.net, bug 299156
if ($dbh->bz_index_info('attachments', 'attachments_submitter_id_idx')
   && (scalar(@{$dbh->bz_index_info('attachments',
                                    'attachments_submitter_id_idx'
                                   )->{FIELDS}}) < 2)
      ) {
    $dbh->bz_drop_index('attachments', 'attachments_submitter_id_idx');
}
$dbh->bz_add_index('attachments', 'attachments_submitter_id_idx',
                   [qw(submitter_id bug_id)]);

# 2005-08-25 - bugreport@peshkin.net - Bug 305333
if ($dbh->bz_column_info("attachments", "thedata")) {
    print "Migrating attachment data to its own table...\n";
    print "(This may take a very long time)\n";
    $dbh->do("INSERT INTO attach_data (id, thedata) 
                   SELECT attach_id, thedata FROM attachments");
    $dbh->bz_drop_column("attachments", "thedata");    
}

# 2005-11-26 - wurblzap@gmail.com - Bug 300473
# Repair broken automatically generated series queries for non-open bugs.
my $broken_series_indicator =
    'field0-0-0=resolution&type0-0-0=notequals&value0-0-0=---';
my $broken_nonopen_series =
    $dbh->selectall_arrayref("SELECT series_id, query FROM series
                               WHERE query LIKE '$broken_series_indicator%'");
if (@$broken_nonopen_series) {
    print 'Repairing broken series...';
    my $sth_nuke =
        $dbh->prepare('DELETE FROM series_data WHERE series_id = ?');
    # This statement is used to repair a series by replacing the broken query
    # with the correct one.
    my $sth_repair =
        $dbh->prepare('UPDATE series SET query = ? WHERE series_id = ?');
    # The corresponding series for open bugs look like one of these two
    # variations (bug 225687 changed the order of bug states).
    # This depends on the set of bug states representing open bugs not to have
    # changed since series creation.
    my $open_bugs_query_base_old = 
        join("&", map { "bug_status=" . url_quote($_) }
                      ('UNCONFIRMED', 'NEW', 'ASSIGNED', 'REOPENED'));
    my $open_bugs_query_base_new = 
        join("&", map { "bug_status=" . url_quote($_) } BUG_STATE_OPEN);
    my $sth_openbugs_series =
        $dbh->prepare("SELECT series_id FROM series
                        WHERE query IN (?, ?)");
    # Statement to find the series which has collected the most data.
    my $sth_data_collected =
        $dbh->prepare('SELECT count(*) FROM series_data WHERE series_id = ?');
    # Statement to select a broken non-open bugs count data entry.
    my $sth_select_broken_nonopen_data =
        $dbh->prepare('SELECT series_date, series_value FROM series_data' .
                      ' WHERE series_id = ?');
    # Statement to select an open bugs count data entry.
    my $sth_select_open_data =
        $dbh->prepare('SELECT series_value FROM series_data' .
                      ' WHERE series_id = ? AND series_date = ?');
    # Statement to fix a broken non-open bugs count data entry.
    my $sth_fix_broken_nonopen_data =
        $dbh->prepare('UPDATE series_data SET series_value = ?' .
                      ' WHERE series_id = ? AND series_date = ?');
    # Statement to delete an unfixable broken non-open bugs count data entry.
    my $sth_delete_broken_nonopen_data =
        $dbh->prepare('DELETE FROM series_data' .
                      ' WHERE series_id = ? AND series_date = ?');

    foreach (@$broken_nonopen_series) {
        my ($broken_series_id, $nonopen_bugs_query) = @$_;

        # Determine the product-and-component part of the query.
        if ($nonopen_bugs_query =~ /^$broken_series_indicator(.*)$/) {
            my $prodcomp = $1;

            # If there is more than one series for the corresponding open-bugs
            # series, we pick the one with the most data, which should be the
            # one which was generated on creation.
            # It's a pity we can't do subselects.
            $sth_openbugs_series->execute($open_bugs_query_base_old . $prodcomp,
                                          $open_bugs_query_base_new . $prodcomp);
            my ($found_open_series_id, $datacount) = (undef, -1);
            foreach my $open_series_id ($sth_openbugs_series->fetchrow_array()) {
                $sth_data_collected->execute($open_series_id);
                my ($this_datacount) = $sth_data_collected->fetchrow_array();
                if ($this_datacount > $datacount) {
                    $datacount = $this_datacount;
                    $found_open_series_id = $open_series_id;
                }
            }

            if ($found_open_series_id) {
                # Move along corrupted series data and correct it. The
                # corruption consists of it being the number of all bugs
                # instead of the number of non-open bugs, so we calculate the
                # correct count by subtracting the number of open bugs.
                # If there is no corresponding open-bugs count for some reason
                # (shouldn't happen), we drop the data entry.
                print " $broken_series_id...";
                $sth_select_broken_nonopen_data->execute($broken_series_id);
                while (my $rowref =
                       $sth_select_broken_nonopen_data->fetchrow_arrayref()) {
                    my ($date, $broken_value) = @$rowref;
                    my ($openbugs_value) =
                        $dbh->selectrow_array($sth_select_open_data, undef,
                                              $found_open_series_id, $date);
                    if (defined($openbugs_value)) {
                        $sth_fix_broken_nonopen_data->execute
                            ($broken_value - $openbugs_value,
                             $broken_series_id, $date);
                    }
                    else {
                        print "\nWARNING - During repairs of series " .
                              "$broken_series_id, the irreparable data\n" .
                              "entry for date $date was encountered and is " .
                              "being deleted.\n" .
                              "Continuing repairs...";
                        $sth_delete_broken_nonopen_data->execute
                            ($broken_series_id, $date);
                    }
                }

                # Fix the broken query so that it collects correct data in the
                # future.
                $nonopen_bugs_query =~
                    s/^$broken_series_indicator/field0-0-0=resolution&type0-0-0=regexp&value0-0-0=./;
                $sth_repair->execute($nonopen_bugs_query, $broken_series_id);
            }
            else {
                print "\nWARNING - Series $broken_series_id was meant to\n" .
                      "collect non-open bug counts, but it has counted\n" .
                      "all bugs instead. It cannot be repaired\n" .
                      "automatically because no series that collected open\n" .
                      "bug counts was found. You'll probably want to delete\n" .
                      "or repair collected data for series $broken_series_id " .
                      "manually.\n" .
                      "Continuing repairs...";
            }
        }
    }
    print " done.\n";
}

# 2005-09-15 lance.larsh@oracle.com Bug 308717
if ($dbh->bz_column_info("series", "public")) {
    # PUBLIC is a reserved word in Oracle, so renaming the column
    # PUBLIC in table SERIES avoids having to quote the column name
    # in every query against that table
    $dbh->bz_rename_column('series', 'public', 'is_public');
}

# 2005-09-28 bugreport@peshkin.net Bug 149504
$dbh->bz_add_column('attachments', 'isurl',
                    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 0});

# 2005-10-21 LpSolit@gmail.com - Bug 313020
$dbh->bz_add_column('namedqueries', 'query_type',
                    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 0});

# 2005-11-04 LpSolit@gmail.com - Bug 305927
$dbh->bz_alter_column('groups', 'userregexp', 
                      {TYPE => 'TINYTEXT', NOTNULL => 1, DEFAULT => "''"});

# 2005-09-26 - olav@bkor.dhs.org - Bug 119524
# Convert logincookies into a varchar
# this allows to store a random token instead of a guessable auto_increment
$dbh->bz_alter_column('logincookies', 'cookie',
                      {TYPE => 'varchar(16)', PRIMARYKEY => 1, NOTNULL => 1});

# Fixup for Bug 101380
# "Newlines, nulls, leading/trailing spaces are getting into summaries"

my $controlchar_bugs =
    $dbh->selectall_arrayref("SELECT short_desc, bug_id FROM bugs WHERE " .
                             $dbh->sql_regexp('short_desc', "'[[:cntrl:]]'"));
if (scalar(@$controlchar_bugs))
{
    my $msg = 'Cleaning control characters from bug summaries...';
    my $found = 0;
    foreach (@$controlchar_bugs) {
        my ($short_desc, $bug_id) = @$_;
        my $clean_short_desc = clean_text($short_desc);
        if ($clean_short_desc ne $short_desc) {
            print $msg if !$found;
            $found = 1;
            print " $bug_id...";
            $dbh->do("UPDATE bugs SET short_desc = ? WHERE bug_id = ?",
                      undef, $clean_short_desc, $bug_id);
        }
    }
    print " done.\n" if $found;
}

# 2005-08-10 Myk Melez <myk@mozilla.org> bug 287325
# Record each field's type and whether or not it's a custom field in fielddefs.
$dbh->bz_add_column('fielddefs', 'type',
                    { TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0 });
$dbh->bz_add_column('fielddefs', 'custom',
                    { TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'FALSE' });

# 2005-12-07 altlst@sonic.net -- Bug 225221
$dbh->bz_add_column('longdescs', 'comment_id',
                    {TYPE => 'MEDIUMSERIAL', NOTNULL => 1, PRIMARYKEY => 1});

# 2006-03-02 LpSolit@gmail.com - Bug 322285
# Do not store inactive flags in the DB anymore.
if ($dbh->bz_column_info('flags', 'id')->{'TYPE'} eq 'INT3') {
    # We first have to remove all existing inactive flags.
    if ($dbh->bz_column_info('flags', 'is_active')) {
        $dbh->do('DELETE FROM flags WHERE is_active = 0');
    }

    # Now we convert the id column to the auto_increment format.
    $dbh->bz_alter_column('flags', 'id',
                          {TYPE => 'MEDIUMSERIAL', NOTNULL => 1, PRIMARYKEY => 1});

    # And finally, we remove the is_active column.
    $dbh->bz_drop_column('flags', 'is_active');
}

# short_desc should not be a mediumtext, fix anything longer than 255 chars.
if($dbh->bz_column_info('bugs', 'short_desc')->{TYPE} eq 'MEDIUMTEXT') {
    # Move extremely long summaries into a comment ("from" the Reporter),
    # and then truncate the summary.
    my $long_summary_bugs = $dbh->selectall_arrayref(
        'SELECT bug_id, short_desc, reporter
           FROM bugs WHERE LENGTH(short_desc) > 255');

    if (@$long_summary_bugs) {
        print <<EOF;

WARNING: Some of your bugs had summaries longer than 255 characters.
They have had their original summary copied into a comment, and then
the summary was truncated to 255 characters. The affected bug numbers were:
EOF
        my $comment_sth = $dbh->prepare(
            'INSERT INTO longdescs (bug_id, who, thetext, bug_when) 
                  VALUES (?, ?, ?, NOW())');
        my $desc_sth = $dbh->prepare('UPDATE bugs SET short_desc = ? 
                                       WHERE bug_id = ?');
        my @affected_bugs;
        foreach my $bug (@$long_summary_bugs) {
            my ($bug_id, $summary, $reporter_id) = @$bug;
            my $summary_comment = "The original summary for this bug"
               . " was longer than 255 characters, and so it was truncated"
               . " when Bugzilla was upgraded. The original summary was:"
               . "\n\n$summary";
            $comment_sth->execute($bug_id, $reporter_id, $summary_comment);
            my $short_summary = substr($summary, 0, 252) . "...";
            $desc_sth->execute($short_summary, $bug_id);
            push(@affected_bugs, $bug_id);
        }
        print join(', ', @affected_bugs) . "\n\n";
    }

    $dbh->bz_alter_column('bugs', 'short_desc', {TYPE => 'varchar(255)', 
                                                 NOTNULL => 1});
}

if (-e "$datadir/versioncache") {
    print "Removing versioncache...\n";
    unlink "$datadir/versioncache";
}

# 2006-07-01 wurblzap@gmail.com -- Bug 69000
$dbh->bz_add_column('namedqueries', 'id',
                    {TYPE => 'MEDIUMSERIAL', NOTNULL => 1, PRIMARYKEY => 1});
if ($dbh->bz_column_info("namedqueries", "linkinfooter")) {
    # Move link-in-footer information into a table of its own.
    my $sth_read = $dbh->prepare('SELECT id, userid
                                    FROM namedqueries 
                                   WHERE linkinfooter = 1');
    my $sth_write = $dbh->prepare('INSERT INTO namedqueries_link_in_footer
                                   (namedquery_id, user_id) VALUES (?, ?)');
    $sth_read->execute();
    while (my ($id, $userid) = $sth_read->fetchrow_array()) {
        $sth_write->execute($id, $userid);
    }
    $dbh->bz_drop_column("namedqueries", "linkinfooter");    
}

# 2006-07-07 olav@bkor.dhs.org - Bug 277377
# Add a sortkey to the classifications
if (!$dbh->bz_column_info('classifications', 'sortkey')) {
    $dbh->bz_add_column('classifications', 'sortkey',
                        {TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0});

    my $class_ids = $dbh->selectcol_arrayref('SELECT id FROM classifications ' .
                                             'ORDER BY name');
    my $sth = $dbh->prepare('UPDATE classifications SET sortkey = ? ' .
                            'WHERE id = ?');
    my $sortkey = 0;
    foreach my $class_id (@$class_ids) {
        $sth->execute($sortkey, $class_id);
        $sortkey += 100;
    }
}

$dbh->bz_add_column('fielddefs', 'enter_bug',
    {TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'FALSE'});

# 2006-07-14 karl@kornel.name - Bug 100953
# If a nomail file exists, move its contents into the DB
$dbh->bz_add_column('profiles', 'disable_mail',
                    { TYPE => 'BOOLEAN', NOTNULL => 1, DEFAULT => 'FALSE' });
if (-e "$datadir/nomail") {
    # We have a data/nomail file, read it in and delete it
    my %nomail;
    print "Found a data/nomail file.  Moving nomail entries into DB...\n";
    open NOMAIL, '<', "$datadir/nomail";
    while (<NOMAIL>) {
        $nomail{trim($_)} = 1;
    }
    close NOMAIL;

    # Go through each entry read.  If a user exists, set disable_mail.
    my $query = $dbh->prepare('UPDATE profiles
                                  SET disable_mail = 1
                                WHERE userid = ?');
    foreach my $user_to_check (keys %nomail) {
        my $uid;
        if ($uid = Bugzilla::User::login_to_id($user_to_check)) {
            my $user = new Bugzilla::User($uid);
            print "\tDisabling email for user ", $user->email, "\n";
            $query->execute($user->id);
            delete $nomail{$user->email};
        }
    }

    # If there are any nomail entries remaining, move them to nomail.bad
    # and say something to the user.
    if (scalar(keys %nomail)) {
        print 'The following users were listed in data/nomail, but do not ' .
              'have an account here.  The unmatched entries have been moved ' .
              "to $datadir/nomail.bad\n";
        open NOMAIL_BAD, '>>', "$datadir/nomail.bad";
        foreach my $unknown_user (keys %nomail) {
            print "\t$unknown_user\n";
            print NOMAIL_BAD "$unknown_user\n";
            delete $nomail{$unknown_user};
        }
        close NOMAIL_BAD;
    }

    # Now that we don't need it, get rid of the nomail file.
    unlink "$datadir/nomail";
}


# If you had to change the --TABLE-- definition in any way, then add your
# differential change code *** A B O V E *** this comment.
#
# That is: if you add a new field, you first search for the first occurrence
# of --TABLE-- and add your field to into the table hash. This new setting
# would be honored for every new installation. Then add your
# bz_add_field/bz_drop_field/bz_change_field_type/bz_rename_field code above.
# This would then be honored by everyone who updates his Bugzilla installation.
#

#
# Bugzilla uses --GROUPS-- to assign various rights to its users.
#

AddGroup('tweakparams', 'Can tweak operating parameters');
AddGroup('editusers', 'Can edit or disable users');
AddGroup('creategroups', 'Can create and destroy groups.');
AddGroup('editclassifications', 'Can create, destroy, and edit classifications.');
AddGroup('editcomponents', 'Can create, destroy, and edit components.');
AddGroup('editkeywords', 'Can create, destroy, and edit keywords.');
AddGroup('admin', 'Administrators');

if (!GroupDoesExist("editbugs")) {
    my $id = AddGroup('editbugs', 'Can edit all bug fields.', ".*");
    my $sth = $dbh->prepare("SELECT userid FROM profiles");
    $sth->execute();
    while (my ($userid) = $sth->fetchrow_array()) {
        $dbh->do("INSERT INTO user_group_map 
            (user_id, group_id, isbless, grant_type) 
            VALUES ($userid, $id, 0, " . GRANT_DIRECT . ")");
    }
}

if (!GroupDoesExist("canconfirm")) {
    my $id = AddGroup('canconfirm',  'Can confirm a bug.', ".*");
    my $sth = $dbh->prepare("SELECT userid FROM profiles");
    $sth->execute();
    while (my ($userid) = $sth->fetchrow_array()) {
        $dbh->do("INSERT INTO user_group_map 
            (user_id, group_id, isbless, grant_type) 
            VALUES ($userid, $id, 0, " . GRANT_DIRECT . ")");
    }

}

# Create bz_canusewhineatothers and bz_canusewhines
if (!GroupDoesExist('bz_canusewhines')) {
    my $whine_group = AddGroup('bz_canusewhines',
                               'User can configure whine reports for self');
    my $whineatothers_group = AddGroup('bz_canusewhineatothers',
                                       'Can configure whine reports for ' .
                                       'other users');
    my $group_exists = $dbh->selectrow_array(
        q{SELECT 1 FROM group_group_map 
           WHERE member_id = ? AND grantor_id = ? AND grant_type = ?},
        undef, $whineatothers_group, $whine_group, GROUP_MEMBERSHIP);
    $dbh->do("INSERT INTO group_group_map " .
             "(member_id, grantor_id, grant_type) " .
             "VALUES (${whineatothers_group}, ${whine_group}, " .
             GROUP_MEMBERSHIP . ")") unless $group_exists;
}

# 2005-08-14 bugreport@peshkin.net -- Bug 304583
use constant GRANT_DERIVED => 1;
# Get rid of leftover DERIVED group permissions
$dbh->do("DELETE FROM user_group_map WHERE grant_type = " . GRANT_DERIVED);
# Evaluate regexp-based group memberships
$sth = $dbh->prepare("SELECT profiles.userid, profiles.login_name,
                         groups.id, groups.userregexp,
                         user_group_map.group_id
                         FROM (profiles
                         CROSS JOIN groups)
                         LEFT JOIN user_group_map
                         ON user_group_map.user_id = profiles.userid
                         AND user_group_map.group_id = groups.id
                         AND user_group_map.grant_type = ?
                         WHERE (userregexp != ''
                         OR user_group_map.group_id IS NOT NULL)");

my $sth_add = $dbh->prepare("INSERT INTO user_group_map 
                       (user_id, group_id, isbless, grant_type) 
                       VALUES(?, ?, 0, " . GRANT_REGEXP . ")");

my $sth_del = $dbh->prepare("DELETE FROM user_group_map 
                       WHERE user_id  = ? 
                       AND group_id = ? 
                       AND isbless = 0
                       AND grant_type = " . GRANT_REGEXP);

$sth->execute(GRANT_REGEXP);
while (my ($uid, $login, $gid, $rexp, $present) = $sth->fetchrow_array()) {
    if ($login =~ m/$rexp/i) {
        $sth_add->execute($uid, $gid) unless $present;
    } else {
        $sth_del->execute($uid, $gid) if $present;
    }
}

# 2005-10-10 karl@kornel.name -- Bug 204498
if (!GroupDoesExist('bz_sudoers')) {
    my $sudoers_group = AddGroup('bz_sudoers',
                                 'Can perform actions as other users');
    my $sudo_protect_group = AddGroup('bz_sudo_protect',
                                      'Can not be impersonated by other users');
    my ($admin_group) = $dbh->selectrow_array('SELECT id FROM groups
                                               WHERE name = ?', undef, 'admin');
    
    # Admins should be given sudo access
    # Everyone in sudo should be in sudo_protect
    # Admins can grant membership in both groups
    my $sth = $dbh->prepare('INSERT INTO group_group_map 
                             (member_id, grantor_id, grant_type) 
                             VALUES (?, ?, ?)');
    $sth->execute($admin_group, $sudoers_group, GROUP_MEMBERSHIP);
    $sth->execute($sudoers_group, $sudo_protect_group, GROUP_MEMBERSHIP);
    $sth->execute($admin_group, $sudoers_group, GROUP_BLESS);
    $sth->execute($admin_group, $sudo_protect_group, GROUP_BLESS);
}

###########################################################################
# Create --SETTINGS-- users can adjust
###########################################################################

# 2005-03-03 travis@sedsystems.ca -- Bug 41972
add_setting ("display_quips", {"on" => 1, "off" => 2 }, "on" );

# 2005-03-10 travis@sedsystems.ca -- Bug 199048
add_setting ("comment_sort_order", {"oldest_to_newest" => 1,
                                    "newest_to_oldest" => 2,
                                    "newest_to_oldest_desc_first" => 3}, 
             "oldest_to_newest" );

# 2005-05-12 bugzilla@glob.com.au -- Bug 63536
add_setting ("post_bug_submit_action", {"next_bug" => 1,
                                        "same_bug" => 2,
                                        "nothing" => 3,
                                       },
             "next_bug" );

# 2005-06-29 wurblzap@gmail.com -- Bug 257767
add_setting ('csv_colsepchar', {',' => 1, ';' => 2 }, ',' );

# 2005-10-26 wurblzap@gmail.com -- Bug 291459
add_setting ("zoom_textareas", {"on" => 1, "off" => 2 }, "on" );

# 2005-10-21 LpSolit@gmail.com -- Bug 313020
add_setting('per_bug_queries', {'on' => 1, 'off' => 2}, 'on');

# 2006-05-01 olav@bkor.dhs.org -- Bug 7710
add_setting('state_addselfcc', {'always' => 1,
                                'never' => 2,
                                'cc_unless_role' => '3'},
            'cc_unless_role');

###########################################################################
# Create Administrator  --ADMIN--
###########################################################################

sub bailout {   # this is just in case we get interrupted while getting passwd
    if ($^O !~ /MSWin32/i) {
        system("stty","echo"); # re-enable input echoing
    }
    exit 1;
}

if (@admins) {
    # Identify admin group.
    my $sth = $dbh->prepare("SELECT id FROM groups 
                WHERE name = 'admin'");
    $sth->execute();
    my ($adminid) = $sth->fetchrow_array();
    foreach my $userid (@admins) {
        $dbh->do("INSERT INTO user_group_map 
            (user_id, group_id, isbless, grant_type) 
            VALUES ($userid, $adminid, 0, " . GRANT_DIRECT . ")");
        # Existing administrators are made blessers of group "admin"
        # but only explicitly defined blessers can bless group admin.
        # Other groups can be blessed by any admin (by default) or additional
        # defined blessers.
        $dbh->do("INSERT INTO user_group_map 
            (user_id, group_id, isbless, grant_type) 
            VALUES ($userid, $adminid, 1, " . GRANT_DIRECT . ")");
    }

    $dbh->bz_lock_tables('groups READ',
                         'group_group_map WRITE');
    $dbh->do('DELETE FROM group_group_map WHERE member_id = ?',
             undef, $adminid);
    $sth = $dbh->prepare("SELECT id FROM groups");
    $sth->execute();
    while ( my ($id) = $sth->fetchrow_array() ) {
        # Admins can bless every group.
        $dbh->do("INSERT INTO group_group_map 
            (member_id, grantor_id, grant_type) 
            VALUES ($adminid, $id," . GROUP_BLESS . ")");
        # Admins can see every group.
        $dbh->do("INSERT INTO group_group_map 
            (member_id, grantor_id, grant_type) 
            VALUES ($adminid, $id," . GROUP_VISIBLE . ")");
        # Admins are initially members of every group.
        next if ($id == $adminid);
        $dbh->do("INSERT INTO group_group_map 
            (member_id, grantor_id, grant_type) 
            VALUES ($adminid, $id," . GROUP_MEMBERSHIP . ")");
    }
    $dbh->bz_unlock_tables();
}


my @groups = ();
$sth = $dbh->prepare("SELECT id FROM groups");
$sth->execute();
while ( my @row = $sth->fetchrow_array() ) {
    push (@groups, $row[0]);
}

#  Prompt the user for the email address and name of an administrator.  Create
#  that login, if it doesn't exist already, and make it a member of all groups.

$sth = $dbh->prepare("SELECT user_id FROM groups INNER JOIN user_group_map " .
                     "ON id = group_id WHERE name = 'admin'");
$sth->execute;
# when we have no admin users, prompt for admin email address and password ...
if ($sth->rows == 0) {
    my $login = "";
    my $realname = "";
    my $pass1 = "";
    my $pass2 = "*";
    my $admin_ok = 0;
    my $admin_create = 1;
    my $mailcheckexp = "";
    my $mailcheck    = ""; 

    # Here we look to see what the emailregexp is set to so we can 
    # check the email address they enter. Bug 96675. If they have no 
    # params (likely but not always the case), we use the default.
    if (-e "$datadir/params") { 
        require "$datadir/params"; # if they have a params file, use that
    }
    if (Bugzilla->params->{'emailregexp'}) {
        $mailcheckexp = Bugzilla->params->{'emailregexp'};
        $mailcheck    = Bugzilla->params->{'emailregexpdesc'};
    } else {
        $mailcheckexp = '^[\\w\\.\\+\\-=]+@[\\w\\.\\-]+\\.[\\w\\-]+$';
        $mailcheck    = 'A legal address must contain exactly one \'@\', 
        and at least one \'.\' after the @.';
    }

    print "\nLooks like we don't have an administrator set up yet.\n";
    print "Either this is your first time using Bugzilla, or your\n ";
    print "administrator's privileges might have accidentally been deleted.\n";
    while(! $admin_ok ) {
        while( $login eq "" ) {
            print "Enter the e-mail address of the administrator: ";
            $login = $answer{'ADMIN_EMAIL'} 
                || ($silent && die("cant preload ADMIN_EMAIL")) 
                || <STDIN>;
            chomp $login;
            if(! $login ) {
                print "\nYou DO want an administrator, don't you?\n";
            }
            unless ($login =~ /$mailcheckexp/) {
                print "\nThe login address is invalid:\n";
                print "$mailcheck\n";
                print "You can change this test on the params page once checksetup has successfully\n";
                print "completed.\n\n";
                # Go round, and ask them again
                $login = "";
            }
        }
        $sth = $dbh->prepare("SELECT login_name FROM profiles " .
                              "WHERE " . $dbh->sql_istrcmp('login_name', '?'));
        $sth->execute($login);
        if ($sth->rows > 0) {
            print "$login already has an account.\n";
            print "Make this user the administrator? [Y/n] ";
            my $ok = $answer{'ADMIN_OK'} 
                || ($silent && die("cant preload ADMIN_OK")) 
                || <STDIN>;
            chomp $ok;
            if ($ok !~ /^n/i) {
                $admin_ok = 1;
                $admin_create = 0;
            } else {
                print "OK, well, someone has to be the administrator.\n";
                print "Try someone else.\n";
                $login = "";
            }
        } else {
            print "You entered $login.  Is this correct? [Y/n] ";
            my $ok = $answer{'ADMIN_OK'} 
                || ($silent && die("cant preload ADMIN_OK")) 
                || <STDIN>;
            chomp $ok;
            if ($ok !~ /^n/i) {
                $admin_ok = 1;
            } else {
                print "That's okay, typos happen.  Give it another shot.\n";
                $login = "";
            }
        }
    }

    if ($admin_create) {
        while( $realname eq "" ) {
            print "Enter the real name of the administrator: ";
            $realname = $answer{'ADMIN_REALNAME'} 
                || ($silent && die("cant preload ADMIN_REALNAME")) 
                || <STDIN>;
            chomp $realname;
            if(! $realname ) {
                print "\nReally.  We need a full name.\n";
            }
            if(! is_7bit_clean($realname)) {
                print "\nSorry, but at this stage the real name can only " . 
                      "contain standard English\ncharacters.  Once Bugzilla " .
                      "has been installed, you can use the 'Prefs' page\nto " .
                      "update the real name.\n";
                $realname = '';
            }
        }

        # trap a few interrupts so we can fix the echo if we get aborted.
        $SIG{HUP}  = \&bailout;
        $SIG{INT}  = \&bailout;
        $SIG{QUIT} = \&bailout;
        $SIG{TERM} = \&bailout;

        if ($^O !~ /MSWin32/i) {
            system("stty","-echo");  # disable input echoing
        }

        while( $pass1 ne $pass2 ) {
            while( $pass1 eq "" || $pass1 !~ /^[[:print:]]{3,16}$/ ) {
                print "Enter a password for the administrator account: ";
                $pass1 = $answer{'ADMIN_PASSWORD'} 
                    || ($silent && die("cant preload ADMIN_PASSWORD")) 
                    || <STDIN>;
                chomp $pass1;
                if(! $pass1 ) {
                    print "\n\nAn empty password is a security risk. Try again!\n";
                } elsif ( $pass1 !~ /^.{3,16}$/ ) {
                    print "\n\nThe password must be 3-16 characters in length.\n";
                } elsif ( $pass1 !~ /^[[:print:]]{3,16}$/ ) {
                    print "\n\nThe password contains non-printable characters.\n";
                }
            }
            print "\nPlease retype the password to verify: ";
            $pass2 = $answer{'ADMIN_PASSWORD'} 
                || ($silent && die("cant preload ADMIN_PASSWORD")) 
                || <STDIN>;
            chomp $pass2;
            if ($pass1 ne $pass2) {
                print "\n\nPasswords don't match.  Try again!\n";
                $pass1 = "";
                $pass2 = "*";
            }
        }

        if ($^O !~ /MSWin32/i) {
            system("stty","echo"); # re-enable input echoing
        }

        $SIG{HUP}  = 'DEFAULT'; # and remove our interrupt hooks
        $SIG{INT}  = 'DEFAULT';
        $SIG{QUIT} = 'DEFAULT';
        $SIG{TERM} = 'DEFAULT';

        insert_new_user($login, $realname, $pass1);
    }

    # Put the admin in each group if not already    
    my $userid = $dbh->selectrow_array("SELECT userid FROM profiles WHERE " .
                                       $dbh->sql_istrcmp('login_name', '?'),
                                       undef, $login);

    # Admins get explicit membership and bless capability for the admin group
    my ($admingroupid) = $dbh->selectrow_array("SELECT id FROM groups 
                                                WHERE name = 'admin'");
    $dbh->do("INSERT INTO user_group_map 
        (user_id, group_id, isbless, grant_type) 
        VALUES ($userid, $admingroupid, 0, " . GRANT_DIRECT . ")");
    $dbh->do("INSERT INTO user_group_map 
        (user_id, group_id, isbless, grant_type) 
        VALUES ($userid, $admingroupid, 1, " . GRANT_DIRECT . ")");

    # Admins get inherited membership and bless capability for all groups
    foreach my $group ( @groups ) {
        my $sth_check = $dbh->prepare("SELECT member_id FROM group_group_map
                                 WHERE member_id = ?
                                 AND  grantor_id = ?
                                 AND grant_type = ?");
        $sth_check->execute($admingroupid, $group, GROUP_MEMBERSHIP);
        unless ($sth_check->rows) {
            $dbh->do("INSERT INTO group_group_map
                      (member_id, grantor_id, grant_type)
                      VALUES ($admingroupid, $group, " . GROUP_MEMBERSHIP . ")");
        }
        $sth_check->execute($admingroupid, $group, GROUP_BLESS);
        unless ($sth_check->rows) {
            $dbh->do("INSERT INTO group_group_map
                      (member_id, grantor_id, grant_type)
                      VALUES ($admingroupid, $group, " . GROUP_BLESS . ")");
        }
    }

    print "\n$login is now set up as an administrator account.\n";
}


#
# Final checks...

$sth = $dbh->prepare("SELECT user_id " .
                       "FROM groups INNER JOIN user_group_map " .
                       "ON groups.id = user_group_map.group_id " .
                      "WHERE groups.name = 'admin'");
$sth->execute;
my ($adminuid) = $sth->fetchrow_array;
if (!$adminuid) { die "No administrator!" } # should never get here
# when test product was created, admin was unknown
$dbh->do("UPDATE components " .
            "SET initialowner = $adminuid " .
          "WHERE initialowner = 0");

# Check if the default parameter for urlbase is still set, and if so, give
# notification that they should go and visit editparams.cgi 

my @params = Bugzilla::Config::Core::get_param_list();
my $urlbase_default = '';
foreach my $item (@params) {
    next unless $item->{'name'} eq 'urlbase';
    $urlbase_default = $item->{'default'};
    last;
}

if (Bugzilla->params->{'urlbase'} eq $urlbase_default) {
    print "Now that you have installed Bugzilla, you should visit the \n" .
          "'Parameters' page (linked in the footer of the Administrator \n" .
          "account) to ensure it is set up as you wish - this includes \n" .
          "setting the 'urlbase' option to the correct url.\n" unless $silent;
}
################################################################################
