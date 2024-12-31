#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for sample directory: build all samples.
#
#-----------------------------------------------------------------------------

.PHONY: default
default:
	@for dir in $(wildcard sample-*/); do $(MAKE) -C $$dir || exit $$?; done
