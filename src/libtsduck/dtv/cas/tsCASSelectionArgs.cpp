//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCASSelectionArgs.h"
#include "tsPIDOperator.h"
#include "tsArgs.h"

const std::vector<ts::CASSelectionArgs::PredefinedCAS> ts::CASSelectionArgs::_predefined_cas{
    {u"conax",       CASID_CONAX_MIN,      CASID_CONAX_MAX},
    {u"irdeto",      CASID_IRDETO_MIN,     CASID_IRDETO_MAX},
    {u"mediaguard",  CASID_MEDIAGUARD_MIN, CASID_MEDIAGUARD_MAX},
    {u"nagravision", CASID_NAGRA_MIN,      CASID_NAGRA_MAX},
    {u"nds",         CASID_NDS_MIN,        CASID_NDS_MAX},
    {u"safeaccess",  CASID_SAFEACCESS,     CASID_SAFEACCESS},
    {u"viaccess",    CASID_VIACCESS_MIN,   CASID_VIACCESS_MAX},
    {u"widevine",    CASID_WIDEVINE_MIN,   CASID_WIDEVINE_MAX}
};


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::CASSelectionArgs::defineArgs(Args& args)
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

    args.option(u"min-cas", 0, Args::UINT16);
    args.help(u"min-cas",
              u"With options --ecm or --emm, select only ECM or EMM for the CA system id "
              u"values in the range --min-cas to --max-cas.");

    args.option(u"operator", 0, Args::UINT32);
    args.help(u"operator", u"Restrict to the specified CAS operator (depends on the CAS).");

    // Predefined CAS options:
    for (const auto& cas : _predefined_cas) {
        args.option(cas.name);
        args.help(cas.name, cas.min == cas.max ?
            UString::Format(u"Equivalent to --cas 0x%04X.", {cas.min}) :
            UString::Format(u"Equivalent to --min-cas 0x%04X --max-cas 0x%04X.", {cas.min, cas.max}));
    }
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::CASSelectionArgs::loadArgs(DuckContext& duck, Args& args)
{
    bool success = true;

    // CAS selection:
    int cas_count = (args.present(u"min-cas") || args.present(u"max-cas"));
    if (args.present(u"cas")) {
        min_cas_id = max_cas_id = args.intValue<uint16_t>(u"cas");
        cas_count++;
    }
    else {
        min_cas_id = args.intValue<uint16_t>(u"min-cas");
        max_cas_id = args.intValue<uint16_t>(u"max-cas");
    }

    // Overridden by predefined CAS options:
    for (const auto& cas : _predefined_cas) {
        if (args.present(cas.name)) {
            min_cas_id = cas.min;
            max_cas_id = cas.max;
            cas_count++;
        }
    }

    // Check that there is only one way to specify the CAS.
    if (cas_count > 1) {
        args.error(u"conflicting CAS selection options");
        success = false;
    }

    // Other options:
    cas_oper = args.intValue<uint32_t>(u"operator");
    pass_ecm = args.present(u"ecm");
    pass_emm = args.present(u"emm");
    return success;
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
    for (const auto& it : pmt.streams) {
        pid_count += addMatchingPIDs(pids, it.second.descs, pmt.tableId(), report);
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
        for (const auto& it : pidop) {
            if (operatorMatch(it.oper) && casMatch(it.cas_id)) {
                pids.set(it.pid);
                pid_count++;
                report.verbose(u"Filtering %s PID %d (0x%X)", {tid == TID_CAT ? u"EMM" : u"ECM", it.pid, it.pid});
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
