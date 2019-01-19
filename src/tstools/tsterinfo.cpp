//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  DVB-T (terrestrial) information utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsTunerParametersBitrateDiffDVBT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Lists of possible real values for DVB-T modulation parameters
//  (exclude "auto" and "unspecified" values).
//----------------------------------------------------------------------------

namespace {

    const ts::Enumeration DVBTModulationEnum({
        {u"QPSK",   ts::QPSK},
        {u"16-QAM", ts::QAM_16},
        {u"64-QAM", ts::QAM_64},
    });

    const ts::Enumeration DVBTHPFECEnum({
        {u"1/2",  ts::FEC_1_2},
        {u"2/3",  ts::FEC_2_3},
        {u"3/4",  ts::FEC_3_4},
        {u"5/6",  ts::FEC_5_6},
        {u"7/8",  ts::FEC_7_8},
    });

    const ts::Enumeration DVBTBandWidthEnum({
        {u"8-MHz", ts::BW_8_MHZ},
        {u"7-MHz", ts::BW_7_MHZ},
        {u"6-MHz", ts::BW_6_MHZ},
        {u"5-MHz", ts::BW_5_MHZ},
    });

    const ts::Enumeration DVBTGuardIntervalEnum({
        {u"1/32", ts::GUARD_1_32},
        {u"1/16", ts::GUARD_1_16},
        {u"1/8",  ts::GUARD_1_8},
        {u"1/4",  ts::GUARD_1_4},
    });
}


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);
    virtual ~Options();

    uint64_t          frequency;     // Carrier frequency from which to get UHF channel
    int               uhf_channel;   // UHF channel from which to compute frequency
    int               vhf_channel;   // VHF channel from which to compute frequency
    int               hf_offset;     // UHF/VHF offset from channel
    ts::BitRate       bitrate;       // TS bitrate from which to guess modulation parameters
    size_t            max_guess;     // Max number of modulation parameters to guess.
    ts::Modulation    constellation; // Modulation parameters to compute bitrate
    ts::InnerFEC      fec_hp;
    ts::GuardInterval guard_interval;
    ts::BandWidth     bandwidth;
    bool              simple;        // Simple output
};

// Destructor.
Options::~Options() {}

