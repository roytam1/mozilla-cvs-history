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

#
# Config stuff for NCR SysVr4 v 3.0
#

include $(GDEPTH)/gconfig/UNIX.mk

DEFAULT_COMPILER = cc

###
NS_USE_NATIVE = 1

# NS_USE_GCC = 1

export PATH:=$(PATH):/opt/ncc/bin
###

RANLIB           = true
GCC_FLAGS_EXTRA += -pipe

DEFINES		+= -DSVR4 -DSYSV -DHAVE_STRERROR -DNCR

OS_CFLAGS	+= -Hnocopyr -DSVR4 -DSYSV -DHAVE_STRERROR -DNCR -DPRFSTREAMS_BROKEN

ifdef NS_USE_NATIVE
	CC       = cc
	CCC      = ncc
	CXX      = ncc
#	OS_LIBS += -L/opt/ncc/lib 
else
#	OS_LIBS	+=
endif

#OS_LIBS    += -lsocket -lnsl -ldl -lc

MKSHLIB     += $(LD) $(DSO_LDOPTS)
#DSO_LDOPTS += -G -z defs
DSO_LDOPTS += -G

CPU_ARCH    = x86
ARCH        = ncr

NOSUCHFILE  = /solaris-rm-f-sucks

# now take care of default GCC (rus@5/5/97)
 
ifdef NS_USE_GCC
	# if gcc-settings are redefined already - don't touch it
	#
	ifeq (,$(findstring gcc, $(CC)))
		CC   = gcc
		CCC  = g++
		CXX  = g++
		# always use -fPIC - some makefiles are still broken and don't distinguish
		# situation when they build shared and static libraries
		CFLAGS  += -fPIC -Wall $(GCC_FLAGS_EXTRA)
#		OS_LIBS += -L/usr/local/lib -lstdc++ -lg++ -lgcc
	endif
endif
###
