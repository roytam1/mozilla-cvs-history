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
#				  David Lawrence <dkl@redhat.com>

# Contains some global variables and routines used throughout bugzilla.

use diagnostics;
use strict;
use DBI;

use Date::Format;               # For time2str().
use Date::Parse;				# For str2time().
# use Carp;                       # for confess

# Shut up misguided -w warnings about "used only once".  For some reason,
# "use vars" chokes on me when I try it here.

sub globals_pl_sillyness {
    my $zz;
    $zz = @main::chooseone;
    $zz = @main::default_column_list;
    $zz = $main::defaultqueryname;
    $zz = @main::dontchange;
    $zz = %main::keywordsbyname;
    $zz = @main::legal_bug_status;
    $zz = @main::legal_components;
    $zz = @main::legal_keywords;
    $zz = @main::legal_opsys;
    $zz = @main::legal_platform;
    $zz = @main::legal_priority;
    $zz = @main::legal_product;
    $zz = @main::legal_severity;
    $zz = @main::legal_target_milestone;
    $zz = @main::legal_versions;
    $zz = @main::milestoneurl;
    $zz = @main::prodmaxvotes;
}

#
# Here are the --LOCAL-- variables defined in 'localconfig' that we'll use
# here
# 

# $::driver = 'mysql';
$::driver = 'Oracle';

# database setup
my $dbhost = "";
my $dbname = "";
my $dbuser = "";
my $dbpass = "";

if ($::driver eq "mysql") {
	$dbhost = "localhost";
	$dbname = "bugs";
	$dbuser = "bugs";
	$dbpass = "bugsPWD";
	do 'localconfig';

} else {
	$ENV{'ORACLE_HOME'} = "/opt/apps/oracle/product/8.0.5/";
	$ENV{'ORACLE_SID'} = "rheng";
	$ENV{'TWO_TASK'} = "rheng";
	$ENV{'ORACLE_USERID'} = "bugzilla/bugzilla";
	$dbname = "rheng";
	$dbuser = "bugzilla/bugzilla";
	$dbhost = "";
	$dbpass = "";
	$::date_string = "SYSDATE";
}

# Contains the version string for the current running Bugzilla.
$::param{'version'} = '2.8';

$::dontchange = "--do_not_change--";
$::chooseone = "--Choose_one:--";
$::currentquery = "";
$::defaultqueryname = "(Default query)";
$::unconfirmedstate = "UNCONFIRMED";
$::dbwritesallowed = 1;

# subroutine:	ConnectToDatabase
# description:	Make initial connection to bugs database
# params:		None
# returns:		None

sub ConnectToDatabase {
	my ($useshadow) = (@_);
	if (!defined $::db) {
        my $name = $dbname;
        if ($useshadow && Param("shadowdb") && Param("queryagainstshadowdb")) {
            $name = Param("shadowdb");
            $::dbwritesallowed = 0;
        }
    	$::db = DBI->connect("dbi:$::driver:$dbname", $dbuser, $dbpass, { RaiseError => 1 })
        	or die "Can't connect to database server: " .  $DBI::errstr;
    	$::db->{LongReadLen} = 1000000;
	}
}


# subroutine: 	ReconnectToShadowDatabase
# description:	connects to a shadow database if the normal one is unavailable
# params:		none
# returns:		none

sub ReconnectToShadowDatabase {
    if (Param("shadowdb") && Param("queryagainstshadowdb")) {
        SendSQL("USE " . Param("shadowdb"));
        $::dbwritesallowed = 0;
    }
}


my $shadowchanges = 0;

# subroutine: 	SyncAnyPendingShadowChanges
# description:	run script to sync up databases
# params:		none
# returns:		none

sub SyncAnyPendingShadowChanges {
    if ($shadowchanges) {
        system("./syncshadowdb &");
        $shadowchanges = 0;
    }
}

# set dosqllog to true if log file exists and is writable
my $dosqllog = (-e "data/sqllog") && (-w "data/sqllog");

# subroutine: 	SqlLog
# description: 	Logs each database query to a log file if that log file exists
# params:		$str = string to write to log file (scalar)
# returns:		None

sub SqlLog {
    if ($dosqllog) {
        my ($str) = (@_);
        open(SQLLOGFID, ">>data/sqllog") || die "Can't write to data/sqllog";
        if (flock(SQLLOGFID,2)) { # 2 is magic 'exclusive lock' const.
            print SQLLOGFID time2str("%D %H:%M:%S $$", time()) . ": $str\n";
        }
        flock(SQLLOGFID,8);     # '8' is magic 'unlock' const.
        close SQLLOGFID;
    }
}


# subroutine:	SendSQL
# description: 	Sends current sql statement to database for execution, also logs statement
#				to file if logfile exists
# params:		$str = string containing current sql statement (scalar)
#				$dontshadow = do not write into shadow database (scalar)
# returns:		None
 
sub SendSQL {
    my ($str, $dontshadow) = (@_);
	if (!$::db) {
		ConnectToDatabase();
	}
    my $iswrite =  ($str =~ /^(INSERT|REPLACE|UPDATE|DELETE)/i);
    if ($iswrite && !$::dbwritesallowed) {
        die "Evil code attempted to write stuff to the shadow database.";
    }
    if ($str =~ /^LOCK TABLES/i && $str !~ /shadowlog/ && $::dbwritesallowed) {
        $str =~ s/^LOCK TABLES/LOCK TABLES shadowlog WRITE, /i;
    }
    SqlLog($str);
    $::currentquery = $::db->prepare($str);
    $::currentquery->execute || die "$str: " . $DBI::errstr;
	SqlLog('Done');
	# Very mysql specific
	if (!$dontshadow && $iswrite && Param("shadowdb")) {
        my $q = SqlQuote($str);
        my $insertid;
        if ($str =~ /^(INSERT|REPLACE)/i) {
            SendSQL("SELECT LAST_INSERT_ID()");
            $insertid = FetchOneColumn();
        }
        SendSQL("INSERT INTO shadowlog (command) VALUES ($q)", 1);
        if ($insertid) {
            SendSQL("SET LAST_INSERT_ID = $insertid");
        }
        $shadowchanges++;
    }
}


# subroutine:	MoreSQLData
# description:	Returns true if there are more rows to return from database
# params:		None
# returns:		1 = more data available (scalar)
#				0 = no more data available (scalar)

