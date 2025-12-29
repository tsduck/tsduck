#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
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

JAR      := $(shell $(SCRIPTSDIR)/java-config.sh --jar)
CLASSDIR  = $(BINDIR)/java
JARFILE   = $(BINDIR)/tsduck.jar
MANIFEST  = $(CLASSDIR)/Manifest.txt
JSOURCES := $(wildcard src/io/tsduck/*.java)
JCLASSES := $(patsubst src/%.java,$(CLASSDIR)/%.class,$(JSOURCES))

$(CLASSDIR)/%.class: src/%.java
	$(call LOG,[JAVAC] $<) mkdir -p $(CLASSDIR); cd src; $(JAVAC) $(JAVAC_FLAGS) -d $(CLASSDIR) $(patsubst src/%,%,$<)

default: $(JARFILE)
	@true

$(JARFILE): $(JCLASSES) $(MANIFEST)
	$(call LOG,[JAR] $@) $(JAR) cfm $@ $(MANIFEST) $(patsubst $(CLASSDIR)/%,-C $(CLASSDIR) %,$(JCLASSES))

$(MANIFEST): Manifest.txt $(LIBTSCOREDIR)/tsVersion.h
	$(call LOG,[GEN] $@) mkdir -p $(dir $@); $(SED) -e 's|{{VERSION}}|$(shell $(GET_TSDUCK_VERSION))|' Manifest.txt >$@

install-tools: $(JARFILE)
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck/java
	install -m 644 $(JARFILE) $(SYSROOT)$(SYSPREFIX)/share/tsduck/java
