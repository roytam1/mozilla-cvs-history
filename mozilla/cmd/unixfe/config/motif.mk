#
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

##########################################################################
#
# Name:			motif.mk
#
# Description:	Motif Makefile hackery shared across unix and unik-like 
#               Mozilla front ends.
#
#               Currently this file is only used for linux, but eventually
#               it should be used for other platforms as well.
#
# Author:		Ramiro Estrugo <ramiro@netscape.com>
#
##########################################################################

ifeq ($(OS_ARCH),Linux)

##########################################################################
#
# MOZILLA_XFE_MOTIF_FLAGS
#
# This macro will contain the default flags needed to link 'mozilla-export' 
#
# The actual value will depend on whether static and/or dynamic motif 
# libraries are found in the system.
#
# The following two macros are used to determine the above value:
#
# MOZILLA_XFE_MOTIF_HAVE_STATIC_LIB
# MOZILLA_XFE_MOTIF_HAVE_DYNAMIC_LIB
#
# These are defined in the system specific generated detect makefile.  
# See mozilla/config/mkdetect for details.
#
##########################################################################
ifdef MOZILLA_XFE_MOTIF_HAVE_STATIC_LIB

MOZILLA_XFE_MOTIF_FLAGS = $(MOZILLA_XFE_MOTIF_STATIC_FLAGS)

else

ifdef MOZILLA_XFE_MOTIF_HAVE_DYNAMIC_LIB

MOZILLA_XFE_MOTIF_FLAGS = $(MOZILLA_XFE_MOTIF_DYNAMIC_FLAGS)

else

error "Motif library (static or dynamic) required"

endif

endif
##########################################################################

endif # OS_ARCH == Linux
