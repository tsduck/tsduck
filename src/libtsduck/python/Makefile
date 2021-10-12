# Dummy makefile for subdirectory in vpath. Actual make is under control of parent directory.

DONT_BUILD_DEPS := true
include ../../../Makefile.tsduck

default:
	@true

.PHONY: install
install:
	rm -rf $(SYSROOT)$(SYSPREFIX)/share/tsduck/python
	install -d -m 755 $(SYSROOT)$(SYSPREFIX)/share/tsduck/python
	install -m 644 tsduck.py ts.py $(SYSROOT)$(SYSPREFIX)/share/tsduck/python
