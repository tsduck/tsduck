CONFIG += plugin
include(../tsduck.pri)
TEMPLATE = lib
TARGET = libtscore
INCLUDEPATH += $$system("find $$SRCROOT/libtscore -type d ! -name windows ! -name \\*bsd ! -name $$NOSYSDIR ! -name __pycache__")

exists(/opt/homebrew/opt/curl/bin/curl-config) {
    CURLCONFIG = /opt/homebrew/opt/curl/bin/curl-config
}
else {
    CURLCONFIG = curl-config
}

mac {
    QMAKE_CXXFLAGS += -I/opt/homebrew/opt/libedit/include -I/opt/homebrew/opt/zlib-ng-compat/include
    LIBS += -L/opt/homebrew/opt/libedit/lib -L/opt/homebrew/opt/zlib-ng-compat/lib
}

linux|mac {
    QMAKE_CXXFLAGS += $$system("$$CURLCONFIG --cflags")
    LIBS += $$system("$$CURLCONFIG --libs")
    LIBS += -lssl -lcrypto -ledit -lz
}

mac {
    QMAKE_POST_LINK += install_name_tool -id $$OUT_PWD/libtscore$$SO $$OUT_PWD/libtscore$$SO $$escape_expand(\\n\\t)
}

HEADERS += $$system("find $$SRCROOT/libtscore -name \\*.h ! -path \\*/windows/\\* ! -path \\*/\\*bsd/\\* ! -path \\*/$$NOSYSDIR/\\*")
SOURCES += $$system("find $$SRCROOT/libtscore -name \\*.cpp ! -path \\*/windows/\\* ! -path \\*/\\*bsd/\\* ! -path \\*/$$NOSYSDIR/\\* ! -name tscore.cpp")
