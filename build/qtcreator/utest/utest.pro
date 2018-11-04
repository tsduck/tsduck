CONFIG += libtsduck
include(../tsduck.pri)
TEMPLATE = app
TARGET = utest
LIBS += -lcppunit
QMAKE_POST_LINK += cp ../tsplugin_drop/tsplugin_drop.so . $$escape_expand(\\n\\t)
QMAKE_POST_LINK += cp ../tsplugin_null/tsplugin_null.so . $$escape_expand(\\n\\t)
QMAKE_POST_LINK += cp ../tsplugin_skip/tsplugin_skip.so . $$escape_expand(\\n\\t)

HEADERS += \
    ../../../src/utest/utestCppUnitMain.h \
    ../../../src/utest/utestCppUnitTest.h \
    ../../../src/utest/utestCppUnitThread.h

SOURCES += \
    ../../../src/utest/utest.cpp \
    ../../../src/utest/utestAlgorithm.cpp \
    ../../../src/utest/utestArgs.cpp \
    ../../../src/utest/utestBitStream.cpp \
    ../../../src/utest/utestByteBlock.cpp \
    ../../../src/utest/utestConfig.cpp \
    ../../../src/utest/utestCppUnitMain.cpp \
    ../../../src/utest/utestCppUnitTest.cpp \
    ../../../src/utest/utestCrypto.cpp \
    ../../../src/utest/utestDemux.cpp \
    ../../../src/utest/utestDirectShow.cpp \
    ../../../src/utest/utestDoubleCheckLock.cpp \
    ../../../src/utest/utestDVB.cpp \
    ../../../src/utest/utestDVBCharset.cpp \
    ../../../src/utest/utestEnumeration.cpp \
    ../../../src/utest/utestFatal.cpp \
    ../../../src/utest/utestGrid.cpp \
    ../../../src/utest/utestGuard.cpp \
    ../../../src/utest/utestInterrupt.cpp \
    ../../../src/utest/utestJSON.cpp \
    ../../../src/utest/utestMessageQueue.cpp \
    ../../../src/utest/utestMPEPacket.cpp \
    ../../../src/utest/utestMonotonic.cpp \
    ../../../src/utest/utestMutex.cpp \
    ../../../src/utest/utestNames.cpp \
    ../../../src/utest/utestNetworking.cpp \
    ../../../src/utest/utestPacketizer.cpp \
    ../../../src/utest/utestPlatform.cpp \
    ../../../src/utest/utestPlugin.cpp \
    ../../../src/utest/utestReport.cpp \
    ../../../src/utest/utestResidentBuffer.cpp \
    ../../../src/utest/utestRing.cpp \
    ../../../src/utest/utestSafePtr.cpp \
    ../../../src/utest/utestScrambling.cpp \
    ../../../src/utest/utestSection.cpp \
    ../../../src/utest/utestSectionFile.cpp \
    ../../../src/utest/utestSingleton.cpp \
    ../../../src/utest/utestStaticInstance.cpp \
    ../../../src/utest/utestSystemRandomGenerator.cpp \
    ../../../src/utest/utestSysUtils.cpp \
    ../../../src/utest/utestTable.cpp \
    ../../../src/utest/utestTablesFactory.cpp \
    ../../../src/utest/utestTagLengthValue.cpp \
    ../../../src/utest/utestThread.cpp \
    ../../../src/utest/utestThreadAttributes.cpp \
    ../../../src/utest/utestTime.cpp \
    ../../../src/utest/utestTSPacket.cpp \
    ../../../src/utest/utestUString.cpp \
    ../../../src/utest/utestVariable.cpp \
    ../../../src/utest/utestWebRequest.cpp \
    ../../../src/utest/utestXML.cpp \
    ../../../src/utest/utestHLS.cpp \
    ../../../src/utest/utestCppUnitThread.cpp

DISTFILES += \
    ../../../src/utest/tables/ts2headers.sh \
    ../../../src/utest/tables/psi_all.xml
