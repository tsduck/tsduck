//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  Report options for the class TSAnalyzer.
//
//----------------------------------------------------------------------------

#include "tsTSAnalyzerOptions.h"
#include "tsException.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSAnalyzerOptions::TSAnalyzerOptions() :
    ts_analysis(false),
    service_analysis(false),
    service_analysis_decimal_pids(false),
    pid_analysis(false),
    table_analysis(false),
    error_analysis(false),
    normalized(false),
    service_list(false),
    pid_list(false),
    global_pid_list(false),
    unreferenced_pid_list(false),
    pes_pid_list(false),
    service_pid_list(false),
    service_id(0),
    prefix(),
    title(),
    suspect_min_error_count(1),
    suspect_max_consecutive(1),
    default_charset(0)
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TSAnalyzerOptions::defineOptions(Args& args) const
{
    args.option(u"ts-analysis");
    args.help(u"ts-analysis",
              u"Report global transport stream analysis.\n\n"
              u"The output can include full synthetic analysis (options *-analysis), "
              u"fully normalized output (option --normalized) or a simple list of "
              u"values on one line (options --*-list). The second and third type of "
              u"options are useful to write automated scripts.\n\n"
              u"If output-control options are specified, only the selected outputs "
              u"are produced. If no option is given, the default is: "
              u"--ts-analysis --service-analysis --pid-analysis --table-analysis");

    args.option(u"service-analysis");
    args.help(u"service-analysis", u"Report analysis for each service.");

    args.option(u"service-analysis-decimal-pids");
    args.help(u"service-analysis-decimal-pids", u"Include decimal pids in service analysis.");

    args.option(u"pid-analysis");
    args.help(u"pid-analysis", u"Report analysis for each PID.");

    args.option(u"table-analysis");
    args.help(u"table-analysis", u"Report analysis for each table.");

    args.option(u"error-analysis");
    args.help(u"error-analysis", u"Report analysis about detected errors.");

    args.option(u"normalized");
    args.help(u"normalized",
              u"Complete report about the transport stream, the services and the "
              u"PID's in a normalized output format (useful for automatic analysis).");

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

    args.option(u"default-charset", 0, Args::STRING);
    args.help(u"default-charset",
              u"Default character set to use when interpreting DVB strings without "
              u"explicit character table code. According to DVB standard ETSI EN 300 468, "
              u"the default DVB character set is ISO-6937. However, some bogus "
              u"signalization may assume that the default character set is different, "
              u"typically the usual local character table for the region. This option "
              u"forces a non-standard character table. The available table names are " +
              UString::Join(DVBCharset::GetAllNames()) + u".");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

void ts::TSAnalyzerOptions::load(Args& args)
{
    ts_analysis = args.present(u"ts-analysis");
    service_analysis = args.present(u"service-analysis");
    service_analysis_decimal_pids = args.present(u"service-analysis-decimal-pids");
    pid_analysis = args.present(u"pid-analysis");
    table_analysis = args.present(u"table-analysis");
    error_analysis = args.present(u"error-analysis");
    normalized = args.present(u"normalized");
    service_list = args.present(u"service-list");
    pid_list = args.present(u"pid-list");
    global_pid_list = args.present(u"global-pid-list");
    unreferenced_pid_list = args.present(u"unreferenced-pid-list");
    pes_pid_list = args.present(u"pes-pid-list");
    service_pid_list = args.present(u"service-pid-list");
    service_id = args.intValue<uint16_t>(u"service-pid-list");
    prefix = args.value(u"prefix");
    title = args.value(u"title");
    suspect_min_error_count = args.intValue<uint64_t>(u"suspect-min-error-count", 1);
    suspect_max_consecutive = args.intValue<uint64_t>(u"suspect-max-consecutive", 1);

    // Get default DVB character set.
    const UString csName(args.value(u"default-charset"));
    if (!csName.empty() && (default_charset = DVBCharset::GetCharset(csName)) == 0) {
        args.error(u"invalid character set name '%s", {csName});
    }

    // Default: --ts-analysis --service-analysis --pid-analysis
    if (!ts_analysis &&
        !service_analysis &&
        !pid_analysis &&
        !table_analysis &&
        !error_analysis &&
        !normalized &&
        !service_list &&
        !pid_list &&
        !global_pid_list &&
        !unreferenced_pid_list &&
        !pes_pid_list &&
        !service_pid_list)
    {
        ts_analysis = service_analysis = pid_analysis = table_analysis = true;
    }
}
