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
# Config stuff for QNX.
######################################################################

include $(MOD_DEPTH)/config/UNIX.mk

CPU_ARCH	= x86

ifndef NS_USE_GCC
CC		= cc
CCC		= cc
endif
RANLIB		= true

G++INCLUDES	=
OS_LIBS		=
XLDOPTS		= -lunix

OS_CFLAGS	= -DQNX -Di386 -D_PR_LOCAL_THREADS_ONLY -D_PR_NEED_H_ERRNO
#IMPL_STRATEGY	= _EMU

NOSUCHFILE	= /no-such-file
