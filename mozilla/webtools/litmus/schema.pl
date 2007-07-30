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

# Litmus database schema

# IMPORTANT: Any changes (other than new tables) made here must also be made 
# by adding --TABLE-- upgrading code to populatedb.pl to handle upgrades from 
# previous versions of the schema.

our $table;

$table{audit_trail} =
   'user_id int(11) NOT NULL,
    action_timestamp timestamp NOT NULL default CURRENT_TIMESTAMP,
    action_type enum("INSERT","UPDATE","DELETE") NOT NULL,
    sql_log longtext character set latin1 collate latin1_bin,
    bind_values longtext character set latin1 collate latin1_bin,   
    KEY `user_id` (user_id),
    KEY `action_timestamp` (action_timestamp),
    KEY `action_type` (action_type),
    KEY `sql_log` (sql_log(255)),
    KEY `bind_values` (bind_values(255))
    ';

$table{branches} = 
   'branch_id smallint(6) not null primary key auto_increment,
    product_id tinyint(4) not null,
    name varchar(64) not null,
    detect_regexp varchar(255),
    enabled tinyint(1) default 1,
    
    index(product_id),
    index(name),
    index(detect_regexp),
    index(enabled)
';

$table{build_type_lookup} = 
	'build_type_id tinyint(4) not null primary key auto_increment,
	 name varchar(64) not null,
	 
	 index(name)';
	 
$table{exit_status_lookup} = 
	'exit_status_id tinyint(4) not null primary key auto_increment,
	 name varchar(64) not null,
	 
	 index(name)';

$table{locale_lookup} = 
	'abbrev varchar(16) not null primary key,
	 name varchar(64) not null,
	 
	 index(name)';

$table{log_type_lookup} = 
	'log_type_id tinyint(4) not null primary key auto_increment,
	 name varchar(64) not null,
	 
	 index(name)';

$table{opsyses} = 
	'opsys_id smallint(6) not null primary key auto_increment,
	 platform_id smallint(6) not null,
	 name varchar(64) not null,
	 detect_regexp varchar(255),
	 
	 index(platform_id),
	 index(name),
	 index(detect_regexp)';

$table{platform_products} = 
        'platform_id smallint(6) not null,
         product_id tinyint(4) not null,

         primary key (platform_id, product_id)';

$table{platforms} = 
	'platform_id smallint not null primary key auto_increment,
	 name varchar(64) not null,
	 detect_regexp varchar(255),
	 iconpath varchar(255),
	 
	 index(name),
	 index(detect_regexp),
	 index(iconpath)';

$table{products} = 
	'product_id tinyint not null primary key auto_increment,
	 name varchar(64) not null,
	 iconpath varchar(255),
	 enabled tinyint(1) default \'1\',
	 
	 unique key(name),
         index(iconpath),
	 index(enabled)';

$table{related_testcases} = 
        'testcase_id int(11) not null,
         related_testcase_id int(11) not null, 

         primary key (testcase_id, related_testcase_id)';

$table{sessions} =
	'session_id int(11) not null primary key auto_increment,
	 user_id int(11) not null,
	 sessioncookie varchar(255) not null,
	 expires datetime not null,
	 
	 index(user_id),
	 index(sessioncookie),
	 index(expires)';

$table{subgroup_testgroups} = 
	'subgroup_id smallint(6) not null,
         testgroup_id smallint(6) not null,
	 sort_order smallint(6) not null default "1",

         primary key(subgroup_id, testgroup_id),
	 index(sort_order)';

$table{subgroups} = 
	'subgroup_id smallint(6) not null primary key auto_increment,
	 name varchar(64) not null,
	 testrunner_group_id int(11),
         enabled tinyint(1) default "1",
         product_id tinyint(4) not null,
         branch_id smallint(6) not null,	 
	 
	 index(name),
	 index(testrunner_group_id),
         index(enabled),
         index(product_id),
         index(branch_id)';

$table{test_format_lookup} = 
	'format_id tinyint(4) not null primary key auto_increment,
	 name varchar(255) not null,
	 
	 index(name)';

