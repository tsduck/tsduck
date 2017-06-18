CONFIG += libtsduck
include(../tsduck.pri)
TEMPLATE = app
TARGET = tsgentab
SOURCES += ../../../src/tstools/tsgentab.cpp
QMAKE_POST_LINK += cp ../tsgentab_*/tsgentab_*.so . $$escape_expand(\\n\\t)
