#!/usr/bin/perl -w
#
# migrate.pl
#

use DBI;
use DBD::Oracle qw{:ora_types};

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
my $oracle_dbh = DBI->connect($oracle_dsn, $oracle_user, '', { RaiseError => 1 })
        || die "Can't connect to database server: " . $DBI::errstr .
        " for $oracle_dsn, $oracle_user";
print "\n\nConnected to Oracle database.\n";

$oracle_dbh->{LongReadLen} = 1000 * 1024; # large object
$oracle_dbh->{LongTruncOk} = 0;           # do not truncate

# Connect to Mysql Database
my $mysql_name = "bugs";
my $mysql_user = "bugs";
my $mysql_dsn = "DBI:mysql:$mysql_name";
my $mysql_date = "now()";
$mysql_dbh = DBI->connect($mysql_dsn, $mysql_user, '', { RaiseError => 1})
        || die "Can't connect to database server: " . $DBI::errstr .
        " for $mysql_dsn, $mysql_user";
$mysql_dbh->{LongReadLen} = 1000 * 1024; # large object
$mysql_dbh->{LongTruncOk} = 0;           # do not truncate
print "Connected to MySQL database.\n";

my $query = "";
my $mysql_sth = "";
my $oracle_sth = "";
my $count = 1;

# grab a list of tables in mysql database
my %tables;
$query = "show tables";
$mysql_sth = $mysql_dbh->prepare($query);
$mysql_sth->execute();
while (my @row = $mysql_sth->fetchrow_array()) {
	$tables{$row[0]} = []; 
}

#grab the columns associated with each of the tables
foreach my $table (keys %tables) {
	$query = "show columns from $table";
	$mysql_sth = $mysql_dbh->prepare($query);
	$mysql_sth->execute();
	while (my @row = $mysql_sth->fetchrow_array()) {
		push (@{$tables{$table}}, $row[0]);
	}
}

# print the table information 
#foreach my $table (keys %tables) {
#	print "\n$table\n";
#	foreach my $column (@{$tables{$table}}) {
#		print "\t$column\n";
#	}
#}


# for each table form insert statements in Oracle
foreach my $table (keys %tables) {
	if ($table eq "bugs" ||
		$table eq "bugs_activity" ||
		$table eq "attachments" ||
		$table eq "longdescs" ||
		$table eq "logincookies" ||
		$table eq "errata" || 
		$table eq "type" || 
		$table eq "queries") {
		next;
	}
	$query = "select " . join (", ", @{$tables{$table}}) . 
			 " from $table";
	$mysql_sth = $mysql_dbh->prepare($query);
    $mysql_sth->execute();
	while (my @row = $mysql_sth->fetchrow_array()) {
		my @quoted = ();
		foreach my $field (@row) {
			push (@quoted, $oracle_dbh->quote($field));
		} 
		$query = "insert into $table (" . join (", ", @{$tables{$table}}) . 
				 ") values (" . join (", ", @quoted) . ")";
		# print $query . "\n";
	    $oracle_sth = $oracle_dbh->prepare($query);
		$oracle_sth->execute();
		print "$count entry added to $table\n";
		$count++;
	}
}


# logincookies
@columns = ( 'cookie',
             'userid',
             'cryptpassword',
             'hostname',
             'lastused');

$query = "select " . join (', ', @columns) . " from logincookies";
$mysql_sth = $mysql_dbh->prepare($query);
$mysql_sth->execute();

$count = 1;
print "\n\n";

while (my @row = $mysql_sth->fetchrow_array()) {
    my $cookie = $oracle_dbh->quote($row[0]);
    my $userid = $oracle_dbh->quote($row[1]);
    my $cryptpassword = $oracle_dbh->quote($row[2]);
    my $hostname = $oracle_dbh->quote($row[3]);
    my $lastused = "TO_DATE(" . $oracle_dbh->quote($row[4]) . ", 'YYYYMMDDHH24MISS')";

    $query = "insert into logincookies ( " .
             join (', ', @columns) . " ) " .
             "values ($cookie, $userid, $cryptpassword, " .
             "$hostname, $lastused)";
    # print $query . "\n";  
    $oracle_sth = $oracle_dbh->prepare($query);
    $oracle_sth->execute();
	print "$count entry added to logincookies\n";
	$count++;
}


# bugs_activity
@columns = ( 'bug_id',
             'who',
             'bug_when',
             'field',
             'oldvalue',
             'newvalue' );