$table{test_result_bugs} =
	'test_result_id int(11) not null,
	 last_updated datetime not null,
	 submission_time datetime not null,
	 user_id int(11),
	 bug_id int(11) not null,
	 
         primary key(test_result_id, bug_id),
	 index(test_result_id),
	 index(last_updated),
	 index(submission_time),
	 index(user_id)';

$table{test_result_comments} = 
	'comment_id int(11) not null primary key auto_increment,
	 test_result_id int(11) not null,
	 last_updated datetime not null,
	 submission_time datetime not null,
	 user_id int(11),
	 comment text,
	 
	 index(test_result_id),
	 index(last_updated),
	 index(submission_time),
	 index(user_id)';
	 
$table{test_result_logs} = 
	'log_id int(11) not null primary key auto_increment,
	 last_updated datetime not null,
	 submission_time datetime not null,
     log_text longtext,
	 log_type_id tinyint(4) not null default \'1\',
	 
	 index(last_updated),
	 index(submission_time),
	 index(log_type_id),
     index(log_text(255))';

$table{testresult_logs_join} = 
	'test_result_id int(11) not null,
	 log_id int(11) not null,
	 
	 primary key(test_result_id, log_id)';

	 
$table{test_result_status_lookup} = 
	'result_status_id smallint(6) not null primary key auto_increment,
	 name varchar(64) not null,
	 style varchar(255) not null,
	 class_name varchar(16),
	 
	 index(name),
	 index(style),
	 index(class_name)';
	 
$table{test_results} = 
	'testresult_id int(11) not null primary key auto_increment,
	 testcase_id int(11) not null,
	 last_updated datetime,
	 submission_time datetime,
	 user_id int(11),
	 opsys_id smallint(6),
	 branch_id smallint(6),
	 build_id int(10) unsigned,
	 user_agent varchar(255),
	 result_status_id smallint(6),
	 build_type_id tinyint(4) not null default \'1\',
	 machine_name varchar(64),
	 exit_status_id tinyint(4) not null default \'1\',
	 duration_ms int(11) unsigned,
	 talkback_id int(11) unsigned,         
	 locale_abbrev varchar(16) not null default \'en-US\',
         valid tinyint(1) not null default \'1\',
         vetted tinyint(1) not null default \'0\',
         validated_by_user_id int(11) default \'0\',
         vetted_by_user_id int(11) default \'0\',
         validated_timestamp datetime,
         vetted_timestamp datetime,
         is_automated_result tinyint(1) not null default \'0\',
	 
	 index(testcase_id),
	 index(last_updated),
	 index(submission_time),
	 index(user_id),
	 index(opsys_id),
	 index(branch_id),
	 index(build_id),
	 index(user_agent),
	 index(result_status_id),
	 index(build_type_id),
	 index(machine_name),
	 index(exit_status_id),
	 index(duration_ms),
	 index(talkback_id),
	 index(locale_abbrev),
         index(valid),
         index(vetted),
         index(validated_by_user_id),
         index(vetted_by_user_id),
         index(validated_timestamp),
         index(vetted_timestamp),
         index(is_automated_result)';

$table{test_run_criteria} = 
	'test_run_id int(11) not null,
         build_id int(10) unsigned not null,
         platform_id smallint(6) not null default "0",
         opsys_id smallint(6) not null default "0",

         primary key(test_run_id,build_id,platform_id,opsys_id)';

$table{test_run_testgroups} = 
	'test_run_id int(11) not null,
         testgroup_id smallint(6) not null,
         sort_order smallint(6) not null default "1",

         primary key(test_run_id,testgroup_id),
         index(sort_order)';

$table{test_runs} = 
	'test_run_id int(11) not null primary key auto_increment,
         name varchar(64) not null,
         description varchar(255),
         start_timestamp timestamp not null,
         finish_timestamp timestamp not null,
         enabled tinyint(1) not null default "1",
         recommended tinyint(1) not null default "0",
         product_id tinyint(4) not null,
         branch_id smallint(6) not null,
         creation_date timestamp not null,
         last_updated timestamp not null,
         author_id int(11) not null,
         version smallint(6) not null default "1",

         index(name),
         index(description),
         index(start_timestamp),
         index(finish_timestamp),
         index(enabled),
         index(recommended),
         index(product_id),
         index(branch_id),
         index(creation_date),
         index(last_updated),
         index(author_id),
         index(version)';


