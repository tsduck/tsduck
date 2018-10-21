#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2018, Thierry Lelegard
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
#  Common definitions for using qmake.
#
#-----------------------------------------------------------------------------

# Do not use Qt, we just use Qt Creator as a general-purpose C++ IDE.
CONFIG *= thread
CONFIG *= largefile
CONFIG *= c++11
CONFIG *= unversioned_libname
CONFIG *= no_plugin_name_prefix
CONFIG -= qt
CONFIG -= debug_and_release
CONFIG -= app_bundle
DEFINES -= UNICODE

# Define the symbol DEBUG in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG

# Project directories.
PROJROOT = $$_PRO_FILE_PWD_/../../..
SRCROOT  = $$PROJROOT/src

# Currently do not use DTAPI with Qt Creator.
DEFINES += TS_NO_DTAPI=1

# Enforce compilation warnings.
CONFIG -= warn_off
CONFIG *= warn_on

# Other configuration.
QMAKE_CXXFLAGS += -I$$SRCROOT/libtsduck
INCLUDEPATH += $$SRCROOT/libtsduck
linux|mac|mingw {
    # GCC/clang options. Some of them depend on the compiler version.
    GCC_VERSION = $$system($$QMAKE_CXX " -dumpversion")
    GCC_FIELDS = $$split(GCC_VERSION, ".")
    GCC_MAJOR = $$member(GCC_FIELDS, 0)
    QMAKE_CXXFLAGS_WARN_ON = -Werror -Wall -Wextra -Wformat-security -Wswitch-default \
        -Wuninitialized -Wno-unused-parameter -Wfloat-equal -Wpointer-arith -Wsign-promo \
        -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual \
        -Wzero-as-null-pointer-constant
    greaterThan(GCC_MAJOR, 4): QMAKE_CXXFLAGS_WARN_ON += -Wpedantic -Weffc++ -Wshadow
    QMAKE_CXXFLAGS += -fno-strict-aliasing -fstack-protector-all -std=c++11
    QMAKE_CXXFLAGS += $$system(curl-config --cflags)
    LIBS += $$system(curl-config --libs)
}
linux|mingw {
    QMAKE_CXXFLAGS_WARN_ON += -Wundef -Wcast-align -Wstrict-null-sentinel
}
linux {
    QMAKE_CXXFLAGS += -I$$SRCROOT/libtsduck/linux -I/usr/include/PCSC
    INCLUDEPATH += $$SRCROOT/libtsduck/linux
    LIBS += -lrt -ldl
}
mac {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-command-line-argument
    QMAKE_CXXFLAGS += -I$$SRCROOT/libtsduck/mac -I/usr/local/include -I/usr/local/opt/pcsc-lite/include/PCSC
    INCLUDEPATH += $$SRCROOT/libtsduck/mac
    LIBS += -L/usr/local/lib -L/usr/local/opt/pcsc-lite/lib
    QMAKE_EXTENSION_SHLIB = so
}
win32|win64 {
    QMAKE_CXXFLAGS += -I$$SRCROOT/libtsduck/windows
    INCLUDEPATH += $$SRCROOT/libtsduck/windows
}
tstool {
    # TSDuck tools shall use "CONFIG += tstool"
    CONFIG += libtsduck
    TEMPLATE = app
    SOURCES += ../../../src/tstools/$${TARGET}.cpp
}
tsplugin {
    # TSP plugins shall use "CONFIG += tsplugin"
    CONFIG += libtsduck plugin
    TEMPLATE = lib
    SOURCES += ../../../src/tsplugins/$${TARGET}.cpp
    QMAKE_POST_LINK += mkdir -p ../tsp $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += cp $${TARGET}.so ../tsp $$escape_expand(\\n\\t)
}
libtsduck {
    # Applications using libtsduck shall use "CONFIG += libtsduck".
    linux:QMAKE_LFLAGS += -Wl,--rpath=\'\$\$ORIGIN/../libtsduck\'
    LIBS += ../libtsduck/tsduck.so
    PRE_TARGETDEPS += ../libtsduck/tsduck.so
    DEPENDPATH += ../libtsduck
    QMAKE_POST_LINK += cp $$SRCROOT/libtsduck/tsduck.xml . $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += cp $$SRCROOT/libtsduck/tsduck.dvb.names . $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += cp $$SRCROOT/libtsduck/tsduck.oui.names . $$escape_expand(\\n\\t)
}
LIBS += -lpcsclite
