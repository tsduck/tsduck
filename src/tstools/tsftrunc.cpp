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
//  Transport Stream file truncation utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
#include "tsTS.h"
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

        bool              check_only;   // check only, do not truncate
        size_t            packet_size;  // packet size in bytes
        ts::PacketCounter trunc_pkt;    // first packet to truncate (0 means eof)
        ts::UStringVector files;        // file names
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Truncate an MPEG transport stream file", u"[options] filename ..."),
    check_only(false),
    packet_size(ts::PKT_SIZE),
    trunc_pkt(0),
    files()
{
    option(u"", 0, FILENAME, 1, UNLIMITED_COUNT);
    help(u"", u"MPEG capture files to be truncated.");

    option(u"byte", 'b', UNSIGNED);
    help(u"byte",
         u"Truncate the file at the next packet boundary after the specified size "
         u"in bytes. Mutually exclusive with --packet.");

    option(u"noaction", 'n');
    help(u"noaction", u"Do not perform truncation, check mode only.");

    option(u"packet", 'p', UNSIGNED);
    help(u"packet",
         u"Index of first packet to truncate. If unspecified, all complete "
         u"packets are kept in the file. Extraneous bytes at end of file "
         u"(after last multiple of 188 bytes) are truncated.");

    option(u"size-of-packet", 's', POSITIVE);
    help(u"size-of-packet",
         u"TS packet size in bytes. The default is " + ts::UString::Decimal(ts::PKT_SIZE) +
         u" bytes. Alternate packet sizes are useful for M2TS or other TS file formats.");

    analyze(argc, argv);

    getValues(files);
    check_only = present(u"noaction");
    packet_size = intValue<size_t>(u"size-of-packet", ts::PKT_SIZE);

    if (present(u"byte") && present(u"packet")) {
        error(u"--byte and --packet are mutually exclusive");
    }
    if (present(u"byte")) {
        trunc_pkt = (intValue<ts::PacketCounter>(u"byte") + packet_size - 1) / packet_size;
    }
    else {
        trunc_pkt = intValue<ts::PacketCounter>(u"packet");
    }
    if (check_only) {
        setMaxSeverity(ts::Severity::Verbose);
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt (argc, argv);
    bool success = true;
    ts::SysErrorCode err;

    for (const auto& file : opt.files) {

        // Get file size

        const int64_t size = ts::GetFileSize(file);

        if (size < 0) {
            err = ts::LastSysErrorCode();
            opt.error(u"%s: %s", {file, ts::SysErrorCodeMessage(err)});
            success = false;
            continue;
        }

        // Compute number of packets and how many bytes to keep in file.

        const uint64_t file_size = uint64_t(size);
        const uint64_t pkt_count = file_size / opt.packet_size;
        const uint64_t extra = file_size % opt.packet_size;
        uint64_t keep;

        if (opt.trunc_pkt == 0 || opt.trunc_pkt > pkt_count) {
            keep = pkt_count * opt.packet_size;
        }
        else {
            keep = opt.trunc_pkt * opt.packet_size;
        }

        // Display info in verbose or check mode

        if (opt.verbose()) {
            if (opt.files.size() > 1) {
                std::cout << file << ": ";
            }
            std::cout << ts::UString::Format(u"%'d bytes, %'d %d-byte packets, ", {file_size, pkt_count, opt.packet_size});
            if (extra > 0) {
                std::cout << ts::UString::Format(u"%'d extra bytes, ", {extra});
            }
            if (keep < file_size) {
                std::cout << ts::UString::Format(u"%'d bytes to truncate, ", {file_size - keep}) << std::endl;
            }
            else {
                std::cout << "ok" << std::endl;
            }
        }

        // Do the truncation

        if (!opt.check_only && keep < file_size && !TruncateFile(file, keep, opt)) {
            success = false;
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
