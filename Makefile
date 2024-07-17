#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Root makefile for the TSDuck project.
#
#  Additional options which can be defined:
#
#  - NOTEST      : Do not build unitary tests.
#  - NOSTATIC    : Do not build the static library and the static tests.
#  - NODEKTEC    : No Dektec device support, remove dependency to DTAPI.
#  - NOHIDES     : No HiDes device support.
#  - NOVATEK     : No Vatek-based device support.
#  - NOCURL      : No HTTP support, remove dependency to libcurl.
#  - NOPCSC      : No smartcard support, remove dependency to pcsc-lite.
#  - NOSRT       : No SRT support, remove dependency to libsrt.
#  - NORIST      : No RIST support, remove dependency to librist.
#  - NOEDITLINE  : No interactive line editing, remove dependency to libedit.
#  - NOGITHUB    : No version check, no download, no upgrade from GitHub.
#  - NOHWACCEL   : Disable hardware acceleration such as crypto instructions.
#  - NOPCSTD     : Remove the std=c++17 flag from libtsduck's pkg-config file.
#  - NODEPRECATE : Do not flag legacy methods as deprecated.
#  - VERBOSE     : Display full commands. Can be abbreviated as V.
#
#  Options to define the representation of bitrates:
#
#  - BITRATE_INTEGER    : Bitrates are 64-bit integer.
#  - BITRATE_FRACTION   : Bitrates are fractions of two 64-bit integers.
#  - BITRATE_FLOAT      : Bitrates are 64-bit floating-point.
#  - BITRATE_FIXED      : Bitrates are 64-bit fixed-point.
#  - BITRATE_DECIMALS=n : Number of decimal with fixed-point (default: 1).
#
#-----------------------------------------------------------------------------

include Makefile.inc

# By default, update git hooks and build TSDuck binaries.

default: git-hooks
	@$(MAKE) -C src $@

# Build and run all tests.

.PHONY: test-all
test-all: test test-suite

# Build and run unitary tests.

.PHONY: test test-static test-shared
test test-static test-shared: default
	@$(MAKE) -C src/utest $@

# Build and run unitary tests under valgrind control.

.PHONY: valgrind valgrind-shared valgrind-static
valgrind valgrind-shared valgrind-static:
	@$(MAKE) DEBUG=true
	@$(MAKE) DEBUG=true -C src/utest $@

# Execute the TSDuck test suite from a sibling directory, if present.

.PHONY: test-suite
test-suite: default
	@if [[ -d ../tsduck-test/.git ]]; then \
	   cd ../tsduck-test; git pull; ./run-all-tests.sh --bin "$(BINDIR)"; \
	 elif [[ -x ../tsduck-test/run-all-tests.sh ]]; then \
	   ../tsduck-test/run-all-tests.sh --bin "$(BINDIR)"; \
	 else \
	   echo >&2 "No test repository in ../tsduck-test"; \
	 fi

# Alternative target to build with cross-compilation

.PHONY: cross
cross:
	+@$(MAKE) CROSS=true

# Alternative target to recompile with debug options

.PHONY: debug
debug:
	+@$(MAKE) DEBUG=true

# Alternative target to recompile with optimizations for reduced code size.

.PHONY: optsize
optsize:
	+@$(MAKE) CXXFLAGS_OPTIMIZE="$(CXXFLAGS_OPTSIZE)"

# Alternative target to recompile with LLVM (clang) compiler

.PHONY: llvm clang
llvm clang:
	+@$(MAKE) LLVM=true

# Alternative target to recompile with gcov support.

.PHONY: gcov
gcov:
	+@$(MAKE) DEBUG=true GCOV=true

# Alternative target to recompile with gprof support.

.PHONY: gprof
gprof:
	+@$(MAKE) DEBUG=true GPROF=true

# Alternative target to recompile for 32-bit target

.PHONY: m32
m32:
	+@$(MAKE) M32=true

# Generate the documentation.

