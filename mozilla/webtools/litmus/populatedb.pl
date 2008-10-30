#!/usr/bin/perl -w
# -*- mode: cperl; c-basic-offset: 8; indent-tabs-mode: nil; -*-

# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Litmus.
#
# The Initial Developer of the Original Code is
# the Mozilla Corporation.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Chris Cooper <ccooper@deadsquid.com>
#   Zach Lipton <zach@zachlipton.com>
#
# ***** END LICENSE BLOCK *****

use strict;
use Getopt::Long;
use Litmus::Config;
use DBI;
$|++;

my $reset_db;
my $help;
GetOptions('help|?' => \$help,'r|resetdb' => \$reset_db);

if ($help) {
  &usage;
  exit;
}

unless (-e "data/") {
    system("mkdir", "data/");
}
system("chmod -R 777 data/");
unless (-e "localconfig") {
     open(OUT, ">localconfig");
         print OUT <<EOT;
our \$db_host = "";
our \$db_name = "";
our \$db_user = "";
our \$db_pass = "";

our \$user_cookiename = "litmus_login";
our \$sysconfig_cookiename = "litmustestingconfiguration";

our \$sendmail_path = "/usr/sbin/sendmail";

our \$tr_host = "";
our \$tr_name = "";
our \$tr_user = "";
our \$tr_pass = "";

our \$bugzilla_auth_enabled = 0;
our \$bugzilla_db = "bugzilla";
our \$bugzilla_host = "localhost";
our \$bugzilla_user = "litmus";
our \$bugzilla_pass = "litmus";
EOT
        close(OUT);
        print "Go edit 'localconfig' with your configuration and \n";
        print "run this script again.\n";
        exit;
} 

# Create database tables, in large part borrowed from the old-school
# --TABLE-- code in checksetup.pl:
print "\nChecking for missing/new database tables...\n";
our %table;
do 'schema.pl'; # import the schema (%table)
my $dbh = DBI->connect("dbi:mysql:;$Litmus::Config::db_host", 
	$Litmus::Config::db_user,
	$Litmus::Config::db_pass) || 
		die "Could not connect to mysql database $Litmus::Config::db_host";

my @databases = $dbh->func('_ListDBs');
    unless (grep($_ eq $Litmus::Config::db_name, @databases)) {
       print "Creating database $Litmus::Config::db_name ...\n";
       if (!$dbh->func('createdb', $Litmus::Config::db_name, 'admin')) {
            my $error = $dbh->errstr;
            die "Could not create database $Litmus::Config::db_name: $error\n";
	}
}
$dbh->disconnect if $dbh;

$dbh = DBI->connect(
	"dbi:mysql:$Litmus::Config::db_name:$Litmus::Config::db_host",
	$Litmus::Config::db_user,
	$Litmus::Config::db_pass);


# Get a list of the existing tables (if any) in the database
my $sth = $dbh->table_info(undef, undef, undef, "TABLE");
my @tables = @{$dbh->selectcol_arrayref($sth, { Columns => [3] })};

# go throught our %table hash and create missing tables
while (my ($tabname, $fielddef) = each %table) {
    next if grep($_ eq $tabname, @tables);
    print "Creating table $tabname ...\n";

    $dbh->do("CREATE TABLE $tabname (\n$fielddef\n) TYPE = MYISAM")
        or die "Could not create table '$tabname'. Please check your database access.\n";
}

# populate the database with the default data in populatedb.sql:
if ($reset_db) {
  my $data_file = "populatedb.sql";

  print "Populating tables with default data...";
  my $cmd = "mysql --user=$Litmus::Config::db_user --password=$Litmus::Config::db_pass $Litmus::Config::db_name < $data_file";
  my $rv = system($cmd);
  if ($rv) {
    die "Error populating database $Litmus::Config::db_name";
  }
  print "done.\n";
}

# UPGRADE THE SCHEMA
# Now we need to deal with upgrading old installations by adding new fields 
# and indicies to the schema. To do this, we use the helpful Litmus::DBTools 
# module.
#
# NOTE: anything changed here must also be added to schema.pl for new 
# installations 
use Litmus::DBTools;
my $dbtool = Litmus::DBTools->new($dbh);

