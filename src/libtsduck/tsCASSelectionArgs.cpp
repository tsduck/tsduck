//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsCASSelectionArgs.h"
#include "tsPIDOperator.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
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

ts::CASSelectionArgs::~CASSelectionArgs()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::CASSelectionArgs::defineOptions(Args& args) const
{
    args.option(u"cas", 0, Args::UINT16);
    args.help(u"cas",
              u"With options --ecm or --emm, select only ECM or EMM for the specified "
              u"CA system id value. Equivalent to --min-cas value --max-cas value.");

    args.option(u"ecm");
    args.help(u"ecm", u"Extract PID's containing ECM.");

    args.option(u"emm");
    args.help(u"emm", u"Extract PID's containing EMM.");

    args.option(u"max-cas", 0, Args::UINT16);
    args.help(u"max-cas",
              u"With options --ecm or --emm, select only ECM or EMM for the CA system id "
              u"values in the range --min-cas to --max-cas.");

    args.option(u"mediaguard");
    args.help(u"mediaguard",
              u"Equivalent to " +
              UString::Format(u"--min-cas 0x%04X --max-cas 0x%04X", {CASID_MEDIAGUARD_MIN, CASID_MEDIAGUARD_MAX}) +
              u".");

    args.option(u"min-cas", 0, Args::UINT16);
    args.help(u"min-cas",
              u"With options --ecm or --emm, select only ECM or EMM for the CA system id "
              u"values in the range --min-cas to --max-cas.");

    args.option(u"nagravision");
    args.help(u"nagravision",
              u"Equivalent to " +
              UString::Format(u"--min-cas 0x%04X --max-cas 0x%04X", {CASID_NAGRA_MIN, CASID_NAGRA_MAX}) +
              u".");

    args.option(u"operator", 0, Args::UINT32);
    args.help(u"operator", u"Restrict to the specified CAS operator (depends on the CAS).");

    args.option(u"safeaccess");
    args.help(u"safeaccess",
              u"Equivalent to " + UString::Format(u"--cas 0x%04X", {CASID_SAFEACCESS}) + u".");

    args.option(u"viaccess");
    args.help(u"viaccess",
              u"Equivalent to " +
              UString::Format(u"--min-cas 0x%04X --max-cas 0x%04X", {CASID_VIACCESS_MIN, CASID_VIACCESS_MAX}) +
              u".");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

void ts::CASSelectionArgs::load(Args& args)
{
    pass_ecm = args.present(u"ecm");
    pass_emm = args.present(u"emm");
    if (args.present(u"safeaccess")) {
        min_cas_id = max_cas_id = CASID_SAFEACCESS;
    }
    else if (args.present(u"mediaguard")) {
        min_cas_id = CASID_MEDIAGUARD_MIN;
        max_cas_id = CASID_MEDIAGUARD_MAX;
    }
    else if (args.present(u"viaccess")) {
        min_cas_id = CASID_VIACCESS_MIN;
        max_cas_id = CASID_VIACCESS_MAX;
    }
    else if (args.present(u"nagravision")) {
        min_cas_id = CASID_NAGRA_MIN;
        max_cas_id = CASID_NAGRA_MAX;
    }
    else if (args.present(u"cas")) {
        min_cas_id = max_cas_id = args.intValue<uint16_t>(u"cas");
    }
    else {
        min_cas_id = args.intValue<uint16_t>(u"min-cas");
        max_cas_id = args.intValue<uint16_t>(u"max-cas");
    }
    cas_family = CASFamilyOf(min_cas_id);
    cas_oper = args.intValue<uint32_t>(u"operator");
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

size_t ts::CASSelectionArgs::addMatchingPIDs(PIDSet& pids, const CAT& cat, Report& report) const
{
    return addMatchingPIDs(pids, cat.descs, cat.tableId(), report);
}

size_t ts::CASSelectionArgs::addMatchingPIDs(PIDSet& pids, const PMT& pmt, Report& report) const
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

size_t ts::CASSelectionArgs::addMatchingPIDs(PIDSet& pids, const DescriptorList& dlist, TID tid, Report& report) const
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
                report.verbose(u"Filtering %s PID %d (0x%X)", {tid == TID_CAT ? u"EMM" : u"ECM", it->pid, it->pid});
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
                    report.verbose(u"Filtering %s PID %d (0x%X)", {tid == TID_CAT ? u"EMM" : u"ECM", pid, pid});
                }
            }
        }
    }

    return pid_count;
}
