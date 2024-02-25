#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
	$(call LOG,[LD] $@) $(CXX) $(LDFLAGS) $^ $(LIBTSDUCK_LDLIBS) $(LDLIBS_EXTRA) $(LDLIBS) -o $@

# Run tests.
# Use make UTESTFLAGS="-d -t ClassTest::testName" for one single test in debug mode.
.PHONY: test
test: execs
	$(call LOG,[UTEST] $(BINDIR)/utest $(UTESTFLAGS)) \
	TSPLUGINS_PATH=$(BINDIR) LD_LIBRARY_PATH=$(BINDIR) \
	TS_CURL_RETRY="RETRY=5,INTERVAL=100,HOST=tsduck.io" \
	$(BINDIR)/utest $(UTESTFLAGS)
	$(call LOG,[UTEST] $(BINDIR)/utest_static $(UTESTFLAGS)) \
	TSPLUGINS_PATH=$(BINDIR) LD_LIBRARY_PATH=$(BINDIR) \
	TS_CURL_RETRY="RETRY=5,INTERVAL=100,HOST=tsduck.io" \
	$(BINDIR)/utest_static $(UTESTFLAGS)
