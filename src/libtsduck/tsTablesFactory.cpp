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
//  Tables and descriptor factory.
//
//----------------------------------------------------------------------------

#include "tsTablesFactory.h"
TSDUCK_SOURCE;

tsDefineSingleton(ts::TablesFactory);


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TablesFactory::TablesFactory() :
    _tableIds(),
    _descriptorIds(),
    _tableNames(),
    _descriptorNames(),
    _sectionDisplays(),
    _descriptorDisplays()
{
}


//----------------------------------------------------------------------------
// Registrations.
//----------------------------------------------------------------------------

ts::TablesFactory::Register::Register(TID id, TableFactory factory)
{
    TablesFactory::Instance()->_tableIds.insert(std::pair<TID,TableFactory>(id, factory));
}

ts::TablesFactory::Register::Register(TID minId, TID maxId, TableFactory factory)
{
    for (TID id = minId; id <= maxId; ++id) {
        TablesFactory::Instance()->_tableIds.insert(std::pair<TID, TableFactory>(id, factory));
    }
}

ts::TablesFactory::Register::Register(const EDID& id, DescriptorFactory factory)
{
    TablesFactory::Instance()->_descriptorIds.insert(std::pair<EDID,DescriptorFactory>(id, factory));
}

ts::TablesFactory::Register::Register(const std::string& node_name, TableFactory factory)
{
    TablesFactory::Instance()->_tableNames.insert(std::pair<std::string,TableFactory>(node_name, factory));
}

ts::TablesFactory::Register::Register(const std::string& node_name, DescriptorFactory factory)
{
    TablesFactory::Instance()->_descriptorNames.insert(std::pair<std::string,DescriptorFactory>(node_name, factory));
}

ts::TablesFactory::Register::Register(TID id, DisplaySectionFunction func)
{
    TablesFactory::Instance()->_sectionDisplays.insert(std::pair<TID,DisplaySectionFunction>(id, func));
}

ts::TablesFactory::Register::Register(TID minId, TID maxId, DisplaySectionFunction func)
{
    for (TID id = minId; id <= maxId; ++id) {
        TablesFactory::Instance()->_sectionDisplays.insert(std::pair<TID, DisplaySectionFunction>(id, func));
    }
}

ts::TablesFactory::Register::Register(const EDID& edid, DisplayDescriptorFunction func)
{
    TablesFactory::Instance()->_descriptorDisplays.insert(std::pair<EDID,DisplayDescriptorFunction>(edid, func));
}


//----------------------------------------------------------------------------
// Get registered items.
//----------------------------------------------------------------------------

ts::TablesFactory::TableFactory ts::TablesFactory::getTableFactory(TID id) const
{
    std::map<TID,TableFactory>::const_iterator it = _tableIds.find(id);
    return it != _tableIds.end() ? it->second : 0;
}

ts::TablesFactory::DescriptorFactory ts::TablesFactory::getDescriptorFactory(const EDID& id) const
{
    std::map<EDID, DescriptorFactory>::const_iterator it = _descriptorIds.find(id);
    return it != _descriptorIds.end() ? it->second : 0;
}

ts::TablesFactory::TableFactory ts::TablesFactory::getTableFactory(const std::string& node_name) const
{
    std::map<std::string,TableFactory>::const_iterator it = _tableNames.find(node_name);
    return it != _tableNames.end() ? it->second : 0;
}

ts::TablesFactory::DescriptorFactory ts::TablesFactory::getDescriptorFactory(const std::string& node_name) const
{
    std::map<std::string,DescriptorFactory>::const_iterator it = _descriptorNames.find(node_name);
    return it != _descriptorNames.end() ? it->second : 0;
}

ts::TablesFactory::DisplaySectionFunction ts::TablesFactory::getSectionDisplay(TID id) const
{
    std::map<TID,DisplaySectionFunction>::const_iterator it = _sectionDisplays.find(id);
    return it != _sectionDisplays.end() ? it->second : 0;
}

ts::TablesFactory::DisplayDescriptorFunction ts::TablesFactory::getDescriptorDisplay(const EDID& edid) const
{
    std::map<EDID,DisplayDescriptorFunction>::const_iterator it = _descriptorDisplays.find(edid);
    return it != _descriptorDisplays.end() ? it->second : 0;
}

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

void ts::TablesFactory::getRegisteredTableNames(StringList& names) const
{
    names.clear();
    for (std::map<std::string,TableFactory>::const_iterator it = _tableNames.begin(); it != _tableNames.end(); ++it) {
        names.push_back(it->first);
    }
}

void ts::TablesFactory::getRegisteredDescriptorNames(StringList& names) const
{
    names.clear();
    for (std::map<std::string,DescriptorFactory>::const_iterator it = _descriptorNames.begin(); it != _descriptorNames.end(); ++it) {
        names.push_back(it->first);
    }
}
