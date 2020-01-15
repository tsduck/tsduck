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

# Build options
CFLAGS_CURL := $(shell curl-config --cflags)
LDLIBS_CURL := $(shell curl-config --libs)
ifdef TS_MAC
    TS_INCLUDES += -I/usr/local/opt/pcsc-lite/include/PCSC -I$(TS_INCLUDE_DIR)
    LDLIBS += $(if $(TS_STATIC),-L/usr/local/lib -ltsduck,/usr/local/bin/tsduck.so) $(LDLIBS_CURL) -L/usr/local/opt/pcsc-lite/lib -lpcsclite -lsrt -lpthread -ldl -lm -lstdc++
    ifndef TS_STATIC
        LDFLAGS += -Wl,-rpath,@executable_path,-rpath,/usr/local/bin
        SOFLAGS = -install_name '@rpath/$(notdir $@)'
    endif
else
    TS_INCLUDES += -I/usr/include/PCSC -I$(TS_INCLUDE_DIR)
    ifeq ($(wildcard /usr/include/srt/*.h)$(wildcard /usr/local/include/srt/*.h),)
        CFLAGS += -DTS_NOSRT=1
    else
       LDLIBS += -lsrt
    endif
    LDLIBS += $(if $(TS_STATIC),-ltsduck,/usr/bin/tsduck.so) $(LDLIBS_CURL) -lpcsclite -lpthread -lrt -ldl -lm -lstdc++
    ifndef TS_STATIC
        LDFLAGS += -Wl,-rpath,'$$ORIGIN',-rpath,/usr/bin
        SOFLAGS = -Wl,-soname=$(notdir $@)
    endif
endif

# Includes may use either CFLAGS of CXXFLAGS
CFLAGS += $(TS_INCLUDES) $(CFLAGS_CURL) --std=c++11
CXXFLAGS += $(TS_INCLUDES) $(CFLAGS_CURL) --std=c++11
