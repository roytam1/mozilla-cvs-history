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

XP_DEFINE  += -DXP_UNIX
LIB_SUFFIX  = a
DLL_SUFFIX  = so
AR          = ar
AR         += cr $@
LDOPTS     += -L$(SOURCE_LIB_DIR)

ifdef BUILD_OPT
	OPTIMIZER  += -O
	DEFINES    += -UDEBUG -DNDEBUG
else
	OPTIMIZER  += -g
	DEFINES    += -DDEBUG -UNDEBUG -DDEBUG_$(shell whoami)
endif

NSINSTALL_DIR  = $(GDEPTH)/gconfig/nsinstall
NSINSTALL      = $(NSINSTALL_DIR)/$(OBJDIR_NAME)/nsinstall

MKDEPEND_DIR    = $(GDEPTH)/gconfig/mkdepend
MKDEPEND        = $(MKDEPEND_DIR)/$(OBJDIR_NAME)/mkdepend
MKDEPENDENCIES  = $(NSINSTALL_DIR)/$(OBJDIR_NAME)/depend.mk

GUI_LIBS        = -lXm -lXt -lX11
MATH_LIB        = -lm
OPT_SLASH       = /
LIB_PREFIX      = lib
LIB_SUFFIX      = a
NSPR_LIBS       = nspr21 plds21 plc21 nspr21
LINK_PROGRAM    = $(CC)
XP_REG_LIB      = reg
ARCHIVE_SUFFIX  = 
VERSION_NUMBER  =
NATIVE_PLATFORM = unix
NATIVE_RAPTOR_WIDGET = widgetunix
NATIVE_RAPTOR_GFX = gfxunix
NATIVE_RAPTOR_WEB=raptorwebwidget
RAPTOR_GFX=raptorgfx
NATIVE_SMTP_LIB=smtp
NATIVE_MSG_COMM_LIB=comm
NATIVE_JULIAN_DLL=julian
PLATFORM_DIRECTORY=unix
NATIVE_ZLIB_DLL=zlib
NATIVE_XP_DLL=xp
XP_PREF_DLL=pref
AR_ALL =
AR_NONE =

ifdef MOZ_ZULU_FREE
NATIVE_LIBNLS_LIBS=nlsstub10
else
NATIVE_LIBNLS_LIBS=nsfmt$(MOZ_BITS)30 nsuni$(MOZ_BITS)30 nscck$(MOZ_BITS)30 nsjpn$(MOZ_BITS)30 nscnv$(MOZ_BITS)30 nssb$(MOZ_BITS)30
endif

####################################################################
#
# One can define the makefile variable NSDISTMODE to control
# how files are published to the 'dist' directory.  If not
# defined, the default is "install using relative symbolic
# links".  The two possible values are "copy", which copies files
# but preserves source mtime, and "absolute_symlink", which
# installs using absolute symbolic links.  The "absolute_symlink"
# option requires NFSPWD.
#   - THIS IS NOT PART OF THE NEW BINARY RELEASE PLAN for 9/30/97
#   - WE'RE KEEPING IT ONLY FOR BACKWARDS COMPATIBILITY
####################################################################

ifeq ($(NSDISTMODE),copy)
	# copy files, but preserve source mtime
	INSTALL  = $(NSINSTALL)
	INSTALL += -t
else
	ifeq ($(NSDISTMODE),absolute_symlink)
		# install using absolute symbolic links
		INSTALL  = $(NSINSTALL)
		INSTALL += -L `$(NFSPWD)`
	else
		# install using relative symbolic links
		INSTALL  = $(NSINSTALL)
		INSTALL += -R
	endif
endif

define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); $(NSINSTALL) -D $(@D); fi
endef

ifdef MOZ_TRACE_XPCOM_REFCNT
DEFINES += -DMOZ_TRACE_XPCOM_REFCNT
endif
