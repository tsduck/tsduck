#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for utils.
#
#-----------------------------------------------------------------------------

# Don't generate .dep for setpath.cpp. It is a Windows tool.
NODEPS_SRC = setpath.cpp

include ../../Makefile.inc

default: execs
	@true

# One source file per executable (setpath is Windows-only, tsprofiling is a test program).
EXECS := $(addprefix $(BINDIR)/,$(filter-out setpath $(if $(NOTEST)$(NOSTATIC),tsprofiling,),$(sort $(notdir $(basename $(wildcard *.cpp))))))

.PHONY: execs
execs: $(EXECS)

# We always build tsprofiling with the static libraries and plugin objects.
TSPLUGINS_OBJS = $(addprefix $(BINDIR)/objs-tsplugins/,$(addsuffix .o,$(TSPLUGINS)))
STATIC_LIBS = $(STATIC_LIBTSDUCK) $(STATIC_LIBTSCORE)
$(BINDIR)/tsprofiling: $(TSPLUGINS_OBJS) $(if $(NODTAPI),,$(STATIC_LIBTSDEKTEC)) $(STATIC_LIBS)
OTHERS_EXECS = $(filter-out $(BINDIR)/tsprofiling,$(EXECS))
CXXFLAGS_INCLUDES += $(APPS_CXXFLAGS_INCLUDES)

ifeq ($(STATIC),)
    $(BINDIR)/tsprofiling: LDLIBS_EXTRA += $(LIBTSDUCK_LDLIBS) $(LIBTSCORE_LDLIBS)
    $(OTHERS_EXECS): $(SHARED_LIBTSDUCK) $(SHARED_LIBTSCORE)
else
    LDFLAGS_EXTRA = -static
    $(OTHERS_EXECS): $(STATIC_LIBS)
endif

# The tsconfig shell script is part of the installation.
.PHONY: install install-tools
install install-tools:
	sed -e 's| *{{STDCPP}} *| $(if $(NOPCSTD),,$(CXXFLAGS_STANDARD))|g' \
	    tsconfig >$(SYSROOT)$(SYSPREFIX)/bin/tsconfig
	chmod 755 $(SYSROOT)$(SYSPREFIX)/bin/tsconfig
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/bin $(SYSROOT)$(BASHCOMP_DIR)
	install -m 644 tsconfig-completion.bash $(SYSROOT)$(BASHCOMP_DIR)/tsconfig
