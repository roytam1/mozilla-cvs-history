#!gmake
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

# Build the Mozilla client.
#
# This needs CVSROOT set to work, e.g.,
#   setenv CVSROOT :pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot
# or
#   setenv CVSROOT :pserver:username%somedomain.org@cvs.mozilla.org:/cvsroot
# 
# To checkout and build a tree,
#    1. cvs co mozilla/client.mk
#    2. cd mozilla
#    3. gmake -f client.mk
#
# Other targets (gmake -f client.mk [targets...]),
#    checkout
#    build
#    realclean (also clobber_all)
#    clean     (also clobber)
#
# See http://www.mozilla.org/build/unix.html for more information.
#
# Options:
#   MOZ_OBJDIR           - Destination object directory
#   MOZ_CO_BRANCH        - Branch tag to use for checkout (default: HEAD)
#   MOZ_CO_DATE          - Date tag to use for checkout (default: none)
#   MOZ_CO_MODULE        - Module to checkout (default: SeaMonkeyEditor)
#   MOZ_CVS_FLAGS        - Flags to pass cvs (default: -q -z3)
#   MOZ_CO_FLAGS         - Flags to pass after 'cvs co' (default: -P)

CWD		:= $(shell pwd)
ifeq (mozilla, $(notdir $(CWD)))
ROOTDIR		:= $(shell dirname $(CWD))
TOPSRCDIR       := $(CWD)
else
ROOTDIR		:= $(CWD)
TOPSRCDIR       := $(CWD)/mozilla
endif

AUTOCONF	:= autoconf
MKDIR		:= mkdir
SH		:= /bin/sh
ifndef MAKE
MAKE		:= gmake
endif

CONFIG_GUESS  := $(wildcard $(TOPSRCDIR)/build/autoconf/config.guess)
ifdef CONFIG_GUESS
  CONFIG_GUESS := $(shell $(CONFIG_GUESS))
else
  _IS_FIRST_CHECKOUT := 1
endif

#######################################################################
# Defines
#

####################################
# CVS

# Add the CVS root to CVS_FLAGS if needed
CVS_ROOT_IN_TREE  := $(shell cat $(TOPSRCDIR)/CVS/Root 2>/dev/null)
ifneq ($(CVS_ROOT_IN_TREE),)
ifneq ($(CVS_ROOT_IN_TREE),$(CVSROOT))
  CVS_FLAGS := -d $(CVS_ROOT_IN_TREE)
endif
endif

CVSCO	      = cvs $(CVS_FLAGS) co $(CVS_CO_FLAGS)
CVSCO_LOGFILE := $(ROOTDIR)/cvsco.log

####################################
# Load mozconfig Options

# See build pages, http://www.mozilla.org/build/unix.html, 
# for how to set up mozconfig.
MOZCONFIG2DEFS := mozilla/build/autoconf/mozconfig2defs.sh
run_for_side_effects := \
  $(shell cd $(ROOTDIR); \
          if test "$(_IS_FIRST_CHECKOUT)"; then \
	    $(CVSCO) mozilla/build/autoconf/find-mozconfig.sh; \
	    $(CVSCO) $(MOZCONFIG2DEFS); \
	  else true; \
	  fi; \
	  $(MOZCONFIG2DEFS) mozilla/.client-defs.mk)
include $(TOPSRCDIR)/.client-defs.mk

####################################
# Options that may come from mozconfig

# MOZ_CVS_FLAGS - Basic CVS flags
ifeq "$(origin MOZ_CVS_FLAGS)" "undefined"
  CVS_FLAGS := $(CVS_FLAGS) -q -z 3
else
  CVS_FLAGS := $(MOZ_CVS_FLAGS)
endif

# MOZ_CO_FLAGS - Anything that we should use on all checkouts
ifeq "$(origin MOZ_CO_FLAGS)" "undefined"
  CVS_CO_FLAGS := -P
else
  CVS_CO_FLAGS := $(MOZ_CO_FLAGS)
endif

ifdef MOZ_CO_BRANCH
  CVS_CO_FLAGS := $(CVS_CO_FLAGS) -r $(MOZ_CO_BRANCH)
endif

ifdef MOZ_CO_DATE
  CVS_CO_FLAGS := $(CVS_CO_FLAGS) -D "$(MOZ_CO_DATE)"
endif

ifndef MOZ_CO_MODULE
  MOZ_CO_MODULE := SeaMonkeyEditor
endif

ifdef MOZ_OBJDIR
  OBJDIR := $(MOZ_OBJDIR)
else
  OBJDIR := $(TOPSRCDIR)
endif


#######################################################################
# Rules
# 

ifdef _IS_FIRST_CHECKOUT
all: checkout build
else
all: checkout depend build
endif

# Windows equivalents
pull_all:     checkout
build_all:    build
clobber:      clean
clobber_all:  realclean
pull_and_build_all: checkout build

# Do everything from scratch
everything: clobber_all all

####################################
# CVS checkout

