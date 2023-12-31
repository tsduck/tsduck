#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
