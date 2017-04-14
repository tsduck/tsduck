#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2017, Thierry Lelegard
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
#  There is nothing to build here, but cleanup is required.
#
#-----------------------------------------------------------------------------

include ../Makefile.tsduck

default:
	@true

# Install system files.
.PHONY: install
install: 80-tsduck.rules 80-tsduck.perms
	install -d -m 755 $(DESTDIR)/etc/udev/rules.d
	install -m 644 80-tsduck.rules $(DESTDIR)/etc/udev/rules.d/
	install -d -m 755 $(DESTDIR)/etc/security/console.perms.d
	install -m 644 80-tsduck.perms $(DESTDIR)/etc/security/console.perms.d/

# Build the source tarballs for distribution.
TARNAME := tsduck-$(TS_VERSION)
TARFILE := $(INSTALLERDIR)/$(TARNAME).tgz
TMPROOT := $(INSTALLERDIR)/tmp
.PHONY: tarball
tarball:
	rm -rf $(TMPROOT)
	mkdir -p $(TMPROOT)/$(TARNAME)
	tar -C $(ROOTDIR) $(patsubst %,--exclude '%',$(NOSOURCE)) -cpf - . | tar -C $(TMPROOT)/$(TARNAME) -xpf -
	$(MAKE) -C $(TMPROOT)/$(TARNAME) distclean
	tar -C $(TMPROOT) -czf $(TARFILE) -p --owner=0 --group=0 $(TARNAME)
	rm -rf $(TMPROOT)

# Linux distro version.
FC_DISTRO   := $(shell grep " release " /etc/fedora-release 2>/dev/null | sed -e 's/^.* release \([0-9]*\).*$$/\1/')
RH_DISTRO   := $(shell grep " release " /etc/redhat-release 2>/dev/null | sed -e 's/^.* release \([0-9]*\).*$$/\1/')
DISTRO      := $(if $(FC_DISTRO),.fc$(FC_DISTRO),$(if $(RH_DISTRO),.el$(RH_DISTRO),))
RPM_RELEASE := 0$(DISTRO)

# Build the tsduck binary RPM.
RPMBUILDROOT ?= $(HOME)/rpmbuild
$(RPMBUILDROOT):
	rpmdev-setuptree
.PHONY: rpm
rpm: tarball $(RPMBUILDROOT)
	cp -f $(TARFILE) $(RPMBUILDROOT)/SOURCES/
	MAKEFLAGS= rpmbuild -bb --quiet -D 'version $(TS_VERSION)' -D 'release $(RPM_RELEASE)' tsduck.spec
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-$(TS_VERSION)-$(RPM_RELEASE).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-devel-$(TS_VERSION)-$(RPM_RELEASE).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/SRPMS/tsduck-$(TS_VERSION)-$(RPM_RELEASE).src.rpm $(INSTALLERDIR)
