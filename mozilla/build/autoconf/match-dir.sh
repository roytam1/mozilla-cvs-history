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

#
# This script will match a dir with a set of dirs.
#
# Usage: match-dir.sh match [dir1 dir2 ... dirn]
#
# Send comments, improvements, bugs to ramiro@netscape.com
# 

if [ -f Makefile ]; then
	MAKEFILE="Makefile"
else
	if [ -f Makefile.in ]; then
		MAKEFILE="Makefile.in"
	else
		echo
		echo "There ain't no 'Makefile' or 'Makefile.in' over here: $pwd, dude."
		echo
		exit 1
	fi
fi

# Use DEPTH in the Makefile.in to determine the depth
depth=`grep -w DEPTH ${MAKEFILE}  | grep -e "\.\." | awk -F"=" '{ print $2; }'`

# Determine the depth count
n=`echo $depth | tr '/' ' ' | wc -w`

# Determine the path (strip anything before the mozilla/ root)
# If we are building in the source directory then the Makefile will have
# srcdir set to '.' and we have to get the path from the pwd command.
# If we are building outside of the source tree then we can get the information
# we need from the srcdir statement in the Makefile.

tpth=`grep '^srcdir.*/mozilla/' ${MAKEFILE}`
if [ "$tpth" = "" ]; then
	tpth=`pwd`
fi

path=`echo $tpth | awk -v count=$n -F"/" '\
{ for(i=NF-count+0; i <= NF ; i++) \
{ \
if (i!=NF) \
  { printf "%s/", $i } \
else \
  { printf "%s", $i } \
} \
}'`

match=$path

for i in $*
do
#	echo "Looking for $match in $i"

	echo $i | grep -q -x $match

	if [ $? -eq 0 ]
	then
		echo "1"

		exit 0
	fi

#	echo "Looking for $i in $match"

	echo $match | grep -q $i

	if [ $? -eq 0 ]
	then
		echo "1"

		exit 0
	fi
done

echo "0"

exit 0
