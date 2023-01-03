CONFIG += plugin
include(../tsduck.pri)
TEMPLATE = lib
TARGET = libtsduck
INCLUDEPATH += $$system("find $$SRCROOT/libtsduck -type d ! -name windows ! -name bsd ! -name freebsd ! -name netbsd ! -name openbsd ! -name dragonflybsd ! -name $$NOSYSDIR ! -name __pycache__")

linux|mac {
    QMAKE_CXXFLAGS += $$system("curl-config --cflags")
    LIBS += $$system("curl-config --libs")
    LIBS += $$system("$$PROJROOT/scripts/vatek-config.sh --ldlibs")
}

linux {
    QMAKE_CXXFLAGS += -isystem $$system("$$PROJROOT/scripts/dtapi-config.sh --include")
    OBJECTS += $$system("$$PROJROOT/scripts/dtapi-config.sh --object")
}

mac {
    QMAKE_POST_LINK += install_name_tool -id $$OUT_PWD/libtsduck$$SO $$OUT_PWD/libtsduck$$SO $$escape_expand(\\n\\t)
}

QMAKE_CXXFLAGS += $$system("$$PROJROOT/scripts/java-config.sh --cflags")
QMAKE_CXXFLAGS += $$system("$$PROJROOT/scripts/vatek-config.sh --cflags")

DISTFILES += $$TS_CONFIG_FILES
HEADERS   += $$system("find $$SRCROOT/libtsduck -name \\*.h ! -path \\*/windows/\\* ! -path \\*/bsd/\\* ! -path \\*/freebsd/\\* ! -path \\*/netbsd/\\* ! -path \\*/openbsd/\\* ! -path \\*/dragonflybsd/\\* ! -path \\*/$$NOSYSDIR/\\*")
SOURCES   += $$system("find $$SRCROOT/libtsduck -name \\*.cpp ! -path \\*/windows/\\* ! -path \\*/bsd/\\* ! -path \\*/freebsd/\\* ! -path \\*/netbsd/\\* ! -path \\*/openbsd/\\* ! -path \\*/dragonflybsd/\\* ! -path \\*/$$NOSYSDIR/\\* ! -name tsduck.cpp")
