#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for TSDuck core library configuration files.
#
#-----------------------------------------------------------------------------

DONT_BUILD_DEPS := TRUE
include ../../../Makefile.inc

CONFIGS_SRC  = $(wildcard tscore.*.xml tscore.*.names)
CONFIGS_DEST = $(addprefix $(BINDIR)/,$(CONFIGS_SRC))

# Main build targets.

default: $(CONFIGS_DEST)
	@true

# Copy TSDuck configuration files in output bin directory.

$(BINDIR)/%: %
	$(call LOG,[COPY] $<) mkdir -p $(BINDIR); cp $< $@

# Install configuration files.

.PHONY: install-tools install-devel install-linux-config

install-tools:
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck
	install -m 644 $(CONFIGS_SRC) $(SYSROOT)$(SYSPREFIX)/share/tsduck
	cd $(SYSROOT)$(SYSPREFIX)/share/tsduck; rm -f tsduck.names tsduck.ip.* tsduck.keytable.* tsduck.monitor.* tsduck.time.*

install-devel:
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/pkgconfig
	sed -e 's|{{VERSION}}|$(shell $(GET_TSDUCK_VERSION) --dotted)|g' \
	    -e 's|{{PREFIX}}|$(SYSPREFIX)|g' \
	    -e 's|{{LIBDIR}}|$(USRLIBDIR)|g' \
	    $(if $(NOOPENSSL),-e 's|libcrypto||g') \
	    $(if $(NOZLIB),-e 's|zlib||g') \
	    $(if $(NOCURL),-e 's|libcurl||g') \
	    $(if $(NOEDITLINE),-e 's|libedit||g') \
	    -e 's| *{{STDCPP}} *| $(if $(NOPCSTD),,$(CXXFLAGS_STANDARD) )|g' \
	    $(if $(OPENBSD)$(NETBSD),-e 's| *-ldl *| |g') \
	    $(if $(MACOS)$(OPENBSD),-e 's| *-lrt *| |g') \
	    $(if $(MACOS)$(BSD),-e 's| *-latomic *| |g') \
	    $(if $(FREEBSD),,-e 's| *-lprocstat *| |g') \
	    $(if $(OPENBSD)$(NETBSD)$(DRAGONFLYBSD),,-e 's| *-lkvm *| |g') \
	    -e 's|:[ ,][ ,]*|: |g' -e 's|,[ ,][ ,]*|, |g' -e 's|[ ,]*$$||' -e '/: *$$/d' \
	    tscore.pc >$(SYSROOT)$(SYSPREFIX)/share/pkgconfig/tscore.pc
	chmod 644 $(SYSROOT)$(SYSPREFIX)/share/pkgconfig/tscore.pc
