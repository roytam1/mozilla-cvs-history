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

#
# config.mk
#
# Determines the platform and builds the macros needed to load the
# appropriate platform-specific .mk file, then defines all (most?)
# of the generic macros.
#

# Define an include-at-most-once flag
INCLUDED_CONFIG_MK = 1

ifndef topsrcdir
topsrcdir	= $(DEPTH)
endif

ifndef INCLUDED_AUTOCONF_MK
include $(DEPTH)/config/autoconf.mk
endif
ifndef INCLUDED_INSURE_MK
ifdef MOZ_INSURIFYING
include $(topsrcdir)/config/insure.mk
endif
endif

ifndef INCLUDED_COMMON_MK
include $(topsrcdir)/config/common.mk
endif

BUILD_TOOLS	= $(topsrcdir)/build/unix
CONFIG_TOOLS	= $(DEPTH)/config
AUTOCONF_TOOLS	= $(topsrcdir)/build/autoconf

#
# Tweak the default OS_ARCH and OS_RELEASE macros as needed.
#
ifeq ($(OS_ARCH),AIX)
OS_RELEASE	:= $(shell uname -v).$(shell uname -r)
endif
ifeq ($(OS_ARCH),BSD_386)
OS_ARCH		:= BSD_OS
endif
ifeq ($(OS_ARCH),dgux)
OS_ARCH		:= DGUX
endif
ifeq ($(OS_ARCH),IRIX64)
OS_ARCH		:= IRIX
endif
ifeq ($(OS_ARCH),UNIX_SV)
ifneq ($(findstring NCR,$(shell grep NCR /etc/bcheckrc | head -1 )),)
OS_ARCH		:= NCR
else
OS_ARCH		:= UNIXWARE
OS_RELEASE	:= $(shell uname -v)
endif
endif
ifeq ($(OS_ARCH),ncr)
OS_ARCH		:= NCR
endif
# This is the only way to correctly determine the actual OS version on NCR boxes.
ifeq ($(OS_ARCH),NCR)
OS_RELEASE	:= $(shell awk '{print $$3}' /etc/.relid | sed 's/^\([0-9]\)\(.\)\(..\)\(.*\)$$/\2.\3/')
endif
ifeq ($(OS_ARCH),UNIX_System_V)
OS_ARCH		:= NEC
endif
ifeq ($(OS_ARCH),OSF1)
OS_SUB		:= $(shell uname -v)
# Until I know the other possibilities, or an easier way to compute them, this is all there's gonna be.
#ifeq ($(OS_SUB),240)
#OS_RELEASE	:= V2.0
#endif
ifeq ($(OS_SUB),148)
OS_RELEASE	:= V3.2C
endif
ifeq ($(OS_SUB),564)
OS_RELEASE	:= V4.0B
endif
ifeq ($(OS_SUB),878)
OS_RELEASE	:= V4.0D
endif
endif
ifneq (,$(findstring OpenVMS,$(OS_ARCH)))
OS_ARCH		:= OpenVMS
OS_RELEASE	:= $(shell uname -v)
CPU_ARCH	:= $(shell uname -Wh)
CPU_ARCH_TAG	:= _$(CPU_ARCH)
PERL		:= perl
endif
ifeq ($(OS_ARCH),QNX)
ifeq ($(OS_TARGET),NTO)
LD		:= qcc -Vgcc_ntox86 -nostdlib
else
OS_RELEASE	:= $(shell uname -v | sed 's/^\([0-9]\)\([0-9]*\)$$/\1.\2/')
LD		:= $(CC)
endif
OS_TEST		:= x86
endif
ifeq ($(OS_ARCH),SCO_SV)
OS_ARCH		:= SCOOS
OS_RELEASE	:= 5.0
endif
ifneq (,$(filter SINIX-N SINIX-Y SINIX-Z ReliantUNIX-M,$(OS_ARCH)))
OS_ARCH		:= SINIX
OS_TEST		:= $(shell uname -p)
endif
ifeq ($(OS_ARCH),UnixWare)
OS_ARCH		:= UNIXWARE
OS_RELEASE	:= $(shell uname -v)
endif
ifeq ($(OS_ARCH),OS_2)
OS_ARCH		:= OS2
OS_RELEASE	:= $(shell uname -v)
endif
ifeq ($(OS_ARCH),BeOS)
BEOS_ADDON_WORKAROUND	= 1
endif

#
# Strip off the excessively long version numbers on these platforms,
# but save the version to allow multiple versions of the same base
# platform to be built in the same tree.
#
ifneq (,$(filter FreeBSD HP-UX IRIX Linux NetBSD OpenBSD OSF1 SunOS,$(OS_ARCH)))
OS_RELEASE	:= $(basename $(OS_RELEASE))

