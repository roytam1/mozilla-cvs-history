#
# GNU Makefile for building gmake.exe on WIN32
# This is the top-level makefile.  It only does the optimized build.
#

CSRCS =	ar.c \
	arscan.c \
	commands.c \
	default.c \
	dir.c \
	expand.c \
	file.c \
	function.c \
	getloadavg.c \
	getopt.c \
	getopt1.c \
	implicit.c \
	job.c \
	main.c \
	misc.c \
	read.c \
	remake.c \
	remote-stub.c \
	rule.c \
	signame.c \
	variable.c \
	version.c \
	vpath.c

OBJS = $(CSRCS:.c=.obj) glob/fnmatch.obj glob/glob.obj

CC = cl

CFLAGS = $(CC_SWITCHES) $(INCLUDES) $(DEFINES) $(CC_OUTPUTS)

CC_SWITCHES = -nologo -ML -W3 -O2

INCLUDES = -I. -Iglob

DEFINES = -DWIN32 -DNDEBUG -D_CONSOLE -DNETSCAPE -DHAVE_CONFIG_H

CC_OUTPUTS = -Fpgmake.pch -YX -c

LINK = link

LINK_FLAGS = -nologo -subsystem:console \
	-incremental:no -machine:I386 -out:$@

all: gmake.exe

build_glob:
	cd glob; $(MAKE)

gmake.exe: build_glob $(OBJS)
	$(LINK) $(LINK_FLAGS) $(OBJS)

%.obj: %.c
	$(CC) $(CFLAGS) $<

clean:
	cd glob; $(MAKE) $@
	rm -rf $(OBJS) gmake.pch gmake.exe
