# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 

#######################################################################
# Master "Core Components" file system "release" prefixes             #
#######################################################################

#	RELEASE_TREE = $(GDEPTH)/../coredist


ifndef RELEASE_TREE
	RELEASE_TREE = /m/dist

	ifeq ($(OS_TARGET), WINNT)
		RELEASE_TREE = \\helium\dist
	endif

	ifeq ($(OS_TARGET), WIN95)
		RELEASE_TREE = \\helium\dist
	endif

	ifeq ($(OS_TARGET), WIN16)
		RELEASE_TREE = \\helium\dist
	endif
endif

RELEASE_XP_DIR = 
RELEASE_MD_DIR = $(PLATFORM)


REPORTER_TREE = $(subst \,\\,$(RELEASE_TREE))

