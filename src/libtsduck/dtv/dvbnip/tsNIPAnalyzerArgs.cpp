//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIPAnalyzerArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Check if something specific was required.
//----------------------------------------------------------------------------

bool ts::NIPAnalyzerArgs::none() const
{
    return ts::FluteDemuxArgs::none() && !summary &&
        save_nif.empty() && save_sif.empty() && save_slep.empty() && save_bootstrap.empty() && save_dvbgw_dir.empty();
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::NIPAnalyzerArgs::defineArgs(Args& args)
{
    // Define arguments from superclass.
    FluteDemuxArgs::defineArgs(args);

    args.option(u"save-bootstrap", 0, Args::FILENAME);
    args.help(u"save-bootstrap",
              u"Save the bootstrap multicast gateway configuration in the specified file. "
              u"This is a XML file. "
              u"If the specified path is '-', the file is written to standard output.");

    args.option(u"save-dvb-gw", 0, Args::DIRECTORY);
    args.help(u"save-dvb-gw",
              u"Save all files in the DVB-NIP carousel with URI starting with http://dvb.gw/. "
              u"The specified path is a directory. "
              u"The file hierarchy is recreated from this directory. "
              u"Example: with '--save-dvb-gw /save/to', the file http://dvb.gw/operator.com/materials/f.jpg "
              u"is saved as /save/to/operator.com/materials/f.jpg.");

    args.option(u"save-nif", 0, Args::FILENAME);
    args.help(u"save-nif",
              u"Save the DVB-NIP Network Information File (NIF) in the specified file. "
              u"This is a XML file. "
              u"If the specified path is '-', the file is written to standard output.");

    args.option(u"save-sif", 0, Args::FILENAME);
    args.help(u"save-sif",
              u"Save the DVB-NIP Service Information File (SIF) in the specified file. "
              u"This is a XML file. "
              u"If the specified path is '-', the file is written to standard output.");

    args.option(u"save-slep", 0, Args::FILENAME);
    args.help(u"save-slep",
              u"Save the DVB-I Service List Entry Points (SLEP) in the specified file. "
              u"This is a XML file. "
              u"If the specified path is '-', the file is written to standard output.");

    args.option(u"summary");
    args.help(u"summary",
              u"Display a summary of the DVB-NIP session. "
              u"This is the default if no other option is specified.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::NIPAnalyzerArgs::loadArgs(DuckContext& duck, Args& args)
{
    // Decode arguments from superclass.
    bool ok = FluteDemuxArgs::loadArgs(duck, args);

    summary = args.present(u"summary");
    args.getPathValue(save_bootstrap, u"save-bootstrap");
    args.getPathValue(save_dvbgw_dir, u"save-dvb-gw");
    args.getPathValue(save_nif, u"save-nif");
    args.getPathValue(save_sif, u"save-sif");
    args.getPathValue(save_slep, u"save-slep");

    // Default option is --summary.
    summary = summary || none();

    return ok;
}
