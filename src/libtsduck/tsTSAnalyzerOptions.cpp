//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
// Set help: application specific help + generic help
//----------------------------------------------------------------------------

void ts::TSAnalyzerOptions::setHelp(const UString& help)
{
    Args::setHelp (help +
        u"\n"
        u"Controlling analysis:\n"
        u"\n"
        u"  --suspect-max-consecutive value\n"
        u"      Specifies the maximum number of consecutive \"suspect\" packets.\n"
        u"      The default value is 1. If set to zero, the suspect packet detection\n"
        u"      is disabled.\n"
        u"\n"
        u"      Suspect packets are TS packets which are technically correct but which\n"
        u"      may be suspected of being incorrect, resulting in analysis errors.\n"
        u"      Typically, in the middle of a suite of packets with uncorrectable\n"
        u"      binary errors, one packet may appear to have no such error while\n"
        u"      it has some errors in fact. To avoid adding this type of packets in the\n"
        u"      analysis, a packet is declared as \"suspect\" (and consequently ignored in\n"
        u"      the analysis) when:\n"
        u"      - its PID is unknown (no other packet was found in this PID)\n"
        u"      - it immediately follows a certain amount of packet containing errors\n"
        u"        (see option --suspect-min-error-count)\n"
        u"      - it immediately follows no more than the specified number consecutive\n"
        u"        suspect packets.\n"
        u"\n"
        u"  --suspect-min-error-count value\n"
        u"      Specifies the minimum number of consecutive packets with errors before\n"
        u"      starting \"suspect\" packet detection. See also option\n"
        u"      --suspect-max-consecutive. The default value is 1. If set to zero,\n"
        u"      the suspect packet detection is disabled.\n"
        u"\n"
        u"Controlling output:\n"
        u"\n"
        u"  The output can include full synthetic analysis (options *-analysis),\n"
        u"  fully normalized output (option --normalized) or a simple list of\n"
        u"  values on one line (options --*-list). The second and third type of\n"
        u"  options are useful to write automated scripts.\n"
        u"\n"
        u"  If output-control options are specified, only the selected outputs\n"
        u"  are produced. If no option is given, the default is:\n"
        u"  --ts-analysis --service-analysis --pid-analysis --table-analysis\n"
        u"\n"
        u"  --ts-analysis\n"
        u"      Report global transport stream analysis.\n"
        u"\n"
        u"  --service-analysis\n"
        u"      Report analysis for each service.\n"
        u"\n"
        u"  --pid-analysis\n"
        u"      Report analysis for each PID.\n"
        u"\n"
        u"  --table-analysis\n"
        u"      Report analysis for each table.\n"
        u"\n"
        u"  --error-analysis\n"
        u"      Report analysis about detected errors.\n"
        u"\n"
        u"  --normalized\n"
        u"      Complete report about the transport stream, the services and\n"
        u"      the PID's in a normalized output format (useful for automatic\n"
        u"      analysis).\n"
        u"\n"
        u"  --service-list\n"
        u"      Report the list of all service ids.\n"
        u"\n"
        u"  --pid-list\n"
        u"      Report the list of all PID's.\n"
        u"\n"
        u"  --global-pid-list\n"
        u"      Report the list of all global PID's, that is to say PID's\n"
        u"      which are not referenced by a specific service but are or\n"
        u"      are referenced by the standard DVB PSI/SI. This include, for\n"
        u"      instance, PID's of the PAT, EMM's, EIT's, stuffing, etc.\n"
        u"\n"
        u"  --unreferenced-pid-list\n"
        u"      Report the list of all unreferenced PID's, that is to say\n"
        u"      PID's which are neither referenced by a service nor known\n"
        u"      as or referenced by the standard DVB PSI/SI.\n"
        u"\n"
        u"  --service-pid-list value\n"
        u"      Report the list of all PID's which are referenced by the\n"
        u"      specified service id.\n"
        u"\n"
        u"  --pes-pid-list\n"
        u"      Report the list of all PID's which are declared as carrying\n"
        u"      PES packets (audio, video, subtitles, etc).\n"
        u"\n"
        u"  --title string\n"
        u"      Display the specified string as title header.\n"
        u"\n"
        u"  --prefix string\n"
        u"      For one-line displays (options --*-list), prepend the\n"
        u"      specified string to all values. For instance, options\n"
        u"      --global --prefix -p outputs something like '-p 0 -p 1 -p 16',\n"
        u"      which is an acceptable option list for the tsp filter plugin.\n");
}


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSAnalyzerOptions::TSAnalyzerOptions(const UString& description, const UString& syntax, const UString& help, int flags) :
    Args(description, syntax, u"", flags),
    ts_analysis (false),
    service_analysis (false),
    pid_analysis (false),
    table_analysis (false),
    error_analysis (false),
    normalized (false),
    service_list (false),
    pid_list (false),
    global_pid_list (false),
    unreferenced_pid_list (false),
    pes_pid_list (false),
    service_pid_list (false),
    service_id (0),
    prefix (""),
    title (""),
    suspect_min_error_count (1),
    suspect_max_consecutive (1)
{
    setHelp (help);

    option(u"ts-analysis");
    option(u"service-analysis");
    option(u"pid-analysis");
    option(u"table-analysis");
    option(u"error-analysis");
    option(u"normalized");
    option(u"service-list");
    option(u"pid-list");
    option(u"global-pid-list");
    option(u"unreferenced-pid-list");
    option(u"pes-pid-list");
    option(u"service-pid-list", 0, UINT16);
    option(u"prefix", 0, STRING);
    option(u"title", 0, STRING);
    option(u"suspect-min-error-count", 0, UNSIGNED);
    option(u"suspect-max-consecutive", 0, UNSIGNED);
}


//----------------------------------------------------------------------------
// Get option values (the public fields) after analysis of another
// ts::Args object defining the same options.
//----------------------------------------------------------------------------

void ts::TSAnalyzerOptions::getOptions (const Args& args)
{
    ts_analysis = args.present(u"ts-analysis");
    service_analysis = args.present(u"service-analysis");
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


//----------------------------------------------------------------------------
// Overriden analysis methods.
//----------------------------------------------------------------------------

bool ts::TSAnalyzerOptions::analyze(int argc, char* argv[])
{
    const bool ok = Args::analyze(argc, argv);
    if (ok) {
        getOptions(*this);
    }
    return ok;
}

bool ts::TSAnalyzerOptions::analyze(const UString& app_name, const UStringVector& arguments)
{
    const bool ok = Args::analyze(app_name, arguments);
    if (ok) {
        getOptions(*this);
    }
    return ok;
}
