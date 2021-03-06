# Dummy makefile for subdirectory in vpath. Actual make is under control of parent directory.

DONT_BUILD_DEPS := true
include ../../../Makefile.tsduck

EXTRA_DISTCLEAN += __pycache__

default:
	@true

.PHONY: install
install:
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck/python
	install -m 644 ts.py $(SYSROOT)$(SYSPREFIX)/share/tsduck/python