checkout:
	@if test -f $(CVSCO_LOGFILE) ; then \
	  mv $(CVSCO_LOGFILE) $(CVSCO_LOGFILE).old; \
	else true; \
	fi
	@date > $(CVSCO_LOGFILE)
	cd $(ROOTDIR) && \
	  $(CVSCO) $(MOZ_CO_MODULE) 2>&1 | tee -a $(CVSCO_LOGFILE)
	@conflicts=`egrep "^C " $(CVSCO_LOGFILE)` ;\
	if test "$$conflicts" ; then \
	  echo "$(MAKE): *** Conflicts during checkout." ;\
	  echo "$$conflicts" ;\
	  echo "$(MAKE): Refer to $(CVSCO_LOGFILE) for full log." ;\
	  exit 1; \
	else true; \
	fi

####################################
# Web configure

WEBCONFIG_URL   := http://cvs-mirror.mozilla.org/webtools/build/config.cgi
WEBCONFIG_FILE  := $(HOME)/.mozconfig

MOZCONFIG2URL := build/autoconf/mozconfig2url.sh
webconfig:
	cd $(TOPSRCDIR); \
	@url=$(WEBCONFIG_URL)`$(MOZCONFIG2URL)`; \
	echo Running netscape with the following url: ;\
	echo ;\
	echo $$url ;\
	netscape -remote "openURL($$url)" || netscape $$url ;\
	echo ;\
	echo   1. Fill out the form on the browser. ;\
	echo   2. Save the results to $(WEBCONFIG_FILE).

#	netscape -remote "saveAs($(WEBCONFIG_FILE))"

#####################################################
# First Checkout

ifdef _IS_FIRST_CHECKOUT
# First time, do build target in a new process to pick up new files.
build:
	$(MAKE) -f $(TOPSRCDIR)/client.mk build
else

#####################################################
# After First Checkout


####################################
# Configure

ALL_TRASH += \
	$(OBJDIR)/config.cache \
	$(OBJDIR)/config.log \
	$(OBJDIR)/config.status \
	$(OBJDIR)/config-defs.h \
	$(NULL)

CONFIG_STATUS := $(wildcard $(OBJDIR)/config.status)
CONFIG_CACHE  := $(wildcard $(OBJDIR)/config.cache)

ifdef RUN_AUTOCONF_LOCALLY
EXTRA_CONFIG_DEPS := \
	$(TOPSRCDIR)/aclocal.m4 \
	$(TOPSRCDIR)/build/autoconf/gtk.m4 \
	$(TOPSRCDIR)/build/autoconf/altoptions.m4 \
	$(NULL)

$(TOPSRCDIR)/configure: $(TOPSRCDIR)/configure.in $(EXTRA_CONFIG_DEPS)
	@echo Generating $@ using autoconf
	cd $(TOPSRCDIR); $(AUTOCONF)
endif

CONFIG_STATUS_DEPS := \
	$(TOPSRCDIR)/configure \
	$(TOPSRCDIR)/allmakefiles.sh \
	$(TOPSRCDIR)/.client-defs.mk \
	$(wildcard $(TOPSRCDIR)/mailnews/makefiles) \
	$(NULL)

$(OBJDIR)/Makefile $(OBJDIR)/config.status: $(CONFIG_STATUS_DEPS)
	@if test ! -d $(OBJDIR); then $(MKDIR) $(OBJDIR); else true; fi
	@echo cd $(OBJDIR); 
	@echo ../configure
	@cd $(OBJDIR) && \
	  $(TOPSRCDIR)/configure \
	  || ( echo "*** Fix above errors and then restart with\
               \"$(MAKE) -f client.mk build\"" && exit 1 )

ifdef CONFIG_STATUS
$(OBJDIR)/config/autoconf.mk: $(TOPSRCDIR)/config/autoconf.mk.in
	cd $(OBJDIR); \
	  CONFIG_FILES=config/autoconf.mk ./config.status
endif


####################################
# Depend

depend: $(OBJDIR)/Makefile $(OBJDIR)/config.status
	cd $(OBJDIR); $(MAKE) $@;

####################################
# Build it

build:  $(OBJDIR)/Makefile $(OBJDIR)/config.status
	cd $(OBJDIR); $(MAKE);

####################################
# Other targets

# Pass these target onto the real build system
clean realclean:
	cd $(OBJDIR); $(MAKE) $@
	rm -fr $(ALL_TRASH)

cleansrcdir:
	@cd $(TOPSRCDIR); \
	if [ -f Makefile ]; then \
	  $(MAKE) realclean && _skip_find=1; \
	fi; \
	if [ ! "$$_skip_find" ]; then \
	  echo "Removing object files from srcdir..."; \
	  rm -fr `find . -type d \( -name .deps -print -o -name CVS \
	          -o -exec test ! -d {}/CVS \; \) -prune \
	          -o \( -name '*.[ao]' -o -name '*.so' \) -type f -print`; \
	fi; \
	echo "Removing files generated by configure..."; \
	build/autoconf/clean-config.sh;


# (! IS_FIRST_CHECKOUT)
endif

.PHONY: checkout depend build clean realclean cleansrcdir pull_all build_all clobber clobber_all pull_and_build_all everything
