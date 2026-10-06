#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile to rebuild draw.io diagrams.
#
#  PNG files are created here and stored in the repository because draw.io
#  may not be available everywhere. PDF files are created in the bin area.
#
#-----------------------------------------------------------------------------

include ../Makefile.inc

BINDOC    = $(BINROOT)/doc
DIAGRAMS  = $(wildcard *.drawio)
DRAWIO   ?= drawio
OPTIPNG  ?= optipng
PNGFLAGS ?= -t
PDFFLAGS ?=
OPTFLAGS ?= -o5 -silent

.PHONY: png pdf

default: png

png: $(patsubst %.drawio,%.png,$(DIAGRAMS))
	@true
pdf: $(patsubst %.drawio,$(BINDOC)/%.pdf,$(DIAGRAMS))
	@true

%.png: %.drawio
	$(call LOG,[DRAWIO] $@) $(DRAWIO) -x $(PNGFLAGS) $< -o $@ && $(OPTIPNG) $(OPTFLAGS) $@
$(BINDOC)/%.pdf: %.drawio
	$(call LOG,[DRAWIO] $@) mkdir -p $(dir $@); $(DRAWIO) -x $(PDFFLAGS) $< -o $@
