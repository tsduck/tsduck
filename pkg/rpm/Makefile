#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for .rpm packages.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

VERSION = $(shell $(GET_TSDUCK_VERSION))
DISTRO  = $(shell $(GET_TSDUCK_VERSION) --distro)
TARNAME = tsduck-$(VERSION)
TARFILE = $(INSTALLERDIR)/$(TARNAME).tgz
TMPROOT = $(INSTALLERDIR)/tmp

PHONY: default installer install-installer tarball
default: installer

tarball:
	rm -rf $(TMPROOT)
	mkdir -p $(TMPROOT)/$(TARNAME)
	cd $(ROOTDIR); \
	    $(TAR) -cpf - sample/sample-*/japanese-tables.bin \
	    $(patsubst %,--exclude '%',.git $(shell cat $(ROOTDIR)/.gitignore)) \
	    . | $(TAR) -C $(TMPROOT)/$(TARNAME) -xpf -
	$(MAKE) -C $(TMPROOT)/$(TARNAME) clean
	$(TAR) -C $(TMPROOT) -czf $(TARFILE) -p --owner=0 --group=0 $(TARNAME)
	rm -rf $(TMPROOT)

RPMBUILDROOT ?= $(HOME)/rpmbuild
$(RPMBUILDROOT):
	rpmdev-setuptree

# RPM package building (Red Hat, Fedora, CentOS, Alma Linux, Rocky Linux, Oracle Linux, etc.)
# The build will take place elsewhere, reuse local Dektec Linux SDK if present.

RPMBUILD ?= rpmbuild
RPMBUILDFLAGS = -ba --clean $(RPMBUILDFLAGS_EXTRA)
RPM_ARCH = $(shell uname -m)

installer: tarball $(RPMBUILDROOT)
	cp -f $(TARFILE) $(RPMBUILDROOT)/SOURCES/
	DTAPI_ORIGIN="$(shell $(SCRIPTSDIR)/dtapi-config.sh --tarball)" \
	  VATEK_SRC_ORIGIN="$(shell $(SCRIPTSDIR)/vatek-config.sh --src-tarball)" \
	  VATEK_BIN_ORIGIN="$(shell $(SCRIPTSDIR)/vatek-config.sh --bin-tarball)" \
	  $(RPMBUILD) $(RPMBUILDFLAGS) \
	      -D 'version $(shell $(GET_TSDUCK_VERSION) --main)' \
	      -D 'commit $(shell $(GET_TSDUCK_VERSION) --commit)' \
	      -D 'distro $(DISTRO)' \
	      -D '_smp_mflags $(MAKEFLAGS_SMP)' \
	      $(if $(NOSRT),-D 'nosrt 1') \
	      $(if $(NORIST),-D 'norist 1') \
	      $(if $(NOPCSC),-D 'nopcsc 1') \
	      $(if $(NOCURL),-D 'nocurl 1') \
	      $(if $(NOEDITLINE),-D 'noeditline 1') \
	      $(if $(NOVATEK),-D 'novatek 1') \
	      tsduck.spec
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-devel-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/SRPMS/tsduck-$(VERSION)$(DISTRO).src.rpm $(INSTALLERDIR)

install-installer:
	$(SUDO) rpm -Uvh $(INSTALLERDIR)/tsduck-$(VERSION)$(DISTRO).$(RPM_ARCH).rpm $(INSTALLERDIR)/tsduck-devel-$(VERSION)$(DISTRO).$(RPM_ARCH).rpm
