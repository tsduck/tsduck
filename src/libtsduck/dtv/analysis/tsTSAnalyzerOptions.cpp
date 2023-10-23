//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSAnalyzerOptions.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TSAnalyzerOptions::defineArgs(Args& args)
{
    args.option(u"ts-analysis");
    args.help(u"ts-analysis",
              u"Report global transport stream analysis.\n\n"
              u"The output can include full synthetic analysis (options *-analysis), "
              u"fully normalized output (options --normalized and --json) or a simple list of "
              u"values on one line (options --*-list). The second and third type of "
              u"options are useful to write automated scripts.\n\n"
              u"If output-control options are specified, only the selected outputs "
              u"are produced. If no option is given, the default is: "
              u"--ts-analysis --service-analysis --pid-analysis --table-analysis");

    args.option(u"service-analysis");
    args.help(u"service-analysis", u"Report analysis for each service.");

    args.option(u"wide-display", 'w');
    args.help(u"wide-display", u"Use a wider grid display with more information on each line.");

    args.option(u"pid-analysis");
    args.help(u"pid-analysis", u"Report analysis for each PID.");

    args.option(u"table-analysis");
    args.help(u"table-analysis", u"Report analysis for each table.");

    args.option(u"error-analysis");
    args.help(u"error-analysis", u"Report analysis about detected errors.");

    json.defineArgs(args, false, u"Complete report about the transport stream, the services and the PID's in JSON format (useful for automatic analysis).");

    args.option(u"normalized");
    args.help(u"normalized",
              u"Complete report about the transport stream, the services and the "
              u"PID's in a normalized output format (useful for automatic analysis).");

    args.option(u"deterministic");
    args.help(u"deterministic",
              u"Enforce a deterministic and reproduceable output. "
              u"Do not output non-reproduceable information such as system time "
              u"(useful for automated tests).");

    args.option(u"service-list");
    args.help(u"service-list", u"Report the list of all service ids.");

    args.option(u"pid-list");
    args.help(u"pid-list", u"Report the list of all PID's.");

    args.option(u"global-pid-list");
    args.help(u"global-pid-list",
              u"Report the list of all global PID's, that is to say PID's "
              u"which are not referenced by a specific service but are or "
              u"are referenced by the standard DVB PSI/SI. This include, for "
              u"instance, PID's of the PAT, EMM's, EIT's, stuffing, etc.");

    args.option(u"unreferenced-pid-list");
    args.help(u"unreferenced-pid-list",
              u"Report the list of all unreferenced PID's, that is to say "
              u"PID's which are neither referenced by a service nor known "
              u"as or referenced by the standard DVB PSI/SI.");

    args.option(u"pes-pid-list");
    args.help(u"pes-pid-list",
              u"Report the list of all PID's which are declared as carrying "
              u"PES packets (audio, video, subtitles, etc).");

    args.option(u"service-pid-list", 0, Args::UINT16);
    args.help(u"service-pid-list",
              u"Report the list of all PID's which are referenced by the "
              u"specified service id.");

    args.option(u"prefix", 0, Args::STRING);
    args.help(u"prefix",
              u"For one-line displays (options --*-list), prepend the "
              u"specified string to all values. For instance, options "
              u"--global --prefix -p outputs something like '-p 0 -p 1 -p 16', "
              u"which is an acceptable option list for the tsp filter plugin.");

    args.option(u"title", 0, Args::STRING);
    args.help(u"title", u"Display the specified string as title header.");

    args.option(u"suspect-min-error-count", 0, Args::UNSIGNED);
    args.help(u"suspect-min-error-count",
              u"Specifies the minimum number of consecutive packets with errors before "
              u"starting \"suspect\" packet detection. See also option "
              u"--suspect-max-consecutive. The default value is 1. If set to zero, "
              u"the suspect packet detection is disabled.");

    args.option(u"suspect-max-consecutive", 0, Args::UNSIGNED);
    args.help(u"suspect-max-consecutive",
              u"Specifies the maximum number of consecutive \"suspect\" packets. "
              u"The default value is 1. If set to zero, the suspect packet detection "
              u"is disabled.\n\n"
              u"Suspect packets are TS packets which are technically correct but which "
              u"may be suspected of being incorrect, resulting in analysis errors. "
              u"Typically, in the middle of a suite of packets with uncorrectable "
              u"binary errors, one packet may appear to have no such error while "
              u"it has some errors in fact. To avoid adding this type of packets in the "
              u"analysis, a packet is declared as \"suspect\" (and consequently ignored in "
              u"the analysis) when:\n"
              u"- its PID is unknown (no other packet was found in this PID)\n"
              u"- it immediately follows a certain amount of packet containing errors "
              u"(see option --suspect-min-error-count)\n"
              u"- it immediately follows no more than the specified number consecutive "
              u"suspect packets.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TSAnalyzerOptions::loadArgs(DuckContext& duck, Args& args)
{
    ts_analysis = args.present(u"ts-analysis");
    service_analysis = args.present(u"service-analysis");
    wide = args.present(u"wide-display");
    pid_analysis = args.present(u"pid-analysis");
    table_analysis = args.present(u"table-analysis");
    error_analysis = args.present(u"error-analysis");
    normalized = args.present(u"normalized");
    deterministic = args.present(u"deterministic");
    service_list = args.present(u"service-list");
    pid_list = args.present(u"pid-list");
    global_pid_list = args.present(u"global-pid-list");
    unreferenced_pid_list = args.present(u"unreferenced-pid-list");
    pes_pid_list = args.present(u"pes-pid-list");
    service_pid_list = args.present(u"service-pid-list");
    args.getIntValue(service_id, u"service-pid-list");
    args.getValue(prefix, u"prefix");
    args.getValue(title, u"title");
    args.getIntValue(suspect_min_error_count, u"suspect-min-error-count", 1);
    args.getIntValue(suspect_max_consecutive, u"suspect-max-consecutive", 1);

    bool ok = json.loadArgs(duck, args);

    // Default: --ts-analysis --service-analysis --pid-analysis
    if (!ts_analysis &&
        !service_analysis &&
        !pid_analysis &&
        !table_analysis &&
        !error_analysis &&
        !normalized &&
        !json.useJSON() &&
        !service_list &&
        !pid_list &&
        !global_pid_list &&
        !unreferenced_pid_list &&
        !pes_pid_list &&
        !service_pid_list)
    {
        ts_analysis = service_analysis = pid_analysis = table_analysis = true;
    }

    return ok;
}
