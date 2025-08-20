//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Amos Cheung
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLatencyMonitorArgs.h"
#include "tsArgsWithPlugins.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::LatencyMonitorArgs::defineArgs(Args& args)
{
    args.option(u"output-file", 'o', Args::FILENAME);
    args.help(u"output-file", u"filename",
              u"Output file name for CSV reporting (standard error by default).");

    args.option<cn::seconds>(u"buffer-time", 'b');
    args.help(u"buffer-time",
              u"Specify the buffer time of timing data list in seconds. "
              u"By default, the buffer time is 1 seconds.");

    args.option<cn::seconds>(u"output-interval");
    args.help(u"output-interval",
              u"Specify the time interval between each output in seconds. "
              u"The default is 1 second.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::LatencyMonitorArgs::loadArgs(Args& args)
{
    app_name = args.appName();
    args.getPathValue(output_name, u"output-file");
    args.getChronoValue(buffer_time, u"buffer-time", cn::seconds(1));
    args.getChronoValue(output_interval, u"output-interval", cn::seconds(1));

    // Load all plugin descriptions. Default output is the standard output file.
    ArgsWithPlugins* pargs = dynamic_cast<ArgsWithPlugins*>(&args);
    if (pargs != nullptr) {
        pargs->getPlugins(inputs, PluginType::INPUT);
    }

    return args.valid();
}
