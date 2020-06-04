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
#  Common GNU Make definitions for tsduck-devel package.
#
#  Installed in same directory as TSDuck header files.
#  To be included by Makefile of third-party applications using TSDuck library.
#
#  If TS_STATIC is defined, the application is linked against the TSDuck
#  library. Otherwise, the dynamic library is used.
#
#------------------------------------------------------------------------------

# Check local platform.
TS_SYSTEM := $(shell uname -s | tr A-Z a-z)
TS_LINUX  := $(if $(subst linux,,$(TS_SYSTEM)),,true)
TS_MAC    := $(if $(subst darwin,,$(TS_SYSTEM)),,true)

# The TSDuck include directory is the one that contains the currently included makefile.
TS_INCLUDE_DIR := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))
TS_INCLUDES += -I$(TS_INCLUDE_DIR)

# The root directory of the TSDuck installation is two levels above (eg. /usr/include/tsduck -> /usr).
# Useful when TSDuck was installed in a local directory (not system-wide).
TS_ROOT_DIR := $(abspath $(TS_INCLUDE_DIR)/../..)

# Default root directory for the platform.
TS_SYSROOT_DIR := $(if $(TS_MAC),/usr/local,/usr)

# Options to link with TSDuck library.
# If the library file is present in TS_ROOT_DIR, use it. Otherwise, use from TS_SYSROOT_DIR.
TS_LIBFILE := $(wildcard $(TS_ROOT_DIR)/lib*/libtsduck.$(if $(TS_STATIC),a,so))
TS_LIBOPT := $(if $(TS_LIBFILE),-L$(dir $(TS_LIBFILE)),) -ltsduck

# Libraries to link with.
CFLAGS_CURL := $(shell curl-config --cflags)
LDLIBS_CURL := $(shell curl-config --libs)
LDLIBS += $(TS_LIBOPT) $(LDLIBS_CURL)

# Additional system-dependent options
ifdef TS_MAC
    TS_INCLUDES += -I/usr/local/opt/pcsc-lite/include/PCSC
    LDLIBS += -L/usr/local/opt/pcsc-lite/lib -lpcsclite -lsrt -lpthread -ldl -lm -lstdc++
else
    TS_INCLUDES += -I/usr/include/PCSC
    ifneq ($(wildcard /usr/include/srt/*.h)$(wildcard /usr/local/include/srt/*.h),)
        LDLIBS += -lsrt
    endif
    LDLIBS += -lpcsclite -lpthread -lrt -ldl -lm -lstdc++
endif

# Includes may use either CFLAGS of CXXFLAGS
CFLAGS += $(TS_INCLUDES) $(CFLAGS_CURL) --std=c++11
CXXFLAGS += $(TS_INCLUDES) $(CFLAGS_CURL) --std=c++11
