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
#include "tsSysUtils.h"
#include "tsStringUtils.h"
#include "tsBinaryTable.h"
#include "tsXMLTables.h"
#include "tsReportWithPrefix.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    ts::StringVector infiles;   // Input file names.
    std::string      outfile;   // Output file path.
    bool             outdir;    // Output name is a directory.
    bool             compile;   // Explicit compilation.
    bool             decompile; // Explicit decompilation.
};

Options::Options(int argc, char *argv[]) :
    ts::Args("PSI/SI tables compiler.", "[options] filename ..."),
    infiles(),
    outfile(),
    outdir(false),
    compile(false),
    decompile(false)
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
            "  -o filepath\n"
            "  --output filepath\n"
            "      Specify the output file name. By default, the output file has the same\n"
            "      name as the input and extension .bin (compile) or .xml (decompile). If\n"
            "      the specified path is a directory, the output file is built from this\n"
            "      directory and default file name. If more than one input file is specified,\n"
            "      the output path, if present, must be a directory name.\n"
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
    outdir = !outfile.empty() && ts::IsDirectory(outfile);

    if (present("verbose")) {
        setDebugLevel(ts::Severity::Verbose);
    }

    if (infiles.size() > 1 && !outfile.empty() && !outdir) {
        error("with more than one input file, --output must be a directory");
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
    opt.verbose("Compiling " + infile + " to " + outfile);
    ts::ReportWithPrefix report(opt, infile + ": ");

    // Load XML file, convert tables to binary and save binary file.
    ts::XMLTables xml;
    return xml.loadXML(infile, report) && ts::BinaryTable::SaveFile(xml.tables(), outfile, report);
}


//----------------------------------------------------------------------------
//  Decompile one binary file. Return true on success, false on error.
//----------------------------------------------------------------------------

bool DecompileBinary(Options& opt, const std::string& infile, const std::string& outfile)
{
    opt.verbose("Decompiling " + infile + " to " + outfile);
    ts::ReportWithPrefix report(opt, infile + ": ");

    // Load binary tables.
    ts::BinaryTablePtrVector tables;
    if (!ts::BinaryTable::LoadFile(tables, infile, ts::CRC32::CHECK, report)) {
        return false;
    }

    // Convert tables to XML and save XML file.
    ts::XMLTables xml;
    xml.add(tables);
    return xml.saveXML(outfile, report);
}


//----------------------------------------------------------------------------
//  Process one file. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ProcessFile(Options& opt, const std::string& infile)
{
    const std::string ext(ts::LowerCaseValue(ts::PathSuffix(infile)));
    const bool isXML = ext == ".xml";
    const bool isBin = ext == ".bin";
    const bool compile = opt.compile || isXML;
    const bool decompile = opt.decompile || isBin;
    const char* const outExt = compile ? ".bin" : ".xml";

    // Compute output file name with default file type.
    std::string outname(opt.outfile);
    if (outname.empty()) {
        outname = ts::PathPrefix(infile) + outExt;
    }
    else if (opt.outdir) {
        outname += ts::PathSeparator + ts::PathPrefix(ts::BaseName(infile)) + outExt;
    }

    // Process the input file, starting with error cases.
    if (!compile && !decompile) {
        opt.error("don't know what to do with file " + infile + ", unknown file type, specify --compile or --decompile");
        return false;
    }
    else if (compile && isBin) {
        opt.error("cannot compile binary file " + infile);
        return false;
    }
    else if (decompile && isXML) {
        opt.error("cannot decompile XML file " + infile);
        return false;
    }
    else if (compile) {
        return CompileXML(opt, infile, outname);
    }
    else {
        return DecompileBinary(opt, infile, outname);
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    Options opt(argc, argv);
    bool ok = true;
    for (size_t i = 0; i < opt.infiles.size(); ++i) {
        if (!opt.infiles[i].empty()) {
            ok = ProcessFile(opt, opt.infiles[i]) && ok;
        }
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