# ZLL: Authentication System fields
$dbtool->DropField("users", "is_trusted");
$dbtool->AddField("users", "bugzilla_uid", "int default '1'");
$dbtool->AddField("users", "password", "varchar(255)");
$dbtool->AddField("users", "realname", "varchar(255)");
$dbtool->AddField("users", "is_admin", "tinyint(1) default '0'");
$dbtool->AddField("users", "irc_nickname", "varchar(32)");
$dbtool->DropIndex("users", "email");
$dbtool->DropIndex("users", "irc_nickname");
$dbtool->AddUniqueKey("users","email","(email)");
$dbtool->AddUniqueKey("users","irc_nickname","(irc_nickname)");
$dbtool->AddKey("users","bugzilla_uid","(bugzilla_uid)");
$dbtool->AddKey("users","realname","(realname)");
$dbtool->AddKey("users","is_admin","(is_admin)");

# replace enums with more portable and flexible formats:
$dbtool->ChangeFieldType("products", "enabled", 'tinyint(1) default "1"');

$dbtool->DropField("test_result_logs", "log_path");
$dbtool->AddField("test_result_logs", "log_text", "longtext");
$dbtool->AddKey("test_result_logs", "log_text", "(log_text(255))");

$dbtool->AddField("test_results", "valid", "tinyint(1) default '1'");
$dbtool->AddField("test_results", "vetted", "tinyint(1) default '0'");
$dbtool->AddField("test_results", "validated_by_user_id", "int(11) default '0'");
$dbtool->AddField("test_results", "vetted_by_user_id", "int(11) default '0'");
$dbtool->AddField("test_results", "validated_timestamp", "datetime");
$dbtool->AddField("test_results", "vetted_timestamp", "datetime");
$dbtool->AddKey("test_results", "valid", "(valid)");
$dbtool->AddKey("test_results", "vetted", "(vetted)");
$dbtool->AddKey("test_results", "validated_by_user_id", "(validated_by_user_id)");
$dbtool->AddKey("test_results", "vetted_by_user_id", "(vetted_by_user_id)");
$dbtool->AddKey("test_results", "validated_timestamp", "(valid)");
$dbtool->AddKey("test_results", "vetted_timestamp", "(vetted)");
$dbtool->DropField("test_results", "validity_id");
$dbtool->DropField("test_results", "vetting_status_id");
$dbtool->DropTable("validity_lookup");
$dbtool->DropTable("vetting_status_lookup");

$dbtool->RenameTable("test_groups","testgroups");
$dbtool->AddField("testgroups", "enabled", "tinyint(1) NO NULL default '1'");
$dbtool->AddKey("testgroups","enabled","(enabled)");
$dbtool->DropField("testgroups", "obsolete");
$dbtool->DropField("testgroups", "expiration_days");

$dbtool->AddField("subgroups", "enabled", "tinyint(1) NOT NULL default '1'");
$dbtool->AddKey("subgroups","enabled","(enabled)");
$dbtool->AddField("subgroups", "product_id", "tinyint(4) NOT NULL");
$dbtool->AddKey("subgroups","product_id","(product_id)");
$dbtool->DropField("subgroups", "testgroup_id");

$dbtool->AddField("users", "enabled", "tinyint(1) NOT NULL default '1'");
$dbtool->AddKey("users","enabled","(enabled)");
$dbtool->DropField("users", "disabled");

$dbtool->DropTable("test_status_lookup");

# Remove reference to test_status_lookup
$dbtool->RenameTable("tests","testcases");
$dbtool->AddField("testcases", "enabled", "tinyint(1) NOT NULL default '1'");
$dbtool->AddKey("testcases","enabled","(enabled)");
$dbtool->DropIndex("testcases", "test_id");
$dbtool->RenameField("testcases", "test_id", "testcase_id");
$dbtool->AddKey("testcases","testcase_id","(testcase_id)");
$dbtool->AddKey("testcases","summary_2","(summary, steps, expected_results)");
$dbtool->ChangeFieldType("testcases", "community_enabled", 'tinyint(1) default "1"');
$dbtool->AddField("testcases", "product_id", "tinyint(4) NOT NULL");
$dbtool->AddKey("testcases","product_id","(product_id)");
$dbtool->DropField("testcases", "subgroup_id");

