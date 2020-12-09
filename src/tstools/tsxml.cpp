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
//  Test tool for XML manipulation in TSDuck context.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsPatchXML.h"
#include "tsxmlModelDocument.h"
#include "tsxmlPatchDocument.h"
#include "tsOutputRedirector.h"
#include "tsSafePtr.h"
#include "tsFatal.h"
TSDUCK_SOURCE;
TS_MAIN(MainCode);

#define DEFAULT_INDENT 2


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext   duck;        // TSDuck execution contexts.
        ts::UStringVector infiles;     // Input file names.
        ts::UString       outfile;     // Output file name.
        ts::UString       model;       // Model file name.
        ts::UStringVector patches;     // XML patch files,.
        bool              reformat;    // Reformat input files.
        size_t            indent;      // Output indentation.
        ts::xml::Tweaks   xml_tweaks;  // XML formatting options.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Test tool for TSDuck XML manipulation", u"[options] [input-file ...]"),
    duck(this),
    infiles(),
    outfile(),
    model(),
    patches(),
    reformat(false),
    indent(2),
    xml_tweaks()
{
    xml_tweaks.defineArgs(*this);

    setIntro(u"Any input XML file name can be replaced with \"inline XML content\", starting with \"<?xml\".");

    option(u"", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"", u"Specify the list of input files. If any is specified as '-', the standard input is used.");

    option(u"channel", 'c');
    help(u"channel",
         u"A shortcut for '--model tsduck.channels.model.xml'. "
         u"It verifies that the input files are valid channel configuration files.");

    option(u"hf-band", 'h');
    help(u"hf-band",
         u"A shortcut for '--model tsduck.hfbands.model.xml'. "
         u"It verifies that the input files are valid HF bands definition files.");

    option(u"indent", 'i', UNSIGNED);
    help(u"indent",
         u"Specify the indentation size of output files. "
         u"The default is " TS_USTRINGIFY(DEFAULT_INDENT) u".");

    option(u"lnb", 'l');
    help(u"lnb",
         u"A shortcut for '--model tsduck.lnbs.model.xml'. "
         u"It verifies that the input files are valid satellite LNB definition files.");

    option(u"model", 'm', STRING);
    help(u"model", u"filename",
         u"Specify an XML model file which is used to validate all input files.");

    option(u"output", 'o', STRING);
    help(u"output", u"filename",
         u"Specify the name of the output file (standard output by default). "
         u"An output file is produced only if --patch or --reformat are specified.");

    option(u"patch", 'p', STRING, 0, UNLIMITED_COUNT);
    help(u"patch", u"filename",
         u"Specify an XML patch file. All operations which are specified in this file are applied on each input file. "
         u"Several --patch options can be specified. Patch files are sequentially applied on each input file.");

    option(u"reformat", 'r');
    help(u"reformat",
         u"Reformat the input XML files according to the default XML layout for TSDuck XML files. "
         u"This option is useful to generate an expected output file format. "
         u"If more than one input file is specified, they are all reformatted in the same output file.");

    option(u"tables", 't');
    help(u"tables",
         u"A shortcut for '--model tsduck.tables.model.xml'. "
         u"It verifies that the input files are valid PSI/SI tables files.");

    analyze(argc, argv);

    xml_tweaks.loadArgs(duck, *this);

    getValues(infiles, u"");
    getValues(patches, u"patch");
    getValue(outfile, u"output");
    getValue(model, u"model");
    getIntValue(indent, u"indent", 2);
    reformat = present(u"reformat") || !patches.empty();

    // Predefined models.
    if (present(u"channel")) {
        model = u"tsduck.channels.model.xml";
    }
    else if (present(u"hf-band")) {
        model = u"tsduck.hfbands.model.xml";
    }
    else if (present(u"lnb")) {
        model = u"tsduck.lnbs.model.xml";
    }
    else if (present(u"tables")) {
        model = u"tsduck.tables.model.xml";
    }

    // An input file named "" or "-" means standard input.
    for (auto it = infiles.begin(); it != infiles.end(); ++it) {
        if (*it == u"-") {
            it->clear();
        }
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    Options opt(argc, argv);

    // Load the model file if any is specified.
    ts::xml::ModelDocument model(opt);
    model.setTweaks(opt.xml_tweaks);
    if (!opt.model.empty() && !model.load(opt.model, true)) {
        opt.error(u"error loading model files, cannot validate input files");
    }

    // Load patch files.
    ts::PatchXML patch(opt.duck);
    patch.addPatchFileNames(opt.patches);
    patch.loadPatchFiles(opt.xml_tweaks);
    opt.exitOnError();

    // Redirect standard output only if required.
    ts::OutputRedirector out(opt.reformat ? opt.outfile : u"", opt, std::cout, std::ios::out);

    // Now process each input file one by one.
    for (size_t i = 0; i < opt.infiles.size(); ++i) {

        const ts::UString& file_name(opt.infiles[i]);
        const ts::UString display_name(ts::xml::Document::DisplayFileName(file_name, true));

        // Load the input file.
        ts::xml::Document doc(opt);
        doc.setTweaks(opt.xml_tweaks);
        bool ok = doc.load(file_name, false, true);
        if (!ok) {
            opt.error(u"error loading %s", {display_name});
        }

        // Validate the file according to the model.
        if (ok && !opt.model.empty() && !model.validate(doc)) {
            opt.error(u"%s is not conformant with the XML model", {display_name});
            ok = false;
        }

        // Apply all patches one by one.
        patch.applyPatches(doc);

        // Output the modified / reformatted document.
        if (ok && opt.reformat) {
            doc.save(u"", opt.indent, true);
        }
    }
    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
