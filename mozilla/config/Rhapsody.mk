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

######################################################################
# Config stuff for Rhapsody
######################################################################
#
######################################################################
# Version-independent
######################################################################

ARCH			:= rhapsody
CPU_ARCH		:= ppc
GFX_ARCH		:= x

OS_INCLUDES		=
G++INCLUDES		=
LOC_LIB_DIR		= 
MOTIF			=
MOTIFLIB		=
OS_LIBS			=

PLATFORM_FLAGS		= -DRHAPSODY -Wall -pipe
MOVEMAIL_FLAGS		= -DHAVE_STRERROR
PORT_FLAGS		= -DSW_THREADS -DHAVE_STDDEF_H -DHAVE_STDLIB_H -DHAVE_FILIO_H -DNTOHL_ENDIAN_H -DMACHINE_ENDIAN_H -DNO_REGEX -DNO_REGCOMP -DHAS_PGNO_T -DNO_TZNAME -DNO_X11
PDJAVA_FLAGS		=

OS_CFLAGS		= $(PLATFORM_FLAGS) $(PORT_FLAGS) $(MOVEMAIL_FLAGS)

######################################################################
# Version-specific stuff
######################################################################

######################################################################
# Overrides for defaults in config.mk (or wherever)
######################################################################

CC			= /bin/cc
CCC			= /bin/cc++
EMACS			= /usr/bin/true
PERL			= /usr/bin/true
RANLIB			= /bin/libtool

######################################################################
# Other
######################################################################

