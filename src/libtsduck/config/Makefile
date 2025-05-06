#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for TSDuck library configuration files.
#
#-----------------------------------------------------------------------------

DONT_BUILD_DEPS := TRUE
include ../../../Makefile.inc

CONFIGS_SRC  = $(filter-out %.skeleton.xml,$(wildcard tsduck*.xml tsduck*.names))
CONFIGS_DEST = $(addprefix $(BINDIR)/,$(CONFIGS_SRC))
TABLES_SRC   = tsduck.tables.skeleton.xml
TABLES_SUBS  = $(wildcard ../dtv/tables/*/*.xml ../dtv/descriptors/*/*.xml ../dtv/dsmcc/*.xml)
TABLES_DEST  = $(BINDIR)/tsduck.tables.model.xml
NAMES_SRC    = $(wildcard ../dtv/*/*.names ../dtv/*/*/*.names)
NAMES_DEST   = $(BINDIR)/tsduck.dtv.names
DEKTEC_DEST  = $(BINDIR)/tsduck.dektec.names

# Main build targets.

default: $(CONFIGS_DEST) $(NAMES_DEST) $(DEKTEC_DEST) $(TABLES_DEST)
	@true

# Copy TSDuck configuration files in output bin directory.

$(BINDIR)/%: %
	$(call LOG,[COPY] $<) mkdir -p $(BINDIR); cp $< $@

# Generate the complete configuration files from separate files.

$(TABLES_DEST): $(TABLES_SRC) $(TABLES_SUBS)
	$(call LOG,[GEN] $(notdir $@)) $(PYTHON) $(SCRIPTSDIR)/build-tables-model.py $@ $^

$(NAMES_DEST): $(NAMES_SRC)
	$(call LOG,[GEN] $(notdir $@)) $(PYTHON) $(SCRIPTSDIR)/build-dtv-names.py $@ $^

$(DEKTEC_DEST): $(shell $(SCRIPTSDIR)/dtapi-config.sh --header)
	$(call LOG,[GEN] $(notdir $@)) $(PYTHON) $(SCRIPTSDIR)/build-dektec-names.py $(if $<,$<,/dev/null) $@

# Install configuration files.

.PHONY: install-tools install-devel install-linux-config

install-tools: $(NAMES_DEST) $(DEKTEC_DEST) $(if $(MACOS),,install-linux-config)
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck
	install -m 644 $(CONFIGS_SRC) $(NAMES_DEST) $(DEKTEC_DEST) $(TABLES_DEST) $(SYSROOT)$(SYSPREFIX)/share/tsduck
	rm -f $(SYSROOT)$(SYSPREFIX)/share/tsduck/tsduck.names
install-linux-config:
	install -d -m 755 $(SYSROOT)$(UDEVDIR) $(SYSROOT)$(ETCDIR)/security/console.perms.d
	install -m 644 80-tsduck.rules $(SYSROOT)$(UDEVDIR)
	install -m 644 80-tsduck.perms $(SYSROOT)$(ETCDIR)/security/console.perms.d
install-devel:
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/pkgconfig
	sed -e 's|{{VERSION}}|$(shell $(GET_TSDUCK_VERSION) --dotted)|g' \
	    -e 's|{{PREFIX}}|$(SYSPREFIX)|g' \
	    -e 's|{{LIBDIR}}|$(USRLIBDIR)|g' \
	    $(if $(NOOPENSSL),-e 's|libcrypto||g') \
	    $(if $(NOZLIB),-e 's|zlib||g') \
	    $(if $(NOSRT),-e 's|srt||g') \
	    $(if $(NORIST),-e 's|librist||g') \
	    $(if $(NOPCSC),-e 's|libpcsclite||g') \
	    $(if $(NOCURL),-e 's|libcurl||g') \
	    $(if $(NOEDITLINE),-e 's|libedit||g') \
	    -e 's| *{{STDCPP}} *| $(if $(NOPCSTD),,$(CXXFLAGS_STANDARD) )|g' \
	    $(if $(NOVATEK),-e 's|libusb-1.0||g') \
	    $(if $(OPENBSD)$(NETBSD),-e 's| *-ldl *| |g') \
	    $(if $(MACOS)$(OPENBSD),-e 's| *-lrt *| |g') \
	    $(if $(MACOS)$(BSD),-e 's| *-latomic *| |g') \
	    $(if $(FREEBSD),,-e 's| *-lprocstat *| |g') \
	    $(if $(OPENBSD)$(NETBSD)$(DRAGONFLYBSD),,-e 's| *-lkvm *| |g') \
	    -e 's|:[ ,][ ,]*|: |g' -e 's|,[ ,][ ,]*|, |g' -e 's|[ ,]*$$||' -e '/: *$$/d' \
	    tsduck.pc >$(SYSROOT)$(SYSPREFIX)/share/pkgconfig/tsduck.pc
	chmod 644 $(SYSROOT)$(SYSPREFIX)/share/pkgconfig/tsduck.pc
