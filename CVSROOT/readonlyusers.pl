#!/usr/bin/perl
#
# This is a simple script that forces specified users to
# have read-only access to the CVS repository when using SSH.
# This must be done in a pre-commit check script, as SSH users
# must have an account on the server with access to the repository.
# Returning "0" means the account is allowed to check-in. Any other
# return type will stop the commit from occurring.
#
# Authors: Reed Loden <reed@mozilla.com>
#          Aravind Gottipati <aravind@mozilla.com> 
#
# To make it work, you need to add the script name to
# CVSROOT/checkoutlist so that the script is checked out correctly
# on the CVS server.
# Also, you need to add a line to your
# CVSROOT/commitinfo file that says something like:
#
#      ALL	$CVSROOT/CVSROOT/readonlyusers.pl

use strict;
use warnings;

# Hash of read-only users
my %read_only_users = (
                       'calbld'    => 1,
                       'caminobld' => 1,
                       'ffxbld'    => 1,
                       'l10nbld'   => 1,
                       'nobody'    => 1,
                       'stgbld'    => 1,
                       'qabld'     => 1,
                       'unittest'  => 1,
                       'trybld'    => 1,
                       'xrbld'     => 1
                      );

my $username = $ENV{'CVS_USER'} || getlogin || (getpwuid($<))[0] || 'nobody';

if (exists $read_only_users{$username}) {
    print STDERR "The $username account is not permitted to check-in to this CVS repository.\n";
    print STDERR "If you think it should be allowed to do so, please contact\n";
    print STDERR "the system administrators at sysadmins\@mozilla.org.\n";
    exit 1;
}

exit 0;
