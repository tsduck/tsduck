#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Makefile for libtsduck.
#
#-----------------------------------------------------------------------------

# Don't generate .dep for tsduck.cpp. It depends on tsduck.h, which will be generated later.
NODEPS_SRC = tsduck.cpp

include ../../Makefile.inc

# Most source files are located in subdirectories but should be considered as part of libtsduck.
VPATH := $(filter-out $(OTHER_OS) $(addprefix %/,$(OTHER_OS)) $(if $(NOJAVA),java/% %/java) $(if $(NOPYTHON),python/% %/python), \
           $(patsubst %/,%,$(sort $(dir $(wildcard */*.cpp */*/*.cpp */*/*/*.cpp)))))
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
	+@$(call RECURSE,config $(if $(NOPYTHON),,python) $(if $(NOJAVA),,java))

# The tsduck.h header is automatically generated from all headers.
TSDUCK_H = $(BINDIR)/include/tsduck.h

.PHONY: headers
headers: $(TSDUCK_H)
$(TSDUCK_H): $(wildcard *.h */*.h */*/*.h */*/*/*.h)
	$(call LOG,[GEN] $(notdir $@)) mkdir -p $(dir $@); \
	$(PYTHON) $(SCRIPTSDIR)/build-tsduck-header.py tsduck $@

$(OBJDIR)/tsduck.o: $(TSDUCK_H)
$(OBJDIR)/tsduck.o: CXXFLAGS_INCLUDES += -I$(dir $(TSDUCK_H))

# Specific compilation options:
CXXFLAGS_INCLUDES += $(LIBTSDUCK_CXXFLAGS_INCLUDES)
$(OBJDIR)/tsDVBCSA2.o: CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)

# By default, both static and dynamic libraries are created but only use
# the dynamic one when building tools and plugins. In case of static build,
# only build the static library.
.PHONY: libs
libs: $(if $(NOSTATIC),,$(STATIC_LIBTSDUCK)) $(if $(STATIC),,$(SHARED_LIBTSDUCK))

# The shared library contains all modules.
# We change the current directory to OBJDIR and use object file names without directory,
# otherwise the command line is too long when the directory path is long.
$(SHARED_LIBTSDUCK): $(OBJS) $(SHARED_LIBTSCORE)
	$(call LOG,[LD] $@) cd $(OBJDIR); \
	$(CXX) $(SOFLAGS) $(LDFLAGS) $(notdir $(OBJS)) -L$(dir $(SHARED_LIBTSCORE)) -ltscore \
	    $(LIBTSDUCK_LDLIBS) $(LDLIBS_EXTRA) $(LDLIBS) -shared -o $@

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

F_OBJ_BLOB  = $(addsuffix .o,$(addprefix $(OBJDIR)/,$(sort $(notdir $(basename $(wildcard $(1)/*.cpp $(1)/*/*.cpp))))))
OBJS_TABLES = $(call F_OBJ_BLOB,dtv/tables)
OBJS_DESCS  = $(call F_OBJ_BLOB,dtv/descriptors)
OBJS_CHARS  = $(call F_OBJ_BLOB,dtv/charset)
OBJS_PLUGS  = $(call F_OBJ_BLOB,plugins/plugins)

OBJ_ALLTABLES = $(OBJDIR)/alltables.o
OBJ_ALLDESCS  = $(OBJDIR)/alldescriptors.o
OBJ_ALLCHARS  = $(OBJDIR)/allcharsets.o
OBJ_ALLPLUGS  = $(OBJDIR)/allplugins.o

$(OBJ_ALLTABLES): $(OBJS_TABLES)
	$(call LOG,[LD] $@) $(LD) -r $^ -o $@
$(OBJ_ALLDESCS): $(OBJS_DESCS)
	$(call LOG,[LD] $@) $(LD) -r $^ -o $@
$(OBJ_ALLCHARS): $(OBJS_CHARS)
	$(call LOG,[LD] $@) $(LD) -r $^ -o $@
$(OBJ_ALLPLUGS): $(OBJS_PLUGS)
	$(call LOG,[LD] $@) $(LD) -r $^ -o $@

OBJS_STATIC_LIB = $(filter-out $(OBJDIR)/tsduck.o $(OBJS_TABLES) $(OBJS_DESCS) $(OBJS_CHARS) $(OBJS_PLUGS),$(OBJS)) \
    $(OBJ_ALLTABLES) $(OBJ_ALLDESCS) $(OBJ_ALLCHARS) $(OBJ_ALLPLUGS)

endif

# Build the static library using one single big 'ar'. There are so many object files
# that this is much faster than individual 'ar' commands per object module.
# We change the current directory to OBJDIR and use object file names without directory,
# otherwise the command line is too long when the directory path is long.
$(STATIC_LIBTSDUCK): $(OBJS_STATIC_LIB)
	$(call LOG,[AR] $@) cd $(OBJDIR); \
	$(AR) $(ARFLAGS) $@ $(notdir $(OBJS_STATIC_LIB))

# Installation targets.
.PHONY: install-tools install-devel

install-tools: $(SHARED_LIBTSDUCK)
	install -d -m 755 $(SYSROOT)$(USRLIBDIR)
	$(if $(MACOS),rm -rf $(SYSROOT)$(USRLIBDIR)/libtsduck.so,)
	install -m 644 $(SHARED_LIBTSDUCK) $(SYSROOT)$(USRLIBDIR)
	$(STRIP) $(SYSROOT)$(USRLIBDIR)/$(notdir $(SHARED_LIBTSDUCK))
	+@$(call RECURSE,config $(if $(NOPYTHON),,python) $(if $(NOJAVA),,java))

install-devel: $(if $(NOSTATIC),,$(STATIC_LIBTSDUCK)) $(TSDUCK_H)
	rm -rf $(SYSROOT)$(SYSPREFIX)/include/tsduck
	install -d -m 755 $(SYSROOT)$(USRLIBDIR) $(SYSROOT)$(SYSPREFIX)/include/tsduck
	$(if $(NOSTATIC),,install -m 644 $(STATIC_LIBTSDUCK) $(SYSROOT)$(USRLIBDIR))
	install -m 644 $(addsuffix /*.h,$(LIBTSDUCK_INCLUDES)) $(TSDUCK_H) $(SYSROOT)$(SYSPREFIX)/include/tsduck
	+@$(call RECURSE,config $(if $(NOPYTHON),,python) $(if $(NOJAVA),,java))
