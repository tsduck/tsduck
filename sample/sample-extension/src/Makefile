# Makefile for TSDuck extension.
# The TSDuck development package must have been installed.

# Default target is build all.
default: all

# TSDuck definitions are in /usr on Linux and /usr/local on Mac.
SYSPREFIX ?= $(if $(subst Darwin,,$(shell uname -s)),/usr,/usr/local)
include $(SYSPREFIX)/include/tsduck/tsduck.mk

# Default installation directory prefix.
# RPM packagers should override this in the "make install" command line.
DESTDIR :=

# Compilation rules for shared libraries.
%.so: %.o
	$(CC) $(CFLAGS) $(SOFLAGS) $(LDFLAGS) $^ $(LDLIBS) -shared -o $@

# Local compilation flags (-fPIC for shared objects).
CXXFLAGS += -fPIC

# Names of all source (.cpp) and header (.h) files without suffix:
SOURCES := $(sort $(notdir $(basename $(wildcard *.cpp))))
HEADERS := $(sort $(notdir $(basename $(wildcard *.h))))

# There is at most one TSDuck extension library, the file starting in "tslibext_":
LIBEXT_SRC := $(firstword $(sort $(notdir $(basename $(wildcard tslibext_*.cpp)))))
LIBEXT_SO  := $(addsuffix .so,$(LIBEXT_SRC))

# There is any number of tsp plugins, all files starting in "tsplugin_".
# We assume here that each plugin has only one source file, the rest is in the tslibext library.
PLUGINS_SRC := $(sort $(notdir $(basename $(wildcard tsplugin_*.cpp))))
PLUGINS_SO  := $(addsuffix .so,$(PLUGINS_SRC))

# Names of the "tools", the executables. They are all source files without header.
# We assume here that each tool has only one source file, the rest is in the tslibext library.
TOOLS := $(filter-out tsplugin_% tslibext_% $(HEADERS),$(SOURCES))

# The modules to link in the tslibext are all sources which are neither plugins nor tools.
LIBSOURCES := $(filter-out $(TOOLS) $(PLUGINS_SRC),$(SOURCES))
LIBOBJS    := $(addsuffix .o,$(LIBSOURCES))

# Configuration files.
CONFIGS := $(wildcard *.xml *.names)

# Main target: build all.
all: $(LIBEXT_SO) $(PLUGINS_SO) $(TOOLS)
	@true
clean distclean:
	rm -rf *.o *.so $(TOOLS)

$(LIBEXT_SO): $(LIBOBJS)
$(PLUGINS_SO) $(TOOLS): $(LIBEXT_SO)

# Installation in the same directories as TSDuck.
install: all
	install -d -m 755 $(DESTDIR)$(TS_BIN_DIR) $(DESTDIR)$(TS_LIB_DIR) $(DESTDIR)$(TS_PLUGIN_DIR) $(DESTDIR)$(TS_CONFIG_DIR)
	$(if $(TOOLS),install -m 755 $(TOOLS) $(DESTDIR)$(TS_BIN_DIR),@true)
	$(if $(LIBEXT_SO),install -m 644 $(LIBEXT_SO) $(DESTDIR)$(TS_LIB_DIR),@true)
	$(if $(PLUGINS_SO),install -m 644 $(PLUGINS_SO) $(DESTDIR)$(TS_PLUGIN_DIR),@true)
	$(if $(CONFIGS),install -m 644 $(CONFIGS) $(DESTDIR)$(TS_CONFIG_DIR),@true)