# Allow the user to ignore the OS_VERSION, which is usually irrelevant.
ifdef WANT_MOZILLA_CONFIG_OS_VERSION
OS_VERS		:= $(suffix $(OS_RELEASE))
OS_VERSION	:= $(shell echo $(OS_VERS) | sed 's/-.*//')
endif

endif

OS_CONFIG	:= $(OS_ARCH)$(OS_RELEASE)

FINAL_LINK_LIBS = $(DEPTH)/config/final-link-libs
FINAL_LINK_COMPS = $(DEPTH)/config/final-link-comps
FINAL_LINK_COMP_NAMES = $(DEPTH)/config/final-link-comp-names

# 
# NSS libs needed for final link in static build
# 

ifneq (,$(filter OS2 WINNT,$(OS_ARCH)))
NSS_LIBS	= \
	$(DIST)/lib/$(LIB_PREFIX)crmf.$(LIB_SUFFIX) \
	$(DIST)/lib/$(LIB_PREFIX)smime3.$(LIB_SUFFIX) \
	$(DIST)/lib/$(LIB_PREFIX)ssl3.$(LIB_SUFFIX) \
	$(DIST)/lib/$(LIB_PREFIX)nss3.$(LIB_SUFFIX) \
	$(DIST)/lib/$(LIB_PREFIX)softokn3.$(LIB_SUFFIX) \
	$(NULL)
else
NSS_LIBS	= \
	$(LIBS_DIR) \
	$(DIST)/lib/$(LIB_PREFIX)crmf.$(LIB_SUFFIX) \
	-lsmime3 \
	-lssl3 \
	-lnss3 \
	-lsoftokn3 \
	$(NULL)
endif

NSS_DEP_LIBS	= \
	$(DIST)/lib/$(LIB_PREFIX)crmf.$(LIB_SUFFIX) \
	$(DIST)/lib/$(LIB_PREFIX)smime3$(DLL_SUFFIX) \
	$(DIST)/lib/$(LIB_PREFIX)ssl3$(DLL_SUFFIX) \
	$(DIST)/lib/$(LIB_PREFIX)nss3$(DLL_SUFFIX) \
	$(DIST)/lib/$(LIB_PREFIX)softokn3$(DLL_SUFFIX) \
	$(NULL)

MOZ_UNICHARUTIL_LIBS = $(DIST)/lib/$(LIB_PREFIX)unicharutil_s.$(LIB_SUFFIX)
MOZ_REGISTRY_LIBS          = $(DIST)/lib/$(LIB_PREFIX)mozreg_s.$(LIB_SUFFIX)
MOZ_WIDGET_SUPPORT_LIBS    = $(DIST)/lib/$(LIB_PREFIX)widgetsupport_s.$(LIB_SUFFIX)

# determine debug-related options
_DEBUG_CFLAGS :=
_DEBUG_LDFLAGS :=

ifndef MOZ_DEBUG
  # global debugging is disabled 
  # check if it was explicitly enabled for this module
  ifneq (, $(findstring $(MODULE), $(MOZ_DEBUG_MODULES)))
    MOZ_DEBUG:=1
  endif
else
  # global debugging is enabled
  # check if it was explicitly disabled for this module
  ifneq (, $(findstring ^$(MODULE), $(MOZ_DEBUG_MODULES)))
    MOZ_DEBUG:=
  endif
endif

ifdef MOZ_DEBUG
  _DEBUG_CFLAGS += $(MOZ_DEBUG_ENABLE_DEFS)
else
  _DEBUG_CFLAGS += $(MOZ_DEBUG_DISABLE_DEFS)
endif

# determine if -g should be passed to the compiler, based on
# the current module, and the value of MOZ_DBGRINFO_MODULES

ifdef MOZ_DEBUG
  MOZ_DBGRINFO_MODULES += ALL_MODULES
  pattern := ALL_MODULES ^ALL_MODULES
else
  MOZ_DBGRINFO_MODULES += ^ALL_MODULES
  pattern := ALL_MODULES ^ALL_MODULES
endif

ifdef MODULE
  # our current Makefile specifies a module name - add it to our pattern
  pattern += $(MODULE) ^$(MODULE)
endif

# start by finding the first relevant module name 
# (remember that the order of the module names in MOZ_DBGRINFO_MODULES 
# is reversed from the order the user specified to configure - 
# this allows the user to put general names at the beginning
# of the list, and to override them with explicit module names later 
# in the list)

