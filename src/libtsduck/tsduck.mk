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
#  To be included by third-party applications using TSDuck library.
#
#  Also define safe and rigorous compilation options. If your application
#  does not compile with that, do not fix this file, fix your application !
#
#  The following variables may be set before inclusion:
#
#  DEBUG
#    If defined (any value), the debug information is included in any
#    compilation. Optimizations are turned off.
#
#  CFLAGS_INCLUDES
#    Contains specific include options.
#
#  CFLAGS_OPTIMIZE
#    Contains specific optimization for the gcc compiler. Defaults to -O2.
#    Ignored if DEBUG is defined.
#
#  TARGET_ARCH
#    Contains the target architecture for gcc. Defaults to -march=`uname -m`.
#    Should be overriden if your `uname -m` is not recognized by gcc.
#
#  NODEP_MAKECMDGOALS
#    List of make targets which do not require the .dep files. Useful to
#    avoid useless generation of .dep files for maintenance targets.
#
#------------------------------------------------------------------------------

# Stay to a known shell, avoid exotic shells as default. 

SHELL := /bin/bash

# Recursive invocations of make should be silent

MAKEFLAGS += --no-print-directory

# The TSDuck include directory is the one that contains the currently included makefile.

TS_INCLUDE_DIR := $(patsubst %/,%,$(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))

# Specific include directories

CFLAGS_INCLUDES += -isystem /usr/include/PCSC -isystem $(TS_INCLUDE_DIR)

# External libraries

LDLIBS += -ltsduck -lpcsclite -lpthread -lrt -ldl -lm -lstdc++

# Compilation options.

LOCAL_ARCH := $(shell uname -m)
TARGET_ARCH ?= -march=$(LOCAL_ARCH:x86_64=x86-64)
CFLAGS_OPTIMIZE ?= -O2 -fno-strict-aliasing

ifdef DEBUG
  CFLAGS_DEBUG = -g -DDEBUG
  LDFLAGS_DEBUG =
else
  CFLAGS_DEBUG = $(CFLAGS_OPTIMIZE)
  LDFLAGS_DEBUG =
endif

CFLAGS_WARNINGS = -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes -Wextra -Wno-unused-parameter
CXXFLAGS_WARNINGS = -Werror -Wall -Wextra -Wno-unused-parameter -Woverloaded-virtual

CFLAGS   = $(CFLAGS_DEBUG) $(CFLAGS_WARNINGS) $(CFLAGS_INCLUDES) -fPIC
CXXFLAGS = $(CFLAGS_DEBUG) $(CXXFLAGS_WARNINGS) $(CFLAGS_INCLUDES) -fPIC
LDFLAGS  = $(LDFLAGS_DEBUG)
ARFLAGS  = rc

# Compilation rules with short display

%.o: %.c
	@echo '  [CC] $<'; \
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<
%.o: %.cpp
	@echo '  [CXX] $<'; \
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<
(%): %
	@echo '  [AR] $<'; \
	$(AR) $(ARFLAGS) $@ $<
%: %.o
	@echo '  [LD] $@'; \
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
%.so: %.o
	@echo '  [CC] $@'; \
	$(CC) $(CFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -shared -o $@

# Object files dependencies

%.dep: %.c
	@echo '  [DEP] $<'; \
	$(CC) -MM $(CPPFLAGS) $(CFLAGS_INCLUDES) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' >$@
%.dep: %.cpp
	@echo '  [DEP] $<'; \
	$(CC) -MM $(CPPFLAGS) $(CFLAGS_INCLUDES) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' >$@

# EXECS specifies the list of all main source files (no suffix).
# MODULES specifies the list of non-main object modules (no suffix).
#
# These lists are built automatically from C/C++ files in the directory.
# The modules are all C/C++ files for which a header file exists
# (ie. all f.c or f.cpp files with a corresponding f.h file).
# The execs are all C/C++ files which are not a module.

HEADERS := $(notdir $(basename $(wildcard *.h)))
SOURCES := $(notdir $(basename $(wildcard *.c *.cpp)))
EXECS   := $(sort $(filter-out $(HEADERS),$(SOURCES)))
MODULES := $(sort $(filter-out $(EXECS),$(SOURCES)))

# Rebuild and include all .dep files, unless the explicit targets do not require the .dep files.

ifeq ($(MAKECMDGOALS),)
  REQUIREDEP := true
else
  REQUIREDEP := $(filter-out install clean% $(NODEP_MAKECMDGOALS),$(MAKECMDGOALS))
endif

ifneq ($(REQUIREDEP),)
  -include $(addsuffix .dep,$(SOURCES))
endif

# List of subdirectories in the current directory.
# $(RECURSE) is a shell command which recurses the current
# make targets in the subdirectories (useful for "all" or "clean").

SUBDIRS := $(sort $(shell find . -noleaf -mindepth 1 -maxdepth 1 -type d ! -name '.*'))
RECURSE = for dir in $(SUBDIRS); do $(MAKE) -C $$dir $@; done
