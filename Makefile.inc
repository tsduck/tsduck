#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Common makefile definitions for the TSDuck project.
#  To be included in all makefiles in the project.
#
#  General note: To speed up make recursion, some variables are exported when
#  their definition take some time.
#
#-----------------------------------------------------------------------------

# If no target precedes the inclusion of this file, use "default" as target.

.PHONY: first default
first: default
	@true

# The directory which contains the currently included Makefile is the project root.

ROOTDIR := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))

# Project specific directories.

INSTALLERDIR = $(ROOTDIR)/pkg/installers
SCRIPTSDIR   = $(ROOTDIR)/scripts
SRCROOT      = $(ROOTDIR)/src
LIBTSDUCKDIR = $(SRCROOT)/libtsduck
TSTOOLSDIR   = $(SRCROOT)/tstools
TSPLUGINSDIR = $(SRCROOT)/tsplugins

# TSDuck library files. See BINDIR later.

STATIC_LIBTSDUCK = $(BINDIR)/libtsduck.a
SHARED_LIBTSDUCK = $(BINDIR)/libtsduck$(SO_SUFFIX)

# Default installation root. Packagers should override this in the "make install"
# command line to build a temporary system root where TSDuck is installed.

SYSROOT =

#-----------------------------------------------------------------------------
# Customization of make.
#-----------------------------------------------------------------------------

# Representation of a space and a comma character (make oddities...)

EMPTY =
SPACE = $(EMPTY) $(EMPTY)
COMMA = ,

# Some nerds define exotic shells as default. Stay to a known shell.
# Skip initialization files to speed up and reproduceability.
# Print all commands if VERBOSE is defined.

SHELL = /usr/bin/env bash --noprofile --norc $(if $(VERBOSE),-x)

# Recursive invocations of make should be silent.

MAKEFLAGS += --no-print-directory

# Best multiprocessor make option if unspecified.

MAKEFLAGS_SMP = $(if $(filter -j%,$(MAKEFLAGS)),$(filter -j%,$(MAKEFLAGS)),$(if $(CPU_COUNT),-j$(CPU_COUNT)))

# Check if "make -k" is specified (ie. continue on error).

CONTINUE := $(if $(findstring k,$(filter-out --%,$(MAKEFLAGS))),true,)

# The function F_RECURSE recurses the current make targets in the specified subdirectories.
# Example: $(call F_RECURSE,src test1 test2)

F_RECURSE = for dir in $(1); do if [[ -d $$dir ]]; then $(MAKE) -C $$dir $@ $(if $(CONTINUE),,|| exit $$?); fi; done

# The functions F_LT, F_LE, F_GE, F_GT perform numeric comparisons ("true" or empty).
# Example: $(if $(call F_LT,$(VERSION),2),"version<2","version>=2")

F_COMP = $(shell [ 0$(1) -$(2) 0$(3) ] && echo true)
F_LT = $(call F_COMP,$(1),lt,$(2))
F_LE = $(call F_COMP,$(1),le,$(2))
F_GE = $(call F_COMP,$(1),ge,$(2))
F_GT = $(call F_COMP,$(1),gt,$(2))

# The function F_SEARCH searches a command in $PATH and in a few other predefined
# directories. Return the full path of the first found, empty if not found.
# Example: $(call F_SEARCH,ls)

F_SEARCH_PATH ?= $(PATH):/usr/bin:/usr/sbin:/usr/local/bin:/opt/homebrew/bin:$(HOME)/bin
F_SEARCH = $(firstword $(wildcard $(addsuffix /$(1),$(subst :, ,$(F_SEARCH_PATH)))))

# The funtion F_COMMAND searches a command with F_SEARCH. If the command is not
# found, return the second parameter as default value. The following example
# searches for a command named "gsed" and, if not found, use command "sed" as
# fallback: $(call F_COMMAND,gsed,sed)

F_COMMAND = $(eval _tmp := $(call F_SEARCH,$(1)))$(if $(_tmp),$(_tmp),$(2))

# Find GNU alternatives to sed, grep. fgrep, chmod.

ifeq ($(SED),)
    export TAR   := $(call F_COMMAND,gtar,tar)
    export SED   := $(call F_COMMAND,gsed,sed)
    export GREP  := $(call F_COMMAND,ggrep,grep)
    export FGREP := $(call F_COMMAND,gfgrep,fgrep)
    export CHMOD := $(call F_COMMAND,gchmod,chmod)
endif

# Use command python3 when available, fallback as python alone.
# But Python 3 is required for build scripts.

ifeq ($(PYTHON),)
    export PYTHON := $(call F_COMMAND,python3,python)
    ifeq ($(notdir $(PYTHON)),python)
        PYVERSION := $(filter-out Python python,$(shell $(PYTHON) --version 2>&1))
        ifneq ($(firstword $(subst ., ,$(PYVERSION))),3)
            $(error Python 3 required, current version: $(PYVERSION))
        endif
    endif
endif

# Command prefix for commands requiring superuser rights.
# Do not use it for "make install" commands, it could be used to install in
# user or temporary directory. Only use it for "rpm" or "dpkg" commands.

SUDO = $(if $(subst 0,,$(shell id -u)),sudo)

# Shell command to get TSDuck version.

GET_VERSION = $(PYTHON) $(SCRIPTSDIR)/get-version-from-sources.py

