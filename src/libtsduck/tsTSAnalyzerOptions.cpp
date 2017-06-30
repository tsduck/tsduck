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
#include "tsStringUtils.h"
#include "tsException.h"
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Set help: application specific help + generic help
//----------------------------------------------------------------------------

void ts::TSAnalyzerOptions::setHelp (const std::string& help)
{
    Args::setHelp (help +
        "\n"
        "Controlling analysis:\n"
        "\n"
        "  --suspect-max-consecutive value\n"
        "      Specifies the maximum number of consecutive \"suspect\" packets.\n"
        "      The default value is 1. If set to zero, the suspect packet detection\n"
        "      is disabled.\n"
        "\n"
        "      Suspect packets are TS packets which are technically correct but which\n"
        "      may be suspected of being incorrect, resulting in analysis errors.\n"
        "      Typically, in the middle of a suite of packets with uncorrectable\n"
        "      binary errors, one packet may appear to have no such error while\n"
        "      it has some errors in fact. To avoid adding this type of packets in the\n"
        "      analysis, a packet is declared as \"suspect\" (and consequently ignored in\n"
        "      the analysis) when:\n"
        "      - its PID is unknown (no other packet was found in this PID)\n"
        "      - it immediately follows a certain amount of packet containing errors\n"
        "        (see option --suspect-min-error-count)\n"
        "      - it immediately follows no more than the specified number consecutive\n"
        "        suspect packets.\n"
        "\n"
        "  --suspect-min-error-count value\n"
        "      Specifies the minimum number of consecutive packets with errors before\n"
        "      starting \"suspect\" packet detection. See also option\n"
        "      --suspect-max-consecutive. The default value is 1. If set to zero,\n"
        "      the suspect packet detection is disabled.\n"
        "\n"
        "Controlling output:\n"
        "\n"
        "  The output can include full synthetic analysis (options *-analysis),\n"
        "  fully normalized output (option --normalized) or a simple list of\n"
        "  values on one line (options --*-list). The second and third type of\n"
        "  options are useful to write automated scripts.\n"
        "\n"
        "  If output-control options are specified, only the selected outputs\n"
        "  are produced. If no option is given, the default is:\n"
        "  --ts-analysis --service-analysis --pid-analysis --table-analysis\n"
        "\n"
        "  --ts-analysis\n"
        "      Report global transport stream analysis.\n"
        "\n"
        "  --service-analysis\n"
        "      Report analysis for each service.\n"
        "\n"
        "  --pid-analysis\n"
        "      Report analysis for each PID.\n"
        "\n"
        "  --table-analysis\n"
        "      Report analysis for each table.\n"
        "\n"
        "  --error-analysis\n"
        "      Report analysis about detected errors.\n"
        "\n"
        "  --normalized\n"
        "      Complete report about the transport stream, the services and\n"
        "      the PID's in a normalized output format (useful for automatic\n"
        "      analysis).\n"
        "\n"
        "  --service-list\n"
        "      Report the list of all service ids.\n"
        "\n"
        "  --pid-list\n"
        "      Report the list of all PID's.\n"
        "\n"
        "  --global-pid-list\n"
        "      Report the list of all global PID's, that is to say PID's\n"
        "      which are not referenced by a specific service but are or\n"
        "      are referenced by the standard DVB PSI/SI. This include, for\n"
        "      instance, PID's of the PAT, EMM's, EIT's, stuffing, etc.\n"
        "\n"
        "  --unreferenced-pid-list\n"
        "      Report the list of all unreferenced PID's, that is to say\n"
        "      PID's which are neither referenced by a service nor known\n"
        "      as or referenced by the standard DVB PSI/SI.\n"
        "\n"
        "  --service-pid-list value\n"
        "      Report the list of all PID's which are referenced by the\n"
        "      specified service id.\n"
        "\n"
        "  --pes-pid-list\n"
        "      Report the list of all PID's which are declared as carrying\n"
        "      PES packets (audio, video, subtitles, etc).\n"
        "\n"
        "  --title string\n"
        "      Display the specified string as title header.\n"
        "\n"
        "  --prefix string\n"
        "      For one-line displays (options --*-list), prepend the\n"
        "      specified string to all values. For instance, options\n"
        "      --global --prefix -p outputs something like '-p 0 -p 1 -p 16',\n"
        "      which is an acceptable option list for the tsp filter plugin.\n");
}


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSAnalyzerOptions::TSAnalyzerOptions (const std::string& description,
                                            const std::string& syntax,
                                            const std::string& help,
                                            int flags) :

    Args (description, syntax, "", flags),
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

    option ("ts-analysis");
    option ("service-analysis");
    option ("pid-analysis");
    option ("table-analysis");
    option ("error-analysis");
    option ("normalized");
    option ("service-list");
    option ("pid-list");
    option ("global-pid-list");
    option ("unreferenced-pid-list");
    option ("pes-pid-list");
    option ("service-pid-list", 0, UINT16);
    option ("prefix", 0, STRING);
    option ("title", 0, STRING);
    option ("suspect-min-error-count", 0, UNSIGNED);
    option ("suspect-max-consecutive", 0, UNSIGNED);
}


//----------------------------------------------------------------------------
// Get option values (the public fields) after analysis of another
// ts::Args object defining the same options.
//----------------------------------------------------------------------------

void ts::TSAnalyzerOptions::getOptions (const Args& args)
{
    ts_analysis = args.present ("ts-analysis");
    service_analysis = args.present ("service-analysis");
    pid_analysis = args.present ("pid-analysis");
    table_analysis = args.present ("table-analysis");
    error_analysis = args.present ("error-analysis");
    normalized = args.present ("normalized");
    service_list = args.present ("service-list");
    pid_list = args.present ("pid-list");
    global_pid_list = args.present ("global-pid-list");
    unreferenced_pid_list = args.present ("unreferenced-pid-list");
    pes_pid_list = args.present ("pes-pid-list");
    service_pid_list = args.present ("service-pid-list");
    service_id = args.intValue<uint16_t> ("service-pid-list");
    prefix = args.value ("prefix");
    title = args.value ("title");
    suspect_min_error_count = args.intValue<uint64_t> ("suspect-min-error-count", 1);
    suspect_max_consecutive = args.intValue<uint64_t> ("suspect-max-consecutive", 1);

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
        !service_pid_list) {

        ts_analysis = service_analysis = pid_analysis = table_analysis = true;
    }
}


//----------------------------------------------------------------------------
// Overriden analysis methods.
//----------------------------------------------------------------------------

bool ts::TSAnalyzerOptions::analyze (int argc, char* argv[])
{
    bool ok = Args::analyze (argc, argv);
    if (ok) {
        getOptions (*this);
    }
    return ok;
}

bool ts::TSAnalyzerOptions::analyze (const std::string& app_name, const StringVector& arguments)
{
    bool ok = Args::analyze (app_name, arguments);
    if (ok) {
        getOptions (*this);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Inaccessible operation. Throw exception when invoked through virtual table.
//----------------------------------------------------------------------------

bool ts::TSAnalyzerOptions::analyze (const char* app_name, const char* arg1, ...)
{
    throw UnimplementedMethod ("analyze with variable args not implemented for ts::TSAnalyzerOptions");
}
