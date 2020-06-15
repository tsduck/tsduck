#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2020, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
#  Makefile for the build subdirectory.
#
#-----------------------------------------------------------------------------

include ../Makefile.tsduck

EXTRA_DISTCLEAN += build-tsduck-*
NORECURSE_SUBDIRS += build-tsduck-%

# The script git-hook.sh shall be invoked for some selected Git hooks.
#  - GITHOOKS_CMD     : File which executes the git hooks (relative to repository root).
#  - GITHOOKS_LIST    : List of git hooks to update.
#  - GITHOOKS_DIR     : Target directory for git hooks.
#  - GITHOOKS_TARGETS : List of hooks to install (none if not in a git repository):
GITHOOKS_CMD     = build/git-hook.sh
GITHOOKS_LIST    = pre-commit post-merge
GITHOOKS_DIR     = ../.git/hooks
GITHOOKS_TARGETS = $(if $(wildcard $(GITHOOKS_DIR)),$(addprefix $(GITHOOKS_DIR)/,$(GITHOOKS_LIST)),)

# A rule to create a git hook script.
$(GITHOOKS_TARGETS): Makefile
	@echo '  [GIT] updating $(notdir $@) hook'; \
	 echo '#!/bin/bash' >$@; \
	 echo 'exec $$(dirname $$0)/../../$(GITHOOKS_CMD) $(notdir $@)' >>$@; \
	 chmod a+x "$@"

# At least enforce the presence of the git hooks.
default: $(GITHOOKS_TARGETS)
	+@$(RECURSE)

# Doxygen needs the TSDuck version and dot (from Graphviz).
ifneq ($(filter-out doxygen,$(MAKECMDGOALS)), $(MAKECMDGOALS))
    export TS_FULL_VERSION := $(shell ./get-version-from-sources.sh)
    export DOT_PATH := $(call F_SEARCH,dot)
    export HAVE_DOT := $(if $(DOT_PATH),YES,NO)
endif

# Install system files.
.PHONY: install install-devel
ifdef MACOS
install:
	@true
else
install: 80-tsduck.rules 80-tsduck.perms
	install -d -m 755 $(SYSROOT)$(UDEVDIR)
	install -m 644 80-tsduck.rules $(SYSROOT)$(UDEVDIR)/
	install -d -m 755 $(SYSROOT)$(ETCDIR)/security/console.perms.d
	install -m 644 80-tsduck.perms $(SYSROOT)$(ETCDIR)/security/console.perms.d/
endif
install-devel:
	@true

# Build the source tarballs for distribution.
VERSION = $(shell ./get-version-from-sources.sh)
DISTRO  = $(shell ./get-version-from-sources.sh --distro)
TARNAME = tsduck-$(VERSION)
TARFILE = $(INSTALLERDIR)/$(TARNAME).tgz
TMPROOT = $(INSTALLERDIR)/tmp
ifdef NOTELETEXT
    NOSOURCE += tsTeletextCharset.h tsTeletextDemux.h tsTeletextCharset.cpp tsTeletextDemux.cpp tsplugin_teletext.cpp
endif
.PHONY: tarball
tarball:
	rm -rf $(TMPROOT)
	mkdir -p $(TMPROOT)/$(TARNAME)
	tar -C $(ROOTDIR) $(patsubst %,--exclude '%',$(NOSOURCE)) -cpf - . | tar -C $(TMPROOT)/$(TARNAME) -xpf -
	$(MAKE) -C $(TMPROOT)/$(TARNAME) distclean
	tar -C $(TMPROOT) -czf $(TARFILE) -p --owner=0 --group=0 $(TARNAME)
	rm -rf $(TMPROOT)