#-----------------------------------------------------------------------------
# Identify the system environment.
# - ALTDEVROOT : alternative root for development tools and library; can be
#   the Homebrew root or "/usr" if no other value is suitable
# - BASHCOMP_DIR : directory for bash completion scripts
# - BASHCOMP_AUTO : if non-empty, automatic bash completions based on command
#   name, create links for each command if necessary
# - BSD : non-empty on all BSD systems
# - CORE_COUNT : number of physical CPU cores in the machine
# - CPU_COUNT : number of logical CPU's in the machine; with hyperthreaded
#   CPU cores, CPU_COUNT is typically twice CORE_COUNT
# - DRAGONFLYBSD : non-empty on DragonFlyBSD systems
# - ETCDIR : system configuration directory; /etc if $(SYSPREFIX) is /usr
#   and $(SYSPREFIX)/etc otherwise
# - FREEBSD : non-empty on FreeBSD systems
# - HOSTNAME : host name without domain name
# - LINUX : non-empty on Linux systems
# - LINUXBREW : to be set on input; force a compilation in a Homebrew
#   environment on Linux. Homebrew on Linux (aka "Linuxbrew") installs a
#   non-standard environment where libraries are fetched from $HOMEBREW_PREFIX
#   instead of /usr. By default, ignore the Homebrew environment on Linux and
#   build using system libraries.
# - LOCAL_ARCH : local processor exact architecture ("x86_64", "armv7l", etc)
# - LOCAL_OS : operating system (linux, darwin, freebsd, openbsd, netbsd, dragonfly)
# - LOCAL_OSDIR : system-specific directory (linux, mac, freebsd, openbsd, netbsd, dragonflybsd)
# - MAIN_ARCH : target processor architecture family ("x86_64", "arm", etc)
# - MACOS : non-empty on macOS systems
# - NETBSD : non-empty on NetBSD systems
# - OPENBSD : non-empty on OpenBSD systems
# - SO_SUFFIX : shared libraries suffix (".so", ".dylib")
# - SYSPREFIX : installation prefix; if not already set on input, it is defined
#   as /usr on Linux and HomeBrew root on macOS.
# - UDEVDIR : directory for system-defined udev rules
# - USELIB64 : non-empty on x86_64 distro using /usr/lib64 instead of /usr/lib
# - USRLIBDIR : library directory for target; $(SYSPREFIX)/lib64 for Linux
#   x86_64 where /usr/lib64/libc.so* exists and $(SYSPREFIX)/lib elsewhere.
#-----------------------------------------------------------------------------

# Enforce English locale by default in all commands for predictible output.

export LANGUAGE = en_US.UTF-8
export LC_ALL = en_US.UTF-8
export LANG = en_US.UTF-8

# Operating system, architecture, host name.

ifeq ($(LOCAL_OS),)
    export LOCAL_OS    := $(shell uname -s | tr A-Z a-z)
    export LOCAL_OSDIR := $(subst darwin,mac,$(subst dragonfly,dragonflybsd,$(LOCAL_OS)))
    export LOCAL_ARCH  := $(shell uname -m)
    export HOSTNAME    := $(firstword $(subst ., ,$(shell hostname 2>/dev/null)))
endif

LINUX        = $(if $(findstring linux,$(LOCAL_OS)),true)
MACOS        = $(if $(findstring darwin,$(LOCAL_OS)),true)
FREEBSD      = $(if $(findstring freebsd,$(LOCAL_OS)),true)
NETBSD       = $(if $(findstring netbsd,$(LOCAL_OS)),true)
OPENBSD      = $(if $(findstring openbsd,$(LOCAL_OS)),true)
DRAGONFLYBSD = $(if $(findstring dragonfly,$(LOCAL_OS)),true)
BSD          = $(if $(FREEBSD)$(NETBSD)$(OPENBSD)$(DRAGONFLYBSD),true)

ifneq ($(M32),)
    # 32-bit cross-comppilation.
    override MAIN_ARCH = i386
else
    MAIN_ARCH = $(patsubst arm%,arm,$(patsubst i%86,i386,$(LOCAL_ARCH)))
endif

# Shared object files suffix.

SO_SUFFIX = $(if $(MACOS),.dylib,.so)

# Logical CPU and physical core count.

ifeq ($(CPU_COUNT),)
    ifneq ($(MACOS),)
        export CPU_COUNT  := $(shell sysctl -n hw.logicalcpu 2>/dev/null)
        export CPU_COUNT  := $(if $(CPU_COUNT),$(CPU_COUNT),1)
        export CORE_COUNT := $(shell sysctl -n hw.physicalcpu 2>/dev/null)
        export CORE_COUNT := $(if $(CORE_COUNT),$(CORE_COUNT),$(CPU_COUNT))
    else ifneq ($(FREEBSD),)
        export CPU_COUNT  := $(shell sysctl -n kern.smp.cpus 2>/dev/null)
        export CPU_COUNT  := $(if $(CPU_COUNT),$(CPU_COUNT),1)
        export CORE_COUNT := $(shell sysctl -n kern.smp.cores 2>/dev/null)
        export CORE_COUNT := $(if $(CORE_COUNT),$(CORE_COUNT),$(CPU_COUNT))
    else ifneq ($(OPENBSD)$(NETBSD)$(DRAGONFLYBSD),)
        export CPU_COUNT  := $(shell sysctl -n hw.ncpu 2>/dev/null)
        export CPU_COUNT  := $(if $(CPU_COUNT),$(CPU_COUNT),1)
        export CORE_COUNT := $(CPU_COUNT)
    else
        export CPU_COUNT  := $(shell nproc 2>/dev/null)
        export CPU_COUNT  := $(if $(CPU_COUNT),$(CPU_COUNT),1)
        export CORE_COUNT := $(lastword $(shell $(FGREP) -m1 'cpu cores' /proc/cpuinfo 2>/dev/null))
        export CORE_COUNT := $(if $(CORE_COUNT),$(CORE_COUNT),$(CPU_COUNT))
    endif
endif

# On macOS, Homebrew development packages are installed in /usr/local (Intel) or /opt/homebrew (Arm).
# On Linux, when Homebrew is forced, use it as alternative development root.
# On FreeBSD, OpenBSD, DragonFlyBSD, the packages are installed in /usr/local.
# On NetBSD, the packages are installed in /usr/pkg.

