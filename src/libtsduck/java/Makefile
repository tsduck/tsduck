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
#  Makefile for TSDuck Java API.
#
#-----------------------------------------------------------------------------


DONT_BUILD_DEPS := TRUE
include ../../../Makefile.inc

.PHONY: install install-tools install-devel
install: install-tools install-devel
install-devel:
	@true

# Check presence of the Java compiler.
JAVAC := $(shell $(SCRIPTSDIR)/java-config.sh --javac)

# Just skip Java build if no compiler is present.
ifeq ($(JAVAC),)

default install-tools:
	@true

else

# Generate classes to make sure they are compatible with Java 8.
JAVAC_FLAGS = -source 1.8 -target 1.8 -Xlint:-options

JAR      := $(shell $(SCRIPTSDIR)/java-config.sh --jar)
CLASSDIR  = $(BINDIR)/java
JARFILE   = $(BINDIR)/tsduck.jar
MANIFEST  = $(CLASSDIR)/Manifest.txt
JSOURCES := $(wildcard src/io/tsduck/*.java)
JCLASSES := $(patsubst src/%.java,$(CLASSDIR)/%.class,$(JSOURCES))

$(CLASSDIR)/%.class: src/%.java
	@echo '  [JAVAC] $<'; \
	mkdir -p $(CLASSDIR); \
	cd src; $(JAVAC) $(JAVAC_FLAGS) -d $(CLASSDIR) $(patsubst src/%,%,$<)

default: $(JARFILE)
	@true

$(JARFILE): $(JCLASSES) $(MANIFEST)
	@echo '  [JAR] $@'; \
	$(JAR) cfm $@ $(MANIFEST) $(patsubst $(CLASSDIR)/%,-C $(CLASSDIR) %,$(JCLASSES))

$(MANIFEST): Manifest.txt ../tsVersion.h
	@echo '  [GEN] $@'; \
	mkdir -p $(dir $@); \
	$(SED) -e 's|{{VERSION}}|$(shell $(GET_VERSION))|' Manifest.txt >$@

install-tools: $(JARFILE)
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck/java
	install -m 644 $(JARFILE) $(SYSROOT)$(SYSPREFIX)/share/tsduck/java

endif
