#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for source root directory.
#
#-----------------------------------------------------------------------------

include ../Makefile.inc

default:
	+@$(call RECURSE,libtscore libtsduck $(if $(NODTAPI),,libtsdektec) tsplugins tstools $(if $(NOTEST),,utest) $(if $(CROSS),,utils))

.PHONY: install install-tools install-devel
install: install-tools install-devel
install-tools:
	@$(call RECURSE,libtscore libtsduck $(if $(NODTAPI),,libtsdektec) tsplugins tstools utils)
install-devel:
	@$(call RECURSE,libtscore libtsduck $(if $(NODTAPI),,libtsdektec))
