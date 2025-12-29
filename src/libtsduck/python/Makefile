#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for TSDuck Python API.
#
#-----------------------------------------------------------------------------


DONT_BUILD_DEPS := true
include ../../../Makefile.inc

default:
	@true

.PHONY: install install-tools install-devel
install: install-tools install-devel

install-tools:
	rm -rf $(SYSROOT)$(SYSPREFIX)/share/tsduck/python
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck/python
	install -m 644 tsduck.py ts.py $(SYSROOT)$(SYSPREFIX)/share/tsduck/python
install-devel:
	@true