$dbtool->DropIndex("test_results", "test_id");
$dbtool->RenameField("test_results", "test_id", "testcase_id");
$dbtool->AddKey("test_results","testcase_id","(testcase_id)");
$dbtool->RenameField("test_results", "buildid", "build_id");
$dbtool->ChangeFieldType("test_results", "build_id", 'int(10) unsigned');
$dbtool->AddKey("test_results","build_id","(build_id)");
$dbtool->DropIndex("test_results", "result_id");
$dbtool->RenameField("test_results", "result_id", "result_status_id");
$dbtool->AddKey("test_results","result_status_id","(result_status_id)");
$dbtool->DropField("test_results", "platform_id");

$dbtool->AddField("branches", "enabled", "tinyint(1) NOT NULL default '1'");
$dbtool->AddKey("branches","enabled","(enabled)");

$dbtool->DropField("platforms", "product_id");

$dbtool->AddField("subgroup_testgroups", "sort_order", "smallint(6) NOT NULL default '1'");
$dbtool->AddKey("subgroup_testgroups","sort_order","(sort_order)");
$dbtool->AddField("testcase_subgroups", "sort_order", "smallint(6) NOT NULL default '1'");
$dbtool->AddKey("testcase_subgroups","sort_order","(sort_order)");
$dbtool->DropField("subgroups", "sort_order");
$dbtool->DropField("testcases", "sort_order");

$dbtool->AddField("users", "authtoken", "varchar(255)");
$dbtool->DropIndex("users", "key contact_info_fulltext (email, realname, irc_nickname)");
$dbtool->AddFullText("users", "key", "contact_info_fulltext (email, realname, irc_nickname)");

# zll 2006-06-15: users.irc_nickname cannot have a unique index, since 
# many users have a null nickname:
$dbtool->DropIndex("users", "irc_nickname");
$dbtool->AddKey("users", "irc_nickname", "(irc_nickname)");

# this should be a normal index, not a fulltext index
$dbtool->DropIndex("users", "key contact_info (email, realname, irc_nickname)");
$dbtool->AddKey("users", 'contact_info (email, realname, irc_nickname)', '');

# make logs have a many-to-many relationship with test_results
$dbtool->DropIndex("test_result_logs", "test_result_id");
$dbtool->DropField("test_result_logs", "test_result_id");

$dbtool->AddField("test_results", "is_automated_result", "tinyint(1) not null default '0'");

$dbtool->AddField("test_runs", "recommended", "tinyint(1) not null default '0'");
$dbtool->AddKey("test_runs", 'recommended (recommended)', '');

$dbtool->AddField("test_runs", "version", "smallint(6) not null default '1'");
$dbtool->AddKey("test_runs", 'version (version)', '');
$dbtool->AddField("test_run_testgroups", "sort_order", "smallint(6) not null default '1'");
$dbtool->AddKey("test_run_testgroups", 'sort_order (sort_order)', '');

# zll: upgrade to new-world group permission system
# do this in a separate package to avoid namespace polution if we're 
# creating the intial db
$dbtool->RenameField("users", "is_admin", "is_admin_old");
package CreateAdminGroups;
require 'Litmus/DB/SecurityGroup.pm';
Litmus::DB::SecurityGroup->import();
Litmus::DB::SecurityGroup->upgradeGroups();
$dbtool->DropField("users", "is_admin_old");

package main;

$dbtool->ChangeFieldType("test_result_bugs", "test_result_id", 'int(11) NOT NULL');
$dbtool->DropPrimaryKey('test_result_bugs');
$dbtool->AddPrimaryKey('test_result_bugs',"(test_result_id,bug_id)");

$dbtool->AddKey('security_groups', '(grouptype)', '');
$dbtool->AddKey('security_groups', 'idtype (group_id, grouptype)', '');

$dbtool->RenameField("locale_lookup", "abbrev", "locale_abbrev");

