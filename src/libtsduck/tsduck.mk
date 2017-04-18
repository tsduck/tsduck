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
#  Common GNU Make definitions for tsduck-devel package.
#
#  Installed in same directory as TSDuck header files.
#  To be included by Makefile of third-party applications using TSDuck library.
#
#------------------------------------------------------------------------------

# Check local platform.
TS_SYSTEM := $(shell uname -s | tr A-Z a-z)
TS_LINUX  := $(if $(subst linux,,$(TS_SYSTEM)),,true)
TS_MAC    := $(if $(subst darwin,,$(TS_SYSTEM)),,true)

# The TSDuck include directory is the one that contains the currently included makefile.
TS_INCLUDE_DIR := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))

# Specific include directories
CFLAGS += -isystem /usr/include/PCSC -isystem $(TS_INCLUDE_DIR)

# External libraries
LDLIBS += -ltsduck -lpcsclite -lpthread $(if $(TS_MAC),,-lrt) -ldl -lm -lstdc++
