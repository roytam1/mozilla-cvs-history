#!/bin/sh
#
# The contents of this file are subject to the Netscape Public License
# Version 1.0 (the "NPL"); you may not use this file except in
# compliance with the NPL.  You may obtain a copy of the NPL at
# http://www.mozilla.org/NPL/
#
# Software distributed under the NPL is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
# for the specific language governing rights and limitations under the
# NPL.
#
# The Initial Developer of this code under the NPL is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation.  All Rights
# Reserved.
#

## 
## Usage:
##
## $ run-mozilla.sh [options] [program] [program arguments]
##
## This script is meant to run a mozilla program from the mozilla
## source tree.  This is mostly useful to folks hacking on mozilla.
##
## A mozilla program is currently either viewer or apprunner.  The
## default is viewer.
##
## The script will setup all the environment voodoo needed to make
## mozilla work.

##
## Standard shell script disclaimer blurb thing:
##
## This script is a hcak.  Its brute force.  Its horrible.  
## It doesnt use Artificial Intelligence.  It doesnt use Virtual Reality.
## Its not perl.  Its not python.  It probably wont work unchanged on
## the "other" thousands of unices.  But it worksforme.
##
## If you have an improvement, patch, idea, whatever, on how to make this
## script better, post it here:
##
## news://news.mozilla.org/netscape.public.mozilla.patches
## news://news.mozilla.org/netscape.public.mozilla.unix
## 

##
## Potential improvements:
##
## + Run from anywhere in the tree.
## + Run ldd on the program and report missing dlls
## + Deal with NSPR in the tree
## + All the "other" unices
##

cmdname=`basename $0`

##
## Constants
##
MOZ_APPRUNNER_NAME="apprunner"
MOZ_VIEWER_NAME="viewer"

##
## Variables
##
MOZ_DIST_BIN=""
MOZ_PROGRAM=""

##
## Functions
##
##########################################################################
moz_usage()
{
    cat << EOF

Usage:  ${cmdname} [options] [program]

  options:

    -g                   Run in debugger.
    --debug

    -d debugger          Debugger to use.
    --debugger debugger

  Examples:

  Run the viewer

    ${cmdname} viewer

  Run the apprunner

    ${cmdname} apprunner

  Debug the viewer in a debbuger

    ${cmdname} -g viewer

  Debug the apprunner in gdb

    ${cmdname} -g viewer -d gdb

EOF

	return 0
}
##########################################################################
moz_bail()
{
	message=$1

	echo
	echo "$cmdname: $message"
	echo

	exit 1
}
##########################################################################
moz_test_binary()
{
	binary=$1

	if [ -f "$binary" ]
	then
		if [ -x "$binary" ]
		then
			return 1
		fi
	fi

	return 0
}
##########################################################################
moz_get_debugger()
{
	debuggers="ddd gdb dbx"

	debugger="notfound"

	done="no"

	for d in $debuggers
	do
		dpath=`which ddd`
	
		if [ -x "$dpath" ]
		then
			debugger=$dpath
			break
		fi
	done

	echo $debugger

	return 0
}
##########################################################################
moz_run_program()
{
	prog=$MOZ_PROGRAM

	##
	## Make sure the program is executable
	##
	if [ ! -x "$prog" ]
	then
		moz_bail "Cannot execute $prog."
	fi

	##
	## Use md5sum to crc a core file.  If md5sum is not found on the system,
	## then dont debug core files.
	##
	crc_prog=`which md5sum`

	if [ -x "$crc_prog" ]
	then
		DEBUG_CORE_FILES=1
	fi

	if [ "$DEBUG_CORE_FILES" ]
	then
		crc_old=

		if [ -f core ]
		then
			crc_old=`$crc_prog core | awk '{print $1;}' `
		fi
	fi

	##
	## Run the program
	##
	$prog ${1+"$@"}

	if [ "$DEBUG_CORE_FILES" ]
	then
		if [ -f core ]
		then
			crc_new=`$crc_prog core | awk '{print $1;}' `
		fi
	fi

	if [ "$crc_old" != "$crc_new" ]
	then
		printf "\n\nOh no!  %s just dumped a core file.\n\n" $prog
		printf "Do you want to debug this ? [y/n] "

		read ans

		if [ "$ans" = "y" ]
		then
			debugger=`moz_get_debugger`

			if [ -x "$debugger" ]
			then
				echo "$debugger $prog core"
				$debugger $prog core
			else
				echo "Could not find a debugger on your system."
			fi
		fi
	fi
}
##########################################################################
moz_debug_program()
{
	prog=$MOZ_PROGRAM

	##
	## Make sure the program is executable
	##
	if [ ! -x "$prog" ]
	then
		moz_bail "Cannot execute $prog."
	fi

	if [ -n "$moz_debugger" ]
	then
		debugger=`which $moz_debugger`
	else
		debugger=`moz_get_debugger`
	fi

	if [ -x "$debugger" ]
	then
		echo "$debugger $prog ${1+"$@"}"
		$debugger $prog ${1+"$@"}
	else
		echo "Could not find a debugger on your system."
	fi
}
##########################################################################