sub MoreSQLData {
    if (defined @::fetchahead) {
		return 1;
    }
    if (@::fetchahead = $::currentquery->fetchrow_array()) {
		return 1;
    }
    return 0;
}


# subroutine:	FetchSQLData
# description:	Returns array containg the current row of data returned from database
# params:		None
# returns:		array containing the current row of data returned from database (array)

sub FetchSQLData {
    if (defined @::fetchahead) {
		my @result = @::fetchahead;
		undef @::fetchahead;
		return @result;
    }
    return $::currentquery->fetchrow();
}


# subroutine:	FetchOneColumn
# description:  Returns single column value if sql statement only receives one value
# params:		None
# returns:		$row[0] = single value returned from database (scalar)

sub FetchOneColumn {
    my @row = FetchSQLData();
    return $row[0];
}


# subroutine:   OracleQuote
# description:  Oracle needs to have certain things quoted in a different way than Mysql
# params:		$str = string to quote (scalar)
# returns:		$str = quoted string (scalar)

sub OracleQuote {
	my ($str) = (@_);
	return $::db->quote($str);
}


# subroutine: 	SqlDate
# description: 	Depending upon which database you are using and whether you are inserting or selecting
#				properly format the SQL statement to work with a date field
# params:		$field = name of date field in the database (scalar)
#				$date = date to insert into database (scalar)
# returns:		$sql = string with properly formed sql text (scalar)

sub SqlDate {
	my ($field, $date) = (@_);
	my $sql = "";

	# This is being used in an insert statement so form is 
	# Oracle: to_date('$date', 'FORMAT') 
	# Mysql: 
	if (defined ($date) && $date ne '') {
		if ($::driver eq 'mysql') {
		
		} else {
			$sql = " to_date('$date', 'YYYY-MM-DD HH24:MI:SS') ";
		}

	# this is being used in a select so form is 
	# Oracle: to_char(field, 'FORMAT')
	# Mysql: date_format(field, 'FORMAT')
	} elsif (defined ($field) && $field ne '') {
		if ($::driver eq 'mysql') {
			$sql = " date_format($field, '%Y-%m-%d %H:%i:%S') "; 
		} else {
			$sql = " to_char($field, 'YYYY-MM-DD HH24:MI:SS') ";
		}
	} else {
		die "Illegal date information passed to SqlDate\n";
	}	
	return $sql;
}


# subroutine:	CanISee
# description: 	Added by Red Hat to support bug privacy feature using new group permissions 
# params:		$id = id number of current bug report (scalar)
# 				$userid = userid of current member (scalar)
# returns:		1 = user can see the bug report (scalar)
#				0 = user does not have permission to see (scalar)
  
sub CanISee {
	my ($id, $userid) = (@_);
	my $query = "select count(groupid) from bug_group where bugid = $id";
	SendSQL($query);
	my $result = FetchOneColumn();
	if (!$result) {	# bug is public if no groups are set
		return 1;
	}
	if ($userid == 0) {
		return 0;
	}
	$query = "select bug_group.bugid from bug_group, user_group " .
             "where user_group.userid = $userid " .
             "and bug_group.bugid = $id " . 
             "and user_group.groupid = bug_group.groupid ";
	SendSQL($query);
	$result = FetchOneColumn();
	if ($result) {
		return 1;
	} else {
		return 0;
	}
}


# subroutine: 	CanIChange
# description:	Added by Red Hat to allow some protection against people changing other's reports.
# 				This really needs to be more flexible.
#				One idea is if you are in the group for which it is private then you are allowed to
#				to change the bug. Also need a caneditall type group designation.
# params:		$id = current id number of bug report (scalar)
#				$login = login name of current bugzilla user (scalar)
#				$reporter = reporter of current bug report (scalar)
#				$assigned = member bug report is currently assigned to (scalar)
# returns:		1 = current member can make modifications to bug (scalar)
# 				0 = current member can not make modifications to bug (scalar)

sub CanIChange {
	my ($id, $login, $reporter, $assigned) = (@_);
	if (!defined($login)) {
		return 0;
	}
	if ($login eq $reporter) {
		return 1;
	}
	if ($login eq $assigned) {
        return 1;
    }
	SendSQL("select count(groupid) from user_group where userid = $login");
	my $result = FetchOneColumn();
	if ($result > 0) {
		return 1;
	}	
	return 0;
}


# subroutine: 	GenProductList
# description: 	Added by Red Hat. Generates product list based on new group permissions
# params: 		$userid = userid of current bugzilla member (scalar)
#				@old_product_list = array holding raw products list (array)
# returns: 		@new_product_list = array holding permitted products (array)

sub GenProductList {
	my ($userid, @old_product_list) = (@_);
	my @new_product_list;
	foreach my $product (@old_product_list) {
		my $query = "select product_group.productid " .
					"from product_group, products " . 
					"where products.id = product_group.productid and " .
					"products.product = " . SqlQuote($product);
		SendSQL($query);
		my $result = FetchOneColumn();
		if (!$result) {
			push (@new_product_list, $product);				
			next;
		}
		$query = "select product_group.productid from product_group, user_group " .
             	 "where user_group.userid = $userid " .
             	 "and product_group.productid = $result " .
             	 "and user_group.groupid = product_group.groupid ";
    	SendSQL($query);
    	$result = FetchOneColumn();
    	if ($result) {
        	push (@new_product_list, $product);
    	} 
	}
	return @new_product_list;	
}


# subroutine: 	GenVersionList
# description: 	Added by Red Hat. Generates version list based on modified product list 
# params: 		none 
# returns: 		array holding sorted legal versions (array)

sub GenVersionList {
	my @new_version_list;
	foreach my $product (@::legal_product) {
		foreach my $version (@{$::versions{$product}}) {
			if (lsearch(\@new_version_list, $version) >= 0) {
				# if already same number in list then skip
				next;
			} else {
				push (@new_version_list, $version);
			}
		}
	}
	# sort numerically
	return (sort { $a <=> $b } @new_version_list);
}


@::default_column_list = ("severity", "priority", "platform", "owner",
                          "status", "resolution", "summary");


# subroutine: 	AppendComment
# description:	adds additional comment to current bug report
# params:		$bugid = current bug report number (scalar)
#				$who = login name of current bugzilla member (scalar)
#				$comment = string containing new comment to append (scalar)
# returns:		none

