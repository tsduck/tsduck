CONFIG += tstool
TARGET = tsp
include(../tsduck.pri)

SOURCES += \
    ../../../src/tstools/tspControlServer.cpp \
    ../../../src/tstools/tspInputExecutor.cpp \
    ../../../src/tstools/tspJointTermination.cpp \
    ../../../src/tstools/tspOptions.cpp \
    ../../../src/tstools/tspOutputExecutor.cpp \
    ../../../src/tstools/tspPluginExecutor.cpp \
    ../../../src/tstools/tspProcessorExecutor.cpp

HEADERS += \
    ../../../src/tstools/tspControlServer.h \
    ../../../src/tstools/tspInputExecutor.h \
    ../../../src/tstools/tspJointTermination.h \
    ../../../src/tstools/tspOptions.h \
    ../../../src/tstools/tspOutputExecutor.h \
    ../../../src/tstools/tspPluginExecutor.h \
    ../../../src/tstools/tspProcessorExecutor.h
