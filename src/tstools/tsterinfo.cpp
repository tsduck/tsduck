//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

#include "tsArgs.h"
#include "tsTunerParametersBitrateDiffDVBT.h"
#include "tsDecimal.h"


using namespace ts;


//----------------------------------------------------------------------------
//  Lists of possible real values for DVB-T modulation parameters
//  (exclude "auto" and "unspecified" values).
//----------------------------------------------------------------------------

namespace {

    const Enumeration DVBTModulationEnum
        ("QPSK",   ts::QPSK,
         "16-QAM", ts::QAM_16,
         "64-QAM", ts::QAM_64,
         TS_NULL);

    const Enumeration DVBTHPFECEnum
        ("1/2",  ts::FEC_1_2,
         "2/3",  ts::FEC_2_3,
         "3/4",  ts::FEC_3_4,
         "5/6",  ts::FEC_5_6,
         "7/8",  ts::FEC_7_8,
         TS_NULL);

    const Enumeration DVBTBandWidthEnum
        ("8-MHz", ts::BW_8_MHZ,
         "7-MHz", ts::BW_7_MHZ,
         "6-MHz", ts::BW_6_MHZ,
         "5-MHz", ts::BW_5_MHZ,
         TS_NULL);

    const Enumeration DVBTGuardIntervalEnum
        ("1/32", ts::GUARD_1_32,
         "1/16", ts::GUARD_1_16,
         "1/8",  ts::GUARD_1_8,
         "1/4",  ts::GUARD_1_4,
         TS_NULL);
}


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public Args
{
    Options (int argc, char *argv[]);

    uint64_t              frequency;     // Carrier frequency from which to get UHF channel
    int                 uhf_channel;   // UHF channel from which to compute frequency
    int                 vhf_channel;   // VHF channel from which to compute frequency
    int                 hf_offset;     // UHF/VHF offset from channel
    BitRate             bitrate;       // TS bitrate from which to guess modulation parameters
    size_t              max_guess;     // Max number of modulation parameters to guess.
    ts::Modulation    constellation; // Modulation parameters to compute bitrate
    ts::InnerFEC      fec_hp;
    ts::GuardInterval guard_interval;
    ts::BandWidth     bandwidth;
    bool                simple;        // Simple output
};

