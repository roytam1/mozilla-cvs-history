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
################################################################################
#
# We now use a 3-pass build system.  This needs to be re-thought....
#
# Pass 1. export  - Create generated headers and stubs.  Publish public headers
#                   to dist/include.
#
# Pass 2. libs    - Create libraries.  Publish libraries to dist/lib.
#
# Pass 3. install - Create programs.  Publish them to dist/bin.
#
# 'gmake' will build each of these properly, but 'gmake -jN' will break (need
# to do each pass explicitly when using -j).
#
# Parameters to this makefile (set these before including):
#
# a)
#	TARGETS	-- the target to create
#			(defaults to $LIBRARY $PROGRAM)
# b)
#	DIRS	-- subdirectories for make to recurse on
#			(the 'all' rule builds $TARGETS $DIRS)
# c)
#	CSRCS, CPPSRCS -- .c and .cpp files to compile
#			(used to define $OBJS)
# d)
#	PROGRAM	-- the target program name to create from $OBJS
# d2)
#	SIMPLE_PROGRAMS	-- Compiles Foo.cpp Bar.cpp into Foo, Bar executables.
# e)
#	LIBRARY_NAME	-- the target library name to create from $OBJS
# f)
#	JSRCS	-- java source files to compile into class files
#			(if you don't specify this it will default to *.java)
# g)
#	PACKAGE	-- the package to put the .class files into
#			(e.g. netscape/applet)
# h)
#	JMC_EXPORT -- java files to be exported for use by JMC_GEN
#			(this is a list of Class names)
# i)
#	JRI_GEN	-- files to run through javah to generate headers and stubs
#			(output goes into the _jri sub-dir)
# j)
#	JNI_GEN	-- files to run through javah to generate headers and stubs
#			(output goes into the _jni sub-dir)
# k)
#	JMC_GEN	-- files to run through jmc to generate headers and stubs
#			(output goes into the _jmc sub-dir)
#
################################################################################
ifndef topsrcdir
topsrcdir		= $(DEPTH)
endif

#
# Common rules used by lots of makefiles...
#
ifndef NS_CONFIG_MK
include $(topsrcdir)/config/config.mk
endif

ifdef INTERNAL_TOOLS
ifdef CROSS_COMPILE
CC			= $(HOST_CC)
CXX			= $(HOST_CXX)
CFLAGS			= $(HOST_CFLAGS) -I$(DIST)/include $(NSPR_CFLAGS)
CXXFLAGS		= $(HOST_CXXFLAGS) -I$(DIST)/include $(NSPR_CFLAGS)
RANLIB			= $(HOST_RANLIB)
AR			= $(HOST_AR)
endif
endif

#
# Library rules
#
# If NO_STATIC_LIB is set, the static library will not be built.
# If NO_SHARED_LIB is set, the shared library will not be built.
#
ifndef LIBRARY
ifdef LIBRARY_NAME
LIBRARY			:= lib$(LIBRARY_NAME).$(LIB_SUFFIX)
endif
endif

ifdef LIBRARY
ifeq ($(OS_ARCH),OS2)
ifndef DEF_FILE
DEF_FILE		:= $(LIBRARY:.lib=.def)
endif
endif

ifndef NO_SHARED_LIB
ifdef MKSHLIB

ifeq ($(OS_ARCH),OS2)
SHARED_LIBRARY		:= $(LIBRARY:.lib=.dll)
MAPS			:= $(LIBRARY:.lib=.map)
else  # OS2
ifeq ($(OS_ARCH),WINNT)
SHARED_LIBRARY		:= $(LIBRARY:.lib=.dll)
else  # WINNT
ifeq ($(OS_ARCH),BeOS)
SHARED_LIBRARY		:= $(LIBRARY:.a=.$(DLL_SUFFIX))
else

# Unix only
ifdef LIB_IS_C_ONLY
MKSHLIB			= $(MKCSHLIB)
endif

ifeq ($(OS_ARCH),HP-UX)
SHARED_LIBRARY		:= $(LIBRARY:.a=.sl)
else  # HPUX
ifeq ($(OS_ARCH)$(OS_RELEASE),SunOS4.1)
SHARED_LIBRARY		:= $(LIBRARY:.a=.so.1.0)
else  # SunOS4
ifeq ($(OS_ARCH)$(OS_RELEASE),AIX4.1)
SHARED_LIBRARY		:= $(LIBRARY:.a=)_shr.a
else  # AIX
SHARED_LIBRARY		:= $(LIBRARY:.a=.$(DLL_SUFFIX))
endif # AIX
endif # SunOS4
endif # HPUX

endif # BeOS
endif # WINNT
endif # OS2
endif # MKSHLIB
endif # !NO_SHARED_LIB
endif

ifdef NO_STATIC_LIB
LIBRARY			= $(NULL)
endif

ifdef NO_SHARED_LIB
DLL_SUFFIX		= a
endif

ifndef TARGETS
TARGETS			= $(LIBRARY) $(SHARED_LIBRARY) $(PROGRAM) $(SIMPLE_PROGRAMS)
endif

ifndef OBJS
OBJS			= $(JRI_STUB_CFILES) $(addsuffix .o, $(JMC_GEN)) $(CSRCS:.c=.o) $(CPPSRCS:.cpp=.o) $(ASFILES:.s=.o)
endif

