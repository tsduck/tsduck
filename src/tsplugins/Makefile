#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2020, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
#  Makefile for tsplugins.
#
#-----------------------------------------------------------------------------

# With static link, we compile in a specific directory.
BINDIR_SUFFIX := $(if $(STATIC),-static,)
OBJSUBDIR := objs-tsplugins

include ../../Makefile.tsduck

.PHONY: shlibs install install-devel

ifndef STATIC

# Dynamic link (the default), we build shared objects.
SHLIBS := $(addprefix $(BINDIR)/,$(addsuffix .so,$(TSPLUGINS)))

default: shlibs
	@true
shlibs: $(SHLIBS)
$(SHLIBS): $(SHARED_LIBTSDUCK)

install: $(SHLIBS)
	install -d -m 755 $(SYSROOT)$(USRLIBDIR)/tsduck
	install -m 644 $(SHLIBS) $(SYSROOT)$(USRLIBDIR)/tsduck
install-devel:
	@true

else

# With static link, we compile only (in a specific directory), we do not build shared objects.
default: $(addprefix $(OBJDIR)/,$(addsuffix .o,$(TSPLUGINS)))
	@true
install:
	@true
install-devel:
	@true

endif
