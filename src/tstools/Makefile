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
#  Makefile for tstools.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

EXECS := $(addprefix $(BINDIR)/,$(TSTOOLS))
ALLTOOLS := $(sort $(basename $(wildcard ts*.cpp)))

default: $(EXECS) tsduck-completion.bash
	@true

tsduck-completion.bash: $(addsuffix .cpp,$(ALLTOOLS))
	@echo '  [GEN] $@'; \
	$(SED) -i -e 's/^__ts_cmds=(.*$$/__ts_cmds=($(ALLTOOLS))/' $@

ifeq ($(STATIC),)
    # With dynamic link (the default), we use the shareable library.
    $(EXECS): $(SHARED_LIBTSDUCK)
else
    # With static link, we compile in a specific directory and we link tsp with all plugins.
    LDFLAGS_EXTRA += -static
    LDLIBS_EXTRA += $(LIBTSDUCK_LDLIBS)
    $(BINDIR)/tsp: $(addprefix $(BINDIR)/objs-tsplugins/,$(addsuffix .o,$(TSPLUGINS)))
    $(EXECS): $(STATIC_LIBTSDUCK)
endif

# Some executables depend on PCSC.
$(BINDIR)/tssmartcard: LDLIBS_EXTRA = $(LDLIBS_PCSC)

.PHONY: install install-tools
install install-tools: $(EXECS) tsduck-completion.bash
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/bin $(SYSROOT)$(BASHCOMP_DIR)
	install -m 755 $(EXECS) $(SYSROOT)$(SYSPREFIX)/bin
	install -m 644 tsduck-completion.bash $(SYSROOT)$(BASHCOMP_DIR)/$(if $(BASHCOMP_AUTO),_tsduck,tsduck)
	$(if $(BASHCOMP_AUTO),for cmd in $(ALLTOOLS); do ln -sf _tsduck $(SYSROOT)$(BASHCOMP_DIR)/$$cmd; done)
	$(if $(NO_TSTOOLS),rm -rf $(addprefix $(SYSROOT)$(SYSPREFIX)/bin/,$(NO_TSTOOLS)))
