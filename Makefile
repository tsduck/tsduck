#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
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
#  Root makefile for the TSDuck project.
#
#  Additional options which can be defined:
#
#  - NOTEST     : Do not build unitary tests.
#  - NODEKTEC   : No Dektec device support, remove dependency to DTAPI.
#  - NOHIDES    : No HiDes device support.
#  - NOVATEK    : No Vatek-based device support.
#  - NOCURL     : No HTTP support, remove dependency to libcurl.
#  - NOPCSC     : No smartcard support, remove dependency to pcsc-lite.
#  - NOSRT      : No SRT support, remove dependency to libsrt.
#  - NORIST     : No RIST support, remove dependency to librist.
#  - NOEDITLINE : No interactive line editing, remove dependency to libedit.
#  - NOGITHUB   : No version check, no download, no upgrade from GitHub.
#  - NOHWACCEL  : Disable hardware acceleration such as crypto instructions.
#
#  Options to define the representation of bitrates:
#
#  - BITRATE_INTEGER    : Bitrates are 64-bit integer.
#  - BITRATE_FRACTION   : Bitrates are fractions of two 64-bit integers.
#  - BITRATE_FLOAT      : Bitrates are 64-bit floating-point.
#  - BITRATE_FIXED      : Bitrates are 64-bit fixed-point.
#  - BITRATE_DECIMALS=n : Number of decimal with fixed-point (default: 1).
#
#-----------------------------------------------------------------------------

include Makefile.inc

# By default, update git hooks and build TSDuck binaries.

default: git-hooks
	@$(MAKE) -C src $@

# Build and run all tests.

.PHONY: test-all
test-all: test test-suite

# Build and run unitary tests.

.PHONY: test
test: default
	@$(MAKE) -C src/utest $@

# Execute the TSDuck test suite from a sibling directory, if present.

.PHONY: test-suite
test-suite: default
	@if [[ -d ../tsduck-test/.git ]]; then \
	   cd ../tsduck-test; git pull; ./run-all-tests.sh --bin "$(BINDIR)"; \
	 elif [[ -x ../tsduck-test/run-all-tests.sh ]]; then \
	   ../tsduck-test/run-all-tests.sh --bin "$(BINDIR)"; \
	 else \
	   echo >&2 "No test repository in ../tsduck-test"; \
	 fi

# Alternative target to build with cross-compilation

.PHONY: cross
cross:
	+@$(MAKE) CROSS=true

# Alternative target to recompile with debug options

.PHONY: debug
debug:
	+@$(MAKE) DEBUG=true

# Alternative target to recompile with optimizations for reduced code size.

.PHONY: optsize
optsize:
	+@$(MAKE) CXXFLAGS_OPTIMIZE="$(CXXFLAGS_OPTSIZE)"

# Alternative target to recompile with LLVM (clang) compiler

.PHONY: llvm clang
llvm clang:
	+@$(MAKE) LLVM=true

# Alternative target to recompile with gcov support.

.PHONY: gcov
gcov:
	+@$(MAKE) DEBUG=true GCOV=true

# Alternative target to recompile with gprof support.

.PHONY: gprof
gprof:
	+@$(MAKE) DEBUG=true GPROF=true

# Alternative target to recompile for 32-bit target

.PHONY: m32
m32:
	+@$(MAKE) M32=true

# Generate the documentation.

.PHONY: doxygen
doxygen:
	@doc/build-doxygen.sh

# Cleanup utilities

.PHONY: clean distclean
clean distclean:
	@$(PYTHON) $(SCRIPTSDIR)/cleanup.py

# Build the sample applications.

.PHONY: sample
sample:
	@$(MAKE) -C sample $@

# Display the built version

.PHONY: show-version
show-version: default
	@$(BINDIR)/tsversion --version=all

# Install files, using SYSROOT as target system root if necessary.

.PHONY: install install-tools install-devel
install install-tools install-devel:
	@$(MAKE) NOTEST=true -C src $@

# Build the source tarball for distribution.

VERSION = $(shell $(GET_VERSION))
DISTRO  = $(shell $(GET_VERSION) --distro)
TARNAME = tsduck-$(VERSION)
TARFILE = $(INSTALLERDIR)/$(TARNAME).tgz
TMPROOT = $(INSTALLERDIR)/tmp

.PHONY: tarball
tarball:
	rm -rf $(TMPROOT)
	mkdir -p $(TMPROOT)/$(TARNAME)
	$(TAR) -cpf - sample/sample-*/japanese-tables.bin \
	    $(patsubst %,--exclude '%',.git $(shell cat .gitignore)) \
	    . | $(TAR) -C $(TMPROOT)/$(TARNAME) -xpf -
	$(MAKE) -C $(TMPROOT)/$(TARNAME) distclean
	$(TAR) -C $(TMPROOT) -czf $(TARFILE) -p --owner=0 --group=0 $(TARNAME)
	rm -rf $(TMPROOT)

# Installer target: rpm or deb.

