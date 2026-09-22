CONFIG += libtscore libtsduck config_files
include(../tsduck.pri)
TEMPLATE = app
TARGET = utest

HEADERS += $$files($$SRCROOT/utest/*.h) $$files($$SRCROOT/utest/*/*.h)
SOURCES += $$files($$SRCROOT/utest/utest*.cpp) $$SRCROOT/utest/tsunit.cpp

DISTFILES += $$SRCROOT/utest/tables/ts2headers.sh $$SRCROOT/utest/tables/psi_all.xml
