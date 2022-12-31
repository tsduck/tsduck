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
#  Makefile for utils.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

default: execs
	@true

# One source file per executable (setpath is Windows-only, tsprofiling is a test program).

EXECS := $(addprefix $(BINDIR)/,$(filter-out setpath $(if $(NOTEST),tsprofiling,),$(sort $(notdir $(basename $(wildcard *.cpp))))))

.PHONY: execs
execs: $(EXECS)

# We always build tsprofiling with the static library.

STATIC_DEPS = $(addprefix $(BINDIR)/objs-tsplugins/,$(addsuffix .o,$(TSPLUGINS))) $(STATIC_LIBTSDUCK)
ifeq ($(STATIC),)
    $(BINDIR)/tsprofiling: $(STATIC_DEPS)
    $(BINDIR)/tsprofiling: LDLIBS_EXTRA += $(LIBTSDUCK_LDLIBS)
    $(filter-out $(BINDIR)/tsprofiling,$(EXECS)): $(SHARED_LIBTSDUCK)
else
    LDFLAGS_EXTRA = -static
    $(EXECS): $(STATIC_DEPS)
endif

# The tsconfig shell script is part of the installation.

.PHONY: install install-tools
install install-tools:
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/bin $(SYSROOT)$(BASHCOMP_DIR)
	install -m 755 tsconfig $(SYSROOT)$(SYSPREFIX)/bin
	install -m 644 tsconfig-completion.bash $(SYSROOT)$(BASHCOMP_DIR)/tsconfig
