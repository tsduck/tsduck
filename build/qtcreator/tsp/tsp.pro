CONFIG += libtsduck
include(../tsduck.pri)
TEMPLATE = app
TARGET = tsp
QMAKE_POST_LINK += cp ../tsplugin_*/tsplugin_*.so . $$escape_expand(\\n\\t)

SOURCES += \
    ../../../src/tstools/tsp.cpp \
    ../../../src/tstools/tspInputExecutor.cpp \
    ../../../src/tstools/tspJointTermination.cpp \
    ../../../src/tstools/tspListProcessors.cpp \
    ../../../src/tstools/tspOptions.cpp \
    ../../../src/tstools/tspOutputExecutor.cpp \
    ../../../src/tstools/tspPluginExecutor.cpp \
    ../../../src/tstools/tspProcessorExecutor.cpp

HEADERS += \
    ../../../src/tstools/tspInputExecutor.h \
    ../../../src/tstools/tspJointTermination.h \
    ../../../src/tstools/tspListProcessors.h \
    ../../../src/tstools/tspOptions.h \
    ../../../src/tstools/tspOutputExecutor.h \
    ../../../src/tstools/tspPluginExecutor.h \
    ../../../src/tstools/tspProcessorExecutor.h
