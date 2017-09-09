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
//  PSI/SI tables compiler. 
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    ts::StringVector infiles;   // Input file names.
    std::string      outfile;   // Output file name.
    bool             compile;   // Explicit compilation.
    bool             decompile; // Explicit decompilation.
    bool             verbose;   // Verbose output
};

Options::Options(int argc, char *argv[]) :
    ts::Args("PSI/SI tables compiler.", "[options] filename ..."),
    infiles(),
    outfile(),
    compile(false),
    decompile(false),
    verbose(false)
{
    option("",           0,  ts::Args::STRING, 1);
    option("compile",   'c');
    option("decompile", 'd');
    option("output",    'o', ts::Args::STRING);
    option("verbose",   'v');

    setHelp("Input files:\n"
            "\n"
            "  XML source files to compile or binary table files to decompile. By default,\n"
            "  files ending in .xml are compiled and files ending in .bin are decompiled.\n"
            "  For other files, explicitly specify --compile or --decompile.\n"
            "\n"
            "Options:\n"
            "\n"
            "  -c\n"
            "  --compile\n"
            "      Compile all files as XML source files into binary files. This is the\n"
            "      default for .xml files.\n"
            "\n"
            "  -d\n"
            "  --decompile\n"
            "      Decompile all files as binary files into XML files. This is the default\n"
            "      for .bin files.\n"
            "\n"
            "  --help\n"
            "      Display this help text.\n"
            "\n"
            "  -o filename\n"
            "  --output filename\n"
            "      Specify the output file name. This is allowed only if there is only one\n"
            "      input file. By default, the output file has the same name as the input\n"
            "      and extension .bin (compile) or .xml (decompile).\n"
            "\n"
            "  -v\n"
            "  --verbose\n"
            "      Produce verbose output.\n"
            "\n"
            "  --version\n"
            "      Display the version number.\n");

    analyze(argc, argv);

    getValues(infiles, "");
    getValue(outfile, "output");
    compile = present("compile");
    decompile = present("decompile");
    verbose = present("verbose");

    if (infiles.size() > 1 && !outfile.empty()) {
        error("cannot specify --output with more than one input file");
    }
    if (compile && decompile) {
        error("specify either --compile or --decompile but not both");
    }
    exitOnError();
}


//----------------------------------------------------------------------------
//  Compile one source file. Return true on success, false on error.
//----------------------------------------------------------------------------

bool CompileXML(Options& opt, const std::string& infile, const std::string& outfile)
{
    return false; //@@@@
}


//----------------------------------------------------------------------------
//  Decompile one binary file. Return true on success, false on error.
//----------------------------------------------------------------------------

bool DecompileBinary(Options& opt, const std::string& infile, const std::string& outfile)
{
    return false; //@@@@
}


//----------------------------------------------------------------------------
//  Process one file. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ProcessFile(Options& opt, const std::string& infile)
{
    return false; //@@@@
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    Options opt(argc, argv);
    bool ok = true;
    for (size_t i = 0; i < opt.infiles.size(); ++i) {
        ok = ProcessFile(opt, opt.infiles[i]) && ok;
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
