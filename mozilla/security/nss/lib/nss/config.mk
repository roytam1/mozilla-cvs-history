#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
# 
# The Original Code is the Netscape security libraries.
# 
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are 
# Copyright (C) 1994-2000 Netscape Communications Corporation.  All
# Rights Reserved.
# 
# Contributor(s):
# 
# Alternatively, the contents of this file may be used under the
# terms of the GNU General Public License Version 2 or later (the
# "GPL"), in which case the provisions of the GPL are applicable 
# instead of those above.  If you wish to allow use of your 
# version of this file only under the terms of the GPL and not to
# allow others to use your version of this file under the MPL,
# indicate your decision by deleting the provisions above and
# replace them with the notice and other provisions required by
# the GPL.  If you do not delete the provisions above, a recipient
# may use your version of this file under either the MPL or the
# GPL.
#

#
#  Override TARGETS variable so that only static libraries
#  are specifed as dependencies within rules.mk.
#

#TARGETS        = $(LIBRARY)
#SHARED_LIBRARY =
#IMPORT_LIBRARY =
#PURE_LIBRARY   =
#PROGRAM        =

# can't do this in manifest.mn because OS_ARCH isn't defined there.
ifeq ($(OS_ARCH), WINNT)

# $(PROGRAM) has explicit dependencies on $(EXTRA_LIBS)
CRYPTOLIB=$(DIST)/lib/freebl.lib
ifdef MOZILLA_SECURITY_BUILD
	CRYPTOLIB=$(DIST)/lib/crypto.lib
endif
ifdef MOZILLA_BSAFE_BUILD
	CRYPTOLIB+=$(DIST)/lib/bsafe$(BSAFEVER).lib
	CRYPTOLIB+=$(DIST)/lib/freebl.lib
endif

EXTRA_LIBS += \
	$(DIST)/lib/certhi.lib \
	$(DIST)/lib/cryptohi.lib \
	$(DIST)/lib/pk11wrap.lib \
	$(DIST)/lib/certdb.lib \
	$(DIST)/lib/softoken.lib \
	$(CRYPTOLIB) \
	$(DIST)/lib/secutil.lib \
	$(DIST)/lib/dbm.lib \
	$(DIST)/lib/$(NSPR31_LIB_PREFIX)plc4.lib \
	$(DIST)/lib/$(NSPR31_LIB_PREFIX)plds4.lib \
	$(DIST)/lib/$(NSPR31_LIB_PREFIX)nspr4.lib \
	$(NULL)

# $(PROGRAM) has NO explicit dependencies on $(OS_LIBS)
#OS_LIBS += \
#	wsock32.lib \
#	winmm.lib \
#	$(NULL)
else

# $(PROGRAM) has explicit dependencies on $(EXTRA_LIBS)
CRYPTOLIB=$(DIST)/lib/libfreebl.$(LIB_SUFFIX)
CRYPTODIR=../freebl
ifdef MOZILLA_SECURITY_BUILD
	CRYPTOLIB=$(DIST)/lib/libcrypto.$(LIB_SUFFIX)
	CRYPTODIR=../crypto
endif
SHARED_LIBRARY_LIBS = \
	$(DIST)/lib/libpkcs7.$(LIB_SUFFIX) \
	$(DIST)/lib/libcerthi.$(LIB_SUFFIX) \
	$(DIST)/lib/libpk11wrap.$(LIB_SUFFIX) \
	$(DIST)/lib/libcryptohi.$(LIB_SUFFIX) \
	$(DIST)/lib/libsoftoken.$(LIB_SUFFIX) \
	$(DIST)/lib/libcertdb.$(LIB_SUFFIX) \
	$(CRYPTOLIB) \
	$(DIST)/lib/libsecutil.$(LIB_SUFFIX) \
	$(NULL)
EXTRA_LIBS += \
	$(DIST)/lib/libdbm.$(LIB_SUFFIX) \
	$(NULL)
ifdef MOZILLA_BSAFE_BUILD
	EXTRA_LIBS+=$(DIST)/lib/libbsafe.$(LIB_SUFFIX)
endif
SHARED_LIBRARY_DIRS = \
	../pkcs7 \
	../certhigh \
	../pk11wrap \
	../cryptohi \
	../softoken \
	../certdb \
	$(CRYPTODIR) \
	../util \
	$(NULL)

# $(PROGRAM) has NO explicit dependencies on $(EXTRA_SHARED_LIBS)
# $(EXTRA_SHARED_LIBS) come before $(OS_LIBS), except on AIX.
EXTRA_SHARED_LIBS += \
	-L$(DIST)/lib/ \
	-lplc4 \
	-lplds4 \
	-lnspr4 \
	$(NULL)
endif

#ifeq ($(OS_ARCH),SunOS)
#MKSHLIB += -z text -M mapfile
#endif

#ifeq ($(OS_ARCH),Linux)
#MKSHLIB += -Wl,--version-script,mapfile
#endif
