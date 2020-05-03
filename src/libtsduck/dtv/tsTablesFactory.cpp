//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//  Tables and descriptor factory.
//
//----------------------------------------------------------------------------

#include "tsTablesFactory.h"
#include "tsDuckContext.h"
#include "tsNames.h"
TSDUCK_SOURCE;

TS_DEFINE_SINGLETON(ts::TablesFactory);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::TablesFactory::TablesFactory() :
    _tables(),
    _descriptorIds(),
    _tableNames(),
    _descriptorNames(),
    _descriptorTablesIds(),
    _descriptorDisplays(),
    _casIdDescriptorDisplays(),
    _xmlModelFiles(),
    _namesFiles()
{
}

ts::TablesFactory::TableDescription::TableDescription() :
    standards(STD_NONE),
    minCAS(CASID_NULL),
    maxCAS(CASID_NULL),
    factory(nullptr),
    display(nullptr),
    log(nullptr),
    pids()
{
    for (size_t i = 0; i < pids.size(); ++i) {
        pids[i] = PID_NULL;
    }
}


//----------------------------------------------------------------------------
// Check if a PID is present in a table description.
//----------------------------------------------------------------------------

bool ts::TablesFactory::TableDescription::hasPID(PID pid) const
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

void ts::TablesFactory::TableDescription::addPIDs(std::initializer_list<PID> morePIDs)
{
    for (auto it = morePIDs.begin(); it != morePIDs.end(); ++it) {
        if (*it != PID_NULL) {
            size_t i = 0;
            while (i < pids.size() && pids[i] != PID_NULL && pids[i] != *it) {
                ++i;
            }
            if (i < pids.size()) {
                pids[i] = *it;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Register a new table id which matches standards and CAS ids.
//----------------------------------------------------------------------------

ts::TablesFactory::TableDescription* ts::TablesFactory::registerTable(TID tid, Standards standards, uint16_t minCAS, uint16_t maxCAS, std::initializer_list<PID> pids)
{
    // Try to find a matching existing table.
    for (auto it = _tables.lower_bound(tid); it != _tables.end() && it->first == tid; ++it) {
        // Found an entry with same table id.
        if ((standards & it->second.standards) == standards && minCAS >= it->second.minCAS && maxCAS <= it->second.maxCAS) {
            // Found an entry which includes all required standards and CAS id.
            it->second.addPIDs(pids);
            return &it->second;
        }
    }

    // No existing entry found, create a new one.
    TableDescription td;
    td.standards = standards;
    td.minCAS = minCAS;
    td.maxCAS = maxCAS;
    td.addPIDs(pids);
    auto it = _tables.insert(std::make_pair(tid, td));
    return &it->second;
}


//----------------------------------------------------------------------------
// Lookup a table function by table id, using standards and CAS id.
//----------------------------------------------------------------------------

template <typename FUNCTION, typename std::enable_if<std::is_pointer<FUNCTION>::value>::type*>
FUNCTION ts::TablesFactory::getTableFunction(TID tid, Standards standards, PID pid, uint16_t cas, FUNCTION TableDescription::* member) const
{
    // Try to find an exact match with standard and CAS id.
    // Otherwise, will use a fallback once for same tid.
    FUNCTION fallbackFunc = nullptr;
    size_t fallbackCount = 0;

    // Look for an exact match.
    for (auto it = _tables.lower_bound(tid); it != _tables.end() && it->first == tid; ++it) {
        // Ignore entris for which the searched function is not present.
        if (it->second.*member != nullptr) {

            // If the table in a standard PID, this is an exact match.
            if (it->second.hasPID(pid)) {
                return it->second.*member;
            }

            // CAS match: either a CAS is specified and is in range, or no CAS specified and CAS-agnostic table (all CASID_NULL).
            const bool casMatch = cas >= it->second.minCAS && cas <= it->second.maxCAS;

            // Standard match: at least one standard of the table is current, or standard-agnostic table (STD_NONE).
            const bool stdMatch = (standards & it->second.standards) != 0 || it->second.standards == STD_NONE;

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
// Registrations using constructors of Register objects.
//----------------------------------------------------------------------------

ts::TablesFactory::Register::Register(TID id, TableFactory factory, Standards standards, std::initializer_list<PID> pids)
{
    TablesFactory::Instance()->registerTable(id, standards, CASID_NULL, CASID_NULL, pids)->factory = factory;
}

ts::TablesFactory::Register::Register(TID minId, TID maxId, TableFactory factory, Standards standards, std::initializer_list<PID> pids)
{
    for (TID id = minId; id <= maxId; ++id) {
        TablesFactory::Instance()->registerTable(id, standards, CASID_NULL, CASID_NULL, pids)->factory = factory;
    }
}

ts::TablesFactory::Register::Register(const EDID& id, DescriptorFactory factory)
{
    TablesFactory::Instance()->_descriptorIds.insert(std::make_pair(id, factory));
}

ts::TablesFactory::Register::Register(const UString& node_name, TableFactory factory)
{
    TablesFactory::Instance()->_tableNames.insert(std::make_pair(node_name, factory));
}

ts::TablesFactory::Register::Register(const UString& node_name, DescriptorFactory factory, std::initializer_list<TID> tids)
{
    TablesFactory::Instance()->_descriptorNames.insert(std::make_pair(node_name, factory));
    for (auto it = tids.begin(); it != tids.end(); ++it) {
        TablesFactory::Instance()->_descriptorTablesIds.insert(std::make_pair(node_name, *it));
    }
}

ts::TablesFactory::Register::Register(DisplaySectionFunction func, TID id, Standards standards, uint16_t minCAS, uint16_t maxCAS, std::initializer_list<PID> pids)
{
    TablesFactory::Instance()->registerTable(id, standards, minCAS, maxCAS, pids)->display = func;
}

ts::TablesFactory::Register::Register(DisplaySectionFunction func, TID minId, TID maxId, Standards standards, uint16_t minCAS, uint16_t maxCAS, std::initializer_list<PID> pids)
{
    for (TID id = minId; id <= maxId; ++id) {
        TablesFactory::Instance()->registerTable(id, standards, minCAS, maxCAS, pids)->display = func;
    }
}

ts::TablesFactory::Register::Register(LogSectionFunction func, TID id, Standards standards, uint16_t minCAS, uint16_t maxCAS, std::initializer_list<PID> pids)
{
    TablesFactory::Instance()->registerTable(id, standards, minCAS, maxCAS, pids)->log = func;
}

ts::TablesFactory::Register::Register(LogSectionFunction func, TID minId, TID maxId, Standards standards, uint16_t minCAS, uint16_t maxCAS, std::initializer_list<PID> pids)
{
    for (TID id = minId; id <= maxId; ++id) {
        TablesFactory::Instance()->registerTable(id, standards, minCAS, maxCAS, pids)->log = func;
    }
}

ts::TablesFactory::Register::Register(DisplayDescriptorFunction func, const EDID& edid)
{
    TablesFactory::Instance()->_descriptorDisplays.insert(std::make_pair(edid, func));
}

ts::TablesFactory::Register::Register(DisplayCADescriptorFunction func, uint16_t minCAS, uint16_t maxCAS)
{
    do {
        TablesFactory::Instance()->_casIdDescriptorDisplays.insert(std::make_pair(minCAS, func));
    } while (minCAS++ < maxCAS);
}

ts::TablesFactory::RegisterXML::RegisterXML(const UString& filename)
{
    TablesFactory::Instance()->_xmlModelFiles.push_back(filename);
}

ts::TablesFactory::RegisterNames::RegisterNames(const UString& filename)
{
    TablesFactory::Instance()->_namesFiles.push_back(filename);
}


//----------------------------------------------------------------------------
// Get registered items.
//----------------------------------------------------------------------------

ts::TablesFactory::TableFactory ts::TablesFactory::getTableFactory(TID id, Standards standards, PID pid, uint16_t cas) const
{
    return getTableFunction(id, standards, pid, cas, &TableDescription::factory);
}

ts::DisplaySectionFunction ts::TablesFactory::getSectionDisplay(TID id, Standards standards, PID pid, uint16_t cas) const
{
    return getTableFunction(id, standards, pid, cas, &TableDescription::display);
}

ts::LogSectionFunction ts::TablesFactory::getSectionLog(TID id, Standards standards, PID pid, uint16_t cas) const
{
    return getTableFunction(id, standards, pid, cas, &TableDescription::log);
}

ts::TablesFactory::TableFactory ts::TablesFactory::getTableFactory(const UString& node_name) const
{
    const auto it = node_name.findSimilar(_tableNames);
    return it != _tableNames.end() ? it->second : nullptr;
}

ts::TablesFactory::DescriptorFactory ts::TablesFactory::getDescriptorFactory(const UString& node_name) const
{
    const auto it = node_name.findSimilar(_descriptorNames);
    return it != _descriptorNames.end() ? it->second : nullptr;
}

ts::DisplayCADescriptorFunction ts::TablesFactory::getCADescriptorDisplay(uint16_t cas_id) const
{
    const auto it = _casIdDescriptorDisplays.find(cas_id);
    return it != _casIdDescriptorDisplays.end() ? it->second : nullptr;
}


//----------------------------------------------------------------------------
// Get the list of standards which are defined for a given table id.
//----------------------------------------------------------------------------

ts::Standards ts::TablesFactory::getTableStandards(TID tid, PID pid) const
{
    // Accumulate the common subset of all standards for this table id.
    Standards standards = STD_NONE;
    for (auto it = _tables.lower_bound(tid); it != _tables.end() && it->first == tid; ++it) {
        if (it->second.hasPID(pid)) {
            // We are in a standard PID for this table id, return the corresponding standards only.
            return it->second.standards;
        }
        else if (standards == STD_NONE) {
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

bool ts::TablesFactory::isDescriptorAllowed(const UString& desc_node_name, TID table_id) const
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

ts::UString ts::TablesFactory::descriptorTables(const DuckContext& duck, const UString& desc_node_name) const
{
    auto it = desc_node_name.findSimilar(_descriptorTablesIds);
    UString result;

    while (it != _descriptorTablesIds.end() && desc_node_name.similar(it->first)) {
        if (!result.empty()) {
            result.append(u", ");
        }
        result.append(names::TID(duck, it->second, CASID_NULL, names::NAME | names::HEXA));
        ++it;
    }

    return result;
}


//----------------------------------------------------------------------------
// Get registered descriptors (specific table-specific feature).
//----------------------------------------------------------------------------

template <typename FUNCTION>
FUNCTION ts::TablesFactory::getDescriptorFunction(const EDID& edid, TID tid, const std::map<EDID,FUNCTION>& funcMap) const
{
    typename std::map<EDID, FUNCTION>::const_iterator it(funcMap.end());
    if (edid.isStandard() && tid != TID_NULL) {
        // For standard descriptors, first search a table-specific descriptor.
        it = funcMap.find(EDID::TableSpecific(edid.did(), tid));
        // If not found and there is a table-specific name for the descriptor,
        // do not fallback to non-table-specific function for this descriptor.
        if (it == funcMap.end() && (edid.isTableSpecific() || names::HasTableSpecificName(edid.did(), tid))) {
            return nullptr;
        }
    }
    if (it == funcMap.end()) {
        // If non-standard or no table-specific descriptor found, use direct lookup.
        it = funcMap.find(edid);
    }
    return it != funcMap.end() ? it->second : nullptr;
}

ts::TablesFactory::DescriptorFactory ts::TablesFactory::getDescriptorFactory(const EDID& edid, TID tid) const
{
    return getDescriptorFunction(edid, tid, _descriptorIds);
}

ts::DisplayDescriptorFunction ts::TablesFactory::getDescriptorDisplay(const EDID& edid, TID tid) const
{
    return getDescriptorFunction(edid, tid, _descriptorDisplays);
}


//----------------------------------------------------------------------------
// Get all registered items of a given type.
//----------------------------------------------------------------------------

void ts::TablesFactory::getRegisteredTableIds(std::vector<TID>& ids) const
{
    ids.clear();
    TID previous = TID_NULL;
    for (auto it = _tables.begin(); it != _tables.end(); ++it) {
        // This is a multimap, the same key can be reused, use it once only.
        if (it->first != previous) {
            ids.push_back(it->first);
            previous = it->first;
        }
    }
}

void ts::TablesFactory::getRegisteredDescriptorIds(std::vector<EDID>& ids) const
{
    ids.clear();
    for (auto it = _descriptorIds.begin(); it != _descriptorIds.end(); ++it) {
        ids.push_back(it->first);
    }
}

void ts::TablesFactory::getRegisteredTableNames(UStringList& names) const
{
    names.clear();
    for (auto it = _tableNames.begin(); it != _tableNames.end(); ++it) {
        names.push_back(it->first);
    }
}

void ts::TablesFactory::getRegisteredDescriptorNames(UStringList& names) const
{
    names.clear();
    for (auto it = _descriptorNames.begin(); it != _descriptorNames.end(); ++it) {
        names.push_back(it->first);
    }
}

void ts::TablesFactory::getRegisteredTablesModels(UStringList& names) const
{
    names = _xmlModelFiles;
}

void ts::TablesFactory::getRegisteredNamesFiles(UStringList &names) const
{
    names = _namesFiles;
}