ifneq ($(MACOS)$(LINUXBREW)$(FREEBSD)$(OPENBSD)$(DRAGONFLYBSD),)
    $(if $(LINUXBREW),$(if $(HOMEBREW_PREFIX),,$(error LINUXBREW is defined but not HOMEBREW_PREFIX)))
    ALTDEVROOT := $(if $(HOMEBREW_PREFIX),$(HOMEBREW_PREFIX),$(if $(wildcard /opt/homebrew/bin),/opt/homebrew,/usr/local))
else ifneq ($(NETBSD),)
    ALTDEVROOT := /usr/pkg
else
    ALTDEVROOT := /usr
endif

# Locate system directories.

USELIB64  := $(if $(findstring linux,$(LOCAL_OS)),$(if $(findstring 64,$(MAIN_ARCH)),$(if $(LINUXBREW),,$(if $(wildcard /usr/lib64/libc.so*),true))))
SYSPREFIX ?= $(if $(MACOS)$(LINUXBREW)$(FREEBSD)$(OPENBSD)$(NETBSD)$(DRAGONFLYBSD),$(ALTDEVROOT),/usr)
USRLIBDIR ?= $(if $(USELIB64),$(SYSPREFIX)/lib64,$(SYSPREFIX)/lib)
ETCDIR    ?= $(if $(subst /usr,,$(SYSPREFIX)),$(SYSPREFIX)/etc,/etc)
UDEVDIR   ?= $(if $(subst /usr,,$(SYSPREFIX)),$(SYSPREFIX)/etc/udev/rules.d,/lib/udev/rules.d)

# Locate bash completions facility.
BASHCOMP_DIR ?= $(if $(MACOS)$(LINUXBREW)$(FREEBSD)$(OPENBSD)$(DRAGONFLYBSD),$(ALTDEVROOT)/etc/bash_completion.d,$(SYSPREFIX)/share/bash-completion/completions)
BASHCOMP_AUTO = $(if $(MACOS)$(LINUXBREW)$(FREEBSD)$(OPENBSD)$(NETBSD)$(DRAGONFLYBSD),,true)

#-----------------------------------------------------------------------------
# Output directories for final binaries and objects.
# - BINDIR : root directory where all binaries are built; named
#   bin/release|debug-arch-hostname in the top-level directory.
# - BINDIR_SUFFIX : fixed suffix to add to generated BINDIR directory name;
#   ignored if BINDIR is defined as input variable
# - OBJDIR : subdirectory of BINDIR where object files are stored; named
#   objs-$(CURDIR) for each source directory where make is executed.
#-----------------------------------------------------------------------------

ifneq ($(GCOV),)
    BINDIR_SUFFIX += -gcov
endif
ifneq ($(GPROF),)
    BINDIR_SUFFIX += -gprof
endif
ifneq ($(LLVM),)
    BINDIR_SUFFIX += -clang
endif
ifneq ($(STATIC),)
    BINDIR_SUFFIX += -static
endif

ifneq ($(BINDIR),)
    # BINDIR is specified in input, transform it into an absolute path for recursion.
    INBINDIR := $(BINDIR)
    override BINDIR := $(abspath $(INBINDIR))
    MAKEOVERRIDES := $(subst BINDIR=$(INBINDIR),BINDIR=$(BINDIR),$(MAKEOVERRIDES))
else ifneq ($(DEBUG),)
    BINDIR = $(ROOTDIR)/bin/debug-$(MAIN_ARCH)$(if $(HOSTNAME),-$(HOSTNAME),)$(BINDIR_SUFFIX)
else
    BINDIR = $(ROOTDIR)/bin/release-$(MAIN_ARCH)$(if $(HOSTNAME),-$(HOSTNAME),)$(BINDIR_SUFFIX)
endif

OBJDIR = $(BINDIR)/objs-$(notdir $(CURDIR))

#-----------------------------------------------------------------------------
# Compilation flags and other build commands.
# - CXX : C++ compiler command
# - GCC : gcc executable, even when CXX is not gcc
# - LD : linker command
# - AR : library archive command
# - CXXFLAGS : complete C++ compilation flags
# - LDFLAGS : complete C++ compilation flags
# - SOFLAGS : complete options when creating shared object libraries
# - ARFLAGS : complete library archive command flags
# - GCC_VERSION : full gcc version
# - GCC_MAJOR : gcc major version
# - LLVM_VERSION : full clang version
# - LLVM_MAJOR : clang major version
# - USE_GCC : the C++ compiler is gcc
# - USE_LLVM : the C++ compiler is clang (LLVM)
# Partial C++ compilation flags (included in CXXFLAGS):
# - CXXFLAGS_CROSS : for cross-compilation
# - CXXFLAGS_DEBUG : for debug mode
# - CXXFLAGS_FPIC : for position-independent code
# - CXXFLAGS_FULLSPEED : for full speed code optimization
# - CXXFLAGS_GCOV : for code coverage using gcov
# - CXXFLAGS_GPROF : for code profiling using gprof
# - CXXFLAGS_INCLUDES : preprocessing options (includes and macros)
# - CXXFLAGS_M32 : for 32-bit cross-compilation
# - CXXFLAGS_OPTIMIZE : for standard code optimization
# - CXXFLAGS_OPTSIZE : for code size optimization
# - CXXFLAGS_PTHREAD : pthread options
# - CXXFLAGS_SECURITY : security-oriented options
# - CXXFLAGS_STANDARD : specifies the standard level of C++ language
# - CXXFLAGS_TARGET : specify target CPU
# - CXXFLAGS_WARNINGS : specify warnings
# Partial linker flags (included in LDFLAGS):
# - LDFLAGS_CROSS : for cross-compilation
# - LDFLAGS_DEBUG : for debug mode
# - LDFLAGS_GCOV : for code coverage using gcov
# - LDFLAGS_GPROF : for code profiling using gprof
# - LDFLAGS_M32 : for 32-bit cross-compilation
# - LDFLAGS_PTHREAD : pthread options
# - LDLIBS : specify external libraries
#-----------------------------------------------------------------------------

