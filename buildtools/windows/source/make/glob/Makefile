#
# GNU Makefile for building gmake.exe on WIN32
# This makefile compiles the files in the glob subdirectory.
#

CSRCS = fnmatch.c glob.c

OBJS = $(CSRCS:.c=.obj)

CC = cl

CFLAGS = $(CC_SWITCHES) $(INCLUDES) $(DEFINES) $(CC_OUTPUTS)

CC_SWITCHES = -nologo -ML -W3 -O2

INCLUDES = -I. -I..

DEFINES = -DWIN32 -DNDEBUG -D_CONSOLE -DNETSCAPE -DHAVE_CONFIG_H

CC_OUTPUTS = -Fpglob.pch -YX -c

all: $(OBJS)

%.obj: %.c
	$(CC) $(CFLAGS) $<

clean:
	rm -rf $(OBJS) glob.pch
