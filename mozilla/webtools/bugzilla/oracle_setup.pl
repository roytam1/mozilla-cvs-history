#!/usr/bonsaitools/bin/perl -w
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
#				  David Lawrence <dkl@redhat.com>
#
#
# Direct any questions on this source code to
#
# Holger Schurig <holgerschurig@nikocity.de>
#
#
#
# Hey, what's this?
#
# 'checksetup.pl' is a script that is supposed to run during installation
# time and also after every upgrade.
#
# The goal of this script is to make the installation even more easy.
# It does so by doing things for you as well as testing for problems
# early.
#
# And you can re-run it whenever you want. Especially after Bugzilla
# get's updated you SHOULD rerun it. Because then it may update your
# SQL table definitions so that they are again in sync with the code.
#
# So, currently this module does:
#
#     - check for required perl modules
#     - set defaults for local configuration variables
#     - create and populate the data directory after installation
#     - set the proper rights for the *.cgi, *.html ... etc files
#     - check if the code can access Oracle 
#     - creates the database 'bugs' if the database does not exist
#     - creates the tables inside the database if they don't exist
#     - automatically changes the table definitions of older BugZilla
#       installations
#     - populates the groups
#     - put the first user into all groups so that the system can
#       be administrated
#     - changes already existing SQL tables if you change your local
#       settings, e.g. when you add a new platform
#
# People that install this module locally are not supposed to modify
# this script. This is done by shifting the user settable stuff intp
# a local configuration file 'localconfig'. When this file get's
# changed and 'checkconfig.pl' will be re-run, then the user changes
# will be reflected back into the database.
#
# Developers however have to modify this file at various places. To
# make this easier, I have added some special comments that one can
# search for.
#
#     To                                               Search for
#
#     add/delete local configuration variables         --LOCAL--
#     check for more prerequired modules               --MODULES--
#     change the defaults for local configuration vars --LOCAL--
#     update the assigned file permissions             --CHMOD--
#     add more ORACLE-related checks                   --ORACLE--
#     change table definitions                         --TABLE--
#     add more groups                                  --GROUPS--
#
# Note: sometimes those special comments occur more then once. For
# example, --LOCAL-- is at least 3 times in this code!  --TABLE--
# also is used more than once. So search for every occurence!
#


###########################################################################
# Global definitions
###########################################################################

use diagnostics;
use strict;



#
# This are the --LOCAL-- variables defined in 'localconfig'
# 

use vars qw(
    $webservergroup
    $db_host $db_port $db_home $db_name $db_user $db_pass $db_check
    @bug_severity @priority @op_sys @rep_platform @resolution @bug_status
);


# Trim whitespace from front and back.

sub trim {
    ($_) = (@_);
    s/^\s+//g;
    s/\s+$//g;
    return $_;
}

# Change this to 1 if you need debugging
my $debug = 0;

###########################################################################
# Check required module
###########################################################################

#
# Here we check for --MODULES--
#

print "Checking perl modules ...\n";
unless (eval "require 5.004") {
    die "Sorry, you need at least Perl 5.004\n";
}

unless (eval "require DBI") {
    die "Please install the DBI module. You can do this by running (as root)\n\n",
        "       perl -MCPAN -eshell\n",
        "       install DBI\n";
}

unless (eval "require Data::Dumper") {
    die "Please install the Data::Dumper module. You can do this by running (as root)\n\n",
        "       perl -MCPAN -eshell\n",
        "       install Data::Dumper\n";
}

unless (eval "require DBD::Oracle") {
    die "Please install the Oracle database driver. You can do this by running (as root)\n\n",
        "       perl -MCPAN -eshell\n",
        "       install DBD::Oracle\n\n";
}

unless (eval "require Date::Parse") {
    die "Please install the Date::Parse module. You can do this by running (as root)\n\n",
        "       perl -MCPAN -eshell\n",
        "       install Date::Parse\n";
}

# The following two modules are optional:
my $charts = 0;
$charts++ if eval "require GD";
$charts++ if eval "require Chart::Base";
if ($charts != 2) {
    print "If you you want to see graphical bug dependency charts, you may install\n",
    "the optional libgd and the Perl modules GD and Chart::Base, e.g. by\n",
    "running (as root)\n\n",
    "   perl -MCPAN -eshell\n",
    "   install GD\n",
    "   install Chart::Base\n";
}





###########################################################################
# Check and update local configuration
###########################################################################

#
# This is quite tricky. But fun!
#
# First we read the file 'localconfig'. Then we check if the variables we
# need are defined. If not, localconfig will be amended by the new settings
# and the user informed to check this. The program then stops.
#
# Why do it this way around?
#
# Assume we will enhance Bugzilla and eventually more local configuration
# stuff arises on the horizon.
#
# But the file 'localconfig' is not in the Bugzilla CVS or tarfile. You
# know, we never want to overwrite your own version of 'localconfig', so
# we can't put it into the CVS/tarfile, can we?
#
# Now, we need a new variable. We simply add the necessary stuff to checksetup.
# The user get's the new version of Bugzilla from the CVS, runs checksetup
# and checksetup finds out "Oh, there is something new". Then it adds some
# default value to the user's local setup and informs the user to check that
# to see if that is what the user wants.
#
# Cute, ey?
#

print "Checking user setup ...\n";
do 'localconfig';
my $newstuff = "";
sub LocalVar ($$)
{
    my ($name, $definition) = @_;

    # Is there a cleaner way to test if the variable defined in scalar $name
    # is defined or not?
    my $defined = 0;
    $_ = "\$defined = 1 if defined $name;";
    eval $_;
    return if $defined;

    $newstuff .= " " . $name;
    open FILE, '>>localconfig';
    print FILE $definition, "\n\n";
    close FILE;
}



