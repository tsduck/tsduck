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
//  Fix continuity counters in a TS file
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsContinuityAnalyzer.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        bool         test;          // Test mode
        bool         circular;      // Add empty packets to enforce circular continuity
        bool         no_replicate;  // Option --no-replicate-duplicated
        ts::UString  filename;      // File name
        std::fstream file;          // File buffer

        // Check if there was an I/O error on the file.
        // Print an error message if this is the case.
        bool fileError(const ts::UChar* message);
    };
}

// Constructor.
Options::Options(int argc, char *argv[]) :
    Args(u"Fix continuity counters in a transport stream", u"[options] filename"),
    test(false),
    circular(false),
    no_replicate(false),
    filename(),
    file()
{
    option(u"", 0, FILENAME, 1, 1);
    help(u"", u"MPEG capture file to be modified.");

    option(u"circular", 'c');
    help(u"circular",
         u"Enforce continuity when the file is played repeatedly. "
         u"Add empty packets, if necessary, on each PID so that the "
         u"continuity is preserved between end and beginning of file.");

    option(u"noaction");
    help(u"noaction", u"Legacy equivalent of --no-action.");

    option(u"no-action", 'n');
    help(u"no-action", u"Display what should be performed but do not modify the file.");

    option(u"no-replicate-duplicated");
    help(u"no-replicate-duplicated",
         u"Two successive packets in the same PID are considered as duplicated if they have "
         u"the same continuity counter and same content (except PCR, if any). "
         u"By default, duplicated input packets are replicated as duplicated on output "
         u"(the corresponding output packets have the same continuity counters). "
         u"When this option is specified, the input packets are not considered as duplicated and "
         u"the output packets receive individually incremented countinuity counters.");

    analyze(argc, argv);

    filename = value(u"");
    circular = present(u"circular");
    test = present(u"no-action") || present(u"noaction");
    no_replicate = present(u"no-replicate-duplicated");

    exitOnError();
}

// Check error on file
bool Options::fileError(const ts::UChar* message)
{
    if (file) {
        return false;
    }
    else {
        error(u"%s: %s", {filename, message});
        return true;
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);
    ts::ContinuityAnalyzer fixer(ts::AllPIDs, &opt);

    // Configure the CC analyzer.
    fixer.setDisplay(true);
    fixer.setFix(!opt.test);
    fixer.setReplicateDuplicated(!opt.no_replicate);
    fixer.setMessageSeverity(opt.test ? ts::Severity::Info : ts::Severity::Verbose);

    // Open file in read/write mode (CC are overwritten)
    std::ios::openmode mode = std::ios::in | std::ios::binary;
    if (!opt.test) {
        mode |= std::ios::out;
    }

    opt.file.open(opt.filename.toUTF8().c_str(), mode);

    if (!opt.file) {
        opt.error(u"cannot open file %s", {opt.filename});
        return EXIT_FAILURE;
    }

    // Process all packets in the file
    ts::TSPacket pkt;

    for (;;) {

        // Save position of current packet
        const std::ios::pos_type pos = opt.file.tellg();
        if (opt.fileError(u"error getting file position")) {
            break;
        }

        // Read a TS packet
        if (!pkt.read(opt.file, true, opt)) {
            break; // end of file
        }

        // Process packet
        if (!fixer.feedPacket(pkt) && !opt.test) {
            // Packet was modified, need to rewrite it.
            // Rewind to beginning of current packet
            opt.file.seekp(pos);
            if (opt.fileError(u"error setting file position")) {
                break;
            }
            // Rewrite the packet
            pkt.write(opt.file, opt);
            if (opt.fileError(u"error rewriting packet")) {
                break;
            }
            // Make sure the get position is ok
            opt.file.seekg(opt.file.tellp());
            if (opt.fileError(u"error setting file position")) {
                break;
            }
        }
    }

    opt.verbose(u"%'d packets read, %'d discontinuities, %'d packets updated", {fixer.totalPackets(), fixer.errorCount(), fixer.fixCount()});

    // Append empty packet to ensure circular continuity
    if (opt.circular && opt.valid()) {

        // Create an empty packet (no payload, 184-byte adaptation field)
        pkt = ts::NullPacket;
        pkt.b[3] = 0x20;    // adaptation field, no payload
        pkt.b[4] = 183;     // adaptation field length
        pkt.b[5] = 0x00;    // nothing in adaptation field

        // Ensure write position is at end of file
        if (!opt.test) {
            // First, need to clear the eof bit
            opt.file.clear();
            // Set write position at eof
            opt.file.seekp(0, std::ios::end);
            // Returned value ignored on purpose, just report error when needed.
            // coverity[CHECKED_RETURN]
            opt.fileError(u"error setting file position");
        }

        // Loop through all PIDs, adding packets where some are missing
        for (ts::PID pid = 0; opt.valid() && pid < ts::PID_MAX; pid++) {
            const uint8_t first_cc = fixer.firstCC(pid);
            uint8_t last_cc = fixer.lastCC(pid);
            if (first_cc != ts::INVALID_CC && first_cc != ((last_cc + 1) & ts::CC_MASK)) {
                // We must add some packets on this PID
                opt.verbose(u"PID: 0x%04X, adding %2d empty packets", {pid, ts::ContinuityAnalyzer::MissingPackets(last_cc, first_cc)});
                if (!opt.test) {
                    for (;;) {
                        last_cc = (last_cc + 1) & ts::CC_MASK;
                        if (first_cc == last_cc) {
                            break; // complete
                        }
                        // Update PID and CC in the packet
                        pkt.setPID(ts::PID(pid));
                        pkt.setCC(last_cc);
                        // Write the new packet
                        pkt.write(opt.file, opt);
                        if (opt.fileError(u"error writing extra packet")) {
                            break;
                        }
                    }
                }
            }
        }
    }

    opt.file.close();

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
