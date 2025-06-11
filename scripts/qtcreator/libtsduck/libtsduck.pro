CONFIG += plugin libtscore
include(../tsduck.pri)
TEMPLATE = lib
TARGET = libtsduck
INCLUDEPATH += $$system("find $$SRCROOT/libtsduck -type d ! -name windows ! -name \\*bsd ! -name $$NOSYSDIR ! -name __pycache__")

linux|mac {
    LIBS += $$system("$$PROJROOT/scripts/vatek-config.sh --ldlibs")
}

mac {
    QMAKE_POST_LINK += install_name_tool -id $$OUT_PWD/libtsduck$$SO $$OUT_PWD/libtsduck$$SO $$escape_expand(\\n\\t)
}

QMAKE_CXXFLAGS += $$system("$$PROJROOT/scripts/java-config.sh --cflags")
QMAKE_CXXFLAGS += $$system("$$PROJROOT/scripts/vatek-config.sh --cflags")

HEADERS += $$system("find $$SRCROOT/libtsduck -name \\*.h ! -path \\*/windows/\\* ! -path \\*/\\*bsd/\\* ! -path \\*/$$NOSYSDIR/\\*")
SOURCES += $$system("find $$SRCROOT/libtsduck -name \\*.cpp ! -path \\*/windows/\\* ! -path \\*/\\*bsd/\\* ! -path \\*/$$NOSYSDIR/\\* ! -name tsduck.cpp")
