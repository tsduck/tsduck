# Makefile for TSDuck sample extension.

# Recursive invocations of make should be silent.
MAKEFLAGS += --no-print-directory

# Default target is build the binaries:
default install:
	@$(MAKE) -C src $@

# Installers:
deb rpm: default
	scripts/build-$@.sh

# Cleanup non-source files:
clean distclean:
	@$(MAKE) -C src $@
	@$(MAKE) -C msvc $@
	@rm -rf installers/tmp demo/*.ts
