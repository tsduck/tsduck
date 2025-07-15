CONFIG += plugin
include(../tsduck.pri)
TEMPLATE = lib
TARGET = libtscore
INCLUDEPATH += $$system("find $$SRCROOT/libtscore -type d ! -name windows ! -name \\*bsd ! -name $$NOSYSDIR ! -name __pycache__")

linux|mac {
    QMAKE_CXXFLAGS += $$system("curl-config --cflags")
    LIBS += $$system("curl-config --libs")
    LIBS += -lssl -lcrypto
}

mac {
    QMAKE_POST_LINK += install_name_tool -id $$OUT_PWD/libtscore$$SO $$OUT_PWD/libtscore$$SO $$escape_expand(\\n\\t)
}

HEADERS += $$system("find $$SRCROOT/libtscore -name \\*.h ! -path \\*/windows/\\* ! -path \\*/\\*bsd/\\* ! -path \\*/$$NOSYSDIR/\\*")
SOURCES += $$system("find $$SRCROOT/libtscore -name \\*.cpp ! -path \\*/windows/\\* ! -path \\*/\\*bsd/\\* ! -path \\*/$$NOSYSDIR/\\* ! -name tscore.cpp")
