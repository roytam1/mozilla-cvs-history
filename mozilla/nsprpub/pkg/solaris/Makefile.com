#
# Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#ident	"$Id$"
#

MACH = $(shell mach)

PUBLISH_ROOT = $(DIST)
ifeq ($(MOD_DEPTH),../..)
ROOT = ROOT
else
ROOT = $(subst ../../,,$(MOD_DEPTH))/ROOT
endif

PKGARCHIVE = $(dist_libdir)/pkgarchive
DATAFILES = copyright
FILES = $(DATAFILES) pkginfo

PACKAGE = $(shell basename `pwd`)

PRODUCT_VERSION = $(MOD_VERSION).$(MOD_MINOR).$(MOD_PATCH)
LN = /usr/bin/ln

CLOBBERFILES = $(FILES)

include $(topsrcdir)/config/rules.mk

# vim: ft=make