INSTALLER_TYPE = $(if $(wildcard /etc/*fedora* /etc/*redhat*),rpm,$(if $(wildcard /etc/*debian*),deb))

.PHONY: installer install-installer
installer: $(INSTALLER_TYPE)
install-installer: install-$(INSTALLER_TYPE)

# User's RPM build area.

RPMBUILDROOT ?= $(HOME)/rpmbuild
$(RPMBUILDROOT):
	rpmdev-setuptree

# RPM package building (Red Hat, Fedora, CentOS, Alma Linux, Rocky Linux, Oracle Linux, etc.)
# The build will take place elsewhere, reuse local Dektec Linux SDK if present.

RPMBUILD ?= rpmbuild
RPMBUILDFLAGS = -ba --clean $(if $(M32),--target $(MAIN_ARCH) -D 'mflags M32=true') $(RPMBUILDFLAGS_EXTRA)
RPM_ARCH = $(shell uname -m)

.PHONY: rpm rpm32
rpm: tarball $(RPMBUILDROOT)
	cp -f $(TARFILE) $(RPMBUILDROOT)/SOURCES/
	DTAPI_ORIGIN="$(shell $(SCRIPTSDIR)/dtapi-config.sh --tarball)" \
	  VATEK_SRC_ORIGIN="$(shell $(SCRIPTSDIR)/vatek-config.sh --src-tarball)" \
	  VATEK_BIN_ORIGIN="$(shell $(SCRIPTSDIR)/vatek-config.sh --bin-tarball)" \
	  $(RPMBUILD) $(RPMBUILDFLAGS) \
	      -D 'version $(shell $(GET_VERSION) --main)' \
	      -D 'commit $(shell $(GET_VERSION) --commit)' \
	      -D 'distro $(DISTRO)' \
	      -D '_smp_mflags $(MAKEFLAGS_SMP)' \
	      $(if $(NOSRT),-D 'nosrt 1') \
	      $(if $(NORIST),-D 'norist 1') \
	      $(if $(NOPCSC),-D 'nopcsc 1') \
	      $(if $(NOCURL),-D 'nocurl 1') \
	      $(if $(NOEDITLINE),-D 'noeditline 1') \
	      $(if $(NOVATEK),-D 'novatek 1') \
	      $(SCRIPTSDIR)/tsduck.spec
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/RPMS/*/tsduck-devel-$(VERSION)$(DISTRO).*.rpm $(INSTALLERDIR)
	cp -uf $(RPMBUILDROOT)/SRPMS/tsduck-$(VERSION)$(DISTRO).src.rpm $(INSTALLERDIR)
rpm32:
	$(MAKE) rpm M32=true

install-rpm:
	$(SUDO) rpm -Uvh $(INSTALLERDIR)/tsduck-$(VERSION)$(DISTRO).$(RPM_ARCH).rpm $(INSTALLERDIR)/tsduck-devel-$(VERSION)$(DISTRO).$(RPM_ARCH).rpm

# DEB package building (Debian, Ubuntu, Linux Mint, Raspbian, etc.)
# Make deb-dev depend on deb-tools to force serialization in case of -j.

DEB_ARCH = $(if $(wildcard /etc/*debian*),$(shell dpkg-architecture -qDEB_BUILD_ARCH))

.PHONY: deb deb-tools deb-dev
deb: deb-tools deb-dev

deb-tools:
	rm -rf $(TMPROOT)
	$(MAKE) $(MAKEFLAGS_SMP) NOTEST=true
	$(MAKE) $(MAKEFLAGS_SMP) NOTEST=true install-tools SYSROOT=$(TMPROOT)
	install -d -m 755 $(TMPROOT)$(SYSPREFIX)/share/doc/tsduck
	install -m 644 CHANGELOG.txt LICENSE.txt OTHERS.txt doc/tsduck.pdf $(TMPROOT)$(SYSPREFIX)/share/doc/tsduck
	$(MAKE) NOTEST=true deb-tools-control
	dpkg-deb --build --root-owner-group $(TMPROOT) $(INSTALLERDIR)
	rm -rf $(TMPROOT)

deb-dev: deb-tools
	rm -rf $(TMPROOT)
	$(MAKE) $(MAKEFLAGS_SMP) NOTEST=true
	$(MAKE) $(MAKEFLAGS_SMP) NOTEST=true install-devel SYSROOT=$(TMPROOT)
	$(MAKE) NOTEST=true deb-dev-control
	dpkg-deb --build --root-owner-group $(TMPROOT) $(INSTALLERDIR)
	rm -rf $(TMPROOT)

install-deb:
	$(SUDO) dpkg -i $(INSTALLERDIR)/tsduck_$(VERSION)$(DISTRO)_$(DEB_ARCH).deb $(INSTALLERDIR)/tsduck-dev_$(VERSION)$(DISTRO)_$(DEB_ARCH).deb

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
	mkdir $(TMPROOT)/DEBIAN
	sed -e 's/{{VERSION}}/$(VERSION)$(DISTRO)/g' \
	    -e 's/{{ARCH}}/$(DEB_ARCH)/g' \
	    $(if $(NOSRT),-e '/libsrt/d',-e 's/ libsrt,/ $(call F_GETSODPKG,libsrt)/') \
	    $(if $(NORIST),-e '/librist/d',-e 's/ librist,/ $(call F_GETSODPKG,librist)/') \
	    $(if $(NOEDITLINE),-e '/libedit/d',-e 's/ libedit,/ $(call F_GETSODPKG,libedit)/') \
	    $(if $(NOVATEK),-e '/libusb/d',-e 's/ libusb,/ $(call F_GETSODPKG,libusb)/') \
	    $(if $(NOPCSC),-e '/libpcsc/d,-e 's/ libpcsc,/ $(call F_GETSODPKG,libpcsc)/') \
	    $(if $(NOCURL),-e '/libcurl/d',-e 's/ libcurl,/ $(call F_GETSODPKG,libcurl)/') \
	    $(SCRIPTSDIR)/tsduck.control >$(TMPROOT)/DEBIAN/control

deb-dev-control:
	mkdir $(TMPROOT)/DEBIAN
	sed -e 's/{{VERSION}}/$(VERSION)$(DISTRO)/g' \
	    -e 's/{{ARCH}}/$(shell dpkg-architecture -qDEB_BUILD_ARCH)/g' \
	    $(if $(NOSRT),-e '/libsrt/d',-e 's/ libsrt-dev,/ $(call F_GETDPKG,srt/srt.h)/') \
	    $(if $(NORIST),-e '/librist/d',-e 's/ librist-dev,/ $(call F_GETDPKG,librist/librist.h)/') \
	    $(if $(NOEDITLINE),-e '/libedit/d',-e 's/ libedit-dev,/ $(call F_GETDPKG,editline/readline.h)/') \
	    $(if $(NOVATEK),-e '/libusb/d',-e 's/ libusb-dev,/ $(call F_GETDPKG,libusb.h)/') \
	    $(if $(NOPCSC),-e '/libpcsc/d,-e 's/ libpcsc-dev,/ $(call F_GETDPKG,PCSC/reader.h)/') \
	    $(if $(NOCURL),-e '/libcurl/d',-e 's/ libcurl-dev,/ $(call F_GETDPKG,curl/curl.h)/') \
	    $(SCRIPTSDIR)/tsduck-dev.control >$(TMPROOT)/DEBIAN/control

# Install Git hooks.

.PHONY: git-hooks
git-hooks:
	@$(PYTHON) $(SCRIPTSDIR)/git-hook-update.py

# Count lines of code: Run cloc on the source code tree starting at current directory.

CLOC         = cloc
CLOC_SOURCES = src
CLOC_FLAGS   = --skip-uniqueness --quiet --exclude-ext=.tgz,.tar.gz,.tar,.pdf,.pptx,.docx
.PHONY: cloc
cloc:
	@$(CLOC) $(CLOC_FLAGS) $(CLOC_SOURCES) | \
	    tee /dev/stderr | grep SUM: | awk '{print "Total lines in source files:   " $$3 + $$4 + $$5}'
	@echo >&2 '-------------------------------------------'

# Static code analysis: Run Coverity.

COVERITY         = cov-build
COVERITY_DIR     = cov-int
COVERITY_SOURCES = src
.PHONY: coverity
coverity:
	rm -rf $(COVERITY_DIR)
	$(COVERITY) --dir $(COVERITY_DIR) $(MAKE) -C $(COVERITY_SOURCES)
	$(TAR) czf $(COVERITY_DIR).tgz $(COVERITY_DIR)

# Static code analysis: Run cppcheck on the source code tree.
# In debug mode, the diagnostics are more aggressive but may be false positive.

CPPCHECK         = cppcheck
CPPCHECK_SOURCES = src
CPPCHECK_FLAGS   = $(CXXFLAGS_INCLUDES) --inline-suppr --quiet --force \
	--template="{file}:{line}: ({severity}) {id}: {message}" \
	--enable=all --suppress=unusedFunction --suppress=missingIncludeSystem \
	$(if $(DEBUG),--inconclusive,)
.PHONY: cppcheck cppcheck-xml
cppcheck:
	$(CPPCHECK) $(CPPCHECK_FLAGS) $(CPPCHECK_SOURCES)
cppcheck-xml:
	$(CPPCHECK) $(CPPCHECK_FLAGS) --xml --xml-version=2 $(CPPCHECK_SOURCES)

# Static code analysis: Run flawfinder on the source code tree.

FLAWFINDER         = flawfinder
FLAWFINDER_SOURCES = src
FLAWFINDER_FLAGS   = --quiet --dataonly
.PHONY: flawfinder
flawfinder:
	$(FLAWFINDER) $(FLAWFINDER_FLAGS) $(FLAWFINDER_SOURCES)

# Static code analysis: Run scan-build on the source code tree.

SCANBUILD          = scan-build
SCANBUILD_SOURCES  = src
SCANBUILD_FLAGS    = -o $(BINDIR)
.PHONY: scan-build
scan-build:
	$(SCANBUILD) $(SCANBUILD_FLAGS) $(MAKE) -C $(SCANBUILD_SOURCES)

# Utilities: display predefined macros for C++

.PHONY: cxxmacros
cxxmacros:
	@$(CPP) $(CXXFLAGS) -x c++ -dM /dev/null | sort