first_match:=$(firstword $(filter $(pattern), $(MOZ_DBGRINFO_MODULES)))

ifeq ($(first_match), $(MODULE))
  # the user specified explicitly that 
  # this module should be compiled with -g
  _DEBUG_CFLAGS += $(MOZ_DEBUG_FLAGS)
  _DEBUG_LDFLAGS += $(MOZ_DEBUG_LDFLAGS)
else
  ifeq ($(first_match), ^$(MODULE))
    # the user specified explicitly that this module 
    # should not be compiled with -g (nothing to do)
  else
    ifeq ($(first_match), ALL_MODULES)
      # the user didn't mention this module explicitly, 
      # but wanted all modules to be compiled with -g
      _DEBUG_CFLAGS += $(MOZ_DEBUG_FLAGS)
      _DEBUG_LDFLAGS += $(MOZ_DEBUG_LDFLAGS)      
    else
      ifeq ($(first_match), ^ALL_MODULES)
        # the user didn't mention this module explicitly, 
        # but wanted all modules to be compiled without -g (nothing to do)
      endif
    endif
  endif
endif


# append debug flags 
# (these might have been above when processing MOZ_DBGRINFO_MODULES)
OS_CFLAGS += $(_DEBUG_CFLAGS)
OS_CXXFLAGS += $(_DEBUG_CFLAGS)
OS_LDFLAGS += $(_DEBUG_LDFLAGS)

# MOZ_PROFILE & MOZ_COVERAGE equivs for win32
ifeq ($(OS_ARCH),WINNT)
ifdef MOZ_DEBUG
ifneq (,$(MOZ_BROWSE_INFO)$(MOZ_BSCFILE))
OS_CFLAGS += /FR
OS_CXXFLAGS += /FR
endif
else
# if MOZ_DEBUG is not set and MOZ_PROFILE is set, then we generate
# an optimized build with debugging symbols. Useful for debugging
# compiler optimization bugs, as well as running with Quantify.
ifdef MOZ_PROFILE
MOZ_OPTIMIZE_FLAGS=-Zi -O1 -UDEBUG -DNDEBUG
OS_LDFLAGS = /DEBUG /DEBUGTYPE:CV /PDB:NONE /OPT:REF /OPT:nowin98
WIN32_EXE_LDFLAGS=/FIXED:NO
endif

# if MOZ_COVERAGE is set, we handle pdb files slightly differently
ifdef MOZ_COVERAGE
MOZ_OPTIMIZE_FLAGS=-Zi -O1 -UDEBUG -DNDEBUG
OS_LDFLAGS = /DEBUG /DEBUGTYPE:CV /PDB:NONE /OPT:REF /OPT:nowin98
_ORDERFILE := $(wildcard $(srcdir)/win32.order)
ifneq (,$(_ORDERFILE))
OS_LDFLAGS += /ORDER:@$(srcdir)/win32.order
endif
endif
# MOZ_COVERAGE

#
# Handle trace-malloc in optimized builds.
# No opt to give sane callstacks.
#
ifdef NS_TRACE_MALLOC
MOZ_OPTIMIZE_FLAGS=-Zi -Od -UDEBUG -DNDEBUG
OS_LDFLAGS = /DEBUG /DEBUGTYPE:CV /PDB:NONE /OPT:REF /OPT:nowin98
endif
# NS_TRACE_MALLOC

endif # MOZ_DEBUG
endif # WINNT


#
# -ffunction-sections is needed to reorder functions using a GNU ld
# script.
#
ifeq ($(MOZ_REORDER),1)
  OS_CFLAGS += -ffunction-sections
  OS_CXXFLAGS += -ffunction-sections
endif

#
# List known meta modules and their dependent libs
#
_ALL_META_COMPONENTS=mail crypto

MOZ_META_COMPONENTS_mail = nsMsgBaseModule IMAP_factory nsVCardModule mime_services nsMimeEmitterModule nsMsgNewsModule  nsMsgComposeModule local_mail_services nsAbSyncModule nsImportServiceModule nsTextImportModule nsAbModule nsMsgDBModule nsMsgMdnModule nsComm4xMailImportModule
MOZ_META_COMPONENTS_mail_comps = msgimap mime msgnews import addrbook impText vcard msgdb msgmdn
MOZ_META_COMPONENTS_mail_libs = mimecthglue_s

ifeq ($(OS_ARCH),WINNT)
MOZ_META_COMPONENTS_mail += nsEudoraImportModule nsOEImport nsOutlookImport msgMapiModule
MOZ_META_COMPONENTS_mail_comps += msgbase impEudra importOE impOutlk msgMapi 
else
MOZ_META_COMPONENTS_mail_comps += mailnews
endif

