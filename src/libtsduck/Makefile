#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2017, Thierry Lelegard
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
#  Makefile for libtsduck.
#
#-----------------------------------------------------------------------------


# Some source files are located in subdirectories but should be considered as part of libtsduck.

OSDIR    := $(if $(findstring darwin,$(shell uname -s | tr A-Z a-z)),mac,linux)
CXXVPATH  = $(OSDIR) private

# Detect the presence of the Dektec DTAPI.
# DTAPI shall be separately downloaded using "make dtapi".
# Define NODTAPI to compile without Dektec support.
# NODTAPI is automatically defined if no DTAPI is found.
# DTAPI is not available for MacOS and is not downloaded, not present.

DTAPI_ROOT := ../../dektec/LinuxSDK/DTAPI

ifeq ($(NODTAPI)$(DTAPI_OBJECT),)
  # No DTAPI was preset by user. Try to find one.
  ifeq ($(shell uname -m)$(M32),x86_64)
    DTAPI_OBJECT_NAME := DTAPI64.o
  else
    DTAPI_OBJECT_NAME := DTAPI.o
  endif
  DTAPI_OBJECT := $(lastword $(wildcard $(DTAPI_ROOT)/Lib/*/$(DTAPI_OBJECT_NAME)))
  DTAPI_HEADER := $(wildcard $(DTAPI_ROOT)/Include/DTAPI.h)
endif

ifeq ($(DTAPI_OBJECT)$(DTAPI_HEADER),)
  NODTAPI := true
  CXXFLAGS_INCLUDES += -DTS_NO_DTAPI=1
else
  CXXFLAGS_INCLUDES += -isystem $(DTAPI_ROOT)/Include
endif

# Now, we can include the common makefile.

include ../../Makefile.tsduck

# Building the TSDuck library.

default: libs tsduck.h
	@true

# Specific (per-module) compilation options:

$(OBJDIR)/tsAES.o:        CFLAGS_OPTIMIZE = $(CFLAGS_FULLSPEED)
$(OBJDIR)/tsScrambling.o: CFLAGS_OPTIMIZE = $(CFLAGS_FULLSPEED)
$(OBJDIR)/tsDES.o:        CFLAGS_OPTIMIZE = $(CFLAGS_FULLSPEED)
$(OBJDIR)/tsTDES.o:       CFLAGS_OPTIMIZE = $(CFLAGS_FULLSPEED)
$(OBJDIR)/tsSHA1.o:       CFLAGS_OPTIMIZE = $(CFLAGS_FULLSPEED)
$(OBJDIR)/tsSHA256.o:     CFLAGS_OPTIMIZE = $(CFLAGS_FULLSPEED)
$(OBJDIR)/tsSHA512.o:     CFLAGS_OPTIMIZE = $(CFLAGS_FULLSPEED)
$(OBJDIR)/tsMD5.o:        CFLAGS_OPTIMIZE = $(CFLAGS_FULLSPEED)

# Dektec code is encapsulated into the TSDuck library.

CFLAGS_INCLUDES += -I$(LIBTSDUCKDIR)/private
OBJS += $(DTAPI_OBJECT)

# Library containing all modules.
# Both static and dynamic libraries are created but only
# use the dynamic one when building tools and plugins.

.PHONY: libs
libs: $(LIBTSDUCK) $(SHLIBTSDUCK)
$(LIBTSDUCK): $(LIBTSDUCK)($(OBJS))
$(SHLIBTSDUCK): $(OBJS)
	@echo '  [DTAPI] $(if $(DTAPI_OBJECT),using $(DTAPI_OBJECT),no DTAPI available)'; \
	echo '  [CC] $@'; \
	$(CC) $(CFLAGS) $^ $(LDLIBS) -shared -o $@

# The header tsduck.h includes all headers

tsduck.h: $(ROOTDIR)/HEADER.txt $(sort $(filter-out tsduck.h %Template.h,$(wildcard *.h)))
	@echo '  [REBUILD] $@'; \
	cp $< $@; \
	echo -e '\n#pragma once\n' >>$@; \
	$(SED) <<<'$(filter-out $<,$^)' >>$@ -e 's/ *\([^\. ]*\).h/#include "\1.h"\n/g'

# Installing the shared library in same directory as executables

.PHONY: install
install: $(LIBTSDUCK) $(SHLIBTSDUCK) tsduck.h
	install -d -m 755 $(DESTDIR)/usr/bin
	install -m 755 $(SHLIBTSDUCK) $(DESTDIR)/usr/bin
	install -d -m 755 $(DESTDIR)/usr/lib
	install -m 644 $(LIBTSDUCK) $(DESTDIR)/usr/lib
	install -d -m 755 $(DESTDIR)/usr/include/tsduck
	install -m 644 tsduck.mk ts*.h $(wildcard $(OSDIR)/ts*.h) $(DESTDIR)/usr/include/tsduck
