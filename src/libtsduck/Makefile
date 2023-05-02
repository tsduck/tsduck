#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
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

# Detect the presence of the Dektec DTAPI.
# Define NODTAPI to compile without Dektec support.
# NODTAPI is automatically defined if no DTAPI is found.

ifeq ($(NODTAPI)$(NODEKTEC),)
    ifeq ($(filter-out default headers libs,$(MAKECMDGOALS)),)
        DTAPI_OBJECT := $(shell ../../scripts/dtapi-config.sh --object --download $(if $(M32),--m32,))
        DTAPI_HEADER := $(shell ../../scripts/dtapi-config.sh --header)
    endif
    ifeq ($(DTAPI_OBJECT)$(DTAPI_HEADER),)
        NODTAPI = true
    else
        LIBTSDUCK_CXXFLAGS_INCLUDES += -isystem $(dir $(DTAPI_HEADER))
    endif
endif

# Detect the presence of the Vatek library.
# Define NOVATEK to compile without Vatek support.
# NOVATEK is automatically defined if no Vatek library is found.

ifeq ($(NOVATEK),)
    ifeq ($(filter-out default headers libs,$(MAKECMDGOALS)),)
        VATEK_CFLAGS := $(shell ../../scripts/vatek-config.sh --cflags --download)
        VATEK_LIBS := $(shell ../../scripts/vatek-config.sh --ldlibs)
    endif
    ifeq ($(VATEK_CFLAGS)$(VATEK_LIBS),)
        NOVATEK = true
    else
        LIBTSDUCK_CXXFLAGS_INCLUDES += $(VATEK_CFLAGS)
	LIBTSDUCK_LDLIBS += $(VATEK_LIBS)
    endif
endif

# Now, we can include the common makefile.

include ../../Makefile.inc

# Most source files are located in subdirectories but should be considered as part of libtsduck.

