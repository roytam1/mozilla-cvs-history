#!/usr/bin/perl -w 
# Run this from cron to update the glimpse database that lxr uses
# to do full-text searches.
# Created 12-Jun-98 by jwz.
# Updated 2-27-99 by endico. Added multiple tree support.

use Cwd;
my @paths=qw(
/opt/local/bin
/opt/cvs-tools/bin
/usr/ucb
/usr/local/apache/html/mxr/glimpse
/usr/local/glimpse-4.18.1p/bin
/usr/local/glimpse-3.6/bin
/export/home/build/glimpse-3.6.src/bin
);
my $TREE=shift || '';
$TREE =~ s{/$}{};
$ENV{'LANG'} = 'C';
$ENV{'TREE'} = $TREE;

$lxr_dir = '.';
my $lxr_conf = "$lxr_dir/lxr.conf";
unless (-f $lxr_conf) {
die "could not find $lxr_conf";
}
open LXRCONF, "< $lxr_conf";
while ($line = <LXRCONF>) {
    $db_dir = "$1/$TREE" if $line =~ /^dbdir:\s*(\S+)/;
    unless ($TREE) {
        #since no tree is defined, assume sourceroot is defined the old way 
        #grab sourceroot from config file indexing only a single tree where
        #format is "sourceroot: dirname"
        $src_dir = $1 if $line =~ /^sourceroot:\s*(\S+)$/;
        if ($line =~ /^sourceroot:\s*(\S+)\s+\S+$/) {
            system("./update-search.pl", $1);
        }
    } else {
        #grab sourceroot from config file indexing multiple trees where
        #format is "sourceroot: treename dirname"
        $src_dir = $1 if $line =~ /^sourceroot:\s*\Q$TREE\E\s+(\S+)$/;
    } 
    if ($line =~ /^glimpsebin:\s*(.*)\s*$/) {
        $glimpse_bin = $1;
        $glimpse_bin =~ m{(.*)/([^/]*)$};
        push @paths, $1;
    }
}
close LXRCONF;

unless (defined $src_dir) {
die "could not find sourceroot for tree $TREE";
}

my %pathmap=();
for my $mapitem (@paths) {
$pathmap{$mapitem} = 1;
}
for my $possible_path (keys %pathmap) {
$ENV{'PATH'} = "$possible_path:$ENV{'PATH'}" if -d $possible_path;
}

mkdir $db_dir unless -d $db_dir;
$log = "$db_dir/glimpseindex.log";

mkdir $db_dir unless -d $db_dir;
#exec > $log 2>&1
#XXX what does |set -x| mean?
#system ("set -x > $log");
=pod
              -e      Exit  immediately  if a simple command (see
                      SHELL GRAMMAR above) exits with a  non-zero
                      status. 

              -x      After  expanding  each  simple command, for
                      command, case command, select  command,  or
                      arithmetic   for   command,   display   the
                      expanded value of PS4, followed by the com�
                      mand  and its expanded arguments or associ�
                      ated word list.
=cut

open LOG, ">>$log";

=pod
for my $envvar (keys %ENV) {
print LOG "$envvar=$ENV{$envvar}
";
}
=cut

#system ("date >> $log");
print LOG 'date
'.localtime().'
';

unless (-d $src_dir) {
print LOG "$TREE src_dir $src_dir does not exist.
";
exit 4;
} 

my $db_dir_tmp = "$db_dir/tmp";
unless (-d $db_dir_tmp) {
print LOG "mkdir $db_dir_tmp
";
unless (mkdir $db_dir_tmp) {
print LOG "mkdir $db_dir_tmp failed";
exit 5;
}
}

print LOG "chdir $db_dir_tmp
";
unless (chdir $db_dir_tmp) {
print LOG "chdir $db_dir_tmp failed";
exit 6;
} 

# do index everything in lxrroot
open GLIMPSEINCLUDE, '>.glimpse_include';
print GLIMPSEINCLUDE $db_dir;
close GLIMPSEINCLUDE;

# don't index CVS files
open GLIMPSEEXCLUDE, '>.glimpse_exclude';
# don't index CVS files
print GLIMPSEEXCLUDE '/CVS/
';
# don't index SVN files
print GLIMPSEEXCLUDE '/.svn/
';
close GLIMPSEEXCLUDE;

#XXX what does |set -e| mean?
#system ("set -e >> $log");
#system("time", "glimpseindex", "-H", ".", "$src_dir");
my $cmd = "time glimpseindex -H . $src_dir >> $log 2>&1";
print LOG "$cmd
";
close LOG;
system($cmd);
open LOG, ">>$log";
print LOG 'chmod -R a+r .
';
system("chmod", "-R", "a+r", ".");
$cmd = 'mv .glimpse* ../'; 
print LOG "$cmd
";
system($cmd);
print LOG 'cd ../..
';
chdir '../..';
print LOG 'date
'.localtime().'
updtime
';
close LOG;
system ("uptime >> $log");

exit 0;
