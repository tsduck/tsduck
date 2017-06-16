#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2017, Thierry Lelegard
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
CONFIG -= qt
CONFIG -= debug_and_release
DEFINES -= UNICODE

# Define the symbol DEBUG in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG

# Project directories.
PROJROOT = $$_PRO_FILE_PWD_/../../..
SRCROOT = $$PROJROOT/src

# Currently do not use DTAPI with Qt Creator.
DEFINES += TS_NO_DTAPI=1

# Enforce compilation warnings.
CONFIG -= warn_off
CONFIG *= warn_on

# Make sure header files are found.
unix:QMAKE_CXXFLAGS += -I/usr/include/PCSC
unix:QMAKE_CXXFLAGS += -I/usr/include/PCSC
QMAKE_CXXFLAGS += -I$$SRCROOT/libtsduck -I$$SRCROOT/libtsduck/private
unix:QMAKE_CXXFLAGS += -I$$SRCROOT/libtsduck/linux
INCLUDEPATH += $$SRCROOT/libtsduck $$SRCROOT/libtsduck/private
unix:INCLUDEPATH += $$SRCROOT/libtsduck

# GCC specific options.
unix|mingw|gcc {
    QMAKE_CXXFLAGS_WARN_ON = -Werror -Wall -Wextra \
        -Wpedantic -Wformat-nonliteral -Wformat-security -Wswitch-default -Wuninitialized \
        -Wshadow -Wno-unused-parameter -Wfloat-equal -Wundef -Wpointer-arith -Wcast-align \
        -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor -Weffc++ -Woverloaded-virtual \
        -Wsign-promo -Wstrict-null-sentinel
    QMAKE_CXXFLAGS += -fno-strict-aliasing -fstack-protector-all
}

# Applications using libtsduck shall use "CONFIG += libtsduck".
libtsduck {
    LIBS += -L../libtsduck -ltsduck -lpcsclite
    PRE_TARGETDEPS += ../libtsduck/libtsduck.so
    INCLUDEPATH += ../libtsduck
    DEPENDPATH += ../libtsduck
}
