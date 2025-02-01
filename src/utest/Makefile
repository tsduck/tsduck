#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for unitary tests.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

default: execs
	@true

.PHONY: execs
execs: $(BINDIR)/utest $(if $(NOSTATIC),,$(BINDIR)/utest_static)

CXXFLAGS_INCLUDES += $(APPS_CXXFLAGS_INCLUDES)

# All source files participate in one single big unitary test executable.
OBJS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(sort $(notdir $(basename $(wildcard *.cpp))))))

# Build two versions of the test executable.
# 1) Using shared object.
$(BINDIR)/utest: $(OBJS) $(SHARED_LIBTSDUCK) $(SHARED_LIBTSCORE)

# 2) Using static library. Skip plugin tests since they use the shared object.
# Add libraries which are otherwise only used by the libtsduck shared object.
$(BINDIR)/utest_static: $(filter-out $(OBJDIR)/utestPluginRepository.o,$(OBJS)) $(STATIC_LIBTSDUCK) $(STATIC_LIBTSCORE)
	$(call LOG,[LD] $@) $(CXX) $(LDFLAGS) $^ $(LIBTSCORE_LDLIBS) $(LIBTSDUCK_LDLIBS) $(LDLIBS_EXTRA) $(LDLIBS) -o $@

# Run tests.
# Use make UTESTFLAGS="-d -t ClassTest::testName" for one single test in debug mode.
# Note: We need to serialize the execution of the tests because they use common resources.
# Using ".NOTPARALLEL: test", which is supposed to serialize the specified target only,
# works on GNU Make 4.4 onwards. With older versions of GNU Make, the simple presence of
# .NOTPARALLEL serializes everything in the makefile. Since we do not want to serialize
# compilations here, we ban the use of .NOTPARALLEL and we manually serialize the tests.
.PHONY: test test-shared test-static
test:
	@$(MAKE) test-shared
	$(if $(NOSTATIC),,@$(MAKE) test-static)
test-shared: $(BINDIR)/utest
	$(call LOG,[UTEST] $(BINDIR)/utest $(UTESTFLAGS)) \
	TSPLUGINS_PATH=$(BINDIR) LD_LIBRARY_PATH=$(BINDIR) \
	TS_CURL_RETRY="RETRY=5,INTERVAL=100,HOST=tsduck.io" \
	$(BINDIR)/utest $(UTESTFLAGS)
test-static: $(BINDIR)/utest_static
	$(call LOG,[UTEST] $(BINDIR)/utest_static $(UTESTFLAGS)) \
	TSPLUGINS_PATH=$(BINDIR) LD_LIBRARY_PATH=$(BINDIR) \
	TS_CURL_RETRY="RETRY=5,INTERVAL=100,HOST=tsduck.io" \
	$(BINDIR)/utest_static $(UTESTFLAGS)

# Valgrind is well supported on Linux only. On macOS, use the builtin "leaks" tool.
ifeq ($(MACOS),)
    VALGRIND ?= valgrind
    VGFLAGS += --quiet --leak-check=full --show-leak-kinds=all --suppressions=valgrind.supp
else
    # Using MallocStackLogging is very verbose, use only when needed?
    # VALGRIND ?= MallocStackLogging=1 leaks
    VALGRIND ?= leaks
    VGFLAGS += --atExit --
endif

# Same unitary tests under valgrind control
.PHONY: valgrind valgrind-shared valgrind-static
valgrind:
	@$(MAKE) valgrind-shared
	$(if $(NOSTATIC),,@$(MAKE) valgrind-static)
valgrind-shared: $(BINDIR)/utest
	$(call LOG,[VALGRIND] $(BINDIR)/utest $(UTESTFLAGS)) \
	TSPLUGINS_PATH=$(BINDIR) LD_LIBRARY_PATH=$(BINDIR) \
	TS_CURL_RETRY="RETRY=5,INTERVAL=100,HOST=tsduck.io" \
	$(VALGRIND) $(VGFLAGS) $(BINDIR)/utest $(UTESTFLAGS)
valgrind-static: $(BINDIR)/utest_static
	$(call LOG,[VALGRIND] $(BINDIR)/utest_static $(UTESTFLAGS)) \
	TSPLUGINS_PATH=$(BINDIR) LD_LIBRARY_PATH=$(BINDIR) \
	TS_CURL_RETRY="RETRY=5,INTERVAL=100,HOST=tsduck.io" \
	$(VALGRIND) $(VGFLAGS) $(BINDIR)/utest_static $(UTESTFLAGS)