$query = "select " . join (', ', @columns) . " from bugs_activity";
$mysql_sth = $mysql_dbh->prepare($query);
$mysql_sth->execute();

$count = 1;
print "\n\n";

while (my @row = $mysql_sth->fetchrow_array()) {
    my $bug_id = $oracle_dbh->quote($row[0]);
    my $who = $oracle_dbh->quote($row[1]);
    my $bug_when = "TO_DATE(" .
        $oracle_dbh->quote($row[2]) . ", 'YYYY-MM-DD HH24:MI:SS')";
    my $field = $oracle_dbh->quote($row[3]);
    if (!defined($row[4])) {
        $row[4] = "";
    }
    my $oldvalue = $oracle_dbh->quote($row[4]);
    if (!defined($row[5])) {
        $row[5] = "";
    }
    my $newvalue = $oracle_dbh->quote($row[5]);

    $query = "insert into bugs_activity ( " .
             join (', ', @columns) . " ) " .
             "values ($bug_id, $who, $bug_when, " .
             "$field, $oldvalue, $newvalue)";
    # print $query . "\n";  
    $oracle_sth = $oracle_dbh->prepare($query);
    $oracle_sth->execute();
	print "$count entry added to bugs_activity\n";
	$count++;
}


# queries
$query = "select userid, query_name, query from queries";
$mysql_sth = $mysql_dbh->prepare($query);
$mysql_sth->execute();

$count = 1;
print "\n\n";

while (my @row = $mysql_sth->fetchrow_array()) {
    my $userid = $mysql_dbh->quote($row[0]);
    my $query_name = $mysql_dbh->quote($row[1]);
    my $query_long = $mysql_dbh->quote($row[2]);
    $oracle_sth = $oracle_dbh->prepare("insert into queries (userid, query_name, query) values ($userid, $query_name, ?)");
    $oracle_sth->bind_param(1, $query_long, { SQL_LONGVARCHAR => 1});
    $oracle_sth->execute();
	print "$count entry added to queries\n";
	$count++;
}


# attachments
@columns = ( 'attach_id',
             'bug_id',
             'creation_ts',
             'description',
             'mimetype',
             'ispatch',
             'filename',
             'thedata',
             'submitter_id' );

$query = "select " . join (', ', @columns) . " from attachments";
$mysql_sth = $mysql_dbh->prepare($query);
$mysql_sth->execute();

$count = 1;
print "\n\n";

while (my @row = $mysql_sth->fetchrow_array()) {
    my $attach_id = $oracle_dbh->quote($row[0]);
    my $bug_id = $oracle_dbh->quote($row[1]);
    my $creation_ts = "TO_DATE(" . $oracle_dbh->quote($row[2]) . ", 'YYYYMMDDHH24MISS')";
    my $desc = $oracle_dbh->quote($row[3]);
    my $mime = $oracle_dbh->quote($row[4]);
    my $ispatch = $oracle_dbh->quote($row[5]);
    my $filename = $oracle_dbh->quote($row[6]);
#   my $thedata = $oracle_dbh->quote( pack ('H*', $row[7]) );
    my $thedata = $oracle_dbh->quote($row[7]);
    my $submitter = $oracle_dbh->quote($row[8]);

    $query = "insert into attachments ( " . join (', ', @columns) . " ) " .
             "values ($attach_id, $bug_id, $creation_ts, $desc, $mime, " .
             "$ispatch, $filename, :1, $submitter)";
    # print $query . "\n";
    $oracle_sth = $oracle_dbh->prepare($query);
    $oracle_sth->bind_param(1, $thedata, { ora_field => 'thedata', ora_type => ORA_BLOB });
    $oracle_sth->execute();
	print "$count entry added to attachments\n";
	$count++;
}


# long descriptions
@columns = ( 'bug_id',
             'who',
             'bug_when',
             'thetext');

$query = "select " . join (', ', @columns) . " from longdescs";
$mysql_sth = $mysql_dbh->prepare($query);
$mysql_sth->execute();

$count = 1;
print "\n\n";