ifdef USE_SHORT_LIBNAME
MOZ_META_COMPONENTS_mail_comps += emitter msgcompo msglocal absyncsv
ifeq ($(OS_ARCH),WINNT)
MOZ_META_COMPONENTS_mail_comps += impComm4xMail
else
MOZ_META_COMPONENTS_mail_comps += imp4Mail
endif
MOZ_META_COMPONENTS_mail_libs += msgbsutl
else
MOZ_META_COMPONENTS_mail_cops += mimeemitter msgcompose localmail absyncsvc impComm4xMail
MOZ_META_COMPONENTS_mail_libs += msgbaseutil
endif

ifdef MOZ_PSM
MOZ_META_COMPONENTS_mail += nsMsgSMIMEModule
MOZ_META_COMPONENTS_mail_comps += msgsmime
else
MOZ_META_COMPONENTS_mail +=  nsSMIMEModule
MOZ_META_COMPONENTS_mail_comps += smimestb
endif

MOZ_META_COMPONENTS_crypto = BOOT PKI NSS
MOZ_META_COMPONENTS_crypto_comps = pipboot pippki pipnss

#
# Build using PIC by default
# Do not use PIC if not building a shared lib (see exceptions below)
#
ifneq (,$(BUILD_SHARED_LIBS)$(FORCE_SHARED_LIB)$(FORCE_USE_PIC))
_ENABLE_PIC=1
endif

# If module is going to be merged into the nsStaticModule, 
# make sure that the entry points are translated and 
# the module is built static.

ifdef IS_COMPONENT
ifneq (,$(MOZ_STATIC_COMPONENT_LIBS)$(findstring $(LIBRARY_NAME), $(MOZ_STATIC_COMPONENTS)))
ifdef MODULE_NAME
DEFINES += -DXPCOM_TRANSLATE_NSGM_ENTRY_POINT=1
FORCE_STATIC_LIB=1
endif
endif
endif

# Determine if module being compiled is destined 
# to be merged into a meta module in the future

ifneq (, $(findstring $(META_COMPONENT), $(MOZ_META_COMPONENTS)))
ifdef IS_COMPONENT
ifdef MODULE_NAME
DEFINES += -DXPCOM_TRANSLATE_NSGM_ENTRY_POINT=1
endif
endif
EXPORT_LIBRARY=
FORCE_STATIC_LIB=1
_ENABLE_PIC=1
endif

#
# Disable PIC if necessary
#

ifndef _ENABLE_PIC
DSO_CFLAGS=
ifeq ($(OS_ARCH)_$(HAVE_GCC3_ABI),Darwin_1)
DSO_PIC_CFLAGS=-mdynamic-no-pic
else
DSO_PIC_CFLAGS=
endif
MKSHLIB=
endif

# Force _all_ exported methods to be |_declspec(dllexport)| when we're
# building them into the executable.
ifeq ($(OS_ARCH),WINNT)
ifdef MOZ_STATIC_COMPONENT_LIBS
DEFINES	+= \
	-D_IMPL_NS_GFX \
	-D_IMPL_NS_MSG_BASE \
	-D_IMPL_NS_PICS \
	-D_IMPL_NS_WIDGET \
	$(NULL)
endif
endif


#
# Personal makefile customizations go in these optional make include files.
#
MY_CONFIG	:= $(DEPTH)/config/myconfig.mk
MY_RULES	:= $(DEPTH)/config/myrules.mk

#
# Relative pathname from top-of-tree to current source directory
#
ifneq ($(OS_ARCH),OS2)
REVDEPTH	= $(CONFIG_TOOLS)/revdepth
endif

#
# Provide the means to easily override our tool directory locations.
#
ifdef NETSCAPE_HIERARCHY
CONTRIB_BIN	:= /tools/contrib/bin/
JAVA_BIN	:= /usr/local/java/bin/
LOCAL_BIN	:= /usr/local/bin/
LOCAL_SUN4	:= /usr/local/sun4/bin/
NS_BIN		:= /tools/ns/bin/
NS_LIB		:= /tools/ns/lib
JAVA_LIB	:= /usr/local/netscape/java/lib
else
NS_LIB		:= .
JAVA_LIB	:= .
endif

# Allow NETSCAPE_COMMERCIAL to include XFEPRIVDIR
ifdef NETSCAPE_COMMERCIAL
XFEPRIVDIR		:= $(DEPTH)/../ns/cmd/xfe/
endif

