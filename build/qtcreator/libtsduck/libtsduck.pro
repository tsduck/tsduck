CONFIG += plugin
include(../tsduck.pri)
TEMPLATE = lib
TARGET = libtsduck
INCLUDEPATH += $$system("find $$SRCROOT/libtsduck -type d ! -name windows ! -name $$NOSYSDIR ! -name release\\* ! -name debug\\*")

linux|mac {
    QMAKE_CXXFLAGS += $$system("curl-config --cflags")
    QMAKE_LFLAGS += $$system("curl-config --libs")
}

linux {
    LIBS += $$system("$$PROJROOT/dektec/dtapi-config.sh --object")
    QMAKE_CXXFLAGS += -isystem $$PROJROOT/dektec/LinuxSDK/DTAPI/Include
}

mac {
    QMAKE_POST_LINK += install_name_tool -id $$OUT_PWD/libtsduck.so $$OUT_PWD/libtsduck.so $$escape_expand(\\n\\t)
}


DISTFILES += $$TS_CONFIG_FILES
HEADERS   += $$system("find $$SRCROOT/libtsduck -name \\*.h ! -path \\*/windows/\\* ! -path \\*/$$NOSYSDIR/\\* ! -path \\*/release\\* ! -path \\*/debug\\*")
SOURCES   += $$system("find $$SRCROOT/libtsduck -name \\*.cpp ! -path \\*/windows/\\* ! -path \\*/$$NOSYSDIR/\\* ! -path \\*/release\\* ! -path \\*/debug\\*")
