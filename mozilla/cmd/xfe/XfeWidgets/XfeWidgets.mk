#!gmake
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


##########################################################################
#
# Name:			XfeWidgets.mk
# Description:	Makefile defines used throughout XfeWidgets
# Author:		Ramiro Estrugo <ramiro@netscape.com>
#
##########################################################################


##
## Convert resource (.ad) files to source code
##
XFE_AD2C = $(DEPTH)/cmd/xfe/XfeWidgets/utils/ad2c

##
## Build demos ?
##
ifdef XFE_WIDGETS_BUILD_DEMO
XFE_EXTRA_DEFINES += -DXFE_WIDGETS_BUILD_DEMO
endif

##
## Build ununsed ?
##
ifdef XFE_WIDGETS_BUILD_UNUSED
XFE_EXTRA_DEFINES += -DXFE_WIDGETS_BUILD_UNUSED
endif

##
## Xfe Widgets Library
##
XFE_STATIC_WIDGETS_LIB	= $(DIST)/lib/libXfeWidgets.a
XFE_SHARED_WIDGETS_LIB	= $(DIST)/bin/libXfeWidgets.$(DLL_SUFFIX)
XFE_WIDGETS_REQUIRES	= XfeWidgets

##
## Xfe Widgets Test Library
##
XFE_STATIC_TEST_LIB		= $(DIST)/lib/libXfeTest.a
XFE_SHARED_TEST_LIB		= $(DIST)/bin/libXfeTest.$(DLL_SUFFIX)
XFE_TEST_REQUIRES		= XfeTest

##
## Xfe Widgets Bm Library
##
XFE_STATIC_BM_LIB		= $(DIST)/lib/libXfeBm.a
XFE_SHARED_BM_LIB		= $(DIST)/bin/libXfeBm.$(DLL_SUFFIX)
XFE_BM_REQUIRES			= XfeBm

##
## Linux specific
##
ifeq ($(OS_ARCH),Linux)

  XFE_X_CC_FLAGS		= $(MOZILLA_XFE_X11_INCLUDE_FLAGS)

  XFE_XM_CC_FLAGS		= $(MOZILLA_XFE_MOTIF_INCLUDE_FLAGS)

  XFE_X_LD_PATH			= $(MOZILLA_XFE_X11_DYNAMIC_PATHS)

  XFE_XM_LD_PATH		= $(MOZILLA_XFE_MOTIF_DYNAMIC_PATHS)

  XFE_XM_LIBS			=\
						$(MOZILLA_XFE_GLIBC_BROKEN_LOCALE_FLAGS) \
						$(MOZILLA_XFE_MOTIF_DYNAMIC_FLAGS) \
						$(MOZILLA_XFE_MOTIF_PRINT_SHELL_FLAGS)

  XFE_XT_LIBS			= -lXt

  XFE_X_LIBS			=\
						$(MOZILLA_XFE_MOTIF_XPM_FLAGS) \
						-lXmu \
						-lXext \
						-lX11 \
						$(MOZILLA_XFE_X11_SM_FLAGS)

  XFE_USE_NATIVE_XPM	= True

  XFE_EXTRA_DEFINES		+= -DXFE_USE_NATIVE_XPM

  XFE_OS_LD_FLAGS		=

endif

##
## SunOS specific
##
ifeq ($(OS_ARCH),SunOS)

  XFE_X_CC_FLAGS		= -I/usr/openwin/include

  XFE_XM_CC_FLAGS		= -I/usr/dt/include

  XFE_X_LD_PATH			= -L/usr/openwin/lib

  XFE_XM_LD_PATH		= -L/usr/dt/lib

  XFE_XM_LIBS			= -lXm

  XFE_XT_LIBS			= -lXt

  XFE_X_LIBS			= -lXmu -lX11

  XFE_USE_NATIVE_XPM	=

  XFE_OS_LD_FLAGS		=\
						-lw \
						-lintl \
						-lsocket \
						-lnsl \
						-lgen \
						-lm \
						-ldl

endif

