#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for .deb packages.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

VERSION  = $(shell $(GET_TSDUCK_VERSION))
DISTRO   = $(shell $(GET_TSDUCK_VERSION) --distro)
DEB_ARCH = $(shell dpkg-architecture -qDEB_BUILD_ARCH)

.PHONY: default installer install-installer installer-tarball deb-tools deb-dev
default: installer
installer: deb-tools deb-dev

deb-tools:
	rm -rf $(TMPROOT)
	$(MAKE) -C $(ROOTDIR) $(MAKEFLAGS_SMP) NOTEST=true
	$(MAKE) -C $(ROOTDIR) NOTEST=true install-tools SYSROOT=$(TMPROOT)
	$(MAKE) NOTEST=true deb-tools-control
	dpkg-deb --build --root-owner-group $(TMPROOT) $(INSTALLERDIR)
	rm -rf $(TMPROOT)

deb-dev: deb-tools
	rm -rf $(TMPROOT)
	$(MAKE) -C $(ROOTDIR) $(MAKEFLAGS_SMP) NOTEST=true
	$(MAKE) -C $(ROOTDIR) NOTEST=true install-devel SYSROOT=$(TMPROOT)
	$(MAKE) NOTEST=true deb-dev-control
	dpkg-deb --build --root-owner-group $(TMPROOT) $(INSTALLERDIR)
	rm -rf $(TMPROOT)

INSTALLERS = $(INSTALLERDIR)/tsduck_$(VERSION)$(DISTRO)_$(DEB_ARCH).deb \
             $(INSTALLERDIR)/tsduck-dev_$(VERSION)$(DISTRO)_$(DEB_ARCH).deb

ifeq ($(INSTALLER_TARBALL),)
    INSTALLER_TARBALL = $(INSTALLERDIR)/tsduck.$(VERSION)$(DISTRO).$(DEB_ARCH).tgz
endif

install-installer:
	$(SUDO) dpkg -i $(INSTALLERS)

installer-tarball:
	$(TAR) -czf $(INSTALLER_TARBALL) -C $(INSTALLERDIR) $(notdir $(INSTALLERS))

# Build the DEBIAN/control files using the exact library dependencies.
# Warning: Because the command lines contain macros which analyze the
# previously built binaries, these targets must be called in a separate
# make subcommand, after the binaries are built.
# Explicitly exclude packages which notoriously provide alternate versions
# of standard library (e.g. wolfram).

F_GETDPKG   = $(addsuffix $(COMMA),$(shell dpkg -S 2>/dev/null $(1) $(2) $(3) $(4) $(5) $(6) $(7) $(8) $(9) | \
                                           grep -v -i -e wolfram | sed -e 's/:.*//' | sort -u))
F_GETSO     = $(shell ldd $(SHARED_LIBTSDUCK) \
                          $(addprefix $(BINDIR)/,$(TSTOOLS)) \
                          $(addprefix $(BINDIR)/,$(addsuffix $(SO_SUFFIX),$(TSPLUGINS))) | \
                      grep -i $(addprefix -e ,$(1) $(2) $(3) $(4) $(5) $(6) $(7) $(8) $(9)) | \
                      sed -e 's/[[:space:]]*=>.*//' -e 's/^[[:space:]]*//' | sort -u)
F_GETSODPKG = $(call F_GETDPKG,$(call F_GETSO,$(1) $(2) $(3) $(4) $(5) $(6) $(7) $(8) $(9)))

deb-tools-control:
	mkdir -p $(TMPROOT)/DEBIAN
	sed -e 's/{{VERSION}}/$(VERSION)$(DISTRO)/g' \
	    -e 's/{{ARCH}}/$(DEB_ARCH)/g' \
	    $(if $(NOOPENSSL),-e '/libssl/d',-e 's/ libssl,/ $(call F_GETSODPKG,libcrypto)/') \
	    $(if $(NOZLIB),-e '/zlib/d',-e 's/ zlib,/ $(call F_GETSODPKG,libz)/') \
	    $(if $(NOSRT),-e '/libsrt/d',-e 's/ libsrt,/ $(call F_GETSODPKG,libsrt)/') \
	    $(if $(NORIST),-e '/librist/d',-e 's/ librist,/ $(call F_GETSODPKG,librist)/') \
	    $(if $(NOEDITLINE),-e '/libedit/d',-e 's/ libedit,/ $(call F_GETSODPKG,libedit)/') \
	    $(if $(NOVATEK),-e '/libusb/d',-e 's/ libusb,/ $(call F_GETSODPKG,libusb)/') \
	    $(if $(NOPCSC),-e '/libpcsc/d',-e 's/ libpcsc,/ $(call F_GETSODPKG,libpcsc)/') \
	    $(if $(NOCURL),-e '/libcurl/d',-e 's/ libcurl,/ $(call F_GETSODPKG,libcurl)/') \
	    -e '/^ *$$/d' \
	    tsduck.control >$(TMPROOT)/DEBIAN/control

deb-dev-control:
	mkdir -p $(TMPROOT)/DEBIAN
	sed -e 's/{{VERSION}}/$(VERSION)$(DISTRO)/g' \
	    -e 's/{{ARCH}}/$(shell dpkg-architecture -qDEB_BUILD_ARCH)/g' \
	    $(if $(NOOPENSSL),-e '/libssl-dev/d') \
	    $(if $(NOZLIB),-e '/zlib/d',-e 's/ zlib-dev,/ $(call F_GETDPKG,/usr/include/zlib.h)/') \
	    $(if $(NOSRT),-e '/libsrt/d',-e 's/ libsrt-dev,/ $(call F_GETDPKG,srt/srt.h)/') \
	    $(if $(NORIST),-e '/librist/d',-e 's/ librist-dev,/ $(call F_GETDPKG,librist/librist.h)/') \
	    $(if $(NOEDITLINE),-e '/libedit/d',-e 's/ libedit-dev,/ $(call F_GETDPKG,editline/readline.h)/') \
	    $(if $(NOVATEK),-e '/libusb/d',-e 's/ libusb-dev,/ $(call F_GETDPKG,libusb.h)/') \
	    $(if $(NOPCSC),-e '/libpcsc/d',-e 's/ libpcsc-dev,/ $(call F_GETDPKG,PCSC/reader.h)/') \
	    $(if $(NOCURL),-e '/libcurl/d',-e 's/ libcurl-dev,/ $(call F_GETDPKG,curl/curl.h)/') \
	    -e '/^ *$$/d' \
	    tsduck-dev.control >$(TMPROOT)/DEBIAN/control
