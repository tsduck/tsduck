CONFIG += libtsduck
include(../tsduck.pri)
TEMPLATE = app
TARGET = utest

QMAKE_POST_LINK += cp ../tsplugin_skip/tsplugin_skip.so . $$escape_expand(\\n\\t)

HEADERS += $$system(find $$SRCROOT/utest -name \\*.h)
SOURCES += $$system(find $$SRCROOT/utest -name \\*.cpp)

DISTFILES += $$SRCROOT/utest/tables/ts2headers.sh $$SRCROOT/utest/tables/psi_all.xml
