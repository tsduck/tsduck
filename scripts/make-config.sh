#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script generates the definitions of most variables which are required
#  by GNU Make in the TSDuck project. Originally, these variables were built
#  inside Makefile.inc. However, it became over-complicated and it is now
#  easier to build them in a shell script.
#
#  The parameters of this script are "name=value" to overridde initial values
#  for make variables. By default, they are grabbed from the environment.
#
#  General note: To speed up make recursion, all make variables are exported.
#  Thus, in a sub-make, all variables are already defined. Depending on the
#  variable, the script shall decide to keep the previous value or rebuild it.
#  Typically, if the value is not supposed to change, do not rebuild it if
#  not empty. This saves time if rebuilding the variable forks a command.
#
#-----------------------------------------------------------------------------

# If $MAKECONFIG_DEBUG is not empty, display debug messages on stderr.
if [[ -n $MAKECONFIG_DEBUG ]]; then
    debug() { echo >&2 "Debug: $*"; }
else
    debug() { true; }
fi

# If $MAKECONFIG_TIME is not empty, display execution time of this script on stderr.
if [[ -n $MAKECONFIG_TIME ]]; then
    if [[ -n $DATE ]]; then
        mudate=$DATE
    elif [[ -n $(which gdate 2>/dev/null) ]]; then
        mudate=gdate
    else
        mudate=date
    fi
    musec() { $mudate '+%s%6N'; }
    debug-time() { local m=$(musec); printf >&2 '==== make-config: %4d ms %s\n' $((($m - $START_MUSEC) / 1000)) "${1:-in $(pwd)}"; }
    START_MUSEC=$(musec)
else
    debug-time() { true; }
fi

# Generate an error in the make commands.
error()
{
    echo "\$(error $*)"
    exit
}

