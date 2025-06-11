CONFIG += plugin libtscore libtsduck
include(../tsduck.pri)
TEMPLATE = lib
TARGET = libtsdektec
INCLUDEPATH += $$system("find $$SRCROOT/libtsdektec -type d ! -name windows ! -name $$NOSYSDIR ! -name __pycache__")

linux {
    QMAKE_CXXFLAGS += -isystem $$system("$$PROJROOT/scripts/dtapi-config.sh --include")
    OBJECTS += $$system("$$PROJROOT/scripts/dtapi-config.sh --object")
}

mac {
    QMAKE_POST_LINK += install_name_tool -id $$OUT_PWD/libtsdektec$$SO $$OUT_PWD/libtsdektec$$SO $$escape_expand(\\n\\t)
}

HEADERS += $$system("find $$SRCROOT/libtsdektec -name \\*.h ! -path \\*/windows/\\* ! -path \\*/$$NOSYSDIR/\\*")
SOURCES += $$system("find $$SRCROOT/libtsdektec -name \\*.cpp ! -path \\*/windows/\\* ! -path \\*/$$NOSYSDIR/\\*")
