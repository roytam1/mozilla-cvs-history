#!/usr/bin/perl -w
###########################################################################
# $Id$
#
# BB_include.pl
# 
# This include file contains global variables used across programs.
# 
# History:
#         Kevin J. McCarthy   [10/17/2000]   Original Coding
###########################################################################

use strict;


######################################################################
# Set this variable to 1 only if the failsafe triggers, to temporarily
# override it.
# TODO: command line switch to over-ride.
######################################################################
use vars qw{$FAILSAFE_OVERRIDE};
$FAILSAFE_OVERRIDE = 0;
# $FAILSAFE_OVERRIDE = 1;

#####################################################################
# This controls the maximum number of days we will attempt to recover
# data for.
#####################################################################
use vars qw($MAX_RECOVER_DAYS);
$MAX_RECOVER_DAYS = 500;


###############
# Directories #
###############
use vars qw{$BILLING_HOME $RRD_DIR $REPORT_HOME $LOG_DIR $LOG_ARCHIVE_DIR
            $BIN_DIR $INC_DIR};

#$BILLING_HOME = "/usr/local/root/apache/cgi-bin/Burstable";
$BILLING_HOME = "/home/kevinmc/excite/burst_billing/code";

$RRD_DIR = "/usr/local/nme/polling/www";

$REPORT_HOME = "$BILLING_HOME/data";

$LOG_DIR = "$BILLING_HOME/log";
$LOG_ARCHIVE_DIR = "$LOG_DIR/archive";

$BIN_DIR = "$BILLING_HOME/bin";

$INC_DIR = "$BILLING_HOME/inc";


##############
# File Names #
##############
use vars qw{$BURST_ACCOUNTS_FILE $LOG_FILE $LAST_RUN_FILE $IN_DATA_FILE $OUT_DATA_FILE
            $CIRCUIT_HEADER_FILE $IN_PROGRESS_FILE $REPORT_FILE};
$BURST_ACCOUNTS_FILE = "$INC_DIR/burst_accounts.txt";

$LOG_FILE = "log";

$LAST_RUN_FILE = "lastrun.txt";

#
# Network data is dumped to these files
#
$IN_DATA_FILE = "in.log";
$OUT_DATA_FILE = "out.log";

#
# This contains information about the circuit
#
$CIRCUIT_HEADER_FILE = "header.txt";

#
# This file is used to mark when this program is running.  We don't
# want to allow to loads to take place at the same time
#
$IN_PROGRESS_FILE = "$BILLING_HOME/.running";

#
# The monthly 95/5 report
#
$REPORT_FILE = "95_5_report.xls";


############
# Programs #
############
use vars qw{$BILLING_MODULE $MONITOR_MODULE};
$BILLING_MODULE = "BB_billing.pl";
$MONITOR_MODULE = "BB_monitor.pl";


##################
# Log File Codes #
##################
use vars qw{$START_STAMP $END_STAMP $LOG_NOTE $LOG_INFO $LOG_WARNING
            $LOG_ERROR};
$START_STAMP = "START_STAMP";
$END_STAMP = "END_STAMP";
$LOG_NOTE = "NOTE";
#
# Info error messages get included in the monitor email.  Note messages don't
#
$LOG_INFO = "INFO";
$LOG_WARNING = "WARNING";
$LOG_ERROR = "ERROR";


#################################################
# Number of archives to keep for each directory #
#################################################
use vars qw{$MAX_LOG_ARCHIVE_FILES};
$MAX_LOG_ARCHIVE_FILES = 10000;


###############
# Error Codes #
###############
use vars qw{$OKAY $ERROR_WARNING $ERROR_CRITICAL};
$OKAY = 0;
$ERROR_WARNING = 1;
$ERROR_CRITICAL = 2;


#################
# Email Setting #
#################
use vars qw{$MAIL_HOSTS $MAIL_FROM $ERROR_MAIL_TO $REPORT_MAIL_TO};
$MAIL_HOSTS = ['localhost',
               'mail.excitehome.net'];
$MAIL_FROM = 'burstbilling@excitehome.net';
$ERROR_MAIL_TO = ['mccarthy@excitehome.net',
            'tunacat@yahoo.com',
#            'michael@excitehome.net',
           ];

# $REPORT_MAIL_TO = 'burstbilling@excitehome.net';
$REPORT_MAIL_TO = 'mccarthy@excitehome.net';


#####################
# Data Error Checking
#####################
use vars qw{$MAX_ZERO_DATA_PERCENT $MAX_NAN_DATA_PERCENT $MIN_RRD_STEP_SIZE};
#
# Maximum percentage of 0's to put up with in the data before I log an error
# Use values from 0 - 100
#
$MAX_ZERO_DATA_PERCENT = 50;

#
# Maximum percentage of NaN's to put up with in the data before I log an error
# Use values from 0 - 100
#
$MAX_NAN_DATA_PERCENT = 0;

#
# This is the smallest step size returned by the RRDs - equal to a 5 minute
# granularity.
#
$MIN_RRD_STEP_SIZE = 300;


###############################
# Monitor Time List Hash Keys #
###############################
use vars qw{$MTL_START_EPOCH $MTL_END_EPOCH $MTL_FIRST_OF_MONTH
            $MTL_YEAR $MTL_MONTH $MTL_MDAY};
$MTL_START_EPOCH = 'MTL_START_EPOCH';
$MTL_END_EPOCH = 'MTL_END_EPOCH';
$MTL_FIRST_OF_MONTH = 'MTL_FIRST_OF_MONTH';
$MTL_YEAR = 'MTL_YEAR';
$MTL_MONTH = 'MTL_MONTH';
$MTL_MDAY = 'MTL_MDAY';


###########################
# Circuit Entry Hash Keys #
###########################
use vars qw{$CIRCUIT_COMPANY_NAME $CIRCUIT_IP_ADDRESS
            $CIRCUIT_PORT_NAME $CIRCUIT_ID};
$CIRCUIT_COMPANY_NAME = 'CIRCUIT_COMPANY_NAME';
$CIRCUIT_IP_ADDRESS = 'CIRCUIT_IP_ADDRESS';
$CIRCUIT_PORT_NAME  = 'CIRCUIT_PORT_NAME';
$CIRCUIT_ID = 'CIRCUIT_ID';