DOC_TARGETS = doxygen docs docs-html docs-pdf \
    userguide userguide-html userguide-pdf open-userguide open-userguide-html open-userguide-pdf \
    devguide devguide-html devguide-pdf open-devguide open-devguide-html open-devguide-pdf

.PHONY: $(DOC_TARGETS)
$(DOC_TARGETS):
	@$(MAKE) -C doc $@

# Cleanup utilities

.PHONY: clean distclean
clean distclean:
	@$(PYTHON) $(SCRIPTSDIR)/cleanup.py

# Build the sample applications.

.PHONY: sample
sample:
	@$(MAKE) -C sample $@

# Display the built version

.PHONY: show-version
show-version: default
	@$(BINDIR)/tsversion --version=all

# Install files, using SYSROOT as target system root if necessary.

.PHONY: install install-tools install-devel
install install-tools install-devel:
	@$(MAKE) NOTEST=true -C src $@
	@$(MAKE) NOTEST=true -C doc $@

# Build installer packages.

.PHONY: installer install-installer
installer install-installer:
	@$(MAKE) -C pkg $@

# Install Git hooks.

.PHONY: git-hooks
git-hooks:
	@$(PYTHON) $(SCRIPTSDIR)/git-hook-update.py

# Count lines of code: Run cloc on the source code tree starting at current directory.

CLOC         = cloc
CLOC_SOURCES = src
CLOC_FLAGS   = --skip-uniqueness --quiet --exclude-ext=.tgz,.tar.gz,.tar,.pdf,.pptx,.docx
.PHONY: cloc
cloc:
	@$(CLOC) $(CLOC_FLAGS) $(CLOC_SOURCES) | \
	    tee /dev/stderr | grep SUM: | awk '{print "Total lines in source files:   " $$3 + $$4 + $$5}'
	@echo >&2 '-------------------------------------------'

# Static code analysis: Run Coverity.

COVERITY         = cov-build
COVERITY_DIR     = cov-int
COVERITY_SOURCES = src
.PHONY: coverity
coverity:
	rm -rf $(COVERITY_DIR)
	$(COVERITY) --dir $(COVERITY_DIR) $(MAKE) -C $(COVERITY_SOURCES)
	$(TAR) czf $(COVERITY_DIR).tgz $(COVERITY_DIR)

# Static code analysis: Run cppcheck on the source code tree.
# In debug mode, the diagnostics are more aggressive but may be false positive.

CPPCHECK         = cppcheck
CPPCHECK_SOURCES = src
CPPCHECK_FLAGS   = $(CXXFLAGS_INCLUDES) --inline-suppr --quiet --force \
	--template="{file}:{line}: ({severity}) {id}: {message}" \
	--enable=all --suppress=unusedFunction --suppress=missingIncludeSystem \
	$(if $(DEBUG),--inconclusive,)
.PHONY: cppcheck cppcheck-xml
cppcheck:
	$(CPPCHECK) $(CPPCHECK_FLAGS) $(CPPCHECK_SOURCES)
cppcheck-xml:
	$(CPPCHECK) $(CPPCHECK_FLAGS) --xml --xml-version=2 $(CPPCHECK_SOURCES)

# Static code analysis: Run flawfinder on the source code tree.

FLAWFINDER         = flawfinder
FLAWFINDER_SOURCES = src
FLAWFINDER_FLAGS   = --quiet --dataonly
.PHONY: flawfinder
flawfinder:
	$(FLAWFINDER) $(FLAWFINDER_FLAGS) $(FLAWFINDER_SOURCES)

# Static code analysis: Run scan-build on the source code tree.

SCANBUILD          = scan-build
SCANBUILD_SOURCES  = src
SCANBUILD_FLAGS    = -o $(BINDIR)
.PHONY: scan-build
scan-build:
	$(SCANBUILD) $(SCANBUILD_FLAGS) $(MAKE) -C $(SCANBUILD_SOURCES)

# Utilities: display predefined macros for C++

.PHONY: cxxmacros
cxxmacros:
	@$(CPP) $(CXXFLAGS) -x c++ -dM /dev/null | sort
