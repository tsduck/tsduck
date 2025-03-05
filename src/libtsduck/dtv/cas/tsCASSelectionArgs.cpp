//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCASSelectionArgs.h"
#include "tsPIDOperator.h"
#include "tsArgs.h"
#include "tsCAS.h"


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

    // The first time, get the list of predefined CAS options.
    if (_cas_options.empty()) {
        // Get all CAS families, get their name and CAS ids.
        std::set<CASFamily> families;
        GetAllCASFamilies(families);
        for (CASFamily f : families) {
            const CASID min = FirstCASId(f);
            const CASID max = LastCASId(f);
            if (min != CASID_NULL && max != CASID_NULL) {
                // Get the CAS name and transform it into an acceptable option name.
                UString name(CASFamilyName(f));
                size_t out = 0;
                for (size_t in = 0; in < name.size(); ++in) {
                    if (IsAlphaNum(name[in])) {
                        name[out++] = ToLower(name[in]);
                    }
                    else if (out > 0 && name[out-1] != u'-') {
                        name[out++] = u'-';
                    }
                }
                name.resize(out);
                if (!name.empty()) {
                    _cas_options.insert(std::make_pair(name, std::make_pair(min, max)));
                }
            }
        }
    }

    // Declare the predefined CAS options.
    for (const auto& cas : _cas_options) {
        args.option(cas.first.c_str());
        args.help(cas.first.c_str(), cas.second.first == cas.second.second ?
            UString::Format(u"Equivalent to --cas 0x%04X.", cas.second.first) :
            UString::Format(u"Equivalent to --min-cas 0x%04X --max-cas 0x%04X.", cas.second.first, cas.second.second));
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
        args.getIntValue(min_cas_id, u"cas");
        max_cas_id = min_cas_id;
        cas_count++;
    }
    else {
        args.getIntValue(min_cas_id, u"min-cas");
        args.getIntValue(max_cas_id, u"max-cas");
    }

    // Overridden by predefined CAS options:
    for (const auto& cas : _cas_options) {
        if (args.present(cas.first.c_str())) {
            min_cas_id = cas.second.first;
            max_cas_id = cas.second.second;
            cas_count++;
        }
    }

    // Check that there is only one way to specify the CAS.
    if (cas_count > 1) {
        args.error(u"conflicting CAS selection options");
        success = false;
    }

    // Other options:
    args.getIntValue(cas_oper, u"operator");
    pass_ecm = args.present(u"ecm");
    pass_emm = args.present(u"emm");
    return success;
}


//----------------------------------------------------------------------------
// Check if the specified CAS or operator id matches the selection criteria.
//----------------------------------------------------------------------------

bool ts::CASSelectionArgs::casMatch(CASID cas) const
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
                report.verbose(u"Filtering %s PID %n", tid == TID_CAT ? u"EMM" : u"ECM", it.pid);
            }
        }
    }
    else {
        // No filtering by operator, loop on all CA descriptors.
        for (size_t index = dlist.search(DID_MPEG_CA); index < dlist.count(); index = dlist.search(DID_MPEG_CA, index + 1)) {
            const uint8_t* desc = dlist[index].payload();
            const size_t size = dlist[index].payloadSize();
            if (size >= 4) {
                // Get CA_system_id and ECM/EMM PID
                const uint16_t sysid = GetUInt16(desc);
                const PID pid = GetUInt16(desc + 2) & 0x1FFF;
                // Add ECM/EMM PID if it matches the required CAS id
                if (casMatch(sysid)) {
                    pids.set(pid);
                    pid_count++;
                    report.verbose(u"Filtering %s PID %n", tid == TID_CAT ? u"EMM" : u"ECM", pid);
                }
            }
        }
    }

    return pid_count;
}
