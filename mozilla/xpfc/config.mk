#
#           CONFIDENTIAL AND PROPRIETARY SOURCE CODE OF
#              NETSCAPE COMMUNICATIONS CORPORATION
# Copyright � 1996, 1997 Netscape Communications Corporation.  All Rights
# Reserved.  Use of this Source Code is subject to the terms of the
# applicable license agreement from Netscape Communications Corporation.
# The copyright notice(s) in this Source Code does not indicate actual or
# intended publication of this Source Code.
#

#
#  Override TARGETS variable so that only static libraries
#  are specifed as dependencies within rules.mk.
#

CFLAGS         +=-D_IMPL_NS_XPFC

LD_LIBS += \
	raptorbase \
	raptorhtmlpars \
	$(NATIVE_RAPTOR_WEB) \
	$(NATIVE_RAPTOR_GFX) \
	$(NATIVE_RAPTOR_WIDGET) \
	xpcom$(MOZ_BITS) \
	netlib \
	$(XP_REG_LIB)

AR_LIBS += \
              core \
              core_$(PLATFORM_DIRECTORY) \
              canvas \
              command \
              chrome \
              chrome_$(PLATFORM_DIRECTORY) \
			  dialog \
              layout \
              commandserver \
              observer \
              parser \
              shell \
              util \
              toolkit \
			  widget \
              $(NULL)

OS_LIBS += $(GUI_LIBS) $(MATH_LIB)

EXTRA_LIBS += $(NSPR_LIBS)

