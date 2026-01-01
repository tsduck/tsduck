//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFluteAnalyzerArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Check if something specific was required.
//----------------------------------------------------------------------------

bool ts::mcast::FluteAnalyzerArgs::none(bool except_summary) const
{
    return FluteDemuxArgs::none() && (except_summary || !summary) && carousel_dir.empty();
}


//----------------------------------------------------------------------------
// Check if an IP socket address is a valid destination.
//----------------------------------------------------------------------------

bool ts::mcast::FluteAnalyzerArgs::isDestination(const IPSocketAddress& addr) const
{
    if (destinations.empty()) {
        return true;
    }
    else {
        for (const auto& a : destinations) {
            if (addr.match(a)) {
                return true;
            }
        }
        return false;
    }
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::mcast::FluteAnalyzerArgs::defineArgs(Args& args)
{
    // Define arguments from superclass.
    FluteDemuxArgs::defineArgs(args);

    args.option(u"destination", 'd', Args::IPSOCKADDR_OAP, 0, Args::UNLIMITED_COUNT);
    args.help(u"destination",
              u"Only use UDP packets with the specified destination IP address and/or UDP port. "
              u"Multiple options --destination can be specified. "
              u"By default, use all UDP packets.");

    args.option(u"extract-carousel", 0, Args::DIRECTORY);
    args.help(u"extract-carousel",
              u"Save all files in the FLUTE carousel. "
              u"The specified path is a directory. "
              u"The file hierarchy is recreated from this directory. "
              u"When a FLUTE file name is an URI, the URI scheme is removed. "
              u"The characters which are not allowed in file names are replaced with an underscore.");

    args.option<cn::seconds>(u"delete-after");
    args.help(u"delete-after",
              u"With --extract-carousel, delete the extracted files the specified number of seconds after their creation. "
              u"This option is useful to prevent disk overflow when the file extraction runs continuously.");

    args.option(u"output-file", 'o', Args::FILENAME);
    args.help(u"output-file",
              u"With --summary, save the report in the specified file. "
              u"By default or if the specified path is '-', the report is written to standard output.");

    args.option(u"summary");
    args.help(u"summary",
              u"Display a summary of the FLUTE sessions and files. "
              u"This is the default if no other option is specified.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::mcast::FluteAnalyzerArgs::loadArgs(DuckContext& duck, Args& args)
{
    // Decode arguments from superclass.
    bool ok = FluteDemuxArgs::loadArgs(duck, args);

    summary = args.present(u"summary");
    args.getPathValue(output_file, u"output-file");
    args.getPathValue(carousel_dir, u"extract-carousel");
    args.getChronoValue(delete_after, u"delete-after");
    args.getSocketValues(destinations, u"destination");

    // Default option is --summary.
    summary = summary || none();

    return ok;
}