##
## IRIX specific
##
ifeq ($(OS_ARCH),IRIX)

  XFE_X_CC_FLAGS		=

  XFE_XM_CC_FLAGS		=

  XFE_X_LD_PATH			=

  XFE_XM_LD_PATH		=

  XFE_XM_LIBS			= -lXm

  XFE_XT_LIBS			= -lXt

  XFE_X_LIBS			= -lXmu -lX11 -lSM -lICE

  XFE_USE_NATIVE_XPM	= True

  XFE_EXTRA_DEFINES		+= -DXFE_USE_NATIVE_XPM

  XFE_OS_LD_FLAGS		=

endif

##
## X Pixmap Library (Xpm)
##
ifeq ($(XFE_USE_NATIVE_XPM),True)

  XFE_NATIVE_XPM_LIBS	= -lXpm

else

  XFE_STATIC_XPM_LIB	= $(DIST)/lib/libXpm.a
  XFE_SHARED_XPM_LIB	= $(DIST)/bin/libXpm.$(DLL_SUFFIX)
  XFE_XPM_REQUIRES		= Xpm

endif

##
## Microline Widget Library (XmL)
##
ifeq ($(XFE_USE_NATIVE_XML),True)

  XFE_NATIVE_XML_LIBS	= -lXmL

else

  XFE_STATIC_XML_LIB	= $(DIST)/lib/libXmL.a
  XFE_SHARED_XML_LIB	= $(DIST)/bin/libXmL.$(DLL_SUFFIX)
  XFE_XML_REQUIRES		= Microline

endif

##
## XfeCaption Library
##
XFE_STATIC_CAPTION_LIB		= $(DIST)/lib/libXfeCaption.a
XFE_SHARED_CAPTION_LIB		= $(DIST)/bin/libXfeCaption.$(DLL_SUFFIX)
XFE_CAPTION_REQUIRES		= XfeCaption

##
## XfeCaption required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_CAPTION_REQUIRES)),$(XFE_CAPTION_REQUIRES))

XFE_SHARED_LIBS			+= $(XFE_SHARED_CAPTION_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_CAPTION_LIB)

endif


##
## Xfe Pref Library
##
XFE_STATIC_PREF_LIB		= $(DIST)/lib/libXfePref.a
XFE_SHARED_PREF_LIB		= $(DIST)/bin/libXfePref.$(DLL_SUFFIX)
XFE_PREF_REQUIRES		= XfePref

##
## XfePref required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_PREF_REQUIRES)),$(XFE_PREF_REQUIRES))

XFE_SHARED_LIBS			+= $(XFE_SHARED_PREF_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_PREF_LIB)

endif

##
## XfeToolTip Library
##
XFE_STATIC_TOOL_TIP_LIB		= $(DIST)/lib/libXfeToolTip.a
XFE_SHARED_TOOL_TIP_LIB		= $(DIST)/bin/libXfeToolTip.$(DLL_SUFFIX)
XFE_TOOL_TIP_REQUIRES		= XfeToolTip

##
## XfeToolTip required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_TOOL_TIP_REQUIRES)),$(XFE_TOOL_TIP_REQUIRES))

XFE_SHARED_LIBS			+= $(XFE_SHARED_TOOL_TIP_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_TOOL_TIP_LIB)

endif

##
## XfeTest required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_TEST_REQUIRES)),$(XFE_TEST_REQUIRES))

XFE_SHARED_LIBS			+= $(XFE_SHARED_TEST_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_TEST_LIB)

endif

##
## XfeToolBar Library
##
XFE_STATIC_TOOL_BAR_LIB		= $(DIST)/lib/libXfeToolBar.a
XFE_SHARED_TOOL_BAR_LIB		= $(DIST)/bin/libXfeToolBar.$(DLL_SUFFIX)
XFE_TOOL_BAR_REQUIRES		= XfeToolBar

##
## XfeToolBar required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_TOOL_BAR_REQUIRES)),$(XFE_TOOL_BAR_REQUIRES))
XFE_SHARED_LIBS			+= $(XFE_SHARED_TOOL_BAR_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_TOOL_BAR_LIB)

