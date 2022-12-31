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
//  PSI/SI tables compiler.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsSectionFileArgs.h"
#include "tsxmlTweaks.h"
#include "tsReportWithPrefix.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
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

        ts::DuckContext     duck;            // Execution context.
        ts::UStringVector   inFiles;         // Input file names.
        ts::UString         outFile;         // Output file path.
        bool                outIsDir;        // Output name is a directory.
        bool                useStdIn;        // At least one input file is the standard input.
        bool                useStdOut;       // Use standard output.
        bool                compile;         // Explicit compilation.
        bool                decompile;       // Explicit decompilation.
        bool                fromJSON;        // All input files are JSON.
        bool                toJSON;          // Decompile to JSON.
        bool                xmlModel;        // Display XML model instead of compilation.
        bool                withExtensions;  // XML model with extensions.
        ts::SectionFileArgs sectionOptions;  // Section file processing options.
        ts::xml::Tweaks     xmlTweaks;       // XML formatting options.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"PSI/SI tables compiler", u"[options] filename ..."),
    duck(this),
    inFiles(),
    outFile(),
    outIsDir(false),
    useStdIn(false),
    useStdOut(false),
    compile(false),
    decompile(false),
    fromJSON(false),
    toJSON(false),
    xmlModel(false),
    withExtensions(false),
    sectionOptions(),
    xmlTweaks()
{
    duck.defineArgsForStandards(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForCharset(*this);
    sectionOptions.defineArgs(*this);
    xmlTweaks.defineArgs(*this);

    option(u"", 0, FILENAME);
    help(u"",
         u"XML or JSON source files to compile or binary table files to decompile. "
         u"By default, files ending in .xml or .json are compiled and files ending in .bin are decompiled. "
         u"For other files, explicitly specify --compile or --decompile.\n\n"
         u"If an input file name is \"-\", the standard input is used. "
         u"In that case, --compile or --decompile must be specified.\n\n"
         u"If an input file name starts with \"<?xml\", it is considered as \"inline XML content\". "
         u"Similarly, if an input file name starts with \"{\" or \"[\", it is considered as \"inline JSON content\".\n\n"
         u"The reference source format is XML. JSON files are first translated to XML using the "
         u"\"automated XML-to-JSON conversion\" rules of TSDuck and then compiled.");

    option(u"compile", 'c');
    help(u"compile",
         u"Compile all files as XML or JSON source files into binary files. "
         u"This is the default for .xml and .json files.");

    option(u"decompile", 'd');
    help(u"decompile",
         u"Decompile all files as binary files into XML files. "
         u"This is the default for .bin files.");

    option(u"extensions", 'e');
    help(u"extensions",
         u"With --xml-model, include the content of the available extensions.");

    option(u"from-json", 'f');
    help(u"from-json",
         u"Each input file must be a JSON file, "
         u"typically from a previous automated XML-to-JSON conversion or in a similar format. "
         u"This is automatically detected for file names ending in .json. "
         u"This option is only required when the input file name has a non-standard extension or is the standard input.");

    option(u"json", 'j');
    help(u"json",
         u"When decompiling, perform an automated XML-to-JSON conversion. "
         u"The output file is in JSON format instead of XML. "
         u"The default output file names have extension .json.");

    option(u"output", 'o', FILENAME);
    help(u"output", u"filepath",
         u"Specify the output file name. "
         u"By default, the output file has the same name as the input and extension .bin (compile), .xml or .json (decompile). "
         u"If the specified path is a directory, the output file is built from this directory and default file name. "
         u"If the specified name is \"-\", the standard output is used.\n\n"
         u"The default output file for the standard input (\"-\") is the standard output (\"-\"). "
         u"If more than one input file is specified, the output path, if present, must be either a directory name or \"-\".");

    option(u"xml-model", 'x');
    help(u"xml-model",
         u"Display the XML model of the table files. This model is not a full "
         u"XML-Schema, this is an informal template file which describes the "
         u"expected syntax of TSDuck XML files. If --output is specified, save "
         u"the model here. Do not specify input files.");

    analyze(argc, argv);

    duck.loadArgs(*this);
    sectionOptions.loadArgs(duck, *this);
    xmlTweaks.loadArgs(duck, *this);

    getValues(inFiles, u"");
    getValue(outFile, u"output");
    compile = present(u"compile");
    decompile = present(u"decompile");
    fromJSON = present(u"from-json");
    toJSON = present(u"json") || outFile.endWith(ts::SectionFile::DEFAULT_JSON_SECTION_FILE_SUFFIX);
    xmlModel = present(u"xml-model");
    withExtensions = present(u"extensions");
    useStdIn = ts::UString(u"-").isContainedSimilarIn(inFiles);
    useStdOut = outFile == u"-";
    outIsDir = !useStdOut && !outFile.empty() && ts::IsDirectory(outFile);

    if (useStdOut) {
        outFile.clear();
    }
    if (!inFiles.empty() && xmlModel) {
        error(u"do not specify input files with --xml-model");
    }
    if (useStdIn && !compile && !decompile) {
        error(u"with standard input, --compile or --decompile must be specified");
    }
    if (inFiles.size() > 1 && !outFile.empty() && !useStdOut && !outIsDir) {
        error(u"with more than one input file, --output must be a directory or standard output");
    }
    if (compile && decompile) {
        error(u"specify either --compile or --decompile but not both");
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Display the XML model.
//----------------------------------------------------------------------------

namespace {
    bool DisplayModel(Options& opt)
    {
        // Save to a file. Default to stdout.
        ts::UString outName(opt.outFile);
        if (opt.outIsDir) {
            // Specified output is a directory, add default name.
            outName.push_back(ts::PathSeparator);
            outName.append(ts::SectionFile::XML_TABLES_MODEL);
        }
        if (!outName.empty()) {
            opt.verbose(u"saving model file to %s", {outName});
        }

        // Load and save the model.
        ts::xml::Document doc;
        return ts::SectionFile::LoadModel(doc, opt.withExtensions) && doc.save(outName);
    }
}


//----------------------------------------------------------------------------
//  Process one file. Return true on success, false on error.
//----------------------------------------------------------------------------

namespace {
    bool ProcessFile(Options& opt, const ts::UString& infile)
    {
        typedef ts::SectionFile::FileType FType;

        const FType inType = opt.fromJSON ? FType::JSON : ts::SectionFile::GetFileType(infile);
        const bool useStdIn = infile.empty() || infile == u"-";
        const bool useStdOut = opt.useStdOut || (useStdIn && opt.outFile.empty());
        const bool compile = opt.compile || inType == FType::XML || inType == FType::JSON;
        const bool decompile = opt.decompile || inType == FType::BINARY;
        const FType outType = compile ? FType::BINARY : (opt.toJSON ? FType::JSON : FType::XML);

        // Set standard input or output in binary mode when necessary.
        if (useStdIn && decompile) {
            ts::SetBinaryModeStdin(opt);
        }
        if (useStdOut && compile) {
            ts::SetBinaryModeStdout(opt);
        }

        // Compute output file name with default file type.
        ts::UString outname(opt.outFile);
        if (!useStdOut) {
            if (outname.empty()) {
                outname = ts::SectionFile::BuildFileName(infile, outType);
            }
            else if (opt.outIsDir) {
                outname += ts::PathSeparator + ts::SectionFile::BuildFileName(ts::BaseName(infile), outType);
            }
        }

        ts::SectionFile file(opt.duck);
        file.setTweaks(opt.xmlTweaks);
        file.setCRCValidation(ts::CRC32::CHECK);

        ts::ReportWithPrefix report(opt, (useStdIn ? u"stdin" : ts::BaseName(infile)) + u": ");

        // Process the input file, starting with error cases.
        if (!compile && !decompile) {
            opt.error(u"don't know what to do with file %s, unknown file type, specify --compile or --decompile", {infile});
            return false;
        }
        else if (compile && inType == FType::BINARY) {
            opt.error(u"cannot compile binary file %s", {infile});
            return false;
        }
        else if (decompile && (inType == FType::XML || inType == FType::JSON)) {
            opt.error(u"cannot decompile XML or JSON file %s", {infile});
            return false;
        }
        else if (compile) {
            // Load XML file and save binary sections.
            opt.verbose(u"Compiling %s to %s", {infile, outname});
            return (inType == FType::JSON ? file.loadJSON(infile) : file.loadXML(infile)) &&
                   opt.sectionOptions.processSectionFile(file, opt) &&
                   file.saveBinary(outname);
        }
        else {
            // Load binary sections and save XML file.
            opt.verbose(u"Decompiling %s to %s", {infile, outname});
            return file.loadBinary(infile) &&
                   opt.sectionOptions.processSectionFile(file, opt) &&
                   (opt.toJSON ? file.saveJSON(outname) : file.saveXML(outname));
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);
    bool ok = true;
    if (opt.xmlModel) {
        ok = DisplayModel(opt);
    }
    else {
        for (size_t i = 0; i < opt.inFiles.size(); ++i) {
            if (!opt.inFiles[i].empty()) {
                ok = ProcessFile(opt, opt.inFiles[i]) && ok;
            }
        }
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
