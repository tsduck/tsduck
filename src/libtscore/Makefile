#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for libtscore.
#
#-----------------------------------------------------------------------------

# Don't generate .dep for tscore.cpp. It depends on tscore.h, which will be generated later.
NODEPS_SRC = tscore.cpp

include ../../Makefile.inc

# Most source files are located in subdirectories but should be considered as part of libtscore.
VPATH := $(filter-out $(OTHER_OS) $(addprefix %/,$(OTHER_OS)), \
           $(patsubst %/,%,$(sort $(dir $(wildcard */*.cpp */*/*.cpp */*/*/*.cpp)))))
VPATH_SOURCES := $(sort $(notdir $(wildcard $(addsuffix /*.cpp,$(VPATH)))))

# Implicit search directives.
vpath %.cpp $(VPATH)

# Add dependency files for sources in VPATH.
ifeq ($(DONT_BUILD_DEPS),)
    -include $(addprefix $(OBJDIR)/,$(addsuffix .dep,$(notdir $(basename $(VPATH_SOURCES)))))
endif

# All sources and objects in libtscore.
SOURCES := $(VPATH_SOURCES) $(sort $(notdir $(wildcard *.cpp)))
OBJS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SOURCES))))

# Building the TSCore library.
default: headers libs
	+@$(call RECURSE,config $(if $(NOPYTHON),,python) $(if $(NOJAVA),,java))

# The tscore.h header is automatically generated from all headers.
TSCORE_H = $(BINDIR)/include/tscore.h

.PHONY: headers
headers: $(TSCORE_H)
$(TSCORE_H): $(wildcard *.h */*.h */*/*.h */*/*/*.h)
	$(call LOG,[GEN] $(notdir $@)) mkdir -p $(dir $@); \
	$(PYTHON) $(SCRIPTSDIR)/build-tsduck-header.py tscore $@

$(OBJDIR)/tscore.o: $(TSCORE_H)
$(OBJDIR)/tscore.o: CXXFLAGS_INCLUDES += -I$(dir $(TSCORE_H))

# Specific compilation options:
CXXFLAGS_INCLUDES += $(LIBTSCORE_CXXFLAGS_INCLUDES)

ifeq ($(LOCAL_OS)-$(subst aarch64,arm64,$(LOCAL_ARCH)),linux-arm64)
    # On Linux Arm64, allow the usage of specialized instructions by the compiler.
    # The code will explicitly check at run time if they are supported before using them.
    # We must limit this to specialized modules which are never called when these
    # instructions are not supported.
    $(OBJDIR)/tsCRC32.accel.o: CXXFLAGS_TARGET = -march=armv8-a+crc
endif

# By default, both static and dynamic libraries are created but only use
# the dynamic one when building tools and plugins. In case of static build,
# only build the static library.
.PHONY: libs
libs: $(if $(NOSTATIC),,$(STATIC_LIBTSCORE)) $(if $(STATIC),,$(SHARED_LIBTSCORE))

# The shared library contains all modules.
# We change the current directory to OBJDIR and use object file names without directory,
# otherwise the command line is too long when the directory path is long.
$(SHARED_LIBTSCORE): $(OBJS)
	$(call LOG,[LD] $@) cd $(OBJDIR); \
	$(CXX) $(SOFLAGS) $(LDFLAGS) $(notdir $(OBJS)) $(LIBTSCORE_LDLIBS) $(LDLIBS_EXTRA) $(LDLIBS) -shared -o $@

# Build the static library using one single big 'ar'. There are so many object files
# that this is much faster than individual 'ar' commands per object module.
# We change the current directory to OBJDIR and use object file names without directory,
# otherwise the command line is too long when the directory path is long.
OBJS_STATIC_LIB = $(filter-out $(OBJDIR)/tscore.o,$(OBJS))

$(STATIC_LIBTSCORE): $(OBJS_STATIC_LIB)
	$(call LOG,[AR] $@) cd $(OBJDIR); \
	$(AR) $(ARFLAGS) $@ $(notdir $(OBJS_STATIC_LIB))

# Installation targets.
.PHONY: install-tools install-devel

install-tools: $(SHARED_LIBTSCORE)
	install -d -m 755 $(SYSROOT)$(USRLIBDIR)
	install -m 644 $(SHARED_LIBTSCORE) $(SYSROOT)$(USRLIBDIR)
	$(STRIP) $(SYSROOT)$(USRLIBDIR)/$(notdir $(SHARED_LIBTSCORE))
	+@$(call RECURSE,config)

PRECONFIG = $(SYSROOT)$(SYSPREFIX)/include/tscore/tsPreConfiguration.h

install-devel: $(if $(NOSTATIC),,$(STATIC_LIBTSCORE)) $(TSCORE_H)
	rm -rf $(SYSROOT)$(SYSPREFIX)/include/tscore
	install -d -m 755 $(SYSROOT)$(USRLIBDIR) $(SYSROOT)$(SYSPREFIX)/include/tscore
	$(if $(NOSTATIC),,install -m 644 $(STATIC_LIBTSCORE) $(SYSROOT)$(USRLIBDIR))
	install -m 644 $(addsuffix /*.h,$(LIBTSCORE_INCLUDES)) $(TSCORE_H) $(SYSROOT)$(SYSPREFIX)/include/tscore
	$(if $(NOOPENSSL), echo '#define TS_NO_OPENSSL 1' >>$(PRECONFIG))
	$(if $(NOPCSC), echo '#define TS_NO_PCSC 1' >>$(PRECONFIG))
	$(if $(NOGITHUB), echo '#define TS_NO_GITHUB 1' >>$(PRECONFIG))
	$(if $(NODTAPI), echo '#define TS_NO_DTAPI 1' >>$(PRECONFIG))
	$(if $(NOHIDES), echo '#define TS_NO_HIDES 1' >>$(PRECONFIG))
	$(if $(NOVATEK), echo '#define TS_NO_VATEK 1' >>$(PRECONFIG))
	$(if $(NOEDITLINE), echo '#define TS_NO_EDITLINE 1' >>$(PRECONFIG))
	$(if $(NOCURL), echo '#define TS_NO_CURL 1' >>$(PRECONFIG))
	$(if $(NOZLIB), echo '#define TS_NO_ZLIB 1' >>$(PRECONFIG))
	$(if $(NOSDEFL), echo '#define TS_NO_SDEFL 1' >>$(PRECONFIG))
	$(if $(NOSRT), echo '#define TS_NO_SRT 1' >>$(PRECONFIG))
	$(if $(NORIST), echo '#define TS_NO_RIST 1' >>$(PRECONFIG))
	+@$(call RECURSE,config)
