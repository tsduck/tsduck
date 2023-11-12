#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for software packaging on Linux.
#
#-----------------------------------------------------------------------------

include ../Makefile.inc

INSTALLER_TYPE := $(if $(wildcard /etc/*fedora* /etc/*redhat*),rpm,$(if $(wildcard /etc/*debian*),deb))
$(if $(INSTALLER_TYPE),,$(error unknown installer type on this operating system))

.PHONY: default installer install-installer
default installer install-installer:
	@$(MAKE) -C $(INSTALLER_TYPE) $@