##
## Command line arg defaults
##
moz_debug=0
moz_debugger=""

##
## Parse the command line
##
while [ -n "$(echo $1 | grep '^-')" ]
do
	case $1 in
		# help
		-h | --help)
			moz_usage

			exit 0
			;;

		# debug
		-g | --debug)
			moz_debug=1
			;;

		-d | --debugger)
			moz_debugger=$2;

			shift

			;;

		*)
			ARGS_SAVE="$ARGS_SAVE $1"
			;;
	esac

	shift
done

##
## Program name given in $1
##
if [ $# -gt 0 ]
then
	MOZ_PROGRAM=$1

	shift
fi

##
## Program not given, try to guess a default
##
if [ -z "$MOZ_PROGRAM" ]
then
	##
	## Try viewer
	## 
	moz_test_binary $MOZ_VIEWER_NAME

	if [ $? -eq 1 ]
	then
		MOZ_PROGRAM=$MOZ_VIEWER_NAME
	##
	## Try apprunner
	## 
	else
		moz_test_binary $MOZ_APPRUNNER_NAME

		if [ $? -eq 1 ]
		then
			MOZ_PROGRAM=$MOZ_APPRUNNER_NAME
		fi
	fi
fi

##
## Running the program from its source dir
##
if [ -f Makefile.in ]
then
	# Use DEPTH in the Makefile.in to determine the depth
	depth=`grep -w DEPTH Makefile.in  | grep -e "\.\." | awk -F"=" '{ print $2; }'`

	##
	## Make sure dist/bin exits
	##
	if [ ! -d $depth/dist/bin ]
	then
		moz_bail "$depth/dist/bin does not exist."
	fi

	# push
	here=`pwd`

	cd $depth/dist/bin

	MOZ_DIST_BIN=`pwd`

	# pop
	cd $here
else
	##
	## Running the program from dist/bin
	##
	if [ -d components -a -d res ]
	then
		MOZ_DIST_BIN=`pwd`
	fi
fi

##
## Make sure dist/bin is ok
##
if [ -z "$MOZ_DIST_BIN" ]
then
	moz_bail "Cannot access dir dist/bin directory."
fi

if [ ! -d $MOZ_DIST_BIN ]
then
	moz_bail "Cannot access dir dist/bin directory."
fi

##
## Make sure the program is executable
##
if [ ! -x "$MOZ_PROGRAM" ]
then
	moz_bail "Cannot execute $MOZ_PROGRAM."
fi

##
## Set MOZILLA_FIVE_HOME
##
MOZILLA_FIVE_HOME=$MOZ_DIST_BIN

##
## Set LD_LIBRARY_PATH
##
if [ -n "$LD_LIBRARY_PATH" ]
then
	LD_LIBRARY_PATH=${MOZ_DIST_BIN}${LD_LIBRARY_PATH+":$LD_LIBRARY_PATH"}
else
	LD_LIBRARY_PATH=${MOZ_DIST_BIN}
fi

echo "MOZILLA_FIVE_HOME=$MOZILLA_FIVE_HOME"
echo "  LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "      MOZ_PROGRAM=$MOZ_PROGRAM"
echo "        moz_debug=$moz_debug"
echo "     moz_debugger=$moz_debugger"

export MOZILLA_FIVE_HOME LD_LIBRARY_PATH

if [ "$moz_debug" = "1" ]
then
	moz_debug_program ${1+"$@"}
else
	moz_run_program $ARGS_SAVE ${1+"$@"}
fi
