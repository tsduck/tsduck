#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2022, Thierry Lelegard
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

# Copy TSDuck configuration files in output bin directory.

default: $(addprefix $(BINDIR)/,$(wildcard tsduck*.xml tsduck*.names) tsduck.dektec.names)
	@true

$(BINDIR)/%: %
	@echo '  [COPY] $<'; \
	mkdir -p $(BINDIR); \
	cp $< $@

# Generate the Dektec names file from DTAPI.h.

tsduck.dektec.names: $(shell $(SCRIPTSDIR)/dtapi-config.sh --header)
	@echo '  [GEN] $@'; \
	echo '# Auto-generated file' >$@; \
	echo '[DtCaps]' >>$@; \
	grep '#define *DTAPI_CAP_.* Dtapi::DtCaps([0-9][0-9]*) *//' '$<' | \
	    sed -e 's|.*Dtapi::DtCaps(||' -e 's|) *// *| = |' -e 's| *\r*$$||' >>$@

# Install configuration files.

.PHONY: install install-tools install-devel install-linux-config
install: install-tools install-devel

install-tools: tsduck.dektec.names $(if $(MACOS),,install-linux-config)
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck
	install -m 644 tsduck*.xml tsduck*.names $(SYSROOT)$(SYSPREFIX)/share/tsduck
install-linux-config:
	install -d -m 755 $(SYSROOT)$(UDEVDIR) $(SYSROOT)$(ETCDIR)/security/console.perms.d
	install -m 644 80-tsduck.rules $(SYSROOT)$(UDEVDIR)
	install -m 644 80-tsduck.perms $(SYSROOT)$(ETCDIR)/security/console.perms.d
install-devel:
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/pkgconfig
	sed -e 's|{{VERSION}}|$(shell $(SCRIPTSDIR)/get-version-from-sources.sh --dotted)|g' \
	    -e 's|{{PREFIX}}|$(SYSPREFIX)|g' \
	    -e 's|{{LIBDIR}}|$(USRLIBDIR)|g' \
	    $(if $(NOSRT),-e 's|srt||g') \
	    $(if $(NORIST),-e 's|librist||g') \
	    $(if $(NOPCSC),-e 's|libpcsclite||g') \
	    $(if $(NOCURL),-e 's|libcurl||g') \
	    $(if $(NOEDITLINE),-e 's|libedit||g') \
	    $(if $(NOVATEK),-e 's|libusb-1.0||g') \
	    $(if $(MACOS),-e 's| *-lrt *| |g' -e 's| *-latomic *| |g') \
	    -e 's|:[ ,]*|: |g' -e 's|,[ ,]*|, |g' -e 's|[ ,]*$$||' -e '/: *$$/d' \
	    tsduck.pc >$(SYSROOT)$(SYSPREFIX)/share/pkgconfig/tsduck.pc
	chmod 644 $(SYSROOT)$(SYSPREFIX)/share/pkgconfig/tsduck.pc
