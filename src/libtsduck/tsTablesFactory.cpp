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
//  Tables and descriptor factory.
//
//----------------------------------------------------------------------------

#include "tsTablesFactory.h"
#include "tsNames.h"
TSDUCK_SOURCE;

TS_DEFINE_SINGLETON(ts::TablesFactory);


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TablesFactory::TablesFactory() :
    _tableIds(),
    _descriptorIds(),
    _tableNames(),
    _descriptorNames(),
    _descriptorTablesIds(),
    _sectionDisplays(),
    _descriptorDisplays()
{
}


//----------------------------------------------------------------------------
// Registrations.
//----------------------------------------------------------------------------

ts::TablesFactory::Register::Register(TID id, TableFactory factory)
{
    TablesFactory::Instance()->_tableIds.insert(std::make_pair(id, factory));
}

ts::TablesFactory::Register::Register(TID minId, TID maxId, TableFactory factory)
{
    for (TID id = minId; id <= maxId; ++id) {
        TablesFactory::Instance()->_tableIds.insert(std::make_pair(id, factory));
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

ts::TablesFactory::Register::Register(TID id, DisplaySectionFunction func)
{
    TablesFactory::Instance()->_sectionDisplays.insert(std::make_pair(id, func));
}

ts::TablesFactory::Register::Register(TID minId, TID maxId, DisplaySectionFunction func)
{
    for (TID id = minId; id <= maxId; ++id) {
        TablesFactory::Instance()->_sectionDisplays.insert(std::make_pair(id, func));
    }
}

ts::TablesFactory::Register::Register(const EDID& edid, DisplayDescriptorFunction func)
{
    TablesFactory::Instance()->_descriptorDisplays.insert(std::make_pair(edid, func));
}


//----------------------------------------------------------------------------
// Get registered items.
//----------------------------------------------------------------------------

ts::TablesFactory::TableFactory ts::TablesFactory::getTableFactory(TID id) const
{
    std::map<TID,TableFactory>::const_iterator it = _tableIds.find(id);
    return it != _tableIds.end() ? it->second : nullptr;
}

ts::TablesFactory::TableFactory ts::TablesFactory::getTableFactory(const UString& node_name) const
{
    std::map<UString,TableFactory>::const_iterator it = node_name.findSimilar(_tableNames);
    return it != _tableNames.end() ? it->second : nullptr;
}

ts::TablesFactory::DescriptorFactory ts::TablesFactory::getDescriptorFactory(const UString& node_name) const
{
    std::map<UString,DescriptorFactory>::const_iterator it = node_name.findSimilar(_descriptorNames);
    return it != _descriptorNames.end() ? it->second : nullptr;
}

ts::DisplaySectionFunction ts::TablesFactory::getSectionDisplay(TID id) const
{
    std::map<TID,DisplaySectionFunction>::const_iterator it = _sectionDisplays.find(id);
    return it != _sectionDisplays.end() ? it->second : nullptr;
}


//----------------------------------------------------------------------------
// Check if a descriptor is allowed in a table.
//----------------------------------------------------------------------------

bool ts::TablesFactory::isDescriptorAllowed(const UString& desc_node_name, TID table_id) const
{
    std::multimap<UString, TID>::const_iterator it = desc_node_name.findSimilar(_descriptorTablesIds);
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

ts::UString ts::TablesFactory::descriptorTables(const UString& desc_node_name) const
{
    std::multimap<UString, TID>::const_iterator it = desc_node_name.findSimilar(_descriptorTablesIds);
    UString result;

    while (it != _descriptorTablesIds.end() && desc_node_name.similar(it->first)) {
        if (!result.empty()) {
            result.append(u", ");
        }
        result.append(names::TID(it->second, CAS_OTHER, names::NAME | names::HEXA));
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
    for (std::map<TID,TableFactory>::const_iterator it = _tableIds.begin(); it != _tableIds.end(); ++it) {
        ids.push_back(it->first);
    }
}

void ts::TablesFactory::getRegisteredDescriptorIds(std::vector<EDID>& ids) const
{
    ids.clear();
    for (std::map<EDID,DescriptorFactory>::const_iterator it = _descriptorIds.begin(); it != _descriptorIds.end(); ++it) {
        ids.push_back(it->first);
    }
}

void ts::TablesFactory::getRegisteredTableNames(UStringList& names) const
{
    names.clear();
    for (std::map<UString,TableFactory>::const_iterator it = _tableNames.begin(); it != _tableNames.end(); ++it) {
        names.push_back(it->first);
    }
}

void ts::TablesFactory::getRegisteredDescriptorNames(UStringList& names) const
{
    names.clear();
    for (std::map<UString,DescriptorFactory>::const_iterator it = _descriptorNames.begin(); it != _descriptorNames.end(); ++it) {
        names.push_back(it->first);
    }
}
