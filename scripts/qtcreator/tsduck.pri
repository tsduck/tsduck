#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Common definitions for using qmake.
#  Defined only for Unix systems (Linux, macOS).
#
#-----------------------------------------------------------------------------

# Do not use Qt, we just use Qt Creator as a general-purpose C++ IDE.
CONFIG *= thread
CONFIG *= largefile
CONFIG += c++2a # should be c++20 but does not work
CONFIG *= unversioned_libname
CONFIG *= no_plugin_name_prefix
CONFIG -= qt
CONFIG -= debug_and_release
CONFIG -= app_bundle
CONFIG += sdk_no_version_check
DEFINES -= UNICODE

# Define the symbol DEBUG in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG

# Project directories.
PROJROOT = $$_PRO_FILE_PWD_/../../..
SRCROOT  = $$PROJROOT/src

# Enforce compilation warnings.
CONFIG -= warn_off
CONFIG *= warn_on

# The other system-specific directories to exclude
linux: NOSYSDIR = mac
mac:   NOSYSDIR = linux

# Shared object suffix
linux: SO = .so
mac:   SO = .dylib

# Locate Python (build is started with minimal environment, clearing current PATH).
linux: ALLPYTHON = $$files(/usr/bin/python3) \
                   $$files(/usr/local/bin/python3) \
                   $$files(/usr/bin/python) \
                   $$files(/usr/local/bin/python)
mac:   ALLPYTHON = $$files(/usr/local/bin/python3) \
                   $$files(/usr/local/bin/python) \
                   $$files(/opt/homebrew/bin/python3) \
                   $$files(/opt/homebrew/bin/python) \
                   $$files(/usr/bin/python3) \
                   $$files(/usr/bin/python)
equals(ALLPYTHON, ""): PYTHON = python
else: PYTHON = $$first(ALLPYTHON)

# Dektec API
DTAPI_HEADER = $$system($$PROJROOT/scripts/dtapi-config.sh --header)
equals(DTAPI_HEADER, ''): DEFINES += TS_NO_DTAPI=1

# JNI API
QMAKE_CXXFLAGS += $$system($$PROJROOT/scripts/java-config.sh --cflags)

