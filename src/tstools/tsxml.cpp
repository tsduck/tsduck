//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Test tool for XML manipulation in TSDuck context.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTablePatchXML.h"
#include "tsxmlElement.h"
#include "tsxmlJSONConverter.h"
#include "tsjsonOutputArgs.h"
#include "tsjsonYAML.h"
#include "tsSectionFile.h"
#include "tsOutputRedirector.h"
TS_MAIN(MainCode);

#define DEFAULT_INDENT 2


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext          duck {this};           // TSDuck execution contexts.
        ts::UStringVector        infiles {};            // Input file names.
        fs::path                 outfile {};            // Output file name.
        ts::UString              model {};              // Model file name.
        ts::UStringVector        patches {};            // XML patch files.
        ts::UStringVector        sorted_tags {};        // Sort the content of these tags.
        bool                     reformat = false;      // Reformat input files.
        bool                     uncomment = false;     // Remove comments.
        bool                     xml_line = false;      // Output XML on one single line.
        bool                     tables_model = false;  // Use table model file.
        bool                     use_model = false;     // There is a model to use.
        bool                     from_json = false;     // Perform an automated JSON-to-XML conversion on input.
        bool                     merge_inputs = false;  // Merge all input XML files as one.
        bool                     expand_input = false;  // Expand environment variables in input files.
        bool                     expand_patch = false;  // Expand environment variables in patch files.
        bool                     yaml_output = false;   // Generate an output in YAML format.
        bool                     no_yaml_head = false;  // No YAML header and trailer.
        bool                     need_output = false;   // An output file is needed.
        ts::UString              xml_prefix {};         // Prefix in XML line.
        size_t                   indent = 2;            // Output indentation.
        ts::xml::Tweaks          xml_tweaks {};         // XML formatting options.
        ts::xml::MergeAttributes merge_attr = ts::xml::MergeAttributes::NONE; // How to merge attributes (with merge_inputs);
        ts::json::OutputArgs     json {};               // JSON output options.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Test tool for TSDuck XML manipulation", u"[options] [input-file ...]")
{
    json.defineArgs(*this, true, u"Perform an automated XML-to-JSON conversion. The output file is in JSON format instead of XML.");
    xml_tweaks.defineArgs(*this);

    setIntro(u"Any input XML file name can be replaced with \"inline XML content\", starting with \"<?xml\".");

    option(u"", 0, FILENAME, 0, UNLIMITED_COUNT);
    help(u"", u"Specify the list of input files. If any is specified as '-', the standard input is used.");

    option(u"attributes-merge", 0, ts::Names({
        {u"add",     ts::xml::MergeAttributes::ADD},
        {u"none",    ts::xml::MergeAttributes::NONE},
        {u"replace", ts::xml::MergeAttributes::REPLACE},
    }));
    help(u"attributes-merge", u"name",
         u"With --merge, specify how attributes are processed in merged node. "
         u"The default is \"add\", meaning that new attributes are added, others are ignored.");

    option(u"channel", 'c');
    help(u"channel",
         u"A shortcut for '--model tsduck.channels.model.xml'. "
         u"It verifies that the input files are valid channel configuration files.");

    option(u"expand-environment", 'e');
    help(u"expand-environment",
         u"Expand environment variables in the input XML files. "
         u"Environment variables must be referenced as '${name}'. "
         u"See also option --expand-patch-xml for patch files.");

    option(u"expand-patch-xml");
    help(u"expand-patch-xml",
         u"With --patch, expand all environment variables in the patch files before patching. "
         u"Environment variables must be referenced as '${name}'. "
         u"See also option --expand-environment for input XML files.");

    option(u"from-json", 'f');
    help(u"from-json",
         u"Each input file must be a JSON file, "
         u"typically from a previous automated XML-to-JSON conversion or in a similar format. "
         u"A reverse conversion is first performed and the resulting XML document is processed as input.");

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

    option(u"merge");
    help(u"merge",
         u"Merge all input files as one, instead of processing all input files one by one. "
         u"With this option, all input XML files must have the same root tag.");

    option(u"model", 'm', FILENAME);
    help(u"model", u"filename",
         u"Specify an XML model file which is used to validate all input files.");

    option(u"monitor");
    help(u"monitor",
         u"A shortcut for '--model tscore.monitor.model.xml'. "
         u"It verifies that the input files are valid system monitoring configuration files.");

    option(u"output", 'o', FILENAME);
    help(u"output", u"filename",
         u"Specify the name of the output file (standard output by default). "
         u"An output file is produced only if at least one of --patch, --reformat, --json, --yaml is specified.");

    option(u"patch", 'p', FILENAME, 0, UNLIMITED_COUNT);
    help(u"patch", u"filename",
         u"Specify an XML patch file. All operations which are specified in this file are applied on each input file. "
         u"Several --patch options can be specified. Patch files are sequentially applied on each input file.");

    option(u"reformat", 'r');
    help(u"reformat",
         u"Reformat the input XML files according to the default XML layout for TSDuck XML files. "
         u"This option is useful to generate an expected output file format. "
         u"If more than one input file is specified, they are all reformatted in the same output file.");

    option(u"sort", 's', STRING, 0, UNLIMITED_COUNT);
    help(u"sort", u"name",
         u"Specify that the sub-elements of all XML structures with the specified tag name will be sorted in alphanumerical order. "
         u"Several --sort options can be specified.");

    option(u"tables", 't');
    help(u"tables",
         u"A shortcut for '--model " + ts::UString(ts::SectionFile::XML_TABLES_MODEL) + "'. "
         u"Table definitions for installed TSDuck extensions are also merged in the main model. "
         u"It verifies that the input files are valid PSI/SI tables files.");

    option(u"uncomment");
    help(u"uncomment",
         u"Remove comments from the XML documents.");

    option(u"xml-line", 0, STRING, 0, 1, 0, UNLIMITED_VALUE, true);
    help(u"xml-line", u"'prefix'",
         u"Log each table as one single XML line in the message logger instead of an output file. "
         u"The optional string parameter specifies a prefix to prepend on the log "
         u"line before the XML text to locate the appropriate line in the logs.");

    option(u"yaml", 'y');
    help(u"yaml",
         u"Convert the XML content to YAML (experimental).");

    option(u"no-yaml-header");
    help(u"no-yaml-header",
         u"With --yaml, do not add the standard YAML header and trailer.");

    analyze(argc, argv);

    json.loadArgs(*this);
    xml_tweaks.loadArgs(*this);

    getValues(infiles, u"");
    getValues(patches, u"patch");
    getValues(sorted_tags, u"sort");
    getPathValue(outfile, u"output");
    getIntValue(indent, u"indent", 2);
    getValue(xml_prefix, u"xml-line");
    getIntValue(merge_attr, u"attributes-merge", ts::xml::MergeAttributes::ADD);
    reformat = present(u"reformat") || !patches.empty();
    xml_line = present(u"xml-line");
    from_json = present(u"from-json");
    yaml_output = present(u"yaml");
    no_yaml_head = present(u"no-yaml-header");
    merge_inputs = present(u"merge");
    expand_input = present(u"expand-environment");
    expand_patch = present(u"expand-patch-xml");
    uncomment = present(u"uncomment");

    // Get model file.
    if (present(u"model") + present(u"tables") + present(u"channel") + present(u"hf-band") + present(u"lnb") > 1) {
        error(u"more than one XML model is specified");
    }
    getValue(model, u"model");
    tables_model = present(u"tables");
    if (present(u"channel")) {
        model = u"tsduck.channels.model.xml";
    }
    else if (present(u"hf-band")) {
        model = u"tsduck.hfbands.model.xml";
    }
    else if (present(u"lnb")) {
        model = u"tsduck.lnbs.model.xml";
    }
    use_model = tables_model || !model.empty();

    // An input file named "" or "-" means standard input.
    for (auto& it : infiles) {
        if (it == u"-") {
            it.clear();
        }
    }

    // An output file wil be produced.
    need_output = reformat || uncomment || merge_inputs || json.useFile() || yaml_output || from_json || expand_input;

    exitOnError();
}