# Installer target: rpm or deb.
installer: $(if $(wildcard /etc/*fedora* /etc/*redhat*),rpm,$(if $(wildcard /etc/*debian*),deb,))

# User's RPM build area.
RPMBUILDROOT ?= $(HOME)/rpmbuild
$(RPMBUILDROOT):
	rpmdev-setuptree

# Reuse local Dektec Linux SDK if present.
DTAPI_ORIGIN = $(if $(wildcard $(ROOTDIR)/dektec/LinuxSDK.version),$(ROOTDIR)/dektec/$(notdir $(shell cat $(ROOTDIR)/dektec/LinuxSDK.version)),)

# Build the tsduck binary RPM.
.PHONY: rpm rpm32
rpm: tarball $(RPMBUILDROOT)
	cp -f $(TARFILE) $(RPMBUILDROOT)/SOURCES/
	$(if $(wildcard $(DTAPI_ORIGIN)),MAKEFLAGS=DTAPI_ORIGIN=$(DTAPI_ORIGIN),) \
	  $(RPMBUILD) $(RPMBUILDFLAGS) \
	      -D 'version $(shell ./get-version-from-sources.sh --main)' \
	      -D 'commit $(shell ./get-version-from-sources.sh --commit)' \
	      -D 'distro $(DISTRO)' \
	      $(if $(NOSRT),-D 'nosrt 1',) \
	      $(if $(NOPCSC),-D 'nopcsc 1',) \
	      $(if $(NOCURL),-D 'nocurl 1',) \
	      tsduck.spec
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-devel-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/SRPMS/tsduck-$(VERSION)$(DISTRO).src.rpm $(INSTALLERDIR)
rpm32:
	$(MAKE) rpm M32=true

# Build the tsduck DEB package.
TMPDIR = /tmp/tsduck-deb-$(USER)
.PHONY: deb deb-main deb-dev
deb: deb-main deb-dev
deb-main:
	$(MAKE) NOTEST=true -C $(ROOTDIR) -j$(CPU_COUNT)
	rm -rf $(TMPDIR)
	$(MAKE) NOTEST=true -C $(ROOTDIR) install SYSROOT=$(TMPDIR)
	mkdir $(TMPDIR)/DEBIAN
	sed -e 's/{{VERSION}}/$(VERSION)$(DISTRO)/g' \
	    -e 's/{{ARCH}}/$(shell dpkg-architecture -qDEB_BUILD_ARCH)/g' \
	    $(if $(NOSRT),-e '/libsrt/d',) \
	    $(if $(NOPCSC),-e '/libpcsc/d',) \
	    $(if $(NOCURL),-e '/libcurl/d',) \
	    tsduck.control >$(TMPDIR)/DEBIAN/control
	sed -e 's|{{EXECS}}|$(addprefix /usr/bin/,$(TSEXECS))|g' \
	    tsduck.postinst >$(TMPDIR)/DEBIAN/postinst
	chmod a+x $(TMPDIR)/DEBIAN/postinst
	dpkg --build $(TMPDIR) $(INSTALLERDIR)
	rm -rf $(TMPDIR)
deb-dev:
	$(MAKE) NOTEST=true -C $(ROOTDIR) -j$(CPU_COUNT)
	rm -rf $(TMPDIR)
	$(MAKE) NOTEST=true -C $(ROOTDIR) install-devel SYSROOT=$(TMPDIR)
	mkdir $(TMPDIR)/DEBIAN
	sed -e 's/{{VERSION}}/$(VERSION)$(DISTRO)/g' \
	    -e 's/{{ARCH}}/$(shell dpkg-architecture -qDEB_BUILD_ARCH)/g' \
	    $(if $(NOSRT),-e '/libsrt/d',) \
	    $(if $(NOPCSC),-e '/libpcsc/d',) \
	    $(if $(NOCURL),-e '/libcurl/d',) \
	    tsduck-dev.control >$(TMPDIR)/DEBIAN/control
	cp tsduck-dev.postinst $(TMPDIR)/DEBIAN/postinst
	chmod a+x $(TMPDIR)/DEBIAN/postinst
	dpkg --build $(TMPDIR) $(INSTALLERDIR)
	rm -rf $(TMPDIR)