sub AppendComment {
    my ($bugid, $who, $comment) = (@_);
    $comment =~ s/\r\n/\n/g;     # Get rid of windows-style line endings.
    $comment =~ s/\r/\n/g;       # Get rid of mac-style line endings.
    if ($comment =~ /^\s*$/) {  # Nothin' but whitespace.
        return;
    }

    my $whoid = DBNameToIdAndCheck($who);

	if ($::driver eq "mysql") {
	    SendSQL("INSERT INTO longdescs (bug_id, who, bug_when, thetext) " .
    	        "VALUES($bugid, $whoid, now(), " . SqlQuote($comment) . ")");
	    SendSQL("UPDATE bugs SET delta_ts = now() WHERE bug_id = $bugid");
	} else {
		SendSQL("INSERT INTO longdescs (bug_id, who, bug_when, thetext) " .
                "VALUES($bugid, $whoid, sysdate, " . SqlQuote($comment) . ")");
        SendSQL("UPDATE bugs SET delta_ts = sysdate WHERE bug_id = $bugid");
	}
}


# subroutine: 	GetFieldID
# description:	returns field id for selected field or creates one if one does not exist
# params:		$f = field name (scalar)
# returns:		$fieldid = id of current field name (scalar)

sub GetFieldID {
    my ($f) = (@_);
    SendSQL("SELECT fieldid FROM fielddefs WHERE name = " . SqlQuote(lc($f)));
    my $fieldid = FetchOneColumn();
    if (!$fieldid) {
        my $q = SqlQuote($f);
		if ($::driver eq 'mysql') {
        	SendSQL("REPLACE INTO fielddefs (name, description) VALUES ($q, $q)");
        	SendSQL("SELECT LAST_INSERT_ID()");
		} else {
        	SendSQL("INSERT INTO fielddefs (fieldid, name, description, sortkey) " .
					"VALUES (fielddefs_seq.nextval, $q, $q, fielddefs_seq.nextval)");
        	SendSQL("SELECT fielddefs_seq.currval from dual");
		}	
        $fieldid = FetchOneColumn();
    }
    return $fieldid;
}


# subroutine:	lsearch
# description:	searchs through list of items and returns index where item is found, -1 if not found
# params:		$list = array reference to list of items to search (arrayref)
#				$item = item to look for in list (scalar)
# returns:		$count = index where item was found (scalar)
#				-1 = item was not found in list (scalar)

sub lsearch {
    my ($list,$item) = (@_);
    my $count = 0;
    foreach my $i (@$list) {
        if ($i eq $item) {
            return $count;
        }
        $count++;
    }
    return -1;
}


# subroutine: 	Product_element
# description:	make popup selection for products
# params:		$prod = default product or current product (scalar)
#				$onchange = javascript string for adding to popup widget (scalar)
# returns: 		string containing html for popup widget creation (scalar)

sub Product_element {
    my ($prod,$onchange) = (@_);
    return make_popup("product", keys %::versions, $prod, 1, $onchange);
}


# subroutine:   Component_element
# description:  make popup selection for components 
# params:       $comp = default component or current component (scalar)
#				$prod = default product or current product (scalar)
#               $onchange = javascript string for adding to popup widget (scalar)
# returns:      string containing html for popup widget creation (scalar)

sub Component_element {
    my ($comp,$prod,$onchange) = (@_);
    my $componentlist;
    if (! defined $::components{$prod}) {
        $componentlist = [];
    } else {
        $componentlist = $::components{$prod};
    }
    my $defcomponent;
    if ($comp ne "" && lsearch($componentlist, $comp) >= 0) {
        $defcomponent = $comp;
    } else {
        $defcomponent = $componentlist->[0];
    }
    return make_popup("component", $componentlist, $defcomponent, 1, "");
}


# subroutine:   Version_element
# description:  make popup selection for versions 
# params: 		$vers = default version of current version (scalar)      
#				$prod = default product or current product (scalar)
#               $onchange = javascript string for adding to popup widget (scalar)
# returns:      string containing html for popup widget creation (scalar)

sub Version_element {
    my ($vers, $prod, $onchange) = (@_);
    my $versionlist;
    if (!defined $::versions{$prod}) {
        $versionlist = [];
    } else {
        $versionlist = $::versions{$prod};
    }
    my $defversion = $versionlist->[0];
    if (lsearch($versionlist,$vers) >= 0) {
        $defversion = $vers;
    }
    return make_popup("version", $versionlist, $defversion, 1, $onchange);
}
 

# subroutine: 	Milestone_element 
# description:	make popup selection for milestones	
# params:		$tm = default or current target milestone (scalar)
#				$prod = default product or current product (scalar)
#				$onchange = javascript string for adding to popup widget (scalar)
# returns:		string containing html for popup widget creation (scalar)
      
sub Milestone_element {
    my ($tm, $prod, $onchange) = (@_);
    my $tmlist;
    if (!defined $::target_milestone{$prod}) {
        $tmlist = [];
    } else {
        $tmlist = $::target_milestone{$prod};
    }

    my $deftm = $tmlist->[0];

    if (lsearch($tmlist, $tm) >= 0) {
        $deftm = $tm;
    }

    return make_popup("target_milestone", $tmlist, $deftm, 1, $onchange);
}


# subroutine:	PerlQuote
# description:	Generate a string which, when later interpreted by the Perl compiler, will
# 				be the same as the given string.
# params:		$str = string for quoting (scalar)
# returns:		quoted string (scalar)