# Use $(CXX) for compilation. Use $(GCC) to explicitly reference GCC.

GCC ?= gcc

# Compilation target options.

CXXFLAGS_M32 =
LDFLAGS_M32 =

ifneq ($(M32),)
    # Define compilation flags for 32-bit cross-compilation.
    override CXXFLAGS_TARGET = -march=i686
    override CXXFLAGS_M32 = -m32
    override LDFLAGS_M32 = -m32
else
    CXXFLAGS_TARGET =
endif

# Cross-compilation support.
# If $(CROSS) is defined (any non-empty value), perform a cross-compilation build.
# Cross-build tools:
#   $(CROSS_TARGET) is the target name of the cross-compilation (e.g. arm-unknown-linux-gnueabi).
#   If $(CROSS_TARGET) is undefined but $(CROSS) is defined, we locate the first GCC executable
#   with path $(CROSS_PREFIX)/TARGET/bin/TARGET-gcc.
# Native installation of TSDuck:
#   The last phase of the build uses tsxml, a TSDuck tool which is built in earlier phases.
#   Using cross-compilation, this tsxml cannot run on the native system. A native installation
#   of TSDuck is required. If not installed in the $PATH, a native TSDuck must have been compiled
#   somewhere and NATIVEBINDIR must point to the BINDIR of the native build.

ifneq ($(NATIVEBINDIR),)
    # NATIVEBINDIR is specified in input, transform it into an absolute path for recursion.
    INNATIVEBINDIR := $(NATIVEBINDIR)
    override NATIVEBINDIR := $(abspath $(INNATIVEBINDIR))
    MAKEOVERRIDES := $(subst NATIVEBINDIR=$(INNATIVEBINDIR),NATIVEBINDIR=$(NATIVEBINDIR),$(MAKEOVERRIDES))
endif

ifeq ($(CROSS)$(CROSS_TARGET),)
    CXXFLAGS_CROSS =
    LDFLAGS_CROSS =
    NATIVEBINDIR ?= $(BINDIR)
    TSXML = LD_LIBRARY_PATH="$(NATIVEBINDIR):$(LD_LIBRARY_PATH)" \
        $(if $(MACOS),DYLD_LIBRARY_PATH="$(NATIVEBINDIR):$(DYLD_LIBRARY_PATH)") \
        $(NATIVEBINDIR)/tsxml
