//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport Stream file truncation utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsErrCodeReport.h"
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

        bool           check_only = false;          // check only, do not truncate
        std::uintmax_t packet_size = ts::PKT_SIZE;  // packet size in bytes
        std::uintmax_t trunc_pkt = 0;               // first packet to truncate (0 means eof)
        std::vector<fs::path> files {};             // file names
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Truncate an MPEG transport stream file", u"[options] filename ...")
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

    getPathValues(files);
    getIntValue(packet_size, u"size-of-packet", ts::PKT_SIZE);
    getIntValue(trunc_pkt, u"packet");
    check_only = present(u"noaction");

    if (present(u"byte") && present(u"packet")) {
        error(u"--byte and --packet are mutually exclusive");
    }
    else if (present(u"byte")) {
        getIntValue(trunc_pkt, u"byte");
        trunc_pkt = (trunc_pkt + packet_size - 1) / packet_size;
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
    Options opt(argc, argv);

    for (const auto& file : opt.files) {

        // Get file size
        const std::uintmax_t file_size = fs::file_size(file, &ts::ErrCodeReport(opt, u"error accessing", file));
        if (file_size == ts::FS_ERROR) {
            continue;
        }

        // Compute number of packets and how many bytes to keep in file.
        const std::uintmax_t pkt_count = file_size / opt.packet_size;
        const std::uintmax_t extra = file_size % opt.packet_size;
        const std::uintmax_t keep = opt.trunc_pkt == 0 || opt.trunc_pkt > pkt_count ? pkt_count * opt.packet_size : opt.trunc_pkt * opt.packet_size;

        // Display info in verbose or check mode
        if (opt.verbose()) {
            if (opt.files.size() > 1) {
                std::cout << file << ": ";
            }
            std::cout << ts::UString::Format(u"%'d bytes, %'d %d-byte packets, ", file_size, pkt_count, opt.packet_size);
            if (extra > 0) {
                std::cout << ts::UString::Format(u"%'d extra bytes, ", extra);
            }
            if (keep < file_size) {
                std::cout << ts::UString::Format(u"%'d bytes to truncate, ", file_size - keep) << std::endl;
            }
            else {
                std::cout << "ok" << std::endl;
            }
        }

        // Do the truncation
        if (!opt.check_only && keep < file_size) {
            fs::resize_file(file, keep, &ts::ErrCodeReport(opt, u"error truncating", file));
        }
    }

    return opt.gotErrors() ? EXIT_FAILURE : EXIT_SUCCESS;
}
