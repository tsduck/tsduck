CONFIG += libtsduck
include(../tsduck.pri)
TEMPLATE = app
TARGET = utest

QMAKE_POST_LINK += cp ../tsplugin_merge/tsplugin_merge$$SO . $$escape_expand(\\n\\t)

HEADERS += $$files($$SRCROOT/utest/*.h) $$files($$SRCROOT/utest/*/*.h)
SOURCES += $$files($$SRCROOT/utest/utest*.cpp) $$SRCROOT/utest/tsunit.cpp

DISTFILES += $$SRCROOT/utest/tables/ts2headers.sh $$SRCROOT/utest/tables/psi_all.xml
