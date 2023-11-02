//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsAlgorithm.h"
#include "tsCerrReport.h"
#include "tsNames.h"

TS_DEFINE_SINGLETON(ts::PSIRepository);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::PSIRepository::PSIRepository()
{
}

ts::PSIRepository::TableDescription::TableDescription()
{
    for (size_t i = 0; i < pids.size(); ++i) {
        pids[i] = PID_NULL;
    }
}

ts::PSIRepository::DescriptorDescription::DescriptorDescription(DescriptorFactory fact, DisplayDescriptorFunction disp) :
    factory(fact),
    display(disp)
{
}


//----------------------------------------------------------------------------
// Check if a PID is present in a table description.
//----------------------------------------------------------------------------

bool ts::PSIRepository::TableDescription::hasPID(PID pid) const
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


//----------------------------------------------------------------------------
// Add more PIDs in a table description.
//----------------------------------------------------------------------------

void ts::PSIRepository::TableDescription::addPIDs(std::initializer_list<PID> morePIDs)
{
    for (auto it : morePIDs) {
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
// Lookup a table function by table id, using standards and CAS id.
//----------------------------------------------------------------------------

template <typename FUNCTION, typename std::enable_if<std::is_pointer<FUNCTION>::value>::type*>
FUNCTION ts::PSIRepository::getTableFunction(TID tid, Standards standards, PID pid, uint16_t cas, FUNCTION TableDescription::* member) const
{
    // Try to find an exact match with standard and CAS id.
    // Otherwise, will use a fallback once for same tid.
    FUNCTION fallbackFunc = nullptr;
    size_t fallbackCount = 0;

    // Look for an exact match.
    for (auto it = _tables.lower_bound(tid); it != _tables.end() && it->first == tid; ++it) {
        // Ignore entries for which the searched function is not present.
        if (it->second.*member != nullptr) {

            // If the table in a standard PID, this is an exact match.
            if (it->second.hasPID(pid)) {
                return it->second.*member;
            }

            // CAS match: either a CAS is specified and is in range, or no CAS specified and CAS-agnostic table (all CASID_NULL).
            const bool casMatch = cas >= it->second.minCAS && cas <= it->second.maxCAS;

            // Standard match: at least one standard of the table is current, or standard-agnostic table (Standards::NONE).
            const bool stdMatch = bool(standards & it->second.standards) || it->second.standards == Standards::NONE;

            if (stdMatch && casMatch) {
                // Found an exact match, no need to search further.
                return it->second.*member;
            }
            else if (it->second.minCAS == CASID_NULL) {
                // Not the right standard but a CAS-agnostic table, use as potential fallback.
                fallbackFunc = it->second.*member;
                fallbackCount++;
            }
        }
    }

    // If no exact match was found, use a fallback if there is only one (no ambiguity).
    return fallbackCount == 1 ? fallbackFunc : nullptr;
}


//----------------------------------------------------------------------------
// Lookup a descriptor function by extended descriptor id.
//----------------------------------------------------------------------------

template <typename FUNCTION, typename std::enable_if<std::is_pointer<FUNCTION>::value>::type*>
FUNCTION ts::PSIRepository::getDescriptorFunction(const EDID& edid, TID tid, FUNCTION DescriptorDescription::* member) const
{
    auto it(_descriptors.end());

    if (edid.isStandard() && tid != TID_NULL) {
        // For standard descriptors, first search a table-specific descriptor.
        it = _descriptors.find(EDID::TableSpecific(edid.did(), tid));
        // If not found and there is a table-specific name for the descriptor,
        // do not fallback to non-table-specific function for this descriptor.
        if (it == _descriptors.end() && (edid.isTableSpecific() || names::HasTableSpecificName(edid.did(), tid))) {
            return nullptr;
        }
    }
    if (it == _descriptors.end()) {
        // If non-standard or no table-specific descriptor found, use direct lookup.
        it = _descriptors.find(edid);
    }
    return it != _descriptors.end() ? it->second.*member : nullptr;
}


//----------------------------------------------------------------------------
// Constructors to register extension files.
//----------------------------------------------------------------------------

ts::PSIRepository::RegisterXML::RegisterXML(const UString& filename)
{
    CERR.debug(u"registering XML file %s", {filename});
    PSIRepository::Instance()._xmlModelFiles.push_back(filename);
}


//----------------------------------------------------------------------------
// Constructors to register a fully or partially implemented table.
//----------------------------------------------------------------------------

ts::PSIRepository::RegisterTable::RegisterTable(TableFactory factory,
                                                const std::vector<TID>& tids,
                                                Standards standards,
                                                const UString& xmlName,
                                                DisplaySectionFunction displayFunction,
                                                LogSectionFunction logFunction,
                                                std::initializer_list<PID> pids,
                                                uint16_t minCAS,
                                                uint16_t maxCAS)
{
    CERR.log(2, u"registering table <%s>", {xmlName});
    PSIRepository& repo(PSIRepository::Instance());

    // XML names are recorded independently.
    if (!xmlName.empty()) {
        repo._tableNames.insert(std::make_pair(xmlName, factory));
    }

    // Build a table description for this table.
    TableDescription desc;
    desc.standards = standards;
    desc.minCAS = minCAS;
    desc.maxCAS = maxCAS;
    desc.factory = factory;
    desc.display = displayFunction;
    desc.log = logFunction;
    desc.addPIDs(pids);

    // Store a copy of the table description for each table id.
    // This is a multimap, distinct definitions for the same table id accumulate.
    for (auto it : tids) {
        repo._tables.insert(std::make_pair(it, desc));
    }
}

ts::PSIRepository::RegisterTable::RegisterTable(const std::vector<TID>& tids,
                                                Standards standards,
                                                DisplaySectionFunction displayFunction,
                                                LogSectionFunction logFunction,
                                                std::initializer_list<PID> pids,
                                                uint16_t minCAS,
                                                uint16_t maxCAS)
{
    // Use the complete constructor for actual registration.
    RegisterTable reg(nullptr, tids, standards, UString(), displayFunction, logFunction, pids, minCAS, maxCAS);
}


//----------------------------------------------------------------------------
// Constructors to register a fully or partially implemented descriptor.
//----------------------------------------------------------------------------

ts::PSIRepository::RegisterDescriptor::RegisterDescriptor(DescriptorFactory factory,
                                                          const EDID& edid,
                                                          const UString& xmlName,
                                                          DisplayDescriptorFunction displayFunction,
                                                          const UString& xmlNameLegacy)
{
    registerXML(factory, edid, xmlName, xmlNameLegacy);
    PSIRepository::Instance()._descriptors.insert(std::make_pair(edid, DescriptorDescription(factory, displayFunction)));
}

void ts::PSIRepository::RegisterDescriptor::registerXML(DescriptorFactory factory, const EDID& edid, const UString& xmlName, const UString& xmlNameLegacy)
{
    PSIRepository& repo(PSIRepository::Instance());

    if (!xmlName.empty()) {
        repo._descriptorNames.insert(std::make_pair(xmlName, factory));
        if (edid.isTableSpecific()) {
            repo._descriptorTablesIds.insert(std::make_pair(xmlName, edid.tableId()));
        }
    }
    if (!xmlNameLegacy.empty()) {
        repo._descriptorNames.insert(std::make_pair(xmlNameLegacy, factory));
        if (edid.isTableSpecific()) {
            repo._descriptorTablesIds.insert(std::make_pair(xmlNameLegacy, edid.tableId()));
        }
    }
}

ts::PSIRepository::RegisterDescriptor::RegisterDescriptor(DisplayCADescriptorFunction displayFunction, uint16_t minCAS, uint16_t maxCAS)
{
    if (displayFunction != nullptr) {
        PSIRepository& repo(PSIRepository::Instance());
        do {
            repo._casIdDescriptorDisplays.insert(std::make_pair(minCAS, displayFunction));
        } while (minCAS++ < maxCAS);
    }
}


//----------------------------------------------------------------------------
// Get registered items.
//----------------------------------------------------------------------------

ts::PSIRepository::TableFactory ts::PSIRepository::getTableFactory(TID id, Standards standards, PID pid, uint16_t cas) const
{
    return getTableFunction(id, standards, pid, cas, &TableDescription::factory);
}

ts::DisplaySectionFunction ts::PSIRepository::getSectionDisplay(TID id, Standards standards, PID pid, uint16_t cas) const
{
    return getTableFunction(id, standards, pid, cas, &TableDescription::display);
}

ts::LogSectionFunction ts::PSIRepository::getSectionLog(TID id, Standards standards, PID pid, uint16_t cas) const
{
    return getTableFunction(id, standards, pid, cas, &TableDescription::log);
}

ts::PSIRepository::TableFactory ts::PSIRepository::getTableFactory(const UString& node_name) const
{
    const auto it = node_name.findSimilar(_tableNames);
    return it != _tableNames.end() ? it->second : nullptr;
}

ts::PSIRepository::DescriptorFactory ts::PSIRepository::getDescriptorFactory(const UString& node_name) const
{
    const auto it = node_name.findSimilar(_descriptorNames);
    return it != _descriptorNames.end() ? it->second : nullptr;
}

ts::PSIRepository::DescriptorFactory ts::PSIRepository::getDescriptorFactory(const EDID& edid, TID tid) const
{
    return getDescriptorFunction(edid, tid, &DescriptorDescription::factory);
}

ts::DisplayDescriptorFunction ts::PSIRepository::getDescriptorDisplay(const EDID& edid, TID tid) const
{
    return getDescriptorFunction(edid, tid, &DescriptorDescription::display);
}

ts::DisplayCADescriptorFunction ts::PSIRepository::getCADescriptorDisplay(uint16_t cas_id) const
{
    const auto it = _casIdDescriptorDisplays.find(cas_id);
    return it != _casIdDescriptorDisplays.end() ? it->second : nullptr;
}


//----------------------------------------------------------------------------
// Get the list of standards which are defined for a given table id.
//----------------------------------------------------------------------------

ts::Standards ts::PSIRepository::getTableStandards(TID tid, PID pid) const
{
    // Accumulate the common subset of all standards for this table id.
    Standards standards = Standards::NONE;
    for (auto it = _tables.lower_bound(tid); it != _tables.end() && it->first == tid; ++it) {
        if (it->second.hasPID(pid)) {
            // We are in a standard PID for this table id, return the corresponding standards only.
            return it->second.standards;
        }
        else if (standards == Standards::NONE) {
            // No standard found yet, use all standards from first definition.
            standards = it->second.standards;
        }
        else {
            // Some standards were already found, keep only the common subset.
            standards &= it->second.standards;
        }
    }
    return standards;
}


//----------------------------------------------------------------------------
// Check if a descriptor is allowed in a table.
//----------------------------------------------------------------------------

bool ts::PSIRepository::isDescriptorAllowed(const UString& desc_node_name, TID table_id) const
{
    auto it = desc_node_name.findSimilar(_descriptorTablesIds);
    if (it == _descriptorTablesIds.end()) {
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
        } while (++it != _descriptorTablesIds.end() && desc_node_name.similar(it->first));
        // The requested table if was not found.
        return false;
    }
}


//----------------------------------------------------------------------------
// Get the list of tables where a descriptor is allowed.
//----------------------------------------------------------------------------

ts::UString ts::PSIRepository::descriptorTables(const DuckContext& duck, const UString& desc_node_name) const
{
    auto it = desc_node_name.findSimilar(_descriptorTablesIds);
    UString result;

    while (it != _descriptorTablesIds.end() && desc_node_name.similar(it->first)) {
        if (!result.empty()) {
            result.append(u", ");
        }
        result.append(names::TID(duck, it->second, CASID_NULL, NamesFlags::NAME | NamesFlags::HEXA));
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
    for (const auto& it : _tables) {
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
    for (const auto& it : _descriptors) {
        ids.push_back(it.first);
    }
}

void ts::PSIRepository::getRegisteredTableNames(UStringList& names) const
{
    names = MapKeysList(_tableNames);
}

void ts::PSIRepository::getRegisteredDescriptorNames(UStringList& names) const
{
    names = MapKeysList(_descriptorNames);
}

void ts::PSIRepository::getRegisteredTablesModels(UStringList& names) const
{
    names = _xmlModelFiles;
}
