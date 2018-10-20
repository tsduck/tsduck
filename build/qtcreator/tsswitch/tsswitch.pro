CONFIG += tstool
TARGET = tsswitch
include(../tsduck.pri)

SOURCES += \
    ../../../src/tstools/tsswitchOptions.cpp \
    ../../../src/tstools/tsswitchInputExecutor.cpp \
    ../../../src/tstools/tsswitchOutputExecutor.cpp \
    ../../../src/tstools/tsswitchCommandListener.cpp \
    ../../../src/tstools/tsswitchCore.cpp

HEADERS += \
    ../../../src/tstools/tsswitchOptions.h \
    ../../../src/tstools/tsswitchInputExecutor.h \
    ../../../src/tstools/tsswitchOutputExecutor.h \
    ../../../src/tstools/tsswitchCommandListener.h \
    ../../../src/tstools/tsswitchCore.h
