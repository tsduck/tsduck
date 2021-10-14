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
#  Root makefile for the TSDuck project.
#
#  Additional options which can be defined:
#
#  - NOTEST     : Do not build unitary tests.
#  - NODEKTEC   : No Dektec support, remove dependency to DTAPI.
#  - NOCURL     : No HTTP support, remove dependency to libcurl.
#  - NOPCSC     : No smartcard support, remove dependency to pcsc-lite.
#  - NOSRT      : No SRT support, remove dependency to libsrt.
#  - NORIST     : No RIST support, remove dependency to librist.
#  - NOEDITLINE : No interactive line editing, remove dependency to libedit.
#  - NOTELETEXT : No Teletext support, remove teletext handling code.
#  - NOGITHUB   : No version check, no download, no upgrade from GitHub.
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


include Makefile.tsduck

# By default, build TSDuck binaries.
default:
	$(MAKE) -C scripts $@
	$(MAKE) -C src $@

# Build and run all tests.
.PHONY: test-all
test-all: test test-suite

# Build and run unitary tests.
.PHONY: test
test: default
	@$(MAKE) -C src/utest $@

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
	+@$(MAKE) CFLAGS_OPTIMIZE="$(CFLAGS_OPTSIZE)"

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
.PHONY: doxygen
doxygen:
	@doc/build-doxygen.sh

# Cleanup utilities
.PHONY: clean distclean
clean distclean:
	@scripts/cleanup.py

# Build the sample applications.
.PHONY: sample
sample:
	@$(MAKE) -C sample $@

# Display the built version
.PHONY: show-version
show-version: default
	$(BINDIR)/tsversion --version=all

# Install files, using SYSROOT as target system root if necessary.
.PHONY: install install-tools install-devel
install install-tools install-devel:
	$(MAKE) NOTEST=true -C src $@

# Installers build targets are redirected to build subdirectory.
.PHONY: tarball rpm rpm32 deb installer
tarball rpm rpm32 deb installer:
	$(MAKE) -C scripts $@

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
	tar czf $(COVERITY_DIR).tgz $(COVERITY_DIR)

# Static code analysis: Run cppcheck on the source code tree starting at current directory.
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

# Static code analysis: Run flawfinder on the source code tree starting at current directory.
FLAWFINDER         = flawfinder
FLAWFINDER_SOURCES = src
FLAWFINDER_FLAGS   = --quiet --dataonly
.PHONY: flawfinder
flawfinder:
	$(FLAWFINDER) $(FLAWFINDER_FLAGS) $(FLAWFINDER_SOURCES)

# Static code analysis: Run scan-build on the source code tree starting at current directory.
SCANBUILD          = scan-build
SCANBUILD_SOURCES  = src
SCANBUILD_FLAGS    = -o $(BINDIR)
.PHONY: scan-build
scan-build:
	$(SCANBUILD) $(SCANBUILD_FLAGS) $(MAKE) -C $(SCANBUILD_SOURCES)

# Cleanup Windows oddities in source files.
# Many IDE's indent with tabs, and tabs are 4 chars wide.
# Tabs shall not be expanded in Makefiles.
.PHONY: unixify
unixify:
	for f in $$(find . -name \*.c -o -name \*.cpp -o -name \*.h -o -name \*.sh -o -name \*.dox -o -name \*.md -o -name \*.xml -o -name \*.txt); do \
	  expand -t 4 $$f >$$f.tmp; \
	  $(CHMOD) --reference=$$f $$f.tmp; \
	  mv -f $$f.tmp $$f; \
	done
	for f in $$(find . -name \*.c -o -name \*.cpp -o -name \*.h -o -name Makefile\* -o -name \*.sh -o -name \*.dox -o -name \*.md -o -name \*.xml -o -name \*.txt); do \
	  dos2unix -q $$f; \
	  $(SED) -i -e 's/  *$$//' $$f; \
	done

# Utilities: display predefined macros for C and C++
.PHONY: cmacros cxxmacros
cmacros:
	@$(CPP) $(CFLAGS) -x c -dM /dev/null | sort
cxxmacros:
	@$(CPP) $(CXXFLAGS) -x c++ -dM /dev/null | sort