# Other configuration.
LIBS += -ledit -lz
linux|mac|mingw {
    QMAKE_CXXFLAGS_WARN_ON = -Werror -Wall -Wextra -Wno-error=deprecated-declarations
    QMAKE_CXXFLAGS += -fno-strict-aliasing -fstack-protector-all
}
linux|mingw {
    # GCC options. Some of them depend on the compiler version.
    GCC_VERSION = $$system($$QMAKE_CXX " -dumpversion")
    GCC_FIELDS = $$split(GCC_VERSION, ".")
    GCC_MAJOR = $$member(GCC_FIELDS, 0)
    QMAKE_CXXFLAGS_WARN_ON += -Wundef -Wcast-align -Wstrict-null-sentinel -Wformat-security \
        -Wswitch-default -Wuninitialized -Wno-unused-parameter -Wfloat-equal -Wpointer-arith \
        -Wsign-promo -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor \
        -Woverloaded-virtual -Wzero-as-null-pointer-constant
    greaterThan(GCC_MAJOR, 4): QMAKE_CXXFLAGS_WARN_ON += -Wpedantic -Weffc++ -Wshadow
}
linux {
    QMAKE_CXXFLAGS += -I/usr/include/PCSC
    LIBS += -lpcsclite -lrt -latomic -ldl
}
mac {
    # LLVM options. Some of them depend on the compiler version.
    LLVM_VERSION = $$system($$QMAKE_CXX " -dumpversion")
    LLVM_FIELDS = $$split(LLVM_VERSION, ".")
    LLVM_MAJOR = $$member(LLVM_FIELDS, 0)
    QMAKE_CXXFLAGS_WARN_ON += -Weverything -Wno-c++98-compat-pedantic
    greaterThan(LLVM_MAJOR, 11): QMAKE_CXXFLAGS_WARN_ON += -Wno-poison-system-directories
    exists(/usr/local/include): QMAKE_CXXFLAGS += -I/usr/local/include
    exists(/opt/homebrew/include): QMAKE_CXXFLAGS += -I/opt/homebrew/include
    LIBS += -framework PCSC
    exists(/usr/local/lib): LIBS += -L/usr/local/lib
    exists(/opt/homebrew/lib): LIBS += -L/opt/homebrew/lib
}
exists(/usr/include/srt/*.h) | exists(/usr/local/include/srt/*.h) | exists(/opt/homebrew/include/srt/*.h) {
    LIBS += -lsrt
}
else {
    DEFINES += TS_NO_SRT=1
}
exists(/usr/include/librist/*.h) | exists(/usr/local/include/librist/*.h) | exists(/opt/homebrew/include/librist/*.h) {
    LIBS += -lrist
}
else {
    DEFINES += TS_NO_RIST=1
}
tstool {
    # TSDuck tools shall use "CONFIG += tstool"
    CONFIG += libtscore libtsduck config_files
    TEMPLATE = app
    SOURCES += $$SRCROOT/tstools/$${TARGET}.cpp
}
util {
    # TSDuck utils shall use "CONFIG += util"
    CONFIG += libtscore libtsduck config_files
    TEMPLATE = app
    SOURCES += $$SRCROOT/utils/$${TARGET}.cpp
}
tsplugin {
    # TSP plugins shall use "CONFIG += tsplugin"
    CONFIG += libtscore libtsduck plugin
    TEMPLATE = lib
    SOURCES += $$SRCROOT/tsplugins/$${TARGET}.cpp
    QMAKE_POST_LINK += mkdir -p ../tsp $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += cp $${TARGET}$$SO ../tsp $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += mkdir -p ../tsprofiling $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += cp $${TARGET}$$SO ../tsprofiling $$escape_expand(\\n\\t)
}
libtscore {
    # Applications using libtscore shall use "CONFIG += libtscore".
    linux:QMAKE_LFLAGS += -Wl,--rpath=\'\$\$ORIGIN/../libtscore\'
    LIBS = ../libtscore/libtscore$$SO $$LIBS
    PRE_TARGETDEPS += ../libtscore/libtscore$$SO
    DEPENDPATH += ../libtscore
    INCLUDEPATH += $$system("find $$SRCROOT/libtscore -type d ! -name windows ! -name \\*bsd ! -name $$NOSYSDIR ! -name private ! -name __pycache__")
}
libtsduck {
    # Applications using libtsduck shall use "CONFIG += libtsduck".
    CONFIG += libtscore
    linux:QMAKE_LFLAGS += -Wl,--rpath=\'\$\$ORIGIN/../libtsduck\'
    LIBS = ../libtsduck/libtsduck$$SO $$LIBS
    PRE_TARGETDEPS += ../libtsduck/libtsduck$$SO
    DEPENDPATH += ../libtsduck
    INCLUDEPATH += $$system("find $$SRCROOT/libtsduck -type d ! -name windows ! -name \\*bsd ! -name $$NOSYSDIR ! -name private ! -name __pycache__")
}
libtsdektec {
    # Applications using libtsdektec shall use "CONFIG += libtsdektec".
    CONFIG += libtsduck
    linux:QMAKE_LFLAGS += -Wl,--rpath=\'\$\$ORIGIN/../libtsdektec\'
    LIBS = ../libtsdektec/libtsdektec$$SO $$LIBS
    PRE_TARGETDEPS += ../libtsdektec/libtsdektec$$SO
    DEPENDPATH += ../libtsdektec
    INCLUDEPATH += $$system("find $$SRCROOT/libtsdektec -type d ! -name windows ! -name $$NOSYSDIR ! -name private ! -name __pycache__")
}
config_files {
    QMAKE_POST_LINK += cp $$SRCROOT/libtscore/config/*.names $$SRCROOT/libtscore/config/*.xml . $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += cp $$SRCROOT/libtsduck/config/*.names $$SRCROOT/libtsduck/config/*.xml . $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += rm -f *.skeleton.xml $$escape_expand(\\n\\t)
    equals(DTAPI_HEADER, ''): DTAPI_INPUT = /dev/null
    else: DTAPI_INPUT = $$DTAPI_HEADER
    QMAKE_POST_LINK += $$PYTHON $$PROJROOT/scripts/build-dektec-names.py $$DTAPI_INPUT tsduck.dektec.names $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$PYTHON $$PROJROOT/scripts/build-dtv-names.py \
                       tsduck.dtv.names \
                       $$SRCROOT/libtsduck/dtv \
                       $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$PYTHON $$PROJROOT/scripts/build-tables-model.py tsduck.tables.model.xml \
                       $$SRCROOT/libtsduck/config/tsduck.tables.skeleton.xml \
                       $$SRCROOT/libtsduck/dtv/tables/*/*.xml \
                       $$SRCROOT/libtsduck/dtv/descriptors/*/*.xml \
                       $$SRCROOT/libtsduck/dtv/dsmcc/*.xml \
                       $$escape_expand(\\n\\t)
}
