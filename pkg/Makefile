#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for software packaging on Linux. The make command is redirected
#  to the appropriate subdirectory for the current operating system.
#
#-----------------------------------------------------------------------------

include ../Makefile.inc

SUBDIR := $(if $(wildcard /etc/*fedora* /etc/*redhat*),rpm,$(if $(wildcard /etc/*debian*),deb))
$(if $(SUBDIR),,$(error unknown installer type on this operating system))

.PHONY: default installer install-installer installer-tarball
default installer install-installer installer-tarball:
	@$(MAKE) -C $(SUBDIR) $@