endif

##
## XfeComboBox Library
##
XFE_STATIC_COMBO_BOX_LIB	= $(DIST)/lib/libXfeComboBox.a
XFE_SHARED_COMBO_BOX_LIB	= $(DIST)/bin/libXfeComboBox.$(DLL_SUFFIX)
XFE_COMBO_BOX_REQUIRES		= XfeComboBox

##
## XfeComboBox required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_COMBO_BOX_REQUIRES)),$(XFE_COMBO_BOX_REQUIRES))

XFE_SHARED_LIBS			+= $(XFE_SHARED_COMBO_BOX_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_COMBO_BOX_LIB)

endif

##
## XfeBm required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_BM_REQUIRES)),$(XFE_BM_REQUIRES))

XFE_SHARED_LIBS			+= $(XFE_SHARED_BM_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_BM_LIB)

endif


##
## XfeWidgets required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_WIDGETS_REQUIRES)),$(XFE_WIDGETS_REQUIRES))

XFE_SHARED_LIBS			+= $(XFE_SHARED_WIDGETS_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_WIDGETS_LIB)

endif

##
## Microline required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_XML_REQUIRES)),$(XFE_XML_REQUIRES))

XFE_SHARED_LIBS			+= $(XFE_SHARED_XML_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_XML_LIB)

endif

##
## Xpm required ?
##
ifeq ($(filter $(REQUIRES),$(XFE_XPM_REQUIRES)),$(XFE_XPM_REQUIRES))

XFE_SHARED_LIBS			+= $(XFE_SHARED_XPM_LIB)
XFE_STATIC_LIBS			+= $(XFE_STATIC_XPM_LIB)

endif

##
## GUI link path
##
XFE_GUI_LD_PATH			=\
						$(XFE_XM_LD_PATH) \
						$(XFE_X_LD_PATH)

##
## GUI libs
##
XFE_GUI_LIBS			=\
						$(XFE_NATIVE_XML_LIBS) \
						$(XFE_XM_LIBS) \
						$(XFE_XT_LIBS) \
						$(XFE_NATIVE_XPM_LIBS) \
						$(XFE_X_LIBS)

##
## Shared link flags
##
XFE_SHARED_LD_FLAGS		=\
						$(XFE_GUI_LD_PATH) \
						$(XFE_SHARED_LIBS) \
						$(XFE_GUI_LIBS) \
						$(XFE_OS_LD_FLAGS)

##
## Static link flags
##
XFE_STATIC_LD_FLAGS		=\
						$(XFE_GUI_LD_PATH) \
						$(XFE_STATIC_LIBS) \
						$(XFE_GUI_LIBS) \
						$(XFE_OS_LD_FLAGS)

DEFINES += $(XFE_EXTRA_DEFINES)

##
## Resource source rule
##
$(OBJDIR)/%.ad.c:: %.ad $(XFE_AD2C)
	@$(MAKE_OBJDIR)
	@echo 'char * fallback_resources[] = {' > $@; \
	$(XFE_AD2C) $< >> $@; \
	echo '0};' >> $@

##
## Resource object rule
##
$(OBJDIR)/%.ad.o: $(OBJDIR)/%.ad.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $<

##
## Static link rule
##
$(OBJDIR)/%.static: $(OBJDIR)/%.o $(OBJDIR)/%.ad.o $(XFE_STATIC_LIBS)
	@$(MAKE_OBJDIR)
	$(XFE_PURIFY) $(CC) -o $@ $< $(OBJDIR)/$*.ad.o $(XFE_STATIC_LD_FLAGS)

##
## Shared link rule
##
$(OBJDIR)/%.shared: $(OBJDIR)/%.o $(OBJDIR)/%.ad.o $(XFE_SHARED_LIBS)
	@$(MAKE_OBJDIR)
	$(XFE_PURIFY) $(CC) -o $@ $< $(OBJDIR)/$*.ad.o $(XFE_SHARED_LD_FLAGS)