$dbtool->AddField('branches','creator_id','int(11) not null');
$dbtool->AddField('branches','last_updated','timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP');
$dbtool->AddField('branches','creation_date','timestamp not null');
$dbtool->AddKey('branches', 'creator_id (creator_id)', '');
$dbtool->AddKey('branches', 'last_updated (last_updated)', '');
$dbtool->AddKey('branches', 'creation_date (creation_date)', '');

$dbtool->AddField('opsyses','creator_id','int(11) not null');
$dbtool->AddField('opsyses','last_updated','timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP');
$dbtool->AddField('opsyses','creation_date','timestamp not null');
$dbtool->AddKey('opsyses', 'creator_id (creator_id)', '');
$dbtool->AddKey('opsyses', 'last_updated (last_updated)', '');
$dbtool->AddKey('opsyses', 'creation_date (creation_date)', '');

$dbtool->AddField('platforms','creator_id','int(11) not null');
$dbtool->AddField('platforms','last_updated','timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP');
$dbtool->AddField('platforms','creation_date','timestamp not null');
$dbtool->AddKey('platforms', 'creator_id (creator_id)', '');
$dbtool->AddKey('platforms', 'last_updated (last_updated)', '');
$dbtool->AddKey('platforms', 'creation_date (creation_date)', '');

$dbtool->AddField('products','creator_id','int(11) not null');
$dbtool->AddField('products','last_updated','timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP');
$dbtool->AddField('products','creation_date','timestamp not null');
$dbtool->AddKey('products', 'creator_id (creator_id)', '');
$dbtool->AddKey('products', 'last_updated (last_updated)', '');
$dbtool->AddKey('products', 'creation_date (creation_date)', '');

$dbtool->AddField('subgroups','creator_id','int(11) not null');
$dbtool->AddField('subgroups','last_updated','timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP');
$dbtool->AddField('subgroups','creation_date','timestamp not null');
$dbtool->AddKey('subgroups', 'creator_id (creator_id)', '');
$dbtool->AddKey('subgroups', 'last_updated (last_updated)', '');
$dbtool->AddKey('subgroups', 'creation_date (creation_date)', '');

$dbtool->ChangeFieldType("test_result_bugs", "last_updated", "timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP");
$dbtool->ChangeFieldType("test_result_comments", "last_updated", "timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP");
$dbtool->ChangeFieldType("test_result_logs", "last_updated", "timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP");
$dbtool->ChangeFieldType("test_results", "last_updated", "timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP");
$dbtool->ChangeFieldType("test_runs", "last_updated", "timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP");

$dbtool->ChangeFieldType("testcases", "last_updated", "timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP");

$dbtool->ChangeFieldType("testdays", "last_updated", "timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP");
$dbtool->AddField('testdays','creator_id','int(11) not null');
$dbtool->AddField('testdays','creation_date','timestamp not null');
$dbtool->AddKey('testdays', 'creator_id (creator_id)', '');
$dbtool->AddKey('testdays', 'creation_date (creation_date)', '');

$dbtool->AddField('testgroups','creator_id','int(11) not null');
$dbtool->AddField('testgroups','last_updated','timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP');
$dbtool->AddField('testgroups','creation_date','timestamp not null');
$dbtool->AddKey('testgroups', 'creator_id (creator_id)', '');
$dbtool->AddKey('testgroups', 'last_updated (last_updated)', '');
$dbtool->AddKey('testgroups', 'creation_date (creation_date)', '');

$dbtool->AddField('users','last_updated','timestamp DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP');
$dbtool->AddField('users','creation_date','timestamp not null');
$dbtool->AddKey('users', 'last_updated (last_updated)', '');
$dbtool->AddKey('users', 'creation_date (creation_date)', '');

$dbtool->ChangeFieldType("test_results", "build_id", "bigint(14) unsigned DEFAULT NULL");
$dbtool->ChangeFieldType("test_run_criteria", "build_id", "bigint(14) unsigned DEFAULT NULL");
$dbtool->ChangeFieldType("testdays", "build_id", "bigint(14) unsigned DEFAULT NULL");

print "Schema update complete.\n\n";

print "Done.\n";

exit;

#########################################################################
sub usage() {
  print "./populatedb.pl [-h|--help] [-r|--resetdb]\n";
}
