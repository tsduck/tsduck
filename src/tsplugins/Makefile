#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for tsplugins.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

.PHONY: shlibs install install-tools

ifeq ($(STATIC),)

# Dynamic link (the default), we build shared objects.
SHLIBS := $(addprefix $(BINDIR)/,$(addsuffix $(SO_SUFFIX),$(TSPLUGINS)))

default: shlibs
	@true
shlibs: $(SHLIBS)
$(SHLIBS): $(SHARED_LIBTSDUCK)

install install-tools: $(SHLIBS)
	install -d -m 755 $(SYSROOT)$(USRLIBDIR)/tsduck
	$(if $(NO_TSPLUGINS),rm -rf $(addsuffix $(SO_SUFFIX),$(addprefix $(SYSROOT)$(USRLIBDIR)/tsduck/,$(NO_TSPLUGINS)),))
	$(if $(MACOS),rm -rf $(SYSROOT)$(USRLIBDIR)/tsduck/*.so,)
	install -m 644 $(SHLIBS) $(SYSROOT)$(USRLIBDIR)/tsduck

else

# With static link, we compile only (in a specific directory), we do not build shared objects.
default: $(addprefix $(OBJDIR)/,$(addsuffix .o,$(TSPLUGINS)))
	@true
install install-tools:
	@true

endif
