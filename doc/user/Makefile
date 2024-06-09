#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for the TSDuck user's guide.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

DOCFILE = tsduck

ADOCFLAGS_HTML += -a data-uri -a docinfo=shared -a docinfodir="$(BINDOCINFO)"

# All input files. Not really all .png images, but the exact list may vary.

INPUTS = $(wildcard *.adoc */*.adoc */*/*.adoc $(IMAGESDIR)/*.png) .all.commands.adoc .all.plugins.adoc

# Default target: HTML and PDF documentation.

default: html pdf
html: $(BINDOC)/$(DOCFILE).html
pdf: $(BINDOC)/$(DOCFILE).pdf

# Generate and open the doc (assuming the existence of the "open" command).

open: open-html open-pdf
open-%: %
	open $(BINDOC)/$(DOCFILE).$<

# Documentation generation.

.all.commands.adoc: $(wildcard commands/*.adoc)
	@echo "[GEN] $@"
	@echo "// Automatically generated file, do not edit" >$@
	@for f in $(sort $^); do echo "include::{docdir}/$$f[]" >>$@; done

.all.plugins.adoc: $(wildcard plugins/*.adoc)
	@echo "[GEN] $@"
	@echo "// Automatically generated file, do not edit" >$@
	@for f in $(sort $^); do echo "include::{docdir}/$$f[]" >>$@; done

$(BINDOC)/$(DOCFILE).html: $(INPUTS) $(CSSFILE) $(BINDOCINFO)/docinfo.html $(BINDOCINFO)/docinfo-footer.html
	mkdir -p $(dir $@)
	$(ASCIIDOCTOR) $(ADOCFLAGS_HTML) $(DOCFILE).adoc -D $(dir $@) -o $(notdir $@)
	$(if $(wildcard $(SHAREDIR)),cp $@ $(SHAREDIR))

$(BINDOC)/$(DOCFILE).pdf: $(INPUTS) $(THEMEFILE)
	mkdir -p $(dir $@)
	$(ASCIIDOCTOR_PDF) $(ADOCFLAGS_PDF) $(DOCFILE).adoc -D $(dir $@) -o $(notdir $@)
	$(if $(wildcard $(SHAREDIR)),cp $@ $(SHAREDIR))

$(BINDOCINFO)/docinfo.html: $(TOCBOTDIR)/docinfo.html
	mkdir -p $(dir $@)
	cp $< $@

$(BINDOCINFO)/docinfo-footer.html: $(TOCBOTDIR)/docinfo-footer.in.html $(TOCBOTDIR)/tocbot.min.js
	mkdir -p $(dir $@)
	echo "<script>" >$@
	cat $(TOCBOTDIR)/tocbot.min.js >>$@
	echo "" >>$@
	echo "</script>" >>$@
	cat $(TOCBOTDIR)/docinfo-footer.in.html >>$@