while (my @row = $mysql_sth->fetchrow_array()) {
    my $bug_id = $oracle_dbh->quote($row[0]);
    my $who = $oracle_dbh->quote($row[1]);
    my $thetext = $oracle_dbh->quote($row[3]);
    my $bug_when = "TO_DATE(" . $oracle_dbh->quote($row[2]) . ", 'YYYY-MM-DD HH24:MI:SS')";

    $query = "insert into longdescs ( " .
             join (', ', @columns) . " ) " .
             "values ($bug_id, $who, $bug_when, $thetext)";
    # print $query . "\n";
    $oracle_sth = $oracle_dbh->prepare($query);
    $oracle_sth->execute();
    print "$count entry added to long descriptions\n";
    $count++;
}


# the bugs themselves
my %columns = ( 'bug_id' => '',
                'group_id' => '',
                'assigned_to' => '',
                'bug_file_loc' => '',
                'patch_file_loc' => '',
                'bug_severity' => '',
                'bug_status' => '',
                'view' => '',
                'creation_ts' => '',
                'delta_ts' => '',
                'short_desc' => '',
                'long_desc' => '',
                'op_sys' => '',
                'priority' => '',
                'product' => '',
                'rep_platform' => '',
                'reporter' => '',
                'version' => '',
                'release' => '',
                'component' => '',
                'resolution' => '',
                'class' => '',
                'target_milestone' => '',
                'qa_contact' => '',
                'status_whiteboard' => '',
                'groupset' => '',
                'votes' => '');

my @columns_select = keys %columns;
my @oracle_columns;
my $longdesc = "";

# all this because Oracle cannot have a column name of 'view'
foreach my $column (@columns_select) {
    if ($column eq 'view') {
        push (@oracle_columns, 'bug_view');
    } else {
        push (@oracle_columns, $column);
    }
}

$query = "select " . join (', ', @columns_select) . " from bugs";
# print $query . "\n";
$mysql_sth = $mysql_dbh->prepare($query);
$mysql_sth->execute();

$count = 1;
print "\n\n";

while (my @row = $mysql_sth->fetchrow_array()) {
    my $count = 0;
    foreach my $column (@columns_select) {
        $columns{$column} = $row[$count];
        $count++;
    }

    foreach my $column (@columns_select) {
        if (!defined($columns{$column})) {
            $columns{$column} = '';
        }
        if ($column eq 'delta_ts') {
            $columns{$column} = "TO_DATE('$columns{$column}', 'YYYYMMDDHH24MISS')";
            next;
        }
        if ($column eq 'creation_ts') {
            $columns{$column} = "TO_DATE('$columns{$column}', 'YYYY-MM-DD HH24-MI-SS')";
            next;
        }
        if ($column eq "long_desc") {
            $longdesc = $columns{$column};
            $columns{$column} = ":1";
            next;
        }
        $columns{$column} = $oracle_dbh->quote($columns{$column});
    }

    my @columns_insert;
    foreach my $column (@columns_select) {
        push (@columns_insert, $columns{$column});
    }

    $query = "insert into bugs ( " . join (', ', @oracle_columns) . " ) " .
             "values (" . join (', ', @columns_insert) . " )";
	# print $query . "\n";
    $oracle_sth = $oracle_dbh->prepare($query);
    $oracle_sth->bind_param(1, $longdesc, {SQL_LONGVARCHAR => 1 });
    $oracle_sth->execute();
    print "$count entry added to bugs\n";
    $count++;
}


# lastly, created the proper sequences
%tables = (  'bugs' 		=> 'bug_id',
             'logincookies' => 'cookie',
             'attachments' 	=> 'attach_id',
             'products' 	=> 'id',
             'profiles' 	=> 'userid',
			 'fielddefs' 	=> 'fieldid');
foreach my $table (keys %tables) {
    eval {
        $query = "drop sequence " . $table . "_seq";
        print $query . "\n";
        $oracle_sth = $oracle_dbh->prepare($query);
        $oracle_sth->execute();
    };
    if ($@) { print "No sequence for $table. Continuing...\n" }
    $query = "select max($tables{$table}) from $table";
    print $query . "\n";
    $oracle_sth = $oracle_dbh->prepare($query);
    $oracle_sth->execute();
    my @result = $oracle_sth->fetchrow_array();
    my $nextval = $result[0] + 1;
    $query = "create sequence " . $table . "_seq start with $nextval increment by 1";
    print $query . "\n";
    $oracle_sth = $oracle_dbh->prepare($query);
    $oracle_sth->execute();
}

print "\n\nAll Done!\n";

$mysql_dbh->disconnect();
$oracle_dbh->disconnect();

