#! gmake

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

MOD_DEPTH = .

DIRS = config pr lib

ifdef MOZILLA_CLIENT
PR_CLIENT_BUILD = 1
PR_CLIENT_BUILD_UNIX = 1
endif

include $(MOD_DEPTH)/config/rules.mk

#
# The -ll option of zip converts CR LF to LF.
#
ifeq ($(OS_ARCH),WINNT)
ZIP_ASCII_OPT = -ll
endif

ifdef PR_CLIENT_BUILD
export::
	rm -r -f $(DIST)/../public/nspr
ifdef PR_CLIENT_BUILD_UNIX
	rm -f $(DIST)/lib/libnspr.a
	rm -f $(DIST)/bin/libnspr.$(DLL_SUFFIX)
endif
endif

distclean::
	@echo "cd pr/tests; $(MAKE) $@"
	@$(MAKE) -C pr/tests $@

release::
	echo $(BUILD_NUMBER) > $(RELEASE_DIR)/$(BUILD_NUMBER)/version.df
	@if test -f imports.df; then \
	    echo "cp -f imports.df $(RELEASE_DIR)/$(BUILD_NUMBER)/imports.df"; \
	    cp -f imports.df $(RELEASE_DIR)/$(BUILD_NUMBER)/imports.df; \
	else \
	    echo "echo > $(RELEASE_DIR)/$(BUILD_NUMBER)/imports.df"; \
	    echo > $(RELEASE_DIR)/$(BUILD_NUMBER)/imports.df; \
	fi
	cd $(RELEASE_DIR)/$(BUILD_NUMBER)/$(OBJDIR_NAME); \
	rm -rf META-INF; mkdir META-INF; cd META-INF; \
	echo "Manifest-Version: 1.0" > MANIFEST.MF; \
	echo "" >> MANIFEST.MF; \
	cd ..; rm -f mdbinary.jar; zip -r mdbinary.jar META-INF bin lib; \
	rm -rf META-INF; \
	cd include; \
	rm -rf META-INF; mkdir META-INF; cd META-INF; \
	echo "Manifest-Version: 1.0" > MANIFEST.MF; \
	echo "" >> MANIFEST.MF; \
	cd ..; rm -f mdheader.jar; zip $(ZIP_ASCII_OPT) -r mdheader.jar *; \
	rm -rf META-INF
ifeq ($(OS_ARCH),WINNT)
	@if test ! -d $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER); then \
		rm -rf $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER); \
		echo "making directory /m/dist/$(MOD_NAME)/$(BUILD_NUMBER)"; \
		config/prmkdir.bat $(MDIST_DOS)\\$(MOD_NAME)\\$(BUILD_NUMBER); \
	fi
	@if test ! -d $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME); then \
		rm -rf $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME); \
		echo "making directory /m/dist/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME)"; \
		config/prmkdir.bat $(MDIST_DOS)\\$(MOD_NAME)\\$(BUILD_NUMBER)\\$(OBJDIR_NAME); \
	fi
else
	@if test ! -d $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER); then \
		rm -rf $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER); \
		echo "making directory /m/dist/$(MOD_NAME)/$(BUILD_NUMBER)"; \
		$(NSINSTALL) -D $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER); \
		chmod 775 $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER); \
	fi
	@if test ! -d $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME); then \
		rm -rf $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME); \
		echo "making directory /m/dist/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME)"; \
		$(NSINSTALL) -D $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME); \
		chmod 775 $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME); \
	fi
endif
	cd $(RELEASE_DIR)/$(BUILD_NUMBER); \
	cp -f version.df imports.df $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER); \
	chmod 664 $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/version.df; \
	chmod 664 $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/imports.df; \
	cd $(OBJDIR_NAME); \
	cp -f mdbinary.jar $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME); \
	chmod 664 $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME)/mdbinary.jar; \
	cd include; \
	cp -f mdheader.jar $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME); \
	chmod 664 $(MDIST)/$(MOD_NAME)/$(BUILD_NUMBER)/$(OBJDIR_NAME)/mdheader.jar

depend:
	@echo "NSPR20 has no dependencies.  Skipped."
