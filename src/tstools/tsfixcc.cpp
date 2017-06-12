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
//  Fix continuity counters in a TS file
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsTSPacket.h"
#include "tsFormat.h"
#include "tsDecimal.h"

using namespace ts;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public Args
{
    Options (int argc, char *argv[]);

    bool         verbose;   // Verbose mode
    bool         test;      // Test mode
    bool         circular;  // Add empty packets to enforce circular continuity
    std::string  filename;  // File name
    std::fstream file;      // File buffer

    bool fileError (const char* message);
};

Options::Options (int argc, char *argv[]) :
    Args("MPEG Transport Stream Fix Continuity Counters Utility.", "[options] filename"),
    verbose(false),
    test(false),
    circular(false),
    filename(),
    file()
{
    option ("",          0,  Args::STRING, 1, 1);
    option ("circular", 'c');
    option ("noaction", 'n');
    option ("verbose",  'v');

    setHelp ("File:\n"
             "\n"
             "  MPEG capture file to be modified.\n"
             "\n"
             "Options:\n"
             "\n"
             "  -c\n"
             "  --circular\n"
             "      Enforce continuity when the file is played repeatedly.\n"
             "      Add empty packets, if necessary, on each PID so that the\n"
             "      continuity is preserved between end and beginning of file.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -n\n"
             "  --noaction\n"
             "      Display what should be performed but do not modify the file.\n"
             "\n"
             "  -v\n"
             "  --verbose\n"
             "      Produce verbose messages.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    analyze (argc, argv);

    filename = value ("");
    circular = present ("circular");
    test = present ("noaction");
    verbose = test || present ("verbose");
}

// Check error on file
bool Options::fileError (const char* message)
{
    if (file) {
        return false;
    }
    else {
        error ((filename + ": ") + message);
        return true;
    }
}


//----------------------------------------------------------------------------
//  Return the number of missing packets between two continuity counters
//----------------------------------------------------------------------------

inline int MissingPackets (int cc1, int cc2)
{
    return (cc2 <= cc1 ? 16 : 0) + cc2 - cc1 - 1;
}


//----------------------------------------------------------------------------
//  PID analysis state
//----------------------------------------------------------------------------

struct PIDState
{
    uint8_t first_cc;
    uint8_t last_cc;
    bool  sync;

    // Constructor
    PIDState () :
        first_cc (0xFF),
        last_cc (0xFF),
        sync (false)
    {
    }
};


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);

    // Open file in read/write mode (CC are overwritten)

    std::ios::openmode mode = std::ios::in | std::ios::binary;
    if (!opt.test) {
        mode |= std::ios::out;
    }

    opt.file.open (opt.filename.c_str(), mode);

    if (!opt.file) {
        opt.error ("cannot open file " + opt.filename);
        return EXIT_FAILURE;
    }

    // Process all packets in the file

    PIDState pids [PID_MAX];
    PacketCounter packet_count = 0;
    PacketCounter error_count = 0;
    PacketCounter rewrite_count = 0;
    TSPacket pkt;

    for (;;) {

        // Save position of current packet

        const std::ios::pos_type pos = opt.file.tellg();
        if (opt.fileError ("error getting file position")) {
            break;
        }

        // Read a TS packet

        if (!pkt.read (opt.file, true, opt)) {
            break; // end of file
        }

        // Process packet

        const ts::PID pid = pkt.getPID();
        const uint8_t cc = pkt.getCC();
        uint8_t good_cc = cc;

        if (pids[pid].first_cc > 0x0F) {
            // First packet on this PID
            pids[pid].first_cc = cc;
            pids[pid].sync = true;
        }
        else {
            // Compute expected CC for this packet
            good_cc = pkt.hasPayload() ? ((pids[pid].last_cc + 1) & 0x0F) : pids[pid].last_cc;
            if (pids[pid].sync && cc != good_cc) {
                // PID was correctly synchronized, but the current CC is wrong.
                // We now loose the synchronization on this PID.
                pids[pid].sync = false;
                error_count++;
                if (opt.verbose) {
                    std::cerr << "TS packet: " << Decimal (packet_count)
                              << Format (", PID: 0x%04X, missing: %2d packets", pid, MissingPackets (pids[pid].last_cc, cc))
                              << std::endl;
                }
            }
        }

        // Rewrite packet if no longer synchronized

        if (!pids[pid].sync && !opt.test) {
            // Update CC in packet with expected value
            pkt.setCC (good_cc);
            // Rewind to beginning of current packet
            opt.file.seekp (pos);
            if (opt.fileError ("error setting file position")) {
                break;
            }
            // Rewrite the packet
            pkt.write (opt.file, opt);
            if (opt.fileError ("error rewriting packet")) {
                break;
            }
            // Make sure the get position is ok
            opt.file.seekg (opt.file.tellp());
            if (opt.fileError ("error setting file position")) {
                break;
            }
            rewrite_count++;
        }

        pids[pid].last_cc = good_cc;
        packet_count++;
    }

    if (opt.verbose) {
        std::cerr << Decimal (packet_count) << " packets read, "
                  << Decimal (error_count) << " discontinuities, "
                  << Decimal (rewrite_count) << " packets updated"
                  << std::endl;
    }

    // Append empty packet to ensure circular continuity

    if (opt.circular && opt.valid()) {

        // Create an empty packet (no payload, 184-byte adaptation field)

        pkt = NullPacket;
        pkt.b[3] = 0x20;         // adaptation field, no payload
        pkt.b[4] = 183;          // adaptation field length
        pkt.b[5] = 0x00;         // nothing in adaptation field

        // Ensure write position is at end of file

        if (!opt.test) {
            // First, need to clear the eof bit
            opt.file.clear();
            // Set write position at eof
            opt.file.seekp (0, std::ios::end);
            opt.fileError ("error setting file position");
        }

        // Loop through all PIDs, adding packets where some are missing

        for (size_t pid = 0; opt.valid() && pid < PID_MAX; pid++) {
            if (pids[pid].first_cc <= 0x0F && pids[pid].first_cc != ((pids[pid].last_cc + 1) & 0x0F)) {
                // We must add some packets on this PID
                if (opt.verbose) {
                    std::cerr << Format ("PID: 0x%04" FMT_SIZE_T "X, adding %2d empty packets",
                                         pid, MissingPackets (pids[pid].last_cc, pids[pid].first_cc))
                              << std::endl;
                }
                if (!opt.test) {
                    for (;;) {
                        pids[pid].last_cc = (pids[pid].last_cc + 1) & 0x0F;
                        if (pids[pid].first_cc == pids[pid].last_cc) {
                            break; // complete
                        }
                        // Update PID and CC in the packet
                        pkt.setPID(ts::PID(pid));
                        pkt.setCC(pids[pid].last_cc);
                        // Write the new packet
                        pkt.write(opt.file, opt);
                        if (opt.fileError("error writing extra packet")) {
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
