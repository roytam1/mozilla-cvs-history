#!/usr/bin/perl 
# Run this from cron to update the identifier database that lxr uses
# to turn function names into clickable links.
# Created 12-Jun-98 by jwz.
# Updated 27-Feb-99 by endico. Added multiple tree support.

use Cwd;
$ENV{'PATH'} = "/opt/local/bin:/opt/cvs-tools/bin:/usr/ucb:$ENV{'PATH'}" if -d '/opt/cvs-tools/bin';
my $TREE=shift;
$TREE =~ s{/$}{};
$ENV{'TREE'} = $TREE;

$lxr_dir = '.';
open LXRCONF, "< $lxr_dir/lxr.conf";
while ($line = <LXRCONF>) {
    $db_dir = "$1/$TREE" if $line =~ /^dbdir:\s*(\S+)/;
    unless ($TREE) {
        #since no tree is defined, assume sourceroot is defined the old way 
        #grab sourceroot from config file indexing only a single tree where
        #format is "sourceroot: dirname"
        $src_dir = $1 if $line =~ /^sourceroot:\s*(\S+)$/;
        if ($line =~ /^sourceroot:\s*(\S+)\s+\S+$/) {
            system("./update-xref.pl", $1);
        }
    } else {
        #grab sourceroot from config file indexing multiple trees where
        #format is "sourceroot: treename dirname"
        $src_dir = $1 if $line =~ /^sourceroot:\s*\Q$TREE\E\s+(\S+)$/;
    } 
}
close LXRCONF;
open HTACCESS, "< $lxr_dir/.htaccess";
while ($line = <HTACCESS>) {
    next unless $line =~ /^SetEnv\s+(\S+)\s+(.*)[\r\n]*$/;
    my ($envvar, $value) = ($1, $2);
#SetEnv LD_LIBRARY_PATH /zfsroot/.zfs/snapshot/solex_snv41_eol/usr/sfw/lib:/usr/sfw/lib:/zfsroot/.zfs/snapshot/solex_snv41_eol/usr/local/BerkeleyDB.4.4/lib:/usr/local/BerkeleyDB.4.4/lib
    if ($envvar =~ /PATH/) {
        $value = $ENV{$envvar}.':'.$value;
        $value =~ s/::/:/g;
    }
    $ENV{$envvar} = $value;
}
close HTACCESS;

mkdir $db_dir unless -d $db_dir;
$log = "$db_dir/genxref.log";

#exec > $log 2>&1
#XXX what does |set -x| mean?
#system ("set -x > $log");
system ("date >> $log");
$lxr_dir=getcwd;
mkdir "$db_dir/tmp" unless -d "$db_dir/tmp";
chdir "$db_dir/tmp";

#XXX what does |set -e| mean?
#system ("set -e >> $log");
if (system("time $lxr_dir/genxref $src_dir >> $log 2>&1") == 0) {
    system("chmod", "-R", "a+r", ".");
    system("mv xref fileidx ../");
} else {
    open LOG, ">> $log";
    print LOG 'Error executing genxref
';
    close LOG;
}
chdir "../..";
system ("date >> $log");
system ("uptime >> $log");

exit 0;
