#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