Options::Options (int argc, char *argv[]) :
    Args ("DVB-Terrestrial information utility.", "[options]")
{
    option ("bandwidth",         'w', DVBTBandWidthEnum);
    option ("bitrate",           'b', Args::UINT32);
    option ("constellation",     'c', DVBTModulationEnum);
    option ("frequency",         'f', Args::UNSIGNED);
    option ("guard-interval",    'g', DVBTGuardIntervalEnum);
    option ("high-priority-fec", 'h', DVBTHPFECEnum);
    option ("max-guess",         'm', Args::POSITIVE);
    option ("offset-count",      'o', Args::INTEGER, 0, 1, -3, 3);
    option ("simple",            's');
    option ("uhf-channel",       'u', Args::INTEGER, 0, 1, UHF::FIRST_CHANNEL, UHF::LAST_CHANNEL);
    option ("vhf-channel",       'v', Args::INTEGER, 0, 1, VHF::FIRST_CHANNEL, VHF::LAST_CHANNEL);

    setHelp ("Options:\n"
             "\n"
             "  -w value\n"
             "  --bandwidth value\n"
             "      Specify the OFMD bandwith, used to compute the resulting bitrate.\n"
             "      Must be one of \"8-MHz\", \"7-MHz\", \"6-MHz\", \"5-MHz\" (default: 8-MHz).\n"
             "\n"
             "  -b value\n"
             "  --bitrate value\n"
             "      Transport stream bitrate in b/s, based on 188-byte packets. Given this\n"
             "      bitrate, tsterinfo will try to guess the OFDM modulation parameters.\n"
             "\n"
             "  -c value\n"
             "  --constellation value\n"
             "      Specify the OFMD constellation, used to compute the resulting bitrate.\n"
             "      Must be one of \"QPSK\", \"16-QAM\", \"64-QAM\" (default: 64-QAM).\n"
             "\n"
             "  -f value\n"
             "  --frequency value\n"
             "      Carrier frequency in Hz. UHF or VHF channel and offset will be displayed.\n"
             "\n"
             "  -g value\n"
             "  --guard-interval value\n"
             "      Specify the OFMD guard interval, used to compute the resulting bitrate.\n"
             "      Must be one of \"1/32\", \"1/16\", \"1/8\", \"1/4\" (no default).\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -h value\n"
             "  --high-priority-fec value\n"
             "      Specify the OFMD error correction for high priority streams, used to\n"
             "      compute the resulting bitrate. Must be one of \"1/2\", \"2/3\", \"3/4\",\n"
             "      \"5/6\", \"7/8\" (no default).\n"
             "\n"
             "  -m value\n"
             "  --max-guess value\n"
             "      When used with --bitrate, specify the maximum number of modulation\n"
             "      parameters sets to display. By default, display one set of parameters,\n"
             "      the one giving the closest bitrate.\n"
             "\n"
             "  -o value\n"
             "  --offset-count value\n"
             "      Specify the number of offsets from the UHF or VHF channel. The default\n"
             "      is zero. See options --uhf-channel and --vhf-channel.\n"
             "\n"
             "  -s\n"
             "  --simple\n"
             "      Produce simple output: only numbers, no comment, typically useful\n"
             "      to write scripts.\n"
             "\n"
             "  -u value\n"
             "  --uhf-channel value\n"
             "      Specify the UHF channel number of the carrier. Can be combined with an\n"
             "      --offset-count option. The resulting frequency will be displayed.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n"
             "\n"
             "  -v value\n"
             "  --vhf-channel value\n"
             "      Specify the VHF channel number of the carrier. Can be combined with an\n"
             "      --offset-count option. The resulting frequency will be displayed.\n");

    analyze (argc, argv);

    frequency      = intValue<uint64_t> ("frequency", 0);
    uhf_channel    = intValue<int> ("uhf-channel", 0);
    vhf_channel    = intValue<int> ("vhf-channel", 0);
    hf_offset      = intValue<int> ("offset-count", 0);
    bitrate        = intValue<BitRate> ("bitrate", 0);
    max_guess      = intValue<BitRate> ("max-guess", 1);
    constellation  = ts::Modulation (intValue<int> ("constellation", ts::QAM_64));
    fec_hp         = ts::InnerFEC (intValue<int>("high-priority-fec", ts::FEC_AUTO));
    guard_interval = ts::GuardInterval (intValue<int> ("guard-interval", ts::GUARD_AUTO));
    bandwidth      = ts::BandWidth (intValue<int> ("bandwidth", ts::BW_8_MHZ));
    simple         = present ("simple");

    if ((fec_hp == ts::FEC_AUTO && guard_interval != ts::GUARD_AUTO) ||
        (fec_hp != ts::FEC_AUTO && guard_interval == ts::GUARD_AUTO)) {
        error ("specify either both --guard-interval and --high-priority-fec value or none");
    }

    exitOnError ();
}


//----------------------------------------------------------------------------
//  This routine displays a name/value pair
//----------------------------------------------------------------------------

namespace {
    void Display (const std::string& name, const std::string& value, const std::string& unit)
    {
        std::cout << "  "
                  << JustifyLeft  (name + " ", 22, '.')
                  << JustifyRight (" " + value, 15, '.')
                  << " " << unit << std::endl;
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);

    // Convert UHF channel to frequency
    if (opt.uhf_channel > 0) {
        if (opt.simple) {
            std::cout << UHF::Frequency (opt.uhf_channel, opt.hf_offset) << std::endl;
        }
        else {
            std::cout << "Carrier Frequency: "
                      << Decimal (UHF::Frequency (opt.uhf_channel, opt.hf_offset))
                      << " Hz" << std::endl;
        }
    }

    // Convert VHF channel to frequency
    if (opt.vhf_channel > 0) {
        if (opt.simple) {
            std::cout << VHF::Frequency (opt.vhf_channel, opt.hf_offset) << std::endl;
        }
        else {
            std::cout << "Carrier Frequency: "
                      << Decimal (VHF::Frequency (opt.vhf_channel, opt.hf_offset))
                      << " Hz" << std::endl;
        }
    }

