#!/usr/bin/perl -w
# 
# filename:		longdescs_convert.pl
# description:	2000-01-20 Added a new "longdescs" table, which is supposed to have all the
# 				long descriptions in it, replacing the old long_desc field in the bugs 
# 				table.  The below hideous code populates this new table with things from
# 				the old field, with ugly parsing and heuristics.
# exceptions:	Oracle Only!
# contributors:	Holger Schurig <holgerschurig@nikocity.de>
#				David Lawrence <dkl@redhat.com>
#

use DBI;
use Date::Parse;
use Date::Format;

# database setup 
# Connect to Oracle Database
$ENV{'ORACLE_HOME'} = "/opt/oracle/product/805/";
$ENV{'ORACLE_SID'} = "bugzilla";
$ENV{'TWO_TASK'} = "bugzilla";
$ENV{'ORACLE_USERID'} = "bugzilla/bugzilla";
my $oracle_name = "bugzilla";
my $oracle_user = "bugzilla/bugzilla";
my $oracle_date = "SYSDATE";
my $oracle_dsn = "DBI:Oracle:$oracle_name";
my $dbh = DBI->connect($oracle_dsn, $oracle_user, '', { RaiseError => 1 })
        || die "Can't connect to database server: " . $DBI::errstr .
        " for $oracle_dsn, $oracle_user";
print "\n\nConnected to Oracle database.\n";

$dbh->{LongReadLen} = 1000 * 1024; # large object to read in
$dbh->{LongTruncOk} = 0;           # do not truncate


# subroutine:	WriteOneDesc
# description:	Inserts a comment into the new longdescs comments table
# params:		$id = current bug number (scalar)
#				$who = userid of who created the comment (scalar)
#				$when = date when the comment was created (scalar)
#				$buffer = the actual text of the comment (scalar)
# returns: 		none

sub WriteOneDesc {
    my ($id, $who, $when, $buffer) = (@_);
    $buffer = trim($buffer);
    if ($buffer eq '') {
        return;
    }
	my $query = "INSERT INTO longdescs (bug_id, who, bug_when, thetext) VALUES " .
             	"($id, $who, TO_DATE(" .  
             	$dbh->quote( time2str('%Y/%m/%d %H:%M:%S', $when) ) . ", 'YYYY-MM-DD HH24:MI:SS'), :1)"; 
    my $sth = $dbh->prepare($query);
	$sth->bind_param(1, $buffer, { SQL_LONGVARCHAR => 1 });
	$sth->execute();
}


# subroutine:   trim
# description:  Trim whitespace from front and back.
# params:       $_ = string to trim whitespace (scalar)
# returns:      $_ = string with whitespace removed (scalar)

sub trim {
    ($_) = (@_);
    s/^\s+//g;
    s/\s+$//g;
    return $_;
}


my $sth = $dbh->prepare("SELECT count(bug_id) FROM bugs");
$sth->execute();
my ($total) = ($sth->fetchrow_array);

print "Populating new long_desc table.  This is slow.  There are $total\n";
print "bugs to process; a line of dots will be printed for each 50.\n\n";
$| = 1;

$dbh->do('DELETE FROM longdescs');

$sth = $dbh->prepare("SELECT bug_id, TO_CHAR(creation_ts, 'YYYY-MM-DD HH24:MI:SS'), reporter, long_desc " .
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
                    # him or something.  Use a summy profile (Anonymous)
                    # since we used to allow anonymous comments.
                    ($who) = '4424'; # userid of user 'Anonymous'
                }
                next;
            } else {
#               print "\nDecided this line of bug $id has a date of " .
#                     time2str("'%Y/%m/%d %H:%M:%S'", $date) .
#                     "\nwhich is less than previous line:\n$line\n\n";
            }

        }
        $buffer .= $line . "\n";
    }
    WriteOneDesc($id, $who, $when, $buffer);
}