//----------------------------------------------------------------------------
// Load a document.
//----------------------------------------------------------------------------

namespace {
    bool LoadDocument(Options& opt, const ts::xml::JSONConverter& model, ts::xml::Document& doc, const ts::UString& file_name)
    {
        doc.setTweaks(opt.xml_tweaks);
        bool ok = true;

        if (opt.from_json) {
            // Load a JSON fil and convert it to XML.
            ts::json::ValuePtr root;
            ok = ts::json::LoadFile(root, file_name, opt) && model.convertToXML(*root, doc, false);
        }
        else {
            // Load a true XML file.
            ok = doc.load(file_name, false);
        }

        if (!ok) {
            opt.error(u"error loading %s", ts::xml::Document::DisplayFileName(file_name, true));
            return false;
        }

        // Validate the file according to the model.
        if (opt.use_model && !model.validate(doc)) {
            opt.error(u"%s is not conformant with the XML model", ts::xml::Document::DisplayFileName(file_name, true));
            return false;
        }

        return doc.rootElement() != nullptr;
    }
}


//----------------------------------------------------------------------------
// Process a document.
//----------------------------------------------------------------------------

namespace {
    void ProcessDocument(Options& opt, const ts::TablePatchXML& patch, ts::xml::Document& doc)
    {
        // Expand environment variables.
        if (opt.expand_input) {
            doc.expandEnvironment(true);
        }

        // Apply all patches one by one.
        patch.applyPatches(doc);

        // Remove comments.
        if (opt.uncomment) {
            doc.removeComments(true);
        }

        // Sort the content of the specified tags.
        ts::xml::Element* root = doc.rootElement();
        if (root != nullptr) {
            for (const auto& name : opt.sorted_tags) {
                root->sort(name);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Save a document.
//----------------------------------------------------------------------------

namespace {
    void SaveDocument(Options& opt, const ts::xml::JSONConverter& model, ts::xml::Document& doc)
    {
        if (opt.xml_line) {
            // Output XML result as one line on error log.
            opt.info(opt.xml_prefix + doc.oneLiner());
        }
        if (opt.json.useJSON() || opt.yaml_output) {
            // Perform XML to JSON conversion.
            const ts::json::ValuePtr jobj(model.convertToJSON(doc));
            // Output JSON and/or YAML result.
            if (opt.json.useJSON()) {
                opt.json.report(*jobj, std::cout, opt);
            }
            if (opt.yaml_output) {
                ts::TextFormatter text(opt);
                text.setStream(std::cout);
                ts::json::YAML::PrintAsYAML(text, *jobj, !opt.no_yaml_head);
            }
        }
        else if (opt.need_output) {
            // Same XML output on stdout (possibly already redirected to a file).
            doc.save(u"", opt.indent);
        }
    }
}


//----------------------------------------------------------------------------
// Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    Options opt(argc, argv);

    // Load the model file if any is specified.
    // Note that JSONConverter is a subclass of ModelDocument.
    // The object named 'model' can be used either as a model and a JSON converter.
    ts::xml::JSONConverter model(opt);
    model.setTweaks(opt.xml_tweaks);
    bool model_ok = true;
    if (opt.tables_model) {
        model_ok = ts::SectionFile::LoadModel(model);
    }
    else if (!opt.model.empty()) {
        model_ok = model.load(opt.model, true);
    }
    if (!model_ok) {
        opt.error(u"error loading model files, cannot validate input files");
    }

    // Load patch files.
    ts::TablePatchXML patch(opt.duck);
    patch.setExpandEnvironment(opt.expand_patch);
    patch.addPatchFileNames(opt.patches);
    patch.loadPatchFiles(opt.xml_tweaks);
    opt.exitOnError();

    // Redirect standard output only if required.
    ts::OutputRedirector out(opt.need_output ? opt.outfile : fs::path(), opt, std::cout, std::ios::out);

    if (opt.merge_inputs && opt.infiles.size() > 1) {
        // Load all input files as one merged document.
        ts::xml::Document doc(opt);
        bool ok = LoadDocument(opt, model, doc, opt.infiles[0]);
        for (size_t i = 1; ok && i < opt.infiles.size(); ++i) {
            ts::xml::Document subdoc(opt);
            ok = LoadDocument(opt, model, subdoc, opt.infiles[i]) && doc.rootElement()->merge(subdoc.rootElement(), opt.merge_attr);
        }
        if (ok) {
            ProcessDocument(opt, patch, doc);
            SaveDocument(opt, model, doc);
        }
    }
    else {
        // Process each input file one by one.
        for (const auto& file : opt.infiles) {
            ts::xml::Document doc(opt);
            if (LoadDocument(opt, model, doc, file)) {
                ProcessDocument(opt, patch, doc);
                SaveDocument(opt, model, doc);
            }
        }
    }

    return opt.valid() && !opt.gotErrors() ? EXIT_SUCCESS : EXIT_FAILURE;
}
