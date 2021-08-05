#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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

OBJSUBDIR := objs-utils
include ../../Makefile.tsduck

# Filter out Windows-only tools.
EXECS := $(filter-out $(BINDIR)/setpath $(if $(NOTEST),$(BINDIR)/tsprofiling,),$(EXECS))

default: execs
	@true

.PHONY: execs
execs: $(EXECS)

# We always build tsprofiling with the static library.
STATIC_DEPS = $(addprefix $(BINDIR)/objs-tsplugins/,$(addsuffix .o,$(TSPLUGINS))) $(STATIC_LIBTSDUCK)
ifndef STATIC
    $(BINDIR)/tsprofiling: $(STATIC_DEPS)
    $(filter-out $(BINDIR)/tsprofiling,$(EXECS)): $(SHARED_LIBTSDUCK)
else
    LDFLAGS_EXTRA = -static
    $(EXECS): $(STATIC_DEPS)
endif

.PHONY: install install-tools install-devel
install: install-tools install-devel
install-tools:
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/bin
	install -m 755 tsconfig $(SYSROOT)$(SYSPREFIX)/bin
install-devel:
	@true
