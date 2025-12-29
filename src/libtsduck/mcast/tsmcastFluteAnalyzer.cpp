//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFluteAnalyzer.h"
#include "tsErrCodeReport.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::mcast::FluteAnalyzer::FluteAnalyzer(DuckContext& duck) :
    _duck(duck)
{
}


//----------------------------------------------------------------------------
// Reset the analysis.
//----------------------------------------------------------------------------

bool ts::mcast::FluteAnalyzer::reset(const FluteAnalyzerArgs& args)
{
    // Check that the root directory exists for carousel files.
    if (!args.carousel_dir.empty() && !fs::is_directory(args.carousel_dir)) {
        _report.error(u"directory not found: %s", args.carousel_dir);
        return false;
    }

    // Local initialization.
    _args = args;
    return _demux.reset(_args, _args.summary);
}


//----------------------------------------------------------------------------
// Process a FLUTE file.
//----------------------------------------------------------------------------

void ts::mcast::FluteAnalyzer::handleFluteFile(const FluteFile& file)
{
    // Save carousel files.
    if (!_args.carousel_dir.empty() && !file.name().empty()) {
        saveFile(file);
    }
}


//----------------------------------------------------------------------------
// Save a carousel file.
//----------------------------------------------------------------------------

void ts::mcast::FluteAnalyzer::saveFile(const FluteFile& file)
{
    // Build the output path. Remove URI scheme if present.
    UString path(file.name());
    size_t sep = path.find(u"://");
    if (sep < path.length()) {
        path.erase(0, sep + 3);
    }

    // Replace forbidden characters with underscores.
    for (auto& c : path) {
#if defined(TS_WINDOWS)
        static const UString forbidden(u"()[]{}:");
#else
        static const UString forbidden(u"()[]{}");
#endif
        if (forbidden.contains(c)) {
            c = '_';
        }
    }

    // Cleanup the file path to avoid directory traversal attack.
    UStringVector comp;
    path.split(comp, u'/', true, true);
    fs::path outpath(_args.carousel_dir);
    fs::path basename;
    for (size_t i = 0; i < comp.size(); ++i) {
        if (comp[i] != u"." && comp[i] != u"..") {
            if (i + 1 < comp.size()) {
                outpath /= comp[i];
            }
            else {
                basename = comp[i];
            }
        }
    }
    if (basename.empty()) {
        _report.error(u"no base name specified in \"%s\"", file.name());
        return;
    }

    // Create intermediate subdirectories if required.
    fs::create_directories(outpath, &ErrCodeReport(_report, u"error creating directory", outpath));

    // Save final file.
    outpath /= basename;
    _report.verbose(u"saving %s", outpath);
    file.content().saveToFile(outpath, &_report);
}


//----------------------------------------------------------------------------
// Print a summary of the DVB-NIP session.
//----------------------------------------------------------------------------

void ts::mcast::FluteAnalyzer::printSummary(std::ostream& user_output)
{
    // Create the user-specified output file if required.
    const bool use_file = !_args.output_file.empty() && _args.output_file != u"-";
    std::ofstream outfile;
    if (use_file) {
        outfile.open(_args.output_file);
        if (!outfile) {
            _report.error(u"error creating %s", _args.output_file);
        }
    }
    std::ostream& out(use_file ? outfile : user_output);

    // Display the status of all files.
    out << std::endl;
    _demux.printFilesStatus(out);

    // Close the user-specified output file if required.
    if (use_file) {
        outfile.close();
    }
}