# A faster version of basename, without creating a process.
fbasename() {
    local base=${1/%\//}
    base=${base/*\//}
    [[ -n $2 ]] && base=${base/%$2/}
    echo $base
}

# Get the first defined command in a list. Default is last one.
getcmd() {
    local cmd=
    for cmd in "$@"; do
        [[ -n $(which "$cmd" 2>/dev/null) ]] && break
    done
    echo $cmd
}

# Find a command in PATH which can be followed by a version.
# For instance, asciidoctor-pdf is named asciidoctor-pdf33 on OpenBDS.
findcmdvers() {
    # Direct command name works in most cases.
    local name="$1"
    local cmd=$(which "$name" 2>/dev/null)
    if [[ -z $cmd ]]; then
        # Not found, look for versioned names.
        cmd=$(for dir in ${PATH//:/ }; do echo "$dir/$name"*; done | tr ' ' '\n' | grep -v '\*' | grep -e "/$name"'[-\.0-9]*$' | tail -1)
    fi
    [[ -n $cmd ]] && echo "$cmd"
}

# Check if a wildcard designates a valid file. Return non empty string if a file exists.
exist-wildcard () {
    local f=
    for f in $*; do
        if [[ -e $f ]]; then
            echo true
            break
        fi
    done
}

# Get the version of a command, usually in form x.y.z
# Warning: some distros repackage compiler version strings in a weird forms.
# This function extracts the first group which looks like a version number in the first line.
# Ignore groups of parentheses since they sometimes contain a distro version.
# Modifying this function may break on other distros. So, take care.
extract-version() {
    "$1" --version 2>/dev/null |
        head -1 |
        sed -e 's/([^(]*)//g' |
        tr ' ' '\n' |
        grep '^[0-9][0-9]*\.[0-9\.]*$' |
        head -1
}

debug "==== make-config in $(pwd), MAKELEVEL=$MAKELEVEL"

# Read list of variables from config file at project root.
root=$(cd $(dirname ${BASH_SOURCE[0]})/..; pwd)
VARNAMES=' '$(sed -e 's/ *#.*//' -e '/^ *$/d' "$root/CONFIG.txt")' '
VARNAMES=${VARNAMES//$'\n'/ }

# Variables are inherited from the environment (export from previous call), then set with values from the command line.
for opt in "$@"; do
    if [[ $opt != =* && $opt == *=* ]]; then
        name=${opt/=*}
        if [[ $VARNAMES == *' '$name' '* ]]; then
            eval $name='"${opt/#$name=/}"'
            debug "input: $name='${!name}'"
        fi
    fi
done

# Where are we? CURDIR is normally set by make.
[[ -z $CURDIR ]] && CURDIR=$(pwd)

# Check if we are building libtstuck.
[[ $CURDIR == */src/libtsduck || $CURDIR == */src/libtsduck/* ]] && INLIBTSDUCK=1 || INLIBTSDUCK=
debug "INLIBTSDUCK='$INLIBTSDUCK'"

# Check if we need to download external libraries which are not installed at system level.
# For maintenance or cleanup operations, we should not download stuff (we may even want
# to delete them in the case of a cleanup operation).
NOEXTLIBS=1
if [[ $INLIBTSDUCK ]]; then
    if [[ -z ${MAKECMDGOALS// /} ]]; then
        # Default build in libtsduck: use external libraries.
        NOEXTLIBS=
    else
        # Each of these targets need the external libraries:
        for target_prefix in default headers libs listvars install; do
            if [[ ' '$MAKECMDGOALS == *' '${target_prefix}* ]]; then
                # This is a target which needs the libraries.
                NOEXTLIBS=
                break
            fi
        done
    fi
fi
debug "NOEXTLIBS='$NOEXTLIBS'"

#-----------------------------------------------------------------------------
# System configuration.
#-----------------------------------------------------------------------------

# Enforce English locale by default in all commands for predictible output.
LANGUAGE=en_US.UTF-8
LC_ALL=en_US.UTF-8
LANG=en_US.UTF-8

[[ -z $LOCAL_OS ]] && LOCAL_OS=$(uname -s | tr A-Z a-z)
[[ -z $LOCAL_ARCH ]] && LOCAL_ARCH=$(uname -m)
if [[ -n $M32 ]]; then
    # 32-bit cross-compilation
    MAIN_ARCH=i386
elif [[ -z $MAIN_ARCH ]]; then
    MAIN_ARCH=${LOCAL_ARCH/i*86/i386}
    MAIN_ARCH=${MAIN_ARCH/arm*/arm}
fi
[[ -z $HOSTNAME ]] && HOSTNAME=$(hostname 2>/dev/null)
HOSTNAME=${HOSTNAME/.*/}

# Possibly used in scripts we will call.
export LOCAL_OS LOCAL_ARCH

[[ $LOCAL_OS == linux ]] && LINUX=1 || LINUX=
[[ $LOCAL_OS == darwin ]] && MACOS=1 || MACOS=
[[ $LOCAL_OS == freebsd ]] && FREEBSD=1 || FREEBSD=
[[ $LOCAL_OS == netbsd ]] && NETBSD=1 || NETBSD=
[[ $LOCAL_OS == openbsd ]] && OPENBSD=1 || OPENBSD=
[[ $LOCAL_OS == dragonfly ]] && DRAGONFLYBSD=1 || DRAGONFLYBSD=
[[ -n $FREEBSD$NETBSD$OPENBSD$DRAGONFLYBSD ]] && BSD=1 || BSD=

# Generic commands which may need alternatives.
[[ -z $TAR ]] && TAR=$(getcmd gtar tar)
[[ -z $SED ]] && SED=$(getcmd gsed sed)
[[ -z $GREP ]] && GREP=$(getcmd ggrep grep)
[[ -z $FGREP ]] && FGREP=$(getcmd gfgrep fgrep)
[[ -z $CHMOD ]] && CHMOD=$(getcmd gchmod chmod)
[[ -z $DATE ]] && DATE=$(getcmd gdate date)
[[ -z $REALPATH ]] && REALPATH=$(getcmd grealpath realpath)
[[ -z $XDGOPEN ]] && XDGOPEN=$(getcmd xdg-open open)
[[ -z $PYTHON ]] && PYTHON=$(getcmd python3 python)
[[ -z $SUDO && $(id -u) -ne 0 ]] && SUDO=sudo

# Commands which are sometimes installed with a version in the name.
[[ -z $ASCIIDOCTOR ]] && ASCIIDOCTOR=$(findcmdvers asciidoctor)
[[ -z $ASCIIDOCTOR_PDF ]] && ASCIIDOCTOR_PDF=$(findcmdvers asciidoctor-pdf)

# Python 3 is required for build scripts.
[[ -z $PYTHON_VERSION ]] && PYTHON_VERSION=$(extract-version "$PYTHON")
[[ -z $PYTHON_MAJOR ]] && PYTHON_MAJOR=${PYTHON_VERSION/.*/}
[[ $PYTHON_MAJOR -ge 3 ]] || error "Python 3 required, current version: $PYTHON_VERSION"

# Shared object files suffix.
[[ -z $SO_SUFFIX && -n $MACOS ]] && SO_SUFFIX=.dylib
[[ -z $SO_SUFFIX && -z $MACOS ]] && SO_SUFFIX=.so

# Logical CPU count and physical core count.
if [[ -n $MACOS ]]; then
    [[ -z $CPU_COUNT ]] && CPU_COUNT=$(sysctl -n hw.logicalcpu 2>/dev/null)
    [[ -z $CORE_COUNT ]] && CORE_COUNT=$(sysctl -n hw.physicalcpu 2>/dev/null)
elif [[ -n $FREEBSD ]]; then
    [[ -z $CPU_COUNT ]] && CPU_COUNT=$(sysctl -n kern.smp.cpus 2>/dev/null)
    [[ -z $CORE_COUNT ]] && CORE_COUNT=$(sysctl -n kern.smp.cores 2>/dev/null)
elif [[ -n $OPENBSD$NETBSD$DRAGONFLYBSD ]]; then
    [[ -z $CPU_COUNT ]] && CPU_COUNT=$(sysctl -n hw.ncpu 2>/dev/null)
else
    [[ -z $CPU_COUNT ]] && CPU_COUNT=$(nproc 2>/dev/null)
    [[ -z $CORE_COUNT ]] && CORE_COUNT=$($FGREP -m1 'cpu cores' /proc/cpuinfo 2>/dev/null | $SED -e 's/.* //')
fi
[[ -z $CPU_COUNT ]] && CPU_COUNT=1
[[ -z $CORE_COUNT ]] && CORE_COUNT=$CPU_COUNT

# Alternative root for development tools and library:
# - On macOS, Homebrew development packages are installed in /usr/local (Intel) or /opt/homebrew (Arm).
# - On Linux, when Homebrew is forced, use it as alternative development root.
# - On FreeBSD, OpenBSD, DragonFlyBSD, the packages are installed in /usr/local.
# - On NetBSD, the packages are installed in /usr/pkg.
if [[ -z $ALTDEVROOT ]]; then
    if [[ -n $NETBSD ]]; then
        ALTDEVROOT=/usr/pkg
    elif [[ -n $LINUXBREW && -z $HOMEBREW_PREFIX ]]; then
        error "LINUXBREW is defined but not HOMEBREW_PREFIX"
    elif [[ -n $MACOS$LINUXBREW$FREEBSD$OPENBSD$DRAGONFLYBSD ]]; then
        if [[ -n $HOMEBREW_PREFIX ]]; then
            ALTDEVROOT=$HOMEBREW_PREFIX
        elif [[ -d /opt/homebrew/bin ]]; then
            ALTDEVROOT=/opt/homebrew
        else
            ALTDEVROOT=/usr/local
        fi
    else
        ALTDEVROOT=/usr
    fi
fi

# Locate system directories.
[[ -n $LINUX && -z $LINUXBREW && $MAIN_ARCH == *64* && -n $(exist-wildcard /usr/lib64/libc.so*) ]] && USELIB64=1
if [[ -z $SYSPREFIX ]]; then
    [[ -n $MACOS$LINUXBREW$BSD ]] && SYSPREFIX="$ALTDEVROOT" || SYSPREFIX=/usr
fi
if [[ -z $USRLIBDIR ]]; then
    [[ -n $USELIB64 ]] && USRLIBDIR="$SYSPREFIX/lib64" || USRLIBDIR="$SYSPREFIX/lib"
fi
if [[ -z $ETCDIR ]]; then
    [[ $SYSPREFIX == /usr ]] && ETCDIR=/etc || ETCDIR="$SYSPREFIX/etc"
fi
if [[ -z $UDEVDIR ]]; then
    [[ $SYSPREFIX == /usr ]] && UDEVDIR=/lib/udev/rules.d || UDEVDIR="$SYSPREFIX/etc/udev/rules.d"
fi

# Locate bash completions facility.
if [[ -z $BASHCOMP_DIR ]]; then
    if [[ -n $MACOS$LINUXBREW$FREEBSD$OPENBSD$DRAGONFLYBSD ]]; then
        BASHCOMP_DIR="$ALTDEVROOT/etc/bash_completion.d"
    else
        BASHCOMP_DIR="$SYSPREFIX/share/bash-completion/completions"
    fi
fi
[[ -z $MACOS$LINUXBREW$BSD ]] && BASHCOMP_AUTO=1

#-----------------------------------------------------------------------------
# Make customization.
#-----------------------------------------------------------------------------

# Use $(call LOG,[op] target) in front of all executed commands.
# "make V=1" is a traditional shortcut for VERBOSE (and vice versa).
if [[ -n $V$VERBOSE ]]; then
     VERBOSE=true
     V=true
     LOG=
else
     LOG="@echo '  \$(1)';"
fi

# Recursive invocations of make should be silent.
[[ $MAKEFLAGS != *no-print-directory* ]] && MAKEFLAGS="--no-print-directory $MAKEFLAGS"

# Best multiprocessor make option if unspecified otherwise.
[[ -z $MAKEFLAGS_SMP && $MAKEFLAGS != *-j* && $CPU_COUNT -gt 1 ]] && MAKEFLAGS_SMP="-j$CPU_COUNT"

#-----------------------------------------------------------------------------
# Cross-compilation support.
#-----------------------------------------------------------------------------

# If NATIVEBINDIR is specified in input, transform it into an absolute path for recursion.
if [[ -n $NATIVEBINDIR && $MAKEOVERRIDES == *NATIVEBINDIR=* ]]; then
    INNATIVEBINDIR="$NATIVEBINDIR"
    NATIVEBINDIR=$($REALPATH -m "$NATIVEBINDIR")
    MAKEOVERRIDES=$($SED <<<$MAKEOVERRIDES -e "s|NATIVEBINDIR=$INNATIVEBINDIR|NATIVEBINDIR=$NATIVEBINDIR|")
fi

if [[ -z $CROSS$CROSS_TARGET ]]; then
    # No cross-compilation.
    CXXFLAGS_CROSS=
    LDFLAGS_CROSS=
    # In last step of build, always use the version of tsxml which is built.
    # Note: DYLD_LIBRARY_PATH is used on macOS only.
    TSXML='LD_LIBRARY_PATH="$(BINDIR):$(LD_LIBRARY_PATH)" DYLD_LIBRARY_PATH="$(BINDIR):$(DYLD_LIBRARY_PATH)" $(BINDIR)/tsxml'
else
    # Perform cross-compilation.
    CROSS=1
    # Cross-compilation tools are in /usr/local by default.
    [[ -z $CROSS_PREFIX ]] && CROSS_PREFIX="/usr/local"
    # If cross target undefined, find the first one.
    # We look for a file pattern: PREFIX/TARGET/bin/TARGET-gcc
    [[ -z $CROSS_TARGET ]] && CROSS_TARGET=$(
        ls "$CROSS_PREFIX"/*/bin/*-gcc 2>/dev/null | \
        $GREP "$CROSS_PREFIX"'//*\([^/]*\)/bin/\1-gcc' | \
        $SED -e "s|^$CROSS_PREFIX//*||" -e 's|/.*$||' | \
        head -1)
    [[ -z $CROSS_TARGET ]] && error "CROSS is defined but no cross-compilation tool-chain was found"
    # Adjust target. Use "arm" as main target if CROSS_TARGET starts with "arm".
    [[ $CROSS_TARGET == arm* ]] && MAIN_ARCH=arm || MAIN_ARCH=$CROSS_TARGET
    CXXFLAGS_TARGET=
    # Redirect build tools.
    search-cross() {
        ls 2>/dev/null \
           $CROSS_PREFIX/$CROSS_TARGET/bin/$CROSS_TARGET-$1 \
           $CROSS_PREFIX/$CROSS_TARGET/bin/$1 \
           $CROSS_PREFIX/bin/$CROSS_TARGET-$1 | \
           head -1
    }
    CXX=$(search-cross g++)
    GCC=$(search-cross gcc)
    LD=$(search-cross ld)
    [[ -z $CXX ]] && error "cross g++ not found for $CROSS_TARGET"
    [[ -z $GCC ]] && error "cross gcc not found for $CROSS_TARGET"
    [[ -z $LD ]] && error "cross ld not found for $CROSS_TARGET"
    # Add options. The layout can be different, so use them all.
    CXXFLAGS_CROSS='-I$(CROSS_PREFIX)/$(CROSS_TARGET)/include -I$(CROSS_PREFIX)/$(CROSS_TARGET)/$(CROSS_TARGET)/include -I$(CROSS_PREFIX)/$(CROSS_TARGET)/$(CROSS_TARGET)/libc/include'
    LDFLAGS_CROSS='-L$(CROSS_PREFIX)/$(CROSS_TARGET)/lib -L$(CROSS_PREFIX)/$(CROSS_TARGET)/$(CROSS_TARGET)/lib -L$(CROSS_PREFIX)/$(CROSS_TARGET)/$(CROSS_TARGET)/libc/lib'
    # In last step of build, use tsxml from 1) command line, 2) already installed, 3) previous native build.
    [[ -z $TSXML ]] && TSXML=$(which tsxml 2>/dev/null)
    [[ -z $TSXML && -x $NATIVEBINDIR/tsxml ]] && \
        TSXML='LD_LIBRARY_PATH="$(NATIVEBINDIR):$(LD_LIBRARY_PATH)" DYLD_LIBRARY_PATH="$(NATIVEBINDIR):$(DYLD_LIBRARY_PATH)" $(NATIVEBINDIR)/tsxml'
    [[ -z $TSXML ]] && error "no native TSDuck found for cross-compilation, check NATIVEBINDIR"
fi

#-----------------------------------------------------------------------------
# Project structure.
#-----------------------------------------------------------------------------

# Keep the value of BINDIR as specified in the top-level make invocation.
if [[ -z $TOPLEVEL_CURDIR ]]; then
    TOPLEVEL_CURDIR="$CURDIR"
    TOPLEVEL_BINDIR="$BINDIR"
fi

# Project specific directories.
if [[ $ROOTDIR != $root ]]; then
    # Switched project root, as in rpm build.
    ROOTDIR="$root"
    TOPLEVEL_CURDIR="$CURDIR"
fi
INSTALLERDIR="$ROOTDIR/pkg/installers"
SCRIPTSDIR="$ROOTDIR/scripts"
SRCROOT="$ROOTDIR/src"
LIBTSDUCKDIR="$SRCROOT/libtsduck"
TSTOOLSDIR="$SRCROOT/tstools"
TSPLUGINSDIR="$SRCROOT/tsplugins"
BINROOT="$ROOTDIR/bin"

# Shell command to get TSDuck version.
[[ -z $GET_TSDUCK_VERSION ]] && GET_TSDUCK_VERSION='$(PYTHON) $(SCRIPTSDIR)/get-version-from-sources.py'

# Fixed suffix to add to generated BINDIR directory name for some specialized builds.
[[ -n $GCOV && $BINDIR_SUFFIX != *-gcov* ]] && BINDIR_SUFFIX="${BINDIR_SUFFIX}-gcov"
[[ -n $GPROF && $BINDIR_SUFFIX != *-gprof* ]] && BINDIR_SUFFIX="${BINDIR_SUFFIX}-gprof"
[[ -n $ASAN && $BINDIR_SUFFIX != *-asan* ]] && BINDIR_SUFFIX="${BINDIR_SUFFIX}-asan"
[[ -n $UBSAN && $BINDIR_SUFFIX != *-ubsan* ]] && BINDIR_SUFFIX="${BINDIR_SUFFIX}-ubsan"
[[ -n $LLVM && $BINDIR_SUFFIX != *-clang* ]] && BINDIR_SUFFIX="${BINDIR_SUFFIX}-clang"
[[ -n $STATIC && $BINDIR_SUFFIX != *-static* ]] && BINDIR_SUFFIX="${BINDIR_SUFFIX}-static"

# Output directories for final binaries and objects.
inbindir="$BINDIR"
if [[ -n $TOPLEVEL_BINDIR ]]; then
    # BINDIR was specified on the initial command line.
    [[ $BINDIR == /* ]] || BINDIR=$($REALPATH -m "$TOPLEVEL_CURDIR/$BINDIR")
else
    [[ -n $HOSTNAME ]] && hpart="-$HOSTNAME" || hpart=""
    if [[ -n $DEBUG ]]; then
        # Default BINDIR for debug mode.
        BINDIR="$BINROOT/debug-${MAIN_ARCH}${hpart}${BINDIR_SUFFIX}"
    else
        # Default BINDIR for release mode.
        BINDIR="$BINROOT/release-${MAIN_ARCH}${hpart}${BINDIR_SUFFIX}"
    fi
fi
if [[ $MAKEOVERRIDES == *BINDIR=* && $BINDIR != $inbindir ]]; then
    # Redefine BINDIR is specified in MAKEOVERRIDES.
    MAKEOVERRIDES=$($SED <<<$MAKEOVERRIDES -e "s|BINDIR=$inbindir|BINDIR=$BINDIR|")
fi

# Subdirectory of $BINDIR where object files are stored; named from make's CURDIR.
OBJDIR="$BINDIR/objs-\$(notdir \$(CURDIR))"

# Output library files depend on $(BINDIR) in makefile.
STATIC_LIBTSDUCK="$BINDIR/libtsduck.a"
SHARED_LIBTSDUCK="$BINDIR/libtsduck$SO_SUFFIX"

#-----------------------------------------------------------------------------
# Preload configuration variables: exclude some dependencies.
# These variables are either specified on the command line or computed from
# the environement (e.g. unsupported feature on that platform). However, there
# may be temporary situations where they are preset in tsPreConfiguration.h.
#-----------------------------------------------------------------------------

if [[ -z $PRECONFIG_DONE ]]; then
    # Avoid re-parsing when make recurses.
    PRECONFIG_DONE=1

    # List of defined macros in tsPreConfiguration.h
    PRECONFIG=/$($SED "$LIBTSDUCKDIR/base/cpp/tsPreConfiguration.h" -e '/^ *#define/!d' -e 's/^ *#define *//' -e 's/[( ].*//')/
    PRECONFIG=${PRECONFIG//$'\n'/\/}

    # See possible predefinitions in tsPreConfiguration.h in src/libtsduck/Makefile.
    [[ $PRECONFIG == */TS_NO_PCSC/* ]] && NOPCSC=1
    [[ $PRECONFIG == */TS_NO_GITHUB/* ]] && NOGITHUB=1
    [[ $PRECONFIG == */TS_NO_DTAPI/* ]] && NODTAPI=1
    [[ $PRECONFIG == */TS_NO_HIDES/* ]] && NOHIDES=1
    [[ $PRECONFIG == */TS_NO_VATEK/* ]] && NOVATEK=1
    [[ $PRECONFIG == */TS_NO_EDITLINE/* ]] && NOEDITLINE=1
    [[ $PRECONFIG == */TS_NO_CURL/* ]] && NOCURL=1
    [[ $PRECONFIG == */TS_NO_ZLIB/* ]] && NOZLIB=1
    [[ $PRECONFIG == */TS_NO_SRT/* ]] && NOSRT=1
    [[ $PRECONFIG == */TS_NO_RIST/* ]] && NORIST=1
    [[ $PRECONFIG == */TS_NO_OPENSSL/* ]] && NOOPENSSL=1
fi

#-----------------------------------------------------------------------------
# Compilation flags and other build commands.
#-----------------------------------------------------------------------------

# Reset these variables, we always rebuild them from scratch.
LIBTSDUCK_CXXFLAGS_INCLUDES=
LIBTSDUCK_LDLIBS=
CXXFLAGS_INCLUDES=
CXXFLAGS_WARNINGS=
CXXFLAGS_NO_WARNINGS=
CPPFLAGS=
CXXFLAGS=
LDFLAGS=
LDLIBS=
ARFLAGS=

# Use $(CXX) for compilation. Use $(GCC) to explicitly reference GCC.
[[ -z $GCC ]] && GCC=gcc

# Define compilation flags for 32-bit cross-compilation.
if [[ -n $M32 ]]; then
    CXXFLAGS_TARGET="-march=i686"
    CXXFLAGS_M32="-m32"
    LDFLAGS_M32="-m32"
fi

# Get current compiler version, usually in form x.y.z
[[ -z $GCC_VERSION ]] && GCC_VERSION=$(extract-version "$GCC")
[[ -z $GCC_MAJOR ]] && GCC_MAJOR=${GCC_VERSION/.*/}
[[ -z $LLVM_VERSION ]] && LLVM_VERSION=$(extract-version clang)
[[ -z $LLVM_MAJOR ]] && LLVM_MAJOR=${LLVM_VERSION/.*/}

# Possibly used in scripts we will call.
export GCC_VERSION GCC_MAJOR LLVM_VERSION LLVM_MAJOR

# Forced usage of LLVM (clang). FreeBSD and OpenBSD have switched to clang by default.
if [[ -n $LLVM$FREEBSD$OPENBSD ]]; then
    USE_GCC=
    USE_LLVM=1
    CXX=clang++
    CC=clang
    # On some systems (e.g. Ubuntu 22.04), clang-14 is the default but is
    # incompatible with the installed gcc header files in C++20 mode.
    # If clang-15 is installed, use it instead of clang-14.
    [[ $LLVM_MAJOR -le 14 ]] && CXX=$(getcmd clang++-15 clang++)
elif [[ -z $USE_GCC$USE_LLVM ]]; then
    # Check if CXX is gcc or clang.
    [[ -n $($CXX --version 2>/dev/null | $GREP -i -e gcc -e g++) ]] && USE_GCC=1 || USE_GCC=
    [[ -n $($CXX --version 2>/dev/null | $GREP -i -e clang) ]] && USE_LLVM=1 || USE_LLVM=
fi

# Compilation flags for various types of optimization.
# Example to specify that some selected modules should be compiled for full speed:
# $(OBJDIR)/fast1.o $(OBJDIR)/fast2.o: CXXFLAGS_OPTIMIZE = $(CXXFLAGS_FULLSPEED)
[[ -z $CXXFLAGS_OPTIMIZE ]] && CXXFLAGS_OPTIMIZE="-O2 -fno-strict-aliasing"
[[ -z $CXXFLAGS_FULLSPEED ]] && CXXFLAGS_FULLSPEED="-O3 -fno-strict-aliasing -funroll-loops -fomit-frame-pointer"
[[ -z $CXXFLAGS_OPTSIZE ]] && CXXFLAGS_OPTSIZE="-Os -fno-strict-aliasing"

if [[ $ALTDEVROOT != /usr ]]; then
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -I$ALTDEVROOT/include"
    LDLIBS="-L$ALTDEVROOT/lib $LDLIBS"
fi

# Always use maximal or even paranoid warning mode.
# With clang, the option -Weverything turns everything on. There is no such option with GCC.
CXXFLAGS_WARNINGS="-Werror"
if [[ -n $USE_LLVM ]]; then
    CXXFLAGS_WARNINGS="$CXXFLAGS_WARNINGS -Weverything"
    CXXFLAGS_NO_WARNINGS="$CXXFLAGS_NO_WARNINGS -Wno-c++98-compat-pedantic"
    if [[ -n $MACOS ]]; then
        # On macOS, it is normal to include from /usr/local/include since some libraries come from Homebrew.
        # Starting with clang 12, this generates a warning we need to disable. However, this disable option
        # generates an error with previous versions. And we cannot disable this warning inside the code since
        # this is a command-line-level error. So, we must check the version here...
        [[ $LLVM_MAJOR -ge 12 ]] && CXXFLAGS_NO_WARNINGS="$CXXFLAGS_NO_WARNINGS -Wno-poison-system-directories"
    fi
else
    CXXFLAGS_WARNINGS="$CXXFLAGS_WARNINGS -Wall -Wextra -Wformat-nonliteral -Wformat-security -Wswitch-default -Wuninitialized"
    CXXFLAGS_WARNINGS="$CXXFLAGS_WARNINGS -Wfloat-equal -Wundef -Wpointer-arith -Woverloaded-virtual -Wctor-dtor-privacy"
    CXXFLAGS_WARNINGS="$CXXFLAGS_WARNINGS -Wnon-virtual-dtor -Wsign-promo -Wzero-as-null-pointer-constant -Wstrict-null-sentinel"
    [[ $GCC_MAJOR -ge 5 ]] && CXXFLAGS_WARNINGS="$CXXFLAGS_WARNINGS -Wshadow -Wpedantic -Weffc++"
    [[ $GCC_MAJOR -ge 6 ]] && CXXFLAGS_WARNINGS="$CXXFLAGS_WARNINGS -Wsuggest-override"
    [[ $MAIN_ARCH == arm* ]] && CXXFLAGS_NO_WARNINGS="$CXXFLAGS_NO_WARNINGS -Wno-psabi"
fi

# Language levels. Minimum level is C++20.
[[ -z $CXXFLAGS_STANDARD ]] && CXXFLAGS_STANDARD="-std=c++20"

# Compilation flags for security.
[[ -z $CXXFLAGS_SECURITY ]] && CXXFLAGS_SECURITY="-fstack-protector-all"

# Compilation flags for position-independent code.
[[ -z $CXXFLAGS_FPIC ]] && CXXFLAGS_FPIC="-fPIC"

# Compilation flags in debug mode.
if [[ -n $DEBUG ]]; then
    CXXFLAGS_DEBUG="-g -DDEBUG=1"
    LDFLAGS_DEBUG=
else
    CXXFLAGS_DEBUG='$(CXXFLAGS_OPTIMIZE)'
    LDFLAGS_DEBUG=
fi

# Compilation flags for code sanitizing using AddressSanitizer with default optimization.
if [[ -n $ASAN ]]; then
    CXXFLAGS_ASAN="-fsanitize=address -g3"
    LDFLAGS_ASAN="-fsanitize=address -g3"
else
    CXXFLAGS_ASAN=
    LDFLAGS_ASAN=
fi

# Compilation flags for code sanitizing using UndefinedBehaviorSanitizer with default optimization.
if [[ -n $UBSAN ]]; then
    CXXFLAGS_UBSAN="-fsanitize=undefined -fno-sanitize=unsigned-integer-overflow -fno-sanitize=alignment -g3"
    LDFLAGS_UBSAN="-fsanitize=undefined -fno-sanitize=unsigned-integer-overflow -fno-sanitize=alignment -g3"
else
    CXXFLAGS_UBSAN=
    LDFLAGS_UBSAN=
fi

# Compilation flags for code coverage using gcov.
if [[ -n $GCOV ]]; then
    CXXFLAGS_GCOV="--coverage -DGCOV=1"
    LDFLAGS_GCOV="--coverage"
else
    CXXFLAGS_GCOV=
    LDFLAGS_GCOV=
fi

# Compilation flags for code profiling using gprof.
if [[ -n $GPROF ]]; then
    CXXFLAGS_GPROF="-g -pg -DGPROF=1"
    LDFLAGS_GPROF="-g -pg"
else
    CXXFLAGS_GPROF=
    LDFLAGS_GPROF=
fi

# Compilation flags for posix threads.
CXXFLAGS_PTHREAD="-pthread"
[[ -z $MACOS ]] && LDFLAGS_PTHREAD="-pthread"

# External libraries
LDLIBS="$LDLIBS -lpthread"
[[ -z $MACOS$OPENBSD ]] && LDLIBS="$LDLIBS -lrt"
LDLIBS="$LDLIBS -lm"

# Add a module with ar.
ARFLAGS_ADD="rc"
[[ -z $MACOS ]] && ARFLAGS_ADD="${ARFLAGS_ADD}U"

# $(SOFLAGS) specifies options when creating shared objects (.so or .dylib files).
if [[ -n $LINUX ]]; then
    LDFLAGS_LINKER="-Wl,-rpath,'\$\$ORIGIN',-z,noexecstack"
    SOFLAGS='-Wl,-soname=$(notdir $@),-z,noexecstack'
elif [[ -n $FREEBSD ]]; then
    LDFLAGS_LINKER="-Wl,-rpath,'\$\$ORIGIN',-z,noexecstack"
    SOFLAGS='-Wl,-soname=$(notdir $@),-z,noexecstack'
elif [[ -n $OPENBSD ]]; then
    LDFLAGS_LINKER="-Wl,-z,origin,-rpath,'\$\$ORIGIN',-z,noexecstack"
    SOFLAGS='-Wl,-soname=$(notdir $@),-z,noexecstack'
elif [[ -n $NETBSD ]]; then
    LDFLAGS_LINKER="-Wl,-rpath,'\$\$ORIGIN',-rpath,\$(ALTDEVROOT)/lib,-z,noexecstack"
    SOFLAGS='-Wl,-soname=$(notdir $@),-z,noexecstack'
elif [[ -n $DRAGONFLYBSD ]]; then
    LDFLAGS_LINKER="-Wl,-z,origin,-rpath,'\$\$ORIGIN',-z,noexecstack"
    SOFLAGS='-Wl,-soname=$(notdir $@),-z,noexecstack'
elif [[ -n $MACOS ]]; then
    [[ $LLVM_MAJOR -ge 15 ]] && dup="-no_warn_duplicate_libraries" || $dup=
    LDFLAGS_LINKER="-Wl,-rpath,@executable_path -Wl,-rpath,@executable_path/../lib -Xlinker $dup"
    SOFLAGS="-install_name '@rpath/\$(notdir \$@)'"
fi

# Global compilation flags, only based on other sub-variables (substitution will be done by make).
CPPFLAGS='$(CXXFLAGS_INCLUDES) $(CXXFLAGS_STANDARD) $(CXXFLAGS_NO_WARNINGS) $(CPPFLAGS_EXTRA)'
CXXFLAGS='$(CXXFLAGS_DEBUG) $(CXXFLAGS_M32) $(CXXFLAGS_ASAN) $(CXXFLAGS_UBSAN) $(CXXFLAGS_GCOV) $(CXXFLAGS_GPROF) $(CXXFLAGS_WARNINGS) $(CXXFLAGS_NO_WARNINGS) $(CXXFLAGS_SECURITY) $(CXXFLAGS_INCLUDES) $(CXXFLAGS_TARGET) $(CXXFLAGS_FPIC) $(CXXFLAGS_STANDARD) $(CXXFLAGS_CROSS) $(CXXFLAGS_PTHREAD) $(CXXFLAGS_EXTRA)'
LDFLAGS='$(LDFLAGS_DEBUG) $(LDFLAGS_M32) $(LDFLAGS_ASAN) $(LDFLAGS_UBSAN) $(LDFLAGS_GCOV) $(LDFLAGS_GPROF) $(CXXFLAGS_TARGET) $(LDFLAGS_CROSS) $(LDFLAGS_PTHREAD) $(LDFLAGS_EXTRA) $(LDFLAGS_LINKER)'
ARFLAGS='$(ARFLAGS_ADD) $(ARFLAGS_EXTRA)'

# Java compiler.
if [[ -z $NOJAVA$JAVA_DONE ]]; then
    JAVA_DONE=1
    JAVAC=$($SCRIPTSDIR/java-config.sh --javac)
    if [[ -z $JAVAC ]]; then
        NOJAVA=1
    else
        CXXFLAGS_JAVA=$($SCRIPTSDIR/java-config.sh --cflags)
        # Generate classes to make sure they are compatible with Java 8.
        [[ -z $JAVAC_FLAGS ]] && JAVAC_FLAGS="-source 1.8 -target 1.8 -Xlint:-options"
    fi
fi

# Static linking.
# Not meaningful everywhere:
# - Static linking with system libraries is not supported on macOS.
# - On Linux, all used static libraries must be installed. This is not supported
#   on all distros. On Fedora, you may install "glibc-static libstdc++-static"
#   but there is no static package for curl and pcsclite.
if [[ -z $STATIC ]]; then
    # Dynamic (default) link
    [[ -z $OPENBSD$NETBSD ]] && LDLIBS="-ldl $LDLIBS"
elif [[ -n $MACOS ]]; then
    error "static linking is not supported on macOS"
else
    [[ -n $NOSTATIC ]] && error "cannot define STATIC and NOSTATIC together"
    NOCURL=1
    NOPCSC=1
    NODTAPI=1
    NOVATEK=1
    NOZLIB=1
    NOSRT=1
    NORIST=1
    NOEDITLINE=1
    NOTEST=1
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTSDUCK_STATIC=1"
fi

# Exclude features which are not supported in the current environment
if [[ -n $CROSS$CROSS_TARGET ]]; then
    # Some libraries are bypassed in cross-compilation.
    NOCURL=1
    NOPCSC=1
    NODTAPI=1
    NOVATEK=1
    NOZLIB=1
    NOSRT=1
    NORIST=1
    NOEDITLINE=1
fi
if [[ -z $NOSRT$SRT_DONE ]]; then
    # SRT not disabled, check if libsrt is available.
    [[ -z $(exist-wildcard /usr/include/srt/*.h $ALTDEVROOT/include/srt/*.h) ]] && NOSRT=1
    SRT_DONE=1
fi
if [[ -z $NORIST$RIST_DONE ]]; then
    # RIST not disabled, check if librist is available.
    [[ -z $(exist-wildcard /usr/include/librist/*.h $ALTDEVROOT/include/librist/*.h) ]] && NORIST=1
    RIST_DONE=1
fi
if [[ -z $NOCURL ]]; then
    # curl not disabled, check if available.
    [[ -z $(which curl-config 2>/dev/null) ]] && NOCURL=1
fi
if [[ -n $NODEKTEC ]]; then
    # NODEKTEC is an alternative name for NODTAPI
    NODTAPI=1
fi
if [[ -n $MACOS$BSD ]]; then
    # Dektec and HiDes devices are not supported on macOS and BSD systems (no driver).
    NODTAPI=1
    NOHIDES=1
fi
if [[ -z $NODTAPI$DTAPI_DONE ]]; then
    # DTAPI not disabled, check if available.
    [[ -z $($SCRIPTSDIR/dtapi-config.sh --support) ]] && NODTAPI=1
    DTAPI_DONE=1
fi
if [[ -n $BSD ]]; then
    # Vatek library has not yet been validated on BSD systems (only depends on libusb).
    NOVATEK=1
fi
if [[ -z $NOPCSC$MACOS$PCSC_DONE ]]; then
    # PCSC not disabled and not on macOS, check if available. On macOS, it is always available.
    [[ -z $(exist-wildcard /usr/include/PCSC/*.h $ALTDEVROOT/include/PCSC/*.h) ]] && NOPCSC=1
    PCSC_DONE=1
fi

# Download Dektec library (DTAPI) if required.
if [[ -z $NODTAPI$NOEXTLIBS ]]; then
    [[ $M32 ]] && m32=--m32 || m32=
    [[ -z $DTAPI_OBJECT ]] && DTAPI_OBJECT=$($SCRIPTSDIR/dtapi-config.sh --object --download $m32)
    [[ -z $DTAPI_HEADER ]] && DTAPI_HEADER=$($SCRIPTSDIR/dtapi-config.sh --header)
    if [[ -z $DTAPI_OBJECT || -z $DTAPI_HEADER ]]; then
        NODTAPI=1
    else
        LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES -isystem $(dirname $DTAPI_HEADER)"
    fi
fi

# Download VATek library if required.
if [[ -z $NOVATEK$NOEXTLIBS ]]; then
    if [[ -z $VATEK_CFLAGS ]]; then
        # Eliminate all our variables from the environment. Compilation options
        # would interfere with VATek build if we need to recompile the library.
        undef=${VARNAMES/% /}
        undef=${undef// / -u }
        VATEK_CFLAGS=$(env $undef $SCRIPTSDIR/vatek-config.sh --cflags --download)
    fi
    [[ -z $VATEK_LDLIBS ]] && VATEK_LDLIBS=$($SCRIPTSDIR/vatek-config.sh --ldlibs)
    if [[ -z $VATEK_LDLIBS ]]; then
        NOVATEK=1
    else
        LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES $VATEK_CFLAGS"
	LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS $VATEK_LDLIBS"
    fi
fi

# These symbols from the make command line drive the bitrate representation.
if [[ -n $BITRATE_FRACTION ]]; then
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_BITRATE_FRACTION=1"
elif [[ -n $BITRATE_INTEGER ]]; then
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_BITRATE_INTEGER=1"
elif [[ -n $BITRATE_FLOAT ]]; then
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_BITRATE_FLOAT=1"
elif [[ -n $BITRATE_FIXED ]]; then
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_BITRATE_FIXED=1"
fi
if [[ -n $BITRATE_DECIMALS ]]; then
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_BITRATE_DECIMALS=$BITRATE_DECIMALS"
fi

# Using PCSC.
if [[ -n $NOPCSC ]]; then
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_NO_PCSC=1"
    LDLIBS_PCSC=
elif [[ -n $MACOS ]]; then
    # On macOS, use PCSC.framework
    LDLIBS_PCSC="-framework PCSC"
elif [[ -n $LINUXBREW$BSD ]]; then
    # PCSC on Linuxbrew and all BSD systems.
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -I$ALTDEVROOT/include/PCSC"
    LDLIBS_PCSC="-lpcsclite"
else
    # PCSC on Linux
    CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -I/usr/include/PCSC"
    LDLIBS_PCSC="-lpcsclite"
fi

# Other optional features.
[[ -n $NOOPENSSL ]] && CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_NO_OPENSSL=1"
[[ -n $NOGITHUB ]] && CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_NO_GITHUB=1"
[[ -n $ASSERTIONS ]] && CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_KEEP_ASSERTIONS=1"
[[ -n $NOHWACCEL ]] && CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_NO_ARM_CRC32_INSTRUCTIONS=1"
[[ -n $NOHWACCEL ]] && CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_NO_ARM_AES_INSTRUCTIONS=1"
[[ -n $NODEPRECATE ]] && CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES -DTS_NODEPRECATE=1"

# These variables are used when building the TSDuck library, not in the applications.
# Note, however, that LIBTSDUCK_LDLIBS is still necessary when linking applications
# against the TSDuck static library.
LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES $CXXFLAGS_JAVA"
LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS $LDLIBS_PCSC"
[[ -n $FREEBSD ]] && LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS -lprocstat"
[[ -n $OPENBSD$NETBSD$DRAGONFLYBSD ]] && LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS -lkvm"
[[ -n $LINUX ]] && LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS -latomic"
[[ -z $NOOPENSSL ]] && LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS -lcrypto"
[[ -n $NODTAPI ]] && LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES -DTS_NO_DTAPI=1"
[[ -n $NOHIDES ]] && LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES -DTS_NO_HIDES=1"
[[ -n $NOVATEK ]] && LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES -DTS_NO_VATEK=1"
if [[ -n $NOEDITLINE ]]; then
    LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES -DTS_NO_EDITLINE=1"
else
    LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS -ledit"
fi
if [[ -n $NOCURL ]]; then
    LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES -DTS_NO_CURL=1"
else
    if [[ -z $CURL_DONE ]]; then
        CURL_DONE=1
        CXXFLAGS_CURL="$(curl-config --cflags)"
        # Remove useless explicit references to /usr/lib which break usage of alternative compilers
        LDLIBS_CURL=$(curl-config --libs | $SED -e 's|-L/usr/lib||' -e 's|-Wl,-R/usr/lib||')
    fi
    LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES $CXXFLAGS_CURL"
    LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS $LDLIBS_CURL"
fi
if [[ -n $NOZLIB ]]; then
    LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES -DTS_NO_ZLIB=1"
else
    LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS -lz"
fi
if [[ -n $NOSRT ]]; then
    LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES -DTS_NO_SRT=1"
else
    LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS -lsrt"
fi
if [[ -n $NORIST ]]; then
    LIBTSDUCK_CXXFLAGS_INCLUDES="$LIBTSDUCK_CXXFLAGS_INCLUDES -DTS_NO_RIST=1"
else
    LIBTSDUCK_LDLIBS="$LIBTSDUCK_LDLIBS -lrist"
fi

#-----------------------------------------------------------------------------
# List of source directories, tools, plugins, etc
#-----------------------------------------------------------------------------

# Subdirectories with system-specific source code for local OS.
if [[ -z $LOCAL_OSDIR ]]; then
    LOCAL_OSDIR=${LOCAL_OS/darwin/mac}
    LOCAL_OSDIR=${LOCAL_OSDIR/dragonfly/dragonflybsd}
fi

# Subdirectories with system-specific source code for other OS (to be excluded from the build).
if [[ -z $OTHER_OS ]]; then
    OTHER_OS=" linux mac freebsd netbsd openbsd dragonflybsd windows "
    [[ -z $BSD ]] && OTHER_OS="${OTHER_OS}bsd"
    OTHER_OS=${OTHER_OS/ $LOCAL_OSDIR / }
    OTHER_OS=${OTHER_OS/# /}
    OTHER_OS=${OTHER_OS/% /}
fi

# List of libtsduck directories containing header files.
if [[ -z $ALL_INCLUDES ]]; then
    for dir in $(find $LIBTSDUCKDIR -type d); do
        [[ " $OTHER_OS " != *" $(fbasename $dir) "* && -n $(exist-wildcard $dir/*.h) ]] && ALL_INCLUDES="$ALL_INCLUDES $dir"
    done
fi

# List of libtsduck directories containing private and public headers.
if [[ -z $PRIVATE_INCLUDES || -z $PUBLIC_INCLUDES || -z $CXXFLAGS_PRIVATE_INCLUDES || -z $CXXFLAGS_PUBLIC_INCLUDES ]]; then
    for dir in $ALL_INCLUDES; do
        if [[ $(fbasename $dir) == private ]]; then
            PRIVATE_INCLUDES="$PRIVATE_INCLUDES $dir"
            CXXFLAGS_PRIVATE_INCLUDES="$CXXFLAGS_PRIVATE_INCLUDES -I$dir"
        else
            PUBLIC_INCLUDES="$PUBLIC_INCLUDES $dir"
            CXXFLAGS_PUBLIC_INCLUDES="$CXXFLAGS_PUBLIC_INCLUDES -I$dir"
        fi
    done
fi
CXXFLAGS_INCLUDES="$CXXFLAGS_INCLUDES $CXXFLAGS_PUBLIC_INCLUDES"

# Obsolete plugins, were in separate shared libraries, now in libtsduck.so.
# Maintenance: also update pkg/nsis/tsduck.nsi (Windows).
NO_TSPLUGINS="tsplugin_dektec tsplugin_drop tsplugin_file tsplugin_fork tsplugin_hls tsplugin_http tsplugin_ip tsplugin_null tsplugin_psi tsplugin_rist tsplugin_skip tsplugin_srt tsplugin_table tsplugin_teletext"

# Build a list of tools and plugins to not build or deinstall from the system tree.
NO_TSTOOLS=
[[ -n $NOOPENSSL ]] && NO_TSPLUGINS="$NO_TSPLUGINS tsplugin_aes tsplugin_descrambler tsplugin_scrambler"
[[ -n $NODTAPI ]] && NO_TSTOOLS="$NO_TSTOOLS tsdektec"
[[ -n $NOHIDES ]] && NO_TSTOOLS="$NO_TSTOOLS tshides"
[[ -n $NOHIDES ]] && NO_TSPLUGINS="$NO_TSPLUGINS tsplugin_hides"
[[ -n $NOVATEK ]] && NO_TSTOOLS="$NO_TSTOOLS tsvatek"
[[ -n $NOPCSC ]] && NO_TSTOOLS="$NO_TSTOOLS tssmartcard"

# List of plugins and tools to build.
if [[ -z $TSPLUGINS ]]; then
    for file in $TSPLUGINSDIR/tsplugin_*.cpp; do
        name=$(fbasename $file .cpp)
        [[ " $NO_TSPLUGINS " != *" $name "* ]] && TSPLUGINS="$TSPLUGINS $name"
    done
fi
if [[ -z $TSTOOLS ]]; then
    for file in $TSTOOLSDIR/ts*.cpp; do
        name=$(fbasename $file .cpp)
        [[ " $NO_TSTOOLS " != *" $name "* ]] && TSTOOLS="$TSTOOLS $name"
    done
fi

#-----------------------------------------------------------------------------
# Final output generation.
#-----------------------------------------------------------------------------

for name in $VARNAMES; do
    echo "export $name = ${!name}"
    debug "output: $name='${!name}'"
done

debug-time
