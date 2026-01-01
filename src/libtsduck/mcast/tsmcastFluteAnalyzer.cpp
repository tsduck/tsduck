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
    _file_extraction.setRootDirectory(_args.carousel_dir);
    _file_extraction.setDeleteAfter(_args.delete_after);
    return _demux.reset(_args, _args.summary);
}


//----------------------------------------------------------------------------
// Process a FLUTE file.
//----------------------------------------------------------------------------

void ts::mcast::FluteAnalyzer::handleFluteFile(const FluteFile& file)
{
    // Save carousel files.
    if (!_args.carousel_dir.empty() && !file.name().empty()) {
        _file_extraction.saveFile(file.content(), file.name());
    }
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