VPATH := $(filter-out $(addprefix %/,$(OTHER_OS)),$(patsubst %/,%,$(sort $(dir $(wildcard */*.cpp */*/*.cpp)))))
VPATH_SOURCES := $(sort $(notdir $(wildcard $(addsuffix /*.cpp,$(VPATH)))))

# Implicit search directives.

vpath %.cpp $(VPATH)

# Add dependency files for sources in VPATH.

ifeq ($(DONT_BUILD_DEPS),)
    -include $(addprefix $(OBJDIR)/,$(addsuffix .dep,$(notdir $(basename $(VPATH_SOURCES)))))
endif

# All sources and objects in libtsduck.

SOURCES := $(VPATH_SOURCES) $(sort $(notdir $(wildcard *.cpp)))
OBJS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SOURCES))))

# Building the TSDuck library.

default: headers libs
	+@$(call F_RECURSE,config python java)

# The tsduck.h header is automatically generated from all headers.

TSDUCK_H = $(BINDIR)/include/tsduck.h

.PHONY: headers
headers: $(TSDUCK_H)
$(TSDUCK_H): $(filter-out %Template.h,$(wildcard *.h */*.h */*/*.h))
	@echo '  [GEN] $(notdir $@)'; \
	mkdir -p $(dir $@); \
	$(PYTHON) $(SCRIPTSDIR)/build-tsduck-header.py $@

$(OBJDIR)/tsduck.o: $(TSDUCK_H)
$(OBJDIR)/tsduck.o: CXXFLAGS_INCLUDES += -I$(dir $(TSDUCK_H))

# Some libraries are used internally to libtsduck only.

CXXFLAGS_JAVA := $(shell $(SCRIPTSDIR)/java-config.sh --cflags)
CXXFLAGS_INCLUDES += $(CXXFLAGS_JAVA) $(LIBTSDUCK_CXXFLAGS_INCLUDES)

# Specific (per-module) compilation options:

$(OBJDIR)/tsAES.o:     CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsDES.o:     CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsTDES.o:    CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsSHA1.o:    CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsSHA256.o:  CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsSHA512.o:  CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
$(OBJDIR)/tsDVBCSA2.o: CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)

ifeq ($(LOCAL_OS)-$(subst aarch64,arm64,$(LOCAL_ARCH)),linux-arm64)
    # On Linux Arm64, allow the usage of specialized instructions by the compiler.
    # The code will explicitly check at run time if they are supported before using them.
    # We must limit this to specialized modules which are never called when these
    # instructions are not supported.
    $(OBJDIR)/tsCRC32.accel.o:  CXXFLAGS_TARGET = -march=armv8-a+crc
    $(OBJDIR)/tsAES.accel.o:    CXXFLAGS_TARGET = -march=armv8-a+crypto
    $(OBJDIR)/tsSHA1.accel.o:   CXXFLAGS_TARGET = -march=armv8-a+crypto
    $(OBJDIR)/tsSHA256.accel.o: CXXFLAGS_TARGET = -march=armv8-a+crypto+sha2
    $(OBJDIR)/tsSHA512.accel.o: CXXFLAGS_TARGET = -march=armv8.2-a+crypto+sha2+sha3
endif

# Add libtsduck internal headers when compiling libtsduck.

CXXFLAGS_INCLUDES += $(addprefix -I,$(PRIVATE_INCLUDES))

# By default, both static and dynamic libraries are created but only use
# the dynamic one when building tools and plugins. In case of static build,
# only build the static library.

.PHONY: libs
libs: $(STATIC_LIBTSDUCK) $(if $(STATIC),,$(SHARED_LIBTSDUCK))

# The shared library contains all modules.
# We change the current directory to OBJDIR and use object file names without directory,
# otherwise the command line is too long when the directory path is long.

$(SHARED_LIBTSDUCK): $(OBJS) $(DTAPI_OBJECT)
	@echo '  [DTAPI] $(if $(DTAPI_OBJECT),using $(DTAPI_OBJECT),no DTAPI available)'; \
	echo '  [LD] $@'; \
	cd $(OBJDIR); \
	$(CXX) $(SOFLAGS) $(LDFLAGS) $(notdir $(OBJS)) $(DTAPI_OBJECT) $(LIBTSDUCK_LDLIBS) $(LDLIBS_EXTRA) $(LDLIBS) -shared -o $@

# The static library is build differently. There are four specific categories:
# tables, descriptors, charsets and plugins. These classes use self-registration
# in their initialization. They are usually not individually referenced. When
# the category is referenced, all members must be loaded. In each category, a
# partial link is performed to pre-link all members in a big object file.
# If NOSTATICBLOB is defined, this phase is skipped and all object files are
# individually archived without grouping.

ifneq ($(NOSTATICBLOB),)

OBJS_STATIC_LIB = $(filter-out $(OBJDIR)/tsduck.o,$(OBJS))

else

F_OBJ_BLOB  = $(addsuffix .o,$(addprefix $(OBJDIR)/,$(sort $(notdir $(basename $(wildcard $(1)/*.cpp))))))
OBJS_TABLES = $(call F_OBJ_BLOB,dtv/tables)
OBJS_DESCS  = $(call F_OBJ_BLOB,dtv/descriptors)
OBJS_CHARS  = $(call F_OBJ_BLOB,dtv/charset)
OBJS_PLUGS  = $(call F_OBJ_BLOB,plugins/plugins)

OBJ_ALLTABLES = $(OBJDIR)/alltables.o
OBJ_ALLDESCS  = $(OBJDIR)/alldescriptors.o
OBJ_ALLCHARS  = $(OBJDIR)/allcharsets.o
OBJ_ALLPLUGS  = $(OBJDIR)/allplugins.o

$(OBJ_ALLTABLES): $(OBJS_TABLES)
	@echo '  [LD] $@'; $(LD) -r $^ -o $@
$(OBJ_ALLDESCS): $(OBJS_DESCS)
	@echo '  [LD] $@'; $(LD) -r $^ -o $@
$(OBJ_ALLCHARS): $(OBJS_CHARS)
	@echo '  [LD] $@'; $(LD) -r $^ -o $@
$(OBJ_ALLPLUGS): $(OBJS_PLUGS)
	@echo '  [LD] $@'; $(LD) -r $^ -o $@

OBJS_STATIC_LIB = $(filter-out $(OBJDIR)/tsduck.o $(OBJS_TABLES) $(OBJS_DESCS) $(OBJS_CHARS) $(OBJS_PLUGS),$(OBJS)) \
    $(OBJ_ALLTABLES) $(OBJ_ALLDESCS) $(OBJ_ALLCHARS) $(OBJ_ALLPLUGS)

endif

# Build the static library using one single big 'ar'. There are so many object files
# that this is much faster than individual 'ar' commands per object module.
# We change the current directory to OBJDIR and use object file names without directory,
# otherwise the command line is too long when the directory path is long.

$(STATIC_LIBTSDUCK): $(OBJS_STATIC_LIB) $(DTAPI_OBJECT)
	@echo '  [AR] $@'; \
	cd $(OBJDIR); \
	$(AR) $(ARFLAGS) $@ $(notdir $(OBJS_STATIC_LIB)) $(DTAPI_OBJECT)

# Installation targets.

.PHONY: install-tools install-devel

install-tools: $(SHARED_LIBTSDUCK)
	install -d -m 755 $(SYSROOT)$(USRLIBDIR)
	$(if $(MACOS),rm -rf $(SYSROOT)$(USRLIBDIR)/libtsduck.so,)
	install -m 644 $(SHARED_LIBTSDUCK) $(SYSROOT)$(USRLIBDIR)
	+@$(call F_RECURSE,config python java)

PRECONFIG = $(SYSROOT)$(SYSPREFIX)/include/tsduck/tsPreConfiguration.h

install-devel: $(STATIC_LIBTSDUCK) $(TSDUCK_H)
	rm -rf $(SYSROOT)$(SYSPREFIX)/include/tsduck
	install -d -m 755 $(SYSROOT)$(USRLIBDIR) $(SYSROOT)$(SYSPREFIX)/include/tsduck
	install -m 644 $(STATIC_LIBTSDUCK) $(SYSROOT)$(USRLIBDIR)
	install -m 644 $(addsuffix /*.h,$(PUBLIC_INCLUDES)) $(TSDUCK_H) $(SYSROOT)$(SYSPREFIX)/include/tsduck
	$(if $(NOPCSC), echo '#define TS_NO_PCSC 1' >>$(PRECONFIG))
	$(if $(NOGITHUB), echo '#define TS_NO_GITHUB 1' >>$(PRECONFIG))
	$(if $(NODTAPI), echo '#define TS_NO_DTAPI 1' >>$(PRECONFIG))
	$(if $(NOHIDES), echo '#define TS_NO_HIDES 1' >>$(PRECONFIG))
	$(if $(NOVATEK), echo '#define TS_NO_VATEK 1' >>$(PRECONFIG))
	$(if $(NOEDITLINE), echo '#define TS_NO_EDITLINE 1' >>$(PRECONFIG))
	$(if $(NOCURL), echo '#define TS_NO_CURL 1' >>$(PRECONFIG))
	$(if $(NOSRT), echo '#define TS_NO_SRT 1' >>$(PRECONFIG))
	$(if $(NORIST), echo '#define TS_NO_RIST 1' >>$(PRECONFIG))
	+@$(call F_RECURSE,config python java)
