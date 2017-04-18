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
#  Makefile for dektec subdirectory.
#
#-----------------------------------------------------------------------------

include ../Makefile.tsduck

# By default, download and expand LinuxSDK (if necessary) on Linux only.
ifdef MACOS
default:
	@true
else
default: dtapi
endif

EXTRA_DISTCLEAN += WinSDK.zip WinSDK LinuxSDK.zip LinuxSDK.tar.gz LinuxSDK
RECURSE_ORDER = none

.PHONY: download expand dtapi

DTAPI_URL  := http://www.dektec.com/products/SDK/DTAPI/Downloads/LinuxSDK.tar.gz
DTAPI_FILE := $(notdir $(DTAPI_URL))
DTAPI_DIR  := $(basename $(basename $(DTAPI_FILE)))
DTAPI_H    := $(DTAPI_DIR)/DTAPI/Include/DTAPI.h

# Forced download of DTAPI from Dektec Web site.
# If DTAPI_ORIGIN is defined (on the make command line for instance), it can be
# the path of a local copy of LinuxSDK.tar.gz which is used instead of download.
download:
	$(if $(wildcard $(DTAPI_ORIGIN)),cp -f $(DTAPI_ORIGIN) $(DTAPI_FILE),wget -O $(DTAPI_FILE) $(DTAPI_URL))
	touch $(DTAPI_FILE)

# Download if not present.
$(DTAPI_FILE):
	$(MAKE) download

# Forced expand of LinuxSDK.
expand: 
	rm -rf $(DTAPI_DIR)
	tar xfz $(DTAPI_FILE)
	find $(DTAPI_DIR) -print0 | xargs -0 touch

# Expand LinuxSDK if archive is newer.
dtapi: $(DTAPI_H)
	@true
$(DTAPI_H): $(DTAPI_FILE)
	$(MAKE) expand
