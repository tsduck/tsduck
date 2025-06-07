//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for modulation types.
//
//----------------------------------------------------------------------------

#include "tsModulationArgs.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ModulationTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(BitRatesDVBT);
    TSUNIT_DECLARE_TEST(BitRatesDVBS);
    TSUNIT_DECLARE_TEST(BitRatesATSC);
};

TSUNIT_REGISTER(ModulationTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(BitRatesDVBT)
{
    ts::ModulationArgs args;

    args.delivery_system = ts::DS_DVB_T;
    args.bandwidth = 7'000'000;
    args.modulation = ts::QAM_64;
    args.guard_interval = ts::GUARD_1_32;
    args.fec_hp = ts::FEC_7_8;
    TSUNIT_EQUAL(27'709'893, args.theoreticalBitrate().toInt());

    args.delivery_system = ts::DS_DVB_T;
    args.bandwidth = 8'000'000;
    args.modulation = ts::QPSK;
    args.guard_interval = ts::GUARD_1_8;
    args.fec_hp = ts::FEC_3_4;
    TSUNIT_EQUAL(8'294'118, args.theoreticalBitrate().toInt());

    args.delivery_system = ts::DS_DVB_T;
    args.bandwidth = 5'000'000;
    args.modulation = ts::QAM_64;
    args.guard_interval = ts::GUARD_1_16;
    args.fec_hp = ts::FEC_7_8;
    TSUNIT_EQUAL(19'210'640, args.theoreticalBitrate().toInt());
}

TSUNIT_DEFINE_TEST(BitRatesDVBS)
{
    ts::ModulationArgs args;

    args.delivery_system = ts::DS_DVB_S;
    args.modulation = ts::QPSK;
    args.symbol_rate = 27'500'000;
    args.inner_fec = ts::FEC_2_3;
    TSUNIT_EQUAL(33'790'850, args.theoreticalBitrate().toInt());
}

TSUNIT_DEFINE_TEST(BitRatesATSC)
{
    ts::ModulationArgs args;

    args.delivery_system = ts::DS_ATSC;
    args.modulation = ts::VSB_8;
    TSUNIT_EQUAL(19'392'658, args.theoreticalBitrate().toInt());

    args.delivery_system = ts::DS_ATSC;
    args.modulation = ts::VSB_16;
    TSUNIT_EQUAL(38'785'317, args.theoreticalBitrate().toInt());

    args.delivery_system = ts::DS_ATSC;
    args.modulation = ts::QPSK;
    TSUNIT_EQUAL(0, args.theoreticalBitrate().toInt());
}
