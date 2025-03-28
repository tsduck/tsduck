//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Test utility for networking functions.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsArgs.h"
#include "tsCerrReport.h"
#include "tsZlib.h"
#include "tsSysUtils.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace ts {
    class ZlibOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(ZlibOptions);
    public:
        ZlibOptions(int argc, char *argv[]);
        virtual ~ZlibOptions() override;

        bool    use_sdefl = false;
        bool    hexa_input = false;
        bool    hexa_output = false;
        bool    compress = false;
        bool    decompress = false;
        int     level = 0;
        UString input_file {};
        UString output_file {};
    };
}

ts::ZlibOptions::~ZlibOptions()
{
}

ts::ZlibOptions::ZlibOptions(int argc, char *argv[]) :
    Args(u"Test utility for compression library", u"[options]")
{
    option(u"compress", 'c');
    help(u"compress", u"Compress the input file into output file.");

    option(u"decompress", 'd');
    help(u"decompress", u"Decompress the input file into output file.");

    option(u"hexa-input");
    help(u"hexa-input", u"Interpret input file as hexa dump. Decode to binary before compressing/decompressing.");

    option(u"hexa-output", 'h');
    help(u"hexa-output", u"Output an hexa dump of the compressed/decompressed data, instead of binary data.");

    option(u"input-file", 'i', STRING);
    help(u"input-file", u"Input file name. Default to the standard input.");

    option(u"level", 'l', INTEGER, 0, 1, 0, 9);
    help(u"level", u"Compression level (with compress). From 0 to 9. The default is 5.");

    option(u"output-file", 'o', STRING);
    help(u"output-file", u"Output file name. Default to the standard output.");

    option(u"sdefl", 's');
    help(u"sdefl", u"Use \"sdefl\", aka \"Small Deflate\", library. Only useful if TSDuck was compiled with zlib.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    use_sdefl = present(u"sdefl");
    hexa_input = present(u"hexa-input");
    hexa_output = present(u"hexa-output");
    compress = present(u"compress");
    decompress = present(u"decompress");
    getValue(input_file, u"input-file");
    getValue(output_file, u"output-file");
    getIntValue(level, u"level", 5);

    if (compress && decompress) {
        error(u"--compress and --decompress are mutually exclusive");
    }

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
// Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    ts::ZlibOptions opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());
    opt.verbose(u"compression library: %s", ts::Zlib::GetLibraryVersion());
    bool ok = true;

    if (opt.compress || opt.decompress) {

        // Read input file.
        ts::ByteBlock input;
        if (opt.hexa_input) {
            ts::UStringList hex;
            if (opt.input_file.empty()) {
                ts::UString::Load(hex, std::cin);
            }
            else {
                ok = ts::UString::Load(hex, opt.input_file);
                if (!ok) {
                    opt.error(u"error reading %s", opt.input_file);
                }
            }
            if (ok) {
                ok = ts::UString().join(hex).hexaDecode(input);
                if (!ok) {
                    opt.error(u"invalid hexadecimal input data");
                }
            }
        }
        else {
            if (opt.input_file.empty()) {
                ts::SetBinaryModeStdin(opt);
                input.read(std::cin);
            }
            else {
                ok = input.loadFromFile(opt.input_file, std::numeric_limits<size_t>::max(), &opt);
            }
        }

        // Compress or decompress.
        ts::ByteBlock output;
        if (ok) {
            opt.verbose(u"input size: %d bytes", input.size());
            if (opt.compress) {
                ok = ts::Zlib::Compress(output, input, opt.level, opt, opt.use_sdefl);
            }
            else {
                ok = ts::Zlib::Decompress(output, input, opt, opt.use_sdefl);
            }
        }

        // Write output file.
        if (ok) {
            opt.verbose(u"output size: %d bytes", output.size());
            if (opt.hexa_output) {
                const ts::UString hex(ts::UString::Dump(output, ts::UString::BPL, 0, 16));
                if (opt.output_file.empty()) {
                    std::cout << hex;
                }
                else {
                    ok = hex.save(opt.output_file, false, true);
                }
            }
            else {
                if (opt.output_file.empty()) {
                    ok = ts::SetBinaryModeStdout(opt) && output.write(std::cout);
                }
                else {
                    ok = output.saveToFile(opt.output_file, &opt);
                }
            }
        }
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
