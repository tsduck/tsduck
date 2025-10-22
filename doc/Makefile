#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for the documentation subdirectory.
#
#-----------------------------------------------------------------------------

include ../Makefile.inc

.PHONY: docs doxygen
default: docs doxygen
docs: userguide devguide
docs-html: userguide-html devguide-html
docs-pdf: userguide-pdf devguide-pdf
doxygen:
	$(PYTHON) doxy/build-doxygen.py

# When opening files using Firefox on Linux, there are a log to anoying error messages.

POST_XDGOPEN = $(if $(LINUX),2>/dev/null)

# Input directories.

IMAGESDIR  = $(ROOTDIR)/images
DOCROOT    = $(ROOTDIR)/doc
ADOCDIR    = $(DOCROOT)/adoc
USERDOCDIR = $(DOCROOT)/user
DEVDOCDIR  = $(DOCROOT)/developer
USERSIXML  = $(USERDOCDIR)/20D-app-si-xml.adoc

# Output directories.

BINDOC     = $(BINROOT)/doc
BINDOCINFO = $(BINROOT)/docinfo

# Common themes.

CSSFILE    = $(ADOCDIR)/tsduck.css
THEME      = tsduck
THEMEFILE  = $(ADOCDIR)/$(THEME)-theme.yml
ROUGE_HTML = thankful_eyes
ROUGE_PDF  = github

# Documentation tools.

ASCIIDOCTOR     ?= asciidoctor
ASCIIDOCTOR_PDF ?= asciidoctor-pdf

# Documentation generation flags.

ADOCFLAGS      = -v -r "$(ADOCDIR)/macros.rb" -a imagesdir="$(IMAGESDIR)" -a includedir="$(ADOCDIR)" \
                 -a revnumber="$(shell $(GET_TSDUCK_VERSION))" -a revdate="$(shell $(DATE) '+%B %Y')"
ADOCFLAGS_HTML = $(ADOCFLAGS) -a stylesheet="$(CSSFILE)" -a rouge-style="$(ROUGE_HTML)" \
                 -a data-uri -a docinfo=shared -a docinfodir="$(BINDOCINFO)"
ADOCFLAGS_PDF  = $(ADOCFLAGS) -a pdf-themesdir="$(ADOCDIR)" -a pdf-theme="$(THEME)" -a rouge-style="$(ROUGE_PDF)"

# Common input files for asciidoctor document.
# Not really all .png images are used, but the exact list may vary while the documents are edited.
# So, better rebuild if an image was added or updated.