#
# Default command macros; can be overridden in <arch>.mk.
#
CCC		= $(CXX)
CCF		= $(CC) $(CFLAGS)
LINK_EXE	= $(LINK) $(OS_LFLAGS) $(LFLAGS)
LINK_DLL	= $(LINK) $(OS_DLLFLAGS) $(DLLFLAGS)
NFSPWD		= $(CONFIG_TOOLS)/nfspwd
PURIFY		= purify $(PURIFYOPTIONS)
QUANTIFY	= quantify $(QUANTIFYOPTIONS)
ifdef CROSS_COMPILE
XPIDL_COMPILE 	= $(DIST)/host/bin/host_xpidl$(BIN_SUFFIX)
XPIDL_LINK	= $(DIST)/host/bin/host_xpt_link$(BIN_SUFFIX)
else
XPIDL_COMPILE 	= $(DIST)/bin/xpidl$(BIN_SUFFIX)
XPIDL_LINK	= $(DIST)/bin/xpt_link$(BIN_SUFFIX)
endif
MIDL		= midl

ifeq ($(OS_ARCH),OS2)
PATH_SEPARATOR	:= \;
else
PATH_SEPARATOR	:= :
ifeq ($(AWT_11),1)
JAVA_PROG	= $(NS_BIN)java
JAVAC_ZIP	= $(NS_LIB)/classes.zip
else
JAVA_PROG	= $(LOCAL_BIN)java
ifdef JDKHOME
JAVAC_ZIP	= $(JAVA_LIB)/classes.zip
else
JAVAC_ZIP	= $(JAVA_LIB)/javac.zip
endif
endif
TAR		= tar
endif # OS2

ifeq ($(OS_ARCH),OpenVMS)
include $(topsrcdir)/config/$(OS_ARCH).mk
endif

XBCFLAGS	=
ifdef MOZ_DEBUG
JAVA_OPTIMIZER	= -g
XBCFLAGS	= -FR$*
endif

REQ_INCLUDES	= $(foreach d,$(REQUIRES),-I$(DIST)/include/$d)

INCLUDES	= $(LOCAL_INCLUDES) $(REQ_INCLUDES) -I$(PUBLIC) -I$(DIST)/include $(OS_INCLUDES)

LIBNT		= $(DIST)/lib/libnt.$(LIB_SUFFIX)
LIBAWT		= $(DIST)/lib/libawt.$(LIB_SUFFIX)
LIBMMEDIA	= $(DIST)/lib/libmmedia.$(LIB_SUFFIX)

NSPRDIR		= nsprpub
LIBNSPR		= $(DIST)/lib/libplds3.$(LIB_SUFFIX) $(DIST)/lib/libnspr3.$(LIB_SUFFIX)
PURELIBNSPR	= $(DIST)/lib/purelibplds3.$(LIB_SUFFIX) $(DIST)/lib/purelibnspr3.$(LIB_SUFFIX)

ifdef DBMALLOC
LIBNSPR		+= $(DIST)/lib/libdbmalloc.$(LIB_SUFFIX)
endif

ifeq ($(OS_ARCH),OS2)
ifneq ($(MOZ_WIDGET_TOOLKIT), os2)
LIBNSJAVA	= $(DIST)/lib/jrt$(MOZ_BITS)$(VERSION_NUMBER).$(LIB_SUFFIX)
LIBMD		= $(DIST)/lib/libjmd.$(LIB_SUFFIX)
LIBJAVA		= $(DIST)/lib/libjrt.$(LIB_SUFFIX)
LIBNSPR		= $(DIST)/lib/pr$(MOZ_BITS)$(VERSION_NUMBER).$(LIB_SUFFIX)
LIBXP		= $(DIST)/lib/libxp.$(LIB_SUFFIX)
endif
else
LIBNSJAVA	= $(DIST)/lib/nsjava32.$(LIB_SUFFIX)
endif

CFLAGS		= $(OS_CFLAGS)
CXXFLAGS	= $(OS_CXXFLAGS)
LDFLAGS		= $(OS_LDFLAGS)

# Allow each module to override the *default* optimization settings
# by setting MODULE_OPTIMIZE_FLAGS iff the developer has not given
# arguments to --enable-optimize
ifdef MOZ_OPTIMIZE
ifeq (1,$(MOZ_OPTIMIZE))
ifdef MODULE_OPTIMIZE_FLAGS
CFLAGS		+= $(MODULE_OPTIMIZE_FLAGS)
CXXFLAGS	+= $(MODULE_OPTIMIZE_FLAGS)
else
CFLAGS		+= $(MOZ_OPTIMIZE_FLAGS)
CXXFLAGS	+= $(MOZ_OPTIMIZE_FLAGS)
endif # MODULE_OPTIMIZE_FLAGS
else
CFLAGS		+= $(MOZ_OPTIMIZE_FLAGS)
CXXFLAGS	+= $(MOZ_OPTIMIZE_FLAGS)
endif # MOZ_OPTIMIZE == 1
LDFLAGS		+= $(MOZ_OPTIMIZE_LDFLAGS)
endif # MOZ_OPTIMIZE

