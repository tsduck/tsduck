#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for .rpm packages.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

VERSION = $(shell $(GET_TSDUCK_VERSION))
DISTRO  = $(shell $(GET_TSDUCK_VERSION) --distro)

PHONY: default installer install-installer installer-tarball
default: installer

RPMBUILDROOT ?= $(HOME)/rpmbuild
$(RPMBUILDROOT):
	rpmdev-setuptree

# RPM package building (Red Hat, Fedora, CentOS, Alma Linux, Rocky Linux, Oracle Linux, etc.)
# The build will take place elsewhere, reuse local Dektec Linux SDK if present.

RPMBUILD ?= rpmbuild
RPMBUILDFLAGS = -ba --clean $(RPMBUILDFLAGS_EXTRA)
RPM_ARCH = $(shell uname -m)

installer: tarball $(RPMBUILDROOT)
	cp -f $(SOURCE_TARBALL) $(RPMBUILDROOT)/SOURCES/
	DTAPI_ORIGIN="$(shell $(SCRIPTSDIR)/dtapi-config.sh --tarball)" \
	  VATEK_SRC_ORIGIN="$(shell $(SCRIPTSDIR)/vatek-config.sh --src-tarball)" \
	  VATEK_BIN_ORIGIN="$(shell $(SCRIPTSDIR)/vatek-config.sh --bin-tarball)" \
	  $(RPMBUILD) $(RPMBUILDFLAGS) \
	      -D 'version $(shell $(GET_TSDUCK_VERSION) --main)' \
	      -D 'commit $(shell $(GET_TSDUCK_VERSION) --commit)' \
	      -D 'distro $(DISTRO)' \
	      $(if $(MAKEFLAGS_SMP),-D '_smp_mflags $(MAKEFLAGS_SMP)') \
	      $(if $(NOOPENSSL),-D 'noopenssl 1') \
	      $(if $(NOZLIB),-D 'nozlib 1') \
	      $(if $(NOSRT),-D 'nosrt 1') \
	      $(if $(NORIST),-D 'norist 1') \
	      $(if $(NOPCSC),-D 'nopcsc 1') \
	      $(if $(NOCURL),-D 'nocurl 1') \
	      $(if $(NOEDITLINE),-D 'noeditline 1') \
	      $(if $(NODEKTEC)$(NODTAPI),-D 'nodektec 1') \
	      $(if $(NOVATEK),-D 'novatek 1') \
	      $(if $(NODOC),-D 'nodoc 1') \
	      tsduck.spec
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-devel-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/SRPMS/tsduck-$(VERSION)$(DISTRO).src.rpm $(INSTALLERDIR)

INSTALLERS = $(INSTALLERDIR)/tsduck-$(VERSION)$(DISTRO).$(RPM_ARCH).rpm \
             $(INSTALLERDIR)/tsduck-devel-$(VERSION)$(DISTRO).$(RPM_ARCH).rpm

ifeq ($(INSTALLER_TARBALL),)
    INSTALLER_TARBALL = $(INSTALLERDIR)/tsduck.$(VERSION)$(DISTRO).$(RPM_ARCH).tgz
endif

install-installer:
	$(SUDO) rpm -Uvh $(INSTALLERS)

installer-tarball:
	$(TAR) -czf $(INSTALLER_TARBALL) -C $(INSTALLERDIR) $(notdir $(INSTALLERS))
