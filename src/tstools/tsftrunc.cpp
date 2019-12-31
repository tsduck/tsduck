//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsMPEG.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

class Options: public ts::Args
{
    TS_NOBUILD_NOCOPY(Options);
public:
    Options(int argc, char *argv[]);
    virtual ~Options();

    bool              check_only;   // check only, do not truncate
    ts::PacketCounter trunc_pkt;    // first packet to truncate (0 means eof)
    ts::UStringVector files;        // file names
};

// Destructor.
Options::~Options() {}

// Constructor.
Options::Options(int argc, char *argv[]) :
    Args(u"Truncate an MPEG transport stream file", u"[options] filename ..."),
    check_only(false),
    trunc_pkt(0),
    files()
{
    option(u"", 0, STRING, 1, UNLIMITED_COUNT);
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

    analyze(argc, argv);

    getValues(files);
    check_only = present(u"noaction");

    if (present(u"byte") && present(u"packet")) {
        error(u"--byte and --packet are mutually exclusive");
    }
    if (present(u"byte")) {
        trunc_pkt = (intValue<ts::PacketCounter>(u"byte") + ts::PKT_SIZE - 1) / ts::PKT_SIZE;
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
    ts::ErrorCode err;

    for (ts::UStringVector::const_iterator file = opt.files.begin(); file != opt.files.end(); ++file) {

        // Get file size

        int64_t size = GetFileSize(*file);

        if (size < 0) {
            err = ts::LastErrorCode();
            opt.error(u"%s: %s", {*file, ts::ErrorCodeMessage(err)});
            success = false;
            continue;
        }

        // Compute number of packets and how many bytes to keep in file.

        uint64_t file_size = uint64_t (size);
        uint64_t pkt_count = file_size / ts::PKT_SIZE;
        uint64_t extra = file_size % ts::PKT_SIZE;
        uint64_t keep;

        if (opt.trunc_pkt == 0 || opt.trunc_pkt > pkt_count) {
            keep = pkt_count * ts::PKT_SIZE;
        }
        else {
            keep = opt.trunc_pkt * ts::PKT_SIZE;
        }

        // Display info in verbose or check mode

        if (opt.verbose()) {
            if (opt.files.size() > 1) {
                std::cout << *file << ": ";
            }
            std::cout << ts::UString::Decimal(file_size) << " bytes, "
                      << ts::UString::Decimal(pkt_count) << " packets, ";
            if (extra > 0) {
                std::cout << extra << " extra bytes, ";
            }
            if (keep < file_size) {
                std::cout << ts::UString::Decimal(file_size - keep) << " bytes to truncate" << std::endl;
            }
            else {
                std::cout << "ok" << std::endl;
            }
        }

        // Do the truncation

        if (!opt.check_only && keep < file_size && (err = TruncateFile(*file, keep)) != ts::SYS_SUCCESS) {
            err = ts::LastErrorCode();
            opt.error(u"%s: %s", {*file, ts::ErrorCodeMessage(err)});
            success = false;
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