ifeq ($(MOZ_OS2_TOOLS),VACPP)
ifdef USE_STATIC_LIBS
RTL_FLAGS += /Gd-
else # !USE_STATIC_LIBS
RTL_FLAGS += /Gd+
endif
endif


ifeq ($(OS_ARCH),WINNT)
#// Currently, unless USE_STATIC_LIBS is defined, the multithreaded
#// DLL version of the RTL is used...
#//
#//------------------------------------------------------------------------
ifdef USE_STATIC_LIBS
RTL_FLAGS=-MT          # Statically linked multithreaded RTL
ifneq (,$(MOZ_DEBUG)$(NS_TRACE_MALLOC))
RTL_FLAGS=-MTd         # Statically linked multithreaded MSVC4.0 debug RTL
endif # MOZ_DEBUG || NS_TRACE_MALLOC

else # !USE_STATIC_LIBS

ifdef USE_NON_MT_LIBS
RTL_FLAGS=-ML          # Statically linked non-multithreaded LIBC RTL
ifneq (,$(MOZ_DEBUG)$(NS_TRACE_MALLOC))
RTL_FLAGS=-MLd         # Statically linked non-multithreaded LIBC debug RTL
endif # MOZ_DEBUG || NS_TRACE_MALLOC

else # ! USE_NON_MT_LIBS

RTL_FLAGS=-MD          # Dynamically linked, multithreaded RTL
ifneq (,$(MOZ_DEBUG)$(NS_TRACE_MALLOC))
ifndef MOZ_NO_DEBUG_RTL
RTL_FLAGS=-MDd         # Dynamically linked, multithreaded MSVC4.0 debug RTL
endif 
endif # MOZ_DEBUG || NS_TRACE_MALLOC
endif # USE_NON_MT_LIBS
endif # USE_STATIC_LIBS
endif # WINNT


COMPILE_CFLAGS	= $(DEFINES) $(INCLUDES) $(XCFLAGS) $(PROFILER_CFLAGS) $(DSO_CFLAGS) $(DSO_PIC_CFLAGS) $(CFLAGS) $(RTL_FLAGS) $(OS_COMPILE_CFLAGS)
COMPILE_CXXFLAGS = $(DEFINES) $(INCLUDES) $(XCFLAGS) $(PROFILER_CFLAGS) $(DSO_CFLAGS) $(DSO_PIC_CFLAGS)  $(CXXFLAGS) $(RTL_FLAGS) $(OS_COMPILE_CXXFLAGS)

#
# Some platforms (Solaris) might require builds using either
# (or both) compiler(s).
#
ifdef SHOW_CC_TYPE
COMPILER	= _$(notdir $(CC))
endif

#
# Name of the binary code directories
#
# Override defaults

# We need to know where to find the libraries we
# put on the link line for binaries, and should
# we link statically or dynamic?  Assuming dynamic for now.
ifneq (,$(filter-out WINNT OS2, $(OS_ARCH)))
LIBS_DIR	= -L$(DIST)/bin -L$(DIST)/lib
endif

# Default location of include files
IDL_DIR		= $(DIST)/idl
ifdef MODULE
PUBLIC		= $(DIST)/include/$(MODULE)
else
PUBLIC		= $(DIST)/include
endif

SDK_PUBLIC	= $(DIST)/sdk/$(MODULE)/include
SDK_IDL_DIR	= $(DIST)/sdk/$(MODULE)/idl
SDK_BIN_DIR	= $(DIST)/sdk/$(MODULE)/bin

DEPENDENCIES	= .md

MOZ_COMPONENT_LIBS=$(MOZ_COMPONENT_XPCOM_LIBS) $(MOZ_COMPONENT_NSPR_LIBS)

ifdef GC_LEAK_DETECTOR
MOZ_COMPONENT_XPCOM_LIBS += -lboehm
XPCOM_LIBS += -lboehm
endif

ifdef MOZ_DEMANGLE_SYMBOLS
MOZ_COMPONENT_XPCOM_LIBS += -liberty
XPCOM_LIBS += -liberty
endif