else
    CROSS ?= true
    # Cross-compilation tools are in /usr/local by default.
    CROSS_PREFIX ?= /usr/local
    # If cross target undefined, find the first one.
    # We look for a file pattern: PREFIX/TARGET/bin/TARGET-gcc
    CROSS_TARGET := $(if $(CROSS_TARGET),$(CROSS_TARGET),$(shell \
        ls $(CROSS_PREFIX)/*/bin/*-gcc 2>/dev/null | \
        $(GREP) '$(CROSS_PREFIX)//*\([^/]*\)/bin/\1-gcc' | \
        $(SED) -e 's|^$(CROSS_PREFIX)//*||' -e 's|/.*$$||' | \
        head -1))
    $(if $(CROSS_TARGET),,$(error CROSS is defined but no cross-compilation tool-chain was found))
    # Adjust target. Use "arm" as main target if CROSS_TARGET starts with "arm".
    override MAIN_ARCH := $(if $(findstring @arm,@$(CROSS_TARGET)),arm,$(CROSS_TARGET))
    override CXXFLAGS_TARGET :=
    override GCC_VERSION_DONE :=
    # Redirect build tools.
    F_SEARCH_CROSS = $(firstword $(wildcard \
        $(CROSS_PREFIX)/$(CROSS_TARGET)/bin/$(CROSS_TARGET)-$(1) \
        $(CROSS_PREFIX)/$(CROSS_TARGET)/bin/$(1) \
        $(CROSS_PREFIX)/bin/$(CROSS_TARGET)-$(1)))
    CXX := $(call F_SEARCH_CROSS,g++)
    GCC := $(call F_SEARCH_CROSS,gcc)
    LD  := $(call F_SEARCH_CROSS,ld)
    $(if $(CXX),,$(error cross g++ not found for $(CROSS_TARGET)))
    $(if $(GCC),,$(error cross gcc not found for $(CROSS_TARGET)))
    $(if $(LD),,$(error cross ld not found for $(CROSS_TARGET)))
    # Add options. The layout can be different, so use them all.
    CXXFLAGS_CROSS = -I$(CROSS_PREFIX)/$(CROSS_TARGET)/include \
                     -I$(CROSS_PREFIX)/$(CROSS_TARGET)/$(CROSS_TARGET)/include \
                     -I$(CROSS_PREFIX)/$(CROSS_TARGET)/$(CROSS_TARGET)/libc/include
    LDFLAGS_CROSS  = -L$(CROSS_PREFIX)/$(CROSS_TARGET)/lib \
                     -L$(CROSS_PREFIX)/$(CROSS_TARGET)/$(CROSS_TARGET)/lib \
                     -L$(CROSS_PREFIX)/$(CROSS_TARGET)/$(CROSS_TARGET)/libc/lib
    TSXML ?= $(call F_SEARCH,tsxml)
    ifeq ($(TSXML),)
        ifneq ($(wildcard $(NATIVEBINDIR)/tsxml),)
            TSXML = LD_LIBRARY_PATH="$(NATIVEBINDIR):$(LD_LIBRARY_PATH)" \
                $(if $(MACOS),DYLD_LIBRARY_PATH="$(NATIVEBINDIR):$(DYLD_LIBRARY_PATH)") \
                $(NATIVEBINDIR)/tsxml
        endif
    endif
    $(if $(TSXML),,$(error no native TSDuck found for cross-compilation, check NATIVEBINDIR))
endif

# Get current compiler version, usually in form x.y.z

ifeq ($(GCC_VERSION_DONE),)
    export GCC_VERSION_DONE := true
    export GCC_VERSION := $(shell $(GCC) -dumpversion 2>/dev/null)
    export GCC_MAJOR := $(firstword $(subst .,$(SPACE),$(GCC_VERSION)))
    export LLVM_VERSION := $(shell clang -dumpversion 2>/dev/null)
    export LLVM_MAJOR := $(firstword $(subst .,$(SPACE),$(LLVM_VERSION)))
    export USE_GCC := $(if $(shell $(CXX) --version 2>/dev/null | $(GREP) -i -e gcc -e g++),true)
    export USE_LLVM := $(if $(shell $(CXX) --version 2>/dev/null | $(GREP) -i clang),true)
    export LLVM_10 := $(call F_GE,$(LLVM_MAJOR),10)
    export LLVM_12 := $(call F_GE,$(LLVM_MAJOR),12)
    export LLVM_15 := $(call F_GE,$(LLVM_MAJOR),15)
    export GCC_5 := $(call F_GE,$(GCC_MAJOR),5)
    export GCC_6 := $(call F_GE,$(GCC_MAJOR),6)
    export GCC_9 := $(call F_GE,$(GCC_MAJOR),9)
endif

# Forced usage of LLVM (clang).
# FreeBSD and OpenBSD have switched to clang by default.

ifneq ($(LLVM)$(FREEBSD)$(OPENBSD),)
    export USE_GCC :=
    export USE_LLVM := true
    CXX = clang++
    # On some systems (e.g. Ubuntu 22.04), clang-14 is the default but is
    # incompatible with the installed gcc header files in C++20 mode.
    # If clang-15 is installed, use it instead of clang-14.
    ifeq ($(LLVM_MAJOR),14)
        CXX := $(call F_COMMAND,clang++-15,clang++)
    endif
endif

# Compilation flags for various types of optimization.
# Example to specify that some selected modules should be compiled for full speed:
# $(OBJDIR)/fast1.o $(OBJDIR)/fast2.o: CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)

CXXFLAGS_OPTIMIZE ?= -O2 -fno-strict-aliasing
CXXFLAGS_FULLSPEED = -O3 -fno-strict-aliasing -funroll-loops -fomit-frame-pointer
CXXFLAGS_OPTSIZE   = -Os -fno-strict-aliasing

ifneq ($(ALTDEVROOT),/usr)
    CXXFLAGS_INCLUDES += -I$(ALTDEVROOT)/include
    LDLIBS := -L$(ALTDEVROOT)/lib $(LDLIBS)
endif

# Always use maximal or even paranoid warning mode.
# With clang, the option -Weverything turns everything on. There is no such option with GCC.

CXXFLAGS_WARNINGS = -Werror
ifneq ($(USE_LLVM),)
    CXXFLAGS_WARNINGS += -Weverything -Wno-c++98-compat-pedantic
    ifneq ($(MACOS),)
        # On macOS, it is normal to include from /usr/local/include since some libraries come from Homebrew.
        # Starting with clang 12, this generates a warning we need to disable. However, this disable option
        # generates an error with previous versions. And we cannot disable this warning inside the code since
        # this is a command-line-level error. So, we must check the version here...
        CXXFLAGS_WARNINGS += $(if $(LLVM_12),-Wno-poison-system-directories)
    endif
else
    CXXFLAGS_WARNINGS += \
        -Wall -Wextra -Wformat-nonliteral -Wformat-security -Wswitch-default -Wuninitialized \
        -Wfloat-equal -Wundef -Wpointer-arith -Woverloaded-virtual -Wctor-dtor-privacy \
        -Wnon-virtual-dtor -Wsign-promo -Wzero-as-null-pointer-constant -Wstrict-null-sentinel \
        $(if $(GCC_5),-Wshadow -Wpedantic -Weffc++) \
        $(if $(GCC_6),-Wsuggest-override) \
        $(if $(findstring arm,$(MAIN_ARCH)),-Wno-psabi)
endif

# Language levels. Minimum level is C++17.

CXXFLAGS_STANDARD ?= -std=c++17

# Compilation flags for security.

CXXFLAGS_SECURITY = -fstack-protector-all

# Compilation flags for position-independent code.

CXXFLAGS_FPIC = -fPIC

# Compilation flags in debug mode.

ifneq ($(DEBUG),)
    CXXFLAGS_DEBUG = -g -DDEBUG=1
    LDFLAGS_DEBUG =
else
    CXXFLAGS_DEBUG = $(CXXFLAGS_OPTIMIZE)
    LDFLAGS_DEBUG =
endif

# Compilation flags for code sanitizing using asan with default optimization.

ifneq ($(ASAN),)
    CXXFLAGS_ASAN = -fsanitize=address -g3
    LDFLAGS_ASAN = -fsanitize=address -g3
else
    CXXFLAGS_ASAN =
    LDFLAGS_ASAN =
endif

# Compilation flags for code coverage using gcov.

ifneq ($(GCOV),)
    CXXFLAGS_GCOV = --coverage -DGCOV=1
    LDFLAGS_GCOV = --coverage
else
    CXXFLAGS_GCOV =
    LDFLAGS_GCOV =
endif

# Compilation flags for code profiling using gprof.

ifneq ($(GPROF),)
    CXXFLAGS_GPROF = -g -pg -DGPROF=1
    LDFLAGS_GPROF = -g -pg
else
    CXXFLAGS_GPROF =
    LDFLAGS_GPROF =
endif

# Compilation flags for posix threads.

CXXFLAGS_PTHREAD = -pthread
LDFLAGS_PTHREAD = $(if $(MACOS),,-pthread)

# External libraries

LDLIBS += -lstdc++ -lpthread $(if $(MACOS)$(OPENBSD),,-lrt) -lm

# Global compilation flags.
# Additional flags can be passed on the "make" command line using xxFLAGS_EXTRA.

CXXFLAGS = $(CXXFLAGS_DEBUG) $(CXXFLAGS_M32) $(CXXFLAGS_ASAN) $(CXXFLAGS_GCOV) $(CXXFLAGS_GPROF) $(CXXFLAGS_WARNINGS) \
           $(CXXFLAGS_SECURITY) $(CXXFLAGS_INCLUDES) $(CXXFLAGS_TARGET) $(CXXFLAGS_FPIC) $(CXXFLAGS_STANDARD) \
           $(CXXFLAGS_CROSS) $(CXXFLAGS_PTHREAD) $(CXXFLAGS_EXTRA)
LDFLAGS  = $(LDFLAGS_DEBUG) $(LDFLAGS_M32) $(LDFLAGS_ASAN) $(LDFLAGS_GCOV) $(LDFLAGS_GPROF) $(CXXFLAGS_TARGET) \
           $(LDFLAGS_CROSS) $(LDFLAGS_PTHREAD) $(LDFLAGS_EXTRA)
ARFLAGS  = rc$(if $(MACOS),,U) $(ARFLAGS_EXTRA)

# $(SOFLAGS) specifies options when creating shared objects (.so or .dylib files).

ifneq ($(LINUX),)
    # Build flags on Linux
    LDFLAGS += -Wl,-rpath,'$$ORIGIN',-z,noexecstack
    SOFLAGS = -Wl,-soname=$(notdir $@),-z,noexecstack
else ifneq ($(FREEBSD),)
    # Build flags on FreeBSD
    LDFLAGS += -Wl,-rpath,'$$ORIGIN',-z,noexecstack
    SOFLAGS = -Wl,-soname=$(notdir $@),-z,noexecstack
else ifneq ($(OPENBSD),)
    # Build flags on OpenBSD
    LDFLAGS += -Wl,-z,origin,-rpath,'$$ORIGIN',-z,noexecstack
    SOFLAGS = -Wl,-soname=$(notdir $@),-z,noexecstack
else ifneq ($(NETBSD),)
    # Build flags on NetBSD
    LDFLAGS += -Wl,-rpath,'$$ORIGIN',-rpath,$(ALTDEVROOT)/lib,-z,noexecstack
    SOFLAGS = -Wl,-soname=$(notdir $@),-z,noexecstack
else ifneq ($(DRAGONFLYBSD),)
    # Build flags on DragonFlyBSD
    LDFLAGS += -Wl,-z,origin,-rpath,'$$ORIGIN',-z,noexecstack
    SOFLAGS = -Wl,-soname=$(notdir $@),-z,noexecstack
else ifneq ($(MACOS),)
    # Build flags on macOS
    LDFLAGS += -Wl,-rpath,@executable_path -Wl,-rpath,@executable_path/../lib -Xlinker $(if $(LLVM_15),-no_warn_duplicate_libraries)
    SOFLAGS = -install_name '@rpath/$(notdir $@)'
endif

# Compilation rules

$(OBJDIR)/%.o: %.cpp
	@echo '  [CXX] $<'; \
	mkdir -p $(OBJDIR); \
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
$(BINDIR)/%: $(OBJDIR)/%.o
	@echo '  [LD] $@'; \
	$(CXX) $(LDFLAGS) $^ $(LDLIBS_EXTRA) $(LDLIBS) -o $@
$(BINDIR)/%$(SO_SUFFIX): $(OBJDIR)/%.o
	@echo '  [LD] $@'; \
	$(CXX) $(SOFLAGS) $(LDFLAGS) $^ $(LDLIBS_EXTRA) $(LDLIBS) -shared -o $@

#-----------------------------------------------------------------------------
# Static linking.
# Not meaningful everywhere:
# - Static linking with system libraries is not supported on macOS.
# - On Linux, all used libraries must be installed. This is not supported
#   on all distros. On Fedora, you may install "glibc-static libstdc++-static"
#   but there is no static package for curl and pcsclite.
#-----------------------------------------------------------------------------

.PHONY: static
static:
	+@$(MAKE) STATIC=true

ifeq ($(STATIC),)
    # Dynamic (default) link
    LDLIBS := $(if $(OPENBSD)$(NETBSD),,-ldl) $(LDLIBS)
else ifneq ($(MACOS),)
    $(error static linking is not supported on macOS)
else
    NOCURL     = true
    NOPCSC     = true
    NODTAPI    = true
    NOVATEK    = true
    NOSRT      = true
    NORIST     = true
    NOEDITLINE = true
    NOTEST     = true
    CXXFLAGS_INCLUDES += -DTSDUCK_STATIC=1
endif

#-----------------------------------------------------------------------------
# Exclude features which are not supported in the current environment
#-----------------------------------------------------------------------------

ifneq ($(CROSS)$(CROSS_TARGET),)
    # Some libraries are bypassed in cross-compilation.
    NOCURL     = true
    NOPCSC     = true
    NODTAPI    = true
    NOVATEK    = true
    NOSRT      = true
    NORIST     = true
    NOEDITLINE = true
endif

ifeq ($(NOSRT),)
    # SRT not disabled, check if libsrt is available.
    NOSRT := $(if $(wildcard /usr/include/srt/*.h $(ALTDEVROOT)/include/srt/*.h),,true)
endif

ifeq ($(NORIST),)
    # RIST not disabled, check if librist is available.
    NORIST := $(if $(wildcard /usr/include/librist/*.h $(ALTDEVROOT)/include/librist/*.h),,true)
endif

ifeq ($(NOCURL),)
    # curl not disabled, check if available.
    NOCURL := $(if $(call F_SEARCH,curl-config),,true)
endif

ifneq ($(MACOS)$(BSD),)
    # Dektec and HiDes devices are not supported on macOS and BSD systems (no driver).
    NODTAPI = true
    NOHIDES = true
endif

ifneq ($(BSD),)
    # Vatek library has not yet been validated on BSD systems (only depends on libusb).
    NOVATEK = true
endif

ifneq ($(NODEKTEC),)
    # NODEKTEC is an alternative name for NODTAPI
    NODTAPI = true
endif

ifeq ($(NODTAPI)$(NODTAPI_CHECKED),)
    # Dektec DTAPI not disabled, check if available.
    NODTAPI := $(if $(shell $(SCRIPTSDIR)/dtapi-config.sh --support),,true)
    export NODTAPI_CHECKED := true
endif

ifeq ($(NOPCSC)$(MACOS),)
    # PCSC not disabled and not on macOS, check if available. On macOS, it is always available.
    NOPCSC := $(if $(wildcard /usr/include/PCSC/*.h $(ALTDEVROOT)/include/PCSC/*.h),,true)
endif

#-----------------------------------------------------------------------------
# Other compilation optional features
#-----------------------------------------------------------------------------

# These symbols from the make command line drive the bitrate representation.

ifneq ($(BITRATE_FRACTION),)
    CXXFLAGS_INCLUDES += -DTS_BITRATE_FRACTION=1
else ifneq ($(BITRATE_INTEGER),)
    CXXFLAGS_INCLUDES += -DTS_BITRATE_INTEGER=1
else ifneq ($(BITRATE_FLOAT),)
    CXXFLAGS_INCLUDES += -DTS_BITRATE_FLOAT=1
else ifneq ($(BITRATE_FIXED),)
    CXXFLAGS_INCLUDES += -DTS_BITRATE_FIXED=1
else ifneq ($(BITRATE_DECIMALS),)
    CXXFLAGS_INCLUDES += -DTS_BITRATE_DECIMALS=$(BITRATE_DECIMALS)
endif

# The following definitions are used in applications and in the library.

ifneq ($(NOPCSC),)
    CXXFLAGS_INCLUDES += -DTS_NO_PCSC=1
    LDLIBS_PCSC =
else ifneq ($(MACOS),)
    # On macOS, use PCSC.framework
    LDLIBS_PCSC = -framework PCSC
else ifneq ($(LINUXBREW)$(BSD),)
    # PCSC on Linuxbrew and all BSD systems.
    CXXFLAGS_INCLUDES += -I$(ALTDEVROOT)/include/PCSC
    LDLIBS_PCSC = -lpcsclite
else
    # PCSC on Linux
    CXXFLAGS_INCLUDES += -I/usr/include/PCSC
    LDLIBS_PCSC = -lpcsclite
endif

ifneq ($(NOGITHUB),)
    CXXFLAGS_INCLUDES += -DTS_NO_GITHUB=1
endif

ifneq ($(ASSERTIONS),)
    CXXFLAGS_INCLUDES += -DTS_KEEP_ASSERTIONS=1
endif

ifneq ($(NOHWACCEL),)
    CXXFLAGS_INCLUDES += -DTS_NO_ARM_CRC32_INSTRUCTIONS
    CXXFLAGS_INCLUDES += -DTS_NO_ARM_AES_INSTRUCTIONS
    CXXFLAGS_INCLUDES += -DTS_NO_ARM_SHA1_INSTRUCTIONS
    CXXFLAGS_INCLUDES += -DTS_NO_ARM_SHA256_INSTRUCTIONS
    CXXFLAGS_INCLUDES += -DTS_NO_ARM_SHA512_INSTRUCTIONS
endif

# These variables are used when building the TSDuck library, not in the
# applications. Note, however, that LIBTSDUCK_LDLIBS is still necessary
# when linking applications against the TSDuck static library.

LIBTSDUCK_CXXFLAGS_INCLUDES +=
LIBTSDUCK_LDLIBS += $(LDLIBS_PCSC) $(if $(FREEBSD),-lprocstat) $(if $(OPENBSD)$(NETBSD)$(DRAGONFLYBSD),-lkvm) $(if $(LINUX),-latomic)

ifneq ($(NODTAPI),)
    LIBTSDUCK_CXXFLAGS_INCLUDES += -DTS_NO_DTAPI=1
endif

ifneq ($(NOHIDES),)
    LIBTSDUCK_CXXFLAGS_INCLUDES += -DTS_NO_HIDES=1
endif

ifneq ($(NOVATEK),)
    LIBTSDUCK_CXXFLAGS_INCLUDES += -DTS_NO_VATEK=1
endif

ifneq ($(NOEDITLINE),)
    LIBTSDUCK_CXXFLAGS_INCLUDES += -DTS_NO_EDITLINE=1
else
    LIBTSDUCK_LDLIBS += -ledit
endif

ifneq ($(NOCURL),)
    LIBTSDUCK_CXXFLAGS_INCLUDES += -DTS_NO_CURL=1
else
    ifeq ($(CURL_DONE),)
        export CXXFLAGS_CURL := $(shell curl-config --cflags)
	export LDLIBS_CURL := $(shell curl-config --libs)
        export CURL_DONE := true
    endif
    LIBTSDUCK_CXXFLAGS_INCLUDES += $(CXXFLAGS_CURL)
    LIBTSDUCK_LDLIBS += $(LDLIBS_CURL)
endif

ifneq ($(NOSRT),)
    LIBTSDUCK_CXXFLAGS_INCLUDES += -DTS_NO_SRT=1
else
    LIBTSDUCK_LDLIBS += -lsrt
endif

ifneq ($(NORIST),)
    LIBTSDUCK_CXXFLAGS_INCLUDES += -DTS_NO_RIST=1
else
    LIBTSDUCK_LDLIBS += -lrist
endif

#-----------------------------------------------------------------------------
# List of source directories, tools, plugins, etc
#-----------------------------------------------------------------------------

# List of libtsduck directories containing private and public headers.

ifeq ($(PUBLIC_INCLUDES),)
    export OTHER_OS := $(filter-out $(LOCAL_OSDIR),linux mac freebsd netbsd openbsd dragonflybsd windows)
    ifeq ($(BSD),)
        export OTHER_OS := $(OTHER_OS) bsd
    endif
    export ALL_INCLUDES := $(filter-out $(addprefix %/,$(OTHER_OS)),$(patsubst %/,%,$(sort $(dir $(wildcard $(LIBTSDUCKDIR)/*.h $(LIBTSDUCKDIR)/*/*.h $(LIBTSDUCKDIR)/*/*/*.h $(LIBTSDUCKDIR)/*/*/*/*.h)))))
    export PRIVATE_INCLUDES := $(filter %/private,$(ALL_INCLUDES))
    export PUBLIC_INCLUDES := $(filter-out %/private,$(ALL_INCLUDES))
endif
CXXFLAGS_INCLUDES += $(addprefix -I,$(PUBLIC_INCLUDES))

# List of plugins and tools to build.

ifeq ($(TSPLUGINS),)
    # All plugins and tools, one source file per plugin or executable.
    TSPLUGINS := $(sort $(notdir $(basename $(wildcard $(TSPLUGINSDIR)/tsplugin_*.cpp))))
    TSTOOLS := $(sort $(notdir $(basename $(wildcard $(TSTOOLSDIR)/ts*.cpp))))
    # Obsolete plugins, were in separate shared libraries, now in libtsduck.so.
    # Maintenance: also update pkg/nsis/tsduck.nsi (Windows).
    NO_TSPLUGINS := $(addprefix tsplugin_,dektec drop file fork hls http ip null psi rist skip srt table teletext)
    # Build a list of tools and plugins to not build or deinstall from the system tree.
    NO_TSTOOLS :=
    ifneq ($(NODTAPI),)
        NO_TSTOOLS += tsdektec
    endif
    ifneq ($(NOHIDES),)
        NO_TSPLUGINS += tsplugin_hides
        NO_TSTOOLS += tshides
    endif
    ifneq ($(NOVATEK),)
        NO_TSTOOLS += tsvatek
    endif
    ifneq ($(NOPCSC),)
        NO_TSTOOLS += tssmartcard
    endif
    TSTOOLS := $(filter-out $(NO_TSTOOLS),$(TSTOOLS))
    TSPLUGINS := $(filter-out $(NO_TSPLUGINS),$(TSPLUGINS))
    export TSPLUGINS TSTOOLS NO_TSTOOLS NO_TSPLUGINS
endif

#-----------------------------------------------------------------------------
# Dependency (.dep) files: For each xxx.cpp source file, there is one
# xxx.dep file in the same directory as xxx.o. The file xxx.dep lists
# all .h dependencies for xxx.cpp.
#
# When make is invoked, the .dep files are automatically updated. This is
# required for most targets, those which build files. Some targets are
# informational or do not need to build object files. In this case,
# rebuilding the .dep files is useless but harmless. However, in some
# cases such as the standard "clean" targets, the .dep files should
# not be rebuilt. In fact, they are deleted by the target. In these
# cases, the "clean" targets must not implicitly rebuild the .dep
# files. The standard "clean" targets are automatically added in NODEPS,
# there is no need to set them. However, if a makefile defines some
# non-standard "clean" targets, it should set them in NODEPS before
# inclusion of this make file.
#
# If $(DONT_BUILD_DEPS) is defined, do not attempt to build the header
# dependency files. Useful when the source file depend on dynamically
# generated header files (initially non-existent).
#
# $(NODEPS) lists the make targets in the parent makefile which shall
# not enforce the generation of header dependency files. This is a bit
# complicated and, most of the time, there is no need to set this variable.
#-----------------------------------------------------------------------------

$(OBJDIR)/%.dep: %.cpp
	@echo '  [DEP] $<'; \
	mkdir -p $(OBJDIR); \
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS_INCLUDES) $(CXXFLAGS_STANDARD) $(CXXFLAGS_NO_WARNINGS) -MT $(OBJDIR)/$*.o -MT $@ $< >$@ || \
	rm -f $@

NODEPS += clean cmacros coverity cppcheck cppcheck-xml cxxmacros distclean doxygen \
          flawfinder cloc listvars scan-build unixify

ifeq ($(DONT_BUILD_DEPS),)
    ifneq ($(MAKECMDGOALS),)
        DONT_BUILD_DEPS := $(if $(filter-out $(NODEPS),$(MAKECMDGOALS)),,true)
    endif
endif

ifeq ($(DONT_BUILD_DEPS),)
    -include $(addprefix $(OBJDIR)/,$(addsuffix .dep,$(notdir $(basename $(CURCPP) $(filter-out tsduck.cpp,$(wildcard *.cpp))))))
endif

#-----------------------------------------------------------------------------
# Cleanup Windows oddities in source files.
# Many IDE's indent with tabs, and tabs are 4 chars wide.
# Tabs shall not be expanded in Makefiles.
#-----------------------------------------------------------------------------

.PHONY: unixify
unixify:
	for f in $$(find . -name \*.c -o -name \*.cpp -o -name \*.h -o -name \*.sh -o -name \*.dox -o -name \*.md -o -name \*.xml -o -name \*.txt); do \
	  expand -t 4 $$f >$$f.tmp; \
	  $(CHMOD) --reference=$$f $$f.tmp; \
	  mv -f $$f.tmp $$f; \
	done
	for f in $$(find . -name \*.c -o -name \*.cpp -o -name \*.h -o -name Makefile\* -o -name \*.sh -o -name \*.dox -o -name \*.md -o -name \*.xml -o -name \*.txt); do \
	  dos2unix $(if $(FREEBSD)$(DRAGONFLYBSD),,-q) $$f; \
	  $(SED) -i -e 's/  *$$//' $$f; \
	done

#-----------------------------------------------------------------------------
# Display make variables for debug purposes.
#-----------------------------------------------------------------------------

.PHONY: listvars
listvars:
	@true
	$(foreach v, \
	  $(sort $(filter-out .% ^% @% _% *% \%% <% +% ?% BASH% LS_COLORS SSH% VTE% XDG% F_%,$(.VARIABLES))), \
	  $(info $(v) = "$($(v))"))
