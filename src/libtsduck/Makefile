#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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

OBJSUBDIR = objs-libtsduck

# Some source files are located in subdirectories but should be considered as part of libtsduck.

SKIPDIR := $(if $(findstring Darwin,$(shell uname -s)),linux,mac)
VPATH = $(patsubst ./%,%,$(shell find . -type d ! -name . ! -name windows ! -name $(SKIPDIR)))

# Detect the presence of the Dektec DTAPI.
# Define NODTAPI to compile without Dektec support.
# NODTAPI is automatically defined if no DTAPI is found.

ifeq ($(NODTAPI),)
    ifeq ($(filter-out default headers libs,$(MAKECMDGOALS)),)
        DTAPI_OBJECT := $(shell ../../scripts/dtapi-config.sh --object --download $(if $(M32),--m32,))
        DTAPI_HEADER := $(shell ../../scripts/dtapi-config.sh --header)
    endif
    ifeq ($(DTAPI_OBJECT)$(DTAPI_HEADER),)
        NODTAPI := true
        CXXFLAGS_INCLUDES += -DTS_NO_DTAPI=1
    else
        CXXFLAGS_INCLUDES += -isystem $(dir $(DTAPI_HEADER))
    endif
endif

# JNI headers (or disable JNI).

JNI_CXXFLAGS := $(shell ../../scripts/java-config.sh --cflags)
CXXFLAGS_INCLUDES += $(JNI_CXXFLAGS)

# Now, we can include the common makefile.

include ../../Makefile.tsduck

# There are things to build or install in those subdirectories.
# All other subdirectories only contain C++ source files (used in VPATH).

RECURSE_ORDER = config python java

# Building the TSDuck library.

default: headers libs
	+@$(RECURSE)

# The tsduck.h header is automatically generated from SRC_HEADERS using BUILD_PROJ_FILES.

SRC_HEADERS = $(filter-out tsduck.h,$(wildcard *.h */*.h */*/*.h))
BUILD_TSDUCK_H = $(ROOTDIR)/scripts/build-tsduck-header.sh

.PHONY: headers
headers: tsduck.h
tsduck.h: $(BUILD_TSDUCK_H) $(SRCROOT)/HEADER.txt $(SRC_HEADERS)
	@echo '  [REBUILD] $@'; \
	$(BUILD_TSDUCK_H)

# This header file is regenerated and, normally, make should know that they must be
# finished before building tsVersionInfo.o. However, the .dep file contains full paths
# of dependencies and make cannot realize it is the same header files.

$(OBJDIR)/tsduck.o: tsduck.h

# Specific (per-module) compilation options:

$(OBJDIR)/tsAES.o:     CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsDES.o:     CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsTDES.o:    CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsSHA1.o:    CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsSHA256.o:  CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsSHA512.o:  CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsDVBCSA2.o: CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)

# Dektec code (if not empty) is encapsulated into the TSDuck library.

OBJS += $(DTAPI_OBJECT)

# Add libtsduck internal headers.

CXXFLAGS_INCLUDES += $(addprefix -I,$(shell find . -type d -name private))

# Library containing all modules.
# - Both static and dynamic libraries are created but only use the dynamic one when building
#   tools and plugins.
# - There are so many object files that one single big 'ar' command is faster than individual
#   'ar' commands per object module.

.PHONY: libs
libs: $(STATIC_LIBTSDUCK) $(if $(STATIC),,$(SHARED_LIBTSDUCK))

$(STATIC_LIBTSDUCK): $(OBJS)
	@echo '  [AR] $@'; $(AR) $(ARFLAGS) $@ $^

$(SHARED_LIBTSDUCK): $(OBJS)
	@echo '  [DTAPI] $(if $(DTAPI_OBJECT),using $(DTAPI_OBJECT),no DTAPI available)'; \
	echo '  [LD] $@'; \
	$(CXX) $(CXXFLAGS) $(SOFLAGS) $^ $(LDLIBS) -shared -o $@

# Installation targets.

.PHONY: install install-tools install-devel
install: install-tools install-devel

install-tools: $(SHARED_LIBTSDUCK)
	install -d -m 755 $(SYSROOT)$(USRLIBDIR)
	$(if $(MACOS),rm -rf $(SYSROOT)$(USRLIBDIR)/libtsduck.so,)
	install -m 644 $(SHARED_LIBTSDUCK) $(SYSROOT)$(USRLIBDIR)
	+@$(RECURSE)

install-devel: $(STATIC_LIBTSDUCK) tsduck.h
	rm -rf $(SYSROOT)$(SYSPREFIX)/include/tsduck
	install -d -m 755 $(SYSROOT)$(USRLIBDIR) $(SYSROOT)$(SYSPREFIX)/include/tsduck
	install -m 644 $(STATIC_LIBTSDUCK) $(SYSROOT)$(USRLIBDIR)
	install -m 644 $(shell find . -name 'ts*.h' ! -path '*/private/*' ! -path '*/windows/*' ! -path '*/$(SKIPDIR)/*' $(if $(NOTELETEXT),! -name tsTeletextDemux.h,)) $(SYSROOT)$(SYSPREFIX)/include/tsduck
	$(if $(NOTELETEXT),sed tsduck.h -e '/tsTeletextDemux.h/d' >$(SYSROOT)$(SYSPREFIX)/include/tsduck/tsduck.h,)
	+@$(RECURSE)