    // Convert frequency to UHF/VHF channel
    if (opt.frequency > 0) {
        if (UHF::InBand (opt.frequency)) {
            if (opt.simple) {
                std::cout << UHF::Channel (opt.frequency) << std::endl
                          << UHF::OffsetCount (opt.frequency) << std::endl;
            }
            else {
                int channel = UHF::Channel (opt.frequency);
                int offset = UHF::OffsetCount (opt.frequency);
                std::cout << "UHF channel: " << channel << ", offset: " << offset << std::endl;
                uint64_t exact_freq = UHF::Frequency (channel, offset);
                int diff = int (int64_t (opt.frequency) - int64_t (exact_freq));
                if (::abs (diff) > 1) {
                    std::cout << "Warning: exact frequency for channel "
                              << channel << ", offset " << offset << " is "
                              << Decimal (exact_freq) << " Hz, differ by "
                              << Decimal (diff) << " Hz" << std::endl;
                }
            }
        }
        else if (VHF::InBand (opt.frequency)) {
            if (opt.simple) {
                std::cout << VHF::Channel (opt.frequency) << std::endl
                          << VHF::OffsetCount (opt.frequency) << std::endl;
            }
            else {
                int channel = VHF::Channel (opt.frequency);
                int offset = VHF::OffsetCount (opt.frequency);
                std::cout << "VHF channel: " << channel << ", offset: " << offset << std::endl;
                uint64_t exact_freq = VHF::Frequency (channel, offset);
                int diff = int (int64_t (opt.frequency) - int64_t (exact_freq));
                if (::abs (diff) > 1) {
                    std::cout << "Warning: exact frequency for channel "
                              << channel << ", offset " << offset << " is "
                              << Decimal (exact_freq) << " Hz, differ by "
                              << Decimal (diff) << " Hz" << std::endl;
                }
            }
        }
        else {
            std::cerr << Decimal (opt.frequency) << " Hz is not in UHF or VHF bands (VHF: "
                      << Decimal (VHF::Frequency (VHF::FIRST_CHANNEL, -3)) << " - "
                      << Decimal (VHF::Frequency (VHF::LAST_CHANNEL, 3)) << ", UHF: "
                      << Decimal (UHF::Frequency (UHF::FIRST_CHANNEL, -3)) << " - "
                      << Decimal (UHF::Frequency (UHF::LAST_CHANNEL, 3)) << ")"
                      << std::endl;
        }
    }

    // Compute TS bitrate from modulation parameters
    if (opt.fec_hp != ts::FEC_AUTO && opt.guard_interval != ts::GUARD_AUTO) {
        TunerParametersDVBT params;
        params.bandwidth = opt.bandwidth;
        params.fec_hp = opt.fec_hp;
        params.modulation = opt.constellation;
        params.guard_interval = opt.guard_interval;
        if (opt.simple) {
            std::cout << params.theoreticalBitrate() << std::endl;
        }
        else {
            std::cout << "Transport stream bitrate: "
                      << Decimal (params.theoreticalBitrate())
                      << " b/s" << std::endl;
        }
    }

    // Guess possible modulation parameters from bitrate
    if (opt.bitrate > 0) {

        // Build a list of all possible modulation parameters for this bitrate.
        TunerParametersBitrateDiffDVBTList params_list;
        TunerParametersBitrateDiffDVBT::EvaluateToBitrate (params_list, opt.bitrate);

        // Display all relevant parameters, up to max_guess
        // (in case of equal differences, display them all)
        int last_diff = 0;
        size_t count = 0;
        for (TunerParametersBitrateDiffDVBTList::const_iterator it = params_list.begin();
             it != params_list.end() && (count < opt.max_guess || ::abs (it->bitrate_diff) == ::abs (last_diff));
             ++it, ++count) {

            last_diff = it->bitrate_diff;
            if (opt.simple) {
                std::cout << it->theoreticalBitrate() << std::endl
                          << BandWidthEnum.name (it->bandwidth) << std::endl
                          << InnerFECEnum.name (it->fec_hp) << std::endl
                          << ModulationEnum.name (it->modulation) << std::endl
                          << GuardIntervalEnum.name (it->guard_interval) << std::endl;
            }
            else {
                if (count > 0) {
                    std::cout << std::endl;
                }
                Display ("Nominal bitrate", Decimal (it->theoreticalBitrate()), "b/s");
                Display ("Bitrate difference", Decimal (it->bitrate_diff), "b/s");
                Display ("Bandwidth", BandWidthEnum.name (it->bandwidth), "");
                Display ("FEC (high priority)", InnerFECEnum.name (it->fec_hp), "");
                Display ("Constellation", ModulationEnum.name (it->modulation), "");
                Display ("Guard interval", GuardIntervalEnum.name (it->guard_interval), "");
            }
        }
    }

    return EXIT_SUCCESS;
}
