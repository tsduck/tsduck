//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTablesLoggerFilterRepository.h"

TS_DEFINE_SINGLETON(ts::TablesLoggerFilterRepository);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TablesLoggerFilterRepository::TablesLoggerFilterRepository()
{
}


//----------------------------------------------------------------------------
// This inner class constructor registers a section filter factory.
//----------------------------------------------------------------------------

ts::TablesLoggerFilterRepository::Register::Register(FilterFactory factory)
{
    if (factory != nullptr) {
        TablesLoggerFilterRepository::Instance()._factories.push_back(factory);
    }
}


//----------------------------------------------------------------------------
// Create an instance of all registered section filters.
//----------------------------------------------------------------------------

void ts::TablesLoggerFilterRepository::createFilters(TablesLoggerFilterVector& filters) const
{
    // Pre-reserve memory.
    filters.clear();
    filters.reserve(_factories.size());

    // Allocate an instance of each registered class.
    // Encapsulate all allocated object into a safe pointer.
    for (size_t i = 0; i < _factories.size(); ++i) {
        if (_factories[i] != nullptr) {
            TablesLoggerFilterPtr ptr(_factories[i]());
            if (!ptr.isNull()) {
                filters.push_back(ptr);
            }
        }
    }
}
