#!/usr/bin/perl -w
# Run this from cron to update the source tree that lxr sees.
# Created 12-Jun-98 by jwz.
# Updated 27-Feb-99 by endico. Added multiple tree support.
my $skip_lxr_update = 1;
my $CVSROOT=':pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot';

$ENV{PATH}='/opt/local/bin:/opt/cvs-tools/bin:'.$ENV{PATH};

my $TREE=shift;
$TREE =~ s{/$}{};

my $lxr_dir='.';
open LXRCONF, '<', "$lxr_dir/lxr.conf" || die "can't open lxr.conf";
my $db_dir;
my %sourceroot = ();
do { 
$line = <LXRCONF>;
$db_dir = "$1" if $line =~ /^dbdir:\s*(.*)$/;
$sourceroot{$1} = $2 if $line =~ /^sourceroot:\s*(\S+ |)(.*)/;
} until eof LXRCONF;
die "could not find dbdir: directive"  unless defined $db_dir;
$db_dir .= "/$TREE" if $TREE ne '';

my $src_dir = $sourceroot{$TREE ? "$TREE " : ''};

    #since no tree is defined, assume sourceroot is defined the old way 
    #grab sourceroot from config file indexing only a single tree where
    #format is "sourceroot: dirname"

    #grab sourceroot from config file indexing multiple trees where
    #format is "sourceroot: treename dirname"

my $log="$db_dir/cvs.log";

open LOG, '>', $log;
#print LOG `set -x`;
print LOG `date`;

# update the lxr sources
print LOG `pwd`;
print LOG `time cvs -d $CVSROOT update -dP` unless $skip_lxr_update;

print LOG `date`;

# then update the Mozilla sources
chdir $src_dir;
chdir '..';

# endico: check out the source
for ($TREE) {
    /^classic$/ && do {
        print LOG `time cvs -Q -d $CVSROOT checkout -P -rMozillaSourceClassic_19981026_BRANCH MozillaSource`;
        last; 
    };
    /^ef$/ && do { 
        print LOG `time cvs -Q -d $CVSROOT checkout -P mozilla/ef mozilla/nsprpub`;
        last;
    };
    /^js$/ && do {
        print LOG `time cvs -Q -d $CVSROOT checkout -P mozilla/js mozilla/js2 mozilla/nsprpub`;
        last;
    };
    /^security$/ && do {
        print LOG `time cvs -Q -d $CVSROOT checkout -P mozilla/security mozilla/nsprpub`;
        last;
    };
    /^webtools$/ && do {
        print LOG `time cvs -Q -d $CVSROOT checkout -d webtools -P mozilla/webtools`;
        last;
    };
    /^bugzilla$/ && do {
        print LOG `time cvs -Q -d $CVSROOT checkout -P mozilla/webtools/bugzilla`;
        last;
    };
    /^update1.0$/ && do {
        print LOG `time cvs -Q -d $CVSROOT checkout -P -r MOZILLA_UPDATE_1_0_BRANCH mozilla/webtools/update`;
        last;
    };
    /^grendel$/ && do {
        print LOG `cd mozilla; time cvs up -d grendel`;
        last;
    };
    /^mailnews$/ && do {
        print LOG `time cvs -Q -d $CVSROOT checkout -P SeaMonkeyMailNews`;
        last;
    };
    /^mozilla$/ && do {
        print LOG `time cvs -Q -d $CVSROOT checkout -P mozilla`;
        last;
    };
    /^nspr$/ && do {
        print LOG `time cvs -Q -d $CVSROOT checkout -P NSPR`;
        last;
    };
    /^(?:seamonkey|aviarybranch|mozilla1.*)$/ && do {
        print LOG `time make -C mozilla -f client.mk pull_all MOZ_CO_PROJECT=all`;
        print LOG `cat cvsco.log`;
        print LOG `cd mozilla; time cvs up -d tools` if /^seamonkey$/;
        last;
    };
    /^(?:netbeans|openoffice|gnome|eclipse|mozilla.*-.*|devmo.*|)$/ && do {
        print LOG `cd $src_dir; time cvs up -d *`;
        last;
    };
    warn "unrecognized tree. fixme!";
}

print LOG `date`;
print LOG `uptime`;
close LOG;
exit 0;
