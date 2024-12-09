//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsAlgorithm.h"
#include "tsCerrReport.h"
#include "tsNames.h"

TS_DEFINE_SINGLETON(ts::PSIRepository);

// Value of a "null" type index.
const std::type_index ts::PSIRepository::null_index = std::type_index(typeid(std::nullptr_t));

// Singleton empty constructor.
ts::PSIRepository::PSIRepository() {}


//----------------------------------------------------------------------------
// Table class description.
//----------------------------------------------------------------------------

// Constructor.
ts::PSIRepository::TableClass::TableClass()
{
    for (size_t i = 0; i < pids.size(); ++i) {
        pids[i] = PID_NULL;
    }
}

// Check if a PID is present in a table description.
bool ts::PSIRepository::TableClass::hasPID(PID pid) const
{
    if (pid != PID_NULL) {
        for (size_t i = 0; i < pids.size() && pids[i] != PID_NULL; ++i) {
            if (pid == pids[i]) {
                return true;
            }
        }
    }
    return false;
}

// Add more PIDs in a table description.
void ts::PSIRepository::TableClass::addPIDs(std::initializer_list<PID> more_pids)
{
    for (auto it : more_pids) {
        if (it != PID_NULL) {
            size_t i = 0;
            while (i < pids.size() && pids[i] != PID_NULL && pids[i] != it) {
                ++i;
            }
            if (i < pids.size()) {
                pids[i] = it;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Constructors to register extension files.
//----------------------------------------------------------------------------

ts::PSIRepository::RegisterXML::RegisterXML(const UString& filename)
{
    CERR.debug(u"registering XML file %s", filename);
    PSIRepository::Instance()._xml_extension_files.push_back(filename);
}


//----------------------------------------------------------------------------
// Constructors to register a fully or partially implemented table.
//----------------------------------------------------------------------------

ts::PSIRepository::RegisterTable::RegisterTable(TableFactory factory,
                                                std::type_index index,
                                                const std::vector<TID>& tids,
                                                Standards standards,
                                                const UString& xml_name,
                                                DisplaySectionFunction display,
                                                LogSectionFunction log,
                                                std::initializer_list<PID> pids,
                                                uint16_t min_cas,
                                                uint16_t max_cas)
{
    CERR.log(2, u"registering table <%s>", xml_name);
    PSIRepository& repo(PSIRepository::Instance());

    // Build a table description for this table.
    TableClassPtr tc = std::make_shared<TableClass>();
    tc->index = index;
    tc->standards = standards;
    tc->min_cas = min_cas;
    tc->max_cas = max_cas;
    tc->factory = factory;
    tc->display = display;
    tc->log = log;
    tc->xml_name = xml_name;
    tc->addPIDs(pids);

    // Store the table description for each table id and XML name.
    for (auto it : tids) {
        repo._tables_by_tid.insert(std::make_pair(it, tc));
    }
    if (!xml_name.empty()) {
        repo._tables_by_name.insert(std::make_pair(xml_name, tc));
    }
}

ts::PSIRepository::RegisterTable::RegisterTable(const std::vector<TID>& tids,
                                                Standards standards,
                                                DisplaySectionFunction display,
                                                LogSectionFunction log,
                                                std::initializer_list<PID> pids,
                                                uint16_t min_cas,
                                                uint16_t max_cas)
{
    // Use the complete constructor for actual registration.
    RegisterTable reg(nullptr, null_index, tids, standards, UString(), display, log, pids, min_cas, max_cas);
}


//----------------------------------------------------------------------------
// Constructors to register a fully or partially implemented descriptor.
//----------------------------------------------------------------------------

ts::PSIRepository::RegisterDescriptor::RegisterDescriptor(DescriptorFactory factory,
                                                          std::type_index index,
                                                          const EDID& edid,
                                                          const UString& xml_name,
                                                          DisplayDescriptorFunction display,
                                                          const UString& legacy_xml_name)
{
    CERR.log(2, u"registering descriptor <%s>", xml_name);
    PSIRepository& repo(PSIRepository::Instance());

    // Build a description for this descriptor.
    DescriptorClassPtr dc = std::make_shared<DescriptorClass>();
    dc->index = index;
    dc->edid = edid;
    dc->factory = factory;
    dc->display = display;
    dc->xml_name = xml_name;

    // Store the descriptor description.
    repo._descriptors_by_xdid.insert(std::make_pair(edid.xdid(), dc));
    repo._descriptors_by_index.insert(std::make_pair(index, dc));

    // Associate XML names with descriptor classes and allowed table ids.
    if (!xml_name.empty()) {
        repo._descriptors_by_name.insert(std::make_pair(xml_name, dc));
        if (edid.isTableSpecific()) {
            repo._descriptor_tids.insert(std::make_pair(xml_name, edid.tableId()));
        }
    }
    if (!legacy_xml_name.empty()) {
        repo._descriptors_by_name.insert(std::make_pair(legacy_xml_name, dc));
        if (edid.isTableSpecific()) {
            repo._descriptor_tids.insert(std::make_pair(legacy_xml_name, edid.tableId()));
        }
    }
}

ts::PSIRepository::RegisterDescriptor::RegisterDescriptor(DisplayCADescriptorFunction display, uint16_t min_cas, uint16_t max_cas)
{
    if (display != nullptr) {
        PSIRepository& repo(PSIRepository::Instance());
        do {
            repo._casid_descriptor_displays.insert(std::make_pair(min_cas, display));
        } while (min_cas++ < max_cas);
    }
}


//----------------------------------------------------------------------------
// Lookup a table function by table id, using standards and CAS id.
//----------------------------------------------------------------------------

template <typename FUNCTION, typename std::enable_if<std::is_pointer<FUNCTION>::value>::type*>
FUNCTION ts::PSIRepository::getTableFunction(TID tid, const SectionContext& context, FUNCTION TableClass::* member) const
{
    // Try to find an exact match with standard and CAS id.
    // Otherwise, will use a fallback once for same tid.
    FUNCTION fallback_func = nullptr;
    size_t fallback_count = 0;

    const PID pid = context.getPID();
    const CASID cas = context.getCAS();
    const Standards standards = context.getStandards();

    // Range of iterators for all table classes matching the table id.
    const auto bounds(_tables_by_tid.equal_range(tid));

    // Look for an exact match.
    for (auto it = bounds.first; it != bounds.second; ++it) {
        const auto& tc(*it->second);

        // Ignore entries for which the searched function is not present.
        if (tc.*member != nullptr) {

            // If the table is in a standard PID, this is an exact match.
            if (tc.hasPID(pid)) {
                return tc.*member;
            }

            // CAS match: either a CAS is specified and is in range, or no CAS specified and CAS-agnostic table (all CASID_NULL).
            const bool cas_match = cas >= tc.min_cas && cas <= tc.max_cas;

            // Standard match: at least one standard of the table is current, or standard-agnostic table (Standards::NONE).
            const bool std_match = bool(standards & tc.standards) || tc.standards == Standards::NONE;

            if (std_match && cas_match) {
                // Found an exact match, no need to search further.
                return tc.*member;
            }
            else if (tc.min_cas == CASID_NULL) {
                // Not the right standard but a CAS-agnostic table, use as potential fallback.
                fallback_func = tc.*member;
                fallback_count++;
            }
        }
    }

    // If no exact match was found, use a fallback if there is only one (no ambiguity).
    return fallback_count == 1 ? fallback_func : nullptr;
}


//----------------------------------------------------------------------------
// Search a descriptor class from its EDID.
//----------------------------------------------------------------------------

ts::PSIRepository::DescriptorClassPtr ts::PSIRepository::getDescriptor(EDID edid) const
{
    // Get the range of XDID entries for this family of descriptors.
    const auto bounds(_descriptors_by_xdid.equal_range(edid.xdid()));

    // If the bounds are equal, no element matches, unknown descritor.
    if (bounds.first == bounds.second) {
        return nullptr;
    }

    // If there is only one descriptor, use it without further analysis.
    auto next = bounds.first;
    if (++next == bounds.second) {
        return bounds.first->second;
    }

    // If there are several descriptor, search for an exact EDID match.
    for (next = bounds.first; next != bounds.second; ++next) {
        if (next->second->edid == edid) {
            return next->second;
        }
    }
    return nullptr; // ambiguous descriptor
}


//----------------------------------------------------------------------------
// Search a descriptor class from the context.
//----------------------------------------------------------------------------

ts::PSIRepository::DescriptorClassPtr ts::PSIRepository::getDescriptor(XDID xdid, const DescriptorContext& context) const
{
    // Get the range of XDID entries for this family of descriptors.
    const auto bounds(_descriptors_by_xdid.equal_range(xdid));

    // If the bounds are equal, no element matches, unknown descritor.
    if (bounds.first == bounds.second) {
        return nullptr;
    }

    // If there is only one descriptor, use it without further analysis.
    auto next = bounds.first;
    if (++next == bounds.second) {
        return bounds.first->second;
    }

    const TID tid = context.getTableId();
    const Standards standards = context.getStandards();
    REGID regid = REGID_NULL;
    PDS pds = PDS_NULL;
    REGID* pregid = nullptr;   // will be set to &regid if we must search a MPEG registration id.
    PDS* ppds = nullptr;       // will be set to &pds if we must search a DVB private data specifier.
    DescriptorClassPtr match;  // possible but not exclusive match.
    size_t match_count = 0;    // number of matches.

    // There are several entries, must be a private descriptor or table-specific descriptor.
    // First pass, look for a table match or which type of private identifier we need.
    for (next = bounds.first; next != bounds.second; ++next) {
        const auto& dc(next->second);
        // If this is a table-specific descriptor for the table we use, we have a match.
        if (dc->edid.matchTableSpecific(tid, standards)) {
            return dc;
        }
        // If we match the standards for a regular descriptor, this is a possible match.
        if (dc->edid.matchRegularStandards(standards)) {
            match = dc;
            match_count++;
        }
        // Check if we need to search for private identifiers.
        // Note that a descriptor can be both MPEG private and DVB private.
        if (dc->edid.isPrivateMPEG()) {
            pregid = &regid;
        }
        if (dc->edid.isPrivateDVB()) {
            ppds = &pds;
        }
    }

    // Search private descriptor ids. If we also match regular descriptors but the
    // corresponding private ids are found, the private descriptor takes precedence.
    if (pregid != nullptr || ppds != nullptr) {

        // Need to search the private identifiers.
        const bool regid_first = context.getPrivateIds(pregid, ppds);

        if (regid != REGID_NULL && (regid_first || pds == PDS_NULL)) {
            // There is REGID and it is closer than the DVB PDS or there is no PDS.
            for (next = bounds.first; next != bounds.second; ++next) {
                if (next->second->edid.regid() == regid) {
                    return next->second;
                }
            }
        }
        if (pds != PDS_NULL) {
            for (next = bounds.first; next != bounds.second; ++next) {
                if (next->second->edid.pds() == pds) {
                    return next->second;
                }
            }
            if (regid != REGID_NULL) {
                // There is a PDS and a REGID but the PDS was first.
                for (next = bounds.first; next != bounds.second; ++next) {
                    if (next->second->edid.regid() == regid) {
                        return next->second;
                    }
                }
            }
        }
    }

    // No private descriptor found. If there is exactly one regular match, we keep it.
    // Otherwise, there is either nothing found or some ambiguity.
    return match_count == 1 ? match : nullptr;
}


//----------------------------------------------------------------------------
// Get simple registered items.
//----------------------------------------------------------------------------

template <typename FUNCTION, typename std::enable_if<std::is_pointer<FUNCTION>::value>::type*>
FUNCTION ts::PSIRepository::getDescriptorFunction(XDID xdid, const DescriptorContext& context, FUNCTION DescriptorClass::* member) const
{
    const auto dc(getDescriptor(xdid, context));
    return dc == nullptr ? nullptr : (*dc).*member;
}

ts::PSIRepository::TableFactory ts::PSIRepository::getTableFactory(TID id, const SectionContext& context) const
{
    return getTableFunction(id, context, &TableClass::factory);
}

ts::DisplaySectionFunction ts::PSIRepository::getSectionDisplay(TID id, const SectionContext& context) const
{
    return getTableFunction(id, context, &TableClass::display);
}

ts::LogSectionFunction ts::PSIRepository::getSectionLog(TID id, const SectionContext& context) const
{
    return getTableFunction(id, context, &TableClass::log);
}

ts::PSIRepository::TableFactory ts::PSIRepository::getTableFactory(const UString& node_name) const
{
    const auto it = node_name.findSimilar(_tables_by_name);
    return it != _tables_by_name.end() ? it->second->factory : nullptr;
}

ts::PSIRepository::DescriptorFactory ts::PSIRepository::getDescriptorFactory(const UString& node_name) const
{
    const auto it = node_name.findSimilar(_descriptors_by_name);
    return it != _descriptors_by_name.end() ? it->second->factory : nullptr;
}

ts::EDID ts::PSIRepository::getDescriptorEDID(std::type_index index, TID tid, Standards standards) const
{
    const auto bounds(_descriptors_by_index.equal_range(index));
    if (bounds.first == bounds.second) {
        return EDID(); // not found
    }
    if (tid != TID_NULL) {
        for (auto next = bounds.first; next != bounds.second; ++next) {
            if (next->second->edid.matchTableSpecific(tid, standards)) {
                return next->second->edid; // exact match for a table-specific descriptor
            }
        }
    }
    return bounds.first->second->edid;
}

ts::EDID ts::PSIRepository::getDescriptorEDID(XDID xdid, const DescriptorContext& context) const
{
    const auto dc(getDescriptor(xdid, context));
    return dc != nullptr ? dc->edid : EDID();
}

ts::PSIRepository::DescriptorFactory ts::PSIRepository::getDescriptorFactory(EDID edid) const
{
    const auto dc(getDescriptor(edid));
    return dc != nullptr ? dc->factory : nullptr;
}

ts::PSIRepository::DescriptorFactory ts::PSIRepository::getDescriptorFactory(XDID xdid, const DescriptorContext& context) const
{
    return getDescriptorFunction(xdid, context, &DescriptorClass::factory);
}

ts::DisplayDescriptorFunction ts::PSIRepository::getDescriptorDisplay(XDID xdid, const DescriptorContext& context) const
{
    return getDescriptorFunction(xdid, context, &DescriptorClass::display);
}

ts::DisplayCADescriptorFunction ts::PSIRepository::getCADescriptorDisplay(uint16_t cas_id) const
{
    const auto it = _casid_descriptor_displays.find(cas_id);
    return it != _casid_descriptor_displays.end() ? it->second : nullptr;
}


//----------------------------------------------------------------------------
// Get the list of standards which are defined for a given table id.
//----------------------------------------------------------------------------

ts::Standards ts::PSIRepository::getTableStandards(TID tid, PID pid) const
{
    // Accumulate the common subset of all standards for this table id.
    Standards standards = Standards::NONE;
    const auto bounds(_tables_by_tid.equal_range(tid));
    for (auto it = bounds.first; it != bounds.second; ++it) {
        const auto& tc(*it->second);

        if (tc.hasPID(pid)) {
            // We are in a standard PID for this table id, return the corresponding standards only.
            return tc.standards;
        }
        else if (tc.hasPID() && pid != PID_NULL) {
            // This is a table with dedicated PID's but we are not in one of them => ignore.
        }
        else if (standards == Standards::NONE) {
            // No standard found yet, use all standards from first definition.
            standards = tc.standards;
        }
        else {
            // Some standards were already found, keep only the common subset.
            standards &= tc.standards;
        }
    }
    return standards;
}


//----------------------------------------------------------------------------
// Check if a descriptor is allowed in a table.
//----------------------------------------------------------------------------

bool ts::PSIRepository::isDescriptorAllowed(const UString& desc_node_name, TID table_id) const
{
    auto it = desc_node_name.findSimilar(_descriptor_tids);
    if (it == _descriptor_tids.end()) {
        // Not a table-specific descriptor, allowed anywhere
        return true;
    }
    else {
        // Table specific descriptor, the table needs to be listed.
        do {
            if (table_id == it->second) {
                // The table is explicitly allowed.
                return true;
            }
        } while (++it != _descriptor_tids.end() && desc_node_name.similar(it->first));
        // The requested table if was not found.
        return false;
    }
}


//----------------------------------------------------------------------------
// Get the list of tables where a descriptor is allowed.
//----------------------------------------------------------------------------

ts::UString ts::PSIRepository::descriptorTables(const DuckContext& duck, const UString& desc_node_name) const
{
    auto it = desc_node_name.findSimilar(_descriptor_tids);
    UString result;

    while (it != _descriptor_tids.end() && desc_node_name.similar(it->first)) {
        if (!result.empty()) {
            result.append(u", ");
        }
        result.append(TIDName(duck, it->second, CASID_NULL, NamesFlags::NAME | NamesFlags::HEXA));
        ++it;
    }

    return result;
}


//----------------------------------------------------------------------------
// Get all registered items of a given type.
//----------------------------------------------------------------------------

void ts::PSIRepository::getRegisteredTableIds(std::vector<TID>& ids) const
{
    ids.clear();
    TID previous = TID_NULL;
    for (const auto& it : _tables_by_tid) {
        // This is a multimap, the same key can be reused, use it once only.
        if (it.first != previous) {
            ids.push_back(it.first);
            previous = it.first;
        }
    }
}

void ts::PSIRepository::getRegisteredDescriptorIds(std::vector<EDID>& ids) const
{
    ids.clear();
    for (const auto& it : _descriptors_by_xdid) {
        ids.push_back(it.second->edid);
    }
}

void ts::PSIRepository::getRegisteredTableNames(UStringList& names) const
{
    names = MapKeysList(_tables_by_name);
}

void ts::PSIRepository::getRegisteredDescriptorNames(UStringList& names) const
{
    names = MapKeysList(_descriptors_by_name);
}

void ts::PSIRepository::getRegisteredTablesModels(UStringList& names) const
{
    names = _xml_extension_files;
}


//----------------------------------------------------------------------------
// Dump the internal state of the PSI repository (for debug only).
//----------------------------------------------------------------------------

void ts::PSIRepository::dumpInternalState(std::ostream& out) const
{
    out << "TSDuck PSI Repository" << std::endl
        << "=====================" << std::endl
        << std::endl
        << "TID to table class: " << _tables_by_tid.size() << std::endl;
    for (const auto& it : _tables_by_tid) {
        const auto& tc(*it.second);
        out << UString::Format(u"  %X: %s, std: %s, index: %X", it.first, tc.xml_name, StandardsNames(tc.standards), tc.index.hash_code());
        const char* sep = ", PIDS: ";
        for (auto p : tc.pids) {
            if (p == PID_NULL) {
                break;
            }
            out << sep << UString::Format(u"%X", p);
            sep = ", ";
        }
        if (tc.min_cas != CASID_NULL) {
            out << UString::Format(u", CAS: %X-%X", tc.min_cas, tc.max_cas);
        }
        out << std::endl;
    }
    out << std::endl << "Table name to table class: " << _tables_by_name.size() << std::endl;
    for (const auto& it : _tables_by_name) {
        out << UString::Format(u"  %s index: %X", it.first, it.second->index.hash_code()) << std::endl;
    }
    out << std::endl << "XDID to descriptor class: " << _descriptors_by_xdid.size() << std::endl;
    for (const auto& it : _descriptors_by_xdid) {
        const auto& dc(*it.second);
        out << UString::Format(u"  %s: %s, %s, index: %X", it.first.toString(), dc.xml_name, dc.edid.toString(), dc.index.hash_code());
        if (!dc.legacy_xml_name.empty()) {
            out << UString::Format(u", legacy: %s", dc.legacy_xml_name);
        }
        out << std::endl;
    }
    out << std::endl << "Descriptor name to descriptor class: " << _descriptors_by_name.size() << std::endl;
    for (const auto& it : _descriptors_by_name) {
        out << UString::Format(u"  %s index: %X", it.first, it.second->index.hash_code()) << std::endl;
    }
    out << std::endl << "Descriptor RTTI index to descriptor class: " << _descriptors_by_index.size() << std::endl;
    for (const auto& it : _descriptors_by_index) {
        out << UString::Format(u"  %X: %s", it.first.hash_code(), it.second->xml_name) << std::endl;
    }
    out << std::endl << "XML descriptor name to table id for table-specific descriptors: " << _descriptor_tids.size() << std::endl;
    for (const auto& it : _descriptor_tids) {
        out << UString::Format(u"  %s: %X", it.first, it.second) << std::endl;
    }
    out << std::endl << "Display CA Descriptor functions: " << _casid_descriptor_displays.size() << std::endl;
    for (const auto& it : _casid_descriptor_displays) {
        out << UString::Format(u"  CASID: %X", it.first) << std::endl;
    }
    out << std::endl << "XML extension files: " << _xml_extension_files.size() << std::endl;
    for (const auto& it : _xml_extension_files) {
        out << " - \"" << it << "\"" << std::endl;
    }
    out << std::endl;
}
