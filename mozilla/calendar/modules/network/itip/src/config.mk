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

CFLAGS         +=-D_IMPL_NS_CALENDAR -DNSPR20
INCLUDES       += -I../inc -I$(GDEPTH)/include -I$(GDEPTH)/htmlparser/src

LIBRARY_NAME      = cal_network_itip
LIBRARY_VERSION   = 10

ARCHIVE_ONLY = 1

TARGETS = $(LIBRARY)