ifndef OS2_IMPLIB
LIBOBJS			:= $(addprefix \", $(OBJS))
LIBOBJS			:= $(addsuffix \", $(LIBOBJS))
endif

ifndef PACKAGE
PACKAGE			= .
endif

ifdef JAVA_OR_NSJVM
ALL_TRASH		= $(TARGETS) $(OBJS) LOGS TAGS $(GARBAGE) a.out \
			  $(NOSUCHFILE) $(JDK_HEADER_CFILES) $(JDK_STUB_CFILES) \
			  $(JRI_HEADER_CFILES) $(JRI_STUB_CFILES) $(JMC_STUBS) \
			  $(JMC_HEADERS) $(JMC_EXPORT_FILES) so_locations \
			  _gen _jmc _jri _stubs $(MDDEPDIR) \
                          $(wildcard gts_tmp_*) $(PROGOBJS) \
			  $(wildcard $(JAVA_DESTPATH)/$(PACKAGE)/*.class)
else
ALL_TRASH		= $(TARGETS) $(OBJS) LOGS TAGS $(GARBAGE) a.out \
			  $(NOSUCHFILE) $(JMC_STUBS) so_locations \
			  _gen _stubs $(MDDEPDIR) $(wildcard gts_tmp_*) \
                          $(PROGOBJS) $(LIBRARY:%.a=.%.timestamp)
endif

ifdef JAVA_OR_NSJVM
ifdef JDIRS
ALL_TRASH		+= $(addprefix $(JAVA_DESTPATH)/,$(JDIRS))
endif
endif

ifdef QTDIR
ALL_TRASH		+= $(MOCSRCS)
endif

ifdef JAVA_OR_NSJVM
JMC_SUBDIR              = _jmc
else
JMC_SUBDIR              = $(LOCAL_JMC_SUBDIR)
endif

JDK_GEN_DIR		= _gen
JMC_GEN_DIR		= $(JMC_SUBDIR)
JRI_GEN_DIR		= _jri
JNI_GEN_DIR		= _jni
JDK_STUB_DIR		= _stubs
XPIDL_GEN_DIR		= _xpidlgen

#
# If this is an "official" build, try to build everything.
# I.e., don't exit on errors.
#
ifdef BUILD_OFFICIAL
EXIT_ON_ERROR		= +e
CLICK_STOPWATCH		= date
else
EXIT_ON_ERROR		= -e
CLICK_STOPWATCH		= true
endif

ifdef MOZ_UPDATE_XTERM
UPDATE_TITLE		= echo "]2;gmake: $@ in $(shell echo $(srcdir) | sed 's:\.\./::g')/$$d"
else
UPDATE_TITLE		= true
endif

ifdef DIRS
LOOP_OVER_DIRS		=					\
	@for d in $(DIRS); do					\
		$(UPDATE_TITLE);				\
		set $(EXIT_ON_ERROR);				\
		echo "cd $$d; $(MAKE) $@";			\
		oldDir=`pwd`;					\
		cd $$d; $(MAKE) $@; cd $$oldDir;		\
		set +e;						\
		$(CLICK_STOPWATCH);				\
	done
endif

#
# Now we can differentiate between objects used to build a library, and
# objects used to build an executable in the same directory.
#
ifndef PROGOBJS
PROGOBJS		= $(OBJS)
endif

# SUBMAKEFILES: List of Makefiles for next level down.
#   This is used to update or create the Makefiles before invoking them.
ifneq ($(DIRS),)
SUBMAKEFILES		:= $(addsuffix /Makefile, $(filter-out $(STATIC_MAKEFILES), $(DIRS)))
endif

# MAKE_DIRS: List of directories to build while looping over directories.
MAKE_DIRS		=

ifdef COMPILER_DEPEND
ifdef OBJS
MAKE_DIRS		+= $(MDDEPDIR)
endif
endif

ifneq ($(XPIDLSRCS),)
MAKE_DIRS		+= $(XPIDL_GEN_DIR)
endif

#
# Tags: emacs (etags), vi (ctags)
# TAG_PROGRAM := ctags -L -
#
TAG_PROGRAM		= xargs etags -a

#
# Turn on C++ linking if we have any .cpp files
# (moved this from config.mk so that config.mk can be included 
#  before the CPPSRCS are defined)
#
ifdef CPPSRCS
CPP_PROG_LINK		= 1
endif

#
# This will strip out symbols that the component shouldnt be 
# exporting from the .dynsym section.
#
ifdef IS_COMPONENT
EXTRA_DSO_LDOPTS += $(MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS)
EXTRA_DSO_LDOPTS += $(MOZ_COMPONENT_LIBS)
endif # IS_COMPONENT

#
# BeOS specific section: link against dependent shared libs
#
ifeq ($(OS_ARCH),BeOS)
ifdef SHARED_LIBRARY
BEOS_LIB_LIST		= $(shell cat $(topsrcdir)/dependencies.beos/$(LIBRARY_NAME).dependencies)
BEOS_LINK_LIBS		= $(foreach lib,$(BEOS_LIB_LIST),$(shell $(topsrcdir)/config/beos/checklib.sh $(DIST)/bin $(lib)))
LDFLAGS			+= -L$(DIST)/bin $(BEOS_LINK_LIBS) $(NSPR_LIBS)
EXTRA_DSO_LDOPTS	+= -L$(DIST)/bin $(BEOS_LINK_LIBS) $(NSPR_LIBS)
endif
endif

#
# AIX & OpenVMS specific section: link against dependent shared libs
#
ifdef SHARED_LIBRARY
ifeq ($(OS_ARCH),AIX)
EXTRA_DSO_LDOPTS	+= -L$(DIST)/bin $(NSPR_LIBS)
endif
ifeq ($(OS_ARCH),OpenVMS)
EXTRA_DSO_LDOPTS	+= -L$(DIST)/bin $(NSPR_LIBS)
endif
endif

################################################################################

all:: export libs install

# Do depend as well
alldep:: export depend libs install

# Do everything from scratch
everything:: realclean alldep

ifdef ALL_PLATFORMS
all_platforms:: $(NFSPWD)
	@d=`$(NFSPWD)`;							\
	if test ! -d LOGS; then rm -rf LOGS; mkdir LOGS; else true; fi;	\
	for h in $(PLATFORM_HOSTS); do					\
		echo "On $$h: $(MAKE) $(ALL_PLATFORMS) >& LOGS/$$h.log";\
		rsh $$h -n "(chdir $$d;					\
			     $(MAKE) $(ALL_PLATFORMS) >& LOGS/$$h.log;	\
			     echo DONE) &" 2>&1 > LOGS/$$h.pid &	\
		sleep 1;						\
	done

$(NFSPWD):
	cd $(@D); $(MAKE) $(@F)
endif

# Target to only regenerate makefiles
makefiles: $(SUBMAKEFILES)
ifdef DIRS
	@for d in $(filter-out $(STATIC_MAKEFILES), $(DIRS)); do\
		$(UPDATE_TITLE);				\
		oldDir=`pwd`;					\
		echo "cd $$d; $(MAKE) $@";			\
		cd $$d; $(MAKE) $@; cd $$oldDir;		\
	done
endif

export:: $(SUBMAKEFILES) $(MAKE_DIRS)
	+$(LOOP_OVER_DIRS)

ifndef LIBS_NEQ_INSTALL
libs install:: $(SUBMAKEFILES) $(MAKE_DIRS) $(LIBRARY) $(SHARED_LIBRARY) $(PROGRAM) $(SIMPLE_PROGRAMS) $(MAPS)
ifndef NO_STATIC_LIB
ifdef LIBRARY
ifdef IS_COMPONENT
	$(INSTALL) -m 444 $(LIBRARY) $(DIST)/lib/components
else
	$(INSTALL) -m 444 $(LIBRARY) $(DIST)/lib
endif
endif
endif
ifdef MAPS
	$(INSTALL) -m 444 $(MAPS) $(DIST)/bin
endif
ifdef SHARED_LIBRARY
ifdef IS_COMPONENT
	$(INSTALL) -m 555 $(SHARED_LIBRARY) $(DIST)/lib/components
	$(INSTALL) -m 555 $(SHARED_LIBRARY) $(DIST)/bin/components
ifeq ($(OS_ARCH),OpenVMS)
	$(INSTALL) -m 555 $(SHARED_LIBRARY:.$(DLL_SUFFIX)=.vms) $(DIST)/bin/components
endif
else
	$(INSTALL) -m 555 $(SHARED_LIBRARY) $(DIST)/lib
	$(INSTALL) -m 555 $(SHARED_LIBRARY) $(DIST)/bin
ifeq ($(OS_ARCH),OpenVMS)
	$(INSTALL) -m 555 $(SHARED_LIBRARY:.$(DLL_SUFFIX)=.vms) $(DIST)/bin
endif
endif
endif
ifdef PROGRAM
	$(INSTALL) -m 555 $(PROGRAM) $(DIST)/bin
endif
ifdef SIMPLE_PROGRAMS
	$(INSTALL) -m 555 $(SIMPLE_PROGRAMS) $(DIST)/bin
endif
	+$(LOOP_OVER_DIRS)
else
libs:: $(SUBMAKEFILES) $(MAKE_DIRS) $(LIBRARY) $(SHARED_LIBRARY) $(SHARED_LIBRARY_LIBS)
ifndef NO_STATIC_LIB
ifdef LIBRARY
ifdef IS_COMPONENT
	$(INSTALL) -m 444 $(LIBRARY) $(DIST)/lib/components
else
	$(INSTALL) -m 444 $(LIBRARY) $(DIST)/lib
endif
endif
endif
ifdef SHARED_LIBRARY
ifdef IS_COMPONENT
	$(INSTALL) -m 555 $(SHARED_LIBRARY) $(DIST)/lib/components
	$(INSTALL) -m 555 $(SHARED_LIBRARY) $(DIST)/bin/components
ifeq ($(OS_ARCH),OpenVMS)
	$(INSTALL) -m 555 $(SHARED_LIBRARY:.$(DLL_SUFFIX)=.vms) $(DIST)/bin/components
endif
else
	$(INSTALL) -m 555 $(SHARED_LIBRARY) $(DIST)/lib
	$(INSTALL) -m 555 $(SHARED_LIBRARY) $(DIST)/bin
ifeq ($(OS_ARCH),OpenVMS)
	$(INSTALL) -m 555 $(SHARED_LIBRARY:.$(DLL_SUFFIX)=.vms) $(DIST)/bin
endif
endif
endif
	+$(LOOP_OVER_DIRS)

install:: $(SUBMAKEFILES) $(PROGRAM) $(SIMPLE_PROGRAMS)
ifdef MAPS
	$(INSTALL) -m 444 $(MAPS) $(DIST)/bin
endif
ifdef PROGRAM
	$(INSTALL) -m 555 $(PROGRAM) $(DIST)/bin
ifeq ($(OS_ARCH),BeOS)
	-rm components add-ons lib
	ln -sf $(DIST)/bin/components components
	ln -sf $(DIST)/bin add-ons
	ln -sf $(DIST)/bin lib
endif
endif
ifdef SIMPLE_PROGRAMS
	$(INSTALL) -m 555 $(SIMPLE_PROGRAMS) $(DIST)/bin
ifeq ($(OS_ARCH),BeOS)
	-rm components add-ons lib
	ln -sf $(DIST)/bin/components components
	ln -sf $(DIST)/bin add-ons
	ln -sf $(DIST)/bin lib
endif
endif
	+$(LOOP_OVER_DIRS)
endif

checkout:
	cd $(topsrcdir); $(MAKE) -f client.mk checkout

run_viewer: $(DIST)/bin/viewer
	cd $(DIST)/bin; \
	MOZILLA_FIVE_HOME=`pwd` \
	LD_LIBRARY_PATH=".:$(LIBS_PATH):$$LD_LIBRARY_PATH" \
	viewer

run_apprunner: $(DIST)/bin/apprunner
	cd $(DIST)/bin; \
	MOZILLA_FIVE_HOME=`pwd` \
	LD_LIBRARY_PATH=".:$(LIBS_PATH):$$LD_LIBRARY_PATH" \
	apprunner

clean clobber:: $(SUBMAKEFILES)
	rm -rf $(ALL_TRASH)
	+$(LOOP_OVER_DIRS)

realclean clobber_all:: $(SUBMAKEFILES)
	rm -rf $(wildcard *.OBJ) dist $(ALL_TRASH)
	+$(LOOP_OVER_DIRS)

distclean:: $(SUBMAKEFILES)
	rm -rf $(wildcard *.OBJ) dist $(ALL_TRASH) $(wildcard *.map) \
	Makefile config.log config.cache depend.mk .md .deps .HSancillary _xpidlgen
	+$(LOOP_OVER_DIRS)

alltags:
	rm -f TAGS
	find $(topsrcdir) -name dist -prune -o \( -name '*.[hc]' -o -name '*.cp' -o -name '*.cpp' -o -name '*.idl' \) -print | $(TAG_PROGRAM)

#
# PROGRAM = Foo
# creates OBJS, links with LIBS to create Foo
#
$(PROGRAM): $(PROGOBJS) $(EXTRA_DEPS) Makefile Makefile.in
ifeq ($(OS_ARCH),OS2)
	$(LINK) -FREE -OUT:$@ $(LDFLAGS) $(OS_LFLAGS) $(PROGOBJS)  $(EXTRA_LIBS) -MAP:$(@:.exe=.map) $(OS_LIBS) $(DEF_FILE)
else
ifeq ($(OS_ARCH),WINNT)
	$(CC) $(PROGOBJS) -Fe$@ -link $(LDFLAGS) $(OS_LIBS) $(EXTRA_LIBS)
else
ifeq ($(CPP_PROG_LINK),1)
	$(CCC) $(WRAP_MALLOC_CFLAGS) -o $@ $(PROGOBJS) $(LDFLAGS) $(LIBS_DIR) $(LIBS) $(OS_LIBS) $(EXTRA_LIBS) $(WRAP_MALLOC_LIB)
	$(MOZ_POST_PROGRAM_COMMAND) $@
ifeq ($(OS_ARCH),BeOS)
ifdef BEOS_PROGRAM_RESOURCE
	xres -o $@ $(BEOS_PROGRAM_RESOURCE)
	mimeset $@
endif
endif
else
	$(CC) -o $@ $(PROGOBJS) $(LDFLAGS) $(LIBS_DIR) $(LIBS) $(OS_LIBS) $(EXTRA_LIBS)
ifeq ($(OS_ARCH),BeOS)
ifdef BEOS_PROGRAM_RESOURCE
	xres -o $@ $(BEOS_PROGRAM_RESOURCE)
	mimeset $@
endif
endif
endif
endif
endif

#
# This is an attempt to support generation of multiple binaries
# in one directory, it assumes everything to compile Foo is in
# Foo.o (from either Foo.c or Foo.cpp).
#
# SIMPLE_PROGRAMS = Foo Bar
# creates Foo.o Bar.o, links with LIBS to create Foo, Bar.
#
$(SIMPLE_PROGRAMS):%: %.o $(EXTRA_DEPS) Makefile Makefile.in
ifeq ($(CPP_PROG_LINK),1)
	$(CCC) $(WRAP_MALLOC_CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS_DIR) $(LIBS) $(OS_LIBS) $(EXTRA_LIBS) $(WRAP_MALLOC_LIB)
	$(MOZ_POST_PROGRAM_COMMAND) $@
else
	$(CC) -o $@ $< $(LDFLAGS) $(LIBS_DIR) $(LIBS) $(OS_LIBS) $(EXTRA_LIBS) $(WRAP_MALLOC_LIB)
	$(MOZ_POST_PROGRAM_COMMAND) $@
endif

#
# Purify target.  Solaris/sparc only to start.
# Purify does not recognize "egcs" or "c++" so we go with 
# "gcc" and "g++" for now.
#
pure:	$(PROGRAM)
ifeq ($(CPP_PROG_LINK),1)
	$(PURIFY) $(CCC) -o $^.pure $(PROGOBJS) $(LDFLAGS) $(LIBS_DIR) $(LIBS) $(OS_LIBS) $(EXTRA_LIBS)
else
	$(PURIFY) $(CC) -o $^.pure $(PROGOBJS) $(LDFLAGS) $(LIBS_DIR) $(LIBS) $(OS_LIBS) $(EXTRA_LIBS)
endif
	$(INSTALL) -m 555 $^.pure $(DIST)/bin

quantify: $(PROGRAM)
ifeq ($(CPP_PROG_LINK),1)
	$(QUANTIFY) $(CCC) -o $^.quantify $(PROGOBJS) $(LDFLAGS) $(LIBS_DIR) $(LIBS) $(OS_LIBS) $(EXTRA_LIBS)
else
	$(QUANTIFY) $(CC) -o $^.quantify $(PROGOBJS) $(LDFLAGS) $(LIBS_DIR) $(LIBS) $(OS_LIBS) $(EXTRA_LIBS)
endif
	$(INSTALL) -m 555 $^.quantify $(DIST)/bin

ifneq ($(OS_ARCH),OS2)
#
# This allows us to create static versions of the shared libraries
# that are built using other static libraries.  Confused...?
#
ifdef SHARED_LIBRARY_LIBS
ifneq (,$(filter OSF1 BSD_OS FreeBSD NetBSD OpenBSD SunOS,$(OS_ARCH)))
CLEANUP1	:= | egrep -v '(________64ELEL_|__.SYMDEF)'
CLEANUP2	:= rm -f ________64ELEL_ __.SYMDEF
else
CLEANUP2	:= true
endif
SUB_LOBJS	= $(shell for lib in $(SHARED_LIBRARY_LIBS); do $(AR_LIST) $${lib} $(CLEANUP1); done;)
endif

ifdef USE_IMPLICIT_ARCHIVE
# This rule overrides the builtin rule that tells make how to insert object
# files into archive libraries.  A few notes about what is going on:
# The whole point of doing this is to avoid having to store the .o files in
# addition to the .a files, but not have regenerate all the .o files every time
# we regenerate the .a file.  Make knows how to do this by itself.  However,
# it wants to insert the .o files into the .a file 1 at a time which is both
# slow and also breaks parallel builds.  These rules get make to build all
# the .o files ahead of time and insert them all at once.  We use the
# .timestamp file to make this work reliably.
#
# I have learned that $(shell) and $(wildcard) variables do not necessarily
# get evaluated when they are used.  All the $(shell) variables for a rule
# seem to get evaluated right before the rules commands are executed, not
# as we proceed from command to the next.  This means that the "ar t" is
# evaluating the library before we insert new objects.
#
ifndef SHARED_LIBRARY_LIBS
$(LIBRARY)(%.o): %.o
	@echo 'queueing $< for insertion into $(LIBRARY)'; \
	touch $(LIBRARY:%.a=.%.timestamp)

$(LIBRARY:%.a=.%.timestamp) :
	@touch $@

ifneq (, $(LIBRARY))
AR_FILES_TO_REMOVE := $(filter-out \
	$(filter %.o, $(LIBRARY)($(OBJS) $(LOBJS)) ), \
	$(shell [ -f $(LIBRARY) ] && $(AR_LIST) $(LIBRARY)))
endif

$(LIBRARY): $(LIBRARY)($(OBJS) $(LOBJS)) $(LIBRARY:%.a=.%.timestamp) Makefile.in Makefile
	@touch $(LIBRARY:%.a=.%.timestamp)
	$(AR) $(AR_FLAGS) $(filter $filter(%.o, $^), $(shell echo *.o))
	if [ "${AR_FILES_TO_REMOVE}" != "" ]; then $(AR_DELETE) $(LIBRARY) $(AR_FILES_TO_REMOVE); else true; fi
	@touch $@

else

$(LIBRARY): $(OBJS) $(LOBJS) Makefile Makefile.in
	rm -f $@
	@rm -f $(SUB_LOBJS)
	@for lib in $(SHARED_LIBRARY_LIBS); do $(AR_EXTRACT) $${lib}; $(CLEANUP2); done
	$(AR) $(AR_FLAGS) $(OBJS) $(LOBJS) $(SUB_LOBJS)
	$(RANLIB) $@
	@rm -f foodummyfilefoo $(SUB_LOBJS)
endif # ! SHARED_LIBRARY_LIBS

else # ! USE_IMPLICIT_ARCHIVE

$(LIBRARY): $(OBJS) $(LOBJS) Makefile Makefile.in
	rm -f $@
ifdef SHARED_LIBRARY_LIBS
	@rm -f $(SUB_LOBJS)
	@for lib in $(SHARED_LIBRARY_LIBS); do $(AR_EXTRACT) $${lib}; $(CLEANUP2); done
endif
	$(AR) $(AR_FLAGS) $(OBJS) $(LOBJS) $(SUB_LOBJS)
	$(RANLIB) $@
	@rm -f foodummyfilefoo $(SUB_LOBJS)

endif # USE_IMPLICIT_ARCHIVE
else # OS2
ifdef OS2_IMPLIB
$(LIBRARY): $(OBJS) $(DEF_FILE)
	rm -f $@
	$(IMPLIB) $@ $(DEF_FILE)
	$(RANLIB) $@
else
$(LIBRARY): $(OBJS)
	rm -f $@
	$(AR) $(AR_FLAGS) $(LIBOBJS),,
	$(RANLIB) $@
endif
endif

ifneq ($(OS_ARCH),OS2)
$(SHARED_LIBRARY): $(OBJS) $(LOBJS) Makefile Makefile.in
	rm -f $@
ifneq ($(OS_ARCH),OpenVMS)
ifeq ($(NO_LD_ARCHIVE_FLAGS),1)
ifdef SHARED_LIBRARY_LIBS
	@rm -f $(SUB_LOBJS)
	@for lib in $(SHARED_LIBRARY_LIBS); do $(AR_EXTRACT) $${lib}; $(CLEANUP2); done
	$(MKSHLIB) -o $@ $(OBJS) $(LOBJS) $(SUB_LOBJS) $(EXTRA_DSO_LDOPTS)
else
	$(MKSHLIB) -o $@ $(OBJS) $(LOBJS) $(EXTRA_DSO_LDOPTS)
endif
else
	$(MKSHLIB) -o $@ $(OBJS) $(LOBJS) $(EXTRA_DSO_LDOPTS)
endif
	@rm -f foodummyfilefoo $(SUB_LOBJS)
else
	@touch no-such-file.vms; rm -f no-such-file.vms $(SUB_LOBJS)
ifndef IS_COMPONENT
	@if test ! -f VMSuni.opt; then \
	    echo "Creating universal symbol option file VMSuni.opt"; \
	    for lib in $(SHARED_LIBRARY_LIBS); do $(AR_EXTRACT) $${lib}; $(CLEANUP2); done; \
	    create_opt_uni $(OBJS) $(SUB_LOBJS); \
	fi
	@touch no-such-file.vms; rm -f no-such-file.vms $(SUB_LOBJS)
endif
	$(MKSHLIB) -o $@ $(OBJS) $(LOBJS) $(EXTRA_DSO_LDOPTS) VMSuni.opt;
	@echo "`translate $@`" > $(@:.$(DLL_SUFFIX)=.vms)
endif
	chmod +x $@
	$(MOZ_POST_DSO_LIB_COMMAND) $@
else
$(SHARED_LIBRARY): $(OBJS) $(DEF_FILE) Makefile Makefile.in
	rm -f $@
	$(LINK_DLL) $(OBJS) $(OS_LIBS) $(EXTRA_LIBS) $(DEF_FILE)
	chmod +x $@
	$(MOZ_POST_DSO_LIB_COMMAND) $@
endif

ifneq (,$(filter OS2 WINNT,$(OS_ARCH)))
$(DLL): $(OBJS) $(EXTRA_LIBS)
	rm -f $@
ifeq ($(OS_ARCH),OS2)
	$(LINK_DLL) $(OBJS) $(EXTRA_LIBS) $(OS_LIBS)
else
	$(LINK_DLL) $(OBJS) $(OS_LIBS) $(EXTRA_LIBS)
endif
endif

%: %.c
ifneq (,$(filter OS2 WINNT,$(OS_ARCH)))
	$(CC) -Fo$@ -c $(CFLAGS) $<
else
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
endif

%.o: %.c Makefile.in
ifneq (,$(filter OS2 WINNT,$(OS_ARCH)))
	$(CC) -Fo$@ -c $(CFLAGS) $<
else
	$(CC) -o $@ -c $(CFLAGS) $<
endif

moc_%.cpp: %.h
	$(MOC) $< -o $@ 

# The AS_DASH_C_FLAG is needed cause not all assemblers (Solaris) accept
# a '-c' flag.
%.o: %.s
	$(AS) -o $@ $(ASFLAGS) $(AS_DASH_C_FLAG) $<

%.o: %.S
	$(AS) -o $@ $(ASFLAGS) -c $<

%: %.cpp
	$(CCC) -o $@ $(CXXFLAGS) $< $(LDFLAGS)

#
# Please keep the next two rules in sync.
#
%.o: %.cc
	$(CCC) -o $@ -c $(CXXFLAGS) $<

%.o: %.cpp
ifdef STRICT_CPLUSPLUS_SUFFIX
	echo "#line 1 \"$*.cpp\"" | cat - $*.cpp > t_$*.cc
	$(CCC) -o $@ -c $(CXXFLAGS) t_$*.cc
	rm -f t_$*.cc
else
ifneq (,$(filter OS2 WINNT,$(OS_ARCH)))
	$(CCC) -Fo$@ -c $(CXXFLAGS) $<
else
	$(CCC) -o $@ -c $(CXXFLAGS) $<
endif
endif #STRICT_CPLUSPLUS_SUFFIX

%.i: %.cpp
	$(CCC) -C -E $(CXXFLAGS) $< > $*.i

%.i: %.c
	$(CC) -C -E $(CFLAGS) $< > $*.i

%: %.pl
	rm -f $@; cp $< $@; chmod +x $@

%: %.sh
	rm -f $@; cp $< $@; chmod +x $@

# Cancel these implicit rules
#
%: %,v

%: RCS/%,v

%: s.%

%: SCCS/s.%

###############################################################################
# Update Makefiles
###############################################################################
# Note: Passing depth to make-makefile is optional.
#       It saves the script some work, though.
Makefile: Makefile.in
	@$(PERL) $(topsrcdir)/build/autoconf/make-makefile -d $(DEPTH)

ifdef SUBMAKEFILES
# VPATH does not work on some machines in this case, so add $(srcdir)
$(SUBMAKEFILES): % : $(srcdir)/%.in
	@$(PERL) $(topsrcdir)/build/autoconf/make-makefile -d $(DEPTH) $@
endif

###############################################################################
# Bunch of things that extend the 'export' rule (in order):
###############################################################################

$(JAVA_DESTPATH) $(JAVA_DESTPATH)/$(PACKAGE) $(JMCSRCDIR)::
	@if test ! -d $@; then		\
		echo Creating $@;	\
		rm -rf $@;		\
		$(NSINSTALL) -D $@;	\
	else				\
		true;			\
	fi

################################################################################
### JSRCS -- for compiling java files

ifneq ($(JSRCS),)
ifdef JAVA_OR_NSJVM
export:: $(JAVA_DESTPATH) $(JAVA_DESTPATH)/$(PACKAGE)
	list=`$(PERL) $(topsrcdir)/config/outofdate.pl $(PERLARG)	\
		    -d $(JAVA_DESTPATH)/$(PACKAGE) $(JSRCS)`;	\
	if test "$$list"x != "x"; then				\
	    echo $(JAVAC) $$list;				\
	    $(JAVAC) $$list;					\
	else				\
		true;			\
	fi

all:: export

clean clobber::
	rm -f $(XPDIST)/classes/$(PACKAGE)/*.class

endif
endif

#
# JDIRS -- like JSRCS, except you can give a list of directories and it will
# compile all the out-of-date java files in those directories.
#
# NOTE: recursing through these can speed things up, but they also cause
# some builds to run out of memory
#
ifdef JDIRS
ifdef JAVA_OR_NSJVM
export:: $(JAVA_DESTPATH) $(JAVA_DESTPATH)/$(PACKAGE)
	@for d in $(JDIRS); do							\
		if test -d $$d; then						\
			set $(EXIT_ON_ERROR);					\
			files=`echo $$d/*.java`;				\
			list=`$(PERL) $(topsrcdir)/config/outofdate.pl $(PERLARG)	\
				    -d $(JAVA_DESTPATH)/$(PACKAGE) $$files`;	\
			if test "$${list}x" != "x"; then			\
			    echo Building all java files in $$d;		\
			    echo $(JAVAC) $$list;				\
			    $(JAVAC) $$list;					\
			else				\
				true;			\
			fi;							\
			set +e;							\
		else								\
			echo "Skipping non-directory $$d...";			\
		fi;								\
		$(CLICK_STOPWATCH);						\
	done
endif
endif

#
# JDK_GEN -- for generating "old style" native methods
#
# Generate JDK Headers and Stubs into the '_gen' and '_stubs' directory
#
ifneq ($(JDK_GEN),)
ifdef JAVA_OR_NSJVM
INCLUDES		+= -I$(JDK_GEN_DIR)
JDK_PACKAGE_CLASSES	= $(JDK_GEN)
JDK_PATH_CLASSES	= $(subst .,/,$(JDK_PACKAGE_CLASSES))
JDK_HEADER_CLASSFILES	= $(patsubst %,$(JAVA_DESTPATH)/%.class,$(JDK_PATH_CLASSES))
JDK_STUB_CLASSFILES	= $(patsubst %,$(JAVA_DESTPATH)/%.class,$(JDK_PATH_CLASSES))
JDK_HEADER_CFILES	= $(patsubst %,$(JDK_GEN_DIR)/%.h,$(JDK_GEN))
JDK_STUB_CFILES		= $(patsubst %,$(JDK_STUB_DIR)/%.c,$(JDK_GEN))

$(JDK_HEADER_CFILES): $(JDK_HEADER_CLASSFILES)
$(JDK_STUB_CFILES): $(JDK_STUB_CLASSFILES)

export::
	@echo Generating/Updating JDK headers
	$(JAVAH) -d $(JDK_GEN_DIR) $(JDK_PACKAGE_CLASSES)
	@echo Generating/Updating JDK stubs
	$(JAVAH) -stubs -d $(JDK_STUB_DIR) $(JDK_PACKAGE_CLASSES)
ifdef MOZ_GENMAC
	@if test ! -d $(DEPTH)/lib/mac/Java/; then						\
		echo "!!! You need to have a ns/lib/mac/Java directory checked out.";		\
		echo "!!! This allows us to automatically update generated files for the mac.";	\
		echo "!!! If you see any modified files there, please check them in.";		\
	else				\
		true;			\
	fi
	@echo Generating/Updating JDK headers for the Mac
	$(JAVAH) -mac -d $(DEPTH)/lib/mac/Java/_gen $(JDK_PACKAGE_CLASSES)
	@echo Generating/Updating JDK stubs for the Mac
	$(JAVAH) -mac -stubs -d $(DEPTH)/lib/mac/Java/_stubs $(JDK_PACKAGE_CLASSES)
endif
endif # JAVA_OR_NSJVM
endif

#
# JRI_GEN -- for generating JRI native methods
#
# Generate JRI Headers and Stubs into the 'jri' directory
#
ifneq ($(JRI_GEN),)
ifdef JAVA_OR_NSJVM
INCLUDES		+= -I$(JRI_GEN_DIR)
JRI_PACKAGE_CLASSES	= $(JRI_GEN)
JRI_PATH_CLASSES	= $(subst .,/,$(JRI_PACKAGE_CLASSES))
JRI_HEADER_CLASSFILES	= $(patsubst %,$(JAVA_DESTPATH)/%.class,$(JRI_PATH_CLASSES))
JRI_STUB_CLASSFILES	= $(patsubst %,$(JAVA_DESTPATH)/%.class,$(JRI_PATH_CLASSES))
JRI_HEADER_CFILES	= $(patsubst %,$(JRI_GEN_DIR)/%.h,$(JRI_GEN))
JRI_STUB_CFILES		= $(patsubst %,$(JRI_GEN_DIR)/%.c,$(JRI_GEN))

$(JRI_HEADER_CFILES): $(JRI_HEADER_CLASSFILES)
$(JRI_STUB_CFILES): $(JRI_STUB_CLASSFILES)

export::
	@echo Generating/Updating JRI headers
	$(JAVAH) -jri -d $(JRI_GEN_DIR) $(JRI_PACKAGE_CLASSES)
	@echo Generating/Updating JRI stubs
	$(JAVAH) -jri -stubs -d $(JRI_GEN_DIR) $(JRI_PACKAGE_CLASSES)
ifdef MOZ_GENMAC
	@if test ! -d $(DEPTH)/lib/mac/Java/; then						\
		echo "!!! You need to have a ns/lib/mac/Java directory checked out.";		\
		echo "!!! This allows us to automatically update generated files for the mac.";	\
		echo "!!! If you see any modified files there, please check them in.";		\
	else				\
		true;			\
	fi
	@echo Generating/Updating JRI headers for the Mac
	$(JAVAH) -jri -mac -d $(DEPTH)/lib/mac/Java/_jri $(JRI_PACKAGE_CLASSES)
	@echo Generating/Updating JRI stubs for the Mac
	$(JAVAH) -jri -mac -stubs -d $(DEPTH)/lib/mac/Java/_jri $(JRI_PACKAGE_CLASSES)
endif
endif # JAVA_OR_NSJVM
endif



#
# JNI_GEN -- for generating JNI native methods
#
# Generate JNI Headers and Stubs into the 'jni' directory
#
ifneq ($(JNI_GEN),)
ifdef JAVA_OR_NSJVM
INCLUDES		+= -I$(JNI_GEN_DIR)
JNI_PACKAGE_CLASSES	= $(JNI_GEN)
JNI_PATH_CLASSES	= $(subst .,/,$(JNI_PACKAGE_CLASSES))
JNI_HEADER_CLASSFILES	= $(patsubst %,$(JAVA_DESTPATH)/%.class,$(JNI_PATH_CLASSES))
JNI_HEADER_CFILES	= $(patsubst %,$(JNI_GEN_DIR)/%.h,$(JNI_GEN))
JNI_STUB_CFILES		= $(patsubst %,$(JNI_GEN_DIR)/%.c,$(JNI_GEN))

$(JNI_HEADER_CFILES): $(JNI_HEADER_CLASSFILES)

export::
	@echo Generating/Updating JNI headers
	$(JAVAH) -jni -d $(JNI_GEN_DIR) $(JNI_PACKAGE_CLASSES)
ifdef MOZ_GENMAC
	@if test ! -d $(DEPTH)/lib/mac/Java/; then						\
		echo "!!! You need to have a ns/lib/mac/Java directory checked out.";		\
		echo "!!! This allows us to automatically update generated files for the mac.";	\
		echo "!!! If you see any modified files there, please check them in.";		\
	else				\
		true;			\
	fi
	@echo Generating/Updating JNI headers for the Mac
	$(JAVAH) -jni -mac -d $(DEPTH)/lib/mac/Java/_jni $(JNI_PACKAGE_CLASSES)
endif
endif # JAVA_OR_NSJVM
endif # JNI_GEN



#
# JMC_EXPORT -- for declaring which java classes are to be exported for jmc
#
ifneq ($(JMC_EXPORT),)
ifdef JAVA_OR_NSJVM
JMC_EXPORT_PATHS	= $(subst .,/,$(JMC_EXPORT))
JMC_EXPORT_FILES	= $(patsubst %,$(JAVA_DESTPATH)/$(PACKAGE)/%.class,$(JMC_EXPORT_PATHS))

#
# We're doing NSINSTALL -t here (copy mode) because calling INSTALL will pick up
# your NSDISTMODE and make links relative to the current directory. This is a
# problem because the source isn't in the current directory:
#
export:: $(JMC_EXPORT_FILES) $(JMCSRCDIR)
	$(NSINSTALL) -t -m 444 $(JMC_EXPORT_FILES) $(JMCSRCDIR)
endif # JAVA_OR_NSJVM
endif

#
# JMC_GEN -- for generating java modules
#
# Provide default export & install rules when using JMC_GEN
#
ifneq ($(JMC_GEN),)
INCLUDES		+= -I$(JMC_GEN_DIR) -I.
ifdef JAVA_OR_NSJVM
JMC_HEADERS		= $(patsubst %,$(JMC_GEN_DIR)/%.h,$(JMC_GEN))
JMC_STUBS		= $(patsubst %,$(JMC_GEN_DIR)/%.c,$(JMC_GEN))
JMC_OBJS		= $(patsubst %,%.o,$(JMC_GEN))

$(JMC_GEN_DIR)/M%.h: $(JMCSRCDIR)/%.class
	$(JMC) -d $(JMC_GEN_DIR) -interface $(JMC_GEN_FLAGS) $(?F:.class=)

$(JMC_GEN_DIR)/M%.c: $(JMCSRCDIR)/%.class
	$(JMC) -d $(JMC_GEN_DIR) -module $(JMC_GEN_FLAGS) $(?F:.class=)

M%.o: $(JMC_GEN_DIR)/M%.h $(JMC_GEN_DIR)/M%.c
ifeq ($(OS_ARCH),OS2)
	$(CC) -Fo$@ -c $(CFLAGS) $(JMC_GEN_DIR)/M$*.c
else
	$(CC) -o $@ -c $(CFLAGS) $(JMC_GEN_DIR)/M$*.c
endif

export:: $(JMC_HEADERS) $(JMC_STUBS)
endif # JAVA_OR_NSJVM
endif

################################################################################
# Copy each element of EXPORTS to $(PUBLIC)

ifneq ($(EXPORTS),)
$(PUBLIC)::
	@if test ! -d $@; then echo Creating $@; rm -rf $@; $(NSINSTALL) -D $@; else true; fi

export:: $(EXPORTS) $(PUBLIC)
	$(INSTALL) -m 444 $^
endif 

################################################################################
# Copy each element of PREF_JS_EXPORTS to $(DIST)/bin/defaults/pref

ifneq ($(PREF_JS_EXPORTS),)
$(DIST)/bin/defaults/pref::
	@if test ! -d $@; then echo Creating $@; rm -rf $@; $(NSINSTALL) -D $@; else true; fi

export:: $(PREF_JS_EXPORTS) $(DIST)/bin/defaults/pref
	$(INSTALL) -m 444 $^
endif 

################################################################################
# Export the elements of $(XPIDLSRCS), generating .h and .xpt files and
# moving them to the appropriate places.

ifneq ($(XPIDLSRCS),)

ifndef XPIDL_MODULE
XPIDL_MODULE		= $(MODULE)
endif

ifeq ($(XPIDL_MODULE),) # we need $(XPIDL_MODULE) to make $(XPIDL_MODULE).xpt
export:: FORCE
	@echo
	@echo "*** Error processing XPIDLSRCS:"
	@echo "Please define MODULE or XPIDL_MODULE when defining XPIDLSRCS,"
	@echo "so we have a module name to use when creating MODULE.xpt."
	@echo; sleep 2; false
else

# export .idl files to $(XPDIST)/idl
$(XPDIST)/idl::
	@if test ! -d $@; then echo Creating $@; rm -rf $@; $(NSINSTALL) -D $@; else true; fi

export:: $(XPIDLSRCS) $(XPDIST)/idl
	$(INSTALL) -m 444 $^

# generate .h files from into $(XPIDL_GEN_DIR), then export to $(PUBLIC);
# warn against overriding existing .h file.  (Added to MAKE_DIRS above.)
$(XPIDL_GEN_DIR):
	@if test ! -d $@; then echo Creating $@; rm -rf $@; mkdir $@; else true; fi

# don't depend on $(XPIDL_GEN_DIR), because the modification date changes
# with any addition to the directory, regenerating all .h files -> everything.

$(XPIDL_GEN_DIR)/%.h: %.idl $(XPIDL_COMPILE)
	$(XPIDL_COMPILE) -m header -w -I $(XPDIST)/idl -I$(srcdir) -o $(XPIDL_GEN_DIR)/$* $<
	@if test -n "$(findstring $*.h, $(EXPORTS))"; \
	  then echo "*** WARNING: file $*.h generated from $*.idl overrides $(srcdir)/$*.h"; else true; fi

export:: $(patsubst %.idl,$(XPIDL_GEN_DIR)/%.h, $(XPIDLSRCS)) $(PUBLIC)
	$(INSTALL) -m 444 $^

ifndef NO_GEN_XPT
# generate intermediate .xpt files into $(XPIDL_GEN_DIR), then link
# into $(XPIDL_MODULE).xpt and export it to $(DIST)/bin/components.
$(XPIDL_GEN_DIR)/%.xpt: %.idl $(XPIDL_COMPILE)
	$(XPIDL_COMPILE) -m typelib -w -I $(XPDIST)/idl -I$(srcdir) -o $(XPIDL_GEN_DIR)/$* $<

$(XPIDL_GEN_DIR)/$(XPIDL_MODULE).xpt: $(patsubst %.idl,$(XPIDL_GEN_DIR)/%.xpt,$(XPIDLSRCS))
	$(XPIDL_LINK) $(XPIDL_GEN_DIR)/$(XPIDL_MODULE).xpt $^

install:: $(XPIDL_GEN_DIR)/$(XPIDL_MODULE).xpt
	$(INSTALL) -m 444 $(XPIDL_GEN_DIR)/$(XPIDL_MODULE).xpt $(DIST)/bin/components

endif

GARBAGE			+= $(XPIDL_GEN_DIR)
endif
endif

################################################################################
# Generate chrome building rules.
#
# You need to set these in your makefile.win to utilize this support:
#   CHROME_DIR - specifies the chrome subdirectory where your chrome files
#                go; e.g., CHROME_DIR=navigator or CHROME_DIR=global
#
# Note:  All file listed in the next three macros MUST be prefaced with .\ (or ./)!
#
#   CHROME_CONTENT - list of chrome content files; these can be prefaced with
#                arbitrary paths; e.g., CHROME_CONTENT=./content/default/foobar.xul
#   CHROME_SKIN - list of skin files
#   CHROME_L10N - list of localization files, e.g., CHROME_L10N=./locale/en-US/foobar.dtd
#
# These macros are optional, if not specified, each defaults to ".".
#   CHROME_CONTENT_DIR - specifies chrome subdirectory where content files will be
#                  installed; this path is inserted between $(CHROME_DIR) and
#                  the path you specify in each $(CHROME_CONTENT) entry; i.e.,
#                  for CHROME_CONTENT=./content/default/foobar.xul, it will be
#                  installed into:
#                    $(DIST)\bin\chrome\$(CHROME_DIR)\$(CHROME_CONTENT_DIR)\content\default\foobar.xul.
#                  e.g., CHROME_DIR=global
#                        CHROME_CONTENT_DIR=content\default
#                        CHROME_CONTENT=.\foobar.xul
#                  will install foobar.xul into content/default (even though it
#                  resides in content/foobar.xul (no default) in the source tree.
#                  But note that such usage must be put in a makefile.win that
#                  itself resides in the content directory (i.e., it can't reside
#                  up a level, since then CHROME_CONTENT=./content/foobar.xul which
#                  would install into ...global\content\default\content\foobar.xul.
#   CHROME_SKIN_DIR - Like above, but for skin files
#   CHROME_L10N_DIR - Like above, but for localization files
ifneq ($(CHROME_DIR),)

# Figure out root of chrome dist dir.
CHROME_DIST=$(DIST)/bin/chrome/$(CHROME_DIR)

# Content
ifneq ($(CHROME_CONTENT),)

# Content goes to CHROME_DIR unless specified otherwise.
ifeq ($(CHROME_CONTENT_DIR),)
CHROME_CONTENT_DIR	= .
endif

# Export content files by copying to dist.
install:: $(addprefix "INSTALL-", $(CHROME_CONTENT))

# Pseudo-target specifying how to install content files.
$(addprefix "INSTALL-", $(CHROME_CONTENT)) :
	$(INSTALL) $(subst "INSTALL-",,$(CHROME_SOURCE_DIR)/$@) $(CHROME_DIST)/$(CHROME_CONTENT_DIR)/$(subst "INSTALL-",,$(@D))

# Clobber content files.
clobber:: $(addprefix "CLOBBER-", $(CHROME_CONTENT))

# Pseudo-target specifying how to clobber content files.
$(addprefix "CLOBBER-", $(CHROME_CONTENT)) :
	-$(RM) $(CHROME_DIST)/$(CHROME_CONTENT_DIR)/$(subst "CLOBBER-",,$@)

endif
# content

# Skin
ifneq ($(CHROME_SKIN),)

# Skin goes to CHROME_DIR unless specified otherwise.
ifeq ($(CHROME_SKIN_DIR),)
CHROME_SKIN_DIR		= .
endif

# Export content files by copying to dist.
install:: $(addprefix "INSTALL-", $(CHROME_SKIN))

# Pseudo-target specifying how to install chrome files.
$(addprefix "INSTALL-", $(CHROME_SKIN)) :
	$(INSTALL) $(subst "INSTALL-",,$(CHROME_SOURCE_DIR)/$@) $(CHROME_DIST)/$(CHROME_SKIN_DIR)/$(subst "INSTALL-",,$(@D))

# Clobber content files.
clobber:: $(addprefix "CLOBBER-", $(CHROME_SKIN))

# Pseudo-target specifying how to clobber content files.
$(addprefix "CLOBBER-", $(CHROME_SKIN)) :
	-$(RM) $(CHROME_DIST)/$(CHROME_SKIN_DIR)/$(subst "CLOBBER-",,$@)

endif
# skin

# Localization.
ifneq ($(CHROME_L10N),)

# L10n goes to CHROME_DIR unless specified otherwise.
ifeq ($(CHROME_L10N_DIR),)
CHROME_L10N_DIR		= .
endif

# Export l10n files by copying to dist.
install:: $(addprefix "INSTALL-", $(CHROME_L10N))

# Pseudo-target specifying how to install l10n files.
$(addprefix "INSTALL-", $(CHROME_L10N)) :
	$(INSTALL) $(subst "INSTALL-",,$(CHROME_SOURCE_DIR)/$@) $(CHROME_DIST)/$(CHROME_L10N_DIR)/$(subst "INSTALL-",,$(@D))

# Clobber l10n files.
clobber:: $(addprefix "CLOBBER-", $(CHROME_L10N))

# Pseudo-target specifying how to clobber l10n files.
$(addprefix "CLOBBER-", $(CHROME_L10N)) :
	-$(RM) $(CHROME_DIST)/$(CHROME_L10N_DIR)/$(subst "CLOBBER-",,$@)

endif
# localization

endif
# chrome
##############################################################################

ifndef NO_MDUPDATE
ifneq (,$(filter-out OS2 WINNT,$(OS_ARCH)))
-include $(DEPENDENCIES)
# Can't use sed because of its 4000-char line length limit, so resort to perl
.DEFAULT:
	@$(PERL) -e '                                                         \
	    open(MD, "< $(DEPENDENCIES)");                                    \
	    while (<MD>) {                                                    \
		if (m@ \.*/*$< @) {                                           \
		    $$found = 1;                                              \
		    last;                                                     \
		}                                                             \
	    }                                                                 \
	    if ($$found) {                                                    \
		print "Removing stale dependency $< from $(DEPENDENCIES)\n";  \
		seek(MD, 0, 0);                                               \
		$$tmpname = "fix.md" . $$$$;                                  \
		open(TMD, "> " . $$tmpname);                                  \
		while (<MD>) {                                                \
		    s@ \.*/*$< @ @;                                           \
		    if (!print TMD "$$_") {                                   \
			unlink(($$tmpname));                                  \
			exit(1);                                              \
		    }                                                         \
		}                                                             \
		close(TMD);                                                   \
		if (!rename($$tmpname, "$(DEPENDENCIES)")) {                  \
		    unlink(($$tmpname));                                      \
		}                                                             \
	    } elsif ("$<" ne "$(DEPENDENCIES)") {                             \
		print "$(MAKE): *** No rule to make target $<.  Stop.\n";     \
		exit(1);                                                      \
	    }'
endif
endif

#############################################################################
# X dependency system
#############################################################################
ifdef COMPILER_DEPEND
depend::
	@echo "$(MAKE): No need to run depend target.\
			Using compiler-based depend." 1>&2
ifeq ($(GNU_CC)$(GNU_CXX),)
# Non-GNU compilers
	@echo "`echo '$(MAKE):'|sed 's/./ /g'`"\
	'(Compiler-based depend was turned on by "--enable-depend".)' 1>&2
else
# GNU compilers
	@space="`echo '$(MAKE): '|sed 's/./ /g'`";\
	echo "$$space"'Since you are using a GNU compiler,\
		it is on by default.' 1>&2; \
	echo "$$space"'To turn it off, pass --disable-md to configure.' 1>&2
endif
else
$(MKDEPENDENCIES)::
	touch $(MKDEPENDENCIES)
ifneq ($(OS_ARCH),OpenVMS)
	$(MKDEPEND) -o'.o' -f$(MKDEPENDENCIES) $(DEFINES) $(ACDEFINES) $(INCLUDES) $(addprefix $(srcdir)/,$(CSRCS) $(CPPSRCS)) >/dev/null 2>&1
	@mv $(MKDEPENDENCIES) depend.mk.old && cat depend.mk.old | sed "s|^$(srcdir)/||g" > $(MKDEPENDENCIES) && rm -f depend.mk.old
else
# OpenVMS can't handle long lines, so make it shorter
	@ln -s $(srcdir) VMSs
	$(MKDEPEND) -o'.o' -f$(MKDEPENDENCIES) $(DEFINES) $(ACDEFINES) $(INCLUDES) $(addprefix VMSs/,$(CSRCS) $(CPPSRCS)) >/dev/null 2>&1
	@mv $(MKDEPENDENCIES) depend.mk.old && cat depend.mk.old | sed "s|^VMSs/||g" | sed "s| VMSs/| $(srcdir)/|g" > $(MKDEPENDENCIES) && rm -f depend.mk.old
	@rm VMSs
endif

ifndef MOZ_NATIVE_MAKEDEPEND
$(MKDEPEND):
	cd $(DEPTH)/config; $(MAKE) nsinstall
	cd $(MKDEPEND_DIR); $(MAKE)
endif

ifndef MOZ_NATIVE_MAKEDEPEND
MKDEPEND_BUILTIN	= $(MKDEPEND)
else
MKDEPEND_BUILTIN	=
endif

ifdef OBJS
depend:: $(SUBMAKEFILES) $(MAKE_DIRS) $(MKDEPEND_BUILTIN) $(MKDEPENDENCIES)
else
depend:: $(SUBMAKEFILES)
endif
	+$(LOOP_OVER_DIRS)

dependclean:: $(SUBMAKEFILES)
	rm -f $(MKDEPENDENCIES)
	+$(LOOP_OVER_DIRS)

-include depend.mk

endif # ! COMPILER_DEPEND

#############################################################################
# Yet another depend system: -MD (configure switch: --enable-md)
ifdef COMPILER_DEPEND
ifdef OBJS
# MDDEPDIR is the subdirectory where all the dependency files are placed.
#   This uses a make rule (instead of a macro) to support parallel
#   builds (-jN). If this were done in the LOOP_OVER_DIRS macro, two
#   processes could simultaneously try to create the same directory.
#
$(MDDEPDIR):
	@if test ! -d $@; then echo Creating $@; rm -rf $@; mkdir $@; else true; fi

MDDEPEND_FILES		:= $(wildcard $(MDDEPDIR)/*.pp)

ifdef MDDEPEND_FILES
ifdef PERL
# The script mddepend.pl checks the dependencies and writes to stdout
# one rule to force out-of-date objects. For example,
#   foo.o boo.o: FORCE
# The script has an advantage over including the *.pp files directly
# because it handles the case when header files are removed from the build.
# 'make' would complain that there is no way to build missing headers.
$(MDDEPDIR)/.all.pp: FORCE
	@$(PERL) $(topsrcdir)/config/mddepend.pl $@ $(MDDEPEND_FILES) 
-include $(MDDEPDIR)/.all.pp
else
include $(MDDEPEND_FILES)
endif
endif
endif
endif
#############################################################################

-include $(MY_RULES)

#
# This speeds up gmake's processing if these files don't exist.
#
$(MY_CONFIG) $(MY_RULES):
	@touch $@

#
# Generate Emacs tags in a file named TAGS if ETAGS was set in $(MY_CONFIG)
# or in $(MY_RULES)
#
ifdef ETAGS
ifneq ($(CSRCS)$(CPPSRCS)$(HEADERS),)
all:: TAGS
TAGS:: $(CSRCS) $(CPPSRCS) $(HEADERS)
	$(ETAGS) $(CSRCS) $(CPPSRCS) $(HEADERS)
endif
endif

################################################################################
# Special gmake rules.
################################################################################

#
# Re-define the list of default suffixes, so gmake won't have to churn through
# hundreds of built-in suffix rules for stuff we don't need.
#
.SUFFIXES:
.SUFFIXES: .out .a .ln .o .c .cc .C .cpp .y .l .s .S .h .sh .i .pl .class .java .html .pp .mk .in

#
# Don't delete these files if we get killed.
#
.PRECIOUS: .java $(JDK_HEADERS) $(JDK_STUBS) $(JRI_HEADERS) $(JRI_STUBS) $(JMC_HEADERS) $(JMC_STUBS)

#
# Fake targets.  Always run these rules, even if a file/directory with that
# name already exists.
#
.PHONY: all all_platforms alltags boot checkout clean clobber clobber_all export install libs makefiles realclean run_viewer run_apprunner $(DIRS) FORCE

# Used as a dependency to force targets to rebuild
FORCE:

tags: TAGS

TAGS: $(SUBMAKEFILES) $(CSRCS) $(CPPSRCS) $(wildcard *.h)
	-etags $(CSRCS) $(CPPSRCS) $(wildcard *.h)
	+$(LOOP_OVER_DIRS)

envirocheck::
	@echo -----------------------------------
	@echo "Enviro-Check (tm)"
	@echo -----------------------------------
	@echo "MOZILLA_CLIENT = $(MOZILLA_CLIENT)"
	@echo "NO_MDUPDATE    = $(NO_MDUPDATE)"
	@echo "BUILD_OPT      = $(BUILD_OPT)"
	@echo "MOZ_LITE       = $(MOZ_LITE)"
	@echo "MOZ_MEDIUM     = $(MOZ_MEDIUM)"
	@echo -----------------------------------

echo-dirs:
	@echo $(DIRS)

echo-depth-path:
	@$(topsrcdir)/build/autoconf/print-depth-path.sh

echo-module-name:
	@$(topsrcdir)/build/package/rpm/print-module-name.sh

echo-module-filelist:
	@$(topsrcdir)/build/package/rpm/print-module-filelist.sh

