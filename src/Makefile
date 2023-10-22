#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for source root directory.
#
#-----------------------------------------------------------------------------

include ../Makefile.inc

default:
	+@$(call F_RECURSE,libtsduck tsplugins tstools $(if $(NOTEST),,utest) $(if $(CROSS),,utils))
	@$(MAKE) -C libtsduck/config post-build

.PHONY: install install-tools install-devel
install: install-tools install-devel
install-tools:
	@$(call F_RECURSE,libtsduck tsplugins tstools utils)
	@$(MAKE) -C libtsduck/config install-post-build
install-devel:
	@$(call F_RECURSE,libtsduck)
