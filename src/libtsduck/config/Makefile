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
#  Makefile for TSDuck library configuration files.
#
#-----------------------------------------------------------------------------


DONT_BUILD_DEPS := TRUE
include ../../../Makefile.inc

CONFIGS_SRC  = $(filter-out %.skeleton.xml %.skeleton.names,$(wildcard tsduck*.xml tsduck*.names))
CONFIGS_DEST = $(addprefix $(BINDIR)/,$(CONFIGS_SRC))
TABLES_SRC   = tsduck.tables.skeleton.xml
TABLES_SUBS  = $(wildcard ../dtv/tables/*.xml ../dtv/descriptors/*.xml)
TABLES_DEST  = $(BINDIR)/tsduck.tables.model.xml
NAMES_SRC    = tsduck.dtv.skeleton.names $(wildcard ../base/*/*.names ../dtv/*/*.names)
NAMES_DEST   = $(BINDIR)/tsduck.dtv.names
DEKTEC_DEST  = $(BINDIR)/tsduck.dektec.names

# Main build targets.
# The complete signalization model, tsduck.tables.model.xml, can be generated only when tsxml is generated.

.PHONY: post-build

default: $(CONFIGS_DEST) $(NAMES_DEST) $(DEKTEC_DEST)
	@true
post-build: $(TABLES_DEST)
	@true

# Copy TSDuck configuration files in output bin directory.

$(BINDIR)/%: %
	@echo '  [COPY] $<'; \
	mkdir -p $(BINDIR); \
	cp $< $@

# Generate the complete configuration files from separate files.

$(TABLES_DEST): $(TABLES_SRC) $(TABLES_SUBS) $(BINDIR)/tsxml
	@echo '  [GEN] $(notdir $@)'; \
	LD_LIBRARY_PATH="$(BINDIR):$(LD_LIBRARY_PATH)" \
	$(if $(MACOS),DYLD_LIBRARY_PATH="$(BINDIR):$(DYLD_LIBRARY_PATH)") \
	$(BINDIR)/tsxml --merge $(TABLES_SRC) $(TABLES_SUBS) --sort _tables --sort _descriptors --uncomment -o $@

$(NAMES_DEST): $(NAMES_SRC)
	@echo '  [GEN] $(notdir $@)'; \
	$(PYTHON) $(SCRIPTSDIR)/build-dtv-names.py $@ $^

$(DEKTEC_DEST): $(shell $(SCRIPTSDIR)/dtapi-config.sh --header)
	@echo '  [GEN] $(notdir $@)'; \
	$(PYTHON) $(SCRIPTSDIR)/build-dektec-names.py $(if $<,$<,/dev/null) $@

# Install configuration files.

.PHONY: install-tools install-post-build install-devel install-linux-config

install-tools: $(NAMES_DEST) $(DEKTEC_DEST) $(if $(MACOS),,install-linux-config)
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck
	install -m 644 $(CONFIGS_SRC) $(NAMES_DEST) $(DEKTEC_DEST) $(SYSROOT)$(SYSPREFIX)/share/tsduck
	rm -f $(SYSROOT)$(SYSPREFIX)/share/tsduck/tsduck.names
install-post-build: $(TABLES_DEST)
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck
	install -m 644 $(TABLES_DEST) $(SYSROOT)$(SYSPREFIX)/share/tsduck
install-linux-config:
	install -d -m 755 $(SYSROOT)$(UDEVDIR) $(SYSROOT)$(ETCDIR)/security/console.perms.d
	install -m 644 80-tsduck.rules $(SYSROOT)$(UDEVDIR)
	install -m 644 80-tsduck.perms $(SYSROOT)$(ETCDIR)/security/console.perms.d
install-devel:
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/pkgconfig
	sed -e 's|{{VERSION}}|$(shell $(GET_VERSION) --dotted)|g' \
	    -e 's|{{PREFIX}}|$(SYSPREFIX)|g' \
	    -e 's|{{LIBDIR}}|$(USRLIBDIR)|g' \
	    $(if $(NOSRT),-e 's|srt||g') \
	    $(if $(NORIST),-e 's|librist||g') \
	    $(if $(NOPCSC),-e 's|libpcsclite||g') \
	    $(if $(NOCURL),-e 's|libcurl||g') \
	    $(if $(NOEDITLINE),-e 's|libedit||g') \
	    $(if $(NOVATEK),-e 's|libusb-1.0||g') \
	    $(if $(OPENBSD)$(NETBSD),-e 's| *-ldl *| |g') \
	    $(if $(MACOS)$(OPENBSD),-e 's| *-lrt *| |g') \
	    $(if $(MACOS)$(BSD),-e 's| *-latomic *| |g') \
	    $(if $(FREEBSD),,-e 's| *-lprocstat *| |g') \
	    $(if $(OPENBSD)$(NETBSD)$(DRAGONFLYBSD),,-e 's| *-lkvm *| |g') \
	    -e 's|:[ ,][ ,]*|: |g' -e 's|,[ ,][ ,]*|, |g' -e 's|[ ,]*$$||' -e '/: *$$/d' \
	    tsduck.pc >$(SYSROOT)$(SYSPREFIX)/share/pkgconfig/tsduck.pc
	chmod 644 $(SYSROOT)$(SYSPREFIX)/share/pkgconfig/tsduck.pc
