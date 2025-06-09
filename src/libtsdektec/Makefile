#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for libtsdektec.
#
#-----------------------------------------------------------------------------

include ../../Makefile.inc

# Don't build anything here if DTAPI is excluded.

ifneq ($(NODTAPI),)
default:
	@true
else

# Most source files are located in subdirectories but should be considered as part of libtsdektec.
VPATH := $(filter-out $(OTHER_OS) $(addprefix %/,$(OTHER_OS)), $(patsubst %/,%,$(sort $(dir $(wildcard */*.cpp)))))
VPATH_SOURCES := $(sort $(notdir $(wildcard $(addsuffix /*.cpp,$(VPATH)))))

# Implicit search directives.
vpath %.cpp $(VPATH)

# Add dependency files for sources in VPATH.
ifeq ($(DONT_BUILD_DEPS),)
    -include $(addprefix $(OBJDIR)/,$(addsuffix .dep,$(notdir $(basename $(VPATH_SOURCES)))))
endif

# All sources and objects in libtsdektec.
SOURCES := $(VPATH_SOURCES) $(sort $(notdir $(wildcard *.cpp)))
OBJS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SOURCES))))

# Building the TSDektec library.
default: libs
	@true

# Specific compilation options:
CXXFLAGS_INCLUDES += $(LIBTSDEKTEC_CXXFLAGS_INCLUDES)

# By default, both static and dynamic libraries are created but only use
# the dynamic one when building tools and plugins. In case of static build,
# only build the static library.

.PHONY: libs
libs: $(if $(NOSTATIC),,$(STATIC_LIBTSDEKTEC)) $(if $(STATIC),,$(SHARED_LIBTSDEKTEC))

# The shared library contains all modules.
# We change the current directory to OBJDIR and use object file names without directory,
# otherwise the command line is too long when the directory path is long.
$(SHARED_LIBTSDEKTEC): $(OBJS) $(DTAPI_OBJECT) $(SHARED_LIBTSDUCK) $(SHARED_LIBTSCORE)
	@echo '  [DTAPI] using $(DTAPI_OBJECT)'
	$(call LOG,[LD] $@) cd $(OBJDIR); \
	$(CXX) $(SOFLAGS) $(LDFLAGS) $(notdir $(OBJS)) $(DTAPI_OBJECT) -L$(dir $(SHARED_LIBTSCORE)) -ltsduck -ltscore \
	    $(LIBTSDEKTEC_LDLIBS) $(LDLIBS_EXTRA) $(LDLIBS) -shared -o $@

# Build the static library.
# We change the current directory to OBJDIR and use object file names without directory,
# otherwise the command line is too long when the directory path is long.
$(STATIC_LIBTSDEKTEC): $(OBJS) $(DTAPI_OBJECT)
	$(call LOG,[AR] $@) cd $(OBJDIR); \
	$(AR) $(ARFLAGS) $@ $(notdir $(OBJS)) $(DTAPI_OBJECT)

# Installation targets.
.PHONY: install-tools install-devel

install-tools: $(SHARED_LIBTSDEKTEC)
	install -d -m 755 $(SYSROOT)$(USRLIBDIR)
	install -m 644 $(SHARED_LIBTSDEKTEC) $(SYSROOT)$(USRLIBDIR)
	$(STRIP) $(SYSROOT)$(USRLIBDIR)/$(notdir $(SHARED_LIBTSDEKTEC))

install-devel: $(if $(NOSTATIC),,$(STATIC_LIBTSDEKTEC))
	$(if $(NOSTATIC),,install -m 644 $(STATIC_LIBTSDEKTEC) $(SYSROOT)$(USRLIBDIR))

endif # NODTAPI
