//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Generate one ECM using any DVB SimulCrypt compliant ECMG.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsECMGClient.h"
#include "tsECMGSCS.h"
#include "tsStandaloneTableDemux.h"
#include "tsSectionFile.h"
#include "tsTSPacket.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class GenECMOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(GenECMOptions);
    public:
        GenECMOptions(int argc, char *argv[]);

        ts::DuckContext    duck;       // TSDuck execution context.
        ts::UString        outFile;    // Name of binary output file.
        ts::ECMGClientArgs ecmg;       // ECMG parameters
        uint16_t           cpNumber;   // Crypto-period number
        ts::ByteBlock      cwCurrent;  // Current CW
        ts::ByteBlock      cwNext;     // Next CW
    };
}

GenECMOptions::GenECMOptions(int argc, char *argv[]) :
    ts::Args(u"Generate one ECM using any DVB SimulCrypt compliant ECMG", u"[options] output-file"),
    duck(this),
    outFile(),
    ecmg(),
    cpNumber(0),
    cwCurrent(),
    cwNext()
{
    setIntro(u"This command connects to a DVB SimulCrypt compliant ECMG and requests "
             u"the generation of one ECM. Restriction: The target ECMG shall support "
             u"current or current/next control words in ECM, meaning CW_per_msg = 1 or 2 "
             u"and lead_CW = 0 or 1.");

    option(u"", 0, FILENAME, 1, 1);
    help(u"", u"filename", u"Name of the binary output file which receives the ECM.");

    option(u"cp-number", 0, UINT16);
    help(u"cp-number", u"Crypto-period number. Default: 0.");

    option(u"cw-current", 'c', HEXADATA, 1, 1, 1);
    help(u"cw-current", u"Current control word (required). The value must be a suite of hexadecimal digits.");

    option(u"cw-next", 'n', HEXADATA, 0, 1, 1);
    help(u"cw-next", u"Next control word (optional). The value must be a suite of hexadecimal digits.");

    // Common ECMG parameters.
    ecmg.defineArgs(*this);

    // Analyze the command line.
    analyze(argc, argv);

    // Analyze parameters.
    ecmg.loadArgs(duck, *this);
    getValue(outFile, u"");
    getIntValue(cpNumber, u"cp-number", 0);
    getHexaValue(cwCurrent, u"cw-current");
    getHexaValue(cwNext, u"cw-next");

    exitOnError();
}


//----------------------------------------------------------------------------
//  Extract sections from an ECM response.
//----------------------------------------------------------------------------

namespace {
    bool ExtractECMs(GenECMOptions& opt, ts::SectionFile& ecmFile, const ts::ecmgscs::ChannelStatus& channelStatus, const ts::ecmgscs::ECMResponse& response)
    {
        if (channelStatus.section_TSpkt_flag) {

            // The ECM is in TS packet format.
            if (response.ECM_datagram.size() % ts::PKT_SIZE != 0) {
                opt.error(u"Invalid ECM reponse, pretend to be in packet mode, returned %d bytes, not a multiple of %d", {response.ECM_datagram.size(), ts::PKT_SIZE});
                return false;
            }

            // Demux the ECM sections from the TS packets.
            ts::StandaloneTableDemux demux(opt.duck, ts::AllPIDs);
            for (size_t index = 0; index + ts::PKT_SIZE <= response.ECM_datagram.size(); index += ts::PKT_SIZE) {
                ts::TSPacket pkt;
                pkt.copyFrom(&response.ECM_datagram[index]);
                demux.feedPacket(pkt);
            }
            for (size_t i = 0; i < demux.tableCount(); ++i) {
                ecmFile.add(demux.tableAt(i));
            }
        }
        else {
            // The ECM is in section format.
            const uint8_t* data = response.ECM_datagram.data();
            size_t remain = response.ECM_datagram.size();
            size_t size = 0;
            while ((size = ts::Section::SectionSize(data, remain)) > 0) {
                // Get one section.
                assert(size <= remain);
                ts::SectionPtr section(new ts::Section(data, size));
                if (section.isNull() || !section->isValid()) {
                    opt.error(u"ECMG returned an invalid section");
                    return false;
                }
                ecmFile.add(section);
                data += size;
                remain -= size;
            }
        }
        return true;
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    GenECMOptions opt(argc, argv);
    ts::tlv::Logger logger(ts::Severity::Debug, &opt);
    ts::ecmgscs::ChannelStatus channelStatus;
    ts::ecmgscs::StreamStatus streamStatus;
    ts::ECMGClient ecmg;

    // Set logging levels.
    logger.setDefaultSeverity(opt.ecmg.log_protocol);
    logger.setSeverity(ts::ecmgscs::Tags::CW_provision, opt.ecmg.log_data);
    logger.setSeverity(ts::ecmgscs::Tags::ECM_response, opt.ecmg.log_data);

    // Specify which ECMG <=> SCS version to use.
    ts::ecmgscs::Protocol::Instance()->setVersion(opt.ecmg.dvbsim_version);

    // Connect to ECMG.
    if (!ecmg.connect(opt.ecmg, channelStatus, streamStatus, nullptr, logger)) {
        // Error connecting to ECMG, error message already reported
        return EXIT_FAILURE;
    }

    // Request the ECM (synchronous operation).
    ts::ecmgscs::ECMResponse response;
    if (!ecmg.generateECM(opt.cpNumber, opt.cwCurrent, opt.cwNext, opt.ecmg.access_criteria, uint16_t(opt.ecmg.cp_duration / 100), response)) {
        ecmg.disconnect();
        return EXIT_FAILURE;
    }

    // Disconnect from ECMG.
    ecmg.disconnect();

    // Get the ECM section from the ECMG response.
    ts::SectionFile ecmFile(opt.duck);
    if (!ExtractECMs(opt, ecmFile, channelStatus, response)) {
        // Malformed response, error message already reported
        return EXIT_FAILURE;
    }

    // Save the binary file containing the ECM's.
    return ecmFile.saveBinary(opt.outFile) ? EXIT_SUCCESS : EXIT_FAILURE;
}
