//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::Tuner.
//
//  Since this test suite requires some hardware, it cannot be executed
//  in a deterministic way. So, these tests are merely template tests which
//  are manually activated using environment variables.
//
//----------------------------------------------------------------------------

#include "tsTuner.h"
#include "tsSignalState.h"
#include "tsDuckContext.h"
#include "tsTSScanner.h"
#include "tsService.h"
#include "tsHFBand.h"
#include "tsEnvironment.h"
#include "tsCOM.h"
#include "tsunit.h"
#if defined(TS_LINUX)
#include "tsDTVProperties.h"
#endif

// Tuners are not supported in macOS and BSD systems. Errors are always returned.
#if !defined(TS_MAC) && !defined(TS_BSD)

//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TunerTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(ListTuners);
    TSUNIT_DECLARE_TEST(ScanDVBT);
    TSUNIT_DECLARE_TEST(SignalState);
#if defined(TS_LINUX)
    TSUNIT_DECLARE_TEST(DTVProperties);
#endif

private:
    ts::COM _com {}; // required in Windows only
};

TSUNIT_REGISTER(TunerTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(ListTuners)
{
    ts::DuckContext duck;
    ts::TunerPtrVector tuners;

    TSUNIT_ASSERT(ts::Tuner::GetAllTuners(duck, tuners));
    debug() << "TunerTest::testListTuners: found " << tuners.size() << " tuners" << std::endl;
    for (size_t i = 0; i < tuners.size(); ++i) {
        debug() << "  Tuner #" << i << ": name: \"" << tuners[i]->deviceName() << "\"" << std::endl;
        debug() << "            info: \"" << tuners[i]->deviceInfo() << "\"" << std::endl;
        debug() << "            path: \"" << tuners[i]->devicePath() << "\"" << std::endl;
        debug() << "            type: " << tuners[i]->deliverySystems() << std::endl;
    }
}

TSUNIT_DEFINE_TEST(ScanDVBT)
{
    // Environment variables to run this test:
    //   TS_TEST_SCAN_DVBT : test not run if empty or undefined
    //   TS_TEST_TUNER : optional tuner device name
    //   TS_TEST_SCAN_REOPEN : if non-empty, close and reopen tuner before each scan
    //   TS_TEST_SCAN_UHF : comma-separated list of UHF channels to scan

    if (ts::GetEnvironment(u"TS_TEST_SCAN_DVBT").empty()) {
        return;
    }

    ts::DuckContext duck;
    ts::Tuner tuner(duck);
    ts::ModulationArgs args;
    const ts::UString name(ts::GetEnvironment(u"TS_TEST_TUNER"));
    const bool reopen = !ts::GetEnvironment(u"TS_TEST_SCAN_REOPEN").empty();

    std::vector<uint32_t> channels;
    ts::GetEnvironment(u"TS_TEST_SCAN_UHF").toIntegers(channels);
    debug() << "TunerTest::testScanDVBT: scanning " << channels.size() << " UHF channels on tuner \"" << name << "\"" << std::endl;

    for (size_t i = 0; i < channels.size(); ++i) {

        if (i == 0 || reopen) {
            debug() << " opening tuner " << name << std::endl;
            TSUNIT_ASSERT(tuner.open(name, false));
            TSUNIT_ASSERT(tuner.isOpen());
        }

        debug() << "  scanning channel " << channels[i] << std::endl;
        args.clear();
        args.delivery_system = ts::DS_DVB_T;
        args.frequency = duck.uhfBand()->frequency(channels[i]);
        args.setDefaultValues();
        debug() << "  tuning options: " << args.toPluginOptions() << std::endl;

        TSUNIT_ASSERT(tuner.tune(args));
        ts::TSScanner scan(duck, tuner, cn::milliseconds(5000));

        ts::ServiceList services;
        scan.getServices(services);
        debug() << "  found " << services.size() << " services" << std::endl;

        for (const auto& srv : services) {
            debug() << "  service " << srv.getName() << ", LCN " << srv.getLCN() << std::endl;
        }

        if (i == channels.size() - 1 || reopen) {
            debug() << " closing tuner " << std::endl;
            TSUNIT_ASSERT(tuner.close());
            TSUNIT_ASSERT(!tuner.isOpen());
        }
    }
}

TSUNIT_DEFINE_TEST(SignalState)
{
    TSUNIT_EQUAL(u"12,345", ts::SignalState::Value(12345).toString());
    TSUNIT_EQUAL(u"48%", ts::SignalState::Value(48, ts::SignalState::Unit::PERCENT).toString());
    TSUNIT_EQUAL(u"-12%", ts::SignalState::Value(-12, ts::SignalState::Unit::PERCENT).toString());
    TSUNIT_EQUAL(u"0 dB", ts::SignalState::Value(0, ts::SignalState::Unit::MDB).toString());
    TSUNIT_EQUAL(u"-2.1 dB", ts::SignalState::Value(-2100, ts::SignalState::Unit::MDB).toString());
    TSUNIT_EQUAL(u"12.345 dB", ts::SignalState::Value(12345, ts::SignalState::Unit::MDB).toString());
}

#if defined(TS_LINUX)
TSUNIT_DEFINE_TEST(DTVProperties)
{
    debug() << "TunerTest::testDTVProperties:" << std::endl;
    for (uint32_t cmd = 0; cmd <= DTV_MAX_COMMAND + 2; cmd++) {
        const char* name = ts::DTVProperties::CommandName(cmd);
        debug() << "  " << cmd << ": " << (name == nullptr ? "(null)" : name) << std::endl;
    }
}
#endif // TS_LINUX

#endif // not TS_MAC
