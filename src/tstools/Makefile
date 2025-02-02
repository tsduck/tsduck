#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for tstools.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

EXECS := $(addprefix $(BINDIR)/,$(TSTOOLS))
ALLTOOLS := $(sort $(basename $(wildcard ts*.cpp)))

CXXFLAGS_INCLUDES += $(APPS_CXXFLAGS_INCLUDES)

default: $(EXECS) tsduck-completion.bash
	@true

tsduck-completion.bash: $(addsuffix .cpp,$(ALLTOOLS))
	$(call LOG,[GEN] $@) $(SED) -i -e 's/^__ts_cmds=(.*$$/__ts_cmds=($(ALLTOOLS))/' $@

# Some executables depend on PCSC.
$(BINDIR)/tssmartcard: LDLIBS_EXTRA = $(LDLIBS_PCSC)

# Most executables need libstuck but some need libtscore only.
TSCORE_ONLY = $(addprefix $(BINDIR)/,tscrc32 tsftrunc tsresync)

ifeq ($(STATIC),)
    # With dynamic link (the default), we use the shareable library.
    $(filter-out $(TSCORE_ONLY),$(EXECS)): $(SHARED_LIBTSDUCK) $(SHARED_LIBTSCORE)
    $(TSCORE_ONLY): $(SHARED_LIBTSCORE)
else
    # With static link, we compile in a specific directory and we link tsp with all plugins.
    LDFLAGS_EXTRA += -static
    LDLIBS_EXTRA += $(LIBTSDUCK_LDLIBS)
    $(BINDIR)/tsp: $(addprefix $(BINDIR)/objs-tsplugins/,$(addsuffix .o,$(TSPLUGINS)))
    $(filter-out $(TSCORE_ONLY),$(EXECS)): $(STATIC_LIBTSDUCK) $(STATIC_LIBTSCORE)
    $(TSCORE_ONLY): $(STATIC_LIBTSCORE)
endif

.PHONY: install install-tools
install install-tools: $(EXECS) tsduck-completion.bash
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/bin $(SYSROOT)$(BASHCOMP_DIR)
	install -m 755 $(EXECS) $(SYSROOT)$(SYSPREFIX)/bin
	install -m 644 tsduck-completion.bash $(SYSROOT)$(BASHCOMP_DIR)/$(if $(BASHCOMP_AUTO),_tsduck,tsduck)
	$(if $(BASHCOMP_AUTO),for cmd in $(ALLTOOLS); do ln -sf _tsduck $(SYSROOT)$(BASHCOMP_DIR)/$$cmd; done)
	$(if $(NO_TSTOOLS),rm -rf $(addprefix $(SYSROOT)$(SYSPREFIX)/bin/,$(NO_TSTOOLS)))