// Constructor.
Options::Options(int argc, char *argv[]) :
    Args(u"Compute or convert DVB-Terrestrial information", u"[options]"),
    frequency(0),
    uhf_channel(0),
    vhf_channel(0),
    hf_offset(0),
    bitrate(0),
    max_guess(0),
    constellation(ts::QAM_AUTO),
    fec_hp(ts::FEC_NONE),
    guard_interval(ts::GUARD_AUTO),
    bandwidth(ts::BW_AUTO),
    simple(false)
{
    option(u"bandwidth", 'w', DVBTBandWidthEnum);
    help(u"bandwidth", u"Specify the OFMD bandwith, used to compute the resulting bitrate.");

    option(u"bitrate", 'b', UINT32);
    help(u"bitrate",
         u"Transport stream bitrate in b/s, based on 188-byte packets. Given this "
         u"bitrate, tsterinfo will try to guess the OFDM modulation parameters.");

    option(u"constellation", 'c', DVBTModulationEnum);
    help(u"constellation", u"Specify the OFMD constellation, used to compute the resulting bitrate.");

    option(u"frequency", 'f', UNSIGNED);
    help(u"frequency", u"Carrier frequency in Hz. UHF or VHF channel and offset will be displayed.");

    option(u"guard-interval", 'g', DVBTGuardIntervalEnum);
    help(u"guard-interval",
         u"Specify the OFMD guard interval, used to compute the resulting bitrate.");

    option(u"high-priority-fec", 'h', DVBTHPFECEnum);
    help(u"high-priority-fec",
         u"Specify the OFMD error correction for high priority streams, "
         u"used to compute the resulting bitrate.");

    option(u"max-guess", 'm', POSITIVE);
    help(u"max-guess",
         u"When used with --bitrate, specify the maximum number of modulation "
         u"parameters sets to display. By default, display one set of parameters, "
         u"the one giving the closest bitrate.");

    option(u"offset-count", 'o', INTEGER, 0, 1, -3, 3);
    help(u"offset-count",
         u"Specify the number of offsets from the UHF or VHF channel. The default "
         u"is zero. See options --uhf-channel and --vhf-channel.");

    option(u"simple", 's');
    help(u"simple",
         u"Produce simple output: only numbers, no comment, typically useful "
         u"to write scripts.");

    option(u"uhf-channel", 'u', INTEGER, 0, 1, ts::UHF::FIRST_CHANNEL, ts::UHF::LAST_CHANNEL);
    help(u"uhf-channel",
         u"Specify the UHF channel number of the carrier. Can be combined with an "
         u"--offset-count option. The resulting frequency will be displayed.");

    option(u"vhf-channel", 'v', INTEGER, 0, 1, ts::VHF::FIRST_CHANNEL, ts::VHF::LAST_CHANNEL);
    help(u"vhf-channel",
         u"Specify the VHF channel number of the carrier. Can be combined with an "
         u"--offset-count option. The resulting frequency will be displayed.");

    analyze(argc, argv);

    frequency      = intValue<uint64_t>(u"frequency", 0);
    uhf_channel    = intValue<int>(u"uhf-channel", 0);
    vhf_channel    = intValue<int>(u"vhf-channel", 0);
    hf_offset      = intValue<int>(u"offset-count", 0);
    bitrate        = intValue<ts::BitRate>(u"bitrate", 0);
    max_guess      = intValue<ts::BitRate>(u"max-guess", 1);
    constellation  = enumValue(u"constellation", ts::QAM_64);
    fec_hp         = enumValue(u"high-priority-fec", ts::FEC_AUTO);
    guard_interval = enumValue(u"guard-interval", ts::GUARD_AUTO);
    bandwidth      = enumValue(u"bandwidth", ts::BW_8_MHZ);
    simple         = present(u"simple");

    if ((fec_hp == ts::FEC_AUTO && guard_interval != ts::GUARD_AUTO) ||
        (fec_hp != ts::FEC_AUTO && guard_interval == ts::GUARD_AUTO))
    {
        error(u"specify either both --guard-interval and --high-priority-fec value or none");
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  This routine displays a name/value pair
//----------------------------------------------------------------------------

namespace {
    void Display(const ts::UString& name, const ts::UString& value, const ts::UString& unit)
    {
        std::cout << "  " << name.toJustified(value, 37, u'.', 1) << " " << unit << std::endl;
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);

    // Convert UHF channel to frequency
    if (opt.uhf_channel > 0) {
        if (opt.simple) {
            std::cout << ts::UHF::Frequency(opt.uhf_channel, opt.hf_offset) << std::endl;
        }
        else {
            std::cout << "Carrier Frequency: "
                      << ts::UString::Decimal(ts::UHF::Frequency(opt.uhf_channel, opt.hf_offset))
                      << " Hz" << std::endl;
        }
    }

    // Convert VHF channel to frequency
    if (opt.vhf_channel > 0) {
        if (opt.simple) {
            std::cout << ts::VHF::Frequency(opt.vhf_channel, opt.hf_offset) << std::endl;
        }
        else {
            std::cout << "Carrier Frequency: "
                      << ts::UString::Decimal(ts::VHF::Frequency(opt.vhf_channel, opt.hf_offset))
                      << " Hz" << std::endl;
        }
    }

    // Convert frequency to UHF/VHF channel
    if (opt.frequency > 0) {
        if (ts::UHF::InBand(opt.frequency)) {
            if (opt.simple) {
                std::cout << ts::UHF::Channel(opt.frequency) << std::endl
                          << ts::UHF::OffsetCount(opt.frequency) << std::endl;
            }
            else {
                int channel = ts::UHF::Channel(opt.frequency);
                int offset = ts::UHF::OffsetCount(opt.frequency);
                std::cout << "UHF channel: " << channel << ", offset: " << offset << std::endl;
                uint64_t exact_freq = ts::UHF::Frequency(channel, offset);
                int diff = int(int64_t(opt.frequency) - int64_t(exact_freq));
                if (::abs(diff) > 1) {
                    std::cout << "Warning: exact frequency for channel "
                              << channel << ", offset " << offset << " is "
                              << ts::UString::Decimal(exact_freq) << " Hz, differ by "
                              << ts::UString::Decimal(diff) << " Hz" << std::endl;
                }
            }
        }
        else if (ts::VHF::InBand(opt.frequency)) {
            if (opt.simple) {
                std::cout << ts::VHF::Channel(opt.frequency) << std::endl
                          << ts::VHF::OffsetCount(opt.frequency) << std::endl;
            }
            else {
                int channel = ts::VHF::Channel(opt.frequency);
                int offset = ts::VHF::OffsetCount(opt.frequency);
                std::cout << "VHF channel: " << channel << ", offset: " << offset << std::endl;
                uint64_t exact_freq = ts::VHF::Frequency(channel, offset);
                int diff = int(int64_t(opt.frequency) - int64_t(exact_freq));
                if (::abs(diff) > 1) {
                    std::cout << "Warning: exact frequency for channel "
                              << channel << ", offset " << offset << " is "
                              << ts::UString::Decimal(exact_freq) << " Hz, differ by "
                              << ts::UString::Decimal(diff) << " Hz" << std::endl;
                }
            }
        }
        else {
            std::cerr << ts::UString::Decimal(opt.frequency) << " Hz is not in UHF or VHF bands (VHF: "
                      << ts::UString::Decimal(ts::VHF::Frequency(ts::VHF::FIRST_CHANNEL, -3)) << " - "
                      << ts::UString::Decimal(ts::VHF::Frequency(ts::VHF::LAST_CHANNEL, 3)) << ", UHF: "
                      << ts::UString::Decimal(ts::UHF::Frequency(ts::UHF::FIRST_CHANNEL, -3)) << " - "
                      << ts::UString::Decimal(ts::UHF::Frequency(ts::UHF::LAST_CHANNEL, 3)) << ")"
                      << std::endl;
        }
    }

    // Compute TS bitrate from modulation parameters
    if (opt.fec_hp != ts::FEC_AUTO && opt.guard_interval != ts::GUARD_AUTO) {
        ts::TunerParametersDVBT params;
        params.bandwidth = opt.bandwidth;
        params.fec_hp = opt.fec_hp;
        params.modulation = opt.constellation;
        params.guard_interval = opt.guard_interval;
        if (opt.simple) {
            std::cout << params.theoreticalBitrate() << std::endl;
        }
        else {
            std::cout << "Transport stream bitrate: "
                      << ts::UString::Decimal(params.theoreticalBitrate())
                      << " b/s" << std::endl;
        }
    }

    // Guess possible modulation parameters from bitrate
    if (opt.bitrate > 0) {

        // Build a list of all possible modulation parameters for this bitrate.
        ts::TunerParametersBitrateDiffDVBTList params_list;
        ts::TunerParametersBitrateDiffDVBT::EvaluateToBitrate(params_list, opt.bitrate);

        // Display all relevant parameters, up to max_guess
        // (in case of equal differences, display them all)
        int last_diff = 0;
        size_t count = 0;
        for (ts::TunerParametersBitrateDiffDVBTList::const_iterator it = params_list.begin();
             it != params_list.end() && (count < opt.max_guess || ::abs(it->bitrate_diff) == ::abs(last_diff));
             ++it, ++count) {

            last_diff = it->bitrate_diff;
            if (opt.simple) {
                std::cout << it->theoreticalBitrate() << std::endl
                          << ts::BandWidthEnum.name(it->bandwidth) << std::endl
                          << ts::InnerFECEnum.name(it->fec_hp) << std::endl
                          << ts::ModulationEnum.name(it->modulation) << std::endl
                          << ts::GuardIntervalEnum.name(it->guard_interval) << std::endl;
            }
            else {
                if (count > 0) {
                    std::cout << std::endl;
                }
                Display(u"Nominal bitrate", ts::UString::Decimal(it->theoreticalBitrate()), u"b/s");
                Display(u"Bitrate difference", ts::UString::Decimal(it->bitrate_diff), u"b/s");
                Display(u"Bandwidth", ts::BandWidthEnum.name(it->bandwidth), u"");
                Display(u"FEC (high priority)", ts::InnerFECEnum.name(it->fec_hp), u"");
                Display(u"Constellation", ts::ModulationEnum.name(it->modulation), u"");
                Display(u"Guard interval", ts::GuardIntervalEnum.name(it->guard_interval), u"");
            }
        }
    }

    return EXIT_SUCCESS;
}

TS_MAIN(MainCode)
