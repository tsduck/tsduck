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
#  Makefile for unitary tests.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

default: execs
	@true

.PHONY: execs
execs: $(BINDIR)/utest $(BINDIR)/utest_static

# All source files participate in one single big unitary test executable.
OBJS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(sort $(notdir $(basename $(wildcard *.cpp))))))

# Build two versions of the test executable.
# 1) Using shared object.
$(BINDIR)/utest: $(OBJS) $(SHARED_LIBTSDUCK)

# 2) Using static library. Skip plugin tests since they use the shared object.
# Add libraries which are otherwise only used by the libtsduck shared object.
$(BINDIR)/utest_static: $(filter-out $(OBJDIR)/utestPluginRepository.o,$(OBJS)) $(STATIC_LIBTSDUCK)
	@echo '  [LD] $@'; \
	$(CXX) $(LDFLAGS) $^ $(LIBTSDUCK_LDLIBS) $(LDLIBS_EXTRA) $(LDLIBS) -o $@

# Run tests.
# Use make UTESTFLAGS="-d -t ClassTest::testName" for one single test in debug mode.
.PHONY: test
test: execs
	TSPLUGINS_PATH=$(BINDIR) LD_LIBRARY_PATH=$(BINDIR) $(BINDIR)/utest $(UTESTFLAGS)
	TSPLUGINS_PATH=$(BINDIR) LD_LIBRARY_PATH=$(BINDIR) $(BINDIR)/utest_static $(UTESTFLAGS)
