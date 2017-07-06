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
//  Command line arguments to select CAS.
//
//----------------------------------------------------------------------------

#include "tsCASSelectionArgs.h"
#include "tsPIDOperator.h"
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::CASSelectionArgs::CASSelectionArgs() :
    pass_ecm(false),
    pass_emm(false),
    min_cas_id(0),
    max_cas_id(0),
    cas_family(CAS_OTHER),
    cas_oper(0)
{
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args
//----------------------------------------------------------------------------

void ts::CASSelectionArgs::addHelp(Args& args) const
{
    std::string help =
        "\n"
        "CAS selection options:\n"
        "\n"
        "  --cas value\n"
        "      With options --ecm or --emm, select only ECM or EMM for the specified\n"
        "      CA system id value. Equivalent to --min-cas value --max-cas value.\n"
        "\n"
        "  --ecm\n"
        "      Extract PID's containing ECM.\n"
        "\n"
        "  --emm\n"
        "      Extract PID's containing EMM.\n"
        "\n"
        "  --max-cas value\n"
        "      With options --ecm or --emm, select only ECM or EMM for the CA system id\n"
        "      values in the range --min-cas to --max-cas.\n"
        "\n"
        "  --mediaguard\n"
        "      Equivalent to " + Format("--min-cas 0x%04X --max-cas 0x%04X", int(CASID_MEDIAGUARD_MIN), int(CASID_MEDIAGUARD_MAX)) + ".\n"
        "\n"
        "  --min-cas value\n"
        "      With options --ecm or --emm, select only ECM or EMM for the CA system id\n"
        "      values in the range --min-cas to --max-cas.\n"
        "\n"
        "  --nagravision\n"
        "      Equivalent to " + Format("--min-cas 0x%04X --max-cas 0x%04X", int(CASID_NAGRA_MIN), int(CASID_NAGRA_MAX)) + ".\n"
        "\n"
        "  --operator value\n"
        "      Restrict to the specified CAS operator (depends on the CAS).\n"
        "\n"
        "  --safeaccess\n"
        "      Equivalent to " + Format("--cas 0x%04X", int(CASID_SAFEACCESS)) + ".\n"
        "\n"
        "  --viaccess\n"
        "      Equivalent to " + Format("--min-cas 0x%04X --max-cas 0x%04X", int(CASID_VIACCESS_MIN), int(CASID_VIACCESS_MAX)) + ".\n";

    args.setHelp(args.getHelp() + help);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::CASSelectionArgs::defineOptions(Args& args) const
{
    args.option("cas",         0, Args::UINT16);
    args.option("ecm",         0);
    args.option("emm",         0);
    args.option("max-cas",     0, Args::UINT16);
    args.option("mediaguard",  0);
    args.option("min-cas",     0, Args::UINT16);
    args.option("nagravision", 0);
    args.option("operator",    0, Args::UINT32);
    args.option("safeaccess",  0);
    args.option("viaccess",    0);
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

void ts::CASSelectionArgs::load(Args& args)
{
    pass_ecm = args.present("ecm");
    pass_emm = args.present("emm");
    if (args.present("safeaccess")) {
        min_cas_id = max_cas_id = CASID_SAFEACCESS;
    }
    else if (args.present("mediaguard")) {
        min_cas_id = CASID_MEDIAGUARD_MIN;
        max_cas_id = CASID_MEDIAGUARD_MAX;
    }
    else if (args.present("viaccess")) {
        min_cas_id = CASID_VIACCESS_MIN;
        max_cas_id = CASID_VIACCESS_MAX;
    }
    else if (args.present("nagravision")) {
        min_cas_id = CASID_NAGRA_MIN;
        max_cas_id = CASID_NAGRA_MAX;
    }
    else if (args.present("cas")) {
        min_cas_id = max_cas_id = args.intValue<uint16_t>("cas");
    }
    else {
        min_cas_id = args.intValue<uint16_t>("min-cas");
        max_cas_id = args.intValue<uint16_t>("max-cas");
    }
    cas_family = CASFamilyOf(min_cas_id);
    cas_oper = args.intValue<uint32_t>("operator");
}


//----------------------------------------------------------------------------
// Check if the specified CAS or operator id matches the selection criteria.
//----------------------------------------------------------------------------

bool ts::CASSelectionArgs::casMatch(uint16_t cas) const
{
    // If min and max CAS ids are zero, this means all CAS.
    return (min_cas_id == 0 && max_cas_id == 0) || (cas >= min_cas_id && cas <= max_cas_id);
}

bool ts::CASSelectionArgs::operatorMatch(uint32_t oper) const
{
    // If cas_oper is zero, this means all operators.
    return cas_oper == 0 || oper == cas_oper;
}


//----------------------------------------------------------------------------
// Analyze all CA_descriptors in a CAT or PMT and locate all matching PID's.
//----------------------------------------------------------------------------

size_t ts::CASSelectionArgs::addMatchingPIDs(PIDSet& pids, const CAT& cat, ReportInterface& report) const
{
    return addMatchingPIDs(pids, cat.descs, cat.tableId(), report);
}

size_t ts::CASSelectionArgs::addMatchingPIDs(PIDSet& pids, const PMT& pmt, ReportInterface& report) const
{
    size_t pid_count = addMatchingPIDs(pids, pmt.descs, pmt.tableId(), report);
    for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
        pid_count += addMatchingPIDs(pids, it->second.descs, pmt.tableId(), report);
    }
    return pid_count;
}


//----------------------------------------------------------------------------
// Analyze all CA_descriptors and locate all matching PID's.
//----------------------------------------------------------------------------

size_t ts::CASSelectionArgs::addMatchingPIDs(PIDSet& pids, const DescriptorList& dlist, TID tid, ReportInterface& report) const
{
    // Filter out useless cases.
    if ((tid == TID_CAT && !pass_emm) || (tid == TID_PMT && !pass_ecm) || (tid != TID_CAT && tid != TID_PMT)) {
        return 0;
    }

    size_t pid_count = 0;

    if (cas_oper != 0) {
        // We must filter by operator id.
        // Collect all known forms of operator ids.
        PIDOperatorSet pidop;
        pidop.addAllOperators(dlist, tid == TID_CAT);

        // Loop on all collected PID and filter by operator id.
        for (PIDOperatorSet::const_iterator it = pidop.begin(); it != pidop.end(); ++it) {
            if (operatorMatch(it->oper) && casMatch(it->cas_id)) {
                pids.set(it->pid);
                pid_count++;
                report.verbose("Filtering %s PID %d (0x%04X)", tid == TID_CAT ? "EMM" : "ECM", int(it->pid), int(it->pid));
            }
        }
    }
    else {
        // No filtering by operator, loop on all CA descriptors.
        for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {
            const uint8_t* desc = dlist[index]->payload();
            const size_t size = dlist[index]->payloadSize();
            if (size >= 4) {
                // Get CA_system_id and ECM/EMM PID
                const uint16_t sysid = GetUInt16(desc);
                const PID pid = GetUInt16(desc + 2) & 0x1FFF;
                // Add ECM/EMM PID if it matches the required CAS id
                if (casMatch(sysid)) {
                    pids.set(pid);
                    pid_count++;
                    report.verbose("Filtering %s PID %d (0x%04X)", tid == TID_CAT ? "EMM" : "ECM", int(pid), int(pid));
                }
            }
        }
    }

    return pid_count;
}