$table{testcase_subgroups} =
        'testcase_id int(11) not null,
         subgroup_id smallint(6) not null,
	 sort_order smallint(6) not null default "1",

         primary key(testcase_id, subgroup_id),
	 index(sort_order)';

$table{testcases} = 
	'testcase_id int(11) not null primary key auto_increment,
	 summary varchar(255) not null,
	 details text,
	 enabled tinyint(1) not null default \'1\',
	 community_enabled tinyint(1) default \'1\',
	 format_id tinyint(4) not null default \'1\',
	 regression_bug_id int(11),
	 steps longtext,
	 expected_results longtext,
	 author_id int(11) not null,
	 creation_date datetime not null,
	 last_updated datetime not null,
	 version smallint(6) not null default \'1\',
	 testrunner_case_id int(11),
	 testrunner_case_version int(11),
         product_id tinyint(4) not null,
         branch_id smallint(6) not null,	 

	 index(summary),
	 index(enabled),
	 index(community_enabled),
         index(format_id),
	 index(regression_bug_id),
	 index(steps(255)),
	 index(expected_results(255)),
	 index(author_id),
	 index(creation_date),
	 index(last_updated),
	 index(testrunner_case_id),
         index(testrunner_case_version),
         index(product_id),
         index(branch_id),
         fulltext index text_search (summary,steps,expected_results)';

$table{testdays} = 
        'testday_id smallint(6) not null primary key auto_increment,
         last_updated timestamp(14),
         start_timestamp timestamp(14) not null,
         finish_timestamp timestamp(14) not null,
         description varchar(255) not null,
	 product_id tinyint(4),
	 testgroup_id smallint(6),
	 build_id int(10) unsigned,         
         branch_id smallint(6),
	 locale_abbrev varchar(16),
         
         index(start_timestamp),
         index(finish_timestamp),
         index(description),
         index(product_id),
         index(testgroup_id),
         index(build_id),
         index(branch_id),
         index(locale_abbrev)';

$table{testgroups} = 
	'testgroup_id smallint(6) not null primary key auto_increment,
	 product_id tinyint(4) not null,
	 name varchar(64) not null,
	 enabled tinyint(1) default "1",
	 testrunner_plan_id int(11),
         branch_id smallint(6) not null,	 

	 index(product_id),
	 index(name),
	 index(enabled),
	 index(testrunner_plan_id),
         index(branch_id)';
	 
$table{users} = 
	'user_id int(11) not null primary key auto_increment,
	 bugzilla_uid int,
	 email varchar(255) not null,
	 password varchar(255),
	 realname varchar(255),
	 irc_nickname varchar(32),
	 enabled tinyint(1),
	 is_admin tinyint(1),
	 authtoken varchar(255),
	 
	 index(bugzilla_uid),
	 unique index(email),
         index(irc_nickname),
         index(password),
         index(realname),
         index(enabled),
	 index(is_admin),
	 index contact_info (email, realname, irc_nickname),
	 fulltext index contact_info_fulltext (email, realname, irc_nickname)';
	 
$table{security_groups} = 
	'group_id mediumint not null primary key auto_increment,
	 name varchar(255) not null,
	 description varchar(255) not null,
	 grouptype tinyint not null,
	 isactive tinyint not null default 1,
	 
	 index(grouptype),
	 index idtype (group_id, grouptype),
	 unique index(name)';
	 
$table{user_group_map} =
	'user_id int(11) not null,
	 group_id mediumint not null,
	 
	 unique(user_id, group_id)';

$table{group_product_map} = 
	'group_id mediumint not null,
	 product_id tinyint(4) not null,
	 
	 unique(group_id, product_id)';
	 
$table{password_resets} =  
	'user_id int(11) not null,
	 session_id int(11) not null,
	 
	 index(user_id, session_id)';