#
# Set up the defaults for the --LOCAL-- variables below:
#

    
LocalVar('$webservergroup', '
#
# This is the group your web server runs on.
# If you have a windows box, ignore this setting.
# If you do not wish for checksetup to adjust the permissions of anything,
# set this to "".
# If you set this to anything besides "", you will need to run checksetup.pl
# as root.
$webservergroup = "nobody";
');



LocalVar('$db_name', '
#
# Name of the Oracle database ...
#
$db_name = "rheng";        
');


LocalVar('$db_user', '
#
# Username for the Oracle database ...
#
$db_user = "bugzilla/bugzilla";
');


LocalVar('$db_pass', '
#
# Password for the database ...
#
$db_pass = "";
');


LocalVar('$db_home', '
#
# The path to Oracles home directory ...
#
$db_home = "/opt/apps/oracle/product/8.0.5/";
');


LocalVar('@bug_severity', '
#
# Which bug and feature-request severities do you want?
#
@bug_severity = (
        "security",
        "high",
        "normal",
        "low",
        "enhancement"
);
');



LocalVar('@priority', '
#
# Which priorities do you want to assign to bugs and feature-request?
#
@priority = (
        "high",
        "normal",
        "low",
        "contract"
);
');



LocalVar('@op_sys', '
#
# What operatings systems may your products run on?
#
@op_sys = (
        "All",
        "Windows 3.1",
        "Windows 95",
        "Windows 98",
		"Windows 2000",
        "Windows NT",
        "Mac System 7",
        "Mac System 7.5",
        "Mac System 7.6.1",
        "Mac System 8.0",
        "Mac System 8.5",
        "Mac System 8.6",
		"Mac System 9.0",
        "AIX",
        "BSDI",
        "HP-UX",
        "IRIX",
        "Linux",
        "FreeBSD",
        "OSF/1",
        "Solaris",
        "SunOS",
        "Neutrino",
        "OS/2",
        "BeOS",
        "OpenVMS",
        "other"
);
');



LocalVar('@rep_platform', '
#
# What hardware platforms may your products run on?
#
@rep_platform = (
        "All",
        "i386",
        "sparc",
        "alpha",
		"noarch"
);
');



LocalVar('@bug_status', '
#
# What bug status entries would you like to have?
#
@bug_status = (
        "NEW",
        "VERIFIED",
        "ASSIGNED",
        "REOPENED",
		"RESOLVED",
		"CLOSED"
);
');


LocalVar('@resolution', '
#
# What resolution states would you like to have?
#
@resolution = (
		" ",
        "NOTABUG",
        "WONTFIX",
        "DEFERRED",
        "WORKSFORME",
        "CURRENTRELEASE",
        "RAWHIDE",
		"ERRATA",
		"DUPLICATE"
);
');


if ($newstuff ne "") {
    print "\nThis version of Bugzilla contains some variables that you may \n",
          "to change and adapt to your local settings. Please edit the file\n",
          "'localconfig' and rerun checksetup.pl\n\n",
          "The following variables are new to localconfig since you last ran\n",
          "checksetup.pl:  $newstuff\n\n";
    exit;
}



###########################################################################
# Check data directory
###########################################################################

#
# Create initial --DATA-- directory and make the initial empty files there:
#

unless (-d 'data') {
    print "Creating data directory ...\n";
    mkdir 'data', 0777;
    if ($webservergroup eq "") {
    	chmod 0777, 'data';
    }
    open FILE, '>>data/comments'; close FILE;
    open FILE, '>>data/nomail'; close FILE;
    open FILE, '>>data/mail'; close FILE;
    chmod 0666, glob('data/*');
}


# Just to be sure ...
unlink "data/versioncache";



###########################################################################
# Set proper rights
###########################################################################

#
# Here we use --CHMOD-- and friends to set the file permissions
#
# The rationale is that the web server generally runs as nobody and so the cgi
# scripts should not be writable for nobody, otherwise someone may be possible
# to change the cgi's when exploiting some security flaw somewhere (not
# necessarily in Bugzilla!)
#
# Also, some *.pl files are executable, some are not.
#
# +++ Can anybody tell me what a Windows Perl would do with this code?
#

if ($webservergroup) {
    mkdir 'shadow', 0770 unless -d 'shadow';
    # Funny! getgrname returns the GID if fed with NAME ...
    my $webservergid = getgrnam($webservergroup);
    chown 0, $webservergid, glob('*');
    chmod 0640, glob('*');

    chmod 0750, glob('*.cgi'),
                'processmail',
                'whineatnews.pl',
                'collectstats.pl',
                'checksetup.pl',
				'oracle_setup.pl';

    chmod 0770, 'data', 'shadow';
    chmod 0666, glob('data/*');
}





###########################################################################
# Check Oracle setup
###########################################################################

#
# Check if we have access to --ORACLE--
#

# This settings are not yet changeable, because other code depends on
# the fact that we use Oracle and not, say, PostgreSQL.

my $db_base = 'Oracle';

use DBI;

# get a handle to the low-level DBD driver
my $drh = DBI->install_driver($db_base)
    or die "Can't connect to the $db_base. Is the database installed and up and running?\n";

my $db_name = "rheng";
my $db_user = "bugzilla/bugzilla";
my $db_pass = "";
my $db_home = "/opt/apps/oracle/product/8.0.5/";

# do 'localconfig';

my $connectstring = "dbi:$db_base:$db_name";

# database setup 
# Connect to Oracle Database with Oracle environment variables;
$ENV{'ORACLE_HOME'} = $db_home;
$ENV{'ORACLE_SID'} = $db_name;
$ENV{'TWO_TASK'} = $db_name;
$ENV{'ORACLE_USERID'} = $db_user;

# now get a handle to the database:
my $dbh = DBI->connect($connectstring, $db_user, $db_pass, { RaiseError => 1 })
    or die "Can't connect to the table '$connectstring'.\n",
           "Have you read Bugzilla's README?  Have you read the doc of '$db_name'?\n";

$dbh->{LongReadLen} = 1000 * 1024; # large object
$dbh->{LongTruncOk} = 0;           # do not truncate
$dbh->{AutoCommit} = 1;			   # commit all changes immediately

print "\n\nOracle database connection successful.\n";

END { $dbh->disconnect if $dbh }


###########################################################################
# Clean tables setup
###########################################################################
# If we were called with the argument clean then we need to create empty
# tables to allow for data migration from another database. This way there
# are no conflicts with primary keys, unique values, etc.

my $clean = 0;

if ($#ARGV >= 0) {
	if ($ARGV[0] eq 'clean') {
		$clean = 1; # clean tables
		print "Creating empty tables.\n";
	} else {
		goto $ARGV[0];
#		print "\nUsage: oracle_setup.pl <option>\noptions: clean - create empty tables\n";
#		exit(0);
	}
}

###########################################################################
# Table definitions
###########################################################################

#
# The following hash stores all --TABLE-- definitions. This will be used
# to automatically create those tables that don't exist. The code is
# safer than the make*.sql shell scripts used to be, because they won't
# delete existing tables.
#
# If you want intentionally do this, yon can always drop a table and re-run
# checksetup, e.g. like this:
#
#    $ sqlplus bugzilla/bugzilla 
#    sqlplus> drop table votes;
#    sqlplus> exit;
#    $ ./checksetup.pl
#
# If you change one of those field definitions, then also go below to the
# next occurence of the string --TABLE-- (near the end of this file) to
# add the code that updates older installations automatically.
#



my %table;

$table{bugs_activity} = 
   'bug_id 		INTEGER 		CONSTRAINT ACT_NN_BUGID not null,
    who 		INTEGER 		CONSTRAINT ACT_NN_WHO 	not null,
    bug_when 	DATE 			CONSTRAINT ACT_NN_WHEN 	not null,
    fieldid 	VARCHAR2(64) 	CONSTRAINT ACT_NN_FIELD not null,
    oldvalue 	VARCHAR2(400),
    newvalue 	VARCHAR2(400)';


$table{attachments} =
   "attach_id       INTEGER         CONSTRAINT ATTACH_PK_ATTACHID 	PRIMARY KEY not null,
    bug_id          INTEGER         CONSTRAINT ATTACH_NN_BUGID 		not null,
    creation_ts 	DATE            CONSTRAINT ATTACH_NN_CREATION 	not null,
    description 	VARCHAR2(2000)  CONSTRAINT ATTACH_NN_DESC 		not null,
    mimetype 		VARCHAR2(255)   CONSTRAINT ATTACH_NN_MIME       not null,
    ispatch 		INTEGER,
    filename 		VARCHAR2(255)   CONSTRAINT ATTACH_NN_FILE       not null,
    thedata 		BLOB        	CONSTRAINT ATTACH_NN_DATA       not null,
    submitter_id 	INTEGER         CONSTRAINT ATTACH_NN_SUBMIT    	not null";

# fields to add later due to foreign keys: bug_status, bug_severity, op_sys, 
# resolution, priority, rep_platform, target_milestone.
$table{bugs} =
   "bug_id 				INTEGER     	CONSTRAINT BUGS_PK_BUGID PRIMARY KEY not null,
    groupset 			INTEGER     	DEFAULT(0) not null,
    assigned_to 		INTEGER     	CONSTRAINT BUGS_NN_ASSITO not null, 
   	bug_file_loc 		VARCHAR2(255)   DEFAULT(''),
    creation_ts         DATE        	CONSTRAINT BUGS_NN_CRTETS not null,
    delta_ts 			DATE			DEFAULT(sysdate),
    short_desc 			VARCHAR2(4000)  CONSTRAINT BUGS_NN_SHORT not null,
    product 			VARCHAR2(256)   CONSTRAINT BUGS_NN_PRODCT not null,
    reporter 			INTEGER     	CONSTRAINT BUGS_NN_REPRTR  not null,
    version 			VARCHAR2(64)    CONSTRAINT BUGS_NN_VERSN  not null,
    component 			VARCHAR2(64)    CONSTRAINT BUGS_NN_COMPNT not null,
    qa_contact 			INTEGER		   	,
    status_whiteboard 	VARCHAR2(4000)  DEFAULT(''),
    votes 				INTEGER     	DEFAULT(''),
    keywords 			VARCHAR2(255)   DEFAULT(''), 
	target_milestone    VARCHAR2(64)	DEFAULT(''),
    lastdiffed 			DATE,
	bug_view			INTEGER			DEFAULT(0),
	long_desc			LONG";


$table{cc} =
   'bug_id 	INTEGER 	CONSTRAINT CC_NN_BUGID 	not null,
    who 	INTEGER 	CONSTRAINT CC_NN_WHO  	not null';


$table{longdescs} = 
   'bug_id 		INTEGER     CONSTRAINT LONG_NN_BUGID	not null,
    who 		INTEGER     CONSTRAINT LONG_NN_WHO   	not null,
    bug_when 	DATE        CONSTRAINT LONG_NN_WHEN   	not null,
    thetext 	LONG		CONSTRAINT LONG_NN_TEXT 	not null';


$table{components} =
   "value 				VARCHAR2(255)   CONSTRAINT COMP_NN_VALUE  not null,
    program 			VARCHAR2(255)   CONSTRAINT COMP_NN_PROGRM not null,
    initialowner 		VARCHAR2(64)    CONSTRAINT COMP_NN_INTOWN  not null, 
	devowner			VARCHAR2(64)    DEFAULT(''),
    initialqacontact 	VARCHAR2(64) 	DEFAULT(''), 
    description 		VARCHAR2(2000) 	DEFAULT('')";


$table{dependencies} =
   'blocked 	INTEGER CONSTRAINT DEPEND_NN_BLOCKED  not null,
    dependson 	INTEGER CONSTRAINT DEPEND_NN_DPNDSON  not null';


# Group bits must be a power of two. Groups are identified by a bit; sets of
# groups are indicated by or-ing these values together.
#
# isbuggroup is nonzero if this is a group that controls access to a set
# of bugs.  In otherword, the groupset field in the bugs table should only
# have this group's bit set if isbuggroup is nonzero.
#
# User regexp is which email addresses are initially put into this group.
# This is only used when an email account is created; otherwise, profiles
# may be individually tweaked to add them in and out of groups.

$table{groups} =
   "bit 		INTEGER 		DEFAULT(0) not null,
	groupid     INTEGER     	CONSTRAINT GROUPS_PK_GROUPID    PRIMARY KEY not null,
    name  		VARCHAR2(255)   CONSTRAINT GROUPS_NN_NAME   not null,
    description VARCHAR2(2000)  CONSTRAINT GROUPS_NN_DESC  not null,
    isbuggroup 	INTEGER     	DEFAULT(0) not null,
    userregexp 	VARCHAR2(255)	DEFAULT('')";


$table{logincookies} =
   'cookie  		INTEGER       CONSTRAINT LOGIN_PK_COOKIE PRIMARY KEY,
    userid 			INTEGER       CONSTRAINT LOGIN_NN_USERID not null,
    cryptpassword 	VARCHAR2(64),
    hostname 		VARCHAR2(128),
    lastused 		DATE DEFAULT(sysdate)';


$table{products} =
   "product 		VARCHAR2(255)   CONSTRAINT PRODUCT_NN_PRODUCT   not null,
    description 	VARCHAR2(2000)  CONSTRAINT PRODUCT_NN_DESC      not null,
    milestoneurl 	VARCHAR2(255)   DEFAULT(''), 
    disallownew  	INTEGER 		DEFAULT(0) not null,
    votesperuser 	INTEGER 		DEFAULT(0) not null,
	id				INTEGER			CONSTRAINT PRODUCT_PK_ID	PRIMARY KEY,
	votestoconfirm  INTEGER			DEFAULT(10000) not null,
	defaultmilestone VARCHAR2(20)   DEFAULT('---') not null";


$table{profiles} =
   "userid 				INTEGER     	CONSTRAINT PROFILE_PK_USRID  PRIMARY KEY,
    login_name 			VARCHAR2(255)   CONSTRAINT PROFILE_NN_LOGIN  not null,
    password 			VARCHAR2(16)    CONSTRAINT PROFILE_NN_PASSWD not null,
    cryptpassword 		VARCHAR2(64)    CONSTRAINT PROFILE_NN_CRYPT  not null,
    realname 			VARCHAR2(255)   DEFAULT(''),
    groupset        	INTEGER     	DEFAULT(0) not null,
	groupid				INTEGER			DEFAULT(0) not null,
    disabledtext 		VARCHAR2(255)   DEFAULT(''),
    newemailtech 		INTEGER     	DEFAULT(0) not null,
    mybugslink  		INTEGER     	DEFAULT(1) not null";


$table{namedqueries} =
    "userid 		INTEGER     	CONSTRAINT NAMED_NN_USRID not null,
     name 			VARCHAR2(255)   CONSTRAINT NAMED_NN_NAME  not null,
     watchfordiffs 	INTEGER 		DEFAULT(0) not null,
     linkinfooter 	INTEGER 		DEFAULT(0) not null,
     query 			LONG     		CONSTRAINT NAMED_NN_QUERY not null";


# This isn't quite cooked yet...
#
#  $table{diffprefs} =
#     'userid mediumint not null,
#      fieldid mediumint not null,
#      mailhead tinyint not null,
#      maildiffs tinyint not null,
#
#      index(userid)';

$table{fielddefs} =
   "fieldid 	INTEGER     	CONSTRAINT FIELDDEF_PK_ID   PRIMARY KEY not null,
    name  		VARCHAR2(64)    CONSTRAINT FIELDDEF_NN_NAME not null,
    description VARCHAR2(255)   CONSTRAINT FIELDDEF_NN_DESC not null,
    mailhead 	INTEGER     	DEFAULT (0) not null,
    sortkey 	INTEGER     	CONSTRAINT FIELDDEF_NN_SORT not null";


$table{versions} =
   "value 	VARCHAR2(255)   CONSTRAINT VERS_NN_VALUE    not null,
    program VARCHAR2(255)   CONSTRAINT VERS_NN_PROGM    not null";


$table{votes} =
   "who 	INTEGER CONSTRAINT VOTE_NN_WHO  	not null,
    bug_id 	INTEGER CONSTRAINT VOTE_NN_BUGID 	not null,
    count 	INTEGER CONSTRAINT VOTE_NN_COUNT  	not null";


$table{keywords} =
    "bug_id 	INTEGER     CONSTRAINT KEYWORDS_NN_BUGID    not null,
     keywordid 	INTEGER     CONSTRAINT KEYWORDS_NN_KEYID   	not null";


$table{keyworddefs} =
    "id 			INTEGER         CONSTRAINT KEYDEF_PK_ID    PRIMARY KEY not null,
     name 			VARCHAR2(64)    CONSTRAINT KEYDEF_NN_NAME  not null,
     description 	VARCHAR2(2000)";


$table{bug_status} = 
	"value	VARCHAR(255)    CONSTRAINT STATUS_PK_VALUE PRIMARY KEY not null";


$table{emailnotification} =
	"value   VARCHAR(255)    CONSTRAINT EMAIL_PK_VALUE PRIMARY KEY NOT NULL";


$table{errata} = 
	"revision	INTEGER     	DEFAULT(0) not null,
    type        VARCHAR2(10)    CONSTRAINT ERRATA_NN_TYPE   NOT NULL,
    issue_date  DATE        	CONSTRAINT ERRATA_NN_ISSUE  NOT NULL,
    updated_on  DATE        	DEFAULT(sysdate),
    id          INTEGER     	CONSTRAINT ERRATA_PK_ID     PRIMARY KEY NOT NULL,
    synopsis    VARCHAR2(2000)  CONSTRAINT ERRATA_NN_SYNOP  NOT NULL,
    mail        INTEGER     	DEFAULT(0) not null,
    files       INTEGER     	DEFAULT(0) not null";


$table{news} =
	"id         INTEGER         CONSTRAINT NEWS_PK_ID   PRIMARY KEY NOT NULL,
    add_date    DATE            CONSTRAINT NEWS_NN_DATE NOT NULL,
    headline    VARCHAR2(2000)  CONSTRAINT NEWS_NN_HEAD NOT NULL,
    story       LONG            CONSTRAINT NEWS_NN_STORY NOT NULL";


$table{op_sys} =
	"value   VARCHAR2(255)   CONSTRAINT OPSYS_PK_VALUE PRIMARY KEY NOT NULL";


$table{priority} = 
	"value   VARCHAR(255)    CONSTRAINT PRIORITY_PK_VALUE PRIMARY KEY NOT NULL";


$table{product_group} =
	"productid       INTEGER         CONSTRAINT PRODGROUP_NN_PROD      NOT NULL,
     groupid         INTEGER         CONSTRAINT PRODGROUP_NN_GROUP     NOT NULL";


$table{user_group} = 
	"userid      INTEGER         CONSTRAINT USERGROUP_NN_USER       NOT NULL,
     groupid     INTEGER         CONSTRAINT USERGROUP_NN_GROUP      NOT NULL";


$table{bug_group} = 
	"bugid       INTEGER         CONSTRAINT USERGROUP_NN_BUGID       NOT NULL,
     groupid     INTEGER         CONSTRAINT USERGROUP_NN_GROUPID     NOT NULL";


$table{rep_platform} =
	"value   VARCHAR(255)    CONSTRAINT PLATFM_PK_VALUE PRIMARY KEY NOT NULL";


$table{resolution} = 
	"value   VARCHAR2(255) CONSTRAINT RESOL_PK_VALUE PRIMARY KEY";


$table{bug_severity} = 
	"value   VARCHAR(255)    CONSTRAINT SEVERE_PK_VALUE  PRIMARY KEY NOT NULL";

$table{milestones} =
	"value       VARCHAR2(20)    CONSTRAINT MILE_NN_VALUE    NOT NULL,
     product     VARCHAR(64)     CONSTRAINT MILE_NN_PRODUCT  NOT NULL,
     sortkey     INTEGER         CONSTRAINT MILE_NN_SORTKEY  NOT NULL";


# necessary sequences for incrementing id numbers
my %sequence;
$sequence{groups} 		= 'create sequence groups_seq 		NOCACHE START WITH 1 INCREMENT BY 1';
$sequence{attachments} 	= 'create sequence attachments_seq 	NOCACHE START WITH 1 INCREMENT BY 1';
$sequence{bugs} 		= 'create sequence bugs_seq 		NOCACHE START WITH 1 INCREMENT BY 1';
$sequence{errata} 		= 'create sequence errata_seq 		NOCACHE START WITH 1 INCREMENT BY 1';
$sequence{fielddefs} 	= 'create sequence fielddefs_seq 	NOCACHE START WITH 1 INCREMENT BY 1';
$sequence{logincookies} = 'create sequence logincookies_seq NOCACHE START WITH 1 INCREMENT BY 1';
$sequence{news} 		= 'create sequence news_seq 		NOCACHE START WITH 1 INCREMENT BY 1';
$sequence{products} 	= 'create sequence products_seq 	NOCACHE START WITH 1 INCREMENT BY 1';
$sequence{profiles} 	= 'create sequence profiles_seq 	NOCACHE START WITH 1 INCREMENT BY 1';

# indexes for various tables
my %index;
$index{bugs} 			= "assigned_to, 
						   creation_ts,
                		   delta_ts,
                		   bug_severity,
                		   bug_status,
                		   op_sys,
                		   priority,
                		   product,
                		   reporter,
                		   version,
                		   component,
                		   resolution,
                		   target_milestone,
                		   qa_contact,
						   votes";

$index{cc} 				= "bug_id,
			  			   who";

$index{dependencies} 	= "blocked,
						   dependson";

$index{fielddefs} 		= "sortkey";

$index{keywords} 		= "bug_id,
						   keywordid";

$index{logincookies} 	= "lastused";

$index{bugs_activity} 	= "bug_id,
    					   bug_when,
    					   fieldid";

$index{attachments} 	= "bug_id,
    				   	   creation_ts";

$index{longdescs} 		= "bug_id,
    				 	   bug_when";

$index{namedqueries} 	= "watchfordiffs";

$index{votes} 			= "who,
    			 		   bug_id";

$index{keywords} 		= "bug_id,
			     		   keywordid";


###########################################################################
# Create tables
###########################################################################

# Learn about current tables in the Oracle Bugzilla database
my @tables;
my $sth = $dbh->prepare("select distinct lower(table_name) from user_tab_columns");
$sth->execute();
while (my @row = $sth->fetchrow_array()) {
	push (@tables, $row[0]);
}


# go throught our %table hash and create missing tables
# while (my ($tabname, $fielddef) = each %table) {
foreach my $tabname (keys %table) {
    next if grep /^$tabname$/, @tables;
    print "Creating table $tabname ...\n";
	my $query = "CREATE TABLE $tabname ($table{$tabname})";
	print "$query\n" if $debug;
	$dbh->do($query)
        or die "Could not create table '$tabname'";
	
    # add lines here if you add more --LOCAL-- config vars that end up in
    # the enums:
    if ($tabname eq 'resolution') {
        foreach my $value (@resolution) {
            $dbh->do("insert into $tabname values ('$value')") unless ($clean);
        }
	}
	if ($tabname eq 'bug_status') {
        foreach my $value (@bug_status) {
            $dbh->do("insert into $tabname values ('$value')") unless ($clean);
        }
    }
	if ($tabname eq 'bug_severity') {
        foreach my $value (@bug_severity) {
            $dbh->do("insert into $tabname values ('$value')") unless ($clean);
        }
    }
	if ($tabname eq 'priority') {
        foreach my $value (@priority) {
            $dbh->do("insert into $tabname values ('$value')") unless ($clean);
        }
    }
	if ($tabname eq 'emailnotification') {
		foreach my $value ("ExcludeSelfChanges", "CConly", "All") {
			$dbh->do("insert into $tabname values ('$value')") unless ($clean);
        }
    }
	if ($tabname eq 'op_sys') {
		foreach my $value (@op_sys) {
			$dbh->do("insert into $tabname values ('$value')") unless ($clean);
		}
    }
	if ($tabname eq 'rep_platform') {
		foreach my $value (@rep_platform) {
			$dbh->do("insert into $tabname values ('$value')") unless ($clean);
		}
    }
}

# 2000-04-05 Add necessary foreign keys to specific tables for enum table types
# These fields were left out of the table defs since they need to be created after the table that
# the field refers to is created. There must be a way to force Oracle to allow the field to be created anyway
# but so far have not been able to find one.

&AddField('bugs', 'bug_status', 'VARCHAR2(64)');
&AddField('bugs', 'bug_severity', 'VARCHAR2(64)');
&AddField('bugs', 'priority', 'VARCHAR2(64)');
&AddField('bugs', 'rep_platform', 'VARCHAR2(64)');
&AddField('bugs', 'resolution', 'VARCHAR2(64) DEFAULT(\'\')');
&AddField('bugs', 'op_sys', 'VARCHAR2(64)');
&AddField('bugs', 'target_milestone', 'VARCHAR2(64)');
&AddField('profiles', 'emailnotification', 'VARCHAR2(64)');

# Create necessary indexes for tables that need them
#foreach my $table (keys %table) {
#	if (exists ($index{$table})) {
#		&CheckIndex($table);
#	}
#}

# Create necessary sequences for tables that need them
#foreach my $table (keys %sequence) {
#    if (exists ($sequence{$table})) {
#        &CheckSequence($table);
#    }
#}

###########################################################################
# Populate groups table
###########################################################################

#
# This subroutine checks if a group exist. If not, it will be automatically
# created with the next available bit set
#

sub AddGroup ($$;$$) {
    my ($name, $desc, $isbuggroup, $contract) = @_;
	my @row;

    # does the group exist?
    my $sth = $dbh->prepare("SELECT name FROM groups WHERE name='$name'");
    $sth->execute;
	my $ref = $sth->fetchrow_arrayref();
	return if $ref;
    
    # get highest bit number
    $sth = $dbh->prepare("SELECT bit FROM groups ORDER BY bit DESC");
    $sth->execute;
    @row = $sth->fetchrow_array;

    # normalize bits
    my $bit;
    if (defined $row[0]) {
        $bit = $row[0] << 1;
    } else {
        $bit = 1;
    }

	# If this is a bug group set to 1 else set to 0
	$isbuggroup = defined($isbuggroup) && $isbuggroup == 1 ? $isbuggroup : 0;
	# do the same for contract
	$contract = defined($contract) && $contract == 1 ? $contract : 0;

    print "Adding group $name ...\n";
	my $query = "INSERT INTO groups
                          (groupid, bit, name, description, userregexp, isbuggroup, contract)
                          VALUES (groups_seq.nextval, ?, ?, ?, ?, ?, ?)";
	print "$query\n" if $debug;
    $sth = $dbh->prepare($query);
    $sth->execute($bit, $name, $desc, "", $isbuggroup, $contract);
}


#
# BugZilla uses --GROUPS-- to assign various rights to it's users. 
#

unless ($clean) {
	AddGroup 'tweakparams',      'Can tweak operating parameters';
	AddGroup 'editgroupmembers', 'Can put people in and out of groups that they are members of.';
	AddGroup 'creategroups',     'Can create and destroy groups.';
	AddGroup 'editcomponents',   'Can create, destroy, and edit components.';
	AddGroup 'editkeywords',   	 'Can create, destroy, and edit keywords.';
	AddGroup 'addnews',			 'Can add/modify/delete news items.';
	AddGroup 'devel',			 'Red Hat Development', '1', '0';
	AddGroup 'qa',				 'Red Hat Quality Assurance', '1', '0';
	AddGroup 'caneditall', 		 'Can edit all bugs', '0', '0';
	AddGroup 'setcontract',		 'Can set a contract bug', '0', '1';
}


###########################################################################
# Create initial test product if there are no products present.
###########################################################################

$sth = $dbh->prepare("SELECT product FROM products");
$sth->execute;

my $productref = $sth->fetchrow_hashref();

if (! defined($productref) && !$clean ) {
    print "Creating initial dummy product 'TestProduct' ...\n";
	my $query = "";
	$query = "INSERT INTO products(id, product, description) VALUES (products_seq.nextval, 'TestProduct', " .
             "'This is a test product.  This ought to be blown away and " .
             "replaced with real stuff in a finished installation of " .
             "bugzilla.')";
	print "$query\n" if $debug;
    $dbh->do($query);

	$query = "INSERT INTO versions (value, program) VALUES ('other', 'TestProduct')";
	print "$query\n" if $debug;
    $dbh->do($query);

	$query = "INSERT INTO components (value, program, initialowner, description) VALUES (" .
             "'TestComponent', 'TestProduct', 'dkl\@redhat.com', " .
             "'This is a test component in the test product database.  " .
             "This ought to be blown away and replaced with real stuff in " .
             "a finished installation of bugzilla.')";
	print "$query\n" if $debug;
    $dbh->do($query);
}





###########################################################################
# Populate the list of fields.
###########################################################################
FIELDDEFS:

my $headernum = 1;

sub AddFDef ($$$) {
    my ($name, $description, $mailhead) = (@_);

    $name = $dbh->quote($name);
    $description = $dbh->quote($description);

    my $sth = $dbh->prepare("SELECT fieldid FROM fielddefs " .
                            "WHERE name = $name");
    $sth->execute();
    my ($fieldid) = ($sth->fetchrow_array());
    if (!$fieldid) {
    	$dbh->do("INSERT INTO fielddefs " .
        	     "(fieldid, name, description, mailhead, sortkey) VALUES " .
            	 "(fielddefs_seq.nextval, $name, $description, $mailhead, $headernum)");
    	$headernum++;
	}
}


AddFDef("view", "View", 1);
AddFDef("class", "Class", 1);
AddFDef("bug_id", "Bug \#", 1);
AddFDef("short_desc", "Summary", 1);
AddFDef("product", "Product", 1);
AddFDef("version", "Version", 1);
AddFDef("rep_platform", "Platform", 1);
AddFDef("bug_file_loc", "URL", 1);
AddFDef("op_sys", "OS/Version", 1);
AddFDef("bug_status", "Status", 1);
AddFDef("status_whiteboard", "Status Whiteboard", 1);
AddFDef("keywords", "Keywords", 1);
AddFDef("resolution", "Resolution", 1);
AddFDef("bug_severity", "Severity", 1);
AddFDef("priority", "Priority", 1);
AddFDef("component", "Component", 1);
AddFDef("assigned_to", "AssignedTo", 1);
AddFDef("reporter", "ReportedBy", 1);
AddFDef("votes", "Votes", 0);
AddFDef("qa_contact", "QAContact", 0);
AddFDef("cc", "CC", 0);
AddFDef("dependson", "BugsThisDependsOn", 0);
AddFDef("blocked", "OtherBugsDependingOnThis", 0);
AddFDef("attachments.description", "Attachment description", 0);
AddFDef("attachments.thedata", "Attachment data", 0);
AddFDef("attachments.mimetype", "Attachment mime type", 0);
AddFDef("attachments.ispatch", "Attachment is patch", 0);
AddFDef("target_milestone", "Target Milestone", 0);
AddFDef("delta_ts", "Last changed date", 0);
AddFDef("(sysdate - bugs.delta_ts)", "Days since bug changed",
0);
AddFDef("longdesc", "Comment", 0);



###########################################################################
# Detect changed local settings
###########################################################################

sub GetFieldDef ($$) {
    my ($table, $field) = @_;
	my $sth = $dbh->prepare("select column_name, data_type from user_tab_columns " .
                			"where table_name = '" . uc($table) . "'");
    $sth->execute;

    while (my $ref = $sth->fetchrow_arrayref) {
        next if $$ref[0] ne uc($field);
        return $ref;
   }
}


sub CheckIndex ($) {
    my ($table) = (@_);
	if (!exists ($index{$table})) {
		return;
	}	
	# See if this table is already indexed in the Oracle Bugzilla database
	my $query = "select index_name from all_indexes where index_name = '" .
                         uc($table) . "_IDX'";
	print "$query\n" if $debug;
	$sth = $dbh->prepare($query);
	$sth->execute();
	my @row = $sth->fetchrow_array();
	
	# If it is not, then create one with the previous index definition
	if (!defined ($row[0])) {
		my $query = "create index " . $table . "_idx on $table (" . 
                    $index{$table} . ")";
		print "$query\n" if $debug;
		print "Creating index for $table ...\n";
		eval {
			$sth = $dbh->prepare($query);
			$sth->execute();
		};
		if ($@) {
			print "Creation of index for $table unsuccessful!\n";
		}
	}
}


sub CheckSequence ($$) {
    my ($table) = (@_);
	if (!exists ($sequence{$table})) {
		return;
	}
    # See if this sequence is already in the Oracle Bugzilla database
    my $query = "select sequence_name from all_sequences where sequence_name = '" .
                         uc($table) . "_SEQ'";
    print "$query\n" if $debug;
    $sth = $dbh->prepare($query);
    $sth->execute();
    my @row = $sth->fetchrow_array();

    # If it is not, then create the sequence definition
    if (!defined ($row[0])) {
        my $query = $sequence{$table}; 
        print "$query\n" if $debug;
		print "Create sequence for $table ...\n";
        eval {
            $sth = $dbh->prepare($query);
            $sth->execute();
        };
        if ($@) {
            print "Creation of sequence for " . $table . "_seq unsuccessful!\n";
        }
    }
}


#
# Check if the enums in the bugs table return the same values that are defined
# in the various locally changeable variables. If this is true, then alter the
# table values.
#

sub CheckEnumTable ($$) {
    my ($table, $against) = @_;
	foreach my $value (@$against) {
		my $sth = $dbh->prepare("select value from $table where value = '$value'");
		$sth->execute();
		my @row = $sth->fetchrow_array();
		if (!$row[0]) {
			$dbh->do("insert into $table values ('$value')") unless ($clean);
		}
    }
}



#
# This code changes the enum types of some SQL tables whenever you change
# some --LOCAL-- variables
#

CheckEnumTable('bug_severity', \@bug_severity);
CheckEnumTable('priority',     \@priority);
CheckEnumTable('op_sys',       \@op_sys);
CheckEnumTable('rep_platform', \@rep_platform);
CheckEnumTable('resolution',   \@resolution);


###########################################################################
# Promote first user into every group
###########################################################################

#
# Assume you just logged in. Now how can you administrate the system? Just
# execute checksetup.pl again. If there is only 1 user in bugzilla, then
# this user is promoted into every group.
#

$sth = $dbh->prepare("SELECT userid FROM profiles");
$sth->execute;
my @result;
while (my @row = $sth->fetchrow_array()) {
	push (@result, $row[0]);
} 

# when we have exactly one user ...
if (defined ($result[0]) && $#result < 1 && !$clean) {
    print "Putting user $result[0] into every group ...\n";
	my $sth = $dbh->prepare("select groupid from groups");
	my @groupids;
	$sth->execute();
	while (my @row = $sth->fetchrow_array()) {
		push (@groupids, $row[0]);
	}
	foreach my $groupid (@groupids) {
		$dbh->do("insert into user_group values ($result[0], $groupid)");
	}
}




###########################################################################
# Update the tables to the current definition
###########################################################################

#
# As time passes, fields in tables get deleted, added, changed and so on.
# So we need some helper subroutines to make this possible:
#

sub ChangeFieldType ($$$) {
    my ($table, $field, $newtype) = @_;

    my $ref = GetFieldDef($table, $field);
    #print "0: $$ref[0]   1: $$ref[1]   2: $$ref[2]   3: $$ref[3]  4: $$ref[4]\n";

    if ($$ref[1] ne $newtype) {
        print "Updating field type $field in table $table ...\n";
		my $query = "ALTER TABLE $table
                  MODIFY ($field $newtype)";
		print "$query\n" if $debug;
        $dbh->do($query);
    }
}

# Oracle cannot rename a field from what I could find,
# funny though you can rename an entire table though.

#sub RenameField ($$$) {
#    my ($table, $field, $newname) = @_;
#
#    my $ref = GetFieldDef($table, $field);
#    return unless $ref; # already fixed?
#    #print "0:	 $$ref[0]   1: $$ref[1]   2: $$ref[2]   3: $$ref[3]  4: $$ref[4]\n";
#
#    if ($$ref[1] ne $newname) {
#        print "Updating field $field in table $table ...\n";
#        my $type = $$ref[1];
#        $type .= " NOT NULL" if $$ref[3];
#        $dbh->do("ALTER TABLE $table
#                  CHANGE $field
#                  $newname $type");
#    }
#}

sub AddField ($$$) {
    my ($table, $field, $definition) = @_;

    my $ref = GetFieldDef($table, $field);
    return if $ref; # already added?

    print "Adding new field $field to table $table ...\n";
	my $query = "ALTER TABLE $table
              ADD ($field $definition)";
	print "$query\n" if $debug;
    $dbh->do($query);
}

# Oracle can't drop a field from what I can tell. Maybe someone knows how.

#sub DropField ($$) {
#    my ($table, $field) = @_;
#
#    my $ref = GetFieldDef($table, $field);
#    return unless $ref; # already dropped?
#
#    print "Deleting unused field $field from table $table ...\n";
#    $dbh->do("ALTER TABLE $table
#              DROP COLUMN $field");
#}


my $regenerateshadow = 0;


# 1999-06-22 Added an entry to the attachments table to record who the
# submitter was.  Nothing uses this yet, but it still should be recorded.

AddField('attachments', 'submitter_id', 'INTEGER CONSTRAINT ATTACH_NN_SUBMIT not null');

#
# One could even populate this field automatically, e.g. with
#
# unless (GetField('attachments', 'submitter_id') {
#    AddField ...
#    populate
# }
#
# For now I was too lazy, so you should read the README :-)


# 1999-10-11 Restructured voting database to add a cached value in each bug
# recording how many total votes that bug has.  

AddField('bugs',     'votes',        'INTEGER CONSTRAINT BUGS_NN_VOTES not null');
AddField('products', 'votesperuser', 'INTEGER CONSTRAINT PRODUCTS_NN_VOTES not null');



# The product name used to be very different in various tables.
#
# It was   varchar(16)   in bugs
#          tinytext      in components
#          tinytext      in products
#          tinytext      in versions
#
# tinytext is equivalent to varchar(255), which is quite huge, so I change
# them all to varchar(64).

# Oracle complains that the column must be empty for a column length to be changed.
# I will look into this sometime.
# This does work in MySQL though.

# ChangeFieldType ('bugs',       'product', 'VARCHAR2(64)');
# ChangeFieldType ('components', 'program', 'VARCHAR2(64)');
# ChangeFieldType ('products',   'product', 'VARCHAR2(64)');
# ChangeFieldType ('versions',   'program', 'VARCHAR2(64)');

# 2000-01-16 Added a "keywords" field to the bugs table, which
# contains a string copy of the entries of the keywords table for this
# bug.  This is so that I can easily sort and display a keywords
# column in bug lists.

if (!GetFieldDef('bugs', 'keywords')) {
    AddField('bugs', 'keywords', 'VARCHAR2(2000) CONSTRAINT BUGS_NN_KEYWDS not null');

    my @kwords;
    print "Making sure 'keywords' field of table 'bugs' is empty ...\n";
    $dbh->do("UPDATE bugs SET delta_ts = sysdate, keywords = '' " .
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
                $dbh->do("UPDATE bugs SET delta_ts = sysdate, keywords = " .
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

AddField('profiles', 'disabledtext',  'VARCHAR2(2000) CONSTRAINT PROFILES_NN_DISABLE not null');



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
    my $query = "INSERT INTO longdescs (bug_id, who, bug_when, thetext) VALUES " .
                "($id, $who, TO_DATE(" .
                $dbh->quote( time2str('%Y/%m/%d %H:%M:%S', $when) ) . ", 'YYYY-MM-DD HH24:MI:SS'), :1)";
    my $sth = $dbh->prepare($query);
    $sth->bind_param(1, $buffer, { SQL_LONGVARCHAR => 1 });
    $sth->execute();
}


if (GetFieldDef('bugs', 'long_desc')) {
    eval("use Date::Parse");
    eval("use Date::Format");
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
#               	print "\nDecided this line of bug $id has a date of " .
#                     time2str("'%Y/%m/%d %H:%M:%S'", $date) .
#                     "\nwhich is less than previous line:\n$line\n\n";
            	}
        	}
        	$buffer .= $line . "\n";
    	}
    	WriteOneDesc($id, $who, $when, $buffer);
		$regenerateshadow = 1;
	}
}


# 2000-01-18 Added a new table fielddefs that records information about the
# different fields we keep an activity log on.  The bugs_activity table
# now has a pointer into that table instead of recording the name directly.
FIELDCONV:

#if (GetFieldDef('bugs_activity', 'field')) {
#    AddField('bugs_activity', 'fieldid',
#             'INTEGER CONSTRAINT ACTIVITY_NN_ID not null');
    print "Populating new fieldid field ...\n";

#    $dbh->do("LOCK TABLES bugs_activity WRITE, fielddefs WRITE");

    my $sth = $dbh->prepare('SELECT DISTINCT field FROM bugs_activity');
    $sth->execute();
    my %ids;
    while (my ($f) = ($sth->fetchrow_array())) {
		if (!defined($f)) {
			next;
		}		
        my $q = $dbh->quote($f);
        my $s2 =
            $dbh->prepare("SELECT fieldid FROM fielddefs WHERE name = $q");
        $s2->execute();
        my ($id) = ($s2->fetchrow_array());
        if (!$id) {
			print "Adding fielddef $q...\n";
			my $s3 = $dbh->prepare("select max(sortkey) from fielddefs");
			$s3->execute();
			my ($sortkey) = ($s3->fetchrow_array());
			$sortkey++;
            $dbh->do("INSERT INTO fielddefs (fieldid, name, description, sortkey) VALUES " .
                     "(fielddefs_seq.nextval, $q, $q, $sortkey)");
            $s2 = $dbh->prepare("SELECT fielddefs_seq.currval from dual");
            $s2->execute();
            ($id) = ($s2->fetchrow_array());
        }
        $dbh->do("UPDATE bugs_activity SET fieldid = $id WHERE field = $q");
    }
#    $dbh->do("UNLOCK TABLES");

#    DropField('bugs_activity', 'field');
#}
        
exit;
# 2000-01-18 New email-notification scheme uses a new field in the bug to 
# record when email notifications were last sent about this bug.  Also,
# added a user pref whether a user wants to use the brand new experimental
# stuff.

if (!GetFieldDef('bugs', 'lastdiffed')) {
    AddField('bugs', 'lastdiffed', 'DATE CONSTRAINT BUGS_NN_LAST not null');
    $dbh->do('UPDATE bugs SET lastdiffed = sysdate, delta_ts = sysdate');
}

AddField('profiles', 'newemailtech', 'INTEGER CONSTRAINT PROFILES_NN_NEWMAIL not null');


# 2000-01-22 The "login_name" field in the "profiles" table was not
# declared to be unique.  Sure enough, somehow, I got 22 duplicated entries
# in my database.  This code detects that, cleans up the duplicates, and
# then tweaks the table to declare the field to be unique.  What a pain.

#if (GetIndexDef('profiles', 'login_name')->[1]) {
#    print "Searching for duplicate entries in the profiles table ...\n";
#    while (1) {
#        # This code is weird in that it loops around and keeps doing this
#        # select again.  That's because I'm paranoid about deleting entries
#        # out from under us in the profiles table.  Things get weird if
#        # there are *three* or more entries for the same user...
#        $sth = $dbh->prepare("SELECT p1.userid, p2.userid, p1.login_name " .
#                             "FROM profiles AS p1, profiles AS p2 " .
#                             "WHERE p1.userid < p2.userid " .
#                             "AND p1.login_name = p2.login_name " .
#                             "ORDER BY p1.login_name");
#        $sth->execute();
#        my ($u1, $u2, $n) = ($sth->fetchrow_array);
#        if (!$u1) {
#            last;
#        }
#        print "Both $u1 & $u2 are ids for $n!  Merging $u2 into $u1 ...\n";
#        foreach my $i (["bugs", "reporter"],
#                       ["bugs", "assigned_to"],
#                       ["bugs", "qa_contact"],
#                       ["attachments", "submitter_id"],
#                       ["bugs_activity", "who"],
#                       ["cc", "who"],
#                       ["votes", "who"],
#                       ["longdescs", "who"]) {
#            my ($table, $field) = (@$i);
#            print "   Updating $table.$field ...\n";
#            my $extra = "";
#            if ($table eq "bugs") {
#                $extra = ", delta_ts = delta_ts";
#            }
#            $dbh->do("UPDATE $table SET $field = $u1 $extra " .
#                     "WHERE $field = $u2");
#        }
#        $dbh->do("DELETE FROM profiles WHERE userid = $u2");
#    }
#    print "OK, changing index type to prevent duplicates in the future ...\n";
#    
#    $dbh->do("ALTER TABLE profiles DROP INDEX login_name");
#    $dbh->do("ALTER TABLE profiles ADD UNIQUE (login_name)");
#
#}    


# 2000-01-24 Added a new field to let people control whether the "My
# bugs" link appears at the bottom of each page.  Also can control
# whether each named query should show up there.

AddField('profiles', 'mybugslink', "INTEGER DEFAULT('0')");
AddField('namedqueries', 'linkinfooter', 'INTEGER CONSTRAINT QUERY_NN_LINK not null');

# 2000-04-03 Added new field to allow certain groups to be considered contract groups
# for tracking bugs submitted by our contract partners.

AddField('groups', 'contract', 'INTEGER DEFAULT(\'0\')');

# 2000-04-07 Added a new field for tracking of CRM numbers per bug report. Requested by
# Cygnus for EDK.

AddField('bugs', 'crm_no', 'VARCHAR2(2000) DEFAULT(\'\')');

#
# If you had to change the --TABLE-- definition in any way, then add your
# differential change code *** A B O V E *** this comment.
#
# That is: if you add a new field, you first search for the first occurence
# of --TABLE-- and add your field to into the table hash. This new setting
# would be honored for every new installation. Then add your
# AddField/DropField/ChangeFieldType/RenameField code above. This would then
# be honored by everyone who updates his Bugzilla installation.
#
#

# Final checks...
if ($regenerateshadow) {
    print "Now regenerating the shadow database for all bugs.\n";
    system("./processmail regenerate");
}
