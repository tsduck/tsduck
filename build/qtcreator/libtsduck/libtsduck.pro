CONFIG += plugin
include(../tsduck.pri)
TEMPLATE = lib
TARGET = tsduck
QMAKE_CXXFLAGS += -I$$SRCROOT/libtsduck/private
INCLUDEPATH += $$SRCROOT/libtsduck/private
linux:QMAKE_LFLAGS += -Wl,-soname=tsduck.so
mac:QMAKE_POST_LINK += install_name_tool -id $$OUT_PWD/tsduck.so $$OUT_PWD/tsduck.so $$escape_expand(\\n\\t)

linux|mac {
    QMAKE_CXXFLAGS += $$system("curl-config --cflags")
    QMAKE_LFLAGS += $$system("curl-config --libs")
}

DISTFILES += \
    ../../../src/libtsduck/tsduck.dvb.names \
    ../../../src/libtsduck/tsduck.oui.names \
    ../../../src/libtsduck/tsduck.xml \
    ../../../src/libtsduck/tsduck.mk

include(libtsduck-files.pri)