COMMON_INPUTS      = $(ADOCDIR)/attributes.adoc $(ADOCDIR)/macros.rb $(wildcard $(IMAGESDIR)/*.png) $(LIBTSCOREDIR)/tsVersion.h
COMMON_INPUTS_HTML = $(COMMON_INPUTS) $(CSSFILE) $(BINDOCINFO)/docinfo.html $(BINDOCINFO)/docinfo-footer.html
COMMON_INPUTS_PDF  = $(COMMON_INPUTS) $(THEMEFILE)

# Asciidoctor creates huge PDF. They need to be compressed afterward.
# The size is typically divided by 4.

$(BINDOC)/%.pdf: $(BINDOC)/%.bigpdf
	$(call LOG,[COMPRESS] $@) qpdf --compress-streams=y --recompress-flate --optimize-images $< $@

#-----------------------------------------------------------------------------
# Docinfo files, for asciidoctor.
# This setup creates an expandable table of content inside the left side bar.
#-----------------------------------------------------------------------------

$(BINDOCINFO)/docinfo.html: $(ADOCDIR)/docinfo.in.html $(IMAGESDIR)/tsduck-logo.svg
	$(call LOG,[GEN] $@) \
	mkdir -p $(dir $@); \
	cp $(ADOCDIR)/docinfo.in.html $@; \
	echo "<style>" >>$@; \
	echo -n "img.tsduck-logo {content: url(data:image/svg+xml;base64," >>$@; \
	$(BASE64) $(IMAGESDIR)/tsduck-logo.svg | tr -d '\n' >>$@; \
	echo ");}" >>$@; \
	echo "</style>" >>$@

$(BINDOCINFO)/docinfo-footer.html: $(ADOCDIR)/docinfo-footer.in.html $(ADOCDIR)/tocbot.min.js
	$(call LOG,[GEN] $@) \
	mkdir -p $(dir $@); \
	echo "<script>" >$@; \
	cat $(ADOCDIR)/tocbot.min.js >>$@; \
	echo "" >>$@; \
	echo "</script>" >>$@; \
	cat $(ADOCDIR)/docinfo-footer.in.html >>$@

#-----------------------------------------------------------------------------
# User guide
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
	$(call LOG,[OPEN] $<) \
	$(XDGOPEN) $< $(POST_XDGOPEN)
open-userguide-pdf: $(BINDOC)/tsduck.pdf
	$(call LOG,[OPEN] $<) \
	$(XDGOPEN) $< $(POST_XDGOPEN)

# Each table and descriptor has a .adoc file in the source tree.
# Create one .adoc file per type (table or descriptor) and standard,
# e.g. .all.dvb.tables.adoc or .all.isdb.descriptors.adoc.

TABLES_STANDARDS = $(sort $(notdir $(patsubst %/,%,$(dir $(wildcard $(LIBTSDUCKDIR)/dtv/tables/*/*.adoc)))))
TABLES_ADOC = $(patsubst %,$(USERDOCDIR)/.all.%.tables.adoc,$(TABLES_STANDARDS))
DESCRIPTORS_STANDARDS = $(sort $(notdir $(patsubst %/,%,$(dir $(wildcard $(LIBTSDUCKDIR)/dtv/descriptors/*/*.adoc)))))
DESCRIPTORS_ADOC = $(patsubst %,$(USERDOCDIR)/.all.%.descriptors.adoc,$(DESCRIPTORS_STANDARDS))

define DOCRULE =
$$(USERDOCDIR)/.all.$(1).$(2).adoc: $$(wildcard $$(LIBTSDUCKDIR)/dtv/$(2)/$(1)/*.adoc)
	$$(call LOG,[GEN] $$@) \
	if ! $$(GREP) -q '^include::$$(notdir $$@)\[]' $$(USERSIXML); then \
	    echo >&2 'error: $$(notdir $$@) not included in $$(USERSIXML)'; \
	    exit 1; \
	fi; \
	echo "// Automatically generated file, do not edit" >$$@; \
	for f in $$$$(grep -H "^====" $$^ | sed -e "s|\(.*\):==== *\(.*\)|\2:\1|" | sort -d -f | sed -e "s|.*:||"); do \
	    echo; \
	    cat $$$$f; \
	done >>$$@; \
	echo >>$$@
endef

$(foreach std,$(TABLES_STANDARDS),$(eval $(call DOCRULE,$(std),tables)))
$(foreach std,$(DESCRIPTORS_STANDARDS),$(eval $(call DOCRULE,$(std),descriptors)))

# Documentation generation.

USERGUIDE_INPUTS = $(wildcard $(USERDOCDIR)/*.adoc $(USERDOCDIR)/*/*.adoc $(USERDOCDIR)/*/*/*.adoc) \
    $(USERDOCDIR)/.all.commands.adoc $(USERDOCDIR)/.all.plugins.adoc $(TABLES_ADOC) $(DESCRIPTORS_ADOC)

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

$(BINDOC)/tsduck.bigpdf: $(USERGUIDE_INPUTS) $(COMMON_INPUTS_PDF)
	$(call LOG,[ASCIIDOC] $@) \
	mkdir -p $(dir $@); \
	$(ASCIIDOCTOR_PDF) $(ADOCFLAGS_PDF) $(USERDOCDIR)/tsduck.adoc -D $(dir $@) -o $(notdir $@)

#-----------------------------------------------------------------------------
# Developer guide
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
	$(call LOG,[OPEN] $<) \
	$(XDGOPEN) $< $(POST_XDGOPEN)
open-devguide-pdf: $(BINDOC)/tsduck-dev.pdf
	$(call LOG,[OPEN] $<) \
	$(XDGOPEN) $< $(POST_XDGOPEN)

# Documentation generation.

DEVGUIDE_INPUTS = $(wildcard $(DEVDOCDIR)/*.adoc) \
    $(DEVDOCDIR)/.all.tables.adoc $(DEVDOCDIR)/.all.descriptors.adoc

$(DEVDOCDIR)/.all.tables.adoc: $(wildcard $(LIBTSDUCKDIR)/dtv/tables/*.cpp $(LIBTSDUCKDIR)/dtv/tables/*/*.cpp)
	$(call LOG,[GEN] $@) \
	$(PYTHON) $(DEVDOCDIR)/build-sigref.py tables "$@"

$(DEVDOCDIR)/.all.descriptors.adoc: $(wildcard $(LIBTSDUCKDIR)/dtv/descriptors/*.cpp $(LIBTSDUCKDIR)/dtv/descriptors/*/*.cpp)
	$(call LOG,[GEN] $@) \
	$(PYTHON) $(DEVDOCDIR)/build-sigref.py descriptors "$@"

$(BINDOC)/tsduck-dev.html: $(DEVGUIDE_INPUTS) $(COMMON_INPUTS_HTML)
	$(call LOG,[ASCIIDOC] $@) \
	mkdir -p $(dir $@); \
	$(ASCIIDOCTOR) $(ADOCFLAGS_HTML) $(DEVDOCDIR)/tsduck-dev.adoc -D $(dir $@) -o $(notdir $@)

$(BINDOC)/tsduck-dev.bigpdf: $(DEVGUIDE_INPUTS) $(COMMON_INPUTS_PDF)
	$(call LOG,[ASCIIDOC] $@) \
	mkdir -p $(dir $@); \
	$(ASCIIDOCTOR_PDF) $(ADOCFLAGS_PDF) $(DEVDOCDIR)/tsduck-dev.adoc -D $(dir $@) -o $(notdir $@)

#-----------------------------------------------------------------------------
# Installation steps
#-----------------------------------------------------------------------------

INSTALLDOC = $(SYSROOT)$(SYSPREFIX)/share/doc/tsduck

.PHONY: install install-tools install-devel
install: install-tools install-devel

install-tools: $(if $(NODOC),,$(BINDOC)/tsduck.html)
	install -d -m 755 $(INSTALLDOC)
	install -m 644 $(ROOTDIR)/CHANGELOG.txt $(ROOTDIR)/LICENSE.txt $(ROOTDIR)/OTHERS.txt $(INSTALLDOC)
	$(if $(NODOC),,install -m 644 $(BINDOC)/tsduck.html $(INSTALLDOC))

install-devel: $(if $(NODOC),,$(BINDOC)/tsduck-dev.html)
	install -d -m 755 $(INSTALLDOC)
	$(if $(NODOC),,install -m 644 $(BINDOC)/tsduck-dev.html $(INSTALLDOC))