sub PerlQuote {
	my ($str) = (@_);
    if (!defined $str) {
         confess("Undefined passed to SqlQuote");
    }
    $str =~ s/([\\\'])/\\$1/g;
    $str =~ s/\0/\\0/g;
    return "'$str'";

# FIXME: Changed this because Oracle uses a different method of escaping characters than Mysql so
# therefore a ' in Mysql is escaped as \' and in Oracle it is ''. So therefore PerlQuote and
# SqlQuote could not really be one in the same.

# The below was my first attempt, but I think just using SqlQuote makes more 
# sense...
#     $result = "'";
#     $length = length($str);
#     for (my $i=0 ; $i<$length ; $i++) {
#         my $c = substr($str, $i, 1);
#         if ($c eq "'" || $c eq '\\') {
#             $result .= '\\';
#         }
#         $result .= $c;
#     }
#     $result .= "'";
#     return $result;

}


# subroutine:	GenerateCode
# description:	Given the name of a global variable, generate Perl code that, if later
# 				executed, would restore the variable to its current value.
# params:		$name = name of global variable (scalar)
# returns:		$result = final code that would restore variable later (scalar)

sub GenerateCode {
    my ($name) = (@_);
    my $result = $name . " = ";
    if ($name =~ /^\$/) {
        my $value = eval($name);
        if (ref($value) eq "ARRAY") {
            $result .= "[" . GenerateArrayCode($value) . "]";
        } else {
            $result .= PerlQuote(eval($name));
        }
    } elsif ($name =~ /^@/) {
        my @value = eval($name);
        $result .= "(" . GenerateArrayCode(\@value) . ")";
    } elsif ($name =~ '%') {
        $result = "";
        foreach my $k (sort { uc($a) cmp uc($b)} eval("keys $name")) {
            $result .= GenerateCode("\$" . substr($name, 1) .
                                    "{'" . $k . "'}");
        }
        return $result;
    } else {
        die "Can't do $name -- unacceptable variable type.";
    }
    $result .= ";\n";
    return $result;
}


# subroutine:   GenerateArrayCode
# description:  Returns a string containing list of items suitable for generating an array  
# params:       $ref = array reference to list of items (arrayref)
# returns:      string containing formatted list of items (scalar)

sub GenerateArrayCode {
    my ($ref) = (@_);
    my @list;
    foreach my $i (@$ref) {
        push @list, PerlQuote($i);
    }
    return join(',', @list);
}


# subroutine:	GenerateVersionTable
# description: 	Generates file containing global variables such as products, 
#				components, versions, etc. Saves time from accessing database for this information.
# params:		None
# returns:		None

sub GenerateVersionTable {
    ConnectToDatabase();
    SendSQL("select value, program from versions order by value");
    my @line;
    my %varray;
    my %carray;
    while (@line = FetchSQLData()) {
        my ($v,$p1) = (@line);
        if (!defined $::versions{$p1}) {
            $::versions{$p1} = [];
        }
        push @{$::versions{$p1}}, $v;
        $varray{$v} = 1;
    }
    SendSQL("select value, program from components order by value");
    while (@line = FetchSQLData()) {
        my ($c,$p) = (@line);
        if (!defined $::components{$p}) {
            $::components{$p} = [];
        }
        my $ref = $::components{$p};
        push @$ref, $c;
        $carray{$c} = 1;
    }

#    my $dotargetmilestone = Param("usetargetmilestone");
	my $dotargetmilestone = 1;  # This used to check the param, but there's
                                # enough code that wants to pretend we're using
                                # target milestones, even if they don't get
                                # shown to the user.  So we cache all the data
                                # about them anyway.

    my $mpart = $dotargetmilestone ? ", milestoneurl" : "";
    SendSQL("select product, description, votesperuser, disallownew$mpart from products");
    $::anyvotesallowed = 0;
    while (@line = FetchSQLData()) {
        my ($p, $d, $votesperuser, $dis, $u) = (@line);
        $::proddesc{$p} = $d;
        if ($dis) {
            # Special hack.  Stomp on the description and make it "0" if we're
            # not supposed to allow new bugs against this product.  This is
            # checked for in enter_bug.cgi.
            $::proddesc{$p} = "0";
        }
        if ($dotargetmilestone) {
            $::milestoneurl{$p} = $u;
        }
        $::prodmaxvotes{$p} = $votesperuser;
		if ($votesperuser > 0) {
            $::anyvotesallowed = 1;
        }
    }
            

    my $cols = LearnAboutColumns("bugs");
    
    @::log_columns = @{$cols->{"-list-"}};
#	foreach my $i ("bug_id", "creation_ts", "delta_ts", "long_desc") {
    foreach my $i ("bug_id", "creation_ts", "delta_ts", "lastdiffed") {
        my $w = lsearch(\@::log_columns, $i);
        if ($w >= 0) {
            splice(@::log_columns, $w, 1);
        }
    }
    @::log_columns = (sort(@::log_columns));

	if ($::driver eq 'mysql') {
    	@::legal_priority_contract = SplitEnumType($cols->{"priority,type"});
#		@::legal_priority = SplitEnumType($cols->{"priority,type"});
    	@::legal_severity = SplitEnumType($cols->{"bug_severity,type"});
    	@::legal_platform = SplitEnumType($cols->{"rep_platform,type"});
    	@::legal_opsys = SplitEnumType($cols->{"op_sys,type"});
    	@::legal_bug_status = SplitEnumType($cols->{"bug_status,type"});
    	@::legal_resolution = SplitEnumType($cols->{"resolution,type"});
	} else {
    	@::legal_severity = SplitTableValues("bug_severity");
    	@::legal_platform = SplitTableValues("rep_platform");
    	@::legal_opsys = SplitTableValues("op_sys");
    	@::legal_bug_status = SplitTableValues("bug_status");
    	@::legal_resolution = SplitTableValues("resolution");
		@::legal_priority_contract = SplitTableValues("priority");
	}

	# Added by Red Hat for contract support bugs
	@::legal_priority = @::legal_priority_contract;
    my $w = lsearch(\@::legal_priority, "contract");
    if ($w >= 0) {
        splice(@::legal_priority, $w, 1);
    }

	# create a list of possible resolutions without a 'duplicate'
    @::legal_resolution_no_dup = @::legal_resolution;
	my $w = lsearch(\@::legal_resolution_no_dup, "DUPLICATE");
    if ($w >= 0) {
        splice(@::legal_resolution_no_dup, $w, 1);
    }

    my @list = sort { uc($a) cmp uc($b)} keys(%::versions);
    @::legal_product = @list;
    mkdir("data", 0777);
    chmod 0777, "data";
    my $tmpname = "data/versioncache.$$";
    open(FID, ">$tmpname") || die "Can't create $tmpname";

    print FID GenerateCode('@::log_columns');
    print FID GenerateCode('%::versions');

    foreach my $i (@list) {
        if (!defined $::components{$i}) {
            $::components{$i} = "";
        }
    }
    @::legal_versions = sort {uc($a) cmp uc($b)} keys(%varray);
    print FID GenerateCode('@::legal_versions');
    print FID GenerateCode('%::components');
    @::legal_components = sort {uc($a) cmp uc($b)} keys(%carray);
    print FID GenerateCode('@::legal_components');
    foreach my $i('product', 'priority', 'severity', 'platform', 'opsys',
				  'bug_status', 'resolution', 'resolution_no_dup', 'priority_contract') {
        print FID GenerateCode('@::legal_' . $i);
    }
    print FID GenerateCode('%::proddesc');
#    print FID GenerateCode('%::prodmaxvotes');
    print FID GenerateCode('$::anyvotesallowed');

    if ($dotargetmilestone) {
        # reading target milestones in from the database - matthew@zeroknowledge.com
        SendSQL("SELECT value, product FROM milestones ORDER BY sortkey, value");
        my @line;
        my %tmarray;
		%::milestoneurl = ();
        @::legal_target_milestone = ();
        while(@line = FetchSQLData()) {
            my ($tm, $pr) = (@line);
            if (!defined $::target_milestone{$pr}) {
                $::target_milestone{$pr} = [];
            }
            push @{$::target_milestone{$pr}}, $tm;
            if (!exists $tmarray{$tm}) {
                $tmarray{$tm} = 1;
                push(@::legal_target_milestone, $tm);
            }
        }

        print FID GenerateCode('%::target_milestone');
        print FID GenerateCode('@::legal_target_milestone');
        print FID GenerateCode('%::milestoneurl');
    }

    SendSQL("SELECT id, name FROM keyworddefs ORDER BY name");
    while (MoreSQLData()) {
        my ($id, $name) = FetchSQLData();
        $::keywordsbyname{$name} = $id;
        push(@::legal_keywords, $name);
    }
    print FID GenerateCode('@::legal_keywords');
    print FID GenerateCode('%::keywordsbyname');

    print FID "1;\n";
    close FID;
    rename $tmpname, "data/versioncache" || die "Can't rename $tmpname to versioncache";
    chmod 0666, "data/versioncache";
}


# subroutine:	ModTime
# description:	Returns the modification time of a file.
# params:		$filename = name of file to return mod time for (scalar)
# returns:		$mtime = modification time of file (scalar)

sub ModTime {
    my ($filename) = (@_);
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks)
        = stat($filename);
    return $mtime;
}


# subroutine:	GetVersionTable
# description:	This proc must be called before using legal_product or the versions array.
#				Loads global variables into memory contains cached product, version values.
# params:		none
# returns:		none

sub GetVersionTable {
    my $mtime = ModTime("data/versioncache");
    if (!defined $mtime || $mtime eq "") {
        $mtime = 0;
    }
    if (time() - $mtime > 3600) {
        GenerateVersionTable();
    }
    require 'data/versioncache';
    if (!defined %::versions) {
        GenerateVersionTable();
        do 'data/versioncache';

        if (!defined %::versions) {
            die "Can't generate file data/versioncache";
        }
    }

	# Added by Red Hat: support for new product group permissions	
	# GenVersionList must come after GenProductList
#	my $userid = 0;
#	if (defined($::COOKIE{'Bugzilla_login'})) {
#		$userid = DBname_to_id($::COOKIE{'Bugzilla_login'});
#	}
#	@::legal_product = GenProductList($userid, @::legal_product);
#	@::legal_versions = GenVersionList();

}


# subroutine: 	InsertNewUser
# description:	Inserts a new bugzilla member into the database
# params:		$username = email address of new member (scalar)
# 				$realname = real name of new member (optional) (scalar)
# returns:		$password = password created for new bugzilla member (scalar)
		
sub InsertNewUser {
    my ($username, $realname) = (@_);
    my $password = "";
	my $groupset = "0";
	for (my $i=0 ; $i<8 ; $i++) {
        $password .= substr("abcdefghijklmnopqrstuvwxyz", int(rand(26)), 1);
    }

	SendSQL("select bit, userregexp from groups where userregexp != ''");
	my @row;
    while (MoreSQLData()) {
        my @row = FetchSQLData();
        if ($username =~ m/$row[1]/) {
            $groupset .= "+$row[0]"; 	# Silly hack to let MySQL do the math,
            							# not Perl, since we're dealing with 64
            							# bit ints here, and Perl won't do it.
    	}
	}
            
	my $encrypted = crypt($password, substr($password, 0, 2));
	if ($::driver eq "mysql") {
    	SendSQL("insert into profiles (login_name, realname, password, cryptpassword, groupset, emailnotification) " .
				"values (" . SqlQuote($username) . ", " . SqlQuote($realname) . ", " . 
				SqlQuote($password) . ", " . SqlQuote($encrypted) . ", $groupset, 'ExludeSelfChangesOnly')");
	} else {
		SendSQL("insert into profiles (userid, login_name, realname, password, cryptpassword, groupset, emailnotification) " .
				"values (profiles_seq.nextval, " . OracleQuote($username) . ", " . OracleQuote($realname) .
				", " . OracleQuote($password) . ", " . OracleQuote($encrypted) . ", $groupset, 'ExludeSelfChangesOnly')");
	}
    return $password;
}


# subroutine: 	DBID_to_name
# description:	converts user id to actual login name 
# params:		$id = user id of current bugzilla member (scalar)
# returns:		string containing login name belonging to user id (scalar)
	
sub DBID_to_name {
    my ($id) = (@_);
    if (!defined $::cachedNameArray{$id}) {
        SendSQL("select login_name from profiles where userid = $id");
        my $r = FetchOneColumn();
        if ($r eq "") {
            $r = "__UNKNOWN__";
        }
        $::cachedNameArray{$id} = $r;
    }
    return $::cachedNameArray{$id};
}


# subroutine:	DBname_to_id
# description:	converts a member's login name to their user id
# params:		$name = current member's login name (scalar)
# returns:		$r = user id for belonging to current login name (scalar)

sub DBname_to_id {
    my ($name) = (@_);
    SendSQL("select userid from profiles where login_name = @{[SqlQuote($name)]}");
    my $r = FetchOneColumn();
    if (!defined $r || $r eq "") {
        return 0;
    }
    return $r;
}


# subroutine:	DBNameToIdAndCheck
# description:  returns the user id belong to current login name and if $forceok is true,
#				add them to database if not currently a member. Sends out quiet email with password.
# params:		$name = current login name to look up user id (scalar)
# 				$forceok = true if add to database is ok, false if not ok (scalar)
# returns:		$result = userid belonging to current login name (scalar)

sub DBNameToIdAndCheck {
    my ($name, $forceok) = (@_);
    my $result = DBname_to_id($name);
    if ($result > 0) {
        return $result;
    }
    if ($forceok) {
		my $password = InsertNewUser($name, "");
		# This account was added as a result of a cc add or component assignment
		# so we want to quitely email out a password.
		MailPassword($name, $password, 1);
		$result = DBname_to_id($name);
        if ($result > 0) {
            return $result;
        }
        print "Yikes; couldn't create user $name.  Please report problem to " .
            Param("maintainer") ."\n";
    } else {
        print "The name <TT>$name</TT> is not a valid username.  Either you\n";
        print "misspelled it, or the person has not registered for a\n";
        print "Bugzilla account.\n";
        print "<P>Please hit the <B>Back</B> button and try again.\n";
    }
	exit(0);
}


# subroutine:	quoteUrls
# description:	This routine quoteUrls contains inspirations from the HTML::FromText CPAN
# 				module by Gareth Rees <garethr@cre.canon.co.uk>.  It has been heavily hacked,
# 				all that is really recognizable from the original is bits of the regular
#			 	expressions.
# params:		$knownattachments = hash reference to list of attachments for bug (hash ref)
#				$text = the actual text of the bug report (scalar)
# returns:		$text = string containing original text with html markups (scalar)

sub quoteUrls {
    my ($knownattachments, $text) = (@_);
    return $text unless $text;

    my $base = Param('urlbase');

    my $protocol = join '|',
    qw(afs cid ftp gopher http https mid news nntp prospero telnet wais);

    my %options = ( metachars => 1, @_ );

    my $count = 0;

    # Now, quote any "#" characters so they won't confuse stuff later
    $text =~ s/#/%#/g;

    # Next, find anything that looks like a URL or an email address and
    # pull them out the the text, replacing them with a "##<digits>##
    # marker, and writing them into an array.  All this confusion is
    # necessary so that we don't match on something we've already replaced,
    # which can happen if you do multiple s///g operations.

    my @things;
    while ($text =~ s%((mailto:)?([\w\.\-\+\=]+\@[\w\-]+(?:\.[\w\-]+)+)\b|
                    (\b((?:$protocol):[^ \t\n<>"]+[\w/])))%"##$count##"%exo) {
        my $item = $&;

        $item = value_quote($item);

        if ($item !~ m/^$protocol:/o && $item !~ /^mailto:/) {
            # We must have grabbed this one because it looks like an email
            # address.
            $item = qq{<A HREF="mailto:$item">$item</A>};
        } else {
            $item = qq{<A HREF="$item">$item</A>};
        }

        $things[$count++] = $item;
    }
    while ($text =~ s/\bbug(\s|%\#)*(\d+)/"##$count##"/ei) {
        my $item = $&;
        my $num = $2;
        $item = value_quote($item); # Not really necessary, since we know
                                    # there's no special chars in it.
        $item = qq{<A HREF="show_bug.cgi?id=$num">$item</A>};
        $things[$count++] = $item;
    }
    while ($text =~ s/\*\*\* This bug has been marked as a duplicate of (\d+) \*\*\*/"##$count##"/ei) {
        my $item = $&;
        my $num = $1;
        $item =~ s@\d+@<A HREF="show_bug.cgi?id=$num">$num</A>@;
        $things[$count++] = $item;
    }
    while ($text =~ s/Created an attachment \(id=(\d+)\)/"##$count##"/e) {
        my $item = $&;
        my $num = $1;
        if ($knownattachments->{$num}) {
            $item = qq{<A HREF="showattachment.cgi?attach_id=$num">$item</A>};
        }
        $things[$count++] = $item;
    }

    $text = value_quote($text);
    $text =~ s/\&#010;/\n/g;

    # Stuff everything back from the array.
    for (my $i=0 ; $i<$count ; $i++) {
        $text =~ s/##$i##/$things[$i]/e;
    }

    # And undo the quoting of "#" characters.
    $text =~ s/%#/#/g;

    return $text;
}


# subroutine:   GetLongDescriptionAsText
# description:  Returns long description belonging to current bug report as text
# params:       $id = id number for current bug report (scalar)
#				$start = start days before now (scalar)
# 				$end = end days before now (scalar)
# returns:      string containing long description (scalar)

sub GetLongDescriptionAsText {
    my ($id, $start, $end) = (@_);
    my $result = "";
    my $count = 0;

    my $query = "";
    if ($::driver eq "mysql") {
        $query = "SELECT profiles.login_name, longdescs.bug_when, " .
                 "longdescs.thetext " .
                 "FROM longdescs, profiles " .
                 "WHERE profiles.userid = longdescs.who " .
                 "AND longdescs.bug_id = $id ";
    } else {
	    $query = "SELECT profiles.login_name, TO_CHAR(longdescs.bug_when, 'YYYY-MM-DD HH24:MI:SS'), " .
                 "longdescs.thetext " .
                 "FROM longdescs, profiles " .
                 "WHERE profiles.userid = longdescs.who " .
                 "AND longdescs.bug_id = $id ";
    }

    if ($start && $start =~ /[1-9]/) {
        # If the start is all zeros, then don't do this (because we want to
        # not emit a leading "Additional Comments" line in that case.)
    	if ($::driver eq 'mysql') {
		    $query .= "AND longdescs.bug_when > '$start'";
    	} else {
			$query .= "AND longdescs.bug_when > TO_DATE('$start', 'YYYY-MM-DD HH24:MI:SS') ";
		}
	    $count = 1;
    }
    if ($end) {
		if ($::driver eq 'mysql') {
 	        $query .= "AND longdescs.bug_when <= '$end'";
		} else {
			$query .= "AND longdescs.bug_when <= TO_DATE('$end', 'YYYY-MM-DD HH24:MI:SS') ";
		}
    }

    $query .= "ORDER BY longdescs.bug_when";
    SendSQL($query);
    while (MoreSQLData()) {
        my ($who, $when, $text) = (FetchSQLData());
        if ($count) {
            $result .= "\n\n------- Additional comments from $who  " .
                time2str("%Y-%m-%d %H:%M", str2time($when)) . " -------\n";
        }
        $result .= $text;
        $count++;
    }
    return $result;
}


# subroutine:   GetLongDescriptionAsHTML
# description:  Returns long description belonging to current bug report as html
# params:       $id = id number for current bug report (scalar)
#               $start = start days before now (scalar)
#               $end = end days before now (scalar)
# returns:      string containing long description (scalar)

sub GetLongDescriptionAsHTML {
    my ($id, $start, $end) = (@_);
    my $result = "";
    my $count = 0;
    my %knownattachments;
    SendSQL("SELECT attach_id FROM attachments WHERE bug_id = $id");
    while (MoreSQLData()) {
        $knownattachments{FetchOneColumn()} = 1;
    }

    my $query = "";
    if ($::driver eq "mysql") {
        $query = "SELECT profiles.login_name, longdescs.bug_when, " .
                 "longdescs.thetext " .
                 "FROM longdescs, profiles " .
                 "WHERE profiles.userid = longdescs.who " .
                 "AND longdescs.bug_id = $id ";
    } else {
    	$query = "SELECT profiles.login_name, TO_CHAR(longdescs.bug_when, 'YYYY-mm-dd HH:MI'), " .
                 "longdescs.thetext " .
                 "FROM longdescs, profiles " .
                 "WHERE profiles.userid = longdescs.who " .
                 "AND longdescs.bug_id = $id ";
    }

    if ($start && $start =~ /[1-9]/) {
        # If the start is all zeros, then don't do this (because we want to
        # not emit a leading "Additional Comments" line in that case.)
        $query .= "AND longdescs.bug_when > '$start'";
        $count = 1;
    }
    if ($end) {
        $query .= "AND longdescs.bug_when <= '$end'";
    }

    $query .= "ORDER BY longdescs.bug_when";
    SendSQL($query);
    while (MoreSQLData()) {
        my ($who, $when, $text) = (FetchSQLData());
        if ($count) {
            $result .= "<BR>------- <I>Additional comments from</I> " .
                qq{<A HREF="mailto:$who">$who</A> } .
                    time2str("%Y-%m-%d %H:%M", str2time($when)) .
                        " -------<BR>\n";
        }
        $result .= "<PRE>" . quoteUrls(\%knownattachments, $text) . "</PRE>\n";
        $count++;
    }

    return $result;
}


# subroutine:	ShowCcList
# description:	returns formatted list of cc addresses for current bug report
# params:		$num = number of current bug report (scalar)
# returns:		string containing comma separated list of cc addresses (scalar)

sub ShowCcList {
    my ($num) = (@_);
    my @ccids;
    my @row;
    SendSQL("select who from cc where bug_id = $num");
    while (@row = FetchSQLData()) {
        push(@ccids, $row[0]);
    }
    my @result = ();
    foreach my $i (@ccids) {
        push @result, DBID_to_name($i);
    }
    return join(',', @result);
}


# subroutine:	LearnAboutColumns
# description:	Fills in a hashtable with info about the columns for the given table in the
# 				database.  The hashtable has the following entries:
#   			-list-  the list of column names
#   			<name>,type  the type for the given name
# params:		$table = name of table to return gather information on (scalar)
# returns:		%a = reference to hash containing table information (scalar)

sub LearnAboutColumns {
    my ($table) = (@_);
    my %a;
	my @list;
	my @row;

	if ($::driver eq "mysql") {
    	SendSQL("show columns from $table");
	} else {
		SendSQL("select column_name, data_type from user_tab_columns " .
                "where table_name = " . SqlQuote(uc($table)));
	}
    while (@row = FetchSQLData()) {
        my ($name,$type) = (@row);
		$name = uc($name);
        $a{"$name,type"} = $type;
        push @list, $name;
		chop($a{"$name,type"});
    }
    $a{"-list-"} = \@list;
    return \%a;
}


# subroutine: 	SplitEnumType
# description: 	If the above returned a enum type, take that type and parse it into the
# 				list of values.  Assumes that enums don't ever contain an apostrophe! (MySQL Only)
# params:		$str = enum values returned from mysql database for parsing (scalar)
# returns:		@result = array holding parsed enum values from mysql database (scalar)

sub SplitEnumType {
    my ($str) = (@_);
    my @result = ();
    if ($str =~ /^enum\((.*)\)$/) {
        my $guts = $1 . ",";
        while ($guts =~ /^\'([^\']*)\',(.*)$/) {
            push @result, $1;
            $guts = $2;
		}
    }
    return @result;
}


# subroutine:	SplitTableValues
# description:	This will take a table of values that were previously enum data types and return 
# 				the legal values
# params:		$str = table name (scalar)
# returns: 		@result = array holding values from table

sub SplitTableValues {
	my ($str) = (@_);
	my @result = ();
	my @row = ();
	my $query = "select value from $str";
	SendSQL($query);
	while (@row = FetchSQLData()) {
		push (@result, $row[0]);
	}
	return @result;
}


# subroutine:	SqlQuote
# description:	This routine is largely copied from Mysql.pm.
# params:		$str = string to sql quote (scalar)
# returns:		$str = quoted string (scalar)

sub SqlQuote {
    my ($str) = (@_);
    if (!defined $str) {
         confess("Undefined passed to SqlQuote");
    }
	if ($::driver eq 'mysql') {
    	$str =~ s/([\\\'])/\\$1/g;
	} else {
		$str =~ s/([\\\'])/\'$1/g;
	}
    $str =~ s/\0/\\0/g;	
#	return $::db->quote($str);
    return "'$str'";
}


# subroutine:	UserInGroup
# description: 	determines if the current bugzilla member is in a particular group
# params:		$groupname = name of group to check for (scalar)
# returns:		1 = current bugzilla member is a member of $groupname (scalar)
#				0 = current bugzilla member is not a member of $groupname (scalar)

sub UserInGroup {
    my ($groupname) = (@_);
	if (!defined($::COOKIE{'Bugzilla_login'})) {
		return 0;
	}
	my $userid = DBname_to_id($::COOKIE{'Bugzilla_login'});
    ConnectToDatabase();
	if ($::driver eq 'mysql') {
		if ($::usergroupset eq '0') {
			return 0;
		}
		SendSQL("select (bit & $::usergroupset) != 0 from groups " .
				"where name = " . SqlQuote($groupname));
	} else {
    	SendSQL("select user_group.groupid from groups, user_group" .
				" where user_group.groupid = groups.groupid" . 
				" and groups.name = " . SqlQuote($groupname) . 
				" and user_group.userid = $userid"); 
	}
    my $result = FetchOneColumn();
    if ($result) {
        return 1;
    }
    return 0;
}


# subroutine:	GroupExists
# description: 	Verifies that specific group name exists in the database
# params:		$groupname = name of group to verify (scalar)
# returns:		$count = number of groups matching name, usually 1 (scalar)

sub GroupExists {
    my ($groupname) = (@_);
    ConnectToDatabase();
    SendSQL("select count(*) from groups where name = " . SqlQuote($groupname));
    my $count = FetchOneColumn();
    return $count;
}


# subroutine:	GroupsBelong
# description:	returns list of groups that the current bugzilla member belongs to
# 				Added by Red Hat
# params:		none
# returns:		@result = list of group numbers that member belongs to (array)

sub GroupsBelong {
		my $userid = DBname_to_id($::COOKIE{'Bugzilla_login'});
		SendSQL("select groupid from user_group where userid = $userid");
		my @result;
		my @row;
		while (@row = FetchSQLData()) {
			push (@result, $row[0]);
		}
		return @result;
}

# subroutine: 	UserInContract
# description: 	returns true if user belongs to one or more contract groups
# exceptions: 	only called if param('contract') true
# params: 		$userid = user id of current bugzilla member (scalar)
# returns: 		1 = member belongs to one or more contract groups (scalar)
#				0 = member does not belong to one or more contract groups (scalar)

sub UserInContract {
	my $userid = shift;
	
	my $query = "select user_group.userid from groups, user_group " . 
				"where groups.groupid = user_group.groupid " .
				"and groups.contract = 1 " . 
				"and user_group.userid = $userid";
	SendSQL($query);
	my $result = FetchOneColumn();
	if ($result) {
		return 1;
	} else { 
		return 0;
	}
}


# subroutine: 	IsOpenedState
# description:	Determines if the given bug_status string represents an "Opened" bug.  This
# 				routine ought to be paramaterizable somehow, as people tend to introduce
# 				new states into Bugzilla.
# params:		$state = current state of the bug report in question (scalar)
# returns:		1 = bug report is in a new, reopened, or assigned state (scalar)
#				0 = bug report is not in a new, reopened, or assigned state (scalar)

sub IsOpenedState {
    my ($state) = (@_);
    if ($state =~ /^(NEW|REOPENED|ASSIGNED)$/ || $state eq $::unconfirmedstate) {
        return 1;
    }
    return 0;
}


# subroutine:	RemovedVotes
# description:	informs people who voted that their votes have been removed from database
# params:		$id = bug report number (scalar)
#				$who = who is removing the votes (scalar)
#				$reason = reason that votes were removed (scalar)
# returns:		none

sub RemoveVotes {
    my ($id, $who, $reason) = (@_);
    ConnectToDatabase();
    my $whopart = "";
    if ($who) {
        $whopart = " AND votes.who = $who";
    }
    SendSQL("SELECT profiles.login_name, votes.count " .
            "FROM votes, profiles " .
            "WHERE votes.bug_id = $id " .
            "AND profiles.userid = votes.who" .
            $whopart);
    my @list;
    while (MoreSQLData()) {
        my ($name, $count) = (FetchSQLData());
        push(@list, [$name, $count]);    
	}
    if (0 < @list) {
        foreach my $ref (@list) {
            my ($name, $count) = (@$ref);
	        if (open(SENDMAIL, "|/usr/lib/sendmail -t")) {
    	        my %substs;
	            $substs{"to"} = $name;
	            $substs{"bugid"} = $id;
	            $substs{"reason"} = $reason;
				$substs{"count"} = $count;
                my $msg = PerformSubsts(Param("voteremovedmail"),
                                        \%substs);
                print SENDMAIL $msg;
	            close SENDMAIL;
	        }
		}
        SendSQL("DELETE FROM votes WHERE bug_id = $id" . $whopart);
        SendSQL("SELECT SUM(count) FROM votes WHERE bug_id = $id");
        my $v = FetchOneColumn();
        $v ||= 0;
		if ($::driver eq 'mysql') {
        	SendSQL("UPDATE bugs SET votes = $v, delta_ts = delta_ts " .
            	    "WHERE bug_id = $id");
		} else {
			SendSQL("UPDATE bugs SET votes = $v, delta_ts = sysdate " .
                    "WHERE bug_id = $id");
		}
    }
}


# subroutine:	Param
# description:	return value defined in the data/params file. 
# params:		$value = param name to return value for (scalar)
# returns:		string containing value associated with the param name (scalar)

sub Param {
    my ($value) = (@_);
    if (defined $::param{$value}) {
        return $::param{$value};
    }
    # See if it is a dynamically-determined param (can't be changed by user).
    if ($value eq "commandmenu") {
        return GetCommandMenu();
    }
    if ($value eq "settingsmenu") {
        return GetSettingsMenu();
    }
    # Um, maybe we haven't sourced in the params at all yet.
    if (stat("data/params")) {
        # Write down and restore the version # here.  That way, we get around
        # anyone who maliciously tries to tweak the version number by editing
        # the params file.  Not to mention that in 2.0, there was a bug that
        # wrote the version number out to the params file...
        my $v = $::param{'version'};
        require "data/params";
        $::param{'version'} = $v;
    }
    if (defined $::param{$value}) {
        return $::param{$value};
    }
    # Well, that didn't help.  Maybe it's a new param, and the user
    # hasn't defined anything for it.  Try and load a default value
    # for it.
    require "defparams.pl";
    WriteParams();
    if (defined $::param{$value}) {
        return $::param{$value};
    }
    # We're pimped.
    die "Can't find param named $value";
}


# subroutine:	PerformSubsts
# description:	replace placeholders with proper variable values
# params:		$str = string containing place holders (scalar)
#				$substs = hash reference containing values to replace with (hashref)
# returns:		$str = string with variable values in place (scalar)
    
sub PerformSubsts {
    my ($str, $substs) = (@_);
    $str =~ s/%([a-z]*)%/(defined $substs->{$1} ? $substs->{$1} : Param($1))/eg;
    return $str;
}


# subroutine:	trim
# description:	Trim whitespace from front and back.
# params:		$_ = string to trim whitespace (scalar)
# returns:		$_ = string with whitespace removed (scalar)

sub trim {
    ($_) = (@_);
    s/^\s+//g;
    s/\s+$//g;
    return $_;
}

1;