ifeq (xpconnect, $(findstring xpconnect, $(BUILD_MODULES)))
DEFINES +=  -DXPCONNECT_STANDALONE
endif

ifeq ($(OS_ARCH),OS2)
ELF_DYNSTR_GC	= echo
else
ELF_DYNSTR_GC	= :
endif

ifndef CROSS_COMPILE
ifdef USE_ELF_DYNSTR_GC
ifdef MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS
ELF_DYNSTR_GC 	= $(DIST)/bin/elf-dynstr-gc
endif
endif
endif

ifeq ($(OS_ARCH),Darwin)
ifdef USE_PREBINDING
export LD_PREBIND=1
export LD_SEG_ADDR_TABLE=$(shell cd $(topsrcdir); pwd)/config/prebind-address-table
endif
endif

ifdef MOZ_NATIVE_MAKEDEPEND
MKDEPEND_DIR	=
MKDEPEND	= $(MOZ_NATIVE_MAKEDEPEND)
else
MKDEPEND_DIR	= $(CONFIG_TOOLS)/mkdepend
MKDEPEND	= $(MKDEPEND_DIR)/mkdepend$(BIN_SUFFIX)
endif

# Set link flags according to whether we want a console.
ifdef MOZ_WINCONSOLE
ifeq ($(MOZ_WINCONSOLE),1)
ifeq ($(MOZ_OS2_TOOLS),EMX)
BIN_FLAGS	:= -Zlinker /PM:VIO -Zlinker /Stack:0x30000
endif
ifeq ($(OS_ARCH),WINNT)
WIN32_EXE_LDFLAGS	+= /SUBSYSTEM:CONSOLE
endif
else # MOZ_WINCONSOLE
ifeq ($(MOZ_OS2_TOOLS),VACPP)
LDFLAGS += /PM:PM
endif
ifeq ($(MOZ_OS2_TOOLS),EMX)
BIN_FLAGS	:= -Zlinker /PM:PM -Zlinker /Stack:0x30000
endif
ifeq ($(OS_ARCH),WINNT)
WIN32_EXE_LDFLAGS	+= /SUBSYSTEM:WINDOWS
endif
endif
endif

#
# Include any personal overrides the user might think are needed.
#
-include $(MY_CONFIG)

######################################################################
# Now test variables that might have been set or overridden by $(MY_CONFIG).

DEFINES		+= -DOSTYPE=\"$(OS_CONFIG)\"
DEFINES		+= -DOSARCH=\"$(OS_ARCH)\"

#
# Platform dependent switching off of JAVA
#
ifdef MOZ_JAVA
DEFINES		+= -DJAVA
ifdef MOZ_OJI
error You can't define both MOZ_JAVA and MOZ_OJI anymore. 
endif
JAVA_OR_OJI	= 1
JAVA_OR_NSJVM	= 1
endif

ifdef NSJVM
JAVA_OR_NSJVM	= 1
endif

ifdef MOZ_OJI
DEFINES		+= -DOJI
JAVA_OR_OJI	= 1
endif

ifdef JAVA_OR_NSJVM	# XXX fix -- su can't depend on java
MOZ_SMARTUPDATE	= 1
endif

ifdef FORTEZZA
DEFINES		+= -DFORTEZZA
endif

# For profiling
ifdef ENABLE_EAZEL_PROFILER
ifndef INTERNAL_TOOLS
ifneq ($(LIBRARY_NAME), xpt)
ifneq (, $(findstring $(shell $(topsrcdir)/build/unix/print-depth-path.sh | awk -F/ '{ print $$2; }'), $(MOZ_PROFILE_MODULES)))
PROFILER_CFLAGS	= $(EAZEL_PROFILER_CFLAGS) -DENABLE_EAZEL_PROFILER
PROFILER_LIBS	= $(EAZEL_PROFILER_LIBS)
endif
endif
endif
endif

######################################################################

GARBAGE		+= $(DEPENDENCIES) $(MKDEPENDENCIES) $(MKDEPENDENCIES).bak core $(wildcard core.[0-9]*) $(wildcard *.err) $(wildcard *.pure) $(wildcard *_pure_*.o) Templates.DB

ifeq (,$(filter-out WINNT OS2, $(OS_ARCH)))
ifeq ($(OS_ARCH),WINNT)
NSINSTALL	= $(CYGWIN_WRAPPER) $(MOZ_TOOLS_DIR)/bin/nsinstall
else
NSINSTALL	= nsinstall
endif
INSTALL		= $(NSINSTALL)
else
NSINSTALL	= $(CONFIG_TOOLS)/nsinstall

