#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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

# The script git-hook.sh shall be invoked for some selected Git hooks.
#  - GITHOOKS_CMD     : File which executes the git hooks (relative to repository root).
#  - GITHOOKS_LIST    : List of git hooks to update.
#  - GITHOOKS_DIR     : Target directory for git hooks.
#  - GITHOOKS_TARGETS : List of hooks to install (none if not in a git repository):
GITHOOKS_CMD     = scripts/git-hook.sh
GITHOOKS_LIST    = pre-commit post-merge
GITHOOKS_DIR     = ../.git/hooks
GITHOOKS_TARGETS = $(if $(wildcard $(GITHOOKS_DIR)),$(addprefix $(GITHOOKS_DIR)/,$(GITHOOKS_LIST)),)

# A rule to create a git hook script.
$(GITHOOKS_TARGETS): Makefile
	@echo '  [GIT] updating $(notdir $@) hook'; \
	 echo '#!/usr/bin/env bash' >$@; \
	 echo 'exec $$(dirname $$0)/../../$(GITHOOKS_CMD) $(notdir $@)' >>$@; \
	 chmod a+x "$@"

# Enforce the presence of the git hooks.
default: $(GITHOOKS_TARGETS)
	@true

# Build the source tarballs for distribution.
VERSION = $(shell ./get-version-from-sources.sh)
DISTRO  = $(shell ./get-version-from-sources.sh --distro)
TARNAME = tsduck-$(VERSION)
TARFILE = $(INSTALLERDIR)/$(TARNAME).tgz
TMPROOT = $(INSTALLERDIR)/tmp
SKIPSRC = $(if $(NOTELETEXT),tsTeletextDemux.h tsTeletextDemux.cpp tsplugin_teletext.cpp,)

.PHONY: tarball
tarball:
	rm -rf $(TMPROOT)
	mkdir -p $(TMPROOT)/$(TARNAME)
	tar -C $(ROOTDIR) $(patsubst %,--exclude '%',$(SKIPSRC) $(NOSOURCE)) -cpf - . | tar -C $(TMPROOT)/$(TARNAME) -xpf -
	$(if $(NOTELETEXT),$(SED) -e '/tsTeletextDemux.h/d' -i $(TMPROOT)/$(TARNAME)/src/libtsduck/tsduck.h,)
	$(MAKE) -C $(TMPROOT)/$(TARNAME) distclean
	tar -C $(TMPROOT) -czf $(TARFILE) -p --owner=0 --group=0 $(TARNAME)
	rm -rf $(TMPROOT)

# Installer target: rpm or deb.
installer: $(if $(wildcard /etc/*fedora* /etc/*redhat*),rpm,$(if $(wildcard /etc/*debian*),deb,))

# User's RPM build area.
RPMBUILDROOT ?= $(HOME)/rpmbuild
$(RPMBUILDROOT):
	rpmdev-setuptree

# Build the tsduck binary RPM.
# The build will take place elsewhere, reuse local Dektec Linux SDK if present.
.PHONY: rpm rpm32
rpm: tarball $(RPMBUILDROOT)
	cp -f $(TARFILE) $(RPMBUILDROOT)/SOURCES/
	DTAPI_ORIGIN="$(shell ./dtapi-config.sh --tarball)" \
	  $(RPMBUILD) $(RPMBUILDFLAGS) \
	      -D 'version $(shell ./get-version-from-sources.sh --main)' \
	      -D 'commit $(shell ./get-version-from-sources.sh --commit)' \
	      -D 'distro $(DISTRO)' \
	      -D '_smp_mflags $(MAKEFLAGS_SMP)' \
	      $(if $(NOSRT),-D 'nosrt 1',) \
	      $(if $(NORIST),-D 'norist 1',) \
	      $(if $(NOPCSC),-D 'nopcsc 1',) \
	      $(if $(NOCURL),-D 'nocurl 1',) \
	      tsduck.spec
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-devel-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/SRPMS/tsduck-$(VERSION)$(DISTRO).src.rpm $(INSTALLERDIR)
rpm32:
	$(MAKE) rpm M32=true

# Build the tsduck DEB package.
# Make deb-dev depend on deb-tools to force serialization in case of -j.
.PHONY: deb deb-tools deb-dev
deb: deb-tools deb-dev

deb-tools:
	rm -rf $(TMPROOT)
	$(MAKE) $(MAKEFLAGS_SMP) NOTEST=true -C $(ROOTDIR)
	$(MAKE) $(MAKEFLAGS_SMP) NOTEST=true -C $(ROOTDIR) install-tools SYSROOT=$(TMPROOT)
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/doc/tsduck
	install -m 644 $(ROOTDIR)/CHANGELOG.txt $(ROOTDIR)/LICENSE.txt $(ROOTDIR)/OTHERS.txt $(ROOTDIR)/doc/tsduck.pdf \
	    $(SYSROOT)$(SYSPREFIX)/share/doc/tsduck
	mkdir $(TMPROOT)/DEBIAN
	sed -e 's/{{VERSION}}/$(VERSION)$(DISTRO)/g' \
	    -e 's/{{ARCH}}/$(shell dpkg-architecture -qDEB_BUILD_ARCH)/g' \
	    $(if $(NOSRT),-e '/libsrt/d',) \
	    $(if $(NORIST),-e '/librist/d',) \
	    $(if $(NOPCSC),-e '/libpcsc/d',) \
	    $(if $(NOCURL),-e '/libcurl/d',) \
	    tsduck.control >$(TMPROOT)/DEBIAN/control
	dpkg-deb --build --root-owner-group $(TMPROOT) $(INSTALLERDIR)
	rm -rf $(TMPROOT)

deb-dev: deb-tools
	rm -rf $(TMPROOT)
	$(MAKE) $(MAKEFLAGS_SMP) NOTEST=true -C $(ROOTDIR)
	$(MAKE) $(MAKEFLAGS_SMP) NOTEST=true -C $(ROOTDIR) install-devel SYSROOT=$(TMPROOT)
	mkdir $(TMPROOT)/DEBIAN
	sed -e 's/{{VERSION}}/$(VERSION)$(DISTRO)/g' \
	    -e 's/{{ARCH}}/$(shell dpkg-architecture -qDEB_BUILD_ARCH)/g' \
	    $(if $(NOSRT),-e '/libsrt/d',) \
	    $(if $(NORIST),-e '/librist/d',) \
	    $(if $(NOPCSC),-e '/libpcsc/d',) \
	    $(if $(NOCURL),-e '/libcurl/d',) \
	    tsduck-dev.control >$(TMPROOT)/DEBIAN/control
	dpkg-deb --build --root-owner-group $(TMPROOT) $(INSTALLERDIR)
	rm -rf $(TMPROOT)
