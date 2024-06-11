#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for the documentation subdirectory.
#
#-----------------------------------------------------------------------------

include ../Makefile.inc

.PHONY: docs doxygen
default: docs doxygen
docs: userguide devguide
doxygen:
	./build-doxygen.sh $(DOXYFLAGS)

# Input directories.

IMAGESDIR  = $(ROOTDIR)/images
DOCROOT    = $(ROOTDIR)/doc
CSSDIR     = $(DOCROOT)/css
TOCBOTDIR  = $(DOCROOT)/tocbot
THEMESDIR  = $(DOCROOT)/themes
USERDOCDIR = $(DOCROOT)/user
DEVDOCDIR  = $(DOCROOT)/developer

# Output directories.

BINDOC     = $(BINROOT)/doc
BINDOCINFO = $(BINROOT)/docinfo

# Common themes.

CSSFILE    = $(CSSDIR)/tsduck.css
THEME      = tsduck
THEMEFILE  = $(THEMESDIR)/$(THEME)-theme.yml
ROUGE_HTML = thankful_eyes
ROUGE_PDF  = github

# Documentation tools.

ASCIIDOCTOR     = asciidoctor
ASCIIDOCTOR_PDF = asciidoctor-pdf

# Documentation generation flags.

ADOCFLAGS      = -v -a revnumber="$(shell $(GET_VERSION))" -a imagesdir="$(IMAGESDIR)" -a revdate="$(shell $(DATE) '+%B %Y')"
ADOCFLAGS_HTML = $(ADOCFLAGS) -a stylesheet="$(CSSFILE)" -a rouge-style="$(ROUGE_HTML)" \
                 -a data-uri -a docinfo=shared -a docinfodir="$(BINDOCINFO)"
ADOCFLAGS_PDF  = $(ADOCFLAGS) -a pdf-themesdir="$(THEMESDIR)" -a pdf-theme="$(THEME)" -a rouge-style="$(ROUGE_PDF)"

# Common input files for asciidoctor document.
# Not really all .png images are used, but the exact list may vary while the documents are edited.
# So, better rebuild if an image was added or updated.

COMMON_INPUTS      = attributes.adoc $(wildcard $(IMAGESDIR)/*.png)
COMMON_INPUTS_HTML = $(COMMON_INPUTS) $(CSSFILE) $(BINDOCINFO)/docinfo.html $(BINDOCINFO)/docinfo-footer.html
COMMON_INPUTS_PDF  = $(COMMON_INPUTS) $(THEMEFILE)

#-----------------------------------------------------------------------------
# Docinfo files, for asciidoctor.
# This setup creates an expandable table of content inside the left side bar.
#-----------------------------------------------------------------------------

$(BINDOCINFO)/docinfo.html: $(TOCBOTDIR)/docinfo.html
	$(call LOG,[GEN] $@) \
	mkdir -p $(dir $@); \
	cp $< $@

$(BINDOCINFO)/docinfo-footer.html: $(TOCBOTDIR)/docinfo-footer.in.html $(TOCBOTDIR)/tocbot.min.js
	$(call LOG,[GEN] $@) \
	mkdir -p $(dir $@); \
	echo "<script>" >$@; \
	cat $(TOCBOTDIR)/tocbot.min.js >>$@; \
	echo "" >>$@; \
	echo "</script>" >>$@; \
	cat $(TOCBOTDIR)/docinfo-footer.in.html >>$@

#-----------------------------------------------------------------------------
# User's guide
#-----------------------------------------------------------------------------

.PHONY: userguide userguide-html userguide-pdf open-userguide open-userguide-html open-userguide-pdf
userguide: userguide-html userguide-pdf
userguide-html: $(BINDOC)/tsduck.html
	@true
userguide-pdf: $(BINDOC)/tsduck.pdf
	@true

# Generate and open documentation (assume the existence of an "open" command);

open-userguide: open-userguide-html open-userguide-pdf
open-userguide-html: $(BINDOC)/tsduck.html
	open $<
open-userguide-pdf: $(BINDOC)/tsduck.pdf
	open $<

# Documentation generation.

USERGUIDE_INPUTS = $(wildcard $(USERDOCDIR)/*.adoc $(USERDOCDIR)/*/*.adoc $(USERDOCDIR)/*/*/*.adoc) \
    $(USERDOCDIR)/.all.commands.adoc $(USERDOCDIR)/.all.plugins.adoc