ifeq ($(NSDISTMODE),copy)
# copy files, but preserve source mtime
INSTALL		= $(NSINSTALL) -t
else
ifeq ($(NSDISTMODE),absolute_symlink)
# install using absolute symbolic links
INSTALL		= $(NSINSTALL) -L `$(NFSPWD)`
else
# install using relative symbolic links
INSTALL		= $(NSINSTALL) -R
endif
endif
endif # WINNT

# Use nsinstall in copy mode to install files on the system
SYSINSTALL	= $(NSINSTALL) -t

######################################################################
### Java Stuff - see common.mk
######################################################################

# where the bytecode will go
JAVA_DESTPATH	= $(DIST)/classes

# where the sources for the module you are compiling are
# default is sun-java/classsrc, override for other modules
ifndef JAVA_SOURCEPATH
JAVA_SOURCEPATH	= $(DEPTH)/sun-java/classsrc
endif

ifndef JAVAH_IN_JAVA
ifeq ($(MOZ_OS2_TOOLS),VACPP)
JAVAH_PROG	= flipper $(DIST)/bin/javah
else
JAVAH_PROG	= $(DIST)/bin/javah
endif
else
JAVAH_PROG	= $(JAVA) netscape.tools.jric.Main
endif

ifneq ($(JDKHOME),)
JAVAH_PROG	= $(JDKHOME)/bin/javah
JAVAC_PROG		= $(JDKHOME)/bin/javac $(JAVAC_FLAGS)
JAVAC 			= $(JAVAC_PROG)
endif

ifeq ($(STAND_ALONE_JAVA),1)
STAND_ALONE_JAVA_DLL_SUFFIX	= s
endif

ifeq ($(MOZ_OS2_TOOLS),OLD_IBM_BUILD) # These DLL names are no longer valid for OS/2
AWTDLL		= awt$(MOZ_BITS)$(VERSION_NUMBER).$(DLL_SUFFIX)
AWTSDLL		= awt$(MOZ_BITS)$(VERSION_NUMBER)$(STAND_ALONE_JAVA_DLL_SUFFIX).$(DLL_SUFFIX)
CONDLL		= con.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
JBNDLL		= jbn.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
JDBCDLL		= jdb.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
JITDLL		= jit.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
JPWDLL		= jpw.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
JRTDLL		= jrt$(MOZ_BITS)$(VERSION_NUMBER).$(DLL_SUFFIX)
JSJDLL		= jsj.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
MMDLL		= mm$(MOZ_BITS)$(VERSION_NUMBER).$(DLL_SUFFIX)
NETDLL		= net.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
NSCDLL		= nsc.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
ZIPDLL		= zip.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
ZPWDLL		= zpw.$(MOZ_BITS)$(VERSION_NUMBER)(DLL_SUFFIX)
else
AWTDLL		= libawt.$(DLL_SUFFIX)
AWTSDLL		= libawt$(STAND_ALONE_JAVA_DLL_SUFFIX).$(DLL_SUFFIX)
CONDLL		= libcon.$(DLL_SUFFIX)
JBNDLL		= libjbn.$(DLL_SUFFIX)
JDBCDLL		= libjdb.$(DLL_SUFFIX)
JITDLL		= libjit.$(DLL_SUFFIX)
JPWDLL		= libjpw.$(DLL_SUFFIX)
JRTDLL		= libjrt.$(DLL_SUFFIX)
JSJDLL		= libjsj.$(DLL_SUFFIX)
MMDLL		= libmm.$(DLL_SUFFIX)
NETDLL		= libnet.$(DLL_SUFFIX)
NSCDLL		= libnsc.$(DLL_SUFFIX)
ZIPDLL		= libzip.$(DLL_SUFFIX)
ZPWDLL		= libzpw.$(DLL_SUFFIX)
endif

JAVA_DEFINES	+= -DAWTSDLL=\"$(AWTSDLL)\" -DCONDLL=\"$(CONDLL)\" -DJBNDLL=\"$(JBNDLL)\" -DJDBDLL=\"$(JDBDLL)\" \
		   -DJSJDLL=\"$(JSJDLL)\" -DNETDLL=\"$(NETDLL)\" -DNSCDLL=\"$(NSCDLL)\" -DZPWDLL=\"$(ZPWDLL)\" \
		   -DJAR_NAME=\"$(JAR_NAME)\"

ifeq ($(AWT_11),1)
JAVA_DEFINES	+= -DAWT_11
else
JAVA_DEFINES	+= -DAWT_102
endif

#caca:
#	@echo $(PROFILER_CFLAGS)
