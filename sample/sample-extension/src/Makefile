# Makefile for TSDuck extension.
# The TSDuck development package must have been installed.

# Default target is build all.
default: all

# TSDuck definitions.
ifeq ($(shell uname -s),Darwin) # Mac
    SYSPREFIX ?= /usr/local
else # Linux
    SYSPREFIX ?= /usr
endif
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

# Installation: all shared libraries and executables are installed in the system "bin".
# This is typically where TSDuck is installed (required to find extensions and plugins).
install: all
	install -d -m 755 $(DESTDIR)$(SYSPREFIX)/bin
	install -m 755 $(LIBEXT_SO) $(PLUGINS_SO) $(TOOLS) $(DESTDIR)$(SYSPREFIX)/bin
	$(if $(CONFIGS),install -m 644 $(CONFIGS) $(DESTDIR)$(SYSPREFIX)/bin,@true)
