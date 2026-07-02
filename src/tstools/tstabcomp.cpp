//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

        ts::DuckContext     duck {this};              // Execution context.
        ts::UStringVector   in_files {};              // Input file names, strings, not fs::path, can be inlined XML or JSON.
        fs::path            out_file {};              // Output file path.
        bool                out_is_dir = false;       // Output name is a directory.
        bool                use_std_in = false;       // At least one input file is the standard input.
        bool                use_std_out = false;      // Use standard output for all input files.
        bool                compile = false;          // Explicit compilation.
        bool                decompile = false;        // Explicit decompilation.
        bool                from_json = false;        // All input files are JSON.
        bool                to_json = false;          // Decompile to JSON.
        bool                xml_model = false;        // Display XML model instead of compilation.
        bool                with_extensions = false;  // XML model with extensions.
        ts::SectionFileArgs section_opts {};          // Section file processing options.
        ts::xml::Tweaks     xml_tweaks {};            // XML formatting options.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"PSI/SI tables compiler", u"[options] filename ...")
{
    duck.defineArgsForStandards(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForCharset(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForFixingPDS(*this);
    section_opts.defineArgs(*this);
    xml_tweaks.defineArgs(*this);

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
    help(u"output",
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
    section_opts.loadArgs(duck, *this);
    xml_tweaks.loadArgs(*this);

    getValues(in_files, u"");
    getPathValue(out_file, u"output");
    compile = present(u"compile");
    decompile = present(u"decompile");
    from_json = present(u"from-json");
    to_json = present(u"json") || ts::UString(out_file.extension()).similar(ts::DEFAULT_JSON_FILE_SUFFIX);
    xml_model = present(u"xml-model");
    with_extensions = present(u"extensions");
    use_std_in = ts::UString(u"-").isContainedSimilarIn(in_files);
    use_std_out = present(u"output") && (out_file.empty() || out_file == u"-");
    out_is_dir = !use_std_out && fs::is_directory(out_file);

    if (use_std_out) {
        out_file.clear();
    }
    if (!in_files.empty() && xml_model) {
        error(u"do not specify input files with --xml-model");
    }
    if (use_std_in && !compile && !decompile) {
        error(u"with standard input, --compile or --decompile must be specified");
    }
    if (in_files.size() > 1 && !out_file.empty() && !use_std_out && !out_is_dir) {
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
        fs::path outName(opt.out_file);
        if (opt.out_is_dir) {
            // Specified output is a directory, add default name.
            outName /= ts::SectionFile::XML_TABLES_MODEL;
        }
        if (!outName.empty()) {
            opt.verbose(u"saving model file to %s", outName);
        }

        // Load and save the model.
        ts::xml::Document doc;
        return ts::SectionFile::LoadModel(doc, opt.with_extensions) && doc.save(outName);
    }
}


//----------------------------------------------------------------------------
//  Process one file. Return true on success, false on error.
//----------------------------------------------------------------------------

namespace {
    bool ProcessFile(Options& opt, const ts::UString& infile)
    {
        const ts::SectionFormat in_type = opt.from_json ? ts::SectionFormat::JSON : ts::GetSectionFileFormat(infile);
        const bool use_std_in = infile.empty() || infile == u"-";
        const bool use_std_out = opt.use_std_out || (use_std_in && opt.out_file.empty());
        const bool compile = opt.compile || in_type == ts::SectionFormat::XML || in_type == ts::SectionFormat::JSON;
        const bool decompile = opt.decompile || in_type == ts::SectionFormat::BINARY;
        const ts::SectionFormat out_type = compile ? ts::SectionFormat::BINARY : (opt.to_json ? ts::SectionFormat::JSON : ts::SectionFormat::XML);

        // Set standard input or output in binary mode when necessary.
        if (use_std_in && decompile) {
            ts::SetBinaryModeStdin(opt);
        }
        if (use_std_out && compile) {
            ts::SetBinaryModeStdout(opt);
        }

        // Compute output file name with default file type.
        fs::path outname(opt.out_file);
        const fs::path inname(infile);
        if (!use_std_out) {
            if (outname.empty()) {
                outname = ts::BuildSectionFileName(inname, out_type);
            }
            else if (opt.out_is_dir) {
                outname /= ts::BuildSectionFileName(inname.filename(), out_type);
            }
        }

        ts::SectionFile file(opt.duck);
        file.setTweaks(opt.xml_tweaks);
        file.setCRCValidation(ts::CRC32::CHECK);

        // Process the input file, starting with error cases.
        if (!compile && !decompile) {
            opt.error(u"don't know what to do with file %s, unknown file type, specify --compile or --decompile", infile);
            return false;
        }
        else if (compile && in_type == ts::SectionFormat::BINARY) {
            opt.error(u"cannot compile binary file %s", infile);
            return false;
        }
        else if (decompile && (in_type == ts::SectionFormat::XML || in_type == ts::SectionFormat::JSON)) {
            opt.error(u"cannot decompile XML or JSON file %s", infile);
            return false;
        }
        else if (compile) {
            // Load XML file and save binary sections.
            opt.verbose(u"Compiling %s to %s", infile, outname);
            return (in_type == ts::SectionFormat::JSON ? file.loadJSON(infile) : file.loadXML(infile)) &&
                   opt.section_opts.processSectionFile(file, opt) &&
                   file.saveBinary(outname);
        }
        else {
            // Load binary sections and save XML file.
            opt.verbose(u"Decompiling %s to %s", infile, outname);
            return file.loadBinary(infile) &&
                   opt.section_opts.processSectionFile(file, opt) &&
                   (opt.to_json ? file.saveJSON(outname) : file.saveXML(outname));
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
    if (opt.xml_model) {
        ok = DisplayModel(opt);
    }
    else {
        for (size_t i = 0; i < opt.in_files.size(); ++i) {
            if (!opt.in_files[i].empty()) {
                ok = ProcessFile(opt, opt.in_files[i]) && ok;
            }
        }
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
