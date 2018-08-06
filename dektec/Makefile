#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2018, Thierry Lelegard
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
#  Makefile for dektec subdirectory.
#
#-----------------------------------------------------------------------------

include ../Makefile.tsduck

# By default, download and expand LinuxSDK (if necessary).
# This SDK is available on Linux and Windows, Intel CPU's only.

DTAPI_TARGET :=
ifeq ($(LOCAL_OS),linux)
    ifeq ($(MAIN_ARCH),i386)
        DTAPI_TARGET := dtapi
    endif
    ifeq ($(MAIN_ARCH),x86_64)
        DTAPI_TARGET := dtapi
    endif
endif

default: $(DTAPI_TARGET)
	@true

EXTRA_DISTCLEAN += WinSDK*.zip WinSDK LinuxSDK.zip LinuxSDK*.tar.gz LinuxSDK LinuxSDK.version
RECURSE_ORDER = none

.PHONY: copy download sdkfile geturl expand dtapi

# The file LinuxSDK.version is created to contain the actual URL name.
DTNAME_FILE = LinuxSDK.version

# If DTAPI_ORIGIN is defined (on the make command line for instance), it is the
# path of a local copy of LinuxSDK*.tar.gz which is used instead of download.
# The name of the tarball is either from DTAPI_ORIGIN, LinuxSDK.version or empty.
DTAPI_URL = $(file <$(DTNAME_FILE))
DTAPI_FILE = $(if $(wildcard $(DTAPI_ORIGIN)),$(notdir $(DTAPI_ORIGIN)),$(if $(wildcard $(DTNAME_FILE)),$(notdir $(DTAPI_URL)),))

# The tarball file always expands as LinuxSDK.
DTAPI_DIR = LinuxSDK
DTAPI_H   = $(DTAPI_DIR)/DTAPI/Include/DTAPI.h

# Get the LinuxSDK tarball, either from a local copy or download.
copy:
	cp -f $(DTAPI_ORIGIN) $(DTAPI_FILE)
download: geturl
	curl --silent --show-error --location '$(DTAPI_URL)' -o $(DTAPI_FILE)
sdkfile: $(if $(wildcard $(DTAPI_ORIGIN)),copy,download)
	touch $(DTAPI_FILE)

# Get full URL of LinuxSDK.
geturl:
	./get-dektec-linux-sdk-url.sh >$(DTNAME_FILE)

# Forced expand of LinuxSDK.
expand:
	rm -rf $(DTAPI_DIR)
	tar xfz $(DTAPI_FILE)
	find $(DTAPI_DIR) -print0 | xargs -0 touch

# Expand LinuxSDK if archive is newer.
dtapi: $(DTAPI_H)
	@true
$(DTAPI_H): $(if $(wildcard $(DTAPI_FILE)),$(DTAPI_FILE),sdkfile)
	$(MAKE) expand