$(USERDOCDIR)/.all.commands.adoc: $(wildcard $(USERDOCDIR)/commands/*.adoc)
	$(call LOG,[GEN] $@) \
	echo "// Automatically generated file, do not edit" >$@; \
	for f in $(sort $(patsubst $(USERDOCDIR)/%,%,$^)); do echo "include::{docdir}/$$f[]" >>$@; done

$(USERDOCDIR)/.all.plugins.adoc: $(wildcard $(USERDOCDIR)/plugins/*.adoc)
	$(call LOG,[GEN] $@) \
	echo "// Automatically generated file, do not edit" >$@; \
	for f in $(sort $(patsubst $(USERDOCDIR)/%,%,$^)); do echo "include::{docdir}/$$f[]" >>$@; done

$(BINDOC)/tsduck.html: $(USERGUIDE_INPUTS) $(COMMON_INPUTS_HTML)
	$(call LOG,[ASCIIDOC] $@) \
	mkdir -p $(dir $@); \
	$(ASCIIDOCTOR) $(ADOCFLAGS_HTML) $(USERDOCDIR)/tsduck.adoc -D $(dir $@) -o $(notdir $@)

$(BINDOC)/tsduck.pdf: $(USERGUIDE_INPUTS) $(COMMON_INPUTS_PDF)
	$(call LOG,[ASCIIDOC] $@) \
	mkdir -p $(dir $@); \
	$(ASCIIDOCTOR_PDF) $(ADOCFLAGS_PDF) $(USERDOCDIR)/tsduck.adoc -D $(dir $@) -o $(notdir $@)

#-----------------------------------------------------------------------------
# Developer's guide
#-----------------------------------------------------------------------------

.PHONY: devguide devguide-html devguide-pdf open-devguide open-devguide-html open-devguide-pdf
devguide: devguide-html devguide-pdf
devguide-html: $(BINDOC)/tsduck-dev.html
	@true
devguide-pdf: $(BINDOC)/tsduck-dev.pdf
	@true

# Generate and open documentation (assume the existence of an "open" command);

open-devguide: open-devguide-html open-devguide-pdf
open-devguide-html: $(BINDOC)/tsduck-dev.html
	open $<
open-devguide-pdf: $(BINDOC)/tsduck-dev.pdf
	open $<

# Documentation generation.

DEVGUIDE_INPUTS = $(wildcard $(DEVDOCDIR)/*.adoc)

$(BINDOC)/tsduck-dev.html: $(DEVGUIDE_INPUTS) $(COMMON_INPUTS_HTML)
	$(call LOG,[ASCIIDOC] $@) \
	mkdir -p $(dir $@); \
	$(ASCIIDOCTOR) $(ADOCFLAGS_HTML) $(DEVDOCDIR)/tsduck-dev.adoc -D $(dir $@) -o $(notdir $@)

$(BINDOC)/tsduck-dev.pdf: $(DEVGUIDE_INPUTS) $(COMMON_INPUTS_PDF)
	$(call LOG,[ASCIIDOC] $@) \
	mkdir -p $(dir $@); \
	$(ASCIIDOCTOR_PDF) $(ADOCFLAGS_PDF) $(DEVDOCDIR)/tsduck-dev.adoc -D $(dir $@) -o $(notdir $@)

#-----------------------------------------------------------------------------
# Installation steps
#-----------------------------------------------------------------------------

.PHONY: install install-tools install-devel
install: install-tools install-devel

install-tools: userguide
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/doc/tsduck
	install -m 644 \
	    $(ROOTDIR)/CHANGELOG.txt $(ROOTDIR)/LICENSE.txt $(ROOTDIR)/OTHERS.txt \
	    $(BINDOC)/tsduck.html $(BINDOC)/tsduck.pdf \
	    $(SYSROOT)$(SYSPREFIX)/share/doc/tsduck

install-devel: devguide
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/doc/tsduck
	install -m 644 $(BINDOC)/tsduck-dev.html $(BINDOC)/tsduck-dev.pdf $(SYSROOT)$(SYSPREFIX)/share/doc/tsduck
