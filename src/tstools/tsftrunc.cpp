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
//  Transport Stream file truncation utility
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsMPEG.h"
#include "tsDecimal.h"
#include "tsSysUtils.h"

using namespace ts;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public Args
{
    Options (int argc, char *argv[]);

    bool          check_only;   // check only, do not truncate
    bool          verbose;      // verbose mode
    PacketCounter trunc_pkt;    // first packet to truncate (0 means eof)
    StringVector  files;        // file names
};

Options::Options (int argc, char *argv[]) :
    Args("MPEG Transport Stream File Truncation Utility.", "[options] filename ..."),
    check_only(false),
    verbose(false),
    trunc_pkt(0),
    files()
{
    option ("",          0,  Args::STRING, 1, Args::UNLIMITED_COUNT);
    option ("packet",   'p', Args::UNSIGNED);
    option ("noaction", 'n');
    option ("verbose",  'v');

    setHelp ("Files:\n"
             "\n"
             "  MPEG capture files to be truncated.\n"
             "\n"
             "Options:\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -n\n"
             "  --noaction\n"
             "      Do not perform truncation, check mode only.\n"
             "\n"
             "  -p value\n"
             "  --packet value\n"
             "      Index of first packet to truncate. If unspecified, all complete\n"
             "      packets are kept in the file. Extraneous bytes at end of file\n"
             "      (after last multiple of 188 bytes) are truncated.\n"
             "\n"
             "  -v\n"
             "  --verbose\n"
             "      Produce verbose messages.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    analyze (argc, argv);

    getValues (files);
    trunc_pkt = intValue<PacketCounter> ("packet");
    check_only = present ("noaction");
    verbose = check_only || present ("verbose");
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);
    bool success = true;
    ErrorCode err;

    for (StringVector::const_iterator file = opt.files.begin(); file != opt.files.end(); ++file) {
    
        // Get file size

        int64_t size = GetFileSize (*file);

        if (size < 0) {
            err = LastErrorCode();
            opt.error (*file + ": " + ErrorCodeMessage (err));
            success = false;
            continue;
        }

        // Compute number of packets and how many bytes to keep in file.

        uint64_t file_size = uint64_t (size);
        uint64_t pkt_count = file_size / PKT_SIZE;
        uint64_t extra = file_size % PKT_SIZE;
        uint64_t keep;

        if (opt.trunc_pkt == 0 || opt.trunc_pkt > pkt_count) {
            keep = pkt_count * PKT_SIZE;
        }
        else {
            keep = opt.trunc_pkt * PKT_SIZE;
        }

        // Display info in verbose or check mode

        if (opt.verbose) {
            if (opt.files.size() > 1) {
                std::cout << *file << ": ";
            }
            std::cout << Decimal (file_size) << " bytes, "
                      << Decimal (pkt_count) << " packets, ";
            if (extra > 0) {
                std::cout << extra << " extra bytes, ";
            }
            if (keep < file_size) {
                std::cout << Decimal (file_size - keep) << " bytes to truncate" << std::endl;
            }
            else {
                std::cout << "ok" << std::endl;
            }
        }

        // Do the truncation

        if (!opt.check_only && keep < file_size && (err = TruncateFile (*file, keep)) != SYS_SUCCESS) {
            err = LastErrorCode();
            opt.error (*file + ": " + ErrorCodeMessage (err));
            success = false;
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
